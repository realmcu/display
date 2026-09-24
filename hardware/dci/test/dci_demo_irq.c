/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DCI demo: interrupt counting and per-frame reporting.
 *
 * Five interrupt sources, three of them always on and two optional:
 *
 *   FRAME  always enabled - the driver needs it to close a frame. Where the per-frame
 *          report is emitted, and where "keep frame N" is decided.
 *   OVR    always enabled. Logs unconditionally.
 *   ERR    always enabled. Logs unconditionally.
 *   VSYNC  off unless DCI_DEMO_IRQ_VSYNC. Counted cumulatively.
 *   LINE   off unless DCI_DEMO_IRQ_LINE. Counted per frame and cumulatively.
 *
 * "Off" here means masked in IER, not counted-and-discarded. The driver enables IT_VSYNC
 * and IT_LINE if and only if the matching callback pointer in DCI_DMA_config is non-NULL
 * (dci_int_config() in rtl_dci.c), so leaving the pointer NULL is the register-level
 * disable. That is why the macros gate a pointer here rather than an IER write: the
 * driver is not modified, and there is no window in which the interrupt is enabled but
 * unhandled.
 *
 * Ordering constraint worth stating because it is not obvious and it fails silently:
 * DCI_Capture_Start() calls dci_int_config(), which ASSIGNS IER, immediately before
 * enabling the peripheral. Any IER bit set with DCI_INTConfig() beforehand is wiped, and
 * any set afterwards races the first frame. Supplying the callbacks and letting the
 * driver derive IER is the only approach that is correct at both ends.
 *
 * No printing in an ISR except on the error paths. LINE is roughly 28,000 interrupts per
 * second at 466 lines and 60 fps; a print there does not slow the report down, it causes
 * the overrun the report would have described. The handlers accumulate into plain
 * counters and the frame handler emits one line.
 */

#include "dci_demo.h"

dci_demo_irq_t dci_demo_irq;

/*
 * Which frame to keep, 1-based, 0 meaning keep none. Separate from the counters because
 * it is written by the thread and read by the ISR, where the counters go the other way.
 */
static volatile uint32_t demo_hold_target;
static volatile bool     demo_hold_done;
static DCI_frame_t       demo_held_frame;

void dci_demo_irq_reset(void)
{
    /* Field by field rather than memset: the struct is volatile, and memset on a
     * volatile object is a constraint violation that some compilers accept silently. */
    dci_demo_irq.vsync      = 0;
    dci_demo_irq.line       = 0;
    dci_demo_irq.line_total = 0;
    dci_demo_irq.line_prev  = 0;
    dci_demo_irq.frame      = 0;
    dci_demo_irq.ovr        = 0;
    dci_demo_irq.err        = 0;
    dci_demo_irq.ready      = 0;
    dci_demo_irq.last_seq   = 0;
    dci_demo_irq.last_err   = DCI_SUCCESS;
    dci_demo_irq.last_ris   = 0;

    demo_hold_done = false;
}

/*============================================================================*
 *                         Optional handlers
 *============================================================================*/
/*
 * Compiled only when enabled, so a disabled interrupt costs nothing at all - not an
 * empty function, not a pointer, not a branch. The #if also makes the pointer NULL,
 * which is what keeps the IER bit clear.
 */
#if (DCI_DEMO_IRQ_VSYNC == 1)
static void demo_on_vsync(void)
{
    dci_demo_irq.vsync++;
}
#endif

#if (DCI_DEMO_IRQ_LINE == 1)
static void demo_on_line(void)
{
    dci_demo_irq.line++;
    dci_demo_irq.line_total++;
}
#endif

void dci_demo_irq_install(DCI_DMA_config *dma)
{
    dma->frame_error = dci_demo_on_frame_error;

#if (DCI_DEMO_IRQ_VSYNC == 1)
    dma->vsync_event = demo_on_vsync;
#else
    dma->vsync_event = NULL;        /* leaves IT_VSYNC masked in IER */
#endif

#if (DCI_DEMO_IRQ_LINE == 1)
    dma->line_event = demo_on_line;
#else
    dma->line_event = NULL;         /* leaves IT_LINE masked in IER */
#endif
}

/*============================================================================*
 *                         Frame retention
 *============================================================================*/
void dci_demo_irq_hold_frame(uint32_t n)
{
    demo_hold_target = n;
    demo_hold_done   = false;
}

bool dci_demo_irq_held(void)
{
    return demo_hold_done;
}

const DCI_frame_t *dci_demo_irq_held_frame(void)
{
    return demo_hold_done ? &demo_held_frame : NULL;
}

/*============================================================================*
 *                         Delivery
 *============================================================================*/
/*
 * The per-frame interrupt handler, in the sense that matters to a user of this demo: the
 * driver's IT_FRAME handler settles the frame and then delivers it here.
 *
 * Three things happen per frame, in this order:
 *
 *   1. the line count of the frame that just ended is latched and the counter restarted,
 *      because it is only meaningful per frame;
 *   2. optionally one line is printed - this frame's lines and the cumulative sync count,
 *      which together say whether the receiver and the camera still agree on geometry;
 *   3. the buffer is released, unless this is the frame being kept.
 *
 * Step 3 is what implements "capture frame N": not releasing it leaves it locked, and
 * under the GATE policy the driver stops capture when the ring next needs that buffer.
 * The data is then static without the thread having to stop the DMA at the right instant.
 */
void dci_demo_on_frame_ready(const DCI_frame_t *frame)
{
    uint32_t n;

    dci_demo_irq.frame++;
    dci_demo_irq.ready++;
    dci_demo_irq.last_seq = frame->seq;
    n = dci_demo_irq.ready;

    /* Per-frame, so latch and restart. Read once - LINE can fire between these two
     * statements if the next frame has already started. */
    dci_demo_irq.line_prev = dci_demo_irq.line;
    dci_demo_irq.line      = 0;

#if (DCI_DEMO_IRQ_LOG == 1)
    /*
     * One line per frame. The line count is 0 unless DCI_DEMO_IRQ_LINE is on, and the
     * sync count is 0 unless DCI_DEMO_IRQ_VSYNC is on; both are printed regardless so
     * the format does not change with the build - a log that is missing a column is
     * harder to read than one with a zero in it.
     */
    DEMO_LOG("[IRQ] frame %u  seq %u  buf %u  %u B  lines %u  vsync %u (total)",
             (unsigned)n, (unsigned)frame->seq, frame->index,
             (unsigned)frame->bytes, (unsigned)dci_demo_irq.line_prev,
             (unsigned)dci_demo_irq.vsync);
#endif

    if (demo_hold_target != 0 && n == demo_hold_target)
    {
        /*
         * Stop the pixel flow here rather than in the thread. This runs at the frame
         * boundary, which is the last moment at which no byte of the next frame has
         * been written yet - a thread that polls for demo_hold_done and then stops
         * capture is already a frame period late, and with one buffer that is a frame
         * period of overwriting.
         *
         * Only CR.capture is touched, which is a single register write and legal from
         * an ISR. The driver's own state is left to DCI_Capture_Stop() in the thread:
         * that path is graceful, so the frame stays delivered and the buffer stays
         * locked. Abort() would clear the lock and is exactly what must not happen.
         */
        DCI_CaptureCmd(DISABLE);

        /* Copy the descriptor: the driver's DCI_frame_t for this buffer is reused on
         * the next delivery to the same index, so the pointer would go stale even
         * though the data behind it is protected by the lock. */
        demo_held_frame = *frame;
        demo_hold_done  = true;
        return;                     /* deliberately NOT released */
    }

    /* Legal from an ISR, and it auto-resumes a capture that the gate had stopped. */
    DCI_Capture_ReleaseBuffer(frame->index);
}

/*============================================================================*
 *                         Errors
 *============================================================================*/
/*
 * Always logged, whatever DCI_DEMO_IRQ_LOG is set to. Both of these mean a whole frame
 * was lost, so they cannot flood the log the way LINE would, and a silent overrun is the
 * worst failure mode this peripheral has: the capture keeps running, the frame count
 * keeps climbing, and the only evidence is in a statistics counter nobody reads.
 *
 * The RIS snapshot is included because it is the only view of which bits were pending at
 * the moment of the fault - by the time the thread looks, the driver has cleared them.
 */
void dci_demo_on_frame_error(DCI_ERROR err, uint32_t ris)
{
    dci_demo_irq.last_err = err;
    dci_demo_irq.last_ris = ris;

    switch (err)
    {
    case DCI_ERROR_OVERRUN:
        dci_demo_irq.ovr++;
        DEMO_LOG("[IRQ] *** OVERRUN *** FIFO overflowed, frame %u lost, RIS 0x%02X",
                 (unsigned)(dci_demo_irq.frame + 1), (unsigned)ris);
        DEMO_LOG("      the DMA could not keep up: PCLK too fast, DMA priority too "
                 "low, or a long print in an ISR");
        break;

    case DCI_ERROR_SYNC:
        dci_demo_irq.err++;
        DEMO_LOG("[IRQ] *** SYNC ERROR *** embedded codes out of order, RIS 0x%02X",
                 (unsigned)ris);
        DEMO_LOG("      expected frame-start/line-start/line-end/frame-end in order; "
                 "check the codes and the unmask values");
        break;

    case DCI_ERROR_DESYNC:
        dci_demo_irq.err++;
        DEMO_LOG("[IRQ] *** DESYNC *** block and frame interrupts disagree, driver "
                 "resynced, RIS 0x%02X", (unsigned)ris);
        break;

    case DCI_ERROR_FRAME_LONG:
        dci_demo_irq.err++;
        DEMO_LOG("[IRQ] frame longer than declared: the camera is emitting more than "
                 "the configured geometry (RIS 0x%02X)", (unsigned)ris);
        DEMO_LOG("      run the PROBE_SIGNAL case - it measures the real line count");
        break;

    case DCI_ERROR_FRAME_SHORT:
        dci_demo_irq.err++;
        DEMO_LOG("[IRQ] frame shorter than declared: configured geometry is larger "
                 "than the camera's output (RIS 0x%02X)", (unsigned)ris);
        break;

    case DCI_ERROR_BUF_OVERFLOW:
        dci_demo_irq.err++;
        DEMO_LOG("[IRQ] JPEG stream exceeded the buffer: raise "
                 "DCI_DEMO_JPEG_CAP_KB (RIS 0x%02X)", (unsigned)ris);
        break;

    default:
        dci_demo_irq.err++;
        DEMO_LOG("[IRQ] error %d, RIS 0x%02X", err, (unsigned)ris);
        break;
    }
}

/*============================================================================*
 *                         Report
 *============================================================================*/
void dci_demo_irq_report(void)
{
    DEMO_LOG("[IRQ] frame=%u ready=%u vsync=%u line=%u(total) ovr=%u err=%u",
             (unsigned)dci_demo_irq.frame, (unsigned)dci_demo_irq.ready,
             (unsigned)dci_demo_irq.vsync, (unsigned)dci_demo_irq.line_total,
             (unsigned)dci_demo_irq.ovr, (unsigned)dci_demo_irq.err);

    /* Stating which sources were masked keeps a zero from being read as evidence. A
     * report saying "vsync=0" is ambiguous; one saying the interrupt was off is not. */
#if (DCI_DEMO_IRQ_VSYNC == 0)
    DEMO_LOG("      IT_VSYNC masked (DCI_DEMO_IRQ_VSYNC=0) - vsync count is not a "
             "measurement");
#endif
#if (DCI_DEMO_IRQ_LINE == 0)
    DEMO_LOG("      IT_LINE masked (DCI_DEMO_IRQ_LINE=0) - line counts are not a "
             "measurement");
#endif

    if (dci_demo_irq.ovr != 0 || dci_demo_irq.err != 0)
    {
        DEMO_LOG("      last error %d, RIS 0x%02X at that point",
                 dci_demo_irq.last_err, (unsigned)dci_demo_irq.last_ris);
    }

    /*
     * The hardware's own view, read straight from the registers. This is the part the
     * counters above cannot tell you: all-zero counts mean either "no signal arrived" or
     * "the interrupt never reached the CPU", and those have completely different causes.
     *
     * RIS latches regardless of IER, so a bit set here proves the PIXCLK/VSYNC/HSYNC edge
     * physically happened even if no ISR ever ran. The combination is the diagnosis:
     *
     *   RIS FRAME=1 but frame count 0 -> the signal is there, the interrupt is not
     *                                    reaching the CPU (NVIC, vector table, IER)
     *   RIS all 0, CR.ENABLE=1        -> nothing is arriving on the pins at all
     *   CR.ENABLE=0 or CAPTURE=0      -> the peripheral was never actually started
     */
    {
        uint32_t ris = DCI->RIS;
        uint32_t ier = DCI->IER;
        DCI_CR_TypeDef cr = {.d32 = DCI->CR};
        DCI_SR_TypeDef sr = {.d32 = DCI->SR};

        DEMO_LOG("      RIS 0x%02X [frame=%u ovr=%u err=%u vsync=%u line=%u]"
                 "  IER 0x%02X",
                 (unsigned)(ris & 0x1FU),
                 (unsigned)((ris >> 0) & 1U), (unsigned)((ris >> 1) & 1U),
                 (unsigned)((ris >> 2) & 1U), (unsigned)((ris >> 3) & 1U),
                 (unsigned)((ris >> 4) & 1U), (unsigned)(ier & 0x1FU));
        DEMO_LOG("      CR enable=%u capture=%u   SR fne=%u vsync=%u hsync=%u",
                 (unsigned)cr.b.enable, (unsigned)cr.b.capture,
                 (unsigned)sr.b.fne, (unsigned)sr.b.vsync, (unsigned)sr.b.hsync);

        if (cr.b.enable == 0)
        {
            DEMO_LOG("      -> CR.ENABLE is 0: the peripheral is not running");
        }
        else if ((ris & DCI_INT_FRAME) != 0 && dci_demo_irq.frame == 0)
        {
            DEMO_LOG("      -> FRAME latched in RIS but never counted: the signal is");
            DEMO_LOG("         arriving, the INTERRUPT is not reaching the CPU");
        }
        else if ((ris & 0x1FU) == 0)
        {
            /*
             * Nothing has ever latched. DO NOT read the SR levels as a diagnosis here -
             * that mistake was made once already and it cost a board-level debug session.
             *
             * SR is COMBINATIONAL on the pin XOR the polarity bit, and it is one sample
             * at one instant. Two consequences, both of which make a single read useless:
             *
             *   - flipping hspol/vspol flips the reading with the pin untouched, so a
             *     changed SR proves nothing about the wire;
             *   - HSYNC is in its active state for most of a line (~85% at 320 of ~376
             *     pclk) and VSYNC for most of a frame, so "reads active" is the overwhelmingly
             *     likely sample for a perfectly healthy signal AND for a pin stuck high.
             *
             * What is diagnostic is that RIS is 0 while IER has LINE enabled: RIS latches
             * regardless of IER, so not one line boundary has been recognised. Together
             * with FNE=0 (not one word ever entered the FIFO) the fault is upstream of the
             * DMA - which is why the DMA registers all looking correct is not reassuring,
             * it is simply irrelevant.
             *
             * The measurement that separates the remaining causes is whether the sync pins
             * TOGGLE over a window longer than a frame. dci_demo_sync_probe() does that.
             */
            DEMO_LOG("      -> nothing latched with ENABLE=1, and LINE is enabled in IER.");
            DEMO_LOG("         RIS latches whether or not the interrupt is enabled, so");
            DEMO_LOG("         not one line boundary has been recognised. FNE=%u: no word",
                     (unsigned)sr.b.fne);
            DEMO_LOG("         ever entered the DCI FIFO either, so this is UPSTREAM of");
            DEMO_LOG("         the DMA - the DMA registers being correct means nothing.");
            DEMO_LOG("         The SR levels above are ONE SAMPLE of a level that is");
            DEMO_LOG("         active most of the time anyway - not evidence either way.");
            DEMO_LOG("         Run dci_demo_sync_probe() for the measurement that is.");
        }
    }
}

/*============================================================================*
 *                         Sync probe
 *============================================================================*/
/*
 * Sample SR in a tight loop for longer than a frame period and report whether the sync
 * inputs ever CHANGE. This is the measurement a single SR read cannot make, and it needs
 * no oscilloscope and no reg-dump build.
 *
 * Why it is the decisive step when RIS stays 0:
 *
 *   sync toggles, RIS still 0  -> the sync wires are alive and DCI is not counting, so
 *                                 PIXCLK is not reaching the peripheral. DCI advances its
 *                                 state machine on PIXCLK; with no clock there is no line
 *                                 boundary, no FIFO write and no frame, whatever the sync
 *                                 pins do. THIS IS THE ONE THAT MATCHES A CORRECT-LOOKING
 *                                 DMA AND AN ALL-ZERO RIS.
 *   sync never toggles         -> the sensor is not emitting, or the pin is not routed.
 *                                 A stuck level, which is what SR alone is mistaken for.
 *   HSYNC toggles, VSYNC not   -> VSYNC unrouted, or the window was shorter than a frame.
 *                                 The printed window length says which.
 *   RIS becomes non-zero       -> it was working and the earlier read was simply too early.
 *
 * The duty cycle is printed as a cross-check on top: for the configured geometry HSYNC
 * should be active for roughly pclk_per_line / total_line_pclk of the time. A duty cycle
 * of 0% or 100% is a stuck pin even if the edge counter caught a glitch.
 *
 * SR and RIS are both read-only with no side effects, so this is safe with a capture
 * armed. DR is deliberately NOT touched: reading it pops the FIFO.
 */
void dci_demo_sync_probe(void)
{
    /* Bit positions of SR, from the register table in rtl_dci_def.h. */
    const uint32_t SR_HSYNC = 1U << 0;
    const uint32_t SR_VSYNC = 1U << 1;
    const uint32_t SR_FNE   = 1U << 2;

    uint32_t i;
    uint32_t prev;
    uint32_t h_edges = 0, v_edges = 0;
    uint32_t h_active = 0, v_active = 0, fne_seen = 0;
    uint32_t ris_acc;
    uint32_t taken = 0;

    DEMO_LOG("  --- sync probe: up to %u samples of SR, looking for EDGES ---",
             (unsigned)DCI_DEMO_SYNC_PROBE_SAMPLES);

    ris_acc = DCI->RIS;
    prev    = DCI->SR;

    for (i = 0; i < DCI_DEMO_SYNC_PROBE_SAMPLES; i++)
    {
        uint32_t cur = DCI->SR;

        ris_acc |= DCI->RIS;

        if (((cur ^ prev) & SR_HSYNC) != 0)
        {
            h_edges++;

            /*
             * Report the FIRST edge on each line the moment it happens, rather than only
             * in the summary. The summary is worth nothing if the log is cut off before
             * the loop ends - which is exactly what happened on the first run of this
             * probe: the banner appeared and nothing else. One line per SIGNAL, not per
             * edge, so this cannot flood: at most two extra lines for the whole probe.
             *
             * It also answers the question early. "HSYNC toggles" is already most of the
             * diagnosis, and waiting 200 000 samples to say so is a pointless delay when
             * the interesting case is a board that is producing nothing.
             */
            if (h_edges == 1)
            {
                DEMO_LOG("      HSYNC TOGGLES (first edge at sample %u)", (unsigned)i);
            }
        }
        if (((cur ^ prev) & SR_VSYNC) != 0)
        {
            v_edges++;

            if (v_edges == 1)
            {
                DEMO_LOG("      VSYNC TOGGLES (first edge at sample %u)", (unsigned)i);
            }
        }

        /* SR is pin XOR polarity, so a 0 here is the ACTIVE (non-blanking) state. */
        if ((cur & SR_HSYNC) == 0)
        {
            h_active++;
        }
        if ((cur & SR_VSYNC) == 0)
        {
            v_active++;
        }
        if ((cur & SR_FNE) != 0)
        {
            fne_seen++;
        }

        prev = cur;
        taken = i + 1u;

        /*
         * Both sync lines have been seen moving, so the only remaining question is whether
         * DCI counts anything - and RIS has already answered it. Stop instead of spinning
         * out the full budget: the verdict cannot change, and this runs before a blocking
         * wait, so a shorter probe is strictly better.
         *
         * Counted first, then broken out of, so the duty cycle below covers every sample
         * that was taken and none that were not.
         */
        if (h_edges > 0 && v_edges > 0)
        {
            break;
        }

        /*
         * Heartbeat, bounded to 8 lines for the whole probe.
         *
         * Reached only while at least one sync input has still never moved - the loop
         * breaks out as soon as both have - and that is precisely the board where the
         * operator cannot tell a long probe from a hang. On an FPGA each register read
         * crosses a slow bus bridge, so the budget can be seconds of wall clock with
         * nothing on the UART otherwise.
         *
         * The counts are printed rather than a fixed "no edge yet" string: when one line
         * is alive and the other is not, saying so here is already the diagnosis, and
         * claiming "no edge" while HSYNC has been toggling for 300 samples would be a
         * plainly false log line.
         */
        if ((i % (DCI_DEMO_SYNC_PROBE_SAMPLES / 8U)) == (DCI_DEMO_SYNC_PROBE_SAMPLES / 8U) - 1U)
        {
            DEMO_LOG("      ... %u/%u samples: hsync %u edge(s), vsync %u edge(s) "
                     "(SR now 0x%02X)",
                     (unsigned)taken, (unsigned)DCI_DEMO_SYNC_PROBE_SAMPLES,
                     (unsigned)h_edges, (unsigned)v_edges, (unsigned)(cur & 0x07U));
        }
    }

    /*
     * Percentages are over samples ACTUALLY TAKEN, which the early break above makes
     * different from the configured budget. Dividing by the budget would report a
     * misleadingly tiny duty cycle on a healthy signal - the one case that exits early.
     * taken is at least 1 whenever the loop body ran, and the guard covers the
     * budget-is-zero build that the #error in dci_demo_config.h already rules out.
     */
    if (taken == 0)
    {
        DEMO_LOG("      no samples taken - DCI_DEMO_SYNC_PROBE_SAMPLES is 0");
        return;
    }

    /*
     * "0 edges at 0%" and "0 edges at 100%" are two different board faults that read
     * almost identically in a wall of numbers, so the stuck level is named in words. A
     * duty cycle strictly between the two is only meaningful when edges were seen.
     */
    DEMO_LOG("      HSYNC: %u edges, active in %u%% of %u samples%s",
             (unsigned)h_edges, (unsigned)((h_active * 100U) / taken), (unsigned)taken,
             (h_edges != 0) ? "" :
             ((h_active == taken) ? "  <- STUCK ACTIVE" : "  <- STUCK BLANKING"));
    DEMO_LOG("      VSYNC: %u edges, active in %u%% of %u samples%s",
             (unsigned)v_edges, (unsigned)((v_active * 100U) / taken), (unsigned)taken,
             (v_edges != 0) ? "" :
             ((v_active == taken) ? "  <- STUCK ACTIVE" : "  <- STUCK BLANKING"));
    DEMO_LOG("      FNE set in %u samples,  RIS accumulated over the window 0x%02X",
             (unsigned)fne_seen, (unsigned)(ris_acc & 0x1FU));

    if (h_edges == 0 && v_edges == 0)
    {
        DEMO_LOG("      => NEITHER OBSERVABLE SYNC INPUT EVER CHANGED over the window.");
        DEMO_LOG("         NOTE: that is 2 of the 3 inputs. SR has only hsync/vsync/fne");
        DEMO_LOG("         bits - there is NO status bit for PIXCLK, so this probe cannot");
        DEMO_LOG("         see it directly. PIXCLK is only ever observed INDIRECTLY, via");
        DEMO_LOG("         FNE/RIS moving, and with sync static those stay 0 regardless.");
        DEMO_LOG("         So PIXCLK is UNTESTED here, not proven absent.");
        DEMO_LOG("         This is a STUCK LEVEL, now proven rather than guessed at: a");
        DEMO_LOG("         single SR read cannot tell this from a healthy signal, a whole");
        DEMO_LOG("         window of samples can.");
        DEMO_LOG("         SR is pin XOR polarity, so with hspol/vspol as configured the");
        DEMO_LOG("         RAW pins are sitting at %s.",
                 (h_active == taken) ? "the ACTIVE level" : "the BLANKING level");
        DEMO_LOG("         POLARITY IS RULED OUT, and not merely unlikely: the polarity");
        DEMO_LOG("         bit is an XOR, and this counts edges as (cur ^ prev), where");
        DEMO_LOG("         (a^p)^(b^p) == a^b - the bit cancels out exactly. So the edge");
        DEMO_LOG("         count is INDEPENDENT of hspol/vspol, and a toggling wire must");
        DEMO_LOG("         produce edges at EITHER setting. Wrong polarity would mis-read");
        DEMO_LOG("         blanking as valid - a wrong frame length, RIS bits, errors -");
        DEMO_LOG("         never silence. Zero edges therefore means zero activity on the");
        DEMO_LOG("         wire, which is a BOARD fault, not a configuration one.");
        DEMO_LOG("         Check, in this order:");
        DEMO_LOG("          1. is the sensor actually streaming - most sensors need a");
        DEMO_LOG("             register write to start output, not just power and MCLK;");
        DEMO_LOG("          2. is MCLK/XCLK reaching the sensor at all - with no input");
        DEMO_LOG("             clock it drives no sync and no PCLK either;");
        DEMO_LOG("          3. dcmi_hsync/dcmi_vsync routing in the FPGA .xdc, and the");
        DEMO_LOG("             ribbon/FMC seating between the sensor board and the FPGA.");
        DEMO_LOG("         IF A SCOPE SHOWS THE PINS TOGGLING AT THE SENSOR while this");
        DEMO_LOG("         says static, the break is BETWEEN the two probe points - the");
        DEMO_LOG("         connector or the FPGA routing - not in the sensor or in DCI.");
    }
    else if ((ris_acc & 0x1FU) != 0)
    {
        DEMO_LOG("      => RIS became NON-ZERO during the probe: the capture is in fact");
        DEMO_LOG("         progressing and the earlier read was taken too early.");
    }
    else if (v_edges == 0)
    {
        DEMO_LOG("      => HSYNC toggles but VSYNC NEVER DID. Either VSYNC is not routed,");
        DEMO_LOG("         or the probe window was shorter than one frame period. Raise");
        DEMO_LOG("         DCI_DEMO_SYNC_PROBE_SAMPLES and re-run to tell those apart.");
    }
    else
    {
        DEMO_LOG("      => BOTH SYNC INPUTS TOGGLE, yet RIS is still 0 and FNE never set.");
        DEMO_LOG("         The sync wires are alive and DCI is counting nothing, which");
        DEMO_LOG("         points at PIXCLK NOT REACHING THE PERIPHERAL. DCI advances its");
        DEMO_LOG("         state machine on PIXCLK: with no clock there is no line boundary,");
        DEMO_LOG("         no FIFO write and no frame no matter what the sync pins do.");
        DEMO_LOG("         This is an INFERENCE, not a direct reading - SR has no PIXCLK");
        DEMO_LOG("         status bit, so the clock is only ever seen through its effects.");
        DEMO_LOG("         Check, in this order: is the sensor's PCLK output enabled and");
        DEMO_LOG("         running; is dcmi_pixclk routed (BB9/J4 G5, a clock-capable");
        DEMO_LOG("         pin); does the FPGA constrain it as a clock in the .xdc.");
    }
}
