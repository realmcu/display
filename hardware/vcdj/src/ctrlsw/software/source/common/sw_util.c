/*------------------------------------------------------------------------------
--       Copyright (c) 2015, VeriSilicon Inc. All rights reserved             --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
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

#if defined(_WIN32) || defined(WIN32)
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include "sw_util.h"
#include "sw_debug.h"
#include "deccfg.h"
#include "ppu.h"
#include "input_queue.h"
#include "sw_stream.h"
#include "vpufeature.h"
#include "dec_log.h"
#include "regdrv.h"
#include "commonconfig.h"

u32 SwCheckMemValid(const struct DWLDecBuffers *buffer, const void *ppu_cfg, u32 use_separated_pp) {
  u32 j, valid = 1;
  const PpUnitIntConfig *ppu = (PpUnitIntConfig *)ppu_cfg;

  if (use_separated_pp == 0) {
    valid = DWL_DEVMEM_VAILD(buffer->dec_buffer[0][0]);
  }
  else {
    for (j=0; j<DEC_MAX_OUT_COUNT; j++, ppu++) {
      if (!ppu->enabled) continue;
      if (!DWL_DEVMEM_VAILD(buffer->dec_buffer[j][0])) {
        valid = 0;
        break;
      }
      /* TODO: FIXME, whether check other plane */
    }
  }

  return valid;
}

av_unused static u32 SwCheckAddrAligned(addr_t bus_address, const u32 *virtual_address) {
  if (X170_CHECK_BUS_ADDRESS_AGLINED(bus_address)) {
    return 0;
  }
  return 1;
}

u32 SwCheckBufferValid(u64 next_buf_size, const struct DWLLinearMem *info, u64 *total_size) {

  av_unused u32 is_addr_aligned = 1, i, j;
  av_unused u32 valid = 1;
  av_unused const struct DWLLinearMem *tmp = NULL;
  av_unused const struct DWLDecBuffers *out_buf = (const struct DWLDecBuffers *)info;

  // for add buffer
  if (*total_size == 0)  {
    // only check whether exist one valid output of the first plane
    for (i=0; i<DEC_MAX_OUT_COUNT; i++) {
      if (DWL_DEVMEM_VAILD(out_buf->dec_buffer[i][0]))
        break;
    }
    if (i == DEC_MAX_OUT_COUNT)
      valid = 0;
    else
      *total_size = next_buf_size;
  }
  else if (*total_size == 0xFF) { // for check buffer inside sdk, like for jpeg/av1/vp9
    *total_size = 0;
    for (i=0; i<DEC_MAX_OUT_COUNT; i++) {
      for (j=0; j<MAX_OUTPUT_PLANE; j++) {
        if (DWL_DEVMEM_VAILD(out_buf->dec_buffer[i][j])) {
          tmp = &out_buf->dec_buffer[i][j];
          is_addr_aligned = SwCheckAddrAligned(tmp->bus_address, tmp->virtual_address);
          if (!is_addr_aligned)
            break;
          *total_size += tmp->logical_size;
        }
      }
      if (!is_addr_aligned)
        break;
    }
    if (!is_addr_aligned || *total_size < next_buf_size)
      valid =  0;
    else
      valid =  1;
  }

  return valid;
}

u32 SwIsMatchBufferByAddr(const struct DWLDecBuffers *buf, const struct DWLLinearMem *addr) {
  u32 j, k;
  u32 matched = 0, cur_channel_match = 0;
  const struct DWLDecBuffers *cur_buffer = (const struct DWLDecBuffers *)addr;

  for (j=0; j<DEC_MAX_OUT_COUNT; j++) {
    u32 cur_plane_match_num = 0;

    for (k=0; k<MAX_OUTPUT_PLANE; k++) {
      // if (!DWL_DEVMEM_VAILD(buf->dec_buffer[j][k]) && !DWL_DEVMEM_VAILD(cur_buffer->dec_buffer[j][k])) {
      //   cur_plane_match_num++;
      //   continue;
      // }
      if (DWL_DEVMEM_COMPARE_V2(buf->dec_buffer[j][k], cur_buffer->dec_buffer[j][k]))
        cur_plane_match_num++;
      else
        break;
    }
    if (cur_plane_match_num == MAX_OUTPUT_PLANE)
      cur_channel_match++;
    else
      break;
  }

  if (cur_channel_match == DEC_MAX_OUT_COUNT)
    matched = 1;

  return matched;
}
#ifdef ENABLE_FPGA_VERIFICATION
u32 is_grid_case;
#endif
u32 SwFindBufferIdByAddr(const struct DWLDecBuffers *buffers, u32 buffer_num,
                         const struct DWLLinearMem *addr) {
  u32 i, index = 0xFFFF; /* not found */

  for (i = 0; i < buffer_num; i++) {
    if (SwIsMatchBufferByAddr(&buffers[i], addr)) {
      index = i;
      break;
    }
  }

/* only used to internal test*/
#ifdef ENABLE_FPGA_VERIFICATION
  if (!is_grid_case)
    ASSERT(index < buffer_num);
#endif

  return index;
}

void SwReplacePpOutBuf(u32 ext_flag, const struct DWLDecBuffers *replace_buffer, struct DWLDecBuffers *out_buf) {
  u32 j, k;

  if (ext_flag) {
    for (j=0; j<DEC_MAX_OUT_COUNT; j++) {
      for (k=0; k<MAX_OUTPUT_PLANE; k++) {
        out_buf->dec_buffer[j][k].virtual_address = (u32 *)replace_buffer->dec_buffer[j][k].virtual_address;
        out_buf->dec_buffer[j][k].bus_address = replace_buffer->dec_buffer[j][k].bus_address;
      }
    }
  }
}

void SwSetPpOutAddr(const struct SwParames *sw_parames, u32 ppu_id, const struct DWLLinearMem *cur_pp_data,
                    struct DWLDecBuffers *temp_out_addr, struct DecOutputPictureAddr *out_pic_addr) {
  const PpUnitIntConfig *ppu_cfg = &sw_parames->ppu_cfg[ppu_id];
  struct DWLLinearMem *cur_pp_out_addr = NULL;
  struct DWLDecBuffers *out_buf = (struct DWLDecBuffers *)cur_pp_data;

  if (sw_parames->use_separated_pp == 0) {
    cur_pp_out_addr = &out_buf->dec_buffer[0][0];
    ASSERT(cur_pp_out_addr);

    out_pic_addr->luma_vir_addr = (u32*)((u8*)cur_pp_out_addr->virtual_address + ppu_cfg->luma_offset);
    out_pic_addr->luma_bus_addr = cur_pp_out_addr->bus_address + ppu_cfg->luma_offset;
    if (!ppu_cfg->monochrome && !ppu_cfg->rgb_planar) {
      //CalcOnePpUnitLumaSize think rgb_planar only one plane
      out_pic_addr->chroma_vir_addr = (u32*)((u8*)cur_pp_out_addr->virtual_address + ppu_cfg->chroma_offset);
      out_pic_addr->chroma_bus_addr = cur_pp_out_addr->bus_address + ppu_cfg->chroma_offset;
    } else {
      out_pic_addr->chroma_vir_addr = NULL;
      out_pic_addr->chroma_bus_addr = 0;
    }

    if (temp_out_addr != NULL && !DWL_DEVMEM_VAILD(temp_out_addr->dec_buffer[0][0])) {
      temp_out_addr->dec_buffer[0][0].virtual_address = (u32 *)out_pic_addr->luma_vir_addr;
      temp_out_addr->dec_buffer[0][0].bus_address = out_pic_addr->luma_bus_addr;
    }
  }
  else {
    cur_pp_out_addr = &out_buf->dec_buffer[ppu_id][0];

    out_pic_addr->luma_vir_addr = cur_pp_out_addr[0].virtual_address;
    out_pic_addr->luma_bus_addr = cur_pp_out_addr[0].bus_address;
    if (!DWL_DEVMEM_VAILD(cur_pp_out_addr[1])) {
      if (!ppu_cfg->monochrome && ppu_cfg->total_buffer_size[1] && !ppu_cfg->rgb_planar) {
        out_pic_addr->chroma_vir_addr = (u32*)((u8*)cur_pp_out_addr[0].virtual_address + ppu_cfg->chroma_offset);
        out_pic_addr->chroma_bus_addr = cur_pp_out_addr[0].bus_address + ppu_cfg->chroma_offset;
      } else {
        out_pic_addr->chroma_vir_addr = NULL;
        out_pic_addr->chroma_bus_addr = 0;
      }
    }
    else {
      if (MAX_OUTPUT_PLANE >= 3) {
        out_pic_addr->chroma_vir_addr = cur_pp_out_addr[1].virtual_address;
        out_pic_addr->chroma_bus_addr = cur_pp_out_addr[1].bus_address;
        out_pic_addr->chroma_cr_vir_addr = cur_pp_out_addr[2].virtual_address;
        out_pic_addr->chroma_cr_bus_addr = cur_pp_out_addr[2].bus_address;
      }
    }

    if (temp_out_addr != NULL) {
      temp_out_addr->dec_buffer[ppu_id][0].virtual_address = (u32 *)out_pic_addr->luma_vir_addr;
      temp_out_addr->dec_buffer[ppu_id][0].bus_address = out_pic_addr->luma_bus_addr;
      if (MAX_OUTPUT_PLANE >= 3 /*&& use_multi_plane == 1*/) {
        temp_out_addr->dec_buffer[ppu_id][1].virtual_address = (u32 *)out_pic_addr->chroma_vir_addr;
        temp_out_addr->dec_buffer[ppu_id][1].bus_address = out_pic_addr->chroma_bus_addr;
        temp_out_addr->dec_buffer[ppu_id][2].virtual_address = (u32 *)out_pic_addr->chroma_cr_vir_addr;
        temp_out_addr->dec_buffer[ppu_id][2].bus_address = out_pic_addr->chroma_cr_bus_addr;
      }
    }
  }
}

void SwConvertOutAddr(const struct DWLDecBuffers *src_out0, u32 ppu_id, u32 use_separated_pp,
                      const struct DecOutputPictureAddr *out_pic_addr, struct DWLDecBuffers *buffers) {
  if (use_separated_pp == 0) {
    buffers->dec_buffer[0][0].virtual_address = (u32 *)out_pic_addr->luma_vir_addr;
    buffers->dec_buffer[0][0].bus_address = out_pic_addr->luma_bus_addr;
  }
  else {
    buffers->dec_buffer[ppu_id][0].virtual_address = (u32 *)out_pic_addr->luma_vir_addr;
    buffers->dec_buffer[ppu_id][0].bus_address = out_pic_addr->luma_bus_addr;
    if (MAX_OUTPUT_PLANE >= 3 /*&& SwGetChannelIsMultiPlane(src_out0, ppu_id)*/) {
      buffers->dec_buffer[ppu_id][1].virtual_address = (u32 *)out_pic_addr->chroma_vir_addr;
      buffers->dec_buffer[ppu_id][1].bus_address = out_pic_addr->chroma_bus_addr;
      if (out_pic_addr->chroma_cr_bus_addr) {
        buffers->dec_buffer[ppu_id][2].virtual_address = (u32 *)out_pic_addr->chroma_cr_vir_addr;
        buffers->dec_buffer[ppu_id][2].bus_address = out_pic_addr->chroma_cr_bus_addr;
      }
    }
  }

  UNUSED(src_out0);
}

void SwSetNoPPOutFormat(u32 is_compressor, const u32 *tiled_mode, u32 bit_depth, u32 chroma_format_idc,
                        enum DecPictureFormat *output_format) {
  if (is_compressor) {
    *output_format = DEC_OUT_FRM_RFC;
  }
  else {
    if (tiled_mode && *tiled_mode == 0) { //264 interlaced or g1 format
      *output_format = DEC_OUT_FRM_RASTER_SCAN;
    }
    else {
#ifdef TILE_8x8
      if (chroma_format_idc == 0)
        *output_format = (bit_depth == 8) ? DEC_OUT_FRM_YUV400TILE8x8 : DEC_OUT_FRM_YUV400TILE8x8_PACK10;
      else if (chroma_format_idc == 1)
        *output_format = (bit_depth == 8) ? DEC_OUT_FRM_YUV420TILE8x8 : DEC_OUT_FRM_YUV420TILE8x8_PACK10;
      else if (chroma_format_idc == 2)
        *output_format = (bit_depth == 8) ? DEC_OUT_FRM_YUV422TILE8x8 : DEC_OUT_FRM_YUV422TILE8x8_PACK10;
      else //if (chroma_format_idc == 3)
        *output_format = (bit_depth == 8) ? DEC_OUT_FRM_YUV444TILE8x8 : DEC_OUT_FRM_YUV444TILE8x8_PACK10;
#else
      if (chroma_format_idc == 0)
        *output_format = (bit_depth == 8) ? DEC_OUT_FRM_YUV400TILE : DEC_OUT_FRM_YUV400TILE_PACK10;
      else if (chroma_format_idc == 1)
        *output_format = (bit_depth == 8) ? DEC_OUT_FRM_YUV420TILE : DEC_OUT_FRM_YUV420TILE_PACK10;
      else if (chroma_format_idc == 2)
        ASSERT(0);
      else //if (chroma_format_idc == 3)
        ASSERT(0);
#endif
    }
  }
}

void SwSetG1NoPPOutFormat(const u32 *tiled_mode, u32 bit_depth, u32 chroma_format_idc,
                          enum DecPictureFormat *output_format) {

  if (tiled_mode && *tiled_mode == 0) { //264 interlaced or g1 format
    *output_format = DEC_OUT_FRM_RASTER_SCAN;
  }
  else {
    if (chroma_format_idc == 0)
      *output_format = (bit_depth == 8) ? DEC_OUT_FRM_YUV400TILE : DEC_OUT_FRM_YUV400TILE_PACK10;
    else if (chroma_format_idc == 1)
      *output_format = (bit_depth == 8) ? DEC_OUT_FRM_YUV420TILE : DEC_OUT_FRM_YUV420TILE_PACK10;
    else if (chroma_format_idc == 2)
      ASSERT(0);
    else //if (chroma_format_idc == 3)
      ASSERT(0);
  }
}

u32 SwSetNoPPStride(u32 is_compressor, u32 chroma_format_idc, u32 bit_depth,
                    u32 align, u32 pic_width_y, u32 pic_width_c,
                    u32 *pic_stride, u32 *pic_stride_ch) {
  u32 line_of_width = 8;

  if (is_compressor) {
    line_of_width = 8;
    *pic_stride = NEXT_MULTIPLE(8 * pic_width_y * bit_depth, ALIGN(align) * 8) / 8;
    if (chroma_format_idc == 3)
      *pic_stride_ch = NEXT_MULTIPLE(4 * pic_width_c * bit_depth * 2, ALIGN(align) * 8) / 8;
    else
      *pic_stride_ch = NEXT_MULTIPLE(4* pic_width_c * bit_depth, ALIGN(align) * 8) / 8;
  }
  else {
    *pic_stride = NEXT_MULTIPLE(TILE_HEIGHT * pic_width_y * bit_depth, ALIGN(align) * 8) / 8;
#ifdef TILE_8x8
    line_of_width = 8;
    if (chroma_format_idc == 3)
      *pic_stride_ch = NEXT_MULTIPLE(4 * pic_width_c * bit_depth * 2, ALIGN(align) * 8) / 8;
    else
      *pic_stride_ch = NEXT_MULTIPLE(4* pic_width_c * bit_depth, ALIGN(align) * 8) / 8;
#else
    line_of_width = 4;
    if (chroma_format_idc == 3)
      *pic_stride_ch = *pic_stride * 2;
    else
      *pic_stride_ch = *pic_stride;
#endif
  }

  return line_of_width;
}

u32 SwSetG1NoPPStride(u32 chroma_format_idc, u32 bit_depth,
                      u32 align, u32 pic_width_y, u32 pic_width_c,
                      u32 *pic_stride, u32 *pic_stride_ch) {
  u32 line_of_width = 8;

  *pic_stride = NEXT_MULTIPLE(4 * pic_width_y * bit_depth, ALIGN(align) * 8) / 8;
  line_of_width = 4;
  if (chroma_format_idc == 1 || chroma_format_idc == 2)
    *pic_stride_ch = *pic_stride;
  else
    *pic_stride_ch = *pic_stride * 2;
  return line_of_width;
}

u32 SwIsMatchBufferByPPData(const struct DecOutputPictureAddr *out_pic_addr,
                            const struct DWLDecBuffers *pp_data, u32 use_separated_pp) {
  u32 j, k;
  u32 matched = 0;
  const struct DWLLinearMem *tmp = NULL;

  if (use_separated_pp == 0) {
    tmp = &pp_data->dec_buffer[0][0];
    for (k=0; k<MAX_OUTPUT_PLANE; k++) {
      if (out_pic_addr[k].luma_bus_addr) {
        if(out_pic_addr[k].luma_bus_addr == tmp->bus_address)
          matched = 1;

        break;
      }
    }
  }
  else {
    u32 cur_channel_match = 0;
    for (j=0; j<DEC_MAX_OUT_COUNT; j++) {
      tmp = pp_data->dec_buffer[j];

      u32 cur_plane_match = 0;
      for (k=0; k<MAX_OUTPUT_PLANE; k++) {
        if (DWL_DEVMEM_VAILD(pp_data->dec_buffer[j][k])) {
          if (k==0 && out_pic_addr[j].luma_bus_addr == tmp[0].bus_address)
            cur_plane_match++;
          else if (k==1 && out_pic_addr[j].chroma_bus_addr == tmp[1].bus_address)
            cur_plane_match++;
          else if (k==2 && out_pic_addr[j].chroma_cr_bus_addr == tmp[2].bus_address)
            cur_plane_match++;
        }
        else
          cur_plane_match++;
      }
      if (cur_plane_match == MAX_OUTPUT_PLANE)
        cur_channel_match++;
      else
        break;
    }
    if (cur_channel_match == DEC_MAX_OUT_COUNT) {
      matched = 1;
    }
  }

  return matched;
}

u32 SwGetChannelIsMultiPlane(const struct DWLDecBuffers *out_addr, u32 ppu_id) {
  u32 k, valid_plane_addr_conut = 0;

  for (k=0; k<MAX_OUTPUT_PLANE; k++) {
    if (DWL_DEVMEM_VAILD(out_addr->dec_buffer[ppu_id][k]))
      valid_plane_addr_conut++;
  }

  if (valid_plane_addr_conut > 1)
    return 1;
  else
    return 0;
}

#ifdef ENABLE_FPGA_VERIFICATION
void SwMemsetBuffer(const struct SwParames *sw_parames, struct DWLDecBuffers *buffers) {
  u32 j, k;
  const PpUnitIntConfig *ppu_cfg = sw_parames->ppu_cfg;
  u32 init_value = sw_parames->init_value;

  for (j=0; j<DEC_MAX_OUT_COUNT; j++) {
    if (!ppu_cfg[j].enabled) continue;
    for (k=0; k<MAX_OUTPUT_PLANE; k++) {
      if (buffers->dec_buffer[j][k].virtual_address != NULL)
        DWLLinearMemset(sw_parames->dwl, &buffers->dec_buffer[j][k], 0, init_value, buffers->dec_buffer[j][k].size);
    }
  }
}
#endif

u32 CheckOutBufMatch(u32 pp_enabled, void *src_buffers, u32 buffer_num, struct DWLLinearMem *dst_addr) {
  u32 matched = 0;
  if (pp_enabled) {
    matched = SwFindBufferIdByAddr((struct DWLDecBuffers *)src_buffers, buffer_num, dst_addr);
    matched = (matched == 0xFFFF ? 0 : 1);
  }
  else
    matched = DWL_DEVMEM_COMPARE_V2(*(struct DWLLinearMem *)src_buffers, *dst_addr);

  return matched;
}

#ifdef STACK_STAT
u32 *stack_top = NULL;
u32 stack_size = 0;

#define STACK_STAT_SIZE (100*1024)
#define MAGICWORD 0xDEADBEEF

void StackStatInit() {
  int i;
  u32 *p;

  if (stack_top == NULL)
  	stack_top = (u32 *)alloca(STACK_STAT_SIZE);  //cppcheck-suppress allocaCalled

  for (i = 0, p = stack_top; i < STACK_STAT_SIZE/sizeof(u32); i++) {
    *p++ = MAGICWORD;
  }
}

void StackConsumption(const char *func) {
  int i;
  u32 *p;
  u32 ss;

  for (i = 0, p = stack_top; i < STACK_STAT_SIZE/sizeof(u32); i++) {
    if (*p == MAGICWORD) {
      p++;
    } else {
      ss = STACK_STAT_SIZE - (p - stack_top) * sizeof(u32);
      if (ss > stack_size)
        stack_size = ss;
      break;
    }
  }

  printf("%s Stack Size: %u bytes(%uKbytes)\n", func, stack_size, stack_size / 1024);
}
#endif

u32 SwGetCores(u32 core_mask) {
  u32 i, n_cores = 0;

  for (i=0; i<8*sizeof(u32); i++) {
    if ((core_mask >> i) & 0x1)
      n_cores++;
  }

  return n_cores;
}

u32 SwGetCoreMaskByFeature(const void *dwl_inst, enum FEATURE feature) {
  u32 i, core_mask = 0;
  const struct DecHwFeatures *hw_feature = NULL;
  u32 n_cores = DWLReadAsicCoreCount(dwl_inst);

#define SET_CORE_MASK(format)     if (hw_feature->format##_support) \
                                    core_mask |= 1 << i;
  for (i=0; i<n_cores; i++) {
    hw_feature = DWLGetHwFeaturesByID(dwl_inst, i);

    switch (feature) {
      case VSI_HEVC:           SET_CORE_MASK(hevc);             break;
      case VSI_HEVC_MAIN10:    SET_CORE_MASK(hevc_main10);      break;
      // case VSI_HEVC_422_INTRA: SET_CORE_MASK(hevc_422_intra);   break;
      case VSI_VP9:            SET_CORE_MASK(vp9);              break;
      case VSI_VP9_PROFILE2:   SET_CORE_MASK(vp9_profile2);     break;
      case VSI_H264:           SET_CORE_MASK(h264);             break;
      case VSI_H264_ADV:       SET_CORE_MASK(h264_adv);         break;
      case VSI_H264_HIGH10:    SET_CORE_MASK(h264_high10);      break;
      // case VSI_H264_422_INTRA: SET_CORE_MASK(h264_422_intra);   break;
      case VSI_CUSTOM_MPEG4:   SET_CORE_MASK(custom_mpeg4);     break;
      case VSI_SORENSON_SPARK: SET_CORE_MASK(sorenson_spark);   break;
      case VSI_VP8:            SET_CORE_MASK(vp8);              break;
      case VSI_WEBP:           SET_CORE_MASK(webp);             break;
      case VSI_VP7:            SET_CORE_MASK(vp7);              break;
      case VSI_AVS2:           SET_CORE_MASK(avs2);             break;
      case VSI_AVS2_MAIN10:    SET_CORE_MASK(avs2_main10);      break;
      case VSI_RFC:            SET_CORE_MASK(rfc);              break;
      case VSI_QP_DUMP:        SET_CORE_MASK(qp_dump);          break;
      case VSI_LOW_LATENCY:    SET_CORE_MASK(low_latency_mode); break;
      case VSI_VP9_HW_PROB:    SET_CORE_MASK(vp9_hw_prob);      break;
      default: break;
    }
  }

  return core_mask;
}

/* accordint to core_mask, get the fisrt hw_feature that support the client_type */
const struct DecHwFeatures *SwGetHwFeature(const void *dwl_inst, u32 core_mask, enum DWLClientType client_type) {
  const void *hw_feature = NULL;
  u32 i, n_cores;

  if (core_mask) {
    n_cores = DWLReadAsicCoreCount(dwl_inst);
    for (i=0; i<n_cores; i++) {
      if ((core_mask >> i) & 0x1)
        break;
    }
    if (i == n_cores) {
      APITRACEERR("%s", "SwGetHwFeature# error misc_ctrl, please fix.\n");
      ASSERT(0);
    }
    else
      hw_feature = DWLGetHwFeaturesByID(dwl_inst, i);
  }

  if (!hw_feature)
    hw_feature = DWLGetHwFeaturesByClientType(dwl_inst, client_type, &core_mask);

  ASSERT(hw_feature);

#ifdef ASIC_TRACE_SUPPORT
  APITRACEERR("SwGetHwFeature# max_regs %d\n", ((struct DecHwFeatures *)hw_feature)->max_regs);
#endif

  return hw_feature;
}

void SwSetHwFeatures(const void *dwl_inst, u32 core_mask, const struct DecHwFeatures *hw_features[]) {
  u32 i = 0;
  u32 n_cores = DWLReadAsicCoreCount(dwl_inst);

  for (i=0; i<n_cores; i++) {
    if ((core_mask >> i) & 0x1)
      hw_features[i] = DWLGetHwFeaturesByID(dwl_inst, i);
  }
}

static void SwGetMaxWxHByClientType(const struct DecHwFeatures *hw_feature,
                                    enum DWLClientType client_type, u32 align,
                                    u32 *max_width, u32 *max_height) {
#define SET_MAX_WXH(format)  *max_width = hw_feature->format##_max_dec_pic_width; \
                             *max_height = hw_feature->format##_max_dec_pic_height;
  switch (client_type) {
    case DWL_CLIENT_TYPE_H264_DEC: SET_MAX_WXH(h264);   break;
    case DWL_CLIENT_TYPE_MPEG4_DEC: SET_MAX_WXH(mpeg4); break;
    case DWL_CLIENT_TYPE_JPEG_DEC:
      *max_width = hw_feature->img_max_dec_width;
      *max_height = hw_feature->img_max_dec_height;     break;
    case DWL_CLIENT_TYPE_VC1_DEC:   SET_MAX_WXH(vc1);   break;
    case DWL_CLIENT_TYPE_MPEG2_DEC: SET_MAX_WXH(mpeg2); break;
    case DWL_CLIENT_TYPE_VP6_DEC:   SET_MAX_WXH(vp6);   break;
    case DWL_CLIENT_TYPE_AVS_DEC:   SET_MAX_WXH(avs);   break;
    case DWL_CLIENT_TYPE_RV_DEC:    SET_MAX_WXH(rv);    break;
    case DWL_CLIENT_TYPE_VP8_DEC:   SET_MAX_WXH(vp8);   break;
    case DWL_CLIENT_TYPE_VP9_DEC:   SET_MAX_WXH(vp9);   break;
    case DWL_CLIENT_TYPE_HEVC_DEC:  SET_MAX_WXH(hevc);  break;
    case DWL_CLIENT_TYPE_AVS2_DEC:  SET_MAX_WXH(hevc);  break;//FIXME, DecHwFeaturesno avs2 max WxH filed?
    case DWL_CLIENT_TYPE_AV1_DEC:   SET_MAX_WXH(av1);   break;
    case DWL_CLIENT_TYPE_VVC_DEC:   SET_MAX_WXH(vvc);   break;
    case DWL_CLIENT_TYPE_AVS3_DEC:  SET_MAX_WXH(avs3);   break;
    case DWL_CLIENT_TYPE_PNG_DEC:
      *max_width = hw_feature->png_max_dec_width;
      *max_height = hw_feature->png_max_dec_height;     break;
    default: break;
  }
  *max_width = NEXT_MULTIPLE(*max_width, align);
  *max_height = NEXT_MULTIPLE(*max_height, align);
}

/* align 1 means intra of DWL_CLIENT_TYPE_VP8_DEC or others video fromats */
/* align 0 means thumb of DWL_CLIENT_TYPE_JPEG_DEC */
u32 SwAdjustCoreMaskByWxH(const void *dwl_inst, u32 width, u32 height, u32 align_m,
                          enum DWLClientType client_type, u32 *core_mask,
                          const struct DecHwFeatures **cur_hw_feature) {
  u32 i, tmp = 0, max_width = 0, max_height = 0, min_width, min_height;
  u32 bak_non_core_mask, is_check_WxH = 0, is_intra = 0;
  const struct DecHwFeatures *hw_feature = NULL;
  u32 n_cores = DWLReadAsicCoreCount(dwl_inst);
  u32 dec_webp = (align_m >> 31);
  u32 align = align_m & 0x7FFFFFFF;

  if (LEGACY_CLEINT_TYPE(client_type)) {
    min_width= MIN_PIC_WIDTH_G1;
    min_height = MIN_PIC_HEIGHT_G1;
  }
  else if (client_type == DWL_CLIENT_TYPE_AVS2_DEC) {
    min_width = MIN_PIC_WIDTH_AVS2;
    min_height = MIN_PIC_HEIGHT_AVS2;
  }
  else if (client_type == DWL_CLIENT_TYPE_H264_DEC) {
    min_width = MIN_PIC_WIDTH_H264;
    min_height = MIN_PIC_HEIGHT_H264;
  }
  else {
    min_width = MIN_PIC_WIDTH;
    min_height = MIN_PIC_HEIGHT;
  }

  if (client_type == DWL_CLIENT_TYPE_JPEG_DEC ||
      client_type == DWL_CLIENT_TYPE_VP6_DEC) {
    is_check_WxH = 1;
  }

  if (client_type == DWL_CLIENT_TYPE_VP8_DEC) {
    is_intra = align;
    align = 1;
  }

  /* check picture size (minimum defined in decapicommon.h) */
  for (i=0; i<n_cores; i++) {
    hw_feature = DWLGetHwFeaturesByID(dwl_inst, i);
    if (client_type == DWL_CLIENT_TYPE_JPEG_DEC && align == 0) {
      max_width = JPEGDEC_MAX_WIDTH_TN;
      max_height = JPEGDEC_MAX_HEIGHT_TN;
    }
    else
      SwGetMaxWxHByClientType(hw_feature, client_type, align, &max_width, &max_height);

    if (dec_webp) {
      max_width = 16384;
      max_height = 16384;
    }
    if (width <= max_width && height <= max_height &&
        width >= min_width && height >= min_height &&
        ((is_check_WxH | is_intra) ? width*height <= max_width*max_height : 1) &&
        (is_intra ? hw_feature->webp_support : 1))
      tmp |= 1 << i;
  }

  bak_non_core_mask = NON_CORE_MASK(*core_mask);
  if (CORE_MASK(*core_mask))
    *core_mask &= tmp;
  else
    *core_mask = tmp;

  if (CORE_MASK(*core_mask) == 0) {
    APITRACEERR("SwAdjustCoreMaskByWxH# no any core support WxH(%d, %d) for dec type %d\n",
                width, height, client_type);
    return 1;
  }

  *core_mask |= bak_non_core_mask;
  *cur_hw_feature = SwGetHwFeature(dwl_inst, CORE_MASK(*core_mask), client_type);

  return 0;
}

void SwCommonInfoInit(const void *dwl, u32 misc_ctrl, enum DWLClientType client_type, u32 dec_mode,
                      u32 *core_mask, u32 *secure_mode, const struct DecHwFeatures *hw_features[],
                      const struct DecHwFeatures **cur_hw_feature) {
   ASSERT(core_mask);
   ASSERT(secure_mode);
   ASSERT(cur_hw_feature);

  *secure_mode = (dec_mode & DEC_SECURITY) ? 1 : 0;
  SET_CLIENT_TYPE(*core_mask, client_type);
  SET_SECURE_MODE(*core_mask , *secure_mode);
  SwSetCommonInfo(dwl, misc_ctrl, client_type, core_mask, hw_features, cur_hw_feature);
}

enum DecRet SwSetCommonInfo(const void *dwl, u32 misc_ctrl, enum DWLClientType client_type, u32 *core_mask,
      const struct DecHwFeatures *hw_features[], const struct DecHwFeatures **cur_hw_feature) {
  u32 bak_non_core_mask = 0;

  if (CORE_MASK(misc_ctrl) != 0) {
    bak_non_core_mask = NON_CORE_MASK(*core_mask);
    *core_mask = CORE_MASK(misc_ctrl);
    *core_mask |= bak_non_core_mask;
  }
  SwSetHwFeatures(dwl, CORE_MASK(misc_ctrl), hw_features);
  *cur_hw_feature = SwGetHwFeature(dwl, CORE_MASK(*core_mask),  client_type);
  if (*cur_hw_feature == NULL) {
    APITRACEDEBUG("%s","SwSetCommonInfo# not found any hw_feature.\n");
    return DEC_PARAM_ERROR;
  }

  return DEC_OK;
}

/* DecInit */
void SwSetCommonCfg(u32 misc_ctrl, CommonCfg *common_cfg) {
  common_cfg->idr_out_imd = G_IDR_OUT_IMD(misc_ctrl);
  common_cfg->add_buf_dyn = G_ADD_BUF_DYN(misc_ctrl);
  common_cfg->picture_discard = G_PIC_DISCARD(misc_ctrl);

}

/* SetInfo */
void SwUpdateCommonCfg(u32 misc_ctrl, CommonCfg *common_cfg) {
  common_cfg->use_separated_pp = G_SEPERATED_PP(misc_ctrl);
}

enum DecRet SwSetLowLatencyInfo(const void *dwl, enum DWLClientType client_type,
                                struct LLStrmInfo *llstrminfo, u32 *regs) {
#ifndef ASIC_TRACE_SUPPORT
  llstrminfo->low_latency = 1;
  llstrminfo->updated_reg = 0;
#endif

#ifdef MODEL_SIMULATION
  if (client_type == DWL_CLIENT_TYPE_JPEG_DEC ||
      client_type == DWL_CLIENT_TYPE_PNG_DEC)
    llstrminfo->low_latency = 0;
#endif
// if (client_type == DWL_CLIENT_TYPE_AV1_DEC)
//   dec_cont->low_latency = 1;

  llstrminfo->dwl_inst = dwl;
  /* Set flag for using ddr_low_latency model and set pollmode and polltime swregs */
  SetDecRegister(regs, HWIF_DEC_MC_POLLMODE, 0);
  SetDecRegister(regs, HWIF_DEC_MC_POLLTIME, 0);

  SetDecRegister(regs, HWIF_STREAM_STATUS_EXT_BUFFER_E, 1);
  SetDecRegister(regs, HWIF_BUFFER_EMPTY_INT_E, 0);
  SetDecRegister(regs, HWIF_BLOCK_BUFFER_MODE_E, 1);
  llstrminfo->strm_status_in_buffer = 1;

  /* low latency: Allocate buffer for ddr_low_latency model */
  if (llstrminfo->strm_status.virtual_address == NULL) {
    llstrminfo->strm_status.mem_type = DWL_MEM_TYPE_SYNC_HOST_TO_DEVICE | DWL_MEM_TYPE_VPU_WORKING;
    SET_MEM_USAGE(llstrminfo->strm_status.mem_type, DWL_MEM_USAGE_IN_STRM_STATUS, 1);
    if(DWLMallocLinear(dwl, 16 * 4, &llstrminfo->strm_status)) {
      APITRACEDEBUG("%s","SwSetLowLatencyInfo# strm_status malloc linear failed.\n");
      return DEC_ERROR;
    }
    llstrminfo->strm_status_addr = llstrminfo->strm_status.virtual_address;
  }

  return DEC_OK;
}

void SwUpdateLowLatencyInfo(const void *dwl, const struct DecHwFeatures *hw_feature, addr_t strm_bus_address, u32 strm_len,
                            enum DWLClientType client_type, struct LLStrmInfo *llstrminfo, u32 *regs) {
  if(llstrminfo->strm_status_in_buffer == 1) {
    *llstrminfo->strm_status_addr = 0;
    if (client_type == DWL_CLIENT_TYPE_AVS2_DEC ||
        client_type == DWL_CLIENT_TYPE_AVS3_DEC ||
        client_type == DWL_CLIENT_TYPE_JPEG_DEC ||
        client_type == DWL_CLIENT_TYPE_PNG_DEC) {
      SET_ADDR_REG(regs, HWIF_LG_STREAM_STATUS_BASE, llstrminfo->strm_status.bus_address);
    }

    if (client_type == DWL_CLIENT_TYPE_JPEG_DEC ||
        client_type == DWL_CLIENT_TYPE_PNG_DEC) {
      llstrminfo->strm_status_addr = llstrminfo->strm_status.virtual_address;
      *llstrminfo->strm_status_addr = 0;
    }

    DWLSyncMemData(dwl, &llstrminfo->strm_status, 0, 16 * 4, HOST_TO_DEVICE);
  }
  llstrminfo->first_update = 1;
  llstrminfo->update_reg_flag = 1;

  if (client_type == DWL_CLIENT_TYPE_AVS2_DEC ||
      client_type == DWL_CLIENT_TYPE_AVS3_DEC ||
      client_type == DWL_CLIENT_TYPE_H264_DEC ||
      client_type == DWL_CLIENT_TYPE_HEVC_DEC ||
      client_type == DWL_CLIENT_TYPE_VP9_DEC ||
      client_type == DWL_CLIENT_TYPE_VVC_DEC ||
      client_type == DWL_CLIENT_TYPE_JPEG_DEC ||
      client_type == DWL_CLIENT_TYPE_PNG_DEC) {
    llstrminfo->ll_strm_bus_address = strm_bus_address;
    llstrminfo->ll_strm_len = strm_len;
    SetDecRegister(regs, HWIF_STREAM_LEN, 0);
    SetDecRegister(regs, HWIF_LAST_BUFFER_E, 0);
  }
}

void SwReleaseLowLatencyInfo(const void *dwl, struct LLStrmInfo *llstrminfo) {
  /* low latency: stream status in ddr buffer */
  if (llstrminfo->strm_status.virtual_address != NULL) {
    DWLFreeLinear(dwl, &llstrminfo->strm_status);
    llstrminfo->strm_status.virtual_address = NULL;
  }
}

/* set irq as normal or abnormal for vcmd mode */
void SwSetDecIRQAttr(u32 *regs, u32 vcmd_version_flag) {
  u32 dec_mode = (regs[3] >> 27) & 0x1F;

  regs[297] = 0x402000; //bus error
  /* Set the buffer empty interrupt to abnormal interrupt. */
  if (vcmd_version_flag &&
      ((dec_mode == DEC_MODE_HEVC) || (dec_mode == DEC_MODE_H264) ||
       (dec_mode == DEC_MODE_H264_H10P) ||
       (dec_mode == DEC_MODE_VVC) ||
       (dec_mode == DEC_MODE_AVS3))) {
    regs[297] |= 0x404000;
  } else {
  	regs[3] |= 0x40;
  }
}

void SwSetCacheShaperReg(u32 *regs, u32 sim_mc) {
  u32 cache_e, allow_intrabc = 0, muti_core_support;
  u32 dec_mode = (regs[3] >> 27) & 0x1F;

#ifdef ASIC_TRACE_SUPPORT
  muti_core_support = sim_mc;
#else
  muti_core_support = (regs[58] >> 30) & 0x1;
#endif

  if (dec_mode == DEC_MODE_AV1)
    allow_intrabc  = ((regs[5] >> 4) & 0x01);

  if (dec_mode == DEC_MODE_JPEG)
    cache_e = 0x0;
  else if (dec_mode < DEC_MODE_HEVC)
    cache_e = 0xa;  /* g1 */
  else if (dec_mode ==  DEC_MODE_AV1 && allow_intrabc) {
    cache_e = 0x188;
  } else {
    if (muti_core_support) {
      if (dec_mode == DEC_MODE_VP9 || dec_mode == DEC_MODE_AV1)
        cache_e = 0x1812;  /* for tile-based multicore like VP9/AV1,
                              ref data/table is cachable for multicore */
      else
        cache_e= 0x18a;
    } else if (dec_mode == DEC_MODE_VVC || dec_mode == DEC_MODE_AVS3)
      cache_e = 0x1802; /* DMV is uncachable for vvc */
    else
      cache_e = 0x1812;
  }
  regs[317] = cache_e;

  /*recon_shaper*/
  if (allow_intrabc)
    regs[3] &= 0xFFFFFFF7;
  else
    regs[3] |= 0x8;
}

void ClearSwIRQ(u32 *regs) {
  SetDecRegister(regs, HWIF_DEC_IRQ_STAT, 0);
  SetDecRegister(regs, HWIF_DEC_IRQ, 0);
  SetDecRegister(regs, HWIF_DEC_E, 0);
}

/* core_id is cmdbuf_id if vcmd_used */
void SwAbortSliceDec(const void *dwl, const void *dec_inst, u32 *regs,
                      u32 vcmd_used, u32 core_id) {
  if (!vcmd_used) {
    ClearSwIRQ(regs);
    DWLReleaseHw(dwl, core_id); /* release HW lock */
  }
  else {
    DWLAbortCmdbuf(dwl, core_id);
    DWLWaitCmdbufsDone(dwl, dec_inst);
    ClearSwIRQ(regs);
  }
}

void SwAbortDec(const void *dwl, struct DWLReqInfo *info, u32 vcmd_used,
                  u32 n_cores_available, void *cmd_info) {
  if (!vcmd_used) {
    DWLWaitOwnerDone(dwl, info->owner);
  }
  else {
    /* only drop current instance cmd job */
    DWLDropCmdbufs(dwl, info->owner);
    DWLWaitCmdbufsDone(dwl, info->owner);
  }
}

u32 SwCheckSliceStatus(const void *dwl, u32 vcmd_used, u32 id) {
  u32 exe_status = 0;

#ifndef MODEL_SIMULATION
  i32 ret = 0;

  if (vcmd_used) {
    exe_status = DWLGetCmdBufExeStatus(dwl, id);

    ret = DWLParseCmdBufExeStatus(dwl, exe_status);
    if (ret == DWL_SLICE_DECODING_ABORTED) {
      DWLWaitCmdBufReady(dwl, id);
      DWLReleaseCmdBuf(dwl, id);

      exe_status = 1;
    }
  }
#endif

  return exe_status;
}

u32 SwCheckClientType(enum DWLClientType client_type) {
  u32 ret = 0;

  switch (client_type) {
    case DWL_CLIENT_TYPE_H264_DEC:
      DTRACE_I("%s","DWL initialized by an H264 decoder instance...\n");
      break;
    case DWL_CLIENT_TYPE_MPEG4_DEC:
      DTRACE_I("%s","DWL initialized by an MPEG4 decoder instance...\n");
      break;
    case DWL_CLIENT_TYPE_JPEG_DEC:
      DTRACE_I("%s","DWL initialized by a JPEG decoder instance...\n");
      break;
    case DWL_CLIENT_TYPE_VC1_DEC:
      DTRACE_I("%s","DWL initialized by an VC1 decoder instance...\n");
      break;
    case DWL_CLIENT_TYPE_MPEG2_DEC:
      DTRACE_I("%s","DWL initialized by an MPEG2 decoder instance...\n");
      break;
    case DWL_CLIENT_TYPE_AVS_DEC:
      DTRACE_I("%s","DWL initialized by an AVS decoder instance...\n");
      break;
    case DWL_CLIENT_TYPE_RV_DEC:
      DTRACE_I("%s","DWL initialized by an RV decoder instance...\n");
      break;
    case DWL_CLIENT_TYPE_VP6_DEC:
      DTRACE_I("%s","DWL initialized by a VP6 decoder instance...\n");
      break;
    case DWL_CLIENT_TYPE_VP8_DEC:
      DTRACE_I("%s","DWL initialized by a VP8 decoder instance...\n");
      break;
    case DWL_CLIENT_TYPE_HEVC_DEC:
      DTRACE_I("%s","DWL initialized by an HEVC decoder instance...\n");
      break;
    case DWL_CLIENT_TYPE_VP9_DEC:
      DTRACE_I("%s","DWL initialized by a VP9 decoder instance...\n");
      break;
    case DWL_CLIENT_TYPE_AVS2_DEC:
      DTRACE_I("%s","DWL initialized by a AVS2 decoder instance...\n");
      break;
    case DWL_CLIENT_TYPE_AVS3_DEC:
      DTRACE_I("%s","DWL initialized by a AVS3 decoder instance...\n");
      break;
    case DWL_CLIENT_TYPE_AV1_DEC:
      DTRACE_I("%s","DWL initialized by a AV1 decoder instance...\n");
      break;
    case DWL_CLIENT_TYPE_VVC_DEC:
      DTRACE_I("%s","DWL initialized by a VVC decoder instance...\n");
      break;
    case DWL_CLIENT_TYPE_ST_PP:
      DTRACE_I("%s","DWL initialized by a standalone PP instance...\n");
      break;
    case DWL_CLIENT_TYPE_PNG_DEC:
      DTRACE_I("%s","DWL initialized by a PNG decoder instance...\n");
      break;
    default:
      DTRACE_E("%s","ERROR: DWL client type has to be always specified!\n");
      ret = 1;
  }

  return ret;
}

void SwSaveBuf(struct DWLLinearMem *info, struct DWLLinearMem **mem_infos, u32 *slots) {
  u32 i = 0;
  struct DWLLinearMem *tmp = NULL;

  if (*mem_infos == NULL) {
    *mem_infos = (struct DWLLinearMem *)DWLcalloc(1, BUF_SLOTS * sizeof(struct DWLLinearMem));
    if (*mem_infos == NULL) {
      APITRACEERR("%s", "shouldn't happen!!!!\n");
      return;
    }
    *slots = BUF_SLOTS;
  }
  tmp = *mem_infos;

  for (i=0; i<*slots; i++) {
    if (!DWL_DEVMEM_VAILD(tmp[i])) {
      tmp[i] = *info;
      break;
    }
  }
  if (i == *slots) {
    u32 old_slots = *slots;
    u32 new_slots = *slots + (*slots / 2);
    tmp = (struct DWLLinearMem *)DWLcalloc(1, new_slots * sizeof(struct DWLLinearMem));
    if (tmp == NULL) {
      APITRACEERR("%s", "ERROR: DWLcalloc failed.\n");
      return;
    }
    DWLmemcpy(tmp, *mem_infos, old_slots * sizeof(struct DWLLinearMem));
    DWLfree(*mem_infos);
    *mem_infos = tmp;
    *slots = new_slots;
    APITRACE("increase the space of mem_infos slots %d -> %d\n", old_slots, new_slots);

    (*mem_infos)[old_slots] = *info;
  }
}

void SwRecycleBuf(struct DWLLinearMem *info, struct DWLLinearMem *mem_infos, u32 slots) {
  u32 i;

  for (i=0; i<slots; i++) {
    if (DWL_DEVMEM_COMPARE_V2(mem_infos[i], *info)) {
      DWLmemset(&mem_infos[i], 0, sizeof(struct DWLLinearMem));
      break;
    }
  }
  if (i == slots)
    APITRACEERR("not found %p from mem_infos, shouldn't happen!!!!\n", (void *)(size_t)info->bus_address);
}

u32 SwGetBufByAddr(struct DWLLinearMem *mem_infos, u32 slots, addr_t addr) {
  u32 i = 0;

  for (i=0; i<slots; i++) {
    if (DWL_DEVMEM_IN_RANGE(mem_infos[i], addr))
      break;
  }
  if (i == slots) {
    APITRACEERR("not found %p from mem_infos, not continue log!!!!\n", (void *)(size_t)addr);
    return slots;
  }

  return i;
}

addr_t GetRegsBufAddr(u32 *regs, u32 reg_lsb_idx, u32 reg_msb_idx, int flag) {
  addr_t addr_lsb = 0, addr_msb = 0, buf_addr = 0;
  addr_lsb = regs[reg_lsb_idx];
  addr_msb = regs[reg_msb_idx];

  if (flag == 1 && addr_msb) {
    addr_msb = addr_msb^ENCRYP_CODE;
  }

  if (sizeof(addr_t) == 8)
    buf_addr = (addr_t)((addr_msb << 32) | addr_lsb);
  else
    buf_addr = (addr_t)addr_lsb;
  return buf_addr;
}

void SwLogRegInfo(const struct DecHwFeatures *hw_feature, u32 *regs, int stage, struct Reg_Info *reg_info, u32 pic_id) {
  char file_name[100];
  char comment[80];
  char fmt[] = "%s %08x/%s \n";
  FILE *fd;
  u32 i, reg_num = MAX_REG_COUNT;

  if (hw_feature)
    reg_num = hw_feature->max_regs;

  ASSERT(reg_info);
  sprintf(file_name,"pic%u.reg", pic_id);
  if (stage == IN_STAGE)
    fd = fopen(file_name,"w");
  else
    fd = fopen(file_name,"a+");
  if (fd == NULL) {
    printf("open %s faild\n", file_name);
    return;
  }
  if (stage == OUT_STAGE) fprintf(fd, " \n");

  for (i=0;i<reg_num;i++) {
    if (reg_info[i].buf_len != 0)
      sprintf(comment, "%08X %s", regs[i], "(LSB ADDR)");
    else if (reg_info[i+1].buf_len != 0)
      sprintf(comment, "%08X %s", regs[i], "(MSB ADDR)");
    else
      sprintf(comment, "%08X", regs[i]);
    if (stage == IN_STAGE)
      fprintf(fd, fmt, "W", i, comment);
    else
      fprintf(fd, fmt, "R", i, comment);
  }
  // if (stage == OUT_STAGE) (*pic_id)++;
  fflush(fd);
  fclose(fd);
}

void SwLogBufInfo(u8 *buf, u32 buf_len, u32 reg_lsb_idx, u32 reg_msb_idx, int stage, u32 pic_id) {
  char file_name[100];
  FILE *fd;
  char attr[10];
  // static u32 last_reg_lsb_idx = 0;

  if (stage == IN_STAGE)
    sprintf(attr, "in");
  else
    sprintf(attr, "out");
  // if (stage == IN_STAGE && last_reg_lsb_idx > reg_lsb_idx)
  //   pic_id++;
  sprintf(file_name,"pic%u_reg%u_reg%u_%s.bin", pic_id, reg_msb_idx, reg_lsb_idx, attr);
  fd = fopen(file_name,"wb+");
  if (fd == NULL) {
    printf("open %s faild\n", file_name);
    return;
  }
  fwrite(buf, 1, buf_len, fd);
  // if (stage == IN_STAGE)
  //   last_reg_lsb_idx = reg_lsb_idx;
  fclose(fd);
}