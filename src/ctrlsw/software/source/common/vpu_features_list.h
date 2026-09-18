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
{ /* 0x902 */
  .id = 0x00002070,
  .id_mask = 0xFFFF,
  /* decoding formats related */
  /* HEVC - 0: HEVC not supported; 1 - YUV420/YUV400; 2 - YUV422/YUV420/YUV400; 3 - YUV444/YUV422/YUV420/YUV400 */
  .hevc_support = 0,
  .hevc_main10_support = 0,
  .vp9_support = 0,
  .vp9_profile2_support = 0,
  /* G1_H264 - 0: H264 not supported; 1 - Baseline; 2 - Main; 3 - High */
  .h264_support = 0,
  /* G2_H264 - 0: H264 not supported; 1 - YUV420/YUV400; 2 - YUV422/YUV420/YUV400; 3 - YUV444/YUV422/YUV420/YUV400 */
  .h264_adv_support = 0,
  .h264_high10_support = 0,
  .mpeg4_support = 0,
  .custom_mpeg4_support = 0,
  .mpeg2_support = 0,
  .sorenson_spark_support = 0,
  .vc1_support = 0,
  .png_support = 1,
  .jpeg_support = 1,
  .pjpeg_support = 0,
  .rv_support = 0,
  .vp8_support = 0,
  .webp_support = 0,
  .vp7_support = 0,
  .vp6_support = 0,
  .avs_support = 0,
  .avs_plus_support = 0,
  .avs2_support = 0,
  .avs2_main10_support = 0,
  /* AV1 - 0: AV1 not supported; 1 - YUV420/YUV400; 2 - YUV422/YUV420/YUV400; 3 - YUV444/YUV422/YUV420/YUV400 */
  .av1_support = 0,
  .vvc_support = 0,
  .avs3_support = 0,
  /* resolution related */
  .vvc_max_dec_pic_width = 8192,
  .vvc_max_dec_pic_height = 4320,
  .avs3_max_dec_pic_width = 8192,
  .avs3_max_dec_pic_height = 4320,
  .av1_max_dec_pic_width = 8192,
  .av1_max_dec_pic_height = 4320,
  .hevc_max_dec_pic_width = 8192,
  .hevc_max_dec_pic_height = 4320,
  .vp9_max_dec_pic_width = 8192,
  .vp9_max_dec_pic_height = 4320,
  .h264_max_dec_pic_width = 1920,
  .h264_max_dec_pic_height = 1088,
  .mpeg4_max_dec_pic_width = 1920,
  .mpeg4_max_dec_pic_height = 1088,
  .mpeg2_max_dec_pic_width = 1920,
  .mpeg2_max_dec_pic_height = 1088,
  .sspark_max_dec_pic_width = 1920,
  .sspark_max_dec_pic_height = 1088,
  .vc1_max_dec_pic_width = 4096,
  .vc1_max_dec_pic_height = 2160,
  .rv_max_dec_pic_width = 1920,
  .rv_max_dec_pic_height = 1088,
  .vp8_max_dec_pic_width = 4096,
  .vp8_max_dec_pic_height = 2160,
  .vp7_max_dec_pic_width = 1920,
  .vp7_max_dec_pic_height = 1088,
  .vp6_max_dec_pic_width = 1920,
  .vp6_max_dec_pic_height = 1088,
  .avs_max_dec_pic_width = 1920,
  .avs_max_dec_pic_height = 1088,
  .divx_max_dec_pic_width = 1920,
  .divx_max_dec_pic_height = 1088,
  .img_max_dec_width = 8192,
  .img_max_dec_height = 8192,
  .png_max_dec_width = 4096,
  .png_max_dec_height = 4096,
  /* Other features */
  .addr64_support = 0,
  .rfc_support = 0,
  .qp_dump_support = 0,
  /* decoding flow supported */
  .low_latency_mode_support = 1,
  .multicore_support = 0,
  .vp9_hw_prob_support = 0,
  .partial_decoding_support = 0,
  /* pp related features */
  .max_ppu_count = 1,
  .pp_standalone = 0,
  .pp_version = UNFIED_PP,
  .max_pp_out_pic_width = {4096,0,0,0},
  .max_pp_out_pic_height = {4096,0,0,0},
  /* down scaling is also supported when pp input size (i.e., cropping output) exceeds pp max output size */
  .dscale_support = {1,0,0,0},
  .uscale_support = {1,0,0,0},
  /*low_cost_scale: Bicubic/Bilinear/Nearest/Area */
  .scale_cost_support = {1,0,0,0},
  /*scale: Area*/
  .only_area_scale_alg_support = {0,0,0,0},
  /*crop_step_rshift: 0:support odd/even crop, 1:support even crop*/
  .crop_step_rshift = 0,
  /* output format supported */
  .fmt_tile_support = {0,0,0,0},
  .fmt_supertile_support = {0,0,0,0},
  .fmt_output_tile8x8_support = {0,0,0,0},
  .fmt_yuv_tile64x64_support = {0,0,0,0},
  .fmt_p010_support = 0,
  .fmt_rgb_support = {1,0,0,0},
  .pp_planar_support = {0,0,0,0},
  .pp_yuv420_101010_support = 0,
  .second_crop_support = 0,
  /*pp_comp: 1:FBC, 2:DEC400, 3:PVFBC, 4:DECNANO*/
  .pp_comp_support = {0,0,0,0},
  .pp_comp_version = {0,0,0,0},
  .pp_420to422_support = {0,0,0,0},
  .pp_tiled_stride_shift = 3,
  .pp_support_6x = 0,
  .pp_area_optimize = 1,
  .other_legal_HV_support = 0,
  .shaper_enabled = {1,0,0,0},
  .pp_half_scale_support = {0,0,0,0},
  .pp_rfd = 0,
  .pp_dec400d = 0,
  .pp_flip_rotation = 0,
  .pp_3dlut = {0,0,0,0},
  .pp_blending = {0,0,0,0},
  .pp_norm = {0,0,0,0},
  /* sub_system supported */
  .shaper_alignment = ALIGN_128B,
  .max_regs = 428,
  .yuv420_8b_to_p010 = 0,
  .master_data_width = 64,
},
