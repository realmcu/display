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

/* This file declares common structures/functions etc used in decoder internals,
   other than APIs. */

#ifndef COMMON_FUNCTION_H
#define COMMON_FUNCTION_H

#include "basetype.h"
#include "sw_util.h"
#include "dwl.h"
#include "fifo.h"
#include "ppu.h"
#include "decapicommon.h"

#define MEM_STAT_DPB 0x1
#define MEM_STAT_OUT 0x2
#define INVALID_MEM_IDX 0xFF

#define INVALID_VALUE 0xFFFFFFFF

struct DecCmdInfo {
  u32 batch_size;
  u32 cmd_num;
  FifoInst fifo_cmd;
  u32 client_type;
  /* to handle batch mode with slice decoding,
   * actully slice decoding should be disabled if eanble batch mode
   */
  u32 dis_batch_mode;
  u32 *b_mc;
  u32 *max_batch_size;
  // u32 is_relink;
};

/* obsoleted structure/enum removed from decapicommon.h, which are not used as
   part of API anymore. */

/**
 * DPB flags to control reference picture format etc.
 * \ingroup common_group
 */
enum DecDpbFlags {
  /** \brief Reference frame format is raster scan.[Not supported] */
  DEC_REF_FRM_RASTER_SCAN = 0x0,
  /** \brief Reference frame format is tiled (Default) */
  DEC_REF_FRM_TILED_DEFAULT = 0x1,
  /** \brief Flag to allow SW to use DPB field ordering on interlaced content.[Not supported] */
  DEC_DPB_ALLOW_FIELD_ORDERING = 0x40000000
};

/**
 * Error information.
 * \ingroup common_group
 */
enum DecErrorInfo {
  DEC_NO_ERROR = 0x0,
  /** HW EC only */
  DEC_NALU_HEADER_ERROR = 0x1,
  DEC_SLICE_HEADER_ERROR = 0x2,
  DEC_SLICE_DATA_ERROR = 0x4,
  DEC_SYNC_WORD_ERROR = 0x8,
  DEC_TRAILING_BITS_ERROR = 0x10,
  DEC_POLLING_STREAM_LEN_ERROR = 0x20,
  /** All use */
  DEC_FRAME_ERROR = 0x100,
  DEC_REF_ERROR = 0x200,
  /* for seek */
  DEC_SEEK_SW_DISCARD = 0x1000,
  /* for openB */
  DEC_OPENB_SW_DISCARD = 0x2000
};

enum SeqErrorState {
  SEQ_CLEAN = 0x0,
  SEQ_DIRTY = 1,
};

/* RFC Table Prefetch */
struct RTP {
  /* input */
  u32 pic_width_in_cbs;
  u32 min_cb_size;
  u32 max_cb_size;
  u32 dec_mode;
  u32 max_tile_width;
  u32 max_tile_height;
  u32 num_tile_cols;
  u32 num_tile_rows;
  u32 num_ref_frames;
  addr_t ref_pic[16];
  /* output */
  u8 rtp_override_e;
  u8 rtp_block_num;
  u32 y_line_len;
  u32 c_line_len;
  u32 y_line_num;
  u32 c_line_num;
  u32 y_line_num_prefetch;
  u32 c_line_num_prefetch;
};

typedef struct CoreInfo_ {
  u32 core_id;
  const struct DWLLinearMem *out_buffer;
  CrcData *crc;
} RdyCoreInfo;

typedef struct OutputPicInfo_ {
  u32 id;
  u32 error_ratio;
  u32 cycles;
  u32 is_field_pic;
  u32 is_bottom_field;
  CrcData *crc;
  enum DecPicCodingType pic_code_type;
  enum DecErrorInfo error_info;
  u64 pic_id;
  u32 core_id;
  const void *out_data;
  const void *out_pp_data;
  u32 pic_out_flag;
  u32 use_separated_pp;
} OutputPicInfo;

struct DecInfo {
  u64 pic_size;
  u32 bit_depth;
  u32 chroma_format;
  enum DecCodec codec;
};

/* function: generate fake table */
#ifdef USE_FAKE_RFC_TABLE
void GenerateFakeRFCTable(u8 *cmp_tble_addr,
                          u32 pic_width_in_cbsy,
                          u32 pic_height_in_cbsy,
                          u32 pic_width_in_cbsc,
                          u32 pic_height_in_cbsc,
                          u32 bit_depth,
                          u32 buffer_align);
#endif

enum DpbResetStatus{
  TO_RELEASE_OLD_DPB = 0,
  TO_ADD_NEW_DPB,
  ADDING_NEW_DPB,
  DPB_RESET_DONE
};

enum DecStat {
  VCDEC_UNINITIALIZED,
  VCDEC_INITIALIZED,
  VCDEC_NEW_HEADERS,
  VCDEC_WAIT_FOR_BUFFER,
  VCDEC_DECODING,
  VCDEC_BUFFER_EMPTY,
  VCDEC_EOS,
  VCDEC_ABORTED
};

struct DecOutInfo {
  u32 cycles;
  u32 error_ratio;
  enum DecErrorInfo error_info;
  u32 luma_sum;
};

#ifdef FPGA_PERF_AND_BW
typedef struct DWLPerfInfo DecPerfInfo;

void DecPerfInfoCount(const void *instance, u32 core_id, DecPerfInfo* perf_info, struct DecInfo dec_info);
void AveragePerfInfoPrint(DecPerfInfo *perf_info);
#endif

#ifdef OPEN_HWCFG_TO_CLIENT
void GetHwConfig(const void *hw_feature, struct DecHwConfig *hw_cfg);
#endif

#define VCMD_HW_ID_1_0_C                0x43421001
#define VCMD_HW_ID_1_1_1                0x43421101
#define VCMD_HW_ID_1_1_2                0x43421102
#define VCMD_HW_ID_1_4_1                0x43421401
#define VCMD_HW_ID_1_5_10               0x4342150A
#define VCMD_HW_ID_1_6_0                0x43421600

void CommonSetM2MPComands(PpUnitIntConfig *ppu_cfg, struct M2MP_INFO vcmd_m2mp_data[MAX_VCMD_M2MP_NUM], addr_t base_bus_addr,
                          u32 pp_enabled, u32 use_rfc, u32 crop_step_rshift, u32 width, u32 height,
                          u32 bd, u32 mono_chrome, u32 align);

void SetRfcTablePrefetch(u32 *regs, u32 max_tile_width, u32 max_tile_height, u32 dpb_size);

enum DecRet DecLinkAndRunCmds(const void *instance, struct DecCmdInfo *info_m, u32 cmdbuf_id);

void SwGetSubsampling(u32 chroma_format_idc, u32 *subsampling_x, u32 *subsampling_y);
enum DecRet SwAllocFilmGrainMem(const void *dwl_inst, u32 subsampling_x, u32 subsampling_y, u32 secure_mode,
                                struct DWLLinearMem *film_grain_mem);

#endif /* COMMON_FUNCTION_H */
