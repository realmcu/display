/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_VCDJ_H
#define RTL_VCDJ_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include <stdint.h>
#include <stddef.h>

/** \defgroup VCDJ      VCDJ
  * \brief    VeriSilicon VC9000DJ (DecoderJ) JPEG/PNG hardware image decoder driver
  *
  * This is the ONLY header an integrator includes. It carries no vendor Control
  * Software type and no OS type, so it stays on plain `<stdint.h>`.
  * \{
  */

/*============================================================================*
 *                         Interface stability
 *============================================================================*/
/**
 * NOT A FROZEN SDK API YET, BUT THE STRUCTURE LAYOUT IS SETTLED.
 *
 * Two different promises are made here, and they must not be confused:
 *
 *  - THE SHAPE IS SETTLED. Every function signature below and every structure
 *    layout is meant to survive the remaining work. Later additions — including
 *    the separate post-processing entry point below, session reuse and cancel —
 *    arrive as NEW structures or NEW functions, never as changed signatures.
 *    There is deliberately no size or version field: this driver is built from
 *    source together with the rest of the SDK, exactly like the other display IPs,
 *    so a layout change is a recompile rather than a binary compatibility problem.
 *    Should this header ever ship ahead of its implementation, a versioning scheme
 *    has to be added at that point.
 *    Symbol names may change until the SDK API is frozen; callers must rebuild
 *    together with the driver.
 *
 *  - THE BEHAVIOUR IS NOT SETTLED. Several fields are specified here but not yet
 *    produced by the implementation, because the hardware contract (register
 *    window, DMA reachability, cache attributes) is still open and the picture
 *    lifecycle is still being unified between the JPEG and the PNG path. Every
 *    such gap is called out on the field or function that carries it. A field
 *    that is documented as "not filled yet" must be treated as unavailable, not
 *    as zero-valued data.
 *
 * Constraints this header deliberately obeys:
 *  - it does not include any vendor header;
 *  - no `JpegDec*`, `PngDec*`, `DWL*` type appears in any signature;
 *  - it depends on nothing but the C standard integer headers.
 *
 * Caller obligation for every structure below: ZERO IT BEFORE USE, for example
 * `vcdj_picture_t pic = {0};`. Reserved fields must stay zero, and an output
 * structure that is not cleared would leave stale plane pointers behind on an
 * error path.
 */

/*============================================================================*
 *                         VCDJ Exported Constants
 *============================================================================*/

/**
 * \brief Hardware build ID this Control Software drop was released for.
 *
 * Taken from the vendor release name
 * (`Hantro_SW_DecoderJ_CtrlSW_Redistributable_0x2070_v4.1.84_CL1411136.tgz`) and
 * present in the vendor feature table `vpu_features_list.h` as `.id = 0x00002070`.
 * It is the written record of which IP this Control Software drop was released
 * for, and the value the bring-up self test compares the register reading
 * against. It is never substituted for a missing reading.
 */
#define VCDJ_EXPECTED_HW_BUILD_ID       0x2070U

/**
 * \brief Upper bound on the planes one decoded picture can be split across.
 *
 * Matches the vendor per-output-channel plane count (luma, chroma, second
 * chroma). Packed RGB, packed 4:2:2 and luma-only output use one plane,
 * semi-planar YUV uses two, planar YUV and planar RGB use three.
 */
#define VCDJ_MAX_PLANES                 3U

/*============================================================================*
 *                         VCDJ Exported Types
 *============================================================================*/

/** \brief VCDJ driver return codes. Stable, independent from the vendor error space. */
typedef enum
{
    VCDJ_SUCCESS            = 0,   /**< Operation completed successfully */
    VCDJ_ERR_PARAM          = -1,  /**< Invalid argument */
    VCDJ_ERR_STATE          = -2,  /**< Driver is in a state that does not allow the request */
    VCDJ_ERR_NO_MEM         = -3,  /**< Out of memory or no allocator injected */
    VCDJ_ERR_UNSUPPORTED    = -4,  /**< Feature or stream format not supported */
    VCDJ_ERR_STREAM         = -5,  /**< Bitstream is corrupted or truncated */
    VCDJ_ERR_TIMEOUT        = -6,  /**< Hardware did not complete within the timeout */
    VCDJ_ERR_HW             = -7,  /**< Hardware reported an error */
    VCDJ_ERR_NO_HW          = -8,  /**< Hardware not present / register window not mapped */
    VCDJ_ERR_INTERNAL       = -9,  /**< Unexpected internal error */
} vcdj_error_t;

/*
 * A NOTE ON THE ENUMERATIONS BELOW, AND ON EXTENDING THEM.
 *
 * They are used as structure fields on purpose: a reader of `param.codec` can
 * jump straight to `vcdj_codec_t` and see every legal value. A `uint8_t` field
 * with the type named only in a comment would force the reader to chase the
 * meaning of a bare number through the implementation.
 *
 * That is safe here only because this SDK builds every translation unit with
 * `-fshort-enums` (see cmake/tool_chain/gcc.cmake and armclang.cmake), so each
 * enumeration below occupies exactly one byte and the structure layouts stay
 * fixed. Two consequences follow, and both are enforced by build-time assertions
 * in rtl_vcdj_hal.c rather than left to reviewer discipline:
 *
 *   1. NO VALUE ABOVE 0xFF MAY EVER BE ADDED. With `-fshort-enums` the width of
 *      an enumeration follows its value range, so a single value of 0x100 would
 *      silently widen the type to two bytes and move every field after it - a
 *      layout change nobody asked for, hidden inside what looks like the harmless
 *      addition of a constant. `VCDJ_OUT_UNKNOWN` deliberately sits at the 0xFF
 *      ceiling to make the limit visible.
 *   2. An integrator who compiles this header WITHOUT `-fshort-enums` gets four
 *      byte enumerations and therefore a different layout. The assertions turn
 *      that into a compile error in this driver instead of a field mismatch at
 *      run time.
 */

/** \brief Input bitstream codec. */
typedef enum
{
    VCDJ_CODEC_JPEG = 0,           /**< JPEG / JFIF */
    VCDJ_CODEC_PNG  = 1,           /**< PNG */
} vcdj_codec_t;

/**
 * \brief Output pixel format of the post processing unit.
 *
 * Used in two directions with two different meanings:
 *  - in \ref vcdj_dec_param_t and \ref vcdj_pp_config_t it is a REQUEST;
 *  - in \ref vcdj_picture_t it is the format that was ACTUALLY produced.
 *
 * The two are not always equal:
 *  - a colour PNG decoded with a YUV format requested is emitted as BGR888
 *    (indexed / truecolour) or ABGR8888 (RGBA or tRNS); a greyscale PNG is
 *    forced onto the luma-only path whatever was asked for. PNG requests are
 *    passed through unchanged, so a YUV layout the vendor cannot fold into
 *    that rewrite (planar, packed 4:2:2) is rejected rather than replaced;
 *  - a greyscale JPEG is forced onto the luma-only path when a YUV format is
 *    requested;
 *  - a 12-bit JPEG with an 8-bit YUV format requested comes back as the matching
 *    *_PACK10 format (4:2:2 / 4:4:4 are rejected instead).
 * Always read the format back from the picture; never assume the request was
 * honoured.
 *
 * Every value can be requested, but the hardware build decides what succeeds:
 * the vendor checks the request against the feature table and rejects what the
 * core cannot do. Formats with a 10/16-bit container need a 12-bit JPEG and
 * YUV400_ALPHA needs a PNG; both return \ref VCDJ_ERR_UNSUPPORTED otherwise.
 * Component order follows the vendor `DEC_OUT_FRM_*` names. YVU means Cr before
 * Cb (YVU420SP is NV21). P010 / I010 / L010 / PACK10 / 1010 are the vendor 10-bit
 * containers.
 */
typedef enum
{
    /* --- 8-bit packed RGB ------------------------------------------------- */
    /* 0 is what a zero-initialised request asks for; keep RGB565 there. */
    VCDJ_OUT_RGB565 = 0,           /**< 16 bpp RGB565, little endian */
    VCDJ_OUT_BGR565 = 1,
    VCDJ_OUT_RGB888 = 2,
    VCDJ_OUT_BGR888 = 3,           /**< 24 bpp BGR888 */
    VCDJ_OUT_ARGB8888 = 4,         /**< 32 bpp ARGB8888 */
    VCDJ_OUT_ABGR8888 = 5,         /**< 32 bpp ABGR8888 */
    VCDJ_OUT_RGBA8888 = 6,
    VCDJ_OUT_BGRA8888 = 7,
    VCDJ_OUT_XRGB8888 = 8,
    VCDJ_OUT_XBGR8888 = 9,

    /* --- 10/16-bit packed RGB; 12-bit JPEG only ---------------------------- */
    VCDJ_OUT_A2R10G10B10 = 10,
    VCDJ_OUT_A2B10G10R10 = 11,
    VCDJ_OUT_X2R10G10B10 = 12,
    VCDJ_OUT_X2B10G10R10 = 13,
    VCDJ_OUT_R10G10B10A2 = 14,
    VCDJ_OUT_B10G10R10A2 = 15,
    VCDJ_OUT_R16G16B16 = 16,
    VCDJ_OUT_B16G16R16 = 17,

    /* --- 8-bit YUV -------------------------------------------------------- */
    VCDJ_OUT_MONOCHROME = 18,      /**< 8 bpp luma only (YUV 4:0:0) */
    VCDJ_OUT_YUV400_ALPHA = 19,    /**< 16 bpp luma + alpha pairs; PNG only */
    VCDJ_OUT_YUV420SP = 20,        /**< NV12 */
    VCDJ_OUT_YVU420SP = 21,        /**< NV21 */
    VCDJ_OUT_YUV420P = 22,         /**< I420 */
    VCDJ_OUT_YVU420P = 23,         /**< YV12 */
    VCDJ_OUT_YUV422SP = 24,
    VCDJ_OUT_YVU422SP = 25,
    VCDJ_OUT_YUV422P = 26,
    VCDJ_OUT_YVU422P = 27,
    VCDJ_OUT_YUV444P = 28,         /**< 4:4:4 exists only as planar */
    VCDJ_OUT_YVU444P = 29,

    /* --- packed 4:2:2 ----------------------------------------------------- */
    VCDJ_OUT_YUYV = 30,
    VCDJ_OUT_UYVY = 31,
    VCDJ_OUT_YVYU = 32,
    VCDJ_OUT_VYUY = 33,

    /* --- 10-bit YUV containers; 12-bit JPEG only --------------------------- */
    VCDJ_OUT_YUV400_PACK10 = 34,
    VCDJ_OUT_YUV400_P010 = 35,
    VCDJ_OUT_YUV400_I010 = 36,
    VCDJ_OUT_YUV400_L010 = 37,
    VCDJ_OUT_YUV420SP_PACK10 = 38,
    VCDJ_OUT_YUV420SP_P010 = 39,
    VCDJ_OUT_YUV420SP_I010 = 40,
    VCDJ_OUT_YUV420SP_L010 = 41,
    VCDJ_OUT_YUV420SP_1010 = 42,
    VCDJ_OUT_YVU420SP_PACK10 = 43,
    VCDJ_OUT_YVU420SP_P010 = 44,
    VCDJ_OUT_YVU420SP_I010 = 45,
    VCDJ_OUT_YVU420SP_L010 = 46,
    VCDJ_OUT_YVU420SP_1010 = 47,
    VCDJ_OUT_YUV420P_PACK10 = 48,
    VCDJ_OUT_YUV420P_P010 = 49,
    VCDJ_OUT_YUV420P_I010 = 50,
    VCDJ_OUT_YUV420P_L010 = 51,
    VCDJ_OUT_YVU420P_PACK10 = 52,
    VCDJ_OUT_YVU420P_P010 = 53,
    VCDJ_OUT_YVU420P_I010 = 54,
    VCDJ_OUT_YVU420P_L010 = 55,
    VCDJ_OUT_YUV422SP_P010 = 56,
    VCDJ_OUT_YUV422SP_I010 = 57,
    VCDJ_OUT_YUV422SP_L010 = 58,
    VCDJ_OUT_YVU422SP_P010 = 59,
    VCDJ_OUT_YVU422SP_I010 = 60,
    VCDJ_OUT_YVU422SP_L010 = 61,
    VCDJ_OUT_YUV422P_P010 = 62,
    VCDJ_OUT_YUV422P_I010 = 63,
    VCDJ_OUT_YUV422P_L010 = 64,
    VCDJ_OUT_YVU422P_P010 = 65,
    VCDJ_OUT_YVU422P_I010 = 66,
    VCDJ_OUT_YVU422P_L010 = 67,
    VCDJ_OUT_YUV444P_P010 = 68,
    VCDJ_OUT_YUV444P_I010 = 69,
    VCDJ_OUT_YUV444P_L010 = 70,
    VCDJ_OUT_YVU444P_P010 = 71,
    VCDJ_OUT_YVU444P_I010 = 72,
    VCDJ_OUT_YVU444P_L010 = 73,

    VCDJ_OUT_UNKNOWN = 0xFF,       /**< Format could not be mapped; treat as an error */
} vcdj_out_format_t;

/** \brief PPU scaling filter quality, independent from the vendor values. */
typedef enum
{
    VCDJ_PP_FILTER_DEFAULT = 0,    /**< Default to bilinear when scaling is enabled */
    VCDJ_PP_FILTER_NEAREST,        /**< Nearest-neighbour, favouring speed */
    VCDJ_PP_FILTER_BILINEAR,       /**< Bilinear */
    VCDJ_PP_FILTER_BICUBIC,        /**< Bicubic, favouring quality */
    VCDJ_PP_FILTER_AREA,           /**< Area; both dimensions must downscale */
} vcdj_pp_filter_t;

/**
 * \brief Which picture of the stream a call addresses.
 *
 * A JPEG may embed a JPEG-coded thumbnail (JFXX or EXIF) beside the main picture;
 * each decode call produces one of them, so a caller that wants both calls twice.
 * \ref vcdj_image_info_t.jpeg_has_thumbnail says whether the second one exists.
 */
typedef enum
{
    VCDJ_PICTURE_MAIN = 0,         /**< Main picture; what a zero-initialised request asks for */
    VCDJ_PICTURE_THUMBNAIL = 1,    /**< Embedded JPEG thumbnail; JPEG only */
} vcdj_picture_sel_t;

/** \brief PPU pre-scale crop rectangle, in pixels. */
typedef struct
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} vcdj_rect_t;

/**
 * \brief YUV to RGB conversion for an RGB output; a YUV output ignores it.
 *
 * The stream carries no reliable indication, so the caller states matrix and range together.
 */
typedef enum
{
    VCDJ_CSC_BT601 = 0,            /**< BT.601, full range both sides */
    VCDJ_CSC_BT601_L,              /**< BT.601, limited range both sides */
    VCDJ_CSC_BT709,                /**< BT.709, full range both sides */
    VCDJ_CSC_BT709_L,              /**< BT.709, limited range both sides */
    VCDJ_CSC_BT2020,               /**< BT.2020, full range both sides */
    VCDJ_CSC_BT2020_L,             /**< BT.2020, limited range both sides */
} vcdj_csc_t;

/**
 * \brief Single-output PPU request.
 *
 * Cropping runs before scaling, and the scale target is relative to the cropped
 * region. A disabled crop selects the whole visible picture; disabled scaling is
 * 1:1. The driver owns alignment, stride, plane offsets and buffer negotiation.
 */
typedef struct
{
    uint8_t crop_enabled;            /**< 1 uses \ref crop; 0 selects the whole picture */
    uint8_t scale_enabled;           /**< 1 scales to output_width/height; 0 keeps 1:1 */
    vcdj_pp_filter_t filter;         /**< Scaling filter; must be DEFAULT when scale is disabled */
    vcdj_csc_t csc;                  /**< RGB output only; see \ref vcdj_csc_t */

    vcdj_rect_t crop;                /**< Pre-scale crop; must be all zero when disabled */
    uint32_t output_width;           /**< Scale target width; must be zero when disabled */
    uint32_t output_height;          /**< Scale target height; must be zero when disabled */
    vcdj_out_format_t output_format; /**< Requested format; see picture for actual format */
    uint8_t alpha;                   /**< Constant alpha: 0 transparent, 255 opaque */
    uint8_t reserved[2];             /**< Must be zero */
} vcdj_pp_config_t;

/** \brief Colour model of the coded picture, independent of the output format. */
typedef enum
{
    VCDJ_COLOR_UNKNOWN = 0,
    VCDJ_COLOR_GRAY,               /**< Greyscale, no alpha */
    VCDJ_COLOR_GRAY_ALPHA,         /**< Greyscale plus alpha */
    VCDJ_COLOR_YCBCR,              /**< JPEG luma/chroma */
    VCDJ_COLOR_RGB,                /**< PNG truecolour */
    VCDJ_COLOR_RGB_ALPHA,          /**< PNG truecolour plus alpha */
    VCDJ_COLOR_INDEXED,            /**< PNG palette */
} vcdj_color_type_t;

/** \brief Chroma sub-sampling reported by the software header parser. */
typedef enum
{
    VCDJ_CHROMA_UNKNOWN = 0,
    VCDJ_CHROMA_MONO,              /**< 4:0:0 */
    VCDJ_CHROMA_411,
    VCDJ_CHROMA_420,
    VCDJ_CHROMA_422,
    VCDJ_CHROMA_440,
    VCDJ_CHROMA_444,
} vcdj_chroma_t;

/**
 * \brief Result of the software only header parse. Carries no vendor type.
 *
 * Codec-neutral geometry comes first; the codec-specific attributes are grouped
 * per codec and are only meaningful when `codec` names that codec. Sharing one
 * field between two codecs with two different meanings is exactly the mistake
 * `progressive` used to make, so \ref jpeg_progressive and \ref png_interlaced
 * are kept apart.
 */
typedef struct
{
    /* --- codec neutral ---------------------------------------------------- */
    uint32_t width;                /**< Visible picture width in pixels */
    uint32_t height;               /**< Visible picture height in pixels */
    uint32_t aligned_width;        /**< Width padded to the hardware alignment */
    uint32_t aligned_height;       /**< Height padded to the hardware alignment */
    uint32_t bit_depth;            /**< 8 or 12 for JPEG, 8 or 16 for PNG */
    vcdj_codec_t      codec;       /**< Codec of the parsed stream */
    vcdj_color_type_t color_type;  /**< Colour model of the coded picture */
    uint8_t  channels;             /**< Component count: 1, 2, 3 or 4 */
    uint8_t  has_alpha;            /**< 1 when an alpha channel is present */

    /* --- JPEG only; zero for any other codec ------------------------------ */
    uint8_t  jpeg_progressive;     /**< 1 when the JPEG uses progressive coding */
    uint8_t  jpeg_has_thumbnail;   /**< 1 when a JPEG-coded thumbnail (JFXX or EXIF) is present */
    vcdj_chroma_t jpeg_chroma;     /**< Chroma sub-sampling */

    /* --- PNG only; zero for any other codec ------------------------------- */
    uint8_t  png_interlaced;       /**< 1 when the PNG uses Adam7 interlacing */
} vcdj_image_info_t;

/**
 * \brief One plane of a decoded picture.
 *
 * \warning `stride` is the real hardware line pitch and is NOT `width * bpp`.
 *          The post processing unit aligns every output line, so a consumer that
 *          assumes a packed layout will tear the image. Walk line `n` at
 *          `(uint8_t *)cpu_addr + offset + n * stride`.
 */
typedef struct
{
    void     *cpu_addr;            /**< CPU accessible base of the plane */
    uintptr_t bus_addr;            /**< Address the decoder core was programmed with */
    uint32_t  size;                /**< Usable bytes in this plane */
    uint32_t  stride;              /**< Bytes per pixel line, padding included */
    uint32_t  offset;              /**< Byte offset of the first visible pixel */
} vcdj_plane_t;

/**
 * \brief A decoded picture and the ownership token needed to give it back.
 *
 * Filled by \ref vcdj_decode, released by \ref vcdj_picture_release.
 * Every field describes what was ACTUALLY produced, never what was requested.
 *
 * \note Every plane size and stride below comes from the decoder's own buffer
 *       negotiation, never from `width * height * bpp`: the post processing unit
 *       aligns each line, so the sizes cannot be reproduced from the geometry.
 *
 * \warning The pixel VALUES are not verified yet. DMA reachability and the cache
 *          attributes of the output memory are still open, so a decode on real
 *          hardware may produce stale or incomplete data even though the
 *          descriptor below is filled in correctly. Do not treat a delivered
 *          picture as evidence that the hardware path works.
 */
typedef struct
{
    uint32_t width;                /**< Visible output width in pixels */
    uint32_t height;               /**< Visible output height in pixels */
    vcdj_out_format_t format;      /**< Format actually produced, not the request */
    uint8_t  plane_count;          /**< Planes filled in \ref planes; 0 = no pixels */
    uint8_t  reserved[2];          /**< Must be zero */

    vcdj_plane_t planes[VCDJ_MAX_PLANES];

    /**
     * \brief Opaque token identifying the picture inside the driver.
     *
     * Must be handed back unmodified to \ref vcdj_picture_release. It is not
     * a pointer and must not be dereferenced or compared for ordering.
     */
    uintptr_t release_token;
} vcdj_picture_t;

/**
 * \brief Decode request.
 *
 * \note Post processing stays in the separate \ref vcdj_pp_config_t passed to
 *       \ref vcdj_decode_pp, so this structure only describes the input.
 *
 * \note `data` must hold exactly one JPEG frame. A buffer of concatenated frames
 *       (MJPEG) is not a supported input: locate the frames with
 *       \ref vcdj_jpeg_scan_frames and pass them one at a time.
 */
typedef struct
{
    const void *data;              /**< Input bitstream, CPU accessible */
    uint32_t    size;              /**< Input bitstream length in bytes */
    vcdj_codec_t      codec;       /**< Codec of \ref data */
    vcdj_out_format_t out_format;  /**< Format for vcdj_decode; decode_pp uses its PPU request */
    vcdj_picture_sel_t picture;    /**< Main picture or embedded thumbnail; see \ref vcdj_picture_sel_t */
    uint8_t     reserved[1];       /**< Must be zero */
} vcdj_dec_param_t;

/**
 * \brief Location of one self-contained JPEG frame inside a scanned buffer.
 *
 * An offset rather than a pointer, so an index stays valid when the buffer is
 * loaded at another address and can be generated offline.
 */
typedef struct
{
    uint32_t offset;               /**< Byte offset of the frame's SOI from the buffer start */
    uint32_t size;                 /**< Bytes from SOI through EOI inclusive */
} vcdj_jpeg_frame_t;

/*============================================================================*
 *                         VCDJ Exported Functions
 *============================================================================*/

/**
 * @brief      Inject the allocator the driver uses for DMA capable buffers.
 *
 * @param[in]  vmalloc  Allocation function with malloc() semantics.
 * @param[in]  vfree    Release function with free() semantics.
 *
 * @note       Must be called before \ref vcdj_init. The caller may pass
 *             `gui_malloc`/`gui_free`, `os_mem_alloc` wrappers, or plain
 *             `malloc`/`free`. The driver never falls back to libc on its own.
 *
 * @warning    Whether the returned memory is reachable by the decoder core, and
 *             whether it must live in SRAM or PSRAM, depends on the hardware
 *             contract that is still open. Do not treat the current
 *             behaviour as final.
 */
void vcdj_mem_init(void *(*vmalloc)(size_t), void (*vfree)(void *));

/**
 * @brief      Bind the static platform operations and bring the core up.
 *
 * @return     \ref VCDJ_SUCCESS on success, otherwise a \ref vcdj_error_t.
 *
 * @note       Enables the clock, releases reset and creates the DWL instance.
 *             Safe to call twice; the second call is a no-op returning
 *             \ref VCDJ_SUCCESS.
 */
vcdj_error_t vcdj_init(void);

/**
 * @brief      Release everything \ref vcdj_init acquired.
 */
void vcdj_deinit(void);

/**
 * @brief      Parse the bitstream header in software only.
 *
 * @param[in]  param  Decode request; only `data`, `size`, `codec` and `picture`
 *                    are used.
 * @param[out] info   Receives geometry, bit depth and colour attributes of the
 *                    picture `param->picture` selects.
 *
 * @return     \ref VCDJ_SUCCESS on success, otherwise a \ref vcdj_error_t. A
 *             `VCDJ_PICTURE_THUMBNAIL` request returns \ref VCDJ_ERR_PARAM when the
 *             stream has no thumbnail or is not JPEG, and \ref VCDJ_ERR_UNSUPPORTED
 *             when the thumbnail is not JPEG coded.
 *
 * @note       This path does not touch the hardware, so it is meaningful even
 *             before the register contract is known.
 *
 * @note       For a thumbnail every field describes the thumbnail, except
 *             `jpeg_has_thumbnail`, which is 1.
 *
 * @warning    Not a cheap call. The vendor header parser validates the bus
 *             address of its input, so the bitstream is copied into a DMA capable
 *             buffer first even though nothing but the CPU reads it. Querying a
 *             large picture therefore costs one full copy of the input.
 */
vcdj_error_t vcdj_get_image_info(const vcdj_dec_param_t *param,
                                 vcdj_image_info_t *info);

/**
 * @brief      Locate the JPEG frames in a buffer of concatenated frames (MJPEG).
 *
 * @param[in]  data         Buffer to scan, CPU accessible; need not be DMA capable.
 * @param[in]  size         Length of \p data in bytes.
 * @param[out] frames       Receives up to \p max_frames locations in stream order;
 *                          may be NULL when \p max_frames is 0.
 * @param[in]  max_frames   Capacity of \p frames.
 * @param[out] frame_count  Receives the number of complete frames found, which may
 *                          exceed \p max_frames. May be NULL; the scan then stops
 *                          as soon as \p max_frames entries are filled.
 *
 * @return     \ref VCDJ_SUCCESS when at least one frame was found and no broken
 *             frame was met; with \p frame_count NULL, only when all \p max_frames
 *             entries were filled.
 * @return     \ref VCDJ_ERR_STREAM when no complete frame exists, a frame is
 *             truncated or malformed, or (with \p frame_count NULL) fewer than
 *             \p max_frames frames exist. Frames before the fault are still reported.
 * @return     \ref VCDJ_ERR_PARAM for a NULL or empty buffer, or when the call
 *             could return nothing.
 *
 * @note       Software only: needs neither \ref vcdj_init nor the hardware,
 *             and copies nothing. Each frame is then decoded by passing
 *             `data + offset` and `size` to \ref vcdj_decode_pp.
 *
 * @note       Bytes outside any frame (padding, garbage) are skipped, so offsets
 *             need not be contiguous. The scan stops at the first broken frame
 *             and does not resynchronise past it.
 *
 * @note       Checks structure only. A frame reported here can still fail to
 *             decode, for example a progressive JPEG.
 *
 * @warning    Cost grows with \p size: entropy-coded data has no length field and
 *             is read byte by byte. Scan once and keep the index.
 */
vcdj_error_t vcdj_jpeg_scan_frames(const void *data, uint32_t size,
                                   vcdj_jpeg_frame_t *frames,
                                   uint32_t max_frames,
                                   uint32_t *frame_count);

/**
 * @brief      Decode one picture synchronously.
 *
 * @param[in]  param  Decode request.
 * @param[out] pic    Receives the decoded picture. On success it must be handed
 *                    to \ref vcdj_picture_release, even when `plane_count`
 *                    is 0.
 *
 * @return     \ref VCDJ_SUCCESS on success, otherwise a \ref vcdj_error_t.
 *
 * @note       This compatibility entry point performs no crop or scale and uses
 *             `param->out_format`. Use \ref vcdj_decode_pp for PPU control.
 *
 * @note       Internally this is not a single vendor call: when no output buffer
 *             has been supplied the normal path is
 *             `Decode -> DEC_WAITING_FOR_BUFFER -> GetBufferInfo -> AddBuffer ->
 *             Decode`. This function drives that negotiation itself and does not
 *             expose the intermediate states.
 *
 * @note       Any number of pictures may be held at once; each keeps its pixel
 *             memory until released, so the limit is the injected allocator.
 *             Decode calls themselves are NOT reentrant: the core is single
 *             instance, so callers serialise decodes against each other.
 *
 * @warning    The register, IRQ and clock contract is not available yet, so on
 *             current hardware this call is expected to fail with
 *             \ref VCDJ_ERR_TIMEOUT after the bounded hardware wait expires.
 *             That is the intended M1 outcome, not a defect.
 */
vcdj_error_t vcdj_decode(const vcdj_dec_param_t *param, vcdj_picture_t *pic);

/**
 * @brief      Decode one picture synchronously with crop/scale/format configuration.
 *
 * @param[in]  param  Input bitstream request.
 * @param[in]  pp     PPU request. Must not be NULL and must be zero-initialized
 *                    before selected fields are assigned. Its output_format
 *                    overrides param->out_format for this call.
 * @param[out] pic    Actual output descriptor and ownership token.
 *
 * @return     \ref VCDJ_SUCCESS on success; \ref VCDJ_ERR_PARAM for malformed
 *             configuration; \ref VCDJ_ERR_UNSUPPORTED for a valid request the
 *             selected hardware mode cannot implement; otherwise another
 *             \ref vcdj_error_t.
 *
 * @note       Configuration is validated and translated before
 *             `JpegDecSetInfo()` / `PngDecSetInfo()`. The legacy
 *             \ref vcdj_decode is equivalent to this function with crop and
 *             scale disabled and `output_format = param->out_format`.
 *
 * @note       With `param->picture = VCDJ_PICTURE_THUMBNAIL`, crop and scale are
 *             relative to the thumbnail, and the error codes for a missing or
 *             non-JPEG thumbnail are those of \ref vcdj_get_image_info. The
 *             vendor cannot up-sample thumbnail chroma: a 4:2:0 thumbnail with a
 *             4:2:2 / 4:4:4 output returns \ref VCDJ_ERR_PARAM.
 */
vcdj_error_t vcdj_decode_pp(const vcdj_dec_param_t *param,
                            const vcdj_pp_config_t *pp,
                            vcdj_picture_t *pic);

/**
 * @brief      Give a picture obtained from a VCDJ decode call back to the driver.
 *
 * @param[in]  pic  Picture to release. NULL is accepted and ignored.
 *
 * @return     \ref VCDJ_SUCCESS on success, otherwise a \ref vcdj_error_t.
 *
 * @return     \ref VCDJ_SUCCESS on success. \ref VCDJ_ERR_PARAM when
 *             `release_token` does not name a live picture - which is what a
 *             second release of the same picture looks like, so the caller's
 *             own copy of the structure must not be reused after releasing it.
 *
 * @note       Releases the pixel memory and zeroes the caller's structure. A
 *             picture whose `plane_count` is 0 carries no memory and releasing it
 *             is a successful no-op.
 *
 * @note       May run on a different thread than, and concurrently with, a
 *             decode call. Not valid after \ref vcdj_deinit, which has
 *             already reclaimed every picture; it returns \ref VCDJ_ERR_PARAM.
 */
vcdj_error_t vcdj_picture_release(vcdj_picture_t *pic);

/*
 * The bring-up self test is NOT declared here. It is test code, not a driver
 * requirement, so it lives in `src/test/vcdj_selftest.h` and this header stays
 * limited to the decode interface a product feature uses.
 */

/** End of VCDJ
  * \}
  */

#ifdef __cplusplus
}
#endif

#endif /* RTL_VCDJ_H */
