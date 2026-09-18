/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DCI capture driver. SoC-independent part: register configuration, GDMA
 * orchestration, frame state machine and the register dump. See DESIGN.md.
 */

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include <string.h>
#include "rtl_dci.h"
#include "rtl_dci_int.h"

/*============================================================================*
 *                           Busy-wait Timeouts
 *============================================================================*/
/* Calibrated loop-count safety nets, not precise deadlines - same convention as
 * IDU_BUSY_WAIT in rtl_idu.c. Overridable per chip. */
#ifndef DCI_WAIT_FIFO_ITER
#define DCI_WAIT_FIFO_ITER      10000UL     /* channel FIFO drain: tens of bus cycles */
#endif

#ifndef DCI_WAIT_CAPTURE_ITER
#define DCI_WAIT_CAPTURE_ITER   2000000UL   /* thread context: may span a whole frame */
#endif

/* ISR context: clearing CAPTURE at a frame boundary takes effect within the
 * vertical blanking, so a short bound is enough. An ISR must never busy-wait for
 * a frame period; if this expires CAPTURE is still cleared and the hardware stops
 * at the end of the frame it is in, which is all the gate needs (DESIGN.md 8.2). */
#ifndef DCI_WAIT_CAPTURE_ISR_ITER
#define DCI_WAIT_CAPTURE_ISR_ITER   10000UL
#endif

#ifndef DCI_WAIT_ITER_PER_MS
#define DCI_WAIT_ITER_PER_MS    20000UL     /* WaitFrame spin budget per ms */
#endif

/*============================================================================*
 *                           Private Macros
 *============================================================================*/
#define DCI_EVT_FRAME           BIT0        /* IT_FRAME arrived */
#define DCI_EVT_BLOCK           BIT1        /* group-last block arrived */

#define DCI_DIV_UP(a, b)        (((a) + (b) - 1) / (b))

/*============================================================================*
 *                           Private Types
 *============================================================================*/
/** Crop window in hardware units: pixel clocks horizontally, lines vertically. */
typedef struct
{
    uint16_t x0;        /* CWSTRT.HOFFCNT */
    uint16_t y0;        /* CWSTRT.VST */
    uint16_t w;         /* pclk to capture, CWSIZE.CAPCNT = w - 1 */
    uint16_t h;         /* lines to capture, CWSIZE.VLINE = h - 1 */
} dci_crop_t;

/** Everything derived from init + crop + buf_bytes. Recomputed on every reconfig. */
typedef struct
{
    uint32_t frame_bytes;       /* 0 for JPEG: the length is only known per frame */
    uint32_t frame_words;
    uint32_t line_bytes;
    uint32_t line_words;        /* valid only when line_aligned */
    uint32_t chunk_words;       /* full chunk; the last one may be shorter */
    uint16_t lines_out;
    uint16_t lines_per_chunk;   /* 0 when not line aligned */
    uint8_t  chunks_per_frame;
    uint8_t  desc_count;
    bool     line_aligned;
    bool     var_len;           /* JPEG */
} dci_geom_t;

typedef struct
{
    DCI_InitTypeDef      init;
    DCI_DMA_config       dma;
    dci_crop_t           crop;
    dci_geom_t           g;

    bool                 dma_owned;     /* channel taken from the HAL allocator */

    DCI_CAPTURE_MODE_T   mode;
    bool                 mode_fixed;    /* mode is settled by the first Start() */
    bool                 ring_closed;   /* CONTINUOUS fixed length: LLP never 0 */
    bool                 armed;         /* channel loaded and enabled for cur_group */
    bool                 xfer_seen;     /* block filled before IT_FRAME */

    volatile DCI_CAPTURE_STATE_T state;
    volatile uint8_t     evt;           /* DCI_EVT_* rendezvous, CONTINUOUS fixed len */
    uint8_t              cur_group;     /* buffer the DMA is filling */
    uint8_t              chunk_idx;     /* blocks completed inside the group */
    uint8_t              buf_lock_mask; /* buffers held by the consumer */
    uint8_t              ready_head;    /* next buffer handed to the consumer */
    uint8_t              ready_count;
    uint32_t             last_bytes;    /* bytes of the most recent delivery */

    DCI_frame_t          frame[DCI_MAX_BUF];
    DCI_stats_t          stats;
} dci_ctrl_t;

/*============================================================================*
 *                           Private Variables
 *============================================================================*/
/* CPU writes it, DMA reads it. Static and non-cacheable rather than allocated:
 * deterministic, no allocation failure path (DESIGN.md 6.5). */
DCI_NONCACHE_SECTION static DCI_DMA_LLIDef dci_lli_ring[DCI_MAX_DESC];

static dci_ctrl_t dci;

/*============================================================================*
 *                           Forward Declarations
 *============================================================================*/
static void dci_dma_isr(void);

/*============================================================================*
 *                           Polling Helpers
 *============================================================================*/
/*
 * Clearing CAPTURE only takes effect at the end of the frame in flight, so the
 * bit has to be read back to know the frame boundary was reached. Same as ST's
 * HAL_DCMI_Suspend / HAL_DCMI_Stop.
 */
static DCI_ERROR dci_capture_off(uint32_t max_iter)
{
    uint32_t cnt = 0;

    DCI_CaptureCmd(DISABLE);
    while (DCI_GetCaptureStatus() != RESET)
    {
        if (++cnt > max_iter)
        {
            return DCI_ERROR_TIMEOUT;
        }
    }
    return DCI_SUCCESS;
}

/*
 * IT_FRAME only says the pixel stream ended; the last words may still sit in the
 * DMAC channel FIFO. CURR_DAR is short until this drains (DESIGN.md 9.3).
 */
static DCI_ERROR dci_wait_fifo_empty(void)
{
    uint32_t cnt = 0;

    while (!rtl_dci_dma_fifo_empty(dci.dma.dma_channel_num))
    {
        if (++cnt > DCI_WAIT_FIFO_ITER)
        {
            return DCI_ERROR_TIMEOUT;
        }
    }
    return DCI_SUCCESS;
}

static void dci_dma_stop(void)
{
    rtl_dci_dma_cmd(dci.dma.dma_channel_num, false);
    rtl_dci_dma_int_clear_all(dci.dma.dma_channel_num);
    dci.armed = false;
}

/*============================================================================*
 *                           Geometry (DESIGN.md 6.2, 6.3)
 *============================================================================*/
/* Byte decimation ratio of CR.BSM, as numerator / denominator. */
static void dci_bsm_ratio(DCI_BSM_T bsm, uint32_t *num, uint32_t *den)
{
    switch (bsm)
    {
    case DCI_BSM_EVERY_2ND:
        *num = 1;
        *den = 2;
        break;
    case DCI_BSM_1_OF_4:
        *num = 1;
        *den = 4;
        break;
    case DCI_BSM_2_OF_4:
        *num = 2;
        *den = 4;
        break;
    default:
        *num = 1;
        *den = 1;
        break;
    }
}

/*
 * frame_bytes cannot be read back from the hardware: with CROP disabled the DCI
 * captures whatever HSYNC/VSYNC delimit, so the sensor geometry has to be
 * declared. On top of that EDM sets the bytes per pixel clock while BSM and LSM
 * decimate bytes and lines (DESIGN.md 12.6-D2).
 */
static DCI_ERROR dci_calc_geometry(const DCI_InitTypeDef *init, const DCI_DMA_config *dma,
                                   const dci_crop_t *crop, dci_geom_t *g)
{
    uint32_t bytes_per_pclk = (init->edm == DCI_EDM_8BIT) ? 1 : 2;
    uint32_t pclk_per_line;
    uint32_t lines_in;
    uint32_t bsm_num, bsm_den;
    uint32_t n;

    memset(g, 0, sizeof(*g));
    g->var_len = init->jpeg;

    if (init->jpeg)
    {
        /* No declared length: one block covering the whole buffer, LLP = 0, so a
         * full block means the stream overflowed the buffer (DESIGN.md 9.2). */
        g->chunk_words      = dma->buf_bytes / 4;
        g->chunks_per_frame = 1;
        g->desc_count       = dma->buf_count;
        if (g->chunk_words == 0 || g->chunk_words > DCI_MAX_BLOCK_TS)
        {
            return DCI_ERROR_SIZE;
        }
        return DCI_SUCCESS;
    }

    pclk_per_line = init->crop ? crop->w : init->sensor_pclk_per_line;
    lines_in      = init->crop ? crop->h : init->sensor_lines;

    /* Line decimation: OELS picks the first or the second line of each pair. */
    if (init->lsm)
    {
        g->lines_out = (uint16_t)(init->oels ? (lines_in / 2) : DCI_DIV_UP(lines_in, 2));
    }
    else
    {
        g->lines_out = (uint16_t)lines_in;
    }
    if (g->lines_out == 0)
    {
        return DCI_ERROR_INVALID_PARAM;
    }

    dci_bsm_ratio(init->bsm, &bsm_num, &bsm_den);
    g->line_bytes = pclk_per_line * bytes_per_pclk;
    if ((g->line_bytes % bsm_den) != 0)
    {
        /* A partial decimation group at the end of a line would shift every
         * following line, so refuse instead of guessing. */
        return DCI_ERROR_INVALID_PARAM;
    }
    g->line_bytes = g->line_bytes / bsm_den * bsm_num;
    if (g->line_bytes == 0)
    {
        return DCI_ERROR_INVALID_PARAM;
    }

    g->frame_bytes  = g->line_bytes * g->lines_out;
    g->frame_words  = DCI_DIV_UP(g->frame_bytes, 4);
    g->line_aligned = ((g->line_bytes % 4) == 0);

    if (g->line_aligned)
    {
        /* Grow the block count only when a block cannot hold the frame, one step
         * at a time, and keep chunk boundaries on line boundaries. */
        g->line_words = g->line_bytes / 4;
        for (n = 1; n <= DCI_MAX_CHUNK; n++)
        {
            uint32_t lpc = DCI_DIV_UP(g->lines_out, n);
            if (lpc * g->line_words <= DCI_MAX_BLOCK_TS)
            {
                g->lines_per_chunk = (uint16_t)lpc;
                g->chunk_words     = lpc * g->line_words;
                break;
            }
        }
    }
    else
    {
        /* Degenerate case: the line length is not a whole number of words (for
         * example 466 wide 8-bit), so split by words. Invisible to the caller
         * because the block layout is not exposed; only the dump shows it. */
        n = DCI_DIV_UP(g->frame_words, DCI_MAX_BLOCK_TS);
        if (n <= DCI_MAX_CHUNK)
        {
            g->chunk_words = DCI_DIV_UP(g->frame_words, n);
        }
    }

    if (n > DCI_MAX_CHUNK || g->chunk_words == 0 || g->chunk_words > DCI_MAX_BLOCK_TS)
    {
        return DCI_ERROR_SIZE;
    }

    g->chunks_per_frame = (uint8_t)n;
    g->desc_count       = (uint8_t)(dma->buf_count * n);
    if (g->desc_count > DCI_MAX_DESC)
    {
        return DCI_ERROR_SIZE;
    }
    return DCI_SUCCESS;
}

/*============================================================================*
 *                           Validation (DESIGN.md 15)
 *============================================================================*/
static DCI_ERROR dci_validate(const DCI_InitTypeDef *init, const DCI_DMA_config *dma,
                              const dci_crop_t *crop, dci_geom_t *g)
{
    DCI_ERROR err;
    uint8_t   i;

    if (init->edm > DCI_EDM_10BIT || init->fcrc > DCI_FCRC_QUARTER ||
        init->bsm > DCI_BSM_2_OF_4 || init->sync > DCI_SYNC_EMBEDDED)
    {
        return DCI_ERROR_INVALID_PARAM;
    }
    if (init->sensor_pclk_per_line == 0 || init->sensor_lines == 0)
    {
        return DCI_ERROR_INVALID_PARAM;
    }

    /* spec 2.7: JPEG rules out crop and embedded sync. spec 2.4: embedded codes
     * are 8-bit only. Register table: BSM is valid only when EDM[1:0] = 00. */
    if (init->jpeg && (init->crop || init->sync == DCI_SYNC_EMBEDDED))
    {
        return DCI_ERROR_MODE_CONFLICT;
    }
    if (init->sync == DCI_SYNC_EMBEDDED && init->edm != DCI_EDM_8BIT)
    {
        return DCI_ERROR_MODE_CONFLICT;
    }
    if (init->bsm != DCI_BSM_ALL && init->edm != DCI_EDM_8BIT)
    {
        return DCI_ERROR_MODE_CONFLICT;
    }

    /* spec 2.9 caps non-JPEG frames at 2048x2048. The driver cannot convert pclk
     * to pixels - that ratio is a property of the pixel format, which DCI never
     * sees - so bound the pclk count by the loosest legal case (2 pclk/pixel). */
    if (!init->jpeg)
    {
        if (init->sensor_lines > DCI_MAX_PIXELS ||
            init->sensor_pclk_per_line > (DCI_MAX_PIXELS * 2))
        {
            return DCI_ERROR_SIZE;
        }
    }

    if (init->crop && (crop->w == 0 || crop->h == 0))
    {
        return DCI_ERROR_INVALID_PARAM;
    }
    /* Checked whenever a window is set, not only while CROP is on, so that
     * DCI_ConfigCrop() rejects a bad window at once instead of letting a later
     * DCI_CropCmd(ENABLE) fail for a reason the caller has to guess. */
    if (crop->w != 0 || crop->h != 0)
    {
        if (crop->w > 0x4000U || crop->h > 0x4000U)   /* CAPCNT / VLINE are 14 bit */
        {
            return DCI_ERROR_SIZE;
        }
        /* Register table, CROP: "If crop window exceeds picture size, only
         * picture size is captured" - the hardware truncates silently, which
         * lands straight in the desync case of DESIGN.md 7.1. */
        if ((uint32_t)crop->x0 + crop->w > init->sensor_pclk_per_line ||
            (uint32_t)crop->y0 + crop->h > init->sensor_lines)
        {
            return DCI_ERROR_SIZE;
        }
    }

    if (dma->buf_count < DCI_BUF_SINGLE || dma->buf_count > DCI_MAX_BUF)
    {
        return DCI_ERROR_INVALID_PARAM;
    }
    if (dma->on_busy > DCI_ON_BUSY_OVERWRITE || dma->notify_mode > DCI_NOTIFY_POLL)
    {
        return DCI_ERROR_INVALID_PARAM;
    }
    if (dma->notify_mode == DCI_NOTIFY_CALLBACK && dma->frame_ready == NULL)
    {
        return DCI_ERROR_NO_CALLBACK;
    }
    if (dma->buf_bytes == 0 || (dma->buf_bytes & 0x3U) != 0)
    {
        return DCI_ERROR_ALIGN;
    }
    for (i = 0; i < dma->buf_count; i++)
    {
        if (dma->buf[i] == NULL)
        {
            return DCI_ERROR_INVALID_PARAM;
        }
        if (((uint32_t)dma->buf[i] & 0x3U) != 0)
        {
            return DCI_ERROR_ALIGN;
        }
    }
    if (rtl_dci_get_dma_depth(dma->dma_channel_num) == 0)
    {
        return DCI_ERROR_DMA_CHANNEL;
    }
    if (dma->dma_priority > 15)
    {
        return DCI_ERROR_INVALID_PARAM;
    }

    err = dci_calc_geometry(init, dma, crop, g);
    if (err != DCI_SUCCESS)
    {
        return err;
    }

    /* Core invariant: a frame lands entirely inside one buffer, contiguously
     * (DESIGN.md 6.7). JPEG has no declared length, so there buf_bytes is a
     * capacity cap and an overrun is reported per frame instead. */
    if (!g->var_len && dma->buf_bytes < g->frame_bytes)
    {
        return DCI_ERROR_BUF_TOO_SMALL;
    }
    return DCI_SUCCESS;
}

/*============================================================================*
 *                           Register Configuration
 *============================================================================*/
void DCI_StructInit(DCI_InitTypeDef *init)
{
    if (init == NULL)
    {
        return;
    }
    memset(init, 0, sizeof(*init));
    init->edm  = DCI_EDM_8BIT;
    init->sync = DCI_SYNC_HARDWARE;
    init->fcrc = DCI_FCRC_ALL;
    init->bsm  = DCI_BSM_ALL;
}

/*
 * Writes every configuration field of CR with ENABLE and CAPTURE left at 0: the
 * register table requires configuration to be programmed before ENABLE is set.
 * CR.CM belongs to the capture session and is written by DCI_Capture_Start().
 */
void DCI_Init(const DCI_InitTypeDef *init)
{
    DCI_CR_TypeDef   cr   = {.d32 = 0};
    DCI_ESCR_TypeDef escr = {.d32 = 0};
    DCI_ESUR_TypeDef esur = {.d32 = 0};

    if (init == NULL)
    {
        return;
    }

    DCI->CR = 0;

    cr.b.crop   = init->crop ? 1 : 0;
    cr.b.jpeg   = init->jpeg ? 1 : 0;
    cr.b.ess    = (init->sync == DCI_SYNC_EMBEDDED) ? 1 : 0;
    cr.b.pckpol = init->pckpol ? 1 : 0;
    cr.b.hspol  = init->hspol ? 1 : 0;
    cr.b.vspol  = init->vspol ? 1 : 0;
    cr.b.fcrc   = init->fcrc;
    cr.b.edm    = init->edm;
    cr.b.bsm    = init->bsm;
    cr.b.oebs   = init->oebs ? 1 : 0;
    cr.b.lsm    = init->lsm ? 1 : 0;
    cr.b.oels   = init->oels ? 1 : 0;
    DCI->CR = cr.d32;

    escr.b.fsc = init->sync_code.frame_start;
    escr.b.lsc = init->sync_code.line_start;
    escr.b.lec = init->sync_code.line_end;
    escr.b.fec = init->sync_code.frame_end;
    DCI->ESCR = escr.d32;

    esur.b.fsu = init->sync_unmask.frame_start;
    esur.b.lsu = init->sync_unmask.line_start;
    esur.b.leu = init->sync_unmask.line_end;
    esur.b.feu = init->sync_unmask.frame_end;
    DCI->ESUR = esur.d32;
}

/* CWSTRT / CWSIZE use N-1 encoding for the counted fields. */
static void dci_write_crop(void)
{
    DCI_CWSTRT_TypeDef strt = {.d32 = 0};
    DCI_CWSIZE_TypeDef size = {.d32 = 0};

    if (dci.crop.w == 0 || dci.crop.h == 0)
    {
        return;
    }
    strt.b.hoffcnt = dci.crop.x0;
    strt.b.vst     = dci.crop.y0;
    DCI->CWSTRT = strt.d32;

    size.b.capcnt = (uint32_t)dci.crop.w - 1;
    size.b.vline  = (uint32_t)dci.crop.h - 1;
    DCI->CWSIZE = size.d32;
}

static void dci_write_mode_bit(void)
{
    DCI_CR_TypeDef cr = {.d32 = DCI->CR};

    cr.b.cm = (dci.mode == DCI_MODE_SNAPSHOT) ? 1 : 0;
    DCI->CR = cr.d32;
}

/*
 * An interrupt with no consumer is left masked. OVR/ERR/FRAME are always on
 * because the driver itself needs them; VSYNC and LINE follow their callbacks.
 * LINE in particular is 28k/s at 466 lines @60fps, which is why ST's
 * unconditional LINE | VSYNC | ERR | OVR default is not copied (DESIGN.md 11.4).
 */
static void dci_int_config(void)
{
    uint32_t en = DCI_INT_FRAME | DCI_INT_OVR | DCI_INT_ERR;

    if (dci.dma.vsync_event != NULL)
    {
        en |= DCI_INT_VSYNC;
    }
    if (dci.dma.line_event != NULL)
    {
        en |= DCI_INT_LINE;
    }
    DCI->IER = en;
}

/*
 * A closed ring never presents LLP = 0, so transfer-complete can never fire and
 * the group-last block is the only completion signal. The LLP = 0 topologies are
 * the mirror image: Transfer fires and means the data exceeded the quota
 * (DESIGN.md 6.4).
 */
static void dci_dma_int_config(void)
{
    rtl_dci_dma_int_select(dci.dma.dma_channel_num, dci.ring_closed);
}

/*============================================================================*
 *                     Descriptor Ring (DESIGN.md 6.4)
 *============================================================================*/
/* Words and byte offset of chunk i inside one frame. */
static void dci_chunk_span(uint8_t i, uint32_t *words, uint32_t *offset)
{
    if (dci.g.var_len)
    {
        *words  = dci.g.chunk_words;
        *offset = 0;
    }
    else if (dci.g.line_aligned)
    {
        uint32_t first = (uint32_t)i * dci.g.lines_per_chunk;
        uint32_t lines = dci.g.lines_out - first;

        if (lines > dci.g.lines_per_chunk)
        {
            lines = dci.g.lines_per_chunk;
        }
        *words  = lines * dci.g.line_words;
        *offset = first * dci.g.line_bytes;
    }
    else
    {
        uint32_t first = (uint32_t)i * dci.g.chunk_words;
        uint32_t words_left = dci.g.frame_words - first;

        *words  = (words_left > dci.g.chunk_words) ? dci.g.chunk_words : words_left;
        *offset = first * 4;
    }
}

/*
 * One group of chunks_per_frame descriptors per buffer. CONTINUOUS links the
 * groups head to tail into a closed ring so the hardware switches buffers on its
 * own; SNAPSHOT leaves each group ending at LLP = 0 so the channel stops dead
 * and can be reloaded from a known state on the next Start().
 */
static void dci_build_ring(void)
{
    uint8_t  ch       = dci.dma.dma_channel_num;
    uint32_t ctl_lli  = rtl_dci_dma_ctl_low(ch, true);
    uint32_t ctl_term = rtl_dci_dma_ctl_low(ch, false);
    uint8_t  chunks   = dci.g.chunks_per_frame;
    uint8_t  k, i;

    memset(dci_lli_ring, 0, sizeof(dci_lli_ring));

    for (k = 0; k < dci.dma.buf_count; k++)
    {
        for (i = 0; i < chunks; i++)
        {
            uint8_t  d = (uint8_t)(k * chunks + i);
            uint32_t words, offset;
            bool     last = (i == chunks - 1);

            dci_chunk_span(i, &words, &offset);

            dci_lli_ring[d].SAR      = (uint32_t)&DCI->DR;
            dci_lli_ring[d].DAR      = (uint32_t)dci.dma.buf[k] + offset;
            dci_lli_ring[d].CTL_HIGH = words;

            if (!last)
            {
                dci_lli_ring[d].LLP     = (uint32_t)&dci_lli_ring[d + 1];
                dci_lli_ring[d].CTL_LOW = ctl_lli;
            }
            else if (dci.ring_closed)
            {
                uint8_t next = (uint8_t)(((k + 1) % dci.dma.buf_count) * chunks);

                dci_lli_ring[d].LLP     = (uint32_t)&dci_lli_ring[next];
                dci_lli_ring[d].CTL_LOW = ctl_lli;
            }
            else
            {
                dci_lli_ring[d].LLP     = 0;
                dci_lli_ring[d].CTL_LOW = ctl_term;
            }
        }
    }
}

/*
 * Loads the channel for one group and opens the gate. The channel registers are
 * programmed with the contents of the group's first descriptor on purpose: the
 * two DW DMAC conventions (start from the register set, or fetch the first LLI)
 * then produce the same block sequence.
 */
static void dci_arm(uint8_t group)
{
    uint8_t ch   = dci.dma.dma_channel_num;
    uint8_t head = (uint8_t)(group * dci.g.chunks_per_frame);

    rtl_dci_dma_arm(ch, (uint32_t)&DCI->DR, dci_lli_ring[head].DAR,
                    dci_lli_ring[head].CTL_HIGH, &dci_lli_ring[head],
                    dci.dma.dma_priority);
    rtl_dci_dma_int_clear_all(ch);
    dci_dma_int_config();

    dci.cur_group = group;
    dci.chunk_idx = 0;
    dci.evt       = 0;
    dci.xfer_seen = false;
    dci.armed     = true;

    rtl_dci_dma_cmd(ch, true);
    DCI_CaptureCmd(ENABLE);
}

/*============================================================================*
 *                     Delivery and Back-pressure (DESIGN.md 8, 11)
 *============================================================================*/
/*
 * Hands buffer k to the consumer and takes ownership away from the driver. The
 * lock happens here, at delivery, not when the consumer collects: a completed but
 * uncollected buffer left unlocked would be overwritten once the ring came round
 * again (DESIGN.md 11.5).
 */
static void dci_deliver(uint8_t k, uint32_t bytes, bool truncated)
{
    DCI_frame_t *f = &dci.frame[k];
    uint32_t     s = rtl_dci_lock();

    if ((dci.buf_lock_mask & BIT(k)) != 0)
    {
        /* Only reachable under OVERWRITE: delivered before, never collected. The
         * consumer sees it as a gap in seq. */
        dci.stats.frames_dropped++;
    }
    dci.buf_lock_mask |= (uint8_t)BIT(k);

    dci.stats.seq++;
    dci.stats.frames_done++;
    dci.last_bytes = bytes;

    f->addr      = (uint8_t *)dci.dma.buf[k];
    f->bytes     = bytes;
    f->index     = k;
    f->seq       = dci.stats.seq;
    f->truncated = truncated;

    if (dci.dma.notify_mode == DCI_NOTIFY_POLL)
    {
        /* The ring makes completion order a fixed rotation, so the queue is just
         * a head and a count (DESIGN.md 11.3). */
        if (dci.ready_count == 0)
        {
            dci.ready_head  = k;
            dci.ready_count = 1;
        }
        else if (dci.ready_count < dci.dma.buf_count)
        {
            dci.ready_count++;
        }
        else
        {
            /* Full: the oldest entry is the one being overwritten. */
            dci.ready_head = (uint8_t)((dci.ready_head + 1) % dci.dma.buf_count);
        }
    }
    rtl_dci_unlock(s);

    /* POLL never calls frame_ready, not even as a "data available" hint: one
     * event must not have two entry points (DESIGN.md 11.2). */
    if (dci.dma.notify_mode == DCI_NOTIFY_CALLBACK && dci.dma.frame_ready != NULL)
    {
        dci.dma.frame_ready(f);
    }
}

static void dci_report(DCI_ERROR err, uint32_t ris)
{
    if (dci.dma.frame_error != NULL)
    {
        dci.dma.frame_error(err, ris);
    }
}

/*
 * Runs after the delivery call has returned, never before: a consumer may finish
 * in the frame_ready callback and release the buffer synchronously, in which case
 * there is nothing to gate. Checking first would waste a frame (DESIGN.md 8.2).
 */
static void dci_gate_check(uint8_t next_group)
{
    uint32_t s;

    if (dci.dma.on_busy == DCI_ON_BUSY_OVERWRITE)
    {
        return;
    }

    s = rtl_dci_lock();
    if (dci.state != DCI_STATE_BUSY || (dci.buf_lock_mask & BIT(next_group)) == 0)
    {
        rtl_dci_unlock(s);
        return;
    }
    dci.state = DCI_STATE_SUSPENDED;
    dci.stats.suspend_count++;
    rtl_dci_unlock(s);

    /* Stops at the frame boundary. The DMA channel is untouched, so it stays
     * parked at the start of the next group with 0 words transferred - the reason
     * a resume needs no DMA work at all, exactly like ST's HAL_DCMI_Resume. */
    (void)dci_capture_off(DCI_WAIT_CAPTURE_ISR_ITER);
}

/*
 * CONTINUOUS JPEG only: the gate decision and the channel reload are the same
 * decision point, so they share one path (DESIGN.md 9.4 step 4).
 */
static void dci_rearm_or_gate(void)
{
    uint8_t  next;
    uint32_t s = rtl_dci_lock();

    next = (uint8_t)((dci.cur_group + 1) % dci.dma.buf_count);
    dci.cur_group = next;

    if (dci.dma.on_busy == DCI_ON_BUSY_GATE && (dci.buf_lock_mask & BIT(next)) != 0)
    {
        dci.state = DCI_STATE_SUSPENDED;
        dci.stats.suspend_count++;
        rtl_dci_unlock(s);
        return;             /* CAPTURE was already cleared before delivery */
    }
    rtl_dci_unlock(s);

    dci_arm(next);
}

/*============================================================================*
 *                     Resync (DESIGN.md 7.2.1)
 *============================================================================*/
/*
 * A counting DMA cannot be spliced back onto a stream that lost words, so
 * recovery is always tear down and restart from a frame boundary - never
 * "continue". ST does the same (unconditional HAL_DMA_Abort_IT on OVR/ERR).
 */
static void dci_resync(void)
{
    (void)dci_capture_off(DCI_WAIT_CAPTURE_ISR_ITER);
    dci_dma_stop();

    DCI_Cmd(DISABLE);           /* resets the DCI FIFO */
    DCI_Cmd(ENABLE);
    DCI_ClearINTPendingBit(DCI_INT_ALL);

    dci.chunk_idx = 0;
    dci.evt       = 0;
    dci.stats.resync_count++;
    dci.stats.frames_dropped++;

    /* Restart from group 0. ready_head needs no fixing: it re-derives itself from
     * the next delivery whenever the queue is empty, and while the queue is not
     * empty its rotation order is still valid. */
    if (dci.dma.on_busy == DCI_ON_BUSY_GATE && (dci.buf_lock_mask & BIT(0)) != 0)
    {
        dci.cur_group = 0;
        dci.state     = DCI_STATE_SUSPENDED;
        dci.stats.suspend_count++;
        return;
    }
    dci_arm(0);
    dci.state = DCI_STATE_BUSY;
}

/*============================================================================*
 *                     Frame Completion (DESIGN.md 7.2)
 *============================================================================*/
/*
 * Shared by the three LLP = 0 topologies: both SNAPSHOT variants and CONTINUOUS
 * JPEG. All three stop the DMA dead at the end of the frame, which makes
 * CURR_DAR trustworthy, so IT_FRAME alone settles the frame - no rendezvous.
 */
static void dci_finish_stopped(bool over_quota, uint32_t ris)
{
    uint8_t   k   = dci.cur_group;
    DCI_ERROR err = DCI_SUCCESS;
    uint32_t  bytes;

    /* CONTINUOUS does not self-clear CAPTURE, so close the gate first and do not
     * gamble on finishing the reload inside the vertical blanking. Worst case is
     * one cleanly skipped frame instead of an overrun (DESIGN.md 9.4 step 1). */
    if (dci.mode == DCI_MODE_CONTINUOUS || over_quota)
    {
        (void)dci_capture_off(DCI_WAIT_CAPTURE_ISR_ITER);
    }

    (void)dci_wait_fifo_empty();

    if (over_quota || dci.xfer_seen)
    {
        /* The block filled, so exactly its quota reached memory. */
        bytes = dci.g.var_len ? dci.dma.buf_bytes : dci.g.frame_bytes;
        if (over_quota)
        {
            err = dci.g.var_len ? DCI_ERROR_BUF_OVERFLOW : DCI_ERROR_FRAME_LONG;
        }
    }
    else
    {
        bytes = rtl_dci_dma_curr_dar(dci.dma.dma_channel_num) -
                (uint32_t)dci.dma.buf[k];
        if (!dci.g.var_len && bytes < dci.g.frame_bytes)
        {
            /* spec 2.6 guarantees IT_FRAME even for a short frame, so this is
             * observable rather than silent corruption. It usually means the
             * declared sensor geometry is larger than the real output. */
            err = DCI_ERROR_FRAME_SHORT;
        }
    }

    dci_dma_stop();

    /* Delivered even when truncated: the buffer holds a clean prefix, and only
     * the delivery path locks it. Reporting the error without delivering would
     * leave the buffer ownerless and the next Start() would overwrite it under
     * the consumer (DESIGN.md 9.2). */
    dci_deliver(k, bytes, err != DCI_SUCCESS);

    if (dci.mode == DCI_MODE_SNAPSHOT)
    {
        /* Rotate so a following Start() fills the other buffer while the consumer
         * still works on this one (DESIGN.md 6.4). Also on the error paths: a
         * truncated frame is still held by the consumer, so a retry belongs in
         * the other buffer. */
        dci.cur_group = (uint8_t)((k + 1) % dci.dma.buf_count);
    }

    if (err != DCI_SUCCESS)
    {
        if (err == DCI_ERROR_BUF_OVERFLOW)
        {
            dci.stats.buf_overflow++;
        }
        dci.state = DCI_STATE_ERROR;
        dci_report(err, ris);
        return;
    }

    if (dci.mode == DCI_MODE_SNAPSHOT)
    {
        dci.state = DCI_STATE_IDLE;
        return;
    }

    dci_rearm_or_gate();
}

/*
 * CONTINUOUS fixed length: the closed ring gives no Transfer event, so the frame
 * is settled by a rendezvous between IT_FRAME and the group-last block. Whichever
 * arrives first only sets its bit; the same bit set twice in a row means the
 * other half never came, which catches both a short and a long frame and cannot
 * misfire on the first frame (DESIGN.md 7.2).
 */
static void dci_rendezvous(uint8_t bit, uint32_t ris)
{
    uint8_t  k, next;
    uint32_t s = rtl_dci_lock();

    if ((dci.evt & bit) != 0)
    {
        dci.evt = 0;
        rtl_dci_unlock(s);
        dci.state = DCI_STATE_ERROR;
        dci_resync();
        dci_report(DCI_ERROR_DESYNC, ris);
        return;
    }
    dci.evt |= bit;

    if (dci.evt != (DCI_EVT_FRAME | DCI_EVT_BLOCK))
    {
        rtl_dci_unlock(s);
        return;
    }
    dci.evt = 0;

    k    = dci.cur_group;
    next = (uint8_t)((k + 1) % dci.dma.buf_count);
    dci.cur_group = next;
    rtl_dci_unlock(s);

    /* The group-last block completing is what proves BLOCK_TS words reached
     * memory; the closed ring means CURR_DAR has already moved on, so the length
     * is the declared frame size (DESIGN.md 7.2 note). */
    dci_deliver(k, dci.g.frame_bytes, false);
    dci_gate_check(next);
}

/*============================================================================*
 *                     Interrupt Service Routines
 *============================================================================*/
/* DMA channel ISR. Only one of transfer / block is unmasked per topology. */
static void dci_dma_isr(void)
{
    uint8_t ch = dci.dma.dma_channel_num;

    if (rtl_dci_dma_int_take_block(ch))
    {
        dci.stats.chunks_done++;

        if (dci.state != DCI_STATE_BUSY)
        {
            return;
        }
        if (++dci.chunk_idx < dci.g.chunks_per_frame)
        {
            return;
        }
        dci.chunk_idx = 0;
        dci_rendezvous(DCI_EVT_BLOCK, DCI->RIS);
        return;
    }

    if (!rtl_dci_dma_int_take_transfer(ch))
    {
        return;
    }
    dci.stats.chunks_done++;

    if (!dci.armed || dci.state != DCI_STATE_BUSY)
    {
        return;
    }

    /* RIS latches independently of IER, so it settles the Transfer-versus-FRAME
     * race without a timeout. FRAME already latched means the frame really did
     * end here and the FRAME path owns the completion. */
    if (DCI_GetRawINTStatus(DCI_INT_FRAME) == SET)
    {
        dci.xfer_seen = true;
        return;
    }

    dci.armed = false;
    dci_finish_stopped(true, DCI->RIS);
}

/*
 * Fatal for the frame in flight: spec 2.8 says an overrun loses words with no
 * protection, and an embedded-code sequence error means the frame structure is
 * already scrambled. Neither leaves a clean prefix, so nothing is delivered and
 * no buffer is locked (DESIGN.md 9.2).
 */
static void dci_fail(DCI_ERROR err, uint32_t ris)
{
    if (err == DCI_ERROR_OVERRUN)
    {
        dci.stats.ovr_count++;
    }
    else
    {
        dci.stats.err_count++;
    }

    if (dci.mode == DCI_MODE_CONTINUOUS)
    {
        dci.state = DCI_STATE_ERROR;
        dci_resync();
        dci_report(err, ris);
        return;
    }

    (void)dci_capture_off(DCI_WAIT_CAPTURE_ISR_ITER);
    dci_dma_stop();
    dci.stats.frames_dropped++;
    dci.state = DCI_STATE_ERROR;
    dci_report(err, ris);
}

void DCI_Handler(void)
{
    uint32_t ris = DCI->RIS;
    uint32_t mis = ris & DCI->IER;

    if ((mis & DCI_INT_LINE) != 0)
    {
        DCI_ClearINTPendingBit(DCI_INT_LINE);
        if (dci.dma.line_event != NULL)
        {
            dci.dma.line_event();
        }
    }

    if ((mis & DCI_INT_VSYNC) != 0)
    {
        DCI_ClearINTPendingBit(DCI_INT_VSYNC);
        if (dci.dma.vsync_event != NULL)
        {
            dci.dma.vsync_event();
        }
    }

    if ((mis & (DCI_INT_OVR | DCI_INT_ERR)) != 0)
    {
        /* The frame in flight is scrap, so its FRAME event goes with it. */
        DCI_ClearINTPendingBit(DCI_INT_OVR | DCI_INT_ERR | DCI_INT_FRAME);
        dci_fail(((mis & DCI_INT_OVR) != 0) ? DCI_ERROR_OVERRUN : DCI_ERROR_SYNC, ris);
        return;
    }

    if ((mis & DCI_INT_FRAME) != 0)
    {
        DCI_ClearINTPendingBit(DCI_INT_FRAME);

        if (dci.state != DCI_STATE_BUSY)
        {
            return;
        }
        if (dci.ring_closed)
        {
            dci_rendezvous(DCI_EVT_FRAME, ris);
        }
        else if (dci.armed)
        {
            dci.armed = false;
            dci_finish_stopped(false, ris);
        }
    }
}

/*============================================================================*
 *                     Capture Layer
 *============================================================================*/
/*
 * Releases a channel only if this driver allocated it, so that a caller who pinned
 * a channel number keeps ownership of it.
 */
static void dci_dma_release(void)
{
    if (dci.dma_owned)
    {
        rtl_dci_dma_channel_free(dci.dma.dma_channel_num);
        dci.dma_owned = false;
    }
}

DCI_ERROR DCI_Capture_Init(const DCI_InitTypeDef *init, const DCI_DMA_config *dma)
{
    DCI_DMA_config cand;
    dci_geom_t     g;
    DCI_ERROR      err;
    uint8_t        ch;

    if (init == NULL || dma == NULL)
    {
        return DCI_ERROR_INVALID_PARAM;
    }

    memset(&dci, 0, sizeof(dci));
    cand = *dma;

    /* Clock first: everything below reads or writes registers. */
    rtl_dci_clock_enable();
    rtl_dci_dma_clock_enable();

    /* The channel has to be reserved before validation, because the FIFO depth that
     * validation checks is a property of the channel the HAL actually hands out. */
    ch = cand.dma_channel_num;
    if (!rtl_dci_dma_channel_alloc(&ch, dci_dma_isr))
    {
        return DCI_ERROR_DMA_CHANNEL;
    }
    cand.dma_channel_num = ch;

    err = dci_validate(init, &cand, &dci.crop, &g);
    if (err != DCI_SUCCESS)
    {
        /* Give the channel back: a rejected configuration must not leak it. */
        rtl_dci_dma_channel_free(ch);
        return err;
    }

    dci.init      = *init;
    dci.dma       = cand;
    dci.g         = g;
    dci.dma_owned = true;

    rtl_dci_pinmux_init();
    rtl_dci_irq_init(DCI_Handler);

    DCI_Init(&dci.init);
    dci_write_crop();
    DCI_ClearINTPendingBit(DCI_INT_ALL);
    dci_int_config();

    dci.state = DCI_STATE_IDLE;
    return DCI_SUCCESS;
}

/*
 * The mode fixes the descriptor topology, so it is settled by the first Start()
 * and immutable afterwards: switching between burst capture and streaming
 * mid-session has no real use case, and allowing it would mean rebuilding
 * everything (DESIGN.md 12.2).
 */
DCI_ERROR DCI_Capture_Start(DCI_CAPTURE_MODE_T mode)
{
    uint8_t   group;
    DCI_ERROR err;

    if (dci.state == DCI_STATE_RESET)
    {
        return DCI_ERROR_NOT_INIT;
    }
    if (dci.state == DCI_STATE_BUSY || dci.state == DCI_STATE_SUSPENDED)
    {
        return DCI_ERROR_STATE;
    }
    if (mode > DCI_MODE_SNAPSHOT)
    {
        return DCI_ERROR_INVALID_PARAM;
    }
    if (dci.mode_fixed && mode != dci.mode)
    {
        return DCI_ERROR_INVALID_PARAM;
    }

    /* CONTINUOUS always enters the closed ring at group 0; SNAPSHOT continues the
     * rotation so the application can process one shot while the next is taken. */
    group = (mode == DCI_MODE_SNAPSHOT) ? dci.cur_group : 0;
    if ((dci.buf_lock_mask & BIT(group)) != 0)
    {
        /* Failing fast beats silent gating here: a snapshot is an explicit
         * one-shot request, and "Start did nothing" reads like a hang. */
        return DCI_ERROR_BUSY;
    }

    err = dci_capture_off(DCI_WAIT_CAPTURE_ITER);
    if (err != DCI_SUCCESS)
    {
        return err;
    }
    dci_dma_stop();

    dci.mode        = mode;
    dci.mode_fixed  = true;
    dci.ring_closed = (mode == DCI_MODE_CONTINUOUS) && !dci.g.var_len;

    DCI_Cmd(DISABLE);
    dci_write_mode_bit();
    dci_build_ring();
    DCI_ClearINTPendingBit(DCI_INT_ALL);
    dci_int_config();
    DCI_Cmd(ENABLE);

    dci.state = DCI_STATE_BUSY;
    dci_arm(group);
    return DCI_SUCCESS;
}

/* Graceful: stops at a frame boundary. Frames already delivered stay valid, so
 * buf_lock_mask is left alone - only Abort() reclaims by force. */
DCI_ERROR DCI_Capture_Stop(void)
{
    DCI_ERROR err;

    if (dci.state == DCI_STATE_RESET)
    {
        return DCI_ERROR_NOT_INIT;
    }

    err = dci_capture_off(DCI_WAIT_CAPTURE_ITER);
    dci_dma_stop();
    DCI_Cmd(DISABLE);

    dci.chunk_idx = 0;
    dci.evt       = 0;
    dci.state     = DCI_STATE_IDLE;
    return err;
}

/*
 * The escape hatch. Contract (DESIGN.md 11.5): once this returns, every frame
 * delivered so far is invalid and the application must not touch frame.addr
 * again. There is no invalidation flag to check - buf_lock_mask is simply
 * cleared, and the next Start() may overwrite that memory immediately.
 */
DCI_ERROR DCI_Capture_Abort(void)
{
    uint32_t s;

    if (dci.state == DCI_STATE_RESET)
    {
        return DCI_ERROR_NOT_INIT;
    }

    DCI_CaptureCmd(DISABLE);        /* immediate, no read-back poll */
    dci_dma_stop();
    DCI_Cmd(DISABLE);

    s = rtl_dci_lock();
    dci.buf_lock_mask = 0;
    dci.ready_head    = 0;
    dci.ready_count   = 0;
    dci.cur_group     = 0;
    dci.chunk_idx     = 0;
    dci.evt           = 0;
    dci.state         = DCI_STATE_IDLE;
    rtl_dci_unlock(s);

    return DCI_SUCCESS;
}

DCI_ERROR DCI_Capture_Suspend(void)
{
    DCI_ERROR err;

    if (dci.state == DCI_STATE_RESET)
    {
        return DCI_ERROR_NOT_INIT;
    }
    if (dci.state != DCI_STATE_BUSY)
    {
        return DCI_ERROR_STATE;
    }

    err = dci_capture_off(DCI_WAIT_CAPTURE_ITER);
    dci.state = DCI_STATE_SUSPENDED;
    dci.stats.suspend_count++;
    return err;
}

DCI_ERROR DCI_Capture_Resume(void)
{
    if (dci.state == DCI_STATE_RESET)
    {
        return DCI_ERROR_NOT_INIT;
    }
    if (dci.state != DCI_STATE_SUSPENDED)
    {
        return DCI_ERROR_STATE;
    }

    dci.state = DCI_STATE_BUSY;
    if (dci.armed)
    {
        /* Channel still parked at the start of the next group with 0 words
         * transferred; setting CAPTURE needs no read-back poll. */
        DCI_CaptureCmd(ENABLE);
    }
    else
    {
        dci_arm(dci.cur_group);
    }
    return DCI_SUCCESS;
}

/*============================================================================*
 *                     State Query and Frame Retrieval (DESIGN.md 11.3)
 *============================================================================*/
DCI_CAPTURE_STATE_T DCI_Capture_GetState(void)
{
    return dci.state;
}

bool DCI_Capture_IsFrameReady(void)
{
    return (dci.ready_count != 0);
}

uint8_t DCI_Capture_GetReadyCount(void)
{
    return dci.ready_count;
}

DCI_ERROR DCI_Capture_GetFrame(DCI_frame_t *frame)
{
    uint32_t s;

    if (frame == NULL)
    {
        return DCI_ERROR_INVALID_PARAM;
    }
    if (dci.state == DCI_STATE_RESET)
    {
        return DCI_ERROR_NOT_INIT;
    }

    s = rtl_dci_lock();
    if (dci.ready_count == 0)
    {
        rtl_dci_unlock(s);
        return DCI_ERROR_NO_FRAME;
    }
    *frame = dci.frame[dci.ready_head];
    dci.ready_head = (uint8_t)((dci.ready_head + 1) % dci.dma.buf_count);
    dci.ready_count--;
    rtl_dci_unlock(s);

    return DCI_SUCCESS;
}

/*
 * Bounded spin, same convention as IDU: a loop-count safety net, not a precise
 * deadline. Not callable from an ISR. A version that really yields belongs in the
 * application, built from GetFrame plus its own semaphore.
 */
DCI_ERROR DCI_Capture_WaitFrame(DCI_frame_t *frame, uint32_t timeout_ms)
{
    uint32_t budget;

    if (frame == NULL)
    {
        return DCI_ERROR_INVALID_PARAM;
    }
    if (dci.state == DCI_STATE_RESET)
    {
        return DCI_ERROR_NOT_INIT;
    }
    if (dci.dma.notify_mode != DCI_NOTIFY_POLL)
    {
        return DCI_ERROR_STATE;
    }

    budget = (timeout_ms > (0xFFFFFFFFUL / DCI_WAIT_ITER_PER_MS))
             ? 0xFFFFFFFFUL : (timeout_ms * DCI_WAIT_ITER_PER_MS);
    for (;;)
    {
        if (DCI_Capture_GetFrame(frame) == DCI_SUCCESS)
        {
            return DCI_SUCCESS;
        }
        if (budget-- == 0)
        {
            return DCI_ERROR_TIMEOUT;
        }
    }
}

/*
 * Callable from the frame_ready callback, i.e. from ISR context - that is what
 * "hand the buffer straight to PPE/LCDC and release" needs. This is why
 * rtl_dci_lock() must disable interrupts and can never become a mutex.
 * Out-of-order release is fine: the gate tests one buffer at a time.
 */
void DCI_Capture_ReleaseBuffer(uint8_t index)
{
    bool     resume;
    uint32_t s;

    if (index >= dci.dma.buf_count)
    {
        return;
    }

    s = rtl_dci_lock();
    dci.buf_lock_mask &= (uint8_t)~BIT(index);
    resume = (dci.state == DCI_STATE_SUSPENDED) &&
             (dci.dma.on_busy == DCI_ON_BUSY_GATE) &&
             ((dci.buf_lock_mask & BIT(dci.cur_group)) == 0);
    if (resume)
    {
        dci.state = DCI_STATE_BUSY;
    }
    rtl_dci_unlock(s);

    if (!resume)
    {
        return;
    }
    if (dci.armed)
    {
        DCI_CaptureCmd(ENABLE);     /* hardware waits for the next VSYNC itself */
    }
    else
    {
        dci_arm(dci.cur_group);
    }
}

/*
 * Live destination progress. Meaningful for the LLP = 0 topologies, where the
 * channel stops in place; in the CONTINUOUS fixed-length ring CURR_DAR may
 * already have advanced into another group, so the last delivered length is
 * reported instead.
 */
uint32_t DCI_Capture_GetTransferredBytes(void)
{
    if (dci.state == DCI_STATE_RESET)
    {
        return 0;
    }
    if (dci.armed && !dci.ring_closed)
    {
        return rtl_dci_dma_curr_dar(dci.dma.dma_channel_num) -
               (uint32_t)dci.dma.buf[dci.cur_group];
    }
    return dci.last_bytes;
}

DCI_ERROR DCI_Capture_GetFrameInfo(DCI_frame_info_t *info)
{
    if (info == NULL)
    {
        return DCI_ERROR_INVALID_PARAM;
    }
    if (dci.state == DCI_STATE_RESET)
    {
        return DCI_ERROR_NOT_INIT;
    }

    info->frame_bytes     = dci.g.frame_bytes;
    info->frame_words     = dci.g.frame_words;
    info->line_bytes      = dci.g.line_bytes;
    info->lines_out       = dci.g.lines_out;
    info->lines_per_chunk = dci.g.lines_per_chunk;
    info->chunk_words     = dci.g.chunk_words;
    info->chunks_per_frame = dci.g.chunks_per_frame;
    info->desc_count      = dci.g.desc_count;
    info->line_aligned    = dci.g.line_aligned;
    info->var_len         = dci.g.var_len;
    return DCI_SUCCESS;
}

void DCI_GetStats(DCI_stats_t *stats)
{
    uint32_t s;

    if (stats == NULL)
    {
        return;
    }
    s = rtl_dci_lock();
    *stats = dci.stats;
    rtl_dci_unlock(s);
}

/*============================================================================*
 *                     Runtime Reconfiguration (DESIGN.md 12)
 *============================================================================*/
/* Fields that map to hardware or change the descriptor ring. */
static bool dci_dma_hw_differs(const DCI_DMA_config *a, const DCI_DMA_config *b)
{
    uint8_t i;

    if (a->dma_channel_num != b->dma_channel_num ||
        a->dma_priority    != b->dma_priority    ||
        a->buf_count       != b->buf_count       ||
        a->buf_bytes       != b->buf_bytes)
    {
        return true;
    }
    for (i = 0; i < DCI_MAX_BUF; i++)
    {
        if (a->buf[i] != b->buf[i])
        {
            return true;
        }
    }
    return false;
}

/*
 * The register table requires configuration to be written with ENABLE = 0, so
 * there is no hot path for geometry - the sequence is stop, change, restart. What
 * this API buys over "Stop then Init" is that a bad parameter set can never take
 * down a capture that is working: validation completes before anything is
 * touched (DESIGN.md 12.3).
 */
static DCI_ERROR dci_reconfig_core(const DCI_InitTypeDef *ni, const DCI_DMA_config *nd,
                                   const dci_crop_t *nc)
{
    DCI_InitTypeDef cand_init = dci.init;
    DCI_DMA_config  cand_dma  = dci.dma;
    dci_crop_t      cand_crop = dci.crop;
    dci_geom_t      g;
    DCI_ERROR       err;
    bool            was_running;
    uint32_t        s;

    if (dci.state == DCI_STATE_RESET)
    {
        return DCI_ERROR_NOT_INIT;
    }
    if (ni != NULL)
    {
        cand_init = *ni;
    }
    if (nd != NULL)
    {
        cand_dma = *nd;
    }
    if (nc != NULL)
    {
        cand_crop = *nc;
    }

    err = dci_validate(&cand_init, &cand_dma, &cand_crop, &g);
    if (err != DCI_SUCCESS)
    {
        return err;
    }

    /* on_busy, notify_mode and the callback pointers map to no register and do
     * not move the ring, so changing only those must not cost a frame. */
    if (memcmp(&cand_init, &dci.init, sizeof(cand_init)) == 0 &&
        memcmp(&cand_crop, &dci.crop, sizeof(cand_crop)) == 0 &&
        !dci_dma_hw_differs(&cand_dma, &dci.dma))
    {
        s = rtl_dci_lock();
        dci.dma = cand_dma;
        rtl_dci_unlock(s);
        dci_int_config();       /* IER is writable while capturing */
        return DCI_SUCCESS;
    }

    /* A buffer the consumer still holds would keep a stale size or format, so
     * refuse rather than silently invalidating it. Abort() is the way out. */
    if (dci.buf_lock_mask != 0)
    {
        return DCI_ERROR_BUSY;
    }

    was_running = (dci.state == DCI_STATE_BUSY || dci.state == DCI_STATE_SUSPENDED);

    err = dci_capture_off(DCI_WAIT_CAPTURE_ITER);
    if (err != DCI_SUCCESS)
    {
        return err;
    }
    dci_dma_stop();
    DCI_Cmd(DISABLE);

    /* Swapping channels re-runs the allocation so the new one gets its own depth and
     * vector entry. If the new channel cannot be had, keep running on the old one and
     * report it rather than ending up with none. */
    if (cand_dma.dma_channel_num != dci.dma.dma_channel_num)
    {
        uint8_t ch = cand_dma.dma_channel_num;

        if (!rtl_dci_dma_channel_alloc(&ch, dci_dma_isr))
        {
            return DCI_ERROR_DMA_CHANNEL;
        }
        dci_dma_release();
        cand_dma.dma_channel_num = ch;
        dci.dma_owned            = true;
    }

    dci.init = cand_init;
    dci.dma  = cand_dma;
    dci.crop = cand_crop;
    dci.g    = g;

    s = rtl_dci_lock();
    dci.ready_head  = 0;
    dci.ready_count = 0;
    dci.cur_group   = 0;
    dci.chunk_idx   = 0;
    dci.evt         = 0;
    rtl_dci_unlock(s);

    DCI_Init(&dci.init);
    dci_write_crop();
    if (dci.mode_fixed)
    {
        dci_write_mode_bit();
    }
    DCI_ClearINTPendingBit(DCI_INT_ALL);
    dci_int_config();

    if (!was_running)
    {
        dci.state = DCI_STATE_IDLE;
        return DCI_SUCCESS;
    }

    dci.ring_closed = (dci.mode == DCI_MODE_CONTINUOUS) && !dci.g.var_len;
    dci_build_ring();
    DCI_Cmd(ENABLE);
    dci.state = DCI_STATE_BUSY;
    dci_arm(0);
    return DCI_SUCCESS;
}

DCI_ERROR DCI_Capture_Reconfig(const DCI_InitTypeDef *init, const DCI_DMA_config *dma)
{
    if (init == NULL && dma == NULL)
    {
        return DCI_ERROR_INVALID_PARAM;
    }
    return dci_reconfig_core(init, dma, NULL);
}

/*
 * Swapping buffer addresses when nothing else moves is the common runtime change
 * (an upper layer rotating a memory pool), so it skips the DCI reconfiguration
 * entirely: ENABLE stays 1 and only the descriptor DARs move.
 *
 * Note the "no buffer held" requirement - this cannot be used to rotate snapshot
 * buffers, because it is exactly unavailable while the application holds one. Use
 * buf_count = 2 and the driver's own rotation for that (DESIGN.md 6.4).
 */
DCI_ERROR DCI_Capture_SetBuffers(uint32_t *const buf[], uint8_t count)
{
    bool      was_running;
    DCI_ERROR err;
    uint32_t  s;
    uint8_t   i;

    if (dci.state == DCI_STATE_RESET)
    {
        return DCI_ERROR_NOT_INIT;
    }
    if (buf == NULL || count != dci.dma.buf_count)
    {
        return DCI_ERROR_INVALID_PARAM;
    }
    for (i = 0; i < count; i++)
    {
        if (buf[i] == NULL)
        {
            return DCI_ERROR_INVALID_PARAM;
        }
        if (((uint32_t)buf[i] & 0x3U) != 0)
        {
            return DCI_ERROR_ALIGN;
        }
    }
    if (dci.buf_lock_mask != 0)
    {
        return DCI_ERROR_BUSY;
    }

    was_running = (dci.state == DCI_STATE_BUSY || dci.state == DCI_STATE_SUSPENDED);

    err = dci_capture_off(DCI_WAIT_CAPTURE_ITER);
    if (err != DCI_SUCCESS)
    {
        return err;
    }
    dci_dma_stop();

    for (i = 0; i < count; i++)
    {
        dci.dma.buf[i] = buf[i];
    }
    dci_build_ring();

    s = rtl_dci_lock();
    dci.ready_head  = 0;
    dci.ready_count = 0;
    dci.cur_group   = 0;
    dci.chunk_idx   = 0;
    dci.evt         = 0;
    rtl_dci_unlock(s);

    if (!was_running)
    {
        dci.state = DCI_STATE_IDLE;
        return DCI_SUCCESS;
    }
    dci.state = DCI_STATE_BUSY;
    dci_arm(0);
    return DCI_SUCCESS;
}

DCI_ERROR DCI_SetCaptureRate(DCI_FCRC_T rate)
{
    DCI_InitTypeDef ni;

    if (dci.state == DCI_STATE_RESET)
    {
        return DCI_ERROR_NOT_INIT;
    }
    ni = dci.init;
    ni.fcrc = rate;
    return dci_reconfig_core(&ni, NULL, NULL);
}

DCI_ERROR DCI_ConfigSyncCode(const DCI_SyncCode_t *code, const DCI_SyncUnmask_t *unmask)
{
    DCI_InitTypeDef ni;

    if (dci.state == DCI_STATE_RESET)
    {
        return DCI_ERROR_NOT_INIT;
    }
    if (code == NULL)
    {
        return DCI_ERROR_INVALID_PARAM;
    }
    ni = dci.init;
    ni.sync_code = *code;
    if (unmask != NULL)
    {
        ni.sync_unmask = *unmask;
    }
    return dci_reconfig_core(&ni, NULL, NULL);
}

/*
 * The sensor does not move, so unlike a geometry change on the sensor side there
 * is no settling period: the first frame after the restart is already valid.
 */
DCI_ERROR DCI_ConfigCrop(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h)
{
    dci_crop_t c;

    if (dci.state == DCI_STATE_RESET)
    {
        return DCI_ERROR_NOT_INIT;
    }
    if (w == 0 || h == 0)
    {
        return DCI_ERROR_INVALID_PARAM;
    }

    /* spec 2.6: the captured pclk count must be a multiple of 4, which is what
     * keeps a line a whole number of words (4 pclk = 1 word at 8-bit, 2 words at
     * 10-bit). Align down and say so rather than failing. */
    if ((w & 0x3U) != 0)
    {
        DCI_LOG("[DCI] crop width %u pclk aligned down to %u", w, w & ~0x3U);
        w = (uint16_t)(w & ~0x3U);
        if (w == 0)
        {
            return DCI_ERROR_INVALID_PARAM;
        }
    }

    c.x0 = x0;
    c.y0 = y0;
    c.w  = w;
    c.h  = h;
    return dci_reconfig_core(NULL, NULL, &c);
}

/* Turning crop on or off changes frame_bytes, so it is a full sequence and can
 * fail - not a single bit write. */
DCI_ERROR DCI_CropCmd(FunctionalState state)
{
    DCI_InitTypeDef ni;

    if (dci.state == DCI_STATE_RESET)
    {
        return DCI_ERROR_NOT_INIT;
    }
    ni = dci.init;
    ni.crop = (state == ENABLE);
    return dci_reconfig_core(&ni, NULL, NULL);
}

/*============================================================================*
 *                     Register Read/Write Self Test
 *============================================================================*/
/*
 * Bring-up aid: proves the register block is reachable and its bits behave, with
 * no camera and no DMA involved. The failure it exists to catch is a clock that is
 * still gated or a wrong base address, both of which otherwise show up much later
 * as "capture produces nothing" and cost a lot of time to trace back.
 *
 * DR is excluded throughout: reading it pops the FIFO (DESIGN.md 13.1).
 */

/* Writable-bit masks, derived from the register table. Bits outside these are
 * reserved or read-only and must read back 0.
 *
 * CR.capture (bit 0) is deliberately excluded: the register table marks it R/WAC,
 * i.e. the hardware clears it on its own (in snapshot mode it self-clears at the
 * end of a frame). A bit that may legitimately refuse to latch cannot be pattern
 * tested - it would report a false stuck-0 on the very first bring-up step, which
 * is worse than not covering it. Every capture test exercises it thoroughly. */
#define DCI_CR_RW_MASK      0x001F4FFEUL    /* [20:16] [14] [11:1], skip capture and
                                               rsvd 15 / 13:12 */
#define DCI_IER_RW_MASK     0x0000001FUL
#define DCI_ESCR_RW_MASK    0xFFFFFFFFUL
#define DCI_ESUR_RW_MASK    0xFFFFFFFFUL
#define DCI_CWSTRT_RW_MASK  0x1FFF3FFFUL    /* vst[28:16], hoffcnt[13:0] */
#define DCI_CWSIZE_RW_MASK  0x3FFF3FFFUL    /* vline[29:16], capcnt[13:0] */

typedef struct
{
    const char *name;
    uint16_t    offset;
    uint32_t    rw_mask;
} dci_reg_desc_t;

/* Order matches the register map so the report reads like the dump. */
static const dci_reg_desc_t dci_reg_tbl[] =
{
    {"CR",     0x00, DCI_CR_RW_MASK    },
    {"SR",     0x04, 0                 },  /* read-only */
    {"RIS",    0x08, 0                 },  /* read-only */
    {"IER",    0x0C, DCI_IER_RW_MASK   },
    {"MIS",    0x10, 0                 },  /* read-only */
    {"ICR",    0x14, 0                 },  /* write-1-only, checked separately */
    {"ESCR",   0x18, DCI_ESCR_RW_MASK  },
    {"ESUR",   0x1C, DCI_ESUR_RW_MASK  },
    {"CWSTRT", 0x20, DCI_CWSTRT_RW_MASK},
    {"CWSIZE", 0x24, DCI_CWSIZE_RW_MASK},
};

#define DCI_REG_TBL_CNT     (sizeof(dci_reg_tbl) / sizeof(dci_reg_tbl[0]))

static volatile uint32_t *dci_reg_at(uint32_t offset)
{
    return (volatile uint32_t *)(DCI_REG_BASE + offset);
}

/*
 * Walking-1 plus all-1 plus 0x5555.../0xAAAA... over the writable bits. Walking-1
 * localises a single stuck bit; the alternating patterns catch shorts between
 * adjacent bits, which a walking pattern alone can miss.
 *
 * `varied` is set when the register returned at least two different values, which
 * is what tells a live block from a gated one - see dci_regtest_alive().
 */
static void dci_regtest_one(const dci_reg_desc_t *d, DCI_regtest_item_t *it,
                            bool *varied)
{
    volatile uint32_t *reg = dci_reg_at(d->offset);
    const uint32_t     pat[] = {0xFFFFFFFFUL, 0x55555555UL, 0xAAAAAAAAUL};
    uint32_t           first_rb = 0;
    bool               have_first = false;
    uint32_t           i;

    it->name    = d->name;
    it->offset  = d->offset;
    it->rw_mask = d->rw_mask;
    it->stuck_0 = 0;
    it->stuck_1 = 0;
    it->skipped = (d->rw_mask == 0);

    if (it->skipped)
    {
        return;
    }

    for (i = 0; i < 32; i++)
    {
        uint32_t w = (1UL << i) & d->rw_mask;
        uint32_t rb;

        if (w == 0)
        {
            continue;
        }
        *reg = w;
        rb   = *reg;

        if (!have_first)
        {
            first_rb = rb;
            have_first = true;
        }
        else if (rb != first_rb)
        {
            *varied = true;
        }

        if ((rb & w) == 0)
        {
            it->stuck_0 |= w;
        }
        /* Anything outside the bit just written must be 0: a 1 here is either a
         * reserved bit that is not really reserved, or bits bleeding into each
         * other. Restricted to rw_mask so that self-setting status bits elsewhere
         * in the word cannot cause a false alarm. */
        it->stuck_1 |= (rb & d->rw_mask & ~w);
    }

    for (i = 0; i < sizeof(pat) / sizeof(pat[0]); i++)
    {
        uint32_t w = pat[i] & d->rw_mask;
        uint32_t rb;

        *reg = w;
        rb   = *reg;

        if (have_first && rb != first_rb)
        {
            *varied = true;
        }

        it->stuck_0 |= (w & ~rb);
        it->stuck_1 |= (rb & d->rw_mask & ~w);
    }

    *reg = 0;
}

/*
 * Liveness: a gated clock, a wrong base or a dead bus returns the *same* value no
 * matter what is written - typically all-0 or all-1. So the question is whether the
 * read-back varies, not whether it equals some particular value.
 *
 * Checking for a specific constant would be wrong: ESCR and ESUR are fully writable
 * 32-bit registers, so an all-ones read-back is a legitimate result there.
 *
 * Uses CR, whose writable mask has both set and clear bits, so 0 and rw_mask must
 * read back differently on any working block.
 */
static bool dci_regtest_alive(uint32_t *lo, uint32_t *hi)
{
    *dci_reg_at(0x00) = 0;
    *lo = *dci_reg_at(0x00) & DCI_CR_RW_MASK;

    *dci_reg_at(0x00) = DCI_CR_RW_MASK;
    *hi = *dci_reg_at(0x00) & DCI_CR_RW_MASK;

    *dci_reg_at(0x00) = 0;
    return (*lo != *hi);
}

/*
 * Every register gets a distinct value, then all are read back. A single shared
 * read-back value means the offsets alias onto one another - the signature of a
 * decode problem or of pointing at the wrong block entirely.
 */
static bool dci_regtest_distinct(void)
{
    uint32_t i;
    bool     ok = true;

    for (i = 0; i < DCI_REG_TBL_CNT; i++)
    {
        if (dci_reg_tbl[i].rw_mask != 0)
        {
            *dci_reg_at(dci_reg_tbl[i].offset) = (0x11111111UL * (i + 1)) &
                                                 dci_reg_tbl[i].rw_mask;
        }
    }
    for (i = 0; i < DCI_REG_TBL_CNT; i++)
    {
        uint32_t want;

        if (dci_reg_tbl[i].rw_mask == 0)
        {
            continue;
        }
        want = (0x11111111UL * (i + 1)) & dci_reg_tbl[i].rw_mask;
        if ((*dci_reg_at(dci_reg_tbl[i].offset) & dci_reg_tbl[i].rw_mask) != want)
        {
            ok = false;
        }
    }
    for (i = 0; i < DCI_REG_TBL_CNT; i++)
    {
        if (dci_reg_tbl[i].rw_mask != 0)
        {
            *dci_reg_at(dci_reg_tbl[i].offset) = 0;
        }
    }
    return ok;
}

/*
 * ICR is write-1-only and RIS is read-only, so with no pixel activity RIS should
 * stay 0 no matter what is written to ICR. MIS must equal RIS & IER - here 0 for
 * any IER, which at least proves MIS is not stuck.
 */
static void dci_regtest_int_paths(DCI_regtest_t *r)
{
    uint32_t ier;

    DCI->IER = 0;
    DCI->ICR = DCI_INT_ALL;
    r->w1c_ok = ((DCI->RIS & DCI_INT_ALL) == 0);

    /* MIS = RIS & IER. Use the IER read-back rather than the value just written, so
     * the comparison states the hardware rule instead of assuming the write stuck -
     * IER's own writability is covered by the pattern test. */
    DCI->IER  = DCI_INT_ALL;
    ier       = DCI->IER & DCI_INT_ALL;
    r->mis_ok = ((DCI->MIS & DCI_INT_ALL) == (DCI->RIS & ier));

    DCI->IER  = 0;
    DCI->ICR  = DCI_INT_ALL;
}

/*
 * Settles TODO(verify) #11: the register table says CAPCNT's low 2 bits are
 * force-cleared, while the dump example shows plain N-1 encoding. Write 1s and
 * look.
 */
static void dci_regtest_capcnt(DCI_regtest_t *r)
{
    DCI_CWSIZE_TypeDef cw;

    DCI->CWSIZE = DCI_CWSIZE_RW_MASK;
    cw.d32      = DCI->CWSIZE;
    r->capcnt_low_forced = ((cw.b.capcnt & 0x3U) == 0);
    DCI->CWSIZE = 0;
}

DCI_ERROR DCI_RegTest(DCI_regtest_t *report)
{
    /* Scratch report for callers that only want the verdict. Initialised at the
     * declaration even though the memset below covers it: taking the address of an
     * uninitialised object is legal but reads as a defect to static analysis, and
     * this runs once at bring-up so the redundant zeroing costs nothing. */
    DCI_regtest_t  local = {0};
    DCI_regtest_t *r     = (report != NULL) ? report : &local;
    uint32_t       i;

    memset(r, 0, sizeof(*r));
    r->base = DCI_REG_BASE;

    /* Leave any prior session behind: the test rewrites every register. */
    if (dci.state != DCI_STATE_RESET)
    {
        (void)DCI_Capture_Abort();
        dci_dma_release();
        memset(&dci, 0, sizeof(dci));
    }

    /* The whole point of this test is to catch a gated clock, so enable it first -
     * otherwise the test reports NO_RESPONSE for a reason it created itself. */
    rtl_dci_clock_enable();

    DCI->CR = 0;        /* ENABLE = 0: configuration registers are writable */

    /* Liveness first: every other result is meaningless on a dead block, and
     * reporting a hundred stuck bits would bury the actual cause. */
    r->responded = dci_regtest_alive(&r->probe_lo, &r->probe_hi);
    if (!r->responded)
    {
        DCI->CR = 0;
        return DCI_ERROR_NO_RESPONSE;
    }

    for (i = 0; i < DCI_REG_TBL_CNT; i++)
    {
        dci_regtest_one(&dci_reg_tbl[i], &r->item[i], &r->varied);
        if (!r->item[i].skipped)
        {
            r->checked++;
            if (r->item[i].stuck_0 != 0 || r->item[i].stuck_1 != 0)
            {
                r->failed++;
            }
        }
    }

    r->distinct_ok = dci_regtest_distinct();
    dci_regtest_int_paths(r);
    dci_regtest_capcnt(r);

    /* Back to reset values. */
    DCI->CR     = 0;
    DCI->IER    = 0;
    DCI->ESCR   = 0;
    DCI->ESUR   = 0;
    DCI->CWSTRT = 0;
    DCI->CWSIZE = 0;
    DCI->ICR    = DCI_INT_ALL;

    if (r->failed != 0 || !r->distinct_ok || !r->w1c_ok || !r->mis_ok)
    {
        return DCI_ERROR_REG_TEST;
    }
    return DCI_SUCCESS;
}

DCI_ERROR DCI_RegTestDump(void)
{
    static DCI_regtest_t r;
    DCI_ERROR            err = DCI_RegTest(&r);
#if (DCI_DEBUG_REG_DUMP == 1)
    uint32_t i;

    DCI_LOG("--- DCI register test @0x%08X ---", r.base);

    if (!r.responded)
    {
        DCI_LOG("  NO RESPONSE: CR read 0x%08X after writing 0, 0x%08X after writing"
                " 0x%08X", r.probe_lo, r.probe_hi, (uint32_t)DCI_CR_RW_MASK);
        DCI_LOG("  check: DCI clock gated? base address wrong? bus/power down?");
        return err;
    }

    for (i = 0; i < DCI_REG_TBL_CNT; i++)
    {
        const DCI_regtest_item_t *it = &r.item[i];

        if (it->skipped)
        {
            /* ICR is write-1-only rather than read-only; either way there is no
             * read-back to pattern-test, so it is covered by the w1c check. */
            DCI_LOG("  %-6s +0x%02X  %s, no read-back to test", it->name, it->offset,
                    (it->offset == 0x14) ? "write-1-only" : "read-only");
        }
        else if (it->stuck_0 == 0 && it->stuck_1 == 0)
        {
            DCI_LOG("  %-6s +0x%02X  rw_mask 0x%08X  OK", it->name, it->offset,
                    it->rw_mask);
        }
        else
        {
            DCI_LOG("  %-6s +0x%02X  rw_mask 0x%08X  FAIL stuck0 0x%08X stuck1 0x%08X",
                    it->name, it->offset, it->rw_mask, it->stuck_0, it->stuck_1);
        }
    }

    DCI_LOG("  distinct=%d varied=%d w1c=%d mis=%d   checked=%d failed=%d",
            r.distinct_ok, r.varied, r.w1c_ok, r.mis_ok, r.checked, r.failed);
    DCI_LOG("  CAPCNT[1:0] %s -> encoding is %s",
            r.capcnt_low_forced ? "force-cleared by HW" : "writable",
            r.capcnt_low_forced ? "N with low 2 bits dropped" : "plain N-1");
    DCI_LOG("  verdict: %s", (err == DCI_SUCCESS) ? "PASS" : "FAIL");
#endif
    return err;
}

/*============================================================================*
 *                     Register Dump (DESIGN.md 13)
 *============================================================================*/
#if (DCI_DEBUG_REG_DUMP == 1)

static const char *const dci_state_name[] = {"RESET", "IDLE", "BUSY", "SUSPENDED", "ERROR"};
static const char *const dci_fcrc_name[]  = {"all", "1/2", "1/4", "rsvd"};
static const char *const dci_msize_name[] = {"1", "4", "8", "16", "32", "64", "128", "256"};

/*
 * DR is never read: it is the FIFO outlet and every read pops a word, so a dump
 * would corrupt the capture in progress. Collecting and printing are split
 * because the interesting moment is inside the ISR, where printing at pixel rates
 * is not an option.
 */
void DCI_RegSnapshot(DCI_snapshot_t *snap)
{
    uint8_t i;

    if (snap == NULL)
    {
        return;
    }
    memset(snap, 0, sizeof(*snap));

    snap->cr     = DCI->CR;
    snap->sr     = DCI->SR;
    snap->ris    = DCI->RIS;
    snap->ier    = DCI->IER;
    snap->mis    = DCI->MIS;
    snap->icr    = DCI->ICR;
    snap->escr   = DCI->ESCR;
    snap->esur   = DCI->ESUR;
    snap->cwstrt = DCI->CWSTRT;
    snap->cwsize = DCI->CWSIZE;

    if (dci.dma_owned)
    {
        rtl_dci_dma_read_regs(dci.dma.dma_channel_num, &snap->dma);
    }

    snap->desc_count = dci.g.desc_count;
    for (i = 0; i < snap->desc_count && i < DCI_MAX_DESC; i++)
    {
        snap->desc[i].sar      = dci_lli_ring[i].SAR;
        snap->desc[i].dar      = dci_lli_ring[i].DAR;
        snap->desc[i].llp      = dci_lli_ring[i].LLP;
        snap->desc[i].ctl_low  = dci_lli_ring[i].CTL_LOW;
        snap->desc[i].ctl_high = dci_lli_ring[i].CTL_HIGH;
    }

    snap->state            = (uint8_t)dci.state;
    snap->mode             = (uint8_t)dci.mode;
    snap->var_len          = dci.g.var_len ? 1 : 0;
    snap->notify_mode      = (uint8_t)dci.dma.notify_mode;
    snap->buf_count        = dci.dma.buf_count;
    snap->chunks_per_frame = dci.g.chunks_per_frame;
    snap->lines_per_chunk  = dci.g.lines_per_chunk;
    snap->line_aligned     = dci.g.line_aligned ? 1 : 0;
    snap->chunk_idx        = dci.chunk_idx;
    snap->evt              = dci.evt;
    snap->buf_lock_mask    = dci.buf_lock_mask;
    snap->ready_count      = dci.ready_count;
    snap->cur_group        = dci.cur_group;
    snap->frame_bytes      = dci.g.frame_bytes;
    snap->stats            = dci.stats;
}

/* Raw hex is not enough on its own; the N-1 encoded fields print both forms. */
void DCI_DumpSnapshot(const DCI_snapshot_t *snap)
{
    DCI_CR_TypeDef     cr;
    DCI_SR_TypeDef     sr;
    DCI_CWSTRT_TypeDef strt;
    DCI_CWSIZE_TypeDef size;
    uint32_t           sum = 0;
    uint8_t            i;

    if (snap == NULL)
    {
        return;
    }
    cr.d32   = snap->cr;
    sr.d32   = snap->sr;
    strt.d32 = snap->cwstrt;
    size.d32 = snap->cwsize;

    DCI_LOG("--- DCI @0x%08X state=%s mode=%s%s buf=%d chunks=%d ---",
            (unsigned)DCI_REG_BASE,
            dci_state_name[(snap->state < 5) ? snap->state : 0],
            cr.b.cm ? "SNAPSHOT" : "CONT", snap->var_len ? "/JPEG" : "",
            snap->buf_count, snap->chunks_per_frame);
    DCI_LOG("  chunk=%d lines/chunk=%d %s frame_bytes=%d notify=%s",
            snap->chunk_idx, snap->lines_per_chunk,
            snap->line_aligned ? "line-aligned" : "word-aligned(degenerate)",
            snap->frame_bytes, snap->notify_mode ? "POLL" : "CALLBACK");

    DCI_LOG("CR     0x%08X  EN=%d CAP=%d CM=%d CROP=%d JPEG=%d ESS=%d",
            snap->cr, cr.b.enable, cr.b.capture, cr.b.cm, cr.b.crop, cr.b.jpeg, cr.b.ess);
    DCI_LOG("                  EDM=%d(%s) FCRC=%d(%s) PCK=%d HS=%d VS=%d",
            cr.b.edm, cr.b.edm ? "10b" : "8b", cr.b.fcrc, dci_fcrc_name[cr.b.fcrc],
            cr.b.pckpol, cr.b.hspol, cr.b.vspol);
    DCI_LOG("                  BSM=%d OEBS=%d LSM=%d OELS=%d",
            cr.b.bsm, cr.b.oebs, cr.b.lsm, cr.b.oels);
    DCI_LOG("SR     0x%08X  FNE=%d VSYNC=%d HSYNC=%d",
            snap->sr, sr.b.fne, sr.b.vsync, sr.b.hsync);
    DCI_LOG("RIS    0x%08X  IER 0x%08X  MIS 0x%08X   (bits: FRAME OVR ERR VSYNC LINE)",
            snap->ris, snap->ier, snap->mis);
    DCI_LOG("CWSTRT 0x%08X  VST=%d HOFF=%d", snap->cwstrt, strt.b.vst, strt.b.hoffcnt);
    DCI_LOG("CWSIZE 0x%08X  VLINE=0x%X(%d lines) CAPCNT=0x%X(%d pclk)",
            snap->cwsize, size.b.vline, size.b.vline + 1,
            size.b.capcnt, size.b.capcnt + 1);
    DCI_LOG("ESCR   0x%08X  ESUR 0x%08X", snap->escr, snap->esur);
    DCI_LOG("DR     (skipped - reading pops FIFO)");

    DCI_LOG("--- DMA ch%d ---", dci.dma.dma_channel_num);
    DCI_LOG("SAR    0x%08X  CURR_SAR 0x%08X  %s", snap->dma.sar, snap->dma.curr_sar,
            (snap->dma.curr_sar == (uint32_t)&DCI->DR) ? "ok" : "!! not &DCI->DR");
    DCI_LOG("DAR    0x%08X  CURR_DAR 0x%08X  LLP 0x%08X",
            snap->dma.dar, snap->dma.curr_dar, snap->dma.llp);
    DCI_LOG("CTL_LO 0x%08X  DMSIZE=%s SMSIZE=%s TT_FC=%d LLP_DST=%d LLP_SRC=%d",
            snap->dma.ctl_low,
            dci_msize_name[snap->dma.dest_msize & 0x7U],
            dci_msize_name[snap->dma.src_msize & 0x7U],
            snap->dma.tt_fc, snap->dma.llp_dst_en, snap->dma.llp_src_en);
    DCI_LOG("CTL_HI 0x%08X  TRANS_DATA_CNT=%d", snap->dma.ctl_high, snap->dma.ctl_high);
    DCI_LOG("CFG_LO 0x%08X  HS_SEL_DST=%d HS_SEL_SRC=%d FIFO_EMPTY=%d",
            snap->dma.cfg_low, snap->dma.hs_sel_dst, snap->dma.hs_sel_src,
            snap->dma.fifo_empty);
    DCI_LOG("CFG_HI 0x%08X  SRC_PER=0x%X%s DEST_PER=0x%X", snap->dma.cfg_high,
            snap->dma.src_per,
            (snap->dma.src_per == 0xFU) ? "(!! HAL placeholder)" : "",
            snap->dma.dest_per);
    DCI_LOG("CH_EN  0x%08X  Tfr 0x%08X  Block 0x%08X  Err 0x%08X",
            snap->dma.ch_en, snap->dma.status_tfr, snap->dma.status_block,
            snap->dma.status_err);

    DCI_LOG("--- LLI ring (%d desc, %d grp x %d, %s) ---", snap->desc_count,
            snap->buf_count, snap->chunks_per_frame,
            dci.ring_closed ? "CONTINUOUS closed" : "LLP=0 chains");
    for (i = 0; i < snap->desc_count && i < DCI_MAX_DESC; i++)
    {
        DCI_LOG("grp%d [%2d] DAR 0x%08X LLP 0x%08X CTL_HI %d", i / snap->chunks_per_frame,
                i, snap->desc[i].dar, snap->desc[i].llp, snap->desc[i].ctl_high);
        sum += snap->desc[i].ctl_high;
    }
    DCI_LOG("          sum(CTL_HI) = %d word%s", sum,
            snap->var_len ? "" : " (expect frame_words x buf_count)");

    DCI_LOG("--- stats --- done %d dropped %d chunks %d ovr %d err %d",
            snap->stats.frames_done, snap->stats.frames_dropped, snap->stats.chunks_done,
            snap->stats.ovr_count, snap->stats.err_count);
    DCI_LOG("  resync %d suspend %d buf_overflow %d seq %d",
            snap->stats.resync_count, snap->stats.suspend_count,
            snap->stats.buf_overflow, snap->stats.seq);
    DCI_LOG("  cur_group %d evt 0x%02X lock_mask 0x%02X ready %d",
            snap->cur_group, snap->evt, snap->buf_lock_mask, snap->ready_count);
}

void DCI_RegDump(void)
{
    static DCI_snapshot_t snap;

    DCI_RegSnapshot(&snap);
    DCI_DumpSnapshot(&snap);
}

/*
 * Only what moved since the previous call. Useful when watching a live capture:
 * a full dump per frame drowns the interesting transition.
 */
void DCI_RegMonitor(void)
{
    static DCI_snapshot_t prev;
    static bool           have_prev = false;
    DCI_snapshot_t        now;

    DCI_RegSnapshot(&now);

    if (!have_prev)
    {
        have_prev = true;
        prev = now;
        DCI_DumpSnapshot(&now);
        return;
    }

#define DCI_MON(field, fmt)                                             \
    do {                                                                \
        if (now.field != prev.field) {                                   \
            DCI_LOG("[mon] " #field " " fmt " -> " fmt,                  \
                    prev.field, now.field);                             \
        }                                                               \
    } while (0)

    DCI_MON(cr, "0x%08X");
    DCI_MON(sr, "0x%08X");
    DCI_MON(ris, "0x%08X");
    DCI_MON(ier, "0x%08X");
    DCI_MON(cwstrt, "0x%08X");
    DCI_MON(cwsize, "0x%08X");
    DCI_MON(dma.dar, "0x%08X");
    DCI_MON(dma.curr_dar, "0x%08X");
    DCI_MON(dma.llp, "0x%08X");
    DCI_MON(dma.ctl_low, "0x%08X");
    DCI_MON(dma.ctl_high, "%d");
    DCI_MON(dma.cfg_low, "0x%08X");
    DCI_MON(dma.cfg_high, "0x%08X");
    DCI_MON(dma.ch_en, "0x%08X");
    DCI_MON(dma.status_err, "0x%08X");
    DCI_MON(state, "%d");
    DCI_MON(chunk_idx, "%d");
    DCI_MON(evt, "0x%02X");
    DCI_MON(buf_lock_mask, "0x%02X");
    DCI_MON(ready_count, "%d");
    DCI_MON(cur_group, "%d");
    DCI_MON(stats.frames_done, "%d");
    DCI_MON(stats.frames_dropped, "%d");
    DCI_MON(stats.ovr_count, "%d");
    DCI_MON(stats.err_count, "%d");
    DCI_MON(stats.resync_count, "%d");
    DCI_MON(stats.suspend_count, "%d");
    DCI_MON(stats.buf_overflow, "%d");

#undef DCI_MON

    prev = now;
}

#endif /* DCI_DEBUG_REG_DUMP */
