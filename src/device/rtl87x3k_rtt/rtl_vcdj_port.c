/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *                        Header Files
 *============================================================================*/
/*
 * PLATFORM LAYER (RT-Thread): the only translation unit that touches the RTOS
 * and the SoC.
 *
 * This is the RT-Thread counterpart of src/device/rtl87x3k/rtl_vcdj_port.c, which
 * targets the Realtek RTOS SDK (FreeRTOS + OSIF). Exactly ONE of the two is
 * compiled into a given build; they implement the same contract from
 * rtl_vcdj_internal.h and nothing else in the driver changes between platforms:
 *
 *   1. `struct vcdj_platform_ops` + vcdj_platform_get_config()
 *      registers, DMA memory, cache, locking, clock/reset, time
 *   2. `vcdj_os_*()`
 *      the mutex / condvar / semaphore primitives the vendor OS bridge in
 *      src/common/rtl_vcdj_dwl.c needs
 *
 * This file MUST NOT include any vendor Control Software header. `dwlthread.h`
 * redefines `EFAULT`/`EINVAL`/`EBUSY`/`ENOTTY` and defines `atomic_t`, `open`,
 * `close`, `ioctl`, `mmap`, `printk`, `request_irq`, `spin_lock_irqsave` and
 * friends; keeping them out of this translation unit is what lets the RTOS
 * headers be included at all.
 *
 * SoC SCOPE: the platform ops and the OS primitives here are OS-specific but
 * SoC-agnostic; only rtl_vcdj_soc_cfg.h is per-SoC.
 *
 * Deliberate differences from the OSIF port, all forced by the RTOS API:
 *
 *   - RT-Thread IPC constructors RETURN a handle (NULL on failure) instead of
 *     filling an out-parameter and returning a bool.
 *   - Timeouts are rt_int32_t TICKS, and "forever" is RT_WAITING_FOREVER (-1),
 *     not 0xFFFFFFFF. vcdj_ticks_from_ms() below is the single conversion point;
 *     passing the raw millisecond value through would turn "wait forever" into a
 *     very large finite timeout.
 *   - There is no CMSIS header in this project (board/board.c hand-declares the
 *     few core registers it needs), so the data barrier is a GCC inline asm
 *     rather than __DMB().
 */
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include <rtthread.h>
#include <rthw.h>

#include "rtl_vcdj_internal.h"
#include "rtl_vcdj_soc_cfg.h"

/*============================================================================*
 *                        Local Definitions
 *============================================================================*/
#ifndef VCDJ_HW_TIMEOUT_MS
#define VCDJ_HW_TIMEOUT_MS      2000U
#endif

/* Matches RT_CONSOLEBUF_SIZE: raising it alone would not widen the console. */
#define VCDJ_CONSOLE_LINE_MAX   128U

/** Guard word placed in front of every aligned allocation. */
#define VCDJ_ALLOC_MAGIC        0x56434A41U     /* 'VCJA' */

/**
 * \brief Data memory barrier.
 *
 * The Realtek SDK port uses the CMSIS __DMB(). This project carries no CMSIS
 * headers, so the instruction is emitted directly. "dmb 0xF" is the SY (full
 * system) option, matching what __DMB() expands to; the memory clobber stops the
 * compiler from moving accesses across it.
 */
#define VCDJ_DMB()              __asm__ volatile("dmb 0xF" ::: "memory")

/* swreg1 bits for the reset abort, from the vendor hantrodec_defs.h. */
#define VCDJ_PLAT_SWREG_IRQ_OFFSET  0x04U
#define VCDJ_PLAT_DEC_E             0x01U
#define VCDJ_PLAT_DEC_IRQ_DISABLE   0x10U
#define VCDJ_PLAT_DEC_ABORT         0x20U

/*
 * Object pool sizing for the JPEG + PNG closure that is actually compiled:
 *   mutexes    input_queue (cs, cnt_release, buf_release) = 3 per queue,
 *              jpeg/png fb_list (out_count, ref_count)    = 2,
 *              dec_log log_mutex                          = 1 (VCD_LOGMSG off)
 *              HAL live-picture list lock                 = 1, held from init to deinit
 *   condvars   input_queue (cnt_release_cv, buf_release_cv) = 2 per queue,
 *              jpeg/png fb_list hw_rdy_cv                   = 1
 *   semaphores fifo (cs, read, write)                       = 3 per fifo,
 *              fb_list out_count_sem, sem_ri_mc_done        = 2
 *
 * Exhaustion is reported as a hard error rather than silently ignored. The 32
 * ceiling comes from the allocation bitmaps being a single uint32_t each.
 */
#ifndef VCDJ_OS_MUTEX_COUNT
#define VCDJ_OS_MUTEX_COUNT     12
#endif

#ifndef VCDJ_OS_COND_COUNT
#define VCDJ_OS_COND_COUNT      8
#endif

#ifndef VCDJ_OS_SEM_COUNT
#define VCDJ_OS_SEM_COUNT       12
#endif

#if (VCDJ_OS_MUTEX_COUNT < 1) || (VCDJ_OS_MUTEX_COUNT > 32)
#error "VCDJ_OS_MUTEX_COUNT must be in the range 1..32"
#endif

#if (VCDJ_OS_COND_COUNT < 1) || (VCDJ_OS_COND_COUNT > 32)
#error "VCDJ_OS_COND_COUNT must be in the range 1..32"
#endif

#if (VCDJ_OS_SEM_COUNT < 1) || (VCDJ_OS_SEM_COUNT > 32)
#error "VCDJ_OS_SEM_COUNT must be in the range 1..32"
#endif

/*============================================================================*
 *                        Local Types
 *============================================================================*/
struct vcdj_alloc_hdr
{
    uint32_t magic;
    void *raw;
    size_t size;
};

struct vcdj_os_sem_slot
{
    rt_sem_t handle;
    uint32_t count;
    uint32_t max_count;
};

/*============================================================================*
 *                        Local Variables
 *============================================================================*/
static rt_mutex_t vcdj_hw_lock;

static rt_mutex_t vcdj_mutex_pool[VCDJ_OS_MUTEX_COUNT];
static rt_sem_t vcdj_cond_pool[VCDJ_OS_COND_COUNT];
static struct vcdj_os_sem_slot vcdj_sem_pool[VCDJ_OS_SEM_COUNT];

static uint32_t vcdj_mutex_used;
static uint32_t vcdj_cond_used;
static uint32_t vcdj_sem_used;

/*============================================================================*
 *                        Local Functions: timeout conversion
 *============================================================================*/
/**
 * \brief Convert a millisecond timeout from the driver contract into RT-Thread
 *        ticks.
 *
 * The driver contract uses 0xFFFFFFFF for "wait forever"
 * (\ref VCDJ_OS_WAIT_FOREVER, and \ref VCDJ_TIMEOUT_FOREVER has the same value),
 * while RT-Thread spells it RT_WAITING_FOREVER (-1). This is the single place
 * that mapping happens.
 *
 * A non-zero millisecond value must never round down to 0 ticks: 0 is
 * RT_WAITING_NO in RT-Thread, which turns a short bounded wait into a
 * non-blocking poll that reports failure immediately. It is clamped to 1 tick.
 * With RT_TICK_PER_SECOND at 1000 the conversion is 1:1 and the clamp never
 * fires, but the driver must not depend on that configuration.
 */
static rt_int32_t vcdj_ticks_from_ms(uint32_t timeout_ms)
{
    rt_tick_t ticks;

    if (timeout_ms == VCDJ_OS_WAIT_FOREVER)
    {
        return RT_WAITING_FOREVER;
    }

    if (timeout_ms == 0U)
    {
        return RT_WAITING_NO;
    }

    ticks = rt_tick_from_millisecond((rt_int32_t)timeout_ms);
    if (ticks == 0U)
    {
        ticks = 1U;
    }

    /* RT-Thread takes a SIGNED tick count. A conversion result above INT32_MAX
     * would reinterpret as a negative number, and -1 in particular is
     * RT_WAITING_FOREVER, so a long bounded wait would silently become unbounded.
     * Saturating at INT32_MAX keeps the wait bounded, which is the whole point of
     * this driver's timeout contract. */
    if (ticks > (rt_tick_t)INT32_MAX)
    {
        return INT32_MAX;
    }

    return (rt_int32_t)ticks;
}

/*============================================================================*
 *                        Local Functions: object pools
 *============================================================================*/
static int vcdj_pool_take(uint32_t *bitmap, int count)
{
    rt_base_t level;
    int i;

    level = rt_hw_interrupt_disable();

    for (i = 0; i < count; i++)
    {
        if ((*bitmap & (1UL << i)) == 0U)
        {
            *bitmap |= (1UL << i);
            rt_hw_interrupt_enable(level);
            return i;
        }
    }

    rt_hw_interrupt_enable(level);
    return -1;
}

static void vcdj_pool_give(uint32_t *bitmap, int index)
{
    rt_base_t level = rt_hw_interrupt_disable();

    *bitmap &= ~(1UL << index);

    rt_hw_interrupt_enable(level);
}

static int vcdj_handle_valid(vcdj_os_handle_t handle, int count,
                             const uint32_t *bitmap)
{
    rt_base_t level;
    int valid;

    if ((handle < 0) || (handle >= count))
    {
        return 0;
    }

    level = rt_hw_interrupt_disable();
    valid = ((*bitmap & (1UL << (uint32_t)handle)) != 0U);
    rt_hw_interrupt_enable(level);

    return valid;
}

/*============================================================================*
 *                        Platform ops: registers
 *============================================================================*/
static uint32_t vcdj_plat_reg_read32(void *ctx, uint32_t offset)
{
    (void)ctx;

#if VCDJ_SOC_REG_BASE_VALID
    return *(volatile uint32_t *)(VCDJ_SOC_REG_BASE + VCDJ_SOC_CORE_OFFSET + offset);
#else
    /* TODO(hardware-contract): no MMIO base has been delivered. Returning zero
     * keeps the driver from touching address zero. */
    (void)offset;
    return 0U;
#endif
}

static void vcdj_plat_reg_write32(void *ctx, uint32_t offset, uint32_t value)
{
    (void)ctx;

#if VCDJ_SOC_REG_BASE_VALID
    *(volatile uint32_t *)(VCDJ_SOC_REG_BASE + VCDJ_SOC_CORE_OFFSET + offset) = value;
#else
    /* TODO(hardware-contract): see vcdj_plat_reg_read32(). */
    (void)offset;
    (void)value;
#endif
}

/*============================================================================*
 *                        Platform ops: DMA capable memory
 *============================================================================*/
/*
 * Optional bump arena for DMA buffers, selected by vcdj_plat_dma_region_set()
 * (0 = heap). Used by the firmware test suite for outputs too big for the heap.
 * It never reclaims: reset it before each job; exhaustion returns ERR_NO_MEM.
 */
extern unsigned char __gpu_ram_start;    /* SYS_RAM reserved for the GPU */

static int vcdj_dma_probe_region;        /* 0 = heap, 1 = sysram2 */
static uintptr_t vcdj_dma_probe_base;
static size_t vcdj_dma_probe_size;
static size_t vcdj_dma_probe_used;

void vcdj_plat_dma_region_set(int region)
{
    vcdj_dma_probe_region = region;
    vcdj_dma_probe_used = 0U;

    switch (region)
    {
    case 1:
        /* GPU RAM is 1.125 MB (link.lds) from 0x20640000. The arena takes
         * 896 KB and leaves the top 256 KB from 0x20720000 for the stream
         * window. Keep the two apart: the arena never reclaims, so an overlap
         * would be silently overwritten mid-decode.
         *
         * The 896 KB is sized by the largest case that fits at all - JPEG@201
         * needs 786,625 B (input + four PPU lanczos tables + the 540x380 RGB
         * output); no case between it and PNG@23 (1.88 MB) exists. */
        vcdj_dma_probe_base = (uintptr_t)&__gpu_ram_start;
        vcdj_dma_probe_size = 896U * 1024U;
        break;
    default:
        vcdj_dma_probe_base = 0U;
        vcdj_dma_probe_size = 0U;
        break;
    }
}

void vcdj_plat_dma_region_info(int *region, uintptr_t *base, size_t *size)
{
    *region = vcdj_dma_probe_region;
    *base = vcdj_dma_probe_base;
    *size = vcdj_dma_probe_size;
}

static int vcdj_plat_dma_alloc(void *ctx, size_t size, size_t align,
                               void **cpu_addr, uintptr_t *bus_addr)
{
    struct vcdj_alloc_hdr *hdr;
    uintptr_t aligned;
    void *raw;

    (void)ctx;

    if ((cpu_addr == NULL) || (bus_addr == NULL) || (size == 0U) ||
        (align == 0U))
    {
        return -1;
    }

    *cpu_addr = NULL;
    *bus_addr = 0U;

    if (align < sizeof(void *))
    {
        align = sizeof(void *);
    }

    if (((align & (align - 1U)) != 0U) ||
        (size > (SIZE_MAX - align - sizeof(struct vcdj_alloc_hdr))))
    {
        return -1;
    }

    if (vcdj_dma_probe_region != 0)
    {
        size_t need = size + align + sizeof(struct vcdj_alloc_hdr);

        if ((need > vcdj_dma_probe_size) ||
            (vcdj_dma_probe_used > (vcdj_dma_probe_size - need)))
        {
            vcdj_plat_log("[vcdj] dma probe arena exhausted: need %u, used %u of %u\n",
                          (unsigned)need, (unsigned)vcdj_dma_probe_used,
                          (unsigned)vcdj_dma_probe_size);
            return -1;
        }

        raw = (void *)(vcdj_dma_probe_base + vcdj_dma_probe_used);
        vcdj_dma_probe_used += need;
    }
    else
    {
        raw = vcdj_mem_alloc(size + align + sizeof(struct vcdj_alloc_hdr));
    }

    if (raw == NULL)
    {
        return -1;
    }

    aligned = ((uintptr_t)raw + sizeof(struct vcdj_alloc_hdr) + (align - 1U)) &
              ~((uintptr_t)align - 1U);

    hdr = (struct vcdj_alloc_hdr *)(aligned - sizeof(struct vcdj_alloc_hdr));
    hdr->magic = VCDJ_ALLOC_MAGIC;
    hdr->raw = raw;
    hdr->size = size;

    *cpu_addr = (void *)aligned;

    /* TODO(hardware-contract): identity CPU-to-bus mapping is only a placeholder.
     * Confirm the address translation and reachable RAM regions before decode. */
    *bus_addr = aligned;

    return 0;
}

static void vcdj_plat_dma_free(void *ctx, void *cpu_addr,
                               uintptr_t bus_addr, size_t size)
{
    struct vcdj_alloc_hdr *hdr;

    (void)ctx;
    (void)bus_addr;
    (void)size;

    if (cpu_addr == NULL)
    {
        return;
    }

    hdr = (struct vcdj_alloc_hdr *)((uintptr_t)cpu_addr -
                                    sizeof(struct vcdj_alloc_hdr));
    if (hdr->magic != VCDJ_ALLOC_MAGIC)
    {
        return;
    }

    hdr->magic = 0U;

    /* BRING-UP: arena blocks are not heap pointers, see vcdj_plat_dma_alloc(). */
    if (vcdj_dma_probe_region != 0)
    {
        return;
    }

    vcdj_mem_free(hdr->raw);
}

/*============================================================================*
 *                        Platform ops: cache and barriers
 *============================================================================*/
static void vcdj_plat_cache_clean(void *ctx, void *addr, size_t size)
{
    (void)ctx;
    (void)addr;
    (void)size;

#if !VCDJ_SOC_DMA_COHERENT
    /* TODO(hardware-contract): cacheability, maintenance granularity and the
     * decoder's coherency domain are not known yet. Do not guess a cache API. */
    VCDJ_DMB();
#endif
}

static void vcdj_plat_cache_invalidate(void *ctx, void *addr, size_t size)
{
    (void)ctx;
    (void)addr;
    (void)size;

#if !VCDJ_SOC_DMA_COHERENT
    /* TODO(hardware-contract): see vcdj_plat_cache_clean(). */
    VCDJ_DMB();
#endif
}

static void vcdj_plat_memory_barrier(void *ctx)
{
    (void)ctx;
    VCDJ_DMB();
}

/*============================================================================*
 *                        Platform ops: locking
 *============================================================================*/
static int vcdj_plat_sync_init(void)
{
    if (vcdj_hw_lock != RT_NULL)
    {
        return 0;
    }

    vcdj_hw_lock = rt_mutex_create("vcdj_hw", RT_IPC_FLAG_PRIO);
    if (vcdj_hw_lock == RT_NULL)
    {
        return -1;
    }

    return 0;
}

static int vcdj_plat_lock(void *ctx, uint32_t timeout_ms)
{
    (void)ctx;

    if (vcdj_hw_lock == RT_NULL)
    {
        return -1;
    }

    return (rt_mutex_take(vcdj_hw_lock, vcdj_ticks_from_ms(timeout_ms)) == RT_EOK)
           ? 0 : -1;
}

static void vcdj_plat_unlock(void *ctx)
{
    (void)ctx;

    if (vcdj_hw_lock != RT_NULL)
    {
        (void)rt_mutex_release(vcdj_hw_lock);
    }
}

/*============================================================================*
 *                        Platform ops: clock, reset and time
 *============================================================================*/
static int vcdj_plat_clock_enable(void *ctx)
{
    (void)ctx;

    if (vcdj_plat_sync_init() != 0)
    {
        return -1;
    }

#if VCDJ_SOC_HAS_CLOCK_CONTROL
#error "VCDJ_SOC_HAS_CLOCK_CONTROL is set but no clock sequence is implemented"
#else
    /* TODO(hardware-contract): the clock, power domain and ordering are unknown.
     * The unrelated Realtek JPU clock gate must not be reused here. */
    return 0;
#endif
}

static void vcdj_plat_clock_disable(void *ctx)
{
    (void)ctx;
    /* TODO(hardware-contract): see vcdj_plat_clock_enable(). */
}

static int vcdj_plat_reset(void *ctx)
{
#if VCDJ_SOC_HAS_RESET_CONTROL
#error "VCDJ_SOC_HAS_RESET_CONTROL is set but no reset sequence is implemented"
#endif

#if VCDJ_SOC_REG_BASE_VALID
    uint32_t status;

    /*
     * Abort step of the vendor ResetAsic(), which touches no SoC reset or clock
     * control. Only swreg1 is written: its full-window zero fill over 896 regs
     * wedges the bus. Runs before vcdj_core_up is set, so the platform accessors
     * are used directly; vcdj_core_write_reg() would drop the write.
     */
    status = vcdj_plat_reg_read32(ctx, VCDJ_PLAT_SWREG_IRQ_OFFSET);
    if ((status & VCDJ_PLAT_DEC_E) != 0U)
    {
        vcdj_plat_reg_write32(ctx, VCDJ_PLAT_SWREG_IRQ_OFFSET,
                              VCDJ_PLAT_DEC_ABORT | VCDJ_PLAT_DEC_IRQ_DISABLE);
    }
#else
    (void)ctx;
#endif

    return 0;
}

static uint32_t vcdj_plat_get_time_ms(void *ctx)
{
    (void)ctx;
    return (uint32_t)rt_tick_get_millisecond();
}

static void vcdj_plat_delay_us(void *ctx, uint32_t us)
{
    (void)ctx;

    if (us != 0U)
    {
        rt_hw_us_delay(us);
    }
}

/*============================================================================*
 *                        Static Binding
 *============================================================================*/
static const struct vcdj_platform_ops vcdj_plat_ops =
{
    .reg_read32       = vcdj_plat_reg_read32,
    .reg_write32      = vcdj_plat_reg_write32,
    .dma_alloc        = vcdj_plat_dma_alloc,
    .dma_free         = vcdj_plat_dma_free,
    .cache_clean      = vcdj_plat_cache_clean,
    .cache_invalidate = vcdj_plat_cache_invalidate,
    .memory_barrier   = vcdj_plat_memory_barrier,
    .lock             = vcdj_plat_lock,
    .unlock           = vcdj_plat_unlock,
    .wait_done        = NULL,
    .wait_reset       = NULL,
    .clock_enable     = vcdj_plat_clock_enable,
    .clock_disable    = vcdj_plat_clock_disable,
    .reset            = vcdj_plat_reset,
    .get_time_ms      = vcdj_plat_get_time_ms,
    .delay_us         = vcdj_plat_delay_us,
};

static const struct vcdj_platform_config vcdj_plat_config =
{
    .ops             = &vcdj_plat_ops,
    .ctx             = NULL,
    .reg_base        = VCDJ_SOC_REG_BASE,
    .core_offset     = VCDJ_SOC_CORE_OFFSET,
    .reg_window_size = VCDJ_SOC_REG_WINDOW_SIZE,
    .core_count      = VCDJ_SOC_CORE_COUNT,
    .hw_timeout_ms   = VCDJ_HW_TIMEOUT_MS,
};

const struct vcdj_platform_config *vcdj_platform_get_config(void)
{
    return &vcdj_plat_config;
}

/*============================================================================*
 *                        OS primitives: mutex
 *============================================================================*/
int vcdj_os_mutex_create(vcdj_os_handle_t *handle)
{
    int index;

    if (handle == NULL)
    {
        return -1;
    }

    *handle = VCDJ_OS_HANDLE_INVALID;
    index = vcdj_pool_take(&vcdj_mutex_used, VCDJ_OS_MUTEX_COUNT);
    if (index < 0)
    {
        return -1;
    }

    /* RT_IPC_FLAG_PRIO: the vendor code takes these mutexes from decode threads
     * of differing priority, so priority inheritance is what keeps a low priority
     * holder from blocking a high priority waiter indefinitely. */
    vcdj_mutex_pool[index] = rt_mutex_create("vcdj_mtx", RT_IPC_FLAG_PRIO);
    if (vcdj_mutex_pool[index] == RT_NULL)
    {
        vcdj_pool_give(&vcdj_mutex_used, index);
        return -1;
    }

    *handle = (vcdj_os_handle_t)index;
    return 0;
}

int vcdj_os_mutex_destroy(vcdj_os_handle_t handle)
{
    if (!vcdj_handle_valid(handle, VCDJ_OS_MUTEX_COUNT, &vcdj_mutex_used))
    {
        return -1;
    }

    if (rt_mutex_delete(vcdj_mutex_pool[handle]) != RT_EOK)
    {
        return -1;
    }

    vcdj_mutex_pool[handle] = RT_NULL;
    vcdj_pool_give(&vcdj_mutex_used, (int)handle);
    return 0;
}

int vcdj_os_mutex_lock(vcdj_os_handle_t handle, uint32_t timeout_ms)
{
    if (!vcdj_handle_valid(handle, VCDJ_OS_MUTEX_COUNT, &vcdj_mutex_used))
    {
        return -1;
    }

    return (rt_mutex_take(vcdj_mutex_pool[handle],
                          vcdj_ticks_from_ms(timeout_ms)) == RT_EOK) ? 0 : -1;
}

int vcdj_os_mutex_unlock(vcdj_os_handle_t handle)
{
    if (!vcdj_handle_valid(handle, VCDJ_OS_MUTEX_COUNT, &vcdj_mutex_used))
    {
        return -1;
    }

    return (rt_mutex_release(vcdj_mutex_pool[handle]) == RT_EOK) ? 0 : -1;
}

/*============================================================================*
 *                        OS primitives: condition variable
 *============================================================================*/
int vcdj_os_cond_create(vcdj_os_handle_t *handle)
{
    int index;

    if (handle == NULL)
    {
        return -1;
    }

    *handle = VCDJ_OS_HANDLE_INVALID;
    index = vcdj_pool_take(&vcdj_cond_used, VCDJ_OS_COND_COUNT);
    if (index < 0)
    {
        return -1;
    }

    /* RT-Thread has no condition variable. A binary semaphore preserves one
     * pending wakeup and lets wait release/reacquire the associated mutex
     * explicitly. */
    vcdj_cond_pool[index] = rt_sem_create("vcdj_cond", 0U, RT_IPC_FLAG_PRIO);
    if (vcdj_cond_pool[index] == RT_NULL)
    {
        vcdj_pool_give(&vcdj_cond_used, index);
        return -1;
    }

    *handle = (vcdj_os_handle_t)index;
    return 0;
}

int vcdj_os_cond_destroy(vcdj_os_handle_t handle)
{
    if (!vcdj_handle_valid(handle, VCDJ_OS_COND_COUNT, &vcdj_cond_used))
    {
        return -1;
    }

    if (rt_sem_delete(vcdj_cond_pool[handle]) != RT_EOK)
    {
        return -1;
    }

    vcdj_cond_pool[handle] = RT_NULL;
    vcdj_pool_give(&vcdj_cond_used, (int)handle);
    return 0;
}

int vcdj_os_cond_wait(vcdj_os_handle_t cond, vcdj_os_handle_t mutex,
                      uint32_t timeout_ms)
{
    int ret;

    if (!vcdj_handle_valid(cond, VCDJ_OS_COND_COUNT, &vcdj_cond_used) ||
        !vcdj_handle_valid(mutex, VCDJ_OS_MUTEX_COUNT, &vcdj_mutex_used))
    {
        return -1;
    }

    /* RT-Thread exposes no condition-variable primitive. A binary semaphore
     * retains a signal that races the release-to-wait window, then the recursive
     * mutex is always reacquired before returning to the vendor predicate loop. */
    if (rt_mutex_release(vcdj_mutex_pool[mutex]) != RT_EOK)
    {
        return -1;
    }

    ret = (rt_sem_take(vcdj_cond_pool[cond],
                       vcdj_ticks_from_ms(timeout_ms)) == RT_EOK) ? 0 : -1;

    if (rt_mutex_take(vcdj_mutex_pool[mutex], RT_WAITING_FOREVER) != RT_EOK)
    {
        return -1;
    }

    return ret;
}

int vcdj_os_cond_signal(vcdj_os_handle_t handle)
{
    if (!vcdj_handle_valid(handle, VCDJ_OS_COND_COUNT, &vcdj_cond_used))
    {
        return -1;
    }

    /* A full binary semaphore already represents the pending wakeup. */
    (void)rt_sem_release(vcdj_cond_pool[handle]);
    return 0;
}

/*============================================================================*
 *                        OS primitives: semaphore
 *============================================================================*/
int vcdj_os_sem_create(vcdj_os_handle_t *handle, uint32_t initial_count)
{
    int index;

    if (handle == NULL)
    {
        return -1;
    }

    *handle = VCDJ_OS_HANDLE_INVALID;

    /* rt_sem_create() takes the initial count as rt_uint32_t but stores it in a
     * rt_uint16_t field, so a larger request cannot be honoured. */
    if (initial_count > (uint32_t)RT_SEM_VALUE_MAX)
    {
        return -1;
    }

    index = vcdj_pool_take(&vcdj_sem_used, VCDJ_OS_SEM_COUNT);
    if (index < 0)
    {
        return -1;
    }

    vcdj_sem_pool[index].handle = rt_sem_create("vcdj_sem", initial_count,
                                                RT_IPC_FLAG_PRIO);
    if (vcdj_sem_pool[index].handle == RT_NULL)
    {
        vcdj_pool_give(&vcdj_sem_used, index);
        return -1;
    }

    vcdj_sem_pool[index].count = initial_count;
    vcdj_sem_pool[index].max_count = (uint32_t)RT_SEM_VALUE_MAX;
    *handle = (vcdj_os_handle_t)index;
    return 0;
}

int vcdj_os_sem_destroy(vcdj_os_handle_t handle)
{
    if (!vcdj_handle_valid(handle, VCDJ_OS_SEM_COUNT, &vcdj_sem_used))
    {
        return -1;
    }

    if (rt_sem_delete(vcdj_sem_pool[handle].handle) != RT_EOK)
    {
        return -1;
    }

    vcdj_sem_pool[handle].handle = RT_NULL;
    vcdj_sem_pool[handle].count = 0U;
    vcdj_sem_pool[handle].max_count = 0U;
    vcdj_pool_give(&vcdj_sem_used, (int)handle);
    return 0;
}

int vcdj_os_sem_wait(vcdj_os_handle_t handle, uint32_t timeout_ms)
{
    rt_base_t level;

    if (!vcdj_handle_valid(handle, VCDJ_OS_SEM_COUNT, &vcdj_sem_used) ||
        (rt_sem_take(vcdj_sem_pool[handle].handle,
                     vcdj_ticks_from_ms(timeout_ms)) != RT_EOK))
    {
        return -1;
    }

    level = rt_hw_interrupt_disable();
    if (vcdj_sem_pool[handle].count > 0U)
    {
        vcdj_sem_pool[handle].count--;
    }
    rt_hw_interrupt_enable(level);

    return 0;
}

int vcdj_os_sem_post(vcdj_os_handle_t handle)
{
    rt_base_t level;

    if (!vcdj_handle_valid(handle, VCDJ_OS_SEM_COUNT, &vcdj_sem_used))
    {
        return -1;
    }

    level = rt_hw_interrupt_disable();
    if (vcdj_sem_pool[handle].count >= vcdj_sem_pool[handle].max_count)
    {
        rt_hw_interrupt_enable(level);
        return -1;
    }

    /* Reserve the count before publishing the RTOS token, so a concurrent take
     * can never observe and consume a token before the shadow count advances. */
    vcdj_sem_pool[handle].count++;
    rt_hw_interrupt_enable(level);

    if (rt_sem_release(vcdj_sem_pool[handle].handle) != RT_EOK)
    {
        level = rt_hw_interrupt_disable();
        vcdj_sem_pool[handle].count--;
        rt_hw_interrupt_enable(level);
        return -1;
    }

    return 0;
}

int vcdj_os_sem_count(vcdj_os_handle_t handle)
{
    rt_base_t level;
    uint32_t count;

    if (!vcdj_handle_valid(handle, VCDJ_OS_SEM_COUNT, &vcdj_sem_used))
    {
        return -1;
    }

    level = rt_hw_interrupt_disable();
    count = vcdj_sem_pool[handle].count;
    rt_hw_interrupt_enable(level);

    return (count > (uint32_t)INT32_MAX) ? INT32_MAX : (int)count;
}

/*============================================================================*
 *                        OS primitives: misc
 *============================================================================*/
void vcdj_os_delay_us(uint32_t us)
{
    if (us != 0U)
    {
        rt_hw_us_delay(us);
    }
}

/*============================================================================*
 *                        Console sinks
 *============================================================================*/
/*
 * Both sinks live here rather than in src/common/, which may not include an OS
 * header. Each renders into a local buffer first because ISO C cannot forward a
 * `va_list` to rt_kprintf(); rt_vsnprintf() is the kernel formatter, so no newlib
 * stdio is pulled back in.
 */

/**
 * \brief Driver-side console sink, see vcdj_plat_log() in rtl_vcdj_internal.h.
 *
 * rt_kprintf() writes straight to the console device, which is what the contract
 * asks for: a message must survive a fault right after it rather than sit in a
 * deferred buffer.
 */
void vcdj_plat_log(const char *fmt, ...)
{
    char line[VCDJ_CONSOLE_LINE_MAX];
    va_list args;

    if (fmt == NULL)
    {
        return;
    }

    va_start(args, fmt);
    (void)rt_vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    /* Callers supply their own "[vcdj]" tag. */
    rt_kprintf("%s", line);
}

#if defined(VCDJ_ENABLE_VENDOR_TRACE)

/** \brief Console sink for the vendor trace macros (see inc/rtl_vcdj_vendor.h). */
void vcdj_vendor_trace(const char *fmt, ...)
{
    char line[VCDJ_CONSOLE_LINE_MAX];
    va_list args;

    if (fmt == NULL)
    {
        return;
    }

    va_start(args, fmt);
    (void)rt_vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    /* Vendor messages carry no prefix of their own. */
    rt_kprintf("[vcdj-vendor] %s", line);
}

#endif /* VCDJ_ENABLE_VENDOR_TRACE */
