/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DCI demo: framebuffer pool.
 *
 * A fixed arena carved into 1 or 2 buffers at run time, rather than a static array
 * per signal row: the demo has to serve rows from 37 KB to 938 KB and only one runs at
 * a time, so one arena sized to the budget is both smaller and simpler than a union
 * of worst cases.
 *
 * Every buffer is fenced with guard words. That is the cheapest way to catch a
 * descriptor whose DAR or BLOCK_TS was computed wrong, and a wrong block split shows
 * up as an out-of-bounds write before it shows up as a wrong image.
 */

#include <string.h>
#include "dci_demo.h"
#include "dci_demo_buf.h"

/*============================================================================*
 *                         Arena
 *============================================================================*/
#define DEMO_GUARD_WORDS        4
#define DEMO_POOL_WORDS         ((DCI_DEMO_POOL_KB * 1024UL) / 4)

/*
 * uint32_t so the arena is word aligned, which is the driver's only hard requirement
 * on a framebuffer (DESIGN.md 10.4).
 *
 * DCI_DEMO_SECTION lets the integrator move this out of the default RW section - on
 * a part where 256 KB will not fit there, point it at SRAM or PSRAM.
 */
#ifndef DCI_DEMO_SECTION
#define DCI_DEMO_SECTION
#endif

DCI_DEMO_SECTION static uint32_t demo_pool[DEMO_POOL_WORDS];

static struct
{
    uint32_t *buf[DCI_MAX_BUF];
    uint32_t  bytes;            /* usable bytes per buffer */
    uint8_t   count;
    bool      valid;
} demo_bufs;

/*============================================================================*
 *                         Allocation
 *============================================================================*/
/*
 * Layout per buffer: [guard][data][guard], with the trailing guard of one buffer
 * doubling as the leading guard of the next, so a run past the end of buffer 0 is
 * caught even though buffer 1 begins right after.
 */
bool dci_demo_buf_alloc(uint32_t bytes, uint8_t count)
{
    uint32_t data_words = (bytes + 3) / 4;
    uint32_t stride;
    uint32_t w;             /* pool index: must be 32-bit, the pool is > 64 K words */
    uint8_t  i;

    demo_bufs.valid = false;

    if (count == 0 || count > DCI_MAX_BUF || bytes == 0)
    {
        return false;
    }

    stride = data_words + DEMO_GUARD_WORDS;
    if ((stride * count + DEMO_GUARD_WORDS) > DEMO_POOL_WORDS)
    {
        DEMO_LOG("  pool too small: need %u B for %u x %u B, have %u B",
                 (unsigned)((stride * count + DEMO_GUARD_WORDS) * 4),
                 count, (unsigned)bytes, (unsigned)(DCI_DEMO_POOL_KB * 1024UL));
        return false;
    }

    /* Guard everything first, then hand out the data windows. */
    for (w = 0; w < DEMO_POOL_WORDS; w++)
    {
        demo_pool[w] = DCI_DEMO_GUARD;
    }

    for (i = 0; i < count; i++)
    {
        demo_bufs.buf[i] = &demo_pool[i * stride + DEMO_GUARD_WORDS];
    }
    demo_bufs.bytes = data_words * 4;
    demo_bufs.count = count;
    demo_bufs.valid = true;

    DEMO_LOG("  pool: %u buffer(s) x %u B at %p, %u B guard fence",
             count, demo_bufs.bytes, (void *)demo_bufs.buf[0], DEMO_GUARD_WORDS * 4);
    return true;
}

uint32_t *dci_demo_buf_get(uint8_t index)
{
    return (demo_bufs.valid && index < demo_bufs.count) ? demo_bufs.buf[index] : NULL;
}

uint32_t dci_demo_buf_bytes(void)
{
    return demo_bufs.valid ? demo_bufs.bytes : 0;
}

uint8_t dci_demo_buf_count(void)
{
    return demo_bufs.valid ? demo_bufs.count : 0;
}

/*
 * Poison rather than zero: a buffer that still reads DCI_DEMO_POISON proves nothing
 * arrived, whereas a buffer of zeros is ambiguous - the sensor may legitimately have
 * sent black.
 */
void dci_demo_buf_poison(void)
{
    uint8_t  i;
    uint32_t w;

    for (i = 0; i < demo_bufs.count; i++)
    {
        for (w = 0; w < demo_bufs.bytes / 4; w++)
        {
            demo_bufs.buf[i][w] = DCI_DEMO_POISON;
        }
    }
}

/*============================================================================*
 *                         Checks
 *============================================================================*/
/*
 * The guard check is the highest-value assertion in the whole demo: it is the only
 * one that catches a miscomputed block split directly, rather than as a corrupt
 * image that still has to be interpreted by eye.
 */
bool dci_demo_buf_guards_ok(void)
{
    uint32_t stride = demo_bufs.bytes / 4 + DEMO_GUARD_WORDS;
    uint32_t i, g;

    if (!demo_bufs.valid)
    {
        return false;
    }

    for (i = 0; i <= demo_bufs.count; i++)
    {
        uint32_t base = i * stride;

        for (g = 0; g < DEMO_GUARD_WORDS; g++)
        {
            if (demo_pool[base + g] != DCI_DEMO_GUARD)
            {
                DEMO_LOG("  GUARD violated at pool word %u (%p): 0x%08X",
                         base + g, (void *)&demo_pool[base + g],
                         demo_pool[base + g]);
                return false;
            }
        }
    }
    return true;
}

/*
 * Two degenerate outcomes that a length check alone would miss: nothing was written
 * at all, or something wrote a constant. Either means the data path is not carrying
 * pixels, whatever the byte count says.
 */
bool dci_demo_buf_has_data(uint8_t index, uint32_t bytes)
{
    const uint32_t *p = dci_demo_buf_get(index);
    uint32_t        words = bytes / 4;
    uint32_t        poison = 0, same = 0;
    uint32_t        first;
    uint32_t        i;

    if (p == NULL || words == 0)
    {
        return false;
    }

    first = p[0];
    for (i = 0; i < words; i++)
    {
        if (p[i] == DCI_DEMO_POISON) { poison++; }
        if (p[i] == first)           { same++; }
    }

    if (poison == words)
    {
        DEMO_LOG("  buffer untouched: still poison across %u words", words);
        return false;
    }
    if (same == words)
    {
        DEMO_LOG("  buffer is a constant 0x%08X: no pixel variation", first);
        return false;
    }
    if (poison != 0)
    {
        /* Partially written: worth reporting but not fatal - a short frame is
         * diagnosed by the length check, which gives a better message. */
        DEMO_LOG("  note: %u of %u words still poison", poison, words);
    }
    return true;
}

/*============================================================================*
 *                         10-bit packing
 *============================================================================*/
/*
 * Under EDM = 10BIT the hardware packs one sample per 16-bit half-word, value in
 * bits 9:0 and bits 15:10 cleared (DESIGN.md 2, "数据打包"). That makes this the
 * only format in the table with a per-sample invariant that holds regardless of what
 * the camera is pointed at - every other content check can only ask "did anything
 * arrive". So it is worth checking exhaustively rather than by sampling.
 *
 * What a violation tells you, in order of likelihood:
 *   - all half-words have high bits set    -> EDM never took effect; the capture is
 *                                             actually running 8-bit, so pairs of
 *                                             8-bit pixels are being read as one
 *                                             10-bit sample
 *   - every other half-word is affected    -> half-word phase is off by one byte,
 *                                             i.e. a block boundary landed mid-sample
 *   - a contiguous tail is affected        -> the frame is longer than declared and
 *                                             the excess is being written as 8-bit
 *
 * Returns the count so the caller can distinguish those patterns; 0 means clean.
 */
uint32_t dci_demo_buf_raw10_bad_samples(const DCI_frame_t *f, uint32_t report_max)
{
    const uint16_t *s = (const uint16_t *)(const void *)f->addr;
    uint32_t        samples = f->bytes / 2;
    uint32_t        bad = 0;
    uint32_t        reported = 0;
    uint32_t        i;

    if (f->addr == NULL || samples == 0)
    {
        return 0;
    }

    for (i = 0; i < samples; i++)
    {
        if ((s[i] & 0xFC00U) != 0U)
        {
            bad++;
            if (reported < report_max)
            {
                DEMO_LOG("  sample %u (byte %u): 0x%04X - bits 15:10 not clear",
                         i, i * 2U, s[i]);
                reported++;
            }
        }
    }

    if (bad != 0)
    {
        DEMO_LOG("  %u of %u samples have padding bits set (%u%% )", bad, samples,
                 (bad * 100U) / samples);

        /* The stride of the failures is the diagnosis, so state it rather than
         * leaving it to be eyeballed from the list above. */
        if (bad == samples)
        {
            DEMO_LOG("  -> ALL samples: EDM=10BIT very likely never took effect");
        }
        else if (bad * 2U >= samples - 1U && bad * 2U <= samples + 1U)
        {
            DEMO_LOG("  -> every 2nd sample: half-word phase off by one byte");
        }
    }
    return bad;
}

/*
 * Sample range, for the same reason the padding check exists: it needs no reference
 * image. A 10-bit sensor pointed at anything real produces a spread of values; an
 * all-identical or 2-value histogram means the low bits are not arriving.
 */
void dci_demo_buf_raw10_range(const DCI_frame_t *f)
{
    const uint16_t *s = (const uint16_t *)(const void *)f->addr;
    uint32_t        samples = f->bytes / 2;
    uint16_t        min = 0x3FFU, max = 0;
    uint32_t        sum = 0;
    uint32_t        i;

    if (f->addr == NULL || samples == 0)
    {
        return;
    }

    for (i = 0; i < samples; i++)
    {
        uint16_t v = (uint16_t)(s[i] & 0x3FFU);

        if (v < min) { min = v; }
        if (v > max) { max = v; }
        sum += v;
    }

    DEMO_LOG("  10-bit range: min=%u max=%u mean=%u (of 1023) over %u samples",
             min, max, (unsigned)(sum / samples), samples);

    if (min == max)
    {
        DEMO_LOG("  -> flat: no pixel variation at all");
    }
    else if (max <= 0xFFU)
    {
        /* Every sample fits in 8 bits. Suspicious for a 10-bit sensor looking at a
         * real scene: the top 2 bits may not be wired or the sensor is in 8-bit mode
         * while DCI is in 10-bit mode. Not conclusive - a dark scene does this too. */
        DEMO_LOG("  -> all samples <= 255: check D[9:8] wiring and sensor bit depth");
    }
}

/*============================================================================*
 *                         Dump
 *============================================================================*/
static uint32_t demo_crc32(const uint8_t *p, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t i;
    uint8_t  b;

    while (len--)
    {
        crc ^= *p++;
        for (i = 0; i < 8; i++)
        {
            b   = (uint8_t)(crc & 1U);
            crc >>= 1;
            if (b) { crc ^= 0xEDB88320UL; }
        }
    }
    return ~crc;
}

/*
 * Levels rather than one format: a 424 KB frame is ~7 s of hex at 1 Mbps, so a
 * continuous run has to stay at level 0 or the UART becomes the bottleneck and the
 * capture itself starts failing.
 */
void dci_demo_buf_dump(const DCI_frame_t *f, const dci_demo_cfg_t *c, uint8_t level)
{
    const uint8_t *d   = f->addr;
    dci_demo_fmt_t fmt = c->sig->fmt;
    uint32_t       line_bytes, y, x;

    DEMO_LOG("[FRAME] idx=%u seq=%u bytes=%u trunc=%u crc32=0x%08X",
             f->index, f->seq, f->bytes, f->truncated,
             demo_crc32(d, f->bytes));

    if (level == 0)
    {
        return;
    }

    /* Taken from the resolved configuration rather than recomputed: it already accounts
     * for crop, byte select and EDM, and a second formula here would be one more place
     * for the two to drift apart. */
    line_bytes = c->line_bytes;

    if (level == 1)
    {
        /* Corners, centre, and the first pixel of a few lines. A broken stride or a
         * miscomputed block split breaks the regularity of the line starts, which is
         * visible here without dumping the whole frame. */
        DEMO_LOG("  head: %02X %02X %02X %02X %02X %02X %02X %02X",
                 d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);
        DEMO_LOG("  tail: %02X %02X %02X %02X %02X %02X %02X %02X",
                 d[f->bytes - 8], d[f->bytes - 7], d[f->bytes - 6], d[f->bytes - 5],
                 d[f->bytes - 4], d[f->bytes - 3], d[f->bytes - 2], d[f->bytes - 1]);

        if (fmt != DCI_FMT_JPEG && line_bytes != 0)
        {
            uint32_t lines = f->bytes / line_bytes;

            for (y = 0; y < lines; y += (lines / 8 ? lines / 8 : 1))
            {
                uint32_t o = y * line_bytes;

                /* RAW10 is half-word data: printing raw bytes would show the
                 * padding byte interleaved and read as garbage, so decode it. */
                if (fmt == DCI_FMT_RAW10)
                {
                    const uint16_t *s = (const uint16_t *)(const void *)&d[o];

                    DEMO_LOG("  line %4u: %4u %4u %4u %4u ... (10-bit)", y,
                             s[0] & 0x3FFU, s[1] & 0x3FFU,
                             s[2] & 0x3FFU, s[3] & 0x3FFU);
                }
                else
                {
                    DEMO_LOG("  line %4u: %02X %02X %02X %02X ...", y,
                             d[o], d[o + 1], d[o + 2], d[o + 3]);
                }
            }
        }
        return;
    }

    /* Level 2: full hex, address-prefixed so a host script can reassemble an image. */
    for (y = 0; y < f->bytes; y += 16)
    {
        char     line[64];
        uint32_t n = 0;

        for (x = 0; x < 16 && (y + x) < f->bytes; x++)
        {
            static const char hex[] = "0123456789ABCDEF";
            line[n++] = hex[d[y + x] >> 4];
            line[n++] = hex[d[y + x] & 0xF];
            line[n++] = ' ';
        }
        line[n] = '\0';
        DEMO_LOG("  %06X  %s", y, line);
    }
}

/*
 * Where the data is, for a debugger rather than for the UART.
 *
 * This is the whole output of the RECEIVE case, and the reason it is printed instead of
 * hex-dumped: a 300 KB frame is minutes of UART, and the bytes are already sitting in
 * RAM at a fixed address with capture stopped. The savebin line is spelled out because
 * the length argument is the usual mistake - JLink takes a byte count, not an end
 * address, and a frame is rarely a round number.
 */
void dci_demo_buf_report_range(const DCI_frame_t *f)
{
    uint8_t i;

    if (f != NULL)
    {
        uint32_t start = (uint32_t)f->addr;
        uint32_t len   = f->bytes;

        DEMO_LOG("  buffer %u: 0x%08X .. 0x%08X  (%u B)", f->index, start,
                 start + len - 1U, (unsigned)len);
        DEMO_LOG("  JLink : savebin frame.bin, 0x%08X, 0x%X", start, (unsigned)len);
        DEMO_LOG("  (length is a BYTE COUNT, not an end address)");
        return;
    }

    /* No held frame: describe the arena instead, which is what someone stopping at a
     * breakpoint mid-run needs. */
    for (i = 0; i < demo_bufs.count; i++)
    {
        uint32_t start = (uint32_t)demo_bufs.buf[i];

        DEMO_LOG("  buffer %u: 0x%08X .. 0x%08X  (%u B allocated)", i, start,
                 start + demo_bufs.bytes - 1U, (unsigned)demo_bufs.bytes);
        DEMO_LOG("  JLink : savebin buf%u.bin, 0x%08X, 0x%X", i, start,
                 (unsigned)demo_bufs.bytes);
    }
}

/*
 * JPEG structure check. Cheap, and it is the only content assertion available for a
 * variable-length format: SOI at the start, EOI at the end, with 0..3 padding bytes
 * after it because the hardware pads the last word (spec 2.7).
 */
bool dci_demo_buf_jpeg_ok(const DCI_frame_t *f)
{
    const uint8_t *d = f->addr;
    uint32_t       i;

    if (f->bytes < 4)
    {
        DEMO_LOG("  JPEG too short: %u B", f->bytes);
        return false;
    }
    if (d[0] != 0xFF || d[1] != 0xD8)
    {
        DEMO_LOG("  JPEG missing SOI: %02X %02X", d[0], d[1]);
        return false;
    }

    for (i = 0; i < 4 && (f->bytes - 2 - i) > 0; i++)
    {
        uint32_t e = f->bytes - 2 - i;

        if (d[e] == 0xFF && d[e + 1] == 0xD9)
        {
            DEMO_LOG("  JPEG ok: SOI..EOI, %u padding byte(s) after EOI", i);
            return true;
        }
    }
    DEMO_LOG("  JPEG missing EOI within the last 4 bytes");
    return false;
}
