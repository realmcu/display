/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DCI demo: case and preset identifiers.
 *
 * Separated from dci_demo.h so dci_demo_config.h can name a case without pulling in
 * the driver headers - which keeps the config file readable as pure configuration.
 *
 * Plain #defines rather than an enum: these are compared in #if, and the preprocessor
 * cannot see enum values.
 */

#ifndef DCI_DEMO_ID_H
#define DCI_DEMO_ID_H

/*============================================================================*
 *                         Test cases
 *============================================================================*/
/* --- no camera signal needed --- */
#define DCI_DEMO_CASE_REG_TEST          0   /*!< Register read/write self test. */
#define DCI_DEMO_CASE_PRESET_DUMP       1   /*!< Print presets and their RAM needs. */
#define DCI_DEMO_CASE_CONFIG_ONLY       2   /*!< Apply a preset, dump, do not capture. */

/* --- camera required --- */
#define DCI_DEMO_CASE_PROBE_SIGNAL      3   /*!< Count VSYNC/LINE; no DMA. */
#define DCI_DEMO_CASE_SNAPSHOT          4   /*!< One frame. */
#define DCI_DEMO_CASE_SNAPSHOT_X2       5   /*!< Two shots; forces 2 buffers. */
#define DCI_DEMO_CASE_CONTINUOUS        6   /*!< N frames, gated. */
#define DCI_DEMO_CASE_CONTINUOUS_1BUF   7   /*!< Overwrite mode; forces 1 buffer. */
#define DCI_DEMO_CASE_RECONFIG          8   /*!< Change preset while running. */
#define DCI_DEMO_CASE_SUSPEND           9   /*!< Suspend / Resume back-pressure. */
#define DCI_DEMO_CASE_STRESS            10  /*!< Long run; watch stats and guards. */
#define DCI_DEMO_CASE_RAW10             11  /*!< EDM=10BIT packing, per sample. */

#define DCI_DEMO_CASE_MAX               12

/*============================================================================*
 *                         Presets
 *============================================================================*/
/*
 * Ordered easiest to hardest. Working down the list localises a failure: if row N
 * fails and rows 0..N-1 passed, the cause is whatever row N adds.
 *
 * The RAM column is the arena needed at DCI_DEMO_BUF_COUNT = 1 / 2, against a 960 KB
 * pool. A cropping preset needs the *uncropped* size because DCI_Capture_Init()
 * validates before the window is programmed.
 */
/*
 * RGB565 and JPEG are the formats under test; the RAW rows are deliberately few, and
 * each is one that cannot be re-expressed in RGB565 - see the comment block after the
 * list for which property each one is the only carrier of.
 */
/*                                            frame    1 buf    2 buf        */
#define DCI_DEMO_PRESET_QQVGA_RGB565    0   /*  38 KB   38 KB    75 KB        */
#define DCI_DEMO_PRESET_QVGA_RGB565     1   /* 150 KB  150 KB   300 KB        */
#define DCI_DEMO_PRESET_QVGA_PCK_RISING 2   /* 150 KB  150 KB   300 KB        */
#define DCI_DEMO_PRESET_QVGA_SYNC_HIGH  3   /* 150 KB  150 KB   300 KB        */
#define DCI_DEMO_PRESET_QVGA_RAW8       4   /*  75 KB   75 KB   150 KB  raw   */
#define DCI_DEMO_PRESET_QVGA_YUV422     5   /* 150 KB  150 KB   300 KB        */
#define DCI_DEMO_PRESET_QVGA_CROP_MID   6   /*  38 KB  150 KB   300 KB  crop  */
#define DCI_DEMO_PRESET_QVGA_CROP_ORG   7   /*   6 KB  150 KB   300 KB  crop  */
#define DCI_DEMO_PRESET_QVGA_LSM        8   /*  75 KB   75 KB   150 KB        */
#define DCI_DEMO_PRESET_QVGA_BSM        9   /*  38 KB   38 KB    75 KB  raw   */
#define DCI_DEMO_PRESET_QVGA_HALF_RATE  10  /* 150 KB  150 KB   300 KB        */
/* --- 10-bit group: see the RAW10 note below --- */
#define DCI_DEMO_PRESET_QVGA_RAW10      11  /* 150 KB  150 KB   300 KB  raw10 */
#define DCI_DEMO_PRESET_RAW10_CROP      12  /*  38 KB  150 KB   300 KB  raw10 */
#define DCI_DEMO_PRESET_VGA_RAW10       13  /* 600 KB  600 KB    -- !  raw10 3blk */
#define DCI_DEMO_PRESET_RAW10_LSM       14  /*  75 KB   75 KB   150 KB  raw10 */
/* --- back to 8-bit --- */
#define DCI_DEMO_PRESET_466_RAW8        15  /* 212 KB  212 KB   424 KB  raw   */
#define DCI_DEMO_PRESET_466_RGB565      16  /* 424 KB  424 KB   848 KB  2 blk */
#define DCI_DEMO_PRESET_VGA_RGB565      17  /* 600 KB  600 KB    -- !  3 blk */
#define DCI_DEMO_PRESET_SVGA_RGB565     18  /* 938 KB  938 KB    -- !  4 blk */
#define DCI_DEMO_PRESET_JPEG            19  /*  64 KB cap                     */
#define DCI_DEMO_PRESET_QVGA_EMBEDDED   20  /*  75 KB   75 KB   150 KB  raw   */

#define DCI_DEMO_PRESET_MAX             21

/*
 * ! VGA_RAW10, VGA_RGB565 and SVGA_RGB565 exceed the pool at 2 buffers. Set
 *   DCI_DEMO_BUF_COUNT = 1 for those, or the case reports DEMO_FAIL_POOL with the
 *   shortfall. Single buffer plus OVERWRITE is the intended configuration at that
 *   size anyway (DESIGN.md 6.8).
 *
 * Why exactly these four RAW rows, given that RGB565 and JPEG are the formats under
 * test. DCI has no notion of a pixel format at all - it sees bytes delimited by
 * HSYNC/VSYNC - so "RAW-only" means "a property that cannot be reached from an RGB565
 * row", and there are only four:
 *
 *   QVGA_RAW10     EDM = 10BIT, plus the three rows after it. The one
 *   RAW10_CROP     hardware-enforced case: 1 pclk = 1 sample in 16 bits, bits 15:10
 *   VGA_RAW10      cleared. RGB565 is 2 pclk/pixel so it cannot use this mode, and
 *   RAW10_LSM      EDM changes bytes_per_pclk - the multiplier the whole geometry
 *                  derivation rests on. Also the only rows where BSM and ESS are
 *                  rejected as mutually exclusive. The padding rule is verifiable
 *                  per sample against any scene, so DCI_DEMO_CASE_RAW10 checks it
 *                  directly; the four rows isolate the packing rule, crop in pclk
 *                  units, multi-block at 2 bytes/pclk, and decimation.
 *   QVGA_BSM       BSM drops bytes, which only equals "drop pixels" at 1 byte/pixel.
 *                  The driver accepts bsm + RGB565 (also EDM_8BIT) and captures
 *                  cleanly while corrupting every pixel - this table is the only
 *                  guard, so the row has to stay where the setting is meaningful.
 *   466_RAW8       The only row reaching the word-aligned fallback: a line is whole
 *                  words unless its byte count is not a multiple of 4, which at
 *                  2 bytes/pixel needs an odd pixel width that no sensor emits.
 *   QVGA_EMBEDDED  ESS is 8-bit per spec 2.4, so RGB565 would pass validation, but the
 *                  sync codes are 0x00/0xFF and RGB565 data contains both - a black or
 *                  white pixel would read as a sync code. Needs a range-limited
 *                  format in practice.
 *
 * QVGA_RAW8 is the plain-RAW8 control: when one of the four above fails, it separates
 * "the format path is broken" from "that specific feature is broken".
 *
 * Everything else is format-agnostic and lives on RGB565 on purpose: LSM drops whole
 * lines, FCRC drops whole frames, CROP is only exclusive with JPEG, and the polarity
 * bits do not care. QVGA_LSM used to be RAW8 for no reason and was moved.
 */

#endif /* DCI_DEMO_ID_H */
