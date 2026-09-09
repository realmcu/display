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

#include "basetype.h"
#include "jpegdeccontainer.h"
#include "jpegdecapi.h"
#include "jpegdecmarkers.h"
#include "jpegdecutils.h"
#include "jpegdechdrs.h"
#include "jpegdecinternal.h"
#include "dwl.h"
#include "deccfg.h"
#include "sw_util.h"
#include "regdrv.h"
#include "ppu.h"
#include "delogo.h"
#include "dec_log.h"
#ifdef JPEGDEC_ASIC_TRACE
#include "jpegasicdbgtrace.h"
#endif /* #ifdef JPEGDEC_TRACE */

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define JPEGDEC_SLICE_START_VALUE 0
#define JPEGDEC_VLC_LEN_START_REG 16
#define JPEGDEC_VLC_LEN_END_REG 29

#ifdef PJPEG_COMPONENT_TRACE
extern u32 pjpeg_component_id;
extern u32 *pjpeg_coeff_base;
extern u32 pjpeg_coeff_size;

#define TRACE_COMPONENT_ID(id) pjpeg_component_id = id
#else
#define TRACE_COMPONENT_ID(id)
#endif

static void JpegMCSetHwRdyCallback(JpegDecInst dec_inst);
static void JpegRiMCSetHwRdyCallback(JpegDecInst dec_inst);
static void ClearHWOutput(FrameBufferList *fb_list, u32 id, u32 type);
static void MarkHWOutput(FrameBufferList *fb_list, u32 id, u32 type);

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static void JpegDecSetHwStrmParams(JpegDecContainer * dec_cont);

/*------------------------------------------------------------------------------
    5. Functions
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

        Function name: JpegDecClearStructs

        Functional description:
          handles the initialisation of jpeg decoder data structure

        Inputs:

        Outputs:
          Returns OK when successful, NOK in case unknown message type is
          asked

------------------------------------------------------------------------------*/
void JpegDecClearStructs(JpegDecContainer * dec_cont, u32 mode) {
  u32 i;

  ASSERT(dec_cont);
  JpegAsicBuffers *asic_buff = &dec_cont->asic_buff;

  /* stream pointers */
  dec_cont->stream.stream_bus = 0;
  dec_cont->stream.p_start_of_stream = NULL;
  dec_cont->stream.p_start_of_buffer = NULL;
  dec_cont->stream.p_curr_pos = NULL;
  dec_cont->stream.bit_pos_in_byte = 0;
  dec_cont->stream.stream_length = 0;
  dec_cont->stream.strm_buff_size = 0;
  dec_cont->stream.read_bits = 0;
  dec_cont->stream.appn_flag = 0;
  dec_cont->stream.return_sos_marker = 0;

  /* output image pointers and variables */
  dec_cont->image.p_start_of_image = NULL;
  dec_cont->image.p_lum = NULL;
  dec_cont->image.p_cr = NULL;
  dec_cont->image.p_cb = NULL;
  dec_cont->image.header_ready = 0;
  dec_cont->image.image_ready = 0;
  dec_cont->image.ready = 0;
  dec_cont->image.size = 0;
  dec_cont->image.size_luma = 0;
  dec_cont->image.size_chroma = 0;
  for(i = 0; i < MAX_NUMBER_OF_COMPONENTS; i++) {
    dec_cont->image.columns[i] = 0;
    dec_cont->image.pixels_per_row[i] = 0;
  }

  /* frame info */
  dec_cont->frame.Lf = 0;
  dec_cont->frame.P = 0;
  //dec_cont->frame.Y = 0;
  //dec_cont->frame.X = 0;

  if(!mode) {
    dec_cont->frame.Y = 0;
    dec_cont->frame.X = 0;
    dec_cont->frame.hw_y = 0;
    dec_cont->frame.hw_x = 0;
    dec_cont->frame.hw_ytn = 0;
    dec_cont->frame.hw_xtn = 0;
    dec_cont->frame.full_x = 0;
    dec_cont->frame.full_y = 0;
    dec_cont->stream.thumbnail = 0;
  } else {
    if(dec_cont->stream.thumbnail) {
      dec_cont->frame.hw_y = dec_cont->frame.full_y;
      dec_cont->frame.hw_x = dec_cont->frame.full_x;
      dec_cont->stream.thumbnail = 0;
    }
  }

  dec_cont->frame.Nf = 0; /* Number of components in frame */
  dec_cont->frame.num_mcu_in_frame = 0;
  dec_cont->frame.num_mcu_in_row = 0;
  dec_cont->frame.mcu_number = 0;
  dec_cont->frame.Ri = 0;
  dec_cont->frame.row = 0;
  dec_cont->frame.col = 0;
  dec_cont->frame.dri_period = 0;
  dec_cont->frame.block = 0;
  dec_cont->frame.c_index = 0;
  dec_cont->frame.buffer_bus = 0;

  if(!mode) {
    dec_cont->frame.p_buffer = NULL;
    dec_cont->frame.p_buffer_cb = NULL;
    dec_cont->frame.p_buffer_cr = NULL;
    for (i = 0; i < MAX_ASIC_CORES; i ++) {
      dec_cont->frame.p_table_base[i].virtual_address = NULL;
      dec_cont->frame.p_table_base[i].bus_address = 0;
    }

    /* asic buffer */
    DWLmemset(asic_buff, 0, sizeof(dec_cont->asic_buff));

    /* resolution */
    dec_cont->max_supported_slice_size = 0;

    /* out bus tmp */
    dec_cont->info.out_luma.virtual_address = NULL;
    dec_cont->info.out_chroma.virtual_address = NULL;
    if(dec_cont->pp_enabled && dec_cont->info.user_alloc_mem) {
      DWLmemset(dec_cont->info.out_pp_luma, 0, sizeof(dec_cont->info.out_pp_luma));
      DWLmemset(dec_cont->info.out_pp_chroma, 0, sizeof(dec_cont->info.out_pp_chroma));
      DWLmemset(dec_cont->info.out_pp_chroma2, 0, sizeof(dec_cont->info.out_pp_chroma2));
    }
    dec_cont->info.out_chroma2.virtual_address = NULL;
    dec_cont->info.out_luma.bus_address = 0;
    dec_cont->info.out_chroma.bus_address = 0;
    dec_cont->info.out_chroma2.bus_address = 0;

    /* user allocated addresses */
    dec_cont->info.given_out_luma.virtual_address = NULL;
    dec_cont->info.given_out_chroma.virtual_address = NULL;
    dec_cont->info.given_out_chroma2.virtual_address = NULL;
    dec_cont->info.given_out_luma.bus_address = 0;
    dec_cont->info.given_out_chroma.bus_address = 0;
    dec_cont->info.given_out_chroma2.bus_address = 0;
  }

  /* asic running flag */
  dec_cont->asic_running = 0;

  /* PJPEG flags */
  dec_cont->last_scan = 0;
  DWLmemset(dec_cont->pjpeg_coeff_bit_map, 0, sizeof(u16)*64*3);

  /* image handling info */
  dec_cont->info.slice_height = 0;
  dec_cont->info.amount_of_qtables = 0;
  dec_cont->info.y_cb_cr_mode = 0;
  dec_cont->info.column = 0;
  dec_cont->info.X = 0;
  dec_cont->info.Y = 0;
  dec_cont->info.mem_size = 0;
  dec_cont->info.SliceCount = 0;
  dec_cont->info.SliceMBCutValue = 0;
  if(!mode)
    dec_cont->info.user_alloc_mem = 0;
  dec_cont->info.slice_start_count = 0;
  dec_cont->info.amount_of_slices = 0;
  dec_cont->info.no_slice_irq_for_user = 0;
  dec_cont->info.SliceReadyForPause = 0;
  dec_cont->info.slice_limit_reached = 0;
  dec_cont->info.slice_mb_set_value = 0;
  dec_cont->info.timeout = (u32) DEC_X170_TIMEOUT_LENGTH;
  dec_cont->info.rlc_mode = 0; /* JPEG always in VLC mode == 0 */
  dec_cont->info.luma_pos = 0;
  dec_cont->info.chroma_pos = 0;
  dec_cont->info.fill_right = 0;
  dec_cont->info.fill_bottom = 0;
  dec_cont->info.stream_end = 0;
  dec_cont->info.stream_end_flag = 0;
  dec_cont->info.input_buffer_empty = 0;
  dec_cont->info.input_streaming = 0;
  dec_cont->info.input_buffer_len = 0;
  dec_cont->info.decoded_stream_len = 0;
  dec_cont->info.init = 0;
  dec_cont->info.init_thumb = 0;
  dec_cont->info.init_buffer_size = 0;

  /* progressive */
  dec_cont->info.non_interleaved = 0;
  dec_cont->info.component_id = 0;
  dec_cont->info.operation_type = 0;
  dec_cont->info.operation_type_thumb = 0;

  if(!mode) {
    dec_cont->info.p_coeff_base.virtual_address = NULL;
    dec_cont->info.p_coeff_base.bus_address = 0;
    dec_cont->info.y_cb_cr_mode_orig = 0;
    dec_cont->info.get_info_ycb_cr_mode = 0;
    dec_cont->info.get_info_ycb_cr_mode_tn = 0;
  }

  dec_cont->info.allocated = 0;

  for(i = 0; i < MAX_NUMBER_OF_COMPONENTS; i++) {
    dec_cont->info.components[i] = 0;
    dec_cont->info.pred[i] = 0;
    dec_cont->info.dc_res[i] = 0;
    dec_cont->frame.num_blocks[i] = 0;
    dec_cont->frame.blocks_per_row[i] = 0;
    dec_cont->frame.use_ac_offset[i] = 0;
    dec_cont->frame.component[i].C = 0;
    dec_cont->frame.component[i].H = 0;
    dec_cont->frame.component[i].V = 0;
    dec_cont->frame.component[i].Tq = 0;
  }

  /* scan info */
  dec_cont->scan.Ls = 0;
  dec_cont->scan.Ns = 0;
  dec_cont->scan.Ss = 0;
  dec_cont->scan.Se = 0;
  dec_cont->scan.Ah = 0;
  dec_cont->scan.Al = 0;
  dec_cont->scan.index = 0;
  dec_cont->scan.num_idct_rows = 0;

  for(i = 0; i < MAX_NUMBER_OF_COMPONENTS; i++) {
    dec_cont->scan.Cs[i] = 0;
    dec_cont->scan.Td[i] = 0;
    dec_cont->scan.Ta[i] = 0;
    dec_cont->scan.pred[i] = 0;
  }
#if 0
  /* huffman table lengths */
  dec_cont->vlc.default_tables = 0;
  dec_cont->vlc.ac_table0.table_length = 0;
  dec_cont->vlc.ac_table1.table_length = 0;
  dec_cont->vlc.ac_table2.table_length = 0;
  dec_cont->vlc.ac_table3.table_length = 0;

  dec_cont->vlc.dc_table0.table_length = 0;
  dec_cont->vlc.dc_table1.table_length = 0;
  dec_cont->vlc.dc_table2.table_length = 0;
  dec_cont->vlc.dc_table3.table_length = 0;

  if(dec_cont->vlc.ac_table0.vals) {
    DWLfree(dec_cont->vlc.ac_table0.vals);
  }
  if(dec_cont->vlc.ac_table1.vals) {
    DWLfree(dec_cont->vlc.ac_table1.vals);
  }
  if(dec_cont->vlc.ac_table2.vals) {
    DWLfree(dec_cont->vlc.ac_table2.vals);
  }
  if(dec_cont->vlc.ac_table3.vals) {
    DWLfree(dec_cont->vlc.ac_table3.vals);
  }
  if(dec_cont->vlc.dc_table0.vals) {
    DWLfree(dec_cont->vlc.dc_table0.vals);
  }
  if(dec_cont->vlc.dc_table1.vals) {
    DWLfree(dec_cont->vlc.dc_table1.vals);
  }
  if(dec_cont->vlc.dc_table2.vals) {
    DWLfree(dec_cont->vlc.dc_table2.vals);
  }
  if(dec_cont->vlc.dc_table3.vals) {
    DWLfree(dec_cont->vlc.dc_table3.vals);
  }

  /* pointer initialisation */
  dec_cont->vlc.ac_table0.vals = NULL;
  dec_cont->vlc.ac_table1.vals = NULL;
  dec_cont->vlc.ac_table2.vals = NULL;
  dec_cont->vlc.ac_table3.vals = NULL;

  dec_cont->vlc.dc_table0.vals = NULL;
  dec_cont->vlc.dc_table1.vals = NULL;
  dec_cont->vlc.dc_table2.vals = NULL;
  dec_cont->vlc.dc_table3.vals = NULL;
#endif
  /* Restart interval */
  dec_cont->frame.Ri = 0;
  if(dec_cont->frame.p_buffer) {
    DWLfree(dec_cont->frame.p_buffer);
  }

  dec_cont->frame.p_buffer = NULL;

  return;
}

/*------------------------------------------------------------------------------

        Function name: JpegDecInitHW

        Functional description:
          Set up HW regs for decode

        Inputs:
          JpegDecContainer *dec_cont

        Outputs:
          Returns OK when successful, NOK in case unknown message type is
          asked

------------------------------------------------------------------------------*/
enum DecRet JpegDecInitHW(JpegDecContainer * dec_cont) {
  u32 i;
  addr_t coeff_buffer = 0;
  u32 core_id = 0;
  ASSERT(dec_cont);
  struct DWLReqInfo info = {0};
  const struct DecHwFeatures *hw_feature = dec_cont->cur_hw_feature;

  TRACE_COMPONENT_ID(dec_cont->info.component_id);

  /* Check if first InitHw call */
  if(dec_cont->info.slice_start_count == 0) {
    /* Check if HW resource is available */
    info.core_mask = dec_cont->seq_core_mask;
    info.width = dec_cont->info.X;
    info.height = dec_cont->info.Y;
    info.owner = (void *)dec_cont;
    if (!dec_cont->vcmd_used) {
      if(DWLReserveHw(dec_cont->dwl,  &info, &dec_cont->core_id) == DWL_ERROR) {
        APITRACEERR("%s","JpegDecInitHW: ERROR hw resource unavailable\n");
        return DEC_HW_RESERVED;
      }
    } else {
      /* Always store registers in jpeg_regs[0] when VCMD is used.*/
#if 0
      core_id = dec_cont->core_id + 1;
      core_id = (core_id>=MAX_MC_CB_ENTRIES)?0:core_id;
      dec_cont->core_id = core_id;
#endif
      dec_cont->core_id = 0;
      if (DWLReserveCmdBuf(dec_cont->dwl, &info, &dec_cont->cmd_buf_id) == DWL_ERROR) {
        APITRACEERR("%s","JpegDecInitHW: ERROR command buffer unavailable\n");
        return DEC_HW_RESERVED;
      }
    }

    if (dec_cont->b_mc) {
      if (!dec_cont->vcmd_used)
        dec_cont->dec_status[dec_cont->core_id] = DEC_RUNNING;
    }
  }

  core_id = dec_cont->b_mc ? dec_cont->core_id : 0;
  /*************** Set swreg4 data ************/
  APITRACEDEBUG("%s","INTERNAL: Set Frame width\n");
  SetDecRegister(dec_cont->jpeg_regs, HWIF_PIC_WIDTH_IN_CBS,
                  dec_cont->info.X >> 3);

  APITRACEDEBUG("%s","INTERNAL: Set Frame height\n");
  SetDecRegister(dec_cont->jpeg_regs, HWIF_PIC_HEIGHT_IN_CBS,
                  dec_cont->info.Y >> 3);

  APITRACEDEBUG("%s","INTERNAL: Set decoding mode: JPEG\n");
  SetDecRegister(dec_cont->jpeg_regs, HWIF_DEC_MODE, DEC_MODE_JPEG);

  APITRACEDEBUG("%s","INTERNAL: Set output write disabled\n");
  SetDecRegister(dec_cont->jpeg_regs, HWIF_DEC_OUT_DIS, 1);

  APITRACEDEBUG("%s","INTERNAL: Set filtering disabled\n");
  SetDecRegister(dec_cont->jpeg_regs, HWIF_FILTERING_DIS, 1);

  APITRACEDEBUG("%s","INTERNAL: Set amount of QP Table\n");
  SetDecRegister(dec_cont->jpeg_regs, HWIF_JPEG_QTABLES,
                 dec_cont->info.amount_of_qtables);

  APITRACEDEBUG("%s","INTERNAL: Set input format\n");
  SetDecRegister(dec_cont->jpeg_regs, HWIF_JPEG_MODE,
                 dec_cont->info.y_cb_cr_mode_orig);

  APITRACEDEBUG("%s","INTERNAL: RLC mode enable, JPEG == disable\n");
  /* In case of JPEG: Always VLC mode used (0) */
  //SetDecRegister(dec_cont->jpeg_regs, HWIF_RLC_MODE_E, dec_cont->info.rlc_mode);

  /*************** Set swreg15 data ************/
  APITRACEDEBUG("%s","INTERNAL: Set slice/full mode: 0 full; other = slice\n");
  SetDecRegister(dec_cont->jpeg_regs, HWIF_JPEG_SLICE_H,
                 dec_cont->info.slice_height);

  /*************** Set swreg52 data ************/
  if(dec_cont->info.operation_type != JPEG_PROGRESSIVE) {
    /* Set JPEG operation mode */
    APITRACEDEBUG("%s","INTERNAL: Set JPEG operation mode\n");
    SetDecRegister(dec_cont->jpeg_regs, HWIF_PJPEG_E, 0);
  } else {
    /* Set JPEG operation mode */
    APITRACEDEBUG("%s","INTERNAL: Set JPEG operation mode\n");
    SetDecRegister(dec_cont->jpeg_regs, HWIF_PJPEG_E, 1);

    /* write coeff table base */
    APITRACEDEBUG("%s","INTERNAL: Set coefficient buffer base address\n");
    coeff_buffer = dec_cont->info.p_coeff_base.bus_address;
    SET_ADDR_REG(dec_cont->jpeg_regs, HWIF_PJPEG_COEFF_BUF,
                 coeff_buffer);
  }

  dec_cont->stream = dec_cont->hw_stream;
  /* Set JPEG operation mode */
  APITRACEDEBUG("%s","INTERNAL: Set JPEG operation mode\n");
  SetDecRegister(dec_cont->jpeg_regs, HWIF_JPEG_FRAME_MODE, dec_cont->info.operation_type);
  SetDecRegister(dec_cont->jpeg_regs, HWIF_JPEG_12BIT_E, (dec_cont->frame.P == 12 ? 1 : 0));
  SetDecRegister(dec_cont->jpeg_regs, HWIF_BIT_DEPTH_Y_MINUS8,
                  (dec_cont->frame.P == 8 ? 0 : 2));
  SetDecRegister(dec_cont->jpeg_regs, HWIF_BIT_DEPTH_C_MINUS8,
                  (dec_cont->frame.P == 8 ? 0 : 2));

  /* write Qtable selector */
  SetDecRegister(dec_cont->jpeg_regs, HWIF_PJPEG_QTABLE_SEL0,
                  dec_cont->frame.component[0].Tq);
  SetDecRegister(dec_cont->jpeg_regs, HWIF_PJPEG_QTABLE_SEL1,
                  dec_cont->frame.component[1].Tq);
  SetDecRegister(dec_cont->jpeg_regs, HWIF_PJPEG_QTABLE_SEL2,
                  dec_cont->frame.component[2].Tq);

  SetDecRegister(dec_cont->jpeg_regs, HWIF_UNIQUE_ID, UNIQUE_ID(0));

  /* set up stream position for HW decode */
  APITRACEDEBUG("%s","INTERNAL: Set stream position for HW\n");
  JpegDecSetHwStrmParams(dec_cont);

  {
    ASSERT(dec_cont->pp_enabled);
    if (dec_cont->pp_enabled /*&& ((asic_id & 0x0000F000) >> 12 >= 7)*/) {
      PpUnitIntConfig *ppu_cfg = &dec_cont->ppu_cfg[0];
      PpUnitIntConfig tmp_ppu_cfg[DEC_MAX_PPU_COUNT];
      DWLmemcpy(tmp_ppu_cfg, dec_cont->ppu_cfg, sizeof(dec_cont->ppu_cfg));
      if (RI_MC_ENABLED(dec_cont->ri_mc_enabled)) {
        u32 ri_height = dec_cont->frame.num_mcu_rows_in_interval *
                        dec_cont->frame.mcu_height;
        for (i = 0; i < DEC_MAX_PPU_COUNT; i++, ppu_cfg++) {
          u32 full_crop_h = ppu_cfg->crop.height;
          u32 full_scale_h = ppu_cfg->scale.height;
          if (!ppu_cfg->enabled ||
              /* Not in cropping window. */
              dec_cont->ri_index < dec_cont->start_ri_index[i] ||
              dec_cont->ri_index > dec_cont->end_ri_index[i]) {
              ppu_cfg->enabled = 0;
              continue;
          }
          if (dec_cont->ri_index == dec_cont->start_ri_index[i]) {
            /* First restart interval:
             * Update:
             *    crop.y
             *    crop.height
             *    scale.height
             */
            /* Becareful of overflow & underflow, especially for 16/32K jpeg. */
            ppu_cfg->crop.y = ppu_cfg->crop.y % ri_height;
            ppu_cfg->crop.height = ri_height - ppu_cfg->crop.y;
          } else {
            /* Middle/last restart intervals:
             * Update:
             *    crop.y -> 0
             *    crop.height -> restart interval height / real height in ri
             *    scale.height
             *    luma_offset/chroma_offset
             */
            if (dec_cont->ri_index == dec_cont->end_ri_index[i]) {
              ppu_cfg->crop.height = (ppu_cfg->crop.y + ppu_cfg->crop.height) % ri_height;
              if (!ppu_cfg->crop.height)
                ppu_cfg->crop.height = ri_height;
            } else {
              ppu_cfg->crop.height = ri_height;
            }
            ppu_cfg->luma_offset += ppu_cfg->luma_size / full_crop_h *
                                    (dec_cont->ri_index * ri_height -
                                     ppu_cfg->crop.y);
            ppu_cfg->chroma_offset += ppu_cfg->chroma_size / full_crop_h *
                                      (dec_cont->ri_index * ri_height -
                                       ppu_cfg->crop.y);
            ppu_cfg->crop.y = 0;
          }
          ppu_cfg->scale.height = ppu_cfg->crop.height *
                                    full_scale_h / full_crop_h;
          }
        }
        /* PPSetRegs need to add slice_mb_set_value support in future  */
        DelogoSetRegs(dec_cont->jpeg_regs, hw_feature, dec_cont->delogo_params);
        struct DWLDecBuffers *dec_buffers = InputQueueGetBufByID(dec_cont->pp_buffer_queue,
                                                            dec_cont->cur_out_ext_buf_index);

        // JpegAsicBuffers *asic_buff = &dec_cont->asic_buff;
        // struct DWLDecBuffers dec_buffers = JpegPrepareCurDecBuffers(dec_cont, asic_buff->pp_luma_buffer,
        //                                      asic_buff->pp_chroma_buffer, asic_buff->pp_chroma_buffer2);
#ifdef ENABLE_FPGA_VERIFICATION
        if (!dec_cont->info.user_alloc_mem && !dec_cont->ppu_cfg[0].sbi_en) {
          struct SwParames sw_parames = {dec_cont->dwl, dec_cont->ppu_cfg, dec_cont->common_cfg.use_separated_pp, 0};

          SwMemsetBuffer(&sw_parames, dec_buffers);
        }
#endif
        u32 pp_out_ctrl = InputQueueGetPPOutCtrl(dec_cont->pp_buffer_queue, &dec_buffers->dec_buffer[0][0]);
        struct PpParams pp_args = {hw_feature,
                                   dec_cont->ppu_cfg,
                                   (struct DWLLinearMem *)dec_buffers,
                                   0,
                                   0,
                                   pp_out_ctrl,
                                   dec_cont->common_cfg.use_separated_pp,
                                   0,
                                   dec_cont->sbi_pic_start,
                                   0
                                  };
        PPSetRegs(dec_cont->jpeg_regs, &pp_args);
        dec_cont->sbi_pic_start = 1;
        PPSetLancozsScaleRegs(dec_cont->jpeg_regs, hw_feature, dec_cont->ppu_cfg, core_id);
        if (RI_MC_ENABLED(dec_cont->ri_mc_enabled)) {
          /* Restore ppu config. */
          DWLmemcpy(dec_cont->ppu_cfg, tmp_ppu_cfg, sizeof(dec_cont->ppu_cfg));
          dec_cont->ri_running_cores++;
        }
      }
      /* Crop when hight is multiple of 8 instead of 16. */
      if (((dec_cont->frame.Y + 7) >> 3) & 1) {
        SetDecRegister(dec_cont->jpeg_regs, HWIF_MB_HEIGHT_OFF, 8);
      } else
        SetDecRegister(dec_cont->jpeg_regs, HWIF_MB_HEIGHT_OFF, 0);
  }
  /* In slice decoding mode, HW is only reserved once, but HW must be reserved for each
   * ri in ri-based multicore decoding. */
  if (!RI_MC_ENABLED(dec_cont->ri_mc_enabled))
    dec_cont->info.slice_start_count = 1;

#ifdef JPEGDEC_ASIC_TRACE
  {
    FILE *fd;

    fd = fopen("picture_ctrl_dec.trc", "at");
    DumpJPEGCtrlReg(dec_cont->jpeg_regs, fd);
    fclose(fd);

    fd = fopen("picture_ctrl_dec.hex", "at");
    HexDumpJPEGCtrlReg(dec_cont->jpeg_regs, fd);
    fclose(fd);

    fd = fopen("jpeg_tables.hex", "at");
    HexDumpJPEGTables(dec_cont->jpeg_regs, dec_cont, fd);
    fclose(fd);

    fd = fopen("registers.hex", "at");
    HexDumpRegs(dec_cont->jpeg_regs, fd);
    fclose(fd);
  }
#endif /* #ifdef JPEGDEC_ASIC_TRACE */

  /* Enable jpeg mode and set slice mode */
  APITRACEDEBUG("%s","INTERNAL: Enable jpeg\n");
  SetDecRegister(dec_cont->jpeg_regs, HWIF_DEC_E, 1);

  /* Flush regs to hw register */
  JpegFlushRegs(dec_cont);

  for (i =0; i < dec_cont->n_cores; i ++) {
    if(dec_cont->frame.p_table_base[i].virtual_address != NULL)
      DWLSyncMemData(dec_cont->dwl, &dec_cont->frame.p_table_base[i], 0,
                      dec_cont->frame.p_table_base[i].size, HOST_TO_DEVICE);
  }
  if (dec_cont->enable_3dlut)
    DWLSyncMemData(dec_cont->dwl, &dec_cont->ppu_cfg[0].table_3dlut_buffer, 0,
                    dec_cont->ppu_cfg[0].table_3dlut_buffer.size, HOST_TO_DEVICE);
  if (dec_cont->b_mc || RI_MC_ENABLED(dec_cont->ri_mc_enabled)) {
    if (RI_MC_DISABLED(dec_cont->ri_mc_enabled)) {
      JpegMCSetHwRdyCallback(dec_cont);
    } else {
      JpegRiMCSetHwRdyCallback(dec_cont);
    }
  }
  if (!dec_cont->vcmd_used)
    DWLEnableHw(dec_cont->dwl, dec_cont->core_id, 4 * 1, dec_cont->jpeg_regs[1]);
  else {
    if(dec_cont->crc_enabled) {
      u32 bd = dec_cont->frame.P == 8? 8 : 12;
      DWLmemset(dec_cont->vcmd_m2mp_data, 0, sizeof(dec_cont->vcmd_m2mp_data));
      CommonSetM2MPComands(dec_cont->ppu_cfg, dec_cont->vcmd_m2mp_data, dec_cont->asic_buff.pp_luma_buffer[0].bus_address,
                           dec_cont->pp_enabled, 0, hw_feature->crop_step_rshift,
                           dec_cont->frame.X , dec_cont->frame.Y,
                           bd, 0, dec_cont->align);
      DWLCmdM2MPSendData(dec_cont->dwl, NULL, dec_cont->vcmd_m2mp_data, dec_cont->cmd_buf_id);
    }
    DecLinkAndRunCmds(dec_cont->dwl, NULL, dec_cont->cmd_buf_id);
  }
  dec_cont->asic_running = 1;

  return DEC_OK;
}

/*------------------------------------------------------------------------------

        Function name: JpegDecInitHWContinue

        Functional description:
          Set up HW regs for decode

        Inputs:
          JpegDecContainer *dec_cont

        Outputs:
          Returns OK when successful, NOK in case unknown message type is
          asked

------------------------------------------------------------------------------*/
void JpegDecInitHWContinue(JpegDecContainer * dec_cont) {
  ASSERT(dec_cont);
  av_unused const struct DecHwFeatures *hw_feature = dec_cont->cur_hw_feature;
  JpegAsicBuffers *asic_buff = &dec_cont->asic_buff;

  /* update slice counter */
  dec_cont->info.amount_of_slices++;

  if (dec_cont->info.user_alloc_mem == 1 && dec_cont->info.slice_start_count > 0) {
    /* if user allocated memory ==> new addresses */
    dec_cont->asic_buff.out_luma_buffer.virtual_address =
      dec_cont->info.given_out_luma.virtual_address;
    dec_cont->asic_buff.out_luma_buffer.bus_address =
      dec_cont->info.given_out_luma.bus_address;
    dec_cont->asic_buff.out_chroma_buffer.virtual_address =
      dec_cont->info.given_out_chroma.virtual_address;
    dec_cont->asic_buff.out_chroma_buffer.bus_address =
      dec_cont->info.given_out_chroma.bus_address;
  }

  /* Update only register/values that might have been changed */

  /*************** Set swreg1 data ************/
  /* clear status bit */

  /*************** Set swreg5 data ************/
  APITRACEDEBUG("%s","INTERNAL CONTINUE: Set stream last buffer bit\n");
  SetDecRegister(dec_cont->jpeg_regs, HWIF_JPEG_STREAM_ALL,
                 dec_cont->info.stream_end);

  /*************** Set swreg13 data ************/
  /* PP depending register writes */
  /* Luminance output */
  APITRACEDEBUG("%s","INTERNAL CONTINUE: Set LUMA OUTPUT data base address\n");
  SET_ADDR_REG(dec_cont->jpeg_regs, HWIF_DEC_OUT_BASE,
                asic_buff->out_luma_buffer.bus_address);

  /*************** Set swreg14 data ************/

  /* Chrominance output */
  if(dec_cont->image.size_chroma) {
    /* write output base */
    APITRACEDEBUG("%s","INTERNAL CONTINUE: Set CHROMA OUTPUT data base address\n");
    SET_ADDR_REG(dec_cont->jpeg_regs, HWIF_JPG_CH_OUT_BASE,
                  asic_buff->out_chroma_buffer.bus_address);
  }

  /*************** Set swreg15 data ************/
  APITRACEDEBUG("%s","INTERNAL CONTINUE: Set slice/full mode: 0 full; other = slice\n");
  SetDecRegister(dec_cont->jpeg_regs, HWIF_JPEG_SLICE_H,
                 dec_cont->info.slice_height);

  /* Flush regs to hw register */
  DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 0x34,
              dec_cont->jpeg_regs[13]);
  DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 0x38,
              dec_cont->jpeg_regs[14]);
  DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 0x3C,
              dec_cont->jpeg_regs[15]);
  if(sizeof(addr_t) == 8) {
    DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 0x1EC,
                dec_cont->jpeg_regs[123]);
    DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 0x1F0,
                dec_cont->jpeg_regs[124]);
    DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 0x1F4,
                dec_cont->jpeg_regs[125]);
  }
  DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 0x14,
              dec_cont->jpeg_regs[5]);
  if (dec_cont->b_mc || RI_MC_ENABLED(dec_cont->ri_mc_enabled)) {
    if (RI_MC_DISABLED(dec_cont->ri_mc_enabled)) {
      JpegMCSetHwRdyCallback(dec_cont);
    } else {
      JpegRiMCSetHwRdyCallback(dec_cont);
    }
  }
  DWLEnableHw(dec_cont->dwl, dec_cont->core_id, 0x4,
              dec_cont->jpeg_regs[1]);

#ifdef JPEGDEC_ASIC_TRACE
  {
    APITRACEDEBUG("%s","INTERNAL CONTINUE: REGS BEFORE IRQ CLEAN\n");
    PrintJPEGReg(dec_cont->jpeg_regs);
  }
#endif /* #ifdef JPEGDEC_ASIC_TRACE */

}

/*------------------------------------------------------------------------------

        Function name: JpegDecInitHWInputBuffLoad

        Functional description:
          Set up HW regs for decode after input buffer load

        Inputs:
          JpegDecContainer *dec_cont

        Outputs:
          Returns OK when successful, NOK in case unknown message type is
          asked

------------------------------------------------------------------------------*/
void JpegDecInitHWInputBuffLoad(JpegDecContainer * dec_cont) {
  ASSERT(dec_cont);
  av_unused const struct DecHwFeatures *hw_feature = dec_cont->cur_hw_feature;

  /* Update only register/values that might have been changed */
  /*************** Set swreg4 data ************/
  APITRACEDEBUG("%s","INTERNAL: Set Frame width\n");
  SetDecRegister(dec_cont->jpeg_regs, HWIF_PIC_WIDTH_IN_CBS,
                  ((dec_cont->info.X >> 4) << 1));

    /* frame size, round up the number of mbs */
  APITRACEDEBUG("%s","INTERNAL: Set Frame height\n");
  SetDecRegister(dec_cont->jpeg_regs, HWIF_PIC_HEIGHT_IN_CBS,
                  (dec_cont->info.Y >> 4) << 1);

  /* Only used in PJPEG 411 to indicate pic_width is filled to 32 or not */
  if(dec_cont->info.operation_type == JPEG_PROGRESSIVE)
    SetDecRegister(dec_cont->jpeg_regs, HWIF_PJPEG_WIDTH_DIV16,
                   dec_cont->info.wdiv16);

  /*************** Set swreg5 data ************/
  APITRACEDEBUG("%s","INTERNAL BUFFER LOAD: Set stream start bit\n");
  SetDecRegister(dec_cont->jpeg_regs, HWIF_STRM_START_BIT,
                 dec_cont->stream.bit_pos_in_byte);

  /*************** Set swreg6 data ************/
  APITRACEDEBUG("%s","INTERNAL BUFFER LOAD: Set stream length\n");

  /* check if all stream will processed in this buffer */
  if((dec_cont->info.decoded_stream_len) >= dec_cont->stream.stream_length) {
    dec_cont->info.stream_end = 1;
  }

  SetDecRegister(dec_cont->jpeg_regs, HWIF_STREAM_LEN,
                 dec_cont->info.input_buffer_len);
  SetDecRegister(dec_cont->jpeg_regs, HWIF_STRM_BUFFER_LEN,
                 dec_cont->info.input_buffer_len);
  SetDecRegister(dec_cont->jpeg_regs, HWIF_STRM_START_OFFSET, 0);

  /*************** Set swreg4 data ************/
  APITRACEDEBUG("%s","INTERNAL BUFFER LOAD: Set stream last buffer bit\n");
  SetDecRegister(dec_cont->jpeg_regs, HWIF_JPEG_STREAM_ALL,
                 dec_cont->info.stream_end);

  /*************** Set swreg12 data ************/
  APITRACEDEBUG("%s","INTERNAL BUFFER LOAD: Set stream start address\n");
  SET_ADDR_REG(dec_cont->jpeg_regs, HWIF_RLC_VLC_BASE,
               dec_cont->stream.stream_bus);
  dec_cont->jpeg_hw_start_bus = dec_cont->stream.stream_bus;

  APITRACEDEBUG("INTERNAL BUFFER LOAD: Stream bus start 0x%08lx\n",
                          dec_cont->stream.stream_bus);
  APITRACEDEBUG("INTERNAL BUFFER LOAD: Bit position 0x%08x\n",
                          dec_cont->stream.bit_pos_in_byte);
  APITRACEDEBUG("INTERNAL BUFFER LOAD: Stream length 0x%08x\n",
                          dec_cont->stream.stream_length);

  /* Flush regs to hw register */
  DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 0x2A0,
              dec_cont->jpeg_regs[168]);
  DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 0x2A4,
              dec_cont->jpeg_regs[169]);
  DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 0x10,
              dec_cont->jpeg_regs[4]);
  DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 0x14,
              dec_cont->jpeg_regs[5]);
  DWLWriteReg(dec_cont->dwl, dec_cont->core_id, 0x18,
              dec_cont->jpeg_regs[6]);
  if (dec_cont->b_mc || RI_MC_ENABLED(dec_cont->ri_mc_enabled)) {
    if (RI_MC_DISABLED(dec_cont->ri_mc_enabled)) {
      JpegMCSetHwRdyCallback(dec_cont);
    } else {
      JpegRiMCSetHwRdyCallback(dec_cont);
    }
  }
  DWLEnableHw(dec_cont->dwl, dec_cont->core_id, 0x04,
              dec_cont->jpeg_regs[1]);

#ifdef JPEGDEC_ASIC_TRACE
  {
    APITRACEDEBUG("%s","INTERNAL BUFFER LOAD: REGS BEFORE IRQ CLEAN\n");
    PrintJPEGReg(dec_cont->jpeg_regs);
  }
#endif /* #ifdef JPEGDEC_ASIC_TRACE */

}

/*------------------------------------------------------------------------------

        Function name: JpegDecSetHwStrmParams

        Functional description:
          set up hw stream start position

        Inputs:
          JpegDecContainer *dec_cont

        Outputs:
          void

------------------------------------------------------------------------------*/
static void JpegDecSetHwStrmParams(JpegDecContainer * dec_cont) {
#define JPG_STR     dec_cont->stream
  addr_t addr_tmp = 0;
  u32 amount_of_stream = 0;
  addr_t mask;
  av_unused const struct DecHwFeatures *hw_feature = dec_cont->cur_hw_feature;

  /* calculate and set stream start address to hw */
  APITRACEDEBUG("INTERNAL: read bits %d\n", JPG_STR.read_bits);
  APITRACEDEBUG("INTERNAL: read bytes %d\n", JPG_STR.read_bits / 8);
  APITRACEDEBUG("INTERNAL: Stream bus start 0x%08lx\n",
                          JPG_STR.stream_bus);
  APITRACEDEBUG("INTERNAL: Stream virtual start %p\n",
                          JPG_STR.p_start_of_stream);


  mask = 15;


  /* calculate and set stream start address to hw */
  addr_tmp = (JPG_STR.stream_bus +
              (addr_t)(JPG_STR.p_curr_pos - JPG_STR.p_start_of_buffer)) & (~mask);

  SET_ADDR_REG(dec_cont->jpeg_regs, HWIF_RLC_VLC_BASE, addr_tmp);
  //SET_ADDR_REG(dec_cont->jpeg_regs, HWIF_RLC_VLC_BASE, JPG_STR.stream_bus);
  dec_cont->jpeg_hw_start_bus = addr_tmp;

  APITRACEDEBUG("INTERNAL: Stream bus start 0x%08lx\n",
                          JPG_STR.stream_bus);

  /* calculate and set stream start bit to hw */

  /* change current pos to bus address style */
  /* remove three lowest bits and add the difference to bitPosInWord */
  /* used as bit pos in word not as bit pos in byte actually... */
#if 0
  switch ((addr_t) JPG_STR.p_curr_pos & (7)) {
  case 0:
    break;
  case 1:
    JPG_STR.bit_pos_in_byte += 8;
    break;
  case 2:
    JPG_STR.bit_pos_in_byte += 16;
    break;
  case 3:
    JPG_STR.bit_pos_in_byte += 24;
    break;
  case 4:
    JPG_STR.bit_pos_in_byte += 32;
    break;
  case 5:
    JPG_STR.bit_pos_in_byte += 40;
    break;
  case 6:
    JPG_STR.bit_pos_in_byte += 48;
    break;
  case 7:
    JPG_STR.bit_pos_in_byte += 56;
    break;
  default:
    ASSERT(0);
    break;
  }
#endif

  JPG_STR.bit_pos_in_byte += ((size_t)JPG_STR.p_curr_pos & mask) * 8;
  SetDecRegister(dec_cont->jpeg_regs, HWIF_STRM_START_BIT,
                 JPG_STR.bit_pos_in_byte);

  /* set up stream length for HW.
   * length = size of original buffer - stream we already decoded in SW */
  JPG_STR.p_curr_pos = (u8 *) (size_t)((size_t)JPG_STR.p_curr_pos & (~mask));

  if(dec_cont->info.input_streaming) {
    amount_of_stream = (dec_cont->info.input_buffer_len -
                        (u32) (JPG_STR.p_curr_pos - JPG_STR.p_start_of_stream));

    SetDecRegister(dec_cont->jpeg_regs, HWIF_STRM_BUFFER_LEN, amount_of_stream);
    SetDecRegister(dec_cont->jpeg_regs, HWIF_STRM_START_OFFSET, 0);
    //SetDecRegister(dec_cont->jpeg_regs, HWIF_STRM_BUFFER_LEN, dec_cont->info.input_buffer_len);
    //SetDecRegister(dec_cont->jpeg_regs, HWIF_STRM_START_OFFSET, (addr_tmp - JPG_STR.stream_bus) & (~mask));
  } else {
    amount_of_stream = (JPG_STR.stream_length -
                        (u32) ((JPG_STR.read_bits - JPG_STR.bit_pos_in_byte) >> 3));

    SetDecRegister(dec_cont->jpeg_regs, HWIF_STRM_BUFFER_LEN, amount_of_stream);
    SetDecRegister(dec_cont->jpeg_regs, HWIF_STRM_START_OFFSET, 0);
    //SetDecRegister(dec_cont->jpeg_regs, HWIF_STRM_START_OFFSET, (addr_tmp - JPG_STR.stream_bus) & (~mask));
    //SetDecRegister(dec_cont->jpeg_regs, HWIF_STRM_BUFFER_LEN, JPG_STR.strm_buff_size);

    /* because no input streaming, frame should be ready during decoding this buffer */
    dec_cont->info.stream_end = 1;
  }

  /* Set the buffer bus address for using ddr_low_latency */
  if(dec_cont->llstrminfo.low_latency == 1) {
    SwUpdateLowLatencyInfo(dec_cont->dwl, hw_feature, addr_tmp, 0,
                           DWL_CLIENT_TYPE_JPEG_DEC, &dec_cont->llstrminfo, dec_cont->jpeg_regs);
  }
  else {
    SetDecRegister(dec_cont->jpeg_regs, HWIF_STREAM_LEN, amount_of_stream);
    DWLSyncMemDataExt(dec_cont->dwl, dec_cont->stream.stream_bus,
                      (void *)dec_cont->stream.p_start_of_buffer,
                      dec_cont->stream.strm_buff_size, HOST_TO_DEVICE);
  }
  JPG_STR.read_bits -= JPG_STR.bit_pos_in_byte;

  /*************** Set swreg4 data ************/
  APITRACEDEBUG("%s","INTERNAL: Set stream last buffer bit\n");
  SetDecRegister(dec_cont->jpeg_regs, HWIF_JPEG_STREAM_ALL,
                 dec_cont->info.stream_end);

  APITRACEDEBUG("INTERNAL: JPG_STR.stream_length %d\n",
                          JPG_STR.stream_length);
  APITRACEDEBUG("INTERNAL: JPG_STR.p_curr_pos %p\n",
                          JPG_STR.p_curr_pos);
  APITRACEDEBUG("INTERNAL: JPG_STR.p_start_of_stream %p\n",
                          JPG_STR.p_start_of_stream);
  APITRACEDEBUG("INTERNAL: JPG_STR.bit_pos_in_byte 0x%08x\n",
                          JPG_STR.bit_pos_in_byte);

  return;

#undef JPG_STR

}

/*------------------------------------------------------------------------------

        Function name: JpegDecAllocateResidual

        Functional description:
          Allocates residual buffer

        Inputs:
          JpegDecContainer *dec_cont  Pointer to DecData structure

        Outputs:
          OK
          DEC_MEMFAIL

------------------------------------------------------------------------------*/
enum DecRet JpegDecAllocateResidual(JpegDecContainer * dec_cont) {

  i32 tmp = DEC_ERROR;
  u32 block_coeff_size = 0;
  u64 total_coeff_size = 0;
  u32 i;
  JpegAsicBuffers *asic_buff = &dec_cont->asic_buff;
  DecInfo *info = &dec_cont->info;
  ASSERT(dec_cont);

  if(info->operation_type == JPEG_PROGRESSIVE) {

    if (dec_cont->frame.P == 8)
      block_coeff_size = JPEGDEC_COEFF_SIZE;
    else
      block_coeff_size = JPEGDEC_COEFF_SIZE_12BIT;

    for(i = 0; i < dec_cont->frame.Nf; i++) {
      total_coeff_size += NEXT_MULTIPLE_1(dec_cont->frame.num_blocks[i] * block_coeff_size, 384);
    }
    /* allocate coefficient buffer */
    info->p_coeff_base.mem_type = DWL_MEM_TYPE_SYNC_DEVICE_ONLY | DWL_MEM_TYPE_VPU_ONLY;

    SET_MEM_USAGE(info->p_coeff_base.mem_type, DWL_MEM_USAGE_TMP_COEEFT,
                    dec_cont->secure_mode);
    if (info->p_coeff_base.logical_size < total_coeff_size) {

      if (info->p_coeff_base.logical_size)
        DWLFreeLinear(dec_cont->dwl, &(info->p_coeff_base));
      tmp = DWLMallocLinear(dec_cont->dwl, (sizeof(u8) * total_coeff_size), &(dec_cont->info.p_coeff_base));
      if(tmp == -1) {
        return (DEC_MEMFAIL);
      }
    }
#ifdef PJPEG_COMPONENT_TRACE
    pjpeg_coeff_base = info->p_coeff_base.virtual_address;
    pjpeg_coeff_size = total_coeff_size;
#endif
    dec_cont->pjpeg_coeff_size_for_cache = total_coeff_size;
    APITRACEDEBUG("ALLOCATE: COEFF virtual %p bus 0x%08llx\n",
                            (void*)info->p_coeff_base.virtual_address,
                            info->p_coeff_base.bus_address);
  }

  if(DWL_DEVMEM_COMPARE(info->given_out_luma, 0)) {
    /* allocate luminance output */

    /* luma bus address to output */
    if (info->user_alloc_mem)
      info->out_luma = asic_buff[0].out_luma_buffer;
    if (dec_cont->pp_enabled) {
      DWLmemcpy(info->out_pp_luma, asic_buff->pp_luma_buffer, sizeof(info->out_pp_luma));
      DWLmemcpy(info->out_pp_chroma, asic_buff->pp_chroma_buffer, sizeof(info->out_pp_chroma));
      DWLmemcpy(info->out_pp_chroma2, asic_buff->pp_chroma_buffer2, sizeof(info->out_pp_chroma2));
    }
  }
  else { //dec_cont->info.user_alloc_mem==1
    asic_buff->out_luma_buffer.virtual_address =
      info->given_out_luma.virtual_address;
    asic_buff->out_luma_buffer.bus_address =
      info->given_out_luma.bus_address;

    info->out_pp_luma[0] = asic_buff->pp_luma_buffer[0] = info->given_out_luma;
    info->out_pp_chroma[0] = asic_buff->pp_chroma_buffer[0] = info->given_out_luma; //info->given_out_chroma;
    /* luma bus address to output */
    info->out_luma = asic_buff->out_luma_buffer;

    /* Chroma output buffer address */
    asic_buff->out_chroma_buffer.virtual_address =
      info->given_out_chroma.virtual_address;
    asic_buff->out_chroma_buffer.bus_address =
      info->given_out_chroma.bus_address;

    /* Chroma bus address to output */
    info->out_chroma = asic_buff->out_luma_buffer; //out_chroma_buffer;

    /* when PP is enabled, because user just allocate one set of output buffer,
            output buffer just use as PP output buffer */
    if (dec_cont->pp_enabled) {
      asic_buff->pp_luma_buffer[0] = info->out_luma;
      asic_buff->pp_chroma_buffer[0] = info->out_chroma;

      info->out_pp_luma[0] = asic_buff->pp_luma_buffer[0];
      info->out_pp_chroma[0] = asic_buff->pp_chroma_buffer[0];

      /* Just to set chroma offset. */
      CalcPpUnitBufferSize(dec_cont->cur_hw_feature, dec_cont->ppu_cfg, 0, dec_cont->common_cfg.use_separated_pp);
    }

    /* flag to release */
    info->user_alloc_mem = 1;
  }

  APITRACEDEBUG("ALLOCATE: Luma virtual %p bus 0x%08llx\n",
                          (void*)asic_buff->out_luma_buffer.virtual_address,
                          asic_buff->out_luma_buffer.bus_address);

  /* allocate chrominance output */
  if(dec_cont->image.size_chroma) {
    asic_buff->out_chroma_buffer.virtual_address =
      info->given_out_chroma.virtual_address;
    asic_buff->out_chroma_buffer.bus_address =
      info->given_out_chroma.bus_address;
    asic_buff->out_chroma_buffer2.virtual_address =
      info->given_out_chroma2.virtual_address;
    asic_buff->out_chroma_buffer2.bus_address =
      info->given_out_chroma2.bus_address;

    /* chroma bus address to output */
    info->out_chroma = asic_buff->out_chroma_buffer;
    info->out_chroma2 = asic_buff->out_chroma_buffer2;

    APITRACEDEBUG("ALLOCATE: Chroma virtual %p bus 0x%08llx\n",
                            (void*)asic_buff->out_chroma_buffer.virtual_address,
                            asic_buff->out_chroma_buffer.bus_address);
  }

  return DEC_OK;
}

/*------------------------------------------------------------------------------

        Function name: JpegDecSliceSizeCalculation

        Functional description:
          Calculates slice size

        Inputs:
          JpegDecContainer *dec_cont

        Outputs:
          void

------------------------------------------------------------------------------*/
void JpegDecSliceSizeCalculation(JpegDecContainer * dec_cont) {

  if(((dec_cont->info.SliceCount +
       1) * (dec_cont->info.slice_mb_set_value * 16)) > dec_cont->info.Y) {
    dec_cont->info.slice_height = ((dec_cont->info.Y / 16) -
                                   (dec_cont->info.SliceCount *
                                    dec_cont->info.slice_height));
  } else {
    /* TODO! other sampling formats also than YUV420 */
    if(dec_cont->info.operation_type == JPEG_PROGRESSIVE &&
        dec_cont->info.component_id != 0)
      dec_cont->info.slice_height = dec_cont->info.slice_mb_set_value / 2;
    else
      dec_cont->info.slice_height = dec_cont->info.slice_mb_set_value;
  }
}

/*------------------------------------------------------------------------------
    Function name   : JpegRefreshRegs
    Description     :
    Return type     : void
    Argument        : PPContainer * ppC
------------------------------------------------------------------------------*/
void JpegRefreshRegs(JpegDecContainer * dec_cont) {
  u32 *dec_regs = dec_cont->jpeg_regs;

  if(dec_cont->vcmd_used) {
    DWLRefreshRegister(dec_cont->dwl, dec_cont->cmd_buf_id, dec_regs);
    if(dec_cont->b_mc)
      DWLVcmdMCRefreshStatusRegs(dec_cont->dwl, dec_regs, dec_cont->cmd_buf_id);
  }
  else {
    DWLReadRegs(dec_cont->dwl, dec_cont->core_id, 4*1, &dec_regs[1], dec_cont->cur_hw_feature->max_regs);
  }
}

/*------------------------------------------------------------------------------
    Function name   : JpegFlushRegs
    Description     :
    Return type     : void
    Argument        : PPContainer * ppC
------------------------------------------------------------------------------*/
void JpegFlushRegs(JpegDecContainer * dec_cont) {

#ifdef JPEGDEC_ASIC_TRACE
  {
    APITRACEDEBUG("%s","INTERNAL: REGS BEFORE HW ENABLE\n");
    PrintJPEGReg(dec_cont->jpeg_regs);
  }
#endif /* #ifdef JPEGDEC_ASIC_TRACE */

  if(dec_cont->vcmd_used)
    DWLFlushRegister(dec_cont->dwl, dec_cont->cmd_buf_id, dec_cont->jpeg_regs,
                     dec_cont->mc_refresh_regs, dec_cont->core_id);
  else
    FlushDecRegisters(dec_cont->dwl, dec_cont->core_id, dec_cont->jpeg_regs,
                      dec_cont->cur_hw_feature->max_regs);
}

u32 JpegCheckHwStatus(JpegDecContainer *dec_cont)
{
   u32 tmp = 0;
   if (dec_cont->vcmd_used) {
     tmp = dec_cont->jpeg_regs[1];
   } else {
     tmp = DWLReadRegFromHw(dec_cont->dwl, dec_cont->core_id, 4 * 1);
   }

  if(tmp & 0x01)
    return 1;
  else
    return 0;
}

static void ClearHWOutput(FrameBufferList *fb_list, u32 id, u32 type) {
  u32 *bs = fb_list->fb_stat + id;

  pthread_mutex_lock(&fb_list->ref_count_mutex);

  ASSERT(*bs & (FB_HW_ONGOING));

  *bs &= ~type;

  if ((*bs & FB_HW_ONGOING) == 0) {
    /* signal that this buffer is done by HW */
    pthread_cond_signal(&fb_list->hw_rdy_cv);
  }

  pthread_mutex_unlock(&fb_list->ref_count_mutex);
}

static void MarkHWOutput(FrameBufferList *fb_list, u32 id, u32 type) {

  pthread_mutex_lock(&fb_list->ref_count_mutex);

  ASSERT( fb_list->fb_stat[id] ^ type );

  fb_list->fb_stat[id] |= type;

  pthread_mutex_unlock(&fb_list->ref_count_mutex);
}

static void JpegMCHwRdyCallback(void *args, i32 core_id) {
  JpegDecContainer *dec_cont = (JpegDecContainer *)args;
  JpegHwRdyCallbackArg *info;

  const void *dwl;

  u32 type = FB_HW_ONGOING;

  ASSERT(dec_cont != NULL);
  ASSERT(core_id < MAX_ASIC_CORES || (dec_cont->vcmd_used && core_id < MAX_MC_CB_ENTRIES));

  /* take a copy of the args as after we release the HW they
   * can be overwritten.
   */
  dwl = dec_cont->dwl;
  info = dec_cont->hw_rdy_callback_arg[core_id];

  /* React to the HW return value */

  if(dec_cont->stream_consumed_callback.fn)
    dec_cont->stream_consumed_callback.fn((u8*)info->stream,
                                          (void*)info->p_user_data);
  ClearHWOutput(&dec_cont->fb_list, info->out_id, type);
  /* clear IRQ status reg and release HW core */
  if (!dec_cont->vcmd_used) {
    DWLDisableHw(dwl, core_id, 0x04, 0);
    dec_cont->dec_status[info->core_id] = DEC_IDLE;
    DWLReleaseHw(dwl, info->core_id);
  } else {
    DWLReleaseCmdBuf(dwl, info->core_id);
  }
}

static void JpegMCSetHwRdyCallback(JpegDecInst dec_inst) {
  JpegDecContainer *dec_cont = (JpegDecContainer *) dec_inst;
  u32 type = FB_HW_ONGOING;
  JpegHwRdyCallbackArg *arg = dec_cont->hw_rdy_callback_arg[dec_cont->core_id];
  i32 core_id = dec_cont->core_id;

  if (arg == NULL) {
    dec_cont->hw_rdy_callback_arg[core_id] = DWLmalloc(sizeof(JpegHwRdyCallbackArg));
    arg = dec_cont->hw_rdy_callback_arg[core_id];
  }
  if (dec_cont->vcmd_used) {
    arg = dec_cont->hw_rdy_callback_arg[dec_cont->cmd_buf_id];
    core_id = dec_cont->cmd_buf_id;
  }
  arg->core_id = core_id;
  arg->stream = dec_cont->stream_consumed_callback.p_strm_buff;
  arg->p_user_data = dec_cont->stream_consumed_callback.p_user_data;
  arg->out_id = dec_cont->fb_list.wr_id;

  DWLSetIRQCallback(dec_cont->dwl, core_id, JpegMCHwRdyCallback,
                    dec_cont);

  MarkHWOutput(&dec_cont->fb_list, arg->out_id, type);
}

static void JpegRiMCHwRdyCallback(void *args, i32 core_id) {
  JpegDecContainer *dec_cont = (JpegDecContainer *)args;
  JpegHwRdyCallbackArg *info;
  const void *dwl;

  ASSERT(dec_cont != NULL);
  ASSERT(core_id < MAX_ASIC_CORES);

  /* take a copy of the args as after we release the HW they
   * can be overwritten.
   */
  dwl = dec_cont->dwl;
  info = dec_cont->hw_rdy_callback_arg[core_id];
  dec_cont->ri_running_cores--;

  /* React to the HW return value */
#if 0
  u32 perf_cycles;
  perf_cycles = DWLReadReg(dwl, core_id, 63 * 4);
  printf("Core[%d] cycles %d/MB\n", core_id,
          perf_cycles /
          (dec_cont->info.X * dec_cont->frame.mcu_height *
           dec_cont->frame.num_mcu_rows_in_interval / 256));
#endif

  /* clear IRQ status reg and release HW core */
  DWLDisableHw(dwl, core_id, 0x04, 0);
  DWLReleaseHw(dwl, info->core_id);

  if (dec_cont->ri_index > dec_cont->last_ri_index &&
      dec_cont->ri_running_cores == 0) {
    sem_post(&dec_cont->sem_ri_mc_done);
  }
  dec_cont->dec_status[info->core_id] = DEC_IDLE;
}

static void JpegRiMCSetHwRdyCallback(JpegDecInst dec_inst) {
  JpegDecContainer *dec_cont = (JpegDecContainer *) dec_inst;
  JpegHwRdyCallbackArg *arg = dec_cont->hw_rdy_callback_arg[dec_cont->core_id];

  arg->core_id = dec_cont->core_id;

  DWLSetIRQCallback(dec_cont->dwl, dec_cont->core_id, JpegRiMCHwRdyCallback,
                    dec_cont);
}

struct DWLDecBuffers JpegPrepareCurDecBuffers(const JpegDecContainer *dec_cont, const struct DWLLinearMem *luma,
                                              const struct DWLLinearMem *chroma, const struct DWLLinearMem *chroma2) {
  struct DWLDecBuffers dec_buffers;
  u32 i = 0;

  DWLmemset(&dec_buffers, 0, sizeof(dec_buffers));
  if (dec_cont->common_cfg.use_separated_pp == 0)
    dec_buffers.dec_buffer[0][0] = luma[0];
  else {
    for (i = 0; i < DEC_MAX_PPU_COUNT; i++) {
      if (!dec_cont->ppu_cfg[i].enabled) continue;
      dec_buffers.dec_buffer[i][0] = luma[i];
      if (MAX_OUTPUT_PLANE >= 3)  {
        dec_buffers.dec_buffer[i][1] = chroma[i];
        dec_buffers.dec_buffer[i][2] = chroma2[i];
      }
    }
  }

  return dec_buffers;
}