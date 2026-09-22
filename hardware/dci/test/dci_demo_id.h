/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DCI demo: case, signal-row and overlay identifiers.
 *
 * Separated from dci_demo.h so dci_demo_config.h can name a case without pulling in
 * the driver headers - which keeps the config file readable as pure configuration.
 *
 * Plain #defines rather than an enum: these are compared in #if, and the preprocessor
 * cannot see enum values. The cost is that nothing but a _Static_assert in
 * dci_demo_signal.c keeps the numbers in step with the table.
 */

#ifndef DCI_DEMO_ID_H
#define DCI_DEMO_ID_H

/*============================================================================*
 *                         Test cases
 *============================================================================*/
/*
 * Three cases. Everything that used to be a case but is really a SETTING is now a macro
 * in dci_demo_config.h, because a case should be a different procedure, not the same
 * procedure with a different number in it:
 *
 *   snapshot-x2      -> DCI_DEMO_SHOTS = 2
 *   continuous-1buf  -> DCI_DEMO_BUF_COUNT = 1, DCI_DEMO_FILL_MODE = OVERWRITE
 *   stress           -> DCI_DEMO_FRAMES = 1000
 *   config-only      -> DCI_DEMO_NO_CAPTURE = 1
 *   receive          -> snapshot already leaves the frame in RAM: CAPTURE self-clears
 *                       at frame end (spec 2.5.1), so the data is static the moment
 *                       WaitFrame() returns. Both capture cases now print the buffer
 *                       address and the JLink savebin line after every frame, so a
 *                       dump needs no special case at all.
 *
 * Removed with no macro equivalent, because they called driver APIs no configuration
 * can reach: signal-dump (table print), probe-signal (DCI_Cmd without CAPTURE, the only
 * way to measure the sensor's real line count), reconfig (DCI_Capture_Reconfig mid-run),
 * suspend (DCI_Capture_Suspend/Resume) and raw10 (per-sample 10-bit assertions plus two
 * mutual-exclusion refusals). See TESTMATRIX.md for what that gave up.
 */
#define DCI_DEMO_CASE_REG_TEST          0   /*!< Register read/write self test. */
#define DCI_DEMO_CASE_SNAPSHOT          1   /*!< N shots, fully checked. Camera. */
#define DCI_DEMO_CASE_CONTINUOUS        2   /*!< N frames, closed ring. Camera. */

#define DCI_DEMO_CASE_MAX               3

/*============================================================================*
 *                         Signal rows
 *============================================================================*/
/*
 * What arrives on the pins. One row per distinct receiver configuration, mirroring the
 * camera-side preset table (note/preset-table.md) with the differences this peripheral
 * cannot see folded away - see dci_demo_signal.c for exactly which camera rows collapse
 * into which row here, and why.
 *
 * Ordered safest first, each row introducing one new variable relative to something
 * above it. Work down the list and a failure at row N with 0..N-1 passing localises the
 * fault to what row N added.
 *
 * Crop, line/byte select and frame rate control are NOT rows - they are the overlay
 * below, applicable to any row they do not contradict. That is the change from the old
 * 21-row preset table, where every feature combination needed its own row.
 *
 * The RAM columns are the arena needed at 1 / 2 buffers against a 960 KB pool, with the
 * overlay off. A cropping overlay does not shrink them: DCI_Capture_Init() validates
 * the uncropped frame because the window is only programmed afterwards.
 */
/*                                        frame    1 buf    2 buf                   */
#define DCI_DEMO_SIG_NULL_MIN           0   /*  38 KB   38 KB    75 KB  empty config */
#define DCI_DEMO_SIG_QQVGA_RGB565       1   /*  38 KB   38 KB    75 KB               */
#define DCI_DEMO_SIG_QVGA_RGB565        2   /* 150 KB  150 KB   300 KB  reference    */
/* --- polarity, one bit at a time --- */
#define DCI_DEMO_SIG_QVGA_PCK_RISING    3   /* 150 KB  150 KB   300 KB               */
#define DCI_DEMO_SIG_QVGA_HREF_HIGH     4   /* 150 KB  150 KB   300 KB               */
#define DCI_DEMO_SIG_QVGA_VSYNC_HIGH    5   /* 150 KB  150 KB   300 KB               */
#define DCI_DEMO_SIG_QVGA_ALL_INVERTED  6   /* 150 KB  150 KB   300 KB  coupling     */
/* --- formats --- */
#define DCI_DEMO_SIG_QVGA_RAW8          7   /*  75 KB   75 KB   150 KB  1 B/pixel    */
#define DCI_DEMO_SIG_QVGA_RAW10         8   /* 150 KB  150 KB   300 KB  EDM=10BIT    */
#define DCI_DEMO_SIG_QVGA_EMBEDDED      9   /* 150 KB  150 KB   300 KB  ESS          */
#define DCI_DEMO_SIG_QVGA_JPEG          10  /*  64 KB cap               variable     */
/* --- size --- */
#define DCI_DEMO_SIG_VGA_RGB565         11  /* 600 KB  600 KB    -- !   3 blk        */
/* --- geometry only, no camera stimulus --- */
#define DCI_DEMO_SIG_466_RAW8           12  /* 212 KB  212 KB   424 KB  unaligned    */

#define DCI_DEMO_SIGNAL_MAX             13

/*
 * ! VGA_RGB565 exceeds the pool at 2 buffers. Set DCI_DEMO_BUF_COUNT = 1 for it, or the
 *   case reports DEMO_FAIL_POOL with the shortfall. Single buffer plus OVERWRITE is the
 *   intended configuration at that size anyway (DESIGN.md 6.8).
 *
 *   XGA (1024x768 RGB565, 1572 KB) does not fit even single-buffered, so there is no
 *   row for it: a row that always fails is noise.
 *
 * Row 12 has no camera stimulus - the OV2640 cannot emit that geometry (no sensor emits
 * an odd pixel width). It exists because the receiver-side behaviour it triggers has to
 * be covered: the word-aligned fallback split, which no other row reaches. Against a
 * live camera it fails on frame length, which is expected; its value is at config time
 * and against a pattern generator.
 *
 * Removed by request: 466_RGB565 (2 blocks, double-buffer ceiling), SVGA_RGB565
 * (4 blocks, pool ceiling) and VGA_RAW10 (same bytes as VGA_RGB565 at half the
 * pclk/line). The table now has 1- and 3-block rows only, so the 2- and 4-block split
 * paths have no coverage, and RAW10 rests on QVGA_RAW10 alone. See TESTMATRIX.md.
 */

/*============================================================================*
 *                         Overlay: crop
 *============================================================================*/
/*
 * Window position and size as a fraction of the row rather than in absolute pixels: the
 * table spans 160x120 to 800x600, so one absolute window would be out of bounds at one
 * end and the whole frame at the other. A fraction is valid on every row.
 */
#define DCI_DEMO_CROP_OFF               0   /*!< No window. CWSTRT/CWSIZE untouched. */
#define DCI_DEMO_CROP_CENTER            1   /*!< Half in each axis, centred. */
#define DCI_DEMO_CROP_ORIGIN            2   /*!< A fifth in each axis at 0,0. */
#define DCI_DEMO_CROP_FULL              3   /*!< Whole frame, CROP enabled. */

#define DCI_DEMO_CROP_MAX               4

/*
 * CROP_FULL looks pointless and is the most useful of the three: it must be byte for
 * byte identical to CROP_OFF. If it is not, enabling CR.crop is itself broken, which is
 * a different fault from a wrong window and is otherwise indistinguishable from one.
 *
 * CROP_ORIGIN covers the x0 = y0 = 0 boundary, where an off-by-one in the N-1 window
 * encoding (capcnt = w-1, vline = h-1) is visible; a centred window hides it.
 *
 * Crop is refused with JPEG (spec 2.7) and the width is aligned down to a multiple of
 * 4 pclk by DCI_ConfigCrop() - the resolver pre-computes that alignment so the printed
 * configuration is what the hardware ends up with, not what was asked for.
 */

/*============================================================================*
 *                         Buffer fill policy
 *============================================================================*/
/*
 * What happens when the next buffer to write is still held by the consumer. Mirrors
 * DCI_ON_BUSY_T; defined here as well so dci_demo_config.h can name it without
 * including the driver header.
 */
#define DCI_DEMO_FILL_GATE              0   /*!< Clear CAPTURE and wait. Never tears. */
#define DCI_DEMO_FILL_OVERWRITE         1   /*!< Keep writing. Full rate, may tear. */

#define DCI_DEMO_FILL_MAX               2

#endif /* DCI_DEMO_ID_H */
