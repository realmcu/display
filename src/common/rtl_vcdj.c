/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *                        Header Files
 *============================================================================*/
/*
 * THE VCDJ DRIVER: everything between the public API in rtl_vcdj.h and the
 * platform layer in src/device/<soc>/rtl_vcdj_port.c. Three parts, bottom up:
 *
 *   1. CORE  bounded-wait state machine, register access, cache/DMA and
 *            allocator injection; reaches hardware only via vcdj_platform_ops
 *   2. DWL   the vendor-facing contracts: DWL (memory, registers, core
 *            scheduling, hardware identification), the 13 dwl_os
 *            Mutex/Event/Semaphore functions and osal_usleep(). Only what the
 *            merged CtrlSW references is implemented.
 *   3. HAL   JPEG/PNG dispatch, `enum DecRet` to \ref vcdj_error_t mapping,
 *            external-buffer negotiation and picture release tokens
 *
 * NO OS header may be included here, directly or indirectly. The vendor headers
 * redefine EINVAL/EBUSY/EFAULT/ENOTTY with values that differ from newlib's, and
 * whichever header comes last wins without a warning. Platform services come
 * exclusively from rtl_vcdj_internal.h, which is plain <stdint.h>.
 *
 * All three parts see the vendor macros, including `up`, `open`, `close` and
 * `printk`: do not reuse those names for local identifiers.
 */
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "rtl_vcdj_internal.h"
#include "rtl_vcdj_ctrlsw.h"

/*============================================================================*
 *                        PART 1 - CORE
 *============================================================================*/

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

/** Timeout value meaning "wait forever"; vcdj_clamp_timeout() bounds it. */
#define VCDJ_TIMEOUT_FOREVER            0xFFFFFFFFU

/*
 * Alignment of every buffer handed to the decoder core. The vendor LINMEM_ALIGN
 * of 4096 is a Linux host convenience; the documented 128-bit minimum lets the
 * shaper miss full-length bursts when a plane lands off a 128-byte boundary.
 */
#define VCDJ_DMA_ALIGN                  128U

/*============================================================================*
 *                        Local Types
 *============================================================================*/
/** Completion status returned by vcdj_core_wait_done(). */
enum vcdj_core_wait
{
    VCDJ_CORE_WAIT_OK      = 0,   /**< Core signalled completion */
    VCDJ_CORE_WAIT_TIMEOUT = 1,   /**< Bounded wait expired */
    VCDJ_CORE_WAIT_ERROR   = -1,  /**< Platform layer failure */
};

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

/* Register write trace, see vcdj_core_trace_start(). */
static uint8_t vcdj_trace_on;
static uint32_t vcdj_trace_crc;
static uint32_t vcdj_trace_count;

/* CRC-32 (IEEE, reflected 0xEDB88320), one entry per nibble. */
static const uint32_t vcdj_crc32_nibble[16] =
{
    0x00000000U, 0x1DB71064U, 0x3B6E20C8U, 0x26D930ACU,
    0x76DC4190U, 0x6B6B51F4U, 0x4DB26158U, 0x5005713CU,
    0xEDB88320U, 0xF00F9344U, 0xD6D6A3E8U, 0xCB61B38CU,
    0x9B64C2B0U, 0x86D3D2D4U, 0xA00AE278U, 0xBDBDF21CU,
};

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
 * Fields that differ between two runs of the same decode, cleared before the CRC.
 * swreg327[15:0] is HWIF_UNIQUE_ID, which CtrlSW fills with rand() per picture.
 */
static uint32_t vcdj_trace_mask(uint32_t offset, uint32_t value)
{
    if (offset == 327U * 4U)
    {
        value &= 0xFFFF0000U;
    }

    return value;
}

static uint32_t vcdj_crc32_word(uint32_t crc, uint32_t word)
{
    uint32_t i;

    for (i = 0U; i < 8U; i++)
    {
        crc = (crc >> 4) ^ vcdj_crc32_nibble[(crc ^ word) & 0xFU];
        word >>= 4;
    }

    return crc;
}

/*============================================================================*
 *                        Allocator injection
 *============================================================================*/
/*
 * Passing NULL for either argument restores the "no allocator" state, after
 * which allocation fails with VCDJ_ERR_NO_MEM instead of falling back to libc.
 */
static void vcdj_mem_set_allocator(void *(*alloc_fn)(size_t), void (*free_fn)(void *))
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

static int vcdj_mem_allocator_ready(void)
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
/* Bind the platform ops, enable the clock and release reset. */
static int vcdj_core_init(void)
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

static void vcdj_core_deinit(void)
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

/* Active platform configuration, or NULL before vcdj_core_init(). */
static const struct vcdj_platform_config *vcdj_core_config(void)
{
    return vcdj_cfg;
}

/*============================================================================*
 *                        Register access
 *============================================================================*/
static uint32_t vcdj_core_read_reg(int core_id, uint32_t offset)
{
    if (!vcdj_core_up)
    {
        return 0U;
    }

    return vcdj_cfg->ops->reg_read32(vcdj_cfg->ctx, vcdj_core_reg_offset(core_id, offset));
}

static void vcdj_core_write_reg(int core_id, uint32_t offset, uint32_t value)
{
    if (!vcdj_core_up)
    {
        return;
    }

    offset = vcdj_core_reg_offset(core_id, offset);
    if (vcdj_trace_on)
    {
        vcdj_trace_crc = vcdj_crc32_word(vcdj_crc32_word(vcdj_trace_crc, offset),
                                         vcdj_trace_mask(offset, value));
        vcdj_trace_count++;
    }

    vcdj_cfg->ops->reg_write32(vcdj_cfg->ctx, offset, value);
}

void vcdj_core_trace_start(void)
{
    vcdj_trace_crc = 0xFFFFFFFFU;
    vcdj_trace_count = 0U;
    vcdj_trace_on = 1U;
}

void vcdj_core_trace_stop(uint32_t *crc, uint32_t *count)
{
    vcdj_trace_on = 0U;
    *crc = vcdj_trace_crc ^ 0xFFFFFFFFU;
    *count = vcdj_trace_count;
}

/*============================================================================*
 *                        Core scheduling and completion
 *============================================================================*/
/*
 * Take exclusive ownership of the core. VCDJ_TIMEOUT_FOREVER is clamped to the
 * platform's `hw_timeout_ms`. \return 0 on success, negative on failure.
 */
static int vcdj_core_reserve(int *core_id, uint32_t timeout_ms)
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

static void vcdj_core_release(int core_id)
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

/* Arm the completion object, then start the core by writing \p value. */
static void vcdj_core_enable(int core_id, uint32_t offset, uint32_t value)
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

static void vcdj_core_disable(int core_id, uint32_t offset, uint32_t value)
{
    /* The PNG timeout path passes back a swreg1 snapshot with DEC_E still set;
     * written as-is it would restart the core that was just aborted. */
    if (offset == VCDJ_SWREG_IRQ_OFFSET)
    {
        value &= ~VCDJ_IRQ_BIT_DEC_E;
    }

    vcdj_core_write_reg(core_id, offset, value);
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

/*
 * Wait for the core to finish. Never spins forever: \p timeout_ms is clamped to
 * the platform's `hw_timeout_ms`, and a core still running at expiry is aborted
 * so the next job starts idle. \return one of enum vcdj_core_wait;
 * VCDJ_CORE_WAIT_ERROR when the core could not be stopped after the timeout.
 */
static int vcdj_core_wait_done(int core_id, uint32_t timeout_ms)
{
    int ret = vcdj_core_wait(core_id, timeout_ms);

    if (ret != VCDJ_CORE_WAIT_OK)
    {
        vcdj_wait_failures++;
    }

    return ret;
}

/*
 * Waits that did not end in completion since vcdj_core_init(); wraps. Lets the
 * HAL see a timeout the vendor code swallowed, see vcdj_vendor_decode_checked().
 */
static uint32_t vcdj_core_wait_failures(void)
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

static void vcdj_core_cache_invalidate(void *addr, size_t size)
{
    if (vcdj_core_up && (vcdj_cfg->ops->cache_invalidate != NULL))
    {
        vcdj_cfg->ops->cache_invalidate(vcdj_cfg->ctx, addr, size);
    }
}

static int vcdj_core_dma_alloc(size_t size, size_t align, void **cpu_addr, uintptr_t *bus_addr)
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

static void vcdj_core_dma_free(void *cpu_addr, uintptr_t bus_addr, size_t size)
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

/*============================================================================*
 *                        PART 2 - DWL
 *============================================================================*/

/*============================================================================*
 *                        Local Definitions
 *============================================================================*/
/** ASIC ID register: swreg 0. */
#define VCDJ_REG_ASIC_ID_OFFSET         0x00U

/**
 * Hardware build ID register: swreg 309.
 *
 * Same index the vendor kernel driver reads
 * (`software/linux/dwl/dwl_linux_hw.c`: `dev->hw_build_id[i] = *(reg_mem_vir + 309)`).
 */
#define VCDJ_REG_HW_BUILD_ID_OFFSET     (309U * 4U)

/** Maximum number of cores this first version tracks. */
#define VCDJ_MAX_CORES                  1

/**
 * Shadow register file depth, in 32-bit words.
 *
 * Like the vendor DWL, DWLWriteReg()/DWLReadReg() only touch this shadow;
 * DWLEnableHw() pushes it in one sweep and DWLWaitHwReady() pulls it back.
 * Writing registers one by one would set DEC_E (swreg1) before the rest of the
 * configuration lands. Pushes and pulls are bounded by reg_window_size.
 */
#define VCDJ_SHADOW_REG_COUNT           896U

/*
 * pthread_cond_wait() blocks forever. With NON_PTHREAD_H the vendor code also
 * turns pthread_create() into a no-op, so nothing else is running to signal the
 * condition variable, and a genuine forever-wait would simply hang the caller.
 *
 * Every vendor call site re-checks its predicate in a `while` loop, so returning
 * from the wait early is safe: it degrades into a slow poll instead of a deadlock.
 * That is strictly preferable to hanging, and it keeps the "no unbounded wait
 * anywhere in this driver" property.
 */
#ifndef VCDJ_COND_WAIT_TIMEOUT_MS
#define VCDJ_COND_WAIT_TIMEOUT_MS   1000U
#endif

/** Vendor convention (pthread): 0 means success, non-zero means failure. */
#define VCDJ_OS_OK      0
#define VCDJ_OS_FAIL    (-1)

/*============================================================================*
 *                        Local Types
 *============================================================================*/
struct vcdj_dwl_inst
{
    u32 in_use;
    enum DWLClientType client_type;
    void *priv;
    u32 n_cores;
    i32 reserved_core;

    /** Shadow register file, see VCDJ_SHADOW_REG_COUNT. */
    u32 shadow_regs[VCDJ_MAX_CORES][VCDJ_SHADOW_REG_COUNT];
};

/*============================================================================*
 *                        Local Variables
 *============================================================================*/
/*
 * One static instance. This SoC has a single decoder core and limited RAM
 * headroom, so a dynamically grown instance table would buy nothing.
 */
static struct vcdj_dwl_inst vcdj_dwl_instance;

/*============================================================================*
 *                        Local Functions
 *============================================================================*/
static struct vcdj_dwl_inst *vcdj_dwl_of(const void *instance)
{
    struct vcdj_dwl_inst *inst = (struct vcdj_dwl_inst *)instance;

    if ((inst == NULL) || (inst->in_use == 0U))
    {
        return NULL;
    }

    return inst;
}

/**
 * \brief Shadow slot for one register, or NULL if the access is out of range.
 *
 * \param inst    DWL instance.
 * \param core_id Core index.
 * \param offset  Byte offset of the register inside the core window.
 */
static u32 *vcdj_shadow_slot(struct vcdj_dwl_inst *inst, i32 core_id, u32 offset)
{
    u32 index = offset / 4U;

    if ((core_id < 0) || ((u32)core_id >= inst->n_cores) ||
        (index >= VCDJ_SHADOW_REG_COUNT))
    {
        return NULL;
    }

    return &inst->shadow_regs[core_id][index];
}

/**
 * \brief Number of registers the hardware window actually decodes.
 *
 * Hard limit for every push and pull: accessing past the decoded range hangs
 * the bus until the FPGA bitfile is reloaded.
 */
static u32 vcdj_shadow_hw_count(void)
{
    const struct vcdj_platform_config *cfg = vcdj_core_config();
    u32 count;

    if (cfg == NULL)
    {
        return 0U;
    }

    count = cfg->reg_window_size / 4U;

    return (count > VCDJ_SHADOW_REG_COUNT) ? VCDJ_SHADOW_REG_COUNT : count;
}

/**
 * \brief Push the shadow register file to the hardware.
 *
 * swreg1 carries DEC_E and is written last, after a barrier.
 */
static void vcdj_shadow_push(struct vcdj_dwl_inst *inst, i32 core_id)
{
    const u32 *regs;
    u32 count = vcdj_shadow_hw_count();
    u32 i;

    if ((count == 0U) || (vcdj_shadow_slot(inst, core_id, 0U) == NULL))
    {
        return;
    }

    regs = inst->shadow_regs[core_id];

    for (i = 0U; i < count; i++)
    {
        if (i != 1U)
        {
            vcdj_core_write_reg(core_id, i * 4U, regs[i]);
        }
    }

    if (count > 1U)
    {
        vcdj_core_enable(core_id, 1U * 4U, regs[1]);
    }
}

/**
 * \brief Pull every hardware register into the shadow file.
 *
 * Called once when the wait ends. CtrlSW reads status and counters from this
 * snapshot; its error path clears swreg1, so a later live read returns zero.
 */
static void vcdj_shadow_pull(struct vcdj_dwl_inst *inst, i32 core_id)
{
    u32 *regs;
    u32 count = vcdj_shadow_hw_count();
    u32 i;

    if ((count == 0U) || (vcdj_shadow_slot(inst, core_id, 0U) == NULL))
    {
        return;
    }

    regs = inst->shadow_regs[core_id];

    for (i = 0U; i < count; i++)
    {
        regs[i] = vcdj_core_read_reg(core_id, i * 4U);
    }
}

/*============================================================================*
 *                        DWL: instance life cycle
 *============================================================================*/
const void *DWLInit(struct DWLInitParam *param)
{
    struct vcdj_dwl_inst *inst = &vcdj_dwl_instance;

    if (param == NULL)
    {
        return NULL;
    }

    if (inst->in_use != 0U)
    {
        /* Single instance only in this version; the multi-instance decision is
         * still open. */
        return NULL;
    }

    if (vcdj_core_init() != 0)
    {
        return NULL;
    }

    memset(inst, 0, sizeof(*inst));
    inst->in_use = 1U;
    inst->client_type = param->client_type;
    inst->priv = param->priv;
    inst->n_cores = VCDJ_MAX_CORES;
    inst->reserved_core = -1;

    return (const void *)inst;
}

enum DWLRet DWLRelease(const void *instance)
{
    struct vcdj_dwl_inst *inst = vcdj_dwl_of(instance);

    if (inst == NULL)
    {
        return DWL_ERROR;
    }

    if (inst->reserved_core >= 0)
    {
        vcdj_core_release(inst->reserved_core);
        inst->reserved_core = -1;
    }

    memset(inst, 0, sizeof(*inst));
    vcdj_core_deinit();

    return DWL_OK;
}

/*============================================================================*
 *                        DWL: plain heap helpers
 *============================================================================*/
void *DWLcalloc(size_t n, size_t s)
{
    size_t total = n * s;
    void *p;

    if ((n != 0U) && ((total / n) != s))
    {
        return NULL;    /* multiplication overflow */
    }

    p = vcdj_mem_alloc(total);
    if (p != NULL)
    {
        memset(p, 0, total);
    }

    return p;
}

void DWLfree(void *p)
{
    vcdj_mem_free(p);
}

void *DWLmemcpy(void *d, const void *s, size_t n)
{
    return memcpy(d, s, n);
}

void *DWLmemset(void *d, i32 c, size_t n)
{
    return memset(d, (int)c, n);
}

/*============================================================================*
 *                        DWL: linear (DMA capable) memory
 *============================================================================*/
enum DWLRet DWLMallocLinear(const void *instance, u64 size, struct DWLLinearMem *info)
{
    void *cpu_addr = NULL;
    uintptr_t bus_addr = 0U;
    size_t alloc_size;

    (void)instance;

    if ((info == NULL) || (size == 0U))
    {
        return DWL_ERROR;
    }

    /* size is u64 in the vendor API but this SoC is a 32-bit target. */
    if (size > (u64)(size_t) - 1)
    {
        return DWL_ERROR;
    }

    alloc_size = (size_t)size;

    if (vcdj_core_dma_alloc(alloc_size, VCDJ_DMA_ALIGN, &cpu_addr, &bus_addr) != 0)
    {
        return DWL_ERROR;
    }

    info->virtual_address = (u32 *)cpu_addr;
    info->bus_address = (addr_t)bus_addr;
    info->alloc_virtual_addr = (u32 *)cpu_addr;
    info->alloc_bus_addr = (addr_t)bus_addr;
    info->size = size;
    info->logical_size = size;
    info->id = 0U;
    info->priv = NULL;

    return DWL_OK;
}

void DWLFreeLinear(const void *instance, struct DWLLinearMem *info)
{
    (void)instance;

    if ((info == NULL) || (info->virtual_address == NULL))
    {
        return;
    }

    vcdj_core_dma_free((void *)info->virtual_address,
                       (uintptr_t)info->bus_address,
                       (size_t)info->size);

    info->virtual_address = NULL;
    info->bus_address = 0U;
    info->alloc_virtual_addr = NULL;
    info->alloc_bus_addr = 0U;
    info->size = 0U;
    info->logical_size = 0U;
}

enum DWLRet DWLMallocRefFrm(const void *instance, u64 size, struct DWLLinearMem *info)
{
    return DWLMallocLinear(instance, size, info);
}

void DWLFreeRefFrm(const void *instance, struct DWLLinearMem *info)
{
    DWLFreeLinear(instance, info);
}

void *DWLLinearMemset(const void *instance, struct DWLLinearMem *mem, i32 offset, i32 c, size_t n)
{
    u8 *dst;

    (void)instance;

    if ((mem == NULL) || (mem->virtual_address == NULL) || (n == 0U))
    {
        return NULL;
    }

    if (((u64)offset + (u64)n) > mem->size)
    {
        return NULL;
    }

    dst = (u8 *)mem->virtual_address + offset;
    (void)memset(dst, (int)c, n);
    vcdj_core_cache_clean(dst, n);

    return dst;
}

/*============================================================================*
 *                        DWL: register access
 *============================================================================*/
/* These accessors serve the shadow, see VCDJ_SHADOW_REG_COUNT. */
void DWLWriteReg(const void *instance, i32 core_id, u32 offset, u32 value)
{
    struct vcdj_dwl_inst *inst = vcdj_dwl_of(instance);
    u32 *slot;

    if (inst == NULL)
    {
        return;
    }

    slot = vcdj_shadow_slot(inst, core_id, offset);
    if (slot != NULL)
    {
        *slot = value;
    }
}

u32 DWLReadReg(const void *instance, i32 core_id, u32 offset)
{
    struct vcdj_dwl_inst *inst = vcdj_dwl_of(instance);
    const u32 *slot;

    if (inst == NULL)
    {
        return 0U;
    }

    slot = vcdj_shadow_slot(inst, core_id, offset);

    return (slot != NULL) ? *slot : 0U;
}

void DWLReadRegs(const void *instance, i32 core_id, u32 offset, u32 *dest, u32 num)
{
    struct vcdj_dwl_inst *inst = vcdj_dwl_of(instance);
    u32 i;

    if ((inst == NULL) || (dest == NULL))
    {
        return;
    }

    for (i = 0U; i < num; i++)
    {
        const u32 *slot = vcdj_shadow_slot(inst, core_id, offset + (i * 4U));

        if (slot == NULL)
        {
            return;
        }

        dest[i] = *slot;
    }
}

/* Live hardware read; the shadow is kept in step. */
u32 DWLReadRegFromHw(const void *instance, i32 core_id, u32 offset)
{
    struct vcdj_dwl_inst *inst = vcdj_dwl_of(instance);
    u32 *slot;
    u32 value;

    if (inst == NULL)
    {
        return 0U;
    }

    value = vcdj_core_read_reg(core_id, offset);

    slot = vcdj_shadow_slot(inst, core_id, offset);
    if (slot != NULL)
    {
        *slot = value;
    }

    return value;
}

/*============================================================================*
 *                        DWL: core scheduling and completion
 *============================================================================*/
enum DWLRet DWLReserveHw(const void *instance, struct DWLReqInfo *info, i32 *core_id)
{
    struct vcdj_dwl_inst *inst = vcdj_dwl_of(instance);
    int reserved = -1;

    (void)info;

    if ((inst == NULL) || (core_id == NULL))
    {
        return DWL_ERROR;
    }

    if (vcdj_core_reserve(&reserved, VCDJ_TIMEOUT_FOREVER) != 0)
    {
        return DWL_ERROR;
    }

    inst->reserved_core = reserved;
    *core_id = (i32)reserved;

    return DWL_OK;
}

void DWLEnableHw(const void *instance, i32 core_id, u32 offset, u32 value)
{
    struct vcdj_dwl_inst *inst = vcdj_dwl_of(instance);

    if (inst == NULL)
    {
        return;
    }

    DWLWriteReg(instance, core_id, offset, value);

    /* Per-job cache/shaper setup, at the same point as the vendor Linux DWL. */
    SwSetCacheShaperReg(inst->shadow_regs[core_id], 0);

    vcdj_shadow_push(inst, core_id);
}

void DWLDisableHw(const void *instance, i32 core_id, u32 offset, u32 value)
{
    struct vcdj_dwl_inst *inst = vcdj_dwl_of(instance);

    if (inst == NULL)
    {
        return;
    }

    DWLWriteReg(instance, core_id, offset, value);
    vcdj_core_disable(core_id, offset, value);
}

enum DWLRet DWLWaitHwReady(const void *instance, i32 core_id, u32 timeout)
{
    int ret;

    if (vcdj_dwl_of(instance) == NULL)
    {
        return DWL_HW_WAIT_ERROR;
    }

    /*
     * The vendor call sites pass (u32)DEC_X170_TIMEOUT_LENGTH, whose default is
     * -1 == 0xFFFFFFFF, i.e. "wait forever". vcdj_core_wait_done() clamps that
     * to the platform bound on purpose: an unbounded poll like the one in the
     * existing Realtek JPU driver would hang the caller when the core is absent
     * or wedged.
     */
    ret = vcdj_core_wait_done(core_id, timeout);

    vcdj_shadow_pull(vcdj_dwl_of(instance), core_id);

    switch (ret)
    {
    case VCDJ_CORE_WAIT_OK:
        return DWL_HW_WAIT_OK;
    case VCDJ_CORE_WAIT_TIMEOUT:
        return DWL_HW_WAIT_TIMEOUT;
    default:
        return DWL_HW_WAIT_ERROR;
    }
}

enum DWLRet DWLReleaseHw(const void *instance, i32 core_id)
{
    struct vcdj_dwl_inst *inst = vcdj_dwl_of(instance);

    if (inst == NULL)
    {
        return DWL_ERROR;
    }

    vcdj_core_release(core_id);
    inst->reserved_core = -1;

    return DWL_OK;
}

/*============================================================================*
 *                        DWL: hardware identification
 *============================================================================*/
u32 DWLReadAsicID(const void *instance, enum DWLClientType client_type)
{
    (void)client_type;

    if (vcdj_dwl_of(instance) == NULL)
    {
        return 0U;
    }

    return vcdj_core_read_reg(0, VCDJ_REG_ASIC_ID_OFFSET);
}

u32 DWLReadCoreHwBuildID(const void *instance, u32 core_id)
{
    u32 id;

    if (vcdj_dwl_of(instance) == NULL)
    {
        return 0U;
    }

    id = vcdj_core_read_reg((i32)core_id, VCDJ_REG_HW_BUILD_ID_OFFSET);

        /*
     * Never substitute a default here. The vendor feature lookup below masks
     * this value, so a fabricated ID would silently select a feature profile
     * the silicon was not measured against; an unmapped or dead core has to
     * reach the caller as 0 / all ones instead.
         */
    return id;
}

const void *DWLGetHwFeaturesByID(const void *instance, u32 core_id)
{
    const struct DecHwFeatures *hw_feature = NULL;

    GetReleaseHwFeaturesByID(DWLReadCoreHwBuildID(instance, core_id), &hw_feature);

    return (const void *)hw_feature;
}

const void *DWLGetHwFeaturesByClientType(const void *instance,
                                         enum DWLClientType client_type,
                                         u32 *core_mask)
{
    (void)client_type;

    if (core_mask != NULL)
    {
        *core_mask = 1U;    /* single core */
    }

    return DWLGetHwFeaturesByID(instance, 0U);
}

u32 DWLReadAsicCoreCount(const void *instance)
{
    struct vcdj_dwl_inst *inst = vcdj_dwl_of(instance);

    if (inst == NULL)
    {
        return 0U;
    }

    return inst->n_cores;
}

/*============================================================================*
 *                        Checksum buffer queue
 *============================================================================*/
/*
 * SwGetCrcMem() / SwGetCrcLast() / SwSetCrcUnused() / SwFreeCrcMem() are DECLARED in software/source/common/sw_util.h, which is part
 * of the compiled subset, but IMPLEMENTED in the DWL layer
 * (software/linux/dwl/dwl_pc.c, dwl_linux_hw.c). They are therefore part of the
 * porting surface even though their names do not start with DWL, and
 * jpegdecapi.c / pnghwd_asic.c reference them unconditionally.
 *
 * Behaviour mirrors the vendor reference implementation: a singly linked list of
 * CrcData nodes with `used` and `last` flags, grown on demand.
 */
static void vcdj_crc_set_used(CrcData *crc)
{
    if (crc != NULL)
    {
        crc->used = 1U;
    }
}

static void vcdj_crc_set_last(CrcData **crc, CrcData **cur)
{
    CrcData *tmp = *crc;

    while (tmp != NULL)
    {
        tmp->last = 0U;
        tmp = tmp->next;
    }

    (*cur)->last = 1U;
}

void SwSetCrcUnused(CrcData *crc)
{
    if (crc != NULL)
    {
        crc->used = 0U;
    }
}

void SwGetCrcMem(CrcData **crc, CrcData **cur)
{
    if ((crc == NULL) || (cur == NULL))
    {
        return;
    }

    *cur = *crc;

    if (*cur == NULL)
    {
        *cur = *crc = (CrcData *)DWLcalloc(sizeof(CrcData), 1U);
    }
    else
    {
        while (*cur != NULL)
        {
            if ((*cur)->used == 0U)
            {
                break;
            }
            *cur = (*cur)->next;
        }

        if (*cur == NULL)
        {
            *cur = (CrcData *)DWLcalloc(sizeof(CrcData), 1U);
            if ((*cur != NULL) && ((*crc)->next != NULL))
            {
                (*cur)->next = (*crc)->next;
            }
            (*crc)->next = *cur;
        }
    }

    if (*cur != NULL)
    {
        vcdj_crc_set_used(*cur);
        vcdj_crc_set_last(crc, cur);
    }
}

void SwGetCrcLast(CrcData **crc, CrcData **cur)
{
    CrcData *tmp;

    if ((crc == NULL) || (cur == NULL))
    {
        return;
    }

    tmp = *crc;

    while (tmp != NULL)
    {
        if (tmp->last != 0U)
        {
            *cur = tmp;
            break;
        }
        tmp = tmp->next;
    }
}

void SwFreeCrcMem(CrcData **crc)
{
    CrcData *tmp;

    if (crc == NULL)
    {
        return;
    }

    tmp = *crc;

    while (tmp != NULL)
    {
        *crc = tmp->next;
        DWLfree(tmp);
        tmp = *crc;
    }
}

/*============================================================================*
 *                        dwl_os: Mutex
 *============================================================================*/
/*
 * The 13 `dwl_os` functions that `dwlthread.h` declares when NON_PTHREAD_H is
 * defined. That header redirects all 139 `pthread_*` and 41 `sem_*` uses in the
 * Control Software onto these functions, so no vendor source file has to be
 * touched.
 *
 * The actual OS objects live in src/device/<soc>/rtl_vcdj_port.c and are reached
 * through the plain-<stdint.h> `vcdj_os_*()` contract.
 */
int MutexInit(PMutex m, PAttribute a)
{
    vcdj_os_handle_t handle;

    (void)a;

    if (m == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    if (vcdj_os_mutex_create(&handle) != 0)
    {
        m->locked = (dwl_bool)VCDJ_OS_HANDLE_INVALID;
        return VCDJ_OS_FAIL;
    }

    /* `locked` is the only storage the vendor Mutex offers; it carries the
     * bridge handle, not a boolean. Nothing in the compiled vendor subset reads
     * the field directly. */
    m->locked = (dwl_bool)handle;
    m->attrib.a = 0;

    return VCDJ_OS_OK;
}

int MutexDestroy(PMutex m)
{
    if (m == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    if (vcdj_os_mutex_destroy((vcdj_os_handle_t)m->locked) != 0)
    {
        return VCDJ_OS_FAIL;
    }

    m->locked = (dwl_bool)VCDJ_OS_HANDLE_INVALID;

    return VCDJ_OS_OK;
}

int MutexLock(PMutex m)
{
    if (m == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    return (vcdj_os_mutex_lock((vcdj_os_handle_t)m->locked, VCDJ_OS_WAIT_FOREVER) == 0)
           ? VCDJ_OS_OK : VCDJ_OS_FAIL;
}

int MutexUnlock(PMutex m)
{
    if (m == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    return (vcdj_os_mutex_unlock((vcdj_os_handle_t)m->locked) == 0)
           ? VCDJ_OS_OK : VCDJ_OS_FAIL;
}

/*============================================================================*
 *                        dwl_os: Event (condition variable)
 *============================================================================*/
int EventInit(PEvent e, PAttribute a)
{
    vcdj_os_handle_t handle;

    (void)a;

    if (e == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    if (vcdj_os_cond_create(&handle) != 0)
    {
        e->singaled = (dwl_bool)VCDJ_OS_HANDLE_INVALID;
        return VCDJ_OS_FAIL;
    }

    e->singaled = (dwl_bool)handle;
    e->attrib.a = 0;

    return VCDJ_OS_OK;
}

int EventDestroy(PEvent e)
{
    if (e == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    if (vcdj_os_cond_destroy((vcdj_os_handle_t)e->singaled) != 0)
    {
        return VCDJ_OS_FAIL;
    }

    e->singaled = (dwl_bool)VCDJ_OS_HANDLE_INVALID;

    return VCDJ_OS_OK;
}

int EventWait(PEvent e, PMutex m)
{
    if ((e == NULL) || (m == NULL))
    {
        return VCDJ_OS_FAIL;
    }

    return (vcdj_os_cond_wait((vcdj_os_handle_t)e->singaled,
                              (vcdj_os_handle_t)m->locked,
                              VCDJ_COND_WAIT_TIMEOUT_MS) == 0)
           ? VCDJ_OS_OK : VCDJ_OS_FAIL;
}

int EventSignal(PEvent e)
{
    if (e == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    return (vcdj_os_cond_signal((vcdj_os_handle_t)e->singaled) == 0)
           ? VCDJ_OS_OK : VCDJ_OS_FAIL;
}

/*============================================================================*
 *                        dwl_os: Semaphore
 *============================================================================*/
int SemaphoreInit(PSemaphore s, u32 value)
{
    vcdj_os_handle_t handle;

    if (s == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    if (vcdj_os_sem_create(&handle, value) != 0)
    {
        s->val = (i32)VCDJ_OS_HANDLE_INVALID;
        return VCDJ_OS_FAIL;
    }

    s->val = (i32)handle;

    return VCDJ_OS_OK;
}

int SemaphoreDestroy(PSemaphore s)
{
    if (s == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    if (vcdj_os_sem_destroy((vcdj_os_handle_t)s->val) != 0)
    {
        return VCDJ_OS_FAIL;
    }

    s->val = (i32)VCDJ_OS_HANDLE_INVALID;

    return VCDJ_OS_OK;
}

int SemaphoreWait(PSemaphore s)
{
    if (s == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    /* Bounded for the same reason as EventWait(): the vendor call sites re-check
     * their predicate, so an expiry becomes a retry rather than a hang. */
    return (vcdj_os_sem_wait((vcdj_os_handle_t)s->val, VCDJ_COND_WAIT_TIMEOUT_MS) == 0)
           ? VCDJ_OS_OK : VCDJ_OS_FAIL;
}

int SemaphorePost(PSemaphore s)
{
    if (s == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    return (vcdj_os_sem_post((vcdj_os_handle_t)s->val) == 0)
           ? VCDJ_OS_OK : VCDJ_OS_FAIL;
}

int SemaphoreGetValue(PSemaphore s)
{
    int count;

    if (s == NULL)
    {
        return 0;
    }

    count = vcdj_os_sem_count((vcdj_os_handle_t)s->val);

    return (count < 0) ? 0 : count;
}

/*============================================================================*
 *                        OSAL shims
 *============================================================================*/
int osal_usleep(unsigned long usec)
{
#if ULONG_MAX > UINT32_MAX
    if (usec > UINT32_MAX)
    {
        usec = UINT32_MAX;
    }
#endif

    vcdj_os_delay_us((uint32_t)usec);

    return 0;
}

/*============================================================================*
 *                        PART 3 - HAL
 *============================================================================*/

/*============================================================================*
 *                        Local Definitions
 *============================================================================*/
/*
 * VCDJ_MAX_EXTERNAL_BUFFERS and VCDJ_MAX_BUFFER_NEGOTIATIONS are in
 * rtl_vcdj_internal.h: the former is the `capacity` argument of
 * vcdj_internal_add_jpeg_buffers(), so it is a cross-file contract rather than a
 * local choice, and the self test drives the same negotiation loop.
 */

/* 0x2070 channel-0 PPU limits; the vendor checker enforces the same bounds. */
#define VCDJ_PP_MIN_DIMENSION           48U
#define VCDJ_PP_MAX_OUTPUT_DIMENSION    4096U
#define VCDJ_PP_MAX_UPSCALE_RATIO       3U

#define VCDJ_STATIC_ASSERT(cond, tag) \
    typedef char vcdj_static_assert_##tag[(cond) ? 1 : -1]

/*
 * Public enumerations are structure fields and rely on short enums (the ARM EABI
 * default). An enumerator above 0xFF, or a build with -fno-short-enums, would
 * silently shift every later field.
 */
VCDJ_STATIC_ASSERT(sizeof(vcdj_codec_t)       == 1U, codec_enum_is_one_byte);
VCDJ_STATIC_ASSERT(sizeof(vcdj_out_format_t)  == 1U, out_format_enum_is_one_byte);
VCDJ_STATIC_ASSERT(sizeof(vcdj_pp_filter_t)   == 1U, pp_filter_enum_is_one_byte);
VCDJ_STATIC_ASSERT(sizeof(vcdj_color_type_t)  == 1U, color_type_enum_is_one_byte);
VCDJ_STATIC_ASSERT(sizeof(vcdj_chroma_t)      == 1U, chroma_enum_is_one_byte);
VCDJ_STATIC_ASSERT(sizeof(vcdj_picture_sel_t) == 1U, picture_sel_enum_is_one_byte);

/* Layout tripwires for the 32-bit target, not an ABI promise. */
VCDJ_STATIC_ASSERT(sizeof(vcdj_dec_param_t)  == 12U, dec_param_layout);
VCDJ_STATIC_ASSERT(sizeof(vcdj_pp_config_t)  == 32U, pp_config_layout);
VCDJ_STATIC_ASSERT(sizeof(vcdj_image_info_t) == 28U, image_info_layout);
VCDJ_STATIC_ASSERT(sizeof(vcdj_plane_t)      == 20U, plane_layout);
VCDJ_STATIC_ASSERT(sizeof(vcdj_picture_t)    == 76U, picture_layout);
VCDJ_STATIC_ASSERT(sizeof(vcdj_jpeg_frame_t)  == 8U, jpeg_frame_layout);
VCDJ_STATIC_ASSERT(VCDJ_MAX_PLANES == MAX_OUTPUT_PLANE, plane_count_matches_vendor);

/*============================================================================*
 *                        Local Types
 *============================================================================*/
/*
 * Vendor API structures are unusually large: JpegDecOutput alone is about
 * 13 KB. Keeping them as automatic variables would make one decode require
 * more than 18 KB of stack. Allocate one temporary workspace through the
 * caller-provided allocator instead, so the API is safe to call from normal
 * application threads and allocation failure remains reportable.
 */
struct vcdj_jpeg_work
{
    struct JpegDecConfig cfg;
    JpegDecInput dec_in;
    JpegDecImageInfo image_info;
};

struct vcdj_png_work
{
    struct PngDecConfig cfg;
    PngDecInput dec_in;
    PngDecImageInfo image_info;
};

/*
 * One live picture: the pixel memory a caller still owns, kept on a heap list so
 * the number of pictures held at once is bounded by memory, not by a table.
 *
 * Only `dec_buffer[0][0]` is kept, not the whole DWLDecBuffers shell: it is the
 * one entry vcdj_add_external_buffers() ever allocates into, and the vendor
 * InputQueue holds its own value copy of the shell, so the shell is freed once
 * the codec instance is released (see vcdj_publish_picture()).
 */
struct vcdj_live_picture
{
    struct vcdj_live_picture *next;
    struct DWLLinearMem mem;        /**< The allocation DWLFreeRefFrm() gets back */
    uint32_t id;                    /**< Release token; never 0, unique while live */
};

/*============================================================================*
 *                        Local Variables
 *============================================================================*/
static const void *vcdj_dwl_handle;
static u32 vcdj_initialised;

/* Guards the list and the id counter: release may run on another thread than decode. */
static vcdj_os_handle_t vcdj_live_lock = VCDJ_OS_HANDLE_INVALID;
static struct vcdj_live_picture *vcdj_live_head;
static uint32_t vcdj_live_last_id;

/*============================================================================*
 *                        Output buffer ownership
 *============================================================================*/
/**
 * Release every plane descriptor in one registered output buffer.
 *
 * Walks the whole channel x plane matrix rather than just `[0][0]`. Today only
 * `[0][0]` ever carries memory - with `use_separated_pp == 0` the vendor treats
 * that single descriptor as the base of every plane - but a future multi-channel
 * or separated-PP configuration would fill more entries, and a release path that
 * only ever frees the first one would then leak silently.
 * DWLFreeRefFrm() ignores a NULL descriptor, so the unused entries cost nothing.
 *
 * Undoes vcdj_add_external_buffers() on the negotiation hand-back and decode
 * cleanup paths. A published picture keeps only `[0][0]` (vcdj_publish_picture()),
 * so filling a second entry would require changing that function too.
 */
static void vcdj_free_all_planes(struct DWLDecBuffers *buffer)
{
    unsigned int ch;
    unsigned int pl;

    for (ch = 0U; ch < DEC_MAX_OUT_COUNT; ch++)
    {
        for (pl = 0U; pl < MAX_OUTPUT_PLANE; pl++)
        {
            struct DWLLinearMem *mem = &buffer->dec_buffer[ch][pl];

            if (mem->virtual_address != NULL)
            {
                DWLFreeRefFrm(vcdj_dwl_handle, mem);
            }
        }
    }
}

/*============================================================================*
 *                        Release token
 *============================================================================*/
/*
 * The token is the picture's id, not its node address: rtl_vcdj.h promises a value
 * that cannot be dereferenced. An id is never reused while its picture is live, so
 * a double release or a fabricated token finds no node and is refused instead of
 * freeing memory that now belongs to another picture. 0 stays "no pixels".
 *
 * vcdj_live_find() and vcdj_live_new_id() expect vcdj_live_lock to be held.
 */
static struct vcdj_live_picture *vcdj_live_find(uint32_t id,
                                                struct vcdj_live_picture ***link_out)
{
    struct vcdj_live_picture **link = &vcdj_live_head;

    while (*link != NULL)
    {
        if ((*link)->id == id)
        {
            if (link_out != NULL)
            {
                *link_out = link;
            }
            return *link;
        }
        link = &(*link)->next;
    }

    return NULL;
}

/** Next id that is neither 0 nor still live; the skip only matters after wrap-around. */
static uint32_t vcdj_live_new_id(void)
{
    do
    {
        vcdj_live_last_id++;
    }
    while ((vcdj_live_last_id == 0U) || (vcdj_live_find(vcdj_live_last_id, NULL) != NULL));

    return vcdj_live_last_id;
}

static void vcdj_live_free(struct vcdj_live_picture *live)
{
    DWLFreeRefFrm(vcdj_dwl_handle, &live->mem);
    vcdj_mem_free(live);
}

static void vcdj_live_lock_take(void)
{
    (void)vcdj_os_mutex_lock(vcdj_live_lock, VCDJ_OS_WAIT_FOREVER);
}

static void vcdj_live_lock_give(void)
{
    (void)vcdj_os_mutex_unlock(vcdj_live_lock);
}

/*============================================================================*
 *                        Local Functions
 *============================================================================*/
/*
 * Collapse the vendor return space into errors that are stable for this
 * synchronous one-picture API. Control-flow states are deliberately not exposed:
 * buffer negotiation is consumed by vcdj_decode(), and streaming, flush and
 * abort have no public operation that a caller could continue.
 *
 * Keep this switch explicit rather than relying on value ranges: enum DecRet is a
 * multi-codec vendor type, while the only contracts relevant here are the return
 * branches of the JpegDec and PngDec APIs. Explicit cases make a newly reachable
 * value fall into INTERNAL until its meaning for this facade has been reviewed.
 */
static vcdj_error_t vcdj_map_dec_ret(enum DecRet ret)
{
    switch (ret)
    {
    case DEC_OK:
    case DEC_PIC_RDY:
    case DEC_PIC_DECODED:
    case DEC_HDRS_RDY:
        return VCDJ_SUCCESS;

    case DEC_PARAM_ERROR:
    case DEC_INVALID_STREAM_LENGTH:
    case DEC_INVALID_INPUT_BUFFER_SIZE:
    case DEC_INFOPARAM_ERROR:
        return VCDJ_ERR_PARAM;

    case DEC_MEMFAIL:
        return VCDJ_ERR_NO_MEM;

    case DEC_STRM_ERROR:
    case DEC_ERROR:
    case DEC_INCREASE_INPUT_BUFFER:
    case DEC_END_OF_STREAM:
    case DEC_STRM_PROCESSED:
    case DEC_DISCARD_INTERNAL:
        /* A one-shot decode that ends, consumes all supplied data or discards its
         * only picture without producing output has a malformed or truncated
         * stream from the caller's point of view. */
        return VCDJ_ERR_STREAM;

    case DEC_UNSUPPORTED:
    case DEC_STREAM_NOT_SUPPORTED:
    case DEC_FORMAT_NOT_SUPPORTED:
    case DEC_SLICE_MODE_UNSUPPORTED:
        return VCDJ_ERR_UNSUPPORTED;

    case DEC_HW_TIMEOUT:
    case DEC_HW_EXT_TIMEOUT:
        return VCDJ_ERR_TIMEOUT;

    case DEC_HW_BUS_ERROR:
    case DEC_HW_RESERVED:
    case DEC_SYSTEM_ERROR:
    case DEC_DWL_ERROR:
    case DEC_FATAL_SYSTEM_ERROR:
        return VCDJ_ERR_HW;

    case DEC_NOT_INITIALIZED:
    case DEC_ABORTED:
    case DEC_NO_DECODING_BUFFER:
    case DEC_EXT_BUFFER_REJECTED:
        /* Buffer slots and abort state are owned by this helper, not by its
         * caller. Do not expose the vendor queue protocol as public states. */
        return VCDJ_ERR_STATE;

    case DEC_WAITING_FOR_BUFFER:
    case DEC_SCAN_PROCESSED:
    case DEC_INITFAIL:
    default:
        /* WAITING_FOR_BUFFER must be consumed by the bounded negotiation loop;
         * SCAN_PROCESSED is not produced by the imported implementation. */
        return VCDJ_ERR_INTERNAL;
    }
}

static vcdj_chroma_t vcdj_map_jpeg_chroma(u32 output_format)
{
    switch (output_format)
    {
    case DEC_OUT_FRM_YUV400:
        return VCDJ_CHROMA_MONO;
    case DEC_OUT_FRM_YUV411SP:
        return VCDJ_CHROMA_411;
    case DEC_OUT_FRM_YUV420SP:
        return VCDJ_CHROMA_420;
    case DEC_OUT_FRM_YUV422SP:
        return VCDJ_CHROMA_422;
    case DEC_OUT_FRM_YUV440:
        return VCDJ_CHROMA_440;
    case DEC_OUT_FRM_YUV444SP:
        return VCDJ_CHROMA_444;
    default:
        return VCDJ_CHROMA_UNKNOWN;
    }
}

/*============================================================================*
 *                        Output format table
 *============================================================================*/
/** How a public format is requested from the vendor PpUnitConfig. */
enum vcdj_fmt_kind
{
    VCDJ_KIND_NONE = 0,             /**< Hole in the table: not a public format */
    VCDJ_KIND_RGB,                  /**< `rgb = 1`, `rgb_format` */
    VCDJ_KIND_YUV,                  /**< semi-planar YUV */
    VCDJ_KIND_LUMA,                 /**< `monochrome = 1` */
    VCDJ_KIND_YUYV,                 /**< `out_yuyv = 1` */
    VCDJ_KIND_UYVY,                 /**< `out_uyvy = 1` */
};

#define VCDJ_FMT_CR_FIRST   0x01U   /**< `cr_first = 1` */
#define VCDJ_FMT_PNG_ONLY   0x02U   /**< only a PNG can produce it */
#define VCDJ_FMT_ALPHA      0x04U   /**< has an alpha byte for rgb_alpha to fill */

struct vcdj_fmt_desc
{
    uint8_t  kind;                  /**< \ref vcdj_fmt_kind */
    uint8_t  chroma;                /**< PpUnitConfig.chroma_format (PP_YUV4xx) */
    uint16_t flags;                 /**< VCDJ_FMT_* */
    uint16_t rgb_format;            /**< DEC_OUT_FRM_* request, RGB kinds only */
    uint16_t vendor;                /**< DEC_OUT_FRM_* TransUnitConfig2Format() reports */
};

VCDJ_STATIC_ASSERT(DEC_OUT_FRM_HIST <= 0xFFFFU, picture_format_fits_u16);

#define VCDJ_RGB(req, rep, fl)      { VCDJ_KIND_RGB, PP_YUV420, (fl), (req), (rep) }
#define VCDJ_YUV(ch, fl, rep)       { VCDJ_KIND_YUV, (ch), (fl), 0U, (rep) }
#define VCDJ_LUMA(fl, rep)          { VCDJ_KIND_LUMA, PP_YUV420, (fl), 0U, (rep) }
#define VCDJ_PACK(k, fl, rep)       { (k), PP_YUV422, (fl), 0U, (rep) }

#define VCDJ_CRF                    VCDJ_FMT_CR_FIRST
#define VCDJ_AL                     VCDJ_FMT_ALPHA

/*
 * Indexed by vcdj_out_format_t. The PpUnitConfig fields come from the vendor
 * testbench options (command_line_parser.c); `vendor` is what ppu.c
 * TransUnitConfig2Format() derives from them. Note the vendor naming swap:
 * requesting DEC_OUT_FRM_BGR888 is reported back as DEC_OUT_FRM_BGR888, but
 * travels through the core as PP_OUT_RGB888.
 */
static const struct vcdj_fmt_desc vcdj_fmt_table[] =
{
    [VCDJ_OUT_RGB565]       = VCDJ_RGB(DEC_OUT_FRM_RGB565, DEC_OUT_FRM_RGB565, 0U),
    [VCDJ_OUT_BGR565]       = VCDJ_RGB(DEC_OUT_FRM_BGR565, DEC_OUT_FRM_BGR565, 0U),
    [VCDJ_OUT_RGB888]       = VCDJ_RGB(DEC_OUT_FRM_RGB888, DEC_OUT_FRM_RGB888, 0U),
    [VCDJ_OUT_BGR888]       = VCDJ_RGB(DEC_OUT_FRM_BGR888, DEC_OUT_FRM_BGR888, 0U),
    [VCDJ_OUT_ARGB8888]     = VCDJ_RGB(DEC_OUT_FRM_ARGB888, DEC_OUT_FRM_ARGB888, VCDJ_AL),
    [VCDJ_OUT_ABGR8888]     = VCDJ_RGB(DEC_OUT_FRM_ABGR888, DEC_OUT_FRM_ABGR888, VCDJ_AL),
    [VCDJ_OUT_RGBA8888]     = VCDJ_RGB(DEC_OUT_FRM_RGBA888, DEC_OUT_FRM_RGBA888, VCDJ_AL),
    [VCDJ_OUT_BGRA8888]     = VCDJ_RGB(DEC_OUT_FRM_BGRA888, DEC_OUT_FRM_BGRA888, VCDJ_AL),
    [VCDJ_OUT_XRGB8888]     = VCDJ_RGB(DEC_OUT_FRM_XRGB888, DEC_OUT_FRM_XRGB888, 0U),
    [VCDJ_OUT_XBGR8888]     = VCDJ_RGB(DEC_OUT_FRM_XBGR888, DEC_OUT_FRM_XBGR888, 0U),

    [VCDJ_OUT_MONOCHROME]   = VCDJ_LUMA(0U, DEC_OUT_FRM_YUV400),
    /* PngDecSetInfo() turns a plain YUV request into luma + alpha by itself. */
    [VCDJ_OUT_YUV400_ALPHA] = VCDJ_YUV(PP_YUV420, VCDJ_FMT_PNG_ONLY,
                                       DEC_OUT_FRM_YUV400_WITH_ALPHA),
    [VCDJ_OUT_YUV420SP]     = VCDJ_YUV(PP_YUV420, 0U, DEC_OUT_FRM_YUV420SP),
    [VCDJ_OUT_YVU420SP]     = VCDJ_YUV(PP_YUV420, VCDJ_CRF, DEC_OUT_FRM_NV21SP),
    [VCDJ_OUT_YUV422SP]     = VCDJ_YUV(PP_YUV422, 0U, DEC_OUT_FRM_YUV422SP),
    [VCDJ_OUT_YVU422SP]     = VCDJ_YUV(PP_YUV422, VCDJ_CRF, DEC_OUT_FRM_422_NV21SP),

    /* The vendor reports the swapped packed orders under the unswapped name. */
    [VCDJ_OUT_YUYV]         = VCDJ_PACK(VCDJ_KIND_YUYV, 0U, DEC_OUT_FRM_YUV422_YUYV),
    [VCDJ_OUT_UYVY]         = VCDJ_PACK(VCDJ_KIND_UYVY, 0U, DEC_OUT_FRM_YUV422_UYVY),
    [VCDJ_OUT_YVYU]         = VCDJ_PACK(VCDJ_KIND_YUYV, VCDJ_CRF, DEC_OUT_FRM_YUV422_YUYV),
    [VCDJ_OUT_VYUY]         = VCDJ_PACK(VCDJ_KIND_UYVY, VCDJ_CRF, DEC_OUT_FRM_YUV422_UYVY),
};

#define VCDJ_FMT_COUNT  (sizeof(vcdj_fmt_table) / sizeof(vcdj_fmt_table[0]))

static const struct vcdj_fmt_desc *vcdj_fmt_lookup(vcdj_out_format_t fmt)
{
    if (((uint32_t)fmt >= VCDJ_FMT_COUNT) ||
        (vcdj_fmt_table[fmt].kind == VCDJ_KIND_NONE))
    {
        return NULL;
    }

    return &vcdj_fmt_table[fmt];
}

static int vcdj_out_format_supported(vcdj_out_format_t out_format)
{
    return (vcdj_fmt_lookup(out_format) != NULL) ? 1 : 0;
}

/**
 * Reject a source or format the hardware build cannot decode, before the vendor sees it.
 *
 * The 12-bit check cannot be left to the vendor: on this build it parses a
 * 12-bit JPEG, sets HWIF_JPEG_12BIT_E and starts a core that only decodes 8 bits.
 */
static vcdj_error_t vcdj_format_fits_source(vcdj_out_format_t out_format,
                                            vcdj_codec_t codec,
                                            uint32_t bit_depth)
{
    const struct vcdj_fmt_desc *fmt = vcdj_fmt_lookup(out_format);

    if (fmt == NULL)
    {
        return VCDJ_ERR_PARAM;
    }
    if ((codec == VCDJ_CODEC_JPEG) && (bit_depth > 8U))
    {
        return VCDJ_ERR_UNSUPPORTED;
    }
    if (((fmt->flags & VCDJ_FMT_PNG_ONLY) != 0U) && (codec != VCDJ_CODEC_PNG))
    {
        return VCDJ_ERR_UNSUPPORTED;
    }

    return VCDJ_SUCCESS;
}

static int vcdj_pp_filter_supported(vcdj_pp_filter_t filter)
{
    return ((filter == VCDJ_PP_FILTER_DEFAULT) ||
            (filter == VCDJ_PP_FILTER_NEAREST) ||
            (filter == VCDJ_PP_FILTER_BILINEAR) ||
            (filter == VCDJ_PP_FILTER_BICUBIC) ||
            (filter == VCDJ_PP_FILTER_AREA)) ? 1 : 0;
}

static int vcdj_csc_supported(vcdj_csc_t csc)
{
    return ((uint32_t)csc <= (uint32_t)VCDJ_CSC_BT2020_L) ? 1 : 0;
}

/**
 * Whether the format carries an alpha byte for PpUnitConfig.rgb_alpha to fill.
 *
 * Not every 32 bpp RGB format does: XRGB8888 and XBGR8888 spend the fourth byte
 * on padding. Those two are absent on purpose - nobody has looked at the
 * hardware's fourth byte yet, and guessing would either reject a working format
 * or accept a silent no-op.
 */
static int vcdj_out_format_has_alpha(vcdj_out_format_t out_format)
{
    const struct vcdj_fmt_desc *fmt = vcdj_fmt_lookup(out_format);

    return ((fmt != NULL) && ((fmt->flags & VCDJ_FMT_ALPHA) != 0U)) ? 1 : 0;
}

/** RGB kinds are the only ones the vendor lets rgb_alpha and rgb_stan reach. */
static int vcdj_out_format_is_rgb(vcdj_out_format_t out_format)
{
    const struct vcdj_fmt_desc *fmt = vcdj_fmt_lookup(out_format);

    return ((fmt != NULL) && (fmt->kind == VCDJ_KIND_RGB)) ? 1 : 0;
}

/**
 * Map the public CSC value onto the vendor pair.
 *
 * rgb_stan and video_range must agree: the vendor rewrites rgb_stan from
 * video_range when the two disagree, so setting only rgb_stan would silently
 * land on the _L variant. The low bit of rgb_stan selects limited range.
 */
static void vcdj_csc_to_vendor(vcdj_csc_t csc, u32 *rgb_stan, u32 *video_range)
{
    *rgb_stan = (u32)csc;
    *video_range = (((u32)csc & 1U) != 0U) ? 0U : 1U;
}

static vcdj_error_t vcdj_validate_pp(const vcdj_pp_config_t *pp,
                                     uint32_t input_width,
                                     uint32_t input_height)
{
    uint32_t crop_right;
    uint32_t crop_bottom;
    uint32_t source_width;
    uint32_t source_height;
    uint32_t max_output_width;
    uint32_t max_output_height;
    unsigned int i;

    if ((pp == NULL) || (input_width == 0U) || (input_height == 0U) ||
        !vcdj_out_format_supported(pp->output_format) ||
        !vcdj_pp_filter_supported(pp->filter) || !vcdj_csc_supported(pp->csc) ||
        (pp->crop_enable > 1U) || (pp->scale_enable > 1U) ||
        (pp->dither_disable > 1U))
    {
        return VCDJ_ERR_PARAM;
    }

    /* The vendor drops an alpha value it cannot use instead of failing, so the
     * caller would get an opaque format and no diagnostic. */
    if ((!vcdj_out_format_is_rgb(pp->output_format) ||
         !vcdj_out_format_has_alpha(pp->output_format)) && (pp->alpha != 0U))
    {
        return VCDJ_ERR_PARAM;
    }

    for (i = 0U; i < sizeof(pp->reserved); i++)
    {
        if (pp->reserved[i] != 0U)
        {
            return VCDJ_ERR_PARAM;
        }
    }

    if (pp->crop_enable != 0U)
    {
        if ((pp->crop.width == 0U) || (pp->crop.height == 0U) ||
            ((pp->crop.width < VCDJ_PP_MIN_DIMENSION) &&
             (pp->crop.width != input_width)) ||
            ((pp->crop.height < VCDJ_PP_MIN_DIMENSION) &&
             (pp->crop.height != input_height)) ||
            (pp->crop.x > UINT32_MAX - pp->crop.width) ||
            (pp->crop.y > UINT32_MAX - pp->crop.height))
        {
            return VCDJ_ERR_PARAM;
        }

        crop_right = pp->crop.x + pp->crop.width;
        crop_bottom = pp->crop.y + pp->crop.height;
        if ((crop_right > input_width) || (crop_bottom > input_height))
        {
            return VCDJ_ERR_PARAM;
        }

        source_width = pp->crop.width;
        source_height = pp->crop.height;
    }
    else
    {
        if ((pp->crop.x != 0U) || (pp->crop.y != 0U) ||
            (pp->crop.width != 0U) || (pp->crop.height != 0U))
        {
            return VCDJ_ERR_PARAM;
        }

        source_width = input_width;
        source_height = input_height;
    }

    if (pp->scale_enable != 0U)
    {
        max_output_width = (source_width >
                            (VCDJ_PP_MAX_OUTPUT_DIMENSION /
                             VCDJ_PP_MAX_UPSCALE_RATIO)) ?
                           VCDJ_PP_MAX_OUTPUT_DIMENSION :
                           source_width * VCDJ_PP_MAX_UPSCALE_RATIO;
        max_output_height = (source_height >
                             (VCDJ_PP_MAX_OUTPUT_DIMENSION /
                              VCDJ_PP_MAX_UPSCALE_RATIO)) ?
                            VCDJ_PP_MAX_OUTPUT_DIMENSION :
                            source_height * VCDJ_PP_MAX_UPSCALE_RATIO;

        if ((pp->output_width < VCDJ_PP_MIN_DIMENSION) ||
            (pp->output_height < VCDJ_PP_MIN_DIMENSION) ||
            (pp->output_width > max_output_width) ||
            (pp->output_height > max_output_height))
        {
            return VCDJ_ERR_PARAM;
        }

        if ((pp->filter == VCDJ_PP_FILTER_AREA) &&
            ((pp->output_width > source_width) ||
             (pp->output_height > source_height) ||
             ((pp->output_width == source_width) &&
              (pp->output_height == source_height))))
        {
            return VCDJ_ERR_UNSUPPORTED;
        }
    }
    else if ((pp->output_width != 0U) || (pp->output_height != 0U) ||
             (pp->filter != VCDJ_PP_FILTER_DEFAULT))
    {
        return VCDJ_ERR_PARAM;
    }

    return VCDJ_SUCCESS;
}

static u32 vcdj_pp_filter_to_vendor(vcdj_pp_filter_t filter)
{
    /* Cast at the trust boundary so the defensive default also covers values
     * outside the public enum without triggering -Wcovered-switch-default. */
    switch ((uint32_t)filter)
    {
    case VCDJ_PP_FILTER_NEAREST:
        return NEAREST;
    case VCDJ_PP_FILTER_BICUBIC:
        return FAST_BICUBIC;
    case VCDJ_PP_FILTER_AREA:
        return AREA;
    case VCDJ_PP_FILTER_DEFAULT:
    case VCDJ_PP_FILTER_BILINEAR:
        return FAST_LINEAR;
    default:
        return VSI_LINEAR;
    }
}

/*
 * Report what the post processing unit actually produced, by looking the vendor
 * format up in the same table the request side uses.
 *
 * This is not a mirror of the request. When YUV is requested for a PNG,
 * PngDecSetInfo() picks RGB from the colour type instead (truecolour/indexed ->
 * BGR888, RGBA or tRNS -> ABGR8888). Greyscale PNG is forced onto the luma-only
 * path whatever was requested, and so is greyscale JPEG when YUV is requested.
 *
 * Two vendor formats are ambiguous and need the request to resolve: YUYV / YVYU
 * (and UYVY / VYUY) share one vendor name and differ only in cr_first. Anything
 * the table does not name - tiled, compressed, histogram - is reported as
 * UNKNOWN rather than guessed.
 */
static vcdj_out_format_t vcdj_out_format_from_vendor(enum DecPictureFormat fmt,
                                                     vcdj_out_format_t requested)
{
    const struct vcdj_fmt_desc *req = vcdj_fmt_lookup(requested);
    unsigned int i;

    if ((req != NULL) && (req->vendor == (uint16_t)fmt))
    {
        return requested;
    }

    for (i = 0U; i < VCDJ_FMT_COUNT; i++)
    {
        if ((vcdj_fmt_table[i].kind != VCDJ_KIND_NONE) &&
            (vcdj_fmt_table[i].vendor == (uint16_t)fmt))
        {
            return (vcdj_out_format_t)i;
        }
    }

    return VCDJ_OUT_UNKNOWN;
}

/** Colour model of a JPEG, derived from the chroma format the parser reported. */
static vcdj_color_type_t vcdj_jpeg_color_type(u32 output_format)
{
    return (output_format == DEC_OUT_FRM_YUV400) ? VCDJ_COLOR_GRAY
           : VCDJ_COLOR_YCBCR;
}

/** Component count of a JPEG, derived the same way. */
static uint8_t vcdj_jpeg_channels(u32 output_format)
{
    return (output_format == DEC_OUT_FRM_YUV400) ? 1U : 3U;
}

/** Map the PNG colour type byte from the header onto the public colour model. */
static vcdj_color_type_t vcdj_png_color_type(u32 color_type)
{
    switch (color_type)
    {
    case PNG_COLOR_TYPE_GRAY:
        return VCDJ_COLOR_GRAY;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        return VCDJ_COLOR_GRAY_ALPHA;
    case PNG_COLOR_TYPE_RGB:
        return VCDJ_COLOR_RGB;
    case PNG_COLOR_TYPE_RGB_ALPHA:
        return VCDJ_COLOR_RGB_ALPHA;
    case PNG_COLOR_TYPE_INDEX:
        return VCDJ_COLOR_INDEXED;
    default:
        return VCDJ_COLOR_UNKNOWN;
    }
}

/**
 * Reject a codec value that is outside \ref vcdj_codec_t.
 *
 * The enumeration keeps the legal values discoverable, but C does not stop a
 * caller from assigning anything that fits the underlying type, so the trust
 * boundary still has to check.
 */
static int vcdj_codec_supported(vcdj_codec_t codec)
{
    return ((codec == VCDJ_CODEC_JPEG) || (codec == VCDJ_CODEC_PNG)) ? 1 : 0;
}

/* Only a JPEG carries a thumbnail the vendor can decode. */
static int vcdj_picture_sel_valid(const vcdj_dec_param_t *param)
{
    if ((param->reserved[0] != 0U) || (param->picture > VCDJ_PICTURE_THUMBNAIL))
    {
        return 0;
    }
    return ((param->picture == VCDJ_PICTURE_MAIN) || (param->codec == VCDJ_CODEC_JPEG)) ? 1 : 0;
}

/* Thumbnail requests only: whether the parse found a thumbnail the core can decode. */
static vcdj_error_t vcdj_thumbnail_present(u32 thumbnail_type)
{
    if (thumbnail_type == JPEGDEC_THUMBNAIL_JPEG)
    {
        return VCDJ_SUCCESS;
    }
    return (thumbnail_type == JPEGDEC_THUMBNAIL_NOT_SUPPORTED_FORMAT) ? VCDJ_ERR_UNSUPPORTED
                                                                     : VCDJ_ERR_PARAM;
}

static void vcdj_setup_ppu(PpUnitConfig *ppu, const vcdj_pp_config_t *config)
{
    const struct vcdj_fmt_desc *fmt;

    memset(ppu, 0, sizeof(PpUnitConfig) * DEC_MAX_OUT_COUNT);

    ppu[0].enabled = 1U;
    ppu[0].align = DEC_ALIGN_64B;
    /* Mandatory: with the shaper off the core never ends the frame. */
    ppu[0].shaper_enabled = 1U;
    /* Only takes effect when bit depth drops (e.g. RGB565); no-op otherwise. */
    ppu[0].dither_enable = (config->dither_disable != 0U) ? 0U : 1U;
    ppu[0].crop.enabled = config->crop_enable;
    ppu[0].crop.set_by_user = config->crop_enable;
    ppu[0].crop.x = config->crop.x;
    ppu[0].crop.y = config->crop.y;
    ppu[0].crop.width = config->crop.width;
    ppu[0].crop.height = config->crop.height;
    ppu[0].scale.enabled = config->scale_enable;
    ppu[0].scale.scale_by_ratio = 0U;
    ppu[0].scale.width = config->output_width;
    ppu[0].scale.height = config->output_height;
    if (config->scale_enable != 0U)
    {
        ppu[0].pp_filter = vcdj_pp_filter_to_vendor(config->filter);
        ppu[0].antialias = (config->filter == VCDJ_PP_FILTER_AREA) ? 1U : 0U;
    }

    fmt = vcdj_fmt_lookup(config->output_format);
    if (fmt == NULL)
    {
        /* Callers validate first; keep the vendor default rather than guess. */
        ppu[0].chroma_format = PP_YUV420;
        return;
    }

    ppu[0].chroma_format = fmt->chroma;
    ppu[0].cr_first = ((fmt->flags & VCDJ_FMT_CR_FIRST) != 0U) ? 1U : 0U;

    switch (fmt->kind)
    {
    case VCDJ_KIND_RGB:
        /* The decoder core emits YUV; the RGB colour space conversion lives in
         * the vendor post processing unit (ppu.c), which is why ppu.c is not an
         * optional part of the compiled subset.
         *
         * CSC and alpha belong here rather than above the switch: the vendor
         * only carries them into the pipeline for an RGB output, so setting
         * them on a YUV request would look applied without being applied. */
        ppu[0].rgb = 1U;
        ppu[0].rgb_format = fmt->rgb_format;
        ppu[0].rgb_alpha = config->alpha;
        vcdj_csc_to_vendor(config->csc, &ppu[0].rgb_stan, &ppu[0].video_range);
        break;
    case VCDJ_KIND_LUMA:
        /*
         * Explicit luma-only also drops PNG alpha: PngDecSetInfo() only picks
         * the luma + alpha layout when monochrome was NOT requested, which is
         * why VCDJ_OUT_YUV400_ALPHA is a plain YUV request in the table.
         */
        ppu[0].monochrome = 1U;
        break;
    case VCDJ_KIND_YUYV:
        ppu[0].out_yuyv = 1U;
        break;
    case VCDJ_KIND_UYVY:
        ppu[0].out_uyvy = 1U;
        break;
    default:
        break;
    }
}

static void vcdj_setup_default_pp(PpUnitConfig *ppu,
                                  vcdj_out_format_t out_format)
{
    vcdj_pp_config_t config;

    memset(&config, 0, sizeof(config));
    config.output_format = out_format;
    config.filter = VCDJ_PP_FILTER_DEFAULT;
    vcdj_setup_ppu(ppu, &config);
}

static void vcdj_release_input(const void *dwl, struct DWLLinearMem *mem)
{
    if (mem->virtual_address != NULL)
    {
        DWLFreeLinear(dwl, mem);
    }
}

static vcdj_error_t vcdj_prepare_input(const void *dwl, const vcdj_dec_param_t *param,
                                       struct DWLLinearMem *mem)
{
    if (DWLMallocLinear(dwl, param->size, mem) != DWL_OK)
    {
        return VCDJ_ERR_NO_MEM;
    }

    (void)memcpy(mem->virtual_address, param->data, param->size);
    vcdj_core_cache_clean(mem->virtual_address, param->size);

    return VCDJ_SUCCESS;
}

/*
 * Header parsing is CPU only and never writes the stream, so it reads the
 * caller's buffer in place. Never pass the result to vcdj_release_input().
 * bus_address only satisfies the vendor's non-NULL check: it is the CPU pointer,
 * not a DMA address, so a decode must use vcdj_prepare_input() instead.
 */
static void vcdj_borrow_input(const vcdj_dec_param_t *param, struct DWLLinearMem *mem)
{
    memset(mem, 0, sizeof(*mem));
    mem->virtual_address = (u32 *)(uintptr_t)param->data;
    mem->bus_address = (addr_t)(uintptr_t)param->data;
    mem->size = param->size;
    mem->logical_size = param->size;
}

/*
 * The PNG vendor path returns the core wait status as an IRQ mask, so a
 * timeout (0xFF) carries DEC_HW_IRQ_RDY and looks like a finished picture.
 * Do not reduce this to a plain passthrough.
 */
static enum DecRet vcdj_vendor_decode_checked(
    void *inst,
    enum DecRet(*decode)(void *, void *, void *),
    void *input, void *output)
{
    uint32_t failures = vcdj_core_wait_failures();
    enum DecRet ret = decode(inst, input, output);

    if (vcdj_core_wait_failures() != failures)
    {
        return DEC_HW_TIMEOUT;
    }

    return ret;
}

static enum DecRet vcdj_get_buffer_info(
    void *inst,
    enum DecRet(*get_info)(void *, struct DecBufferInfo *),
    struct DecBufferInfo *info)
{
    return get_info(inst, info);
}

/**
 * Discard the buffer the vendor asked us to hand back.
 *
 * The re-allocation protocol works by returning the OLD descriptor in
 * `DecBufferInfo.buf_to_free` while asking for a larger buffer. The vendor has
 * already dropped its own reference by then - `JpegDecGetBufferInfo()` zeroes
 * `dec_cont->buf_to_free` right after copying it out - so the memory is ours to
 * release and leaking it here is invisible until the heap runs dry.
 *
 * Matching is by address rather than by index because the descriptor the vendor
 * returns is a copy, not the object we registered.
 */
static void vcdj_release_returned_buffer(const struct DWLDecBuffers *to_free,
                                         void **buffers, unsigned int *count)
{
    const u32 *addr = to_free->dec_buffer[0][0].virtual_address;
    unsigned int i;

    if (addr == NULL)
    {
        return;     /* nothing to hand back this round */
    }

    for (i = 0U; i < *count; i++)
    {
        struct DWLDecBuffers *owned = (struct DWLDecBuffers *)buffers[i];

        if ((owned == NULL) || (owned->dec_buffer[0][0].virtual_address != addr))
        {
            continue;
        }

        vcdj_free_all_planes(owned);
        vcdj_mem_free(owned);

        /* Keep the array dense: move the last entry down into the hole. */
        (*count)--;
        buffers[i] = buffers[*count];
        buffers[*count] = NULL;
        return;
    }
}

static int vcdj_add_external_buffers(const void *dwl, void *inst,
                                     enum DecRet(*get_info)(void *, struct DecBufferInfo *),
                                     enum DecRet(*add_buffer)(void *, struct DWLLinearMem *),
                                     void **buffers, unsigned int capacity,
                                     unsigned int *count,
                                     struct vcdj_buffer_layout *layout)
{
    struct DecBufferInfo *info;
    enum DecRet ret;
    unsigned int i;
    unsigned int required;

    if ((dwl == NULL) || (inst == NULL) || (get_info == NULL) ||
        (add_buffer == NULL) || (buffers == NULL) || (count == NULL))
    {
        return (int)DEC_PARAM_ERROR;
    }

    info = vcdj_mem_alloc(sizeof(*info));
    if (info == NULL)
    {
        return (int)DEC_MEMFAIL;
    }

    memset(info, 0, sizeof(*info));
    ret = vcdj_get_buffer_info(inst, get_info, info);
    if ((ret != DEC_OK) && (ret != DEC_WAITING_FOR_BUFFER))
    {
        vcdj_mem_free(info);
        return (int)ret;
    }

    /* Do this before allocating: the round that asks for a bigger buffer is the
     * same round that hands the old one back. */
    vcdj_release_returned_buffer(&info->buf_to_free, buffers, count);

    if ((info->next_buf_size == 0U) || (info->buf_num == 0U))
    {
        /* Release-only round: the vendor wanted the old buffer back and nothing
         * more. Reporting DEC_OK lets the caller re-enter Decode(). */
        vcdj_mem_free(info);
        return (int)DEC_OK;
    }

    required = info->buf_num;
    if ((required > capacity) || ((*count + required) > capacity))
    {
        vcdj_mem_free(info);
        return (int)DEC_MEMFAIL;
    }

    /*
     * Report the geometry the vendor computed. Every value here comes from
     * `DecBufferInfo`; none of it is derived from width * height * bpp, because
     * the post processing unit aligns each line and the resulting sizes cannot be
     * reproduced from the picture dimensions (5.1 item 4 of the interface plan).
     */
    if (layout != NULL)
    {
        unsigned int pl;

        layout->total_size = (unsigned long long)info->next_buf_size;
        layout->ystride = info->ystride[0];
        layout->cstride = info->cstride[0];

        for (pl = 0U; pl < VCDJ_MAX_PLANES; pl++)
        {
            layout->plane_size[pl] = (unsigned long long)info->output_buf_size[0][pl];
        }
    }

    for (i = 0U; i < required; i++)
    {
        struct DWLDecBuffers *buffer;
        struct DWLLinearMem *mem;

        /*
         * Despite its DWLLinearMem pointer type, the vendor AddBuffer API casts
         * this argument to DWLDecBuffers and copies the complete output matrix
         * into InputQueue, so the enclosing type has to be the one we allocate.
         *
         * ONE ALLOCATION, NOT ONE PER PLANE. With `use_separated_pp == 0` - which
         * is our configuration, since misc_ctrl leaves bit 16 clear - the vendor
         * addresses every plane of every channel as an offset into
         * `dec_buffer[0][0]`: SwSetPpOutAddr() reads that one descriptor and adds
         * `ppu_cfg->luma_offset` / `chroma_offset`. Splitting the planes into
         * separate allocations would put them at unrelated addresses and the
         * chroma offset would land outside its buffer. `next_buf_size` is
         * therefore the size of the whole multi-plane block.
         */
        buffer = vcdj_mem_alloc(sizeof(*buffer));
        if (buffer == NULL)
        {
            vcdj_mem_free(info);
            return (int)DEC_MEMFAIL;
        }
        memset(buffer, 0, sizeof(*buffer));
        mem = &buffer->dec_buffer[0][0];
        mem->mem_type = DWL_MEM_TYPE_DPB | DWL_MEM_TYPE_SYNC_DEVICE_TO_HOST;
        SET_MEM_USAGE(mem->mem_type, DWL_MEM_USAGE_OUT_REFERENCE, 0U);

        if (DWLMallocRefFrm(dwl, info->next_buf_size, mem) != DWL_OK)
        {
            vcdj_mem_free(buffer);
            vcdj_mem_free(info);
            return (int)DEC_MEMFAIL;
        }

        ret = add_buffer(inst, (struct DWLLinearMem *)buffer);
        if (ret != DEC_OK)
        {
            DWLFreeRefFrm(dwl, mem);
            vcdj_mem_free(buffer);
            vcdj_mem_free(info);
            return (int)ret;
        }

        buffers[*count] = buffer;
        (*count)++;
    }

    vcdj_mem_free(info);
    return (int)DEC_OK;
}

/*============================================================================*
 *                        Picture delivery
 *============================================================================*/
/**
 * \brief Fill \ref vcdj_picture_t.planes from what the vendor actually produced.
 *
 * Shared by the JPEG and the PNG path on purpose: the three vendor descriptors
 * have identical meaning in `JpegDecOutput.pictures[]` and
 * `PngDecOutput.pictures[]`, so passing them in individually keeps one
 * implementation for both codecs without a vendor type reaching the public header.
 *
 * \param luma,chroma,chroma_cr  Plane base addresses NextPicture() reported.
 * \param layout                 Sizes and strides from the buffer negotiation.
 * \param pic                    Receives `planes[]` and `plane_count`.
 *
 * A plane counts as present only when \ref vcdj_buffer_layout.plane_size says so.
 * The addresses cannot be used for that test: for packed RGB the vendor still
 * derives a chroma address as `base + chroma_offset`, and because
 * `chroma_offset == luma_size` with a zero chroma size, that address is one past
 * the end of the buffer. Believing it would publish memory the caller has no
 * right to touch.
 */
static void vcdj_fill_planes(const struct DWLLinearMem *luma,
                             const struct DWLLinearMem *chroma,
                             const struct DWLLinearMem *chroma_cr,
                             const struct vcdj_buffer_layout *layout,
                             vcdj_picture_t *pic)
{
    const struct DWLLinearMem *src[VCDJ_MAX_PLANES];
    unsigned int i;

    src[0] = luma;
    src[1] = chroma;
    src[2] = chroma_cr;

    pic->plane_count = 0U;

    for (i = 0U; i < VCDJ_MAX_PLANES; i++)
    {
        if ((layout->plane_size[i] == 0U) || (src[i]->virtual_address == NULL))
        {
            break;      /* planes are contiguous; the first gap ends the picture */
        }

        pic->planes[i].cpu_addr = (void *)src[i]->virtual_address;
        pic->planes[i].bus_addr = (uintptr_t)src[i]->bus_address;
        pic->planes[i].size = (uint32_t)layout->plane_size[i];
        pic->planes[i].stride = (i == 0U) ? layout->ystride : layout->cstride;

        /*
         * Zero, and not the vendor's luma_offset / chroma_offset. Those offsets
         * are relative to the start of the whole multi-plane block, and
         * NextPicture() has already applied them to the addresses above, so
         * repeating them here would skip the data twice. The first visible pixel
         * of each plane sits exactly at its reported address.
         */
        pic->planes[i].offset = 0U;

        pic->plane_count++;
    }

    /*
     * Make the pixels visible to the CPU before the caller reads them: the core
     * wrote them behind our back through its own bus master.
     *
     * TODO(hardware-contract): the platform cache_invalidate is only a barrier
     * until the memory attributes and maintenance granularity are known. The
     * call site stays, so the range operation drops in here without rework.
     */
    for (i = 0U; i < pic->plane_count; i++)
    {
        vcdj_core_cache_invalidate(pic->planes[i].cpu_addr, pic->planes[i].size);
    }
}

/**
 * \brief Hand the decoded pixels to the caller and take ownership of the memory.
 *
 * Moves the pixel allocation onto the live list and mints the release token. On
 * success `buffers[0]` is no longer owned by the decode path - `*count` drops to
 * 0 - so the normal cleanup must not free it; \ref vcdj_picture_release does.
 *
 * Freeing the DWLDecBuffers shell here, while the codec instance still lives, is
 * safe: AddBuffer copied it by value into the InputQueue (input_queue.c).
 *
 * \return \ref VCDJ_SUCCESS, or \ref VCDJ_ERR_NO_MEM when the list node cannot be
 *         allocated; the pixels then stay with the decode path and are freed there.
 */
static vcdj_error_t vcdj_publish_picture(void **buffers, unsigned int *count,
                                         vcdj_picture_t *pic)
{
    struct vcdj_live_picture *live;
    struct DWLDecBuffers *shell;

    if (*count == 0U)
    {
        /* No external buffer was ever registered, so there are no pixels to give
         * away. Geometry has already been reported; leave plane_count at 0. */
        return VCDJ_SUCCESS;
    }

    live = vcdj_mem_alloc(sizeof(*live));
    if (live == NULL)
    {
        return VCDJ_ERR_NO_MEM;
    }

    /*
     * Exactly one buffer is transferred: `buf_num` is hardcoded to 1 by both
     * codecs, and with `use_separated_pp == 0` its `[0][0]` entry is the one
     * allocation that holds every plane (see vcdj_add_external_buffers()).
     */
    shell = (struct DWLDecBuffers *)buffers[0];
    live->mem = shell->dec_buffer[0][0];
    vcdj_mem_free(shell);
    buffers[0] = NULL;
    *count = 0U;

    vcdj_live_lock_take();
    live->id = vcdj_live_new_id();
    live->next = vcdj_live_head;
    vcdj_live_head = live;
    vcdj_live_lock_give();

    pic->release_token = (uintptr_t)live->id;

    return VCDJ_SUCCESS;
}

/* PNG counterpart of vcdj_internal_add_jpeg_buffers(); the self test is JPEG only. */
static int vcdj_add_png_buffers(void *inst, void **buffers,
                                unsigned int capacity,
                                unsigned int *count,
                                struct vcdj_buffer_layout *layout)
{
    return vcdj_add_external_buffers(vcdj_dwl_handle, inst,
                                     (enum DecRet(*)(void *, struct DecBufferInfo *))PngDecGetBufferInfo,
                                     (enum DecRet(*)(void *, struct DWLLinearMem *))PngDecAddBuffer,
                                     buffers, capacity, count, layout);
}

/*============================================================================*
 *                        Cross-file driver internals
 *============================================================================*/
int vcdj_internal_jpeg_decode(void *inst, void *input, void *output)
{
    return (int)vcdj_vendor_decode_checked(
               inst, (enum DecRet(*)(void *, void *, void *))JpegDecDecode,
               input, output);
}

int vcdj_internal_add_jpeg_buffers(void *inst, void **buffers,
                                   unsigned int capacity,
                                   unsigned int *count,
                                   struct vcdj_buffer_layout *layout)
{
    return vcdj_add_external_buffers(vcdj_dwl_handle, inst,
                                     (enum DecRet(*)(void *, struct DecBufferInfo *))JpegDecGetBufferInfo,
                                     (enum DecRet(*)(void *, struct DWLLinearMem *))JpegDecAddBuffer,
                                     buffers, capacity, count, layout);
}

void vcdj_internal_free_external_buffers(void **buffers, unsigned int count)
{
    unsigned int i;

    if (buffers == NULL)
    {
        return;
    }

    for (i = 0U; i < count; i++)
    {
        struct DWLDecBuffers *buffer = (struct DWLDecBuffers *)buffers[i];

        if (buffer != NULL)
        {
            vcdj_free_all_planes(buffer);
            vcdj_mem_free(buffer);
            buffers[i] = NULL;
        }
    }
}

const void *vcdj_internal_dwl(void)
{
    return vcdj_dwl_handle;
}

/*============================================================================*
 *                        JPEG path
 *============================================================================*/
static vcdj_error_t vcdj_jpeg_image_info(const vcdj_dec_param_t *param,
                                         vcdj_image_info_t *info)
{
    struct vcdj_jpeg_work *work;
    JpegDecInst inst = NULL;
    enum DecRet ret;
    vcdj_error_t err;

    work = vcdj_mem_alloc(sizeof(*work));
    if (work == NULL)
    {
        return VCDJ_ERR_NO_MEM;
    }
    memset(work, 0, sizeof(*work));

    work->cfg.decoder_mode = DEC_NORMAL;
    work->cfg.align = DEC_ALIGN_64B;
    work->cfg.error_handling = DEC_EC_FRAME_NO_ERROR;
    work->cfg.dec_image_type = JPEGDEC_IMAGE;
    vcdj_setup_default_pp(work->cfg.ppu_config, param->out_format);

    ret = JpegDecInit(&inst, vcdj_dwl_handle, &work->cfg);
    if (ret != DEC_OK)
    {
        err = vcdj_map_dec_ret(ret);
        goto out_work;
    }

    vcdj_borrow_input(param, &work->dec_in.stream_buffer);

    work->dec_in.stream_length = param->size;
    work->dec_in.buffer_size = 0U;
    work->dec_in.dec_image_type = JPEGDEC_IMAGE;
    work->dec_in.stream = (u8 *)work->dec_in.stream_buffer.virtual_address;

    /* thumbnail_done stays 0 for both pictures: with 1 the parser skips the APPn
     * segment and thumbnail_type always reads "none". The price: with a thumbnail
     * present, bit_depth is the thumbnail's even for the main picture. */
    ret = JpegDecGetImageInfo(inst, &work->dec_in, &work->image_info);
    err = vcdj_map_dec_ret(ret);

    if ((err == VCDJ_SUCCESS) && (param->picture == VCDJ_PICTURE_THUMBNAIL))
    {
        const JpegDecImageInfo *ii = &work->image_info;

        err = vcdj_thumbnail_present(ii->thumbnail_type);
        if (err == VCDJ_SUCCESS)
        {
            info->width = ii->display_width_thumb;
            info->height = ii->display_height_thumb;
            info->aligned_width = ii->output_width_thumb;
            info->aligned_height = ii->output_height_thumb;
            info->color_type = vcdj_jpeg_color_type(ii->output_format_thumb);
            info->channels = vcdj_jpeg_channels(ii->output_format_thumb);
            info->jpeg_progressive = (ii->coding_mode_thumb == JPEG_PROGRESSIVE) ? 1U : 0U;
            info->jpeg_chroma = vcdj_map_jpeg_chroma(ii->output_format_thumb);
        }
    }
    else if (err == VCDJ_SUCCESS)
    {
        const JpegDecImageInfo *ii = &work->image_info;

        info->width = ii->display_width;
        info->height = ii->display_height;
        info->aligned_width = ii->output_width;
        info->aligned_height = ii->output_height;
        info->color_type = vcdj_jpeg_color_type(ii->output_format);
        info->channels = vcdj_jpeg_channels(ii->output_format);
        info->jpeg_progressive = (ii->coding_mode == JPEG_PROGRESSIVE) ? 1U : 0U;
        info->jpeg_chroma = vcdj_map_jpeg_chroma(ii->output_format);
    }

    if (err == VCDJ_SUCCESS)
    {
        info->bit_depth = work->image_info.bit_depth;
        info->codec = VCDJ_CODEC_JPEG;
        info->has_alpha = 0U;    /* JPEG has no alpha channel */
        /* No vendor thumbnail_type is 0: NO_THUMBNAIL is 0x12, so a non-zero test is always true. */
        info->jpeg_has_thumbnail =
            (work->image_info.thumbnail_type == JPEGDEC_THUMBNAIL_JPEG) ? 1U : 0U;
        info->png_interlaced = 0U;
    }

    JpegDecRelease(inst);
out_work:
    vcdj_mem_free(work);

    return err;
}

static vcdj_error_t vcdj_jpeg_decode(const vcdj_dec_param_t *param,
                                     const vcdj_pp_config_t *pp,
                                     vcdj_picture_t *pic)
{
    struct vcdj_jpeg_work *work;
    JpegDecOutput *dec_out = NULL;
    JpegDecInst inst = NULL;
    enum DecRet ret;
    vcdj_error_t err;
    void *external_buffers[VCDJ_MAX_EXTERNAL_BUFFERS] = {0};
    unsigned int external_count = 0U;
    unsigned int negotiations = 0U;
    struct vcdj_buffer_layout layout;
    const int thumb = (param->picture == VCDJ_PICTURE_THUMBNAIL) ? 1 : 0;
    const u32 image_type = thumb ? JPEGDEC_THUMBNAIL : JPEGDEC_IMAGE;
    u32 src_w, src_h;

    memset(&layout, 0, sizeof(layout));

    work = vcdj_mem_alloc(sizeof(*work));
    if (work == NULL)
    {
        return VCDJ_ERR_NO_MEM;
    }
    memset(work, 0, sizeof(*work));

    work->cfg.decoder_mode = DEC_NORMAL;
    work->cfg.align = DEC_ALIGN_64B;
    work->cfg.error_handling = DEC_EC_FRAME_NO_ERROR;
    work->cfg.dec_image_type = image_type;

    ret = JpegDecInit(&inst, vcdj_dwl_handle, &work->cfg);
    if (ret != DEC_OK)
    {
        err = vcdj_map_dec_ret(ret);
        goto out_work;
    }

    err = vcdj_prepare_input(vcdj_dwl_handle, param, &work->dec_in.stream_buffer);
    if (err != VCDJ_SUCCESS)
    {
        goto out_inst;
    }

    work->dec_in.stream_length = param->size;
    work->dec_in.dec_image_type = image_type;
    /* thumbnail_done selects which picture the parser leaves the HW geometry on:
     * 0 switches it to the EXIF/JFXX thumbnail whatever dec_image_type says, so it
     * must be 1 for the main picture and 0 for the thumbnail. */
    work->dec_in.thumbnail_done = thumb ? 0U : 1U;
    work->dec_in.stream = (u8 *)work->dec_in.stream_buffer.virtual_address;

    ret = JpegDecGetImageInfo(inst, &work->dec_in, &work->image_info);
    if (ret != DEC_OK)
    {
        err = vcdj_map_dec_ret(ret);
        goto out_input;
    }

    if (thumb)
    {
        err = vcdj_thumbnail_present(work->image_info.thumbnail_type);
        if (err != VCDJ_SUCCESS)
        {
            goto out_input;
        }
        src_w = work->image_info.display_width_thumb;
        src_h = work->image_info.display_height_thumb;
    }
    else
    {
        src_w = work->image_info.display_width;
        src_h = work->image_info.display_height;
    }

    err = vcdj_validate_pp(pp, src_w, src_h);
    if (err == VCDJ_SUCCESS)
    {
        err = vcdj_format_fits_source(pp->output_format, VCDJ_CODEC_JPEG,
                                      work->image_info.bit_depth);
    }
    if (err != VCDJ_SUCCESS)
    {
        goto out_input;
    }

    vcdj_setup_ppu(work->cfg.ppu_config, pp);
    work->cfg.chroma_format = thumb ? work->image_info.output_format_thumb
                                    : work->image_info.output_format;
    ret = JpegDecSetInfo(inst, &work->cfg);
    if (ret != DEC_OK)
    {
        err = vcdj_map_dec_ret(ret);
        goto out_input;
    }

    dec_out = vcdj_mem_alloc(sizeof(*dec_out));
    if (dec_out == NULL)
    {
        err = VCDJ_ERR_NO_MEM;
        goto out_input;
    }
    memset(dec_out, 0, sizeof(*dec_out));

    ret = vcdj_vendor_decode_checked(
              inst, (enum DecRet(*)(void *, void *, void *))JpegDecDecode,
              &work->dec_in, dec_out);
    while ((ret == DEC_WAITING_FOR_BUFFER) &&
           (negotiations < VCDJ_MAX_BUFFER_NEGOTIATIONS))
    {
        ret = (enum DecRet)vcdj_internal_add_jpeg_buffers(inst, external_buffers,
                                                          VCDJ_MAX_EXTERNAL_BUFFERS,
                                                          &external_count, &layout);
        if (ret != DEC_OK)
        {
            err = vcdj_map_dec_ret(ret);
            goto out_input;
        }
        negotiations++;
        ret = vcdj_vendor_decode_checked(
                  inst, (enum DecRet(*)(void *, void *, void *))JpegDecDecode,
                  &work->dec_in, dec_out);
    }
    if ((ret != DEC_PIC_RDY) && (ret != DEC_PIC_DECODED) && (ret != DEC_OK))
    {
        err = vcdj_map_dec_ret(ret);
        goto out_input;
    }

    ret = JpegDecNextPicture(inst, dec_out, &work->image_info);
    if ((ret != DEC_PIC_RDY) && (ret != DEC_OK))
    {
        err = vcdj_map_dec_ret(ret);
        goto out_input;
    }

    /*
     * Report the geometry and the format the post processing unit really chose,
     * taken from the picture the decoder produced rather than from what was
     * requested.
     */
    /* A thumbnail lands in the *_thumb fields; display_width stays 0 for it. */
    pic->width = thumb ? dec_out->pictures[0].display_width_thumb
                       : dec_out->pictures[0].display_width;
    pic->height = thumb ? dec_out->pictures[0].display_height_thumb
                        : dec_out->pictures[0].display_height;
    pic->format = vcdj_out_format_from_vendor(dec_out->pictures[0].output_format,
                                              pp->output_format);

    if ((pic->width == 0U) || (pic->height == 0U))
    {
        /* NextPicture() succeeded but reported nothing usable. Fall back to the
         * header geometry so the caller still gets a meaningful size. */
        pic->width = src_w;
        pic->height = src_h;
    }

    /*
     * Prefer the strides NextPicture() reported over the ones the negotiation
     * predicted: post processing can settle its alignment while configuring the
     * hardware, and this picture is the authority on what was written.
     */
    if (dec_out->pictures[0].pic_stride != 0U)
    {
        layout.ystride = dec_out->pictures[0].pic_stride;
    }
    if (dec_out->pictures[0].pic_stride_ch != 0U)
    {
        layout.cstride = dec_out->pictures[0].pic_stride_ch;
    }

    vcdj_fill_planes(&dec_out->pictures[0].output_picture_y,
                     &dec_out->pictures[0].output_picture_cb_cr,
                     &dec_out->pictures[0].output_picture_cr,
                     &layout, pic);

    /*
     * Give the queue slot back BEFORE releasing the instance, which is the order
     * the vendor protocol requires - PictureConsumed() reaches into the
     * InputQueue that JpegDecRelease() destroys.
     *
     * This says "the slot is reusable", not "the pixels are free": the memory is
     * ours (we allocated it with DWLMallocRefFrm) and the vendor never frees it,
     * so ownership can move to the caller in vcdj_publish_picture() below while
     * the vendor bookkeeping is wound down here.
     */
    (void)JpegDecPictureConsumed(inst, dec_out);

    err = vcdj_publish_picture(external_buffers, &external_count, pic);
    if (err != VCDJ_SUCCESS)
    {
        /* No list node: the pixels stay in external_buffers and are freed below. */
        pic->plane_count = 0U;
        memset(pic->planes, 0, sizeof(pic->planes));
    }

out_input:
    vcdj_release_input(vcdj_dwl_handle, &work->dec_in.stream_buffer);
out_inst:
    JpegDecRelease(inst);
    vcdj_internal_free_external_buffers(external_buffers, external_count);
out_work:
    vcdj_mem_free(dec_out);
    vcdj_mem_free(work);

    return err;
}

/*============================================================================*
 *                        PNG path
 *============================================================================*/
static vcdj_error_t vcdj_png_image_info(const vcdj_dec_param_t *param,
                                        vcdj_image_info_t *info)
{
    struct vcdj_png_work *work;
    PngDecInst inst = NULL;
    enum DecRet ret;
    vcdj_error_t err;

    /* Ours to check: the vendor parser starts at byte 8 without reading the
     * signature, and a shorter stream makes `size - 8U` underflow uint32 into a
     * ~4 GB read length. */
    if (param->size < 8U)
    {
        return VCDJ_ERR_PARAM;
    }
    if (memcmp(param->data, "\x89PNG\r\n\x1a\n", 8U) != 0)
    {
        return VCDJ_ERR_STREAM;
    }

    work = vcdj_mem_alloc(sizeof(*work));
    if (work == NULL)
    {
        return VCDJ_ERR_NO_MEM;
    }
    memset(work, 0, sizeof(*work));

    work->cfg.decoder_mode = DEC_NORMAL;
    work->cfg.align = DEC_ALIGN_64B;
    vcdj_setup_default_pp(work->cfg.ppu_config, param->out_format);

    ret = PngDecInit(&inst, vcdj_dwl_handle, &work->cfg);
    if (ret != DEC_OK)
    {
        err = vcdj_map_dec_ret(ret);
        goto out_work;
    }

    vcdj_borrow_input(param, &work->dec_in.stream_buffer);

    work->dec_in.stream_length = param->size - 8U;
    work->dec_in.stream = (u8 *)work->dec_in.stream_buffer.virtual_address + 8U;

    ret = PngDecGetImageInfo(inst, &work->dec_in, &work->image_info);

    if (ret == DEC_OK)
    {
        info->width = work->image_info.display_width;
        info->height = work->image_info.display_height;
        info->aligned_width = work->image_info.output_width;
        info->aligned_height = work->image_info.output_height;
        info->bit_depth = work->image_info.bit_depth;
        info->codec = VCDJ_CODEC_PNG;
        info->color_type = vcdj_png_color_type(work->image_info.color_type);
        info->channels = (uint8_t)work->image_info.channels;
        info->has_alpha = ((info->color_type == VCDJ_COLOR_GRAY_ALPHA) ||
                           (info->color_type == VCDJ_COLOR_RGB_ALPHA)) ? 1U : 0U;

        /* JPEG-only attributes stay zero: PNG has no progressive coding mode,
         * no JFIF thumbnail and no chroma sub-sampling. Adam7 interlacing is a
         * different property and gets its own field. */
        info->jpeg_progressive = 0U;
        info->jpeg_has_thumbnail = 0U;
        info->jpeg_chroma = VCDJ_CHROMA_UNKNOWN;

        info->png_interlaced = (work->image_info.interlaced != 0U) ? 1U : 0U;
    }

    err = vcdj_map_dec_ret(ret);

    PngDecRelease(inst);
out_work:
    vcdj_mem_free(work);

    return err;
}

static vcdj_error_t vcdj_png_decode(const vcdj_dec_param_t *param,
                                    const vcdj_pp_config_t *pp,
                                    vcdj_picture_t *pic)
{
    struct vcdj_png_work *work;
    PngDecOutput *dec_out = NULL;
    PngDecInst inst = NULL;
    enum DecRet ret;
    vcdj_error_t err;
    void *external_buffers[VCDJ_MAX_EXTERNAL_BUFFERS] = {0};
    unsigned int external_count = 0U;
    unsigned int negotiations = 0U;
    struct vcdj_buffer_layout layout;

    memset(&layout, 0, sizeof(layout));

    /* Same check as vcdj_png_image_info(): see there for why. */
    if (param->size < 8U)
    {
        return VCDJ_ERR_PARAM;
    }
    if (memcmp(param->data, "\x89PNG\r\n\x1a\n", 8U) != 0)
    {
        return VCDJ_ERR_STREAM;
    }

    work = vcdj_mem_alloc(sizeof(*work));
    if (work == NULL)
    {
        return VCDJ_ERR_NO_MEM;
    }
    memset(work, 0, sizeof(*work));

    work->cfg.decoder_mode = DEC_NORMAL;
    work->cfg.align = DEC_ALIGN_64B;

    ret = PngDecInit(&inst, vcdj_dwl_handle, &work->cfg);
    if (ret != DEC_OK)
    {
        err = vcdj_map_dec_ret(ret);
        goto out_work;
    }

    err = vcdj_prepare_input(vcdj_dwl_handle, param, &work->dec_in.stream_buffer);
    if (err != VCDJ_SUCCESS)
    {
        goto out_inst;
    }

    work->dec_in.stream_length = param->size - 8U;
    work->dec_in.stream = (u8 *)work->dec_in.stream_buffer.virtual_address + 8U;

    ret = PngDecGetImageInfo(inst, &work->dec_in, &work->image_info);
    if (ret != DEC_OK)
    {
        err = vcdj_map_dec_ret(ret);
        goto out_input;
    }

    err = vcdj_validate_pp(pp, work->image_info.display_width,
                           work->image_info.display_height);
    if (err == VCDJ_SUCCESS)
    {
        err = vcdj_format_fits_source(pp->output_format, VCDJ_CODEC_PNG,
                                      work->image_info.bit_depth);
    }
    if (err != VCDJ_SUCCESS)
    {
        goto out_input;
    }

    /*
     * Passed through unchanged, like JPEG: what PngDecSetInfo() and the checker
     * make of a YUV request for a PNG is exactly what has to be verified, so it
     * must not be normalised away here.
     */
    vcdj_setup_ppu(work->cfg.ppu_config, pp);
    ret = PngDecSetInfo(inst, &work->cfg);
    if (ret != DEC_OK)
    {
        err = vcdj_map_dec_ret(ret);
        goto out_input;
    }

    dec_out = vcdj_mem_alloc(sizeof(*dec_out));
    if (dec_out == NULL)
    {
        err = VCDJ_ERR_NO_MEM;
        goto out_input;
    }
    memset(dec_out, 0, sizeof(*dec_out));

    ret = vcdj_vendor_decode_checked(
              inst, (enum DecRet(*)(void *, void *, void *))PngDecDecode,
              &work->dec_in, dec_out);
    while ((ret == DEC_WAITING_FOR_BUFFER) &&
           (negotiations < VCDJ_MAX_BUFFER_NEGOTIATIONS))
    {
        ret = (enum DecRet)vcdj_add_png_buffers(inst, external_buffers,
                                                VCDJ_MAX_EXTERNAL_BUFFERS,
                                                &external_count, &layout);
        if (ret != DEC_OK)
        {
            err = vcdj_map_dec_ret(ret);
            goto out_input;
        }
        negotiations++;
        ret = vcdj_vendor_decode_checked(
                  inst, (enum DecRet(*)(void *, void *, void *))PngDecDecode,
                  &work->dec_in, dec_out);
    }
    if ((ret != DEC_PIC_RDY) && (ret != DEC_PIC_DECODED) && (ret != DEC_OK))
    {
        err = vcdj_map_dec_ret(ret);
        goto out_input;
    }

    /*
     * NextPicture() is what populates `pictures[]`: PngDecDecode() zeroes the
     * whole array, so before this call there is no output descriptor and no
     * actual format to report. This is the call the PNG path used to skip - it
     * got away with it only because the instance was destroyed after every
     * decode, which is exactly the masking described in 5.2.3 item 2 of the
     * interface plan.
     */
    ret = PngDecNextPicture(inst, dec_out, &work->image_info);
    if ((ret != DEC_PIC_RDY) && (ret != DEC_OK))
    {
        err = vcdj_map_dec_ret(ret);
        goto out_input;
    }

    pic->width = dec_out->pictures[0].display_width;
    pic->height = dec_out->pictures[0].display_height;

    /*
     * Now a real answer instead of VCDJ_OUT_UNKNOWN. Reading it back matters for
     * PNG: a YUV420SP request comes back as BGR888 / ABGR8888, and greyscale is
     * forced onto the monochrome path whatever was requested.
     */
    pic->format = vcdj_out_format_from_vendor(dec_out->pictures[0].output_format,
                                              pp->output_format);

    if ((pic->width == 0U) || (pic->height == 0U))
    {
        pic->width = work->image_info.display_width;
        pic->height = work->image_info.display_height;
    }

    if (dec_out->pictures[0].pic_stride != 0U)
    {
        layout.ystride = dec_out->pictures[0].pic_stride;
    }
    if (dec_out->pictures[0].pic_stride_ch != 0U)
    {
        layout.cstride = dec_out->pictures[0].pic_stride_ch;
    }

    vcdj_fill_planes(&dec_out->pictures[0].output_picture_y,
                     &dec_out->pictures[0].output_picture_cb_cr,
                     &dec_out->pictures[0].output_picture_cr,
                     &layout, pic);

    /*
     * Same order and same reasoning as the JPEG path - this is the point of N2:
     * both codecs now run Decode -> NextPicture -> PictureConsumed rather than
     * relying on instance teardown to paper over a missing return.
     *
     * The two are not symmetric underneath, and that is the vendor's doing: the
     * JPEG version reads a consumed flag out of `pp_index` while the PNG version
     * passes a fixed external flag. Both end up clearing the same queue slot, so
     * the call sequence here is identical even though the implementations differ.
     */
    (void)PngDecPictureConsumed(inst, dec_out);

    err = vcdj_publish_picture(external_buffers, &external_count, pic);
    if (err != VCDJ_SUCCESS)
    {
        pic->plane_count = 0U;
        memset(pic->planes, 0, sizeof(pic->planes));
    }

out_input:
    vcdj_release_input(vcdj_dwl_handle, &work->dec_in.stream_buffer);
out_inst:
    PngDecRelease(inst);
    vcdj_internal_free_external_buffers(external_buffers, external_count);
out_work:
    vcdj_mem_free(dec_out);
    vcdj_mem_free(work);

    return err;
}

/*============================================================================*
 *                        JPEG Frame Scanner
 *============================================================================*/
/*
 * Frames are found by walking the marker structure, not by searching for FF D9:
 * an EXIF or JFXX thumbnail carries its own EOI inside an APPn payload, and only
 * skipping that segment by its length keeps it from ending the frame early.
 */
#define VCDJ_JPEG_SOI   0xD8U
#define VCDJ_JPEG_EOI   0xD9U
#define VCDJ_JPEG_SOS   0xDAU
#define VCDJ_JPEG_RST0  0xD0U
#define VCDJ_JPEG_RST7  0xD7U
#define VCDJ_JPEG_TEM   0x01U

/* Offset of the next FF D8 at or after `pos`, or `size` when there is none. */
static uint32_t vcdj_jpeg_find_soi(const uint8_t *buf, uint32_t size, uint32_t pos)
{
    while ((size - pos) >= 2U)
    {
        if ((buf[pos] == 0xFFU) && (buf[pos + 1U] == VCDJ_JPEG_SOI))
        {
            return pos;
        }
        pos++;
    }

    return size;
}

/*
 * Offset of the first marker ending the entropy-coded data at `pos`, or `size`.
 * Only C0..FE can end a scan; any other FF xx is corrupt data and is left for
 * the decoder to report, so one bad byte does not hide the frame from the caller.
 */
static uint32_t vcdj_jpeg_skip_entropy(const uint8_t *buf, uint32_t size, uint32_t pos)
{
    while ((size - pos) >= 2U)
    {
        if (buf[pos] == 0xFFU)
        {
            uint8_t next = buf[pos + 1U];

            if ((next >= 0xC0U) && (next != 0xFFU) &&
                ((next < VCDJ_JPEG_RST0) || (next > VCDJ_JPEG_RST7)))
            {
                return pos;
            }
            if (next != 0xFFU)
            {
                pos += 2U;  /* FF 00 stuffing, RSTn, or corrupt FF xx */
                continue;
            }
        }
        pos++;
    }

    return size;
}

/*
 * Walk the frame whose SOI is at `soi`. Returns 0 with `*end` just past its EOI,
 * or -1 when the frame is truncated or malformed.
 */
static int vcdj_jpeg_frame_end(const uint8_t *buf, uint32_t size, uint32_t soi,
                               uint32_t *end)
{
    uint32_t pos = soi + 2U;

    for (;;)
    {
        uint8_t marker;
        uint32_t len;

        if (((size - pos) < 2U) || (buf[pos] != 0xFFU))
        {
            return -1;
        }
        /* Any number of FF fill bytes may precede a marker. */
        while (((size - pos) >= 2U) && (buf[pos + 1U] == 0xFFU))
        {
            pos++;
        }
        if ((size - pos) < 2U)
        {
            return -1;
        }

        marker = buf[pos + 1U];
        if (marker == VCDJ_JPEG_EOI)
        {
            *end = pos + 2U;
            return 0;
        }
        if ((marker == VCDJ_JPEG_SOI) || (marker == 0x00U))
        {
            return -1;
        }
        if ((marker == VCDJ_JPEG_TEM) ||
            ((marker >= VCDJ_JPEG_RST0) && (marker <= VCDJ_JPEG_RST7)))
        {
            pos += 2U;
            continue;
        }

        if ((size - pos) < 4U)
        {
            return -1;
        }
        len = ((uint32_t)buf[pos + 2U] << 8) | buf[pos + 3U];
        if ((len < 2U) || (len > (size - pos - 2U)))
        {
            return -1;
        }
        pos += 2U + len;

        /* A progressive frame has several scans; each comes back here. */
        if (marker == VCDJ_JPEG_SOS)
        {
            pos = vcdj_jpeg_skip_entropy(buf, size, pos);
        }
    }
}

/*============================================================================*
 *                        Public Functions
 *============================================================================*/
void vcdj_mem_init(void *(*vmalloc)(size_t), void (*vfree)(void *))
{
    vcdj_mem_set_allocator(vmalloc, vfree);
}

vcdj_error_t vcdj_init(void)
{
    struct DWLInitParam param;

    if (vcdj_initialised)
    {
        return VCDJ_SUCCESS;
    }

    if (!vcdj_mem_allocator_ready())
    {
        /* vcdj_mem_init() has not been called; refuse instead of quietly
         * pulling in libc malloc. */
        return VCDJ_ERR_NO_MEM;
    }

    memset(&param, 0, sizeof(param));
    param.client_type = DWL_CLIENT_TYPE_JPEG_DEC;

    if (vcdj_os_mutex_create(&vcdj_live_lock) != 0)
    {
        vcdj_live_lock = VCDJ_OS_HANDLE_INVALID;
        return VCDJ_ERR_NO_MEM;
    }

    vcdj_dwl_handle = DWLInit(&param);
    if (vcdj_dwl_handle == NULL)
    {
        (void)vcdj_os_mutex_destroy(vcdj_live_lock);
        vcdj_live_lock = VCDJ_OS_HANDLE_INVALID;
        return VCDJ_ERR_NO_HW;
    }

    vcdj_initialised = 1U;

    return VCDJ_SUCCESS;
}

void vcdj_deinit(void)
{
    struct vcdj_live_picture *live;

    if (!vcdj_initialised)
    {
        return;
    }

    /*
     * Reclaim any picture the caller never released. Leaking it instead would be
     * unrecoverable: the tokens naming them die with the caller's structures, so
     * nothing could ever free the memory again.
     *
     * This runs BEFORE DWLRelease(), because DWLFreeRefFrm() goes through the DWL
     * instance that DWLRelease() tears down.
     */
    vcdj_live_lock_take();
    while (vcdj_live_head != NULL)
    {
        live = vcdj_live_head;
        vcdj_live_head = live->next;
        vcdj_live_free(live);
    }
    vcdj_live_lock_give();

    (void)vcdj_os_mutex_destroy(vcdj_live_lock);
    vcdj_live_lock = VCDJ_OS_HANDLE_INVALID;

    (void)DWLRelease(vcdj_dwl_handle);
    vcdj_dwl_handle = NULL;
    vcdj_initialised = 0U;
}

vcdj_error_t vcdj_get_image_info(const vcdj_dec_param_t *param,
                                 vcdj_image_info_t *info)
{
    if ((param == NULL) || (info == NULL) || (param->data == NULL) || (param->size == 0U))
    {
        return VCDJ_ERR_PARAM;
    }

    if (!vcdj_codec_supported(param->codec))
    {
        return VCDJ_ERR_UNSUPPORTED;
    }

    if (!vcdj_picture_sel_valid(param))
    {
        return VCDJ_ERR_PARAM;
    }

    if (!vcdj_initialised)
    {
        return VCDJ_ERR_STATE;
    }

    memset(info, 0, sizeof(*info));

    if (param->codec == VCDJ_CODEC_JPEG)
    {
        return vcdj_jpeg_image_info(param, info);
    }

    return vcdj_png_image_info(param, info);
}

vcdj_error_t vcdj_jpeg_scan_frames(const void *data, uint32_t size,
                                   vcdj_jpeg_frame_t *frames,
                                   uint32_t max_frames,
                                   uint32_t *frame_count)
{
    const uint8_t *buf = (const uint8_t *)data;
    uint32_t found = 0U;
    uint32_t pos = 0U;
    uint32_t end = 0U;
    int broken = 0;

    if ((data == NULL) || (size == 0U) ||
        ((frames == NULL) && (max_frames != 0U)) ||
        ((frame_count == NULL) && (max_frames == 0U)))
    {
        return VCDJ_ERR_PARAM;
    }

    for (;;)
    {
        /* Without a count to report there is nothing to learn past a full array. */
        if ((frame_count == NULL) && (found == max_frames))
        {
            break;
        }

        pos = vcdj_jpeg_find_soi(buf, size, pos);
        if (pos == size)
        {
            break;
        }
        if (vcdj_jpeg_frame_end(buf, size, pos, &end) != 0)
        {
            broken = 1;
            break;
        }

        if (found < max_frames)
        {
            frames[found].offset = pos;
            frames[found].size = end - pos;
        }
        found++;
        pos = end;
    }

    if (frame_count != NULL)
    {
        *frame_count = found;
    }

    if (broken || (found == 0U) || ((frame_count == NULL) && (found < max_frames)))
    {
        return VCDJ_ERR_STREAM;
    }

    return VCDJ_SUCCESS;
}

vcdj_error_t vcdj_decode_pp(const vcdj_dec_param_t *param,
                            const vcdj_pp_config_t *pp,
                            vcdj_picture_t *pic)
{
    if (pic != NULL)
    {
        memset(pic, 0, sizeof(*pic));
        pic->format = VCDJ_OUT_UNKNOWN;
    }

    if ((param == NULL) || (pp == NULL) || (pic == NULL) ||
        (param->data == NULL) || (param->size == 0U))
    {
        return VCDJ_ERR_PARAM;
    }

    if (!vcdj_codec_supported(param->codec))
    {
        return VCDJ_ERR_UNSUPPORTED;
    }

    if (!vcdj_out_format_supported(pp->output_format) ||
        !vcdj_pp_filter_supported(pp->filter) ||
        (pp->crop_enable > 1U) || (pp->scale_enable > 1U) ||
        (pp->dither_disable > 1U) || !vcdj_picture_sel_valid(param))
    {
        return VCDJ_ERR_PARAM;
    }

    if (!vcdj_initialised)
    {
        return VCDJ_ERR_STATE;
    }

    if (param->codec == VCDJ_CODEC_JPEG)
    {
        return vcdj_jpeg_decode(param, pp, pic);
    }

    return vcdj_png_decode(param, pp, pic);
}

vcdj_error_t vcdj_decode(const vcdj_dec_param_t *param, vcdj_picture_t *pic)
{
    vcdj_pp_config_t pp;

    if (param == NULL)
    {
        return VCDJ_ERR_PARAM;
    }

    memset(&pp, 0, sizeof(pp));
    pp.output_format = param->out_format;
    pp.filter = VCDJ_PP_FILTER_DEFAULT;

    return vcdj_decode_pp(param, &pp, pic);
}

vcdj_error_t vcdj_picture_release(vcdj_picture_t *pic)
{
    struct vcdj_live_picture *live;

    if (pic == NULL)
    {
        return VCDJ_SUCCESS;
    }

    /*
     * A zero token is the documented "no pixels" case: a decode that reported
     * geometry only still has to round-trip through release, and calling release
     * twice has to stay safe, so this is a success rather than an error.
     */
    if (pic->release_token == 0U)
    {
        memset(pic, 0, sizeof(*pic));
        return VCDJ_SUCCESS;
    }

    /* After deinit every picture has been reclaimed, so no token is live. */
    if (!vcdj_initialised)
    {
        return VCDJ_ERR_PARAM;
    }

    {
        struct vcdj_live_picture **link = NULL;

        vcdj_live_lock_take();
        live = vcdj_live_find((uint32_t)pic->release_token, &link);
        if ((live != NULL) && ((uintptr_t)live->id != pic->release_token))
        {
            live = NULL;    /* high bits set: not a token this driver minted */
        }
        if (live != NULL)
        {
            *link = live->next;
        }
        vcdj_live_lock_give();
    }

    if (live == NULL)
    {
        /* Already released, or never handed out by this driver. */
        return VCDJ_ERR_PARAM;
    }

    /*
     * Only the pixel memory is released here. The queue slot and the codec
     * instance were already wound down inside vcdj_decode(), so no vendor
     * call is needed - and none would be valid, because the instance that owned
     * that bookkeeping no longer exists. The node is off the list, so the free
     * itself needs no lock.
     */
    vcdj_live_free(live);

    memset(pic, 0, sizeof(*pic));

    return VCDJ_SUCCESS;
}
