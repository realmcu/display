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

#ifndef PNGDECCONT_H
#define PNGDECCONT_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "pngdecapi.h"
#include "dwl.h"
#include "deccfg.h"
#include "vpufeature.h"
#include "decapicommon.h"
#include "ppu.h"
#include "input_queue.h"
#include "commonfunction.h"
#include "dec_log.h"
/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/
#if 0
#ifdef _ASSERT_USED
#include <assert.h>
#endif

/* macro for assertion, used only if compiler flag _ASSERT_USED is defined */
#ifdef _ASSERT_USED
#define ASSERT(expr) assert(expr)
#else
#define ASSERT(expr)
#endif
#endif

#define PNGDEC_X170_MIN_BUFFER 5120
#define PNGDEC_X170_MAX_BUFFER (1<<30) /* 16776960 */

#define PNGDEC_MAX_WIDTH_TN 8192
#define PNGDEC_MAX_HEIGHT_TN 8192

#define MAX_FRAME_NUMBER 2

#define PNG_MAX_LENGTH  0xFFFFFFFF
#define PNG_U32(b1,b2,b3,b4) \
   (b1 << 24) | (b2 << 16) |  (b3 << 8) | b4

#define PNG_MAX_PALETTE_LENGTH 256

typedef struct {
  u8 *p_start_of_stream;
  u8 *p_start_of_buffer;
  u8 *p_curr_pos;
  addr_t stream_bus;
  u32 bit_pos_in_byte;
  u32 stream_length;
  u32 read_bits;
  u32 strm_buff_size;        /* size of stream buffer (bytes) */
  struct strmInfo *stream_info;
} StreamStorage;

enum Lz77Mode {
  PNG_MODE = 0,
  LZ77_UNIVERSAL_MODE = 1,
  LZ77_PIC_MODE = 2
};

enum ChunkType {
/* critical chunks */
  PNG_IHDR = PNG_U32( 73,  72,  68,  82),
  PNG_PLTE = PNG_U32( 80,  76,  84,  69),
  PNG_IDAT = PNG_U32( 73,  68,  65,  84),
  PNG_IEND = PNG_U32( 73,  69,  78,  68),
/* ancillary chunks */
  PNG_cHRM = PNG_U32( 99,  72,  82,  77),
  PNG_gAMA = PNG_U32(103,  65,  77,  65),
  PNG_iCPP = PNG_U32(105,  67,  67,  80),
  PNG_sBIT = PNG_U32(115,  66,  73,  84),
  PNG_sRGB = PNG_U32(115,  82,  71,  66),
  PNG_bKGD = PNG_U32( 98,  75,  71,  68),
  PNG_hIST = PNG_U32(104,  73,  83,  84),
  PNG_tRNS = PNG_U32(116,  82,  78,  83),
  PNG_pHYs = PNG_U32(112,  72,  89, 115),
  PNG_tIME = PNG_U32(116,  73,  77,  69),
  PNG_iTXT = PNG_U32(105,  84,  88, 116),
  PNG_zTXT = PNG_U32(122,  84,  88, 116),
  PNG_prVt = PNG_U32(112,  114, 86, 116)
};

typedef struct {
 u32 pic_width;
 u32 pic_height;
 u32 bit_depth;
 enum ColorType color_type;
 u32 compression_method;
 u32 filter_method;
 u32 interlaced;
} PngDecHdrs;

typedef struct {
 enum ChunkType chunk_type;
 u32 length;
 u32 crc;
} ChunkInfo;

typedef struct {
  struct DWLLinearMem out_luma_buffer;
  struct DWLLinearMem out_chroma_buffer;
  struct DWLLinearMem pp_luma_buffer[DEC_MAX_OUT_COUNT];  /* luma/chroma both in this buffer */
  struct DWLLinearMem pp_chroma_buffer[DEC_MAX_OUT_COUNT]; /* not allocated, just form pp_luma_buffer */
  struct DWLLinearMem pp_chroma_buffer2[DEC_MAX_OUT_COUNT]; /* not allocated, just form pp_luma_buffer */
} PngAsicBuffers;

typedef struct OutElement_ {
  u32 mem_idx;
  PngDecOutput pic;
  PngDecImageInfo info;
} OutElement;

typedef struct FrameBufferList_ {
  u32  fb_stat[MAX_FRAME_NUMBER];
  OutElement out_fifo[MAX_FRAME_NUMBER];
  int wr_id;
  int rd_id;
  int num_out;
  u32 b_initialized;
  u32 end_of_stream;

  sem_t out_count_sem;
  pthread_mutex_t out_count_mutex;
  pthread_mutex_t ref_count_mutex;
  pthread_cond_t hw_rdy_cv;
  u32 abort;
} FrameBufferList;

enum DecodeStatus {
  PNG_DEC_INIT = 0,
  PNG_DEC_BUFFER_EMPTY= 0x1,
  PNG_HAVE_IHDR = 0x10,
  PNG_HAVE_IDAT = 0x100,
  PNG_HAVE_PALETTE = 0x1000,
  PNG_HAVE_tRNS = 0x2000,
  PNG_HAVE_OTHERS = 0x10000,
  PNG_HAVE_IEND = 0x100000,
};

typedef struct {
  u32 png_regs[TOTAL_X170_REGISTERS];
  u32 asic_running;
  StreamStorage stream;
  StreamStorage hw_stream;
  PngAsicBuffers asic_buff;
  const void *dwl;    /* DWL instance */
  i32 core_id;
  u32 pp_enabled;     /* set to 1 to enable pp */
  u32 pp_chroma_format;
  DecPicAlignment align;
  u32 dscale_shift_x;
  u32 dscale_shift_y;
  PpUnitIntConfig ppu_cfg[DEC_MAX_PPU_COUNT];
  DelogoConfig delogo_params[2];
  PngDecImageInfo image_info;
  u64 next_buf_size;  /* size of the requested external buffer */
  u32 buf_num;        /* number of buffers (with size of next_buf_size) requested to be allocated externally */
  u32 ext_buffer_num;
  u32 buffer_index;
  u32 tot_buffers;
  u64 n_ext_buf_size;        // size of external buffers added
  u32 buffer_queue_index;
  u32 realloc_buffer;
  u32 use_adaptive_buffers;
  u32 cur_out_ext_buf_index;
  InputQueue pp_buffer_queue;

  FrameBufferList fb_list;
  PngDecHdrs Hdrs;
  enum Lz77Mode lz77_mode;
  u32 lz77_tail_size; //for lz77_mode==1, lz77_tail_size = (origin_size % 1280) ? (origin_size % 1280) : 1280.
  u32 channels;
  u32 chunk_crc;
  enum DecodeStatus decode_stat;        /* which process have been finished */
  ChunkInfo chunk_info;
  u8 has_tRNS;
  u32 tRNS_sample;
#ifdef SUPPORT_TRNS
  u8 palette[PNG_MAX_PALETTE_LENGTH * 4];
#else
  u8 palette[PNG_MAX_PALETTE_LENGTH * 3];
#endif
  struct DWLLinearMem palette_buf;
  u32 allocated;
  u32 abort;

  addr_t png_hw_start_bus;
  u32 vcmd_used;  /* decoder connects to VCMD engine to start HW */
  u32 cmd_buf_id; /* command buffer id used for current picture */
  u32 mc_refresh_regs[READ_STATUS_REGS_COUNT];
  u32 n_cores_available;
  u32 n_cores;

  struct LLStrmInfo llstrminfo;

  u32 secure_mode;

  enum {
    DEC_IDLE = 0,       /* DEC is idle */
    DEC_RUNNING         /* DEC is running */
  } dec_status[MAX_ASIC_CORES]; /* used to track each core's status in MC mode */

#ifdef FPGA_PERF_AND_BW
  DecPerfInfo perf_info;
#endif
  u32 core_mask;
  const struct DecHwFeatures *hw_features[MAX_ASIC_CORES];
  u32 seq_core_mask;
  const struct DecHwFeatures *cur_hw_feature;

  struct M2MP_INFO vcmd_m2mp_data[MAX_VCMD_M2MP_NUM];
  CrcData *crc;
  u32 crc_enabled;

  CommonCfg common_cfg;
  u32 idata_left;
  u32 idata_header_left;
  u32 chunk_type_left;
  u32 last_chunk_type;
  u8* last_idata_pos;
  u32 send_end;
  StreamStorage idat_stream;

  u32 sbi_pic_start;
} PngDecContainer;

#endif /* #endif PNGDECDATA_H */
