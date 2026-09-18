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
 * To run something else: change DCI_DEMO_CASE / DCI_DEMO_PRESET, rebuild, reflash.
 */

#ifndef DCI_DEMO_CONFIG_H
#define DCI_DEMO_CONFIG_H

#include "dci_demo_id.h"

/*============================================================================*
 *                    1. Which test case                    <-- EDIT
 *============================================================================*/
/*
 * See dci_demo_id.h for the list. Suggested order for a new board:
 *
 *   REG_TEST      -> registers reachable            (no camera needed)
 *   PRESET_DUMP   -> preset table sanity            (no camera needed)
 *   CONFIG_ONLY   -> configuration accepted         (no camera needed)
 *   PROBE_SIGNAL  -> camera clocking, line count    (camera, no DMA)
 *   SNAPSHOT      -> one frame end to end
 *   SNAPSHOT_X2   -> double buffer rotation
 *   CONTINUOUS    -> closed ring, rendezvous, gating
 *   STRESS        -> long run
 */
/* #ifndef so a build system can override with -DDCI_DEMO_CASE=... without editing
 * this file - useful for a CI job that compiles every case in turn. */
#ifndef DCI_DEMO_CASE
#define DCI_DEMO_CASE               DCI_DEMO_CASE_REG_TEST
#endif

/*============================================================================*
 *                    2. Which preset                       <-- EDIT
 *============================================================================*/
/*
 * Ignored by REG_TEST and PRESET_DUMP. See dci_demo_id.h for the list and
 * dci_demo_preset.c for what each row proves.
 */
#ifndef DCI_DEMO_PRESET
#define DCI_DEMO_PRESET             DCI_DEMO_PRESET_QVGA_RGB565
#endif

/*============================================================================*
 *                    3. Buffers                            <-- EDIT
 *============================================================================*/
/*
 * 1 or 2. Two buffers give the consumer a full frame period of slack; one buffer
 * halves the RAM and is the only option for the largest presets at a 1 MB budget.
 *
 * Some cases override this because the behaviour under test requires it:
 * SNAPSHOT_X2 needs 2 (it checks rotation), CONTINUOUS_1BUF needs 1.
 */
#ifndef DCI_DEMO_BUF_COUNT
#define DCI_DEMO_BUF_COUNT          2
#endif

/*============================================================================*
 *                    RAM budget
 *============================================================================*/
/*
 * Total RAM available to the demo is 1 MB. The pool is a single static arena carved
 * into buffers at run time; whatever is left of the 1 MB covers stack, globals and
 * the driver's descriptor ring (1320 bytes at DCI_MAX_CHUNK = 33).
 *
 * A preset that does not fit is reported as DEMO_FAIL_POOL with the exact shortfall -
 * never silently truncated, because a short buffer would look like a capture fault.
 *
 * Run the PRESET_DUMP case to see the requirement of every preset at 1x and 2x.
 */
#define DCI_DEMO_RAM_BUDGET_KB      1024

#ifndef DCI_DEMO_POOL_KB
#define DCI_DEMO_POOL_KB            960
#endif

/*
 * Where the pool lives. Left empty so it lands in the default RW section; point it at
 * a placement section if 960 KB will not fit there.
 *
 *   #define DCI_DEMO_SECTION  BUFFER_RAM_DATA_SECTION
 */
#ifndef DCI_DEMO_SECTION
#define DCI_DEMO_SECTION
#endif

/*============================================================================*
 *                    Run-time knobs
 *============================================================================*/
/** \brief  JPEG capacity cap: a variable-length frame has no computable size. */
#ifndef DCI_DEMO_JPEG_CAP_KB
#define DCI_DEMO_JPEG_CAP_KB        64
#endif

/** \brief  Frames to collect in the CONTINUOUS case. */
#ifndef DCI_DEMO_FRAMES
#define DCI_DEMO_FRAMES             8
#endif

/** \brief  Frames for the STRESS case. */
#ifndef DCI_DEMO_STRESS_FRAMES
#define DCI_DEMO_STRESS_FRAMES      1000
#endif

/** \brief  How long to wait for one frame. */
#ifndef DCI_DEMO_TIMEOUT_MS
#define DCI_DEMO_TIMEOUT_MS         3000
#endif

/**
 * \brief   Frame reporting detail.
 *
 * 0 = summary + CRC32, 1 = plus sampled bytes, 2 = full hex.
 *
 * Levels above 0 are forced to 0 by the continuous and stress cases: a 424 KB frame
 * is roughly 7 s of hex at 1 Mbps, and printing that between frames causes the very
 * overruns those cases exist to detect.
 */
#ifndef DCI_DEMO_DUMP_LEVEL
#define DCI_DEMO_DUMP_LEVEL         1
#endif

/*============================================================================*
 *                    Logging
 *============================================================================*/
#ifndef DEMO_LOG
#include "trace.h"
#define DEMO_LOG(...)               DBG_DIRECT(__VA_ARGS__)
#endif

/*============================================================================*
 *                    Consistency checks
 *============================================================================*/
#if (DCI_DEMO_CASE < 0) || (DCI_DEMO_CASE >= DCI_DEMO_CASE_MAX)
#error "DCI_DEMO_CASE is not a valid case - see dci_demo_id.h"
#endif

#if (DCI_DEMO_PRESET < 0) || (DCI_DEMO_PRESET >= DCI_DEMO_PRESET_MAX)
#error "DCI_DEMO_PRESET is not a valid preset - see dci_demo_id.h"
#endif

#if (DCI_DEMO_BUF_COUNT < 1) || (DCI_DEMO_BUF_COUNT > 2)
#error "DCI_DEMO_BUF_COUNT must be 1 or 2"
#endif

#if (DCI_DEMO_POOL_KB > DCI_DEMO_RAM_BUDGET_KB)
#error "DCI_DEMO_POOL_KB exceeds the 1 MB RAM budget"
#endif

#if (DCI_DEMO_DUMP_LEVEL > 2)
#error "DCI_DEMO_DUMP_LEVEL must be 0, 1 or 2"
#endif

#endif /* DCI_DEMO_CONFIG_H */
