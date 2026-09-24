/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DCI demo: the test cases.
 *
 * Three cases: a register self test, a snapshot case and a continuous case. Each is a
 * linear script - readable top to bottom, breakpoint-friendly on any line - built from
 * the shared setup in dci_demo.c plus whatever assertions are specific to it. Nothing
 * here duplicates the setup sequence.
 *
 * A case is a different PROCEDURE. Anything that is the same procedure with a different
 * number in it is a macro in dci_demo_config.h instead, which is why what used to be
 * snapshot-x2, continuous-1buf, stress and config-only are now DCI_DEMO_SHOTS,
 * DCI_DEMO_BUF_COUNT + DCI_DEMO_FILL_MODE, DCI_DEMO_FRAMES and DCI_DEMO_NO_CAPTURE.
 *
 * A case takes a RESOLVED configuration (dci_demo_cfg_t): the signal row chosen by
 * DCI_DEMO_SIGNAL with the overlay macros applied and the geometry already derived. No
 * case recomputes a frame size, so there is exactly one formula to be wrong.
 *
 * Both capture cases use POLL notification, except continuous when it has to hold a
 * frame: the driver's WaitFrame() is a bounded busy-wait designed for exactly this, no OS
 * required, but deciding WHICH frame to keep has to happen in the frame interrupt.
 *
 * Interrupt counting and the frame_ready / frame_error handlers live in dci_demo_irq.c,
 * shared by every case, because "the image looked right but the sync count drifted" is
 * precisely the fault those counters exist to expose.
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
 * The shared helpers below are only referenced by the capture cases. Compiling them for
 * REG_TEST would produce unused-function warnings on every build, so they are gated on
 * "is this a capture case" rather than left unconditional.
 */
#define DEMO_IS_CAPTURE_CASE                    \
    (DEMO_CASE_IS(DCI_DEMO_CASE_SNAPSHOT) ||    \
     DEMO_CASE_IS(DCI_DEMO_CASE_CONTINUOUS))

/*
 * demo_check_frame() validates a delivered frame. Two builds never call it: NO_CAPTURE
 * (no frame is ever delivered) and the continuous hold path, which deliberately asserts
 * almost nothing - a short or long frame is what the dump is there to explain, so it is
 * reported rather than judged.
 */
#define DEMO_NEEDS_FRAME_CHECK                                              \
    ((DEMO_CASE_IS(DCI_DEMO_CASE_SNAPSHOT) && (DCI_DEMO_NO_CAPTURE == 0)) || \
     (DEMO_CASE_IS(DCI_DEMO_CASE_CONTINUOUS) && (DCI_DEMO_CAPTURE_FRAME == 0)))

/*
 * DCI_DEMO_DUMP_LEVEL applies to the cases that stop between frames. A streaming case
 * pins it to 0 no matter what the config says: a 424 KB frame is roughly 7 s of hex at
 * 1 Mbps, and printing that between frames causes the very overruns the case exists to
 * detect. Naming the two levels here keeps that reasoning in one place instead of
 * appearing as an unexplained literal at each call site.
 */
#define DEMO_DUMP_ONESHOT       DCI_DEMO_DUMP_LEVEL
#define DEMO_DUMP_STREAM        0

/*
 * The frame wait is split into slices so it can report progress. DCI_Capture_WaitFrame()
 * spins a count rather than reading a clock, and prints nothing while it does, so a
 * single call with the full budget is indistinguishable from a hang. 10 ms per slice is
 * well over one frame period at any sane rate, so a frame that is coming still arrives
 * inside the first slice.
 */
#define DEMO_WAIT_SLICE_MS      10
#define DEMO_WAIT_SLICES        ((DCI_DEMO_TIMEOUT_MS + DEMO_WAIT_SLICE_MS - 1) \
                                 / DEMO_WAIT_SLICE_MS)

/*
 * DMA ERROR interrupts seen by the SoC layer. Declared here rather than reached through
 * rtl_dci_dma.h, because that header is the SoC port's own and the demo has no business
 * including it; one extern for one diagnostic counter is the smaller coupling. Returns 0
 * on a port that does not track them.
 */
extern uint32_t rtl_dci_dma_err_count(void);

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
                                          const dci_demo_cfg_t *c,
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

    (void)c;

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
 *                         Dump report
 *============================================================================*/
/*
 * Printed after every captured frame by both capture cases, because the address of the
 * data is information whether or not the automatic checks passed - arguably more so when
 * they failed. This is why there is no separate "receive" case any more: getting the
 * bytes out is not a different procedure, it is one more line of output.
 *
 * A length that disagrees with the configuration is reported and NOT treated as a
 * failure here: a short or long frame is exactly the kind of thing a dump is meant to
 * explain, and suppressing the address would remove the only evidence. The caller still
 * applies demo_check_frame() and decides pass/fail on its own.
 */
#if DEMO_IS_CAPTURE_CASE && (DCI_DEMO_NO_CAPTURE == 0)
static void demo_report_dump(const DCI_frame_t *f, uint32_t expect, bool static_now)
{
    DEMO_LOG("");
    DEMO_LOG("=========== CAPTURE COMPLETE ===========");
    DEMO_LOG("  seq %u, buffer %u, %u B%s", (unsigned)f->seq, f->index,
             (unsigned)f->bytes, f->truncated ? "  (TRUNCATED)" : "");
    if (expect != 0 && f->bytes != expect)
    {
        DEMO_LOG("  NOTE: expected %u B - the length disagrees with the config",
                 (unsigned)expect);
    }
    dci_demo_buf_report_range(f);

    /*
     * The first words of the frame, printed here rather than left to JLink. Reading a
     * dozen words over the UART answers the one question a length and an address cannot:
     * whether the DMA wrote anything at all. 0xDEADBEEF is this demo's poison, so a row
     * of it means the buffer was never touched - which looks identical to a successful
     * capture if all you have is "seq 1, 38400 B".
     */
    dci_demo_buf_head_dump(f, 16);

    if (static_now)
    {
        DEMO_LOG("  capture is stopped; the data is static and will not change");
        DEMO_LOG("  set a breakpoint here, then dump with the command above");
    }
    else
    {
        DEMO_LOG("  NOTE: capture is still running - this buffer will be rewritten.");
        DEMO_LOG("  For a dump use the SNAPSHOT case, or set DCI_DEMO_CAPTURE_FRAME.");
    }
    DEMO_LOG("========================================");
}
#endif /* DEMO_IS_CAPTURE_CASE && !NO_CAPTURE */

/*============================================================================*
 *                         Case: register self test
 *============================================================================*/
/*
 * First case for a reason: it needs no camera, no DMA and no pins, so it separates
 * "the peripheral is not reachable" from "the peripheral is reachable but the capture
 * is wrong". Those two have completely different causes.
 */
#if DEMO_CASE_IS(DCI_DEMO_CASE_REG_TEST)
dci_demo_result_t dci_demo_case_reg_test(const dci_demo_cfg_t *c)
{
    DCI_ERROR err;

    (void)c;

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
        DEMO_LOG("   1. wrong base address (expect DCI at 0x40643000)");
        DEMO_LOG("   2. display island powered down");
        DEMO_LOG("   3. DCI clock gated after all - it is documented as on by");
        DEMO_LOG("      default, so this is last; see rtl_dci_clock_enable()");
        return DEMO_FAIL_REG_DEAD;

    default:
        DEMO_LOG("  register block responds but some bits misbehave");
        DEMO_LOG("   check the writable-bit masks against the register table");
        return DEMO_FAIL_REG_BITS;
    }
}
#endif

/*============================================================================*
 *                         Case: snapshot
 *============================================================================*/
/*
 * The case to use for a JLink dump. A snapshot frame is static the moment it is
 * delivered: CAPTURE self-clears at frame end (spec 2.5.1), so nothing is still writing
 * to the buffer while the address is printed. dci_demo.c also skips the usual Abort()
 * teardown for this case, because Abort() invalidates every delivered frame by contract.
 *
 * DCI_DEMO_NO_CAPTURE = 1 stops after the configuration is applied and dumped. That
 * covers everything checkable without pixels - Capture_Init, the crop programming and
 * the geometry cross-check against the driver - so signal rows and overlays can be swept
 * on a board with no sensor attached.
 *
 * DCI_DEMO_SHOTS = 2 takes a second snapshot while the first is still held, which is the
 * double-buffer rotation check: if rotation is broken the driver either overwrites the
 * held buffer or reports BUSY. dci_demo_config.h requires BUF_COUNT = 2 for it, because
 * two shots into one buffer would look like a passing rotation check while testing
 * nothing.
 */
#if DEMO_CASE_IS(DCI_DEMO_CASE_SNAPSHOT)
dci_demo_result_t dci_demo_case_snapshot(const dci_demo_cfg_t *c)
{
    dci_demo_result_t r;
    uint32_t          expect;

#if (DCI_DEMO_NO_CAPTURE == 1)

    /* Configure only. The setup already ran Capture_Init, the crop programming and the
     * geometry cross-check; all that is left is to prove the peripheral settled in a
     * state that can be started, and to show the registers that were written. */
    r = dci_demo_setup(c, dci_demo_buf_count_cfg(), dci_demo_on_busy_cfg(),
                       DCI_NOTIFY_POLL, &expect);
    if (r != DEMO_PASS)
    {
        return r;
    }
    (void)expect;

    DEMO_LOG("  configuration accepted; not capturing "
             "(DCI_DEMO_NO_CAPTURE = 1, no camera needed)");

    /*
     * The whole table, resolved against this build's overlay. Costs nothing here - no
     * capture is running - and it is the pre-flight the deleted signal-dump case used to
     * provide: a bad pclk ratio, an impossible frame size or a row the overlay rules out
     * is visible before any hardware is involved. The overlay is printed first because
     * the same table yields different numbers under different macros.
     */
    DEMO_LOG("");
    DEMO_LOG("  overlay in force for every row below:");
    DEMO_LOG("   crop=%d fcrc=%d bsm=%d oebs=%d lsm=%d oels=%d",
             DCI_DEMO_OV_CROP, DCI_DEMO_OV_FCRC, DCI_DEMO_OV_BSM,
             DCI_DEMO_OV_OEBS, DCI_DEMO_OV_LSM, DCI_DEMO_OV_OELS);
    DEMO_LOG("");
    dci_demo_signal_list();
    DEMO_LOG("");

#if (DCI_DEBUG_REG_DUMP == 1)
    DCI_RegDump();
#else
    DEMO_LOG("  (build with DCI_DEBUG_REG_DUMP=1 to see the registers)");
#endif

    /* IDLE means configured and startable. RESET would mean Capture_Init never ran,
     * which the setup above would already have reported. */
    if (DCI_Capture_GetState() != DCI_STATE_IDLE)
    {
        DEMO_LOG("  state %d after Init, expected IDLE", DCI_Capture_GetState());
        return DEMO_FAIL_STATE;
    }

    /* The pool is worth printing even with no frame in it: "where is the buffer and how
     * big is it" is the first thing needed when nothing arrives later. */
    dci_demo_buf_report_range(NULL);
    return DEMO_PASS;

#else /* capture */

    DCI_frame_t f[DCI_DEMO_SHOTS];
    DCI_ERROR   err;
    uint8_t     shot;

    r = dci_demo_setup(c, dci_demo_buf_count_cfg(), dci_demo_on_busy_cfg(),
                       DCI_NOTIFY_POLL, &expect);
    if (r != DEMO_PASS)
    {
        return r;
    }

    for (shot = 0; shot < DCI_DEMO_SHOTS; shot++)
    {
        if (DCI_DEMO_SHOTS > 1)
        {
            DEMO_LOG("  --- shot %u of %u ---", shot + 1u, DCI_DEMO_SHOTS);
        }

        err = DCI_Capture_Start(DCI_MODE_SNAPSHOT);
        if (err != DCI_SUCCESS)
        {
            DEMO_LOG("  Start failed: %d", err);
            return DEMO_FAIL_START;
        }

        /*
         * Say so BEFORE blocking. WaitFrame() has no clock - it spins
         * DCI_DEMO_TIMEOUT_MS * DCI_WAIT_ITER_PER_MS times - so at the default 3000 it
         * can sit here for a minute with nothing on the UART. A log that just stops
         * after the geometry line is that, and it is indistinguishable from a crash
         * unless this line exists.
         */
        DEMO_LOG("  capture started, polling for a frame in %u slices of %u ms",
                 (unsigned)DEMO_WAIT_SLICES, (unsigned)DEMO_WAIT_SLICE_MS);

        /*
         * Dump the registers HERE, with the capture armed and before anything can block.
         *
         * Every other DCI_RegDump() in this demo is on a failure path, which is useless
         * when the failure is a hang: the log stops before reaching it. This one always
         * runs, so the state that was actually programmed - IER, CR.ENABLE/CAPTURE, the
         * DMA channel's CFG_HI.SRC_PER and CH_EN - is on record whatever happens next.
         */
#if (DCI_DEBUG_REG_DUMP == 1)
        DEMO_LOG("  --- registers with capture armed ---");
        DCI_RegDump();
#endif
        /* And the hardware's own view of the sync signals, which needs no reg dump build:
         * RIS latches whether or not the interrupt is enabled, so this says straight away
         * whether FRAME/VSYNC/HSYNC edges are physically arriving. */
        dci_demo_irq_report();

        /*
         * If nothing has latched yet, measure the sync pins before blocking.
         *
         * This runs BEFORE the wait rather than after the timeout on purpose: the probe is
         * the only thing here that can tell a stuck level from a live signal, and after a
         * full timeout budget the operator has already waited a minute for a log line that
         * says less. RIS is checked first so a working capture is never delayed by it.
         *
         * The condition is deliberately RIS, not the frame counter: RIS latches without
         * the interrupt reaching the CPU, so it stays 0 only when the peripheral itself
         * has seen nothing - which is exactly when the probe has something to say.
         */
        if ((DCI->RIS & 0x1FU) == 0)
        {
            dci_demo_sync_probe();
        }

        /*
         * Sliced rather than one long wait, so the wait itself reports progress. One
         * WaitFrame(3000) is silent for its whole duration; N x WaitFrame(short) prints
         * between slices, which distinguishes "still polling" from "hung in an ISR" -
         * the two look identical from outside, and only one of them is a driver bug.
         */
        {
            uint32_t slice;

            /* Print every Nth slice, not every one. At the default budget this loop runs
             * 300 times, and 300 identical "no frame yet" lines push the diagnosis that
             * follows them off the top of a terminal scrollback - which is how a log ends
             * up truncated exactly where it matters. Roughly 10 lines is enough to show
             * the wait is progressing. */
            const uint32_t step = (DEMO_WAIT_SLICES >= 10u) ? (DEMO_WAIT_SLICES / 10u) : 1u;

            err = DCI_ERROR_TIMEOUT;
            for (slice = 0; slice < DEMO_WAIT_SLICES; slice++)
            {
                err = DCI_Capture_WaitFrame(&f[shot], DEMO_WAIT_SLICE_MS);
                if (err == DCI_SUCCESS)
                {
                    break;
                }
                if ((slice % step) == step - 1u)
                {
                    DEMO_LOG("    ... slice %u/%u: no frame yet "
                             "(dci frame irq %u, dma err %u)",
                             (unsigned)(slice + 1u), (unsigned)DEMO_WAIT_SLICES,
                             (unsigned)dci_demo_irq.frame,
                             (unsigned)rtl_dci_dma_err_count());
                }
            }
        }
        if (err != DCI_SUCCESS)
        {
            DEMO_LOG("  NO FRAME: WaitFrame returned %d after the full budget", err);
            DEMO_LOG("  -> no frame interrupt arrived, so nothing was delivered.");
            DEMO_LOG("     Check in this order: PCLK present? VSYNC/HSYNC toggling?");
            DEMO_LOG("     polarity matching the sensor? CR.ENABLE and CR.CAPTURE set?");
            dci_demo_irq_report();
            dci_demo_buf_report_range(NULL);
#if (DCI_DEBUG_REG_DUMP == 1)
            /* The registers at the moment of failure are the diagnostic - in particular
             * CFG_HI.SRC_PER (should be 0x30 = 48), CH_EN for the channel, and whether
             * CURR_DAR has moved from DAR at all. */
            DCI_RegDump();
#else
            DEMO_LOG("     rebuild with DCI_DEBUG_REG_DUMP=1 for the register dump");
#endif
            return DEMO_FAIL_TIMEOUT;
        }

        r = demo_check_frame(&f[shot], c, expect);
        dci_demo_buf_dump(&f[shot], c, DEMO_DUMP_ONESHOT);
        if (r != DEMO_PASS)
        {
            /* Still print the address: a frame that failed a check is precisely the one
             * worth looking at byte by byte. */
            demo_report_dump(&f[shot], expect, true);
            dci_demo_irq_report();
            return r;
        }

        /* CAPTURE self-clears after a snapshot frame, so the driver must be back to IDLE
         * without anyone stopping it (spec 2.5.1). */
        if (DCI_Capture_GetState() != DCI_STATE_IDLE)
        {
            DEMO_LOG("  state %d after snapshot, expected IDLE",
                     DCI_Capture_GetState());
            return DEMO_FAIL_STATE;
        }

        /* Deliberately NOT released inside the loop: holding shot 1 while shot 2 is
         * taken is what makes the second shot a rotation test rather than a repeat. */
    }

    /*
     * Rotation: the two shots must land in different buffers and carry consecutive
     * sequence numbers. With one buffer there is nothing to rotate, which is why
     * dci_demo_config.h refuses SHOTS = 2 there.
     *
     * #if, not if: f is DCI_frame_t[DCI_DEMO_SHOTS], so at the default SHOTS = 1 the f[1]
     * accesses below are out of bounds. A plain `if` on a compile-time constant leaves
     * them compiled - dead at run time, but still an out-of-bounds subscript in the
     * translation unit, which static analysis flags as an error and is entitled to.
     * Preprocessing the block away means the code that indexes f[1] only exists in a
     * build where f actually has an f[1].
     */
#if (DCI_DEMO_SHOTS > 1)
    if (f[1].index == f[0].index)
    {
        DEMO_LOG("  both shots landed in buffer %u: rotation stalled while the "
                 "first was still held", f[0].index);
        r = DEMO_FAIL_SEQ;
    }
    else if (f[1].seq != f[0].seq + 1u)
    {
        DEMO_LOG("  seq %u then %u: a frame was delivered but never collected",
                 (unsigned)f[0].seq, (unsigned)f[1].seq);
        r = DEMO_FAIL_SEQ;
    }
    else
    {
        DEMO_LOG("  rotation ok: buffer %u then %u, seq %u then %u",
                 f[0].index, f[1].index, (unsigned)f[0].seq, (unsigned)f[1].seq);
    }
#endif

    /*
     * The last frame stays locked on purpose - that is the deliverable for a JLink dump,
     * and dci_demo.c skips teardown for this case so it stays valid. Earlier shots are
     * released: their buffers are of no further use and holding them would confuse the
     * state a rerun starts from.
     */
    for (shot = 0; shot + 1u < DCI_DEMO_SHOTS; shot++)
    {
        DCI_Capture_ReleaseBuffer(f[shot].index);
    }

    demo_report_dump(&f[DCI_DEMO_SHOTS - 1], expect, true);

    dci_demo_irq_report();
    if (r != DEMO_PASS)
    {
        return r;
    }
    return dci_demo_check_stats(DCI_DEMO_SHOTS);

#endif /* DCI_DEMO_NO_CAPTURE */
}
#endif

/*============================================================================*
 *                         Case: continuous
 *============================================================================*/
/*
 * The only case that exercises the closed descriptor ring (LLP != 0, so transfer-complete
 * never fires and completion comes from the evt rendezvous). The snapshot path does not
 * go near any of that.
 *
 * Back-pressure comes from DCI_DEMO_FILL_MODE rather than being pinned: GATE clears
 * CAPTURE and waits, which is the safe default; OVERWRITE with BUF_COUNT = 1 is the
 * viewfinder configuration, and there the key assertion is that the driver never gates -
 * suspend_count must stay 0.
 *
 * DCI_DEMO_FRAMES scales it from a quick check to a long soak; 1000 frames is the
 * stress setting. DCI_DEMO_CAPTURE_FRAME != 0 switches to callback notification and
 * keeps that frame for a dump: frames before it are received and released normally,
 * frame N is kept, and capture is stopped from the ISR (dci_demo_irq.c). Holding uses
 * Stop() rather than Abort() by contract - Abort() invalidates delivered frames.
 */
#if DEMO_CASE_IS(DCI_DEMO_CASE_CONTINUOUS)

#if (DCI_DEMO_CAPTURE_FRAME != 0)
/*
 * Spin budget scaled to the frame being waited for - waiting for frame 100 takes about a
 * hundred frame periods, and a fixed budget would time out on exactly the configurations
 * someone would use it for. Clamped rather than allowed to wrap.
 */
#define DEMO_HOLD_SPIN_PER_FRAME    8000000UL
#if (DCI_DEMO_CAPTURE_FRAME > 500)
#define DEMO_HOLD_SPIN              0xFFFFFFFFUL
#else
#define DEMO_HOLD_SPIN              (DCI_DEMO_CAPTURE_FRAME * DEMO_HOLD_SPIN_PER_FRAME)
#endif
#endif

dci_demo_result_t dci_demo_case_continuous(const dci_demo_cfg_t *c)
{
    dci_demo_result_t r;
    uint32_t          expect;

#if (DCI_DEMO_CAPTURE_FRAME != 0)

    /*---- hold a frame for a dump ----*/
    const DCI_frame_t *held;
    uint32_t           spin;

    r = dci_demo_setup(c, dci_demo_buf_count_cfg(), dci_demo_on_busy_cfg(),
                       DCI_NOTIFY_CALLBACK, &expect);
    if (r != DEMO_PASS)
    {
        return r;
    }

    /* Arm the hold before starting: with DCI_DEMO_CAPTURE_FRAME = 1 the first frame
     * interrupt can arrive before the next statement would have run. */
    dci_demo_irq_hold_frame(DCI_DEMO_CAPTURE_FRAME);

    DEMO_LOG("  receiving; frame %u will be kept, earlier frames released",
             (unsigned)DCI_DEMO_CAPTURE_FRAME);

    if (DCI_Capture_Start(DCI_MODE_CONTINUOUS) != DCI_SUCCESS)
    {
        return DEMO_FAIL_START;
    }

    for (spin = 0; spin < DEMO_HOLD_SPIN && !dci_demo_irq_held(); spin++)
    {
        __asm volatile("" ::: "memory");
    }

    /* Graceful stop: frames already delivered stay valid, buf_lock_mask is left alone.
     * This is the reason the hold path does not call Abort(). */
    DCI_Capture_Stop();

    DEMO_LOG("");
    dci_demo_irq_report();

    if (!dci_demo_irq_held())
    {
        DEMO_LOG("");
        DEMO_LOG("  NO FRAME KEPT: %u frame(s) delivered, needed %u",
                 (unsigned)dci_demo_irq.ready, (unsigned)DCI_DEMO_CAPTURE_FRAME);
        if (dci_demo_irq.ready == 0)
        {
            DEMO_LOG("  nothing arrived at all - check PCLK, the sync polarity and the");
            DEMO_LOG("  wiring; DCI_DEMO_IRQ_VSYNC=1 reports whether the pins are alive");
        }
        dci_demo_buf_report_range(NULL);
        return DEMO_FAIL_TIMEOUT;
    }

    held = dci_demo_irq_held_frame();

    /* Guards are checked because a DMA that wrote past its buffer has already corrupted
     * something else, and that is worth knowing before the dump is trusted. Everything
     * else is reported, not asserted: judging the data is the user's job. */
    r = dci_demo_buf_guards_ok() ? DEMO_PASS : DEMO_FAIL_GUARD;

    demo_report_dump(held, expect, true);

    /* A brief look over the UART as well, so a completely wrong result is obvious
     * without attaching a debugger at all. */
    dci_demo_buf_dump(held, c, 1);
    return r;

#else /* stream DCI_DEMO_FRAMES frames */

    DCI_frame_t f;
    uint32_t    prev_seq = 0;
    uint32_t    got = 0;
    uint8_t     prev_index = 0xFF;

    r = dci_demo_setup(c, dci_demo_buf_count_cfg(), dci_demo_on_busy_cfg(),
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
            dci_demo_irq_report();
            return DEMO_FAIL_TIMEOUT;
        }

        r = demo_check_frame(&f, c, expect);
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

        dci_demo_buf_dump(&f, c, DEMO_DUMP_STREAM);

        prev_seq   = f.seq;
        prev_index = f.index;
        got++;

        /* The last frame's address is worth printing even though it is about to be
         * released: it says where the data actually landed, and it states plainly that
         * the ring will overwrite it - which is the reason DCI_DEMO_CAPTURE_FRAME and
         * the SNAPSHOT case exist. */
        if (got == DCI_DEMO_FRAMES)
        {
            demo_report_dump(&f, expect, false);
        }

        DCI_Capture_ReleaseBuffer(f.index);
    }

    DCI_Capture_Stop();
    DEMO_LOG("  collected %u frames, seq %u..%u", got, prev_seq - got + 1, prev_seq);

    /*
     * OVERWRITE must never gate: the whole point of the policy is that the capture keeps
     * running at full rate. A non-zero suspend_count means the driver stopped for a busy
     * buffer, which is GATE behaviour and would make a viewfinder stutter.
     */
    if (dci_demo_on_busy_cfg() == DCI_ON_BUSY_OVERWRITE)
    {
        DCI_stats_t st;

        DCI_GetStats(&st);
        if (st.suspend_count != 0)
        {
            DEMO_LOG("  suspend_count %u under OVERWRITE - the driver gated, which "
                     "this policy must never do", (unsigned)st.suspend_count);
            dci_demo_irq_report();
            return DEMO_FAIL_STATS;
        }
        DEMO_LOG("  OVERWRITE held full rate: suspend_count 0");
    }

    DEMO_LOG("  (set DCI_DEMO_CAPTURE_FRAME = N to keep frame N for a JLink dump,");
    DEMO_LOG("   or use the SNAPSHOT case, where the frame is static on delivery)");

    dci_demo_irq_report();
    return dci_demo_check_stats(DCI_DEMO_FRAMES);

#endif /* DCI_DEMO_CAPTURE_FRAME */
}
#endif
