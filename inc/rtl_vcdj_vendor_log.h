/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RTL_VCDJ_VENDOR_LOG_H
#define RTL_VCDJ_VENDOR_LOG_H

/*
 * Redirects the five dec_log.h macros that expand to fprintf(stderr, ...) to
 * VCDJ_VENDOR_TRACE() from rtl_vcdj_vendor.h. Must come right after dec_log.h:
 * placed before it, the #undefs hit nothing and dec_log.h restores the macros.
 */
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

#endif /* RTL_VCDJ_VENDOR_LOG_H */
