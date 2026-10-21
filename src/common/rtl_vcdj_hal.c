/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *                        Header Files
 *============================================================================*/
/*
 * HAL FACADE: everything the layers above this driver call.
 *
 * Responsibilities:
 *   - dispatch JPEG and PNG to the vendor decode APIs
 *   - translate `enum DecRet` into the stable \ref vcdj_error_t space, so no
 *     vendor type ever escapes through rtl_vcdj.h
 *   - drive the vendor external-buffer negotiation protocol
 *
 * The bring-up self test is NOT here: it lives in src/test/vcdj_selftest.c and
 * reaches this file only through the `vcdj_internal_*()` contract, so this
 * facade holds nothing a product feature cannot reach.
 *
 * VENDOR FACING TRANSLATION UNIT: no OS header may be included here, without
 * exception. See rtl_vcdj_dwl.c for the rationale.
 */
#include <stddef.h>
#include <string.h>

#include "rtl_vcdj_internal.h"

#include "basetype.h"
#include "dwl.h"
#include "dectypes.h"
#include "decapicommon.h"
#include "jpegdecapi.h"
#include "pngdecapi.h"

/*============================================================================*
 *                        Local Definitions
 *============================================================================*/
/*
 * VCDJ_MAX_EXTERNAL_BUFFERS and VCDJ_MAX_BUFFER_NEGOTIATIONS are in
 * rtl_vcdj_internal.h: the former is the `capacity` argument of
 * vcdj_internal_add_jpeg_buffers(), so it is a cross-file contract rather than a
 * local choice, and the self test drives the same negotiation loop.
 */

/* 0x2070 channel-0 PPU limits; the vendor checker enforces the same bounds. */
#define VCDJ_PP_MIN_DIMENSION           48U
#define VCDJ_PP_MAX_OUTPUT_DIMENSION    4096U
#define VCDJ_PP_MAX_UPSCALE_RATIO       3U

/*
 * Live picture slots: one decoded picture the caller has not released yet.
 *
 * One is enough because a single decode is in flight at a time (the DWL layer is
 * a single instance, see DWLInit() in rtl_vcdj_dwl.c) and because
 * hal_vcdj_decode() completes the whole vendor lifecycle before it returns.
 */
#define VCDJ_MAX_LIVE_PICTURES          1U

#define VCDJ_STATIC_ASSERT(cond, tag) \
    typedef char vcdj_static_assert_##tag[(cond) ? 1 : -1]

/*
 * The public enumerations are used as structure fields, which is what makes the
 * legal values discoverable from the field declaration. With -fshort-enums (set
 * SDK wide, see cmake/tool_chain/{gcc,armclang}.cmake) the width of an
 * enumeration follows its value range, so these five checks guard two things a
 * reviewer would not notice:
 *
 *   1. adding an enumerator above 0xFF widens the type to two bytes and silently
 *      moves every field after it. `VCDJ_OUT_UNKNOWN` sits at 0xFF for exactly
 *      that reason;
 *   2. compiling this driver WITHOUT -fshort-enums makes them four bytes.
 *
 * Both are layout changes disguised as harmless edits, so fail the build with a
 * name that says which contract broke.
 */
VCDJ_STATIC_ASSERT(sizeof(vcdj_codec_t)       == 1U, codec_enum_is_one_byte);
VCDJ_STATIC_ASSERT(sizeof(vcdj_out_format_t)  == 1U, out_format_enum_is_one_byte);
VCDJ_STATIC_ASSERT(sizeof(vcdj_pp_filter_t)   == 1U, pp_filter_enum_is_one_byte);
VCDJ_STATIC_ASSERT(sizeof(vcdj_color_type_t)  == 1U, color_type_enum_is_one_byte);
VCDJ_STATIC_ASSERT(sizeof(vcdj_chroma_t)      == 1U, chroma_enum_is_one_byte);

/*
 * Measured layouts on the target ABI (Cortex-M55, 32-bit, -fshort-enums):
 * 12 / 32 / 28 / 20 / 76 bytes. These are not an ABI promise - the driver is compiled
 * from source with the rest of the SDK - they are a tripwire: an unintended
 * layout change, most likely from one of the two enum mistakes above, shows up
 * here instead of in a hard-to-read run-time symptom.
 */
VCDJ_STATIC_ASSERT(sizeof(vcdj_dec_param_t)  == 12U, dec_param_layout);
VCDJ_STATIC_ASSERT(sizeof(vcdj_pp_config_t)  == 32U, pp_config_layout);
VCDJ_STATIC_ASSERT(sizeof(vcdj_image_info_t) == 28U, image_info_layout);
VCDJ_STATIC_ASSERT(sizeof(vcdj_plane_t)      == 20U, plane_layout);
VCDJ_STATIC_ASSERT(sizeof(vcdj_picture_t)    == 76U, picture_layout);
VCDJ_STATIC_ASSERT(VCDJ_MAX_PLANES == MAX_OUTPUT_PLANE, plane_count_matches_vendor);

/*============================================================================*
 *                        Local Types
 *============================================================================*/
/*
 * Vendor API structures are unusually large: JpegDecOutput alone is about
 * 13 KB. Keeping them as automatic variables would make one decode require
 * more than 18 KB of stack. Allocate one temporary workspace through the
 * caller-provided allocator instead, so the API is safe to call from normal
 * application threads and allocation failure remains reportable.
 */
struct vcdj_jpeg_work
{
    struct JpegDecConfig cfg;
    JpegDecInput dec_in;
    JpegDecImageInfo image_info;
};

struct vcdj_png_work
{
    struct PngDecConfig cfg;
    PngDecInput dec_in;
    PngDecImageInfo image_info;
};

/*
 * One live picture: the pixel memory a caller still owns.
 *
 * WHY THE WHOLE DWLDecBuffers AND NOT JUST AN ADDRESS. The vendor AddBuffer API
 * takes a `struct DWLLinearMem *` but casts it to `struct DWLDecBuffers` and
 * copies the entire output matrix into its InputQueue, so the descriptor we
 * registered has to stay byte-identical for as long as the memory is live. It is
 * also the object DWLFreeRefFrm() needs back, which is why the descriptor and the
 * pixels have separate lifetimes (see 5.1 item 3 of the interface plan): the
 * descriptor belongs to this slot, the pixels belong to the caller until release.
 */
struct vcdj_live_picture
{
    struct DWLDecBuffers *buffer;   /**< Descriptor registered with the vendor */
    uint32_t generation;            /**< Incremented on every release; see below */
    uint8_t  in_use;
};

/*============================================================================*
 *                        Local Variables
 *============================================================================*/
static const void *vcdj_dwl_handle;
static u32 vcdj_initialised;

static struct vcdj_live_picture vcdj_live[VCDJ_MAX_LIVE_PICTURES];

/*============================================================================*
 *                        Output buffer ownership
 *============================================================================*/
/**
 * Release every plane descriptor in one registered output buffer.
 *
 * Walks the whole channel x plane matrix rather than just `[0][0]`. Today only
 * `[0][0]` ever carries memory - with `use_separated_pp == 0` the vendor treats
 * that single descriptor as the base of every plane - but a future multi-channel
 * or separated-PP configuration would fill more entries, and a release path that
 * only ever frees the first one would then leak silently.
 * DWLFreeRefFrm() ignores a NULL descriptor, so the unused entries cost nothing.
 *
 * This is the one place that knows how to undo vcdj_add_external_buffers(); all
 * three release paths (negotiation hand-back, decode cleanup, picture release)
 * go through it so they cannot drift apart.
 */
static void vcdj_free_all_planes(struct DWLDecBuffers *buffer)
{
    unsigned int ch;
    unsigned int pl;

    for (ch = 0U; ch < DEC_MAX_OUT_COUNT; ch++)
    {
        for (pl = 0U; pl < MAX_OUTPUT_PLANE; pl++)
        {
            struct DWLLinearMem *mem = &buffer->dec_buffer[ch][pl];

            if (mem->virtual_address != NULL)
            {
                DWLFreeRefFrm(vcdj_dwl_handle, mem);
            }
        }
    }
}

/*============================================================================*
 *                        Release token
 *============================================================================*/
/*
 * WHAT THE TOKEN ENCODES, AND WHY IT IS NOT A POINTER.
 *
 * rtl_vcdj.h promises the caller an opaque value that must not be dereferenced,
 * so handing out `&vcdj_live[i]` would make that promise a lie the moment someone
 * tried it. Instead the token carries a slot index plus a generation counter:
 *
 *     bits  7..0   slot index
 *     bits 31..8   generation of that slot
 *
 * The generation is what makes a stale token detectable rather than dangerous.
 * Without it, releasing a picture twice would look exactly like a valid release
 * of whatever picture landed in the same slot afterwards, and the second call
 * would free memory the new owner is still using. With it, the counter has moved
 * on and the mismatch is reported as VCDJ_ERR_PARAM.
 *
 * A zero token means "no pixels" and is always valid to release: the header
 * documents that a picture must be released even when plane_count is 0, so the
 * geometry-only result of a failed decode still round-trips cleanly. Encoding
 * therefore starts the generation at 1.
 */
#define VCDJ_TOKEN_SLOT_BITS            8U
#define VCDJ_TOKEN_SLOT_MASK            ((1U << VCDJ_TOKEN_SLOT_BITS) - 1U)

VCDJ_STATIC_ASSERT(VCDJ_MAX_LIVE_PICTURES <= VCDJ_TOKEN_SLOT_MASK,
                   live_pictures_fit_in_token);

static uintptr_t vcdj_token_encode(unsigned int slot, uint32_t generation)
{
    return (uintptr_t)(((generation + 1U) << VCDJ_TOKEN_SLOT_BITS) |
                       ((uint32_t)slot & VCDJ_TOKEN_SLOT_MASK));
}

/**
 * Resolve a token back to its slot, or NULL when it does not name a live picture.
 *
 * Rejects an out-of-range index, an unoccupied slot and a generation that has
 * moved on, so a double release and a fabricated token are both caught here
 * rather than turning into a free of someone else's memory.
 */
static struct vcdj_live_picture *vcdj_token_lookup(uintptr_t token)
{
    unsigned int slot = (unsigned int)((uint32_t)token & VCDJ_TOKEN_SLOT_MASK);
    uint32_t generation = (uint32_t)(token >> VCDJ_TOKEN_SLOT_BITS);

    if ((token == 0U) || (slot >= VCDJ_MAX_LIVE_PICTURES))
    {
        return NULL;
    }

    if ((vcdj_live[slot].in_use == 0U) ||
        ((vcdj_live[slot].generation + 1U) != generation))
    {
        return NULL;
    }

    return &vcdj_live[slot];
}

/** Claim a free slot, or NULL when the caller is already holding its quota. */
static struct vcdj_live_picture *vcdj_live_claim(unsigned int *slot_out)
{
    unsigned int i;

    for (i = 0U; i < VCDJ_MAX_LIVE_PICTURES; i++)
    {
        if (vcdj_live[i].in_use == 0U)
        {
            vcdj_live[i].in_use = 1U;
            vcdj_live[i].buffer = NULL;
            *slot_out = i;
            return &vcdj_live[i];
        }
    }

    return NULL;
}

/**
 * Free the pixel memory a slot owns and retire the slot.
 *
 * The generation advances here, which is what invalidates every token that named
 * this slot. It must therefore run on the error path too, otherwise a slot
 * claimed by a decode that later failed would stay occupied forever.
 */
static void vcdj_live_free(struct vcdj_live_picture *live)
{
    if (live->buffer != NULL)
    {
        vcdj_free_all_planes(live->buffer);
        vcdj_mem_free(live->buffer);
        live->buffer = NULL;
    }

    live->generation++;
    live->in_use = 0U;
}

/*============================================================================*
 *                        Local Functions
 *============================================================================*/
/*
 * Collapse the vendor return space into errors that are stable for this
 * synchronous one-picture API. Control-flow states are deliberately not exposed:
 * buffer negotiation is consumed by hal_vcdj_decode(), and streaming, flush and
 * abort have no public operation that a caller could continue.
 *
 * Keep this switch explicit rather than relying on value ranges: enum DecRet is a
 * multi-codec vendor type, while the only contracts relevant here are the return
 * branches of the JpegDec and PngDec APIs. Explicit cases make a newly reachable
 * value fall into INTERNAL until its meaning for this facade has been reviewed.
 */
static vcdj_error_t vcdj_map_dec_ret(enum DecRet ret)
{
    switch (ret)
    {
    case DEC_OK:
    case DEC_PIC_RDY:
    case DEC_PIC_DECODED:
    case DEC_HDRS_RDY:
        return VCDJ_SUCCESS;

    case DEC_PARAM_ERROR:
    case DEC_INVALID_STREAM_LENGTH:
    case DEC_INVALID_INPUT_BUFFER_SIZE:
    case DEC_INFOPARAM_ERROR:
        return VCDJ_ERR_PARAM;

    case DEC_MEMFAIL:
        return VCDJ_ERR_NO_MEM;

    case DEC_STRM_ERROR:
    case DEC_ERROR:
    case DEC_INCREASE_INPUT_BUFFER:
    case DEC_END_OF_STREAM:
    case DEC_STRM_PROCESSED:
    case DEC_DISCARD_INTERNAL:
        /* A one-shot decode that ends, consumes all supplied data or discards its
         * only picture without producing output has a malformed or truncated
         * stream from the caller's point of view. */
        return VCDJ_ERR_STREAM;

    case DEC_UNSUPPORTED:
    case DEC_STREAM_NOT_SUPPORTED:
    case DEC_FORMAT_NOT_SUPPORTED:
    case DEC_SLICE_MODE_UNSUPPORTED:
        return VCDJ_ERR_UNSUPPORTED;

    case DEC_HW_TIMEOUT:
    case DEC_HW_EXT_TIMEOUT:
        return VCDJ_ERR_TIMEOUT;

    case DEC_HW_BUS_ERROR:
    case DEC_HW_RESERVED:
    case DEC_SYSTEM_ERROR:
    case DEC_DWL_ERROR:
    case DEC_FATAL_SYSTEM_ERROR:
        return VCDJ_ERR_HW;

    case DEC_NOT_INITIALIZED:
    case DEC_ABORTED:
    case DEC_NO_DECODING_BUFFER:
    case DEC_EXT_BUFFER_REJECTED:
        /* Buffer slots and abort state are owned by this helper, not by its
         * caller. Do not expose the vendor queue protocol as public states. */
        return VCDJ_ERR_STATE;

    case DEC_WAITING_FOR_BUFFER:
    case DEC_SCAN_PROCESSED:
    case DEC_INITFAIL:
    default:
        /* WAITING_FOR_BUFFER must be consumed by the bounded negotiation loop;
         * SCAN_PROCESSED is not produced by the imported implementation. */
        return VCDJ_ERR_INTERNAL;
    }
}

static vcdj_chroma_t vcdj_map_jpeg_chroma(u32 output_format)
{
    switch (output_format)
    {
    case DEC_OUT_FRM_YUV400:
        return VCDJ_CHROMA_MONO;
    case DEC_OUT_FRM_YUV411SP:
        return VCDJ_CHROMA_411;
    case DEC_OUT_FRM_YUV420SP:
        return VCDJ_CHROMA_420;
    case DEC_OUT_FRM_YUV422SP:
        return VCDJ_CHROMA_422;
    case DEC_OUT_FRM_YUV440:
        return VCDJ_CHROMA_440;
    case DEC_OUT_FRM_YUV444SP:
        return VCDJ_CHROMA_444;
    default:
        return VCDJ_CHROMA_UNKNOWN;
    }
}

/*============================================================================*
 *                        Output format table
 *============================================================================*/
/** How a public format is requested from the vendor PpUnitConfig. */
enum vcdj_fmt_kind
{
    VCDJ_KIND_NONE = 0,             /**< Hole in the table: not a public format */
    VCDJ_KIND_RGB,                  /**< `rgb = 1`, `rgb_format` */
    VCDJ_KIND_RGB_PLANAR,           /**< `rgb_planar = 1`, `rgb_format` */
    VCDJ_KIND_YUV,                  /**< semi-planar or planar YUV */
    VCDJ_KIND_LUMA,                 /**< `monochrome = 1` */
    VCDJ_KIND_YUYV,                 /**< `out_yuyv = 1` */
    VCDJ_KIND_UYVY,                 /**< `out_uyvy = 1` */
};

#define VCDJ_FMT_CR_FIRST   0x01U   /**< `cr_first = 1` */
#define VCDJ_FMT_PLANAR     0x02U   /**< `planar = 1` */
#define VCDJ_FMT_P010       0x04U   /**< `out_p010 = 1` */
#define VCDJ_FMT_I010       0x08U   /**< `out_I010 = 1` */
#define VCDJ_FMT_L010       0x10U   /**< `out_L010 = 1` */
#define VCDJ_FMT_1010       0x20U   /**< `out_1010 = 1` */
#define VCDJ_FMT_HBD        0x40U   /**< needs a JPEG deeper than 8 bits */
#define VCDJ_FMT_PNG_ONLY   0x80U   /**< only a PNG can produce it */

struct vcdj_fmt_desc
{
    uint8_t  kind;                  /**< \ref vcdj_fmt_kind */
    uint8_t  chroma;                /**< PpUnitConfig.chroma_format (PP_YUV4xx) */
    uint8_t  flags;                 /**< VCDJ_FMT_* */
    uint16_t rgb_format;            /**< DEC_OUT_FRM_* request, RGB kinds only */
    uint16_t vendor;                /**< DEC_OUT_FRM_* TransUnitConfig2Format() reports */
};

VCDJ_STATIC_ASSERT(DEC_OUT_FRM_HIST <= 0xFFFFU, picture_format_fits_u16);

#define VCDJ_RGB(req, rep, fl)      { VCDJ_KIND_RGB, PP_YUV420, (fl), (req), (rep) }
#define VCDJ_RGBP(req, rep, fl)     { VCDJ_KIND_RGB_PLANAR, PP_YUV420, (fl), (req), (rep) }
#define VCDJ_YUV(ch, fl, rep)       { VCDJ_KIND_YUV, (ch), (fl), 0U, (rep) }
#define VCDJ_LUMA(fl, rep)          { VCDJ_KIND_LUMA, PP_YUV420, (fl), 0U, (rep) }
#define VCDJ_PACK(k, fl, rep)       { (k), PP_YUV422, (fl), 0U, (rep) }

#define VCDJ_CRF                    VCDJ_FMT_CR_FIRST
#define VCDJ_PL                     VCDJ_FMT_PLANAR
#define VCDJ_HBD                    VCDJ_FMT_HBD

/*
 * Indexed by vcdj_out_format_t. The PpUnitConfig fields come from the vendor
 * testbench options (command_line_parser.c); `vendor` is what ppu.c
 * TransUnitConfig2Format() derives from them. Note the vendor naming swap:
 * requesting DEC_OUT_FRM_BGR888 is reported back as DEC_OUT_FRM_BGR888, but
 * travels through the core as PP_OUT_RGB888.
 */
static const struct vcdj_fmt_desc vcdj_fmt_table[] =
{
    [VCDJ_OUT_RGB565]       = VCDJ_RGB(DEC_OUT_FRM_RGB565, DEC_OUT_FRM_RGB565, 0U),
    [VCDJ_OUT_ARGB8888]     = VCDJ_RGB(DEC_OUT_FRM_ARGB888, DEC_OUT_FRM_ARGB888, 0U),
    [VCDJ_OUT_YUV420SP]     = VCDJ_YUV(PP_YUV420, 0U, DEC_OUT_FRM_YUV420SP),
    [VCDJ_OUT_BGR888]       = VCDJ_RGB(DEC_OUT_FRM_BGR888, DEC_OUT_FRM_BGR888, 0U),
    [VCDJ_OUT_ABGR8888]     = VCDJ_RGB(DEC_OUT_FRM_ABGR888, DEC_OUT_FRM_ABGR888, 0U),
    [VCDJ_OUT_MONOCHROME]   = VCDJ_LUMA(0U, DEC_OUT_FRM_YUV400),
    /* PngDecSetInfo() turns a plain YUV request into luma + alpha by itself. */
    [VCDJ_OUT_YUV400_ALPHA] = VCDJ_YUV(PP_YUV420, VCDJ_FMT_PNG_ONLY,
                                       DEC_OUT_FRM_YUV400_WITH_ALPHA),

    [VCDJ_OUT_RGB888]       = VCDJ_RGB(DEC_OUT_FRM_RGB888, DEC_OUT_FRM_RGB888, 0U),
    [VCDJ_OUT_BGR565]       = VCDJ_RGB(DEC_OUT_FRM_BGR565, DEC_OUT_FRM_BGR565, 0U),
    [VCDJ_OUT_ARGB8565]     = VCDJ_RGB(DEC_OUT_FRM_ARGB8565, DEC_OUT_FRM_ARGB8565, 0U),
    [VCDJ_OUT_XRGB8888]     = VCDJ_RGB(DEC_OUT_FRM_XRGB888, DEC_OUT_FRM_XRGB888, 0U),
    [VCDJ_OUT_XBGR8888]     = VCDJ_RGB(DEC_OUT_FRM_XBGR888, DEC_OUT_FRM_XBGR888, 0U),
    [VCDJ_OUT_RGBA8888]     = VCDJ_RGB(DEC_OUT_FRM_RGBA888, DEC_OUT_FRM_RGBA888, 0U),
    [VCDJ_OUT_BGRA8888]     = VCDJ_RGB(DEC_OUT_FRM_BGRA888, DEC_OUT_FRM_BGRA888, 0U),
    [VCDJ_OUT_A2R10G10B10]  = VCDJ_RGB(DEC_OUT_FRM_A2R10G10B10, DEC_OUT_FRM_A2R10G10B10, VCDJ_HBD),
    [VCDJ_OUT_A2B10G10R10]  = VCDJ_RGB(DEC_OUT_FRM_A2B10G10R10, DEC_OUT_FRM_A2B10G10R10, VCDJ_HBD),
    [VCDJ_OUT_X2R10G10B10]  = VCDJ_RGB(DEC_OUT_FRM_X2R10G10B10, DEC_OUT_FRM_X2R10G10B10, VCDJ_HBD),
    [VCDJ_OUT_X2B10G10R10]  = VCDJ_RGB(DEC_OUT_FRM_X2B10G10R10, DEC_OUT_FRM_X2B10G10R10, VCDJ_HBD),
    [VCDJ_OUT_R10G10B10A2]  = VCDJ_RGB(DEC_OUT_FRM_R10G10B10A2, DEC_OUT_FRM_R10G10B10A2, VCDJ_HBD),
    [VCDJ_OUT_B10G10R10A2]  = VCDJ_RGB(DEC_OUT_FRM_B10G10R10A2, DEC_OUT_FRM_B10G10R10A2, VCDJ_HBD),
    [VCDJ_OUT_R16G16B16]    = VCDJ_RGB(DEC_OUT_FRM_R16G16B16, DEC_OUT_FRM_R16G16B16, VCDJ_HBD),
    [VCDJ_OUT_B16G16R16]    = VCDJ_RGB(DEC_OUT_FRM_B16G16R16, DEC_OUT_FRM_B16G16R16, VCDJ_HBD),

    [VCDJ_OUT_RGB888_PLANAR]    = VCDJ_RGBP(DEC_OUT_FRM_RGB888, DEC_OUT_FRM_RGB888_P, 0U),
    [VCDJ_OUT_BGR888_PLANAR]    = VCDJ_RGBP(DEC_OUT_FRM_BGR888, DEC_OUT_FRM_BGR888_P, 0U),
    [VCDJ_OUT_R16G16B16_PLANAR] = VCDJ_RGBP(DEC_OUT_FRM_R16G16B16, DEC_OUT_FRM_R16G16B16_P, VCDJ_HBD),
    [VCDJ_OUT_B16G16R16_PLANAR] = VCDJ_RGBP(DEC_OUT_FRM_B16G16R16, DEC_OUT_FRM_B16G16R16_P, VCDJ_HBD),

    [VCDJ_OUT_YVU420SP]     = VCDJ_YUV(PP_YUV420, VCDJ_CRF, DEC_OUT_FRM_NV21SP),
    [VCDJ_OUT_YUV420P]      = VCDJ_YUV(PP_YUV420, VCDJ_PL, DEC_OUT_FRM_YUV420P),
    [VCDJ_OUT_YVU420P]      = VCDJ_YUV(PP_YUV420, VCDJ_PL | VCDJ_CRF, DEC_OUT_FRM_NV21P),
    [VCDJ_OUT_YUV422SP]     = VCDJ_YUV(PP_YUV422, 0U, DEC_OUT_FRM_YUV422SP),
    [VCDJ_OUT_YVU422SP]     = VCDJ_YUV(PP_YUV422, VCDJ_CRF, DEC_OUT_FRM_422_NV21SP),
    [VCDJ_OUT_YUV422P]      = VCDJ_YUV(PP_YUV422, VCDJ_PL, DEC_OUT_FRM_YUV422P),
    [VCDJ_OUT_YVU422P]      = VCDJ_YUV(PP_YUV422, VCDJ_PL | VCDJ_CRF, DEC_OUT_FRM_422_NV21P),
    [VCDJ_OUT_YUV444P]      = VCDJ_YUV(PP_YUV444, VCDJ_PL, DEC_OUT_FRM_YUV444P),
    [VCDJ_OUT_YVU444P]      = VCDJ_YUV(PP_YUV444, VCDJ_PL | VCDJ_CRF, DEC_OUT_FRM_444_NV21P),
    /* The vendor reports the swapped packed orders under the unswapped name. */
    [VCDJ_OUT_YUYV]         = VCDJ_PACK(VCDJ_KIND_YUYV, 0U, DEC_OUT_FRM_YUV422_YUYV),
    [VCDJ_OUT_UYVY]         = VCDJ_PACK(VCDJ_KIND_UYVY, 0U, DEC_OUT_FRM_YUV422_UYVY),
    [VCDJ_OUT_YVYU]         = VCDJ_PACK(VCDJ_KIND_YUYV, VCDJ_CRF, DEC_OUT_FRM_YUV422_YUYV),
    [VCDJ_OUT_VYUY]         = VCDJ_PACK(VCDJ_KIND_UYVY, VCDJ_CRF, DEC_OUT_FRM_YUV422_UYVY),

    [VCDJ_OUT_YUV400_PACK10]   = VCDJ_LUMA(VCDJ_HBD, DEC_OUT_FRM_YUV400_PACK10),
    [VCDJ_OUT_YUV400_P010]     = VCDJ_LUMA(VCDJ_HBD | VCDJ_FMT_P010, DEC_OUT_FRM_YUV400_P010),
    [VCDJ_OUT_YUV400_I010]     = VCDJ_LUMA(VCDJ_HBD | VCDJ_FMT_I010, DEC_OUT_FRM_YUV400_I010),
    [VCDJ_OUT_YUV400_L010]     = VCDJ_LUMA(VCDJ_HBD | VCDJ_FMT_L010, DEC_OUT_FRM_YUV400_L010),
    [VCDJ_OUT_YUV420SP_PACK10] = VCDJ_YUV(PP_YUV420, VCDJ_HBD, DEC_OUT_FRM_YUV420SP_PACK10),
    [VCDJ_OUT_YUV420SP_P010]   = VCDJ_YUV(PP_YUV420, VCDJ_HBD | VCDJ_FMT_P010, DEC_OUT_FRM_YUV420SP_P010),
    [VCDJ_OUT_YUV420SP_I010]   = VCDJ_YUV(PP_YUV420, VCDJ_HBD | VCDJ_FMT_I010, DEC_OUT_FRM_YUV420SP_I010),
    [VCDJ_OUT_YUV420SP_L010]   = VCDJ_YUV(PP_YUV420, VCDJ_HBD | VCDJ_FMT_L010, DEC_OUT_FRM_YUV420SP_L010),
    [VCDJ_OUT_YUV420SP_1010]   = VCDJ_YUV(PP_YUV420, VCDJ_HBD | VCDJ_FMT_1010, DEC_OUT_FRM_YUV420SP_1010),
    [VCDJ_OUT_YVU420SP_PACK10] = VCDJ_YUV(PP_YUV420, VCDJ_HBD | VCDJ_CRF, DEC_OUT_FRM_NV21SP_PACK10),
    [VCDJ_OUT_YVU420SP_P010]   = VCDJ_YUV(PP_YUV420, VCDJ_HBD | VCDJ_CRF | VCDJ_FMT_P010, DEC_OUT_FRM_NV21SP_P010),
    [VCDJ_OUT_YVU420SP_I010]   = VCDJ_YUV(PP_YUV420, VCDJ_HBD | VCDJ_CRF | VCDJ_FMT_I010, DEC_OUT_FRM_NV21SP_I010),
    [VCDJ_OUT_YVU420SP_L010]   = VCDJ_YUV(PP_YUV420, VCDJ_HBD | VCDJ_CRF | VCDJ_FMT_L010, DEC_OUT_FRM_NV21SP_L010),
    [VCDJ_OUT_YVU420SP_1010]   = VCDJ_YUV(PP_YUV420, VCDJ_HBD | VCDJ_CRF | VCDJ_FMT_1010, DEC_OUT_FRM_NV21SP_1010),
    [VCDJ_OUT_YUV420P_PACK10]  = VCDJ_YUV(PP_YUV420, VCDJ_HBD | VCDJ_PL, DEC_OUT_FRM_YUV420P_PACK10),
    [VCDJ_OUT_YUV420P_P010]    = VCDJ_YUV(PP_YUV420, VCDJ_HBD | VCDJ_PL | VCDJ_FMT_P010, DEC_OUT_FRM_YUV420P_P010),
    [VCDJ_OUT_YUV420P_I010]    = VCDJ_YUV(PP_YUV420, VCDJ_HBD | VCDJ_PL | VCDJ_FMT_I010, DEC_OUT_FRM_YUV420P_I010),
    [VCDJ_OUT_YUV420P_L010]    = VCDJ_YUV(PP_YUV420, VCDJ_HBD | VCDJ_PL | VCDJ_FMT_L010, DEC_OUT_FRM_YUV420P_L010),
    [VCDJ_OUT_YVU420P_PACK10]  = VCDJ_YUV(PP_YUV420, VCDJ_HBD | VCDJ_PL | VCDJ_CRF, DEC_OUT_FRM_NV21P_PACK10),
    [VCDJ_OUT_YVU420P_P010]    = VCDJ_YUV(PP_YUV420, VCDJ_HBD | VCDJ_PL | VCDJ_CRF | VCDJ_FMT_P010, DEC_OUT_FRM_NV21P_P010),
    [VCDJ_OUT_YVU420P_I010]    = VCDJ_YUV(PP_YUV420, VCDJ_HBD | VCDJ_PL | VCDJ_CRF | VCDJ_FMT_I010, DEC_OUT_FRM_NV21P_I010),
    [VCDJ_OUT_YVU420P_L010]    = VCDJ_YUV(PP_YUV420, VCDJ_HBD | VCDJ_PL | VCDJ_CRF | VCDJ_FMT_L010, DEC_OUT_FRM_NV21P_L010),
    [VCDJ_OUT_YUV422SP_P010]   = VCDJ_YUV(PP_YUV422, VCDJ_HBD | VCDJ_FMT_P010, DEC_OUT_FRM_YUV422SP_P010),
    [VCDJ_OUT_YUV422SP_I010]   = VCDJ_YUV(PP_YUV422, VCDJ_HBD | VCDJ_FMT_I010, DEC_OUT_FRM_YUV422SP_I010),
    [VCDJ_OUT_YUV422SP_L010]   = VCDJ_YUV(PP_YUV422, VCDJ_HBD | VCDJ_FMT_L010, DEC_OUT_FRM_YUV422SP_L010),
    [VCDJ_OUT_YVU422SP_P010]   = VCDJ_YUV(PP_YUV422, VCDJ_HBD | VCDJ_CRF | VCDJ_FMT_P010, DEC_OUT_FRM_422_NV21SP_P010),
    [VCDJ_OUT_YVU422SP_I010]   = VCDJ_YUV(PP_YUV422, VCDJ_HBD | VCDJ_CRF | VCDJ_FMT_I010, DEC_OUT_FRM_422_NV21SP_I010),
    [VCDJ_OUT_YVU422SP_L010]   = VCDJ_YUV(PP_YUV422, VCDJ_HBD | VCDJ_CRF | VCDJ_FMT_L010, DEC_OUT_FRM_422_NV21SP_L010),
    [VCDJ_OUT_YUV422P_P010]    = VCDJ_YUV(PP_YUV422, VCDJ_HBD | VCDJ_PL | VCDJ_FMT_P010, DEC_OUT_FRM_YUV422P_P010),
    [VCDJ_OUT_YUV422P_I010]    = VCDJ_YUV(PP_YUV422, VCDJ_HBD | VCDJ_PL | VCDJ_FMT_I010, DEC_OUT_FRM_YUV422P_I010),
    [VCDJ_OUT_YUV422P_L010]    = VCDJ_YUV(PP_YUV422, VCDJ_HBD | VCDJ_PL | VCDJ_FMT_L010, DEC_OUT_FRM_YUV422P_L010),
    [VCDJ_OUT_YVU422P_P010]    = VCDJ_YUV(PP_YUV422, VCDJ_HBD | VCDJ_PL | VCDJ_CRF | VCDJ_FMT_P010, DEC_OUT_FRM_422_NV21P_P010),
    [VCDJ_OUT_YVU422P_I010]    = VCDJ_YUV(PP_YUV422, VCDJ_HBD | VCDJ_PL | VCDJ_CRF | VCDJ_FMT_I010, DEC_OUT_FRM_422_NV21P_I010),
    [VCDJ_OUT_YVU422P_L010]    = VCDJ_YUV(PP_YUV422, VCDJ_HBD | VCDJ_PL | VCDJ_CRF | VCDJ_FMT_L010, DEC_OUT_FRM_422_NV21P_L010),
    [VCDJ_OUT_YUV444P_P010]    = VCDJ_YUV(PP_YUV444, VCDJ_HBD | VCDJ_PL | VCDJ_FMT_P010, DEC_OUT_FRM_YUV444P_P010),
    [VCDJ_OUT_YUV444P_I010]    = VCDJ_YUV(PP_YUV444, VCDJ_HBD | VCDJ_PL | VCDJ_FMT_I010, DEC_OUT_FRM_YUV444P_I010),
    [VCDJ_OUT_YUV444P_L010]    = VCDJ_YUV(PP_YUV444, VCDJ_HBD | VCDJ_PL | VCDJ_FMT_L010, DEC_OUT_FRM_YUV444P_L010),
    [VCDJ_OUT_YVU444P_P010]    = VCDJ_YUV(PP_YUV444, VCDJ_HBD | VCDJ_PL | VCDJ_CRF | VCDJ_FMT_P010, DEC_OUT_FRM_444_NV21P_P010),
    [VCDJ_OUT_YVU444P_I010]    = VCDJ_YUV(PP_YUV444, VCDJ_HBD | VCDJ_PL | VCDJ_CRF | VCDJ_FMT_I010, DEC_OUT_FRM_444_NV21P_I010),
    [VCDJ_OUT_YVU444P_L010]    = VCDJ_YUV(PP_YUV444, VCDJ_HBD | VCDJ_PL | VCDJ_CRF | VCDJ_FMT_L010, DEC_OUT_FRM_444_NV21P_L010),
};

#define VCDJ_FMT_COUNT  (sizeof(vcdj_fmt_table) / sizeof(vcdj_fmt_table[0]))

static const struct vcdj_fmt_desc *vcdj_fmt_lookup(vcdj_out_format_t fmt)
{
    if (((uint32_t)fmt >= VCDJ_FMT_COUNT) ||
        (vcdj_fmt_table[fmt].kind == VCDJ_KIND_NONE))
    {
        return NULL;
    }

    return &vcdj_fmt_table[fmt];
}

static int vcdj_out_format_supported(vcdj_out_format_t out_format)
{
    return (vcdj_fmt_lookup(out_format) != NULL) ? 1 : 0;
}

/**
 * Reject a format the source can never produce, before the vendor sees it.
 *
 * Without this a 10/16-bit request on an 8-bit stream would not fail: ppu.c
 * clears the 10-bit flags for 8-bit input and silently emits 8-bit pixels.
 */
static vcdj_error_t vcdj_format_fits_source(vcdj_out_format_t out_format,
                                            vcdj_codec_t codec,
                                            uint32_t bit_depth)
{
    const struct vcdj_fmt_desc *fmt = vcdj_fmt_lookup(out_format);

    if (fmt == NULL)
    {
        return VCDJ_ERR_PARAM;
    }
    if (((fmt->flags & VCDJ_FMT_HBD) != 0U) &&
        ((codec != VCDJ_CODEC_JPEG) || (bit_depth <= 8U)))
    {
        return VCDJ_ERR_UNSUPPORTED;
    }
    if (((fmt->flags & VCDJ_FMT_PNG_ONLY) != 0U) && (codec != VCDJ_CODEC_PNG))
    {
        return VCDJ_ERR_UNSUPPORTED;
    }

    return VCDJ_SUCCESS;
}

static int vcdj_pp_filter_supported(vcdj_pp_filter_t filter)
{
    return ((filter == VCDJ_PP_FILTER_DEFAULT) ||
            (filter == VCDJ_PP_FILTER_NEAREST) ||
            (filter == VCDJ_PP_FILTER_BILINEAR) ||
            (filter == VCDJ_PP_FILTER_BICUBIC) ||
            (filter == VCDJ_PP_FILTER_AREA)) ? 1 : 0;
}

static vcdj_error_t vcdj_validate_pp(const vcdj_pp_config_t *pp,
                                     uint32_t input_width,
                                     uint32_t input_height)
{
    uint32_t crop_right;
    uint32_t crop_bottom;
    uint32_t source_width;
    uint32_t source_height;
    uint32_t max_output_width;
    uint32_t max_output_height;
    unsigned int i;

    if ((pp == NULL) || (input_width == 0U) || (input_height == 0U) ||
        !vcdj_out_format_supported(pp->output_format) ||
        !vcdj_pp_filter_supported(pp->filter) ||
        (pp->crop_enabled > 1U) || (pp->scale_enabled > 1U) ||
        (pp->reserved0 != 0U))
    {
        return VCDJ_ERR_PARAM;
    }

    for (i = 0U; i < sizeof(pp->reserved); i++)
    {
        if (pp->reserved[i] != 0U)
        {
            return VCDJ_ERR_PARAM;
        }
    }

    if (pp->crop_enabled != 0U)
    {
        if ((pp->crop.width == 0U) || (pp->crop.height == 0U) ||
            ((pp->crop.width < VCDJ_PP_MIN_DIMENSION) &&
             (pp->crop.width != input_width)) ||
            ((pp->crop.height < VCDJ_PP_MIN_DIMENSION) &&
             (pp->crop.height != input_height)) ||
            (pp->crop.x > UINT32_MAX - pp->crop.width) ||
            (pp->crop.y > UINT32_MAX - pp->crop.height))
        {
            return VCDJ_ERR_PARAM;
        }

        crop_right = pp->crop.x + pp->crop.width;
        crop_bottom = pp->crop.y + pp->crop.height;
        if ((crop_right > input_width) || (crop_bottom > input_height))
        {
            return VCDJ_ERR_PARAM;
        }

        source_width = pp->crop.width;
        source_height = pp->crop.height;
    }
    else
    {
        if ((pp->crop.x != 0U) || (pp->crop.y != 0U) ||
            (pp->crop.width != 0U) || (pp->crop.height != 0U))
        {
            return VCDJ_ERR_PARAM;
        }

        source_width = input_width;
        source_height = input_height;
    }

    if (pp->scale_enabled != 0U)
    {
        max_output_width = (source_width >
                            (VCDJ_PP_MAX_OUTPUT_DIMENSION /
                             VCDJ_PP_MAX_UPSCALE_RATIO)) ?
                           VCDJ_PP_MAX_OUTPUT_DIMENSION :
                           source_width * VCDJ_PP_MAX_UPSCALE_RATIO;
        max_output_height = (source_height >
                             (VCDJ_PP_MAX_OUTPUT_DIMENSION /
                              VCDJ_PP_MAX_UPSCALE_RATIO)) ?
                            VCDJ_PP_MAX_OUTPUT_DIMENSION :
                            source_height * VCDJ_PP_MAX_UPSCALE_RATIO;

        if ((pp->output_width < VCDJ_PP_MIN_DIMENSION) ||
            (pp->output_height < VCDJ_PP_MIN_DIMENSION) ||
            (pp->output_width > max_output_width) ||
            (pp->output_height > max_output_height))
        {
            return VCDJ_ERR_PARAM;
        }

        if ((pp->filter == VCDJ_PP_FILTER_AREA) &&
            ((pp->output_width > source_width) ||
             (pp->output_height > source_height) ||
             ((pp->output_width == source_width) &&
              (pp->output_height == source_height))))
        {
            return VCDJ_ERR_UNSUPPORTED;
        }
    }
    else if ((pp->output_width != 0U) || (pp->output_height != 0U) ||
             (pp->filter != VCDJ_PP_FILTER_DEFAULT))
    {
        return VCDJ_ERR_PARAM;
    }

    return VCDJ_SUCCESS;
}

static u32 vcdj_pp_filter_to_vendor(vcdj_pp_filter_t filter)
{
    /* Cast at the trust boundary so the defensive default also covers values
     * outside the public enum without triggering -Wcovered-switch-default. */
    switch ((uint32_t)filter)
    {
    case VCDJ_PP_FILTER_NEAREST:
        return NEAREST;
    case VCDJ_PP_FILTER_BICUBIC:
        return FAST_BICUBIC;
    case VCDJ_PP_FILTER_AREA:
        return AREA;
    case VCDJ_PP_FILTER_DEFAULT:
    case VCDJ_PP_FILTER_BILINEAR:
        return FAST_LINEAR;
    default:
        return VSI_LINEAR;
    }
}

/*
 * Report what the post processing unit actually produced, by looking the vendor
 * format up in the same table the request side uses.
 *
 * This is not a mirror of the request. When YUV is requested for a PNG,
 * PngDecSetInfo() picks RGB from the colour type instead (truecolour/indexed ->
 * BGR888, RGBA or tRNS -> ABGR8888). Greyscale PNG is forced onto the luma-only
 * path whatever was requested, and so is greyscale JPEG when YUV is requested.
 *
 * Two vendor formats are ambiguous and need the request to resolve: YUYV / YVYU
 * (and UYVY / VYUY) share one vendor name and differ only in cr_first. Anything
 * the table does not name - tiled, compressed, histogram - is reported as
 * UNKNOWN rather than guessed.
 */
static vcdj_out_format_t vcdj_out_format_from_vendor(enum DecPictureFormat fmt,
                                                     vcdj_out_format_t requested)
{
    const struct vcdj_fmt_desc *req = vcdj_fmt_lookup(requested);
    unsigned int i;

    if ((req != NULL) && (req->vendor == (uint16_t)fmt))
    {
        return requested;
    }

    for (i = 0U; i < VCDJ_FMT_COUNT; i++)
    {
        if ((vcdj_fmt_table[i].kind != VCDJ_KIND_NONE) &&
            (vcdj_fmt_table[i].vendor == (uint16_t)fmt))
        {
            return (vcdj_out_format_t)i;
        }
    }

    return VCDJ_OUT_UNKNOWN;
}

/** Colour model of a JPEG, derived from the chroma format the parser reported. */
static vcdj_color_type_t vcdj_jpeg_color_type(u32 output_format)
{
    return (output_format == DEC_OUT_FRM_YUV400) ? VCDJ_COLOR_GRAY
           : VCDJ_COLOR_YCBCR;
}

/** Component count of a JPEG, derived the same way. */
static uint8_t vcdj_jpeg_channels(u32 output_format)
{
    return (output_format == DEC_OUT_FRM_YUV400) ? 1U : 3U;
}

/** Map the PNG colour type byte from the header onto the public colour model. */
static vcdj_color_type_t vcdj_png_color_type(u32 color_type)
{
    switch (color_type)
    {
    case PNG_COLOR_TYPE_GRAY:
        return VCDJ_COLOR_GRAY;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        return VCDJ_COLOR_GRAY_ALPHA;
    case PNG_COLOR_TYPE_RGB:
        return VCDJ_COLOR_RGB;
    case PNG_COLOR_TYPE_RGB_ALPHA:
        return VCDJ_COLOR_RGB_ALPHA;
    case PNG_COLOR_TYPE_INDEX:
        return VCDJ_COLOR_INDEXED;
    default:
        return VCDJ_COLOR_UNKNOWN;
    }
}

/**
 * Reject a codec value that is outside \ref vcdj_codec_t.
 *
 * The enumeration keeps the legal values discoverable, but C does not stop a
 * caller from assigning anything that fits the underlying type, so the trust
 * boundary still has to check.
 */
static int vcdj_codec_supported(vcdj_codec_t codec)
{
    return ((codec == VCDJ_CODEC_JPEG) || (codec == VCDJ_CODEC_PNG)) ? 1 : 0;
}

static void vcdj_setup_ppu(PpUnitConfig *ppu, const vcdj_pp_config_t *config)
{
    const struct vcdj_fmt_desc *fmt;

    memset(ppu, 0, sizeof(PpUnitConfig) * DEC_MAX_OUT_COUNT);

    ppu[0].enabled = 1U;
    ppu[0].align = DEC_ALIGN_64B;
    /* Mandatory: with the shaper off the core never ends the frame. */
    ppu[0].shaper_enabled = 1U;
    /* Only takes effect when bit depth drops (e.g. RGB565); no-op otherwise. */
    ppu[0].dither_enable = 1U;
    ppu[0].crop.enabled = config->crop_enabled;
    ppu[0].crop.set_by_user = config->crop_enabled;
    ppu[0].crop.x = config->crop.x;
    ppu[0].crop.y = config->crop.y;
    ppu[0].crop.width = config->crop.width;
    ppu[0].crop.height = config->crop.height;
    ppu[0].scale.enabled = config->scale_enabled;
    ppu[0].scale.scale_by_ratio = 0U;
    ppu[0].scale.width = config->output_width;
    ppu[0].scale.height = config->output_height;
    if (config->scale_enabled != 0U)
    {
        ppu[0].pp_filter = vcdj_pp_filter_to_vendor(config->filter);
        ppu[0].antialias = (config->filter == VCDJ_PP_FILTER_AREA) ? 1U : 0U;
    }

    fmt = vcdj_fmt_lookup(config->output_format);
    if (fmt == NULL)
    {
        /* Callers validate first; keep the vendor default rather than guess. */
        ppu[0].chroma_format = PP_YUV420;
        return;
    }

    ppu[0].chroma_format = fmt->chroma;
    ppu[0].cr_first = ((fmt->flags & VCDJ_FMT_CR_FIRST) != 0U) ? 1U : 0U;
    ppu[0].planar = ((fmt->flags & VCDJ_FMT_PLANAR) != 0U) ? 1U : 0U;
    ppu[0].out_p010 = ((fmt->flags & VCDJ_FMT_P010) != 0U) ? 1U : 0U;
    ppu[0].out_I010 = ((fmt->flags & VCDJ_FMT_I010) != 0U) ? 1U : 0U;
    ppu[0].out_L010 = ((fmt->flags & VCDJ_FMT_L010) != 0U) ? 1U : 0U;
    ppu[0].out_1010 = ((fmt->flags & VCDJ_FMT_1010) != 0U) ? 1U : 0U;

    switch (fmt->kind)
    {
    case VCDJ_KIND_RGB:
        /* The decoder core emits YUV; the RGB colour space conversion lives in
         * the vendor post processing unit (ppu.c), which is why ppu.c is not an
         * optional part of the compiled subset. */
        ppu[0].rgb = 1U;
        ppu[0].rgb_format = fmt->rgb_format;
        break;
    case VCDJ_KIND_RGB_PLANAR:
        ppu[0].rgb_planar = 1U;
        ppu[0].rgb_format = fmt->rgb_format;
        break;
    case VCDJ_KIND_LUMA:
        /*
         * Explicit luma-only also drops PNG alpha: PngDecSetInfo() only picks
         * the luma + alpha layout when monochrome was NOT requested, which is
         * why VCDJ_OUT_YUV400_ALPHA is a plain YUV request in the table.
         */
        ppu[0].monochrome = 1U;
        break;
    case VCDJ_KIND_YUYV:
        ppu[0].out_yuyv = 1U;
        break;
    case VCDJ_KIND_UYVY:
        ppu[0].out_uyvy = 1U;
        break;
    default:
        break;
    }
}

static void vcdj_setup_default_pp(PpUnitConfig *ppu,
                                  vcdj_out_format_t out_format)
{
    vcdj_pp_config_t config;

    memset(&config, 0, sizeof(config));
    config.output_format = out_format;
    config.filter = VCDJ_PP_FILTER_DEFAULT;
    vcdj_setup_ppu(ppu, &config);
}

static void vcdj_release_input(const void *dwl, struct DWLLinearMem *mem)
{
    if (mem->virtual_address != NULL)
    {
        DWLFreeLinear(dwl, mem);
    }
}

static vcdj_error_t vcdj_prepare_input(const void *dwl, const vcdj_dec_param_t *param,
                                       struct DWLLinearMem *mem)
{
    if (DWLMallocLinear(dwl, param->size, mem) != DWL_OK)
    {
        return VCDJ_ERR_NO_MEM;
    }

    (void)memcpy(mem->virtual_address, param->data, param->size);
    vcdj_core_cache_clean(mem->virtual_address, param->size);

    return VCDJ_SUCCESS;
}

static enum DecRet vcdj_decode(
    void *inst,
    enum DecRet(*decode)(void *, void *, void *),
    void *input, void *output)
{
    return decode(inst, input, output);
}

static enum DecRet vcdj_get_buffer_info(
    void *inst,
    enum DecRet(*get_info)(void *, struct DecBufferInfo *),
    struct DecBufferInfo *info)
{
    return get_info(inst, info);
}

/**
 * Discard the buffer the vendor asked us to hand back.
 *
 * The re-allocation protocol works by returning the OLD descriptor in
 * `DecBufferInfo.buf_to_free` while asking for a larger buffer. The vendor has
 * already dropped its own reference by then - `JpegDecGetBufferInfo()` zeroes
 * `dec_cont->buf_to_free` right after copying it out - so the memory is ours to
 * release and leaking it here is invisible until the heap runs dry.
 *
 * Matching is by address rather than by index because the descriptor the vendor
 * returns is a copy, not the object we registered.
 */
static void vcdj_release_returned_buffer(const struct DWLDecBuffers *to_free,
                                         void **buffers, unsigned int *count)
{
    const u32 *addr = to_free->dec_buffer[0][0].virtual_address;
    unsigned int i;

    if (addr == NULL)
    {
        return;     /* nothing to hand back this round */
    }

    for (i = 0U; i < *count; i++)
    {
        struct DWLDecBuffers *owned = (struct DWLDecBuffers *)buffers[i];

        if ((owned == NULL) || (owned->dec_buffer[0][0].virtual_address != addr))
        {
            continue;
        }

        vcdj_free_all_planes(owned);
        vcdj_mem_free(owned);

        /* Keep the array dense: move the last entry down into the hole. */
        (*count)--;
        buffers[i] = buffers[*count];
        buffers[*count] = NULL;
        return;
    }
}

static int vcdj_add_external_buffers(const void *dwl, void *inst,
                                     enum DecRet(*get_info)(void *, struct DecBufferInfo *),
                                     enum DecRet(*add_buffer)(void *, struct DWLLinearMem *),
                                     void **buffers, unsigned int capacity,
                                     unsigned int *count,
                                     struct vcdj_buffer_layout *layout)
{
    struct DecBufferInfo *info;
    enum DecRet ret;
    unsigned int i;
    unsigned int required;

    if ((dwl == NULL) || (inst == NULL) || (get_info == NULL) ||
        (add_buffer == NULL) || (buffers == NULL) || (count == NULL))
    {
        return (int)DEC_PARAM_ERROR;
    }

    info = vcdj_mem_alloc(sizeof(*info));
    if (info == NULL)
    {
        return (int)DEC_MEMFAIL;
    }

    memset(info, 0, sizeof(*info));
    ret = vcdj_get_buffer_info(inst, get_info, info);
    if ((ret != DEC_OK) && (ret != DEC_WAITING_FOR_BUFFER))
    {
        vcdj_mem_free(info);
        return (int)ret;
    }

    /* Do this before allocating: the round that asks for a bigger buffer is the
     * same round that hands the old one back. */
    vcdj_release_returned_buffer(&info->buf_to_free, buffers, count);

    if ((info->next_buf_size == 0U) || (info->buf_num == 0U))
    {
        /* Release-only round: the vendor wanted the old buffer back and nothing
         * more. Reporting DEC_OK lets the caller re-enter Decode(). */
        vcdj_mem_free(info);
        return (int)DEC_OK;
    }

    required = info->buf_num;
    if ((required > capacity) || ((*count + required) > capacity))
    {
        vcdj_mem_free(info);
        return (int)DEC_MEMFAIL;
    }

    /*
     * Report the geometry the vendor computed. Every value here comes from
     * `DecBufferInfo`; none of it is derived from width * height * bpp, because
     * the post processing unit aligns each line and the resulting sizes cannot be
     * reproduced from the picture dimensions (5.1 item 4 of the interface plan).
     */
    if (layout != NULL)
    {
        unsigned int pl;

        layout->total_size = (unsigned long long)info->next_buf_size;
        layout->ystride = info->ystride[0];
        layout->cstride = info->cstride[0];

        for (pl = 0U; pl < VCDJ_MAX_PLANES; pl++)
        {
            layout->plane_size[pl] = (unsigned long long)info->output_buf_size[0][pl];
        }
    }

    for (i = 0U; i < required; i++)
    {
        struct DWLDecBuffers *buffer;
        struct DWLLinearMem *mem;

        /*
         * Despite its DWLLinearMem pointer type, the vendor AddBuffer API casts
         * this argument to DWLDecBuffers and copies the complete output matrix
         * into InputQueue, so the enclosing type has to be the one we allocate.
         *
         * ONE ALLOCATION, NOT ONE PER PLANE. With `use_separated_pp == 0` - which
         * is our configuration, since misc_ctrl leaves bit 16 clear - the vendor
         * addresses every plane of every channel as an offset into
         * `dec_buffer[0][0]`: SwSetPpOutAddr() reads that one descriptor and adds
         * `ppu_cfg->luma_offset` / `chroma_offset`. Splitting the planes into
         * separate allocations would put them at unrelated addresses and the
         * chroma offset would land outside its buffer. `next_buf_size` is
         * therefore the size of the whole multi-plane block.
         */
        buffer = vcdj_mem_alloc(sizeof(*buffer));
        if (buffer == NULL)
        {
            vcdj_mem_free(info);
            return (int)DEC_MEMFAIL;
        }
        memset(buffer, 0, sizeof(*buffer));
        mem = &buffer->dec_buffer[0][0];
        mem->mem_type = DWL_MEM_TYPE_DPB | DWL_MEM_TYPE_SYNC_DEVICE_TO_HOST;
        SET_MEM_USAGE(mem->mem_type, DWL_MEM_USAGE_OUT_REFERENCE, 0U);

        if (DWLMallocRefFrm(dwl, info->next_buf_size, mem) != DWL_OK)
        {
            vcdj_mem_free(buffer);
            vcdj_mem_free(info);
            return (int)DEC_MEMFAIL;
        }

        ret = add_buffer(inst, (struct DWLLinearMem *)buffer);
        if (ret != DEC_OK)
        {
            DWLFreeRefFrm(dwl, mem);
            vcdj_mem_free(buffer);
            vcdj_mem_free(info);
            return (int)ret;
        }

        buffers[*count] = buffer;
        (*count)++;
    }

    vcdj_mem_free(info);
    return (int)DEC_OK;
}

/*============================================================================*
 *                        Picture delivery
 *============================================================================*/
/**
 * \brief Expose the planes the vendor does not hand out separately.
 *
 * With `use_separated_pp == 0` SwSetPpOutAddr() reports at most two addresses,
 * while the core writes three planes back to back (ppu.c SetPpuRegs):
 *  - planar YUV: the second chroma plane starts right after the first, and the
 *    negotiation already reports its size in `plane_size[2]`;
 *  - planar RGB: all three planes are counted as one, each a third of it.
 */
static void vcdj_split_planar(const struct vcdj_buffer_layout *layout,
                              vcdj_picture_t *pic)
{
    const struct vcdj_fmt_desc *fmt = vcdj_fmt_lookup(pic->format);
    vcdj_plane_t *p = pic->planes;
    unsigned int i;

    if (fmt == NULL)
    {
        return;
    }

    if ((fmt->kind == VCDJ_KIND_RGB_PLANAR) && (pic->plane_count == 1U))
    {
        uint32_t size = p[0].size / 3U;

        for (i = 0U; i < 3U; i++)
        {
            p[i].cpu_addr = (uint8_t *)p[0].cpu_addr + (i * size);
            p[i].bus_addr = p[0].bus_addr + (i * size);
            p[i].size = size;
            p[i].stride = p[0].stride;
            p[i].offset = 0U;
        }
        pic->plane_count = 3U;
    }
    else if ((fmt->kind == VCDJ_KIND_YUV) &&
             ((fmt->flags & VCDJ_FMT_PLANAR) != 0U) &&
             (pic->plane_count == 2U) && (layout->plane_size[2] != 0U))
    {
        p[2].cpu_addr = (uint8_t *)p[1].cpu_addr + p[1].size;
        p[2].bus_addr = p[1].bus_addr + p[1].size;
        p[2].size = (uint32_t)layout->plane_size[2];
        p[2].stride = layout->cstride;
        p[2].offset = 0U;
        pic->plane_count = 3U;
    }
}

/**
 * \brief Fill \ref vcdj_picture_t.planes from what the vendor actually produced.
 *
 * Shared by the JPEG and the PNG path on purpose: the three vendor descriptors
 * have identical meaning in `JpegDecOutput.pictures[]` and
 * `PngDecOutput.pictures[]`, so passing them in individually keeps one
 * implementation for both codecs without a vendor type reaching the public header.
 *
 * \param luma,chroma,chroma_cr  Plane base addresses NextPicture() reported.
 * \param layout                 Sizes and strides from the buffer negotiation.
 * \param pic                    Receives `planes[]` and `plane_count`.
 *
 * A plane counts as present only when \ref vcdj_buffer_layout.plane_size says so.
 * The addresses cannot be used for that test: for packed RGB the vendor still
 * derives a chroma address as `base + chroma_offset`, and because
 * `chroma_offset == luma_size` with a zero chroma size, that address is one past
 * the end of the buffer. Believing it would publish memory the caller has no
 * right to touch.
 */
static void vcdj_fill_planes(const struct DWLLinearMem *luma,
                             const struct DWLLinearMem *chroma,
                             const struct DWLLinearMem *chroma_cr,
                             const struct vcdj_buffer_layout *layout,
                             vcdj_picture_t *pic)
{
    const struct DWLLinearMem *src[VCDJ_MAX_PLANES];
    unsigned int i;

    src[0] = luma;
    src[1] = chroma;
    src[2] = chroma_cr;

    pic->plane_count = 0U;

    for (i = 0U; i < VCDJ_MAX_PLANES; i++)
    {
        if ((layout->plane_size[i] == 0U) || (src[i]->virtual_address == NULL))
        {
            break;      /* planes are contiguous; the first gap ends the picture */
        }

        pic->planes[i].cpu_addr = (void *)src[i]->virtual_address;
        pic->planes[i].bus_addr = (uintptr_t)src[i]->bus_address;
        pic->planes[i].size = (uint32_t)layout->plane_size[i];
        pic->planes[i].stride = (i == 0U) ? layout->ystride : layout->cstride;

        /*
         * Zero, and not the vendor's luma_offset / chroma_offset. Those offsets
         * are relative to the start of the whole multi-plane block, and
         * NextPicture() has already applied them to the addresses above, so
         * repeating them here would skip the data twice. The first visible pixel
         * of each plane sits exactly at its reported address.
         */
        pic->planes[i].offset = 0U;

        pic->plane_count++;
    }

    vcdj_split_planar(layout, pic);

    /*
     * Make the pixels visible to the CPU before the caller reads them: the core
     * wrote them behind our back through its own bus master.
     *
     * TODO(hardware-contract): vcdj_core_cache_invalidate() is still only a
     * barrier because the memory attributes and the maintenance granularity are
     * unknown (A4). The call is placed correctly, so when A4 lands the range
     * operation drops in here without restructuring anything.
     */
    for (i = 0U; i < pic->plane_count; i++)
    {
        vcdj_core_cache_invalidate(pic->planes[i].cpu_addr, pic->planes[i].size);
    }
}

/**
 * \brief Hand the decoded pixels to the caller and take ownership of the memory.
 *
 * Claims a live slot, moves the registered descriptor into it and mints the
 * release token. On success the buffer is no longer owned by the decode path -
 * `*owned_count` is cleared for that entry - so the normal cleanup must not free
 * it; \ref hal_vcdj_picture_release does that later.
 *
 * \return \ref VCDJ_SUCCESS, or \ref VCDJ_ERR_STATE when the caller has not
 *         released the previous picture yet.
 */
static vcdj_error_t vcdj_publish_picture(void **buffers, unsigned int *count,
                                         vcdj_picture_t *pic)
{
    struct vcdj_live_picture *live;
    unsigned int slot = 0U;

    if (*count == 0U)
    {
        /* No external buffer was ever registered, so there are no pixels to give
         * away. Geometry has already been reported; leave plane_count at 0. */
        return VCDJ_SUCCESS;
    }

    live = vcdj_live_claim(&slot);
    if (live == NULL)
    {
        /*
         * Refuse rather than overwrite. The alternative - silently freeing the
         * picture the caller is still holding - would corrupt whatever is being
         * displayed from it.
         */
        return VCDJ_ERR_STATE;
    }

    /*
     * Exactly one buffer is transferred: `buf_num` is hardcoded to 1 by both
     * codecs, and the single descriptor covers every plane of the output.
     */
    live->buffer = (struct DWLDecBuffers *)buffers[0];
    buffers[0] = NULL;
    *count = 0U;

    pic->release_token = vcdj_token_encode(slot, live->generation);

    return VCDJ_SUCCESS;
}

/*============================================================================*
 *                        Cross-file driver internals
 *============================================================================*/
int vcdj_internal_jpeg_decode(void *inst, void *input, void *output)
{
    return (int)vcdj_decode(
               inst, (enum DecRet(*)(void *, void *, void *))JpegDecDecode,
               input, output);
}

int vcdj_internal_add_jpeg_buffers(void *inst, void **buffers,
                                   unsigned int capacity,
                                   unsigned int *count,
                                   struct vcdj_buffer_layout *layout)
{
    return vcdj_add_external_buffers(vcdj_dwl_handle, inst,
                                     (enum DecRet(*)(void *, struct DecBufferInfo *))JpegDecGetBufferInfo,
                                     (enum DecRet(*)(void *, struct DWLLinearMem *))JpegDecAddBuffer,
                                     buffers, capacity, count, layout);
}

int vcdj_internal_add_png_buffers(void *inst, void **buffers,
                                  unsigned int capacity,
                                  unsigned int *count,
                                  struct vcdj_buffer_layout *layout)
{
    return vcdj_add_external_buffers(vcdj_dwl_handle, inst,
                                     (enum DecRet(*)(void *, struct DecBufferInfo *))PngDecGetBufferInfo,
                                     (enum DecRet(*)(void *, struct DWLLinearMem *))PngDecAddBuffer,
                                     buffers, capacity, count, layout);
}

void vcdj_internal_free_external_buffers(void **buffers, unsigned int count)
{
    unsigned int i;

    if (buffers == NULL)
    {
        return;
    }

    for (i = 0U; i < count; i++)
    {
        struct DWLDecBuffers *buffer = (struct DWLDecBuffers *)buffers[i];

        if (buffer != NULL)
        {
            vcdj_free_all_planes(buffer);
            vcdj_mem_free(buffer);
            buffers[i] = NULL;
        }
    }
}

const void *vcdj_internal_dwl(void)
{
    return vcdj_dwl_handle;
}

/*============================================================================*
 *                        JPEG path
 *============================================================================*/
static vcdj_error_t vcdj_jpeg_image_info(const vcdj_dec_param_t *param,
                                         vcdj_image_info_t *info)
{
    struct vcdj_jpeg_work *work;
    JpegDecInst inst = NULL;
    enum DecRet ret;
    vcdj_error_t err;

    work = vcdj_mem_alloc(sizeof(*work));
    if (work == NULL)
    {
        return VCDJ_ERR_NO_MEM;
    }
    memset(work, 0, sizeof(*work));

    work->cfg.decoder_mode = DEC_NORMAL;
    work->cfg.align = DEC_ALIGN_64B;
    work->cfg.error_handling = DEC_EC_FRAME_NO_ERROR;
    work->cfg.dec_image_type = JPEGDEC_IMAGE;
    vcdj_setup_default_pp(work->cfg.ppu_config, param->out_format);

    ret = JpegDecInit(&inst, vcdj_dwl_handle, &work->cfg);
    if (ret != DEC_OK)
    {
        err = vcdj_map_dec_ret(ret);
        goto out_work;
    }

    err = vcdj_prepare_input(vcdj_dwl_handle, param, &work->dec_in.stream_buffer);
    if (err != VCDJ_SUCCESS)
    {
        goto out_inst;
    }

    work->dec_in.stream_length = param->size;
    work->dec_in.buffer_size = 0U;
    work->dec_in.dec_image_type = JPEGDEC_IMAGE;
    work->dec_in.stream = (u8 *)work->dec_in.stream_buffer.virtual_address;

    ret = JpegDecGetImageInfo(inst, &work->dec_in, &work->image_info);

    if (ret == DEC_OK)
    {
        info->width = work->image_info.display_width;
        info->height = work->image_info.display_height;
        info->aligned_width = work->image_info.output_width;
        info->aligned_height = work->image_info.output_height;
        info->bit_depth = work->image_info.bit_depth;
        info->codec = VCDJ_CODEC_JPEG;
        info->color_type = vcdj_jpeg_color_type(work->image_info.output_format);
        info->channels = vcdj_jpeg_channels(work->image_info.output_format);
        info->has_alpha = 0U;    /* JPEG has no alpha channel */

        info->jpeg_progressive = (work->image_info.coding_mode == JPEG_PROGRESSIVE) ? 1U : 0U;
        info->jpeg_has_thumbnail = (work->image_info.thumbnail_type != 0U) ? 1U : 0U;
        info->jpeg_chroma = vcdj_map_jpeg_chroma(work->image_info.output_format);

        info->png_interlaced = 0U;
    }

    err = vcdj_map_dec_ret(ret);
    vcdj_release_input(vcdj_dwl_handle, &work->dec_in.stream_buffer);

out_inst:
    JpegDecRelease(inst);
out_work:
    vcdj_mem_free(work);

    return err;
}

static vcdj_error_t vcdj_jpeg_decode(const vcdj_dec_param_t *param,
                                     const vcdj_pp_config_t *pp,
                                     vcdj_picture_t *pic)
{
    struct vcdj_jpeg_work *work;
    JpegDecOutput *dec_out = NULL;
    JpegDecInst inst = NULL;
    enum DecRet ret;
    vcdj_error_t err;
    void *external_buffers[VCDJ_MAX_EXTERNAL_BUFFERS] = {0};
    unsigned int external_count = 0U;
    unsigned int negotiations = 0U;
    struct vcdj_buffer_layout layout;

    memset(&layout, 0, sizeof(layout));

    work = vcdj_mem_alloc(sizeof(*work));
    if (work == NULL)
    {
        return VCDJ_ERR_NO_MEM;
    }
    memset(work, 0, sizeof(*work));

    work->cfg.decoder_mode = DEC_NORMAL;
    work->cfg.align = DEC_ALIGN_64B;
    work->cfg.error_handling = DEC_EC_FRAME_NO_ERROR;
    work->cfg.dec_image_type = JPEGDEC_IMAGE;

    ret = JpegDecInit(&inst, vcdj_dwl_handle, &work->cfg);
    if (ret != DEC_OK)
    {
        err = vcdj_map_dec_ret(ret);
        goto out_work;
    }

    err = vcdj_prepare_input(vcdj_dwl_handle, param, &work->dec_in.stream_buffer);
    if (err != VCDJ_SUCCESS)
    {
        goto out_inst;
    }

    work->dec_in.stream_length = param->size;
    work->dec_in.dec_image_type = JPEGDEC_IMAGE;
    /* Only the main picture is decoded. With 0 the parser switches the HW
     * geometry to the EXIF/JFXX thumbnail even though JPEGDEC_IMAGE is asked. */
    work->dec_in.thumbnail_done = 1U;
    work->dec_in.stream = (u8 *)work->dec_in.stream_buffer.virtual_address;

    ret = JpegDecGetImageInfo(inst, &work->dec_in, &work->image_info);
    if (ret != DEC_OK)
    {
        err = vcdj_map_dec_ret(ret);
        goto out_input;
    }

    err = vcdj_validate_pp(pp, work->image_info.display_width,
                           work->image_info.display_height);
    if (err == VCDJ_SUCCESS)
    {
        err = vcdj_format_fits_source(pp->output_format, VCDJ_CODEC_JPEG,
                                      work->image_info.bit_depth);
    }
    if (err != VCDJ_SUCCESS)
    {
        goto out_input;
    }

    vcdj_setup_ppu(work->cfg.ppu_config, pp);
    work->cfg.chroma_format = work->image_info.output_format;
    ret = JpegDecSetInfo(inst, &work->cfg);
    if (ret != DEC_OK)
    {
        err = vcdj_map_dec_ret(ret);
        goto out_input;
    }

    dec_out = vcdj_mem_alloc(sizeof(*dec_out));
    if (dec_out == NULL)
    {
        err = VCDJ_ERR_NO_MEM;
        goto out_input;
    }
    memset(dec_out, 0, sizeof(*dec_out));

    ret = vcdj_decode(inst, (enum DecRet(*)(void *, void *, void *))JpegDecDecode,
                      &work->dec_in, dec_out);
    while ((ret == DEC_WAITING_FOR_BUFFER) &&
           (negotiations < VCDJ_MAX_BUFFER_NEGOTIATIONS))
    {
        ret = (enum DecRet)vcdj_internal_add_jpeg_buffers(inst, external_buffers,
                                                          VCDJ_MAX_EXTERNAL_BUFFERS,
                                                          &external_count, &layout);
        if (ret != DEC_OK)
        {
            err = vcdj_map_dec_ret(ret);
            goto out_input;
        }
        negotiations++;
        ret = vcdj_decode(inst, (enum DecRet(*)(void *, void *, void *))JpegDecDecode,
                          &work->dec_in, dec_out);
    }
    if ((ret != DEC_PIC_RDY) && (ret != DEC_PIC_DECODED) && (ret != DEC_OK))
    {
        err = vcdj_map_dec_ret(ret);
        goto out_input;
    }

    ret = JpegDecNextPicture(inst, dec_out, &work->image_info);
    if ((ret != DEC_PIC_RDY) && (ret != DEC_OK))
    {
        err = vcdj_map_dec_ret(ret);
        goto out_input;
    }

    /*
     * Report the geometry and the format the post processing unit really chose,
     * taken from the picture the decoder produced rather than from what was
     * requested.
     */
    pic->width = dec_out->pictures[0].display_width;
    pic->height = dec_out->pictures[0].display_height;
    pic->format = vcdj_out_format_from_vendor(dec_out->pictures[0].output_format,
                                              pp->output_format);

    if ((pic->width == 0U) || (pic->height == 0U))
    {
        /* NextPicture() succeeded but reported nothing usable. Fall back to the
         * header geometry so the caller still gets a meaningful size. */
        pic->width = work->image_info.display_width;
        pic->height = work->image_info.display_height;
    }

    /*
     * Prefer the strides NextPicture() reported over the ones the negotiation
     * predicted: post processing can settle its alignment while configuring the
     * hardware, and this picture is the authority on what was written.
     */
    if (dec_out->pictures[0].pic_stride != 0U)
    {
        layout.ystride = dec_out->pictures[0].pic_stride;
    }
    if (dec_out->pictures[0].pic_stride_ch != 0U)
    {
        layout.cstride = dec_out->pictures[0].pic_stride_ch;
    }

    vcdj_fill_planes(&dec_out->pictures[0].output_picture_y,
                     &dec_out->pictures[0].output_picture_cb_cr,
                     &dec_out->pictures[0].output_picture_cr,
                     &layout, pic);

    /*
     * Give the queue slot back BEFORE releasing the instance, which is the order
     * the vendor protocol requires - PictureConsumed() reaches into the
     * InputQueue that JpegDecRelease() destroys.
     *
     * This says "the slot is reusable", not "the pixels are free": the memory is
     * ours (we allocated it with DWLMallocRefFrm) and the vendor never frees it,
     * so ownership can move to the caller in vcdj_publish_picture() below while
     * the vendor bookkeeping is wound down here.
     */
    (void)JpegDecPictureConsumed(inst, dec_out);

    err = vcdj_publish_picture(external_buffers, &external_count, pic);
    if (err != VCDJ_SUCCESS)
    {
        /* No slot free: the caller still owns the previous picture. Report the
         * state error rather than the geometry, and drop these pixels. */
        pic->plane_count = 0U;
        memset(pic->planes, 0, sizeof(pic->planes));
    }

out_input:
    vcdj_release_input(vcdj_dwl_handle, &work->dec_in.stream_buffer);
out_inst:
    JpegDecRelease(inst);
    vcdj_internal_free_external_buffers(external_buffers, external_count);
out_work:
    vcdj_mem_free(dec_out);
    vcdj_mem_free(work);

    return err;
}

/*============================================================================*
 *                        PNG path
 *============================================================================*/
static vcdj_error_t vcdj_png_image_info(const vcdj_dec_param_t *param,
                                        vcdj_image_info_t *info)
{
    struct vcdj_png_work *work;
    PngDecInst inst = NULL;
    enum DecRet ret;
    vcdj_error_t err;

    work = vcdj_mem_alloc(sizeof(*work));
    if (work == NULL)
    {
        return VCDJ_ERR_NO_MEM;
    }
    memset(work, 0, sizeof(*work));

    work->cfg.decoder_mode = DEC_NORMAL;
    work->cfg.align = DEC_ALIGN_64B;
    vcdj_setup_default_pp(work->cfg.ppu_config, param->out_format);

    ret = PngDecInit(&inst, vcdj_dwl_handle, &work->cfg);
    if (ret != DEC_OK)
    {
        err = vcdj_map_dec_ret(ret);
        goto out_work;
    }

    err = vcdj_prepare_input(vcdj_dwl_handle, param, &work->dec_in.stream_buffer);
    if (err != VCDJ_SUCCESS)
    {
        goto out_inst;
    }

    work->dec_in.stream_length = param->size - 8U;
    work->dec_in.stream = (u8 *)work->dec_in.stream_buffer.virtual_address + 8U;

    ret = PngDecGetImageInfo(inst, &work->dec_in, &work->image_info);

    if (ret == DEC_OK)
    {
        info->width = work->image_info.display_width;
        info->height = work->image_info.display_height;
        info->aligned_width = work->image_info.output_width;
        info->aligned_height = work->image_info.output_height;
        info->bit_depth = work->image_info.bit_depth;
        info->codec = VCDJ_CODEC_PNG;
        info->color_type = vcdj_png_color_type(work->image_info.color_type);
        info->channels = (uint8_t)work->image_info.channels;
        info->has_alpha = ((info->color_type == VCDJ_COLOR_GRAY_ALPHA) ||
                           (info->color_type == VCDJ_COLOR_RGB_ALPHA)) ? 1U : 0U;

        /* JPEG-only attributes stay zero: PNG has no progressive coding mode,
         * no JFIF thumbnail and no chroma sub-sampling. Adam7 interlacing is a
         * different property and gets its own field. */
        info->jpeg_progressive = 0U;
        info->jpeg_has_thumbnail = 0U;
        info->jpeg_chroma = VCDJ_CHROMA_UNKNOWN;

        info->png_interlaced = (work->image_info.interlaced != 0U) ? 1U : 0U;
    }

    err = vcdj_map_dec_ret(ret);
    vcdj_release_input(vcdj_dwl_handle, &work->dec_in.stream_buffer);

out_inst:
    PngDecRelease(inst);
out_work:
    vcdj_mem_free(work);

    return err;
}

static vcdj_error_t vcdj_png_decode(const vcdj_dec_param_t *param,
                                    const vcdj_pp_config_t *pp,
                                    vcdj_picture_t *pic)
{
    struct vcdj_png_work *work;
    PngDecOutput *dec_out = NULL;
    PngDecInst inst = NULL;
    enum DecRet ret;
    vcdj_error_t err;
    void *external_buffers[VCDJ_MAX_EXTERNAL_BUFFERS] = {0};
    unsigned int external_count = 0U;
    unsigned int negotiations = 0U;
    struct vcdj_buffer_layout layout;

    memset(&layout, 0, sizeof(layout));

    work = vcdj_mem_alloc(sizeof(*work));
    if (work == NULL)
    {
        return VCDJ_ERR_NO_MEM;
    }
    memset(work, 0, sizeof(*work));

    work->cfg.decoder_mode = DEC_NORMAL;
    work->cfg.align = DEC_ALIGN_64B;

    ret = PngDecInit(&inst, vcdj_dwl_handle, &work->cfg);
    if (ret != DEC_OK)
    {
        err = vcdj_map_dec_ret(ret);
        goto out_work;
    }

    err = vcdj_prepare_input(vcdj_dwl_handle, param, &work->dec_in.stream_buffer);
    if (err != VCDJ_SUCCESS)
    {
        goto out_inst;
    }

    work->dec_in.stream_length = param->size - 8U;
    work->dec_in.stream = (u8 *)work->dec_in.stream_buffer.virtual_address + 8U;

    ret = PngDecGetImageInfo(inst, &work->dec_in, &work->image_info);
    if (ret != DEC_OK)
    {
        err = vcdj_map_dec_ret(ret);
        goto out_input;
    }

    err = vcdj_validate_pp(pp, work->image_info.display_width,
                           work->image_info.display_height);
    if (err == VCDJ_SUCCESS)
    {
        err = vcdj_format_fits_source(pp->output_format, VCDJ_CODEC_PNG,
                                      work->image_info.bit_depth);
    }
    if (err != VCDJ_SUCCESS)
    {
        goto out_input;
    }

    /*
     * Passed through unchanged, like JPEG: what PngDecSetInfo() and the checker
     * make of a YUV request for a PNG is exactly what has to be verified, so it
     * must not be normalised away here.
     */
    vcdj_setup_ppu(work->cfg.ppu_config, pp);
    ret = PngDecSetInfo(inst, &work->cfg);
    if (ret != DEC_OK)
    {
        err = vcdj_map_dec_ret(ret);
        goto out_input;
    }

    dec_out = vcdj_mem_alloc(sizeof(*dec_out));
    if (dec_out == NULL)
    {
        err = VCDJ_ERR_NO_MEM;
        goto out_input;
    }
    memset(dec_out, 0, sizeof(*dec_out));

    ret = vcdj_decode(inst, (enum DecRet(*)(void *, void *, void *))PngDecDecode,
                      &work->dec_in, dec_out);
    while ((ret == DEC_WAITING_FOR_BUFFER) &&
           (negotiations < VCDJ_MAX_BUFFER_NEGOTIATIONS))
    {
        ret = (enum DecRet)vcdj_internal_add_png_buffers(inst, external_buffers,
                                                         VCDJ_MAX_EXTERNAL_BUFFERS,
                                                         &external_count, &layout);
        if (ret != DEC_OK)
        {
            err = vcdj_map_dec_ret(ret);
            goto out_input;
        }
        negotiations++;
        ret = vcdj_decode(inst, (enum DecRet(*)(void *, void *, void *))PngDecDecode,
                          &work->dec_in, dec_out);
    }
    if ((ret != DEC_PIC_RDY) && (ret != DEC_PIC_DECODED) && (ret != DEC_OK))
    {
        err = vcdj_map_dec_ret(ret);
        goto out_input;
    }

    /*
     * NextPicture() is what populates `pictures[]`: PngDecDecode() zeroes the
     * whole array, so before this call there is no output descriptor and no
     * actual format to report. This is the call the PNG path used to skip - it
     * got away with it only because the instance was destroyed after every
     * decode, which is exactly the masking described in 5.2.3 item 2 of the
     * interface plan.
     */
    ret = PngDecNextPicture(inst, dec_out, &work->image_info);
    if ((ret != DEC_PIC_RDY) && (ret != DEC_OK))
    {
        err = vcdj_map_dec_ret(ret);
        goto out_input;
    }

    pic->width = dec_out->pictures[0].display_width;
    pic->height = dec_out->pictures[0].display_height;

    /*
     * Now a real answer instead of VCDJ_OUT_UNKNOWN. Reading it back matters for
     * PNG: a YUV420SP request comes back as BGR888 / ABGR8888, and greyscale is
     * forced onto the monochrome path whatever was requested.
     */
    pic->format = vcdj_out_format_from_vendor(dec_out->pictures[0].output_format,
                                              pp->output_format);

    if ((pic->width == 0U) || (pic->height == 0U))
    {
        pic->width = work->image_info.display_width;
        pic->height = work->image_info.display_height;
    }

    if (dec_out->pictures[0].pic_stride != 0U)
    {
        layout.ystride = dec_out->pictures[0].pic_stride;
    }
    if (dec_out->pictures[0].pic_stride_ch != 0U)
    {
        layout.cstride = dec_out->pictures[0].pic_stride_ch;
    }

    vcdj_fill_planes(&dec_out->pictures[0].output_picture_y,
                     &dec_out->pictures[0].output_picture_cb_cr,
                     &dec_out->pictures[0].output_picture_cr,
                     &layout, pic);

    /*
     * Same order and same reasoning as the JPEG path - this is the point of N2:
     * both codecs now run Decode -> NextPicture -> PictureConsumed rather than
     * relying on instance teardown to paper over a missing return.
     *
     * The two are not symmetric underneath, and that is the vendor's doing: the
     * JPEG version reads a consumed flag out of `pp_index` while the PNG version
     * passes a fixed external flag. Both end up clearing the same queue slot, so
     * the call sequence here is identical even though the implementations differ.
     */
    (void)PngDecPictureConsumed(inst, dec_out);

    err = vcdj_publish_picture(external_buffers, &external_count, pic);
    if (err != VCDJ_SUCCESS)
    {
        pic->plane_count = 0U;
        memset(pic->planes, 0, sizeof(pic->planes));
    }

out_input:
    vcdj_release_input(vcdj_dwl_handle, &work->dec_in.stream_buffer);
out_inst:
    PngDecRelease(inst);
    vcdj_internal_free_external_buffers(external_buffers, external_count);
out_work:
    vcdj_mem_free(dec_out);
    vcdj_mem_free(work);

    return err;
}

/*============================================================================*
 *                        Public Functions
 *============================================================================*/
void hal_vcdj_mem_init(void *(*vmalloc)(size_t), void (*vfree)(void *))
{
    vcdj_mem_set_allocator(vmalloc, vfree);
}

vcdj_error_t hal_vcdj_init(void)
{
    struct DWLInitParam param;

    if (vcdj_initialised)
    {
        return VCDJ_SUCCESS;
    }

    if (!vcdj_mem_allocator_ready())
    {
        /* hal_vcdj_mem_init() has not been called; refuse instead of quietly
         * pulling in libc malloc. */
        return VCDJ_ERR_NO_MEM;
    }

    memset(&param, 0, sizeof(param));
    param.client_type = DWL_CLIENT_TYPE_JPEG_DEC;

    vcdj_dwl_handle = DWLInit(&param);
    if (vcdj_dwl_handle == NULL)
    {
        return VCDJ_ERR_NO_HW;
    }

    vcdj_initialised = 1U;

    return VCDJ_SUCCESS;
}

void hal_vcdj_deinit(void)
{
    unsigned int i;

    if (!vcdj_initialised)
    {
        return;
    }

    /*
     * Reclaim any picture the caller never released. Leaking it instead would be
     * unrecoverable: the tokens naming these slots die with the caller's
     * structures, so nothing could ever free the memory again.
     *
     * This runs BEFORE DWLRelease(), because DWLFreeRefFrm() goes through the DWL
     * instance that DWLRelease() tears down.
     */
    for (i = 0U; i < VCDJ_MAX_LIVE_PICTURES; i++)
    {
        if (vcdj_live[i].in_use != 0U)
        {
            vcdj_live_free(&vcdj_live[i]);
        }
    }

    (void)DWLRelease(vcdj_dwl_handle);
    vcdj_dwl_handle = NULL;
    vcdj_initialised = 0U;
}

vcdj_error_t hal_vcdj_get_hw_build_id(uint32_t *build_id)
{
    if (build_id == NULL)
    {
        return VCDJ_ERR_PARAM;
    }

    if (!vcdj_initialised)
    {
        return VCDJ_ERR_STATE;
    }

    *build_id = DWLReadCoreHwBuildID(vcdj_dwl_handle, 0U);

    return VCDJ_SUCCESS;
}

vcdj_error_t hal_vcdj_get_image_info(const vcdj_dec_param_t *param,
                                     vcdj_image_info_t *info)
{
    if ((param == NULL) || (info == NULL) || (param->data == NULL) || (param->size == 0U))
    {
        return VCDJ_ERR_PARAM;
    }

    if (!vcdj_codec_supported(param->codec))
    {
        return VCDJ_ERR_UNSUPPORTED;
    }

    if (!vcdj_initialised)
    {
        return VCDJ_ERR_STATE;
    }

    memset(info, 0, sizeof(*info));

    if (param->codec == VCDJ_CODEC_JPEG)
    {
        return vcdj_jpeg_image_info(param, info);
    }

    return vcdj_png_image_info(param, info);
}

vcdj_error_t hal_vcdj_decode_pp(const vcdj_dec_param_t *param,
                                const vcdj_pp_config_t *pp,
                                vcdj_picture_t *pic)
{
    if (pic != NULL)
    {
        memset(pic, 0, sizeof(*pic));
        pic->format = VCDJ_OUT_UNKNOWN;
    }

    if ((param == NULL) || (pp == NULL) || (pic == NULL) ||
        (param->data == NULL) || (param->size == 0U))
    {
        return VCDJ_ERR_PARAM;
    }

    if (!vcdj_codec_supported(param->codec))
    {
        return VCDJ_ERR_UNSUPPORTED;
    }

    if (!vcdj_out_format_supported(pp->output_format) ||
        !vcdj_pp_filter_supported(pp->filter) ||
        (pp->crop_enabled > 1U) || (pp->scale_enabled > 1U))
    {
        return VCDJ_ERR_PARAM;
    }

    if (!vcdj_initialised)
    {
        return VCDJ_ERR_STATE;
    }

    if (param->codec == VCDJ_CODEC_JPEG)
    {
        return vcdj_jpeg_decode(param, pp, pic);
    }

    return vcdj_png_decode(param, pp, pic);
}

vcdj_error_t hal_vcdj_decode(const vcdj_dec_param_t *param, vcdj_picture_t *pic)
{
    vcdj_pp_config_t pp;

    if (param == NULL)
    {
        return VCDJ_ERR_PARAM;
    }

    memset(&pp, 0, sizeof(pp));
    pp.output_format = param->out_format;
    pp.filter = VCDJ_PP_FILTER_DEFAULT;

    return hal_vcdj_decode_pp(param, &pp, pic);
}

vcdj_error_t hal_vcdj_picture_release(vcdj_picture_t *pic)
{
    struct vcdj_live_picture *live;

    if (pic == NULL)
    {
        return VCDJ_SUCCESS;
    }

    /*
     * A zero token is the documented "no pixels" case: a decode that reported
     * geometry only still has to round-trip through release, and calling release
     * twice has to stay safe, so this is a success rather than an error.
     */
    if (pic->release_token == 0U)
    {
        memset(pic, 0, sizeof(*pic));
        return VCDJ_SUCCESS;
    }

    live = vcdj_token_lookup(pic->release_token);
    if (live == NULL)
    {
        /*
         * The token does not name a live picture: either it was already released
         * or it was never handed out by this driver. Refusing is the whole point
         * of the generation counter - freeing on a stale token would release
         * memory that now belongs to a different picture.
         */
        return VCDJ_ERR_PARAM;
    }

    /*
     * Only the pixel memory is released here. The queue slot and the codec
     * instance were already wound down inside hal_vcdj_decode(), so no vendor
     * call is needed - and none would be valid, because the instance that owned
     * that bookkeeping no longer exists.
     */
    vcdj_live_free(live);

    memset(pic, 0, sizeof(*pic));

    return VCDJ_SUCCESS;
}
