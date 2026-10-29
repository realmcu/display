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
  * \brief    JPEG/PNG hardware image decoder driver
  *
  * The only header an integrator includes. It depends on nothing but the C
  * standard headers.
  * \{
  */

/*
 * Every structure below must be zeroed before use, e.g. `vcdj_picture_t pic = {0};`.
 * Reserved fields must stay zero. The driver is built from source with the SDK,
 * so structures carry no size or version field.
 */

/*============================================================================*
 *                         VCDJ Exported Constants
 *============================================================================*/

/**
 * \brief Length of \ref vcdj_picture_t.planes: the most planes one picture uses.
 *
 * Matches the vendor per-output plane count, which a build-time check in the
 * driver enforces. Packed and luma-only output use one plane, semi-planar YUV
 * two, planar YUV three (not available on every hardware build).
 */
#define VCDJ_MAX_PLANES                 3U

/*============================================================================*
 *                         VCDJ Exported Types
 *============================================================================*/

/** \brief VCDJ driver return codes. */
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
 * The enumerations below are structure fields and must stay one byte wide, so
 * the build needs short enums (GCC's arm-none-eabi default; pass -fshort-enums
 * where the toolchain does not). Never add a value above 0xFF: it would widen
 * the type and shift every later field. Build-time checks enforce both.
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
 * A REQUEST in \ref vcdj_dec_param_t / \ref vcdj_pp_config_t, the format ACTUALLY
 * produced in \ref vcdj_picture_t. Always read it back from the picture; the two
 * differ in these cases:
 *  - colour PNG with a YUV request: BGR888, or ABGR8888 when the PNG has alpha
 *    or a tRNS chunk; a YUV layout that cannot be rewritten this way (planar,
 *    packed 4:2:2) is rejected;
 *  - greyscale PNG, or greyscale JPEG with a YUV request: luma only;
 *  - 12-bit JPEG with an 8-bit YUV request: the matching *_PACK10 format
 *    (4:2:2 / 4:4:4 are rejected).
 *
 * RGB names give the channels from the most to the least significant bits of
 * one pixel word, stored little endian, so memory order is reversed: RGB888 is
 * B,G,R in memory, BGR888 is R,G,B, ARGB8888 is B,G,R,A, RGBA8888 is A,B,G,R.
 *
 * The hardware build decides what succeeds. 10/16-bit containers need a 12-bit
 * JPEG and YUV400_ALPHA needs a PNG; otherwise \ref VCDJ_ERR_UNSUPPORTED. On the
 * current build every planar, P010/I010/L010 and 1010 format is rejected, and
 * 4:2:2 output (semi-planar or packed) needs a 4:2:2 source.
 * YVU means Cr before Cb.
 */
typedef enum
{
    /* --- 8-bit packed RGB ------------------------------------------------- */
    /* 0 is what a zero-initialised request asks for; keep RGB565 there. */
    VCDJ_OUT_RGB565 = 0,           /**< 16 bpp; R in bits 15:11 */
    VCDJ_OUT_BGR565 = 1,
    VCDJ_OUT_RGB888 = 2,           /**< 24 bpp; memory B,G,R */
    VCDJ_OUT_BGR888 = 3,           /**< 24 bpp; memory R,G,B */
    VCDJ_OUT_ARGB8888 = 4,         /**< 32 bpp; memory B,G,R,A */
    VCDJ_OUT_ABGR8888 = 5,         /**< 32 bpp; memory R,G,B,A */
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

/** \brief PPU scaling filter quality. */
typedef enum
{
    VCDJ_PP_FILTER_DEFAULT = 0,    /**< Same as BILINEAR when scaling is enabled */
    VCDJ_PP_FILTER_NEAREST,        /**< Nearest-neighbour, favouring speed */
    VCDJ_PP_FILTER_BILINEAR,       /**< Fast linear interpolation */
    VCDJ_PP_FILTER_BICUBIC,        /**< Fast bicubic interpolation, favouring quality */
    VCDJ_PP_FILTER_AREA,           /**< Area; no upscale, at least one dimension downscales */
} vcdj_pp_filter_t;

/**
 * \brief Which picture of the stream a call addresses.
 *
 * A JPEG may embed a JPEG-coded thumbnail (JFXX or EXIF); each decode call
 * produces one picture, so decoding both takes two calls.
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
    VCDJ_CSC_BT601 = 0,            /**< BT.601, full-range YCbCr in, full-range RGB out */
    VCDJ_CSC_BT601_L,              /**< BT.601, limited-range YCbCr in, full-range RGB out */
    VCDJ_CSC_BT709,                /**< BT.709, full-range YCbCr in, full-range RGB out */
    VCDJ_CSC_BT709_L,              /**< BT.709, limited-range YCbCr in, full-range RGB out */
    VCDJ_CSC_BT2020,               /**< BT.2020, full-range YCbCr in, full-range RGB out */
    VCDJ_CSC_BT2020_L,             /**< BT.2020, limited-range YCbCr in, full-range RGB out */
} vcdj_csc_t;

/**
 * \brief Single-output PPU request.
 *
 * Cropping runs before scaling, and the scale target is relative to the cropped
 * region. A disabled crop selects the whole visible picture; disabled scaling is
 * 1:1. The driver owns alignment, stride, plane offsets and buffer negotiation.
 *
 * Geometry limits on the current hardware build, each returning
 * \ref VCDJ_ERR_PARAM when violated:
 *  - every output dimension is at least 48, scaled or not;
 *  - a scaled dimension is at most min(4096, 3 x the cropped or visible size)
 *    and at least 1/32 of it;
 *  - a crop dimension is at least 48 unless it equals the picture's; the crop
 *    lies inside the picture, and starts on an even x and y for sub-sampled
 *    sources or any YUV 4:2:0 / 4:2:2 output;
 *  - packed 4:2:2 output has an even width.
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
 * \brief Result of the software only header parse.
 *
 * The `jpeg_*` / `png_*` fields are only meaningful when `codec` names that codec,
 * and are zero otherwise.
 *
 * \note For a JPEG that carries a thumbnail, `bit_depth` is the thumbnail's even
 *       when the main picture is requested.
 */
typedef struct
{
    /* --- codec neutral ---------------------------------------------------- */
    uint32_t width;                /**< Visible picture width in pixels */
    uint32_t height;               /**< Visible picture height in pixels */
    uint32_t aligned_width;        /**< JPEG: width rounded up to 64; PNG: width. Not a stride */
    uint32_t aligned_height;       /**< JPEG: height rounded up to 8; PNG: height */
    uint32_t bit_depth;            /**< JPEG: 8 or 12. PNG: 8, or 1/2/4 index bits for a
                                        palette picture (samples still output as 8-bit) */
    vcdj_codec_t      codec;       /**< Codec of the parsed stream */
    vcdj_color_type_t color_type;  /**< Colour model of the coded picture */
    uint8_t  channels;             /**< Coded components, 1 to 4; a palette PNG counts 3,
                                        or 4 with tRNS */
    uint8_t  has_alpha;            /**< 1 for a GRAY_ALPHA or RGB_ALPHA picture only; tRNS
                                        transparency is not reported here */

    /* --- JPEG only -------------------------------------------------------- */
    uint8_t  jpeg_progressive;     /**< 1 when the JPEG uses progressive coding */
    uint8_t  jpeg_has_thumbnail;   /**< 1 when a JPEG-coded thumbnail (JFXX or EXIF) is present */
    vcdj_chroma_t jpeg_chroma;     /**< Chroma sub-sampling */

    /* --- PNG only --------------------------------------------------------- */
    uint8_t  png_interlaced;       /**< 1 when the PNG uses Adam7 interlacing */
} vcdj_image_info_t;

/**
 * \brief One plane of a decoded picture.
 *
 * \warning `stride` is the hardware line pitch, NOT `width * bpp`, and `size`
 *          cannot be derived from the geometry either. Walk line `n` at
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
 * Filled by \ref vcdj_decode / \ref vcdj_decode_pp, released by
 * \ref vcdj_picture_release. Every field describes what was actually produced.
 */
typedef struct
{
    uint32_t width;                /**< Visible output width in pixels */
    uint32_t height;               /**< Visible output height in pixels */
    vcdj_out_format_t format;      /**< Format actually produced, not the request */
    uint8_t  plane_count;          /**< Planes filled in \ref planes; 0 = no pixels */
    uint8_t  reserved[2];          /**< Must be zero */

    vcdj_plane_t planes[VCDJ_MAX_PLANES];

    /** \brief Opaque; hand back unmodified. Not a pointer, never dereference it. */
    uintptr_t release_token;
} vcdj_picture_t;

/**
 * \brief Decode request; describes the input only.
 *
 * \note `data` must hold exactly one JPEG frame. Split MJPEG with
 *       \ref vcdj_jpeg_scan_frames first.
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
 * @note       Must be called before \ref vcdj_init. The driver never falls back
 *             to libc on its own.
 *
 * @warning    The returned memory must be reachable by the decoder's bus master.
 */
void vcdj_mem_init(void *(*vmalloc)(size_t), void (*vfree)(void *));

/**
 * @brief      Bring the decoder core up.
 *
 * @return     \ref VCDJ_SUCCESS on success, otherwise a \ref vcdj_error_t.
 *
 * @note       Calling it again is a no-op returning \ref VCDJ_SUCCESS.
 */
vcdj_error_t vcdj_init(void);

/**
 * @brief      Release everything \ref vcdj_init acquired, including every
 *             picture not yet released.
 */
void vcdj_deinit(void);

/**
 * @brief      Parse the bitstream header in software only.
 *
 * @param[in]  param  Decode request; only `data`, `size`, `codec` and `picture`
 *                    are used.
 * @param[out] info   Receives the attributes of the picture `param->picture`
 *                    selects; for a thumbnail `jpeg_has_thumbnail` is still 1.
 *
 * @return     \ref VCDJ_SUCCESS on success, otherwise a \ref vcdj_error_t. A
 *             thumbnail request returns \ref VCDJ_ERR_PARAM when the stream is
 *             not JPEG or has none, \ref VCDJ_ERR_UNSUPPORTED when it is not JPEG coded.
 *
 * @note       Requires \ref vcdj_init. Reads `data` in place, so it need only be
 *             CPU accessible; the input is neither copied nor modified.
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
 * @note       Software only; needs no \ref vcdj_init and copies nothing. Bytes
 *             between frames are skipped; the scan stops at the first broken frame.
 *             Only structure is checked, so a listed frame may still fail to decode.
 *
 * @warning    Reads entropy-coded data byte by byte, so cost grows with \p size.
 *             Scan once and keep the index.
 */
vcdj_error_t vcdj_jpeg_scan_frames(const void *data, uint32_t size,
                                   vcdj_jpeg_frame_t *frames,
                                   uint32_t max_frames,
                                   uint32_t *frame_count);

/**
 * @brief      Decode one picture synchronously, without crop or scale.
 *
 * Equivalent to \ref vcdj_decode_pp with crop and scale disabled and
 * `output_format = param->out_format`.
 *
 * @param[in]  param  Decode request.
 * @param[out] pic    Receives the decoded picture. On success it must be handed
 *                    to \ref vcdj_picture_release, even when `plane_count` is 0.
 *
 * @return     \ref VCDJ_SUCCESS on success, otherwise a \ref vcdj_error_t.
 *
 * @note       Decode calls are not reentrant; callers serialise them. Any number
 *             of pictures may be held at once, limited only by the allocator.
 */
vcdj_error_t vcdj_decode(const vcdj_dec_param_t *param, vcdj_picture_t *pic);

/**
 * @brief      Decode one picture synchronously with crop/scale/format configuration.
 *
 * @param[in]  param  Input bitstream request.
 * @param[in]  pp     PPU request; must not be NULL. Its `output_format`
 *                    overrides `param->out_format`.
 * @param[out] pic    Actual output descriptor and ownership token.
 *
 * @return     \ref VCDJ_SUCCESS on success; \ref VCDJ_ERR_PARAM for malformed
 *             configuration; \ref VCDJ_ERR_UNSUPPORTED for a valid request the
 *             hardware cannot implement; otherwise another \ref vcdj_error_t.
 *
 * @note       For a thumbnail, crop and scale are relative to the thumbnail and
 *             the errors are those of \ref vcdj_get_image_info. A 4:2:0 thumbnail
 *             with a 4:2:2 / 4:4:4 output returns \ref VCDJ_ERR_PARAM.
 */
vcdj_error_t vcdj_decode_pp(const vcdj_dec_param_t *param,
                            const vcdj_pp_config_t *pp,
                            vcdj_picture_t *pic);

/**
 * @brief      Give a picture obtained from a VCDJ decode call back to the driver.
 *
 * @param[in]  pic  Picture to release. NULL is accepted and ignored.
 *
 * @return     \ref VCDJ_SUCCESS on success. \ref VCDJ_ERR_PARAM when
 *             `release_token` names no live picture, e.g. a second release from
 *             a stale copy, or any release after \ref vcdj_deinit.
 *
 * @note       Frees the pixel memory and zeroes \p pic. May run concurrently with
 *             a decode call on another thread.
 */
vcdj_error_t vcdj_picture_release(vcdj_picture_t *pic);

/** End of VCDJ
  * \}
  */

#ifdef __cplusplus
}
#endif

#endif /* RTL_VCDJ_H */
