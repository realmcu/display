/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *                        Header Files
 *============================================================================*/
/*
 * BRING-UP SELF TEST: platform-side test code, not part of the driver.
 *
 * It lives outside src/common/ because nothing in the product calls it - it
 * exists to prove the vendor call path is reachable during bring-up. Keeping it
 * here means the three driver translation units contain only code a product
 * feature can reach, and this file can be dropped from a build without touching
 * them.
 *
 * VENDOR FACING TRANSLATION UNIT. Like the three in src/common/ it includes no OS
 * header (see rtl_vcdj_dwl.c for the rationale): every step is reported through
 * vcdj_plat_log(), which the platform layer implements on top of whatever console
 * the OS offers. This file is that seam's only caller.
 *
 * Everything this file needs from the driver comes through the `vcdj_internal_*()`
 * contract in rtl_vcdj_internal.h - it calls no static helper of the HAL.
 */
#include <stddef.h>
#include <string.h>

#include "rtl_vcdj_internal.h"
#include "vcdj_selftest.h"

#include "basetype.h"
#include "dwl.h"
#include "dectypes.h"
#include "decapicommon.h"
#include "jpegdecapi.h"

#if defined(VCDJ_ENABLE_SELFTEST)

/*============================================================================*
 *                        Local Types
 *============================================================================*/
/*
 * Vendor API structures are unusually large: JpegDecOutput alone is about
 * 13 KB. Keeping them as automatic variables would make one self test require
 * more than 18 KB of stack, so this workspace is allocated through the injected
 * allocator instead.
 *
 * Deliberately a private copy of the HAL's `struct vcdj_jpeg_work` rather than a
 * shared type: it is a stack-avoidance detail of one function, not a contract
 * between the two files. Sharing it would put a vendor type into
 * rtl_vcdj_internal.h, which that header must stay free of.
 */
struct vcdj_selftest_work
{
    struct JpegDecConfig cfg;
    JpegDecInput dec_in;
    JpegDecImageInfo image_info;
};

/*============================================================================*
 *                        Local Functions
 *============================================================================*/
/*
 * M1 interface self test.
 *
 * Walks the whole call path
 *   DWLInit -> JpegDecInit -> JpegDecGetImageInfo -> JpegDecSetInfo
 *           -> JpegDecDecode -> bounded hardware wait -> error exit
 * and reports every step over the console.
 *
 * Two things this proves without any hardware contract:
 *   1. the vendor code path is reachable and the bounded wait actually fires
 *      instead of spinning forever (expected result: VCDJ_ERR_TIMEOUT);
 *   2. JpegDecGetImageInfo() is a pure software header parse, so the geometry it
 *      reports can be checked against an independent parse of the same bytes.
 *      That check is the only real functional verification possible at this
 *      stage, so it is performed and reported explicitly.
 */
static const char *vcdj_dec_ret_str(enum DecRet ret)
{
    switch (ret)
    {
    case DEC_OK:                    return "DEC_OK";
    case DEC_STRM_PROCESSED:        return "DEC_STRM_PROCESSED";
    case DEC_PIC_RDY:               return "DEC_PIC_RDY";
    case DEC_PIC_DECODED:           return "DEC_PIC_DECODED";
    case DEC_HDRS_RDY:              return "DEC_HDRS_RDY";
    case DEC_SLICE_RDY:             return "DEC_SLICE_RDY";
    case DEC_WAITING_FOR_BUFFER:    return "DEC_WAITING_FOR_BUFFER";
    case DEC_SCAN_PROCESSED:        return "DEC_SCAN_PROCESSED";
    case DEC_ABORTED:               return "DEC_ABORTED";
    case DEC_PARAM_ERROR:           return "DEC_PARAM_ERROR";
    case DEC_STRM_ERROR:            return "DEC_STRM_ERROR";
    case DEC_NOT_INITIALIZED:       return "DEC_NOT_INITIALIZED";
    case DEC_MEMFAIL:               return "DEC_MEMFAIL";
    case DEC_INITFAIL:              return "DEC_INITFAIL";
    case DEC_HW_BUS_ERROR:          return "DEC_HW_BUS_ERROR";
    case DEC_HW_TIMEOUT:            return "DEC_HW_TIMEOUT";
    case DEC_SYSTEM_ERROR:          return "DEC_SYSTEM_ERROR";
    case DEC_HW_RESERVED:           return "DEC_HW_RESERVED";
    case DEC_STREAM_NOT_SUPPORTED:  return "DEC_STREAM_NOT_SUPPORTED";
    case DEC_FORMAT_NOT_SUPPORTED:  return "DEC_FORMAT_NOT_SUPPORTED";
    case DEC_UNSUPPORTED:           return "DEC_UNSUPPORTED";
    default:                        return "DEC_<other>";
    }
}

/**
 * Independent minimal JFIF walker: finds the first SOF marker and reads the frame
 * geometry straight out of the bitstream. Used only to cross-check what
 * JpegDecGetImageInfo() reported, so it must not share code with the vendor
 * parser.
 *
 * \return 1 when a SOF was found.
 */
static int vcdj_scan_jpeg_sof(const u8 *p, u32 len, u32 *width, u32 *height,
                              u32 *components, u32 *progressive)
{
    u32 i = 0U;

    if ((len < 4U) || (p[0] != 0xFFU) || (p[1] != 0xD8U))
    {
        return 0;   /* not an SOI */
    }

    i = 2U;

    while ((i + 3U) < len)
    {
        u8 marker;
        u32 seg_len;

        if (p[i] != 0xFFU)
        {
            i++;
            continue;
        }

        marker = p[i + 1U];

        /* padding fill bytes and standalone markers */
        if ((marker == 0xFFU) || (marker == 0x01U) ||
            ((marker >= 0xD0U) && (marker <= 0xD9U)))
        {
            i += 2U;
            continue;
        }

        if ((i + 4U) >= len)
        {
            break;
        }

        seg_len = ((u32)p[i + 2U] << 8) | (u32)p[i + 3U];

        /* SOF0/1/2/3/5..7/9..11/13..15, excluding DHT(C4) DAC(CC) */
        if (((marker >= 0xC0U) && (marker <= 0xCFU)) &&
            (marker != 0xC4U) && (marker != 0xC8U) && (marker != 0xCCU))
        {
            if ((i + 9U) >= len)
            {
                break;
            }

            *height     = ((u32)p[i + 5U] << 8) | (u32)p[i + 6U];
            *width      = ((u32)p[i + 7U] << 8) | (u32)p[i + 8U];
            *components = (u32)p[i + 9U];
            *progressive = ((marker == 0xC2U) || (marker == 0xC6U) ||
                            (marker == 0xCAU) || (marker == 0xCEU)) ? 1U : 0U;
            return 1;
        }

        if (marker == 0xDAU)
        {
            break;      /* start of scan, no SOF seen */
        }

        if (seg_len < 2U)
        {
            break;
        }

        i += 2U + seg_len;
    }

    return 0;
}

/*============================================================================*
 *                        Public Functions
 *============================================================================*/
vcdj_error_t hal_vcdj_selftest(const void *jpeg, uint32_t jpeg_size)
{
    struct vcdj_selftest_work *work = NULL;
    JpegDecOutput *dec_out = NULL;
    const void *dwl;
    JpegDecInst inst = NULL;
    enum DecRet ret;
    vcdj_error_t err = VCDJ_ERR_INTERNAL;
    void *external_buffers[VCDJ_MAX_EXTERNAL_BUFFERS] = {0};
    unsigned int external_count = 0U;
    unsigned int negotiations = 0U;
    const struct vcdj_platform_config *plat;
    u32 ref_w = 0U, ref_h = 0U, ref_comp = 0U, ref_prog = 0U;
    int ref_ok;
    int info_ok = 0;

    vcdj_plat_log("[vcdj] ---- DecoderJ (VC9000DJ) M1 self test ----\n");

    if ((jpeg == NULL) || (jpeg_size == 0U))
    {
        vcdj_plat_log("[vcdj] step 0 FAIL: no input bitstream\n");
        return VCDJ_ERR_PARAM;
    }

    /* ---- step 1: platform layer -------------------------------------- */
    plat = vcdj_platform_get_config();
    if (plat == NULL)
    {
        vcdj_plat_log("[vcdj] step 1 FAIL: no platform configuration\n");
        return VCDJ_ERR_NO_HW;
    }
    vcdj_plat_log("[vcdj] step 1 OK  : platform reg_base=0x%08lx core_offset=0x%x "
               "window=%u cores=%u timeout=%ums\n",
               (unsigned long)plat->reg_base, (unsigned)plat->core_offset,
               (unsigned)plat->reg_window_size, (unsigned)plat->core_count,
               (unsigned)plat->hw_timeout_ms);

    /* ---- step 2: DWL instance ---------------------------------------- */
    err = hal_vcdj_init();
    if (err != VCDJ_SUCCESS)
    {
        vcdj_plat_log("[vcdj] step 2 FAIL: hal_vcdj_init -> %d\n", (int)err);
        return err;
    }
    dwl = vcdj_internal_dwl();
    vcdj_plat_log("[vcdj] step 2 OK  : DWLInit, instance=%p\n", dwl);

    /* ---- step 3: hardware identification ----------------------------- */
    /* Reported only: a wrong ID still has to decode, and the vendor feature
     * lookup already decides what this core can do. */
    {
        uint32_t build_id = 0U;

        err = hal_vcdj_get_hw_build_id(&build_id);
        if (err != VCDJ_SUCCESS)
        {
            vcdj_plat_log("[vcdj] step 3 WARN: hw build id unavailable -> %d\n", (int)err);
        }
        else
        {
            vcdj_plat_log("[vcdj] step 3 OK  : hw build id = 0x%08x (expected low half 0x%04x)\n",
                       (unsigned)build_id, (unsigned)VCDJ_EXPECTED_HW_BUILD_ID);
        }
    }

    /* ---- step 4: JpegDecInit ----------------------------------------- */
    work = vcdj_mem_alloc(sizeof(*work));
    if (work == NULL)
    {
        vcdj_plat_log("[vcdj] step 4 FAIL: cannot allocate %u-byte self-test workspace\n",
                   (unsigned)sizeof(*work));
        hal_vcdj_deinit();
        return VCDJ_ERR_NO_MEM;
    }
    memset(work, 0, sizeof(*work));

    work->cfg.decoder_mode = DEC_NORMAL;
    work->cfg.align = DEC_ALIGN_64B;
    work->cfg.error_handling = DEC_EC_FRAME_NO_ERROR;
    work->cfg.dec_image_type = JPEGDEC_IMAGE;
    work->cfg.ppu_config[0].enabled = 1U;
    work->cfg.ppu_config[0].align = DEC_ALIGN_64B;
    work->cfg.ppu_config[0].shaper_enabled = 1U;
    work->cfg.ppu_config[0].rgb = 1U;
    work->cfg.ppu_config[0].rgb_format = DEC_OUT_FRM_RGB565;

    ret = JpegDecInit(&inst, dwl, &work->cfg);
    if (ret != DEC_OK)
    {
        vcdj_plat_log("[vcdj] step 4 FAIL: JpegDecInit -> %s (%d)\n", vcdj_dec_ret_str(ret), (int)ret);
        vcdj_mem_free(work);
        hal_vcdj_deinit();
        return VCDJ_ERR_STATE;
    }
    vcdj_plat_log("[vcdj] step 4 OK  : JpegDecInit, inst=%p\n", inst);

    /* ---- step 5: input buffer ---------------------------------------- */
    if (DWLMallocLinear(dwl, jpeg_size, &work->dec_in.stream_buffer) != DWL_OK)
    {
        vcdj_plat_log("[vcdj] step 5 FAIL: DWLMallocLinear(%u) - is hal_vcdj_mem_init() done?\n",
                   (unsigned)jpeg_size);
        JpegDecRelease(inst);
        vcdj_mem_free(work);
        hal_vcdj_deinit();
        return VCDJ_ERR_NO_MEM;
    }
    (void)memcpy(work->dec_in.stream_buffer.virtual_address, jpeg, jpeg_size);
    vcdj_core_cache_clean(work->dec_in.stream_buffer.virtual_address, jpeg_size);

    work->dec_in.stream_length = jpeg_size;
    work->dec_in.dec_image_type = JPEGDEC_IMAGE;
    work->dec_in.stream = (u8 *)work->dec_in.stream_buffer.virtual_address;

    vcdj_plat_log("[vcdj] step 5 OK  : input %u bytes, cpu=%p bus=0x%08lx\n",
               (unsigned)jpeg_size, (void *)work->dec_in.stream_buffer.virtual_address,
               (unsigned long)work->dec_in.stream_buffer.bus_address);

    /* ---- step 6: software header parse ------------------------------- */
    ret = JpegDecGetImageInfo(inst, &work->dec_in, &work->image_info);
    if (ret != DEC_OK)
    {
        vcdj_plat_log("[vcdj] step 6 FAIL: JpegDecGetImageInfo -> %s (%d)\n",
                   vcdj_dec_ret_str(ret), (int)ret);
        err = VCDJ_ERR_STREAM;
        goto out;
    }

    vcdj_plat_log("[vcdj] step 6 OK  : JpegDecGetImageInfo display=%ux%u output=%ux%u "
               "bit_depth=%u out_format=0x%x coding_mode=%u thumbnail=%u\n",
               (unsigned)work->image_info.display_width, (unsigned)work->image_info.display_height,
               (unsigned)work->image_info.output_width, (unsigned)work->image_info.output_height,
               (unsigned)work->image_info.bit_depth, (unsigned)work->image_info.output_format,
               (unsigned)work->image_info.coding_mode, (unsigned)work->image_info.thumbnail_type);

    /* ---- step 7: cross-check the parse against the raw bitstream ----- */
    ref_ok = vcdj_scan_jpeg_sof((const u8 *)jpeg, jpeg_size,
                                &ref_w, &ref_h, &ref_comp, &ref_prog);
    if (!ref_ok)
    {
        vcdj_plat_log("[vcdj] step 7 SKIP: no SOF marker found by the reference scan\n");
    }
    else if ((work->image_info.display_width == ref_w) && (work->image_info.display_height == ref_h))
    {
        info_ok = 1;
        vcdj_plat_log("[vcdj] step 7 PASS: header parse matches bitstream SOF "
                   "(%ux%u, %u components, progressive=%u)\n",
                   (unsigned)ref_w, (unsigned)ref_h, (unsigned)ref_comp, (unsigned)ref_prog);
    }
    else
    {
        vcdj_plat_log("[vcdj] step 7 FAIL: header parse %ux%u != bitstream SOF %ux%u\n",
                   (unsigned)work->image_info.display_width, (unsigned)work->image_info.display_height,
                   (unsigned)ref_w, (unsigned)ref_h);
        err = VCDJ_ERR_STREAM;
        goto out;
    }

    /* ---- step 8: apply the post processing configuration ------------- */
    /* As in the HAL: left at 0, JpegDecSetInfo() configures the PPU as 4:0:0. */
    work->cfg.chroma_format = work->image_info.output_format;
    ret = JpegDecSetInfo(inst, &work->cfg);
    if (ret != DEC_OK)
    {
        vcdj_plat_log("[vcdj] step 8 FAIL: JpegDecSetInfo -> %s (%d)\n",
                   vcdj_dec_ret_str(ret), (int)ret);
        err = VCDJ_ERR_UNSUPPORTED;
        goto out;
    }
    vcdj_plat_log("[vcdj] step 8 OK  : JpegDecSetInfo (RGB565 out, 64B aligned)\n");

    dec_out = vcdj_mem_alloc(sizeof(*dec_out));
    if (dec_out == NULL)
    {
        vcdj_plat_log("[vcdj] step 9 FAIL: cannot allocate %u-byte decoder output\n",
                   (unsigned)sizeof(*dec_out));
        err = VCDJ_ERR_NO_MEM;
        goto out;
    }
    memset(dec_out, 0, sizeof(*dec_out));

    /* ---- step 9: first hardware decode -------------------------------- */
    vcdj_plat_log("[vcdj] step 9 ... : JpegDecDecode - a %ums bounded wait is expected\n",
               (unsigned)plat->hw_timeout_ms);

    ret = (enum DecRet)vcdj_internal_jpeg_decode(inst, &work->dec_in, dec_out);

    while ((ret == DEC_WAITING_FOR_BUFFER) &&
           (negotiations < VCDJ_MAX_BUFFER_NEGOTIATIONS))
    {
        struct vcdj_buffer_layout layout;

        memset(&layout, 0, sizeof(layout));

        vcdj_plat_log("[vcdj] step 9 BUF : querying external buffer requirements\n");
        ret = (enum DecRet)vcdj_internal_add_jpeg_buffers(inst, external_buffers,
                                                          VCDJ_MAX_EXTERNAL_BUFFERS,
                                                          &external_count,
                                                          &layout);
        if (ret != DEC_OK)
        {
            vcdj_plat_log("[vcdj] step 9 FAIL: external buffer setup -> %s (%d)\n",
                       vcdj_dec_ret_str(ret), (int)ret);
            err = (ret == DEC_MEMFAIL) ? VCDJ_ERR_NO_MEM : VCDJ_ERR_INTERNAL;
            goto out;
        }

        negotiations++;
        /* Print the per-plane breakdown, not just the total: a wrong plane split
         * is the failure this negotiation is most likely to produce, and the
         * total alone would hide it. 64-bit values are split in halves: the
         * board's rt_vsnprintf has no %llu. */
        vcdj_plat_log("[vcdj] step 9 BUF : %u buffer(s), total=%u:%08x planes=%u:%08x/%u:%08x/%u:%08x "
                   "ystride=%u cstride=%u\n",
                   external_count,
                   (unsigned)(layout.total_size >> 32), (unsigned)layout.total_size,
                   (unsigned)(layout.plane_size[0] >> 32), (unsigned)layout.plane_size[0],
                   (unsigned)(layout.plane_size[1] >> 32), (unsigned)layout.plane_size[1],
                   (unsigned)(layout.plane_size[2] >> 32), (unsigned)layout.plane_size[2],
                   (unsigned)layout.ystride, (unsigned)layout.cstride);
        ret = (enum DecRet)vcdj_internal_jpeg_decode(inst, &work->dec_in, dec_out);
    }

    if ((ret == DEC_PIC_RDY) || (ret == DEC_PIC_DECODED) || (ret == DEC_OK))
    {
        vcdj_plat_log("[vcdj] step 9 OK  : JpegDecDecode -> %s - a picture was produced\n",
                   vcdj_dec_ret_str(ret));
        (void)JpegDecPictureConsumed(inst, dec_out);
        err = VCDJ_SUCCESS;
    }
    else if (ret == DEC_HW_TIMEOUT)
    {
        vcdj_plat_log("[vcdj] step 9 EXPECTED: JpegDecDecode -> DEC_HW_TIMEOUT. The vendor "
                   "path is reachable, the bounded wait fired, nothing hung.\n");
        err = VCDJ_ERR_TIMEOUT;
    }
    else
    {
        vcdj_plat_log("[vcdj] step 9 FAIL: JpegDecDecode -> %s (%d)\n",
                   vcdj_dec_ret_str(ret), (int)ret);
        err = VCDJ_ERR_HW;
    }

out:
    DWLFreeLinear(dwl, &work->dec_in.stream_buffer);
    JpegDecRelease(inst);
    vcdj_internal_free_external_buffers(external_buffers, external_count);
    vcdj_mem_free(dec_out);
    vcdj_mem_free(work);
    hal_vcdj_deinit();

    vcdj_plat_log("[vcdj] ---- self test done: header parse %s, result %d ----\n",
               info_ok ? "PASS" : "not verified", (int)err);

    return err;
}

#endif /* VCDJ_ENABLE_SELFTEST */
