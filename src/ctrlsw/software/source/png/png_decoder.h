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

#ifndef PNG_DECODER_H
#define PNG_DECODER_H

#include "basetype.h"
#include "pngdeccontainer.h"
#include "pngdecutils.h"
#include "sw_util.h"

enum PngResult {
  PNG_PIC_RDY,
  PNG_HDRS_RDY,
  PNG_PALETTE_PARSED,
  PNG_tRNS_PARSED,
  PNG_OTHERS_PARSED,
  PNG_IEND_PARSED,
  PNG_BUFFER_NOT_READY,

  /* error case */
  PNG_MEMFAIL = -1,
  PNG_HDRS_ERROR = -2,
  PNG_STREAM_ERROR = -3
};

u32 PngDecode(PngDecContainer *dec_cont);
u32 PngReadChunkHeader(PngDecContainer *dec_cont);
u32 PngDecodeIHDR(PngDecContainer *dec_cont);
#ifdef SUPPORT_LZ77
u32 PngDecodeLZ77(PngDecContainer *dec_cont);
#endif
u32 PngDecodeIDAT(PngDecContainer *dec_cont);
u32 PngDecodeIEND(PngDecContainer *dec_cont);
u32 PngDecodePLTE(PngDecContainer *dec_cont);
u32 PngDecodetRNS(PngDecContainer *dec_cont);
u32 PngChecktRNSExist(PngDecContainer *dec_cont);

#if 0
u32 PngDecodecHRM(PngDecContainer *dec_cont);
u32 PngDecodegAMA(PngDecContainer *dec_cont);
u32 PngDecodeiCPP(PngDecContainer *dec_cont);
u32 PngDecodesBIT(PngDecContainer *dec_cont);
u32 PngDecodesRGB(PngDecContainer *dec_cont);
u32 PngDecodebKGD(PngDecContainer *dec_cont);
u32 PngDecodehIST(PngDecContainer *dec_cont);
u32 PngDecodepHYs(PngDecContainer *dec_cont);
u32 PngDecodetIME(PngDecContainer *dec_cont);
u32 PngDecodetiTXT(PngDecContainer *dec_cont);
u32 PngDecodetzTXT(PngDecContainer *dec_cont);
#endif

u32 PngCheckIsValidChunk(PngDecContainer *dec_cont, u32 chunk_type, u32 len);

#endif /* #endif PNG_DECODER_H */