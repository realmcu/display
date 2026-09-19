/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef VCDJ_TEST_STREAM_H
#define VCDJ_TEST_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include <stdint.h>

#include "rtl_vcdj.h"

/** \defgroup VCDJ_TEST_STREAM   VCDJ test streams
  * \brief    Embedded bitstreams of the vendor release case set, plus the geometry
  *           each one is expected to parse to.
  *
  * The streams live in this repository rather than in the application, because the
  * bytes come from the VC9000DJ Control Software release archive and must stay on
  * the same side of the confidentiality boundary as src/ctrlsw/. An application
  * gets the whole table through \ref vcdj_test_streams and therefore embeds no
  * vendor data of its own.
  * \{
  */

/*============================================================================*
 *                         Exported Types
 *============================================================================*/

/**
 * \brief One test case: a bitstream plus what the header parse must report for it.
 *
 * `exp_width`, `exp_height` and `exp_bit_depth` are taken from the container
 * header of the stream itself (JPEG SOF, PNG IHDR), read out of the file, so they
 * are a real expectation and a mismatch is a real defect.
 *
 * `channels` and `color_type` are deliberately absent: how the vendor parser maps
 * a PNG colour type onto them has not been measured on this IP yet, and a guessed
 * expectation would turn into a false failure. A consumer prints those two and
 * judges only the three fields below.
 */
typedef struct
{
    const char   *name;            /**< Short case name, used on the console */
    const char   *vendor_case;     /**< Case ID in the vendor ReleaseCaseList.txt */
    const uint8_t *data;           /**< Bitstream bytes, in read-only memory */
    uint32_t      size;            /**< Length of \ref data in bytes */
    vcdj_codec_t  codec;           /**< Codec of \ref data */
    uint32_t      exp_width;       /**< Visible width the header parse must report */
    uint32_t      exp_height;      /**< Visible height the header parse must report */
    uint32_t      exp_bit_depth;   /**< Bit depth the header parse must report */
} vcdj_test_stream_t;

/*============================================================================*
 *                         Exported Variables
 *============================================================================*/

/** \brief Number of entries in \ref vcdj_test_streams. */
extern const uint32_t vcdj_test_stream_count;

/**
 * \brief The test case table.
 *
 * Ordered smallest picture first. Adding a stream means adding one array to
 * vcdj_test_stream.c and one row to this table; no consumer has to change.
 */
extern const vcdj_test_stream_t vcdj_test_streams[];

/** End of VCDJ_TEST_STREAM
  * \}
  */

#ifdef __cplusplus
}
#endif

#endif /* VCDJ_TEST_STREAM_H */
