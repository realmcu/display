/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DCI demo: the test cases.
 *
 * Each case is a linear script - readable top to bottom, breakpoint-friendly on any
 * line - built from the shared setup in dci_demo.c plus whatever assertions are
 * specific to that case. Nothing here duplicates the setup sequence.
 *
 * All capture cases use POLL notification: the driver's WaitFrame() is a bounded
 * busy-wait designed for exactly this, no OS required. The callback path is exercised
 * by the one case that opts into it.
 *
 * Every case is wrapped in DEMO_CASE_IS(), so only the one selected in
 * dci_demo_config.h is compiled. That keeps an unfinished case from breaking the build
 * of the one under test, and keeps the image down to what is actually being run.
 */

#include <string.h>
#include "dci_demo.h"
#include "dci_demo_buf.h"
#include "dci_demo_case.h"

/** \brief  True when the config selected this case. */
#define DEMO_CASE_IS(c)     (DCI_DEMO_CASE == (c))

/*
 * The shared helpers below are only referenced by the capture cases. Compiling them
 * for a case that never calls them would produce unused-function warnings on every
 * build, so they are gated on "is this a capture case" rather than left unconditional.
 */
#define DEMO_IS_CAPTURE_CASE                    \
    (DEMO_CASE_IS(DCI_DEMO_CASE_SNAPSHOT)    || \
     DEMO_CASE_IS(DCI_DEMO_CASE_SNAPSHOT_X2) || \
     DEMO_CASE_IS(DCI_DEMO_CASE_CONTINUOUS)  || \
     DEMO_CASE_IS(DCI_DEMO_CASE_CONTINUOUS_1BUF) || \
     DEMO_CASE_IS(DCI_DEMO_CASE_RECONFIG)    || \
     DEMO_CASE_IS(DCI_DEMO_CASE_SUSPEND)     || \
     DEMO_CASE_IS(DCI_DEMO_CASE_STRESS)      || \
     DEMO_CASE_IS(DCI_DEMO_CASE_RAW10))

/* demo_check_frame() validates a delivered frame, which the two cases that only count
 * callbacks or only reconfigure never do. */
#define DEMO_NEEDS_FRAME_CHECK                  \
    (DEMO_CASE_IS(DCI_DEMO_CASE_SNAPSHOT)    || \
     DEMO_CASE_IS(DCI_DEMO_CASE_SNAPSHOT_X2) || \
     DEMO_CASE_IS(DCI_DEMO_CASE_CONTINUOUS)  || \
     DEMO_CASE_IS(DCI_DEMO_CASE_SUSPEND))

/*
 * DCI_DEMO_DUMP_LEVEL applies to the cases that stop between frames. A streaming case
 * pins it to 0 no matter what the config says: a 424 KB frame is roughly 7 s of hex at
 * 1 Mbps, and printing that between frames causes the very overruns the case exists to
 * detect. Naming the two levels here keeps that reasoning in one place instead of
 * appearing as an unexplained literal at each call site.
 */
#define DEMO_DUMP_ONESHOT       DCI_DEMO_DUMP_LEVEL
#define DEMO_DUMP_STREAM        0

/*============================================================================*
 *                         Callbacks
 *============================================================================*/
/*
 * ISR context. Records and returns - no printing, because a UART write at pixel rate
 * would overrun the FIFO and the resulting OVR would be blamed on the hardware.
 */
static volatile uint32_t demo_cb_frames;
static volatile uint32_t demo_cb_last_seq;
static volatile uint32_t demo_err_count;
static volatile DCI_ERROR demo_last_err;
static volatile uint32_t demo_last_ris;

void dci_demo_on_frame_ready(const DCI_frame_t *frame)
{
    demo_cb_frames++;
    demo_cb_last_seq = frame->seq;
    /* Release immediately: this is the "hand it straight on and free it" pattern the
     * driver explicitly supports from ISR context (DESIGN.md 11.5). */
    DCI_Capture_ReleaseBuffer(frame->index);
}

void dci_demo_on_frame_error(DCI_ERROR err, uint32_t ris)
{
    demo_err_count++;
    demo_last_err = err;
    demo_last_ris = ris;
}

#if DEMO_IS_CAPTURE_CASE
static void demo_cb_reset(void)
{
    demo_cb_frames   = 0;
    demo_cb_last_seq = 0;
    demo_err_count   = 0;
    demo_last_err    = DCI_SUCCESS;
    demo_last_ris    = 0;
}

/*
 * Errors are latched rather than acted on, so a case can distinguish "no frame
 * because nothing arrived" from "no frame because the driver rejected the frame that
 * did arrive" - two very different investigations.
 */
static void demo_report_errors(void)
{
    if (demo_err_count != 0)
    {
        DEMO_LOG("  frame_error fired %u time(s), last err=%d ris=0x%08X",
                 demo_err_count, demo_last_err, demo_last_ris);
    }
}
#endif /* DEMO_IS_CAPTURE_CASE */

/*============================================================================*
 *                         Frame validation
 *============================================================================*/
/*
 * Everything checkable about a delivered frame without knowing what the camera was
 * pointed at. Content-independent by design: a real sensor looking at a real scene
 * has no golden data to compare against, so the assertions have to be structural.
 */
#if DEMO_NEEDS_FRAME_CHECK
static dci_demo_result_t demo_check_frame(const DCI_frame_t *f,
                                          const dci_demo_preset_t *p,
                                          uint32_t expect_bytes)
{
    if (expect_bytes != 0)
    {
        if (f->bytes != expect_bytes)
        {
            DEMO_LOG("  length %u, expected %u", f->bytes, expect_bytes);
            return DEMO_FAIL_FRAME_LEN;
        }
        if (f->truncated)
        {
            DEMO_LOG("  frame flagged truncated");
            return DEMO_FAIL_TRUNCATED;
        }
    }
    else
    {
        /* JPEG: any length up to the cap is legal, so only sanity applies. */
        if (f->bytes == 0 || f->bytes > dci_demo_buf_bytes())
        {
            DEMO_LOG("  JPEG length %u outside 1..%u", f->bytes,
                     dci_demo_buf_bytes());
            return DEMO_FAIL_FRAME_LEN;
        }
        if (!dci_demo_buf_jpeg_ok(f))
        {
            return DEMO_FAIL_CONTENT;
        }
    }

    if (!dci_demo_buf_guards_ok())
    {
        return DEMO_FAIL_GUARD;
    }
    if (!dci_demo_buf_has_data(f->index, f->bytes))
    {
        return DEMO_FAIL_CONTENT;
    }
    return DEMO_PASS;
}
#endif /* DEMO_NEEDS_FRAME_CHECK */

/*============================================================================*
 *                         Case: register self test
 *============================================================================*/
/*
 * First case for a reason: it needs no camera, no DMA and no pins, so it separates
 * "the peripheral is not reachable" from "the peripheral is reachable but the capture
 * is wrong". Those two have completely different causes.
 */
#if DEMO_CASE_IS(DCI_DEMO_CASE_REG_TEST)
dci_demo_result_t dci_demo_case_reg_test(const dci_demo_preset_t *p)
{
    DCI_ERROR err;

    (void)p;

#if (DCI_DEBUG_REG_DUMP == 1)
    err = DCI_RegTestDump();        /* prints the per-register report */
#else
    err = DCI_RegTest(NULL);
    DEMO_LOG("  (build with DCI_DEBUG_REG_DUMP=1 for the per-register report)");
#endif

    switch (err)
    {
    case DCI_SUCCESS:
        return DEMO_PASS;

    case DCI_ERROR_NO_RESPONSE:
        DEMO_LOG("  register block did not respond. In order of likelihood:");
        DEMO_LOG("   1. DCI clock still gated - see rtl_dci_clock_enable() TODO(HAL)");
        DEMO_LOG("   2. wrong base address");
        DEMO_LOG("   3. display island powered down");
        return DEMO_FAIL_REG_DEAD;

    default:
        DEMO_LOG("  register block responds but some bits misbehave");
        DEMO_LOG("   check the writable-bit masks against the register table");
        return DEMO_FAIL_REG_BITS;
    }
}
#endif

/*============================================================================*
 *                         Case: preset table dump
 *============================================================================*/
/*
 * Pure software. Prints what every preset derives to, so a bad pclk ratio or an
 * impossible frame size is visible before any hardware is involved.
 */
#if DEMO_CASE_IS(DCI_DEMO_CASE_PRESET_DUMP)
dci_demo_result_t dci_demo_case_preset_dump(const dci_demo_preset_t *unused)
{
    uint8_t i;

    (void)unused;

    dci_demo_preset_list();

    DEMO_LOG("");
    DEMO_LOG("--- derived values ---");
    DEMO_LOG("  # pclk/line lines   frame B  buf B  fits %u KB pool?",
             (unsigned)DCI_DEMO_POOL_KB);
    DEMO_LOG("  (buf B > frame B means the preset crops: Init validates uncropped)");

    for (i = 0; i < dci_demo_preset_count(); i++)
    {
        const dci_demo_preset_t *p = dci_demo_preset_get(i);
        DCI_InitTypeDef          init;
        uint32_t                 bytes = dci_demo_preset_frame_bytes(p);
        /* A cropping preset has to allocate for the uncropped frame - see
         * dci_demo_preset_init_bytes(). */
        uint32_t                 need  = (p->fmt == DCI_FMT_JPEG)
                                         ? (DCI_DEMO_JPEG_CAP_KB * 1024UL)
                                         : dci_demo_preset_init_bytes(p);

        dci_demo_preset_to_init(p, &init);

        DEMO_LOG(" %2u %9u %5u %9u %8u  1x:%-3s 2x:%-3s", i,
                 init.sensor_pclk_per_line, init.sensor_lines, (unsigned)bytes,
                 (unsigned)need,
                 (need <= DCI_DEMO_POOL_KB * 1024UL) ? "yes" : "NO",
                 (2 * need <= DCI_DEMO_POOL_KB * 1024UL) ? "yes" : "NO");
    }
    return DEMO_PASS;
}
#endif

/*============================================================================*
 *                         Case: configure only
 *============================================================================*/
/*
 * Applies a preset and stops. Proves the configuration is accepted and the derived
 * geometry agrees, without needing a camera - which makes it the last purely local
 * check before a live signal is required.
 */
#if DEMO_CASE_IS(DCI_DEMO_CASE_CONFIG_ONLY)
dci_demo_result_t dci_demo_case_config_only(const dci_demo_preset_t *p)
{
    dci_demo_result_t r;
    uint32_t          frame_bytes;

    r = dci_demo_setup(p, dci_demo_buf_count_cfg(), DCI_ON_BUSY_GATE,
                       DCI_NOTIFY_POLL, &frame_bytes);
    if (r != DEMO_PASS)
    {
        return r;
    }

    if (DCI_Capture_GetState() != DCI_STATE_IDLE)
    {
        DEMO_LOG("  state %d after Init, expected IDLE", DCI_Capture_GetState());
        return DEMO_FAIL_STATE;
    }

#if (DCI_DEBUG_REG_DUMP == 1)
    DCI_RegDump();
#endif
    DEMO_LOG("  configuration accepted; no capture attempted");
    return DEMO_PASS;
}
#endif

/*============================================================================*
 *                         Case: probe the signal
 *============================================================================*/
/*
 * Counts VSYNC and LINE interrupts without arming the DMA, so it measures what the
 * sensor actually emits rather than what the preset claims.
 *
 * This catches the most common integration fault there is - a preset that says 240
 * lines against a sensor configured for 480 - and it catches it as one line of
 * output instead of as a stream of FRAME_SHORT errors whose cause has to be inferred.
 */
#if DEMO_CASE_IS(DCI_DEMO_CASE_PROBE_SIGNAL)

static volatile uint32_t demo_vsync_count;
static volatile uint32_t demo_line_count;
static volatile uint32_t demo_lines_measured;

static void demo_on_vsync(void)
{
    /* Skip the first VSYNC: capture may have begun mid-frame, so its line count is
     * a partial frame and would read low. */
    if (demo_vsync_count != 0)
    {
        demo_lines_measured = demo_line_count;
    }
    demo_line_count = 0;
    demo_vsync_count++;
}

static void demo_on_line(void)
{
    demo_line_count++;
}

dci_demo_result_t dci_demo_case_probe_signal(const dci_demo_preset_t *p)
{
    DCI_InitTypeDef init;
    DCI_DMA_config  dma;
    uint32_t        spin;
    uint16_t        expect_lines;

    demo_vsync_count    = 0;
    demo_line_count     = 0;
    demo_lines_measured = 0;

    /* A buffer is still required by Capture_Init even though nothing will be written
     * into it - one word is enough. */
    if (!dci_demo_buf_alloc(4, 1))
    {
        return DEMO_FAIL_POOL;
    }

    dci_demo_preset_to_init(p, &init);

    memset(&dma, 0, sizeof(dma));
    dma.dma_channel_num = DCI_DMA_CH_AUTO;
    dma.buf_count       = DCI_BUF_SINGLE;
    dma.buf[0]          = dci_demo_buf_get(0);
    dma.buf_bytes       = 4;
    dma.notify_mode     = DCI_NOTIFY_POLL;
    dma.vsync_event     = demo_on_vsync;
    dma.line_event      = demo_on_line;   /* enabling LINE is the whole point here */

    /* buf_bytes is deliberately smaller than a frame, which Capture_Init would
     * normally reject - so declare a geometry of one line to keep it consistent. */
    init.sensor_lines         = 1;
    init.sensor_pclk_per_line = 2;

    if (DCI_Capture_Init(&init, &dma) != DCI_SUCCESS)
    {
        DEMO_LOG("  probe Init failed");
        return DEMO_FAIL_INIT;
    }

    /* CR.ENABLE on, CAPTURE off: the pins are sampled and the interrupts fire, but no
     * DMA request is ever raised, so nothing is written anywhere. */
    DCI_Cmd(ENABLE);

    for (spin = 0; spin < 40000000UL && demo_vsync_count < 4; spin++)
    {
        __asm volatile("" ::: "memory");
    }

    DCI_Cmd(DISABLE);

    expect_lines = p->crop_en ? p->crop_h : p->height;
    if (p->lsm)
    {
        expect_lines = p->oels ? (expect_lines / 2) : ((expect_lines + 1) / 2);
    }

    DEMO_LOG("  vsync=%u  lines/frame measured=%u  preset declares=%u",
             demo_vsync_count, demo_lines_measured, expect_lines);

    if (demo_vsync_count == 0)
    {
        DEMO_LOG("  no VSYNC at all. Check, in order:");
        DEMO_LOG("   1. sensor powered and clocked (is PIXCLK running?)");
        DEMO_LOG("   2. VSYNC pin wired and muxed - see rtl_dci_pinmux_init() TODO(HAL)");
        DEMO_LOG("   3. vspol matches the sensor");
        return DEMO_FAIL_NO_VSYNC;
    }
    if (demo_lines_measured == 0)
    {
        DEMO_LOG("  VSYNC present but no LINE: check HSYNC wiring and hspol");
        return DEMO_FAIL_LINE_COUNT;
    }
    if (demo_lines_measured != expect_lines)
    {
        DEMO_LOG("  LINE COUNT MISMATCH - the sensor is not emitting what the preset");
        DEMO_LOG("  declares. Fix this first: every capture will desync otherwise.");
        DEMO_LOG("   sensor really emits %u lines; either reconfigure the sensor or",
                 demo_lines_measured);
        DEMO_LOG("   pick/add a preset with height=%u", demo_lines_measured);
        return DEMO_FAIL_LINE_COUNT;
    }

    DEMO_LOG("  geometry confirmed against the real signal");
    return DEMO_PASS;
}
#endif

/*============================================================================*
 *                         Case: snapshot
 *============================================================================*/
#if DEMO_CASE_IS(DCI_DEMO_CASE_SNAPSHOT)
dci_demo_result_t dci_demo_case_snapshot(const dci_demo_preset_t *p)
{
    DCI_frame_t       f;
    dci_demo_result_t r;
    uint32_t          expect;
    DCI_ERROR         err;

    demo_cb_reset();

    r = dci_demo_setup(p, dci_demo_buf_count_cfg(), DCI_ON_BUSY_GATE,
                       DCI_NOTIFY_POLL, &expect);
    if (r != DEMO_PASS)
    {
        return r;
    }

    err = DCI_Capture_Start(DCI_MODE_SNAPSHOT);
    if (err != DCI_SUCCESS)
    {
        DEMO_LOG("  Start failed: %d", err);
        return DEMO_FAIL_START;
    }

    err = DCI_Capture_WaitFrame(&f, DCI_DEMO_TIMEOUT_MS);
    if (err != DCI_SUCCESS)
    {
        DEMO_LOG("  no frame in %u ms: %d", DCI_DEMO_TIMEOUT_MS, err);
        demo_report_errors();
        return DEMO_FAIL_TIMEOUT;
    }

    r = demo_check_frame(&f, p, expect);
    dci_demo_buf_dump(&f, p, DEMO_DUMP_ONESHOT);
    DCI_Capture_ReleaseBuffer(f.index);

    if (r != DEMO_PASS)
    {
        demo_report_errors();
        return r;
    }

    /* CAPTURE self-clears after a snapshot frame, so the driver must be back to IDLE
     * without anyone stopping it (spec 2.5.1). */
    if (DCI_Capture_GetState() != DCI_STATE_IDLE)
    {
        DEMO_LOG("  state %d after snapshot, expected IDLE", DCI_Capture_GetState());
        return DEMO_FAIL_STATE;
    }

    return dci_demo_check_stats(1);
}
#endif

/*============================================================================*
 *                         Case: two snapshots, double buffered
 *============================================================================*/
/*
 * The driver rotates the target buffer between shots so an application can process
 * shot N while shot N+1 is being taken. Verified by holding the first buffer while
 * the second shot runs - if rotation were broken, the second shot would either
 * overwrite the held buffer or fail with BUSY.
 */
#if DEMO_CASE_IS(DCI_DEMO_CASE_SNAPSHOT_X2)
dci_demo_result_t dci_demo_case_snapshot_x2(const dci_demo_preset_t *p)
{
    DCI_frame_t       f1, f2;
    dci_demo_result_t r;
    uint32_t          expect;

    demo_cb_reset();

    r = dci_demo_setup(p, 2, DCI_ON_BUSY_GATE, DCI_NOTIFY_POLL, &expect);
    if (r != DEMO_PASS)
    {
        return r;
    }

    if (DCI_Capture_Start(DCI_MODE_SNAPSHOT) != DCI_SUCCESS)
    {
        return DEMO_FAIL_START;
    }
    if (DCI_Capture_WaitFrame(&f1, DCI_DEMO_TIMEOUT_MS) != DCI_SUCCESS)
    {
        demo_report_errors();
        return DEMO_FAIL_TIMEOUT;
    }
    r = demo_check_frame(&f1, p, expect);
    if (r != DEMO_PASS)
    {
        return r;
    }
    DEMO_LOG("  shot 1 -> buffer %u, seq %u", f1.index, f1.seq);

    /* Deliberately do NOT release f1: the second shot must go elsewhere. */
    if (DCI_Capture_Start(DCI_MODE_SNAPSHOT) != DCI_SUCCESS)
    {
        DEMO_LOG("  second Start failed while buffer %u held - rotation broken",
                 f1.index);
        return DEMO_FAIL_START;
    }
    if (DCI_Capture_WaitFrame(&f2, DCI_DEMO_TIMEOUT_MS) != DCI_SUCCESS)
    {
        demo_report_errors();
        return DEMO_FAIL_TIMEOUT;
    }
    r = demo_check_frame(&f2, p, expect);
    if (r != DEMO_PASS)
    {
        return r;
    }
    DEMO_LOG("  shot 2 -> buffer %u, seq %u", f2.index, f2.seq);

    if (f2.index == f1.index)
    {
        DEMO_LOG("  both shots landed in buffer %u: no rotation", f1.index);
        return DEMO_FAIL_SEQ;
    }
    if (f2.seq != f1.seq + 1)
    {
        DEMO_LOG("  seq %u -> %u, expected consecutive", f1.seq, f2.seq);
        return DEMO_FAIL_SEQ;
    }

    dci_demo_buf_dump(&f2, p, DEMO_DUMP_ONESHOT);
    DCI_Capture_ReleaseBuffer(f1.index);
    DCI_Capture_ReleaseBuffer(f2.index);

    return dci_demo_check_stats(2);
}
#endif

/*============================================================================*
 *                         Case: continuous
 *============================================================================*/
/*
 * The one case that exercises the closed descriptor ring, the FRAME/block rendezvous
 * and the gating back-pressure - none of which the snapshot path uses at all.
 *
 * Two assertions matter beyond the per-frame ones: seq must not gap (a gap means a
 * frame was delivered and never collected), and consecutive frames must differ (equal
 * frames mean the DMA is replaying one buffer rather than advancing).
 */
#if DEMO_CASE_IS(DCI_DEMO_CASE_CONTINUOUS)
dci_demo_result_t dci_demo_case_continuous(const dci_demo_preset_t *p)
{
    DCI_frame_t       f;
    dci_demo_result_t r;
    uint32_t          expect;
    uint32_t          prev_seq = 0;
    uint32_t          prev_crc = 0;
    uint32_t          got = 0;
    uint8_t           prev_index = 0xFF;

    demo_cb_reset();

    r = dci_demo_setup(p, dci_demo_buf_count_cfg(), DCI_ON_BUSY_GATE,
                       DCI_NOTIFY_POLL, &expect);
    if (r != DEMO_PASS)
    {
        return r;
    }

    if (DCI_Capture_Start(DCI_MODE_CONTINUOUS) != DCI_SUCCESS)
    {
        return DEMO_FAIL_START;
    }

    while (got < DCI_DEMO_FRAMES)
    {
        if (DCI_Capture_WaitFrame(&f, DCI_DEMO_TIMEOUT_MS) != DCI_SUCCESS)
        {
            DEMO_LOG("  timeout after %u frame(s)", got);
            demo_report_errors();
            return DEMO_FAIL_TIMEOUT;
        }

        r = demo_check_frame(&f, p, expect);
        if (r != DEMO_PASS)
        {
            DCI_Capture_ReleaseBuffer(f.index);
            return r;
        }

        if (got != 0)
        {
            if (f.seq != prev_seq + 1)
            {
                DEMO_LOG("  seq gap: %u -> %u. A frame was delivered but never "
                         "collected (see DESIGN.md 11.1)", prev_seq, f.seq);
                DCI_Capture_ReleaseBuffer(f.index);
                return DEMO_FAIL_SEQ;
            }
            if (f.index == prev_index && dci_demo_buf_count() > 1)
            {
                DEMO_LOG("  buffer %u twice in a row: rotation stalled", f.index);
                DCI_Capture_ReleaseBuffer(f.index);
                return DEMO_FAIL_SEQ;
            }
        }

        dci_demo_buf_dump(&f, p, DEMO_DUMP_STREAM);

        prev_seq   = f.seq;
        prev_index = f.index;
        (void)prev_crc;
        got++;

        DCI_Capture_ReleaseBuffer(f.index);
    }

    DCI_Capture_Stop();
    DEMO_LOG("  collected %u frames, seq %u..%u", got, prev_seq - got + 1, prev_seq);

    return dci_demo_check_stats(DCI_DEMO_FRAMES);
}
#endif

/*============================================================================*
 *                         Case: continuous, single buffer, overwrite
 *============================================================================*/
/*
 * Viewfinder configuration: one buffer, never gate, accept tearing. Uses the callback
 * path rather than POLL, which is what a real viewfinder would do - hand the buffer
 * straight to the display and release it in the ISR.
 *
 * seq gaps are expected here, not a failure: with one buffer and OVERWRITE the driver
 * keeps writing whether or not anyone collected the previous frame.
 */
#if DEMO_CASE_IS(DCI_DEMO_CASE_CONTINUOUS_1BUF)
dci_demo_result_t dci_demo_case_continuous_1buf(const dci_demo_preset_t *p)
{
    dci_demo_result_t r;
    uint32_t          expect;
    uint32_t          spin;

    demo_cb_reset();

    r = dci_demo_setup(p, 1, DCI_ON_BUSY_OVERWRITE, DCI_NOTIFY_CALLBACK, &expect);
    if (r != DEMO_PASS)
    {
        return r;
    }

    if (DCI_Capture_Start(DCI_MODE_CONTINUOUS) != DCI_SUCCESS)
    {
        return DEMO_FAIL_START;
    }

    /* The callback releases each buffer as it arrives, so nothing to do but wait. */
    for (spin = 0; spin < 60000000UL && demo_cb_frames < DCI_DEMO_FRAMES; spin++)
    {
        __asm volatile("" ::: "memory");
    }

    DCI_Capture_Stop();

    DEMO_LOG("  callback fired %u time(s), last seq %u", demo_cb_frames,
             demo_cb_last_seq);
    demo_report_errors();

    if (demo_cb_frames == 0)
    {
        DEMO_LOG("  no callback: nothing was captured");
        return DEMO_FAIL_TIMEOUT;
    }
    if (!dci_demo_buf_guards_ok())
    {
        return DEMO_FAIL_GUARD;
    }
    if (!dci_demo_buf_has_data(0, expect))
    {
        return DEMO_FAIL_CONTENT;
    }

    /* No gating in this mode, so suspend_count must be 0 - that is the difference
     * from the gated case, and worth asserting rather than assuming. */
    {
        DCI_stats_t s;

        DCI_GetStats(&s);
        DEMO_LOG("[STATS] done=%u dropped=%u suspend=%u ovr=%u",
                 s.frames_done, s.frames_dropped, s.suspend_count, s.ovr_count);
        if (s.suspend_count != 0)
        {
            DEMO_LOG("  OVERWRITE must never gate, yet suspend_count=%u",
                     s.suspend_count);
            return DEMO_FAIL_STATS;
        }
    }
    return DEMO_PASS;
}
#endif

/*============================================================================*
 *                         Case: reconfigure while running
 *============================================================================*/
/*
 * Swaps to a smaller preset mid-capture. The property being tested is the one that
 * makes Reconfig worth having over Stop-then-Init: an invalid configuration is
 * rejected without disturbing the capture in progress.
 */
#if DEMO_CASE_IS(DCI_DEMO_CASE_RECONFIG)
dci_demo_result_t dci_demo_case_reconfig(const dci_demo_preset_t *p)
{
    const dci_demo_preset_t *small;
    DCI_frame_t              f;
    DCI_InitTypeDef          init;
    dci_demo_result_t        r;
    uint32_t                 expect;
    DCI_ERROR                err;

    demo_cb_reset();

    r = dci_demo_setup(p, dci_demo_buf_count_cfg(), DCI_ON_BUSY_GATE,
                       DCI_NOTIFY_POLL, &expect);
    if (r != DEMO_PASS)
    {
        return r;
    }

    if (DCI_Capture_Start(DCI_MODE_CONTINUOUS) != DCI_SUCCESS)
    {
        return DEMO_FAIL_START;
    }
    if (DCI_Capture_WaitFrame(&f, DCI_DEMO_TIMEOUT_MS) != DCI_SUCCESS)
    {
        demo_report_errors();
        return DEMO_FAIL_TIMEOUT;
    }
    DEMO_LOG("  running on '%s', got seq %u", p->name, f.seq);
    DCI_Capture_ReleaseBuffer(f.index);

    /* An impossible geometry must be refused with the capture left alone. */
    dci_demo_preset_to_init(p, &init);
    init.sensor_lines = 4096;               /* beyond the 2048 spec limit */
    err = DCI_Capture_Reconfig(&init, NULL);
    if (err == DCI_SUCCESS)
    {
        DEMO_LOG("  Reconfig accepted 4096 lines - validation is not working");
        return DEMO_FAIL_STATE;
    }
    DEMO_LOG("  bad Reconfig correctly refused: %d", err);

    if (DCI_Capture_GetState() != DCI_STATE_BUSY)
    {
        DEMO_LOG("  capture was disturbed by a refused Reconfig (state %d)",
                 DCI_Capture_GetState());
        return DEMO_FAIL_STATE;
    }
    if (DCI_Capture_WaitFrame(&f, DCI_DEMO_TIMEOUT_MS) != DCI_SUCCESS)
    {
        DEMO_LOG("  capture stopped after a refused Reconfig");
        return DEMO_FAIL_TIMEOUT;
    }
    DEMO_LOG("  still capturing after the refusal: seq %u", f.seq);
    DCI_Capture_ReleaseBuffer(f.index);

    /* Now a legitimate change. The frame size shrinks, so the buffers stay valid. */
    small = dci_demo_preset_get(DCI_DEMO_PRESET_QQVGA_RGB565);
    dci_demo_preset_to_init(small, &init);
    err = DCI_Capture_Reconfig(&init, NULL);
    if (err != DCI_SUCCESS)
    {
        DEMO_LOG("  valid Reconfig to '%s' failed: %d", small->name, err);
        return DEMO_FAIL_INIT;
    }
    DEMO_LOG("  reconfigured to '%s'", small->name);

    expect = dci_demo_preset_frame_bytes(small);
    if (DCI_Capture_WaitFrame(&f, DCI_DEMO_TIMEOUT_MS) != DCI_SUCCESS)
    {
        demo_report_errors();
        return DEMO_FAIL_TIMEOUT;
    }
    if (f.bytes != expect)
    {
        DEMO_LOG("  after Reconfig got %u B, expected %u B", f.bytes, expect);
        return DEMO_FAIL_FRAME_LEN;
    }
    DEMO_LOG("  new geometry in effect: %u B", f.bytes);
    DCI_Capture_ReleaseBuffer(f.index);

    DCI_Capture_Stop();
    return dci_demo_buf_guards_ok() ? DEMO_PASS : DEMO_FAIL_GUARD;
}
#endif

/*============================================================================*
 *                         Case: suspend / resume
 *============================================================================*/
#if DEMO_CASE_IS(DCI_DEMO_CASE_SUSPEND)
dci_demo_result_t dci_demo_case_suspend(const dci_demo_preset_t *p)
{
    DCI_frame_t       f;
    dci_demo_result_t r;
    uint32_t          expect;

    demo_cb_reset();

    r = dci_demo_setup(p, dci_demo_buf_count_cfg(), DCI_ON_BUSY_GATE,
                       DCI_NOTIFY_POLL, &expect);
    if (r != DEMO_PASS)
    {
        return r;
    }

    if (DCI_Capture_Start(DCI_MODE_CONTINUOUS) != DCI_SUCCESS)
    {
        return DEMO_FAIL_START;
    }
    if (DCI_Capture_WaitFrame(&f, DCI_DEMO_TIMEOUT_MS) != DCI_SUCCESS)
    {
        demo_report_errors();
        return DEMO_FAIL_TIMEOUT;
    }
    DCI_Capture_ReleaseBuffer(f.index);

    if (DCI_Capture_Suspend() != DCI_SUCCESS)
    {
        return DEMO_FAIL_STATE;
    }
    if (DCI_Capture_GetState() != DCI_STATE_SUSPENDED)
    {
        DEMO_LOG("  state %d after Suspend", DCI_Capture_GetState());
        return DEMO_FAIL_STATE;
    }
    DEMO_LOG("  suspended at a frame boundary");

    /* While suspended no frame may arrive; a timeout here is the pass condition. */
    if (DCI_Capture_WaitFrame(&f, 200) == DCI_SUCCESS)
    {
        DEMO_LOG("  a frame arrived while suspended - CAPTURE was not cleared");
        DCI_Capture_ReleaseBuffer(f.index);
        return DEMO_FAIL_STATE;
    }
    DEMO_LOG("  no frames while suspended, as expected");

    if (DCI_Capture_Resume() != DCI_SUCCESS)
    {
        return DEMO_FAIL_STATE;
    }
    if (DCI_Capture_WaitFrame(&f, DCI_DEMO_TIMEOUT_MS) != DCI_SUCCESS)
    {
        DEMO_LOG("  no frame after Resume");
        demo_report_errors();
        return DEMO_FAIL_TIMEOUT;
    }
    DEMO_LOG("  resumed: seq %u, %u B", f.seq, f.bytes);

    r = demo_check_frame(&f, p, expect);
    DCI_Capture_ReleaseBuffer(f.index);
    DCI_Capture_Stop();
    return r;
}
#endif

/*============================================================================*
 *                         Case: stress
 *============================================================================*/
/*
 * Long run at level 0 reporting. Looks for the slow failures a short run cannot show:
 * a guard word eventually clipped, suspend_count climbing because the consumer is
 * marginally too slow, or a resync every few hundred frames.
 */
#if DEMO_CASE_IS(DCI_DEMO_CASE_STRESS)
dci_demo_result_t dci_demo_case_stress(const dci_demo_preset_t *p)
{
    DCI_frame_t       f;
    dci_demo_result_t r;
    uint32_t          expect;
    uint32_t          got = 0;
    uint32_t          prev_seq = 0;
    uint32_t          gaps = 0;
    const uint32_t    target = DCI_DEMO_STRESS_FRAMES;
    /* Ten progress lines regardless of the run length, so shortening the run for a
     * quick check still produces intermediate output rather than silence. */
    const uint32_t    every = (target >= 10) ? (target / 10) : 1;

    demo_cb_reset();

    r = dci_demo_setup(p, dci_demo_buf_count_cfg(), DCI_ON_BUSY_GATE,
                       DCI_NOTIFY_POLL, &expect);
    if (r != DEMO_PASS)
    {
        return r;
    }

    if (DCI_Capture_Start(DCI_MODE_CONTINUOUS) != DCI_SUCCESS)
    {
        return DEMO_FAIL_START;
    }

    DEMO_LOG("  running %u frames, reporting every %u", target, every);

    while (got < target)
    {
        if (DCI_Capture_WaitFrame(&f, DCI_DEMO_TIMEOUT_MS) != DCI_SUCCESS)
        {
            DEMO_LOG("  timeout at frame %u", got);
            demo_report_errors();
            return DEMO_FAIL_TIMEOUT;
        }

        if (expect != 0 && f.bytes != expect)
        {
            DEMO_LOG("  frame %u: %u B, expected %u", got, f.bytes, expect);
            DCI_Capture_ReleaseBuffer(f.index);
            return DEMO_FAIL_FRAME_LEN;
        }
        if (got != 0 && f.seq != prev_seq + 1)
        {
            gaps++;
        }
        prev_seq = f.seq;
        got++;

        DCI_Capture_ReleaseBuffer(f.index);

        /* Guards are checked periodically rather than per frame: the check walks the
         * whole fence, and doing that at frame rate would itself perturb the timing. */
        if ((got % every) == 0)
        {
            DCI_stats_t s;

            if (!dci_demo_buf_guards_ok())
            {
                DEMO_LOG("  guard violated at frame %u", got);
                return DEMO_FAIL_GUARD;
            }
            DCI_GetStats(&s);
            DEMO_LOG("  %4u frames: ovr=%u err=%u resync=%u suspend=%u dropped=%u "
                     "seq-gaps=%u", got, s.ovr_count, s.err_count, s.resync_count,
                     s.suspend_count, s.frames_dropped, gaps);
        }
    }

    DCI_Capture_Stop();
    demo_report_errors();

    DEMO_LOG("  %u frames done, %u seq gap(s)", got, gaps);
    if (gaps != 0)
    {
        DEMO_LOG("  seq gaps mean frames were delivered but not collected");
        return DEMO_FAIL_SEQ;
    }
    return dci_demo_check_stats(target);
}
#endif

/*============================================================================*
 *                         Case: 10-bit (EDM = 10BIT)
 *============================================================================*/
/*
 * Why 10-bit gets its own case rather than being just another preset under snapshot.
 *
 * Every other format can only be checked structurally - right length, guards intact,
 * not all one value - because a real sensor pointed at a real scene has no golden
 * data. EDM = 10BIT is the exception: the hardware packs one sample per 16-bit
 * half-word with bits 15:10 CLEARED (DESIGN.md 2), so every sample carries a
 * verifiable invariant no matter what the camera sees. Checking it exhaustively turns
 * "the image looks wrong" into "sample 4097 has 0x8000 set", and the ratio of bad
 * samples names the fault outright.
 *
 * The case also asserts the two refusals that only exist in 10-bit mode. Those are
 * cheap, need no camera, and are the kind of thing that silently rots: BSM requires
 * EDM = 8BIT (register table) and embedded sync is 8-bit only (spec 2.4). If a future
 * edit loosens dci_validate_init(), a corrupt configuration would be accepted and the
 * symptom would be a sheared image, blamed on wiring.
 */
#if DEMO_CASE_IS(DCI_DEMO_CASE_RAW10)
/*
 * Both mutual exclusions must be refused. Run before the capture: neither needs a
 * camera, so a validation regression is reported even on a bench with no sensor.
 */
static dci_demo_result_t demo_raw10_check_exclusions(const dci_demo_preset_t *p)
{
    DCI_InitTypeDef init;
    DCI_DMA_config  dma;
    DCI_ERROR       err;
    uint8_t         i;

    /* A minimal but otherwise valid dma config, so that whatever Init returns is
     * about the mode conflict and not about a missing buffer. */
    memset(&dma, 0, sizeof(dma));
    dma.dma_channel_num = DCI_DMA_CH_AUTO;
    dma.buf_count       = (DCI_BUF_COUNT_T)1;
    dma.buf_bytes       = dci_demo_buf_bytes();
    dma.notify_mode     = DCI_NOTIFY_POLL;
    dma.on_busy         = DCI_ON_BUSY_GATE;
    dma.frame_error     = dci_demo_on_frame_error;
    for (i = 0; i < 1; i++)
    {
        dma.buf[i] = dci_demo_buf_get(i);
    }

    /* 1. BSM under 10-bit: register table says BSM is valid only when EDM = 00. */
    dci_demo_preset_to_init(p, &init);
    init.bsm = DCI_BSM_EVERY_2ND;
    err = DCI_Capture_Init(&init, &dma);
    if (err == DCI_SUCCESS)
    {
        DEMO_LOG("  BSM was ACCEPTED under EDM=10BIT - validation regressed");
        DEMO_LOG("   register table: BSM is valid only when EDM[1:0] = 00");
        DCI_Capture_Abort();
        return DEMO_FAIL_STATE;
    }
    DEMO_LOG("  BSM correctly refused under 10-bit: %d", err);

    /* 2. Embedded sync under 10-bit: spec 2.4, codes are 8-bit only. */
    dci_demo_preset_to_init(p, &init);
    init.sync = DCI_SYNC_EMBEDDED;
    err = DCI_Capture_Init(&init, &dma);
    if (err == DCI_SUCCESS)
    {
        DEMO_LOG("  embedded sync was ACCEPTED under EDM=10BIT - validation regressed");
        DEMO_LOG("   spec 2.4: embedded codes are 8-bit only");
        DCI_Capture_Abort();
        return DEMO_FAIL_STATE;
    }
    DEMO_LOG("  embedded sync correctly refused under 10-bit: %d", err);

    return DEMO_PASS;
}

dci_demo_result_t dci_demo_case_raw10(const dci_demo_preset_t *p)
{
    DCI_frame_t       f;
    DCI_frame_info_t  info;
    dci_demo_result_t r;
    uint32_t          expect;
    uint32_t          bad;
    DCI_ERROR         err;

    demo_cb_reset();

    /*
     * Guard against being pointed at the wrong preset. The case is specifically about
     * the 10-bit data path, so on an 8-bit row every assertion below would either be
     * vacuous or wrong - better to say so than to "pass" without testing anything.
     */
    if (p->fmt != DCI_FMT_RAW10)
    {
        DEMO_LOG("  preset '%s' is %s, not RAW10", p->name, dci_demo_fmt_name(p->fmt));
        DEMO_LOG("   this case only means anything under EDM=10BIT; pick one of:");
        DEMO_LOG("   %d QVGA_RAW10, %d RAW10_CROP, %d VGA_RAW10, %d RAW10_LSM",
                 DCI_DEMO_PRESET_QVGA_RAW10, DCI_DEMO_PRESET_RAW10_CROP,
                 DCI_DEMO_PRESET_VGA_RAW10, DCI_DEMO_PRESET_RAW10_LSM);
        return DEMO_FAIL_BAD_PRESET;
    }

    r = dci_demo_setup(p, dci_demo_buf_count_cfg(), DCI_ON_BUSY_GATE,
                       DCI_NOTIFY_POLL, &expect);
    if (r != DEMO_PASS)
    {
        return r;
    }

    /*
     * Confirm the geometry really is 2 bytes per pclk before capturing. The frame
     * size alone cannot show this - RGB565 at half the width gives the same byte
     * count - so compare the line length against the declared pclk count.
     */
    if (DCI_Capture_GetFrameInfo(&info) == DCI_SUCCESS && !info.var_len)
    {
        uint16_t pclk_line = p->crop_en ? p->crop_w : p->width;

        DEMO_LOG("  10-bit geometry: %u pclk/line -> %u B/line (expect 2x)",
                 pclk_line, info.line_bytes);

        if (info.line_bytes != (uint32_t)pclk_line * 2U)
        {
            DEMO_LOG("  EDM=10BIT should give 2 bytes per pclk, got %u/%u",
                     info.line_bytes, pclk_line);
            return DEMO_FAIL_GEOMETRY;
        }
    }

    /* The two refusals. Done after setup so a buffer exists, and before Start so a
     * failure here is not confused with a capture fault. Each Init attempt is
     * expected to fail, which leaves the driver unconfigured - so reconfigure by
     * running setup again afterwards. */
    r = demo_raw10_check_exclusions(p);
    if (r != DEMO_PASS)
    {
        return r;
    }

    r = dci_demo_setup(p, dci_demo_buf_count_cfg(), DCI_ON_BUSY_GATE,
                       DCI_NOTIFY_POLL, &expect);
    if (r != DEMO_PASS)
    {
        return r;
    }

    err = DCI_Capture_Start(DCI_MODE_SNAPSHOT);
    if (err != DCI_SUCCESS)
    {
        DEMO_LOG("  Start failed: %d", err);
        return DEMO_FAIL_START;
    }

    err = DCI_Capture_WaitFrame(&f, DCI_DEMO_TIMEOUT_MS);
    if (err != DCI_SUCCESS)
    {
        DEMO_LOG("  no frame in %u ms: %d", DCI_DEMO_TIMEOUT_MS, err);
        demo_report_errors();
        return DEMO_FAIL_TIMEOUT;
    }

    /* Structural checks first: a wrong length would make the per-sample scan report
     * thousands of failures that are really one length fault. */
    if (f.bytes != expect)
    {
        DEMO_LOG("  length %u, expected %u", f.bytes, expect);
        dci_demo_buf_dump(&f, p, DEMO_DUMP_ONESHOT);
        DCI_Capture_ReleaseBuffer(f.index);
        return DEMO_FAIL_FRAME_LEN;
    }
    if (f.truncated)
    {
        DEMO_LOG("  frame flagged truncated");
        DCI_Capture_ReleaseBuffer(f.index);
        return DEMO_FAIL_TRUNCATED;
    }
    if (!dci_demo_buf_guards_ok())
    {
        DCI_Capture_ReleaseBuffer(f.index);
        return DEMO_FAIL_GUARD;
    }

    /* The point of the case. */
    DEMO_LOG("  checking %u samples for 10-bit packing", f.bytes / 2U);
    bad = dci_demo_buf_raw10_bad_samples(&f, 8);
    dci_demo_buf_raw10_range(&f);
    dci_demo_buf_dump(&f, p, DEMO_DUMP_ONESHOT);

    if (bad != 0)
    {
        DCI_Capture_ReleaseBuffer(f.index);
        return DEMO_FAIL_CONTENT;
    }
    DEMO_LOG("  all samples correctly packed: bits 15:10 clear throughout");

    if (!dci_demo_buf_has_data(f.index, f.bytes))
    {
        DCI_Capture_ReleaseBuffer(f.index);
        return DEMO_FAIL_CONTENT;
    }

    DCI_Capture_ReleaseBuffer(f.index);

    if (DCI_Capture_GetState() != DCI_STATE_IDLE)
    {
        DEMO_LOG("  state %d after snapshot, expected IDLE", DCI_Capture_GetState());
        return DEMO_FAIL_STATE;
    }
    return dci_demo_check_stats(1);
}
#endif
