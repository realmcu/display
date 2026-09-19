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
 * It is a software-side expectation, not a value read from silicon.
 */
#define VCDJ_EXPECTED_HW_BUILD_ID       0x2070U

/**
 * \brief Upper bound on the planes one decoded picture can be split across.
 *
 * Matches the vendor per-output-channel plane count (luma, chroma, second
 * chroma). Packed RGB output uses exactly one plane; monochrome output uses one;
 * planar YUV uses two or three.
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
 * The two are not always equal. `PngDecSetInfo()` overrides the request based on
 * the PNG colour type: an indexed or truecolour PNG is emitted as BGR888, or as
 * ABGR8888 when it carries transparency, and a greyscale PNG is forced onto the
 * monochrome path regardless of what was asked for. Always read the format back
 * from the picture; never assume the request was honoured.
 */
typedef enum
{
    VCDJ_OUT_RGB565 = 0,           /**< 16 bpp RGB565, little endian */
    VCDJ_OUT_ARGB8888 = 1,         /**< 32 bpp ARGB8888 */
    VCDJ_OUT_YUV420SP = 2,         /**< Linear YUV420 semi-planar output */
    VCDJ_OUT_YUV_PLANAR = VCDJ_OUT_YUV420SP, /**< Compatibility alias for the old name */
    /* --- values below are only ever REPORTED, never requested ------------- */
    VCDJ_OUT_BGR888 = 3,           /**< 24 bpp BGR888; PNG truecolour / indexed */
    VCDJ_OUT_ABGR8888 = 4,         /**< 32 bpp ABGR8888; PNG truecolour + alpha */
    VCDJ_OUT_MONOCHROME = 5,       /**< 8 bpp luma only; PNG greyscale */
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

/** \brief PPU pre-scale crop rectangle, in pixels. */
typedef struct
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} vcdj_rect_t;

/**
 * \brief Single-output PPU request.
 *
 * Cropping runs before scaling, and the scale target is relative to the cropped
 * region. A disabled crop selects the whole visible picture; disabled scaling is
 * 1:1. The driver owns alignment, stride, plane offsets and buffer negotiation.
 */
typedef struct
{
    uint8_t crop_enabled;          /**< 1 uses \ref crop; 0 selects the whole picture */
    uint8_t scale_enabled;         /**< 1 scales to output_width/height; 0 keeps 1:1 */
    vcdj_pp_filter_t filter;       /**< Scaling filter; must be DEFAULT when scale is disabled */
    uint8_t reserved0;             /**< Must be zero */

    vcdj_rect_t crop;              /**< Pre-scale crop; must be all zero when disabled */
    uint32_t output_width;         /**< Scale target width; must be zero when disabled */
    uint32_t output_height;        /**< Scale target height; must be zero when disabled */
    vcdj_out_format_t output_format; /**< Requested format; see picture for actual format */
    uint8_t reserved[3];           /**< Must be zero */
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
    uint8_t  jpeg_has_thumbnail;   /**< 1 when a JFIF thumbnail is present */
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
 * Filled by \ref hal_vcdj_decode, released by \ref hal_vcdj_picture_release.
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
     * Must be handed back unmodified to \ref hal_vcdj_picture_release. It is not
     * a pointer and must not be dereferenced or compared for ordering.
     */
    uintptr_t release_token;
} vcdj_picture_t;

/**
 * \brief Decode request.
 *
 * \note Post processing stays in the separate \ref vcdj_pp_config_t passed to
 *       \ref hal_vcdj_decode_pp, so this structure only describes the input.
 */
typedef struct
{
    const void *data;              /**< Input bitstream, CPU accessible */
    uint32_t    size;              /**< Input bitstream length in bytes */
    vcdj_codec_t      codec;       /**< Codec of \ref data */
    vcdj_out_format_t out_format;  /**< Format for hal_vcdj_decode; decode_pp uses its PPU request */
    uint8_t     reserved[2];       /**< Must be zero */
} vcdj_dec_param_t;

/*============================================================================*
 *                         VCDJ Exported Functions
 *============================================================================*/

/**
 * @brief      Inject the allocator the driver uses for DMA capable buffers.
 *
 * @param[in]  vmalloc  Allocation function with malloc() semantics.
 * @param[in]  vfree    Release function with free() semantics.
 *
 * @note       Must be called before \ref hal_vcdj_init. The caller may pass
 *             `gui_malloc`/`gui_free`, `os_mem_alloc` wrappers, or plain
 *             `malloc`/`free`. The driver never falls back to libc on its own.
 *
 * @warning    Whether the returned memory is reachable by the decoder core, and
 *             whether it must live in SRAM or PSRAM, depends on the hardware
 *             contract that is still open. Do not treat the current
 *             behaviour as final.
 */
void hal_vcdj_mem_init(void *(*vmalloc)(size_t), void (*vfree)(void *));

/**
 * @brief      Bind the static platform operations and bring the core up.
 *
 * @return     \ref VCDJ_SUCCESS on success, otherwise a \ref vcdj_error_t.
 *
 * @note       Enables the clock, releases reset and creates the DWL instance.
 *             Safe to call twice; the second call is a no-op returning
 *             \ref VCDJ_SUCCESS.
 */
vcdj_error_t hal_vcdj_init(void);

/**
 * @brief      Release everything \ref hal_vcdj_init acquired.
 */
void hal_vcdj_deinit(void);

/**
 * @brief      Read the hardware build ID of core 0.
 *
 * @param[out] build_id  Receives the raw build ID register value.
 *
 * @return     \ref VCDJ_SUCCESS on success, otherwise a \ref vcdj_error_t.
 *
 * @note       Expected to read `0x2070` on the target IP revision. Until the
 *             register base is known, this reads a placeholder address and
 *             the value is meaningless.
 */
vcdj_error_t hal_vcdj_get_hw_build_id(uint32_t *build_id);

/**
 * @brief      Parse the bitstream header in software only.
 *
 * @param[in]  param  Decode request; only `data`, `size` and `codec` are used.
 * @param[out] info   Receives geometry, bit depth and colour attributes.
 *
 * @return     \ref VCDJ_SUCCESS on success, otherwise a \ref vcdj_error_t.
 *
 * @note       This path does not touch the hardware, so it is meaningful even
 *             before the register contract is known.
 *
 * @warning    Not a cheap call. The vendor header parser validates the bus
 *             address of its input, so the bitstream is copied into a DMA capable
 *             buffer first even though nothing but the CPU reads it. Querying a
 *             large picture therefore costs one full copy of the input.
 */
vcdj_error_t hal_vcdj_get_image_info(const vcdj_dec_param_t *param,
                                     vcdj_image_info_t *info);

/**
 * @brief      Decode one picture synchronously.
 *
 * @param[in]  param  Decode request.
 * @param[out] pic    Receives the decoded picture. On success it must be handed
 *                    to \ref hal_vcdj_picture_release, even when `plane_count`
 *                    is 0.
 *
 * @return     \ref VCDJ_SUCCESS on success, otherwise a \ref vcdj_error_t.
 *
 * @note       This compatibility entry point performs no crop or scale and uses
 *             `param->out_format`. Use \ref hal_vcdj_decode_pp for PPU control.
 *
 * @note       Internally this is not a single vendor call: when no output buffer
 *             has been supplied the normal path is
 *             `Decode -> DEC_WAITING_FOR_BUFFER -> GetBufferInfo -> AddBuffer ->
 *             Decode`. This function drives that negotiation itself and does not
 *             expose the intermediate states.
 *
 * @note       Only one picture may be held at a time. A second decode while the
 *             previous picture is unreleased returns \ref VCDJ_ERR_STATE rather
 *             than reusing the memory the caller is still reading from.
 *
 * @warning    The register, IRQ and clock contract is not available yet, so on
 *             current hardware this call is expected to fail with
 *             \ref VCDJ_ERR_TIMEOUT after the bounded hardware wait expires.
 *             That is the intended M1 outcome, not a defect.
 */
vcdj_error_t hal_vcdj_decode(const vcdj_dec_param_t *param, vcdj_picture_t *pic);

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
 *             \ref hal_vcdj_decode is equivalent to this function with crop and
 *             scale disabled and `output_format = param->out_format`.
 */
vcdj_error_t hal_vcdj_decode_pp(const vcdj_dec_param_t *param,
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
 * @note       Not thread safe against \ref hal_vcdj_decode: the driver holds one
 *             picture at a time and does not serialise the two calls itself.
 */
vcdj_error_t hal_vcdj_picture_release(vcdj_picture_t *pic);

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
