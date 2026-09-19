/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RTL_VCDJ_VENDOR_H
#define RTL_VCDJ_VENDOR_H

/*============================================================================*
 *                  Private compile contract for the vendor CtrlSW
 *============================================================================*/
/*
 * This header is FORCE-INCLUDED in front of every VCDJ translation unit:
 *
 *   CMake / GCC / armclang : -include  inc/rtl_vcdj_vendor.h
 *   Keil MDK (armcc / AC5) : --preinclude rtl_vcdj_vendor.h
 *
 * It is force-included rather than patched into the vendor sources because the
 * tree under src/ctrlsw/ must stay byte identical to the release archive
 * (SHA256SUMS is verified). Keep it to macros and prototypes only.
 *
 * NDEBUG must be set before the libc block below includes <assert.h>.
 */
#define NON_PTHREAD_H
#define NDEBUG
#define FIFO_DATATYPE void *
#define TILE_8x8
#define USE_FAKE_RFC_TABLE
#define FAKE_RFC_TBL_LITTLE_ENDIAN

/*
 * Bring-up self test: fixed ON.
 *
 * Defined here so the driver 4 TUs always build it regardless of build system
 * (CMake / Keil). The Keil project equally carries it in the target-level
 * <Define> for Basic/src/ap/app_task.c, which does NOT go through this
 * force-included header; the #ifndef guard makes the two coexist without a
 * macro-redefined warning.
 */
#ifndef VCDJ_ENABLE_SELFTEST
#define VCDJ_ENABLE_SELFTEST
#endif

/* Keep the original ULTRA_SCALER path available for diagnostic builds. */
#ifndef VCDJ_ENABLE_ULTRA_SCALER
#define VCDJ_ENABLE_ULTRA_SCALER 0
#endif

/*
 * Vendor trace, OFF by default: without VCD_LOGMSG the five dec_log.h macros
 * below expand to `fprintf(stderr, ...)` at 302 call sites, costing ~38 KB of
 * newlib stdio for output nothing on this target reads. Define
 * VCDJ_ENABLE_VENDOR_TRACE for a diagnostic build. The off case is not
 * `((void)0)`: `sizeof` keeps the format checking and the argument evaluation
 * that discarding __VA_ARGS__ would lose, while generating no code.
 */
#ifdef __cplusplus
extern "C" {
#endif

#if defined(VCDJ_ENABLE_VENDOR_TRACE)
/** Console sink, implemented by the platform layer src/device/<soc>/. */
void vcdj_vendor_trace(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#define VCDJ_VENDOR_TRACE(...)      vcdj_vendor_trace(__VA_ARGS__)
#else
/* Never defined: unevaluated inside sizeof, so it resolves to no symbol. */
int vcdj_vendor_trace_check(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#define VCDJ_VENDOR_TRACE(...)      ((void)sizeof(vcdj_vendor_trace_check(__VA_ARGS__)))
#endif

#ifdef __cplusplus
}
#endif

/*
 * Include before overriding, or the #undefs hit macros that do not exist yet:
 * this header is force-included ahead of every vendor source. The `_DEC_LOG_H`
 * guard then makes the later in-source include a no-op, so these win.
 */
#include "dec_log.h"

#undef  APITRACEERR
#undef  APITRACEWARN
#undef  CONFIGTRACE_W
#undef  DPB_TRACE_E
#undef  DTRACE_E

#define APITRACEERR(...)            VCDJ_VENDOR_TRACE(__VA_ARGS__)
#define APITRACEWARN(...)           VCDJ_VENDOR_TRACE(__VA_ARGS__)
#define CONFIGTRACE_W(...)          VCDJ_VENDOR_TRACE(__VA_ARGS__)
#define DPB_TRACE_E(...)            VCDJ_VENDOR_TRACE(__VA_ARGS__)
#define DTRACE_E(...)               VCDJ_VENDOR_TRACE(__VA_ARGS__)

/*============================================================================*
 *                        Header Files
 *============================================================================*/
/*
 * Standard headers the vendor Control Software assumes are already in scope.
 *
 * On Linux they arrive transitively through `osal_linux.h`, which
 * `software/source/inc/osal.h` only includes when `__linux__` is defined. On a
 * bare-metal / RTOS target that block is skipped, so `ppu.c` (memmove/memset),
 * `jpegdecutils.c` (strcpy/strncpy/strcat/atoi), `sw_util.h` (rand) and
 * `regdrv.h` (assert) would all be compiled against implicit declarations.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

/**
 * `ppu.c` uses M_PI when it builds the Lanczos filter coefficients. M_PI is not
 * ISO C: newlib hides it under __STRICT_ANSI__, which -std=c99 turns on. The
 * vendor build works around this with -D_GNU_SOURCE; defining the value directly
 * has the same effect with a much smaller blast radius than switching the whole
 * library over to GNU libc feature macros.
 */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Vendor "unused variable" attribute.
 *
 * `software/source/inc/osal.h` defines this, but inside a
 * `#if defined(__FREERTOS__) || defined(_WIN32) || defined(__linux__)` block that
 * this build skips, while `errorhandling.c` uses it unconditionally
 * (`av_unused char buf[...]`). The definition below is textually identical to the
 * vendor one, so even if that block ever becomes active the redefinition is
 * benign.
 */
#ifndef av_unused
#if defined(__GNUC__) || defined(__clang__)
#define av_unused __attribute__((unused))
#else
#define av_unused
#endif
#endif

/**
 * \brief `usleep()` replacement reached through the vendor `osal.h` macro.
 *
 * `osal.h` unconditionally does `#define usleep osal_usleep`, but the block that
 * would declare the function is wrapped in the platform `#if` above, which this
 * build does not satisfy. `jpegdecapi.c` and `pngdecapi.c` do call `usleep()`, so
 * without this prototype the compiler emits -Wimplicit-function-declaration.
 *
 * Implemented in src/common/rtl_vcdj_dwl.c.
 */
int osal_usleep(unsigned long usec);

#ifdef __cplusplus
}
#endif

#endif /* RTL_VCDJ_VENDOR_H */
