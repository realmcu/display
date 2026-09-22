/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DCI_DEMO_BUF_H
#define DCI_DEMO_BUF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rtl_dci.h"
#include "dci_demo_signal.h"

/** \brief  Carve the pool into count buffers of bytes each. false if it will not fit. */
bool dci_demo_buf_alloc(uint32_t bytes, uint8_t count);

uint32_t *dci_demo_buf_get(uint8_t index);
uint32_t  dci_demo_buf_bytes(void);
uint8_t   dci_demo_buf_count(void);

/** \brief  Fill every buffer with DCI_DEMO_POISON before a capture. */
void dci_demo_buf_poison(void);

/** \brief  Check the guard fence around every buffer. */
bool dci_demo_buf_guards_ok(void);

/** \brief  Reject a buffer that is still poison or holds one repeated value. */
bool dci_demo_buf_has_data(uint8_t index, uint32_t bytes);

/** \brief  Report a frame. Level 0 = summary + CRC, 1 = samples, 2 = full hex. */
void dci_demo_buf_dump(const DCI_frame_t *f, const dci_demo_cfg_t *c, uint8_t level);

/**
 * \brief   Print the address and byte range of a buffer, ready for a JLink dump.
 *
 * The end of the RECEIVE case: the data is in RAM, capture has stopped, and the only
 * thing left to say is where to look. Prints the savebin command line as well, because
 * getting the length argument wrong is the usual way a dump comes back short.
 *
 * \param[in] f  The held frame, or NULL to describe every allocated buffer.
 */
void dci_demo_buf_report_range(const DCI_frame_t *f);

/** \brief  Verify JPEG SOI/EOI framing, tolerating the hardware's word padding. */
bool dci_demo_buf_jpeg_ok(const DCI_frame_t *f);

/**
 * \brief   Count samples whose EDM=10BIT padding bits (15:10) are not clear.
 *
 * The only per-sample invariant in the table that holds against an arbitrary scene,
 * so it is checked exhaustively rather than sampled. The count is returned - not just
 * a bool - because its ratio to the sample total identifies the fault: all samples
 * means EDM never took effect, every second sample means the half-word phase is off.
 *
 * \param[in] report_max  Stop listing individual samples after this many.
 *
 * \return  Number of bad samples; 0 if the frame is correctly packed.
 */
uint32_t dci_demo_buf_raw10_bad_samples(const DCI_frame_t *f, uint32_t report_max);

/** \brief  Log min/max/mean of the 10-bit samples, and flag a flat or 8-bit range. */
void dci_demo_buf_raw10_range(const DCI_frame_t *f);

#ifdef __cplusplus
}
#endif

#endif /* DCI_DEMO_BUF_H */
