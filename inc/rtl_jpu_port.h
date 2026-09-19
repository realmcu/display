/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_JPU_PORT_H
#define RTL_JPU_PORT_H

/**
 * OS / SoC porting layer. The driver needs exactly three things from its
 * environment -- the CMSIS register qualifiers, a console sink and the clock
 * gate -- and they are all collected here, so nothing under src/device/
 * includes an OS header directly.
 *
 * Which OS is targeted cannot be detected from a predefined macro: SCons builds
 * define neither __RTTHREAD__ (xmake only) nor RT_THREAD, and __arm__ holds for
 * both. The build system must therefore state it, the way vcdj's vendor compile
 * contract is stated in SConscript and CMake:
 *
 *   JPU_OS_RTTHREAD  RT-Thread, built by SCons here
 *   JPU_OS_OSIF      FreeRTOS + OSIF on the Realtek RTOS SDK, built by Keil MDK
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(JPU_OS_RTTHREAD)

#include <rtthread.h>

#define DBG_DIRECT(...)             rt_kprintf(__VA_ARGS__)

/*
 * TODO(hardware-contract): the JPU clock gate owner on this FPGA build is not
 * confirmed, so no clock action is taken -- the clock is assumed to be always
 * on. Do NOT reuse a clock path from another IP to fill this in.
 */
#define JPU_PORT_CLK_ENABLE()       ((void)0)
#define JPU_PORT_CLK_DISABLE()      ((void)0)

#elif defined(JPU_OS_OSIF)

#include "rtl876x.h"
#include "rtl876x_rcc.h"
#include "trace.h"

#define JPU_PORT_CLK_ENABLE() \
    RCC_PeriphClockCmd(APBPeriph_JPEG, APBPeriph_JPEG_CLOCK, ENABLE)
#define JPU_PORT_CLK_DISABLE() \
    RCC_PeriphClockCmd(APBPeriph_JPEG, APBPeriph_JPEG_CLOCK, DISABLE)

#else
#error "Define JPU_OS_RTTHREAD or JPU_OS_OSIF; see rtl_jpu_port.h"
#endif

/*
 * Register qualifiers. This workspace ships no CMSIS core header, so they are
 * defined here for RT-Thread; on OSIF rtl876x.h has already provided them.
 */
#ifndef __IO
#define __IO    volatile
#endif
#ifndef __I
#define __I     volatile const
#endif
#ifndef __O
#define __O     volatile
#endif

#ifdef __cplusplus
}
#endif

#endif /* RTL_JPU_PORT_H */
