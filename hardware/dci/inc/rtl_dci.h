/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_DCI_H
#define RTL_DCI_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include <stdbool.h>
#include "rtl_dci_def.h"

/** \defgroup DCI        DCI
  * \brief    Digital Camera Interface driver module
  * \{
  */

/*============================================================================*
 *                          Private Macros
 *============================================================================*/
#define DCI                         ((DCI_TypeDef *)DCI_REG_BASE)

/*============================================================================*
 *                         Constants
 *============================================================================*/
/** \defgroup DCI_Exported_Constants DCI Exported Constants
  * \{
  */

/** \defgroup DCI_Limits DCI Limits
  * \{
  */
#define DCI_MAX_BUF                 2       /*!< Max framebuffer count. */

/**
 * \brief   Max blocks per frame.
 *
 * Derived from the DMA's BLOCK_TS width, which is **16 bit on this SoC family**
 * (65535 words = 256 KB per block), not the 20 bit of the older parts. Worst legal
 * case is 2048x2048 at 2 bytes/pixel = 2,097,152 words, needing 33 blocks.
 *
 * The ring costs DCI_MAX_BUF * DCI_MAX_CHUNK * 20 bytes = 1320 bytes at 33, which is
 * the price of covering every resolution the spec allows. Common cases stay far
 * below: 320x240 needs 1, 466x466 needs 2, 640x480 needs 3, 1280x720 needs 8.
 */
#define DCI_MAX_CHUNK               33
#define DCI_MAX_DESC                (DCI_MAX_BUF * DCI_MAX_CHUNK)

/**
 * \brief   Largest BLOCK_TS the DMA can be given, in source data items (words here).
 *
 * \note    Must match the SoC's DMA_CTLx_H block_ts field width. The SoC layer
 *          cross-checks this with a static assertion.
 */
#define DCI_MAX_BLOCK_TS            65535UL
#define DCI_MAX_PIXELS              2048        /*!< Non-JPEG max width/height. */
/** \} */

/** \defgroup DCI_INT_Sources DCI Interrupt Sources
  * \{
  */
#define DCI_INT_FRAME               BIT0
#define DCI_INT_OVR                 BIT1
#define DCI_INT_ERR                 BIT2
#define DCI_INT_VSYNC               BIT3
#define DCI_INT_LINE                BIT4
#define DCI_INT_ALL                 (DCI_INT_FRAME | DCI_INT_OVR | DCI_INT_ERR | \
                                     DCI_INT_VSYNC | DCI_INT_LINE)
/** \} */

/** \} */ /* End of DCI_Exported_Constants */

/*============================================================================*
 *                         Types
 *============================================================================*/
/** \defgroup DCI_Exported_Types DCI Exported Types
  * \{
  */

/**
 * \brief   DCI driver return codes, grouped by what the caller can do about them.
 */
typedef enum
{
    DCI_SUCCESS = 0,

    /* Configuration: caller must fix a parameter before it can ever succeed. */
    DCI_ERROR_INVALID_PARAM,        /*!< NULL pointer, out-of-range enum, mode mismatch. */
    DCI_ERROR_SIZE,                 /*!< Frame exceeds 2048x2048 or DCI_MAX_CHUNK blocks. */
    DCI_ERROR_ALIGN,                /*!< buf address or buf_bytes not 4-byte aligned. */
    DCI_ERROR_BUF_TOO_SMALL,        /*!< buf_bytes < frame_bytes (fixed-length formats). */
    DCI_ERROR_MODE_CONFLICT,        /*!< jpeg+crop, jpeg+ess, ess+edm!=0, bsm+edm!=0. */
    DCI_ERROR_NO_CALLBACK,          /*!< notify_mode CALLBACK but frame_ready is NULL. */
    DCI_ERROR_DMA_CHANNEL,          /*!< Channel number out of range. */

    /* State: parameters are fine, the timing is not. Retry or change state first. */
    DCI_ERROR_NOT_INIT,             /*!< Start/Reconfig before Capture_Init. */
    DCI_ERROR_STATE,                /*!< Operation not allowed in the current state. */
    DCI_ERROR_BUSY,                 /*!< Target buffer still held by the consumer. */
    DCI_ERROR_NO_FRAME,             /*!< GetFrame with nothing pending. */
    DCI_ERROR_TIMEOUT,              /*!< Read-back poll timed out (CAPTURE or FIFO). */

    /* Runtime: reported through frame_error only, never as a return value. */
    DCI_ERROR_OVERRUN,              /*!< IT_OVR: FIFO overrun, whole frame lost. */
    DCI_ERROR_SYNC,                 /*!< IT_ERR: embedded sync codes out of order. */
    DCI_ERROR_BUF_OVERFLOW,         /*!< JPEG stream exceeded buf_bytes. */
    DCI_ERROR_FRAME_LONG,           /*!< Sensor output exceeds the declared geometry. */
    DCI_ERROR_FRAME_SHORT,          /*!< Sensor output smaller than the declared geometry. */
    DCI_ERROR_DESYNC,               /*!< Block/frame mismatch, resync performed. */

    /* Bring-up: returned by DCI_RegTest() only. */
    DCI_ERROR_NO_RESPONSE,          /*!< Register block reads all-0 or all-1 whatever is
                                         written: clock gated, wrong base, or dead bus. */
    DCI_ERROR_REG_TEST,             /*!< Block responds but some bits misbehave. */
} DCI_ERROR;

/**
 * \brief   Extended data mode. Only 8/10-bit exist: the pin set is D[9:0].
 */
typedef enum
{
    DCI_EDM_8BIT  = 0x0,
    DCI_EDM_10BIT = 0x1,
} DCI_EDM_T;

/**
 * \brief   Frame capture rate control, continuous mode only.
 */
typedef enum
{
    DCI_FCRC_ALL    = 0x0,      /*!< Capture every frame. */
    DCI_FCRC_HALF   = 0x1,      /*!< Capture every 2nd frame. */
    DCI_FCRC_QUARTER = 0x2,     /*!< Capture 1 frame out of 4. */
} DCI_FCRC_T;

/**
 * \brief   Byte select mode. Valid only when edm is DCI_EDM_8BIT.
 */
typedef enum
{
    DCI_BSM_ALL       = 0x0,    /*!< All bytes. */
    DCI_BSM_EVERY_2ND = 0x1,    /*!< 1 byte out of 2. */
    DCI_BSM_1_OF_4    = 0x2,    /*!< 1 byte out of 4. */
    DCI_BSM_2_OF_4    = 0x3,    /*!< 2 bytes out of 4. */
} DCI_BSM_T;

typedef enum
{
    DCI_SYNC_HARDWARE = 0x0,    /*!< HSYNC/VSYNC pins. */
    DCI_SYNC_EMBEDDED = 0x1,    /*!< Codes in the data stream, 8-bit only. */
} DCI_SYNC_T;

typedef enum
{
    DCI_MODE_CONTINUOUS = 0x0,
    DCI_MODE_SNAPSHOT   = 0x1,
} DCI_CAPTURE_MODE_T;

typedef enum
{
    DCI_STATE_RESET = 0,        /*!< Capture_Init not called yet. */
    DCI_STATE_IDLE,
    DCI_STATE_BUSY,
    DCI_STATE_SUSPENDED,        /*!< Gated because the next buffer is still held. */
    DCI_STATE_ERROR,
} DCI_CAPTURE_STATE_T;

typedef enum
{
    DCI_BUF_SINGLE = 1,
    DCI_BUF_DOUBLE = 2,
} DCI_BUF_COUNT_T;

/**
 * \brief   What to do when the next buffer to write is still held by the consumer.
 *
 * Must be chosen explicitly: with buf_count == 1 the GATE policy turns continuous
 * capture into "stop after every frame", which is not what a viewfinder wants.
 */
typedef enum
{
    DCI_ON_BUSY_GATE      = 0,  /*!< Clear CAPTURE and wait. Never tears. */
    DCI_ON_BUSY_OVERWRITE = 1,  /*!< Keep writing. Full rate, may tear. */
} DCI_ON_BUSY_T;

/**
 * \brief   Frame delivery channel. Exactly one is used, never both.
 */
typedef enum
{
    DCI_NOTIFY_CALLBACK = 0,    /*!< frame_ready carries the frame, no queue. */
    DCI_NOTIFY_POLL     = 1,    /*!< Queued; take it with GetFrame. frame_ready unused. */
} DCI_NOTIFY_MODE_T;

/**
 * \brief   Embedded synchronization codes. Only used when sync is DCI_SYNC_EMBEDDED.
 */
typedef struct
{
    uint8_t frame_start;
    uint8_t line_start;
    uint8_t line_end;
    uint8_t frame_end;
} DCI_SyncCode_t;

/** \brief  Per-bit compare mask for the embedded sync codes. */
typedef DCI_SyncCode_t DCI_SyncUnmask_t;

/**
 * \brief   A delivered frame.
 *
 * addr is always the buffer base: one frame lands entirely in one buffer,
 * contiguously. That invariant is what makes {addr, bytes} sufficient.
 */
typedef struct
{
    uint8_t  *addr;
    uint32_t  bytes;        /*!< Bytes actually written. JPEG keeps the zero padding. */
    uint8_t   index;        /*!< Pass back to DCI_Capture_ReleaseBuffer(). */
    uint32_t  seq;          /*!< Monotonic delivery counter. */
    bool      truncated;    /*!< Clean prefix but incomplete: JPEG overflow, or the
                                 fixed-length frame was longer/shorter than declared. */
} DCI_frame_t;

typedef struct
{
    uint32_t frames_done;
    uint32_t frames_dropped;    /*!< Only drops the driver knows about: resync, OVR. */
    uint32_t chunks_done;
    uint32_t ovr_count;
    uint32_t err_count;
    uint32_t resync_count;
    uint32_t suspend_count;
    uint32_t buf_overflow;
    uint32_t seq;
} DCI_stats_t;

/**
 * \brief   DCI peripheral configuration.
 *
 * sensor_pclk_per_line / sensor_lines must be declared by the application: with
 * CROP disabled the hardware captures whatever HSYNC/VSYNC delimit, so DCI cannot
 * discover the frame size from its own registers. Counted in pixel clocks rather
 * than pixels because CAPCNT is a pclk count and the pclk-per-pixel ratio depends
 * on the format (RGB565 and YCbCr422 are 2, monochrome 8-bit is 1).
 */
typedef struct
{
    uint16_t     sensor_pclk_per_line;
    uint16_t     sensor_lines;

    DCI_EDM_T    edm;
    DCI_SYNC_T   sync;
    DCI_FCRC_T   fcrc;

    bool         pckpol;        /*!< false: falling edge, true: rising edge. */
    bool         hspol;         /*!< false: HSYNC active low. */
    bool         vspol;         /*!< false: VSYNC active low. */

    bool         jpeg;
    bool         crop;          /*!< Window comes from DCI_ConfigCrop(). */

    DCI_BSM_T    bsm;           /*!< Byte decimation. Requires edm == DCI_EDM_8BIT. */
    bool         oebs;          /*!< Byte select start, works with bsm != ALL. */
    bool         lsm;           /*!< Line decimation: capture 1 line out of 2. */
    bool         oels;          /*!< Line select start, works with lsm. */

    DCI_SyncCode_t   sync_code;
    DCI_SyncUnmask_t sync_unmask;
} DCI_InitTypeDef;

/**
 * \brief   Buffer and delivery configuration.
 *
 * Block count is not here: it is derived from the frame size (grow only when a
 * block cannot hold the frame), so there is nothing for the caller to tune.
 */
/**
 * \brief   Let the driver allocate a DMA channel instead of naming one.
 *
 * Preferred: the driver then asks the DMA HAL for the deepest FIFO available, which
 * is what a pixel stream wants, and the channel is released on Capture_Init failure
 * or on a later Reconfig. Naming a channel explicitly is still allowed but takes
 * the FIFO depth out of the driver's hands.
 *
 * On BBPro4 (rtl87x3k, AP core) a DCI capture only works on a DMA1 channel, which is
 * 16..23 there: AP DMA0 was dropped from the silicon. An explicit channel outside
 * that range is refused with DCI_ERROR_DMA_CHANNEL rather than quietly corrected,
 * because it means the caller expected a channel that cannot carry DCI at all. AUTO
 * handles the constraint on its own.
 */
#define DCI_DMA_CH_AUTO             0xFFU

typedef struct
{
    uint8_t           dma_channel_num;  /*!< DCI_DMA_CH_AUTO, or an explicit channel -
                                             on BBPro4 AP that must be a DMA1 channel,
                                             16..23. */
    uint8_t           dma_priority;     /*!< 0 is highest. */

    DCI_BUF_COUNT_T   buf_count;
    uint32_t         *buf[DCI_MAX_BUF]; /*!< 4-byte aligned. The memory region is the
                                             caller's business, the driver does not check. */
    uint32_t          buf_bytes;        /*!< Per buffer. JPEG treats it as a capacity cap. */

    DCI_ON_BUSY_T     on_busy;
    DCI_NOTIFY_MODE_T notify_mode;

    void (*frame_ready)(const DCI_frame_t *frame);      /*!< Required for CALLBACK mode. */
    void (*frame_error)(DCI_ERROR err, uint32_t ris);
    void (*vsync_event)(void);          /*!< NULL leaves IT_VSYNC disabled. */
    void (*line_event)(void);           /*!< NULL leaves IT_LINE disabled. */
} DCI_DMA_config;

/** \} */ /* End of DCI_Exported_Types */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** \defgroup DCI_Exported_Functions DCI Exported Functions
  * \{
  */

/* ---- Peripheral layer: DCI registers only ---- */

void DCI_StructInit(DCI_InitTypeDef *init);
void DCI_Init(const DCI_InitTypeDef *init);

/**
 * \brief  Enable or disable the DCI peripheral (CR.ENABLE).
 *
 * \note   Configuration registers must be written while this is 0.
 */
__STATIC_INLINE void DCI_Cmd(FunctionalState state)
{
    DCI_CR_TypeDef cr = {.d32 = DCI->CR};
    cr.b.enable = (state == ENABLE) ? 1 : 0;
    DCI->CR = cr.d32;
}

/** \brief  Enable or disable capture (CR.CAPTURE). Also gates the DMA request line. */
__STATIC_INLINE void DCI_CaptureCmd(FunctionalState state)
{
    DCI_CR_TypeDef cr = {.d32 = DCI->CR};
    cr.b.capture = (state == ENABLE) ? 1 : 0;
    DCI->CR = cr.d32;
}

/**
 * \brief  Read back CR.CAPTURE.
 *
 * \note   Clearing CAPTURE only takes effect at the end of the current frame, so
 *         this must be polled to 0 to know the frame boundary has been reached.
 */
__STATIC_INLINE FlagStatus DCI_GetCaptureStatus(void)
{
    DCI_CR_TypeDef cr = {.d32 = DCI->CR};
    return (cr.b.capture != 0) ? SET : RESET;
}

__STATIC_INLINE void DCI_INTConfig(uint32_t src, FunctionalState state)
{
    if (state == ENABLE)
    {
        DCI->IER |= src;
    }
    else
    {
        DCI->IER &= ~src;
    }
}

/** \brief  Masked interrupt status: RIS & IER. */
__STATIC_INLINE ITStatus DCI_GetINTStatus(uint32_t src)
{
    return ((DCI->MIS & src) != 0) ? SET : RESET;
}

/**
 * \brief  Raw interrupt status.
 *
 * \note   RIS latches regardless of IER, so this stays valid for a masked source.
 *         Used to settle FRAME-versus-Transfer races without a timeout.
 */
__STATIC_INLINE ITStatus DCI_GetRawINTStatus(uint32_t src)
{
    return ((DCI->RIS & src) != 0) ? SET : RESET;
}

/** \brief  Clear pending interrupts. ICR is write-1-only; writing 0 has no effect. */
__STATIC_INLINE void DCI_ClearINTPendingBit(uint32_t src)
{
    DCI->ICR = src;
}

/** \brief  FIFO not empty. */
__STATIC_INLINE FlagStatus DCI_GetFifoNotEmpty(void)
{
    DCI_SR_TypeDef sr = {.d32 = DCI->SR};
    return (sr.b.fne != 0) ? SET : RESET;
}

/* ---- Capture layer ---- */

/**
 * \brief  Validate the configuration, derive the block layout and program the
 *         peripheral. Leaves the DCI disabled; DCI_Capture_Start() enables it.
 *
 * \note   init->crop must be false here: the window lives in DCI_ConfigCrop() and
 *         is not known yet. To capture cropped, call this with crop disabled, then
 *         DCI_ConfigCrop() followed by DCI_CropCmd(ENABLE). Same sequence as ST's
 *         HAL_DCMI_Init / HAL_DCMI_ConfigCrop / HAL_DCMI_EnableCrop.
 */
DCI_ERROR DCI_Capture_Init(const DCI_InitTypeDef *init, const DCI_DMA_config *dma);

/**
 * \brief  Start a capture session.
 *
 * \note   The mode fixes the descriptor topology, so it is settled by the first
 *         call and every later call must pass the same value. To change it, call
 *         DCI_Capture_Init() again.
 */
DCI_ERROR DCI_Capture_Start(DCI_CAPTURE_MODE_T mode);
DCI_ERROR DCI_Capture_Stop(void);
DCI_ERROR DCI_Capture_Abort(void);
DCI_ERROR DCI_Capture_Suspend(void);
DCI_ERROR DCI_Capture_Resume(void);
void      DCI_Handler(void);

/* ---- State query and frame retrieval ---- */

DCI_CAPTURE_STATE_T DCI_Capture_GetState(void);
bool                DCI_Capture_IsFrameReady(void);
uint8_t             DCI_Capture_GetReadyCount(void);
DCI_ERROR           DCI_Capture_GetFrame(DCI_frame_t *frame);
DCI_ERROR           DCI_Capture_WaitFrame(DCI_frame_t *frame, uint32_t timeout_ms);
void                DCI_Capture_ReleaseBuffer(uint8_t index);
uint32_t            DCI_Capture_GetTransferredBytes(void);

/* ---- Runtime reconfiguration ---- */

DCI_ERROR DCI_Capture_Reconfig(const DCI_InitTypeDef *init, const DCI_DMA_config *dma);
DCI_ERROR DCI_Capture_SetBuffers(uint32_t *const buf[], uint8_t count);
DCI_ERROR DCI_SetCaptureRate(DCI_FCRC_T rate);
DCI_ERROR DCI_ConfigSyncCode(const DCI_SyncCode_t *code, const DCI_SyncUnmask_t *unmask);
DCI_ERROR DCI_ConfigCrop(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h);
DCI_ERROR DCI_CropCmd(FunctionalState state);

/* ---- Derived geometry ---- */

/**
 * \brief   What the driver derived from the current configuration.
 */
typedef struct
{
    uint32_t frame_bytes;       /*!< 0 for JPEG: the length is only known per frame. */
    uint32_t frame_words;
    uint32_t line_bytes;
    uint16_t lines_out;         /*!< After LSM decimation. */
    uint16_t lines_per_chunk;   /*!< 0 when not line aligned. */
    uint32_t chunk_words;
    uint8_t  chunks_per_frame;
    uint8_t  desc_count;
    bool     line_aligned;      /*!< false = degenerate word-aligned split. */
    bool     var_len;           /*!< JPEG. */
} DCI_frame_info_t;

/**
 * \brief   Read back the derived geometry after DCI_Capture_Init().
 *
 * Lets a caller confirm what the driver concluded - frame size, block count - rather
 * than re-deriving the formula and hoping the two agree.
 *
 * \return  DCI_ERROR_NOT_INIT before DCI_Capture_Init().
 */
DCI_ERROR DCI_Capture_GetFrameInfo(DCI_frame_info_t *info);

/* ---- Diagnostics ---- */

void DCI_GetStats(DCI_stats_t *stats);

/**
 * \brief   Result of one register under test.
 */
typedef struct
{
    const char *name;
    uint32_t    offset;     /*!< Byte offset from DCI_REG_BASE. */
    uint32_t    rw_mask;    /*!< Bits expected to be writable. */
    uint32_t    stuck_0;    /*!< Writable bits that never read back as 1. */
    uint32_t    stuck_1;    /*!< Bits that read 1 although 0 was written. */
    bool        skipped;    /*!< Read-only or no writable bits, nothing to check. */
} DCI_regtest_item_t;

/**
 * \brief   Register read/write self test report.
 */
typedef struct
{
    uint32_t base;              /*!< Base address under test. */
    uint32_t probe_lo;          /*!< CR read back after writing 0. */
    uint32_t probe_hi;          /*!< CR read back after writing its writable mask.
                                     Equal to probe_lo means the block never changed,
                                     i.e. clock gated / wrong base / dead bus. */
    uint8_t  checked;           /*!< Registers actually exercised. */
    uint8_t  failed;            /*!< Registers with a stuck bit. */
    bool     responded;         /*!< probe_lo != probe_hi: the block reacts to writes. */
    bool     varied;            /*!< Some register returned more than one distinct value. */
    bool     distinct_ok;       /*!< Each register held its own value: no address aliasing. */
    bool     w1c_ok;            /*!< ICR is write-1-only and left RIS clean. */
    bool     mis_ok;            /*!< MIS tracked RIS & IER. */
    bool     capcnt_low_forced; /*!< CWSIZE.CAPCNT[1:0] read back 0 after writing 1, i.e.
                                     the hardware force-clears them. Informational, not a
                                     failure: it settles how CAPCNT is encoded. */
    DCI_regtest_item_t item[10];    /*!< One per register, DR excluded. */
} DCI_regtest_t;

/**
 * \brief   Exercise the DCI register block without a camera attached.
 *
 * Writes walking patterns into the writable bits of every register and reads them
 * back, so a clock that is still gated, a wrong base address or a stuck bus is
 * caught before anyone starts debugging pixel data. DR is never touched: reading
 * it pops the FIFO.
 *
 * \param[out] report  Optional per-register detail. May be NULL.
 *
 * \return  DCI_SUCCESS, DCI_ERROR_NO_RESPONSE if the block looks dead, or
 *          DCI_ERROR_REG_TEST if it responds but some bits misbehave.
 *
 * \note    Destructive: leaves every register at its reset value and puts the
 *          driver back to RESET state. Must not run during a capture; call it
 *          before DCI_Capture_Init() or after DCI_Capture_Abort().
 */
DCI_ERROR DCI_RegTest(DCI_regtest_t *report);

/**
 * \brief   Run DCI_RegTest() and print the report.
 *
 * \note    Needs DCI_DEBUG_REG_DUMP == 1 to produce output; otherwise it only
 *          returns the verdict.
 */
DCI_ERROR DCI_RegTestDump(void);

#ifndef DCI_DEBUG_REG_DUMP
#define DCI_DEBUG_REG_DUMP      0
#endif

#if (DCI_DEBUG_REG_DUMP == 1)

/**
 * \brief   DMA channel and global state, already field-decoded.
 *
 * Plain integers rather than the SoC's register types: this header must stay free of
 * DMA and OS dependencies, and the decoding belongs to the layer that knows the
 * field positions anyway.
 */
typedef struct
{
    uint32_t sar, curr_sar, dar, curr_dar, llp;
    uint32_t ctl_low, ctl_high, cfg_low, cfg_high;
    uint32_t ch_en, status_tfr, status_block, status_err;
    uint32_t src_per, dest_per;         /*!< Extracted from cfg_high. */
    uint32_t dest_msize, src_msize;     /*!< Extracted from ctl_low, raw encoding. */
    uint32_t tt_fc;
    bool     fifo_empty;
    bool     hs_sel_src, hs_sel_dst;    /*!< 0 = hardware handshake. */
    bool     llp_dst_en, llp_src_en;
} DCI_DMA_regs_t;

/**
 * \brief   Register and driver-state snapshot.
 *
 * DR is deliberately absent: reading it pops the FIFO and would corrupt the
 * capture in progress.
 */
typedef struct
{
    /* DCI registers, DR excluded. */
    uint32_t cr, sr, ris, ier, mis, icr;
    uint32_t escr, esur, cwstrt, cwsize;

    /* DMA channel and global state, already field-decoded by the SoC layer. */
    DCI_DMA_regs_t dma;

    /* Descriptors. */
    uint8_t  desc_count;
    struct
    {
        uint32_t sar, dar, llp, ctl_low, ctl_high;
    } desc[DCI_MAX_DESC];

    /* Driver state. */
    uint8_t  state;
    uint8_t  mode;
    uint8_t  var_len;
    uint8_t  notify_mode;
    uint8_t  buf_count;
    uint8_t  chunks_per_frame;
    uint16_t lines_per_chunk;
    uint8_t  line_aligned;
    uint8_t  chunk_idx;
    uint8_t  evt;
    uint8_t  buf_lock_mask;
    uint8_t  ready_count;
    uint8_t  cur_group;
    uint32_t frame_bytes;
    DCI_stats_t stats;
} DCI_snapshot_t;

void DCI_RegSnapshot(DCI_snapshot_t *snap);         /* ISR-safe, no printing. */
void DCI_DumpSnapshot(const DCI_snapshot_t *snap);  /* Thread context. */
void DCI_RegDump(void);
void DCI_RegMonitor(void);                          /* Only fields that changed. */

#endif /* DCI_DEBUG_REG_DUMP */

/** \} */ /* End of DCI_Exported_Functions */

/** \} */ /* End of DCI */

#ifdef __cplusplus
}
#endif

#endif /* RTL_DCI_H */
