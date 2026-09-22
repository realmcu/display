/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DCI demo: the signal table and the overlay resolver. THE configuration source.
 *
 * This project is the receiver under test. The stimulus comes from a separate camera
 * board whose own preset table is documented in
 *
 *     .../realtek-app/applications/dci_8773g/note/preset-table.md
 *
 * and the job of this file is to declare, for every stimulus that board can emit, how
 * this machine must be configured to receive it. Every row carries the name of the
 * camera-side row that produces it (`stim`), so a test is always a pair: one camera
 * preset, one signal row. A row whose stim is NULL needs a stimulus the OV2640 table
 * cannot produce - it is there for a receiver-side property (block splitting, pool
 * ceiling) that has to be covered anyway.
 *
 * Two axes, deliberately separate:
 *
 *   signal row  - what arrives on the pins: size, format, clock and sync polarity,
 *                 hardware or embedded sync. Mirrors the camera table, deduplicated.
 *   overlay     - what this machine then does with it: crop, line/byte select, frame
 *                 rate control. Chosen by macros in dci_demo_config.h, valid on any
 *                 row it does not contradict.
 *
 * The old 21-row preset table folded both axes into one list, so "QVGA + crop" and
 * "RAW10 + crop" were separate rows and every new feature multiplied the table. Here
 * they are 16 rows x an overlay word, and a case is a point in that product. Which
 * combinations are legal is a rule in dci_demo_cfg_resolve(), not a hole in the table.
 *
 * Deduplication against the camera table is by what reaches the pins, not by intent:
 * the receiver cannot tell YUV422 from RGB565 (same 2 bytes per pixel, same pclk
 * count), cannot tell a colour bar from a scene, and has no configuration bit for
 * "PCLK is gated during blanking". Those four camera rows therefore collapse into
 * qvga-rgb565, as do the two hardware-sync OV5640 rows. 16 camera rows -> 12 receiver
 * rows, plus a null row and three pure-geometry rows.
 */

#ifndef DCI_DEMO_SIGNAL_H
#define DCI_DEMO_SIGNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rtl_dci.h"

/*============================================================================*
 *                         Pixel format
 *============================================================================*/
/*
 * DCI is format-blind: DCI_InitTypeDef has no format field at all, only a pclk count
 * per line, a line count, and EDM. The format here is a demo-side concept that exists
 * for exactly two reasons - it fixes the pclk-per-pixel ratio that turns a width in
 * pixels into the pclk count the hardware needs, and it tells the dump how to read the
 * bytes back.
 *
 * DCI_FMT_NONE is what "no configuration" means on a receiver: no format is assumed at
 * all, `width` is read as bytes on the bus, and 1 pclk carries 1 byte. It is not a
 * degenerate case - it is the honest description of what the peripheral does, and it
 * is why the null row and the 160x120 RGB565 row must derive identical geometry.
 */
typedef enum
{
    DCI_FMT_NONE,           /*!< No interpretation: width is bus bytes, 1 pclk = 1 B. */
    DCI_FMT_RGB565,         /*!< 2 bytes/pixel, 2 pclk/pixel on an 8-bit bus. */
    DCI_FMT_YUV422,         /*!< 2 bytes/pixel, interleaved CbYCrY. */
    DCI_FMT_RAW8,           /*!< 1 byte/pixel, Bayer or monochrome. */
    DCI_FMT_RAW10,          /*!< 10-bit samples, 2 bytes each, EDM = 10BIT. */
    DCI_FMT_JPEG,           /*!< Variable length byte stream. */
} dci_demo_fmt_t;

/*============================================================================*
 *                         Signal row
 *============================================================================*/
/*
 * Only what arrives on the pins. Nothing this machine chooses to do afterwards belongs
 * here - that is the overlay's job, and keeping the split clean is what stops the
 * table growing a row per feature combination.
 */
typedef struct
{
    const char     *name;           /*!< Short id, shown in every log line. */
    const char     *stim;           /*!< Camera-side row that produces this signal,
                                         NULL if the OV2640/OV5640 tables cannot. */
    const char     *note;           /*!< What this row proves, and what it replaced. */

    /*
     * CAREFUL: width is in PIXELS for every format except DCI_FMT_NONE, where it is
     * BYTES ON THE BUS. The null row has no format, so it has no pixel to count -
     * pclk_per_line = width * pclk_per_pixel, and pclk_per_pixel is 1 for NONE.
     *
     * The consequence is that row 0 reads "320 x 120" against a camera preset that
     * reads "160x120", and the two DO agree: 160 RGB565 pixels are 320 bus bytes,
     * both are 320 pclk/line and 38400 B/frame. Anything printing this field has to
     * say which unit it is using, or the comparison against the camera-side table
     * looks wrong (dci_demo_cfg_print() and dci_demo_signal_list() both do).
     */
    uint16_t        width;          /*!< Pixels; bus BYTES when fmt == DCI_FMT_NONE. */
    uint16_t        height;         /*!< Lines per frame, as the sensor emits them. */
    dci_demo_fmt_t  fmt;

    /* Clock and sync polarity. The camera table writes these as four bits,
     * PCLK-rising / PCLK-gated / HREF-negative / VSYNC-negative; the second has no
     * receiver counterpart, which is why the gated row deduplicates away. */
    bool            pckpol;         /*!< false: sample on the falling edge. */
    bool            hspol;          /*!< false: HSYNC active low. */
    bool            vspol;          /*!< false: VSYNC active low. */
    DCI_SYNC_T      sync;           /*!< Hardware pins or embedded codes. */
} dci_demo_signal_t;

/*============================================================================*
 *                         Resolved configuration
 *============================================================================*/
/*
 * A signal row with the overlay applied and the geometry derived. This is what a case
 * is handed, what gets printed before a capture, and what the frame dump reads - so
 * there is one place where the numbers come from and one place to check them against
 * DCI_Capture_GetFrameInfo().
 *
 * The derivation deliberately follows the driver's own order (dci_calc_geometry):
 * pclk x bytes_per_pclk, then byte select, then line select. Working in pclk rather
 * than in pixels is what makes crop come out right - CAPCNT counts pixel clocks, and
 * RAW10 is the one format where a pclk is 2 bytes.
 */
typedef struct
{
    const dci_demo_signal_t *sig;
    uint8_t   index;                /*!< Row index, for logs. */

    /* --- overlay, from dci_demo_config.h --- */
    bool      crop_en;
    uint16_t  crop_x, crop_y;       /*!< Pixels (bus bytes for DCI_FMT_NONE). */
    uint16_t  crop_w, crop_h;
    DCI_FCRC_T fcrc;
    DCI_BSM_T bsm;
    bool      oebs;
    bool      lsm;
    bool      oels;

    /* --- derived --- */
    uint8_t   bytes_per_pclk;       /*!< 2 under EDM=10BIT, else 1. */
    uint16_t  pclk_per_line;        /*!< Uncropped, as declared to Capture_Init. */
    uint16_t  lines_in;             /*!< Uncropped line count. */
    uint16_t  crop_pclk_x;          /*!< Crop window in pclk, as ConfigCrop wants it. */
    uint16_t  crop_pclk_w;          /*!< Already aligned down to a multiple of 4. */
    uint32_t  line_bytes;           /*!< After crop and byte select. */
    uint16_t  lines_out;            /*!< After line select. */
    uint32_t  frame_bytes;          /*!< What will be delivered. 0 for JPEG. */
    uint32_t  init_bytes;           /*!< What Capture_Init validates against. */
} dci_demo_cfg_t;

/*============================================================================*
 *                         Table access
 *============================================================================*/
/** \brief  Number of rows in the signal table. */
uint8_t dci_demo_signal_count(void);

/** \brief  Signal row by index, or NULL if out of range. */
const dci_demo_signal_t *dci_demo_signal_get(uint8_t index);

/** \brief  Print the table as a numbered menu, with the camera row that drives each. */
void dci_demo_signal_list(void);

/*============================================================================*
 *                         Resolver
 *============================================================================*/
/**
 * \brief   Apply the overlay macros to a signal row and derive the geometry.
 *
 * Also the place where an illegal combination is caught. Two kinds:
 *
 *   - the driver would refuse it anyway (crop+JPEG, embedded sync+10-bit,
 *     byte select+10-bit). Caught here so the message names the macro rather than
 *     surfacing as DCI_ERROR_MODE_CONFLICT from Capture_Init.
 *   - the driver would ACCEPT it and capture a corrupt image. Byte select on a
 *     2-bytes-per-pixel format is the whole category: BSM drops bytes, which only
 *     means "drop pixels" at 1 byte per pixel, and DCI has no idea what a pixel is.
 *     The old table guarded this by only ever putting BSM on a RAW8 row; here it is a
 *     rule, which is strictly better - a rule cannot be worked around by picking a
 *     different row.
 *
 * Reasons are logged, so a caller only needs the bool.
 *
 * \return  true if the combination is usable.
 */
bool dci_demo_cfg_resolve(uint8_t index, dci_demo_cfg_t *out);

/** \brief  Translate a resolved configuration into the driver's init struct. */
void dci_demo_cfg_to_init(const dci_demo_cfg_t *c, DCI_InitTypeDef *init);

/**
 * \brief   Total arena this configuration needs at a given buffer count.
 *
 * Guard fence included, so it is directly comparable with DCI_DEMO_POOL_KB. Uses
 * init_bytes, not frame_bytes: a cropping configuration has to allocate for the
 * uncropped frame because Capture_Init() validates before the window is programmed.
 */
uint32_t dci_demo_cfg_pool_bytes(const dci_demo_cfg_t *c, uint8_t buf_count);

/**
 * \brief   Print every configuration value that will be programmed.
 *
 * Called before a capture starts, so the log carries the complete record of what was
 * flashed. That matters more here than in most demos: the selection is entirely
 * compile time, so two builds produce identical-looking output otherwise.
 */
void dci_demo_cfg_print(const dci_demo_cfg_t *c, uint8_t buf_count);

/*============================================================================*
 *                         Format helpers
 *============================================================================*/
/** \brief  Bytes per pixel implied by the format. 0 for JPEG (variable). */
uint8_t dci_demo_fmt_bytes_per_pixel(dci_demo_fmt_t fmt);

/** \brief  Pixel clocks per pixel implied by the format. */
uint8_t dci_demo_fmt_pclk_per_pixel(dci_demo_fmt_t fmt);

/** \brief  Format name for logs. */
const char *dci_demo_fmt_name(dci_demo_fmt_t fmt);

#ifdef __cplusplus
}
#endif

#endif /* DCI_DEMO_SIGNAL_H */
