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

#ifndef __PNGDECAPI_H__
#define __PNGDECAPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------

    Table of contents

    1. Include headers
    2. External compiler flags
    3. Module defines
    4. Local function prototypes
    5. Functions

------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "decapicommon.h"
#include "dectypes.h"
#include "dwl.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/
typedef void *PngDecInst;

/* Image information */
typedef struct {
  u32 display_width;
  u32 display_height;
  u32 output_width;    /* Number of pixels/line in the image  */
  u32 output_height;   /* Number of lines in in the image     */
  u32 bit_depth; /* luma & chroma */
  u32 interlaced;
  u32 channels;
  u32 color_type;
  u32 coding_mode;
  u32 dec_image_type;
  DecPicAlignment align; /* alignment information, it is maybe default value defined inside ctrlSW,
                                               application uses it for output buffer allocation */
} PngDecImageInfo;

/* Decoder input PNG information */
typedef struct {
  struct DWLLinearMem stream_buffer;  /* input stream buffer */
  u32 stream_length;   /* input stream length */
  u32 buffer_size; /* input stream buffer size in input stream buffering mode */
  struct DWLLinearMem picture_buffer_y;    /* luma output address ==> if user allocated */
  struct DWLLinearMem picture_buffer_cb_cr; /* chroma output address ==> if user allocated */
  struct DWLLinearMem picture_buffer_cr; /* chroma output address ==> if user allocated */
  void *p_user_data;
  u8 *stream;  /* input stream bus address */
  u32 dec_ctrl;
} PngDecInput;

/* Decoder output */
typedef struct {
  u32 cycles_per_mb;   /**< Avarage cycle count per macroblock */
  u32 bit_depth;
  struct DecOutput output;
  CrcData *crc;
  u32 color_type;
  u32 pp_index; /** < only vaild pp enabled, app not midify it, need keep it and return by PngDecPictureConsumed */
  struct PngOutputInfo {
    struct DWLLinearMem output_picture_y;    /* Pointer to the Luma output image */
    struct DWLLinearMem output_picture_cb_cr; /* Pointer to the Chroma output image */
    struct DWLLinearMem output_picture_cr; /* Pointer to the Chroma output image */
    u32 output_width;           /* bytes in a pixel line (with padded bytes) */
    u32 output_height;
    u32 display_width;          /* valid pixels */
    u32 display_height;
    u32 pic_stride;
    u32 pic_stride_ch;
    enum DecPictureFormat output_format;
    struct DWLLinearMem dec400_luma_table;           /**< Buffer properties *//*sunny add for tile status address*/
    struct DWLLinearMem dec400_chroma_table;
  } pictures[DEC_MAX_OUT_COUNT];
} PngDecOutput;

struct PngDecConfig {
  DecPicAlignment align;
  PpUnitConfig ppu_config[DEC_MAX_OUT_COUNT];
  DelogoConfig delogo_params[2];
  enum DecDecoderMode decoder_mode;
  u32 strm_status_in_buffer; /**< With low-latency decoding mode,
                             stream status is updated in buffer instead of registers. */
  enum DecPictureFormat chroma_format;
  u32 use_adaptive_buffers;
  u32 crc_enabled;
  u32 misc_ctrl;
};
/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

/* Initialization */
enum DecRet PngDecInit(PngDecInst *dec_inst, const void *dwl, struct PngDecConfig *dec_cfg);

/* Release */
void PngDecRelease(PngDecInst dec_inst);

/* Get image information of the PNG */
enum DecRet PngDecGetImageInfo(PngDecInst dec_inst, PngDecInput *p_dec_in, PngDecImageInfo *info);

/* Decode PNG */
enum DecRet PngDecDecode(PngDecInst dec_inst,
                         PngDecInput * p_dec_in, PngDecOutput * p_dec_out);

/* Update HW in low latency mode */
void PngDecUpdateStrmInfoCtrl(PngDecInst dec_inst,
                                struct strmInfo info);


enum DecRet PngDecNextPicture(PngDecInst dec_inst,
                              PngDecOutput * output,
                              PngDecImageInfo *info);

enum DecRet PngDecPictureConsumed(PngDecInst dec_inst, PngDecOutput * output);

enum DecRet PngDecEndOfStream(PngDecInst dec_inst);

enum DecRet PngDecSetInfo(PngDecInst dec_inst, struct PngDecConfig *dec_cfg);

enum DecRet PngDecGetBufferInfo(PngDecInst dec_inst, struct DecBufferInfo *mem_info);

enum DecRet PngDecAddBuffer(PngDecInst dec_inst, struct DWLLinearMem *info);
enum DecRet PngDecAbort(PngDecInst dec_inst);

enum DecRet PngDecAbortAfter(PngDecInst dec_inst);

void PngDecUpdateStrmInfoCtrl(PngDecInst dec_inst,
                               struct strmInfo info);
#ifdef __cplusplus
}
#endif

#endif
