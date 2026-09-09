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

#include "commonfunction.h"
#include "dec_log.h"
#include "vpufeature.h"
#include "regdrv.h"
#include "deccfg.h"
#include "sw_debug.h"

void CommonSetM2MPComands(PpUnitIntConfig *ppu_cfg, struct M2MP_INFO vcmd_m2mp_data[MAX_VCMD_M2MP_NUM], addr_t base_bus_addr,
                          u32 pp_enabled, u32 use_rfc, u32 crop_step_rshift, u32 width, u32 height,
                          u32 bd, u32 mono_chrome, u32 align) {
  u32 srcskip;
  u32 pic_size, size = 0;
  u32 lu_w =0, lu_h = 0, ch_w = 0, ch_h = 0, pic_width = 0, pic_height = 0;
  u32 pic_stride = 0, pic_stride_ch = 0;
  enum DecPictureFormat format;
  u32 i = 0, j = 0, count = 0;
  addr_t luma_addr = 0, chroma_addr = 0;
  u32 lumasum_enable = 0;

  if(!pp_enabled) {
    if(use_rfc) {
      lu_w = NEXT_MULTIPLE(width * bd, 8) / 8;
      lu_h = height / 8;
      pic_stride = NEXT_MULTIPLE(8 * width * bd, ALIGN(align) * 8) / 8;
      luma_addr = base_bus_addr;
      if(!mono_chrome) {
        ch_w = lu_w / 2;
        ch_h = lu_h;
        pic_stride_ch = NEXT_MULTIPLE(4 * width * bd, ALIGN(align) * 8) / 8;
        chroma_addr = luma_addr + pic_stride * lu_h;
      }
    } else {
      luma_addr = base_bus_addr;
#ifdef TILE8X8
      pic_width = NEXT_MULTIPLE(width, 8) * 8;
      lu_w = NEXT_MULTIPLE(pic_width * bd, 8) / 8;
      lu_h = NEXT_MULTIPLE(height, 8) / 8;
      pic_stride = NEXT_MULTIPLE(8 * width * bd, ALIGN(align) * 8) / 8;

      if(!mono_chrome) {
        pic_width = NEXT_MULTIPLE(width, 4) * 4;
        ch_w = NEXT_MULTIPLE(pic_width * bd, 8) / 8;
        ch_h = NEXT_MULTIPLE(height / 2, 4) / 4;
        pic_stride_ch = NEXT_MULTIPLE(4 * width * bd, ALIGN(align) * 8) / 8;
        chroma_addr = base_bus_addr + pic_stride * height / 8;
      }
#else
      pic_width = NEXT_MULTIPLE(width, 4) * 4;
      lu_w = NEXT_MULTIPLE(pic_width * bd, 8) / 8;
      lu_h = NEXT_MULTIPLE(height, 4) / 4;
      pic_stride = pic_stride_ch = NEXT_MULTIPLE(4 * width * bd, ALIGN(align) * 8) / 8;
      if(!mono_chrome) {
        ch_w = lu_w;
        ch_h = NEXT_MULTIPLE(height / 2, 4) / 4;
        chroma_addr = base_bus_addr + pic_stride * height / 4;
      }
#endif
    }
    for(j = 0; j < (mono_chrome? 1 : 2); j++) {
      if(j) count++;
      if(!j) {
        ASSERT(pic_stride >= lu_w);
        srcskip = pic_stride - lu_w;
      } else if (!mono_chrome && j) {
        ASSERT(pic_stride_ch >= ch_w);
        srcskip = pic_stride_ch - ch_w;
      }
      pic_size = j ? (ch_w * ch_h) : (lu_w * lu_h);
      vcmd_m2mp_data[count].m2mp_width = srcskip ? (j ? ch_w : lu_w) : (pic_size & 0xFFFF);
      vcmd_m2mp_data[count].m2mp_height = srcskip ? (j ? ch_h : lu_h) : ((pic_size >> 16) & 0xFFFF);
      vcmd_m2mp_data[count].m2mp_size = size;
      vcmd_m2mp_data[count].m2mp_src_addr = j ? (addr_t)chroma_addr: (addr_t)luma_addr;
      vcmd_m2mp_data[count].m2mp_srcskip = srcskip;
      vcmd_m2mp_data[count].m2mp_mode = 1;
    }
    return;
  }

  for(i = 0; i < DEC_MAX_OUT_COUNT; i++, ppu_cfg++) {
    if(!ppu_cfg->enabled) continue;
    if(i) count++;
    lumasum_enable = 0;
    format = TransUnitConfig2Format(ppu_cfg);
    if(!crop_step_rshift) {
      if (ppu_cfg->crop2.enabled) {
        pic_width = ppu_cfg->crop2.width;
        pic_height = ppu_cfg->crop2.height;
      } else {
        pic_width = (ppu_cfg->scale.width / 2) << 1;
        pic_height = (ppu_cfg->scale.height / 2) << 1;
      }
    } else {
      if (ppu_cfg->crop2.enabled) {
        pic_width = (ppu_cfg->crop2.width / 2) << 1;
        pic_height = (ppu_cfg->crop2.height / 2) << 1;
      } else {
        pic_width = (ppu_cfg->scale.width / 2) << 1;
        pic_height = (ppu_cfg->scale.height / 2) << 1;
      }
    }
    luma_addr = base_bus_addr + ppu_cfg->luma_offset;
    if(!ppu_cfg->monochrome && !IS_PIC_RGB(format) && !IS_PIC_422PACKED(format)
      && !IS_PIC_TILED16x16(format))
      chroma_addr = base_bus_addr + ppu_cfg->chroma_offset;
    else
      chroma_addr = 0;
    if(IS_PIC_TILED4x4(format)) {
      size = (IS_PIC_8BIT(format) || IS_PIC_10BIT(format)) ? 0 : 1;
      if(IS_PIC_10BIT(format)) {
        pic_width = NEXT_MULTIPLE(pic_width, 4) * 4;
        lu_w = NEXT_MULTIPLE(pic_width * 10, 8) / 8;
      } else
        lu_w = NEXT_MULTIPLE(pic_width, 4) * 4;
      lu_h = NEXT_MULTIPLE(pic_height, 4) / 4;

      if(!(IS_PIC_MONOCHROME(format)) && chroma_addr) {
        ch_w = NEXT_MULTIPLE(pic_width, 4) * 4;
        ch_h = NEXT_MULTIPLE(pic_height / 2, 4) / 4;
      }
    } else if(IS_PIC_REF_TILED8x8(format)) {
      size = (IS_PIC_8BIT(format) || IS_PIC_10BIT(format)) ? 0 : 1;
      if(IS_PIC_10BIT(format)) {
        pic_width = NEXT_MULTIPLE(pic_width, 8) * 8;
        lu_w = NEXT_MULTIPLE(pic_width * 10, 8) / 8;
      } else
        lu_w = NEXT_MULTIPLE(pic_width, 8) * 8;
      lu_h = NEXT_MULTIPLE(pic_height, 8) / 8;

      if(!(IS_PIC_MONOCHROME(format)) && chroma_addr) {
        ch_w = NEXT_MULTIPLE(pic_width, 4) * 4;
        ch_h = NEXT_MULTIPLE(pic_height / 2, 4) / 4;
      }
    } else if(IS_PIC_TILED8x8(format)) {
      size = IS_PIC_8BIT(format) ? 0 : 1;
      lu_w = NEXT_MULTIPLE(pic_width, 8) * 8;
      lu_h = NEXT_MULTIPLE(pic_height, 8) / 8;

      if(!(IS_PIC_MONOCHROME(format)) && chroma_addr) {
        ch_w = NEXT_MULTIPLE(pic_width, 4) * 4;
        ch_h = NEXT_MULTIPLE(pic_height / 2, 4) / 4;
      }
    } else if(IS_PIC_TILED16x16(format)) {
      size = IS_PIC_8BIT(format) ? 0 : 1;
      lu_w = NEXT_MULTIPLE(pic_width, 16) * 16 * 3 / 2;
      lu_h = NEXT_MULTIPLE(pic_height, 16)/ 16;

    } else if(IS_PIC_TILED128x2(format)) {
      size = IS_PIC_8BIT(format) ? 0 : 1;
      lu_w = NEXT_MULTIPLE(pic_width, 128) * 2;
      lu_h = pic_height / 2;

      if(!(IS_PIC_MONOCHROME(format)) && chroma_addr) {
        ch_w = NEXT_MULTIPLE(pic_width, 128) * 2;
        ch_h = pic_height / 4;
      }

    } else if(IS_PIC_RGB_TILED64x64(format)) {
      size = 2;
      lu_w = NEXT_MULTIPLE(pic_width, 64) * 64;
      lu_h = NEXT_MULTIPLE(pic_height, 64) / 64;

    } else if(IS_PIC_TILED64x64(format)) {
      size = 0;
      lu_w = NEXT_MULTIPLE(pic_width, 64) * 64;
      lu_h = NEXT_MULTIPLE(pic_height, 64) / 64;

      if(!(IS_PIC_MONOCHROME(format)) && chroma_addr) {
        ch_w = NEXT_MULTIPLE((pic_width + 1) / 2, 32) * 32 * 2; // Distinguish U and V
        ch_h = NEXT_MULTIPLE((pic_height + 1) / 2, 32) / 32;
      }
    } else if(IS_PIC_PACKED_RGB(format)) {
      if (IS_PIC_24BIT(format))
        size = 3;
      else if (IS_PIC_32BIT(format))
        size = 4;
      else if (IS_PIC_48BIT(format))
        size = 6;
      lu_w = pic_width * size;
      lu_h = pic_height;
      size = 0;

    } else if(IS_PIC_PLANAR_RGB(format)) {
      size = IS_PIC_8BIT(format) ? 0 : 1;
      lu_w = pic_width;
      lu_h = pic_height;

    } else if(IS_PIC_422PACKED(format)) {
      size = IS_PIC_8BIT(format) ? 0 : 1;
      lu_w = pic_width * 2;
      lu_h = pic_height;

    } else {
      /* 400/420/422*/
      /* luma */
      size = (IS_PIC_8BIT(format) || IS_PIC_1010(format) || IS_PIC_10BIT(format)) ? 0 : 1;
      if(IS_PIC_1010(format)) {
        lu_w = pic_width / 3 * 4 + NEXT_MULTIPLE(pic_width % 3 * 10, 8) / 8;
      } else if(IS_PIC_10BIT(format)) {
        lu_w = NEXT_MULTIPLE(pic_width * 10, 8) / 8;
      } else {
        lu_w = pic_width;
      }
	  lu_h = pic_height;
      /* chroma */
      if(!(IS_PIC_MONOCHROME(format)) && chroma_addr) {
        if(IS_PIC_PLANAR(format)) {
          if (IS_PIC_1010(format)) {
            if(IS_PIC_YCbCr444(format))
              ch_w = lu_w;
            else {
              ch_w = NEXT_MULTIPLE(pic_width, 2);
              ch_w = (ch_w / 3 * 4 + NEXT_MULTIPLE(ch_w % 3 * 10, 8) / 8 ) / 2;
            }
          } else {
            if(IS_PIC_YCbCr444(format))
              ch_w = lu_w;
            else
              ch_w = NEXT_MULTIPLE(pic_width, 2) / 2;
          }
        } else {
          if (IS_PIC_1010(format)) {
            if(IS_PIC_YCbCr444(format))
              ch_w = lu_w * 2;
            else {
              ch_w = NEXT_MULTIPLE(pic_width, 2);
              ch_w = ch_w / 3 * 4 + NEXT_MULTIPLE(ch_w % 3 * 10, 8) / 8;
            }
          } else {
            if(IS_PIC_YCbCr444(format))
              ch_w = lu_w * 2;
            else
              ch_w = NEXT_MULTIPLE(lu_w, 2);
          }
        }
        if(IS_PIC_YCbCr422(format) || IS_PIC_YCbCr444(format))
          ch_h = lu_h;
        else
          ch_h = NEXT_MULTIPLE(lu_h, 2) / 2;
      }
    }

/* luma */
    if (IS_PIC_DEC400(format) || IS_PIC_RGB_TILED64x64(format) || IS_PIC_TILED64x64(format) ||
      IS_PIC_PVFBC(format) || IS_PIC_PVFBC_BYPASS(format)) {
      srcskip = 0;
      size = 0;
      lu_w = ppu_cfg->ystride;
    }
    else {
      ASSERT(ppu_cfg->ystride >= (lu_w * (1 << size)));
      srcskip = ppu_cfg->ystride - lu_w * (1 << size);
      srcskip = srcskip >> size;
    }
    if (IS_PIC_PVFBC(format) || IS_PIC_PVFBC_BYPASS(format)) {
      pic_size = CalcOnePpUnitPVFBCPlane0Size(ppu_cfg);
    }
    else {
      pic_size = lu_w * lu_h;
    }
    //if pp2 luma sum
    if(ppu_cfg->luma_sum_e || ppu_cfg->hist_local_e || ppu_cfg->hist_global_e || ppu_cfg->hist_hsv_e)
    {
      srcskip = 0;
      size = 0;
      pic_size = 16 * 904;
      lumasum_enable = 1;
    }
    vcmd_m2mp_data[count].m2mp_width = srcskip ? lu_w : (pic_size & 0xFFFF);
    vcmd_m2mp_data[count].m2mp_height = srcskip ? lu_h : ((pic_size >> 16) & 0xFFFF);
    vcmd_m2mp_data[count].m2mp_size = size;
    vcmd_m2mp_data[count].m2mp_src_addr = (addr_t)luma_addr;
    vcmd_m2mp_data[count].m2mp_srcskip = srcskip;
    vcmd_m2mp_data[count].m2mp_mode = 1;
/* chroma */
    if(ch_h && ch_w && chroma_addr && !lumasum_enable) {
      if (IS_PIC_DEC400(format) || IS_PIC_RGB_TILED64x64(format) || IS_PIC_TILED64x64(format) ||
        IS_PIC_PVFBC(format) || IS_PIC_PVFBC_BYPASS(format)) {
        srcskip = 0;
        size = 0;
        ch_w = ppu_cfg->cstride;
      }
      else {
        ASSERT(ppu_cfg->ystride >= (ch_w * (1 << size)));
        srcskip = ppu_cfg->cstride - ch_w * (1 << size);
        srcskip = srcskip >> size;
      }
      if (IS_PIC_PVFBC(format) || IS_PIC_PVFBC_BYPASS(format)) {
        pic_size = CalcOnePpUnitPVFBCPlane1Size(ppu_cfg,0);
      }
      else {
        pic_size = ch_w * ch_h;
      }
      for(j = 0; j < (IS_PIC_PLANAR(format) ? 2 : 1); j++) {
        count++;
        chroma_addr = chroma_addr + NEXT_MULTIPLE(ppu_cfg->cstride * ch_h, PLANE_ALIGNMENT) * j;
        vcmd_m2mp_data[count].m2mp_width = srcskip ? ch_w : (pic_size & 0xFFFF);
        vcmd_m2mp_data[count].m2mp_height = srcskip ? ch_h : ((pic_size >> 16) & 0xFFFF);
        vcmd_m2mp_data[count].m2mp_size = size;
        vcmd_m2mp_data[count].m2mp_src_addr = (addr_t)chroma_addr;
        vcmd_m2mp_data[count].m2mp_srcskip = srcskip;
        vcmd_m2mp_data[count].m2mp_mode = 1;
      }
    } else if(IS_PIC_PLANAR_RGB(format) && !lumasum_enable) {
      ASSERT(ppu_cfg->ystride >= (lu_w * (1 << size)));
      srcskip = ppu_cfg->ystride - lu_w * (1 << size);
      srcskip = srcskip >> size;
      pic_size = lu_w * lu_h;
      for(j = 0; j < 2; j++) {
        count++;
        luma_addr = luma_addr + NEXT_MULTIPLE(lu_h * ppu_cfg->ystride, PLANE_ALIGNMENT);
        vcmd_m2mp_data[count].m2mp_width = srcskip ? lu_w : (pic_size & 0xFFFF);
        vcmd_m2mp_data[count].m2mp_height = srcskip ? lu_h : ((pic_size >> 16) & 0xFFFF);
        vcmd_m2mp_data[count].m2mp_size = size;
        vcmd_m2mp_data[count].m2mp_src_addr = (addr_t)luma_addr;
        vcmd_m2mp_data[count].m2mp_srcskip = srcskip;
        vcmd_m2mp_data[count].m2mp_mode = 1;
      }
    }
/* dec400 table */
    if (IS_PIC_DEC400(format)) {
      /* luma tbl*/
      luma_addr = base_bus_addr + ppu_cfg->dec400_pln0_tile_status_offset + DEC400_IN_HEADER_SIZE;
      size = 0;
      srcskip = 0;
      pic_size = ppu_cfg->dec400_luma_table.logical_size;
      count++;
      vcmd_m2mp_data[count].m2mp_width = srcskip ? lu_w : (pic_size & 0xFFFF);
      vcmd_m2mp_data[count].m2mp_height = srcskip ? lu_h : ((pic_size >> 16) & 0xFFFF);
      vcmd_m2mp_data[count].m2mp_size = size;
      vcmd_m2mp_data[count].m2mp_src_addr = (addr_t)luma_addr;
      vcmd_m2mp_data[count].m2mp_srcskip = srcskip;
      vcmd_m2mp_data[count].m2mp_mode = 1;
      /* chroma tbl*/
      if(ch_h && ch_w && chroma_addr) {
        chroma_addr = base_bus_addr + ppu_cfg->dec400_pln1_tile_status_offset + DEC400_IN_HEADER_SIZE;
        size = 0;
        srcskip = 0;
        pic_size = ppu_cfg->dec400_chroma_table.logical_size;
        count++;
        vcmd_m2mp_data[count].m2mp_width = srcskip ? lu_w : (pic_size & 0xFFFF);
        vcmd_m2mp_data[count].m2mp_height = srcskip ? lu_h : ((pic_size >> 16) & 0xFFFF);
        vcmd_m2mp_data[count].m2mp_size = size;
        vcmd_m2mp_data[count].m2mp_src_addr = (addr_t)chroma_addr;
        vcmd_m2mp_data[count].m2mp_srcskip = srcskip;
        vcmd_m2mp_data[count].m2mp_mode = 1;
      }
    }
  }
}

enum DecRet DecLinkAndRunCmds(const void *instance, struct DecCmdInfo *info_m, u32 cmdbuf_id) {
  u32 cmd_buf_id = MAX_MC_CB_ENTRIES;
  FifoObject obj;
  u64 interrupt_ctrl = 0;
  enum DecRet ret = DEC_OK;
  struct DecCmdInfo info_tmp = {0}, *info = info_m;

  if (!info) {
    /* for all formats except 264/265 */
    info = &info_tmp;

    info->batch_size = 1;
    info->cmd_num = 1;
    info->dis_batch_mode = 0;
    // info->is_relink = 0;
  }

  if (info->dis_batch_mode && *info->b_mc) {
    info->batch_size = 1;
    *info->b_mc = 0;
    *info->max_batch_size = 1;
  }

  if (info->batch_size && cmdbuf_id != -1) {
    if (info_m) {
      FifoPush(info->fifo_cmd, (FifoObject)(size_t)cmdbuf_id, FIFO_EXCEPTION_DISABLE);
      info->cmd_num++;
    }
    // if (!info->is_relink)
      DWLPrepareCmds(instance, cmdbuf_id);
  }

  if(info->cmd_num >= info->batch_size || cmdbuf_id == -1) {
    while(info->cmd_num) {
      cmd_buf_id = cmdbuf_id;
      if (info_m) {
        interrupt_ctrl = 0;
        FifoPop(info->fifo_cmd, &obj, FIFO_EXCEPTION_DISABLE);
        cmd_buf_id = (size_t)obj;
        if ((info->client_type == DWL_CLIENT_TYPE_H264_DEC ||
             info->client_type == DWL_CLIENT_TYPE_HEVC_DEC) && info->batch_size > 1) { //batch mode
          interrupt_ctrl = ((u32)1 << 31);
          interrupt_ctrl &= ~(u64)1;
          interrupt_ctrl |= ((u64)info->batch_size << 32);
          if (info->cmd_num == 1) { //the last job
            /* set IE flag for last job in current batch */
            interrupt_ctrl |= (u64)1;
          }
        }
        DWLSetCmdBufIntCtrl(instance, cmd_buf_id, interrupt_ctrl);
      }
      DWLEnableCmdBuf(instance, cmd_buf_id);
      info->cmd_num--;
    }
  }

  return ret;
}

/* function: generate fake table */
#ifdef USE_FAKE_RFC_TABLE
void GenerateFakeRFCTable(u8 *cmp_tble_addr,
                          u32 pic_width_in_cbsy,
                          u32 pic_height_in_cbsy,
                          u32 pic_width_in_cbsc,
                          u32 pic_height_in_cbsc,
                          u32 bit_depth,
                          u32 buffer_align) {
  u8 cbs_size_y = 0, cbs_size_c = 0;
  u8 ec_word_align = 1;
#ifdef FAKE_RFC_TBL_LITTLE_ENDIAN
  u8 cbs_sizes_8bit[14] = {64, 32, 16, 8, 4, 2, 129, 64, 32, 16, 8, 4, 2, 129};
  u8 cbs_sizes_10bit[14] = {80, 40, 20, 10, 0x85, 0x42, 0xa1, 80, 40, 20, 10, 0x85, 0x42, 0xa1};
#else
  u8 cbs_sizes_8bit[14] = {129, 2, 4, 8, 16, 32, 64, 129, 2, 4, 8, 16, 32, 64};
  u8 cbs_sizes_10bit[14] = {0xa1, 0x42, 0x85, 10, 20, 40, 80, 0xa1, 0x42, 0x85, 10, 20, 40, 80};
#endif
  u32 i, j, offset;
  u8 *pcbs, *ptbl = NULL;
  if (bit_depth == 8) {
    cbs_size_y = cbs_size_c = 64;
    ptbl = cbs_sizes_8bit;
  } else if (bit_depth == 10) {
    cbs_size_y = cbs_size_c = 80;
    ptbl = cbs_sizes_10bit;
  }
  /*
  LOG(SDK_DEBUG_OUT, "%s#  0x%08x Y: (%d x %d), C: (%d x %d) bitdepth %d\n",
              __FUNCTION__,
              cmp_tble_addr,
              pic_width_in_cbsy,
              pic_height_in_cbsy,
              pic_width_in_cbsc,
              pic_height_in_cbsc,
              bit_depth);
  */

  // Compression table for Y
  pcbs = cmp_tble_addr;
#ifdef DBG_DPB
  printf("\tGenerateFakeRFCTable# fake table luma %p chroma %p\n", (void *)pcbs, (void *)(cmp_tble_addr + NEXT_MULTIPLE(pic_height_in_cbsy * pic_width_in_cbsy, buffer_align)));
#endif
  for (i = 0; i < pic_height_in_cbsy; i++) {
    offset = 0;
    for (j = 0; j < pic_width_in_cbsy/16; j++) {
#ifdef FAKE_RFC_TBL_LITTLE_ENDIAN
      *(u16 *)pcbs = offset;
      if (ptbl != NULL) DWLmemcpy(pcbs+2, ptbl, 14);
#else
      if (ptbl != NULL) DWLmemcpy(pcbs, ptbl, 14);
      *(pcbs+14) = offset >> 8;
      *(pcbs+15) = offset & 0xff;
#endif
      pcbs += 16;
      offset += (16 * cbs_size_y) >> ec_word_align;
    }
  }

  pcbs = cmp_tble_addr +
    NEXT_MULTIPLE(pic_height_in_cbsy * pic_width_in_cbsy, buffer_align);

  // Compression table for C
  for (i = 0; i < pic_height_in_cbsc; i++) {
    offset = 0;
    for (j = 0; j < pic_width_in_cbsc/16; j++) {
#ifdef FAKE_RFC_TBL_LITTLE_ENDIAN
      *(u16 *)pcbs = offset;
      if (ptbl != NULL) DWLmemcpy(pcbs+2, ptbl, 14);
#else
      if (ptbl != NULL) DWLmemcpy(pcbs, ptbl, 14);
      *(pcbs+14) = offset >> 8;
      *(pcbs+15) = offset & 0xff;
#endif
      pcbs += 16;
      offset += (16 * cbs_size_c) >> ec_word_align;
    }
  }

  /*
    u8 *p = cmp_tble_addr;
    printf("Fake RFC table:\n");
    for (i = 0; i < 8; i++) {
      printf("0x%02x 0x%02x 0x%02x 0x%02x ", p[0], p[1], p[2], p[3]);
      p += 4;
    }
    printf("\n");
  */
}
#endif


static const u32 ref_ybase[16] = {
  HWIF_REFER0_YBASE_LSB,  HWIF_REFER1_YBASE_LSB,  HWIF_REFER2_YBASE_LSB,
  HWIF_REFER3_YBASE_LSB,  HWIF_REFER4_YBASE_LSB,  HWIF_REFER5_YBASE_LSB,
  HWIF_REFER6_YBASE_LSB,  HWIF_REFER7_YBASE_LSB,  HWIF_REFER8_YBASE_LSB,
  HWIF_REFER9_YBASE_LSB,  HWIF_REFER10_YBASE_LSB, HWIF_REFER11_YBASE_LSB,
  HWIF_REFER12_YBASE_LSB, HWIF_REFER13_YBASE_LSB, HWIF_REFER14_YBASE_LSB,
  HWIF_REFER15_YBASE_LSB
};

static const u32 ref_ybase_msb[16] = {
  HWIF_REFER0_YBASE_MSB,  HWIF_REFER1_YBASE_MSB,  HWIF_REFER2_YBASE_MSB,
  HWIF_REFER3_YBASE_MSB,  HWIF_REFER4_YBASE_MSB,  HWIF_REFER5_YBASE_MSB,
  HWIF_REFER6_YBASE_MSB,  HWIF_REFER7_YBASE_MSB,  HWIF_REFER8_YBASE_MSB,
  HWIF_REFER9_YBASE_MSB,  HWIF_REFER10_YBASE_MSB, HWIF_REFER11_YBASE_MSB,
  HWIF_REFER12_YBASE_MSB, HWIF_REFER13_YBASE_MSB, HWIF_REFER14_YBASE_MSB,
  HWIF_REFER15_YBASE_MSB
};

#define LUM_CBS_WIDTH 8
#define CH_CBS_WIDTH 16
#define LUMA_CACHE_RAM_SIZE (24*1024)
#define CHROMA_CACHE_RAM_SIZE (12*1024)
#define CBS_ONE_ROW_PREFTECH_PIXELS 4096 // one cbs row prefetching covers 4096 pixels
#define CBS_ONE_ROW_PREFTECH_BYTES_Y (CBS_ONE_ROW_PREFTECH_PIXELS/LUM_CBS_WIDTH)
#define CBS_ONE_ROW_PREFTECH_BYTES_C (CBS_ONE_ROW_PREFTECH_PIXELS/CH_CBS_WIDTH)
#define TILE_EXTRA_PIXELS 128
#define MAX_RFC_TABLE_LINES 63

#define UP_ALIGN(x, b) (((x) + (b) - 1)& ~((b)-1))
#define DOWN_ALIGN(x, b) ((x)& ~((b)-1))

static u32 GetDifferentRefFrmNum(struct RTP *rtp, u32 dpb_size) {

  ASSERT(rtp->dec_mode != DEC_MODE_VP9 || rtp->num_ref_frames == 3);
  ASSERT(rtp->num_ref_frames <= dpb_size);

  /* AV1 and HEVC have got accurate number of reference pictures . */
  if (rtp->dec_mode == DEC_MODE_AV1 ||
      rtp->dec_mode == DEC_MODE_HEVC)
    return rtp->num_ref_frames;

  u32 i, j;
  u32 num_ref_frames = rtp->num_ref_frames;

  if (num_ref_frames == 1)
    return num_ref_frames;

  if (rtp->dec_mode == DEC_MODE_H264_H10P)
    num_ref_frames = dpb_size;

  /* Find how many difference buffers are there in the referenece picture list.
      Speical treatment for AV1/VP9 and can use a new register instead. */

  u32 dpb_buffers = 1;
  // printf("ref %2d: 0x%p\n", 0, (void *)ref_pic[0]);
  for (i = 1; i < num_ref_frames; i++) {
    // printf("ref %2d: 0x%p\n", i, (void *)rtp->ref_pic[i]);
    for (j = 0; j < i; j++) {
      if (rtp->ref_pic[j] == rtp->ref_pic[i]) {
        break;
      }
    }
    if (j == i) dpb_buffers++;    // a new reference picture.
  }

  return dpb_buffers;
}

void SetRfcTablePrefetch(u32 *regs, u32 max_tile_width, u32 max_tile_height, u32 dpb_size) {
  // read from registers
  struct RTP rtp;
  u32 pic_width;
  int i;

  {
    rtp.pic_width_in_cbs = GetDecRegister(regs, HWIF_PIC_WIDTH_IN_CBS);
    rtp.min_cb_size = GetDecRegister(regs, HWIF_MIN_CB_SIZE);
    rtp.max_cb_size = GetDecRegister(regs, HWIF_MAX_CB_SIZE);
    rtp.dec_mode = GetDecRegister(regs, HWIF_DEC_MODE);
    rtp.max_tile_width = max_tile_width << (rtp.max_cb_size - 3);
    rtp.max_tile_height = max_tile_height << (rtp.max_cb_size - 3);
    if (rtp.dec_mode == DEC_MODE_H264_H10P || rtp.dec_mode == DEC_MODE_H264) {
      rtp.num_tile_cols = 1;
      rtp.num_tile_rows = 1;
    } else {
      rtp.num_tile_cols = GetDecRegister(regs, HWIF_NUM_TILE_COLS_8K);
      if (rtp.dec_mode == DEC_MODE_AV1)
        rtp.num_tile_rows = GetDecRegister(regs, HWIF_NUM_TILE_ROWS_8K_AV1);
      else if (rtp.dec_mode == DEC_MODE_VVC)
        rtp.num_tile_rows = GetDecRegister(regs, HWIF_PPS_NUM_TILE_ROWS_VVC);
      else
        rtp.num_tile_rows = GetDecRegister(regs, HWIF_NUM_TILE_ROWS_8K);
    }
    rtp.num_ref_frames = GetDecRegister(regs, HWIF_REF_FRAMES);
    if (rtp.dec_mode == DEC_MODE_VP9) {
      rtp.ref_pic[0] = GET_ADDR_REG2(regs, ref_ybase[0], ref_ybase_msb[0]);
      rtp.ref_pic[1] = GET_ADDR_REG2(regs, ref_ybase[4], ref_ybase_msb[4]);
      rtp.ref_pic[2] = GET_ADDR_REG2(regs, ref_ybase[5], ref_ybase_msb[5]);
    } else {
      for (i = 0; i < 16; i++) {
        rtp.ref_pic[i] = GET_ADDR_REG2(regs, ref_ybase[i], ref_ybase_msb[i]);
      }
    }
  }
  /* nothing to do if there are no reference pictures */
  if (!rtp.num_ref_frames) return;

  u32 y_cache_size, c_cache_size;
  u32 lu_tbl_stride;
  u32 ch_tbl_stride;
  u32 g_cache_y_line_num;
  u32 g_cache_y_line_len;
  u32 g_cache_c_line_num;
  u32 g_cache_c_line_len;
  u32 g_cache_block_num;
  /* real prefetched region for luma/chroma */
  u32 g_cache_y_line_num_real;
  u32 g_cache_c_line_num_real;

  /* Get real different reference pictures number. */
  u32 num_ref_frames = GetDifferentRefFrmNum(&rtp, dpb_size);

  pic_width = rtp.pic_width_in_cbs << rtp.min_cb_size;
  lu_tbl_stride = UP_ALIGN((UP_ALIGN(pic_width, LUM_CBS_WIDTH)/LUM_CBS_WIDTH), 16);
  ch_tbl_stride = UP_ALIGN((UP_ALIGN(pic_width, CH_CBS_WIDTH)/CH_CBS_WIDTH), 16);

  if(rtp.dec_mode == DEC_MODE_VP9) {
    g_cache_block_num = num_ref_frames;

    if(pic_width > 640) {
      g_cache_y_line_len = 16*8;      // can cover up to 1024 pixel width
      g_cache_y_line_num = 25;
      g_cache_c_line_len = 16*5;
      g_cache_c_line_num = 24;
    } else {
      g_cache_y_line_len = 16*5;    // can cover up to 640 pixel width
      g_cache_y_line_num = 40;
      g_cache_c_line_len = 16*3;
      g_cache_c_line_num = 40;
    }
  } else {
    switch (num_ref_frames) {
    case 1:
      g_cache_y_line_len = 16*15;
      g_cache_y_line_num = 20;
      g_cache_c_line_len = 16*9;
      g_cache_c_line_num = 20;
      g_cache_block_num = 1;
      break;
    case 2:
      if(pic_width <= 1280) {
        g_cache_y_line_len = 16*10;
        g_cache_y_line_num = 30;
        g_cache_c_line_len = 16*6;
        g_cache_c_line_num = 30;
      } else {
        g_cache_y_line_len = 16*15;
        g_cache_y_line_num = 20;
        g_cache_c_line_len = 16*9;
        g_cache_c_line_num = 20;
      }
      g_cache_block_num = 2;
      break;
    case 3:
      if(pic_width <= 1024) {
        g_cache_y_line_len = 16*8;
        g_cache_y_line_num = 25;
        g_cache_c_line_len = 16*5;
        g_cache_c_line_num = 24;
      } else {
        g_cache_y_line_len = 16*10;
        g_cache_y_line_num = 20;
        g_cache_c_line_len = 16*6;
        g_cache_c_line_num = 20;
      }
      g_cache_block_num = 3;
      break;
    case 4:
      if(pic_width <= 1024) {
        g_cache_y_line_len = 16*6;
        g_cache_y_line_num = 25;
        g_cache_c_line_len = 16*4;
        g_cache_c_line_num = 22;
      } else {
        g_cache_y_line_len = 16*8;
        g_cache_y_line_num = 18;
        g_cache_c_line_len = 16*5;
        g_cache_c_line_num = 18;
      }
      g_cache_block_num = 4;
      break;
    case 5:
      if(pic_width <= 1024) {
        g_cache_y_line_len = 16*6;
        g_cache_y_line_num = 25;
        g_cache_c_line_len = 16*4;
        g_cache_c_line_num = 22;
      } else {
        g_cache_y_line_len = 16*8;
        g_cache_y_line_num = 18;
        g_cache_c_line_len = 16*5;
        g_cache_c_line_num = 18;
      }
      g_cache_block_num = 5;
      break;
    default:
      g_cache_y_line_len = 16*5;
      g_cache_y_line_num = 20;
      g_cache_c_line_len = 16*3;
      g_cache_c_line_num = 20;
      g_cache_block_num = 6;
      break;
    }
  }

  y_cache_size = LUMA_CACHE_RAM_SIZE/g_cache_block_num;
  c_cache_size = CHROMA_CACHE_RAM_SIZE/g_cache_block_num;

  if (rtp.max_cb_size == 4) { //CTB16
    g_cache_y_line_num = 12;
    g_cache_c_line_num = 12;

    g_cache_y_line_len = MIN(DOWN_ALIGN(y_cache_size /g_cache_y_line_num, 16), CBS_ONE_ROW_PREFTECH_BYTES_Y);
    g_cache_c_line_len = MIN(DOWN_ALIGN(c_cache_size /g_cache_c_line_num, 16), CBS_ONE_ROW_PREFTECH_BYTES_C);
  }

  /*
      Above is the default settings for RFC CBS table prefetching.
      We optimize following two cases:
      1. pic width less than 4096: to prefetch one cbs row of a whole picture once for CTB<=64x64;
      2. if tile colomns is used, reduce the width prefetching window to maximum tile column width.
    */
  u32 cbs_prefetch_optimized = 0;
  u32 cache_y_line_num_expected = 12;
  u32 cache_c_line_num_expected = 12;

  if (rtp.num_tile_cols > 1 || rtp.num_tile_rows > 1) {

    if (rtp.max_cb_size == 7) {
      cache_y_line_num_expected = 20;
      cache_c_line_num_expected = 20;
    } else if(rtp.max_cb_size == 6) {
      cache_y_line_num_expected = 12;
      cache_c_line_num_expected = 12;
    }
    // find the widest of tile column
    u32 max_tile_width = rtp.max_tile_width;
    u32 max_tile_height = rtp.max_tile_height;
    // whole tile row table size
    u32 y_cbs_tile_row_size = max_tile_height * lu_tbl_stride;
    u32 c_cbs_tile_row_size = max_tile_height * ch_tbl_stride;

    if (rtp.num_tile_cols > 1) {
      if (y_cbs_tile_row_size > y_cache_size) {
        u32 tile_width = MIN(pic_width, (max_tile_width << 3) + TILE_EXTRA_PIXELS);
        lu_tbl_stride = UP_ALIGN((UP_ALIGN(tile_width, LUM_CBS_WIDTH)/LUM_CBS_WIDTH), 16);

        u32 y_cbs_tile_size = max_tile_height * lu_tbl_stride;
        if (y_cache_size >= y_cbs_tile_size) {
          // can hold one row in tile
          cache_y_line_num_expected = max_tile_height;
          lu_tbl_stride = DOWN_ALIGN(y_cache_size / cache_y_line_num_expected, 16);
        }
      }

      if (c_cbs_tile_row_size > c_cache_size) {
        /* prefetch buffer cannot hold one tile row */
        u32 tile_width = MIN(pic_width, (max_tile_width << 3) + TILE_EXTRA_PIXELS);
        ch_tbl_stride = UP_ALIGN((UP_ALIGN(tile_width, CH_CBS_WIDTH)/CH_CBS_WIDTH), 16);

        u32 c_cbs_tile_size = max_tile_height * ch_tbl_stride;
        if (c_cache_size >= c_cbs_tile_size) {
          // can hold one row in tile
          cache_c_line_num_expected = max_tile_height;
          ch_tbl_stride = DOWN_ALIGN(c_cache_size / cache_c_line_num_expected, 16);
        }
      }

      u32 cache_y_line_num = y_cache_size / lu_tbl_stride;
      u32 cache_c_line_num = c_cache_size / ch_tbl_stride;

      if (cache_y_line_num >= cache_y_line_num_expected &&
          cache_c_line_num >= cache_c_line_num_expected ) {
        /* can prefetch a whole cbs row once */
        g_cache_y_line_num = cache_y_line_num_expected;
        g_cache_c_line_num = cache_c_line_num_expected;

        g_cache_y_line_len = lu_tbl_stride;
        g_cache_c_line_len = ch_tbl_stride;
        cbs_prefetch_optimized = 1;
      }
      if (cbs_prefetch_optimized) {
        g_cache_y_line_num_real = MIN(y_cache_size/g_cache_y_line_len, max_tile_height + 8);
        g_cache_c_line_num_real = MIN(c_cache_size/g_cache_c_line_len, max_tile_height + 8);
      }
    } else {
      /*  rtp.num_tile_rows > 1 */
      u32 y_tile_rows, c_tile_rows; //tile rows/columns covered
      u32 cache_y_line_num = y_cache_size / lu_tbl_stride;
      u32 cache_c_line_num = c_cache_size / ch_tbl_stride;

      y_tile_rows = y_cache_size / y_cbs_tile_row_size;
      if (y_tile_rows >= 1) {
        cbs_prefetch_optimized = 1;
        g_cache_y_line_len = lu_tbl_stride;
        g_cache_y_line_num = cache_y_line_num_expected;
        g_cache_y_line_num_real = y_tile_rows * max_tile_height;
      } else if (cache_y_line_num >= cache_y_line_num_expected) {
        cbs_prefetch_optimized = 1;
        g_cache_y_line_len = lu_tbl_stride;
        g_cache_y_line_num = cache_y_line_num_expected;
        g_cache_y_line_num_real = cache_y_line_num;
      } else {
        g_cache_y_line_num_real = g_cache_y_line_num;
      }

      c_tile_rows = c_cache_size / c_cbs_tile_row_size;
      if (c_tile_rows >= 1) {
        cbs_prefetch_optimized = 1;
        g_cache_c_line_len = ch_tbl_stride;
        g_cache_c_line_num = cache_c_line_num_expected;
        g_cache_c_line_num_real = c_tile_rows * max_tile_height;
      } else if (cache_c_line_num >= cache_c_line_num_expected) {
        cbs_prefetch_optimized = 1;
        g_cache_c_line_len = ch_tbl_stride;
        g_cache_c_line_num = cache_c_line_num_expected;
        g_cache_c_line_num_real = cache_c_line_num;
      } else {
        g_cache_c_line_num_real = g_cache_c_line_num;
      }
    }
  } else {
    /* no tiles */
    cache_y_line_num_expected = 12;
    cache_c_line_num_expected = 12;

    /* threshold determined by ctb size */
    if (rtp.max_cb_size == 7) {
      cache_y_line_num_expected = 20;
      cache_c_line_num_expected = 20;
    }

    // Prefetch size is whole RFC table line, but maybe due to too many reference
    // pictures, the expected lines need to be decreased to make sure rfc cbs
    // table prefetch is enough.
    u32 cache_y_line_num = y_cache_size / lu_tbl_stride;
    u32 cache_c_line_num = c_cache_size / ch_tbl_stride;

    if (cache_y_line_num >= cache_y_line_num_expected &&
        cache_c_line_num >= cache_c_line_num_expected) {

      g_cache_y_line_num = cache_y_line_num_expected;
      g_cache_c_line_num = cache_c_line_num_expected;

      g_cache_y_line_len = lu_tbl_stride;
      g_cache_c_line_len = ch_tbl_stride;

      cbs_prefetch_optimized = 1;

      g_cache_y_line_num_real = y_cache_size/g_cache_y_line_len;
      g_cache_c_line_num_real = c_cache_size/g_cache_c_line_len;
    } else {
      /* Cannot cover complete CBS table lines: try to read as many bytes in cbs row
         as possible. */
      g_cache_y_line_num = cache_y_line_num_expected;
      g_cache_c_line_num = cache_c_line_num_expected;

      g_cache_y_line_len = DOWN_ALIGN(y_cache_size / g_cache_y_line_num, 16);
      g_cache_c_line_len =  DOWN_ALIGN(c_cache_size / g_cache_c_line_num, 16);

      cbs_prefetch_optimized = 1;

      g_cache_y_line_num_real = g_cache_y_line_num;
      g_cache_c_line_num_real = g_cache_c_line_num;
    }
  }

  if (!cbs_prefetch_optimized) {
    g_cache_y_line_num_real = g_cache_y_line_num;
    g_cache_c_line_num_real = g_cache_c_line_num;
  }

  ASSERT(y_cache_size >= g_cache_y_line_len*g_cache_y_line_num_real);
  ASSERT(c_cache_size >= g_cache_c_line_len*g_cache_c_line_num_real);

  rtp.rtp_block_num = g_cache_block_num;
  rtp.y_line_num = MIN(g_cache_y_line_num, MAX_RFC_TABLE_LINES);
  rtp.c_line_num = MIN(g_cache_c_line_num, MAX_RFC_TABLE_LINES);
  rtp.y_line_num_prefetch = g_cache_y_line_num_real;
  rtp.c_line_num_prefetch = g_cache_c_line_num_real;
  rtp.y_line_len = g_cache_y_line_len;
  rtp.c_line_len = g_cache_c_line_len;
  rtp.rtp_override_e = 1;

  SetDecRegister(regs, HWIF_RTP_OVERRIDE_E, rtp.rtp_override_e);
  SetDecRegister(regs, HWIF_RTP_BLOCK_NUM, rtp.rtp_block_num);
  SetDecRegister(regs, HWIF_RTP_Y_LINE_NUM, rtp.y_line_num);
  SetDecRegister(regs, HWIF_RTP_C_LINE_NUM, rtp.c_line_num);
  SetDecRegister(regs, HWIF_RTP_Y_LINE_NUM_PREFETCH, rtp.y_line_num_prefetch);
  SetDecRegister(regs, HWIF_RTP_C_LINE_NUM_PREFETCH, rtp.c_line_num_prefetch);
  SetDecRegister(regs, HWIF_RTP_Y_LINE_LEN, rtp.y_line_len);
  SetDecRegister(regs, HWIF_RTP_C_LINE_LEN, rtp.c_line_len);
}


#ifdef FPGA_PERF_AND_BW
void DecPerfInfoCount(const void *instance, u32 core_id, DecPerfInfo* perf_info, struct DecInfo dec_info) {
  struct DWLInstance *dwl = (struct DWLInstance *)instance;
  struct DWLPerfInfo perf_count;
  u64 pic_size, bit_depth;

  pic_size = dec_info.pic_size;
  bit_depth = dec_info.bit_depth;
  DWLmemset(&perf_count, 0, sizeof(struct DWLPerfInfo));
  DWLPerfInfoCollect(dwl, core_id, &perf_count);
#ifdef DUMP_STRM_COMP_RATIO
  if (dec_info.codec == DEC_MODE_PNG)
    return;
  double strm_comp_ratio = 0;
  u64 yuv_size = 0;
  u32 chroma_format = dec_info.chroma_format;
  if(perf_count.bitrate) {
    if (chroma_format == PP_CHROMA_400)
      yuv_size = pic_size * bit_depth;
    else if (chroma_format == PP_CHROMA_420 || chroma_format == PP_CHROMA_411)
      yuv_size = pic_size * bit_depth * 3/2;
    else if (chroma_format == PP_CHROMA_422 || chroma_format == PP_CHROMA_440)
      yuv_size = pic_size * bit_depth * 2;
    else if (chroma_format == PP_CHROMA_444)
      yuv_size = pic_size * bit_depth * 3;
    strm_comp_ratio = ((u64)perf_count.bitrate * 8 * 100 * 100) / yuv_size / 100.00;
  }
  perf_info->strm_len += perf_count.bitrate;
  perf_info->yuv_size += yuv_size/8;
  PERFTRACE_I("PIC_ID %d , Stream Compression Ratio %.2f%%\n", perf_info->pic_num + 1, strm_comp_ratio);
#else
  u32 mbs;
  if(perf_count.bitrate)
    perf_count.bitrate = 60 * perf_count.bitrate * 8 * 3840 * 2160 / pic_size / 1000 / 1000;
  mbs = pic_size >> 8;
  if(mbs)
    perf_count.cycles = perf_count.cycles / mbs;
  perf_count.read_bw = perf_count.read_bw * 16 * 1000 / pic_size;
  perf_count.write_bw = perf_count.write_bw * 16 * 1000 / pic_size;
  if(bit_depth == 10) {
    perf_count.read_bw = perf_count.read_bw * 8 / 10;
    perf_count.write_bw = perf_count.write_bw * 8 / 10 ;
  } else if ( bit_depth == 12) {
    perf_count.read_bw = perf_count.read_bw * 8 / 12;
    perf_count.write_bw = perf_count.write_bw * 8 /12;
  }

  PERFTRACE_I("PIC_ID %d , %4d Mbps (4k@60fps),%4d cycles / mb, BW R/W: %0.3f/%0.3f GBps (x FRAME 4k@60fps)\n",
              perf_info->pic_num + 1, (u32)perf_count.bitrate,
              perf_count.cycles, (double)perf_count.read_bw/ 1000 / 2.009,
              (double)perf_count.write_bw/ 1000 / 2.009);
#endif
  perf_info->bitrate += perf_count.bitrate;
  perf_info->read_bw += perf_count.read_bw;
  perf_info->write_bw += perf_count.write_bw;
  perf_info->cycles += perf_count.cycles;
  perf_info->pic_num ++;
}

void AveragePerfInfoPrint(DecPerfInfo *perf_info) {
#ifdef DUMP_STRM_COMP_RATIO
  double strm_comp_ratio = (perf_info->strm_len * 100 * 100) / perf_info->yuv_size / 100.00;
  PERFTRACE_I("Total Stream Compression Ratio %.2f%%\n", strm_comp_ratio);
#else
  PERFTRACE_I("Average bitrate/frame (4k@60fps): %4d Mbps, Average cycles/MB: %4d, Average BW R/W (x FRAME 4k@60fps): %0.3f / %0.3f , %0.3f GBps\n",
              (u32)(perf_info->bitrate/perf_info->pic_num), perf_info->cycles/perf_info->pic_num,
              (double)perf_info->read_bw / perf_info->pic_num / 1000 / 2.009,
              (double)perf_info->write_bw / perf_info->pic_num / 1000 / 2.009,
              (double)(perf_info->read_bw + perf_info->write_bw) / perf_info->pic_num / 1000/ 2.009);
#endif
}
#endif

#ifdef OPEN_HWCFG_TO_CLIENT
void GetHwConfig(const void *p_hw_feature, struct DecHwConfig *hw_cfg) {
  u32 tmp;
  const struct DecHwFeatures *hw_feature = (const struct DecHwFeatures *)p_hw_feature;

  ASSERT(hw_cfg);
  (void)DWLmemset(hw_cfg, 0, sizeof(struct DecHwConfig));

  hw_cfg->hevc_main10_support = hw_feature->hevc_main10_support;
  hw_cfg->vp9_10bit_support = hw_feature->vp9_profile2_support;
  hw_cfg->ds_support = hw_feature->dscale_support[0] |
                       hw_feature->dscale_support[1] |
                       hw_feature->dscale_support[2] |
                       hw_feature->dscale_support[3];
  hw_cfg->rfc_support = hw_feature->rfc_support;
  hw_cfg->addr64_support = hw_feature->addr64_support;
  hw_cfg->fmt_p010_support = hw_feature->fmt_p010_support;
  hw_cfg->max_pp_out_pic_width = hw_feature->max_pp_out_pic_width[0];
  hw_cfg->max_pp_out_pic_height = hw_feature->max_pp_out_pic_height[0];

  hw_cfg->h264_support = hw_feature->h264_support;
  if(hw_cfg->h264_support) {
    hw_cfg->max_dec_pic_width[DWL_CLIENT_TYPE_H264_DEC] = hw_feature->h264_max_dec_pic_width;
    hw_cfg->max_dec_pic_height[DWL_CLIENT_TYPE_H264_DEC] = hw_feature->h264_max_dec_pic_height;
  }
  hw_cfg->vp9_support = hw_feature->vp9_support;
  if(hw_cfg->vp9_support) {
    hw_cfg->max_dec_pic_width[DWL_CLIENT_TYPE_VP9_DEC] = hw_feature->vp9_max_dec_pic_width;
    hw_cfg->max_dec_pic_height[DWL_CLIENT_TYPE_VP9_DEC] = hw_feature->vp9_max_dec_pic_height;
  }
  hw_cfg->hevc_support = hw_feature->hevc_support;
  if(hw_cfg->hevc_support) {
    hw_cfg->max_dec_pic_width[DWL_CLIENT_TYPE_HEVC_DEC] = hw_feature->hevc_max_dec_pic_width;
    hw_cfg->max_dec_pic_height[DWL_CLIENT_TYPE_HEVC_DEC] = hw_feature->hevc_max_dec_pic_height;
  }
  hw_cfg->avs2_support = hw_feature->avs2_support;
  if(hw_cfg->avs2_support) {
    hw_cfg->max_dec_pic_width[DWL_CLIENT_TYPE_AVS2_DEC] = hw_feature->hevc_max_dec_pic_width;
    hw_cfg->max_dec_pic_height[DWL_CLIENT_TYPE_AVS2_DEC] = hw_feature->hevc_max_dec_pic_width;
  }
  hw_cfg->jpeg_support = hw_feature->jpeg_support;
  if(hw_cfg->jpeg_support) {
    hw_cfg->max_dec_pic_width[DWL_CLIENT_TYPE_JPEG_DEC] = hw_feature->img_max_dec_width;
    hw_cfg->max_dec_pic_height[DWL_CLIENT_TYPE_JPEG_DEC] = hw_feature->img_max_dec_height;
  }
  hw_cfg->mpeg4_support = hw_feature->mpeg4_support;
  if(hw_cfg->mpeg4_support) {
    hw_cfg->max_dec_pic_width[DWL_CLIENT_TYPE_MPEG4_DEC] = hw_feature->mpeg4_max_dec_pic_width;
    hw_cfg->max_dec_pic_height[DWL_CLIENT_TYPE_MPEG4_DEC] = hw_feature->mpeg4_max_dec_pic_height;
  }
  hw_cfg->mpeg2_support = hw_feature->mpeg2_support;
  if(hw_cfg->mpeg2_support) {
    hw_cfg->max_dec_pic_width[DWL_CLIENT_TYPE_MPEG2_DEC] = hw_feature->mpeg2_max_dec_pic_width;
    hw_cfg->max_dec_pic_height[DWL_CLIENT_TYPE_MPEG2_DEC] = hw_feature->mpeg2_max_dec_pic_height;
  }
  hw_cfg->vc1_support = hw_feature->vc1_support;
  if(hw_cfg->vc1_support) {
    hw_cfg->max_dec_pic_width[DWL_CLIENT_TYPE_VC1_DEC] = hw_feature->vc1_max_dec_pic_width;
    hw_cfg->max_dec_pic_height[DWL_CLIENT_TYPE_VC1_DEC] = hw_feature->vc1_max_dec_pic_height;
  }
  hw_cfg->vp6_support = hw_feature->vp6_support;
  if(hw_cfg->vp6_support) {
    hw_cfg->max_dec_pic_width[DWL_CLIENT_TYPE_VP6_DEC] = hw_feature->vp6_max_dec_pic_width;
    hw_cfg->max_dec_pic_height[DWL_CLIENT_TYPE_VP6_DEC] = hw_feature->vp6_max_dec_pic_height;
  }
  hw_cfg->vp7_support = hw_feature->vp7_support;
  hw_cfg->vp8_support = hw_feature->vp8_support;
  if(hw_cfg->vp8_support) {
    hw_cfg->max_dec_pic_width[DWL_CLIENT_TYPE_VP8_DEC] = hw_feature->vp8_max_dec_pic_width;
    hw_cfg->max_dec_pic_height[DWL_CLIENT_TYPE_VP8_DEC] = hw_feature->vp8_max_dec_pic_height;
  }
  hw_cfg->av1_support = hw_feature->av1_support;
  if(hw_cfg->av1_support) {
    hw_cfg->max_dec_pic_width[DWL_CLIENT_TYPE_AV1_DEC] = hw_feature->av1_max_dec_pic_width;
    hw_cfg->max_dec_pic_height[DWL_CLIENT_TYPE_AV1_DEC] = hw_feature->av1_max_dec_pic_height;
  }
  hw_cfg->vvc_support = hw_feature->vvc_support;
  if(hw_cfg->vvc_support) {
    hw_cfg->max_dec_pic_width[DWL_CLIENT_TYPE_VVC_DEC] = hw_feature->vvc_max_dec_pic_width;
    hw_cfg->max_dec_pic_height[DWL_CLIENT_TYPE_VVC_DEC] = hw_feature->vvc_max_dec_pic_height;
  }

  hw_cfg->custom_mpeg4_support = hw_feature->custom_mpeg4_support;
  hw_cfg->pp_standalone = hw_feature->pp_standalone;
  tmp = 0;
  if (hw_feature->dscale_support[0] || hw_feature->uscale_support[0]) {
    u32 scaling_bits;
    scaling_bits = 0x3;
    scaling_bits <<= 26;
    tmp |= scaling_bits; /* PP_SCALING */
  }

  hw_cfg->pp_config = tmp;
  hw_cfg->sorenson_spark_support = hw_feature->sorenson_spark_support;
#ifdef DEC_X170_APF_DISABLE
  if (DEC_X170_APF_DISABLE) {

  }
#endif /* DEC_X170_APF_DISABLE */

  hw_cfg->avs_support = hw_feature->avs_support;
  if(hw_cfg->avs_support) {
    hw_cfg->max_dec_pic_width[DWL_CLIENT_TYPE_AVS_DEC] = hw_feature->avs_max_dec_pic_width;
    hw_cfg->max_dec_pic_height[DWL_CLIENT_TYPE_AVS_DEC] = hw_feature->avs_max_dec_pic_height;
  }
  if (hw_feature->avs_support == 2)
    hw_cfg->avs_plus_support = 1;
  else
    hw_cfg->avs_plus_support = 0;

  hw_cfg->rv_support = hw_feature->rv_support;
  if(hw_cfg->rv_support) {
    hw_cfg->max_dec_pic_width[DWL_CLIENT_TYPE_RV_DEC] = hw_feature->rv_max_dec_pic_width;
    hw_cfg->max_dec_pic_height[DWL_CLIENT_TYPE_RV_DEC] = hw_feature->rv_max_dec_pic_height;
  }
  hw_cfg->webp_support = hw_feature->webp_support;
}
#endif

void SwGetSubsampling(u32 chroma_format_idc, u32 *subsampling_x, u32 *subsampling_y) {
  if (chroma_format_idc == 3) { //444
    *subsampling_x = 0;
    *subsampling_y = 0;
  } else if (chroma_format_idc == 2) { //422
    *subsampling_x = 1;
    *subsampling_y = 0;
  } else { //420/400
    *subsampling_x = 1;
    *subsampling_y = 1;
  }
}

enum DecRet SwAllocFilmGrainMem(const void *dwl_inst, u32 subsampling_x, u32 subsampling_y, u32 secure_mode,
                                struct DWLLinearMem *film_grain_mem) {
  ASSERT(film_grain_mem);
  enum DWLRet ret = DEC_OK;
  u32 size;

  size = 256 * 3 + sizeof(i16) * 64 * 64 + sizeof(i16) * (!subsampling_y ? 64 : 32) * (!subsampling_x ? 64 : 32) * 2;

  film_grain_mem->mem_type |= DWL_MEM_TYPE_SYNC_HOST_TO_DEVICE;
  SET_MEM_USAGE(film_grain_mem->mem_type, DWL_MEM_USAGE_IN_GRAIN, secure_mode);
  if (film_grain_mem->size != NEXT_MULTIPLE(size, DEC_X170_BUS_ADDR_ALIGNMENT)){
    if (film_grain_mem->virtual_address != NULL) {
      DWLFreeLinear(dwl_inst, film_grain_mem);
      film_grain_mem->virtual_address = NULL;
    }
    ret = DWLMallocLinear(dwl_inst, size, film_grain_mem);
    if (ret) return DEC_ERROR;
  }

  return DEC_OK;
}