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

#include "version.h"
#include "dwl.h"
#include "basetype.h"
#include "pngdecapi.h"
#include "pngdeccontainer.h"
#include "regdrv.h"
#include "commonconfig.h"
#include "sw_util.h"
#include "vpufeature.h"
#include "ppu.h"
#include "delogo.h"
#include <string.h>
#include "low_latency.h"
#include "commonfunction.h"
#include "sw_util.h"
#include "png_decoder.h"
#include "pnghwd_asic.h"


#define PNGDEC_CLEAR_IRQ  SetDecRegister(dec_cont->png_regs, \
                                          HWIF_DEC_IRQ_STAT, 0); \
                           SetDecRegister(dec_cont->png_regs, \
                                          HWIF_DEC_IRQ, 0);
#define ABORT_MARKER                 3
#define PNG_MERGE 1

static u32 InitList(FrameBufferList *fb_list);
static void ReleaseList(FrameBufferList *fb_list);
static void PushOutputPic(FrameBufferList *fb_list, const PngDecOutput *pic, const PngDecImageInfo *info);
static u32 PeekOutputPic(FrameBufferList *fb_list, PngDecOutput *pic, PngDecImageInfo *info);
static void PngFillPictureStruct(PngDecContainer * dec_cont, PngDecOutput *p_dec_out);
static u32 PngCycleCount(PngDecContainer *dec_cont);
static u32 PngDecAllocatePalette(PngDecContainer *dec_cont);
static void PngDecClearStructs(PngDecContainer *dec_cont);
void PngSetExternalBufferInfo(PngDecInst dec_inst);

#ifdef CASE_INFO_STAT
#include "case_info.h"
extern CaseInfo case_info;
static void PngCaseInfoCollect(PngDecContainer *dec_cont, CaseInfo *case_info);
#endif


/*------------------------------------------------------------------------------
    5. Functions
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Function name: PngDecInit

        Functional description:
            Init png decoder

        Inputs:
            PngDecInst * dec_inst     a reference to the png decoder instance is
                                         stored here

        Outputs:
            DEC_OK
            DEC_INITFAIL
            DEC_PARAM_ERROR
            DEC_DWL_ERROR
            DEC_MEMFAIL

------------------------------------------------------------------------------*/
enum DecRet PngDecInit(PngDecInst * dec_inst, const void *dwl, struct PngDecConfig *dec_cfg) {
  PngDecContainer *dec_cont;
  enum DecRet ret = DEC_OK;

  APITRACE("%s","PngDecInit#\n");

  /* check that right shift on negative numbers is performed signed */
  /*lint -save -e* following check causes multiple lint messages */
  if(((-1) >> 1) != (-1)) {
    APITRACEERR("%s","PngDecInit# ERROR: Right shift is not signed\n");
    return (DEC_INITFAIL);
  }

  /*lint -restore */
  if(dec_inst == NULL || dwl == NULL) {
    APITRACEERR("%s","PngDecInit# ERROR: dec_inst == NULL\n");
    return (DEC_PARAM_ERROR);
  }
  *dec_inst = NULL;   /* return NULL instance for any error */

  dec_cont = (PngDecContainer *) DWLcalloc(1, sizeof(PngDecContainer));
  if(dec_cont == NULL) {
    APITRACEERR("%s","PngDecInit# Memory allocation failed\n");
    return (DEC_MEMFAIL);
  }
  DWLmemset(dec_cont, 0, sizeof(PngDecContainer));

  dec_cont->dwl = dwl;
  dec_cont->n_cores = DWLReadAsicCoreCount(dec_cont->dwl);
  SwCommonInfoInit(dwl, dec_cfg->misc_ctrl,  DWL_CLIENT_TYPE_PNG_DEC, dec_cfg->decoder_mode,
                   &dec_cont->core_mask, &dec_cont->secure_mode,
                   dec_cont->hw_features, &dec_cont->cur_hw_feature);
  dec_cont->seq_core_mask = dec_cont->core_mask;
  dec_cont->n_cores_available = SwGetCores(CORE_MASK(dec_cfg->misc_ctrl));

  // dec_cont->stream_consumed_callback.fn = dec_cfg->mcinit_cfg.stream_consumed_callback;
  // dec_cont->b_mc = dec_cfg->mcinit_cfg.mc_enable;
  // if (dec_cont->b_mc && dec_cont->n_cores_available > 1)
    SetDecRegister(dec_cont->png_regs, HWIF_DEC_MULTICORE_E, 0);

  /* reset internal structures */
  PngDecClearStructs(dec_cont);

  dec_cont->vcmd_used = DWLVcmdIsUsed(dec_cont->dwl);
  dec_cont->crc_enabled = dec_cfg->crc_enabled;

  dec_cont->png_regs[0] = DWLReadAsicID(dec_cont->dwl, DWL_CLIENT_TYPE_PNG_DEC);

  dec_cont->pp_enabled = 1;
  dec_cont->pp_buffer_queue = InputQueueInit(0);
  if (dec_cont->pp_buffer_queue == NULL) {
    ret = (DEC_MEMFAIL);
    goto err;
  }

  dec_cont->align = dec_cfg->align;
  dec_cont->use_adaptive_buffers = dec_cfg->use_adaptive_buffers;

  SetCommonConfigRegs(dec_cont->png_regs);
  dec_cont->decode_stat = PNG_DEC_INIT;

  if (dec_cfg->decoder_mode & DEC_LOW_LATENCY) {
    if (SwSetLowLatencyInfo(dec_cont->dwl, DWL_CLIENT_TYPE_PNG_DEC,
                            &dec_cont->llstrminfo, dec_cont->png_regs)) {
      ret = DEC_MEMFAIL;
      goto err;
    }
    if (dec_cont->llstrminfo.low_latency == 1)
      dec_cont->llstrminfo.stream_info.low_latency = 1;
  } else {
    SetDecRegister(dec_cont->png_regs, HWIF_BUFFER_EMPTY_INT_E, 1);
    SetDecRegister(dec_cont->png_regs, HWIF_BLOCK_BUFFER_MODE_E, 0);
  }

  /* Init frame buffer list */
  InitList(&dec_cont->fb_list);

  *dec_inst = (PngDecContainer *) dec_cont;

  APITRACE("%s","PngDecInit# OK\n");
  return (DEC_OK);

err:
  if(dec_cont->pp_buffer_queue)
    InputQueueRelease(dec_cont->pp_buffer_queue);
  DWLfree(dec_cont);
  return ret;
}

/* Release */
void PngDecRelease(PngDecInst dec_inst) {
  PngDecContainer *dec_cont = (PngDecContainer *)dec_inst;
  u32 i = 0;

  APITRACE("%s","PngDecRelease#\n");

  if(dec_cont == NULL) {
    APITRACEERR("%s","PngDecRelease# ERROR: dec_inst == NULL\n");
    return;
  }

  if(dec_cont->asic_running) {
    if (!dec_cont->vcmd_used) {
      /* Release HW */
      DWLDisableHw(dec_cont->dwl, dec_cont->core_id, 4 * 1, 0);
      DWLReleaseHw(dec_cont->dwl, dec_cont->core_id);
    }
    else {
      DWLReleaseCmdBuf(dec_cont->dwl, dec_cont->cmd_buf_id);
    }
  }

/* TODO realease other buffer here , such as palette buffer */
  if(dec_cont->palette_buf.virtual_address) {
    DWLFreeLinear(dec_cont->dwl, &dec_cont->palette_buf);
  }

  for (i = 0; i < DEC_MAX_PPU_COUNT; i++) {
    if (dec_cont->ppu_cfg[i].lanczos_table.virtual_address) {
      DWLFreeLinear(dec_cont->dwl, &dec_cont->ppu_cfg[i].lanczos_table);
      dec_cont->ppu_cfg[i].lanczos_table.virtual_address = NULL;
    }
  }

  SwReleaseLowLatencyInfo(dec_cont->dwl, &dec_cont->llstrminfo);

  if (dec_cont->pp_buffer_queue) InputQueueRelease(dec_cont->pp_buffer_queue);
  if (dec_cont->crc_enabled && dec_cont->crc) SwFreeCrcMem(&dec_cont->crc);

  ReleaseList(&dec_cont->fb_list);
  if(dec_inst) {
    DWLfree(dec_cont);
  }

  APITRACE("%s","PngDecRelease# OK\n");

  return;

}

/* Get image information of the IHDR */
enum DecRet PngDecGetImageInfo(PngDecInst dec_inst, PngDecInput *p_dec_in, PngDecImageInfo *info) {
  u32 ret;
  PngDecContainer *dec_cont = (PngDecContainer *)dec_inst;

    /* Store the stream parameters */
  DWLmemset(&dec_cont->hw_stream, 0, sizeof(StreamStorage));
  dec_cont->stream.bit_pos_in_byte = 0;
  dec_cont->stream.p_start_of_stream =
    dec_cont->stream.p_curr_pos = (u8 *) p_dec_in->stream;
  dec_cont->stream.stream_bus = p_dec_in->stream_buffer.bus_address;
  dec_cont->stream.p_start_of_buffer =
    (u8 *) p_dec_in->stream_buffer.virtual_address;
  dec_cont->stream.strm_buff_size = p_dec_in->stream_buffer.logical_size;
  dec_cont->stream.read_bits = 0;
  dec_cont->stream.stream_length = p_dec_in->stream_length;

  /* stream length */
  if(!p_dec_in->buffer_size)
    dec_cont->stream.stream_length = p_dec_in->stream_length;
  else
    dec_cont->stream.stream_length = p_dec_in->buffer_size;

  ret = PngDecode(dec_cont);
  if(ret == PNG_HDRS_RDY) {
    info->coding_mode = dec_cont->image_info.coding_mode = PNGDEC_BASELINE;
    info->bit_depth = dec_cont->image_info.bit_depth = dec_cont->Hdrs.bit_depth;
    info->output_width = dec_cont->image_info.output_width = dec_cont->Hdrs.pic_width;
    info->output_height = dec_cont->image_info.output_height = dec_cont->Hdrs.pic_height;
    info->display_width =  dec_cont->image_info.display_width = info->output_width;
    info->display_height = dec_cont->image_info.display_height = info->output_height;
    info->interlaced = dec_cont->image_info.interlaced = dec_cont->Hdrs.interlaced;
    info->color_type = dec_cont->image_info.color_type  = dec_cont->Hdrs.color_type;

    /* check for minimum and maximum dimensions */
    if(dec_cont->lz77_mode != LZ77_UNIVERSAL_MODE && (dec_cont->Hdrs.pic_width < PNGDEC_MIN_WIDTH ||
        dec_cont->Hdrs.pic_height < PNGDEC_MIN_HEIGHT ||
        dec_cont->Hdrs.pic_width > dec_cont->cur_hw_feature->png_max_dec_width ||
        dec_cont->Hdrs.pic_height > dec_cont->cur_hw_feature->png_max_dec_height ||
        (dec_cont->Hdrs.pic_width * dec_cont->Hdrs.pic_height) >
          (dec_cont->cur_hw_feature->png_max_dec_width * dec_cont->cur_hw_feature->png_max_dec_height))) {
      APITRACEERR("PNGDecGetImageInfo# ERROR: Unsupported size WxH(%d, %d)\n",
                  dec_cont->Hdrs.pic_width, dec_cont->Hdrs.pic_height);
      return (DEC_UNSUPPORTED);
    }
    switch(dec_cont->Hdrs.color_type) {
      case PNG_COLOR_TYPE_GRAY:
        info->channels = 1;
        break;
      case LZ77_COLOR_TYPE_RGB565:
      case PNG_COLOR_TYPE_GRAY_ALPHA:
        info->channels = 2;
        break;
      case LZ77_COLOR_TYPE_RGB565_ALPHA:
      case PNG_COLOR_TYPE_RGB:
      case PNG_COLOR_TYPE_INDEX:
        info->channels = 3;
        if (dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_INDEX &&
            dec_cont->has_tRNS) //index-color with alpha
          info->channels = 4;
        break;
      case PNG_COLOR_TYPE_RGB_ALPHA:
        info->channels = 4;
        break;
      default:
        info->channels = 0;
        break;
    }
    dec_cont->channels = info->channels;
  } else {
    return DEC_STRM_ERROR;
  }
  return DEC_OK;
}

void PngSetAsicBuffer(const PngDecContainer *dec_cont, const struct DWLLinearMem *buffer,
                       PngAsicBuffers *asic_buff) {
  u32 i;
  u32 valid_2nd_addr = 0;
  const struct DWLDecBuffers *out_buf = (const struct DWLDecBuffers *)buffer;

  if (dec_cont->common_cfg.use_separated_pp == 0) {
    asic_buff->pp_luma_buffer[0] = out_buf->dec_buffer[0][0];
  }
  else {
    for (i=0; i<DEC_MAX_OUT_COUNT; i++) {
      if (!dec_cont->ppu_cfg[i].enabled) continue;

      // u32 use_multi_plane = SwGetChannelIsMultiPlane(out_buf, i);
      if (MAX_OUTPUT_PLANE >= 3 && DWL_DEVMEM_VAILD(out_buf->dec_buffer[i][1]))
        valid_2nd_addr = 1;
      if (valid_2nd_addr == 0)
        asic_buff->pp_luma_buffer[i] = out_buf->dec_buffer[i][0];
      else {
        asic_buff->pp_luma_buffer[i] = out_buf->dec_buffer[i][0];
        asic_buff->pp_chroma_buffer[i] = out_buf->dec_buffer[i][1];
        asic_buff->pp_chroma_buffer2[i] = out_buf->dec_buffer[i][2];
      }
    }
  }
}

enum DecRet PngDecDecode(PngDecInst dec_inst,
                         PngDecInput *p_dec_in, PngDecOutput *p_dec_out) {
  PngDecContainer *dec_cont = (PngDecContainer *)dec_inst;
  u32 i = 0, wait = 0, pp_index = 0xFF;
  u32 asic_status = 0;
  u32 ret; /* Returned code container */

  struct DWLLinearMem *pp_buffer = NULL;
  PngAsicBuffers *asic_buff = NULL;

  APITRACE("%s","PngDecDecode#\n");

  /* check null */
  if(dec_inst == NULL || p_dec_in == NULL ||
      X170_CHECK_VIRTUAL_ADDRESS(p_dec_in->stream_buffer.virtual_address) ||
      X170_CHECK_BUS_ADDRESS(p_dec_in->stream_buffer.bus_address) ||
      p_dec_out == NULL) {
    APITRACEERR("%s","PngDecDecode# ERROR: NULL parameter\n");
    return (DEC_PARAM_ERROR);
  }

  /* check user allocated null */
  if((p_dec_in->picture_buffer_y.virtual_address != NULL &&
       p_dec_in->picture_buffer_y.bus_address == 0) ||
      (p_dec_in->picture_buffer_cb_cr.virtual_address != NULL &&
       p_dec_in->picture_buffer_cb_cr.bus_address == 0)) {
    APITRACEERR("%s","PngDecDecode# ERROR: NULL parameter\n");
    return (DEC_PARAM_ERROR);
  }

  if(dec_cont->abort)
    return (DEC_ABORTED);

  p_dec_out->bit_depth = 0;
  for (i = 0; i < DEC_MAX_OUT_COUNT; i++) {
    p_dec_out->pictures[i].output_picture_y.virtual_address = NULL;
    p_dec_out->pictures[i].output_picture_y.bus_address = 0;
    p_dec_out->pictures[i].output_picture_cb_cr.virtual_address = NULL;
    p_dec_out->pictures[i].output_picture_cb_cr.bus_address = 0;
    p_dec_out->pictures[i].output_picture_cr.virtual_address = NULL;
    p_dec_out->pictures[i].output_picture_cr.bus_address = 0;
    p_dec_out->pictures[i].output_width = 0;
    p_dec_out->pictures[i].output_height = 0;
    p_dec_out->pictures[i].display_width = 0;
    p_dec_out->pictures[i].display_height = 0;
    p_dec_out->pictures[i].pic_stride = 0;
    p_dec_out->pictures[i].pic_stride_ch = 0;
  }
  asic_buff = &dec_cont->asic_buff;

  if (!dec_cont->ext_buffer_num) {
    p_dec_out->output.data_left = p_dec_in->stream_length;
    p_dec_out->output.strm_curr_pos = (u8 *)p_dec_in->stream;
    return DEC_WAITING_FOR_BUFFER;
  }

  do {
    ret = PngDecode(dec_cont);
    switch(ret) {
      case PNG_PALETTE_PARSED:
        if(!dec_cont->allocated) {
          ret = PngDecAllocatePalette(dec_cont);
          if(ret != DEC_OK) {
            APITRACEERR("%s","PNG: ALLOCATE ERROR\n");
            goto end;
          }
            /* update */
          dec_cont->allocated = 1;
        }
        DWLmemcpy(dec_cont->palette_buf.virtual_address, dec_cont->palette, sizeof(dec_cont->palette));
        p_dec_out->output.strm_curr_pos = dec_cont->stream.p_curr_pos;
        p_dec_out->output.data_left = p_dec_in->stream_length - (dec_cont->stream.read_bits >> 3);
        break;
      case PNG_tRNS_PARSED:
        if(dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_INDEX)
          DWLmemcpy(dec_cont->palette_buf.virtual_address, dec_cont->palette, sizeof(dec_cont->palette));
        p_dec_out->output.strm_curr_pos = dec_cont->stream.p_curr_pos;
        p_dec_out->output.data_left = p_dec_in->stream_length - (dec_cont->stream.read_bits >> 3);
        break;
      case PNG_PIC_RDY:
        pp_buffer = InputQueueGetBuffer(dec_cont->pp_buffer_queue, &wait);
        if (pp_buffer == NULL) {
          if (dec_cont->abort)
            return DEC_ABORTED;
          else {
            p_dec_out->output.data_left = p_dec_in->stream_length;
            p_dec_out->output.strm_curr_pos = (u8 *)p_dec_in->stream;
            return DEC_NO_DECODING_BUFFER;
          }
        }
        pp_index = wait;
        InputQueueSetPPOutCtrl(dec_cont->pp_buffer_queue, pp_buffer, PP_OUT_CTRL(p_dec_in->dec_ctrl));
        dec_cont->cur_out_ext_buf_index = pp_index;
        PngSetAsicBuffer(dec_cont, pp_buffer, asic_buff);
#ifdef ENABLE_FPGA_VERIFICATION
        DWLLinearMemset(dec_cont->dwl, pp_buffer, 0, 0, pp_buffer->size);
#endif

        asic_status = PngRunAsic(dec_cont);
        APITRACE("asic_status = %d\n", asic_status);
#ifdef CASE_INFO_STAT
        PngCaseInfoCollect(dec_cont, &case_info);
#endif
        p_dec_out->output.strm_curr_pos = dec_cont->stream.p_curr_pos;
        if (!dec_cont->llstrminfo.low_latency) {
          p_dec_out->output.data_left = p_dec_in->stream_length - (dec_cont->stream.read_bits >> 3);
        } else {
          p_dec_out->output.data_left = p_dec_in->stream_length - (dec_cont->idat_stream.read_bits >> 3);
          dec_cont->stream = dec_cont->idat_stream;
        }
        if(asic_status & DEC_HW_IRQ_RDY) {
          PngFillPictureStruct(dec_cont, p_dec_out);
          SwGetCrcLast(&dec_cont->crc, &p_dec_out->crc);
          PushOutputPic(&dec_cont->fb_list, p_dec_out, &dec_cont->image_info);

          if (!dec_cont->vcmd_used) {
            DWLDisableHw(dec_cont->dwl, dec_cont->core_id, 4 * 1,
                         dec_cont->png_regs[1]);
            /* Release HW */
            (void) DWLReleaseHw(dec_cont->dwl, dec_cont->core_id);
          }
          else {
            DWLReleaseCmdBuf(dec_cont->dwl, dec_cont->cmd_buf_id);
          }
          /* update asic_running */
          dec_cont->asic_running = 0;
          return DEC_PIC_RDY;
        } else if(asic_status & DEC_HW_IRQ_BUS) {
          APITRACEERR
          ("%s","PngDecDecode# DEC_HW_IRQ_BUS\n");
          /* clear interrupts */
          PNGDEC_CLEAR_IRQ;

          SetDecRegister(dec_cont->png_regs, HWIF_DEC_E, 0);
          if (!dec_cont->vcmd_used) {
            DWLDisableHw(dec_cont->dwl, dec_cont->core_id, 4 * 1,
                         dec_cont->png_regs[1]);
            /* Release HW */
            (void) DWLReleaseHw(dec_cont->dwl, dec_cont->core_id);
          }
          else {
            DWLReleaseCmdBuf(dec_cont->dwl, dec_cont->cmd_buf_id);
          }
          /* update asic_running */
          dec_cont->asic_running = 0;

          ret = DEC_HW_BUS_ERROR;
          goto end;
        } else if(asic_status & DEC_HW_IRQ_ERROR ||
                  asic_status & DEC_HW_IRQ_TIMEOUT ||
                  asic_status & DEC_HW_IRQ_ABORT ||
                  asic_status & DEC_HW_IRQ_ADLER32_ERROR ||
                  asic_status & DEC_HW_IRQ_CRC_ERROR) {
          if(asic_status & DEC_HW_IRQ_ERROR) {
            APITRACEERR
            ("%s","PngDecDecode# DEC_HW_IRQ_ERROR\n");
          } else if(asic_status & DEC_HW_IRQ_TIMEOUT) {
            APITRACEERR
            ("%s","PngDecDecode# DEC_HW_IRQ_TIMEOUT\n");
          } else if(asic_status & DEC_HW_IRQ_ADLER32_ERROR) {
            APITRACEERR
            ("%s","PngDecDecode# DEC_HW_IRQ_ADLER32_ERROR\n");
          } else if(asic_status & DEC_HW_IRQ_CRC_ERROR) {
            APITRACEERR
            ("%s","PngDecDecode# DEC_HW_IRQ_CRC_ERROR\n");
          } else {
            APITRACEERR
            ("%s","PngDecDecode# DEC_X170_IRQ_ABORT\n");
          }

          /* clear interrupts */
          PNGDEC_CLEAR_IRQ;

          SetDecRegister(dec_cont->png_regs, HWIF_DEC_E, 0);
          /* Release HW */
          if (!dec_cont->vcmd_used) {
            DWLDisableHw(dec_cont->dwl, dec_cont->core_id, 4 * 1,
                         dec_cont->png_regs[1]);

            (void)DWLReleaseHw(dec_cont->dwl, dec_cont->core_id);
          } else
            DWLReleaseCmdBuf(dec_cont->dwl, dec_cont->cmd_buf_id);

          /* update asic_running */
          dec_cont->asic_running = 0;
          /* also need to set ouput */

          PngFillPictureStruct(dec_cont, p_dec_out);
          SwGetCrcLast(&dec_cont->crc, &p_dec_out->crc);
          PushOutputPic(&dec_cont->fb_list, p_dec_out, &dec_cont->image_info);
          PngDecClearStructs(dec_cont);
          ret = DEC_STRM_ERROR;
          goto end;
        }
        break;
      case PNG_IEND_PARSED:
        ret = DEC_END_OF_STREAM;
        break;
      case PNG_MEMFAIL:
      case PNG_HDRS_ERROR:
      case PNG_STREAM_ERROR:
        goto end;
      case PNG_OTHERS_PARSED:
      default:
        p_dec_out->output.strm_curr_pos = dec_cont->stream.p_curr_pos;
        p_dec_out->output.data_left = p_dec_in->stream_length - (dec_cont->stream.read_bits >> 3);
        break;
    }
  } while(!(dec_cont->decode_stat & PNG_HAVE_IEND));

end:
  p_dec_out->output.data_left = 0;
  p_dec_out->output.strm_curr_pos = (u8 *)p_dec_in->stream + p_dec_in->stream_length;
  if (pp_index != 0xFF)
    InputQueueReturnBuffer(dec_cont->pp_buffer_queue, pp_buffer, 0, pp_index);
  return (ret);
}

static u32 PeekOutputPic(FrameBufferList *fb_list, PngDecOutput *pic, PngDecImageInfo *info) {
  PngDecOutput *out;
  PngDecImageInfo *info_tmp;
#ifndef GET_OUTPUT_BUFFER_NON_BLOCK
#ifdef _HAVE_PTHREAD_H
  sem_wait(&fb_list->out_count_sem);
#else
  if(sem_wait(&fb_list->out_count_sem)) return 0;
#endif
#endif
  if (fb_list->abort)
    return ABORT_MARKER;

  pthread_mutex_lock(&fb_list->out_count_mutex);
  if (!fb_list->num_out) {
    if (fb_list->end_of_stream) {
      pthread_mutex_unlock(&fb_list->out_count_mutex);
      return 2;
    } else {
      pthread_mutex_unlock(&fb_list->out_count_mutex);
      return 0;
    }
  }

  pthread_mutex_unlock(&fb_list->out_count_mutex);

  ASSERT(fb_list->rd_id < MAX_FRAME_NUMBER);

  out = &fb_list->out_fifo[fb_list->rd_id].pic;
  info_tmp = &fb_list->out_fifo[fb_list->rd_id].info;
  /* pop from head */
  (void)DWLmemcpy(pic, out, sizeof(PngDecOutput));
  (void)DWLmemcpy(info, info_tmp, sizeof(PngDecImageInfo));

  pthread_mutex_lock(&fb_list->out_count_mutex);

  fb_list->num_out--;

  /* go to next output */
  fb_list->rd_id++;
  if (fb_list->rd_id >= MAX_FRAME_NUMBER)
    fb_list->rd_id = 0;

  pthread_mutex_unlock(&fb_list->out_count_mutex);

  return 1;
}

enum DecRet PngDecNextPicture(PngDecInst dec_inst,
                              PngDecOutput * output,
                              PngDecImageInfo *info) {
  PngDecContainer *dec_cont = (PngDecContainer *) dec_inst;
  u32 ret;

  if(dec_inst == NULL || output == NULL) {
    APITRACEERR("%s","PngDecNextPicture# ERROR: dec_inst or output is NULL\n");
    return (DEC_PARAM_ERROR);
  }

  if((ret = PeekOutputPic(&dec_cont->fb_list, output, info))) {
    if (ret == 3) {
      APITRACE("%s","PngDecNextPicture# DEC_ABORTED\n");
      return (DEC_ABORTED);
    }
    if(ret == 2) {
      APITRACE("%s","PngDecNextPicture# DEC_END_OF_STREAM\n");
      return (DEC_END_OF_STREAM);
    }
    if(ret == 1) {
     APITRACE("%s","PngDecNextPicture# PNGDEC_PIC_RDY\n");
      return (DEC_PIC_RDY);
    }
    if (ret == 0) {
      APITRACE("%s","PngDecNextPicture# DEC_OK\n");
      return (DEC_OK);
    }
  }
  return (DEC_OK);
}

static void PngConvertStruct_0(const PngDecContainer *dec_cont, const PngDecOutput *output,
                                struct DecOutputPictureAddr *out_pic_addr) {
  u32 i;

  for (i = 0; i < DEC_MAX_PPU_COUNT; i++) {
    if (!dec_cont->ppu_cfg[i].enabled) continue;

    out_pic_addr[i].luma_vir_addr = (u32 *)output->pictures[i].output_picture_y.virtual_address;
    out_pic_addr[i].luma_bus_addr = output->pictures[i].output_picture_y.bus_address;
    out_pic_addr[i].chroma_vir_addr = (u32 *)output->pictures[i].output_picture_cb_cr.virtual_address;
    out_pic_addr[i].chroma_bus_addr = output->pictures[i].output_picture_cb_cr.bus_address;
    out_pic_addr[i].chroma_cr_vir_addr = (u32 *)output->pictures[i].output_picture_cr.virtual_address;
    out_pic_addr[i].chroma_cr_bus_addr = output->pictures[i].output_picture_cr.bus_address;
  }
}

enum DecRet PngDecPictureConsumed(PngDecInst dec_inst, PngDecOutput *output) {
  PngDecContainer *dec_cont;
  u32 i;
  struct DWLDecBuffers output_picture;
  struct DecOutputPictureAddr out_pic_addr[DEC_MAX_OUT_COUNT];
  PpUnitIntConfig *ppu_cfg;
  APITRACE("%s","PngDecPictureConsumed#\n");

  if(dec_inst == NULL || output == NULL) {
    APITRACEERR("%s","PngDecPictureConsumed# ERROR: dec_inst or output is NULL\n");
    return (DEC_PARAM_ERROR);
  }
  dec_cont = (PngDecContainer *) dec_inst;
  ppu_cfg = dec_cont->ppu_cfg;
  DWLmemset(&output_picture, 0, sizeof(output_picture));
  DWLmemset(&out_pic_addr, 0, sizeof(out_pic_addr));
  PngConvertStruct_0(dec_cont, output, out_pic_addr);

  for (i = 0; i < DEC_MAX_PPU_COUNT; i++, ppu_cfg++) {
    if (!ppu_cfg->enabled)
      continue;
    else {
      SwConvertOutAddr(InputQueueGetBufByID(dec_cont->pp_buffer_queue, 0), i,
                  dec_cont->common_cfg.use_separated_pp, &out_pic_addr[i],
                  &output_picture);
      if (!dec_cont->common_cfg.use_separated_pp)
        break;
    }
  }
  u32 index = InputQueueReturnBuffer(dec_cont->pp_buffer_queue, output_picture.dec_buffer[0],
                                     1, output->pp_index);
  if (index >= dec_cont->ext_buffer_num) {
    ASSERT(0);
    APITRACEERR("PngDecPictureConsumed failed with index %d\n", index);
    return (DEC_PARAM_ERROR);
  }

  if (output->crc) SwSetCrcUnused(output->crc);
  return (DEC_OK);
}

#ifndef ASIC_TRACE_SUPPORT
static void PngDecStopSBI(PngDecContainer *dec_cont, u32 i) {
  u8 dummy[20] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00};
  u32 buffer_len = 4096;
  u32 stream_legth = sizeof(dummy);
  struct DWLLinearMem dummy_in;
  DWLmemset(&dummy_in, 0, sizeof(struct DWLLinearMem));
  addr_t tmp_addr;
  const struct DecHwFeatures *hw_feature = dec_cont->cur_hw_feature;
  DWLMallocLinear(dec_cont->dwl, 4096, &dummy_in);
  ASSERT(DWL_DEVMEM_VAILD(dummy_in));
  DWLmemcpy(dummy_in.virtual_address, dummy, stream_legth);
  tmp_addr = dummy_in.bus_address;
  tmp_addr &= (~(addr_t)DEC_HW_ALIGN_MASK);
  ASSERT(tmp_addr == dummy_in.bus_address);
  SET_ADDR_REG(dec_cont->png_regs, HWIF_RLC_VLC_BASE, tmp_addr);
  SetDecRegister(dec_cont->png_regs, HWIF_STRM_BUFFER_LEN, buffer_len);
  SetDecRegister(dec_cont->png_regs, HWIF_STREAM_LEN, stream_legth);
  SetDecRegister(dec_cont->png_regs, HWIF_STRM_START_OFFSET, 0);
  SetDecRegister(dec_cont->png_regs, HWIF_ERROR_CONCEAL_E, 0);

  u32 start = 0xFF, asic_status = 0;
  av_unused enum DWLRet ret = 0;
  struct DWLReqInfo info = {0};
  info.core_mask = dec_cont->seq_core_mask;
  info.width = dec_cont->Hdrs.pic_width;
  info.height = dec_cont->Hdrs.pic_height;
  info.owner = (void *)dec_cont;
  if (!dec_cont->vcmd_used)
    ret = DWLReserveHw(dec_cont->dwl, &info, &dec_cont->core_id);
  else
    ASSERT(0);
  PpUnitIntConfig *ppu_cfg = &dec_cont->ppu_cfg[i];
  ASSERT(dec_cont->pp_enabled && hw_feature->max_ppu_count);
  struct PpParams pp_args = {dec_cont->cur_hw_feature,
                             ppu_cfg,
                             dec_cont->asic_buff.pp_luma_buffer,
                             0, 0, 0, 0, 0,
                             start,
                             0
                            };
  PPSetRegs(dec_cont->png_regs, &pp_args);
  ClearSwIRQ(dec_cont->png_regs);
  SetDecRegister(dec_cont->png_regs, HWIF_DEC_E, 1);
  SetDecRegister(dec_cont->png_regs, HWIF_DEC_ABORT_E, 1);
  PngFlushRegs(dec_cont);
  if (!dec_cont->vcmd_used) {
    DWLEnableHw(dec_cont->dwl, dec_cont->core_id, 4 * 1, dec_cont->png_regs[1]);
    ret = DWLWaitHwReady(dec_cont->dwl, dec_cont->core_id, (u32) DEC_X170_TIMEOUT_LENGTH);
  }
  else
    ASSERT(0);
  PngRefreshRegs(dec_cont);
  asic_status = GetDecRegister(dec_cont->png_regs, HWIF_DEC_IRQ_STAT);
  ASSERT(asic_status & DEC_HW_IRQ_ABORT);
  if (!dec_cont->vcmd_used)
    (void) DWLReleaseHw(dec_cont->dwl, dec_cont->core_id);
  else
    ASSERT(0);

  DWLFreeLinear(dec_cont->dwl, &dummy_in);
}
#endif

enum DecRet PngDecEndOfStream(PngDecInst dec_inst) {
  PngDecContainer *dec_cont;
  u32 count = 0;

  if(dec_inst == NULL) {
    APITRACEERR("%s","PngDecNextPicture# ERROR: dec_inst or output is NULL\n");
    return (DEC_PARAM_ERROR);
  }
  dec_cont = (PngDecContainer *) dec_inst;

#ifndef ASIC_TRACE_SUPPORT
  for (u32 i = 0; i < DEC_MAX_PPU_COUNT; i++) {
    if (dec_cont->ppu_cfg[i].sbi_en) {
      usleep(10000);
      PngDecStopSBI(dec_cont, i);
      break;
    }
  }
#endif

  if(dec_cont->vcmd_used){
    DWLWaitCmdbufsDone(dec_cont->dwl, dec_inst);
  } else {
    /* Check all Core in idle state */
    for(count = 0; count < dec_cont->n_cores_available; count++) {
      while (dec_cont->dec_status[count] == DEC_RUNNING)
        sched_yield();
    }
  }

  /* wake-up output thread */
  PushOutputPic(&dec_cont->fb_list, NULL, NULL);
  return (DEC_OK);
}

enum DecRet PngDecSetInfo(PngDecInst dec_inst, struct PngDecConfig *dec_cfg) {
  PngDecContainer *dec_cont = (PngDecContainer *)dec_inst;
  PpUnitConfig *ppu_cfg = &dec_cfg->ppu_config[0];
  u32 i = 0, pixel_width = 8;
  enum DecRet ret = DEC_OK;
  u32 shaper_alignment;

  ret = SwSetCommonInfo(dec_cont->dwl, dec_cfg->misc_ctrl, DWL_CLIENT_TYPE_PNG_DEC,
            &dec_cont->core_mask, dec_cont->hw_features, &dec_cont->cur_hw_feature);
  if (ret != DEC_OK)
    return ret;

  shaper_alignment = (u32)dec_cont->cur_hw_feature->shaper_alignment;
  /* ref aligment */
  dec_cont->align = dec_cfg->align;

  /* rgb default*/
  dec_cont->pp_chroma_format = PP_CHROMA_400;

  /* ppu alignment */
  for (i = 0; i < DEC_MAX_PPU_COUNT; i++) {
    dec_cfg->ppu_config[i].align = dec_cfg->align;
    dec_cfg->ppu_config[i].chroma_format = PP_CHROMA_400;
    if ((u32)dec_cfg->ppu_config[i].align < shaper_alignment) {
      if (dec_cfg->ppu_config[i].comp_enabled && (dec_cont->cur_hw_feature->pp_comp_support[i] == 2)) { /*enable dec400c*/
        dec_cfg->ppu_config[i].align = shaper_alignment;
      } else {
        dec_cfg->ppu_config[i].shaper_no_pad = 0;
      }
    } else {
      if(dec_cont->cur_hw_feature->shaper_enabled[i]) {
        dec_cfg->ppu_config[i].shaper_no_pad = 0;
      } else if(!dec_cont->cur_hw_feature->shaper_enabled[i]){
        dec_cfg->ppu_config[i].shaper_no_pad = 1;
      }
    }
  }
  /* Range Mapping */
  for (i = 0; i < DEC_MAX_PPU_COUNT; i++) {
    dec_cont->ppu_cfg[i].source_range = 1; //PNG default full range
  }

  // PNG can set yuv/rgb source_range by option
  for (i = 0; i < DEC_MAX_PPU_COUNT; i++) {
    if (dec_cfg->ppu_config[i].set_source_range_enable) {
      dec_cont->ppu_cfg[i].source_range = dec_cfg->ppu_config[i].source_range;
    }
    dec_cont->ppu_cfg[i].dwl = dec_cont->dwl;
  }

  for(i = 0; i < DEC_MAX_PPU_COUNT; i++) {
    if(!ppu_cfg->rgb && !ppu_cfg->rgb_planar){
      if(dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_RGB ||
         dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_INDEX) {
        ppu_cfg->rgb_format = DEC_OUT_FRM_BGR888;
#ifdef SUPPORT_TRNS
        if(dec_cont->has_tRNS) ppu_cfg->rgb_format = DEC_OUT_FRM_ABGR888;
#endif
        ppu_cfg->rgb = 1;
      } else if (dec_cont->lz77_mode == 0 && dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
        ppu_cfg->rgb_format = DEC_OUT_FRM_ABGR888;
        ppu_cfg->rgb = 1;
      }
#ifdef SUPPORT_LZ77
      else if (dec_cont->lz77_mode == LZ77_PIC_MODE && dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
        ppu_cfg->rgb_format = DEC_OUT_FRM_ARGB888;
        ppu_cfg->rgb = 1;
      }
      else if (dec_cont->Hdrs.color_type == LZ77_COLOR_TYPE_RGB565) {
        ppu_cfg->rgb_format = DEC_OUT_FRM_RGB565;
        ppu_cfg->rgb = 1;
      } else if (dec_cont->Hdrs.color_type == LZ77_COLOR_TYPE_RGB565_ALPHA) {
        ppu_cfg->rgb_format = DEC_OUT_FRM_ARGB8565;
        ppu_cfg->rgb = 1;
      }
#endif
    }
#ifdef SUPPORT_LZ77
    ppu_cfg->lz77_mode = dec_cont->lz77_mode;
#endif
    //for GRAY/GRAY_ALPHA, force YUV400 output rather than RGB.
    if(dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_GRAY ||
      dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_GRAY_ALPHA){
#ifdef SUPPORT_TRNS
      if (ppu_cfg->monochrome == 0 && (dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_GRAY_ALPHA || dec_cont->has_tRNS)) {
        ppu_cfg->yuv400_with_alpha = 1;
      }
#endif
      ppu_cfg->rgb = ppu_cfg->rgb_planar = 0;
      ppu_cfg->monochrome = 1;
      if(ppu_cfg->tiled_e && ppu_cfg->tile_mode == TILED8x8) { //disable YUV400+TILED8X8 for PNG Only.
        APITRACEERR("%s","PngDecSetInfo# Unsupported TILED8X8 for PNG YUV400.\n");
        return DEC_PARAM_ERROR;
      }
    }
    ppu_cfg++;
  }
  ppu_cfg = &dec_cfg->ppu_config[0];
  PpUnitSetIntConfig(dec_cont->ppu_cfg, ppu_cfg, dec_cont->cur_hw_feature, pixel_width , 1,
                     dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_GRAY || dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_GRAY_ALPHA);

#ifdef PNG_MERGE
  for (i = 0; i < DEC_MAX_PPU_COUNT; i++) {
    if (dec_cont->ppu_cfg[i].lanczos_table.virtual_address == NULL) {
      u32 size = LANCZOS_COEFF_BUFFER_SIZE * sizeof(i16) * dec_cont->n_cores;
      dec_cont->ppu_cfg[i].lanczos_table.mem_type = DWL_MEM_TYPE_SYNC_HOST_TO_DEVICE | DWL_MEM_TYPE_VPU_CPU;
      SET_MEM_USAGE(dec_cont->ppu_cfg[i].lanczos_table.mem_type,
                    DWL_MEM_USAGE_IN_PPULANCZOS_TABLE, dec_cont->secure_mode);
      i32 ret = DWLMallocLinear(dec_cont->dwl, size, &dec_cont->ppu_cfg[i].lanczos_table);
      if (ret != 0)
        return(DEC_MEMFAIL);
    }
    if(ppu_cfg->rgb && (dec_cont->ppu_cfg[i].hist_global_e || dec_cont->ppu_cfg[i].hist_local_e)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d], Current PNG Stream can't support HIST LOCAL/GLOBAL feature.\n", i);
      return DEC_PARAM_ERROR;
    }
  }
#endif

  /*just to print in function 'PrintPpunitConfig()', don't check*/
  if (CheckPpUnitConfig(dec_cont->cur_hw_feature, dec_cont->Hdrs.pic_width,
          dec_cont->Hdrs.pic_height, 0, pixel_width, 0, dec_cont->ppu_cfg))
    return DEC_PARAM_ERROR;

  SwUpdateCommonCfg(dec_cfg->misc_ctrl, &dec_cont->common_cfg);
  if (dec_cont->common_cfg.use_separated_pp && !dec_cont->pp_enabled) {
    APITRACEERR("%s","PngDecSetInfo# pp muse be enabled if use separated pp.\n");
    return DEC_PARAM_ERROR;
  }

  DWLmemcpy(dec_cont->delogo_params, dec_cfg->delogo_params, sizeof(dec_cont->delogo_params));
  if (CheckDelogo(dec_cont->delogo_params, 8, 8))
    return DEC_PARAM_ERROR;
  return(DEC_OK);
}

void PngSetExternalBufferInfo(PngDecInst dec_inst) {
  PngDecContainer *dec_cont = (PngDecContainer *)dec_inst;
  u64 ext_buffer_size = 0;
  u32 buffers = 1;

  PpUnitIntConfig *ppu_cfg = dec_cont->ppu_cfg;
  ext_buffer_size = CalcPpUnitBufferSize(dec_cont->cur_hw_feature, ppu_cfg, 0, dec_cont->common_cfg.use_separated_pp);

  dec_cont->tot_buffers = dec_cont->buf_num = buffers;
  dec_cont->next_buf_size = ext_buffer_size;
}


enum DecRet PngDecGetBufferInfo(PngDecInst dec_inst, struct DecBufferInfo *mem_info) {
  PngDecContainer  *dec_cont = (PngDecContainer *)dec_inst;
  u32 i;// core_mask = 0;

  if(dec_cont == NULL || mem_info == NULL)
    return DEC_PARAM_ERROR;

  DWLmemset(mem_info, 0, sizeof(struct DecBufferInfo));

  if (dec_cont->pp_enabled) {
    for (i = 0; i < DEC_MAX_OUT_COUNT; i++) {
      if (!dec_cont->ppu_cfg[i].enabled) continue;
      mem_info->ystride[i] = dec_cont->ppu_cfg[i].ystride;
      mem_info->cstride[i] = dec_cont->ppu_cfg[i].cstride;
    }
  }

  u32 pic_width = (dec_cont->Hdrs.pic_width + 1) & ~0x1;
  u32 pic_height = (dec_cont->Hdrs.pic_height + 1) & ~0x1;

  if (CheckPpUnitConfig(dec_cont->cur_hw_feature, pic_width, pic_height, 0, 8,
                        dec_cont->pp_chroma_format, dec_cont->ppu_cfg))
    return DEC_PARAM_ERROR;

  PngSetExternalBufferInfo(dec_inst);
  if(dec_cont == NULL || mem_info == NULL) {
    return DEC_PARAM_ERROR;
  }

  mem_info->next_buf_size = dec_cont->next_buf_size;
  for (i=0; i<DEC_MAX_OUT_COUNT; i++)
    DWLmemcpy(mem_info->output_buf_size[i], dec_cont->ppu_cfg[i].total_buffer_size, MAX_OUTPUT_PLANE*sizeof(u64));
  mem_info->buf_num = dec_cont->buf_num;

  ASSERT((mem_info->buf_num && mem_info->next_buf_size) ||
         (SwCheckMemValid(&mem_info->buf_to_free, dec_cont->ppu_cfg, dec_cont->common_cfg.use_separated_pp)));

  return DEC_OK;
}

static void PngConvertStruct_2(const PngDecContainer *dec_cont, const struct DecOutputPictureAddr *out_pic_addr,
                               PngDecOutput *p_dec_out) {
  u32 i;

  for (i = 0; i < DEC_MAX_OUT_COUNT; i++) {
    if (!dec_cont->ppu_cfg[i].enabled) continue;

    p_dec_out->pictures[i].output_picture_y.virtual_address = (u32 *)out_pic_addr[i].luma_vir_addr;
    p_dec_out->pictures[i].output_picture_y.bus_address = out_pic_addr[i].luma_bus_addr;
    p_dec_out->pictures[i].output_picture_cb_cr.virtual_address = (u32 *)out_pic_addr[i].chroma_vir_addr;
    p_dec_out->pictures[i].output_picture_cb_cr.bus_address = out_pic_addr[i].chroma_bus_addr;
    p_dec_out->pictures[i].output_picture_cr.virtual_address = (u32 *)out_pic_addr[i].chroma_cr_vir_addr;
    p_dec_out->pictures[i].output_picture_cr.bus_address = out_pic_addr[i].chroma_cr_bus_addr;
  }
}

void PngSetPpOutBuffer(const PngDecContainer *dec_cont, PngDecOutput *p_dec_out) {
  u32 i = 0;
  const PpUnitIntConfig *ppu_cfg = dec_cont->ppu_cfg;
  struct DWLDecBuffers *out_buf = InputQueueGetBufByID(dec_cont->pp_buffer_queue, dec_cont->cur_out_ext_buf_index);
  struct DecOutputPictureAddr out_pic_addr[DEC_MAX_OUT_COUNT];
  struct SwParames sw_parames = {dec_cont->dwl, dec_cont->ppu_cfg, dec_cont->common_cfg.use_separated_pp};

  DWLmemset(out_pic_addr, 0, sizeof(out_pic_addr));
  p_dec_out->pp_index = dec_cont->cur_out_ext_buf_index;
  for (i = 0; i < DEC_MAX_OUT_COUNT; i++, ppu_cfg++) {
    if (!dec_cont->ppu_cfg[i].enabled) continue;

    SwSetPpOutAddr(&sw_parames, i, &out_buf->dec_buffer[0][0], NULL, &out_pic_addr[i]);
  }
  PngConvertStruct_2(dec_cont, out_pic_addr, p_dec_out);
}

void PngFillPictureStruct(PngDecContainer * dec_cont, PngDecOutput *p_dec_out) {
  u32 i = 0;
  PpUnitIntConfig *ppu_cfg = dec_cont->ppu_cfg;
  p_dec_out->bit_depth = dec_cont->Hdrs.bit_depth;
  for (i = 0; i < DEC_MAX_OUT_COUNT; i++, ppu_cfg++) {
    if (!ppu_cfg->enabled) continue;
      p_dec_out->pictures[i].output_width = ((dec_cont->ppu_cfg[i].scale.width + 15) >> 4) << 4;
      p_dec_out->pictures[i].output_height = dec_cont->ppu_cfg[i].scale.height;
      p_dec_out->pictures[i].display_width = dec_cont->ppu_cfg[i].scale.enabled ? dec_cont->ppu_cfg[i].scale.width : dec_cont->Hdrs.pic_width;
      p_dec_out->pictures[i].display_height = dec_cont->ppu_cfg[i].scale.height;
      p_dec_out->pictures[i].pic_stride = dec_cont->ppu_cfg[i].ystride;
      p_dec_out->pictures[i].pic_stride_ch = dec_cont->ppu_cfg[i].cstride;
      p_dec_out->pictures[i].output_format = TransUnitConfig2Format(ppu_cfg);
      if (ppu_cfg->dec400_enabled)
        PpFillDec400TblInfo(ppu_cfg,
                            dec_cont->asic_buff.pp_luma_buffer->virtual_address,
                            dec_cont->asic_buff.pp_luma_buffer->bus_address,
                            &p_dec_out->pictures[i].dec400_luma_table,
                            &p_dec_out->pictures[i].dec400_chroma_table);
  }

  PngSetPpOutBuffer(dec_cont, p_dec_out);
  p_dec_out->cycles_per_mb = PngCycleCount(dec_cont);
}

enum DecRet PngDecAddBuffer(PngDecInst dec_inst, struct DWLLinearMem *info) {
  PngDecContainer *dec_cont = (PngDecContainer *)dec_inst;
  enum DecRet dec_ret = DEC_OK;

  if(dec_inst == NULL || info == NULL)
    return DEC_PARAM_ERROR;

  u64 total_size = 0;
  if (!SwCheckBufferValid(dec_cont->next_buf_size, info, &total_size))

  dec_cont->n_ext_buf_size = total_size;
  if (!dec_cont->realloc_buffer) {
    dec_cont->ext_buffer_num++;

    InputQueueAddBuffer(dec_cont->pp_buffer_queue, info);
  } else {
    InputQueueUpdateBuffer(dec_cont->pp_buffer_queue, info, dec_cont->buffer_queue_index);
  }

  return dec_ret;
}


static void PushOutputPic(FrameBufferList *fb_list, const PngDecOutput *pic, const PngDecImageInfo *info) {
  if (pic != NULL && info != NULL) {
    pthread_mutex_lock(&fb_list->out_count_mutex);

    while(fb_list->num_out == MAX_FRAME_NUMBER) {
      /* make sure we do not overflow the output */
      /* pthread_cond_signal(&fb_list->out_empty_cv); */
      pthread_mutex_unlock(&fb_list->out_count_mutex);
      sched_yield();
      pthread_mutex_lock(&fb_list->out_count_mutex);
    }

    /* push to tail */
    if (fb_list->wr_id >= MAX_FRAME_NUMBER)
      fb_list->wr_id = 0;

    fb_list->out_fifo[fb_list->wr_id].pic = *pic;
    fb_list->out_fifo[fb_list->wr_id].info = *info;
    fb_list->out_fifo[fb_list->wr_id].mem_idx = fb_list->wr_id;
    fb_list->num_out++;
    fb_list->wr_id++;

    ASSERT(fb_list->num_out <= MAX_FRAME_NUMBER);

    pthread_mutex_unlock(&fb_list->out_count_mutex);
  } else {
    fb_list->end_of_stream = 1;
  }
  sem_post(&fb_list->out_count_sem);
}


static u32 PngCycleCount(PngDecContainer *dec_cont) {
  u32 cycles = 0;
  u32 mbs;
#ifdef FPGA_PERF_AND_BW
  struct DecInfo dec_info;
  DWLmemset(&dec_info, 0, sizeof(struct DecInfo));
  dec_info.pic_size = NEXT_MULTIPLE(dec_cont->Hdrs.pic_height, 16) * NEXT_MULTIPLE(dec_cont->Hdrs.pic_width, 16);
  dec_info.bit_depth = dec_cont->Hdrs.bit_depth;
  dec_info.chroma_format = dec_cont->pp_chroma_format;
  dec_info.codec = DEC_MODE_PNG;
  DecPerfInfoCount(dec_cont->dwl, dec_cont->core_id, &dec_cont->perf_info, dec_info);
#endif
  mbs = (NEXT_MULTIPLE(dec_cont->Hdrs.pic_height, 16) *
             NEXT_MULTIPLE(dec_cont->Hdrs.pic_width, 16)) >> 8;

  if (mbs)
    cycles = GetDecRegister(dec_cont->png_regs, HWIF_PERF_CYCLE_COUNT) / mbs;

  return cycles;
}


static void SetAbortStatusInList(FrameBufferList *fb_list) {
  if(fb_list == NULL || !fb_list->b_initialized)
    return;

  pthread_mutex_lock(&fb_list->ref_count_mutex);
  fb_list->abort = 1;
  pthread_mutex_unlock(&fb_list->ref_count_mutex);
  sem_post(&fb_list->out_count_sem);
}

static void ClearAbortStatusInList(FrameBufferList *fb_list) {
  if(fb_list == NULL || !fb_list->b_initialized)
    return;

  pthread_mutex_lock(&fb_list->ref_count_mutex);
  fb_list->abort = 0;
  pthread_mutex_unlock(&fb_list->ref_count_mutex);
}

static void ResetOutFifoInList(FrameBufferList *fb_list) {
  (void)DWLmemset(fb_list->out_fifo, 0, MAX_FRAME_NUMBER * sizeof(struct OutElement_));
  fb_list->wr_id = 0;
  fb_list->rd_id = 0;
  fb_list->num_out = 0;
}

void PngExistAbortState(PngDecContainer *dec_cont) {
  ClearAbortStatusInList(&dec_cont->fb_list);
  InputQueueClearAbort(dec_cont->pp_buffer_queue);
  dec_cont->abort = 0;
}

enum DecRet PngDecAbort(PngDecInst dec_inst) {
  PngDecContainer *dec_cont = (PngDecContainer *) dec_inst;
  if(dec_inst == NULL) {
    APITRACEERR("%s","PngDecAbort# ERROR: dec_inst or output is NULL\n");
    return (DEC_PARAM_ERROR);
  }

  SetAbortStatusInList(&dec_cont->fb_list);
  InputQueueSetAbort(dec_cont->pp_buffer_queue);
  dec_cont->abort = 1;

  return (DEC_OK);
}

enum DecRet PngDecAbortAfter(PngDecInst dec_inst) {
  PngDecContainer *dec_cont = (PngDecContainer *) dec_inst;

  if(dec_inst == NULL) {
    APITRACEERR("%s","PngDecAbortAfter# ERROR: dec_inst or output is NULL\n");
    return (DEC_PARAM_ERROR);
  }

  ResetOutFifoInList(&dec_cont->fb_list);
  InputQueueReset(dec_cont->pp_buffer_queue);
  dec_cont->ext_buffer_num = 0;
  ClearAbortStatusInList(&dec_cont->fb_list);
  InputQueueClearAbort(dec_cont->pp_buffer_queue);
  return (DEC_OK);
}

static u32 InitList(FrameBufferList *fb_list) {
  (void) DWLmemset(fb_list, 0, sizeof(*fb_list));

  sem_init(&fb_list->out_count_sem, 0, 0);
  pthread_mutex_init(&fb_list->out_count_mutex, NULL);

  pthread_mutex_init(&fb_list->ref_count_mutex, NULL );

  /* this CV is used to signal the HW has finished processing a picture
   * that is needed for output ( FB_OUTPUT | FB_HW_ONGOING )
   */
  pthread_cond_init(&fb_list->hw_rdy_cv, NULL);

  fb_list->b_initialized = 1;
  return 0;
}

static void ReleaseList(FrameBufferList *fb_list) {
  if (!fb_list->b_initialized)
    return;

  fb_list->b_initialized = 0;

  pthread_mutex_destroy(&fb_list->ref_count_mutex);

  pthread_mutex_destroy(&fb_list->out_count_mutex);
  pthread_cond_destroy(&fb_list->hw_rdy_cv);

  sem_destroy(&fb_list->out_count_sem);
}

static u32 PngDecAllocatePalette(PngDecContainer *dec_cont) {
  u32 tmp;
#ifdef SUPPORT_TRNS
  u32 pal_size = PNG_MAX_PALETTE_LENGTH * 4;
#else
  u32 pal_size = PNG_MAX_PALETTE_LENGTH * 3;
#endif
  pal_size = NEXT_MULTIPLE(pal_size, MAX(dec_cont->align, 16));
  dec_cont->palette_buf.mem_type = DWL_MEM_TYPE_SYNC_HOST_TO_DEVICE |
                                          DWL_MEM_TYPE_VPU_WORKING;
  tmp = DWLMallocLinear(dec_cont->dwl, (sizeof(u8) * pal_size), &dec_cont->palette_buf);
  if(tmp == -1) {
    return (DEC_MEMFAIL);
  }
  DWLmemset(dec_cont->palette_buf.virtual_address, 0x0, dec_cont->palette_buf.size);
  return DEC_OK;
}


static void PngDecClearStructs(PngDecContainer *dec_cont) {
  ASSERT(dec_cont);

  /* stream pointers */
  dec_cont->stream.stream_bus = 0;
  dec_cont->stream.p_start_of_stream = NULL;
  dec_cont->stream.p_start_of_buffer = NULL;
  dec_cont->stream.p_curr_pos = NULL;
  dec_cont->stream.bit_pos_in_byte = 0;
  dec_cont->stream.stream_length = 0;
  dec_cont->stream.strm_buff_size = 0;
  dec_cont->stream.read_bits = 0;

  /* asic buffer */
  dec_cont->asic_buff.out_luma_buffer.virtual_address = NULL;
  dec_cont->asic_buff.out_chroma_buffer.virtual_address = NULL;
  if(dec_cont->pp_enabled) {
    DWLmemset(dec_cont->asic_buff.pp_luma_buffer, 0, sizeof(dec_cont->asic_buff.pp_luma_buffer));
    DWLmemset(dec_cont->asic_buff.pp_chroma_buffer, 0, sizeof(dec_cont->asic_buff.pp_chroma_buffer));
    DWLmemset(dec_cont->asic_buff.pp_chroma_buffer2, 0, sizeof(dec_cont->asic_buff.pp_chroma_buffer2));
  }
  dec_cont->asic_buff.out_luma_buffer.bus_address = 0;
  dec_cont->asic_buff.out_chroma_buffer.bus_address = 0;
  dec_cont->asic_buff.out_luma_buffer.size = 0;
  dec_cont->asic_buff.out_chroma_buffer.size = 0;

  /* asic running flag */
  dec_cont->asic_running = 0;
}
char* PrintChunkType(u32 chunk_type) {
  switch(chunk_type) {
    case PNG_IDAT:
      return "PNG_IDAT";
      break;
    case PNG_IHDR:
      return "PNG_IHDR";
      break;
    case PNG_PLTE:
      return "PNG_PLTE";
    case PNG_IEND:
      return "PNG_IEND";
    default:
      return "OTHER CHUNK";
  }
}

u32 FindLastIDAT(PngDecInst dec_inst, u32 len) {
  PngDecContainer *dec_cont = (PngDecContainer *) dec_inst;
  StreamStorage *strm = &dec_cont->idat_stream;
  u32 ret = HANTRO_OK;
  u32 length = 0, chunk_type = 0, send_len = 0, find_last_idata = 0;


  send_len = len - (strm->p_curr_pos-strm->p_start_of_buffer);
  APITRACE("send_len  %d, cur pos %p, left %d\n", len, (void*)strm->p_curr_pos, send_len);
  while (send_len > 0) {
    length = 0;
    if (dec_cont->idata_header_left) {
      if (send_len < 5) {
        APITRACE("%s","send_len not enough, last idata header left break\n");
        break;
      }
      length = PngDecGet4Bytes(strm);
      send_len -= (strm->bit_pos_in_byte ? 5 : 4);
      dec_cont->idata_header_left = 0;
      APITRACE("%s","last idata header and new idata header consumed\n");
    } else if (dec_cont->chunk_type_left) {
      if (send_len < 5) {
        APITRACE("%s","send_len not enough, last chunk type left break\n");
        break;
      }
      APITRACE("%s","last chunk type left\n");
      ASSERT(dec_cont->idata_left > 4);
      length = dec_cont->idata_left - 4;
    } else if (dec_cont->idata_left) {
      if (send_len >= dec_cont->idata_left) {
        send_len -= dec_cont->idata_left;
        PngDecFlushBits(strm, dec_cont->idata_left * 8);
        if (dec_cont->last_chunk_type == PNG_IDAT) {
          dec_cont->last_idata_pos = strm->p_curr_pos;
          dec_cont->decode_stat |= PNG_HAVE_IDAT;
          APITRACE("Next time found the last idata pos %p\n", (void*)dec_cont->last_idata_pos);
        }
      } else {
        APITRACE("send_len not enough, last idata left %d break\n",  dec_cont->idata_left);
        break;
      }
      dec_cont->idata_left = 0;
      APITRACE("%s","last idata left consumed\n");
      continue;
    } else {
      if (send_len < 5) {
        dec_cont->idata_header_left = 1;
        APITRACE("%s","send_len not enough, idata header left break\n");
        break;
      }
      length = PngDecGet4Bytes(strm);
      send_len -= (strm->bit_pos_in_byte ? 5 : 4);
      dec_cont->idata_header_left = 0;
      APITRACE("new idata length %d\n", length);
    }
    dec_cont->idata_left = length + 4;
    if (send_len < 5) {
      dec_cont->chunk_type_left = 1;
      APITRACE("%s","send_len not enough, new chunk type left break\n");
      break;
    }
    chunk_type = PngDecGet4Bytes(strm);
    APITRACE("new chunk %s \n", PrintChunkType(chunk_type));
    dec_cont->last_chunk_type = chunk_type;
    send_len -= (strm->bit_pos_in_byte ? 5 : 4);
    dec_cont->chunk_type_left = 0;
    ret = PngCheckIsValidChunk(dec_cont, chunk_type, length);
    if (ret == HANTRO_NOK) {
      find_last_idata = 1;
      ASSERT(0);
      break;
    }
    if ((dec_cont->decode_stat & PNG_HAVE_IDAT) &&
        (chunk_type != PNG_IDAT)) { /* PNG_IEND or error case that not PNG_IDAT */
      find_last_idata = 1;
      strm->p_curr_pos -= 8;
      strm->read_bits -= (8 * 8);
      APITRACE("%s","chunk not idata but had ever idata\n");
      break;
    }
    if (send_len < (length+4)) {
      APITRACE("send_len not enough, new idata left %d break\n",  dec_cont->idata_left);
      break;
    }
    dec_cont->idata_left -= (length+4);
    ASSERT(dec_cont->idata_left==0);
    PngDecFlushBits(strm, (length+4) * 8);
    APITRACE("idata+crc here pos %p\n", (void*)strm->p_curr_pos);
    send_len -= (length+4);
    if (chunk_type == PNG_IDAT) {
      dec_cont->last_idata_pos = strm->p_curr_pos;
      dec_cont->decode_stat |= PNG_HAVE_IDAT;
      APITRACE("This time found the new idata pos %p\n", (void*)strm->p_curr_pos);
    }
  }
  APITRACE("find last end pos %p\n", (void*)strm->p_curr_pos);
  return find_last_idata;
}

/*------------------------------------------------------------------------------
         Function name   : PngDecUpdateStrmInfoCtrl
         Description     : called in low latency to set relative registers dynamically.
------------------------------------------------------------------------------*/
void PngDecUpdateStrmInfoCtrl(PngDecInst dec_inst,
                               struct strmInfo info)
{
  PngDecContainer *dec_cont = (PngDecContainer *) dec_inst;
  static u32 len_update = 1;
  struct LLStrmInfo *llstrminfo = &dec_cont->llstrminfo;
  struct strmInfo *stream_info = &llstrminfo->stream_info;
  u32 find_last_idata = 0;

  *stream_info = info;
  if (dec_cont->send_end) return;
  // if (!info.last_flag) return;
  stream_info->last_flag = 0;/* depend on last idata chunk */
#ifdef SUPPORT_MEM_SYNC
    llstrminfo->dwl_inst = dec_cont->dwl;
#endif
  if(llstrminfo->update_reg_flag)
  {
    APITRACE("src end addr %p bus %08x\n", (void*)info.strm_vir_addr, (u32)info.strm_bus_addr);
    if(llstrminfo->first_update)
    {
      while (!dec_cont->asic_running)
        sched_yield();
      llstrminfo->first_update = 0;
      llstrminfo->ll_strm_len = 0;
      len_update = 1;
    }
    find_last_idata = FindLastIDAT(dec_cont, info.send_len);
    if (find_last_idata) {
      stream_info->last_flag = 1;
      if (dec_cont->last_idata_pos == NULL) {
        stream_info->strm_vir_addr = info.strm_vir_addr;
        stream_info->strm_bus_addr = info.strm_bus_addr;
        APITRACEERR("%s","PngCheckIsValidChunk check failed !!!\n");
      } else {
        stream_info->strm_vir_addr = dec_cont->last_idata_pos;
        stream_info->strm_bus_addr = dec_cont->stream.stream_bus +
                                    (addr_t)(dec_cont->last_idata_pos - dec_cont->stream.p_start_of_buffer);
        dec_cont->send_end = 1;
        APITRACE("found the last idata pos %p\n", (void*)dec_cont->last_idata_pos);
      }
    }
    SwUpdateStrmInfoCtrl(llstrminfo, &len_update);

    /* update strm length register */
    if(PngCheckHwStatus(dec_cont) == 0) {
      llstrminfo->update_reg_flag = 0;
      dec_cont->last_idata_pos = NULL;
      dec_cont->idata_left = 0;
      dec_cont->idata_header_left = 0;
      dec_cont->chunk_type_left = 0;
      dec_cont->last_chunk_type = 0;
    }
  }
}

#ifdef CASE_INFO_STAT
static void PngCaseInfoCollect(PngDecContainer *dec_cont, CaseInfo *case_info) {
  u32 display_width, display_height;
  u32 pic_width, pic_height, bit_depth;

  display_width = pic_width = dec_cont->Hdrs.pic_width;
  display_height = pic_height = dec_cont->Hdrs.pic_height;
  bit_depth = dec_cont->Hdrs.bit_depth;

  if(!case_info->frame_num) {
    case_info->display_width = display_width;
    case_info->display_height = display_height;
    case_info->decode_width = pic_width;
    case_info->decode_height = pic_height;
    case_info->bit_depth = bit_depth;
  } else {
    if(case_info->decode_width != pic_width
    || case_info->decode_height != pic_height) {
      case_info->decode_width = MAX (pic_width , case_info->decode_width);
      case_info->decode_height = MAX (pic_height, case_info->decode_height);
      case_info->resolution_flag = 1;
    }
    case_info->display_height = MIN (display_height, case_info->display_height);
    case_info->display_width = MIN (display_width, case_info->display_width);
    case_info->bit_depth = MAX (bit_depth, case_info->bit_depth);
  }

  case_info->chroma_format_id = dec_cont->Hdrs.color_type;
  if(dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_INDEX)
    case_info->palette_mode = 1;

  case_info->codec = DEC_MODE_PNG;
  case_info->frame_num ++;
  case_info->bitrate = ((60 * GetDecRegister(dec_cont->png_regs, HWIF_STREAM_LEN) * 8 / case_info->decode_width)
                          * 3840/ case_info->decode_height) * 2160 / 1000 / 1000;
}
#endif