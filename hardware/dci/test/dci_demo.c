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
 * difference decides whether the largest signal rows are runnable at all.
 *
 * The setup sequence (pool, init, crop, interrupt install, geometry cross-check) is
 * shared because it is identical for every capture case, and duplicating it per case is
 * how the copies drift apart.
 */

#include <string.h>
#include "dci_demo.h"
#include "dci_demo_buf.h"
#include "dci_demo_case.h"

/*============================================================================*
 *                         Selected case
 *============================================================================*/
/*
 * One name per case, resolved here so the rest of the file has a single symbol to call
 * and a single string to print.
 *
 *   DEMO_NEEDS_CONFIG   the case is handed a resolved signal row. Off only for REG_TEST,
 *                       which is about the peripheral rather than the signal.
 *   DEMO_NEEDS_SIGNAL   a live camera must be driving the pins. Printed in the banner so
 *                       a missing sensor is stated up front rather than surfacing as a
 *                       timeout thirty seconds later.
 *   DEMO_SKIP_TEARDOWN  the case owns a buffer after it returns, so the usual Abort()
 *                       must not run - Abort() invalidates every delivered frame by
 *                       contract (DESIGN.md 11.5).
 *
 * Nothing pins the buffer count any more: what used to need a dedicated case is now a
 * macro combination, and dci_demo_config.h rejects the combinations that cannot work.
 */
#if   (DCI_DEMO_CASE == DCI_DEMO_CASE_REG_TEST)
#define DEMO_NAME               "reg-test"
#define DEMO_ENTRY              dci_demo_case_reg_test
#define DEMO_NEEDS_CONFIG       0
#define DEMO_NEEDS_SIGNAL       0

#elif (DCI_DEMO_CASE == DCI_DEMO_CASE_SNAPSHOT)
#define DEMO_NAME               "snapshot"
#define DEMO_ENTRY              dci_demo_case_snapshot
#define DEMO_NEEDS_CONFIG       1
/* NO_CAPTURE stops after the configuration is applied and dumped, so no pixels are
 * needed and the banner should not claim otherwise. */
#define DEMO_NEEDS_SIGNAL       (DCI_DEMO_NO_CAPTURE == 0)
/* A snapshot frame stays valid after the case returns - CAPTURE self-clears at frame end,
 * so the data is static and the printed address is dumpable. Tearing down with Abort()
 * would invalidate exactly what the user is about to dump. */
#define DEMO_SKIP_TEARDOWN      (DCI_DEMO_NO_CAPTURE == 0)

#elif (DCI_DEMO_CASE == DCI_DEMO_CASE_CONTINUOUS)
#define DEMO_NAME               "continuous"
#define DEMO_ENTRY              dci_demo_case_continuous
#define DEMO_NEEDS_CONFIG       1
#define DEMO_NEEDS_SIGNAL       1
/* Only when a frame is being held for a dump; a plain run releases everything and can be
 * torn down normally. */
#define DEMO_SKIP_TEARDOWN      (DCI_DEMO_CAPTURE_FRAME != 0)

#else
#error "DCI_DEMO_CASE selects no known case - see dci_demo_id.h"
#endif

#define DEMO_FORCE_BUF_COUNT    DCI_DEMO_BUF_COUNT

#ifndef DEMO_SKIP_TEARDOWN
#define DEMO_SKIP_TEARDOWN      0
#endif

/* Which of the two capture cases is selected, for the banner. Named here rather than
 * tested inline because a preprocessor directive between a macro's parentheses is
 * undefined behaviour (C99 6.10.3p11) - GCC accepts it, so a build sweep would not
 * catch it. */
#define DEMO_CASE_IS_SNAPSHOT   (DCI_DEMO_CASE == DCI_DEMO_CASE_SNAPSHOT)
#define DEMO_CASE_IS_CONTINUOUS (DCI_DEMO_CASE == DCI_DEMO_CASE_CONTINUOUS)

/*============================================================================*
 *                         Result names
 *============================================================================*/
const char *dci_demo_result_name(dci_demo_result_t r)
{
    static const char *const names[] =
    {
        "PASS", "BAD_CONFIG", "REG_DEAD", "REG_BITS", "POOL", "INIT", "GEOMETRY",
        "CROP", "START", "TIMEOUT", "FRAME_LEN", "TRUNCATED", "GUARD", "CONTENT",
        "SEQ", "STATS", "NO_VSYNC", "LINE_COUNT", "STATE",
    };

    return (r < sizeof(names) / sizeof(names[0])) ? names[r] : "?";
}

/*============================================================================*
 *                         Buffer and fill policy
 *============================================================================*/
uint8_t dci_demo_buf_count_cfg(void)
{
    return DEMO_FORCE_BUF_COUNT;
}

/*
 * One translation point for the fill-mode macro. The two enums happen to use the same
 * values, but a cast would make dci_demo_id.h and rtl_dci.h silently dependent on each
 * other's numbering - and dci_demo_id.h exists precisely so the config file needs no
 * driver header.
 */
DCI_ON_BUSY_T dci_demo_on_busy_cfg(void)
{
#if (DCI_DEMO_FILL_MODE == DCI_DEMO_FILL_OVERWRITE)
    return DCI_ON_BUSY_OVERWRITE;
#else
    return DCI_ON_BUSY_GATE;
#endif
}

/*============================================================================*
 *                         Shared setup
 *============================================================================*/
/*
 * Everything a capture case needs before it can start.
 *
 * The geometry cross-check is the reason this is one function rather than a few lines
 * per case. The demo computes the frame size from the signal row and the driver computes
 * it from DCI_InitTypeDef; if those disagree, one of the two formulas is wrong and
 * every later symptom - short frames, overruns, desync - is a consequence. Catching it
 * here turns a day of guessing into one line of output.
 */
dci_demo_result_t dci_demo_setup(const dci_demo_cfg_t *c, uint8_t buf_count,
                                 DCI_ON_BUSY_T on_busy, DCI_NOTIFY_MODE_T notify,
                                 uint32_t *frame_bytes_out)
{
    DCI_InitTypeDef  init;
    DCI_DMA_config   dma;
    DCI_frame_info_t info;
    uint32_t         expect;
    DCI_ERROR        err;
    uint8_t          i;

    expect = c->frame_bytes;            /* 0 for JPEG: no frame size exists */

    /*
     * Size the buffer for what Capture_Init() will validate, not for what will finally
     * be delivered. Those differ when the overlay crops - the window is only programmed
     * afterwards, so Init still measures against the full frame - and for JPEG, where
     * the buffer is a capacity cap rather than a frame size.
     */
    if (!dci_demo_buf_alloc(c->init_bytes, buf_count))
    {
        return DEMO_FAIL_POOL;
    }

    dci_demo_cfg_to_init(c, &init);

    memset(&dma, 0, sizeof(dma));
    dma.dma_channel_num = DCI_DMA_CH_AUTO;
    dma.dma_priority    = 0;
    dma.buf_count       = (DCI_BUF_COUNT_T)buf_count;
    dma.buf_bytes       = dci_demo_buf_bytes();
    dma.on_busy         = on_busy;
    dma.notify_mode     = notify;
    if (notify == DCI_NOTIFY_CALLBACK)
    {
        dma.frame_ready = dci_demo_on_frame_ready;
    }
    for (i = 0; i < buf_count; i++)
    {
        dma.buf[i] = dci_demo_buf_get(i);
    }

    /*
     * frame_error, and the VSYNC/LINE handlers the macros asked for. This is also the
     * register-level interrupt enable: the driver adds IT_VSYNC / IT_LINE to IER only
     * for the pointers that are non-NULL, so a disabled interrupt is a NULL pointer
     * here rather than a masking step somewhere else. Must be before Start(), which
     * re-assigns IER from exactly these fields.
     */
    dci_demo_irq_install(&dma);
    dci_demo_irq_reset();

    err = DCI_Capture_Init(&init, &dma);
    if (err != DCI_SUCCESS)
    {
        DEMO_LOG("  Capture_Init failed: %d", err);
        if (err == DCI_ERROR_BUF_TOO_SMALL)
        {
            DEMO_LOG("  -> allocated %u B for a declared %u x %u pclk frame; Init "
                     "measures the UNCROPPED size", (unsigned)dci_demo_buf_bytes(),
                     c->pclk_per_line, c->lines_in);
        }
        return DEMO_FAIL_INIT;
    }

    /* Crop must follow Init: Capture_Init() does not take a window. The pclk values are
     * pre-derived, including the align-down that DCI_ConfigCrop() would apply, so what
     * was printed is what gets programmed. */
    if (c->crop_en)
    {
        err = DCI_ConfigCrop(c->crop_pclk_x, c->crop_y, c->crop_pclk_w, c->crop_h);
        if (err != DCI_SUCCESS)
        {
            DEMO_LOG("  ConfigCrop(%u,%u,%u,%u) failed: %d", c->crop_pclk_x, c->crop_y,
                     c->crop_pclk_w, c->crop_h, err);
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
        DEMO_LOG("  GEOMETRY MISMATCH: driver says %u B, the table implies %u B",
                 info.frame_bytes, (unsigned)expect);
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
 *
 * Skipped whenever a frame is still held for a dump (DEMO_SKIP_TEARDOWN): Abort()
 * invalidates every delivered frame by contract, which would destroy exactly the data the
 * printed address points at.
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

    /*
     * frames_done may legitimately be one higher than asked for in continuous mode:
     * DCI_Capture_Stop() polls CAPTURE down to 0 with the interrupt still enabled, so a
     * frame that had already ended is still settled and counted. Fewer than expected is
     * always a fault; one extra is the stop boundary, not a lost frame.
     */
    if (s.frames_done < expect_frames)
    {
        DEMO_LOG("  frames_done %u, expected at least %u", s.frames_done,
                 (unsigned)expect_frames);
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
 *
 * The full configuration record comes from dci_demo_cfg_print() rather than from a
 * second set of DEMO_LOG lines here - one printer, so what is printed cannot drift from
 * what is programmed.
 */
static void dci_demo_banner(const dci_demo_cfg_t *c)
{
    DEMO_LOG("");
    DEMO_LOG("================ DCI demo ================");
    DEMO_LOG(" case   : %d  %s%s", DCI_DEMO_CASE, DEMO_NAME,
             DEMO_NEEDS_SIGNAL ? "   [needs a live camera]" : "   [no camera needed]");
    DEMO_LOG(" buffers: %d", DEMO_FORCE_BUF_COUNT);
    DEMO_LOG(" fill   : %s", (dci_demo_on_busy_cfg() == DCI_ON_BUSY_OVERWRITE)
             ? "OVERWRITE - keeps writing into a held buffer, may tear"
             : "GATE - stops rather than overwrite a held buffer");
    DEMO_LOG(" irq    : FRAME/OVR/ERR on (always), VSYNC %s, LINE %s, per-frame log %s",
             DCI_DEMO_IRQ_VSYNC ? "on" : "off",
             DCI_DEMO_IRQ_LINE  ? "on" : "off",
             DCI_DEMO_IRQ_LOG   ? "on" : "off");
    /*
     * The settings that used to be separate cases. Printed unconditionally, including the
     * defaults, because the whole point of the banner is that a log from another build
     * looks identical otherwise - and these four now decide what the run actually does.
     */
#if DEMO_CASE_IS_SNAPSHOT
    DEMO_LOG(" shots  : %d%s", DCI_DEMO_SHOTS,
             (DCI_DEMO_SHOTS > 1) ? "  (rotation check: shot 1 held while 2 is taken)"
             : "");
    DEMO_LOG(" capture: %s", (DCI_DEMO_NO_CAPTURE != 0)
             ? "NO - configure and dump registers only"
             : "yes - frame held on delivery, address printed for JLink");
#elif DEMO_CASE_IS_CONTINUOUS
    DEMO_LOG(" frames : %d", DCI_DEMO_FRAMES);
#if (DCI_DEMO_CAPTURE_FRAME != 0)
    DEMO_LOG(" hold   : frame %d, kept for a JLink dump", DCI_DEMO_CAPTURE_FRAME);
#else
    DEMO_LOG(" hold   : no - every frame released, nothing static to dump");
#endif
#endif
    DEMO_LOG("==========================================");

    if (c != NULL)
    {
        DEMO_LOG("");
        dci_demo_cfg_print(c, DEMO_FORCE_BUF_COUNT);
    }
}

/*============================================================================*
 *                         Test environment
 *============================================================================*/
/*
 * Board bring-up writes that are NOT part of the DCI driver, which is why they live
 * here and not in rtl_dci_int.c: the driver configures DCI, while this opens up the
 * environment DCI needs to exist in on this bring-up platform.
 *
 * Transcribed from the JLink sequence used to bring the part up by hand:
 *
 *     w4 0x40701080, 0xFFFFFFFF
 *     w4 0x40701084, 0xFFFFFFFF
 *     w4 0x40701088, 0xFFFFFFFF
 *     w4 0x4070108C, 0xFFFFFFFF
 *     w4 0x40701090, 0xFFFFFFFF
 *
 * Five consecutive 32-bit words, all ones. Kept as the literal addresses the JLink
 * script uses rather than being dressed up as named registers: no register table for
 * this block has been seen, so inventing names would assert more than is known. When
 * that table turns up, name them and say what the bits do.
 *
 * Idempotent and side-effect free as far as DCI is concerned, so it is safe to call
 * from every case, including the ones that never touch a pixel.
 */
/*
 * Overridable so the host-side build can point it at plain RAM. On target it is the
 * literal address; nothing else should ever redefine it.
 */
#ifndef DEMO_ENV_REG_FIRST
#define DEMO_ENV_REG_FIRST      0x40701080UL
#endif
#define DEMO_ENV_REG_COUNT      5U

void dci_demo_env_init(void)
{
    uint32_t i;
    uint32_t stuck = 0;

    DEMO_LOG("[ENV] 0x%08X..0x%08X <- 0xFFFFFFFF (%u words)",
             (unsigned)DEMO_ENV_REG_FIRST,
             (unsigned)(DEMO_ENV_REG_FIRST + (DEMO_ENV_REG_COUNT - 1U) * 4U),
             (unsigned)DEMO_ENV_REG_COUNT);

    for (i = 0; i < DEMO_ENV_REG_COUNT; i++)
    {
        *(volatile uint32_t *)(DEMO_ENV_REG_FIRST + i * 4U) = 0xFFFFFFFFUL;
    }

    /*
     * Read back and report, because up to now this block was written blind.
     *
     * These five words are the platform enables this bring-up depends on, and nothing
     * downstream can work if they did not take. A write to an ungated-but-absent block,
     * or to one whose own clock is off, is silently dropped on this bus - so "the log
     * printed [ENV]" has never actually proven the writes landed.
     *
     * Not all-ones is NOT automatically a fault: these are unknown bits with no register
     * table, so some are very likely read-only, reserved or write-1-to-clear, and reading
     * back something else is expected for those. What matters is the all-zero case: a word
     * that reads 0 right after 0xFFFFFFFF was written to it means the write went nowhere,
     * and that is a finding rather than a curiosity.
     */
    for (i = 0; i < DEMO_ENV_REG_COUNT; i++)
    {
        uint32_t v = *(volatile uint32_t *)(DEMO_ENV_REG_FIRST + i * 4U);

        DEMO_LOG("      [%u] 0x%08X -> reads 0x%08X%s", (unsigned)i,
                 (unsigned)(DEMO_ENV_REG_FIRST + i * 4U), (unsigned)v,
                 (v == 0xFFFFFFFFUL) ? "  (took)" :
                 ((v == 0U) ? "  <- READS ZERO: write did not land" : "  (partial)"));

        if (v == 0U)
        {
            stuck++;
        }
    }

    if (stuck != 0)
    {
        DEMO_LOG("      *** %u of %u env words read back 0 after writing all-ones.",
                 (unsigned)stuck, (unsigned)DEMO_ENV_REG_COUNT);
        DEMO_LOG("      The writes are NOT reaching this block, so whatever it enables");
        DEMO_LOG("      is still off. Everything downstream - including any external");
        DEMO_LOG("      pin or clock it gates - has to be treated as unconfigured.");
    }
}

dci_demo_result_t dci_demo_run(void)
{
    const dci_demo_cfg_t *cp = NULL;
    dci_demo_result_t     r;

    /*
     * First thing, before the signal row is even resolved: every later step either
     * reads DCI registers or arms the DMA, and both assume the environment is already
     * open. An app that has its own bring-up can call dci_demo_env_init() itself and
     * this second call costs five harmless stores.
     */
    dci_demo_env_init();

#if (DEMO_NEEDS_CONFIG == 1)
    dci_demo_cfg_t cfg;

    /*
     * Resolving before the banner, not after: the resolver is also where an illegal
     * overlay/row combination is rejected, and its own log lines name the macro to
     * change. Printing a configuration record first would make a refusal look like a
     * failure of something that was actually programmed.
     */
    if (!dci_demo_cfg_resolve(DCI_DEMO_SIGNAL, &cfg))
    {
        DEMO_LOG("[DEMO] signal %d is not usable with the current overlay macros",
                 DCI_DEMO_SIGNAL);
        DEMO_LOG("       the whole table follows, with this overlay applied - pick a "
                 "row that is not ruled out:");
        DEMO_LOG("");
        dci_demo_signal_list();
        return DEMO_FAIL_BAD_CONFIG;
    }
    cp = &cfg;
#endif

    dci_demo_banner(cp);

    r = DEMO_ENTRY(cp);

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

#if (DEMO_NEEDS_CONFIG == 1) && (DEMO_SKIP_TEARDOWN == 0)
    dci_demo_teardown();
#elif (DEMO_SKIP_TEARDOWN == 1)
    /* Left running-but-stopped on purpose: the buffer is still locked and its contents
     * are the deliverable. A rerun without a reset starts with Capture_Init, which
     * reconfigures everything this would have cleared. */
    DEMO_LOG("    (peripheral left as-is: the held buffer must stay valid)");
#endif
    return r;
}
