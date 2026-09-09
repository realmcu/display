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

#include "basetype.h"
#include "regdrv.h"
#include "pnghwd_asic.h"
#include "pngdeccontainer.h"
#include "pngdecutils.h"
#include "dwl.h"
#include "commonconfig.h"
#include "commonfunction.h"
#include "vpufeature.h"
#include "ppu.h"
#include "delogo.h"
#include <string.h>
#include "dec_log.h"
#include <math.h>

u32 PngRunAsic(PngDecContainer *dec_cont) {
  ASSERT(dec_cont);
  struct DWLReqInfo info = {0};
  enum DWLRet ret;
  u32 asic_status;
  av_unused const struct DecHwFeatures *hw_feature = dec_cont->cur_hw_feature;

  /* Check if first InitHw call */
  if(dec_cont->asic_running == 0) {
    /* Check if HW resource is available */
    info.core_mask = dec_cont->seq_core_mask;
    info.width = dec_cont->Hdrs.pic_width;
    info.height = dec_cont->Hdrs.pic_height;
    info.owner = (void *)dec_cont;
    if (!dec_cont->vcmd_used) {
      if(DWLReserveHw(dec_cont->dwl,  &info, &dec_cont->core_id) == DWL_ERROR) {
        APITRACEERR("%s","PngDecInitHW: ERROR hw resource unavailable\n");
        return DEC_HW_RESERVED;
      }
    } else {
      /* Always store registers in png_regs[0] when VCMD is used.*/
      dec_cont->core_id = 0;
      if (DWLReserveCmdBuf(dec_cont->dwl, &info, &dec_cont->cmd_buf_id) == DWL_ERROR) {
        APITRACEERR("%s","PngDecInitHW: ERROR command buffer unavailable\n");
        return DEC_HW_RESERVED;
      }
    }
  }

  /*************** Set swreg4 data ************/

  APITRACEDEBUG("%s","INTERNAL: Set Frame width\n");
  SetDecRegister(dec_cont->png_regs, HWIF_PIC_WIDTH_IN_CBS,
                  dec_cont->Hdrs.pic_width>> 3);

  APITRACEDEBUG("%s","INTERNAL: Set Frame height\n");
  SetDecRegister(dec_cont->png_regs, HWIF_PIC_HEIGHT_IN_CBS,
                  dec_cont->Hdrs.pic_height >> 3);

   /* Crop when hight is multiple of 8 instead of 16. */
  if (dec_cont->Hdrs.pic_height & 0x7) {
    SetDecRegister(dec_cont->png_regs, HWIF_MB_HEIGHT_OFF, dec_cont->Hdrs.pic_height & 0x7);
  } else
    SetDecRegister(dec_cont->png_regs, HWIF_MB_HEIGHT_OFF, 0);

  if (dec_cont->Hdrs.pic_width & 0x7) {
    SetDecRegister(dec_cont->png_regs, HWIF_MB_WIDTH_OFF, dec_cont->Hdrs.pic_width & 0x7);
  } else
    SetDecRegister(dec_cont->png_regs, HWIF_MB_WIDTH_OFF, 0);


  APITRACEDEBUG("%s","INTERNAL: Set output write disabled\n");
  SetDecRegister(dec_cont->png_regs, HWIF_DEC_OUT_DIS, 1);


  APITRACEDEBUG("%s","INTERNAL: Set filtering disabled\n");
  SetDecRegister(dec_cont->png_regs, HWIF_FILTERING_DIS, 1);

  APITRACEDEBUG("%s","INTERNAL: Set decoding mode: PNG\n");
  SetDecRegister(dec_cont->png_regs, HWIF_DEC_MODE, DEC_MODE_PNG);

  APITRACEDEBUG("%s","INTERNAL: Set color typen\n");
  SetDecRegister(dec_cont->png_regs, HWIF_DEC_COLOR_TYPE, dec_cont->Hdrs.color_type);

  APITRACEDEBUG("%s","INTERNAL: Set lz77 mode\n");
  SetDecRegister(dec_cont->png_regs, HWIF_LZ77_MODE, dec_cont->lz77_mode);
  if (dec_cont->lz77_mode == LZ77_UNIVERSAL_MODE){
    SetDecRegister(dec_cont->png_regs, HWIF_PIC_WIDTH_4X4, dec_cont->lz77_tail_size);
  }

  //dec_cont->stream = dec_cont->hw_stream;
  /* Set PNG operation mode */
  APITRACEDEBUG("%s","INTERNAL: Set PNG operation mode\n");

  /* only support bit_depth = 8 except index colour*/
  SetDecRegister(dec_cont->png_regs, HWIF_BIT_DEPTH_Y_MINUS8, 0);
  if(dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_INDEX) {
    SetDecRegister(dec_cont->png_regs, HWIF_BIT_DEPTH_INDEX_LOG2, log2(dec_cont->Hdrs.bit_depth));
  }

  SetDecRegister(dec_cont->png_regs, HWIF_UNIQUE_ID, UNIQUE_ID(0));

  /* set up stream position for HW decode */
  APITRACEDEBUG("%s","INTERNAL: Set stream position for HW\n");
  PngDecSetHwStrmParams(dec_cont);

  /* PP depending register writes */
  if(dec_cont->pp_enabled) {
    ASSERT(dec_cont->pp_enabled);
    if (dec_cont->pp_enabled /*&& ((asic_id & 0x0000F000) >> 12 >= 7)*/) {
      PpUnitIntConfig tmp_ppu_cfg[DEC_MAX_PPU_COUNT];
      DWLmemcpy(tmp_ppu_cfg, dec_cont->ppu_cfg, sizeof(dec_cont->ppu_cfg));
     /* PPSetRegs need to add slice_mb_set_value support in future  */
      DelogoSetRegs(dec_cont->png_regs, hw_feature, dec_cont->delogo_params);
      struct DWLLinearMem *ppu_out_addr =
              (struct DWLLinearMem *)InputQueueGetBufByID(dec_cont->pp_buffer_queue,
                                                          dec_cont->cur_out_ext_buf_index);
      PngAsicBuffers *asic_buff = &dec_cont->asic_buff;
      struct DWLDecBuffers dec_buffers = PngPrepareCurDecBuffers(dec_cont, asic_buff->pp_luma_buffer,
                                            asic_buff->pp_chroma_buffer, asic_buff->pp_chroma_buffer2);
      u32 pp_out_ctrl = 0;
      if (!dec_cont->ppu_cfg[0].sbi_en)
        pp_out_ctrl = InputQueueGetPPOutCtrl(dec_cont->pp_buffer_queue, &dec_buffers.dec_buffer[0][0]);
      struct PpParams pp_args = { hw_feature,
                                  dec_cont->ppu_cfg,
                                  ppu_out_addr,
                                  1,
                                  0,
                                  pp_out_ctrl,
                                  dec_cont->common_cfg.use_separated_pp,
                                  dec_cont->sbi_pic_start,
                                  0,
                                  0
                                };
      PPSetRegs(dec_cont->png_regs, &pp_args);
      dec_cont->sbi_pic_start = 1;
    }
  }
  APITRACEDEBUG("%s","INTERNAL: Enable png\n");

  SetDecRegister(dec_cont->png_regs, HWIF_DEC_E, 1);
  if(dec_cont->decode_stat & PNG_HAVE_PALETTE) {
    DWLSyncMemData(dec_cont->dwl, &dec_cont->palette_buf, 0,
                   dec_cont->palette_buf.size, HOST_TO_DEVICE);
    SET_ADDR_REG(dec_cont->png_regs, HWIF_PNG_PALETTE_BUF,
                 dec_cont->palette_buf.bus_address);
  }

  if(dec_cont->decode_stat & PNG_HAVE_tRNS) {
    SetDecRegister(dec_cont->png_regs, HWIF_PNG_TRNS_ENABLE, dec_cont->has_tRNS);
    SetDecRegister(dec_cont->png_regs, HWIF_PNG_TRNS_SAMPLE, dec_cont->tRNS_sample);
  }

  /* Flush regs to hw register */
  PngFlushRegs(dec_cont);

  if (!dec_cont->vcmd_used)
    DWLEnableHw(dec_cont->dwl, dec_cont->core_id, 4 * 1, dec_cont->png_regs[1]);
  else {
    if(dec_cont->crc_enabled) {
      DWLmemset(dec_cont->vcmd_m2mp_data, 0, sizeof(dec_cont->vcmd_m2mp_data));
      CommonSetM2MPComands(dec_cont->ppu_cfg, dec_cont->vcmd_m2mp_data, dec_cont->asic_buff.pp_luma_buffer[0].bus_address,
                           dec_cont->pp_enabled, 0, hw_feature->crop_step_rshift,
                           dec_cont->Hdrs.pic_width, dec_cont->Hdrs.pic_height,
                           8, 0, dec_cont->align);
      DWLCmdM2MPSendData(dec_cont->dwl, NULL, dec_cont->vcmd_m2mp_data, dec_cont->cmd_buf_id);
    }
    DecLinkAndRunCmds(dec_cont->dwl, NULL, dec_cont->cmd_buf_id);
  }
  dec_cont->asic_running = 1;
  if (dec_cont->vcmd_used) {
    ret = DWLWaitCmdBufReady(dec_cont->dwl, dec_cont->cmd_buf_id);
    if(dec_cont->crc_enabled) {
      CrcData *tmp;
      SwGetCrcMem(&dec_cont->crc, &tmp);
      DWLRefreshCrcResult(dec_cont->dwl, &tmp, dec_cont->cmd_buf_id);
    }
  } else {
    ret = DWLWaitHwReady(dec_cont->dwl, dec_cont->core_id,
                         (u32)DEC_X170_TIMEOUT_LENGTH);
  }

  if (ret != DWL_HW_WAIT_OK) {
    APITRACEDEBUG("%s","DWLWaitHwReady\n");
    APITRACEDEBUG("%s: %d\n", "DWLWaitHwReady returned",ret);

    /* Reset HW */
    SetDecRegister(dec_cont->png_regs, HWIF_DEC_IRQ_STAT, 0);
    SetDecRegister(dec_cont->png_regs, HWIF_DEC_IRQ, 0);

    dec_cont->asic_running = 0;
    if (!dec_cont->vcmd_used) {
      DWLDisableHw(dec_cont->dwl, dec_cont->core_id, 4 * 1,
                   dec_cont->png_regs[1]);
      (void) DWLReleaseHw(dec_cont->dwl, dec_cont->core_id);
    }
    else
      (void) DWLReleaseCmdBuf(dec_cont->dwl, dec_cont->cmd_buf_id);

    return (ret == DWL_HW_WAIT_ERROR) ? X170_DEC_SYSTEM_ERROR : X170_DEC_SYSTEM_TIMEOUT;
  }

  PngRefreshRegs(dec_cont);
  asic_status = GetDecRegister(dec_cont->png_regs, HWIF_DEC_IRQ_STAT);

  SetDecRegister(dec_cont->png_regs, HWIF_DEC_IRQ_STAT, 0);
  SetDecRegister(dec_cont->png_regs, HWIF_DEC_IRQ, 0); /* just in case */

  return asic_status;
}


void PngFlushRegs(PngDecContainer *dec_cont) {
  if(dec_cont->vcmd_used)
    DWLFlushRegister(dec_cont->dwl, dec_cont->cmd_buf_id, dec_cont->png_regs,
                     dec_cont->mc_refresh_regs, dec_cont->core_id);
  else
    FlushDecRegisters(dec_cont->dwl, dec_cont->core_id, dec_cont->png_regs,
                      dec_cont->cur_hw_feature->max_regs);
}

void PngRefreshRegs(PngDecContainer *dec_cont) {
  u32 *dec_regs = dec_cont->png_regs;

  if(dec_cont->vcmd_used) {
    DWLRefreshRegister(dec_cont->dwl, dec_cont->cmd_buf_id, dec_regs);
  } else {
    DWLReadRegs(dec_cont->dwl, dec_cont->core_id, 4*1, &dec_regs[1], dec_cont->cur_hw_feature->max_regs);
  }
}

void PngDecSetHwStrmParams(PngDecContainer *dec_cont) {
#define PNG_STR     dec_cont->stream
#define PNG_HW_STR     dec_cont->hw_stream
  addr_t addr_tmp = 0;
  u32 amount_of_stream = 0;
  addr_t mask;
  u32 buf_len = 0;
  av_unused const struct DecHwFeatures *hw_feature = dec_cont->cur_hw_feature;

  /* calculate and set stream start address to hw */
  APITRACEDEBUG("INTERNAL: read bits %d\n", PNG_HW_STR.read_bits);
  APITRACEDEBUG("INTERNAL: read bytes %d\n", PNG_HW_STR.read_bits / 8);
  APITRACEDEBUG("INTERNAL: Stream bus start 0x%08lx\n",
                          PNG_HW_STR.stream_bus);
  APITRACEDEBUG("INTERNAL: Stream virtual start 0x%p\n",
                          PNG_HW_STR.p_start_of_stream);


  mask = 15;

  /* calculate and set stream start address to hw */
  addr_tmp = (PNG_HW_STR.stream_bus +
              (addr_t)(PNG_HW_STR.p_curr_pos - PNG_HW_STR.p_start_of_buffer)) & (~mask);
  SET_ADDR_REG(dec_cont->png_regs, HWIF_RLC_VLC_BASE, addr_tmp);
  dec_cont->png_hw_start_bus = addr_tmp;

  APITRACEDEBUG("INTERNAL: Stream bus start 0x%08lx\n",
                          PNG_HW_STR.stream_bus);

  PNG_HW_STR.bit_pos_in_byte += ((size_t) PNG_HW_STR.p_curr_pos & mask) * 8;
  SetDecRegister(dec_cont->png_regs, HWIF_STRM_START_BIT, PNG_HW_STR.bit_pos_in_byte);

  /* set up stream length for HW.
   * length = all idat size */
  PNG_HW_STR.p_curr_pos = (u8 *)(size_t) ((size_t) PNG_HW_STR.p_curr_pos & (~mask));

  amount_of_stream = (u32) ((PNG_STR.read_bits - (PNG_HW_STR.read_bits - PNG_HW_STR.bit_pos_in_byte)) >> 3);

  /* Set the buffer bus address for using ddr_low_latency */
  if(dec_cont->llstrminfo.low_latency == 1) {
    SwUpdateLowLatencyInfo(dec_cont->dwl, hw_feature, addr_tmp, 0,
                           DWL_CLIENT_TYPE_PNG_DEC, &dec_cont->llstrminfo, dec_cont->png_regs);
  }
  else {
    SetDecRegister(dec_cont->png_regs, HWIF_STREAM_LEN, amount_of_stream);
    DWLSyncMemDataExt(dec_cont->dwl, dec_cont->hw_stream.stream_bus,
                  (void *)dec_cont->hw_stream.p_start_of_buffer,
                  dec_cont->hw_stream.strm_buff_size, HOST_TO_DEVICE);
  }
  buf_len = (PNG_STR.stream_length - (u32)((PNG_HW_STR.read_bits - PNG_HW_STR.bit_pos_in_byte) >> 3));
  SetDecRegister(dec_cont->png_regs, HWIF_STRM_BUFFER_LEN, buf_len);
  SetDecRegister(dec_cont->png_regs, HWIF_STRM_START_OFFSET, 0);


  PNG_HW_STR.read_bits -= PNG_HW_STR.bit_pos_in_byte;

  APITRACEDEBUG("INTERNAL: PNG_HW_STR.stream_length %d\n",
                          PNG_HW_STR.stream_length);
  APITRACEDEBUG("INTERNAL: PNG_HW_STR.p_curr_pos 0x%p\n",
                          PNG_HW_STR.p_curr_pos);
  APITRACEDEBUG("INTERNAL: PNG_HW_STR.p_start_of_stream 0x%p\n",
                          PNG_HW_STR.p_start_of_stream);
  APITRACEDEBUG("INTERNAL: PNG_HW_STR.bit_pos_in_byte 0x%08x\n",
                          PNG_HW_STR.bit_pos_in_byte);

  return;

#undef PNG_STR
#undef PNG_HW_STR

}

u32 PngCheckHwStatus(PngDecContainer *dec_cont)
{
   u32 tmp = 0;
   if (dec_cont->vcmd_used) {
     tmp = dec_cont->png_regs[1];
   } else {
     tmp = DWLReadRegFromHw(dec_cont->dwl, dec_cont->core_id, 4 * 1);
   }

  if(tmp & 0x01)
    return 1;
  else
    return 0;
}