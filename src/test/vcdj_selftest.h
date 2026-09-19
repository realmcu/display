/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef VCDJ_SELFTEST_H
#define VCDJ_SELFTEST_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include <stdint.h>

#include "rtl_vcdj.h"

/** \defgroup VCDJ_SELFTEST   VCDJ bring-up self test
  * \brief    Test-only entry point that walks the whole vendor call path and
  *           reports each step over the console.
  *
  * Declared here rather than in `inc/rtl_vcdj.h` because it is not a driver
  * requirement: no product feature calls it, and the public header describes the
  * decode interface only. The implementation lives in `src/test/vcdj_selftest.c`
  * and reaches the vendor layer exclusively through the `vcdj_internal_*()`
  * contract in `src/common/rtl_vcdj_internal.h`.
  * \{
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
#if defined(VCDJ_ENABLE_SELFTEST)
/**
 * @brief      M1 self test: walk the whole vendor call path and report each step.
 *
 * @param[in]  jpeg      JPEG bitstream to feed the parser and the decoder.
 * @param[in]  jpeg_size Length of \p jpeg in bytes.
 *
 * @return     \ref VCDJ_SUCCESS only if a picture was actually produced.
 *             \ref VCDJ_ERR_TIMEOUT is the expected result while the hardware
 *             contract is open: it proves that the vendor code path is reachable,
 *             that the bounded wait works and that nothing spins forever.
 *
 * @note       Progress is printed through vcdj_plat_log(), implemented by the
 *             platform layer. The software header parse result is checked against
 *             the bitstream, which is the one piece of real functional
 *             verification possible at this stage.
 *
 * @note       Compiled while VCDJ_ENABLE_SELFTEST is defined. That macro is set
 *             unconditionally in rtl_vcdj_vendor.h during bring-up; remove it
 *             there (and from the Keil project <Define>) for a production image.
 */
vcdj_error_t hal_vcdj_selftest(const void *jpeg, uint32_t jpeg_size);
#endif

/** End of VCDJ_SELFTEST
  * \}
  */

#ifdef __cplusplus
}
#endif

#endif /* VCDJ_SELFTEST_H */
