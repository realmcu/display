/*
  * Copyright 2014-2025 Verisilicon Inc.
  *
  * The software contained herein may be directly re-distributed or may be adapted for redistribution
  * by SoC vendors who have licensed the related VeriSilicon/Hantro core IP, with the understanding
  * that the re-distribution is limited to the users who have purchased the SoC product which contains
  * the related VeriSilicon/Hantro IP. Otherwise, the information contained herein is not to be used by
  * or disclosed to the third parties without the express written permission of an officer of VeriSilicon Holdings Co., Ltd.
  *
  * The above copyright notice and this permission notice (including the
  * next paragraph) shall be included in all copies or substantial
  * portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
  * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
  * IN NO EVENT SHALL VERISILICON AND/OR ITS SUPPLIERS BE LIABLE FOR ANY
  * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef DECAPICOMMON_H
#define DECAPICOMMON_H

#include "basetype.h"

#ifdef ENABLE_PROFILE_LEVEL_CHECK
#define MAIN_HEVC_PROFILE(profile_idc, profile_space, profile_compat_flag) \
        ((((profile_idc) == 1) || ((profile_compat_flag) & 0x02)) && ((profile_space) == 0))
#define MAIN10_HEVC_PROFILE(profile_idc, profile_space, profile_compat_flag) \
        ((((profile_idc) == 2) || ((profile_compat_flag) & 0x04)) && ((profile_space) == 0))
#define MAIN_HEVC_STILL_PROFILE(profile_idc, profile_space, profile_compat_flag) \
        (((profile_idc) == 3 || ((profile_compat_flag) & 0x08)) && ((profile_space) == 0))
#define MONOCHROME_420_HEVC_EXT_PROFILE(profile_idc, profile_space, profile_compat_flag, chroma_format_idc, extension_present_flag, hevc_support) \
        (((profile_idc) == 4 || ((profile_compat_flag) & 0x10)) && ((profile_space) == 0) && \
         (chroma_format_idc == 0 || chroma_format_idc == 1) && (extension_present_flag == 1) && (hevc_support == 3))
#define MONOCHROME_420_HEVC_PROFILE(profile_idc, profile_space, profile_compat_flag, chroma_format_idc, extension_present_flag, hevc_support, range_extension_flag, \
                                    transform_skip_rotation_enabled_flag, transform_skip_context_enabled_flag, \
                                    persistent_rice_adaptation_enabled_flag, explicit_rdpcm_enabled_flag, \
                                    implicit_rdpcm_enabled_flag, intra_smoothing_disabled_flag, \
                                    chroma_qp_offset_list_enabled_flag) \
        (((profile_idc) == 4 || ((profile_compat_flag) & 0x10)) && ((profile_space) == 0) && \
          (chroma_format_idc == 0 || chroma_format_idc == 1) && (hevc_support >= 1) &&  ((extension_present_flag == 0) || \
           (extension_present_flag == 1 && range_extension_flag == 1 && \
            transform_skip_rotation_enabled_flag == 0 && transform_skip_context_enabled_flag == 0 && \
            persistent_rice_adaptation_enabled_flag == 0 && explicit_rdpcm_enabled_flag == 0 && \
            implicit_rdpcm_enabled_flag == 0 && intra_smoothing_disabled_flag == 0 && \
            chroma_qp_offset_list_enabled_flag == 0)))
#define HEVC_422_PROFILE(profile_idc, profile_space, profile_compat_flag, chroma_format_idc, hevc_support) \
        (((profile_idc) == 4 || ((profile_compat_flag) & 0x10)) && ((profile_space) == 0) && \
          (chroma_format_idc == 2) && (hevc_support >= 2))
#define HEVC_444_PROFILE(profile_idc, profile_space, profile_compat_flag, chroma_format_idc, hevc_support) \
        (((profile_idc) == 4 || ((profile_compat_flag) & 0x10)) && ((profile_space) == 0) && \
          (chroma_format_idc == 3) && (hevc_support == 3))
#define DCODEDABLE_PROFILE0(profile_idc, profile_space, extension_present_flag, profile_compat_flag) \
        ((((profile_idc) == 0) && ((profile_space) == 0) && (extension_present_flag == 0)) || \
         (((profile_space) > 0) && ((profile_idc) == 2) && ((profile_compat_flag) == 0)))

#define IS_SUPPORT_PROFILE_LEVEL_HEVC(profile_idc, profile_space, profile_compat_flag, chroma_format_idc, extension_present_flag, range_extension_flag, hevc_support,\
                                      transform_skip_rotation_enabled_flag, transform_skip_context_enabled_flag, \
                                      persistent_rice_adaptation_enabled_flag, explicit_rdpcm_enabled_flag, \
                                      implicit_rdpcm_enabled_flag, intra_smoothing_disabled_flag, \
                                      chroma_qp_offset_list_enabled_flag) \
        (MAIN_HEVC_PROFILE(profile_idc, profile_space, profile_compat_flag) || \
         MAIN10_HEVC_PROFILE(profile_idc, profile_space, profile_compat_flag) || \
         MAIN_HEVC_STILL_PROFILE(profile_idc, profile_space, profile_compat_flag) || \
         MONOCHROME_420_HEVC_PROFILE(profile_idc, profile_space, profile_compat_flag, chroma_format_idc, extension_present_flag, hevc_support, range_extension_flag, \
                                     transform_skip_rotation_enabled_flag, transform_skip_context_enabled_flag, \
                                     persistent_rice_adaptation_enabled_flag, explicit_rdpcm_enabled_flag, \
                                     implicit_rdpcm_enabled_flag, intra_smoothing_disabled_flag, \
                                     chroma_qp_offset_list_enabled_flag) || \
         DCODEDABLE_PROFILE0(profile_idc, profile_space, extension_present_flag, profile_compat_flag) || \
         MONOCHROME_420_HEVC_EXT_PROFILE(profile_idc, profile_space, profile_compat_flag, chroma_format_idc, extension_present_flag, hevc_support) || \
         HEVC_422_PROFILE(profile_idc, profile_space, profile_compat_flag, chroma_format_idc, hevc_support) || \
         HEVC_444_PROFILE(profile_idc, profile_space, profile_compat_flag, chroma_format_idc, hevc_support))

#define STERRO_HIGH_MVC_PROFILE(profile) \
        ((profile) == 118 || (profile) == 77 || \
         (profile) == 100 || (profile) == 128)
#define SVC_PROFILE(profile) ((profile) == 83 || (profile) == 86)
#define BASELINE_H264_PROFILE(profile) ((profile) == 66)
#define MAIN_H264_PROFILE(profile) ((profile) == 77)
#define MONOCHROME_420_H264_PROFILE(chroma_format_idc) ((chroma_format_idc) == 0 || (chroma_format_idc) == 1)
#define HIGH_H264_PROFILE(profile) ((profile) == 100)
#define HIGH10_H264_PROFILE(profile) ((profile) == 110)
#define H264_422_PROFILE(profile, h264_support) (((profile) == 122) && (h264_support >= 2))
#define H264_444_PROFILE(profile, h264_support) (((profile) == 244 || (profile) == 44) && (h264_support == 3))
#define IS_SUPPORT_PROFILE_LEVEL_H264(profile, chroma_format_idc, h264_support) \
        (STERRO_HIGH_MVC_PROFILE(profile) || SVC_PROFILE(profile) || BASELINE_H264_PROFILE(profile) || \
         MONOCHROME_420_H264_PROFILE(chroma_format_idc) || MAIN_H264_PROFILE(profile) || HIGH_H264_PROFILE(profile) || \
         HIGH10_H264_PROFILE(profile) || H264_422_PROFILE(profile, h264_support) || H264_444_PROFILE(profile, h264_support))

#define VP9_420_PROFILE(profile) (((profile) == 0) || ((profile) == 2))
#define IS_SUPPORT_PROFILE_LEVEL_VP9(profile) (VP9_420_PROFILE(profile))
#define AV1_420_PROFILE(profile) ((profile) == 0)
#define AV1_444_PROFILE(profile, av1_support) (((profile) == 1) && (av1_support == 3))
#define AV1_422_PROFILE(profile, av1_support) (((profile) == 2) && (av1_support >= 2))
#define IS_SUPPORT_PROFILE_LEVEL_AV1(profile, av1_support) \
  (AV1_420_PROFILE(profile) || AV1_422_PROFILE(profile, av1_support) || AV1_444_PROFILE(profile, av1_support))
#define IS_SUPPORT_PROFILE_LEVEL_AVS2(profile) \
  ((profile) == 0x20 || (profile) == 0x22)
#define VVC_MAIN10_PROFILE(profile) ((profile) == 1)
#define VVC_MAIN10_STILL_PROFILE(profile) ((profile) == 65)
#define IS_SUPPORT_PROFILE_LEVEL_VVC(profile) \
  (VVC_MAIN10_PROFILE(profile) || VVC_MAIN10_STILL_PROFILE(profile))
#define AVS3_MAIN_PROFILE(profile) ((profile) == 0x20)
#define AVS3_MAIN10_PROFILE(profile) ((profile) == 0x22)
#define AVS3_HIGH_PROFILE(profile) ((profile) == 0x30)
#define AVS3_HIGH10_PROFILE(profile) ((profile) == 0x32)
#define IS_SUPPORT_PROFILE_LEVEL_AVS3(profile) \
  (AVS3_MAIN_PROFILE(profile) || AVS3_MAIN10_PROFILE(profile) || AVS3_HIGH_PROFILE(profile) || AVS3_HIGH10_PROFILE(profile))
#define IS_SUPPORT_PROFILE_LEVEL_VC1(profile) \
  ((profile) == VC1_SIMPLE_PROFILE || (profile) == VC1_MAIN_PROFILE || (profile) == VC1_ADVANCED_PROFILE)

#define IS_SUPPORT_PROFILE_LEVEL_MPEG4(profile_type, profile_level) \
  (((profile_type) == 1 || \
    (profile_type) == 4 || \
    (profile_type) == 17) || \
   ((profile_level) == 8 || \
    (profile_level) == 1 || \
    (profile_level) == 2 || \
    (profile_level) == 3 || \
    (profile_level) == 16 || \
    (profile_level) == 17 || \
    (profile_level) == 18 || \
    (profile_level) == 50 || \
    (profile_level) == 51 || \
    (profile_level) == 52 || \
    (profile_level) == 240 || \
    (profile_level) == 241 || \
    (profile_level) == 242 || \
    (profile_level) == 243 || \
    (profile_level) == 244 || \
    (profile_level) == 245))

#define IS_SUPPORT_VERSION_DIVX(version) \
  ((version) == 3 || \
   (version) == 4 || \
   (version) == 5)

#define IS_SUPPORT_PROFILE_LEVEL_MPEG2_MPEG1(is_mpeg2_stream, profile) \
  ((is_mpeg2_stream == 0) || \
   ((profile) == 5 || (profile) == 4))

#define IS_SUPPORT_PROFILE_LEVEL_VP8_VP7(version) \
  ((version) == 0 || \
   (version) == 1 || \
   (version) == 2 || \
   (version) == 3)

#define IS_SUPPORT_PROFILE_LEVEL_VP6(version, profile) \
  (((version) == 6 || (version) == 7 || (version) == 8) && (((profile) == 0) || (profile) == 3))

#define IS_SUPPORT_PROFILE_LEVEL_AVS(profile) \
  ((profile) == 0x20 || (profile) == 0x48)
#else
#define IS_SUPPORT_PROFILE_LEVEL_HEVC(profile_idc, profile_space, profile_compat_flag, chroma_format_idc, extension_present_flag, range_extension_flag, hevc_support,\
                                      transform_skip_rotation_enabled_flag, transform_skip_context_enabled_flag, \
                                      persistent_rice_adaptation_enabled_flag, explicit_rdpcm_enabled_flag, \
                                      implicit_rdpcm_enabled_flag, intra_smoothing_disabled_flag, \
                                      chroma_qp_offset_list_enabled_flag) 1
#define IS_SUPPORT_PROFILE_LEVEL_H264(profile, chroma_format_idc, h264_support) 1
#define IS_SUPPORT_PROFILE_LEVEL_AV1(profile, av1_support) 1
#define IS_SUPPORT_PROFILE_LEVEL_VP9(profile) 1
#define IS_SUPPORT_PROFILE_LEVEL_AVS3(profile) 1
#define IS_SUPPORT_PROFILE_LEVEL_VVC(profile) 1
#define IS_SUPPORT_PROFILE_LEVEL_AVS2(profile) 1
#define IS_SUPPORT_PROFILE_LEVEL_VC1(profile) 1
#define IS_SUPPORT_PROFILE_LEVEL_MPEG4(profile_type, profile_level) 1
#define IS_SUPPORT_PROFILE_LEVEL_MPEG2_MPEG1(is_mpeg2_stream, profile) 1
#define IS_SUPPORT_PROFILE_LEVEL_VP8_VP7(version) 1
#define IS_SUPPORT_PROFILE_LEVEL_VP6(version, profile) 1
#define IS_SUPPORT_PROFILE_LEVEL_AVS(profile) 1
#define IS_SUPPORT_VERSION_DIVX(version) 1
#endif

/**< \brief The maximum number of client types. */
#define MAX_CLIENT_TYPE 21

/** \brief The maximum number of cores supported in multi-core configuration.
 *  \ingroup common_group */
#define MAX_ASIC_CORES 4

/** \brief The maximum batch size in batch mode. \n This macro indicates the maximum number of frames that can be accumulated before decoding starts.
 *  \ingroup common_group */
#if (MAX_ASIC_CORES > 16)
#define MAX_BATCH_SIZE MAX_ASIC_CORES
#else
#define MAX_BATCH_SIZE 16
#endif

/**
 * \addtogroup common_group
 *
 * @{
 */
/**@}*/

/** \brief The maximum IDs of AV1 temporal layers and spatial layers for identifying frames to be decoded.
 * \n Bits 0 to 3 indicate the maximum temporal layer ID, which ranges from 0 to 7.
 * \n Bits 4 to 7 indicate the maximum spatial layer ID, which ranges from 0 to 3.
 *  \ingroup common_group */
#define MAX_SVC_POINT_LAYER  0x37
/** \brief The maximum number of output channels from \c VCDecNextPicture().
 *  \ingroup common_group */
#define MAX_PPU_COUNT 4
#define DEC_MAX_OUT_COUNT MAX_PPU_COUNT
/** \brief The post-processing channel 0 start register.
 *  \ingroup common_group */
#define PP0_START_REG 384
/** \brief The register range of post-processing channels.
 *  \ingroup common_group */
#define PPU_REG_RANGE 128
/** \brief The maximum number of registers.
 *  \ingroup common_group */
#define MAX_REG_COUNT (PP0_START_REG + PPU_REG_RANGE * MAX_PPU_COUNT)
/** \brief The YUV-to RGB conversion standard BT.601 for full range YUV.
 *  \ingroup common_group */
#define BT601 0
/** \brief The YUV-to RGB conversion standard BT.601 for limited range YUV.
 *  \ingroup common_group */
#define BT601_L 1
/** \brief The YUV-to RGB conversion standard BT.709 for full range YUV.
 *  \ingroup common_group */
#define BT709 2
/** \brief The YUV-to RGB conversion standard BT.709 for limited range YUV.
 *  \ingroup common_group */
#define BT709_L 3
/** \brief The YUV-to RGB conversion standard BT.2020 for full range YUV.
 *  \ingroup common_group */
#define BT2020 4
/** \brief The YUV-to RGB conversion standard BT.2020 for limited range YUV.
 *  \ingroup common_group */
#define BT2020_L 5
/** \brief A function that checks whether full range is used.
 *  \ingroup common_group */
#define IS_FULL_RANGE(fmt) ((fmt == BT601) || (fmt == BT709) || (fmt == BT2020))

/** \brief The linear interpolation algorithm.
 *  \ingroup common_group */
#define VSI_LINEAR 0
/** \brief The Lanczos interpolation algorithm.
 *  \ingroup common_group */
#define LANCZOS 1
/** \brief The nearest interpolation algorithm.
 *  \ingroup common_group */
#define NEAREST  2
/** \brief The bilinear interpolation algorithm.
 *  \ingroup common_group */
#define BI_LINEAR 3
/** \brief The bicubic interpolation algorithm.
 *  \ingroup common_group */
#define BICUBIC 4
/** \brief The Spline interpolation algorithm.
 *  \ingroup common_group */
#define SPLINE 5
/** \brief The box interpolation algorithm.
 *  \ingroup common_group */
#define BOX 6
/** \brief The fast linear interpolation algorithm.
 *  \ingroup common_group */
#define FAST_LINEAR 7
/** \brief The fast bicubic interpolation algorithm.
 *  \ingroup common_group */
#define FAST_BICUBIC 8
/** \brief The area interpolation algorithm.
 *  \ingroup common_group */
#define AREA 9

/** \brief The 4x4 tiled output mode.
 * \ingroup common_group */
#define TILED4x4 0
/** \brief The 8x8 tiled output mode.
 * \ingroup common_group */
#define TILED8x8 1
/** \brief The 16x16 tiled output mode.
 * \ingroup common_group */
#define TILED16x16 2
/** \brief The 32x8 tiled output mode.
 * \ingroup common_group */
#define TILED32x8 3
/** \brief The 128x2 tiled output mode.
 * \ingroup common_group */
#define TILED128x2 4
/** \brief The YUYV or UYVY 2x4 tiled output mode.
 * \ingroup common_group */
#define TILED2x4 5
/** \brief The RECON8x8 tiled mode.
 * \ingroup common_group */
#define RECONTILED8x8 6
/** \brief The 64x64 tiled output mode.
 * \ingroup common_group */
#define TILED64x64 7
/** \brief The YUYV or UYVY 8x8 tiled output mode.
 * \ingroup common_group */
#define PACKED422_TILED8x8 8

/** \brief The PVFBC 8x8 tiled output mode.
 * \ingroup common_group */
#define PVFBC_TILED8x8 1
/** \brief The PVFBC 16x4 tiled output mode.
 * \ingroup common_group */
#define PVFBC_TILED16x4 2
/** */
// #define PVFBC_TILED32x2 3 /* Not supported */



/** \brief The round-down algorithm for center-point selection.
 *  \ingroup common_group */
#define DOWN_ROUND 0
/** \brief The no-round algorithm for center-point selection.
 *  \ingroup common_group */
#define NO_ROUND   1
/** \brief The round-up algorithm for center-point selection.
 *  \ingroup common_group */
#define UP_ROUND   2

/** \brief The packed mode YUYV for YUV422 input.
 *  \ingroup common_group */
#define YUYV 0
/** \brief The packed mode UYVY for YUV422 input.
 *  \ingroup common_group */
#define UYVY 1

/** \brief The output format YUV400.
 *  \ingroup common_group */
#define PP_YUV400 0
/** \brief The output format YUV420.
 *  \ingroup common_group */
#define PP_YUV420 1
/** \brief The output format YUV422.
 *  \ingroup common_group */
#define PP_YUV422 2
/** \brief The output format YUV444.
 *  \ingroup common_group */
#define PP_YUV444 3

/** Defines VC1 profiles.
 *  \ingroup common_group */
enum VC1Profile {
  /** The Simple profile. */
  VC1_SIMPLE_PROFILE = 0,
  /** The Main profile. */
  VC1_MAIN_PROFILE = 4,
  /** The Advanced profile. */
  VC1_ADVANCED_PROFILE = 12
};

/**
 * \ingroup common_group
 */
/**  \brief The maximum number of output planes. */
#define MAX_OUTPUT_PLANE 3
#define PLANE_ALIGNMENT 1

/** \brief The 4x4 tiled input mode for standalone post-processing core without decoder pipeline.
 *  \ingroup common_group */
#define IN_TILED4x4 0
/** \brief The 8x8 tiled input mode for standalone post-processing core without decoder pipeline.
 *  \ingroup common_group */
#define IN_TILED8x8 1
/** \brief The 64x64 tiled input mode for standalone post-processing core without decoder pipeline.
 *  \ingroup common_group */
#define IN_TILED64x64 2
/** \brief The input 8x8 tiled mode.
 *  \ingroup common_group */
#define IN_RECON_TILED8x8 3
/** \brief The input 8x8 tiled mode.
 *  \ingroup common_group */
#define IN_RECON_TILED8x8_PACK10 4

/** Specifies the YUV-to-RGB conversion type.
 * \ingroup common_group
 */
enum ColorConversionTYpe{
  /** \brief BT601 or BT601_L */
  BT601_TYPE = 0,
  /** \brief BT709 or BT709_L. */
  BT709_TYPE = 1,
  /** \brief BT2020 or BT2020_L. */
  BT2020_TYPE = 2
};

/** \brief The input data format Y8b_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \n For details about data formats, see <em>Hantro VC9x00D Series Memory Buffer Format
 *  Specifications</em>.
 *  \ingroup common_group */
#define PP_IN_400_8BIT  0
/** \brief The input data format YUV420SP8b_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_NV12_8BIT 1
/** \brief The input data format YVU420SP8b_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_NV21_8BIT 2
/** \brief The input data format YUV420P8b_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_420P_8BIT 3
/** \brief The input data format YUV422SP8b_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_422_NV16_8BIT 4
/** \brief The third-party compressed 4x4 tiled input data format for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_TILED4x4  5
/** \brief The input data format YUV422YUYV8b_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_422_YUYV_8BIT 6
/** \brief The input data format YUV422YVYU8b_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_422_YVYU_8BIT 7
/** \brief The input data format YUV422UYVY8b_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_422_UYVY_8BIT 8
/** \brief The input data format YUV422VYUY8b_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_422_VYUY_8BIT 9
//#define PP_IN_TILED4x4_P010 9
/** \brief The input data format YUV420SP10bWH_Raster_A<em>X</em>. for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_NV12_P010 10
/** \brief The input data format YVU420SP10bWH_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_NV21_P010 11
/** \brief The input data format YUV420P10bWH_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_420P_P010 12
/** \brief The input data format YUV422SP10bWH_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_422_NV16_P010 13
/** \brief The third-party compressed 4x4 tiled P010 input data format for standalone
 *  post-processing core without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_TILED4x4_P010 14
/** \brief The input data format Y10bWH_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_400_P010  15
/** \brief The input data format RGB888P_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_RGB888_P 16
/** \brief The input data format BGR888P_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_BGR888_P 17
/** \brief The input data format RGB16l16l16lP_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_R16G16B16_P 18
/** \brief The input data format BGR16l16l16lP_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_B16G16R16_P 19
/** \brief The input data format RGB888_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_RGB888 20
/** \brief The input data format BGR888_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_BGR888 21
/** \brief The input data format ARGB8888_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_ARGB888 22
/** \brief The input data format ABGR8888_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_ABGR888 23
/** \brief The input data format ARGB2101010_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_A2R10G10B10 24
/** \brief The input data format ABGR2101010_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_A2B10G10R10 25
/** \brief The input data format XRGB2101010_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_X2R10G10B10 26
/** \brief The input data format XBGR2101010_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_X2B10G10R10 27
/** \brief The RFC compressed input data format for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_RFC         28
/** \brief The input data format YVU422SP8b_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_422_NV61_8BIT 29
/** \brief The input data format YVU422SP10bWH_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_422_NV61_P010 30
/** \brief The input data format YUV444P8b_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_444P_8BIT 31
/** \brief The input data format YUV444P10bWH_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_444P_P010 32
/** \brief The input data format RGBA8888_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_RGBA888 33 /* R8G8B8A8 */
/** \brief The input data format BGRA8888_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_BGRA888 34 /* B8G8R8A8 */
/** \brief The input data format RGBA1010102_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_R10G10B10A2 35
/** \brief The input data format BGRA1010102_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_B10G10R10A2 36
/** \brief The input data format XRGB888_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_XRGB888 37
/** \brief The input data format XBGR888_Raster_A<em>X</em> for standalone post-processing core
 *  without decoder pipeline.
 *  \ingroup common_group */
#define PP_IN_XBGR888 38
  /** \brief the Third-party 8x8 tiled format for standalone post-processing core. */
#define PP_IN_RECON_TILED8x8 39
  /** \brief the Third-party 8x8 tiled pack10 format for standalone post-processing core. */
#define  PP_IN_RECON_TILED8x8_PACK10 40

#if 0
#define PP_IN_422P_8BIT     40   /*< planar YUV 4:2:2*/
#define PP_IN_444_NV24_8BIT 41 /*< semi planar YUV 4:4:4, 24bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V)*/
#define PP_IN_444_NV42_8BIT 42 /*as above, but U and V bytes are swapped*/

#define PP_IN_422P_P010     50   /*< planar YUV 4:2:2*/
#define PP_IN_444_NV24_P010 51
#define PP_IN_444_NV42_P010 52
#endif

#if !defined(MODEL_SIMULATION) || defined(HW_PIC_DIMENSIONS)
/** \brief The minimum picture width supported by advanced decoder hardware for codec standards except AVS2.
 *  \ingroup common_group */
#define MIN_PIC_WIDTH 72
/** \brief The minimum picture height supported by advanced decoder hardware for codec standards except AVS2.
 *  \ingroup common_group */
#define MIN_PIC_HEIGHT 72
/** \brief The minimum picture width supported by advanced decoder hardware for AVS2.
 *  \ingroup common_group */
#define MIN_PIC_WIDTH_AVS2 128
/** \brief The minimum picture height supported by advanced decoder hardware for AVS2.
 *  \ingroup common_group */
#define MIN_PIC_HEIGHT_AVS2 128
#define MIN_PIC_WIDTH_H264          48
#define MIN_PIC_HEIGHT_H264          48
#else /* MODEL_SIMULATION */
#define MIN_PIC_WIDTH               8
#define MIN_PIC_HEIGHT              8

#define MIN_PIC_WIDTH_AVS2          8
#define MIN_PIC_HEIGHT_AVS2         8

#define MIN_PIC_WIDTH_H264          8
#define MIN_PIC_HEIGHT_H264         8
#endif /* MODEL_SIMULATION */

#define MIN_PIC_WIDTH_G1            48
#define MIN_PIC_HEIGHT_G1           48
#define PNGDEC_MIN_WIDTH            32
#define PNGDEC_MIN_HEIGHT           32
#define JPEGDEC_MAX_WIDTH_TN        256
#define JPEGDEC_MAX_HEIGHT_TN       256

/** JPEG specific.
 *  \ingroup jpeg_group */
enum {
  /** No units, X and Y specify the pixel aspect ratio. */
  JPEGDEC_NO_UNITS = 0,
  /** \brief X and Y are dots per inch. */
  JPEGDEC_DOTS_PER_INCH = 1,
  /** \brief X and Y are dots per cm. */
  JPEGDEC_DOTS_PER_CM = 2
};

/** Defines JPEG thumbnail types.
 *  \ingroup jpeg_group */
enum {
  /** Thumbnail in JPEG format. */
  JPEGDEC_THUMBNAIL_JPEG = 0x10,
  /** Unsupported thumbnail format. */
  JPEGDEC_THUMBNAIL_NOT_SUPPORTED_FORMAT = 0x11,
  /** No thumbnail. */
  JPEGDEC_NO_THUMBNAIL = 0x12
};

/** Defines to-be-decoded JPEG/PNG image types.
 *  \ingroup jpeg_png_group */
enum {
  /** Full resolution JPEG. */
  JPEGDEC_IMAGE = 0,
  /** Thumbnail JPEG. */
  JPEGDEC_THUMBNAIL = 1,
  /** PNG. */
  PNGDEC_IMAGE = 2
};

/** Defines JPEG coding modes.
 *  \ingroup jpeg_group */
enum JpegCodingMode {
  /** Unsupported process. */
  JPEG_NOT_SUPPORTED = 0x0,
  /** Baseline DCT process. */
  JPEG_BASELINE = 0x01,
  /** Progressive DCT process, Huffman coding. */
  JPEG_PROGRESSIVE = 0x02,
  /** Extended sequential DCT process, Huffman coding. */
  JPEG_EXTENDED = 0x03,
  /** Non-interleaved encoding. */
  JPEG_NONINTERLEAVED = 0x04,
};

/** Defines the PNG color type.
 *  \ingroup png_group */
enum ColorType {
  /** Grayscale. */
  PNG_COLOR_TYPE_GRAY = 0,
  /** RGB565.
   * \n This color type is available only for LZ77 decoding.  */
  LZ77_COLOR_TYPE_RGB565 = 1,
  /** True color. */
  PNG_COLOR_TYPE_RGB = 2,
  /** Indexed color. */
  PNG_COLOR_TYPE_INDEX = 3,
  /** Grayscale with alpha. */
  PNG_COLOR_TYPE_GRAY_ALPHA = 4,
  /** RGB565 with alpha.
   * \n This color type is available only for LZ77 decoding.  */
  LZ77_COLOR_TYPE_RGB565_ALPHA = 5,
  /** True color with alpha. */
  PNG_COLOR_TYPE_RGB_ALPHA = 6
};

/**
 * \brief Different PNG coding mode.
 * \ingroup png_group
 */
enum PngCodingMode{
  PNGDEC_BASELINE = 0x5,  /**< \brief png non-interlaced to be decoded */
  PNGDEC_INTERLACE = 0x6  /**< \brief png interlaced to be decoded */
};

#ifdef OPEN_HWCFG_TO_CLIENT
/** \brief A structure to store hardware configuration.
 *  \ingroup common_group */
struct DecHwConfig {
  u32 mpeg4_support;        /**< \brief one of the MPEG4 values defined above */
  u32 custom_mpeg4_support; /**< \brief one of the MPEG4 custom values defined above */
  u32 h264_support;         /**< \brief one of the H264 values defined above */
  u32 vc1_support;          /**< \brief one of the VC1 values defined above */
  u32 mpeg2_support;        /**< \brief one of the MPEG2 values defined above */
  u32 jpeg_support;         /**< \brief one of the JPEG values defined above */
  u32 png_support;         /**< \brief one of the PNG values defined above */
  u32 max_dec_pic_width[MAX_CLIENT_TYPE];    /**< \brief maximum picture width in decoder */
  u32 max_dec_pic_height[MAX_CLIENT_TYPE];   /**< \brief maximum picture height in decoder */
  u32 pp_standalone;        /**< \brief PP_SUPPORTED or PP_NOT_SUPPORTED */
  u32 pp_config;            /**< \brief Bitwise list of PP function */
  u32 max_pp_out_pic_width;   /**< \brief maximum post-processor output picture width */
  u32 max_pp_out_pic_height;   /**< \brief maximum post-processor output picture height */
  u32 sorenson_spark_support; /**< \brief one of the SORENSON_SPARK values defined above */
  u32 vp6_support;           /**< \brief one of the VP6 values defined above */
  u32 vp7_support;           /**< \brief one of the VP7 values defined above */
  u32 vp8_support;           /**< \brief one of the VP8 values defined above */
  u32 vp9_support;           /**< \brief HW supports VP9 */
  u32 avs_support;           /**< \brief one of the AVS values defined above */
  u32 rv_support;            /**< \brief one of the HUKKA values defined above */
  u32 webp_support;          /**< \brief one of the WEBP values defined above */
  u32 stride_support;        /**< \brief HW supports separate Y and C strides */
  u32 avs_plus_support;      /**< \brief one of the AVS PLUS values defined above */
  u32 addr64_support;         /**< \brief HW supports 64bit addressing */
  u32 avs2_support;           /**< \brief HW supports AVS2 */
  u32 hevc_support;          /**< \brief HW supports HEVC */
  u32 av1_support;           /**< \brief HW supports AV1 */
  u32 vvc_support;           /**< \brief HW supports VVC */

  u32 hevc_main10_support;  /**< \brief HW supports HEVC Main10 profile*/
  u32 vp9_10bit_support;     /**< \briefHW supports VP9 10 bits profile */
  u32 ds_support;            /**< \brief HW supports down scaling. */
  u32 rfc_support;           /**< \brief HW supports reference frame compression. */
  u32 fmt_p010_support;      /**< \brief HW supports P010 format. */
};
#endif

/** \brief Contains the information of the decoder API version.
 *  \ingroup common_group */
struct DecApiVersion {
  /** \brief The major version with architecture update or product promotion. */
  u32 major;
  /** \brief The minor version with milestone features added. */
  u32 minor;
  /** \brief The micro version with feature improvement or interface changes. */
  u32 micro;
};

/** \brief Contains the build information.
 *  \ingroup common_group */
struct DecSwHwBuild {
  /** \brief The software build ID. */
  u32 sw_build;
  /** \brief The hardware ASIC ID. */
  u32 asic_id;
  /** \brief The hardware build ID of each core. */
  u32 hw_build_id[MAX_ASIC_CORES];
#ifdef OPEN_HWCFG_TO_CLIENT
  /** \brief The hardware configurations of each core. */
  struct DecHwConfig hw_config[MAX_ASIC_CORES];
#endif
};

#define LOW_LATENCY_PACKET_SIZE 256

/** Defines content storage modes in DPB.
 *  \ingroup common_group */
enum DecDpbMode {
  /** (Default) Data in DPB is stored by frame. */
  DEC_DPB_FRAME = 0,
  /** Data in DPB is stored by interfaced field. */
  DEC_DPB_INTERLACED_FIELD = 1
};

/** Defines decoder working modes.
 *  \ingroup common_group */
enum DecDecoderMode {
  /** The normal decoding mode, in which the hardware is not enabled until the complete data of a
   *  frame is ready.
   *  \n The minimum hardware decoding unit is a frame. */
  DEC_NORMAL =           0x00000000,
  /** The low-latency decoding mode, in which the hardware can be enabled before the input
   *  stream data arrives.
   *  \n The minimum hardware decoding unit is 256 bytes. */
  DEC_LOW_LATENCY =      0x00000001,
  /** The security playback mode to adapt to Digital Rights Management (DRM) streams.
   * */
  DEC_SECURITY =         0x00000002,
  /** A special decoding mode which does not write reconstructed data, reference data,
   *  MV, or post-processing output for non-reference pictures.
   *  \n This mode is not supported currently. */
  DEC_PARTIAL_DECODING = 0x00000004,
  /** The intra-only decoding mode, which decodes intra-coded frames only. */
  DEC_INTRA_ONLY =       0x00000008,
  /** The HEIF mode, in which the decoder is fed with HEIF files as its input. */
  DEC_HEIF =             0x00000010
};


/** Defines flags to indicate whether auxiliary infomation is present.
 *  \ingroup common_group */
enum DecAuxInfo {
  /** No auxiliary infomation is present. */
  DEC_AUX_NONE =          0x00000000,
  /** QP information is present. */
  DEC_AUX_QP =            0x00000001,
  /** MV information is present. */
  DEC_AUX_MV =            0x00000002,
};

/** Defines delogo filter modes.
 *  \ingroup common_group */
enum DelogoMode {
  /** \brief Disables the delogo filter. */
  PIXEL_NO_DELOGO = 0,
  /** \brief Enables the delogo filter in replacement mode. */
  PIXEL_REPLACE = 1,
  /** \brief Enables the delogo filter in interpolation mode. */
  PIXEL_INTERPOLATION = 2
};
/* DEPRECATED!!! do not use in new applications! */
#define DEC_DPB_DEFAULT DEC_DPB_FRAME

/** Defines output picture formats.
 *
 *  For details about color formats, see <em>Hantro VC9x00D Series Memory Buffer Format
 *  Specifications</em>.
 *  \ingroup common_group
 *  \latexonly
 *  \small
 *  \endlatexonly
 */
enum DecPictureFormat {
  /** Tiled 4x4 format. */
  DEC_OUT_FRM_TILED_4X4 = 0,
  /** (Deprecated) Tiled 8x4 format. */
  DEC_OUT_FRM_TILED_8X4 = 1,
  /** YUV420SP8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_RASTER_SCAN = 2,
  /** YUV420P8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_PLANAR_420 = 3,
  /** Tiled 8x8 format. */
  DEC_OUT_FRM_TILED_8X8 = 4,
  /** Tiled 8x8 P010 format, 10-bit and 12-bit. */
  DEC_OUT_FRM_TILED_8X8_P010 = 5,
  /** Tiled 16x16 format. */
  DEC_OUT_FRM_TILED_16X16 = 6,
  /** Tiled 16x16 P010 format, 10-bit and 12-bit. */
  DEC_OUT_FRM_TILED_16X16_P010 = 7,
  /** Tiled 64x64 format. */
  DEC_OUT_FRM_TILED_64X64,
  /** Tiled 64x64 packed 10-bit format. */
  DEC_OUT_FRM_TILED_64X64_PACK10,
  /** Y8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_MONOCHROME,
  /** Reference frame compression (RFC)
   * \n compressed format. */
  DEC_OUT_FRM_RFC,

  /* FBC */
  /** Third-party compressed 32x8 tiled format. */
  DEC_OUT_FRM_TILED_32X8_COMP,
  /** Third-party compressed 32x8 tiled \n
   * P010 format, 10-bit and 12-bit. */
  DEC_OUT_FRM_TILED_32X8_P010_COMP,
  /** Third-party compressed 16x16 tiled format. */
  DEC_OUT_FRM_TILED_16X16_COMP,
  /** Third-party compressed 16x16 tiled \n
   * P010 format, 10-bit and 12-bit. */
  DEC_OUT_FRM_TILED_16X16_P010_COMP,

  /* DEC400 */
  /** DEC400 compressed YUV420 semi-planar. */
  DEC_OUT_FRM_DEC400_420SP,
  /** DEC400 compressed YUV420 10-bit semi-planar. */
  DEC_OUT_FRM_DEC400_420SP_P010,
  /** DEC400 compressed YVU420 semi-planar. */
  DEC_OUT_FRM_DEC400_NV21SP,
  /** DEC400 compressed YVU420 10-bit semi-planar. */
  DEC_OUT_FRM_DEC400_NV21SP_P010,
  /** DEC400 compressed YUV400 tiled 4x4. */
  DEC_OUT_FRM_DEC400_400TILE,
  /** DEC400 compressed YUV420 tiled 4x4. */
  DEC_OUT_FRM_DEC400_420TILE,
  /** DEC400 compressed YUV400 8-bit tiled 8x8. */
  DEC_OUT_FRM_DEC400_400TILED8x8,
  /** DEC400 compressed YUV400 10-bit tiled 8x8. */
  DEC_OUT_FRM_DEC400_400TILED8x8_P010,
  /** DEC400 compressed YUV420 8-bit tiled 8x8. */
  DEC_OUT_FRM_DEC400_420TILED8x8,
  /** DEC400 compressed YUV420 10-bit tiled 8x8. */
  DEC_OUT_FRM_DEC400_420TILED8x8_P010,
  /** DEC400 compressed YVU420 8-bit tiled 8x8. */
  DEC_OUT_FRM_DEC400_NV21TILED8x8,
  /** DEC400 compressed YVU420 10-bit tiled 8x8. */
  DEC_OUT_FRM_DEC400_NV21TILED8x8_P010,
  /** DEC400 compressed YUV400 8-bit planar. */
  DEC_OUT_FRM_DEC400_400,
  /** DEC400 compressed YUV400 10-bit planar. */
  DEC_OUT_FRM_DEC400_400_P010,
  /** DEC400 compressed YUV420 8-bit planar. */
  DEC_OUT_FRM_DEC400_420P,
  /** DEC400 compressed YUV420 10-bit planar. */
  DEC_OUT_FRM_DEC400_420P_P010,
  /** DEC400 compressed YUV422YUYV packed \n
   * 8-bit format. */
  DEC_OUT_FRM_DEC400_YUV422_YUYV,
  /** DEC400 compressed YUV422UYVY packed \n
   * 8-bit format. */
  DEC_OUT_FRM_DEC400_YUV422_UYVY,
  /** DEC400 compressed BGR888. */
  DEC_OUT_FRM_DEC400_BGR888,
  /** DEC400 compressed RGB888. */
  DEC_OUT_FRM_DEC400_RGB888,
  /** DEC400 compressed ABGR8888. */
  DEC_OUT_FRM_DEC400_ABGR888,
  /** DEC400 compressed ARGB8888. */
  DEC_OUT_FRM_DEC400_ARGB888,
  /** DEC400 compressed BGRA8888. */
  DEC_OUT_FRM_DEC400_BGRA888,
  /** DEC400 compressed RGBA8888. */
  DEC_OUT_FRM_DEC400_RGBA888,
  /** DEC400 compressed A2B10G10R10. */
  DEC_OUT_FRM_DEC400_A2B10G10R10,
  /** DEC400 compressed A2R10G10B10. */
  DEC_OUT_FRM_DEC400_A2R10G10B10,
  /** DEC400 compressed X2B10G10R10. */
  DEC_OUT_FRM_DEC400_X2B10G10R10,
  /** DEC400 compressed X2R10G10B10. */
  DEC_OUT_FRM_DEC400_X2R10G10B10,
  /** DEC400 compressed A2B10G10R10. */
  DEC_OUT_FRM_DEC400_B10G10R10A2,
  /** DEC400 compressed A2R10G10B10. */
  DEC_OUT_FRM_DEC400_R10G10B10A2,
  /** DEC400 compressed XBGR8888. */
  DEC_OUT_FRM_DEC400_XBGR888,
  /** DEC400 compressed XRGB8888. */
  DEC_OUT_FRM_DEC400_XRGB888,
  /** DEC400 compressed ABGR8888 \n
   *  tiled 64x64.*/
  DEC_OUT_FRM_DEC400_ABGR888_TILED64X64,
  /** DEC400 compressed ARGB8888 \n
   * tiled 64x64. */
  DEC_OUT_FRM_DEC400_ARGB888_TILED64X64,
  /** DEC400 compressed A2B10G10R10 \n
   * tiled 64x64. */
  DEC_OUT_FRM_DEC400_A2B10G10R10_TILED64X64,
  /** DEC400 compressed A2R10G10B10 \n
   * tiled 64x64. */
  DEC_OUT_FRM_DEC400_A2R10G10B10_TILED64X64,
  /** DEC400 compressed X2B10G10R10 \n
   * tiled 64x64. */
  DEC_OUT_FRM_DEC400_X2B10G10R10_TILED64X64,
  /** DEC400 compressed X2R10G10B10 \n
   * tiled 64x64. */
  DEC_OUT_FRM_DEC400_X2R10G10B10_TILED64X64,
  /** DEC400 compressed XBGR8888 \n
   * tiled 64x64. */
  DEC_OUT_FRM_DEC400_XBGR888_TILED64X64,
  /** DEC400 compressed XRGB8888 \n
   * tiled 64x64. */
  DEC_OUT_FRM_DEC400_XRGB888_TILED64X64,
  /** DEC400 compressed BGRA8888 \n
   * tiled 64x64. */
  DEC_OUT_FRM_DEC400_BGRA888_TILED64X64,
  /** DEC400 compressed RGBA8888 \n
   * tiled 64x64. */
  DEC_OUT_FRM_DEC400_RGBA888_TILED64X64,
  /** DEC400 compressed B10G10R10A2 \n
   * tiled 64x64. */
  DEC_OUT_FRM_DEC400_B10G10R10A2_TILED64X64,
  /** DEC400 compressed R10G10B10A2 \n
   * tiled 64x64. */
  DEC_OUT_FRM_DEC400_R10G10B10A2_TILED64X64,

  /*PVFBC*/
  /** PVFBC compressed \n
   *  YUV420 semi-planar tiled 16x4.*/
  DEC_OUT_FRM_PVFBC_TILED16X4_420SP,
  /** PVFBC compressed \n
   *  YUV420 semi-planar tiled 8x8.*/
  DEC_OUT_FRM_PVFBC_TILED8X8_420SP,
  /** PVFBC compressed \n
   * YUV420 P010 semi-planar \n
   * tiled 16x4.*/
  DEC_OUT_FRM_PVFBC_TILED16X4_420SP_P010,
  /** PVFBC compressed \n
   * YUV420 P010 semi-planar \n
   * tiled 8x8.*/
  DEC_OUT_FRM_PVFBC_TILED8X8_420SP_P010,

  /*PVFBC BYPASS*/
  /** YUV420 semi-planar tiled 16x4 \n
   * without PVFBC compression.*/
  DEC_OUT_FRM_PVFBC_BYPASS_TILED16X4_420SP,
  /** YUV420 semi-planar tiled 8x8 \n
   * without PVFBC compression.*/
  DEC_OUT_FRM_PVFBC_BYPASS_TILED8X8_420SP,
  /** YUV420 P010 semi-planar tiled 16x4 \n
   * without PVFBC compression.*/
  DEC_OUT_FRM_PVFBC_BYPASS_TILED16X4_420SP_P010,
  /** YUV420 P010 semi-planar tiled 8x8 \n
   * without PVFBC compression.*/
  DEC_OUT_FRM_PVFBC_BYPASS_TILED8X8_420SP_P010,


  /* YUV420 */
  /** Uncompressed YUV420SP8b_YuvSp4x4_A<em>X</em> \n
   *  for output to decoded picture buffer (DPB). */
  DEC_OUT_FRM_YUV420TILE,
  /** Uncompressed YUV420SP10b_YuvSp4x4_A<em>X</em> \n
   * for output to DPB. */
  DEC_OUT_FRM_YUV420TILE_PACK10,
  /** Uncompressed YUV420SP12b_YuvSp4x4_A<em>X</em> \n
   * for output to DPB. */
  DEC_OUT_FRM_YUV420TILE_PACK12,
  /** Uncompressed YUV420SP8b_YuvSp8x8_A<em>X</em> \n
   * for output to DPB. */
  DEC_OUT_FRM_YUV420TILE8x8,
  /** Uncompressed YUV420SP10b_YuvSp8x8_A<em>X</em> \n
   * for output to DPB. */
  DEC_OUT_FRM_YUV420TILE8x8_PACK10,
  /** Uncompressed YUV420SP12b_YuvSp8x8_A<em>X</em> \n
   * for output to DPB. */
  DEC_OUT_FRM_YUV420TILE8x8_PACK12,
  /** Uncompressed reconstructed format \n
   * for YUV420SP8b_YuvSp8x8_A<em>X</em> \n
   * for output to DPB. */
  DEC_OUT_FRM_YUV420RECONTILE8X8,
  /** Reconstructed format \n
   * for YUV420SP8b_Tile8x8_A<em>X</em>. */
  DEC_OUT_FRM_YUV420RECONTILE8X8_P010,
  /** Reconstructed format \n
   * for YUV420SP10b_Tile8x8_A<em>X</em>. */
  DEC_OUT_FRM_YUV420RECONTILE8X8_PACK10,
  /** YUV420SP10bWH_YuvSp4x4_A<em>X</em>. */
  DEC_OUT_FRM_YUV420TILE_P010,
  /** YUV420SP10bWL_YuvSp4x4_A<em>X</em> I010. */
  DEC_OUT_FRM_YUV420TILE_I010,
  /** YUV420SP10bWL_YuvSp4x4_A<em>X</em> L010. */
  DEC_OUT_FRM_YUV420TILE_L010,
  /** YUV420SP12bWH_YuvSp4x4_A<em>X</em>. */
  DEC_OUT_FRM_YUV420TILE_P012,
  /** YUV420SP12bWL_YuvSp4x4_A<em>X</em> I012. */
  DEC_OUT_FRM_YUV420TILE_I012,
  /** YUV420SP8b_YuvSp128x2_A<em>X</em>. */
  DEC_OUT_FRM_YUV420TILE_128X2,
  /** YUV420SP10bWH_YuvSp128x2_A<em>X</em>. */
  DEC_OUT_FRM_YUV420TILE_128X2_P010,
  /** YUV420SP8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV420SP,
  /** YUV420SP10b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV420SP_PACK10,
  /** (Deprecated) YUV420SP12b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV420SP_PACK12,
  /** YUV420SP10bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV420SP_P010,
  /** YUV420SP10bWL_Raster_A<em>X</em> I010. */
  DEC_OUT_FRM_YUV420SP_I010,
  /** YUV420SP10bWL_Raster_A<em>X</em> L010. */
  DEC_OUT_FRM_YUV420SP_L010,
  /** YUV420SP12bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV420SP_P012,
  /** YUV420SP12bWL_Raster_A<em>X</em> I012. */
  DEC_OUT_FRM_YUV420SP_I012,
  /** YUV420SP10bDWL_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV420SP_1010,
  /** YUV420P8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV420P,
  /** YUV420P10b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV420P_PACK10,
  /** YUV420P12b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV420P_PACK12,
  /** YUV420P10bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV420P_P010,
  /** YUV420P10bWL_Raster_A<em>X</em> I010. */
  DEC_OUT_FRM_YUV420P_I010,
  /** YUV420P10bWL_Raster_A<em>X</em> L010. */
  DEC_OUT_FRM_YUV420P_L010,
  /** YUV420P12bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV420P_P012,
  /** YUV420P12bWL_Raster_A<em>X</em> I012. */
  DEC_OUT_FRM_YUV420P_I012,
  /** YUV420P10bDWL_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV420P_1010,

  /* YUV400 */
  /** Uncompressed Y8b_Tile4x4_A<em>X</em> \n
   * for output to DPB. */
  DEC_OUT_FRM_YUV400TILE,
  /** Uncompressed Y10b_Tile4x4_A<em>X</em> \n
   * for output to DPB. */
  DEC_OUT_FRM_YUV400TILE_PACK10,
  /** Uncompressed Y8b_Tile8x8_A<em>X</em> \n
   * for output to DPB. */
  DEC_OUT_FRM_YUV400TILE8x8,
  /** Uncompressed Y10b_Tile8x8_A<em>X</em> \n
   * for output to DPB. */
  DEC_OUT_FRM_YUV400TILE8x8_PACK10,
  /** Uncompressed Y10bWH_Tile8x8_A<em>X</em> \n
   * for output to DPB. */
  DEC_OUT_FRM_YUV400TILE8x8_P010,
  /** Y10bWH_Tile4x4_A<em>X</em>. */
  DEC_OUT_FRM_YUV400TILE_P010,
  /** Y10bWL_Tile4x4_A<em>X</em> I010. */
  DEC_OUT_FRM_YUV400TILE_I010,
  /** Y10bWL_Tile4x4_A<em>X</em> L010. */
  DEC_OUT_FRM_YUV400TILE_L010,
  /** (Deprecated) Y12bWH_Tile4x4_A<em>X</em>. */
  DEC_OUT_FRM_YUV400TILE_P012,
  /** (Deprecated) Y12bWL_Tile4x4_A<em>X</em> I012. */
  DEC_OUT_FRM_YUV400TILE_I012,
  /** Y8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV400,
  /** Y10b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV400_PACK10,
  /** (Deprecated) Y12b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV400_PACK12,
  /** Y10bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV400_P010,
  /** Y10bWL_Raster_A<em>X</em> I010. */
  DEC_OUT_FRM_YUV400_I010,
  /** Y10bWL_Raster_A<em>X</em> L010. */
  DEC_OUT_FRM_YUV400_L010,
  /** Y12bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV400_P012,
  /** Y12bWL_Raster_A<em>X</em> I012. */
  DEC_OUT_FRM_YUV400_I012,
  /** Y8b_Tile128x2_A<em>X</em>. */
  DEC_OUT_FRM_YUV400TILE_128X2,
  /** Y10bWH_Tile128x2_A<em>X</em>. */
  DEC_OUT_FRM_YUV400TILE_128X2_P010,
  /** YUV400_with_alpha_8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV400_WITH_ALPHA,

  /* YUV422 */
  /** YUV422SP8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV422SP,
  /** YUV422SP10b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV422SP_PACK10,
  /** (Deprecated) YUV422SP12b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV422SP_PACK12,
  /** 8-bit YUV422 tiled 8x8 output format \n
   * for non-reference frames.*/
  DEC_OUT_FRM_YUV422TILE8x8,
  /** 10-bit YUV422 tiled 8x8 output format \n
   * for non-reference frames.*/
  DEC_OUT_FRM_YUV422TILE8x8_PACK10,
  /** 12-bit YUV422 tiled 8x8 output format \n
   * for non-reference frames.*/
  DEC_OUT_FRM_YUV422TILE8x8_PACK12,
  /** YUV422SP10bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV422SP_P010,
  /** YUV422SP10bWL_Raster_A<em>X</em> I010. */
  DEC_OUT_FRM_YUV422SP_I010,
  /** YUV422SP10bWL_Raster_A<em>X</em> L010. */
  DEC_OUT_FRM_YUV422SP_L010,
  /** (Deprecated) YUV422SP12bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV422SP_P012,
  /** (Deprecated) YUV422SP12bWL_Raster_A<em>X</em> I012. */
  DEC_OUT_FRM_YUV422SP_I012,
  /** YUV422SP10bDWL_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV422SP_1010,
  /** (Deprecated) YUV422P8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV422P,
  /** (Deprecated) YUV422P10b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV422P_PACK10,
  /** (Deprecated) YUV422P12b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV422P_PACK12,
  /** YUV422P10bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV422P_P010,
  /** (Deprecated) YUV422P10bWL_Raster_A<em>X</em> I010. */
  DEC_OUT_FRM_YUV422P_I010,
  /** (Deprecated) YUV422P10bWL_Raster_A<em>X</em> L010. */
  DEC_OUT_FRM_YUV422P_L010,
  /** (Deprecated) YUV422P12bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV422P_P012,
  /** (Deprecated) YUV422P12bWL_Raster_A<em>X</em> I012. */
  DEC_OUT_FRM_YUV422P_I012,
  /** YUV422YUYV8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV422_YUYV,
  /** YUV422UYVY8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV422_UYVY,
  /** YUV422YVYU8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV422_YVYU,
  /** YUV422VYUY8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV422_VYUY,
  /** YUV422YUYV8b_Tile8x8_A<em>X</em>. */
  DEC_OUT_FRM_YUV422_YUYV_TILED8X8,
  /** YUV422UYVY8b_Tile8x8_A<em>X</em>. */
  DEC_OUT_FRM_YUV422_UYVY_TILED8X8,
  /** YUV422YUYV8b_Tile2x4_A<em>X</em>. */
  DEC_OUT_FRM_YUV422_YUYV_TILED2X4,
  /** YUV422UYVY8b_Tile2x4_A<em>X</em>. */
  DEC_OUT_FRM_YUV422_UYVY_TILED2X4,

  /** RGB888_Tile4x4_A<em>X</em>. */
  DEC_OUT_FRM_RGB888_TILED4X4,
  /** ARGB888_Tile4x4_A<em>X</em>. */
  DEC_OUT_FRM_ARGB888_TILED4X4,
  /** RGB565_Tile4x4_A<em>X</em>. */
  DEC_OUT_FRM_RGB565_TILED4X4,
  /** ARGB565_Tile4x4_A<em>X</em>. */
  DEC_OUT_FRM_ARGB565_TILED4X4,

  /* <div class="smalllist"> */
  /* DECNano*/
  /** DECNano mode 0 compressed
   * \n YUV422YUYV8b_Tile2x4_A<em>X</em>. */
  DEC_OUT_FRM_DECNANO_MODE0_YUV422_YUYV_TILED2X4,
  /** DECNano mode 0 compressed
   * \n YUV422UYVY8b_Tile2x4_A<em>X</em>. */
  DEC_OUT_FRM_DECNANO_MODE0_YUV422_UYVY_TILED2X4,
  /** DECNano mode 0 compressed
   * \n RGB888_Tile4x4_A<em>X</em>. */
  DEC_OUT_FRM_DECNANO_MODE0_RGB888_TILED4X4,
  /** DECNano mode 0 compressed
   * \n ARGB888_Tile4x4_A<em>X</em>. */
  DEC_OUT_FRM_DECNANO_MODE0_ARGB888_TILED4X4,
  /** DECNano mode 0 compressed
   * \n RGB565_Tile4x4_A<em>X</em>. */
  DEC_OUT_FRM_DECNANO_MODE0_RGB565_TILED4X4,
  /** DECNano mode 0 compressed
   * \n ARGB8565_Tile4x4_A<em>X</em>. */
  DEC_OUT_FRM_DECNANO_MODE0_ARGB8565_TILED4X4,

  /* YUV440 */
  /** (Deprecated) YUV440 8-bit raster-scan. */
  DEC_OUT_FRM_YUV440,

  /* YUV411 */
  /** (Deprecated) YUV411SP8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV411SP,

  /* YUV444 */
  /** (Deprecated) YUV444SP8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV444SP,
  /** (Deprecated) YUV444SP10b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV444SP_PACK10,
  /** (Deprecated) YUV444SP12b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV444SP_PACK12,
  /** 8-bit YUV444 tiled 8x8 output format \n
   * for non-reference frames.*/
  DEC_OUT_FRM_YUV444TILE8x8,
  /** 10-bit YUV444 tiled 8x8 output format \n
   * for non-reference frames.*/
  DEC_OUT_FRM_YUV444TILE8x8_PACK10,
  /** 12-bit YUV444 tiled 8x8 output format \n
   * for non-reference frames.*/
  DEC_OUT_FRM_YUV444TILE8x8_PACK12,
  /** (Deprecated) YUV444SP10bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV444SP_P010,
  /** (Deprecated) YUV444SP10bWL_Raster_A<em>X</em> I010. */
  DEC_OUT_FRM_YUV444SP_I010,
  /** (Deprecated) YUV444SP10bWL_Raster_A<em>X</em> L010. */
  DEC_OUT_FRM_YUV444SP_L010,
  /** (Deprecated) YUV444SP12bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV444SP_P012,
  /** (Deprecated) YUV444SP12bWL_Raster_A<em>X</em> I012. */
  DEC_OUT_FRM_YUV444SP_I012,
  /** (Deprecated) YUV444SP10bDWL_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV444SP_1010,
  /** YUV444P8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV444P,
  /** (Deprecated) YUV444P10b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV444P_PACK10,
  /** (Deprecated) YUV444P12b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV444P_PACK12,
  /** YUV444P10bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV444P_P010,
  /** (Deprecated) YUV444P10bWL_Raster_A<em>X</em> I010. */
  DEC_OUT_FRM_YUV444P_I010,
  /** (Deprecated) YUV444P10bWL_Raster_A<em>X</em> L010. */
  DEC_OUT_FRM_YUV444P_L010,
  /** (Deprecated) YUV444P12bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_YUV444P_P012,
  /** (Deprecated) YUV444P12bWL_Raster_A<em>X</em> I012. */
  DEC_OUT_FRM_YUV444P_I012,

  /* NV21 */
  /** YVU420SP8b_YuvSp4x4_A<em>X</em>. */
  DEC_OUT_FRM_NV21TILE,
  /** YVU420SP10b_YuvSp4x4_A<em>X</em>. */
  DEC_OUT_FRM_NV21TILE_PACK10,
  /** YVU420SP10bWH_YuvSp4x4_A<em>X</em>. */
  DEC_OUT_FRM_NV21TILE_P010,
  /** YVU420SP10bWL_YuvSp4x4_A<em>X</em> I010. */
  DEC_OUT_FRM_NV21TILE_I010,
  /** YVU420SP10bWL_YuvSp4x4_A<em>X</em> L010. */
  DEC_OUT_FRM_NV21TILE_L010,
  /** YVU420SP12bWH_YuvSp4x4_A<em>X</em>. */
  DEC_OUT_FRM_NV21TILE_P012,
  /** YVU420SP12bWL_YuvSp4x4_A<em>X</em> I012. */
  DEC_OUT_FRM_NV21TILE_I012,
  /** YVU420SP8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_NV21SP,
  /** YVU420SP10b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_NV21SP_PACK10,
  /** YVU420SP12b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_NV21SP_PACK12,
  /** YVU420SP10bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_NV21SP_P010,
  /** YVU420SP10bWL_Raster_A<em>X</em> I010. */
  DEC_OUT_FRM_NV21SP_I010,
  /** YVU420SP10bWL_Raster_A<em>X</em> L010. */
  DEC_OUT_FRM_NV21SP_L010,
  /** (Deprecated) YVU420SP12bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_NV21SP_P012,
  /** (Deprecated) YVU420SP12bWL_Raster_A<em>X</em> I012. */
  DEC_OUT_FRM_NV21SP_I012,
  /** YVU420SP10bDWL_Raster_A<em>X</em>. */
  DEC_OUT_FRM_NV21SP_1010,
  /** YVU420P8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_NV21P,
  /** YVU420P10b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_NV21P_PACK10,
  /** (Deprecated) YVU420P12b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_NV21P_PACK12,
  /** YVU420P10bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_NV21P_P010,
  /** YVU420P10bWL_Raster_A<em>X</em> I010. */
  DEC_OUT_FRM_NV21P_I010,
  /** YVU420P10bWL_Raster_A<em>X</em> L010. */
  DEC_OUT_FRM_NV21P_L010,
  /** YVU420SP12bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_NV21P_P012,
  /** YVU420P12bWL_Raster_A<em>X</em> I012. */
  DEC_OUT_FRM_NV21P_I012,
  /** YVU420P10bDWL_Raster_A<em>X</em>. */
  DEC_OUT_FRM_NV21P_1010,

  /* 422 NV21 */
  /** YVU422SP8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_422_NV21SP,
  /** YVU422SP10b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_422_NV21SP_PACK10,
  /** YVU422SP12b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_422_NV21SP_PACK12,
  /** YVU422SP10bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_422_NV21SP_P010,
  /** YVU422SP10bWL_Raster_A<em>X</em> I010. */
  DEC_OUT_FRM_422_NV21SP_I010,
  /** YVU422SP10bWL_Raster_A<em>X</em> L010. */
  DEC_OUT_FRM_422_NV21SP_L010,
  /** (Deprecated) YVU422SP12bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_422_NV21SP_P012,
  /** (Deprecated) YVU422SP12bWL_Raster_A<em>X</em> I012. */
  DEC_OUT_FRM_422_NV21SP_I012,
  /** YVU422SP10bDWL_Raster_A<em>X</em>. */
  DEC_OUT_FRM_422_NV21SP_1010,
  /** YVU422P8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_422_NV21P,
  /** YVU422P10b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_422_NV21P_PACK10,
  /** (Deprecated) YVU422P12b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_422_NV21P_PACK12,
  /** YVU422P10bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_422_NV21P_P010,
  /** (Deprecated) YVU422P10bWL_Raster_A<em>X</em> I010. */
  DEC_OUT_FRM_422_NV21P_I010,
  /** (Deprecated) YVU422P10bWL_Raster_A<em>X</em> L010. */
  DEC_OUT_FRM_422_NV21P_L010,
  /** (Deprecated) YVU422P12bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_422_NV21P_P012,
  /** (Deprecated) YVU422P12bWL_Raster_A<em>X</em> I012. */
  DEC_OUT_FRM_422_NV21P_I012,

  /* 444 NV21 */
  /** YVU444SP8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_444_NV21SP,
  /** YVU444SP10b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_444_NV21SP_PACK10,
  /** (Deprecated) YVU444SP12b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_444_NV21SP_PACK12,
  /** YVU444SP10bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_444_NV21SP_P010,
  /** (Deprecated) YVU422SP10bWL_Raster_A<em>X</em> I010. */
  DEC_OUT_FRM_444_NV21SP_I010,
  /** (Deprecated) YVU422SP10bWL_Raster_A<em>X</em> L010. */
  DEC_OUT_FRM_444_NV21SP_L010,
  /** (Deprecated) YVU444SP12bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_444_NV21SP_P012,
  /** (Deprecated) YVU444SP12bWL_Raster_A<em>X</em> I012. */
  DEC_OUT_FRM_444_NV21SP_I012,
  /** YVU444P8b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_444_NV21P,
  /** YVU444P10b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_444_NV21P_PACK10,
  /** YVU444P12b_Raster_A<em>X</em>. */
  DEC_OUT_FRM_444_NV21P_PACK12,
  /** YVU444P10bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_444_NV21P_P010,
  /** YVU444P10bWL_Raster_A<em>X</em> I010. */
  DEC_OUT_FRM_444_NV21P_I010,
  /** YVU444P10bWL_Raster_A<em>X</em> L010. */
  DEC_OUT_FRM_444_NV21P_L010,
  /** YVU444P12bWH_Raster_A<em>X</em>. */
  DEC_OUT_FRM_444_NV21P_P012,
  /** YVU444P12bWL_Raster_A<em>X</em> I012. */
  DEC_OUT_FRM_444_NV21P_I012,

  /* RGB */
  /* sw_ppx_out_rgb_fmt == 0 */
  /** RGB888P_Raster_A<em>X</em>. */
  DEC_OUT_FRM_RGB888_P,
  /** BGR888_Raster_A<em>X</em>. */
  DEC_OUT_FRM_BGR888,
  /* sw_ppx_out_rgb_fmt == 1 */
  /** BGR888P_Raster_A<em>X</em>. */
  DEC_OUT_FRM_BGR888_P,
  /** RGB888_Raster_A<em>X</em>. */
  DEC_OUT_FRM_RGB888,
  /* sw_ppx_out_rgb_fmt == 2 */
  /** RGB16l16l16lP_Raster_A<em>X</em>. */
  DEC_OUT_FRM_R16G16B16_P,
  /** BGR16l16l16l_Raster_A<em>X</em>. */
  DEC_OUT_FRM_B16G16R16,
  /* sw_ppx_out_rgb_fmt == 3 */
  /** BGR16l16l16lP_Raster_A<em>X</em>. */
  DEC_OUT_FRM_B16G16R16_P,
  /** RGB161616_Raster_A<em>X</em>. */
  DEC_OUT_FRM_R16G16B16,
  /* sw_ppx_out_rgb_fmt == 4 */
  /** ABGR8888_Raster_A<em>X</em>. */
  DEC_OUT_FRM_ABGR888,
  /* sw_ppx_out_rgb_fmt == 5 */
  /** ARGB8888_Raster_A<em>X</em>. */
  DEC_OUT_FRM_ARGB888,
  /* sw_ppx_out_rgb_fmt == 6 */
  /** ABGR2101010_Raster_A<em>X</em>. */
  DEC_OUT_FRM_A2B10G10R10,
  /* sw_ppx_out_rgb_fmt == 7 */
  /** ARGB2101010_Raster_A<em>X</em>. */
  DEC_OUT_FRM_A2R10G10B10,
  /* sw_ppx_out_rgb_fmt == 8 */
  /** XBGR8888_Raster_A<em>X</em>. */
  DEC_OUT_FRM_XBGR888,
  /* sw_ppx_out_rgb_fmt == 9 */
  /** XRGB8888_Raster_A<em>X</em>. */
  DEC_OUT_FRM_XRGB888,
  /** BGRA8888_Raster_A<em>X</em>. */
  DEC_OUT_FRM_BGRA888,
  /** RGBA8888_Raster_A<em>X</em>. */
  DEC_OUT_FRM_RGBA888,
  /** BGRA1010102_Raster_A<em>X</em>. */
  DEC_OUT_FRM_B10G10R10A2,
  /** RGBA1010102_Raster_A<em>X</em>. */
  DEC_OUT_FRM_R10G10B10A2,
  /** XRGB2101010_Raster_A<em>X</em>. */
  DEC_OUT_FRM_X2R10G10B10,
  /** XBGR2101010_Raster_A<em>X</em>. */
  DEC_OUT_FRM_X2B10G10R10,
  /** RGB565_Raster_A<em>X</em>. */
  DEC_OUT_FRM_RGB565,
  /** BGR565_Raster_A<em>X</em>. */
  DEC_OUT_FRM_BGR565,
  /** ARGB8565_Raster_A<em>X</em>. */
  DEC_OUT_FRM_ARGB8565,
  /* super_tile */
  /** ABGR8888_Tile64x64_A<em>X</em>. */
  DEC_OUT_FRM_ABGR888_TILED64X64,
  /** ARGB8888_Tile64x64_A<em>X</em>. */
  DEC_OUT_FRM_ARGB888_TILED64X64,
  /** ABGR2101010_Tile64x64_A<em>X</em>. */
  DEC_OUT_FRM_A2B10G10R10_TILED64X64,
  /** ARGB2101010_Tile64x64_A<em>X</em>. */
  DEC_OUT_FRM_A2R10G10B10_TILED64X64,
  /** XBGR8888_Tile64x64_A<em>X</em>. */
  DEC_OUT_FRM_XBGR888_TILED64X64,
  /** XRGB8888_Tile64x64_A<em>X</em>. */
  DEC_OUT_FRM_XRGB888_TILED64X64,
  /** XBGR2101010_Tile64x64_A<em>X</em>. */
  DEC_OUT_FRM_X2B10G10R10_TILED64X64,
  /** XRGB2101010_Tile64x64_A<em>X</em>. */
  DEC_OUT_FRM_X2R10G10B10_TILED64X64,
  /** BGRA8888_Tile64x64_A<em>X</em>. */
  DEC_OUT_FRM_BGRA888_TILED64X64,
  /** RGBA8888_Tile64x64_A<em>X</em>. */
  DEC_OUT_FRM_RGBA888_TILED64X64,
  /** BGRA1010102_Tile64x64_A<em>X</em>. */
  DEC_OUT_FRM_B10G10R10A2_TILED64X64,
  /** RGBA1010102_Tile64x64_A<em>X</em>. */
  DEC_OUT_FRM_R10G10B10A2_TILED64X64,
  /** HIST_A<em>X</em>. */
  DEC_OUT_FRM_HIST
  /* Note: Please update the ParseDecPictureFormat() function in parselogmsg.c synchornously if you modify this enumeration. */
};

/** \brief A function that checks whether the given format is a packed RGB format, in which all
 *  components are packed together in the same plane.
 *  \ingroup common_group */
static inline u32 IS_PIC_PACKED_RGB(enum DecPictureFormat fmt)
{
  return (
    (fmt) == DEC_OUT_FRM_RGB888 || \
    (fmt) == DEC_OUT_FRM_BGR888 || \
    (fmt) == DEC_OUT_FRM_R16G16B16 || \
    (fmt) == DEC_OUT_FRM_B16G16R16 || \
    (fmt) == DEC_OUT_FRM_ARGB888 || \
    (fmt) == DEC_OUT_FRM_ABGR888 || \
    (fmt) == DEC_OUT_FRM_A2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_A2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_X2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_X2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_XRGB888 || \
    (fmt) == DEC_OUT_FRM_XBGR888 || \
    (fmt) == DEC_OUT_FRM_R10G10B10A2 || \
    (fmt) == DEC_OUT_FRM_B10G10R10A2 || \
    (fmt) == DEC_OUT_FRM_RGBA888 || \
    (fmt) == DEC_OUT_FRM_BGRA888 || \
	  (fmt) == DEC_OUT_FRM_RGB565 || \
    (fmt) == DEC_OUT_FRM_BGR565 || \
    (fmt) == DEC_OUT_FRM_ARGB8565 || \
    (fmt) == DEC_OUT_FRM_ABGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_ARGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_A2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_A2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_X2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_X2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_XBGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_XRGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_BGRA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_RGBA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_B10G10R10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_R10G10B10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_BGR888 || \
    (fmt) == DEC_OUT_FRM_DEC400_RGB888 || \
    (fmt) == DEC_OUT_FRM_DEC400_ABGR888 || \
    (fmt) == DEC_OUT_FRM_DEC400_ARGB888 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_DEC400_BGRA888 || \
    (fmt) == DEC_OUT_FRM_DEC400_RGBA888 || \
    (fmt) == DEC_OUT_FRM_DEC400_B10G10R10A2 || \
    (fmt) == DEC_OUT_FRM_DEC400_R10G10B10A2 || \
    (fmt) == DEC_OUT_FRM_DEC400_XBGR888 || \
    (fmt) == DEC_OUT_FRM_DEC400_XRGB888 || \
    (fmt) == DEC_OUT_FRM_DEC400_ABGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_ARGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_XBGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_XRGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_BGRA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_RGBA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_B10G10R10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_R10G10B10A2_TILED64X64);
}
/** \brief A function that checks whether the given format is a packed RGB super tile with dec400 compression format,
 *  in which all components are packed together in the same plane.
 *  \ingroup common_group */
static inline u32 IS_PIC_DEC400_SUPER_TILE(enum DecPictureFormat fmt)
{
  return (
    (fmt) == DEC_OUT_FRM_DEC400_ABGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_ARGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_XBGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_XRGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_BGRA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_RGBA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_B10G10R10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_R10G10B10A2_TILED64X64);
}

/** \brief A function that checks whether the component of each plane is stored in 8x8 tiles.
 *  \ingroup common_group */
static inline u32 IS_PIC_DEC400_TILE8x8(enum DecPictureFormat fmt)
{
  return (
    (fmt) == DEC_OUT_FRM_DEC400_400TILED8x8 || \
    (fmt) == DEC_OUT_FRM_DEC400_400TILED8x8_P010 || \
    (fmt) == DEC_OUT_FRM_DEC400_420TILED8x8 || \
    (fmt) == DEC_OUT_FRM_DEC400_NV21TILED8x8 || \
    (fmt) == DEC_OUT_FRM_DEC400_420TILED8x8_P010 || \
    (fmt) == DEC_OUT_FRM_DEC400_NV21TILED8x8_P010);
}
/** \brief A function that checks whether the given format is a planar RGB format, in which R, G,
 *  and B components are stored in three separate planes.
 *  \ingroup common_group */
static inline u32 IS_PIC_PLANAR_RGB(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_RGB888_P || \
    (fmt) == DEC_OUT_FRM_BGR888_P || \
    (fmt) == DEC_OUT_FRM_R16G16B16_P || \
    (fmt) == DEC_OUT_FRM_B16G16R16_P);
}

/** \brief A function that checks whether the given format is an RGB format, either packed or
 *  planar.
 *  \ingroup common_group */
static inline u32 IS_PIC_RGB(enum DecPictureFormat fmt)
{
  return (
    (fmt) == DEC_OUT_FRM_RGB888 || \
    (fmt) == DEC_OUT_FRM_BGR888 || \
    (fmt) == DEC_OUT_FRM_R16G16B16 || \
    (fmt) == DEC_OUT_FRM_B16G16R16 || \
    (fmt) == DEC_OUT_FRM_ARGB888 || \
    (fmt) == DEC_OUT_FRM_ABGR888 || \
    (fmt) == DEC_OUT_FRM_A2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_A2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_X2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_X2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_XRGB888 || \
    (fmt) == DEC_OUT_FRM_RGB888_P || \
    (fmt) == DEC_OUT_FRM_BGR888_P || \
    (fmt) == DEC_OUT_FRM_R16G16B16_P || \
    (fmt) == DEC_OUT_FRM_B16G16R16_P || \
    (fmt) == DEC_OUT_FRM_XBGR888 || \
    (fmt) == DEC_OUT_FRM_RGBA888 || \
    (fmt) == DEC_OUT_FRM_BGRA888 || \
    (fmt) == DEC_OUT_FRM_R10G10B10A2 || \
    (fmt) == DEC_OUT_FRM_B10G10R10A2 || \
    (fmt) == DEC_OUT_FRM_RGB565 || \
    (fmt) == DEC_OUT_FRM_BGR565 || \
    (fmt) == DEC_OUT_FRM_ARGB8565 || \
    (fmt) == DEC_OUT_FRM_ABGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_ARGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_A2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_A2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_X2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_X2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_XBGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_XRGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_BGRA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_RGBA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_B10G10R10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_R10G10B10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_BGR888 || \
    (fmt) == DEC_OUT_FRM_DEC400_RGB888 || \
    (fmt) == DEC_OUT_FRM_DEC400_ABGR888 || \
    (fmt) == DEC_OUT_FRM_DEC400_ARGB888 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_DEC400_BGRA888 || \
    (fmt) == DEC_OUT_FRM_DEC400_RGBA888 || \
    (fmt) == DEC_OUT_FRM_DEC400_B10G10R10A2 || \
    (fmt) == DEC_OUT_FRM_DEC400_R10G10B10A2 || \
    (fmt) == DEC_OUT_FRM_DEC400_XBGR888 || \
    (fmt) == DEC_OUT_FRM_DEC400_XRGB888 || \
    (fmt) == DEC_OUT_FRM_DEC400_ABGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_ARGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_XBGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_XRGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_BGRA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_RGBA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_B10G10R10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_R10G10B10A2_TILED64X64);
}

/** \brief A function that checks whether the given format includes an alpha component.
 *  \ingroup common_group */
static inline u32 IS_PIC_ALPHA_FORMAT(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_ARGB888 || \
    (fmt) == DEC_OUT_FRM_ABGR888 || \
    (fmt) == DEC_OUT_FRM_A2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_A2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_RGBA888 || \
    (fmt) == DEC_OUT_FRM_BGRA888 || \
    (fmt) == DEC_OUT_FRM_R10G10B10A2 || \
    (fmt) == DEC_OUT_FRM_B10G10R10A2 || \
    (fmt) == DEC_OUT_FRM_ABGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_ARGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_A2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_A2R10G10B10_TILED64X64 ||\
    (fmt) == DEC_OUT_FRM_BGRA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_RGBA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_B10G10R10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_R10G10B10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_ABGR888 || \
    (fmt) == DEC_OUT_FRM_DEC400_ARGB888 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_DEC400_BGRA888 || \
    (fmt) == DEC_OUT_FRM_DEC400_RGBA888 || \
    (fmt) == DEC_OUT_FRM_DEC400_B10G10R10A2 || \
    (fmt) == DEC_OUT_FRM_DEC400_R10G10B10A2 || \
    (fmt) == DEC_OUT_FRM_DEC400_ABGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_ARGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_BGRA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_RGBA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_B10G10R10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_R10G10B10A2_TILED64X64);
}

/** \brief A function that checks whether the given format is an RGB format with an X component.
 *  \ingroup common_group */
static inline u32 IS_PIC_XRGB_FORMAT(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_XRGB888 || \
    (fmt) == DEC_OUT_FRM_XBGR888 || \
    (fmt) == DEC_OUT_FRM_X2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_X2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_XBGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_XRGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_X2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_X2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_XBGR888 || \
    (fmt) == DEC_OUT_FRM_DEC400_XRGB888 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_XBGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_XRGB888_TILED64X64);
}

/** \brief A function that checks whether the given format is a monochroma format, which includes
 *  only a Y component.
 *  \ingroup common_group */
static inline u32 IS_PIC_MONOCHROME(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_MONOCHROME || \
    (fmt) == DEC_OUT_FRM_YUV400TILE || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_P010 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_I010 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_L010 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_P012 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_I012 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE8x8_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE8x8_P010 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE8x8 || \
    (fmt) == DEC_OUT_FRM_YUV400 || \
    (fmt) == DEC_OUT_FRM_YUV400_P010 || \
    (fmt) == DEC_OUT_FRM_YUV400_I010 || \
    (fmt) == DEC_OUT_FRM_YUV400_L010 || \
    (fmt) == DEC_OUT_FRM_YUV400_P012 || \
    (fmt) == DEC_OUT_FRM_YUV400_I012 || \
    (fmt) == DEC_OUT_FRM_YUV400_PACK10 || \
    (fmt) == DEC_OUT_FRM_DEC400_400 || \
    (fmt) == DEC_OUT_FRM_DEC400_400_P010 || \
    (fmt) == DEC_OUT_FRM_DEC400_400TILE || \
    (fmt) == DEC_OUT_FRM_DEC400_400TILED8x8 || \
    (fmt) == DEC_OUT_FRM_DEC400_400TILED8x8_P010 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_128X2 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_128X2_P010);
}

/** \brief A function that checks whether the given format is a YUV420 format, in which the
 *  U and V components have half the height and half the width of the Y component.
 *  \ingroup common_group */
static inline u32 IS_PIC_YCbCr420(enum DecPictureFormat fmt)
{
  return (((fmt) >= DEC_OUT_FRM_NV21SP && \
   (fmt) <= DEC_OUT_FRM_NV21P_1010) || \
   ((fmt) >= DEC_OUT_FRM_YUV420SP && \
   (fmt) <= DEC_OUT_FRM_YUV420P_1010) || \
   (fmt) == DEC_OUT_FRM_PVFBC_TILED16X4_420SP || \
   (fmt) == DEC_OUT_FRM_PVFBC_TILED8X8_420SP || \
   (fmt) == DEC_OUT_FRM_PVFBC_TILED16X4_420SP_P010 || \
   (fmt) == DEC_OUT_FRM_PVFBC_TILED8X8_420SP_P010 || \
   (fmt) == DEC_OUT_FRM_PVFBC_BYPASS_TILED16X4_420SP || \
   (fmt) == DEC_OUT_FRM_PVFBC_BYPASS_TILED8X8_420SP || \
   (fmt) == DEC_OUT_FRM_PVFBC_BYPASS_TILED16X4_420SP_P010 || \
   (fmt) == DEC_OUT_FRM_PVFBC_BYPASS_TILED8X8_420SP_P010);
}

/** \brief A function that checks whether the given format is a YUV422 format, in which the
 *  U and V components have the same height and half the width of the Y component.
 *  \ingroup common_group */
static inline u32 IS_PIC_YCbCr422(enum DecPictureFormat fmt)
{
  return (((fmt) >= DEC_OUT_FRM_YUV422SP && \
     (fmt) <= DEC_OUT_FRM_YUV422_UYVY_TILED2X4) || \
     ((fmt) >= DEC_OUT_FRM_422_NV21SP && \
     (fmt) <= DEC_OUT_FRM_422_NV21P_I012));
}

/** \brief A funciton that checks whether the given format is a YUV444 format, in which the
 *  U and V components have the same height and width of the Y component.
 *  \ingroup common_group */
static inline u32 IS_PIC_YCbCr444(enum DecPictureFormat fmt)
{
  return (((fmt) >= DEC_OUT_FRM_YUV444SP && \
     (fmt) <= DEC_OUT_FRM_YUV444P_I012) || \
     ((fmt) >= DEC_OUT_FRM_444_NV21SP && \
     (fmt) <= DEC_OUT_FRM_444_NV21P_I012));
 }

/** \brief A function that checks whether the given format is a planar YUV format, in which each
 *  component is stored in a separate buffer.
 *  \ingroup common_group */
static inline u32 IS_PIC_PLANAR(enum DecPictureFormat fmt)
{
  //cppcheck-suppress duplicateExpression
  return ((fmt) == DEC_OUT_FRM_PLANAR_420 || \
    (fmt) == DEC_OUT_FRM_YUV420P || \
    (fmt) == DEC_OUT_FRM_YUV420P_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV422P_PACK12 || \
    (fmt) == DEC_OUT_FRM_YUV420P_P010 || \
    (fmt) == DEC_OUT_FRM_YUV420P_I010 || \
    (fmt) == DEC_OUT_FRM_YUV420P_L010 || \
    (fmt) == DEC_OUT_FRM_YUV420P_P012 || \
    (fmt) == DEC_OUT_FRM_YUV420P_I012 || \
    (fmt) == DEC_OUT_FRM_YUV420P_1010 || \
    (fmt) == DEC_OUT_FRM_NV21P || \
    (fmt) == DEC_OUT_FRM_NV21P_PACK10 || \
    (fmt) == DEC_OUT_FRM_NV21P_PACK12 || \
    (fmt) == DEC_OUT_FRM_NV21P_P010 || \
    (fmt) == DEC_OUT_FRM_NV21P_I010 || \
    (fmt) == DEC_OUT_FRM_NV21P_L010 || \
    (fmt) == DEC_OUT_FRM_NV21P_P012 || \
    (fmt) == DEC_OUT_FRM_NV21P_I012 || \
    (fmt) == DEC_OUT_FRM_NV21P_1010 || \
    (fmt) == DEC_OUT_FRM_YUV422P || \
    (fmt) == DEC_OUT_FRM_YUV422P_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV422P_PACK12 || \
    (fmt) == DEC_OUT_FRM_YUV422P_P010 || \
    (fmt) == DEC_OUT_FRM_YUV422P_I010 || \
    (fmt) == DEC_OUT_FRM_YUV422P_L010 || \
    (fmt) == DEC_OUT_FRM_YUV422P_P012 || \
    (fmt) == DEC_OUT_FRM_YUV422P_I012 || \
    (fmt) == DEC_OUT_FRM_422_NV21P || \
    (fmt) == DEC_OUT_FRM_422_NV21P_PACK10 || \
    (fmt) == DEC_OUT_FRM_422_NV21P_PACK12 || \
    (fmt) == DEC_OUT_FRM_422_NV21P_P010 || \
    (fmt) == DEC_OUT_FRM_422_NV21P_I010 || \
    (fmt) == DEC_OUT_FRM_422_NV21P_L010 || \
    (fmt) == DEC_OUT_FRM_422_NV21P_P012 || \
    (fmt) == DEC_OUT_FRM_422_NV21P_I012 || \
    (fmt) == DEC_OUT_FRM_YUV444P || \
    (fmt) == DEC_OUT_FRM_YUV444P_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV444P_PACK12 || \
    (fmt) == DEC_OUT_FRM_YUV444P_P010 || \
    (fmt) == DEC_OUT_FRM_YUV444P_I010 || \
    (fmt) == DEC_OUT_FRM_YUV444P_L010 || \
    (fmt) == DEC_OUT_FRM_YUV444P_P012 || \
    (fmt) == DEC_OUT_FRM_YUV444P_I012 || \
    (fmt) == DEC_OUT_FRM_444_NV21P || \
    (fmt) == DEC_OUT_FRM_444_NV21P_PACK10 || \
    (fmt) == DEC_OUT_FRM_444_NV21P_PACK12 || \
    (fmt) == DEC_OUT_FRM_444_NV21P_P010 || \
    (fmt) == DEC_OUT_FRM_444_NV21P_I010 || \
    (fmt) == DEC_OUT_FRM_444_NV21P_L010 || \
    (fmt) == DEC_OUT_FRM_444_NV21P_P012 || \
    (fmt) == DEC_OUT_FRM_444_NV21P_I012 || \
    (fmt) == DEC_OUT_FRM_DEC400_420P || \
    (fmt) == DEC_OUT_FRM_DEC400_420P_P010 || \
    IS_PIC_PLANAR_RGB(fmt));
}

/** \brief A function that checks whether the given field format is a planar YUV format, in which
 *  each component is stored in a separate buffer.
 *  \ingroup common_group */
static inline u32 IS_PIC_FIELD_PLANAR(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_PLANAR_420 || \
    (fmt) == DEC_OUT_FRM_YUV420P || \
    (fmt) == DEC_OUT_FRM_YUV420P_P010 || \
    (fmt) == DEC_OUT_FRM_YUV420P_I010 );
}

/** \brief A function that checks whether the given format is a 1010 format, in which the three
 *  10-bit components of a pixel are stored in 32 bits with 2 bits pending.
 *  \ingroup common_group */
static inline u32 IS_PIC_1010(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_NV21P_1010 || \
   (fmt) == DEC_OUT_FRM_NV21SP_1010 || \
   (fmt) == DEC_OUT_FRM_YUV420P_1010 || \
   (fmt) == DEC_OUT_FRM_YUV420SP_1010 || \
   (fmt) == DEC_OUT_FRM_YUV422SP_1010 || \
   (fmt) == DEC_OUT_FRM_422_NV21SP_1010);
}

/** \brief A function that checks whether the given format is an NV21 format, in which the U and V
 *  components of a pixel are stored in VU interleaved mode, like V0U0V1U1V2U2...
 *  \ingroup common_group */
static inline u32 IS_PIC_NV21(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_NV21TILE || \
    (fmt) == DEC_OUT_FRM_NV21TILE_PACK10 || \
    (fmt) == DEC_OUT_FRM_NV21TILE_P010 || \
    (fmt) == DEC_OUT_FRM_NV21TILE_I010 || \
    (fmt) == DEC_OUT_FRM_NV21TILE_L010 || \
    (fmt) == DEC_OUT_FRM_NV21TILE_P012 || \
    (fmt) == DEC_OUT_FRM_NV21TILE_I012 || \
    (fmt) == DEC_OUT_FRM_NV21SP || \
    (fmt) == DEC_OUT_FRM_NV21SP_PACK10 || \
    (fmt) == DEC_OUT_FRM_NV21SP_P010 || \
    (fmt) == DEC_OUT_FRM_NV21SP_I010 || \
    (fmt) == DEC_OUT_FRM_NV21SP_L010 || \
    (fmt) == DEC_OUT_FRM_NV21SP_P012 || \
    (fmt) == DEC_OUT_FRM_NV21SP_I012 || \
    (fmt) == DEC_OUT_FRM_NV21P || \
    (fmt) == DEC_OUT_FRM_NV21P_PACK10 || \
    (fmt) == DEC_OUT_FRM_NV21P_P010 || \
    (fmt) == DEC_OUT_FRM_NV21P_I010 || \
    (fmt) == DEC_OUT_FRM_NV21P_P012 || \
    (fmt) == DEC_OUT_FRM_NV21P_I012 || \
    (fmt) == DEC_OUT_FRM_NV21P_L010 );
}

/** \brief A function that checks whether the given format is a semi-planar format, in which the U
 *  and V components of a pixel are stored in interleaved mode, like V0U0V1U1V2U2... or U0V0U1V1U2V2...
 *  \ingroup common_group */
static inline u32 IS_PIC_SEMIPLANAR(enum DecPictureFormat fmt)/*no using*/
{
  return ((fmt) == DEC_OUT_FRM_YUV420SP || \
    (fmt) == DEC_OUT_FRM_YUV420SP_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV420SP_P010 || \
    (fmt) == DEC_OUT_FRM_YUV420SP_I010 || \
    (fmt) == DEC_OUT_FRM_YUV420SP_L010 || \
    (fmt) == DEC_OUT_FRM_YUV420SP_P012 || \
    (fmt) == DEC_OUT_FRM_YUV420SP_I012 || \
    (fmt) == DEC_OUT_FRM_NV21SP || \
    (fmt) == DEC_OUT_FRM_NV21SP_PACK10 || \
    (fmt) == DEC_OUT_FRM_NV21SP_P010 || \
    (fmt) == DEC_OUT_FRM_NV21SP_I010 || \
    (fmt) == DEC_OUT_FRM_NV21SP_L010 || \
    (fmt) == DEC_OUT_FRM_NV21SP_P012 || \
    (fmt) == DEC_OUT_FRM_NV21SP_I012 || \
    (fmt) == DEC_OUT_FRM_RASTER_SCAN);
}

/** \brief A function that checks whether the component of each pixel is of 8 bits.
 *  \ingroup common_group */
static inline u32 IS_PIC_8BIT(enum DecPictureFormat fmt)
{
  //cppcheck-suppress duplicateExpression
  return ((fmt) == DEC_OUT_FRM_YUV420TILE || \
    (fmt) == DEC_OUT_FRM_YUV420SP || \
    (fmt) == DEC_OUT_FRM_YUV420P || \
    (fmt) == DEC_OUT_FRM_YUV422SP || \
    (fmt) == DEC_OUT_FRM_YUV422P || \
    (fmt) == DEC_OUT_FRM_YUV444SP || \
    (fmt) == DEC_OUT_FRM_YUV444P || \
    (fmt) == DEC_OUT_FRM_YUV400TILE || \
    (fmt) == DEC_OUT_FRM_YUV400 || \
    (fmt) == DEC_OUT_FRM_NV21TILE || \
    (fmt) == DEC_OUT_FRM_NV21SP || \
    (fmt) == DEC_OUT_FRM_NV21P || \
    (fmt) == DEC_OUT_FRM_422_NV21SP || \
    (fmt) == DEC_OUT_FRM_422_NV21P || \
    (fmt) == DEC_OUT_FRM_444_NV21SP || \
    (fmt) == DEC_OUT_FRM_444_NV21P || \
    (fmt) == DEC_OUT_FRM_RGB888_P || \
    (fmt) == DEC_OUT_FRM_BGR888_P || \
    (fmt) == DEC_OUT_FRM_RGB888 || \
    (fmt) == DEC_OUT_FRM_BGR888 || \
    (fmt) == DEC_OUT_FRM_ARGB888 || \
    (fmt) == DEC_OUT_FRM_ABGR888 || \
    (fmt) == DEC_OUT_FRM_XRGB888 || \
    (fmt) == DEC_OUT_FRM_XBGR888 || \
    (fmt) == DEC_OUT_FRM_RGBA888 || \
    (fmt) == DEC_OUT_FRM_BGRA888 || \
    (fmt) == DEC_OUT_FRM_ABGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_ARGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_XBGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_XRGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_BGRA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_BGRA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_TILED_4X4 || \
    (fmt) == DEC_OUT_FRM_TILED_8X8 || \
    (fmt) == DEC_OUT_FRM_TILED_16X16 || \
    (fmt) == DEC_OUT_FRM_TILED_16X16_COMP || \
    (fmt) == DEC_OUT_FRM_TILED_32X8_COMP || \
    (fmt) == DEC_OUT_FRM_TILED_64X64 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_128X2 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE8x8 || \
    (fmt) == DEC_OUT_FRM_YUV420RECONTILE8X8 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE8x8 || \
    (fmt) == DEC_OUT_FRM_YUV422TILE8x8 || \
    (fmt) == DEC_OUT_FRM_YUV444TILE8x8 || \
    (fmt) == DEC_OUT_FRM_YUV422_YUYV || \
    (fmt) == DEC_OUT_FRM_YUV422_UYVY || \
    (fmt) == DEC_OUT_FRM_YUV422_YVYU || \
    (fmt) == DEC_OUT_FRM_YUV422_VYUY || \
    (fmt) == DEC_OUT_FRM_PVFBC_TILED16X4_420SP || \
    (fmt) == DEC_OUT_FRM_PVFBC_TILED8X8_420SP || \
    (fmt) == DEC_OUT_FRM_PVFBC_BYPASS_TILED16X4_420SP || \
    (fmt) == DEC_OUT_FRM_PVFBC_BYPASS_TILED8X8_420SP || \
    (fmt) == DEC_OUT_FRM_YUV422_YUYV_TILED8X8 || \
    (fmt) == DEC_OUT_FRM_YUV422_UYVY_TILED8X8 || \
    (fmt) == DEC_OUT_FRM_YUV422_YUYV_TILED2X4 || \
    (fmt) == DEC_OUT_FRM_YUV422_UYVY_TILED2X4 || \
    (fmt) == DEC_OUT_FRM_RGB888_TILED4X4|| \
    (fmt) == DEC_OUT_FRM_ARGB888_TILED4X4 || \
    (fmt) == DEC_OUT_FRM_RGB565_TILED4X4|| \
    (fmt) == DEC_OUT_FRM_ARGB565_TILED4X4);
}

/** \brief A function that checks whether the component of each pixel is of 10 bits.
 *  \ingroup common_group */
static inline u32 IS_PIC_10BIT(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_YUV420SP_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV420P_PACK10 || \
    (fmt) == DEC_OUT_FRM_NV21SP_PACK10 || \
    (fmt) == DEC_OUT_FRM_NV21P_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV422SP_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV422P_PACK10 || \
    (fmt) == DEC_OUT_FRM_422_NV21SP_PACK10 || \
    (fmt) == DEC_OUT_FRM_422_NV21P_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV444SP_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV444P_PACK10 || \
    (fmt) == DEC_OUT_FRM_444_NV21SP_PACK10 || \
    (fmt) == DEC_OUT_FRM_444_NV21P_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_PACK10 || \
    (fmt) == DEC_OUT_FRM_NV21TILE_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE8x8_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV400_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE8x8_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE8x8_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV422TILE8x8_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV444TILE8x8_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV420RECONTILE8X8_PACK10 || \
    (fmt) == DEC_OUT_FRM_TILED_64X64_PACK10);
}

/** \brief A function that checks whether the component of each pixel is of 12 bits.
 *  \ingroup common_group */
static inline u32 IS_PIC_12BIT(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_YUV420SP_PACK12 || \
    (fmt) == DEC_OUT_FRM_YUV400_PACK12 || \
    (fmt) == DEC_OUT_FRM_YUV420P_PACK12 || \
    (fmt) == DEC_OUT_FRM_NV21SP_PACK12 || \
    (fmt) == DEC_OUT_FRM_NV21P_PACK12  || \
    (fmt) == DEC_OUT_FRM_YUV422SP_PACK12 || \
    (fmt) == DEC_OUT_FRM_YUV422P_PACK12 || \
    (fmt) == DEC_OUT_FRM_422_NV21SP_PACK12 || \
    (fmt) == DEC_OUT_FRM_422_NV21P_PACK12  || \
    (fmt) == DEC_OUT_FRM_YUV444SP_PACK12 || \
    (fmt) == DEC_OUT_FRM_YUV444P_PACK12 || \
    (fmt) == DEC_OUT_FRM_444_NV21SP_PACK12 || \
    (fmt) == DEC_OUT_FRM_444_NV21P_PACK12 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE8x8_PACK12);
}

/** \brief A function that checks whether the component of each pixel is of 16 bits.
 *  \ingroup common_group */
static inline u32 IS_PIC_16BIT(enum DecPictureFormat fmt)
{
  return (
    (fmt) == DEC_OUT_FRM_YUV420TILE_P010 || \
    (fmt) == DEC_OUT_FRM_YUV420SP_P010 || \
    (fmt) == DEC_OUT_FRM_YUV420P_P010 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE_P012 || \
    (fmt) == DEC_OUT_FRM_YUV420SP_P012 || \
    (fmt) == DEC_OUT_FRM_YUV420P_P012 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE_I010 || \
    (fmt) == DEC_OUT_FRM_YUV420SP_I010 || \
    (fmt) == DEC_OUT_FRM_YUV420P_I010 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE_I012 || \
    (fmt) == DEC_OUT_FRM_YUV420SP_I012 || \
    (fmt) == DEC_OUT_FRM_YUV420P_I012 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE_L010 || \
    (fmt) == DEC_OUT_FRM_YUV420SP_L010 || \
    (fmt) == DEC_OUT_FRM_YUV420P_L010 || \
    (fmt) == DEC_OUT_FRM_YUV422SP_P010 || \
    (fmt) == DEC_OUT_FRM_YUV422P_P010 || \
    (fmt) == DEC_OUT_FRM_YUV422SP_P012 || \
    (fmt) == DEC_OUT_FRM_YUV422P_P012 || \
    (fmt) == DEC_OUT_FRM_YUV422SP_I010 || \
    (fmt) == DEC_OUT_FRM_YUV422P_I010 || \
    (fmt) == DEC_OUT_FRM_YUV422SP_I012 || \
    (fmt) == DEC_OUT_FRM_YUV422P_I012 || \
    (fmt) == DEC_OUT_FRM_YUV422SP_L010 || \
    (fmt) == DEC_OUT_FRM_YUV422P_L010 || \
    (fmt) == DEC_OUT_FRM_YUV444SP_P010 || \
    (fmt) == DEC_OUT_FRM_YUV444P_P010 || \
    (fmt) == DEC_OUT_FRM_YUV444SP_P012 || \
    (fmt) == DEC_OUT_FRM_YUV444P_P012 || \
    (fmt) == DEC_OUT_FRM_YUV444SP_I010 || \
    (fmt) == DEC_OUT_FRM_YUV444P_I010 || \
    (fmt) == DEC_OUT_FRM_YUV444SP_I012 || \
    (fmt) == DEC_OUT_FRM_YUV444P_I012 || \
    (fmt) == DEC_OUT_FRM_YUV444SP_L010 || \
    (fmt) == DEC_OUT_FRM_YUV444P_L010 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_P010 || \
    (fmt) == DEC_OUT_FRM_YUV400_P010 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_P012 || \
    (fmt) == DEC_OUT_FRM_YUV400_P012 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_I010 || \
    (fmt) == DEC_OUT_FRM_YUV400_I010 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_I012 || \
    (fmt) == DEC_OUT_FRM_YUV400_I012 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_L010 || \
    (fmt) == DEC_OUT_FRM_YUV400_L010 || \
    (fmt) == DEC_OUT_FRM_NV21TILE_P010 || \
    (fmt) == DEC_OUT_FRM_NV21SP_P010 || \
    (fmt) == DEC_OUT_FRM_NV21P_P010 || \
    (fmt) == DEC_OUT_FRM_NV21TILE_P012 || \
    (fmt) == DEC_OUT_FRM_NV21SP_P012 || \
    (fmt) == DEC_OUT_FRM_NV21P_P012 || \
    (fmt) == DEC_OUT_FRM_NV21TILE_I010 || \
    (fmt) == DEC_OUT_FRM_NV21SP_I010 || \
    (fmt) == DEC_OUT_FRM_NV21P_I010 || \
    (fmt) == DEC_OUT_FRM_NV21SP_I012 || \
    (fmt) == DEC_OUT_FRM_NV21P_I012 || \
    (fmt) == DEC_OUT_FRM_NV21TILE_L010 || \
    (fmt) == DEC_OUT_FRM_NV21SP_L010 || \
    (fmt) == DEC_OUT_FRM_NV21P_L010 || \
    (fmt) == DEC_OUT_FRM_422_NV21SP_P010 || \
    (fmt) == DEC_OUT_FRM_422_NV21P_P010 || \
    (fmt) == DEC_OUT_FRM_422_NV21SP_P012 || \
    (fmt) == DEC_OUT_FRM_422_NV21P_P012 || \
    (fmt) == DEC_OUT_FRM_422_NV21SP_I010 || \
    (fmt) == DEC_OUT_FRM_422_NV21P_I010 || \
    (fmt) == DEC_OUT_FRM_422_NV21SP_I012 || \
    (fmt) == DEC_OUT_FRM_422_NV21P_I012 || \
    (fmt) == DEC_OUT_FRM_422_NV21SP_L010 || \
    (fmt) == DEC_OUT_FRM_422_NV21P_L010 || \
    (fmt) == DEC_OUT_FRM_444_NV21SP_P010 || \
    (fmt) == DEC_OUT_FRM_444_NV21P_P010 || \
    (fmt) == DEC_OUT_FRM_444_NV21SP_P012 || \
    (fmt) == DEC_OUT_FRM_444_NV21P_P012 || \
    (fmt) == DEC_OUT_FRM_444_NV21SP_I010 || \
    (fmt) == DEC_OUT_FRM_444_NV21P_I010 || \
    (fmt) == DEC_OUT_FRM_444_NV21SP_I012 || \
    (fmt) == DEC_OUT_FRM_444_NV21P_I012 || \
    (fmt) == DEC_OUT_FRM_444_NV21SP_L010 || \
    (fmt) == DEC_OUT_FRM_444_NV21P_L010 || \
    (fmt) == DEC_OUT_FRM_R16G16B16_P || \
    (fmt) == DEC_OUT_FRM_B16G16R16_P || \
    (fmt) == DEC_OUT_FRM_R16G16B16 || \
    (fmt) == DEC_OUT_FRM_B16G16R16 || \
    (fmt) == DEC_OUT_FRM_RGB565 || \
    (fmt) == DEC_OUT_FRM_BGR565 || \
    (fmt) == DEC_OUT_FRM_YUV400_WITH_ALPHA || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_128X2_P010 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE_128X2_P010 || \
    (fmt) == DEC_OUT_FRM_PVFBC_TILED16X4_420SP_P010 || \
    (fmt) == DEC_OUT_FRM_PVFBC_TILED8X8_420SP_P010  || \
    (fmt) == DEC_OUT_FRM_PVFBC_BYPASS_TILED16X4_420SP_P010 || \
    (fmt) == DEC_OUT_FRM_PVFBC_BYPASS_TILED8X8_420SP_P010);
}

/** \brief A function that checks whether the three RGB components of each pixel are stored in
 *  packed 24 bits.
 *  \ingroup common_group */
static inline u32 IS_PIC_24BIT(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_RGB888 || \
    (fmt) == DEC_OUT_FRM_BGR888 || \
    (fmt) == DEC_OUT_FRM_ARGB8565 || \
    (fmt) == DEC_OUT_FRM_DEC400_BGR888 || \
    (fmt) == DEC_OUT_FRM_DEC400_RGB888);
}

/** \brief A function that checks whether the three RGB components with an extra alpha or X
 *  component of each pixel are stored in packed 32 bits.
 *  \ingroup common_group */
static inline u32 IS_PIC_32BIT(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_ARGB888 || \
    (fmt) == DEC_OUT_FRM_ABGR888 || \
    (fmt) == DEC_OUT_FRM_A2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_A2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_X2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_X2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_XRGB888 || \
    (fmt) == DEC_OUT_FRM_XBGR888 || \
    (fmt) == DEC_OUT_FRM_RGBA888 || \
    (fmt) == DEC_OUT_FRM_BGRA888 || \
    (fmt) == DEC_OUT_FRM_R10G10B10A2 || \
    (fmt) == DEC_OUT_FRM_B10G10R10A2 || \
    (fmt) == DEC_OUT_FRM_ABGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_ARGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_A2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_A2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_X2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_X2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_XBGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_XRGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_BGRA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_RGBA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_B10G10R10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_R10G10B10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_ABGR888 || \
    (fmt) == DEC_OUT_FRM_DEC400_ARGB888 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_DEC400_BGRA888 || \
    (fmt) == DEC_OUT_FRM_DEC400_RGBA888 || \
    (fmt) == DEC_OUT_FRM_DEC400_B10G10R10A2 || \
    (fmt) == DEC_OUT_FRM_DEC400_R10G10B10A2 || \
    (fmt) == DEC_OUT_FRM_DEC400_XBGR888 || \
    (fmt) == DEC_OUT_FRM_DEC400_XRGB888 || \
    (fmt) == DEC_OUT_FRM_DEC400_ABGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_ARGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_XBGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_XRGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_BGRA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_RGBA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_B10G10R10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_R10G10B10A2_TILED64X64);
}

/** \brief A function that checks whether the three RGB components of each pixel are stored in
 *  packed 48 bits.
 *  \ingroup common_group */
static inline u32 IS_PIC_48BIT(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_R16G16B16 || \
    (fmt) == DEC_OUT_FRM_B16G16R16);
}

/** \brief A function that checks whether the component of each plane is stored in tiles.
 *  \ingroup common_group */
static inline u32 IS_PIC_TILE(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_TILED_4X4 || \
  (fmt) == DEC_OUT_FRM_TILED_8X4 || \
  (fmt) == DEC_OUT_FRM_YUV420TILE || \
  (fmt) == DEC_OUT_FRM_YUV420TILE_P010 || \
  (fmt) == DEC_OUT_FRM_YUV420TILE_I010 || \
  (fmt) == DEC_OUT_FRM_YUV420TILE_L010 || \
  (fmt) == DEC_OUT_FRM_YUV420TILE_P012 || \
  (fmt) == DEC_OUT_FRM_YUV420TILE_I012 || \
  (fmt) == DEC_OUT_FRM_YUV420TILE_PACK10 || \
  (fmt) == DEC_OUT_FRM_YUV400TILE || \
  (fmt) == DEC_OUT_FRM_YUV400TILE_P010 || \
  (fmt) == DEC_OUT_FRM_YUV400TILE_I010 || \
  (fmt) == DEC_OUT_FRM_YUV400TILE_L010 || \
  (fmt) == DEC_OUT_FRM_YUV400TILE_P012 || \
  (fmt) == DEC_OUT_FRM_YUV400TILE_I012 || \
  (fmt) == DEC_OUT_FRM_YUV400TILE_PACK10 || \
  (fmt) == DEC_OUT_FRM_NV21TILE || \
  (fmt) == DEC_OUT_FRM_NV21TILE_PACK10 || \
  (fmt) == DEC_OUT_FRM_NV21TILE_P010 || \
  (fmt) == DEC_OUT_FRM_NV21TILE_I010 || \
  (fmt) == DEC_OUT_FRM_NV21TILE_L010 || \
  (fmt) == DEC_OUT_FRM_NV21TILE_P012 || \
  (fmt) == DEC_OUT_FRM_NV21TILE_I012 || \
  (fmt) == DEC_OUT_FRM_DEC400_400TILE || \
  (fmt) == DEC_OUT_FRM_DEC400_420TILE || \
  (fmt) == DEC_OUT_FRM_YUV400TILE8x8_PACK10 || \
  (fmt) == DEC_OUT_FRM_YUV400TILE8x8_P010 || \
  (fmt) == DEC_OUT_FRM_YUV400TILE8x8 || \
  (fmt) == DEC_OUT_FRM_YUV420TILE8x8 || \
  (fmt) == DEC_OUT_FRM_YUV420RECONTILE8X8 || \
  (fmt) == DEC_OUT_FRM_YUV420RECONTILE8X8_P010 || \
  (fmt) == DEC_OUT_FRM_YUV420RECONTILE8X8_PACK10 || \
  (fmt) == DEC_OUT_FRM_DEC400_400TILED8x8 || \
  (fmt) == DEC_OUT_FRM_DEC400_400TILED8x8_P010 || \
  (fmt) == DEC_OUT_FRM_DEC400_420TILED8x8 || \
  (fmt) == DEC_OUT_FRM_DEC400_420TILED8x8_P010 || \
  (fmt) == DEC_OUT_FRM_DEC400_NV21TILED8x8 || \
  (fmt) == DEC_OUT_FRM_DEC400_NV21TILED8x8_P010 || \
  (fmt) == DEC_OUT_FRM_DEC400_ABGR888_TILED64X64 || \
  (fmt) == DEC_OUT_FRM_DEC400_ARGB888_TILED64X64 || \
  (fmt) == DEC_OUT_FRM_DEC400_A2B10G10R10_TILED64X64 || \
  (fmt) == DEC_OUT_FRM_DEC400_A2R10G10B10_TILED64X64 || \
  (fmt) == DEC_OUT_FRM_DEC400_X2B10G10R10_TILED64X64 || \
  (fmt) == DEC_OUT_FRM_DEC400_X2R10G10B10_TILED64X64 || \
  (fmt) == DEC_OUT_FRM_DEC400_XBGR888_TILED64X64 || \
  (fmt) == DEC_OUT_FRM_DEC400_XRGB888_TILED64X64 || \
  (fmt) == DEC_OUT_FRM_DEC400_BGRA888_TILED64X64 || \
  (fmt) == DEC_OUT_FRM_DEC400_RGBA888_TILED64X64 || \
  (fmt) == DEC_OUT_FRM_DEC400_B10G10R10A2_TILED64X64 || \
  (fmt) == DEC_OUT_FRM_DEC400_R10G10B10A2_TILED64X64);
}

/** \brief A function that checks whether the component of each plane is stored in an RFC
 *  compressed format.
 *  \ingroup common_group */
static inline u32 IS_PIC_RFC(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_RFC);
}

/** \brief A function that checks whether the component of each plane is stored in a DEC400
 *  compressed format.
 *  \ingroup common_group */
static inline u32 IS_PIC_DEC400(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_DEC400_420SP || \
    (fmt) == DEC_OUT_FRM_DEC400_420SP_P010 || \
    (fmt) == DEC_OUT_FRM_DEC400_NV21SP || \
    (fmt) == DEC_OUT_FRM_DEC400_NV21SP_P010 || \
    (fmt) == DEC_OUT_FRM_DEC400_400 || \
    (fmt) == DEC_OUT_FRM_DEC400_400_P010 || \
    (fmt) == DEC_OUT_FRM_DEC400_420P || \
    (fmt) == DEC_OUT_FRM_DEC400_420P_P010 || \
    (fmt) == DEC_OUT_FRM_DEC400_400TILE || \
    (fmt) == DEC_OUT_FRM_DEC400_420TILE || \
    (fmt) == DEC_OUT_FRM_DEC400_400TILED8x8 || \
    (fmt) == DEC_OUT_FRM_DEC400_400TILED8x8_P010 || \
    (fmt) == DEC_OUT_FRM_DEC400_420TILED8x8 || \
    (fmt) == DEC_OUT_FRM_DEC400_420TILED8x8_P010 || \
    (fmt) == DEC_OUT_FRM_DEC400_NV21TILED8x8 || \
    (fmt) == DEC_OUT_FRM_DEC400_NV21TILED8x8_P010 || \
    (fmt) == DEC_OUT_FRM_DEC400_BGR888 || \
    (fmt) == DEC_OUT_FRM_DEC400_RGB888 || \
    (fmt) == DEC_OUT_FRM_DEC400_ABGR888 || \
    (fmt) == DEC_OUT_FRM_DEC400_ARGB888 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2B10G10R10 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2R10G10B10 || \
    (fmt) == DEC_OUT_FRM_DEC400_BGRA888 || \
    (fmt) == DEC_OUT_FRM_DEC400_RGBA888 || \
    (fmt) == DEC_OUT_FRM_DEC400_B10G10R10A2 || \
    (fmt) == DEC_OUT_FRM_DEC400_R10G10B10A2 || \
    (fmt) == DEC_OUT_FRM_DEC400_XBGR888 || \
    (fmt) == DEC_OUT_FRM_DEC400_XRGB888 || \
    (fmt) == DEC_OUT_FRM_DEC400_ABGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_ARGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_XBGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_XRGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_BGRA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_RGBA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_B10G10R10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_R10G10B10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_YUV422_YUYV || \
    (fmt) == DEC_OUT_FRM_DEC400_YUV422_UYVY);
}

/** \brief A function that checks whether the component of each plane is stored in a DECNano
 *  compressed format.
 *  \ingroup common_group */
static inline u32 IS_PIC_DECNANO(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_DECNANO_MODE0_YUV422_YUYV_TILED2X4 || \
      (fmt) == DEC_OUT_FRM_DECNANO_MODE0_YUV422_UYVY_TILED2X4 || \
      (fmt) == DEC_OUT_FRM_DECNANO_MODE0_RGB888_TILED4X4 || \
      (fmt) == DEC_OUT_FRM_DECNANO_MODE0_ARGB888_TILED4X4 || \
      (fmt) == DEC_OUT_FRM_DECNANO_MODE0_RGB565_TILED4X4 || \
      (fmt) == DEC_OUT_FRM_DECNANO_MODE0_ARGB8565_TILED4X4);
}

/** \brief A function that checks whether the component of each plane is stored in a PVFBC
 *  compressed format.
 *  \ingroup common_group */
static inline u32 IS_PIC_PVFBC(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_PVFBC_TILED16X4_420SP || \
    (fmt) == DEC_OUT_FRM_PVFBC_TILED8X8_420SP || \
    (fmt) == DEC_OUT_FRM_PVFBC_TILED16X4_420SP_P010 || \
    (fmt) == DEC_OUT_FRM_PVFBC_TILED8X8_420SP_P010);
}

static inline u32 IS_PIC_PVFBC_BYPASS(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_PVFBC_BYPASS_TILED16X4_420SP || \
    (fmt) == DEC_OUT_FRM_PVFBC_BYPASS_TILED8X8_420SP || \
    (fmt) == DEC_OUT_FRM_PVFBC_BYPASS_TILED16X4_420SP_P010 || \
    (fmt) == DEC_OUT_FRM_PVFBC_BYPASS_TILED8X8_420SP_P010);
}

/** \brief A function that checks whether the component of each plane is stored in 4x4 tiles.
 *  \ingroup common_group */
static inline u32 IS_PIC_TILED4x4(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_TILED_4X4 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE || \
    (fmt) == DEC_OUT_FRM_YUV420TILE_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE_P010 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE_I010 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE_L010 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE_P012 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE_I012 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_P010 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_I010 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_L010 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_P012 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE_I012 || \
    (fmt) == DEC_OUT_FRM_NV21TILE || \
    (fmt) == DEC_OUT_FRM_NV21TILE_PACK10 || \
    (fmt) == DEC_OUT_FRM_NV21TILE_P012 || \
    (fmt) == DEC_OUT_FRM_NV21TILE_I010 || \
    (fmt) == DEC_OUT_FRM_NV21TILE_L010 || \
    (fmt) == DEC_OUT_FRM_NV21TILE_I012 || \
    (fmt) == DEC_OUT_FRM_NV21TILE_P010 || \
    (fmt) == DEC_OUT_FRM_RGB888_TILED4X4 || \
    (fmt) == DEC_OUT_FRM_ARGB888_TILED4X4 || \
    (fmt) == DEC_OUT_FRM_RGB565_TILED4X4 || \
    (fmt) == DEC_OUT_FRM_ARGB565_TILED4X4);
}

/** \brief A function that checks whether the component of each plane is stored in RGB packed with 4x4 tiles.
 *  \ingroup common_group */
static inline u32 IS_PIC_RGBPACKED_TILED4x4(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_RGB888_TILED4X4|| \
    (fmt) == DEC_OUT_FRM_DECNANO_MODE0_RGB888_TILED4X4);
}

/** \brief A function that checks whether the component of each plane is stored in ARGB packed with 4x4 tiles.
 *  \ingroup common_group */
static inline u32 IS_PIC_ARGBPACKED_TILED4x4(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_ARGB888_TILED4X4|| \
    (fmt) == DEC_OUT_FRM_ARGB565_TILED4X4 || \
    (fmt) == DEC_OUT_FRM_DECNANO_MODE0_ARGB888_TILED4X4);
}

/** \brief A function that checks whether the component of each plane in the refernce buffer is
 *  stored in 8x8 tiles.
 *  \ingroup common_group */
static inline u32 IS_PIC_REF_TILED8x8(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_YUV420TILE8x8 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE8x8_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE8x8_PACK12 || \
    (fmt) == DEC_OUT_FRM_YUV420RECONTILE8X8 || \
    (fmt) == DEC_OUT_FRM_YUV420RECONTILE8X8_P010 || \
    (fmt) == DEC_OUT_FRM_YUV420RECONTILE8X8_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE8x8 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE8x8_P010 || \
    (fmt) == DEC_OUT_FRM_YUV422TILE8x8_PACK10 || \
	  (fmt) == DEC_OUT_FRM_YUV422TILE8x8 || \
    (fmt) == DEC_OUT_FRM_YUV444TILE8x8 || \
    (fmt) == DEC_OUT_FRM_YUV444TILE8x8_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE8x8_PACK10);
}

/** \brief A function that checks whether the component of each plane is stored in 8x8 tiles.
 *  \ingroup common_group */
static inline u32 IS_PIC_TILED8x8(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_TILED_8X8 || \
    (fmt) == DEC_OUT_FRM_TILED_8X8_P010 ||	\
    (fmt) == DEC_OUT_FRM_DEC400_400TILED8x8 || \
    (fmt) == DEC_OUT_FRM_DEC400_400TILED8x8_P010 || \
    (fmt) == DEC_OUT_FRM_DEC400_420TILED8x8 || \
    (fmt) == DEC_OUT_FRM_DEC400_420TILED8x8_P010 || \
    (fmt) == DEC_OUT_FRM_DEC400_NV21TILED8x8 || \
    (fmt) == DEC_OUT_FRM_DEC400_NV21TILED8x8_P010 || \
    (fmt) == DEC_OUT_FRM_YUV420RECONTILE8X8 || \
    (fmt) == DEC_OUT_FRM_YUV420RECONTILE8X8_P010 || \
    (fmt) == DEC_OUT_FRM_YUV420RECONTILE8X8_PACK10 || \
    (fmt) == DEC_OUT_FRM_YUV400TILE8x8_P010 || \
    (fmt) == DEC_OUT_FRM_YUV420TILE8x8);
}

/** \brief A function that checks whether the component of each plane is stored in 16x16 tiles.
 *  \ingroup common_group */
static inline u32 IS_PIC_TILED16x16(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_TILED_16X16 || (fmt) == DEC_OUT_FRM_TILED_16X16_P010);
}

/** \brief A function that checks whether the component of each plane is stored in third-party
 *  compressed 16x16 tiles.
 *  \ingroup common_group */
static inline u32 IS_PIC_TILED16x16_COMP(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_TILED_16X16_COMP || (fmt) == DEC_OUT_FRM_TILED_16X16_P010_COMP);
}

/** \brief A function that checks whether the component of each plane is stored in third-party
 *  compressed 32x8 tiles.
 * \ingroup common_group */
static inline u32 IS_PIC_TILED32x8_COMP(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_TILED_32X8_COMP || (fmt) == DEC_OUT_FRM_TILED_32X8_P010_COMP);
}

/** \brief A function that checks whether the three components are stored in a packed YUV422
 *  format, either in YUYV or UYVY mode, or in a DEC400 compressed format.
 *  \ingroup common_group */
static inline u32 IS_PIC_422PACKED(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_YUV422_YUYV || \
    (fmt) == DEC_OUT_FRM_YUV422_UYVY || \
    (fmt) == DEC_OUT_FRM_YUV422_YVYU || \
    (fmt) == DEC_OUT_FRM_YUV422_VYUY || \
    (fmt) == DEC_OUT_FRM_YUV422_YUYV_TILED2X4 || \
    (fmt) == DEC_OUT_FRM_YUV422_UYVY_TILED2X4 || \
    (fmt) == DEC_OUT_FRM_YUV422_YUYV_TILED8X8 || \
    (fmt) == DEC_OUT_FRM_YUV422_UYVY_TILED8X8 || \
    (fmt) == DEC_OUT_FRM_DEC400_YUV422_YUYV || \
    (fmt) == DEC_OUT_FRM_DEC400_YUV422_UYVY);
}

/** \brief A function that checks whether the three components are stored in a packed YUV422 with tile2x4
 *  format, either in YUYV or UYVY mode.
 *  \ingroup common_group */
static inline u32 IS_PIC_422PACKED_TILED2X4(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_YUV422_YUYV_TILED2X4 || \
    (fmt) == DEC_OUT_FRM_YUV422_UYVY_TILED2X4);
}

/** \brief A function that checks whether the three components are stored in a packed YUV422 with tile8x8
 *  format, either in YUYV or UYVY mode.
 *  \ingroup common_group */
static inline u32 IS_PIC_422PACKED_TILED8x8(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_YUV422_YUYV_TILED8X8 || \
    (fmt) == DEC_OUT_FRM_YUV422_UYVY_TILED8X8);
}

/** \brief A function that checks whether the components of each plane are stored in 128x2 tiles.
 *  \ingroup common_group */
static inline u32 IS_PIC_TILED128x2(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_YUV400TILE_128X2 || \
          (fmt) == DEC_OUT_FRM_YUV420TILE_128X2 || \
          (fmt) == DEC_OUT_FRM_YUV400TILE_128X2_P010 || \
          (fmt) == DEC_OUT_FRM_YUV420TILE_128X2_P010);
}

/** \brief A function that checks whether the components of each plane are stored in 64x64 tiles.
 *  \ingroup common_group */
static inline u32 IS_PIC_TILED64x64(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_TILED_64X64 || \
    (fmt) == DEC_OUT_FRM_TILED_64X64_PACK10);
}

/** \brief A function that checks whether the three RGB components are packed and stored in 64x64
 *  tiles.
 *  \ingroup common_group */
static inline u32 IS_PIC_RGB_TILED64x64(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_ABGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_ARGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_A2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_A2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_X2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_X2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_XBGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_XRGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_BGRA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_RGBA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_B10G10R10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_R10G10B10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_ABGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_ARGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_A2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2B10G10R10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_X2R10G10B10_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_XBGR888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_XRGB888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_BGRA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_RGBA888_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_B10G10R10A2_TILED64X64 || \
    (fmt) == DEC_OUT_FRM_DEC400_R10G10B10A2_TILED64X64);
}

/** \brief A function that checks whether the components are stored in the HIST format.
 *  \ingroup common_group */
static inline u32 IS_PIC_HIST(enum DecPictureFormat fmt)
{
  return ((fmt) == DEC_OUT_FRM_HIST);
}

/** \brief A function that checks whether each component of a standalone input buffer is stored in
 *  the 16-bit P010 format.
 *  \ingroup common_group */
static inline u32 IS_PP_IN_P010(enum DecPictureFormat fmt)
{
  return ((fmt == PP_IN_400_P010) || \
  	(fmt == PP_IN_NV12_P010) || \
  	(fmt == PP_IN_NV21_P010) || \
  	(fmt == PP_IN_420P_P010) || \
  	(fmt == PP_IN_422_NV16_P010) || \
  	(fmt == PP_IN_422_NV61_P010) || \
  	(fmt == PP_IN_444P_P010) || \
  	(fmt == PP_IN_TILED4x4_P010));
}

/** \brief A function that checks whether each component of a standalone input buffer is stored in
 *  a planar YUV format.
 *  \ingroup common_group */
static inline u32 IS_PP_IN_YUV_PLANAR(enum DecPictureFormat fmt)
{
  return ((fmt == PP_IN_420P_8BIT) || \
  	(fmt == PP_IN_420P_P010) || \
  	(fmt == PP_IN_444P_8BIT) || \
  	(fmt == PP_IN_444P_P010));
}

/** \brief A function that checks whether each component of a standalone input buffer is stored in
 *  4x4 tiles.
 *  \ingroup common_group */
static inline u32 IS_PP_IN_TILED4x4(enum DecPictureFormat fmt)
{
  return ((fmt == PP_IN_TILED4x4) || (fmt == PP_IN_TILED4x4_P010));
}

/** \brief A function that checks whether data in a standalone input buffer is stored in 4x4 tiled
 *  packed RGB format.
 *  \ingroup common_group */
static inline u32 IS_PP_IN_RGB(enum DecPictureFormat fmt)
{
  return ((fmt == PP_IN_RGB888) || \
	   (fmt == PP_IN_BGR888) || \
	   (fmt == PP_IN_ARGB888) || \
	   (fmt == PP_IN_ABGR888) || \
	   (fmt == PP_IN_RGBA888) || \
	   (fmt == PP_IN_BGRA888) || \
	   (fmt == PP_IN_XRGB888) || \
	   (fmt == PP_IN_XBGR888) || \
	   (fmt == PP_IN_A2R10G10B10) || \
	   (fmt == PP_IN_A2B10G10R10) || \
	   (fmt == PP_IN_X2R10G10B10) || \
	   (fmt == PP_IN_X2B10G10R10) || \
	   (fmt == PP_IN_R10G10B10A2) || \
	   (fmt == PP_IN_B10G10R10A2) || \
	   (fmt == PP_IN_RGB888_P) || \
	   (fmt == PP_IN_BGR888_P) || \
	   (fmt == PP_IN_R16G16B16_P) || \
	   (fmt == PP_IN_B16G16R16_P));
}

/** \brief A function that checks whether data in a standalone input buffer is stored in a packed
 *  ARGB or XRGB format.
 *  \ingroup common_group */
static inline u32 IS_PP_IN_ALPHA_X_FROMAT_RGB(enum DecPictureFormat fmt)
{
  return ((fmt == PP_IN_ARGB888) || \
    (fmt == PP_IN_ABGR888) || \
    (fmt == PP_IN_XRGB888) || \
    (fmt == PP_IN_XBGR888) || \
    (fmt == PP_IN_RGBA888) || \
    (fmt == PP_IN_BGRA888) || \
    (fmt == PP_IN_A2R10G10B10) || \
    (fmt == PP_IN_A2B10G10R10) || \
    (fmt == PP_IN_X2R10G10B10) || \
    (fmt == PP_IN_X2B10G10R10) || \
    (fmt == PP_IN_R10G10B10A2) || \
    (fmt == PP_IN_B10G10R10A2));
}

/** \brief A function that checks whether data in a standalone input buffer is stored in a planar
 *  RGB format.
 *  \ingroup common_group */
static inline u32 IS_PP_IN_RGB_PLANAR(enum DecPictureFormat fmt)
{
  return ((fmt == PP_IN_RGB888_P) || \
	   (fmt == PP_IN_BGR888_P) || \
	   (fmt == PP_IN_R16G16B16_P) || \
	   (fmt == PP_IN_B16G16R16_P));
}

/** \brief A function that checks whether each component of a standalone input buffer is of 8
 *  valid bits.
 *  \ingroup common_group */
static inline u32 IS_PP_IN_8BIT(enum DecPictureFormat fmt)
{
  return ((fmt == PP_IN_400_8BIT) || \
     (fmt == PP_IN_NV12_8BIT) || \
     (fmt == PP_IN_NV21_8BIT) || \
     (fmt == PP_IN_420P_8BIT) || \
     (fmt == PP_IN_422_NV16_8BIT) || \
     (fmt == PP_IN_422_NV61_8BIT) || \
     (fmt == PP_IN_422_YUYV_8BIT) || \
     (fmt == PP_IN_422_YVYU_8BIT) || \
     (fmt == PP_IN_422_UYVY_8BIT) || \
     (fmt == PP_IN_422_VYUY_8BIT) || \
     (fmt == PP_IN_444P_8BIT) || \
     (fmt == PP_IN_TILED4x4) || \
     (fmt == PP_IN_RGB888) || \
     (fmt == PP_IN_BGR888) || \
     (fmt == PP_IN_ARGB888) || \
     (fmt == PP_IN_ABGR888) || \
     (fmt == PP_IN_XRGB888) || \
     (fmt == PP_IN_XBGR888) || \
     (fmt == PP_IN_RGBA888) || \
     (fmt == PP_IN_BGRA888) || \
     (fmt == PP_IN_RGB888_P) || \
     (fmt == PP_IN_RECON_TILED8x8));
}

/** \brief A function that checks whether each component of a standalone input buffer is of 10
 *  valid bits.
 *  \ingroup common_group */
static inline u32 IS_PP_IN_10BIT(enum DecPictureFormat fmt)
{
  return ((fmt == PP_IN_400_P010) || \
	   (fmt == PP_IN_NV12_P010) || \
	   (fmt == PP_IN_NV21_P010) || \
	   (fmt == PP_IN_420P_P010) || \
	   (fmt == PP_IN_422_NV16_P010) || \
	   (fmt == PP_IN_422_NV61_P010) || \
	   (fmt == PP_IN_444P_P010) || \
	   (fmt == PP_IN_TILED4x4_P010) || \
	   (fmt == PP_IN_A2R10G10B10) || \
	   (fmt == PP_IN_A2B10G10R10) || \
	   (fmt == PP_IN_X2R10G10B10) || \
	   (fmt == PP_IN_X2B10G10R10) || \
	   (fmt == PP_IN_R10G10B10A2) || \
	   (fmt == PP_IN_B10G10R10A2) || \
	   (fmt == PP_IN_R16G16B16_P) || \
	   (fmt == PP_IN_B16G16R16_P) || \
     (fmt == PP_IN_RECON_TILED8x8_PACK10));
}

/** \brief A function that checks whether data in a standalone input buffer is stored in a packed
 *  YUV422 format.
 *  \ingroup common_group */
static inline u32 IS_PP_IN_422PACKED(enum DecPictureFormat fmt)
{
  return ((fmt == PP_IN_422_YUYV_8BIT) || \
	   (fmt == PP_IN_422_YVYU_8BIT) || \
	   (fmt == PP_IN_422_UYVY_8BIT) || \
	   (fmt == PP_IN_422_VYUY_8BIT));
}

/** \brief A function that checks whether data in a standalone input buffer is stored in a
 *  semi-planar YUV format.
 *  \ingroup common_group */
static inline u32 IS_PP_IN_YUV_SEMIPLANAR(enum DecPictureFormat fmt)
{
  return ((fmt == PP_IN_NV12_8BIT) || \
	   (fmt == PP_IN_NV21_8BIT) || \
	   (fmt == PP_IN_NV12_P010) || \
	   (fmt == PP_IN_NV21_P010) || \
	   (fmt == PP_IN_422_NV16_8BIT) || \
	   (fmt == PP_IN_422_NV16_P010) || \
	   (fmt == PP_IN_422_NV61_8BIT) || \
	   (fmt == PP_IN_422_NV61_P010));
}

/** \brief A function that checks whether data in a standalone input buffer is stored in a YUV420
 *  format.
 *  \ingroup common_group */
static inline u32 IS_PP_IN_YUV420(enum DecPictureFormat fmt)
{
  return ((fmt == PP_IN_NV12_8BIT) || \
	   (fmt == PP_IN_NV21_8BIT) || \
	   (fmt == PP_IN_NV12_P010) || \
	   (fmt == PP_IN_NV21_P010) || \
	   (fmt == PP_IN_420P_8BIT) || \
     (fmt == PP_IN_420P_P010) || \
     (fmt == PP_IN_RECON_TILED8x8) || \
     (fmt == PP_IN_RECON_TILED8x8_PACK10));
}

/** \brief A function that checks whether data in a standalone input buffer is stored in a YUV422
 *  format.
 *  \ingroup common_group */
static inline u32 IS_PP_IN_YUV422(enum DecPictureFormat fmt)
{
  return ((fmt == PP_IN_422_NV16_8BIT) || \
	   (fmt == PP_IN_422_NV16_P010) || \
	   (fmt == PP_IN_422_NV61_8BIT) || \
	   (fmt == PP_IN_422_NV61_P010) || \
	   (fmt == PP_IN_422_YUYV_8BIT) || \
	   (fmt == PP_IN_422_YVYU_8BIT) || \
	   (fmt == PP_IN_422_UYVY_8BIT) || \
	   (fmt == PP_IN_422_VYUY_8BIT));
}

/** \brief A function that checks whether data in a standalone input buffer is stored in a YUV444
 *  format.
 *  \ingroup common_group */
static inline u32 IS_PP_IN_YUV444(enum DecPictureFormat fmt)
{
  return ((fmt == PP_IN_444P_8BIT) || (fmt == PP_IN_444P_P010));
}

/** \brief A function that checks whether data in a standalone input buffer is RFC compressed.
 *  \ingroup common_group */
static inline u32 IS_PP_IN_RFC(enum DecPictureFormat fmt)
{
   return (fmt == PP_IN_RFC);
}

/**@}*/

/**
 *  \latexonly
 *  \large
 *  \endlatexonly
 *
 * Defines picture coding types.
 *  \ingroup common_group */
enum DecPicCodingType {
  /** Intra coded picture. */
  DEC_PIC_TYPE_I           = 0,
  /** P-picture, which is a picture coded using intra or inter prediction with one motion
   *  vector and one reference index to predict the sample values of each block. */
  DEC_PIC_TYPE_P           = 1,
  /** B-picture, which is a picture coded using intra or inter prediction with up to two motion
   *  vectors and two reference indices to predict the sample values of each block. */
  DEC_PIC_TYPE_B           = 2,
  /** D-picture in MPEG-2. */
  DEC_PIC_TYPE_D           = 3,
  /** Forced intra coded picture in RealVideo. */
  DEC_PIC_TYPE_FI          = 4,
  /** BI-picture in VC-1. A BI-picture is a B-picture where all the macroblocks are intra-coded. */
  DEC_PIC_TYPE_BI          = 5
};

/** Defines error handling modes.
 *  \ingroup common_group */
enum DecErrorHandling {
  /** Clarity-first mode, which outputs only correct pictures.
   *  \n Correct pictures refer to pictures with no stream errors in decoding. All referenced
   *  pictures are correct pictures. */
  DEC_EC_FRAME_NO_ERROR = 1,
  /** Fluency-first mode, which decodes as many pictures as possible and outputs all pictures
   *  including erroneous pictures. */
  DEC_EC_FRAME_IGNORE_ERROR = 2,
  /** Fluency-clarity balanced mode, which outputs only good pictures.
   *  \n Good pictures refer to both correct pictures and erroneous pictures with the error ratio
   *  less than <tt>DecInitConfig.error_ratio</tt>.
   *  \n All good frames are used as reference and output. Erroneous frames with the error ratio
   *  exceeding \c DecInitConfig.error_ratio are replaced by the nearest good frame in decoding. */
  DEC_EC_FRAME_TOLERANT_ERROR = 3
};

/** Defines stride alignments.
 *  \ingroup common_group */
typedef enum {
  /** 1-byte alignment. */
  DEC_ALIGN_1B = 0,
  /** 8-byte alignment. */
  DEC_ALIGN_8B = 3,
  /** 16-byte alignment. */
  DEC_ALIGN_16B,
  /** 32-byte alignment. */
  DEC_ALIGN_32B,
  /** 64-byte alignment. */
  DEC_ALIGN_64B,
  /** 128-byte alignment. */
  DEC_ALIGN_128B,
  /** 256-byte alignment. */
  DEC_ALIGN_256B,
  /** 512-byte alignment. */
  DEC_ALIGN_512B,
  /** 1024-byte alignment. */
  DEC_ALIGN_1024B,
  /** 2048-byte alignment. */
  DEC_ALIGN_2048B,
} DecPicAlignment;

/** \brief Contains delogo filter configurations.
 *  \ingroup common_group */
typedef struct _DelogoConfig {
  /** \brief Whether to enable the delogo filter.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 enabled;
  /** \brief The X coordinate. */
  u32 x;
  /** \brief The Y coordinate. */
  u32 y;
  /** \brief The width. */
  u32 w;
  /** \brief The height. */
  u32 h;
  /** \brief The delogo filter level. */
  u32 show;
  /** \brief The delogo filter mode. */
  enum DelogoMode mode;
  /** \brief The Y value. */
  u32 Y;
  /** \brief The U value. */
  u32 U;
  /** \brief The V value. */
  u32 V;
} DelogoConfig;

/** \brief Contains offset value of each slice in the data buffer when low-latency output is enabled.
 * \ingroup common_group */
struct DecSliceTextInfo {
/** \brief The number of textual packets to be output when a slice is decoded. */
  u32 slice_txt_cnt;
/**
 * \ingroup common_group
 */
  struct {
    /** \brief <small> The number of lines to output for a decoded video slice. <small>*/
    u32 lines;
    /** \brief <small> The base address of the slice text buffer. <small> */
    addr_t text_base;
    /** \brief <small> Four pieces of 32-bit data values to be written to slice text buffer. <small> */
    u32 text[4];
    /** \brief <small> The offsets for four text entries within the slice text buffer. <small> */
    u32 offset[4];
  } slice_params[4];
};


/** \brief Contains configurations of a cropping region for post-processing.
 *  \ingroup common_group */
struct CropParams {
  /** \brief Whether to enable cropping of the region.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 enabled;
  /** \brief Whether the configurations of the region are user specified.
   *  \n Valid values:
   *  \n - <tt>0</tt>: The configurations are not user specified.
   *  \n - <tt>1</tt>: The configurations are user specified. */
  u32 set_by_user;
  /** \brief The start X coordinate of the region. */
  u32 x;
  /** \brief The start Y coordinate of the region. */
  u32 y;
  /** \brief The width of the region. */
  u32 width;
  /** \brief The height of the region. */
  u32 height;
};

/** \brief Contains normalization configurations of a post-processing channel.
 *  \ingroup common_group */
struct NormParams {
  /** \brief Whether to enable normalization.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u8 enable;
  /** \brief The normalization algorithm.
   *  \n Valid values:
   *  \n - <tt>0</tt>: The common z-score normalization algorithm.
   *  \n - <tt>1</tt>: The z-score normalization with a scaling factor. */
  u32 filter;
  /** \brief The normalized output data precision.
   *  \n Valid values:
   *  \n - <tt>0</tt>: float32.
   *  \n - <tt>1</tt>: float16.
   *  \n - <tt>2</tt>: int16.
   *  \n - <tt>3</tt>: int8.
   *  \n - <tt>4</tt>: uint8. */
  u8 fmat;
  /** \brief The mean value for the R channel. */
  u32 mean_r;
  /** \brief The mean value for the G channel. */
  u32 mean_g;
  /** \brief The mean value for the B channel. */
  u32 mean_b;
  /** \brief The standard deviation for the R channel. */
  u32 std_r;
  /** \brief The standard deviation for the G channel. */
  u32 std_g;
  /** \brief The standard deviation for the B channel. */
  u32 std_b;
  /** \brief The quantization step. */
  u32 quant_step;
};

/** \brief Contains configurations of a post-processing channel.
 *  \ingroup common_group */
typedef struct _PpUnitConfig {
  /** \brief Whether to enable the post-processing channel.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 enabled;
  /** \brief Whether to enable 4x4 tiled output.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 tiled_e;
  /** \brief Whether to enable RGB output.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 rgb;
  /** \brief Whether to enable planar RGB output.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 rgb_planar;
  /** \brief Whether to output the chroma components in VU order or UV order.
   *  \n Valid values:
   *  \n - <tt>0</tt>: UV order.
   *  \n - <tt>1</tt>: VU order. */
  u32 cr_first;
  /** \brief Whether to enable the hardware shaper feature.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 shaper_enabled;
  /** \brief Whether to disallow the shaper to pad at the frame right edge.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Allow.
   *  \n - <tt>1</tt>: Disallow. */
  u32 shaper_no_pad;
  /** \brief Whether to enable the shaper to pad at the tile right edge.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 shaper_tile_edge_pad;
  /** \brief The alignment for DEC400 compression.
   *  \n Valid values: <tt> \ref DEC_ALIGN_32B</tt> and <tt> \ref DEC_ALIGN_64B</tt>. */
  DecPicAlignment dec400_align;
  /** \brief Whether to enable DEC400 compression conformance test. */
  u8 dec400_conformance_test_en;
  /** \brief Whether to enable planar output.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 planar;
   /** \brief Whether to forcibly use planar output.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Do not forcibly use planar output. \n The output format depends on the project configurations.
   *  \n - <tt>1</tt>: Forcibly use planar output.
   *  \n This field is used only for design verification. */
  u32 planar_f;
  /** \brief The alignment of the post-processing output. */
  DecPicAlignment align;
  /** \brief The height alignment of the post-processing output. */
  DecPicAlignment align_h;
  /** \brief The width alignment of the post-processing output. */
  u32 align_pixel_w;
  /** \brief The stride for the Y plane.
   *  \n If this field is set to a non-zero value, the software checks the validity of the value.
   *  If the value is valid, the software uses the value as the stride.
   *  \n If this field is set to <tt>0</tt>, the software uses the stride that is automatically
   *  calculated. */
  u32 ystride;
  /** \brief The stride for the UV plane.
   *  \n If this field is set to a non-zero value, the software checks the validity of the value.
   *  If the value is valid, the software uses the value as the stride.
   *  \n If this field is set to <tt>0</tt>, the software uses the stride that is automatically
   *  calculated. */
  u32 cstride;
  /** \brief The user-defined stride. */
  u32 stride_set_by_user;

  /** \brief The configurations of region 0 for cropping before scaling. */
  struct CropParams crop;
  /** \brief The cropping region ID. */
  u32 crop_id;
 /**
 * \ingroup common_group
 */
  struct {
    /** \brief <small> Whether to enable cropping after scaling.</small> */
    u32 enabled;
    /** \brief <small> The start X coordinate of the cropping region. </small> */
    u32 x;
    /** \brief <small> The start Y coordinate of the cropping region.</small> */
    u32 y;
    /** \brief <small> The width of the cropping region.</small> */
    u32 width;
    /** \brief <small> The height of the cropping region.</small> */
    u32 height;
  } crop2;
 /**
 * \ingroup common_group
 */
  struct {
    /** \brief <small> Whether to enable scaling.</small> */
    u32 enabled;
    /** \brief <small> Whether the size of the scaled output is based on ratio
     *  configurations.</small> */
    u32 scale_by_ratio;
    /** \brief <small> Value 1, 2, 4, or 8: Downscale to 1/ratio_x the width. Value 0: Downscale to specific width.*/
    u32 ratio_x;
    /** \brief <small> Value 1, 2, 4, or 8: Downscale to 1/ratio_y the height. Value 0: Downscale to specific height.*/
    u32 ratio_y;
    /** \brief <small> The width of the scaled output.</small> */
    u32 width;
    /** \brief <small> The height of the scaled output.</small> */
    u32 height;
  } scale;
  /** \brief Whether to output data in monochrome (Y-only) format.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Do not output data in a monochrome format.
   *  \n - <tt>1</tt>: Output data in a monochrome format. */
  u32 monochrome;
  /** \brief Whether to output data in a YUV P010 format.
   *  \n For details about P010, see <em>Hantro VC9x00D Series Memory Buffer Format
   *  Specifications</em>.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Do not output data in a YUV P010 format.
   *  \n - <tt>1</tt>: Output data in a YUV P010 format. */
  u32 out_p010;
   /** \brief Whether to forcibly convert a YUV420 8-bit format into a YUV420 10-bit format when the conversion feature is not supported.
   *  \n Valid values:
   *  \n - <tt>0</tt>: (Default) Do not forcibly convert.
   *  \n - <tt>1</tt>: Forcibly convert the format. */
  u8 Explicit_Specified_010;
  /** \brief Whether to output data in a YUV 1010 format, where each component is stored using 10 bits.
   *  \n For details about YUV 1010, see <em>Hantro VC9x00D Series Memory Buffer Format Specifications</em>.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Do not output data in a YUV 1010 format.
   *  \n - <tt>1</tt>: Output data in a YUV 1010 format. */
  u32 out_1010;
  /** \brief Whether to output data in a YUV I010 format.
   *  \n For details about I010, see <em>Hantro VC9x00D Series Memory Buffer Format
   *  Specifications</em>.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Do not output data in a YUV I010 format.
   *  \n - <tt>1</tt>: Output data in a YUV I010 format. */
  u32 out_I010;
  /** \brief Whether to output data in a YUV L010 format.
   *  \n For details about L010, see <em>Hantro VC9x00D Series Memory Buffer Format
   *  Specifications</em>.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Do not output data in a YUV L010 format.
   *  \n - <tt>1</tt>: Output data in a YUV L010 format. */
  u32 out_L010;
  /** \brief Whether to output data in a YUV P012 format.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Do not output data in a YUV P012 format.
   *  \n - <tt>1</tt>: Output data in a YUV P012 format. */
  u32 out_p012;
  /** \brief Whether to output data in a YUV I012 format.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Do not output data in a YUV I012 format.
   *  \n - <tt>1</tt>: Output data in a YUV I012 format. */
  u32 out_I012;
  /** \brief (Deprecated). */
  u32 out_be;
  /** \brief Whether to force 8-bit output.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Do not force 8-bit output.
   *  \n - <tt>1</tt>: Force 8-bit output. */
  u32 out_cut_8bits;
  /** \brief Whether to output data in the YUYV format, which is a packed YUV422 format.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Do not output data in the YUYV format.
   *  \n - <tt>1</tt>: Output data in the YUYV format. */
  u32 out_yuyv;
  /** \brief Whether to output data in the UYVY format, which is a packed YUV422 format.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Do not output data in the UYVY format.
   *  \n - <tt>1</tt>: Output data in the UYVY format. */
  u32 out_uyvy;
  /** \brief Whether to use the full or limited pixel value range.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Limited range.
   *  \n - <tt>1</tt>: Full range. */
  u32 video_range;
  /** \brief The RGB output format. */
  u32 rgb_format;
  /** \brief The color space conversion standard.
   *  \n The standard can be <tt> \ref BT601</tt>, <tt> \ref BT601_L</tt>, <tt> \ref BT709</tt>,
   *  <tt> \ref BT709_L</tt>, <tt> \ref BT2020</tt>, and <tt> \ref BT2020_L</tt>. */
  u32 rgb_stan;

  /** \brief The source YUV/RGB range for range mapping.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Limited range.
   *  \n - <tt>1</tt>: Full range. */
  u32 source_range;
  /** \brief Whether \c PpUnitConfig.source_range is valid.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Invalid
   *  \n - <tt>1</tt>: Valid. */
  u32 set_source_range_enable;
  /** \brief The target YUV/RGB range for range mapping.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Limited range.
   *  \n - <tt>1</tt>: Full range. */
  u32 target_range;
  /** \brief Whether \c PpUnitConfig.target_range is valid.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Invalid
   *  \n - <tt>1</tt>: Valid. */
  u32 set_target_range_enable;
  /** \brief Whether to enable dithering.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 dither_enable;
  /** \brief Whether to enable 3D LUT for color remapping.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u8 enable_3dlut;

  /** \brief The alpha value for output ARGB pixels in YUV-to-RGB conversion. */
  u32 rgb_alpha;
  /** \brief The scaling algorithm.
   *  \n Valid values: <tt> \ref VSI_LINEAR</tt>, <tt> \ref LANCZOS</tt>, <tt> \ref NEAREST</tt>,
   *  <tt> \ref BI_LINEAR</tt>, <tt> \ref BICUBIC</tt>, <tt> \ref SPLINE</tt>, <tt> \ref BOX</tt>,
   *  <tt> \ref FAST_LINEAR</tt>, <tt> \ref FAST_BICUBIC</tt>, and <tt> \ref AREA</tt>. */
  u32 pp_filter;
  /** \brief The horizontal window for the Lanczos algorithm. */
  u32 x_filter_param;
  /** \brief The vertical window for the Lanczos algorithm. */
  u32 y_filter_param;
  /** \brief The tiled mode when tiled output is enabled. */
  u32 tile_mode;
  /** \brief Whether to pad with 0 or edge pixel value when YUV420 tiled64x64 output is enabled. */
  u32 yuv_supertile_pad_copy_e;
  /** \brief Whether to enable compression.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 comp_enabled;
  /** \brief The constant luma value 0 for VFBC. */
  u32 pp_pvfbc_lu_const0;
  /** \brief The constant luma value 1 for VFBC. */
  u32 pp_pvfbc_lu_const1;
  /** \brief The constant chroma value 0 for VFBC. */
  u32 pp_pvfbc_ch_const0;
  /** \brief The constant chroma value 1 for VFBC. */
  u32 pp_pvfbc_ch_const1;
  /** \brief The PVFBC tile type. */
  u8  pp_pvfbc_tile_type;
  /** \brief Whether to bypass PVFBC compression for tiled 32x8 or tiled 64x4 output.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u8  pp_pvfbc_bypass;
  /** \brief Whether to enable FBC tiled header mode.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 pp_fbc_tile_enable;
  /** \brief The algorithm used to select the center point for scaling.
   *  \n Valid values: <tt> \ref DOWN_ROUND</tt>, <tt> \ref NO_ROUND</tt>, and
   *  <tt> \ref UP_ROUND</tt>.
   *  \n Default value: <tt> \ref DOWN_ROUND</tt>. */
  u32 src_sel_mode;
  /** \brief Whether to pad a fixed YUV value at picture edges in scaling.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Do not pad a fixed YUV value.
   *  \n - <tt>1</tt>: Pad a fixed YUV value. */
  u32 pad_sel;
  /** \brief The Y component of the fixed YUV value for padding in scaling. */
  u32 pad_Y;
  /** \brief The U component of the fixed YUV value for padding in scaling. */
  u32 pad_U;
  /** \brief The V component of the fixed YUV value for padding in scaling. */
  u32 pad_V;
  /** \brief The YUV output format.
   *  \n Valid values: <tt> \ref PP_YUV400</tt>, <tt> \ref PP_YUV420</tt>,
   *  <tt> \ref PP_YUV422</tt>, and <tt> \ref PP_YUV444</tt>. */
  u32 chroma_format;
  /** \brief The number of lines to trigger the line counter interrupt. */
  u32 lc_stripe;
  /** \brief Whether the slice 0 interrupt output can change the state.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Cannot change.
   *  \n - <tt>1</tt>: Change. */
  u32 lc_int_enabled;
  /** \brief The beginning offset for luma data write in the post-processing output buffer for
   *  HEIF. */
  u32 luma_offset;
  /** \brief The beginning offset for chroma data write in the post-processing output buffer for
   *  HEIF. */
  u32 chroma_offset;
  /** \brief The user-defined offset for writing luma and chroma data into the post-processing output
   *  buffer when HEIF images are decoded. */
  u32 buf_off_set_by_user;
  /** \brief The user defined buffer offset. */
  struct DecSliceTextInfo slice_text;
  /** \brief Whether the filter size changes with the scaling ratio.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Does not change.
   *  \n - <tt>1</tt>: Change. */
  u32 antialias;
  /** \brief Whether to enable global histogram statistics.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 hist_global_e;
  /** \brief Whether to enable local histogram statistics.
   * \n The ROI of this feature is a cropped window that consists of 6 blocks horizontally and 4 blocks vertically.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 hist_local_e;
  /** \brief Whether to enable histogram statistics on Hue, Saturation, and Value (HSV) converted from RGB.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 hist_hsv_e;
  /** \brief Whether to enable statistics on the sum of luma values.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 luma_sum_e;
  /** \brief The number of digits by which the luma sum shifts to the right. */
  u32 luma_sum_shift;
   /** \brief The X coordinates of 5 horizontal blocks in the local histogram statistics ROI.
    * \n The ROI consists of 6 blocks horizontally and 4 blocks vertically, and the X coordinate of the remaining horizontal block is automatically calculated.
   */
  u32 hist_offset_x[5];
   /** \brief The Y coordinates of 3 vertical blocks in the local histogram statistics ROI.
    * \n The ROI consists of 6 blocks horizontally and 4 blocks vertically, and the Y coordinate of the remaining vertical block is automatically calculated.
   */
  u32 hist_offset_y[3];
  /** \brief The normalization configurations. */
  struct NormParams norm;
  /** \brief Whether to disable specific post-processing integrity checks.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Enable.
   *  \n - <tt>1</tt>: Disable. */
  u8  dis_checker;

  /** \brief Whether to enable post-processing output in FLEXA SBI mode.
   *  \n The FLEXA SBI mode only supports post-processing output of YUV420 8-bit NV12, YUV420 8-bit NV21, and YUV400 8-bit color formats for H.264, HEVC, or JPEG decoding. */
  u8  sbi_en;
  /** \brief The stream ID of luma data. */
  u32 sbi_lu_stream_id;
  /** \brief The stream ID of chroma data. */
  u32 sbi_ch_stream_id;
  /** \brief The size of each entry in the luma segment buffer.
   *  \n Unit: Bytes.
   *  \n All entries have the same size. */
  u32 sbi_lu_segmentbuffer_entry_size;
  /** \brief The size of each entry in the chroma segment buffer.
   *  \n Unit: Bytes.
   *  \n All entries have the same size. */
  u32 sbi_ch_segmentbuffer_entry_size;
  /** \brief The number of entries in the luma segment buffer. */
  u32 sbi_lu_segmentbuffer_entry_count;
  /** \brief The number of entries in the chroma segment buffer. */
  u32 sbi_ch_segmentbuffer_entry_count;
  /** \brief The offset to the base address for writing luma data to the SBI producer output buffer.
   *  \n Unit: Bytes.
   *  \n The base address for writing luma data to the SBI producer outptut buffer is defined by the <b>sw_ppX_out_lu_base_lsb</b> register. */
  u32 sbi_lu_segmentbuffer_offset;
  /** \brief The offset to the base address for writing chroma data to the SBI producer output buffer.
   *  \n Unit: Bytes.
   *  \n The base address for writing chroma data to the SBI producer outptut buffer is defined by the <b>sw_ppX_out_ch_base_lsb</b> register. */
  u32 sbi_ch_segmentbuffer_offset;
  /** \brief Whether to enable the segment buffer with coupled luma and chroma data from the SBI producer.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u8  sbi_coupled;
  /** \brief (Mode 1) The bus address for output luma data when the SBI producer is enabled. */
  addr_t sbi_lu_addr;
  /** \brief (Mode 1) The bus address for output chroma data when the SBI producer is enabled. */
  addr_t sbi_ch_addr;
  /** \brief (Mode 2) The bus address for output data when the SBI producer is enabled.*/
  addr_t sbi_base_addr;
  /** \brief The flag that indicates whether to write a new frame into the luma segment buffer at the position indicated by the segment buffer offset.
   * \n Valid values:
   * \n - <tt>0</tt>: Do not write.
   * \n - <tt>1</tt>: Write. */
  u32 sbi_lu_reset_offset;
  /** \brief The flag that indicates whether to write a new frame into the chroma segment buffer at the position indicated by the segment buffer offset.
   * \n Valid values:
   * \n - <tt>0</tt>: Do not write.
   * \n - <tt>1</tt>: Write. */
  u32 sbi_ch_reset_offset;
  /** \brief Whether to enable post-processing output in ring buffer mode.
   * \n The ring buffer mode only supports post-processing output of YUV420 8-bit NV12, YUV420 8-bit NV21, and YUV400 8-bit color formats for H.264 or JPEG decoding.
   * \n Valid values:
   * \n - <tt>0</tt>: Disable.
   * \n - <tt>1</tt>: Enable.*/
  u8  rb_en;
  /** \brief The height of each luma segment entry (in the unit of 16-pixel rows) for ring buffer output. */
  u32 rb_height;
  /** \brief The total number of segment entries in the ring buffer of luma data output from the post-processing pipeline. */
  u32 rb_num;
  /** \brief Whether to enable the ring buffer mode or the line interrupt mode for the post-processing output.
   * \n Valid values:
   * \n - <tt>0</tt>: Enable the ring buffer mode.
   * \n The size of the ring buffer equals the size of a frame.
   * \n - <tt>1</tt>: Enable the line interrupt mode 0.
   * \n In this mode, an interrupt is generated every time a segment entry is ready.
   * \n - <tt>2</tt>: Enable the line interrupt mode 1.
   * \n In this mode, an interrupt is generated only when a frame is ready. */
  u8  rb_mode;
  /** \brief Whether to enable DECNano compression.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u8 decnano_e;
  /** \brief The DECNano compression mode.
   *  \n Valid values:
   *  \n - <tt>0</tt>: No sampling.
   *  \n - <tt>1</tt>: Horizontal sampling.
   *  \n - <tt>2</tt>: Horizontal and vertical sampling.
   *  \n - <tt>3</tt>: High quality mode. */
  u8 decnano_mode;
  /** \brief The LZ77 decoding mode.
   *  \n LZ77 decoding can be enabled only when PNG decoding is enabled.
   *  \n Valid values:
   *  \n - <tt>0</tt>: PNG_MODE.
   *  \n - <tt>1</tt>: LZ77_UNIVERSAL_MODE.
   *  \n - <tt>2</tt>: LZ77_PIC_MODE.
  */
  u8 lz77_mode;
  /** \brief Whether to enable the output format of YUV400 with an alpha value.
   *  \n LZ77 decoding can be enabled only when PNG decoding is enabled.
   *  \n Valid values:
*  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable.
   */
  u8 yuv400_with_alpha;
} PpUnitConfig;

  /** \brief Contains the output buffer information.
   *  \ingroup common_group */
struct DecOutputPictureAddr {
  /** \brief A pointer to the luma picture data. */
  const u32 *luma_vir_addr;
  /** \brief The bus address of the buffer that stores output luma picture data. */
  addr_t luma_bus_addr;
  /** \brief A pointer to the chroma picture data. */
  const u32 *chroma_vir_addr;
  /** \brief The bus address of the buffer that stores the output chroma picture buffer. */
  addr_t chroma_bus_addr;
  /** \brief A pointer to the chroma Cr picture data. */
  const u32 *chroma_cr_vir_addr;
  /** \brief The bus address of the buffer that stores the output chroma Cr picture buffer. */
  addr_t chroma_cr_bus_addr;
};

typedef struct {
  u32 numerator;
  u32 denominator;
} Fraction;

struct PpFilmGrainParams {
  u8 scaling_points_y[14][2];   // Luma scaling points (point value, scaling)
  u8 num_y_points;              // Number of luma scaling points
  u8 scaling_points_cb[10][2];  // Cb scaling points (point value, scaling)
  u8 num_cb_points;             // Number of Cb scaling points
  u8 scaling_points_cr[10][2];  // Cr scaling points (point value, scaling)
  u8 num_cr_points;             // Number of Cr scaling points
  u8 scaling_shift;             // Shift for scaling values
  u8 ar_coeff_lag;              // Lag determining AR coefficient count
  u8 ar_coeffs_y[24];           // AR coefficients for luma
  u8 ar_coeffs_cb[25];          // AR coefficients for Cb
  u8 ar_coeffs_cr[25];          // AR coefficients for Cr
  u8 ar_coeff_shift;            // Shift for AR coefficients
  u8 cb_mult;                   // Cb multiplier from luma grain
  u8 cb_luma_mult;              // Luma multiplier for Cb grain
  u16 cb_offset;                // Offset for Cb grain
  u8 cr_mult;                   // Cr multiplier from luma grain
  u8 cr_luma_mult;              // Luma multiplier for Cr grain
  u16 cr_offset;                // Offset for Cr grain
  u8 overlap_flag;              // Overlap between grain blocks
  u8 clip_to_restricted_range;  // Clip to restricted range
  u8 chroma_scaling_from_luma;  // Derive chroma scaling from luma
  u8 grain_scale_shift;         // Shift for grain scaling
  u16 random_seed;              // Random seed for grain generation
};
#endif /* DECAPICOMMON_H */
