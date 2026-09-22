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
}
