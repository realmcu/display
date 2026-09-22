/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DCI_DEMO_CASE_H
#define DCI_DEMO_CASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dci_demo.h"

/*============================================================================*
 *                         Shared setup, from dci_demo.c
 *============================================================================*/
/**
 * \brief   Pool, Capture_Init, crop, interrupt install, and the geometry cross-check.
 *
 * Takes a resolved configuration rather than a signal row: the overlay has already been
 * applied and the geometry derived, so there is exactly one place the numbers come from.
 *
 * \param[out] frame_bytes_out  Expected frame size, 0 for JPEG.
 */
dci_demo_result_t dci_demo_setup(const dci_demo_cfg_t *c, uint8_t buf_count,
                                 DCI_ON_BUSY_T on_busy, DCI_NOTIFY_MODE_T notify,
                                 uint32_t *frame_bytes_out);

void              dci_demo_teardown(void);
dci_demo_result_t dci_demo_check_stats(uint32_t expect_frames);

/**
 * \brief   Buffer count for this build: DCI_DEMO_BUF_COUNT.
 *
 * Nothing pins it any more - what used to need a dedicated case is a macro combination,
 * and dci_demo_config.h rejects the ones that cannot work.
 */
uint8_t dci_demo_buf_count_cfg(void);

/**
 * \brief   Buffer fill policy for this build.
 *
 * DCI_DEMO_FILL_MODE translated to the driver's enum.
 */
DCI_ON_BUSY_T dci_demo_on_busy_cfg(void);

/*============================================================================*
 *                         Cases
 *============================================================================*/
/** \brief  Register read/write self test. No camera, no DMA, no pins. */
dci_demo_result_t dci_demo_case_reg_test(const dci_demo_cfg_t *c);

/**
 * \brief   DCI_DEMO_SHOTS snapshots, fully checked, address printed for a JLink dump.
 *
 * DCI_DEMO_NO_CAPTURE = 1 configures and dumps registers without capturing, so it needs
 * no camera. DCI_DEMO_SHOTS = 2 (with 2 buffers) is the rotation check.
 */
dci_demo_result_t dci_demo_case_snapshot(const dci_demo_cfg_t *c);

/**
 * \brief   DCI_DEMO_FRAMES frames through the closed ring.
 *
 * DCI_DEMO_CAPTURE_FRAME = N instead keeps frame N in RAM for a dump.
 */
dci_demo_result_t dci_demo_case_continuous(const dci_demo_cfg_t *c);

#ifdef __cplusplus
}
#endif

#endif /* DCI_DEMO_CASE_H */
