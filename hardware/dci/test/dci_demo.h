/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DCI application-level functional demo. Bare metal, no OS.
 *
 * ONE CASE PER BUILD. The case, the signal row and the overlay are chosen in
 * dci_demo_config.h and fixed at compile time; only the selected case is compiled and
 * only its buffers exist. See that file for why, and test/README.md for the whole
 * picture.
 *
 * Usage from the SDK's main(), once the clock and log UART are up:
 *
 *     #include "dci_demo.h"
 *
 *     int main(void)
 *     {
 *         ...
 *         dci_demo_run();          // runs whatever dci_demo_config.h selected
 *     }
 *
 * Three independent axes: a case says what procedure to run, a signal row says what
 * arrives on the pins, an overlay says what this machine then does with it. The same
 * case therefore covers every signal and every feature combination without new code.
 */

#ifndef DCI_DEMO_H
#define DCI_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rtl_dci.h"
#include "dci_demo_id.h"
#include "dci_demo_config.h"
#include "dci_demo_signal.h"

/*============================================================================*
 *                         Fixed constants
 *============================================================================*/
/** \brief  Guard word written around every buffer to catch a DMA overrun. */
#define DCI_DEMO_GUARD              0xA5A5A5A5UL

/** \brief  Guard bytes per fence. Four words is enough to see a small overshoot. */
#define DCI_DEMO_GUARD_BYTES        16

/** \brief  Fill before capture: tells "nothing arrived" from "the data is zeros". */
#define DCI_DEMO_POISON             0xDEADBEEFUL

/*============================================================================*
 *                         Result
 *============================================================================*/
typedef enum
{
    DEMO_PASS = 0,
    DEMO_FAIL_BAD_CONFIG,       /*!< Signal row out of range, or the overlay rules it
                                     out - see the log for which macro. */
    DEMO_FAIL_REG_DEAD,         /*!< Register block unresponsive: clock or base. */
    DEMO_FAIL_REG_BITS,         /*!< Register bits misbehave. */
    DEMO_FAIL_POOL,             /*!< Configuration does not fit the framebuffer pool. */
    DEMO_FAIL_INIT,             /*!< DCI_Capture_Init() rejected the configuration. */
    DEMO_FAIL_GEOMETRY,         /*!< Driver and demo disagree on the frame size. */
    DEMO_FAIL_CROP,             /*!< Crop configuration rejected. */
    DEMO_FAIL_START,            /*!< DCI_Capture_Start() failed. */
    DEMO_FAIL_TIMEOUT,          /*!< No frame within DCI_DEMO_TIMEOUT_MS. */
    DEMO_FAIL_FRAME_LEN,        /*!< Delivered length is not the expected one. */
    DEMO_FAIL_TRUNCATED,        /*!< Frame arrived flagged truncated. */
    DEMO_FAIL_GUARD,            /*!< DMA wrote outside its buffer. */
    DEMO_FAIL_CONTENT,          /*!< Buffer still poison, or all bytes identical. */
    DEMO_FAIL_SEQ,              /*!< Sequence gapped or buffer index did not rotate. */
    DEMO_FAIL_STATS,            /*!< A counter that should be 0 is not. */
    DEMO_FAIL_NO_VSYNC,         /*!< No VSYNC seen: wiring, polarity or clock. */
    DEMO_FAIL_LINE_COUNT,       /*!< Measured line count disagrees with the config. */
    DEMO_FAIL_STATE,            /*!< Driver state machine did not go where expected. */
} dci_demo_result_t;

/*============================================================================*
 *                         Interrupt counters
 *============================================================================*/
/*
 * Shared by every capture case, because a frame that looks right while the sync count
 * drifts is the fault these counters exist to expose.
 *
 * All of them are written in interrupt context and read from the thread, hence volatile.
 * Nothing here is printed from an ISR: LINE alone is around 28,000 interrupts per second
 * at 466 lines and 60 fps, and a print inside that handler causes the overrun it would
 * be reporting. The ISR accumulates; the frame handler prints one line.
 */
typedef struct
{
    volatile uint32_t vsync;        /*!< VSYNC interrupts since reset. Cumulative. */
    volatile uint32_t line;         /*!< LINE interrupts in the frame in progress. */
    volatile uint32_t line_total;   /*!< LINE interrupts since reset. */
    volatile uint32_t line_prev;    /*!< Lines counted in the frame just finished. */
    volatile uint32_t frame;        /*!< FRAME interrupts. Always enabled. */
    volatile uint32_t ovr;          /*!< Overruns reported through frame_error. */
    volatile uint32_t err;          /*!< Sync errors reported through frame_error. */
    volatile uint32_t ready;        /*!< Frames delivered to frame_ready. */
    volatile uint32_t last_seq;     /*!< seq of the last delivered frame. */
    volatile DCI_ERROR last_err;    /*!< Last error code seen. */
    volatile uint32_t last_ris;     /*!< RIS snapshot at that error. */
} dci_demo_irq_t;

/** \brief  The counters. One instance, owned by dci_demo_irq.c. */
extern dci_demo_irq_t dci_demo_irq;

/** \brief  Zero the counters. Call before arming, never while capturing. */
void dci_demo_irq_reset(void);

/**
 * \brief   Install the event callbacks the enabled macros ask for.
 *
 * VSYNC and LINE are enabled in IER by the driver if and only if the matching pointer is
 * non-NULL, so this is the register-level enable - there is no separate masking step, and
 * the driver is not modified to provide one.
 *
 * Must be called before DCI_Capture_Start(): Start() re-assigns IER from these pointers
 * just before enabling the peripheral, so anything set afterwards is overwritten.
 */
void dci_demo_irq_install(DCI_DMA_config *dma);

/** \brief  Print the counters. Safe from thread context only. */
void dci_demo_irq_report(void);

/**
 * \brief   Keep the nth delivered frame in its buffer instead of releasing it.
 *
 * Every other frame is released from the delivery callback so capture keeps running.
 * Frame n is left locked, which under the GATE policy stops capture on its own once the
 * ring comes back round to that buffer - so the data survives without the thread having
 * to win a race against the DMA.
 *
 * \param[in] n  1-based frame number, or 0 to release every frame.
 */
void dci_demo_irq_hold_frame(uint32_t n);

/** \brief  True once the held frame has been delivered. */
bool dci_demo_irq_held(void);

/** \brief  The held frame, or NULL if nothing is being held yet. */
const DCI_frame_t *dci_demo_irq_held_frame(void);

/**
 * \brief   Frame delivery callback. ISR context.
 *
 * The per-frame interrupt hook: counts the frame, latches the line count of the frame
 * that just ended, optionally prints one line, and releases the buffer unless it is the
 * one dci_demo_irq_hold_frame() asked to keep.
 *
 * Only reached under DCI_NOTIFY_CALLBACK - in POLL mode the driver never calls it, by
 * design, so a polling case has to count frames for itself.
 */
void dci_demo_on_frame_ready(const DCI_frame_t *frame);

/**
 * \brief   Error callback: counts and logs. ISR context, always installed.
 *
 * Overrun and sync errors always log, whatever DCI_DEMO_IRQ_LOG says. They are rare by
 * construction - one per lost frame at worst - and an overrun that passes silently is
 * the most expensive fault in this peripheral to track down afterwards.
 */
void dci_demo_on_frame_error(DCI_ERROR err, uint32_t ris);

/*============================================================================*
 *                         Entry point
 *============================================================================*/
/**
 * \brief   Run the case selected in dci_demo_config.h.
 *
 * Prints what it is about to do, runs it, and prints the verdict. On failure the
 * reason is logged along with a register dump, if DCI_DEBUG_REG_DUMP is enabled.
 *
 * \return  DEMO_PASS, or the first failure encountered.
 */
dci_demo_result_t dci_demo_run(void);

/** \brief  Result name for logs. */
const char *dci_demo_result_name(dci_demo_result_t r);

#ifdef __cplusplus
}
#endif

#endif /* DCI_DEMO_H */
