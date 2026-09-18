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
 * \brief   Pool, Capture_Init, crop, and the geometry cross-check.
 *
 * \param[out] frame_bytes_out  Expected frame size, 0 for JPEG.
 */
dci_demo_result_t dci_demo_setup(const dci_demo_preset_t *p, uint8_t buf_count,
                                 DCI_ON_BUSY_T on_busy, DCI_NOTIFY_MODE_T notify,
                                 uint32_t *frame_bytes_out);

void              dci_demo_teardown(void);
dci_demo_result_t dci_demo_check_stats(uint32_t expect_frames);

/**
 * \brief   Buffer count for this build.
 *
 * DCI_DEMO_BUF_COUNT from the config, unless the selected case pins it because the
 * behaviour under test needs a particular value.
 */
uint8_t dci_demo_buf_count_cfg(void);

/*============================================================================*
 *                         Driver callbacks
 *============================================================================*/
void dci_demo_on_frame_ready(const DCI_frame_t *frame);
void dci_demo_on_frame_error(DCI_ERROR err, uint32_t ris);

/*============================================================================*
 *                         Cases
 *============================================================================*/
dci_demo_result_t dci_demo_case_reg_test(const dci_demo_preset_t *p);
dci_demo_result_t dci_demo_case_preset_dump(const dci_demo_preset_t *p);
dci_demo_result_t dci_demo_case_config_only(const dci_demo_preset_t *p);
dci_demo_result_t dci_demo_case_probe_signal(const dci_demo_preset_t *p);
dci_demo_result_t dci_demo_case_snapshot(const dci_demo_preset_t *p);
dci_demo_result_t dci_demo_case_snapshot_x2(const dci_demo_preset_t *p);
dci_demo_result_t dci_demo_case_continuous(const dci_demo_preset_t *p);
dci_demo_result_t dci_demo_case_continuous_1buf(const dci_demo_preset_t *p);
dci_demo_result_t dci_demo_case_reconfig(const dci_demo_preset_t *p);
dci_demo_result_t dci_demo_case_suspend(const dci_demo_preset_t *p);
dci_demo_result_t dci_demo_case_stress(const dci_demo_preset_t *p);

/** \brief  EDM=10BIT packing, per sample. Requires a RAW10 preset. */
dci_demo_result_t dci_demo_case_raw10(const dci_demo_preset_t *p);

#ifdef __cplusplus
}
#endif

#endif /* DCI_DEMO_CASE_H */
