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
#ifndef __VPU_FEATURES_STRUCT_H__
#define __VPU_FEATURES_STRUCT_H__
struct DecHwFeatures {
  u32 id;
  u32 id_mask;
  /* decoding formats related */
  /* HEVC - 0: HEVC not supported; 1 - YUV420/YUV400; 2 - YUV422/YUV420/YUV400; 3 - YUV444/YUV422/YUV420/YUV400 */
  u32 hevc_support;
  u32 hevc_main10_support;
  u32 vp9_support;
  u32 vp9_profile2_support;
  /* G1_H264 - 0: H264 not supported; 1 - Baseline; 2 - Main; 3 - High */
  u32 h264_support;
  /* G2_H264 - 0: H264 not supported; 1 - YUV420/YUV400; 2 - YUV422/YUV420/YUV400; 3 - YUV444/YUV422/YUV420/YUV400 */
  u32 h264_adv_support;
  u32 h264_high10_support;
  u32 mpeg4_support;
  u32 custom_mpeg4_support;
  u32 mpeg2_support;
  u32 sorenson_spark_support;
  u32 vc1_support;
  u32 png_support;
  u32 jpeg_support;
  u32 pjpeg_support;
  u32 rv_support;
  u32 vp8_support;
  u32 webp_support;
  u32 vp7_support;
  u32 vp6_support;
  u32 avs_support;
  u32 avs_plus_support;
  u32 avs2_support;
  u32 avs2_main10_support;
  /* AV1 - 0: AV1 not supported; 1 - YUV420/YUV400; 2 - YUV422/YUV420/YUV400; 3 - YUV444/YUV422/YUV420/YUV400 */
  u32 av1_support;
  u32 vvc_support;
  u32 avs3_support;
  /* resolution related */
  u32 vvc_max_dec_pic_width;
  u32 vvc_max_dec_pic_height;
  u32 avs3_max_dec_pic_width;
  u32 avs3_max_dec_pic_height;
  u32 av1_max_dec_pic_width;
  u32 av1_max_dec_pic_height;
  u32 hevc_max_dec_pic_width;
  u32 hevc_max_dec_pic_height;
  u32 vp9_max_dec_pic_width;
  u32 vp9_max_dec_pic_height;
  u32 h264_max_dec_pic_width;
  u32 h264_max_dec_pic_height;
  u32 mpeg4_max_dec_pic_width;
  u32 mpeg4_max_dec_pic_height;
  u32 mpeg2_max_dec_pic_width;
  u32 mpeg2_max_dec_pic_height;
  u32 sspark_max_dec_pic_width;
  u32 sspark_max_dec_pic_height;
  u32 vc1_max_dec_pic_width;
  u32 vc1_max_dec_pic_height;
  u32 rv_max_dec_pic_width;
  u32 rv_max_dec_pic_height;
  u32 vp8_max_dec_pic_width;
  u32 vp8_max_dec_pic_height;
  u32 vp7_max_dec_pic_width;
  u32 vp7_max_dec_pic_height;
  u32 vp6_max_dec_pic_width;
  u32 vp6_max_dec_pic_height;
  u32 avs_max_dec_pic_width;
  u32 avs_max_dec_pic_height;
  u32 divx_max_dec_pic_width;
  u32 divx_max_dec_pic_height;
  u32 img_max_dec_width;
  u32 img_max_dec_height;
  u32 png_max_dec_width;
  u32 png_max_dec_height;
  /* Other features */
  u32 addr64_support;
  u32 rfc_support;
  u32 qp_dump_support;
  /* decoding flow supported */
  u32 low_latency_mode_support;
  u32 multicore_support;
  u32 vp9_hw_prob_support;
  u32 partial_decoding_support;
  /* pp related features */
  u32 max_ppu_count;
  u32 pp_standalone;
  enum {
    NO_PP = 0,
    G1_NATIVE_PP,
    G2_NATIVE_PP,
    FIXED_DS_PP,
    UNFIED_PP
  } pp_version;
  u32 max_pp_out_pic_width[6];
  u32 max_pp_out_pic_height[6];
  /* down scaling is also supported when pp input size (i.e., cropping output) exceeds pp max output size */
  u32 dscale_support[6];
  u32 uscale_support[6];
  /*low_cost_scale: Bicubic/Bilinear/Nearest/Area */
  u32 scale_cost_support[6];
  /*scale: Area*/
  u32 only_area_scale_alg_support[6];
  /*crop_step_rshift: 0:support odd/even crop, 1:support even crop*/
  u32 crop_step_rshift;
  /* output format supported */
  u32 fmt_tile_support[6];
  u32 fmt_supertile_support[6];
  u32 fmt_output_tile8x8_support[6];
  u32 fmt_yuv_tile64x64_support[6];
  u32 fmt_p010_support;
  u32 fmt_rgb_support[6];
  u32 pp_planar_support[6];
  u32 pp_yuv420_101010_support;
  u32 second_crop_support;
  /*pp_comp: 1:FBC, 2:DEC400, 3:PVFBC, 4:DECNANO*/
  u32 pp_comp_support[6];
  u32 pp_comp_version[6];
  u32 pp_420to422_support[6];
  u32 pp_tiled_stride_shift;
  u32 pp_support_6x;
  u32 pp_area_optimize;
  u32 other_legal_HV_support;
  u32 shaper_enabled[6];
  u32 pp_half_scale_support[6];
  u32 pp_rfd;
  u32 pp_dec400d;
  u32 pp_flip_rotation;
  u32 pp_3dlut[6];
  u32 pp_blending[6];
  u32 pp_norm[6];
  /* sub_system supported */
  enum {
  ALIGN_1B = 0,
  ALIGN_8B = 3,
  ALIGN_16B = 4,
  ALIGN_32B,
  ALIGN_64B,
  ALIGN_128B,
  ALIGN_256B,
  ALIGN_512B,
  ALIGN_1024B,
  ALIGN_2048B
  } shaper_alignment;
  u32 max_regs;
  u32 yuv420_8b_to_p010;
  u32 master_data_width;
};
#endif
