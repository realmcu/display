/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *                        Header Files
 *============================================================================*/
/*
 * PLATFORM INDEPENDENT CORE.
 *
 * The only translation unit of this driver that includes neither a vendor
 * Control Software header nor an OS header. It talks to the hardware exclusively
 * through \ref vcdj_platform_ops, which keeps the bounded-wait state machine
 * below testable and portable.
 */
#include <stdint.h>
#include <stddef.h>

#include "rtl_vcdj_internal.h"

/*============================================================================*
 *                        Local Definitions
 *============================================================================*/
/*
 * Decoder interrupt / enable register. Taken from the vendor register
 * description tables (`software/source/common/8170table.h`, entries
 * HWIF_DEC_E .. HWIF_DEC_TIMEOUT), all of which live in swreg 1:
 *
 *   bit  0  DEC_E           core enable
 *   bit  2  DEC_BUS_INT_DIS
 *   bit  3  DEC_TIMEOUT_SOURCE
 *   bit  4  DEC_IRQ_DIS
 *   bit  8  DEC_IRQ         interrupt asserted
 *   bit 11  DEC_ABORT_INT
 *   bit 12  DEC_RDY_INT     decoding finished
 *   bit 13  DEC_BUS_INT     bus error
 *   bit 16  DEC_ERROR_INT   stream error detected
 *   bit 18  DEC_TIMEOUT     hardware watchdog
 *
 * This is software-side register layout knowledge from the vendor package, not
 * SoC integration data; the MMIO base itself is still a placeholder.
 */
#define VCDJ_SWREG_IRQ_OFFSET           0x04U
#define VCDJ_IRQ_BIT_DEC_E              (1U << 0)
#define VCDJ_IRQ_BIT_IRQ_DIS            (1U << 4)
#define VCDJ_IRQ_BIT_ABORT_E            (1U << 5)
#define VCDJ_IRQ_BIT_DEC_IRQ            (1U << 8)
#define VCDJ_IRQ_BIT_ABORT_INT          (1U << 11)
#define VCDJ_IRQ_BIT_RDY_INT            (1U << 12)
#define VCDJ_IRQ_BIT_BUS_INT            (1U << 13)
#define VCDJ_IRQ_BIT_ERROR_INT          (1U << 16)
#define VCDJ_IRQ_BIT_TIMEOUT            (1U << 18)

/*
 * Every terminating condition, not just success; CtrlSW classifies the status.
 * DEC_ERROR_INT is listed even though DEC_IRQ usually comes with it: relying on
 * that pairing would turn a stream error into a full timeout wait.
 */
#define VCDJ_IRQ_DONE_MASK              (VCDJ_IRQ_BIT_DEC_IRQ | \
                                         VCDJ_IRQ_BIT_ABORT_INT | \
                                         VCDJ_IRQ_BIT_RDY_INT | \
                                         VCDJ_IRQ_BIT_BUS_INT | \
                                         VCDJ_IRQ_BIT_ERROR_INT | \
                                         VCDJ_IRQ_BIT_TIMEOUT)

/** Interval between two status reads while polling. */
#define VCDJ_POLL_INTERVAL_US           50U

/** Fallback bound applied when the platform config leaves `hw_timeout_ms` at 0. */
#define VCDJ_DEFAULT_TIMEOUT_MS         2000U

/** Upper bound for an abort request to clear DEC_E. */
#define VCDJ_ABORT_TIMEOUT_MS           10U

/*============================================================================*
 *                        Local Variables
 *============================================================================*/
static const struct vcdj_platform_config *vcdj_cfg;
static uint8_t vcdj_core_up;
static uint8_t vcdj_core_busy;
static uint32_t vcdj_wait_failures;

/*
 * Allocator injection.
 *
 * The driver never calls libc malloc() on its own: the integrator injects the
 * allocator, so the same driver can be fed gui_malloc/gui_free, an os_mem
 * wrapper, or a dedicated DMA pool once the hardware contract tells us
 * which memory region the decoder core can actually reach.
 */
static void *(*vcdj_alloc_fn)(size_t);
static void (*vcdj_free_fn)(void *);

/*============================================================================*
 *                        Local Functions
 *============================================================================*/
static uint32_t vcdj_clamp_timeout(uint32_t timeout_ms)
{
    uint32_t bound = VCDJ_DEFAULT_TIMEOUT_MS;

    if ((vcdj_cfg != NULL) && (vcdj_cfg->hw_timeout_ms != 0U))
    {
        bound = vcdj_cfg->hw_timeout_ms;
    }

    /* VCDJ_TIMEOUT_FOREVER reaches us because the vendor code passes
     * (u32)DEC_X170_TIMEOUT_LENGTH, which defaults to -1. An unbounded wait is
     * exactly what must not happen here, so it is clamped like any other
     * oversized request. */
    if ((timeout_ms == 0U) || (timeout_ms > bound))
    {
        return bound;
    }

    return timeout_ms;
}

static uint32_t vcdj_core_reg_offset(int core_id, uint32_t offset)
{
    uint32_t core_base = 0U;

    if ((vcdj_cfg != NULL) && (core_id > 0))
    {
        core_base = (uint32_t)core_id * vcdj_cfg->reg_window_size;
    }

    return core_base + offset;
}

/*
 * Stop a core whose wait expired with DEC_E still set; left running, every
 * later decode times out too. Same request as the vendor ResetAsic() abort.
 *
 * \return 0 when the core is stopped, -1 when DEC_E outlived the abort.
 */
static int vcdj_core_abort(int core_id)
{
    uint32_t start;

    if ((vcdj_core_read_reg(core_id, VCDJ_SWREG_IRQ_OFFSET) & VCDJ_IRQ_BIT_DEC_E) == 0U)
    {
        return 0;
    }

    vcdj_core_write_reg(core_id, VCDJ_SWREG_IRQ_OFFSET,
                        VCDJ_IRQ_BIT_ABORT_E | VCDJ_IRQ_BIT_IRQ_DIS);

    start = vcdj_cfg->ops->get_time_ms(vcdj_cfg->ctx);
    while ((vcdj_core_read_reg(core_id, VCDJ_SWREG_IRQ_OFFSET) & VCDJ_IRQ_BIT_DEC_E) != 0U)
    {
        if ((vcdj_cfg->ops->get_time_ms(vcdj_cfg->ctx) - start) >= VCDJ_ABORT_TIMEOUT_MS)
        {
            return -1;
        }

        vcdj_cfg->ops->delay_us(vcdj_cfg->ctx, VCDJ_POLL_INTERVAL_US);
    }

    vcdj_core_write_reg(core_id, VCDJ_SWREG_IRQ_OFFSET, 0U);
    return 0;
}

/*============================================================================*
 *                        Allocator injection
 *============================================================================*/
void vcdj_mem_set_allocator(void *(*alloc_fn)(size_t), void (*free_fn)(void *))
{
    if ((alloc_fn == NULL) || (free_fn == NULL))
    {
        vcdj_alloc_fn = NULL;
        vcdj_free_fn = NULL;
        return;
    }

    vcdj_alloc_fn = alloc_fn;
    vcdj_free_fn = free_fn;
}

int vcdj_mem_allocator_ready(void)
{
    return (vcdj_alloc_fn != NULL) && (vcdj_free_fn != NULL);
}

void *vcdj_mem_alloc(size_t size)
{
    if ((vcdj_alloc_fn == NULL) || (size == 0U))
    {
        return NULL;
    }

    return vcdj_alloc_fn(size);
}

void vcdj_mem_free(void *ptr)
{
    if ((vcdj_free_fn == NULL) || (ptr == NULL))
    {
        return;
    }

    vcdj_free_fn(ptr);
}

/*============================================================================*
 *                        Core life cycle
 *============================================================================*/
int vcdj_core_init(void)
{
    const struct vcdj_platform_config *cfg;

    if (vcdj_core_up)
    {
        return 0;
    }

    cfg = vcdj_platform_get_config();
    if ((cfg == NULL) || (cfg->ops == NULL))
    {
        return -1;
    }

    if ((cfg->ops->reg_read32 == NULL) || (cfg->ops->reg_write32 == NULL) ||
        (cfg->ops->dma_alloc == NULL) || (cfg->ops->dma_free == NULL) ||
        (cfg->ops->get_time_ms == NULL) || (cfg->ops->delay_us == NULL))
    {
        return -1;
    }

    vcdj_cfg = cfg;

    if (cfg->ops->clock_enable != NULL)
    {
        if (cfg->ops->clock_enable(cfg->ctx) != 0)
        {
            vcdj_cfg = NULL;
            return -1;
        }
    }

    if (cfg->ops->reset != NULL)
    {
        (void)cfg->ops->reset(cfg->ctx);
    }

    vcdj_core_busy = 0U;
    vcdj_core_up = 1U;

    return 0;
}

void vcdj_core_deinit(void)
{
    const struct vcdj_platform_config *cfg = vcdj_cfg;

    if (!vcdj_core_up)
    {
        return;
    }

    if ((cfg != NULL) && (cfg->ops != NULL) && (cfg->ops->clock_disable != NULL))
    {
        cfg->ops->clock_disable(cfg->ctx);
    }

    vcdj_core_up = 0U;
    vcdj_core_busy = 0U;
    vcdj_cfg = NULL;
}

int vcdj_core_ready(void)
{
    return (int)vcdj_core_up;
}

const struct vcdj_platform_config *vcdj_core_config(void)
{
    return vcdj_cfg;
}

/*============================================================================*
 *                        Register access
 *============================================================================*/
uint32_t vcdj_core_read_reg(int core_id, uint32_t offset)
{
    if (!vcdj_core_up)
    {
        return 0U;
    }

    return vcdj_cfg->ops->reg_read32(vcdj_cfg->ctx, vcdj_core_reg_offset(core_id, offset));
}

void vcdj_core_write_reg(int core_id, uint32_t offset, uint32_t value)
{
    if (!vcdj_core_up)
    {
        return;
    }

    vcdj_cfg->ops->reg_write32(vcdj_cfg->ctx, vcdj_core_reg_offset(core_id, offset), value);
}

/*============================================================================*
 *                        Core scheduling and completion
 *============================================================================*/
int vcdj_core_reserve(int *core_id, uint32_t timeout_ms)
{
    if ((core_id == NULL) || !vcdj_core_up)
    {
        return -1;
    }

    if (vcdj_cfg->ops->lock != NULL)
    {
        if (vcdj_cfg->ops->lock(vcdj_cfg->ctx, vcdj_clamp_timeout(timeout_ms)) != 0)
        {
            return -1;
        }
    }
    else if (vcdj_core_busy)
    {
        /* No OS mutex available and the core is taken: fail instead of spinning. */
        return -1;
    }

    vcdj_core_busy = 1U;
    *core_id = 0;

    return 0;
}

void vcdj_core_release(int core_id)
{
    (void)core_id;

    if (!vcdj_core_up)
    {
        return;
    }

    vcdj_core_busy = 0U;

    if (vcdj_cfg->ops->unlock != NULL)
    {
        vcdj_cfg->ops->unlock(vcdj_cfg->ctx);
    }
}

void vcdj_core_enable(int core_id, uint32_t offset, uint32_t value)
{
    if (!vcdj_core_up)
    {
        return;
    }

    if (vcdj_cfg->ops->wait_reset != NULL)
    {
        vcdj_cfg->ops->wait_reset(vcdj_cfg->ctx);
    }

    if (vcdj_cfg->ops->memory_barrier != NULL)
    {
        vcdj_cfg->ops->memory_barrier(vcdj_cfg->ctx);
    }

    vcdj_core_write_reg(core_id, offset, value);
}

void vcdj_core_disable(int core_id, uint32_t offset, uint32_t value)
{
    /* The PNG timeout path passes back a swreg1 snapshot with DEC_E still set;
     * written as-is it would restart the core that was just aborted. */
    if (offset == VCDJ_SWREG_IRQ_OFFSET)
    {
        value &= ~VCDJ_IRQ_BIT_DEC_E;
    }

    vcdj_core_write_reg(core_id, offset, value);
}

static int vcdj_core_wait(int core_id, uint32_t timeout_ms)
{
    uint32_t bound;
    uint32_t start;
    uint32_t irq_stat;

    if (!vcdj_core_up)
    {
        return VCDJ_CORE_WAIT_ERROR;
    }

    /* There is no register mapping yet. Waiting here would only burn two seconds
     * before returning the same result, and a busy wait can also trip the board's
     * watchdog. Preserve the declared hardware boundary explicitly. */
    if (vcdj_cfg->reg_base == 0U)
    {
        return VCDJ_CORE_WAIT_TIMEOUT;
    }

    bound = vcdj_clamp_timeout(timeout_ms);

#if defined(VCDJ_USE_IRQ)
    if (vcdj_cfg->ops->wait_done != NULL)
    {
        int ret = vcdj_cfg->ops->wait_done(vcdj_cfg->ctx, bound);

        if (ret == 0)
        {
            return VCDJ_CORE_WAIT_OK;
        }

        if (ret != VCDJ_PLAT_ENOTSUP)
        {
            /* Genuine expiry. One more status read first: a completion that
             * raced the semaphore must not be reported as a timeout. */
            irq_stat = vcdj_core_read_reg(core_id, VCDJ_SWREG_IRQ_OFFSET);
            if ((irq_stat & VCDJ_IRQ_DONE_MASK) != 0U)
            {
                return VCDJ_CORE_WAIT_OK;
            }

            return (vcdj_core_abort(core_id) == 0) ? VCDJ_CORE_WAIT_TIMEOUT
                   : VCDJ_CORE_WAIT_ERROR;
        }

        /* VCDJ_PLAT_ENOTSUP: this build has no interrupt wired, so
         * fall through to the polling path below rather than failing. */
    }
#endif

    /* Polling path: bounded by wall clock, never by an iteration count. */
    start = vcdj_cfg->ops->get_time_ms(vcdj_cfg->ctx);

    for (;;)
    {
        irq_stat = vcdj_core_read_reg(core_id, VCDJ_SWREG_IRQ_OFFSET);

        if ((irq_stat & VCDJ_IRQ_DONE_MASK) != 0U)
        {
            return VCDJ_CORE_WAIT_OK;
        }

        if ((vcdj_cfg->ops->get_time_ms(vcdj_cfg->ctx) - start) >= bound)
        {
            return (vcdj_core_abort(core_id) == 0) ? VCDJ_CORE_WAIT_TIMEOUT
                   : VCDJ_CORE_WAIT_ERROR;
        }

        vcdj_cfg->ops->delay_us(vcdj_cfg->ctx, VCDJ_POLL_INTERVAL_US);
    }
}

int vcdj_core_wait_done(int core_id, uint32_t timeout_ms)
{
    int ret = vcdj_core_wait(core_id, timeout_ms);

    if (ret != VCDJ_CORE_WAIT_OK)
    {
        vcdj_wait_failures++;
    }

    return ret;
}

uint32_t vcdj_core_wait_failures(void)
{
    return vcdj_wait_failures;
}

/*============================================================================*
 *                        Cache maintenance and DMA memory
 *============================================================================*/
void vcdj_core_cache_clean(void *addr, size_t size)
{
    if (vcdj_core_up && (vcdj_cfg->ops->cache_clean != NULL))
    {
        vcdj_cfg->ops->cache_clean(vcdj_cfg->ctx, addr, size);
    }
}

void vcdj_core_cache_invalidate(void *addr, size_t size)
{
    if (vcdj_core_up && (vcdj_cfg->ops->cache_invalidate != NULL))
    {
        vcdj_cfg->ops->cache_invalidate(vcdj_cfg->ctx, addr, size);
    }
}

int vcdj_core_dma_alloc(size_t size, size_t align, void **cpu_addr, uintptr_t *bus_addr)
{
    const struct vcdj_platform_config *cfg = vcdj_cfg;

    if ((cpu_addr == NULL) || (bus_addr == NULL))
    {
        return -1;
    }

    *cpu_addr = NULL;
    *bus_addr = 0U;

    if (cfg == NULL)
    {
        /* Buffers may be requested before vcdj_init(); fall back to the
         * statically known configuration so allocation still works. */
        cfg = vcdj_platform_get_config();
        if ((cfg == NULL) || (cfg->ops == NULL) || (cfg->ops->dma_alloc == NULL))
        {
            return -1;
        }
    }

    return cfg->ops->dma_alloc(cfg->ctx, size, align, cpu_addr, bus_addr);
}

void vcdj_core_dma_free(void *cpu_addr, uintptr_t bus_addr, size_t size)
{
    const struct vcdj_platform_config *cfg = vcdj_cfg;

    if (cfg == NULL)
    {
        cfg = vcdj_platform_get_config();
    }

    if ((cfg != NULL) && (cfg->ops != NULL) && (cfg->ops->dma_free != NULL))
    {
        cfg->ops->dma_free(cfg->ctx, cpu_addr, bus_addr, size);
    }
}
