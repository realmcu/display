/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *                        Header Files
 *============================================================================*/
/*
 * PLATFORM LAYER: the only translation unit that touches OSIF and the SoC.
 *
 * Implements both halves of the platform contract declared in
 * rtl_vcdj_internal.h:
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
 * friends; keeping them out of this translation unit is what lets the OS headers
 * be included at all.
 *
 * The two halves are in one file because OSIF is macro-clean: all 124
 * identifiers `dwlthread.h` defines under NON_PTHREAD_H were checked against
 * os_sync.h / os_sched.h / os_mem.h / utils.h and the CMSIS headers, and the only
 * textual match was the English word "up" inside a comment. A Zephyr port could
 * not do this - `atomic_t` and `EINVAL` collide with `<zephyr/kernel.h>` - and
 * would have to split the OS primitives back into their own file.
 *
 * SoC SCOPE: the platform ops and the OS primitives here are OS-specific but
 * SoC-agnostic; only rtl_vcdj_soc_cfg.h is per-SoC. With a single FreeRTOS SoC
 * supported there is no reason to split them, but if a second FreeRTOS SoC
 * appears, move this file to src/device/rtl_common/ (the convention the other
 * display IPs use) and leave rtl_vcdj_soc_cfg.h in each SoC directory. That
 * migration is a file move, not a code change.
 */
#include <stddef.h>
#include <stdint.h>

#include "cmsis_compiler.h"
#include "os_sched.h"
#include "os_sync.h"
#include "utils.h"

#include "rtl_vcdj_internal.h"
#include "rtl_vcdj_soc_cfg.h"

/*============================================================================*
 *                        Local Definitions
 *============================================================================*/
#ifndef VCDJ_HW_TIMEOUT_MS
#define VCDJ_HW_TIMEOUT_MS      2000U
#endif

/** Guard word placed in front of every aligned allocation. */
#define VCDJ_ALLOC_MAGIC        0x56434A41U     /* 'VCJA' */

/*
 * Object pool sizing for the JPEG + PNG closure that is actually compiled:
 *   mutexes    input_queue (cs, cnt_release, buf_release) = 3 per queue,
 *              jpeg/png fb_list (out_count, ref_count)    = 2,
 *              dec_log log_mutex                          = 1 (VCD_LOGMSG off)
 *   condvars   input_queue (cnt_release_cv, buf_release_cv) = 2 per queue,
 *              jpeg/png fb_list hw_rdy_cv                   = 1
 *   semaphores fifo (cs, read, write)                       = 3 per fifo,
 *              fb_list out_count_sem, sem_ri_mc_done        = 2
 *
 * The defaults are Kconfig tunable and exhaustion is reported as a hard error
 * rather than silently ignored. The 32 ceiling comes from the allocation bitmaps
 * being a single uint32_t each.
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
    void *handle;
    uint32_t count;
    uint32_t max_count;
};

/*============================================================================*
 *                        Local Variables
 *============================================================================*/
static void *vcdj_hw_lock;

static void *vcdj_mutex_pool[VCDJ_OS_MUTEX_COUNT];
static void *vcdj_cond_pool[VCDJ_OS_COND_COUNT];
static struct vcdj_os_sem_slot vcdj_sem_pool[VCDJ_OS_SEM_COUNT];

static uint32_t vcdj_mutex_used;
static uint32_t vcdj_cond_used;
static uint32_t vcdj_sem_used;

/*============================================================================*
 *                        Local Functions: object pools
 *============================================================================*/
static int vcdj_pool_take(uint32_t *bitmap, int count)
{
    uint32_t flags;
    int i;

    flags = os_lock();

    for (i = 0; i < count; i++)
    {
        if ((*bitmap & (1UL << i)) == 0U)
        {
            *bitmap |= (1UL << i);
            os_unlock(flags);
            return i;
        }
    }

    os_unlock(flags);
    return -1;
}

static void vcdj_pool_give(uint32_t *bitmap, int index)
{
    uint32_t flags = os_lock();

    *bitmap &= ~(1UL << index);

    os_unlock(flags);
}

static int vcdj_handle_valid(vcdj_os_handle_t handle, int count,
                             const uint32_t *bitmap)
{
    uint32_t flags;
    int valid;

    if ((handle < 0) || (handle >= count))
    {
        return 0;
    }

    flags = os_lock();
    valid = ((*bitmap & (1UL << (uint32_t)handle)) != 0U);
    os_unlock(flags);

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

    raw = vcdj_mem_alloc(size + align + sizeof(struct vcdj_alloc_hdr));
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
    __DMB();
#endif
}

static void vcdj_plat_cache_invalidate(void *ctx, void *addr, size_t size)
{
    (void)ctx;
    (void)addr;
    (void)size;

#if !VCDJ_SOC_DMA_COHERENT
    /* TODO(hardware-contract): see vcdj_plat_cache_clean(). */
    __DMB();
#endif
}

static void vcdj_plat_memory_barrier(void *ctx)
{
    (void)ctx;
    __DMB();
}

/*============================================================================*
 *                        Platform ops: locking
 *============================================================================*/
static int vcdj_plat_sync_init(void)
{
    if (vcdj_hw_lock != NULL)
    {
        return 0;
    }

    if (!os_mutex_create(&vcdj_hw_lock))
    {
        return -1;
    }

    return 0;
}

static int vcdj_plat_lock(void *ctx, uint32_t timeout_ms)
{
    (void)ctx;

    if (vcdj_hw_lock == NULL)
    {
        return -1;
    }

    return os_mutex_take(vcdj_hw_lock, timeout_ms) ? 0 : -1;
}

static void vcdj_plat_unlock(void *ctx)
{
    (void)ctx;

    if (vcdj_hw_lock != NULL)
    {
        (void)os_mutex_give(vcdj_hw_lock);
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
    (void)ctx;

#if VCDJ_SOC_HAS_RESET_CONTROL
#error "VCDJ_SOC_HAS_RESET_CONTROL is set but no reset sequence is implemented"
#else
    /* TODO(hardware-contract): reset controller and ordering are unknown. */
    return 0;
#endif
}

static uint32_t vcdj_plat_get_time_ms(void *ctx)
{
    (void)ctx;
    return (uint32_t)os_sys_time_get();
}

static void vcdj_plat_delay_us(void *ctx, uint32_t us)
{
    (void)ctx;

    if (us != 0U)
    {
        platform_delay_us(us);
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

    if (!os_mutex_create(&vcdj_mutex_pool[index]))
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

    if (!os_mutex_delete(vcdj_mutex_pool[handle]))
    {
        return -1;
    }

    vcdj_mutex_pool[handle] = NULL;
    vcdj_pool_give(&vcdj_mutex_used, (int)handle);
    return 0;
}

int vcdj_os_mutex_lock(vcdj_os_handle_t handle, uint32_t timeout_ms)
{
    if (!vcdj_handle_valid(handle, VCDJ_OS_MUTEX_COUNT, &vcdj_mutex_used))
    {
        return -1;
    }

    return os_mutex_take(vcdj_mutex_pool[handle], timeout_ms) ? 0 : -1;
}

int vcdj_os_mutex_unlock(vcdj_os_handle_t handle)
{
    if (!vcdj_handle_valid(handle, VCDJ_OS_MUTEX_COUNT, &vcdj_mutex_used))
    {
        return -1;
    }

    return os_mutex_give(vcdj_mutex_pool[handle]) ? 0 : -1;
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

    /* OSIF has no condition variable. A binary semaphore preserves one pending
     * wakeup and lets wait release/reacquire the associated mutex explicitly. */
    if (!os_sem_create(&vcdj_cond_pool[index], "vcdj_cond", 0U, 1U))
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

    if (!os_sem_delete(vcdj_cond_pool[handle]))
    {
        return -1;
    }

    vcdj_cond_pool[handle] = NULL;
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

    /* OSIF exposes no condition-variable primitive. A binary semaphore retains
     * a signal that races the release-to-wait window, then the recursive mutex is
     * always reacquired before returning to the vendor predicate loop. */
    if (!os_mutex_give(vcdj_mutex_pool[mutex]))
    {
        return -1;
    }

    ret = os_sem_take(vcdj_cond_pool[cond], timeout_ms) ? 0 : -1;

    if (!os_mutex_take(vcdj_mutex_pool[mutex], VCDJ_OS_WAIT_FOREVER))
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
    (void)os_sem_give(vcdj_cond_pool[handle]);
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
    index = vcdj_pool_take(&vcdj_sem_used, VCDJ_OS_SEM_COUNT);
    if (index < 0)
    {
        return -1;
    }

    if (!os_sem_create(&vcdj_sem_pool[index].handle, "vcdj_sem",
                       initial_count, (uint32_t)INT32_MAX))
    {
        vcdj_pool_give(&vcdj_sem_used, index);
        return -1;
    }

    vcdj_sem_pool[index].count = initial_count;
    vcdj_sem_pool[index].max_count = (uint32_t)INT32_MAX;
    *handle = (vcdj_os_handle_t)index;
    return 0;
}

int vcdj_os_sem_destroy(vcdj_os_handle_t handle)
{
    if (!vcdj_handle_valid(handle, VCDJ_OS_SEM_COUNT, &vcdj_sem_used))
    {
        return -1;
    }

    if (!os_sem_delete(vcdj_sem_pool[handle].handle))
    {
        return -1;
    }

    vcdj_sem_pool[handle].handle = NULL;
    vcdj_sem_pool[handle].count = 0U;
    vcdj_sem_pool[handle].max_count = 0U;
    vcdj_pool_give(&vcdj_sem_used, (int)handle);
    return 0;
}

int vcdj_os_sem_wait(vcdj_os_handle_t handle, uint32_t timeout_ms)
{
    uint32_t flags;

    if (!vcdj_handle_valid(handle, VCDJ_OS_SEM_COUNT, &vcdj_sem_used) ||
        !os_sem_take(vcdj_sem_pool[handle].handle, timeout_ms))
    {
        return -1;
    }

    flags = os_lock();
    if (vcdj_sem_pool[handle].count > 0U)
    {
        vcdj_sem_pool[handle].count--;
    }
    os_unlock(flags);

    return 0;
}

int vcdj_os_sem_post(vcdj_os_handle_t handle)
{
    uint32_t flags;

    if (!vcdj_handle_valid(handle, VCDJ_OS_SEM_COUNT, &vcdj_sem_used))
    {
        return -1;
    }

    flags = os_lock();
    if (vcdj_sem_pool[handle].count >= vcdj_sem_pool[handle].max_count)
    {
        os_unlock(flags);
        return -1;
    }

    /* Reserve the count before publishing the OSIF token, so a concurrent take
     * can never observe and consume a token before the shadow count advances. */
    vcdj_sem_pool[handle].count++;
    os_unlock(flags);

    if (!os_sem_give(vcdj_sem_pool[handle].handle))
    {
        flags = os_lock();
        vcdj_sem_pool[handle].count--;
        os_unlock(flags);
        return -1;
    }

    return 0;
}

int vcdj_os_sem_count(vcdj_os_handle_t handle)
{
    uint32_t flags;
    uint32_t count;

    if (!vcdj_handle_valid(handle, VCDJ_OS_SEM_COUNT, &vcdj_sem_used))
    {
        return -1;
    }

    flags = os_lock();
    count = vcdj_sem_pool[handle].count;
    os_unlock(flags);

    return (count > (uint32_t)INT32_MAX) ? INT32_MAX : (int)count;
}

/*============================================================================*
 *                        OS primitives: misc
 *============================================================================*/
void vcdj_os_delay_us(uint32_t us)
{
    if (us != 0U)
    {
        platform_delay_us(us);
    }
}
