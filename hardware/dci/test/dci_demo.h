/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DCI application-level functional demo. Bare metal, no OS.
 *
 * ONE CASE PER BUILD. The case and preset are chosen in dci_demo_config.h and fixed at
 * compile time; only the selected case is compiled and only its buffers exist. See
 * that file for why, and test/README.md for the whole picture.
 *
 * Usage from the SDK's main(), once the clock and log UART are up:
 *
 *     #include "dci_demo.h"
 *
 *     int main(void)
 *     {
 *         ...
 *         dci_demo_run();          // runs whatever dci_demo_config.h selected
 *     }
 *
 * A case says what to exercise; a preset says how the hardware is wired. The two are
 * independent, so the same case covers every wiring variant in the preset table.
 */

#ifndef DCI_DEMO_H
#define DCI_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rtl_dci.h"
#include "dci_demo_id.h"
#include "dci_demo_config.h"
#include "dci_demo_preset.h"

/*============================================================================*
 *                         Fixed constants
 *============================================================================*/
/** \brief  Guard word written around every buffer to catch a DMA overrun. */
#define DCI_DEMO_GUARD              0xA5A5A5A5UL

/** \brief  Guard bytes per fence. Four words is enough to see a small overshoot. */
#define DCI_DEMO_GUARD_BYTES        16

/** \brief  Fill before capture: tells "nothing arrived" from "the data is zeros". */
#define DCI_DEMO_POISON             0xDEADBEEFUL

/*============================================================================*
 *                         Result
 *============================================================================*/
typedef enum
{
    DEMO_PASS = 0,
    DEMO_FAIL_BAD_PRESET,       /*!< Preset index out of range. */
    DEMO_FAIL_REG_DEAD,         /*!< Register block unresponsive: clock or base. */
    DEMO_FAIL_REG_BITS,         /*!< Register bits misbehave. */
    DEMO_FAIL_POOL,             /*!< Preset does not fit the framebuffer pool. */
    DEMO_FAIL_INIT,             /*!< DCI_Capture_Init() rejected the configuration. */
    DEMO_FAIL_GEOMETRY,         /*!< Driver and demo disagree on the frame size. */
    DEMO_FAIL_CROP,             /*!< Crop configuration rejected. */
    DEMO_FAIL_START,            /*!< DCI_Capture_Start() failed. */
    DEMO_FAIL_TIMEOUT,          /*!< No frame within DCI_DEMO_TIMEOUT_MS. */
    DEMO_FAIL_FRAME_LEN,        /*!< Delivered length is not the expected one. */
    DEMO_FAIL_TRUNCATED,        /*!< Frame arrived flagged truncated. */
    DEMO_FAIL_GUARD,            /*!< DMA wrote outside its buffer. */
    DEMO_FAIL_CONTENT,          /*!< Buffer still poison, or all bytes identical. */
    DEMO_FAIL_SEQ,              /*!< Sequence gapped or buffer index did not rotate. */
    DEMO_FAIL_STATS,            /*!< A counter that should be 0 is not. */
    DEMO_FAIL_NO_VSYNC,         /*!< No VSYNC seen: wiring, polarity or clock. */
    DEMO_FAIL_LINE_COUNT,       /*!< Measured line count disagrees with the preset. */
    DEMO_FAIL_STATE,            /*!< Driver state machine did not go where expected. */
} dci_demo_result_t;

/*============================================================================*
 *                         Entry point
 *============================================================================*/
/**
 * \brief   Run the case selected in dci_demo_config.h.
 *
 * Prints what it is about to do, runs it, and prints the verdict. On failure the
 * reason is logged along with a register dump, if DCI_DEBUG_REG_DUMP is enabled.
 *
 * \return  DEMO_PASS, or the first failure encountered.
 */
dci_demo_result_t dci_demo_run(void);

/** \brief  Result name for logs. */
const char *dci_demo_result_name(dci_demo_result_t r);

#ifdef __cplusplus
}
#endif

#endif /* DCI_DEMO_H */
