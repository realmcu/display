/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_VCDJ_TRACE_H
#define RTL_VCDJ_TRACE_H

/*============================================================================*
 *                        Scope of this header
 *============================================================================*/
/**
 * RT-Thread stand-in for the Realtek SDK `trace.h`.
 *
 * `src/common/rtl_vcdj_hal.c` is shared verbatim between the two build
 * environments and reaches the platform log through `DBG_DIRECT()`:
 *
 *   Realtek RTOS SDK : trace.h ships with the SDK and DBG_DIRECT() goes to the
 *                      SDK trace path
 *   RT-Thread        : there is no SDK trace layer, so this header supplies the
 *                      same macro name on top of rt_kprintf()
 *
 * Providing the macro here rather than editing rtl_vcdj_hal.c is what keeps the
 * HAL a single source file across both platforms: this file is only on the
 * include path of the RT-Thread platform layer (src/device/rtl87x3k_rtt/), so the
 * SDK build never sees it and picks up the real SDK header instead.
 *
 * All 32 DBG_DIRECT() call sites in the HAL use plain ISO C format specifiers,
 * which rt_kprintf() accepts.
 */

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Macros
 *============================================================================*/
/**
 * \brief Unbuffered diagnostic print.
 *
 * rt_kprintf() writes straight to the console device, which matches what the
 * name DBG_DIRECT promises on the SDK side: the message must survive a fault
 * right after it, so it may not sit in a deferred trace buffer.
 */
#ifndef DBG_DIRECT
#define DBG_DIRECT(...)         rt_kprintf(__VA_ARGS__)
#endif

#ifdef __cplusplus
}
#endif

#endif /* RTL_VCDJ_TRACE_H */
