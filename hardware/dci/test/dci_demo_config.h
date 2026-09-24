/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DCI demo: build configuration. THIS IS THE ONLY FILE YOU EDIT.
 *
 * Exactly one test case is compiled per build. That is deliberate rather than a
 * limitation:
 *
 *   - The RAM budget is 1 MB total and a framebuffer pool is most of it. Only the
 *     selected case's buffers exist, so nothing is reserved for cases not being run.
 *   - A case cannot be reached by accident. What was flashed is what runs.
 *   - Cases not selected are not compiled, so a half-finished case cannot break the
 *     build of the one under test.
 *
 * A test is three independent choices:
 *
 *   1. the CASE      - what procedure to run
 *   2. the SIGNAL    - what arrives on the pins (must match how the camera is set up)
 *   3. the OVERLAY   - what this machine then does with it: crop, select, frame rate
 *
 * They multiply: 13 cases x 16 signal rows x the overlay settings, from 3 lines of
 * configuration and no new code. Illegal combinations are refused with the name of the
 * macro to change, either at compile time below or by dci_demo_cfg_resolve().
 *
 * To run something else: change the macros, rebuild, reflash.
 */

#ifndef DCI_DEMO_CONFIG_H
#define DCI_DEMO_CONFIG_H

#include "dci_demo_id.h"

/*============================================================================*
 *                    1. Which test case                    <-- EDIT
 *============================================================================*/
/*
 * Three cases only - see dci_demo_id.h. Suggested order for a new board:
 *
 *   REG_TEST                      -> registers reachable      (no camera needed)
 *   SNAPSHOT + DCI_DEMO_NO_CAPTURE -> configuration accepted   (no camera needed)
 *   SNAPSHOT                      -> one frame, every check, address printed for JLink
 *   SNAPSHOT + SHOTS = 2          -> double buffer rotation
 *   CONTINUOUS                    -> closed ring, rendezvous, gating
 *   CONTINUOUS + FRAMES = 1000    -> long run
 *
 * Both capture cases print the buffer address and a ready-made JLink savebin line after
 * every frame, so dumping data needs no special case: build SNAPSHOT, break at the end,
 * and run the printed command. SNAPSHOT is the one to use for that - the frame is static
 * as soon as it is delivered, because CAPTURE self-clears at frame end (spec 2.5.1).
 */
/* #ifndef so a build system can override with -DDCI_DEMO_CASE=... without editing
 * this file - useful for a CI job that compiles every case in turn. */
#ifndef DCI_DEMO_CASE
#define DCI_DEMO_CASE               DCI_DEMO_CASE_SNAPSHOT
#endif

/*============================================================================*
 *                    2. Which signal                       <-- EDIT
 *============================================================================*/
/*
 * Must match how the camera board is configured. Every row in dci_demo_signal.c names
 * the camera-side preset that produces it, so the pair is always explicit.
 *
 * Ignored by REG_TEST and SIGNAL_DUMP.
 *
 * DCI_DEMO_SIG_NULL_MIN is the empty configuration: DCI_StructInit() defaults and
 * nothing else - no format, no polarity, no feature. Start there when bringing up a new
 * board, because it is the only row where a failure cannot be a configuration mistake.
 */
#ifndef DCI_DEMO_SIGNAL
#define DCI_DEMO_SIGNAL             DCI_DEMO_SIG_QQVGA_RGB565
#endif

/*============================================================================*
 *                    3. Overlay                            <-- EDIT
 *============================================================================*/
/*
 * Receiver-side features, applied on top of whichever signal row is selected. All
 * default to off, so the default build receives the signal exactly as it arrives - add
 * one at a time, for the same reason the signal rows are ordered the way they are.
 */

/** \brief  Crop window: DCI_DEMO_CROP_OFF / _CENTER / _ORIGIN / _FULL. */
#ifndef DCI_DEMO_OV_CROP
#define DCI_DEMO_OV_CROP            DCI_DEMO_CROP_OFF
#endif

/**
 * \brief   Frame rate control: 0 every frame, 1 every 2nd, 2 one of four.
 *
 * Hardware frame dropping, continuous mode only. Refused on a JPEG row, where a dropped
 * frame cannot be told apart from a truncated one.
 */
#ifndef DCI_DEMO_OV_FCRC
#define DCI_DEMO_OV_FCRC            0
#endif

/**
 * \brief   Byte select: 0 all bytes, 1 one of two, 2 one of four, 3 two of four.
 *
 * ONLY MEANINGFUL AT 1 BYTE PER PIXEL. This drops bytes, and DCI does not know what a
 * pixel is - on a 2-byte format the driver accepts it, the capture reports success, and
 * every pixel in the buffer is half a pixel. The resolver refuses that combination
 * because nothing in the hardware can.
 *
 * Usable rows: NULL_MIN and QVGA_RAW8 (320 B/line, all four settings), and 466_RAW8 at
 * setting 1 only - the group has to divide the line exactly, and 466 is not a multiple
 * of 4. The resolver names the line length if it does not.
 */
#ifndef DCI_DEMO_OV_BSM
#define DCI_DEMO_OV_BSM             0
#endif

/** \brief  With byte select on, take the odd phase instead of the even one. */
#ifndef DCI_DEMO_OV_OEBS
#define DCI_DEMO_OV_OEBS            0
#endif

/**
 * \brief   Line select: drop every other line. Format-independent - whole lines.
 */
#ifndef DCI_DEMO_OV_LSM
#define DCI_DEMO_OV_LSM             0
#endif

/** \brief  With line select on, keep the odd lines instead of the even ones. */
#ifndef DCI_DEMO_OV_OELS
#define DCI_DEMO_OV_OELS            0
#endif

/*============================================================================*
 *                    4. Buffers                            <-- EDIT
 *============================================================================*/
/*
 * 1 or 2. Two buffers give the consumer a full frame period of slack; one buffer halves
 * the RAM and is the only option for the largest rows at a 1 MB budget.
 *
 * Nothing overrides this any more - it is the setting, not a case. Two combinations are
 * worth knowing:
 *
 *   BUF_COUNT = 2 with SHOTS = 2          -> double buffer rotation (was snapshot-x2)
 *   BUF_COUNT = 1 with FILL_MODE OVERWRITE -> viewfinder mode      (was continuous-1buf)
 */
#ifndef DCI_DEMO_BUF_COUNT
#define DCI_DEMO_BUF_COUNT          1
#endif

/**
 * \brief   How many snapshots the SNAPSHOT case takes. Ignored by CONTINUOUS.
 *
 * 2 with DCI_DEMO_BUF_COUNT = 2 is the rotation check: the first frame is still held
 * while the second is taken, so a broken rotation either overwrites the held buffer or
 * reports BUSY. With one buffer there is nothing to rotate, which is why the check needs
 * both settings rather than one.
 */
#ifndef DCI_DEMO_SHOTS
#define DCI_DEMO_SHOTS              1
#endif

/**
 * \brief   Configure and dump registers, then stop without capturing. No camera needed.
 *
 * Runs the whole Capture_Init + crop + geometry cross-check path, which is every check
 * that does not need pixels. Use it to sweep signal rows and overlays on a board with no
 * sensor attached.
 */
#ifndef DCI_DEMO_NO_CAPTURE
#define DCI_DEMO_NO_CAPTURE         0
#endif

/**
 * \brief   What to do when the next buffer is still held: GATE or OVERWRITE.
 *
 * GATE clears CAPTURE and waits for the consumer, so a frame is never torn but frames
 * are skipped under load. OVERWRITE keeps writing at full rate and may hand over a
 * buffer that is half old and half new.
 *
 * Must be chosen rather than defaulted silently: with one buffer, GATE turns continuous
 * capture into "stop after every frame", which is not what a viewfinder wants. GATE is
 * the default here because a torn frame is much harder to recognise than a missing one.
 */
#ifndef DCI_DEMO_FILL_MODE
#define DCI_DEMO_FILL_MODE          DCI_DEMO_FILL_GATE
#endif

/*============================================================================*
 *                    5. Interrupts                         <-- EDIT
 *============================================================================*/
/*
 * FRAME, OVR and ERR are always enabled in IER - the driver needs them to function, so
 * they are not negotiable (dci_int_config() in rtl_dci.c). VSYNC and LINE are off by
 * default and enabled here.
 *
 * The mechanism is the driver's, unchanged: it adds IT_VSYNC / IT_LINE to IER only if
 * the matching callback pointer is non-NULL. Leaving the pointer NULL is a register-
 * level disable, so these macros gate the pointer, not a masking step afterwards.
 *
 * Why LINE is off by default: at 466 lines and 60 fps it is 28,000 interrupts per
 * second. Enabling it costs real bandwidth, and any print inside that ISR guarantees an
 * overrun - which is why the counters are accumulated in the ISR and printed at frame
 * time instead.
 */

/** \brief  Enable IT_VSYNC and count vertical syncs. */
#ifndef DCI_DEMO_IRQ_VSYNC
#define DCI_DEMO_IRQ_VSYNC          1
#endif

/** \brief  Enable IT_LINE and count lines. See the bandwidth note above. */
#ifndef DCI_DEMO_IRQ_LINE
#define DCI_DEMO_IRQ_LINE           1
#endif

/**
 * \brief   Print one line per frame interrupt: this frame's line count and the
 *          cumulative VSYNC count.
 *
 * Off by default because it prints at frame rate. The frame interrupt itself is always
 * enabled, so turning this on costs a print, not an interrupt.
 *
 * Overrun and error interrupts always log - they are rare by definition, and a silent
 * overrun is the single most expensive thing to debug in this peripheral.
 */
#ifndef DCI_DEMO_IRQ_LOG
#define DCI_DEMO_IRQ_LOG            1
#endif

/*============================================================================*
 *                    6. Which frame to keep               <-- EDIT
 *============================================================================*/
/**
 * \brief   Frame number to leave in the buffer for a JLink dump. 1 = the first.
 *
 * SNAPSHOT does not need this - a snapshot frame is static the moment it is delivered,
 * because CAPTURE self-clears at frame end (spec 2.5.1). This is for CONTINUOUS, where
 * the ring keeps writing: frames before this one are received and released normally,
 * frame N is kept, capture stops, and its address and range are printed.
 *
 * Reasons to want a later frame: the first frame after the camera starts is often
 * partial (capture begins at the next VSYNC, so a frame already in progress is missed
 * rather than half-captured, but the sensor's own AE/AWB is still settling), and a fault
 * that only appears once the ring has turned over cannot be seen in frame 1.
 *
 * 0 disables holding, so CONTINUOUS runs its full DCI_DEMO_FRAMES and tears down
 * normally. That is the right setting for a stress run.
 *
 * Default 0, not 1: this only applies to CONTINUOUS, and the consistency check below
 * rejects a non-zero value on SNAPSHOT. Defaulting to 1 would make the default SNAPSHOT
 * build fail to compile on a macro the user never set.
 */
#ifndef DCI_DEMO_CAPTURE_FRAME
#define DCI_DEMO_CAPTURE_FRAME      0
#endif

/*============================================================================*
 *                    RAM budget
 *============================================================================*/
/*
 * The pool is a single static arena carved into buffers at run time. A configuration
 * that does not fit is reported as DEMO_FAIL_POOL with the exact shortfall - never
 * silently truncated, because a short buffer would look like a capture fault.
 *
 * ############################################################################
 * # READ THIS FIRST IF THE LINKER COMPLAINS ABOUT RAM.                       #
 * #                                                                          #
 * # These two numbers describe the BOARD, not the test, and the defaults      #
 * # assume a part with 1 MB of ordinary RW RAM. Many do not have it. On the    #
 * # RTL87X3K AP ram_proj, DTCM is 128 KB total and its scatter file gives all  #
 * # of RW+ZI just DATA_RAM_VECTOR_TABLE_SIZE = 10 KB, so a 960 KB pool fails   #
 * # at LINK time, not run time:                                               #
 * #                                                                          #
 * #   L6221E: Execution region RW_SECTION ... overlaps with ARM_LIB_HEAP      #
 * #   L6388E: ScatterAssert ... failed: (0xf1580 < 0x2800)                    #
 * #                                                                          #
 * # 0xF1580 is this pool. Two ways out, and they compose:                     #
 * #                                                                          #
 * #  1. For the two cases that never touch the pool - REG_TEST, and SNAPSHOT  #
 * #     with NO_CAPTURE=1 - shrink it to nothing and link immediately:        #
 * #                                                                          #
 * #        -DDCI_DEMO_POOL_KB=1   -DDCI_DEMO_RAM_BUDGET_KB=1                  #
 * #                                                                          #
 * #     Those two are the whole no-camera part of bring-up, so this unblocks  #
 * #     real testing today.                                                   #
 * #                                                                          #
 * #  2. To CAPTURE anything, the pool must go somewhere big, via              #
 * #     DCI_DEMO_SECTION (below) plus a matching region in the scatter file.  #
 * #     Shrinking is not an option: the smallest row in the table needs        #
 * #     38 KB single-buffered, already more than the 10 KB RW+ZI budget.       #
 * #     Measured requirements, single / double buffered:                       #
 * #                                                                          #
 * #        null-min, qqvga-rgb565      38 KB  /  76 KB                        #
 * #        qvga-jpeg (64 KB cap)       65 KB  / 131 KB                        #
 * #        qvga-raw8                   76 KB  / 150 KB                        #
 * #        qvga-rgb565 and friends    150 KB  / 300 KB                        #
 * #        466-raw8                   212 KB  / 424 KB                        #
 * #        vga-rgb565                 600 KB  / 1200 KB                       #
 * ############################################################################
 *
 * Whatever is left of the budget covers stack, globals and the driver's descriptor
 * ring (1320 bytes at DCI_MAX_CHUNK = 33).
 */
/*
 * Defaults sized for the RTL87X3K AP ram_proj bring-up: 40 KB in its own DTCM region,
 * which is what 160x120 RGB565 single-buffered needs (38 432 B measured) with a little
 * slack. Raise to 80 for double buffering at that size (76 848 B), and see the table
 * above for the larger rows - those need a region outside DTCM, which is only 128 KB.
 */
#ifndef DCI_DEMO_RAM_BUDGET_KB
#define DCI_DEMO_RAM_BUDGET_KB      40
#endif

#ifndef DCI_DEMO_POOL_KB
#define DCI_DEMO_POOL_KB            40
#endif

/*
 * Where the pool lives. Placed in its own section rather than the default RW one,
 * because RW+ZI on this part is capped at 10 KB by the scatter file and the pool does
 * not fit - see the box above.
 *
 * THIS NEEDS A MATCHING REGION IN THE LINKER SCRIPT. Two edits, both in
 * soc/ram_proj/keil_proj/ap:
 *
 *   1. inc/mem_config.h - carve the pool off the front of the heap:
 *
 *        #define DCI_POOL_SIZE   (40*1024)
 *        #define DCI_POOL_ADDR   (DATA_RAM_VECTOR_TABLE_ADDR + DATA_RAM_VECTOR_TABLE_SIZE)
 *
 *      and change DATA_HEAP_ADDR to start after it:
 *
 *        #define DATA_HEAP_ADDR  (DCI_POOL_ADDR + DCI_POOL_SIZE)
 *
 *      DATA_HEAP_SIZE is computed from DATA_HEAP_ADDR, so it shrinks on its own
 *      (118 KB -> 78 KB at a 40 KB pool).
 *
 *   2. app_rom.sct - its OWN execution region, after the RW_SECTION ScatterAssert and
 *      before ARM_LIB_HEAP. It must not land inside RW_SECTION or that assert catches
 *      it again:
 *
 *        DCI_POOL DCI_POOL_ADDR DCI_POOL_SIZE
 *        {
 *            * (.dci_pool)
 *        }
 *
 *      (app_ram.sct has no heap region, so there only the DCI_POOL region is needed.)
 *
 * Nothing in the driver or the demo cares where the pool is, as long as the DMA can
 * write it: the buffer addresses are handed to the driver at run time. So an integrator
 * who has somewhere better - SRAM, PSRAM, the DSP shared RAM - only has to change the
 * section name and the region.
 */
#ifndef DCI_DEMO_SECTION
#define DCI_DEMO_SECTION            __attribute__((section(".dci_pool")))
#endif

/*============================================================================*
 *                    Run-time knobs
 *============================================================================*/
/** \brief  JPEG capacity cap: a variable-length frame has no computable size. */
#ifndef DCI_DEMO_JPEG_CAP_KB
#define DCI_DEMO_JPEG_CAP_KB        64
#endif

/**
 * \brief   Frames to collect in the CONTINUOUS case.
 *
 * 1000 or so is the stress setting - a long run finds what a short one cannot: guards
 * nibbled progressively, suspend_count climbing slowly because the consumer is a little
 * too slow, a resync every few hundred frames.
 */
#ifndef DCI_DEMO_FRAMES
#define DCI_DEMO_FRAMES             8
#endif

/**
 * \brief   How long to wait for one frame.
 *
 * NOT MILLISECONDS ON THE WIRE. DCI_Capture_WaitFrame() has no clock; it spins
 * timeout_ms * DCI_WAIT_ITER_PER_MS times, and that constant (20000, in rtl_dci.c) is a
 * guess at how many polls fit in a millisecond. Each poll takes a critical section and
 * several register reads, so on this part the default 3000 works out to 60 million polls
 * - which can be a minute or more of apparent hang, not three seconds.
 *
 * So when nothing arrives, the symptom is a log that simply stops after the geometry
 * line. If that happens, shorten the wait rather than waiting it out:
 *
 *     -DDCI_DEMO_TIMEOUT_MS=50
 *
 * 50 is still ~3 frame periods at 60 fps, i.e. long enough for a frame that is coming,
 * and short enough that a frame that is not coming reports it in about a second.
 * DCI_WAIT_ITER_PER_MS itself is overridable too if the ratio needs calibrating.
 */
#ifndef DCI_DEMO_TIMEOUT_MS
#define DCI_DEMO_TIMEOUT_MS         3000
#endif

/**
 * \brief   How many times dci_demo_sync_probe() samples SR.
 *
 * This is a CEILING, not a fixed cost: the probe stops as soon as it has seen both sync
 * inputs toggle, because at that point the verdict can no longer change. Only a board
 * that produces nothing pays the whole budget, and that is the case where a long window
 * is the point.
 *
 * Must cover MORE THAN ONE FRAME PERIOD or a VSYNC edge can be missed and the probe
 * reports "VSYNC never toggled" for a healthy signal. The loop body is a handful of
 * register reads - call it ~20 cycles, so at a few tens of MHz the default 200 000 is
 * comfortably tens of milliseconds, i.e. several frames at any sane frame rate.
 */
#ifndef DCI_DEMO_SYNC_PROBE_SAMPLES
#define DCI_DEMO_SYNC_PROBE_SAMPLES 200000
#endif

/**
 * \brief   Frame reporting detail.
 *
 * 0 = summary + CRC32, 1 = plus sampled bytes, 2 = full hex.
 *
 * Levels above 0 are forced to 0 by the continuous and stress cases: a 424 KB frame
 * is roughly 7 s of hex at 1 Mbps, and printing that between frames causes the very
 * overruns those cases exist to detect.
 *
 * The RECEIVE case ignores this entirely - its output is a buffer address for JLink, not
 * a hex dump over the UART.
 */
#ifndef DCI_DEMO_DUMP_LEVEL
#define DCI_DEMO_DUMP_LEVEL         1
#endif

/*============================================================================*
 *                    Logging
 *============================================================================*/
/*
 * soc_log.h, not trace.h: on RTL87X3K the logging header is
 * kernel/logging/inc/rtl87x3k/soc_log.h, and there is no rtl87x3k trace.h at all - that
 * name belongs to the older parts. The peripheral samples in soc/ram_proj do the same.
 *
 * Define DEMO_LOG before including this file to send the output somewhere else.
 */
#ifndef DEMO_LOG
#include "soc_log.h"
#define DEMO_LOG(...)               DBG_DIRECT(__VA_ARGS__)
#endif

/*============================================================================*
 *                    Consistency checks
 *============================================================================*/
/*
 * Everything checkable without the table is checked here, because a compile error names
 * the macro and a run-time failure costs a flash cycle. What needs the table - "byte
 * select on a 2-byte format", "crop with JPEG" - is checked by dci_demo_cfg_resolve()
 * instead, which can name the row as well as the macro.
 */
#if (DCI_DEMO_CASE < 0) || (DCI_DEMO_CASE >= DCI_DEMO_CASE_MAX)
#error "DCI_DEMO_CASE is not a valid case - see dci_demo_id.h"
#endif

#if (DCI_DEMO_SIGNAL < 0) || (DCI_DEMO_SIGNAL >= DCI_DEMO_SIGNAL_MAX)
#error "DCI_DEMO_SIGNAL is not a valid signal row - see dci_demo_id.h"
#endif

#if (DCI_DEMO_OV_CROP < 0) || (DCI_DEMO_OV_CROP >= DCI_DEMO_CROP_MAX)
#error "DCI_DEMO_OV_CROP must be DCI_DEMO_CROP_OFF/_CENTER/_ORIGIN/_FULL"
#endif

#if (DCI_DEMO_OV_FCRC < 0) || (DCI_DEMO_OV_FCRC > 2)
#error "DCI_DEMO_OV_FCRC must be 0 (all), 1 (half) or 2 (quarter)"
#endif

#if (DCI_DEMO_OV_BSM < 0) || (DCI_DEMO_OV_BSM > 3)
#error "DCI_DEMO_OV_BSM must be 0 (all), 1 (1 of 2), 2 (1 of 4) or 3 (2 of 4)"
#endif

#if (DCI_DEMO_BUF_COUNT < 1) || (DCI_DEMO_BUF_COUNT > 2)
#error "DCI_DEMO_BUF_COUNT must be 1 or 2"
#endif

#if (DCI_DEMO_FILL_MODE < 0) || (DCI_DEMO_FILL_MODE >= DCI_DEMO_FILL_MAX)
#error "DCI_DEMO_FILL_MODE must be DCI_DEMO_FILL_GATE or DCI_DEMO_FILL_OVERWRITE"
#endif

#if (DCI_DEMO_CAPTURE_FRAME < 0)
#error "DCI_DEMO_CAPTURE_FRAME is a frame number counting from 1, or 0 to hold none"
#endif

#if (DCI_DEMO_SHOTS < 1) || (DCI_DEMO_SHOTS > 2)
#error "DCI_DEMO_SHOTS must be 1 or 2"
#endif

/* Rotation needs somewhere to rotate to. Two shots into one buffer is not a weaker test,
 * it is a different one - and it would silently look like a passing rotation check. */
#if (DCI_DEMO_SHOTS > 1) && (DCI_DEMO_BUF_COUNT < 2)
#error "DCI_DEMO_SHOTS = 2 is the rotation check - it needs DCI_DEMO_BUF_COUNT = 2"
#endif

/* Holding a frame is a continuous-mode notion: in snapshot mode capture has already
 * stopped by the time the frame is delivered, so every snapshot frame is held. */
#if (DCI_DEMO_CAPTURE_FRAME != 0) && (DCI_DEMO_CASE != DCI_DEMO_CASE_CONTINUOUS)
#error "DCI_DEMO_CAPTURE_FRAME applies to CONTINUOUS - a snapshot frame is already static"
#endif

/* OVERWRITE with one buffer means the ring's next write target IS the held buffer, and
 * OVERWRITE by definition does not stop for it. The address would point at data that is
 * still moving, which is worse than refusing. */
#if (DCI_DEMO_CAPTURE_FRAME != 0) && (DCI_DEMO_BUF_COUNT == 1) && \
    (DCI_DEMO_FILL_MODE == DCI_DEMO_FILL_OVERWRITE)
#error "holding a frame under OVERWRITE with 1 buffer cannot work - use GATE or 2 buffers"
#endif

#if (DCI_DEMO_POOL_KB > DCI_DEMO_RAM_BUDGET_KB)
#error "DCI_DEMO_POOL_KB exceeds the 1 MB RAM budget"
#endif

#if (DCI_DEMO_DUMP_LEVEL > 2)
#error "DCI_DEMO_DUMP_LEVEL must be 0, 1 or 2"
#endif

/* At least 8 because the probe's heartbeat divides the budget into 8 intervals, and its
 * duty cycle computes active * 100 in 32 bits - so the budget also has to stay under
 * 2^32/100 to not wrap. A window this small is useless anyway; the real floor is "longer
 * than one frame period", which the comment on the macro explains. */
#if (DCI_DEMO_SYNC_PROBE_SAMPLES < 8)
#error "DCI_DEMO_SYNC_PROBE_SAMPLES must be at least 8, and should span over a frame"
#endif
#if (DCI_DEMO_SYNC_PROBE_SAMPLES > 42000000)
#error "DCI_DEMO_SYNC_PROBE_SAMPLES over 42,000,000 overflows the duty-cycle arithmetic"
#endif

/* The odd-phase bits select between two halves of a decimation that is not happening.
 * Harmless, but it always means the wrong macro was set. */
#if (DCI_DEMO_OV_OEBS != 0) && (DCI_DEMO_OV_BSM == 0)
#error "DCI_DEMO_OV_OEBS has no meaning without DCI_DEMO_OV_BSM"
#endif

#if (DCI_DEMO_OV_OELS != 0) && (DCI_DEMO_OV_LSM == 0)
#error "DCI_DEMO_OV_OELS has no meaning without DCI_DEMO_OV_LSM"
#endif

/* Frame rate control only exists in continuous mode (spec 2.5): in snapshot mode the
 * hardware takes the next frame regardless, so the setting would be silently ignored -
 * and a silently ignored configuration is worse than a build error. */
#if (DCI_DEMO_OV_FCRC != 0) && (DCI_DEMO_CASE == DCI_DEMO_CASE_SNAPSHOT)
#error "DCI_DEMO_OV_FCRC applies to continuous mode only - this case uses snapshot"
#endif

#endif /* DCI_DEMO_CONFIG_H */
