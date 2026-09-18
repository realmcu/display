/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DCI demo: preset table.
 *
 * One row describes a complete signal + geometry + format configuration. Test cases
 * name a preset instead of open-coding register values, so a case and a wiring
 * variant are independent choices: "snapshot on preset 3" and "continuous on
 * preset 3" exercise the same hardware setup through two different code paths.
 *
 * Adding a board or a sensor mode means adding a row, not editing a case.
 */

#ifndef DCI_DEMO_PRESET_H
#define DCI_DEMO_PRESET_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rtl_dci.h"

/*============================================================================*
 *                         Pixel format
 *============================================================================*/
/*
 * DCI itself is format-blind - it only counts words - so the format matters to the
 * demo for two reasons only: it fixes the pixel-clock-per-pixel ratio that turns a
 * width in pixels into the pclk count the hardware needs, and it tells the dump how
 * to interpret the bytes.
 */
typedef enum
{
    DCI_FMT_RGB565,         /*!< 2 bytes/pixel, 2 pclk/pixel on an 8-bit bus. */
    DCI_FMT_YUV422,         /*!< 2 bytes/pixel, interleaved CbYCrY. */
    DCI_FMT_RAW8,           /*!< 1 byte/pixel, Bayer or monochrome. */
    DCI_FMT_RAW10,          /*!< 10-bit samples, 2 bytes each, EDM = 10BIT. */
    DCI_FMT_JPEG,           /*!< Variable length byte stream. */
} dci_demo_fmt_t;

/*============================================================================*
 *                         Preset
 *============================================================================*/
typedef struct
{
    const char     *name;           /*!< Shown in the menu. */
    const char     *note;           /*!< Why this preset exists / what it proves. */

    /* --- geometry, in pixels as a human thinks of it --- */
    uint16_t        width;
    uint16_t        height;
    dci_demo_fmt_t  fmt;

    /* --- signal polarity, per the sensor datasheet and the board --- */
    bool            pckpol;         /*!< false: sample on falling edge. */
    bool            hspol;          /*!< false: HSYNC active low. */
    bool            vspol;          /*!< false: VSYNC active low. */
    DCI_SYNC_T      sync;           /*!< Hardware pins or embedded codes. */

    /* --- optional crop, applied after DCI_Capture_Init() --- */
    bool            crop_en;
    uint16_t        crop_x;         /*!< In pixels; converted to pclk internally. */
    uint16_t        crop_y;
    uint16_t        crop_w;
    uint16_t        crop_h;

    /* --- decimation --- */
    DCI_FCRC_T      fcrc;           /*!< Frame rate: all / half / quarter. */
    DCI_BSM_T       bsm;            /*!< Byte select; requires 8-bit EDM. */
    bool            oebs;
    bool            lsm;            /*!< Line select: 1 of 2. */
    bool            oels;
} dci_demo_preset_t;

/*============================================================================*
 *                         Table access
 *============================================================================*/
/** \brief  Number of presets in the table. */
uint8_t dci_demo_preset_count(void);

/** \brief  Preset by index, or NULL if out of range. */
const dci_demo_preset_t *dci_demo_preset_get(uint8_t index);

/** \brief  Print the table as a numbered menu. */
void dci_demo_preset_list(void);

/**
 * \brief   Translate a preset into the driver's init struct.
 *
 * Does the pixel-to-pclk conversion and leaves init->crop false: the window belongs
 * to DCI_ConfigCrop(), which cannot be called until after Capture_Init().
 */
void dci_demo_preset_to_init(const dci_demo_preset_t *p, DCI_InitTypeDef *init);

/** \brief  Bytes per pixel implied by the format. 0 for JPEG (variable). */
uint8_t dci_demo_fmt_bytes_per_pixel(dci_demo_fmt_t fmt);

/** \brief  Pixel clocks per pixel implied by the format. */
uint8_t dci_demo_fmt_pclk_per_pixel(dci_demo_fmt_t fmt);

/** \brief  Format name for logs. */
const char *dci_demo_fmt_name(dci_demo_fmt_t fmt);

/**
 * \brief   Frame size the preset will finally deliver, in bytes.
 *
 * Computed the same way the driver does, so a mismatch against
 * DCI_Capture_GetFrameInfo() is a real disagreement worth reporting rather than two
 * different formulas drifting apart.
 *
 * \return  0 for JPEG - the length is only known once a frame arrives.
 */
uint32_t dci_demo_preset_frame_bytes(const dci_demo_preset_t *p);

/**
 * \brief   Buffer size that DCI_Capture_Init() will demand, in bytes.
 *
 * Differs from dci_demo_preset_frame_bytes() in two cases:
 *
 *   - the preset crops: the window is programmed by DCI_ConfigCrop() *after*
 *     Capture_Init(), so Init validates buf_bytes against the **uncropped** frame and
 *     rejects a buffer sized for the cropped one. The surplus goes unused once crop
 *     takes effect, but it has to be allocated.
 *   - JPEG: there is no computable frame size, so this returns the configured
 *     capacity cap (DCI_DEMO_JPEG_CAP_KB).
 */
uint32_t dci_demo_preset_init_bytes(const dci_demo_preset_t *p);

/**
 * \brief   Total arena a preset needs at a given buffer count, guard fence included.
 *
 * The figure to compare against DCI_DEMO_POOL_KB. Exposed so the preset listing can
 * show what fits before anything is flashed, rather than leaving it to a
 * DEMO_FAIL_POOL at run time.
 */
uint32_t dci_demo_preset_pool_bytes(const dci_demo_preset_t *p, uint8_t buf_count);

#ifdef __cplusplus
}
#endif

#endif /* DCI_DEMO_PRESET_H */
