/*------------------------------------------------------------------------------
--       Copyright (c) 2015, VeriSilicon Inc. All rights reserved             --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
-- * Redistributions of source code must retain the above copyright notice,   --
--       this list of conditions and the following disclaimer.                --
-- * Redistributions in binary form must reproduce the above copyright        --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
-- * Neither the names of the copyright holder nor the names of its           --
--       contributors may be used to endorse or promote products derived from --
--       this software without specific prior written permission.             --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

#ifndef __PPU__
#define __PPU__

#include "basetype.h"
#include "dwl.h"
#include "dwlthread.h"
#include "decapicommon.h"
#include "vpufeature.h"
#include "dectypes.h"
#include "input_queue.h"

#define PP_OUT_FMT_YUV420PACKED10 0
#define PP_OUT_FMT_YUV420_P010 1
#define PP_OUT_FMT_YUV420_BIGE 2
#define PP_OUT_FMT_YUV420_8BIT 3
#define PP_OUT_FMT_YUV400 4       /* A.k.a., Monochrome/luma only*/
#define PP_OUT_FMT_YUV400_P010 5
#define PP_OUT_FMT_YUV400_8BIT 6
#define PP_OUT_FMT_IYUVPACKED10_420 7
#define PP_OUT_FMT_IYUV_420_P010 8
#define PP_OUT_FMT_IYUV_420_8BIT 9
#define PP_OUT_FMT_YUV420_10 10 /*YUV420SP p32: 101010*/
#define PP_OUT_FMT_RGB       11
#define PP_OUT_FMT_YUV422_10 12
#define PP_OUT_FMT_YUV422_8BIT 13
#define PP_OUT_FMT_YUV422_P010 14
#define PP_OUT_FMT_YUV422_P012 15
#define PP_OUT_FMT_IYUV_422_8BIT 16
#define PP_OUT_FMT_IYUV_422_P010 17
#define PP_OUT_FMT_IYUV_422_P012 18
#define PP_OUT_FMT_YUV422_YUYV_8BIT 19
#define PP_OUT_FMT_YUV422_UYVY_8BIT 20
#define PP_OUT_FMT_YUV400_P012 21
#define PP_OUT_FMT_YUV420_P012 22
#define PP_OUT_FMT_IYUV_420_P012 23
#define PP_OUT_FMT_IYUV_444_8BIT 24
#define PP_OUT_FMT_IYUV_444_P010 25
#define PP_OUT_FMT_IYUV_444_P012 26
#define PP_OUT_FMT_YUV400_WITH_ALPHA 27
#define PP_OUT_FMT_YUV422_YUYV_P010 28
#define PP_OUT_FMT_YUV422_UYVY_P010 29
#define PP_OUT_FMT_PVFBC_YUV420_8BIT 30
#define PP_OUT_FMT_PVFBC_YUV420_P010 31

#define IS_PP_OUT_PACKED10(fmt) \
  ((fmt) == PP_OUT_FMT_YUV420PACKED10 || (fmt) == PP_OUT_FMT_IYUVPACKED10_420)

#define IS_PP_OUT_MONOCHROME(fmt) \
        (((fmt) >= PP_OUT_FMT_YUV400 && (fmt) <= PP_OUT_FMT_YUV400_8BIT) || \
         ((fmt) ==  PP_OUT_FMT_YUV400_P012))

#define IS_PP_OUT_420(fmt) \
        (((fmt) >= PP_OUT_FMT_YUV420PACKED10 && (fmt) <= PP_OUT_FMT_YUV420_8BIT) || \
         ((fmt) >= PP_OUT_FMT_IYUV_420_P010 && (fmt) <= PP_OUT_FMT_YUV420_10) || \
         (fmt) == PP_OUT_FMT_YUV420_P012 || \
         (fmt) == PP_OUT_FMT_IYUV_420_P012 || \
         (fmt) == PP_OUT_FMT_IYUVPACKED10_420)

#define IS_PP_OUT_PACKED422(fmt) \
        ((fmt) == PP_OUT_FMT_YUV422_YUYV_8BIT || (fmt) == PP_OUT_FMT_YUV422_UYVY_8BIT || \
        (fmt) == PP_OUT_FMT_YUV422_YUYV_P010 || (fmt) == PP_OUT_FMT_YUV422_UYVY_P010)

#define IS_PP_OUT_422(fmt) \
        (((fmt) >= PP_OUT_FMT_YUV422_10 && (fmt) <= PP_OUT_FMT_IYUV_422_P012) || \
         IS_PP_OUT_PACKED422(fmt))

#define IS_PP_OUT_444(fmt) \
        (((fmt) >= PP_OUT_FMT_IYUV_444_8BIT && (fmt) <= PP_OUT_FMT_IYUV_444_P012))




#define PP_OUT_RGB888 0
#define PP_OUT_BGR888 1
#define PP_OUT_R16G16B16 2
#define PP_OUT_B16G16R16 3
#define PP_OUT_ABGR888 4
#define PP_OUT_ARGB888 5
#define PP_OUT_A2B10G10R10 6
#define PP_OUT_A2R10G10B10 7
#define PP_OUT_XBGR888 8
#define PP_OUT_XRGB888 9
#define RGB_PLANAR_DIFF (PP_OUT_XRGB888 + 1 - PP_OUT_RGB888)
#define PP_OUT_RGB888_P 10
#define PP_OUT_BGR888_P 11
#define PP_OUT_R16G16B16_P 12
#define PP_OUT_B16G16R16_P 13
#define PP_OUT_BGRA888 14
#define PP_OUT_RGBA888 15
#define PP_OUT_B10G10R10A2 16
#define PP_OUT_R10G10B10A2 17
#define PP_OUT_X2B10G10R10 18
#define PP_OUT_X2R10G10B10 19
#define PP_OUT_BGR565 20
#define PP_OUT_RGB565 21
#define PP_OUT_ARGB8565 22

#define PP_CHROMA_400 0
#define PP_CHROMA_420 1
#define PP_CHROMA_422 2
#define PP_CHROMA_444 3
#define PP_CHROMA_440 4
#define PP_CHROMA_411 5

#define IS_PACKED_RGB(fmt) \
          ((fmt == PP_OUT_RGB888) || \
           (fmt == PP_OUT_BGR888) || \
           (fmt == PP_OUT_ARGB888) || \
           (fmt == PP_OUT_ABGR888) || \
           (fmt == PP_OUT_A2R10G10B10) || \
           (fmt == PP_OUT_A2B10G10R10) || \
           (fmt == PP_OUT_X2R10G10B10) || \
           (fmt == PP_OUT_X2B10G10R10) || \
           (fmt == PP_OUT_R16G16B16) || \
           (fmt == PP_OUT_B16G16R16) || \
           (fmt == PP_OUT_XRGB888) || \
           (fmt == PP_OUT_XBGR888) || \
           (fmt == PP_OUT_RGBA888) || \
           (fmt == PP_OUT_BGRA888) || \
           (fmt == PP_OUT_R10G10B10A2) || \
           (fmt == PP_OUT_B10G10R10A2) || \
           (fmt == PP_OUT_BGR565) || \
           (fmt == PP_OUT_RGB565) || \
           (fmt == PP_OUT_ARGB8565))

#define IS_PLANAR_RGB(fmt) \
          ((fmt == PP_OUT_RGB888_P) || \
          (fmt == PP_OUT_BGR888_P) || \
          (fmt == PP_OUT_R16G16B16_P) || \
          (fmt == PP_OUT_B16G16R16_P))


#define IS_ALPHA_FORMAT(fmt) \
         ((fmt == PP_OUT_ARGB888) || \
          (fmt == PP_OUT_ABGR888) || \
          (fmt == PP_OUT_A2R10G10B10) || \
          (fmt == PP_OUT_A2B10G10R10) || \
          (fmt == PP_OUT_RGBA888) || \
          (fmt == PP_OUT_BGRA888) || \
          (fmt == PP_OUT_R10G10B10A2) || \
          (fmt == PP_OUT_B10G10R10A2))

#define IS_3PLAN_FORMAT(fmt) \
         ((fmt == PP_OUT_RGB888) || \
          (fmt == PP_OUT_BGR888) || \
          (fmt == PP_OUT_R16G16B16) || \
          (fmt == PP_OUT_B16G16R16) || \
          (fmt == PP_OUT_RGB888_P) || \
          (fmt == PP_OUT_BGR888_P) || \
          (fmt == PP_OUT_R16G16B16_P) || \
          (fmt == PP_OUT_B16G16R16_P))

#define IS_4PLAN_FORMAT(fmt) ((fmt) == PP_OUT_ARGB888 || \
                           (fmt) == PP_OUT_ABGR888 || \
                           (fmt) == PP_OUT_A2R10G10B10 || \
                           (fmt) == PP_OUT_A2B10G10R10 || \
                           (fmt) == PP_OUT_X2R10G10B10 || \
                           (fmt) == PP_OUT_X2B10G10R10 || \
                           (fmt) == PP_OUT_XRGB888 || \
                           (fmt) == PP_OUT_XBGR888 || \
                           (fmt) == PP_OUT_RGBA888 || \
                           (fmt) == PP_OUT_BGRA888 || \
                           (fmt) == PP_OUT_R10G10B10A2 || \
                           (fmt) == PP_OUT_B10G10R10A2 )

#define IS_5_6_5_FORMAT(fmt) \
        ((fmt == PP_OUT_RGB565) || \
         (fmt == PP_OUT_BGR565))

#define IS_8BITS_FORMAT(fmt) \
        ((fmt == PP_OUT_RGB888) || \
         (fmt == PP_OUT_BGR888) || \
         (fmt == PP_OUT_RGB888_P) || \
         (fmt == PP_OUT_BGR888_P) || \
         (fmt == PP_OUT_ARGB888) || \
         (fmt == PP_OUT_ABGR888) || \
         (fmt == PP_OUT_RGBA888) || \
         (fmt == PP_OUT_BGRA888) || \
         (fmt == PP_OUT_XRGB888) || \
         (fmt == PP_OUT_XBGR888))

#define IS_PP_OUT_RGB(fmt) (((fmt) == PP_OUT_FMT_RGB))

#define IS_16BITS_FORMAT(fmt) \
        ((fmt == PP_OUT_R16G16B16) || \
         (fmt == PP_OUT_B16G16R16) || \
         (fmt == PP_OUT_R16G16B16_P) || \
         (fmt == PP_OUT_B16G16R16_P))

#define IS_BGR_FORMAT(fmt) \
         ((fmt == PP_OUT_BGR888) || \
          (fmt == PP_OUT_B16G16R16) || \
          (fmt == PP_OUT_BGR888_P) || \
          (fmt == PP_OUT_B16G16R16_P) || \
          (fmt == PP_OUT_ABGR888) || \
          (fmt == PP_OUT_BGRA888) || \
          (fmt == PP_OUT_A2B10G10R10) || \
          (fmt == PP_OUT_B10G10R10A2) || \
          (fmt == PP_OUT_XBGR888))

#define IS_BGR_P_FORMAT(fmt) \
          (fmt == PP_OUT_BGR888_P) || \
          (fmt == PP_OUT_B16G16R16_P)

#define PP_VSI_LINEAR 0
#define PP_LANCZOS 1
#define PP_NEAREST 2
#define PP_BILINEAR 3
#define PP_BICUBIC 4
#define PP_SPLINE 5
#define PP_BOX  6
#define PP_FAST_LINEAR 7
#define PP_FAST_BICUBIC 8
#define PP_AREA 9

#define HIGH_SCALER 0
#define MIDDLE_SCALER 1
#define ULTRA_SCALER 2

#define ULTRA_SCALE_BIT 14

#define X_COEFF_OFFSET 256
#define Y_COEFF_OFFSET 256

enum StrideUnit {
  STRIDE_UNIT_1B = 0,
  STRIDE_UNIT_2B,
  STRIDE_UNIT_4B,
  STRIDE_UNIT_8B,
  STRIDE_UNIT_16B,
  STRIDE_UNIT_3B,
  STRIDE_UNIT_64B,
  STRIDE_UNIT_512B,
  STRIDE_UNIT_NOT_SUPPORTED
};

typedef struct PpLanczosInfo {
  u32 crop_w;
  u32 crop_h;
  u32 scale_w;
  u32 scale_h;
  u32 pp_filter;
} PpLanczosInfo;

/* PPU internal config */
/* Compared with PpUnitConfig, more internal member variables are added,
   which should not be exposed to external user. */
typedef struct _PpUnitIntConfig {
  u32 enabled;    /* PP unit enabled */
  u32 tiled_e;    /* PP unit tiled4x4 output enabled */
  u32 rgb;
  u32 rgb_planar;
  u32 cr_first;   /* CrCb instead of CbCr */
  addr_t luma_offset;   /* luma offset of current PPU to pp buffer start address(can't bigger than (1<<32 - 1) in 32bit env) */
  addr_t chroma_offset;
  addr_t luma_offset_1;
  addr_t chroma_offset_1;
  addr_t luma_offset_2;
  addr_t chroma_offset_2;
  addr_t luma_offset_3;
  addr_t chroma_offset_3;
  u64 ext_buff_size; /* full pp size in buff */
  u32 hdr_offset;
  u64 luma_size;     /* size of luma/chroma for ppu buffer */
  u64 chroma_size;
  u64 luma_size_1;
  u64 chroma_size_1;
  u64 luma_size_2;
  u64 chroma_size_2;
  u64 luma_size_3;
  u64 chroma_size_3;
  u32 header_offset;   /* FBC header offset of current PPU to pp buffer start addres */
  u32 payload_offset;  /* FBC payload offset of current PPU to pp buffer start addres */
  u64 plane0_offset;   /* PVFBC plane0 offset of current PPU to pp buffer start addres */
  u64 plane1_offset;  /* PVFBC plane1 offset of current PPU to pp buffer start addres */
  u32 pixel_width;   /* pixel bit depth store in external memory */
  u32 stream_pixel_width; /* pixel bit depth in stream header */
  u32 shaper_enabled;
  u32 shaper_no_pad; /*disable frame right edge shaper padding*/
  u32 shaper_tile_edge_pad; /*enable tile edge shaper padding*/
  u32 dec400_enabled; /* --sw don't confige, 1--sw confige */
  DecPicAlignment dec400_align; /**dec400 compression align, 32bytes or 64 bytes*/
  u32 dec400_pln0_tile_status_offset;   /* DEC400 pln0 tile status offset of current PPU to pp buffer start addres */
  u32 dec400_pln1_tile_status_offset;  /* DEC400 pln1 tile status offset of current PPU to pp buffer start addres */
  u8  dec400_conformance_test_en;
  u32 planar;        /* Planar output */
  u32 planar_f;
  u32 align_pixel_w; /* width align in pixel,set by user */
  DecPicAlignment align;    /* alignment for current PPU */
  DecPicAlignment align_h;    /* height alignment for current PPU */
  u32 tile_coded_image; /*current pic is tile coded image*/
  u32 ystride;
  u32 cstride;
  u32 stride_set_by_user; /* stride set by user.*/
  u32 false_ystride;
  u32 false_cstride;
  u32 ystride_1;
  u32 cstride_1;
  u32 false_ystride_1;
  u32 false_cstride_1;
  u32 ystride_2;
  u32 cstride_2;
  u32 false_ystride_2;
  u32 false_cstride_2;
  u32 ystride_3;
  u32 cstride_3;
  u32 false_ystride_3;
  u32 false_cstride_3;
  addr_t bus_address;
  struct {
    u32 enabled;  /* whether cropping is enabled */
    u32 set_by_user;   /* cropping set by user */
    u32 x;        /* cropping start x */
    u32 y;        /* cropping start y */
    u32 width;    /* cropping width */
    u32 height;   /* cropping height */
  } crop;
  struct {
    u32 enabled;
    u32 x;        /* cropping start x */
    u32 y;        /* cropping start y */
    u32 width;    /* cropping width */
    u32 height;   /* cropping height */
  } crop2;
  struct {
    u32 enabled;  /* whether scaling is enabled */
    u32 scale_by_ratio;   /* scaling by ratio */
    u32 ratio_x;  /* 0 indicate flexiable scale */
    u32 ratio_y;
    u32 width;    /* scaled output width */
    u32 height;   /* scaled output height */
  } scale;
  struct {
    /* padding mode:
     * 0 - no padding
     * 1 - padding with scaled output boundary pixels
     * 2 - padding with values specified by
       When not 0, software should check the validation of the value. */
    u32 mode;
    u32 r_y;    /* \brief padding Y componet for YUV, or R component for RGB */
    u32 g_u;    /* \brief padding U componet for YUV, or G component for RGB */
    u32 b_v;    /* \brief padding V componet for YUV, or B component for RGB */
    u32 a;      /* \brief alpha component for RGB */
    u32 l_off;  /* \brief left offset of scaling output window from left boundary of otput frame */
    u32 r_off;  /* \brief right offset of scaling output window from right boundary of otput frame */
    u32 t_off;  /* \brief top offset of scaling output window from top boundary of otput frame */
    u32 b_off;  /* \brief bottom offset of scaling output window from bottom boundary of otput frame */
  } pad;
  u32 frm_width;  /* final frame buffer width in pixels, including padded left, right offset */
  u32 frm_height; /* final frame buffer height in pixels, including padded top, bottom offset */
  u32 monochrome; /* PP output monochrome (luma only) */
  u32 pp_in_format;
  u32 out_format;
  u8  Explicit_Specified_010;
  u32 out_p010;
  u32 out_1010;
  u32 out_I010;
  u32 out_L010;
  u32 out_p012;
  u32 out_I012;
  u32 out_cut_8bits;
  u32 video_range;
  u32 range_max;
  u32 range_min;
  u32 rgb_format;
  u32 rgb_stan;
  u32 target_range;
  u32 set_target_range_enable;
  u32 source_range; /* soure range for range mapping*/
  u32 range_map_flag; /* for range mapping*/
  i32 range_map_max;
  i32 range_map_min;
  struct DWLLinearMem table_3dlut_buffer; /* the table buffer for 3dlut. */
  u8 enable_3dlut; /* for color remapping (3dlut) */
  u32 dither_enable;
  u32 rgb_alpha;
  u32 x_filter_size;
  u32 y_filter_size;
  u32 x_filter_offset;
  u32 y_filter_offset;
  u32 out_width;
  u32 out_height;
  u32 out_ratio_x;
  u32 out_ratio_y;
  u32 pp_filter;
  u32 x_filter_param;
  u32 y_filter_param;
  u32 antialias;
  u32 tile_mode;
  u32 yuv_supertile_pad_copy_e;
  u32 pp_comp;
  u32 pp_fbc_tile_enable;
  u32 pp_pvfbc;
  u32 pvfbc_plane0_headr_size;
  u32 pvfbc_plane1_headr_size;
  //char *in_lib_name;
  u32 pp_pvfbc_lu_const0;
  u32 pp_pvfbc_lu_const1;
  u32 pp_pvfbc_ch_const0;
  u32 pp_pvfbc_ch_const1;
  u8  pp_pvfbc_tile_type;
  u8  pp_pvfbc_bypass;
  u32 vir_left;
  u32 vir_right;
  u32 vir_top;
  u32 vir_bottom;
  u32 src_sel_mode;
  u32 pad_sel;
  u32 pad_Y;
  u32 pad_U;
  u32 pad_V;
  u32 sub_x;
  u32 sub_y;
  u32 chroma_format;
  u32 x_phase_num;
  u32 y_phase_num;
  u32 lc_stripe;  /* number of lines to trigger line counter interrupt */
  u32 lc_int_enabled;

  struct DWLLinearMem lanczos_table;
  struct DWLLinearMem fbc_tile;
  addr_t reorder_buf_bus[MAX_ASIC_CORES];
  u32 reorder_size;
  addr_t scale_buf_bus[MAX_ASIC_CORES];
  u32 scale_size;
  addr_t scale_out_buf_bus[MAX_ASIC_CORES];
  u32 scale_out_size;
  u32 fbc_tile_offset;
  u32 out_yuyv;
  u32 out_uyvy;
  u32 buf_off_set_by_user; // used in heif format with grid
  u32 heif_mode;
  PpLanczosInfo pre_lanczos_info;
  enum StrideUnit unit;
  struct DecSliceTextInfo slice_text;
  const void *dwl;
  struct DWLLinearMem dec400_luma_table;
  struct DWLLinearMem dec400_chroma_table;
  u32 pp_in_org_width;
  u32 pp_in_org_height;
  /* hist */
  u32 hist_local_e;
  u32 hist_global_e;
  u32 hist_hsv_e;
  u32 luma_sum_e;
  u32 luma_sum_shift;
  u32 hist_offset_x[5];
  u32 hist_offset_y[3];
  /* norm */
  u8 norm_enable;
  u32 norm_filter;
  u8 norm_fmat;
  u32 norm_mean_r;
  u32 norm_mean_g;
  u32 norm_mean_b;
  u32 norm_std_r;
  u32 norm_std_g;
  u32 norm_std_b;
  u32 norm_quant_step;
  u64 total_buffer_size[MAX_OUTPUT_PLANE]; /* buffer size needed by current pp channal for each plane */

  u8 dis_checker;

  /* sbi */
  u8  sbi_en;
  u32 sbi_lu_stream_id;
  u32 sbi_ch_stream_id;
  u32 sbi_lu_segmentbuffer_entry_size;
  u32 sbi_ch_segmentbuffer_entry_size;
  u32 sbi_lu_segmentbuffer_entry_count;
  u32 sbi_ch_segmentbuffer_entry_count;
  u32 sbi_lu_segmentbuffer_offset;
  u32 sbi_ch_segmentbuffer_offset;
  u8  sbi_coupled;
  addr_t sbi_lu_addr;
  addr_t sbi_ch_addr;
  u32 sbi_lu_reset_offset;
  u32 sbi_ch_reset_offset;
  /* ringbuffer */
  u8  rb_en;
  u32 rb_height;
  u32 rb_num;
  u8  rb_mode;

  u8 decnano_e;
  u8 decnano_mode;
  u8 lz77_mode;
  u8 yuv400_with_alpha;
  u8 interlace;
} PpUnitIntConfig;

struct PpParams {
  const struct DecHwFeatures *p_hw_feature; /* hw feature struct pointer */
  PpUnitIntConfig *ppu_cfg;
  struct DWLLinearMem *ppu_out_addr; /* base address of pp buffer */
  u32 mono_chrome; /* whether input picture is mono-chrome */
  u32 bottom_field_flag; /* set 1 if current is the bottom field of a picture,
                              otherwise (top field or frame) 0
                         */
  u32 pp_out_ctrl;
  u32 use_separated_pp;
  u32 tile_enable;
  u32 sbi_pic_start;
  u8  interlace;
};

#define DEC400_TILE_SIZE        (256)
#define DEC400_TBL_ALIGN_FACTOR (256)
#define DEC400_IN_HEADER_SIZE   (128)
void PpFillDec400TblInfo(PpUnitIntConfig *ppu_cfg,
                         const u32 *pp_start_vir_addr,
                         addr_t pp_start_bus_addr,
                         struct DWLLinearMem *luma_tbl,
                         struct DWLLinearMem *chroma_tbl);

void UpdatePpUnitStride(PpUnitIntConfig *ppu_cfg);
u32 CheckPpUnitConfig(const struct DecHwFeatures *hw_feature,
                      u32 in_width,
                      u32 in_height,
                      u32 interlace,
                      u32 pixel_width,
                      u32 chroma_format,
                      PpUnitIntConfig *ppu_cfg);
u64 CalcPpUnitBufferSize(const struct DecHwFeatures *hw_feature, PpUnitIntConfig *ppu_cfg, u32 mono_chrome, u32 use_separated_pp);
void CalcOnePpUnitLumaSize(PpUnitIntConfig *ppu_cfg, u64* size_array);
void CalcOnePpUnitChromaSize(PpUnitIntConfig *ppu_cfg, u32 mono_chrome, u64* size_array);
u64 CalcOnePpUnitDec400TblSize(const struct DecHwFeatures *hw_feature, PpUnitIntConfig *ppu_cfg, u64 luma_size, u64 chroma_size, u32 i);
u32 CalcOnePpUnitFBCHeaderPayloadSize(PpUnitIntConfig *ppu_cfg, u32 header);
u64 CalcOnePpUnitPVFBCPlane0Size(PpUnitIntConfig *ppu_cfg);
u64 CalcOnePpUnitPVFBCPlane1Size(PpUnitIntConfig *ppu_cfg, u32 mono_chrome);

void calSecondUpScaleRatio(const struct DecHwFeatures *hw_feature, PpUnitIntConfig *ppu_cfg, u32 interlace);

void PpUnitSetIntConfig(PpUnitIntConfig *ppu_int_cfg,
                        PpUnitConfig *ppu_ext_cfg,
                        const struct DecHwFeatures *hw_feature,
                        u32 pixel_width,
                        u32 frame_only,
                        u32 mono_chrome);
void PPSetRegs(u32 *pp_regs, const struct PpParams *pp_args);
void PPSetOneChannelRegs(u32 *pp_regs, const struct PpParams *pp_args, u32 channel_id);
u32 PPGetLancozsColumnBufferSize(PpUnitIntConfig *ppu_cfg, u32 pic_height, u32 pixel_width, u32 num_tile_cols);
void PPSetLancozsScaleRegs(u32 *pp_regs,
                           const struct DecHwFeatures *hw_feature,
                           PpUnitIntConfig *ppu_cfg,
                           u32 core_id);

void PPSetLancozsMutiCoreScaleRegs(u32 *pp_regs,
                                   const struct DecHwFeatures *hw_feature,
                                   PpUnitIntConfig *ppu_cfg, u32 tile_id);

u32 PPCheckMutiCoreSupport(PpUnitIntConfig *ppu_cfg, u32 filter_bypass, u32 sb_size, u32 tile_cols, u8* tile_col_mem);

enum DecPictureFormat TransUnitConfig2Format(PpUnitIntConfig *ppu_int_cfg);

void InitPpUnitBoundCoeff(const struct DecHwFeatures *hw_feature,
                                  u32 field_pic,
                                  PpUnitIntConfig *ppu_cfg);

void PPSetFbcRegs(u32 *pp_regs, const struct PpParams *pp_args);
void PPSetSBIRegs(u32 *pp_regs, u32 i, PpUnitIntConfig *ppu_cfg, u32 sbi_pic_start);
u32 CheckPpStride(PpUnitIntConfig *ppu_cfg);

#define PI 3.141592653589793
#define TABLE_LENGTH 5
#define TABLE_SIZE  32
#define LAN_WINDOW_HOR 2
#define LAN_WINDOW_VER 2
#define LAN_SIZE_HOR (2*LAN_WINDOW_HOR+1)
#define LAN_SIZE_VER (2*LAN_WINDOW_VER+1)
#define LANCZOS_MAX_DOWN_RATIO 32
#define MAX_COEFF_SIZE_HOR_REAL  (2*LAN_WINDOW_HOR*LANCZOS_MAX_DOWN_RATIO-1)
#define MAX_COEFF_SIZE_VER_REAL  (2*LAN_WINDOW_VER*LANCZOS_MAX_DOWN_RATIO-1)
#define MAX_COEFF_SIZE_HOR  (((MAX_COEFF_SIZE_HOR_REAL+3)/4)*4)
#define MAX_COEFF_SIZE_VER  (((MAX_COEFF_SIZE_VER_REAL+3)/4)*4)
#define LANCZOS_TABLE_SIZE  (TABLE_SIZE / 2 + 1)*(MAX_COEFF_SIZE_HOR + MAX_COEFF_SIZE_VER)
#define LANCZOS_EDGE_SIZE    (2*(((LAN_WINDOW_HOR+3)/4)*4+((LAN_WINDOW_VER+3)/4)*4))
#define LANCZOS_MAX_HEIGHT   8192
//#define LANCZOS_TILE_EDGE_SIZE (3*LANCZOS_MAX_HEIGHT*64/4)
//#define LANCZOS_TILE_PPOUT_SIZE (24*LANCZOS_MAX_HEIGHT/4)
#define LANCZOS_TILE_EDGE_SIZE (1024 * 1024 / 4)
#define LANCZOS_TILE_PPOUT_SIZE (1024 * 128)
/* Note : 512*1024/4 this number has been used in function DWLSyncMemData(ppu.c),
 * if you change it, please check it in function DWLSyncMemData(ppu.c). */
// #define LANCZOS_COEFF_BUFFER_SIZE  544
#define LANCZOS_COEFF_BUFFER_SIZE  (32*32*2+8*4)
//#define LANCZOS_BUFFER_SIZE  (LANCZOS_TABLE_SIZE+LANCZOS_EDGE_SIZE)
#define FIXED_BITS_UNI 16
#define MAKE_FIXED_UNI(x) ((x)<<FIXED_BITS_UNI)
#define FIXED_RADIX MAKE_FIXED_UNI(1)
#define FLOOR(x) ((x)/(FIXED_RADIX))
#if 0
#define DEC400_YUV_TABLE_SIZE (131072)
#define DEC400_PP_TABLE_SIZE (2*DEC400_YUV_TABLE_SIZE)
#define DEC400_PPn_TABLE_OFFSET(n) (((n)*(DEC400_PP_TABLE_SIZE)))
#define DEC400_PPn_Y_TABLE_OFFSET(n) (DEC400_PPn_TABLE_OFFSET(n))
#define DEC400_PPn_UV_TABLE_OFFSET(n) (DEC400_PPn_TABLE_OFFSET(n) + DEC400_YUV_TABLE_SIZE)
#endif
#endif /* __PPU__ */
