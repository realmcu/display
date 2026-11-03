/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_VCDJ_CTRLSW_H
#define RTL_VCDJ_CTRLSW_H

/*============================================================================*
 *                        Header Files
 *============================================================================*/
/*
 * The vendor CtrlSW declarations the driver's own translation units use, from
 * the vendor headers or, when SConscript sets VCDJ_CTRLSW_MERGED, from the
 * generated vcdj_ctrlsw.h. The #else list is the generator's input: keep it to
 * plain #include lines.
 */
#if defined(VCDJ_CTRLSW_MERGED)
#include "../ctrlsw/vcdj_ctrlsw.h"
#else
#include "basetype.h"
#include "dwl.h"
#include "dwlthread.h"
#include "dectypes.h"
#include "decapicommon.h"
#include "sw_util.h"
#include "vpufeature.h"
#include "jpegdecapi.h"
#include "pngdecapi.h"
#endif

#endif /* RTL_VCDJ_CTRLSW_H */
