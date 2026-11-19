/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_VCDJ_INTERNAL_H
#define RTL_VCDJ_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include <stdint.h>
#include <stddef.h>

#include "rtl_vcdj.h"

/*============================================================================*
 *                        Scope of this header
 *============================================================================*/
/**
 * Driver internal contracts. NOT part of the exported interface: only
 * src/common/rtl_vcdj.c, the platform layer and the self test include it.
 *
 * It carries three contracts:
 *
 *   1. \ref vcdj_platform_ops / \ref vcdj_platform_config
 *      what the platform layer (src/device/<soc>/) provides.
 *   2. `vcdj_mem_*()` / `vcdj_core_*()`
 *      the few driver services the platform layer and the tests call back into.
 *   3. `vcdj_os_*()`
 *      OS primitives the vendor OS bridge needs.
 *
 * ONE HARD RULE APPLIES: this header must stay on plain `<stdint.h>` types. No
 * OS type (`k_sem`, `struct device`, OSIF handle) and no vendor Control Software
 * type (`DWL*`, `JpegDec*`, `PngDec*`) may appear here, because it is included
 * both by vendor facing translation units (which pull in `dwlthread.h` and its
 * Linux-kernel-ism macros) and by the platform layer (which pulls in OSIF and
 * CMSIS headers).
 *
 * The two sets DO collide: the vendor headers define EINVAL/EBUSY/EFAULT/ENOTTY
 * with values that differ from newlib's <errno.h>, which RT-Thread pulls in,
 * and Zephyr adds `atomic_t`. Whichever header comes last wins silently, so a
 * vendor facing file and an OS facing file must never be the same translation
 * unit; this header is the only thing they share.
 */

/*============================================================================*
 *                        Platform contract
 *============================================================================*/
/**
 * \brief Returned by an operation the platform cannot provide in this build.
 *
 * Distinct from a plain -1 failure so the caller can choose a different strategy
 * instead of reporting an error: \ref vcdj_platform_ops.wait_done uses it to say
 * "there is no interrupt here, poll instead".
 */
#define VCDJ_PLAT_ENOTSUP               (-2)

/**
 * \brief Narrow set of platform services the platform independent layer needs.
 *
 * All callbacks receive the opaque \p ctx that was passed in
 * \ref vcdj_platform_config.ctx. A NULL callback means "not available"; the
 * core layer must degrade gracefully rather than dereference it.
 */
struct vcdj_platform_ops
{
    /* --- register access, relative to the decoder core register window --- */
    uint32_t (*reg_read32)(void *ctx, uint32_t offset);
    void (*reg_write32)(void *ctx, uint32_t offset, uint32_t value);

    /* --- DMA capable memory ---------------------------------------------- */
    /** Allocate \p size bytes aligned to \p align, returning both the CPU
      * pointer and the bus address the decoder core must be programmed with. */
    int (*dma_alloc)(void *ctx, size_t size, size_t align,
                     void **cpu_addr, uintptr_t *bus_addr);
    void (*dma_free)(void *ctx, void *cpu_addr, uintptr_t bus_addr, size_t size);

    /* --- cache maintenance ----------------------------------------------- */
    void (*cache_clean)(void *ctx, void *addr, size_t size);
    void (*cache_invalidate)(void *ctx, void *addr, size_t size);
    void (*memory_barrier)(void *ctx);

    /* --- mutual exclusion and completion --------------------------------- */
    int (*lock)(void *ctx, uint32_t timeout_ms);
    void (*unlock)(void *ctx);
    /** Block until the core signals completion.
      *
      * \return 0 on completion, \ref VCDJ_PLAT_ENOTSUP when this build has no
      *         interrupt to wait on (the core layer then polls), negative
      *         otherwise. May be NULL in a polling only build. */
    int (*wait_done)(void *ctx, uint32_t timeout_ms);
    /** Re-arm the completion object before the core is started. */
    void (*wait_reset)(void *ctx);

    /* --- clock / reset ---------------------------------------------------- */
    int (*clock_enable)(void *ctx);
    void (*clock_disable)(void *ctx);
    int (*reset)(void *ctx);

    /* --- time ------------------------------------------------------------- */
    uint32_t (*get_time_ms)(void *ctx);
    void (*delay_us)(void *ctx, uint32_t us);
};

/**
 * \brief Static description of one decoder instance on a given SoC.
 *
 * Populated by `src/device/<soc>/` and bound in `vcdj_init()`
 * (compile time static ops, no DTS device model in the first version).
 */
struct vcdj_platform_config
{
    const struct vcdj_platform_ops *ops;
    void       *ctx;               /**< Opaque cookie handed back to every op */
    uintptr_t   reg_base;          /**< Subsystem MMIO base, for diagnostics only */
    uint32_t    core_offset;       /**< Byte offset of the decoder core inside the subsystem */
    uint32_t    reg_window_size;   /**< Size of the core register window in bytes */
    uint32_t    core_count;        /**< Number of decoder cores; 1 in the first version */
    uint32_t    hw_timeout_ms;     /**< Upper bound applied to every hardware wait */
};

/**
 * \brief Return the platform configuration of this build.
 *
 * Implemented by `src/device/<soc>/rtl_vcdj_port.c`. Returns NULL when the
 * platform layer is not available in the current configuration.
 */
const struct vcdj_platform_config *vcdj_platform_get_config(void);

/*============================================================================*
 *                        Diagnostic console
 *============================================================================*/
/**
 * \brief Unbuffered diagnostic print, implemented by `src/device/<soc>/`.
 *
 * The one log seam of this driver, so that no file outside the platform layer
 * needs an OS header: RT-Thread reaches rt_kprintf(), the Realtek SDK port
 * reaches DBG_DIRECT(). Must write straight to the console rather than a
 * deferred buffer, because a message has to survive a fault right after it.
 *
 * Only `src/test/vcdj_selftest.c` calls this; src/common/rtl_vcdj.c emits no log
 * at all.
 */
void vcdj_plat_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/*============================================================================*
 *                        Driver services
 *============================================================================*/
/**
 * \brief Allocate through the allocator injected by vcdj_mem_init().
 *
 * The platform layer's `dma_alloc`/`dma_free` build on this pair. Returns NULL
 * when no allocator is set; never falls back to libc.
 */
void *vcdj_mem_alloc(size_t size);

/** \brief Release memory obtained from \ref vcdj_mem_alloc. */
void vcdj_mem_free(void *ptr);

/** \brief Flush CPU writes so the core sees the buffer contents. */
void vcdj_core_cache_clean(void *addr, size_t size);

/**
 * \brief Start folding every register write into a CRC-32.
 *
 * Covers (window offset, value) of each write in order, so two builds that
 * program the core differently disagree even when their pixels match.
 * Test instrumentation: not thread safe, not part of rtl_vcdj.h.
 */
void vcdj_core_trace_start(void);

/** \brief Stop the trace; \p crc and \p count receive the CRC and write count. */
void vcdj_core_trace_stop(uint32_t *crc, uint32_t *count);

/*============================================================================*
 *                        OS primitive contract
 *============================================================================*/
/**
 * Narrow OS primitive contract used by the vendor OS bridge in
 * `src/common/rtl_vcdj.c` and implemented in
 * `src/device/<soc>/rtl_vcdj_port.c`.
 *
 * Handles are small integers rather than pointers on purpose: the vendor objects
 * they have to be stored in are tiny (`Semaphore` is a single `i32`), so there is
 * no room to embed an OS object or even a pointer portably.
 */

/** \brief Opaque OS object handle. Negative values are invalid. */
typedef int32_t vcdj_os_handle_t;

/** \brief Value stored in vendor structures for "no object". */
#define VCDJ_OS_HANDLE_INVALID          ((vcdj_os_handle_t) - 1)

/** \brief Wait without a bound. */
#define VCDJ_OS_WAIT_FOREVER            0xFFFFFFFFU

/* --- mutex ---------------------------------------------------------------- */
int vcdj_os_mutex_create(vcdj_os_handle_t *handle);
int vcdj_os_mutex_destroy(vcdj_os_handle_t handle);
int vcdj_os_mutex_lock(vcdj_os_handle_t handle, uint32_t timeout_ms);
int vcdj_os_mutex_unlock(vcdj_os_handle_t handle);

/* --- condition variable --------------------------------------------------- */
int vcdj_os_cond_create(vcdj_os_handle_t *handle);
int vcdj_os_cond_destroy(vcdj_os_handle_t handle);
/** Atomically release \p mutex, wait for a signal, re-acquire \p mutex. */
int vcdj_os_cond_wait(vcdj_os_handle_t cond, vcdj_os_handle_t mutex, uint32_t timeout_ms);
int vcdj_os_cond_signal(vcdj_os_handle_t handle);

/* --- counting semaphore --------------------------------------------------- */
int vcdj_os_sem_create(vcdj_os_handle_t *handle, uint32_t initial_count);
int vcdj_os_sem_destroy(vcdj_os_handle_t handle);
int vcdj_os_sem_wait(vcdj_os_handle_t handle, uint32_t timeout_ms);
int vcdj_os_sem_post(vcdj_os_handle_t handle);
/** \return current count, or a negative value on error. */
int vcdj_os_sem_count(vcdj_os_handle_t handle);

/* --- misc ----------------------------------------------------------------- */
/** Busy/sleep wait, used by the `usleep` shim the vendor OSAL expects. */
void vcdj_os_delay_us(uint32_t us);

/*============================================================================*
 *                        Cross-file driver internals
 *============================================================================*/
/**
 * Shared between `rtl_vcdj.c` and the bring-up self test in
 * `src/test/vcdj_selftest.c`. Deliberately uses `void *` rather than a vendor
 * type so this header stays vendor-header-free - which is also what lets the
 * self test live in its own translation unit without duplicating any of the
 * facade's internals.
 */

/*
 * Upper bound on the output buffers one decode registers with the vendor.
 *
 * Here rather than in rtl_vcdj.c because it is the `capacity` argument of
 * vcdj_internal_add_jpeg_buffers() below: both callers size their `buffers[]`
 * array with it, so a copy per file could drift and overflow the array the
 * helper writes into.
 *
 * NOT a guess: both codecs hardcode a single output buffer -
 * `JpegSetExternalBufferInfo()` and `PngSetExternalBufferInfo()` each open with
 * `u32 buffers = 1` and never raise it, so `DecBufferInfo.buf_num` is always 1
 * for this Control Software drop. Two slots are kept rather than one because the
 * re-allocation protocol hands back the old buffer while asking for a bigger one,
 * so both can be live for the duration of one negotiation round.
 *
 * The vendor's own ceiling is `MAX_PIC_BUFFERS` (34, see dwl.h); exceeding it is
 * reported as `DEC_EXT_BUFFER_REJECTED`. We stay far below it deliberately -
 * this driver decodes one still picture at a time.
 */
#define VCDJ_MAX_EXTERNAL_BUFFERS       2U

/*
 * Bound on the `Decode -> DEC_WAITING_FOR_BUFFER -> AddBuffer -> Decode` rounds.
 *
 * Three is derived, not picked: with `use_adaptive_buffers == 0` (our config)
 * the JPEG path returns DEC_WAITING_FOR_BUFFER once because the picture geometry
 * differs from the previous one - and `prev_display_width` starts at 0, so the
 * FIRST picture always trips it (jpegdecapi.c, the `prev_display_width !=
 * display_width` branch). A second round can follow if the reported size grew.
 * Three leaves one spare round; a fourth would mean the negotiation is not
 * converging, which is a defect to surface rather than to keep retrying.
 */
#define VCDJ_MAX_BUFFER_NEGOTIATIONS    3U

/** \brief Active DWL instance, or NULL before vcdj_init(). */
const void *vcdj_internal_dwl(void);

/**
 * \brief Buffer geometry the vendor external-buffer negotiation reported.
 *
 * Everything `DecBufferInfo` says about the output buffer the decoder asked for,
 * expressed without a vendor type so this header stays vendor-header-free.
 *
 * \warning A plane is produced if and only if its \ref plane_size is non-zero.
 *          Do NOT infer this from the plane addresses instead: for a packed RGB
 *          output the vendor still computes a chroma address as
 *          `base + chroma_offset`, and since `chroma_offset == luma_size` while
 *          the chroma size is zero, that address points one byte past the end of
 *          the buffer. Treating it as a plane would hand the caller memory it
 *          does not own.
 */
struct vcdj_buffer_layout
{
    unsigned long long total_size;                  /**< Whole buffer, every plane included */
    unsigned long long plane_size[VCDJ_MAX_PLANES]; /**< Usable bytes per plane; 0 = absent */
    uint32_t ystride;                               /**< Luma line pitch in bytes */
    uint32_t cstride;                               /**< Chroma line pitch in bytes */
};

/**
 * \brief Run one JPEG decode state-machine step.
 *
 * The arguments are opaque here so this internal header remains free of vendor
 * types. The caller passes JpegDecInput and JpegDecOutput objects.
 */
int vcdj_internal_jpeg_decode(void *inst, void *input, void *output);

/**
 * \brief Allocate and register all external buffers currently requested by a
 *        JPEG decoder instance.
 *
 * The returned buffers remain owned by the caller and must be released with
 * vcdj_internal_free_external_buffers() after the decoder instance is released.
 */
int vcdj_internal_add_jpeg_buffers(void *inst, void **buffers,
                                   unsigned int capacity,
                                   unsigned int *count,
                                   struct vcdj_buffer_layout *layout);

/** \brief Release buffers allocated by either external-buffer helper. */
void vcdj_internal_free_external_buffers(void **buffers, unsigned int count);

#ifdef __cplusplus
}
#endif

#endif /* RTL_VCDJ_INTERNAL_H */
