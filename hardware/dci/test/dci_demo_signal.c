/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DCI demo: the signal table, the overlay resolver and the geometry derivation.
 *
 * Row order follows the camera table's own principle - the earlier a row is, the safer
 * it is, and each row introduces exactly one new variable relative to something above
 * it. Run them in order and a failure at row N with 0..N-1 passing localises the fault
 * to what row N added, with no bisection.
 *
 * The geometry derivation here is a deliberate second implementation of the driver's
 * dci_calc_geometry(). It is not redundancy for its own sake: the two are computed from
 * different inputs (a table row versus DCI_InitTypeDef) and compared at setup time, so
 * a disagreement points at whichever formula is wrong before any capture is trusted.
 * If this file ever just called into the driver, the check would be vacuous.
 */

#include <string.h>
#include "dci_demo.h"
#include "dci_demo_signal.h"

/*============================================================================*
 *                         Format properties
 *============================================================================*/
/*
 * The one place the bytes-per-pixel / pclk-per-pixel distinction is written down.
 * RAW10 is the row that matters: 1 pclk per pixel but 2 bytes per pixel, the inverse
 * of RGB565. Any formula that conflates the two works on every other format and fails
 * only here, which is exactly why the table is a table.
 */
typedef struct
{
    const char *name;
    uint8_t     bytes_per_pixel;
    uint8_t     pclk_per_pixel;
} dci_fmt_prop_t;

static const dci_fmt_prop_t dci_fmt_prop[] =
{
    [DCI_FMT_NONE]   = {"raw-bus", 1, 1},
    [DCI_FMT_RGB565] = {"RGB565",  2, 2},
    [DCI_FMT_YUV422] = {"YUV422",  2, 2},
    [DCI_FMT_RAW8]   = {"RAW8",    1, 1},
    [DCI_FMT_RAW10]  = {"RAW10",   2, 1},
    [DCI_FMT_JPEG]   = {"JPEG",    0, 1},
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
 *                         The signal table
 *============================================================================*/
/*
 * Rows 0..11 each correspond to a camera-side preset; rows 12..15 have no camera row
 * because the OV2640 cannot emit those geometries, but they cover receiver-side
 * behaviour (block splitting, the pool ceiling) that has to be covered somewhere.
 *
 * What is NOT here, and why:
 *
 *   camera qvga-yuv422    -> row 2. Two bytes per pixel, two pclk per pixel, identical
 *                            geometry. The receiver has no way to tell them apart, and
 *                            a row that configures the peripheral identically is not a
 *                            second test. Kept as a distinct FORMAT value only so the
 *                            log records which stimulus was used.
 *   camera qvga-colorbar  -> row 2. The pattern is generated inside the sensor; the
 *                            receiver configuration is byte for byte the same.
 *   camera qvga-pclk-gated-> row 2. "PCLK absent during blanking" has no receiver
 *                            configuration bit at all - DCI counts the edges it is
 *                            given. Worth running AS a stimulus against row 2, which
 *                            is why the row's stim column names it too.
 *   camera ov5640-hwsync-yuv422 and ov5640-rgb565 -> row 2, both of them. Hardware sync
 *                            at 320x240 is already covered there, and the sensor
 *                            identity is not a receiver-side variable: hwsync-yuv422 is
 *                            {2 bytes, 2 pclk} like RGB565, and ov5640-rgb565 is
 *                            field for field row 2 already.
 *
 * That is 16 camera rows mapping onto 12 receiver rows. The collapse is the point: it
 * says out loud which stimulus differences this peripheral is blind to.
 */
static const dci_demo_signal_t dci_signals[] =
{
    /*--- 0: the null row -------------------------------------------------------*/
    /*
     * No format, no polarity, no feature: every DCI_InitTypeDef field at its reset
     * value, which is what DCI_StructInit() gives. 320 bus bytes over 120 lines, so
     * one block and 37 KB - the smallest thing that can still prove data moved.
     *
     * This exists because "nothing arrived" and "something arrived wrong" are
     * different faults with the same symptom, and every other row can produce both.
     * Here there is no configuration left to be wrong: if this row fails, the fault
     * is in the clock, the pins, the DMA or the peripheral itself, and no amount of
     * table-fiddling will help. It is the row that makes the others interpretable.
     *
     * Deliberately identical to row 1 - and not just in geometry. Run both through
     * dci_demo_cfg_to_init() and the two DCI_InitTypeDef structs are BYTE IDENTICAL
     * (verified by memcmp on the host: 320 pclk/line either way, because NONE is
     * {1 byte, 1 pclk} per unit at width 320 and RGB565 is {2, 2} at width 160).
     * fmt is the only field that differs between the rows, and for these two values
     * fmt reaches no register at all: it only selects edm (both 8BIT here) and jpeg
     * (both false).
     *
     * So by the "do not list duplicate configurations" rule these two are ONE
     * configuration, and the second is kept on purpose. The difference is not what
     * the hardware is told - it is what the DEMO claims. Row 1 declares a format,
     * a pixel count and a bytes-per-pixel ratio; row 0 declares nothing. The same
     * bytes reach memory through a configuration that has no opinion about them, so
     * if row 0 passes and row 1 fails, the fault is in the demo's own arithmetic and
     * not in the peripheral - which no single row can show on its own.
     */
    {
        .name   = "null-min",
        .stim   = "safe-160x120",
        .note   = "empty config: DCI_StructInit defaults only, 320 B/line x 120. "
        "Basic reception. Nothing here can be misconfigured.",
        .width  = 320,          /* bus bytes, not pixels - fmt is NONE */
        .height = 120,
        .fmt    = DCI_FMT_NONE,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync   = DCI_SYNC_HARDWARE,
    },

    /*--- 1..2: the baselines ---------------------------------------------------*/
    {
        .name   = "qqvga-rgb565",
        .stim   = "safe-160x120",
        .note   = "smallest sane frame: 1 block, 37 KB, fits anywhere. Start here.",
        .width  = 160, .height = 120,
        .fmt    = DCI_FMT_RGB565,
        .pckpol = true, .hspol = true, .vspol = true,
        .sync   = DCI_SYNC_HARDWARE,
    },
    {
        /* The reference point. Every polarity and format row below differs from this
         * one in exactly one field, and the camera table's main row drives it. */
        .name   = "qvga-rgb565",
        .stim   = "qvga-rgb565 (also -yuv422 / -colorbar / -pclk-gated)",
        .note   = "target resolution, the reference for every single-variable row. "
        "Also receives yuv422, colorbar and pclk-gated unchanged.",
        .width  = 320, .height = 240,
        .fmt    = DCI_FMT_RGB565,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync   = DCI_SYNC_HARDWARE,
    },

    /*--- 3..6: polarity, one bit at a time -------------------------------------*/
    /*
     * The pass criterion is not "the image looks right" - it is that matching the
     * stimulus works and mismatching it visibly breaks. A polarity row that produces
     * a good image with the wrong setting means the peripheral is ignoring that
     * configuration bit, which is itself the bug these rows exist to find.
     *
     * READ THIS BEFORE PAIRING A ROW WITH A CAMERA PRESET. Two inversions live here,
     * and both of them flip the answer:
     *
     * 1. "Active" in hspol/vspol is the level during BLANKING, not during valid data.
     *    hspol = false ("HSYNC active low") therefore means HSYNC sits low while
     *    blanking and data is captured while it is HIGH. See TESTPLAN.md 3.1, whose
     *    expected result is "only data with both HSYNC and VSYNC in their INACTIVE
     *    state is captured". So camera qvga-href-neg - HREF inverted, i.e. high
     *    during blanking - is received with hspol = TRUE, which is what row 4 sets.
     *    The row name says "high" because that is the register's own wording for the
     *    bit; it does not mean "valid when high". Same for vspol and row 5.
     *
     * 2. pckpol is the edge this receiver SAMPLES on, while the camera's COM10[4] is
     *    the edge the sensor UPDATES data on - opposite things (ov2640_preset.h says
     *    so explicitly). Data updated on the falling edge is stable around the
     *    rising one, so the electrically correct pairing is the OPPOSITE edge:
     *    OV2640 default (COM10[4]=0, updates falling) wants pckpol = true, and
     *    camera qvga-pclk-rise wants pckpol = false. ST's own OV2640 examples use
     *    DCMI_PCKPOLARITY_RISING against a default-configured sensor.
     *
     *    The rows below pair the SAME edge instead: rows 0..2 take pckpol = false
     *    against a falling-edge sensor and row 3 takes pckpol = true against
     *    qvga-pclk-rise. That is sampling on the edge the data changes on. At the
     *    <= 10 MHz PCLK this bench runs it may latch correctly anyway on internal
     *    delay alone, which is the danger: if BOTH settings produce a good image the
     *    bit is not being honoured, and by the criterion above that is the bug the
     *    row exists to find. The pairing is left as-is because it has never been
     *    checked against hardware - when row 3 is first run, try both pckpol values
     *    and keep whichever one the data agrees with, then fix this comment.
     */
    {
        .name   = "qvga-pck-rising",
        .stim   = "qvga-pclk-rise",
        .note   = "polarity: pckpol=1, sample on the rising edge. One bit from "
        "qvga-rgb565. See the edge-pairing warning above.",
        .width  = 320, .height = 240,
        .fmt    = DCI_FMT_RGB565,
        .pckpol = true, .hspol = false, .vspol = false,
        .sync   = DCI_SYNC_HARDWARE,
    },
    {
        .name   = "qvga-href-high",
        .stim   = "qvga-href-neg",
        .note   = "polarity: hspol=1, HSYNC high during blanking (data captured while "
        "low). One bit from qvga-rgb565.",
        .width  = 320, .height = 240,
        .fmt    = DCI_FMT_RGB565,
        .pckpol = false, .hspol = true, .vspol = false,
        .sync   = DCI_SYNC_HARDWARE,
    },
    {
        .name   = "qvga-vsync-high",
        .stim   = "qvga-vsync-neg",
        .note   = "polarity: vspol=1, VSYNC high during blanking. Separate from row 4 "
        "because HSYNC and VSYNC take different state machine branches.",
        .width  = 320, .height = 240,
        .fmt    = DCI_FMT_RGB565,
        .pckpol = false, .hspol = false, .vspol = true,
        .sync   = DCI_SYNC_HARDWARE,
    },
    {
        /* The combination boundary. Three rows above pass individually and this one
         * fails only if the polarity bits are coupled - a class of bug no single-bit
         * row can reach, and the reason the camera table carries all-inverted. */
        .name   = "qvga-all-inverted",
        .stim   = "qvga-all-inverted",
        .note   = "all three polarities inverted at once: catches coupling between "
        "the bits that the single-bit rows cannot.",
        .width  = 320, .height = 240,
        .fmt    = DCI_FMT_RGB565,
        .pckpol = true, .hspol = true, .vspol = true,
        .sync   = DCI_SYNC_HARDWARE,
    },

    /*--- 7..10: formats --------------------------------------------------------*/
    {
        /* The 8-bit RAW control group: when a 10-bit or byte-select row fails, this
         * separates "the RAW path is broken" from "that feature is broken". */
        .name   = "qvga-raw8",
        .stim   = "qvga-raw8",
        .note   = "1 byte/pixel: pclk/line halves while lines stay. RAW control "
        "group, and the main row byte select is exercised on (only this "
        "row and null-min take all four settings).",
        .width  = 320, .height = 240,
        .fmt    = DCI_FMT_RAW8,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync   = DCI_SYNC_HARDWARE,
    },
    {
        /*
         * The one format where pclk per pixel and bytes per pixel disagree: 320 pclk
         * per line carrying 640 bytes, where RGB565 at the same width needs 640 pclk
         * for the same 640 bytes. Every formula that conflates the two survives all
         * other rows and dies here. EDM=10BIT is also the only hardware-forced mode
         * in the table - it cannot be reached with any RGB565 configuration.
         */
        .name   = "qvga-raw10",
        .stim   = "qvga-raw10",
        .note   = "EDM=10BIT: 1 pclk = one 16-bit sample. The only row where "
        "pclk/pixel != bytes/pixel. bit15:10 of every sample must be 0.",
        .width  = 320, .height = 240,
        .fmt    = DCI_FMT_RAW10,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync   = DCI_SYNC_HARDWARE,
    },
    {
        /*
         * Embedded sync: no HSYNC/VSYNC pins, frame and line boundaries arrive as
         * 0x00/0xFF code words in the data. Spec 2.4 only requires 8-bit, so any
         * 8-bit-EDM format passes validation - but full-range data contains pure black
         * and pure white pixels, which produce exactly those byte values and get
         * mistaken for codes. Restricted range is the practical requirement, and
         * YUV422 is the format that provides it natively: BT.656 limits Y to 16..235
         * and C to 16..240, so 0x00/0xFF cannot occur in pixel data at all. That is
         * also what the camera-side preset emits, so the geometry matches.
         *
         * Geometry is identical to qvga-rgb565 - YUV422 and RGB565 are both {2 bytes,
         * 2 pclk} per pixel and DCI cannot tell them apart. This row exists for the one
         * field that differs: sync = EMBEDDED, which is the only path through ESCR/ESUR
         * and the code-word parser.
         */
        .name   = "qvga-embedded",
        .stim   = "ov5640-sync-yuv422",
        .note   = "embedded sync codes instead of sync pins. YUV422 because BT.656 "
        "restricted range (Y 16..235, C 16..240) cannot produce the "
        "0x00/0xFF code words that full-range pixels alias onto. "
        "NOTE the OV5640 does not actually emit codes yet - see the "
        "camera-side table, it warns and behaves as hardware sync.",
        .width  = 320, .height = 240,
        .fmt    = DCI_FMT_YUV422,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync   = DCI_SYNC_EMBEDDED,
    },
    {
        /*
         * Variable length: there is no frame size to validate against, the buffer is
         * a capacity cap, and the frame ends at FF D9 or VSYNC rather than at a byte
         * count. Structurally different from every other row, which is why crop is
         * refused with it (spec 2.7) and why the descriptor topology changes.
         */
        .name   = "qvga-jpeg",
        .stim   = "qvga-jpeg",
        .note   = "variable length: buffer is a capacity cap, frame ends at FF D9. "
        "Fixed-size checks do not apply; crop is refused.",
        .width  = 320, .height = 240,
        .fmt    = DCI_FMT_JPEG,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync   = DCI_SYNC_HARDWARE,
    },

    /*--- 11: largest camera row ------------------------------------------------*/
    {
        .name   = "vga-rgb565",
        .stim   = "vga-rgb565",
        .note   = "largest size the camera emits: 600 KB, 3 blocks. Single buffer "
        "only at a 960 KB pool. Bandwidth and memory pressure.",
        .width  = 640, .height = 480,
        .fmt    = DCI_FMT_RGB565,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync   = DCI_SYNC_HARDWARE,
    },

    /*--- 12: geometry-only row, no camera stimulus -----------------------------*/
    /*
     * Beyond what the OV2640 table can produce, kept because the receiver-side
     * behaviour it triggers has to be covered. Against a live camera it will fail on
     * frame length - that is expected and stated in the note. Its value is at config
     * time (the word-aligned split) and against a pattern generator.
     *
     * This used to be four rows (466-raw8, 466-rgb565, svga-rgb565, vga-raw10). The
     * other three were removed by request; what went with them is recorded in
     * TESTMATRIX.md, because the coverage is gone rather than relocated: the 2-block
     * and 4-block split paths now have no row at all (the table has 1- and 3-block
     * rows only), the double-buffer and pool ceilings are no longer pinned by a row,
     * and RAW10 rests on row 8 alone.
     */
    {
        /*
         * The only row that reaches the driver's word-aligned fallback split
         * (rtl_dci.c, dci_calc_geometry() line_aligned == false branch), where the
         * driver gives up on splitting by lines and splits the frame by words
         * instead: a different offset formula (first * 4 rather than
         * first * line_bytes, dci_chunk_span()), a different block-count derivation,
         * and lines_per_chunk forced to 0 because it no longer means anything.
         *
         * Note what this row does NOT demonstrate: an actual mid-line block boundary.
         * At 466x466 the frame is 54289 words, under the 65535-word block limit, so
         * it takes ONE block and the only boundary is the start of the frame. What
         * the row covers is the branch itself - whether the driver takes it and
         * whether the geometry it computes there is right. A boundary landing
         * mid-line additionally needs a frame large enough for 2+ blocks at an
         * unaligned line length, which no row in this table produces.
         *
         * Verified by sweeping all 16 rows against all 32 accepted overlay
         * combinations on the host: line_aligned == 0 occurs on this row and no
         * other, in 4 of those combinations (overlay off, LSM, BSM=1, BSM=1+LSM).
         * Delete this row and that branch has zero coverage.
         *
         * Why nothing else gets there. The trigger is line_bytes % 4 != 0, and
         * line_bytes = pclk_per_line * bytes_per_pclk / bsm_den * bsm_num:
         *   - every other row is 320/466/640/932/1280/1600 pclk at 1 or 2 bytes,
         *     and all of those products except 466 are multiples of 4;
         *   - 2 bytes/pixel could only get there with an ODD pixel width, and no
         *     sensor emits one;
         *   - BSM can make a line odd (466 -> 233), but the resolver refuses BSM on
         *     any 2-byte format, so a 466-wide RGB565 row could only reach it through
         *     the raw driver. That was true while 466-rgb565 existed as a row and is
         *     moot now that it is gone.
         *
         * CAREFUL: cropping this row DESTROYS what it tests. DCI_ConfigCrop() aligns
         * the window width down to a multiple of 4 pclk, so CROP_FULL turns 466 into
         * 464 and line_bytes becomes word aligned again - the row silently stops
         * exercising the branch. Run it with the overlay off, or with LSM/BSM=1,
         * which keep the line length odd.
         */
        .name   = "466-raw8",
        .stim   = NULL,
        .note   = "466 B/line is not a multiple of 4 -> the word-aligned fallback "
        "split, blocks no longer whole lines. The only row that reaches "
        "that branch. No camera stimulus: config-time row.",
        .width  = 466, .height = 466,
        .fmt    = DCI_FMT_RAW8,
        .pckpol = false, .hspol = false, .vspol = false,
        .sync   = DCI_SYNC_HARDWARE,
    },
};

#define DCI_SIGNAL_COUNT    (sizeof(dci_signals) / sizeof(dci_signals[0]))

/* The ids in dci_demo_id.h are plain #defines because they must work inside #if, so
 * nothing but this assert keeps them in step with the table. */
_Static_assert(DCI_SIGNAL_COUNT == DCI_DEMO_SIGNAL_MAX,
               "signal table and DCI_DEMO_SIG_* in dci_demo_id.h are out of step");

uint8_t dci_demo_signal_count(void)
{
    return (uint8_t)DCI_SIGNAL_COUNT;
}

const dci_demo_signal_t *dci_demo_signal_get(uint8_t index)
{
    return (index < DCI_SIGNAL_COUNT) ? &dci_signals[index] : NULL;
}

/*============================================================================*
 *                         Overlay
 *============================================================================*/
/*
 * The overlay is compile-time, so it is read once here rather than threaded through
 * every case. Crop dimensions are expressed as a percentage of the row rather than in
 * absolute pixels: the table spans 160x120 to 800x600, and an absolute window would be
 * meaningless on one end or out of bounds on the other. Percentages make a single
 * macro setting valid on every row.
 */
static void dci_demo_overlay_apply(dci_demo_cfg_t *c)
{
    const dci_demo_signal_t *s = c->sig;

    (void)s;                    /* only read by the crop branches below */

    c->fcrc = (DCI_FCRC_T)DCI_DEMO_OV_FCRC;
    c->bsm  = (DCI_BSM_T)DCI_DEMO_OV_BSM;
    c->oebs = (DCI_DEMO_OV_OEBS != 0);
    c->lsm  = (DCI_DEMO_OV_LSM  != 0);
    c->oels = (DCI_DEMO_OV_OELS != 0);

#if (DCI_DEMO_OV_CROP == DCI_DEMO_CROP_OFF)
    c->crop_en = false;
#else
    c->crop_en = true;
#if   (DCI_DEMO_OV_CROP == DCI_DEMO_CROP_CENTER)
    /* Half in each axis, centred: the ordinary case, and the one where a
     * transposed x/y or w/h shows up as a shifted window rather than a failure. */
    c->crop_w = (uint16_t)(s->width  / 2);
    c->crop_h = (uint16_t)(s->height / 2);
    c->crop_x = (uint16_t)(s->width  / 4);
    c->crop_y = (uint16_t)(s->height / 4);
#elif (DCI_DEMO_OV_CROP == DCI_DEMO_CROP_ORIGIN)
    /* The x0 = y0 = 0 boundary. Worth its own setting because an off-by-one in
     * the N-1 window encoding is invisible when the window starts in the middle. */
    c->crop_w = (uint16_t)(s->width  / 5);
    c->crop_h = (uint16_t)(s->height / 5);
    c->crop_x = 0;
    c->crop_y = 0;
#elif (DCI_DEMO_OV_CROP == DCI_DEMO_CROP_FULL)
    /* The whole frame, cropping enabled. Should be byte-identical to crop off:
     * isolates "the window is wrong" from "enabling CROP at all is wrong". */
    c->crop_w = s->width;
    c->crop_h = s->height;
    c->crop_x = 0;
    c->crop_y = 0;
#else
#error "DCI_DEMO_OV_CROP is not one of DCI_DEMO_CROP_*"
#endif
#endif /* DCI_DEMO_OV_CROP */
}

/*============================================================================*
 *                         Geometry
 *============================================================================*/
/*
 * Mirrors dci_calc_geometry() in rtl_dci.c, from the table row instead of from
 * DCI_InitTypeDef. Same order, same rounding:
 *
 *   line_bytes = pclk_per_line x bytes_per_pclk     (crop replaces pclk_per_line)
 *   line_bytes = line_bytes / bsm_den x bsm_num     (byte select)
 *   lines_out  = lsm ? (oels ? h/2 : ceil(h/2)) : h (line select)
 *
 * Byte select is the subtle one: the group has to divide the line exactly. The driver
 * refuses it otherwise rather than rounding (dci_demo_bsm_divides() below), so the
 * division here is only ever exact - if it truncated, the cross-check against
 * DCI_Capture_GetFrameInfo() would fire on the next line of output.
 */
static void dci_demo_bsm_ratio(DCI_BSM_T bsm, uint32_t *num, uint32_t *den)
{
    switch (bsm)
    {
    case DCI_BSM_EVERY_2ND: *num = 1; *den = 2; break;
    case DCI_BSM_1_OF_4:    *num = 1; *den = 4; break;
    case DCI_BSM_2_OF_4:    *num = 2; *den = 4; break;
    default:                *num = 1; *den = 1; break;
    }
}

/*
 * Whether the byte-select group divides the line evenly, which the driver requires:
 * dci_calc_geometry() returns INVALID_PARAM on (line_bytes % bsm_den) != 0, because a
 * partial group at the end of a line would shift every line after it.
 *
 * Both geometries have to divide, not just the one that gets delivered. Capture_Init()
 * measures the uncropped line, and DCI_CropCmd() then re-runs the same arithmetic on the
 * window - so a row can be refused at Init and never reach the crop at all. The window
 * always divides in practice, since DCI_ConfigCrop() aligns its width down to 4 pclk, but
 * that is a property of the driver rather than of this table, so it is checked rather
 * than assumed.
 *
 * 466-raw8 is the row this exists for: 466 bytes per line is even but not a multiple of
 * 4, so 1-of-2 is legal there and 1-of-4 and 2-of-4 are not.
 */
static bool dci_demo_bsm_divides(const dci_demo_cfg_t *c, uint32_t *bad_line,
                                 uint32_t *den_out)
{
    uint32_t num, den;
    uint32_t full = (uint32_t)c->pclk_per_line * c->bytes_per_pclk;

    dci_demo_bsm_ratio(c->bsm, &num, &den);
    *den_out = den;
    (void)num;

    if ((full % den) != 0)
    {
        *bad_line = full;
        return false;
    }
    if (c->crop_en)
    {
        uint32_t win = (uint32_t)c->crop_pclk_w * c->bytes_per_pclk;

        if ((win % den) != 0)
        {
            *bad_line = win;
            return false;
        }
    }
    return true;
}

static void dci_demo_cfg_derive(dci_demo_cfg_t *c)
{
    const dci_demo_signal_t *s = c->sig;
    uint8_t  ppp = dci_demo_fmt_pclk_per_pixel(s->fmt);
    uint32_t num, den;
    uint32_t pclk;
    uint16_t lines;

    c->bytes_per_pclk = (s->fmt == DCI_FMT_RAW10) ? 2 : 1;
    c->pclk_per_line  = (uint16_t)(s->width * ppp);
    c->lines_in       = s->height;

    /* Crop is programmed in pixel clocks, and RAW10 is where that stops being a
     * formality. Align the width down exactly as DCI_ConfigCrop() will, so the
     * printed configuration is what the hardware ends up with rather than what was
     * asked for. */
    c->crop_pclk_x = (uint16_t)(c->crop_x * ppp);
    c->crop_pclk_w = (uint16_t)((c->crop_w * ppp) & ~0x3U);

    if (s->fmt == DCI_FMT_JPEG)
    {
        /* No frame size exists. The buffer cap is the only number, and the demo
         * must not pretend to know more than that. */
        c->line_bytes  = 0;
        c->lines_out   = 0;
        c->frame_bytes = 0;
        c->init_bytes  = DCI_DEMO_JPEG_CAP_KB * 1024UL;
        return;
    }

    pclk  = c->crop_en ? c->crop_pclk_w : c->pclk_per_line;
    lines = c->crop_en ? c->crop_h      : c->lines_in;

    dci_demo_bsm_ratio(c->bsm, &num, &den);
    c->line_bytes = pclk * c->bytes_per_pclk;
    c->line_bytes = c->line_bytes / den * num;

    c->lines_out = c->lsm ? (c->oels ? (uint16_t)(lines / 2)
                             : (uint16_t)((lines + 1) / 2))
                   : lines;

    c->frame_bytes = c->line_bytes * c->lines_out;

    /*
     * What Capture_Init() will validate against, which is not what will be delivered
     * when cropping. Init sees only DCI_InitTypeDef - the window arrives later via
     * DCI_ConfigCrop() - so it measures the UNCROPPED frame and rejects a buffer
     * sized for the cropped one with BUF_TOO_SMALL. Allocating for the larger of the
     * two is the whole reason this field is separate from frame_bytes.
     */
    if (c->crop_en)
    {
        uint32_t full_line = (uint32_t)c->pclk_per_line * c->bytes_per_pclk / den * num;
        uint16_t full_out  = c->lsm ? (c->oels ? (uint16_t)(c->lines_in / 2)
                                       : (uint16_t)((c->lines_in + 1) / 2))
                             : c->lines_in;
        c->init_bytes = full_line * full_out;
    }
    else
    {
        c->init_bytes = c->frame_bytes;
    }
}

/*============================================================================*
 *                         Resolve
 *============================================================================*/
bool dci_demo_cfg_resolve(uint8_t index, dci_demo_cfg_t *out)
{
    const dci_demo_signal_t *s = dci_demo_signal_get(index);

    if (out == NULL)
    {
        return false;
    }
    memset(out, 0, sizeof(*out));

    if (s == NULL)
    {
        DEMO_LOG("[CFG] signal %u does not exist (have %u)", index,
                 dci_demo_signal_count());
        return false;
    }
    out->sig   = s;
    out->index = index;

    dci_demo_overlay_apply(out);

    /*
     * Rules, in two groups. The first three the driver would refuse anyway; catching
     * them here means the message names the macro that has to change instead of
     * surfacing as MODE_CONFLICT from Capture_Init with no hint which of four rules
     * tripped.
     */
    if (out->crop_en && s->fmt == DCI_FMT_JPEG)
    {
        DEMO_LOG("[CFG] %s: crop and JPEG are mutually exclusive (spec 2.7)", s->name);
        DEMO_LOG("      -> set DCI_DEMO_OV_CROP to DCI_DEMO_CROP_OFF, or pick "
                 "another signal row");
        return false;
    }
    if (out->bsm != DCI_BSM_ALL && s->fmt == DCI_FMT_RAW10)
    {
        DEMO_LOG("[CFG] %s: byte select needs EDM=8BIT, this row is 10-bit", s->name);
        DEMO_LOG("      -> DCI_DEMO_OV_BSM must be DCI_BSM_ALL on a RAW10 row");
        return false;
    }
    if (s->sync == DCI_SYNC_EMBEDDED && s->fmt == DCI_FMT_RAW10)
    {
        /* Not reachable from the table as written - no row is both - but the check
         * belongs here rather than in a comment, because adding such a row is a
         * plausible mistake and this is where it would be caught. */
        DEMO_LOG("[CFG] %s: embedded sync is 8-bit only (spec 2.4)", s->name);
        return false;
    }

    /*
     * The second group is the dangerous one: the driver ACCEPTS these, the geometry
     * computes, the capture reports no error, and the image is quietly wrong.
     *
     * Byte select drops bytes. At 1 byte per pixel that is "drop pixels"; at 2 bytes
     * per pixel it takes one byte out of each pixel and leaves a buffer full of half
     * pixels. DCI has no concept of a pixel and so cannot possibly refuse it - this
     * check is the only thing standing between the macro and a corrupt frame that
     * looks like a hardware fault.
     */
    if (out->bsm != DCI_BSM_ALL && s->fmt == DCI_FMT_JPEG)
    {
        /* Before the bytes-per-pixel rule below, which would report JPEG as a
         * "0-byte/pixel format" - true of the table entry, meaningless as a diagnosis.
         * Dropping bytes out of an entropy-coded stream does not decimate it, it
         * destroys it from the first dropped byte onwards. */
        DEMO_LOG("[CFG] %s: byte select on a JPEG stream corrupts it from the first "
                 "dropped byte on", s->name);
        DEMO_LOG("      -> set DCI_DEMO_OV_BSM to 0 on a JPEG row");
        return false;
    }
    if (out->bsm != DCI_BSM_ALL && dci_demo_fmt_bytes_per_pixel(s->fmt) != 1)
    {
        DEMO_LOG("[CFG] %s: byte select on a %u-byte/pixel format would keep part "
                 "of every pixel", s->name, dci_demo_fmt_bytes_per_pixel(s->fmt));
        DEMO_LOG("      the driver would ACCEPT this and the frame would be corrupt "
                 "with no error reported");
        DEMO_LOG("      -> byte select belongs on a 1-byte/pixel row: null-min, "
                 "qvga-raw8, or 466-raw8 at setting 1");
        return false;
    }
    if (out->fcrc != DCI_FCRC_ALL && s->fmt == DCI_FMT_JPEG)
    {
        /* Legal, but frame rate control drops whole frames and JPEG frames end by
         * content: dropping one mid-stream is not distinguishable from a truncated
         * one. Refuse rather than produce an unreadable result. */
        DEMO_LOG("[CFG] %s: frame rate control on JPEG makes a dropped frame "
                 "indistinguishable from a truncated one", s->name);
        return false;
    }

    dci_demo_cfg_derive(out);

    /*
     * The one byte-select rule that needs the derived line length. Unlike the rules
     * above, the driver does refuse this - but it refuses it as INVALID_PARAM out of
     * Capture_Init, alongside a null lines_out, a zero-length line and four unrelated
     * parameter faults. Naming the line length and the group size here is the difference
     * between "change DCI_DEMO_OV_BSM" and reading dci_calc_geometry().
     */
    if (out->bsm != DCI_BSM_ALL)
    {
        uint32_t bad_line, den;

        if (!dci_demo_bsm_divides(out, &bad_line, &den))
        {
            DEMO_LOG("[CFG] %s: %u B/line is not a multiple of the %u-byte select "
                     "group", s->name, (unsigned)bad_line, (unsigned)den);
            DEMO_LOG("      a partial group at the end of a line would shift every "
                     "line after it, so the driver refuses it (INVALID_PARAM)");
            DEMO_LOG("      -> DCI_DEMO_OV_BSM 1 (one of two) divides this row; 2 and "
                     "3 need a line length that is a multiple of 4");
            return false;
        }
    }

    /* Only reachable through the crop macros; a 0-pclk window after aligning down
     * would be rejected by ConfigCrop, so say which macro produced it. */
    if (out->crop_en && (out->crop_pclk_w == 0 || out->crop_h == 0))
    {
        DEMO_LOG("[CFG] %s: crop window is empty after alignment (%u x %u pclk)",
                 s->name, out->crop_pclk_w, out->crop_h);
        return false;
    }
    if (s->fmt != DCI_FMT_JPEG && out->frame_bytes == 0)
    {
        DEMO_LOG("[CFG] %s: geometry collapses to 0 bytes", s->name);
        return false;
    }
    return true;
}

/*============================================================================*
 *                         Translate
 *============================================================================*/
void dci_demo_cfg_to_init(const dci_demo_cfg_t *c, DCI_InitTypeDef *init)
{
    const dci_demo_signal_t *s = c->sig;

    /* Defaults first, so a field added to DCI_InitTypeDef later arrives at its reset
     * value here instead of as stack garbage. */
    DCI_StructInit(init);

    /* The declared geometry is always the UNCROPPED one: Capture_Init() validates
     * against it, and the window is programmed afterwards. */
    init->sensor_pclk_per_line = c->pclk_per_line;
    init->sensor_lines         = c->lines_in;

    init->edm    = (s->fmt == DCI_FMT_RAW10) ? DCI_EDM_10BIT : DCI_EDM_8BIT;
    init->jpeg   = (s->fmt == DCI_FMT_JPEG);
    init->sync   = s->sync;
    init->pckpol = s->pckpol;
    init->hspol  = s->hspol;
    init->vspol  = s->vspol;

    init->fcrc = c->fcrc;
    init->bsm  = c->bsm;
    init->oebs = c->oebs;
    init->lsm  = c->lsm;
    init->oels = c->oels;

    /* Left false even when the overlay crops: DCI_CropCmd(ENABLE) turns it on after
     * the window exists. Setting it here would make Init derive its geometry from a
     * window of zeros. */
    init->crop = false;
}

/*============================================================================*
 *                         Pool arithmetic
 *============================================================================*/
uint32_t dci_demo_cfg_pool_bytes(const dci_demo_cfg_t *c, uint8_t buf_count)
{
    uint32_t per = (c->init_bytes + 3U) & ~3UL;

    /* Mirrors dci_demo_buf_alloc(): a fence between every pair of buffers and one
     * at each end, hence count+1 fences. */
    return buf_count * (per + DCI_DEMO_GUARD_BYTES) + DCI_DEMO_GUARD_BYTES;
}

/*============================================================================*
 *                         Printing
 *============================================================================*/
static const char *dci_demo_fcrc_name(DCI_FCRC_T f)
{
    switch (f)
    {
    case DCI_FCRC_HALF:    return "every 2nd frame";
    case DCI_FCRC_QUARTER: return "1 frame of 4";
    default:               return "every frame";
    }
}

static const char *dci_demo_bsm_name(DCI_BSM_T b)
{
    switch (b)
    {
    case DCI_BSM_EVERY_2ND: return "1 byte of 2";
    case DCI_BSM_1_OF_4:    return "1 byte of 4";
    case DCI_BSM_2_OF_4:    return "2 bytes of 4";
    default:                return "all bytes";
    }
}

void dci_demo_signal_list(void)
{
    uint8_t i;

    DEMO_LOG("signal table: %u rows", dci_demo_signal_count());
    DEMO_LOG("");

    for (i = 0; i < dci_demo_signal_count(); i++)
    {
        const dci_demo_signal_t *s = dci_demo_signal_get(i);
        dci_demo_cfg_t           c;

        /* "B/line" rather than a bare WxH for the null row: width is bus bytes there,
         * so printing it like a pixel count makes the row look like it disagrees with
         * the 160x120 camera preset it is actually matched to. */
        DEMO_LOG(" %2u) %-18s %-8s %u%sx%u  pol pck=%u hs=%u vs=%u  sync=%s",
                 i, s->name, dci_demo_fmt_name(s->fmt), s->width,
                 (s->fmt == DCI_FMT_NONE) ? "B/line" : "", s->height,
                 s->pckpol, s->hspol, s->vspol,
                 (s->sync == DCI_SYNC_EMBEDDED) ? "embedded" : "hw");

        /* Stimulus first: without it a row is not runnable against the camera, and
         * that is the first thing someone picking a row needs to know. */
        if (s->stim != NULL)
        {
            DEMO_LOG("     camera: %s", s->stim);
        }
        else
        {
            DEMO_LOG("     camera: none - config-time row only");
        }

        /* Resolved with the current overlay, so the sizes shown are the sizes this
         * build would actually use. A row the overlay rules out says so here rather
         * than at capture time. */
        if (dci_demo_cfg_resolve(i, &c))
        {
            if (s->fmt == DCI_FMT_JPEG)
            {
                DEMO_LOG("     variable length, cap %u B", (unsigned)c.init_bytes);
            }
            else
            {
                DEMO_LOG("     frame %7u B  (%u B/line x %u lines)",
                         (unsigned)c.frame_bytes, (unsigned)c.line_bytes, c.lines_out);
                if (c.init_bytes != c.frame_bytes)
                {
                    DEMO_LOG("     alloc %7u B  (uncropped: what Capture_Init "
                             "validates)", (unsigned)c.init_bytes);
                }
            }
            DEMO_LOG("     pool 1x %7u B %-3s   2x %7u B %s",
                     (unsigned)dci_demo_cfg_pool_bytes(&c, 1),
                     (dci_demo_cfg_pool_bytes(&c, 1) <= DCI_DEMO_POOL_KB * 1024UL)
                     ? "ok" : "NO",
                     (unsigned)dci_demo_cfg_pool_bytes(&c, 2),
                     (dci_demo_cfg_pool_bytes(&c, 2) <= DCI_DEMO_POOL_KB * 1024UL)
                     ? "ok" : "NO");
        }
        else
        {
            DEMO_LOG("     RULED OUT by the current overlay macros");
        }
        DEMO_LOG("     %s", s->note);
        DEMO_LOG("");
    }
}

/*
 * The complete record of what is about to be programmed, printed before capture
 * starts. Deliberately verbose and deliberately unconditional: the selection is
 * entirely compile-time, so two builds produce identical output apart from this, and a
 * log without it cannot be attributed to a configuration afterwards.
 */
void dci_demo_cfg_print(const dci_demo_cfg_t *c, uint8_t buf_count)
{
    const dci_demo_signal_t *s = c->sig;
    uint32_t pool = dci_demo_cfg_pool_bytes(c, buf_count);

    DEMO_LOG("---------------- configuration ----------------");
    DEMO_LOG(" signal row   : %u  %s", c->index, s->name);
    DEMO_LOG(" stimulus     : %s", (s->stim != NULL) ? s->stim
             : "none (no camera row produces this)");
    DEMO_LOG(" note         : %s", s->note);
    DEMO_LOG("");
    DEMO_LOG(" format       : %s", dci_demo_fmt_name(s->fmt));
    if (s->fmt == DCI_FMT_NONE)
    {
        DEMO_LOG(" size         : %u B/line x %u lines  (no format assumed)",
                 s->width, s->height);
        /* The camera preset is quoted in pixels, so say what those bytes are in pixel
         * terms too - otherwise this row reads as disagreeing with its stimulus. */
        DEMO_LOG("                = %u x %u if read as RGB565/YUV422  (same bytes, "
                 "same pclk - this row just does not claim a format)",
                 s->width / 2u, s->height);
    }
    else
    {
        DEMO_LOG(" size         : %u x %u pixels", s->width, s->height);
    }
    DEMO_LOG(" edm          : %s  (%u byte(s) per pclk)",
             (s->fmt == DCI_FMT_RAW10) ? "10BIT" : "8BIT", c->bytes_per_pclk);
    DEMO_LOG(" pclk/line    : %u  (declared to Capture_Init)", c->pclk_per_line);
    DEMO_LOG(" jpeg         : %s", (s->fmt == DCI_FMT_JPEG) ? "yes" : "no");
    DEMO_LOG("");
    /* "active" in these register bits is the level during BLANKING, so spelling that out
     * is the difference between a reader pairing this with the camera correctly and
     * pairing it exactly backwards. See the note above rows 3..6. */
    DEMO_LOG(" pckpol       : sample on the %s edge",
             c->sig->pckpol ? "rising" : "falling");
    DEMO_LOG(" hspol        : %u - HSYNC %s during blanking", (unsigned)c->sig->hspol,
             c->sig->hspol ? "high" : "low");
    DEMO_LOG(" vspol        : %u - VSYNC %s during blanking", (unsigned)c->sig->vspol,
             c->sig->vspol ? "high" : "low");
    DEMO_LOG(" sync         : %s",
             (s->sync == DCI_SYNC_EMBEDDED) ? "embedded codes" : "hardware pins");
    DEMO_LOG("");
    if (c->crop_en)
    {
        DEMO_LOG(" crop         : on   %u,%u  %u x %u %s",
                 c->crop_x, c->crop_y, c->crop_w, c->crop_h,
                 (s->fmt == DCI_FMT_NONE) ? "bus bytes x lines" : "pixels");
        DEMO_LOG("                registers: hoffcnt=%u vst=%u capcnt=%u vline=%u",
                 c->crop_pclk_x, c->crop_y,
                 (c->crop_pclk_w != 0) ? c->crop_pclk_w - 1 : 0,
                 (c->crop_h != 0) ? c->crop_h - 1 : 0);
        if (c->crop_pclk_w != (uint16_t)(c->crop_w * dci_demo_fmt_pclk_per_pixel(s->fmt)))
        {
            DEMO_LOG("                width aligned down to %u pclk (spec 2.6 "
                     "wants a multiple of 4)", c->crop_pclk_w);
        }
    }
    else
    {
        DEMO_LOG(" crop         : off");
    }
    DEMO_LOG(" frame rate   : %s", dci_demo_fcrc_name(c->fcrc));
    DEMO_LOG(" byte select  : %s%s", dci_demo_bsm_name(c->bsm),
             (c->bsm != DCI_BSM_ALL && c->oebs) ? "  (odd phase)" : "");
    DEMO_LOG(" line select  : %s%s", c->lsm ? "every 2nd line" : "all lines",
             (c->lsm && c->oels) ? "  (odd phase)" : "");
    DEMO_LOG("");
    if (s->fmt == DCI_FMT_JPEG)
    {
        DEMO_LOG(" frame        : variable length, %u B cap", (unsigned)c->init_bytes);
    }
    else
    {
        DEMO_LOG(" frame        : %u B  =  %u B/line x %u lines",
                 (unsigned)c->frame_bytes, (unsigned)c->line_bytes, c->lines_out);
        if (c->init_bytes != c->frame_bytes)
        {
            DEMO_LOG(" allocated    : %u B  (uncropped - Capture_Init validates "
                     "before the window exists)", (unsigned)c->init_bytes);
        }
    }
    DEMO_LOG(" buffers      : %u", buf_count);
    DEMO_LOG(" pool need    : %u B of %u B  (budget %u KB)", (unsigned)pool,
             (unsigned)(DCI_DEMO_POOL_KB * 1024UL),
             (unsigned)DCI_DEMO_RAM_BUDGET_KB);
    DEMO_LOG("-----------------------------------------------");
}
