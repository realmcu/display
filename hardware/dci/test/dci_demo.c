/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DCI demo: entry point, compile-time case selection and shared setup.
 *
 * The switch over DCI_DEMO_CASE is resolved by the preprocessor, so only the selected
 * case is compiled and linked. The alternative - a table of function pointers - would
 * pull every case into the image and, more importantly, would make the framebuffer
 * pool sized for the worst case rather than the chosen one. At a 1 MB budget that
 * difference decides whether the largest presets are runnable at all.
 *
 * The setup sequence (pool, init, crop, geometry cross-check) is shared because it is
 * identical for every capture case, and duplicating it per case is how the copies
 * drift apart.
 */

#include <string.h>
#include "dci_demo.h"
#include "dci_demo_buf.h"
#include "dci_demo_case.h"

/*============================================================================*
 *                         Selected case
 *============================================================================*/
/*
 * One name per case, resolved here so the rest of the file has a single symbol to
 * call and a single string to print. DCI_DEMO_CASE_NEEDS_PRESET and
 * DCI_DEMO_CASE_NEEDS_SIGNAL let the banner state up front whether a camera is
 * required, rather than letting a missing sensor surface as a timeout later.
 *
 * Two cases pin the buffer count because the behaviour under test requires it: a
 * rotation check needs two buffers, and an overwrite check needs one.
 */
#if   (DCI_DEMO_CASE == DCI_DEMO_CASE_REG_TEST)
#define DEMO_NAME               "reg-test"
#define DEMO_ENTRY              dci_demo_case_reg_test
#define DEMO_NEEDS_PRESET       0
#define DEMO_NEEDS_SIGNAL       0

#elif (DCI_DEMO_CASE == DCI_DEMO_CASE_PRESET_DUMP)
#define DEMO_NAME               "preset-dump"
#define DEMO_ENTRY              dci_demo_case_preset_dump
#define DEMO_NEEDS_PRESET       0
#define DEMO_NEEDS_SIGNAL       0

#elif (DCI_DEMO_CASE == DCI_DEMO_CASE_CONFIG_ONLY)
#define DEMO_NAME               "config-only"
#define DEMO_ENTRY              dci_demo_case_config_only
#define DEMO_NEEDS_PRESET       1
#define DEMO_NEEDS_SIGNAL       0

#elif (DCI_DEMO_CASE == DCI_DEMO_CASE_PROBE_SIGNAL)
#define DEMO_NAME               "probe-signal"
#define DEMO_ENTRY              dci_demo_case_probe_signal
#define DEMO_NEEDS_PRESET       1
#define DEMO_NEEDS_SIGNAL       1

#elif (DCI_DEMO_CASE == DCI_DEMO_CASE_SNAPSHOT)
#define DEMO_NAME               "snapshot"
#define DEMO_ENTRY              dci_demo_case_snapshot
#define DEMO_NEEDS_PRESET       1
#define DEMO_NEEDS_SIGNAL       1

#elif (DCI_DEMO_CASE == DCI_DEMO_CASE_SNAPSHOT_X2)
#define DEMO_NAME               "snapshot-x2"
#define DEMO_ENTRY              dci_demo_case_snapshot_x2
#define DEMO_NEEDS_PRESET       1
#define DEMO_NEEDS_SIGNAL       1
#define DEMO_FORCE_BUF_COUNT    2       /* rotation cannot be observed with one */

#elif (DCI_DEMO_CASE == DCI_DEMO_CASE_CONTINUOUS)
#define DEMO_NAME               "continuous"
#define DEMO_ENTRY              dci_demo_case_continuous
#define DEMO_NEEDS_PRESET       1
#define DEMO_NEEDS_SIGNAL       1

#elif (DCI_DEMO_CASE == DCI_DEMO_CASE_CONTINUOUS_1BUF)
#define DEMO_NAME               "continuous-1buf"
#define DEMO_ENTRY              dci_demo_case_continuous_1buf
#define DEMO_NEEDS_PRESET       1
#define DEMO_NEEDS_SIGNAL       1
#define DEMO_FORCE_BUF_COUNT    1       /* the point is single-buffer overwrite */

#elif (DCI_DEMO_CASE == DCI_DEMO_CASE_RECONFIG)
#define DEMO_NAME               "reconfig"
#define DEMO_ENTRY              dci_demo_case_reconfig
#define DEMO_NEEDS_PRESET       1
#define DEMO_NEEDS_SIGNAL       1

#elif (DCI_DEMO_CASE == DCI_DEMO_CASE_SUSPEND)
#define DEMO_NAME               "suspend"
#define DEMO_ENTRY              dci_demo_case_suspend
#define DEMO_NEEDS_PRESET       1
#define DEMO_NEEDS_SIGNAL       1

#elif (DCI_DEMO_CASE == DCI_DEMO_CASE_STRESS)
#define DEMO_NAME               "stress"
#define DEMO_ENTRY              dci_demo_case_stress
#define DEMO_NEEDS_PRESET       1
#define DEMO_NEEDS_SIGNAL       1

#elif (DCI_DEMO_CASE == DCI_DEMO_CASE_RAW10)
#define DEMO_NAME               "raw10"
#define DEMO_ENTRY              dci_demo_case_raw10
#define DEMO_NEEDS_PRESET       1
#define DEMO_NEEDS_SIGNAL       1
/* Forced to 1: the case takes a single snapshot, and VGA_RAW10 is 600 KB - at two
 * buffers the largest 10-bit preset would not fit the pool, and this is the case most
 * likely to be pointed at it. */
#define DEMO_FORCE_BUF_COUNT    1
/* The 10-bit rows are the only ones where the config's default preset (an 8-bit row)
 * would make the case refuse to run, so catch it at compile time instead of after a
 * flash cycle. */
#if (DCI_DEMO_PRESET != DCI_DEMO_PRESET_QVGA_RAW10) &&  \
    (DCI_DEMO_PRESET != DCI_DEMO_PRESET_RAW10_CROP) &&  \
    (DCI_DEMO_PRESET != DCI_DEMO_PRESET_VGA_RAW10)  &&  \
    (DCI_DEMO_PRESET != DCI_DEMO_PRESET_RAW10_LSM)
#error "DCI_DEMO_CASE_RAW10 needs a RAW10 preset: QVGA_RAW10, RAW10_CROP, VGA_RAW10 or RAW10_LSM"
#endif

#else
#error "DCI_DEMO_CASE selects no known case - see dci_demo_id.h"
#endif

#ifndef DEMO_FORCE_BUF_COUNT
#define DEMO_FORCE_BUF_COUNT    DCI_DEMO_BUF_COUNT
#endif

/*
 * Resolved here rather than with an #if inside the DEMO_LOG() call: a preprocessor
 * directive between a macro's parentheses is undefined behaviour (C99 6.10.3p11).
 * GCC happens to accept it, so a build sweep does not catch it - cppcheck does.
 */
#if (DEMO_FORCE_BUF_COUNT != DCI_DEMO_BUF_COUNT)
#define DEMO_BUF_FORCED_NOTE    "(forced by this case)"
#else
#define DEMO_BUF_FORCED_NOTE    ""
#endif

/*============================================================================*
 *                         Result names
 *============================================================================*/
const char *dci_demo_result_name(dci_demo_result_t r)
{
    static const char *const names[] =
    {
        "PASS", "BAD_PRESET", "REG_DEAD", "REG_BITS", "POOL", "INIT", "GEOMETRY",
        "CROP", "START", "TIMEOUT", "FRAME_LEN", "TRUNCATED", "GUARD", "CONTENT",
        "SEQ", "STATS", "NO_VSYNC", "LINE_COUNT", "STATE",
    };

    return (r < sizeof(names) / sizeof(names[0])) ? names[r] : "?";
}

/*============================================================================*
 *                         Buffer count
 *============================================================================*/
uint8_t dci_demo_buf_count_cfg(void)
{
    return DEMO_FORCE_BUF_COUNT;
}

/*============================================================================*
 *                         Shared setup
 *============================================================================*/
/*
 * Everything a capture case needs before it can start.
 *
 * The geometry cross-check is the reason this is one function rather than a few lines
 * per case. The demo computes the frame size from the preset and the driver computes
 * it from DCI_InitTypeDef; if those disagree, one of the two formulas is wrong and
 * every later symptom - short frames, overruns, desync - is a consequence. Catching
 * it here turns a day of guessing into one line of output.
 */
dci_demo_result_t dci_demo_setup(const dci_demo_preset_t *p, uint8_t buf_count,
                                 DCI_ON_BUSY_T on_busy, DCI_NOTIFY_MODE_T notify,
                                 uint32_t *frame_bytes_out)
{
    DCI_InitTypeDef  init;
    DCI_DMA_config   dma;
    DCI_frame_info_t info;
    uint32_t         expect;
    uint32_t         buf_bytes;
    DCI_ERROR        err;
    uint8_t          i;

    expect = dci_demo_preset_frame_bytes(p);

    /*
     * Size the buffer for what Capture_Init() will validate, not for what will finally
     * be delivered. Those differ when the preset crops - the window is only programmed
     * afterwards, so Init still measures against the full frame - and for JPEG, where
     * the buffer is a capacity cap rather than a frame size.
     */
    buf_bytes = dci_demo_preset_init_bytes(p);

    if (!dci_demo_buf_alloc(buf_bytes, buf_count))
    {
        return DEMO_FAIL_POOL;
    }

    dci_demo_preset_to_init(p, &init);

    memset(&dma, 0, sizeof(dma));
    dma.dma_channel_num = DCI_DMA_CH_AUTO;
    dma.dma_priority    = 0;
    dma.buf_count       = (DCI_BUF_COUNT_T)buf_count;
    dma.buf_bytes       = dci_demo_buf_bytes();
    dma.on_busy         = on_busy;
    dma.notify_mode     = notify;
    dma.frame_error     = dci_demo_on_frame_error;
    if (notify == DCI_NOTIFY_CALLBACK)
    {
        dma.frame_ready = dci_demo_on_frame_ready;
    }
    for (i = 0; i < buf_count; i++)
    {
        dma.buf[i] = dci_demo_buf_get(i);
    }

    err = DCI_Capture_Init(&init, &dma);
    if (err != DCI_SUCCESS)
    {
        DEMO_LOG("  Capture_Init failed: %d", err);
        return DEMO_FAIL_INIT;
    }

    /* Crop must follow Init: Capture_Init() does not take a window. */
    if (p->crop_en)
    {
        uint8_t  ppp = dci_demo_fmt_pclk_per_pixel(p->fmt);
        uint16_t x   = p->crop_x * ppp;
        uint16_t w   = p->crop_w * ppp;

        err = DCI_ConfigCrop(x, p->crop_y, w, p->crop_h);
        if (err != DCI_SUCCESS)
        {
            DEMO_LOG("  ConfigCrop(%u,%u,%u,%u) failed: %d", x, p->crop_y, w,
                     p->crop_h, err);
            return DEMO_FAIL_CROP;
        }
        err = DCI_CropCmd(ENABLE);
        if (err != DCI_SUCCESS)
        {
            DEMO_LOG("  CropCmd failed: %d", err);
            return DEMO_FAIL_CROP;
        }
    }

    if (DCI_Capture_GetFrameInfo(&info) != DCI_SUCCESS)
    {
        return DEMO_FAIL_INIT;
    }

    DEMO_LOG("  geometry: %u B/frame, %u B/line, %u lines, %u block(s) x %u words%s",
             info.frame_bytes, info.line_bytes, info.lines_out,
             info.chunks_per_frame, info.chunk_words,
             info.line_aligned ? "" : "  (word-aligned split)");

    if (!info.var_len && info.frame_bytes != expect)
    {
        DEMO_LOG("  GEOMETRY MISMATCH: driver says %u B, preset implies %u B",
                 info.frame_bytes, expect);
        DEMO_LOG("  -> one of the two formulas is wrong; fix before trusting a capture");
        return DEMO_FAIL_GEOMETRY;
    }

    if (frame_bytes_out != NULL)
    {
        *frame_bytes_out = info.var_len ? 0 : info.frame_bytes;
    }

    dci_demo_buf_poison();
    return DEMO_PASS;
}

/*
 * Leaves the peripheral idle so a reset-less rerun starts from a known state rather
 * than inheriting a running ring.
 */
void dci_demo_teardown(void)
{
    DCI_Capture_Abort();
}

/*
 * Every counter that should be zero after a clean run, checked as a group. A single
 * non-zero counter is the cheapest early warning available: an image can look right
 * while suspend_count climbs into the hundreds.
 */
dci_demo_result_t dci_demo_check_stats(uint32_t expect_frames)
{
    DCI_stats_t s;

    DCI_GetStats(&s);

    DEMO_LOG("[STATS] done=%u dropped=%u chunks=%u ovr=%u err=%u resync=%u "
             "suspend=%u ovf=%u seq=%u",
             s.frames_done, s.frames_dropped, s.chunks_done, s.ovr_count,
             s.err_count, s.resync_count, s.suspend_count, s.buf_overflow, s.seq);

    if (s.frames_done != expect_frames)
    {
        DEMO_LOG("  frames_done %u, expected %u", s.frames_done, expect_frames);
        return DEMO_FAIL_STATS;
    }
    if (s.ovr_count || s.err_count || s.resync_count || s.buf_overflow)
    {
        DEMO_LOG("  error counters are not clean");
        return DEMO_FAIL_STATS;
    }
    return DEMO_PASS;
}

/*============================================================================*
 *                         Entry point
 *============================================================================*/
/*
 * Prints the build's configuration before running. That banner is the record of what
 * was actually flashed, which matters precisely because the selection is compile
 * time: log output from a previous build is otherwise indistinguishable.
 */
static void dci_demo_banner(const dci_demo_preset_t *p)
{
    DEMO_LOG("");
    DEMO_LOG("================ DCI demo ================");
    DEMO_LOG(" case   : %d  %s%s", DCI_DEMO_CASE, DEMO_NAME,
             DEMO_NEEDS_SIGNAL ? "   [needs a live camera]" : "   [no camera needed]");

    if (p != NULL)
    {
        uint32_t pool = dci_demo_preset_pool_bytes(p, DEMO_FORCE_BUF_COUNT);

        DEMO_LOG(" preset : %d  %s", DCI_DEMO_PRESET, p->name);
        DEMO_LOG("          %s %ux%u, %u B/frame",
                 dci_demo_fmt_name(p->fmt),
                 p->crop_en ? p->crop_w : p->width,
                 p->crop_en ? p->crop_h : p->height,
                 (unsigned)dci_demo_preset_frame_bytes(p));
        DEMO_LOG(" buffers: %d %s", DEMO_FORCE_BUF_COUNT, DEMO_BUF_FORCED_NOTE);
        DEMO_LOG(" RAM    : %u B needed, %u B pool, budget %u KB",
                 (unsigned)pool, (unsigned)(DCI_DEMO_POOL_KB * 1024UL),
                 (unsigned)DCI_DEMO_RAM_BUDGET_KB);
        DEMO_LOG("          %s", p->note);
    }
    DEMO_LOG("==========================================");
}

dci_demo_result_t dci_demo_run(void)
{
    const dci_demo_preset_t *p = NULL;
    dci_demo_result_t        r;

#if (DEMO_NEEDS_PRESET == 1)
    p = dci_demo_preset_get(DCI_DEMO_PRESET);
    if (p == NULL)
    {
        DEMO_LOG("[DEMO] preset %d does not exist (have %u)",
                 DCI_DEMO_PRESET, dci_demo_preset_count());
        return DEMO_FAIL_BAD_PRESET;
    }
#endif

    dci_demo_banner(p);

    r = DEMO_ENTRY(p);

    DEMO_LOG("");
    if (r == DEMO_PASS)
    {
        DEMO_LOG("======== %s: PASS ========", DEMO_NAME);
    }
    else
    {
        DEMO_LOG("======== %s: FAIL (%s) ========", DEMO_NAME,
                 dci_demo_result_name(r));
        /* The register state at the moment of failure is the diagnostic; print it
         * before anything else can overwrite it. */
#if (DCI_DEBUG_REG_DUMP == 1)
        DCI_RegDump();
#else
        DEMO_LOG("    rebuild with DCI_DEBUG_REG_DUMP=1 for a register dump");
#endif
    }

#if (DEMO_NEEDS_PRESET == 1)
    dci_demo_teardown();
#endif
    return r;
}
