/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dci_demo_preset.h"
#include "dci_demo.h"

/*============================================================================*
 *                         Format properties
 *============================================================================*/
/*
 * pclk_per_pixel is the part that matters to the hardware: DCI counts pixel clocks,
 * not pixels, so a 320-pixel RGB565 line is 640 pclk on an 8-bit bus. Getting this
 * wrong is the single most likely cause of a desync, which is why it lives in one
 * table rather than being open-coded per preset.
 *
 * RAW10 is 1 pclk per sample with EDM = 10BIT, where each sample occupies 2 bytes in
 * memory - the only format where bytes_per_pixel and pclk_per_pixel differ in that
 * direction.
 */
typedef struct
{
    const char *name;
    uint8_t     bytes_per_pixel;
    uint8_t     pclk_per_pixel;
} dci_fmt_prop_t;

static const dci_fmt_prop_t dci_fmt_prop[] =
{
    [DCI_FMT_RGB565] = {"RGB565", 2, 2},
    [DCI_FMT_YUV422] = {"YUV422", 2, 2},
    [DCI_FMT_RAW8]   = {"RAW8",   1, 1},
    [DCI_FMT_RAW10]  = {"RAW10",  2, 1},
    [DCI_FMT_JPEG]   = {"JPEG",   0, 1},
};

uint8_t dci_demo_fmt_bytes_per_pixel(dci_demo_fmt_t fmt)
{
    return (fmt <= DCI_FMT_JPEG) ? dci_fmt_prop[fmt].bytes_per_pixel : 0;
}

uint8_t dci_demo_fmt_pclk_per_pixel(dci_demo_fmt_t fmt)
{
    return (fmt <= DCI_FMT_JPEG) ? dci_fmt_prop[fmt].pclk_per_pixel : 1;
}

const char *dci_demo_fmt_name(dci_demo_fmt_t fmt)
{
    return (fmt <= DCI_FMT_JPEG) ? dci_fmt_prop[fmt].name : "?";
}

/*============================================================================*
 *                         Preset table
 *============================================================================*/
/*
 * Ordered so that the earliest rows are the safest to bring up: smallest frame,
 * simplest signalling, no crop, no decimation. Work down the list - a failure at
 * row N with rows 0..N-1 passing localises the cause to whatever row N adds.
 *
 * Sized against a 1 MB RAM budget (960 KB pool). The last two RGB565 rows exceed the
 * pool at 2 buffers and are meant to be run single-buffered; the table stops short of
 * anything that will not fit even at 1 buffer, since an unrunnable row is just a
 * guaranteed DEMO_FAIL_POOL.
 *
 * RGB565 and JPEG are the formats under test. The four RAW rows are the ones carrying
 * a property no RGB565 row can reach - EDM=10BIT, meaningful BSM, the word-aligned
 * split, and range-limited data for ESS - plus one plain RAW8 control. dci_demo_id.h
 * records which is which; do not convert one to RGB565 without reading it.
 *
 * Index order must match the DCI_DEMO_PRESET_* macros in dci_demo_id.h - a static
 * assertion at the end of this file pins the count.
 */
static const dci_demo_preset_t dci_presets[] =
{
    {
        .name = "QQVGA RGB565 160x120",
        .note = "smallest sane frame: 1 block, 37 KB, fits anywhere. Start here.",
        .width = 160, .height = 120, .fmt = DCI_FMT_RGB565,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
    },
    {
        .name = "QVGA RGB565 320x240",
        .note = "OV2640 default in the reference code (0x5a=0x50, 0x5b=0x3c).",
        .width = 320, .height = 240, .fmt = DCI_FMT_RGB565,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
    },
    {
        .name = "QVGA RGB565, rising PCLK",
        .note = "same as above with PCKPOL flipped: proves polarity control works.",
        .width = 320, .height = 240, .fmt = DCI_FMT_RGB565,
        .pckpol = true, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
    },
    {
        .name = "QVGA RGB565, HS/VS active high",
        .note = "inverted sync polarity; blanking data must still be dropped.",
        .width = 320, .height = 240, .fmt = DCI_FMT_RGB565,
        .pckpol = false, .hspol = true, .vspol = true,
        .sync = DCI_SYNC_HARDWARE,
    },
    {
        .name = "QVGA RAW8 320x240",
        .note = "1 byte/pixel: halves the frame and changes the pclk ratio.",
        .width = 320, .height = 240, .fmt = DCI_FMT_RAW8,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
    },
    {
        .name = "QVGA YUV422 320x240",
        .note = "same byte count as RGB565; checks CbYCrY order is left untouched.",
        .width = 320, .height = 240, .fmt = DCI_FMT_YUV422,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
    },
    {
        .name = "QVGA RGB565 + centre crop 160x120",
        .note = "crop halves both axes; first byte must be source pixel (80,60).",
        .width = 320, .height = 240, .fmt = DCI_FMT_RGB565,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
        .crop_en = true, .crop_x = 80, .crop_y = 60, .crop_w = 160, .crop_h = 120,
    },
    {
        .name = "QVGA RGB565 + crop at (0,0) 64x48",
        .note = "top-left boundary: first captured byte is source (0,0).",
        .width = 320, .height = 240, .fmt = DCI_FMT_RGB565,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
        .crop_en = true, .crop_x = 0, .crop_y = 0, .crop_w = 64, .crop_h = 48,
    },
    {
        .name = "QVGA RGB565 + LSM (every 2nd line)",
        .note = "line decimation: output height halves to 120. LSM drops whole lines, "
        "so it is format-agnostic - tested on RGB565 because that is the "
        "format the product uses.",
        .width = 320, .height = 240, .fmt = DCI_FMT_RGB565,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
        .lsm = true, .oels = false,
    },
    {
        /*
         * Must stay on RAW8. BSM drops bytes, so it only means "drop pixels" where a
         * pixel is one byte. The driver cannot enforce that - DCI never sees a pixel
         * format, and RGB565 is EDM_8BIT too, so bsm + RGB565 is accepted, derives a
         * consistent geometry and captures without an error, yet throws away one byte
         * of every pixel. The wrongness shows up only as a broken image, which makes
         * this the one place where the preset table is the only guard there is.
         */
        .name = "QVGA RAW8 + BSM (every 2nd byte)",
        .note = "byte decimation: line halves. Requires 8-bit EDM, and only makes "
        "sense at 1 byte/pixel - on RGB565 it would silently corrupt pixels.",
        .width = 320, .height = 240, .fmt = DCI_FMT_RAW8,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
        .bsm = DCI_BSM_EVERY_2ND, .oebs = false,
    },
    {
        .name = "QVGA RGB565 @ half frame rate",
        .note = "FCRC drops every other frame; seq must NOT gap (driver cannot see it).",
        .width = 320, .height = 240, .fmt = DCI_FMT_RGB565,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
        .fcrc = DCI_FCRC_HALF,
    },
    /*
     * --- The 10-bit group -------------------------------------------------------
     *
     * EDM = 10BIT is the one hardware-enforced RAW-only mode, and it changes the
     * layout rather than just the size: 2 pclk per word instead of 4, each sample
     * occupying the low 10 bits of a 16-bit half-word with bits 15:10 cleared
     * (DESIGN.md 2). That padding rule is checkable per sample without knowing what
     * the camera was pointed at, which makes it the strongest content assertion any
     * format in this table offers - hence a dedicated case (DCI_DEMO_CASE_RAW10).
     *
     * Four rows, each isolating one interaction:
     *   base   - the packing rule itself, nothing else
     *   crop   - CAPCNT in 10-bit units: a pclk is 2 bytes, so the pclk<->byte
     *            conversion differs from every 8-bit row
     *   VGA    - multi-block reassembly at 2 bytes/pclk (3 blocks), where a block
     *            boundary must still land on a half-word or the samples shear
     *   LSM    - decimation combined with 10-bit, i.e. the one combination where
     *            both the line count and the bytes-per-pclk are non-default
     *
     * Deliberately absent: BSM (rejected, spec requires EDM=8BIT) and ESS (rejected,
     * embedded codes are 8-bit only). Both refusals are asserted by the case rather
     * than given a row, since a row that cannot be configured is not a test.
     */
    {
        .name = "QVGA RAW10 320x240",
        .note = "EDM = 10BIT baseline: 2 bytes/sample, 2 pclk/word, bits 15:10 of "
        "every sample must read 0. Start the 10-bit ladder here.",
        .width = 320, .height = 240, .fmt = DCI_FMT_RAW10,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
    },
    {
        /*
         * Crop is where 10-bit is most likely to go wrong: CAPCNT counts pixel
         * clocks, and dci_demo_setup() converts the window from pixels to pclk with
         * dci_demo_fmt_pclk_per_pixel(). RAW10 is the only format where that ratio
         * is 1 while bytes_per_pixel is 2, so a formula that conflates "pclk" with
         * "byte" passes on every other row and fails here.
         */
        .name = "QVGA RAW10 + centre crop 160x120",
        .note = "crop under EDM=10BIT: CAPCNT is in pclk (1/sample) while a sample "
        "is 2 bytes - the one row where pclk != bytes.",
        .width = 320, .height = 240, .fmt = DCI_FMT_RAW10,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
        .crop_en = true, .crop_x = 80, .crop_y = 60, .crop_w = 160, .crop_h = 120,
    },
    {
        /*
         * 1280 B/line x 480 lines = 600 KB in 3 blocks of 160 lines. Deliberately the
         * same byte count and the same split as VGA RGB565, reached with half the pclk
         * per line - which is the point: it shows the block split follows BYTES, not
         * pixel clocks. If the split were driven by pclk this row would come out with
         * a different block count than its RGB565 twin.
         */
        .name = "VGA RAW10 640x480",
        .note = "3 blocks at 2 bytes/pclk - same split as VGA RGB565 despite half the "
        "pclk/line, proving the split follows bytes. 600 KB: 1 BUFFER ONLY.",
        .width = 640, .height = 480, .fmt = DCI_FMT_RAW10,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
    },
    {
        .name = "QVGA RAW10 + LSM (every 2nd line)",
        .note = "decimation x 10-bit: lines_out halves to 120 while bytes_per_pclk "
        "stays 2. Both non-default at once.",
        .width = 320, .height = 240, .fmt = DCI_FMT_RAW10,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
        .lsm = true, .oels = false,
    },
    {
        /*
         * The only row that reaches the word-aligned fallback, and it takes a
         * single-byte format to get there: a line is a whole number of words unless
         * its byte count is not a multiple of 4, and at 2 bytes/pixel that needs an
         * odd pixel width, which sensors do not output. So this row cannot be
         * re-expressed in RGB565 - dropping it would drop the branch.
         */
        .name = "466x466 RAW8 (degenerate split)",
        .note = "466 bytes/line is not a whole word -> word-aligned split, "
        "line_aligned = 0. The only preset that reaches the fallback path.",
        .width = 466, .height = 466, .fmt = DCI_FMT_RAW8,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
    },
    {
        .name = "466x466 RGB565",
        .note = "the product panel size; 2 blocks of 233 lines. 848 KB at 2 buffers, "
        "so it only just fits - use 1 buffer if the pool is trimmed.",
        .width = 466, .height = 466, .fmt = DCI_FMT_RGB565,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
    },
    {
        .name = "VGA RGB565 640x480",
        .note = "3 blocks: multi-block reassembly. SINGLE BUFFER ONLY at 1 MB "
        "(600 KB x 2 does not fit).",
        .width = 640, .height = 480, .fmt = DCI_FMT_RGB565,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
    },
    {
        .name = "SVGA RGB565 800x600",
        .note = "4 blocks, 938 KB: the largest frame a 1 MB budget can hold at all. "
        "SINGLE BUFFER ONLY, and pair it with OVERWRITE.",
        .width = 800, .height = 600, .fmt = DCI_FMT_RGB565,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
    },
    {
        .name = "JPEG 320x240 (variable length)",
        .note = "buffer is a capacity cap, not a frame size. Length from CURR_DAR.",
        .width = 320, .height = 240, .fmt = DCI_FMT_JPEG,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_HARDWARE,
    },
    {
        .name = "QVGA RAW8, embedded sync",
        .note = "ESS mode, 8-bit only. Needs the sync codes programmed to match "
        "the sensor - see DCI_ConfigSyncCode().",
        .width = 320, .height = 240, .fmt = DCI_FMT_RAW8,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync = DCI_SYNC_EMBEDDED,
    },
};

#define DCI_PRESET_COUNT    (sizeof(dci_presets) / sizeof(dci_presets[0]))

/* The DCI_DEMO_PRESET_* macros index this table by position, so a row inserted
 * without updating dci_demo_id.h would silently shift every later name. */
_Static_assert(DCI_PRESET_COUNT == DCI_DEMO_PRESET_MAX,
               "preset table and DCI_DEMO_PRESET_* in dci_demo_id.h are out of step");

uint8_t dci_demo_preset_count(void)
{
    return (uint8_t)DCI_PRESET_COUNT;
}

const dci_demo_preset_t *dci_demo_preset_get(uint8_t index)
{
    return (index < DCI_PRESET_COUNT) ? &dci_presets[index] : NULL;
}

/*============================================================================*
 *                         Derivation
 *============================================================================*/
/*
 * apply_crop distinguishes the two sizes a cropping preset has: the final delivered
 * frame, and the larger one Capture_Init() validates against before the window is
 * programmed. Both come from the same formula, which is the point - two formulas
 * would drift.
 */
static uint32_t dci_preset_bytes(const dci_demo_preset_t *p, bool apply_crop)
{
    uint32_t w, h, line_bytes;

    if (p == NULL || p->fmt == DCI_FMT_JPEG)
    {
        return 0;
    }

    w = (p->crop_en && apply_crop) ? p->crop_w : p->width;
    h = (p->crop_en && apply_crop) ? p->crop_h : p->height;

    line_bytes = w * dci_demo_fmt_bytes_per_pixel(p->fmt);

    /* Byte decimation shrinks the line; the driver rejects a ratio that does not
     * divide it, so mirror that here rather than rounding. */
    switch (p->bsm)
    {
    case DCI_BSM_EVERY_2ND: line_bytes /= 2; break;
    case DCI_BSM_1_OF_4:    line_bytes /= 4; break;
    case DCI_BSM_2_OF_4:    line_bytes /= 2; break;
    default: break;
    }

    if (p->lsm)
    {
        h = p->oels ? (h / 2) : ((h + 1) / 2);
    }

    return line_bytes * h;
}

uint32_t dci_demo_preset_frame_bytes(const dci_demo_preset_t *p)
{
    return dci_preset_bytes(p, true);
}

uint32_t dci_demo_preset_init_bytes(const dci_demo_preset_t *p)
{
    if (p != NULL && p->fmt == DCI_FMT_JPEG)
    {
        /* No computable frame size: the buffer is a capacity cap chosen by the demo. */
        return DCI_DEMO_JPEG_CAP_KB * 1024UL;
    }
    return dci_preset_bytes(p, false);
}

/*
 * Total arena a preset needs, guard fence included. Mirrors the layout in
 * dci_demo_buf.c: one fence word-group per buffer plus a trailing one, so a run past
 * the last buffer is still caught.
 */
uint32_t dci_demo_preset_pool_bytes(const dci_demo_preset_t *p, uint8_t buf_count)
{
    uint32_t per = dci_demo_preset_init_bytes(p);

    if (p == NULL || buf_count == 0)
    {
        return 0;
    }
    per = (per + 3U) & ~3U;             /* word aligned, as the pool carves it */

    return buf_count * (per + DCI_DEMO_GUARD_BYTES) + DCI_DEMO_GUARD_BYTES;
}

void dci_demo_preset_to_init(const dci_demo_preset_t *p, DCI_InitTypeDef *init)
{
    if (p == NULL || init == NULL)
    {
        return;
    }

    DCI_StructInit(init);

    /* The hardware counts pixel clocks, so declare the geometry in pclk. This is the
     * conversion that has to agree with the sensor's real output or every frame
     * desyncs (DESIGN.md 6.2). */
    init->sensor_pclk_per_line = p->width * dci_demo_fmt_pclk_per_pixel(p->fmt);
    init->sensor_lines         = p->height;

    init->edm    = (p->fmt == DCI_FMT_RAW10) ? DCI_EDM_10BIT : DCI_EDM_8BIT;
    init->jpeg   = (p->fmt == DCI_FMT_JPEG);
    init->sync   = p->sync;
    init->fcrc   = p->fcrc;
    init->pckpol = p->pckpol;
    init->hspol  = p->hspol;
    init->vspol  = p->vspol;
    init->bsm    = p->bsm;
    init->oebs   = p->oebs;
    init->lsm    = p->lsm;
    init->oels   = p->oels;

    /* Left false on purpose: Capture_Init() does not accept a crop window, because
     * the window is programmed by DCI_ConfigCrop() afterwards. */
    init->crop = false;
}

/*============================================================================*
 *                         Menu
 *============================================================================*/
/*
 * The RAM columns are the point of this listing at a 1 MB budget: which presets are
 * runnable is decided by the pool, and a "2x: NO" here is the same information as a
 * DEMO_FAIL_POOL later, just available before flashing.
 */
void dci_demo_preset_list(void)
{
    uint32_t pool = DCI_DEMO_POOL_KB * 1024UL;
    uint8_t  i;

    DEMO_LOG("--- presets (pool %u KB, guard fence included) ---",
             (unsigned)DCI_DEMO_POOL_KB);

    for (i = 0; i < DCI_PRESET_COUNT; i++)
    {
        const dci_demo_preset_t *p     = &dci_presets[i];
        uint32_t                 frame = dci_demo_preset_frame_bytes(p);
        uint32_t                 need  = dci_demo_preset_pool_bytes(p, 1);
        uint32_t                 need2 = dci_demo_preset_pool_bytes(p, 2);

        DEMO_LOG(" %2d) %-32s %-6s %ux%u", i, p->name,
                 dci_demo_fmt_name(p->fmt),
                 p->crop_en ? p->crop_w : p->width,
                 p->crop_en ? p->crop_h : p->height);
        DEMO_LOG("     frame %7u B   pool 1x %7u B %-3s   2x %7u B %-3s",
                 (unsigned)frame,
                 (unsigned)need, (need  <= pool) ? "ok" : "NO",
                 (unsigned)need2, (need2 <= pool) ? "ok" : "NO");
        DEMO_LOG("     pol pck=%u hs=%u vs=%u  sync=%s%s%s%s",
                 p->pckpol, p->hspol, p->vspol,
                 (p->sync == DCI_SYNC_EMBEDDED) ? "embedded" : "hw",
                 p->crop_en ? "  crop" : "",
                 p->lsm ? "  lsm" : "",
                 (p->bsm != DCI_BSM_ALL) ? "  bsm" : "");
        DEMO_LOG("     %s", p->note);
    }
}
