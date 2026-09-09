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
#include "png_decoder.h"
#include "pngdecutils.h"

u32 PngDecode(PngDecContainer *dec_cont) {
  u32 ret = 0;
  u32 crc = 0;
  av_unused StreamStorage tmp_pos;

  tmp_pos = dec_cont->stream;
  ret = PngReadChunkHeader(dec_cont);
  if(ret != HANTRO_OK) {
    STREAMTRACE_E("%s","Parse Header Error\n");
    ret = PNG_STREAM_ERROR;
    return ret;
  }
  switch(dec_cont->chunk_info.chunk_type) {
    case PNG_IHDR:
      ret = PngDecodeIHDR(dec_cont);
      if(ret != HANTRO_OK) {
        return PNG_HDRS_ERROR;
      }
      dec_cont->lz77_mode = PNG_MODE;
      dec_cont->decode_stat |= PNG_HAVE_IHDR;
      dec_cont->hw_stream = dec_cont->stream;
#ifdef SUPPORT_TRNS
      PngChecktRNSExist(dec_cont);
#endif
      return PNG_HDRS_RDY;
    case PNG_IDAT:
      if((dec_cont->decode_stat & PNG_HAVE_IHDR) == 0) {
        STREAMTRACE_E("%s","Missing IHDR before IDAT\n");
        ret = PNG_STREAM_ERROR;
      } else if((dec_cont->decode_stat & PNG_HAVE_OTHERS) && (dec_cont->decode_stat & PNG_HAVE_IDAT)) {
        STREAMTRACE_E("%s","Don't allow there are other chunks between IDAT chunks\n");
        ret = PNG_STREAM_ERROR;
      } else {
        (void)PngDecodeIDAT(dec_cont);
        if (!dec_cont->llstrminfo.low_latency) {
          dec_cont->decode_stat |= PNG_HAVE_IDAT;
        } else {
          dec_cont->idat_stream = tmp_pos;
        }
        return PNG_PIC_RDY;
      }
    case PNG_PLTE:
      ret = PngDecodePLTE(dec_cont);
      dec_cont->hw_stream = dec_cont->stream;
      if(ret != HANTRO_OK)
        return PNG_STREAM_ERROR;
      else
        return PNG_PALETTE_PARSED;
    case PNG_tRNS:
#ifdef SUPPORT_TRNS
      ret = PngDecodetRNS(dec_cont);
      dec_cont->hw_stream = dec_cont->stream;
      if(ret != HANTRO_OK)
        return PNG_STREAM_ERROR;
      else
        return PNG_tRNS_PARSED;
#else
      STREAMTRACE_I("%s","Unknown chunk type, skipped\n");
      dec_cont->stream.stream_info = &dec_cont->llstrminfo.stream_info;
      dec_cont->chunk_info.crc = PngCalcCrc32(&dec_cont->stream, dec_cont->chunk_info.length, dec_cont->chunk_info.crc);
      ret = PngDecFlushBits(&dec_cont->stream, dec_cont->chunk_info.length * 8);;
      crc = PngDecGet4Bytes(&dec_cont->stream);
      if(crc != dec_cont->chunk_info.crc) {
        STREAMTRACE_E("%s", "CRC Not Correct\n");
        return HANTRO_NOK;
      }
      if(ret != HANTRO_OK)
        return PNG_STREAM_ERROR;
      dec_cont->hw_stream = dec_cont->stream;
      dec_cont->decode_stat |= PNG_HAVE_OTHERS;
      return PNG_OTHERS_PARSED;
#endif
    case PNG_IEND:
      ret = PngDecodeIEND(dec_cont);
      dec_cont->decode_stat |= PNG_HAVE_IEND;
      if(ret != HANTRO_OK)
        return PNG_STREAM_ERROR;
      else
        return PNG_IEND_PARSED;
#ifdef SUPPORT_LZ77
    case PNG_prVt:
      ret = PngDecodeLZ77(dec_cont);
      if(ret == HANTRO_OK) {
        dec_cont->decode_stat |= PNG_HAVE_IHDR;
        dec_cont->hw_stream = dec_cont->stream;
        return PNG_HDRS_RDY;
        //return PNG_HDRS_ERROR;
      }
#endif
/* skip ancillary chunks */
    case PNG_sBIT:
    case PNG_sRGB:
    case PNG_tIME:
    case PNG_iTXT:
    case PNG_pHYs:
    case PNG_bKGD:
    case PNG_cHRM:
    case PNG_gAMA:
    case PNG_hIST:
    case PNG_iCPP:
    default:
      STREAMTRACE_I("%s","Unknown chunk type, skipped\n");
      StreamStorage *strm = &dec_cont->stream;
      strm->stream_info = &dec_cont->llstrminfo.stream_info;
      dec_cont->chunk_info.crc = PngCalcCrc32(strm, dec_cont->chunk_info.length, dec_cont->chunk_info.crc);
      ret = PngDecFlushBits(strm, dec_cont->chunk_info.length * 8);;
      crc = PngDecGet4Bytes(strm);
      if(crc != dec_cont->chunk_info.crc) {
        STREAMTRACE_E("%s", "CRC Not Correct\n");
        return HANTRO_NOK;
      }
      if(ret != HANTRO_OK)
        return PNG_STREAM_ERROR;
      dec_cont->hw_stream = dec_cont->stream;
      dec_cont->decode_stat |= PNG_HAVE_OTHERS;
      return PNG_OTHERS_PARSED;
  }
}

u32 PngReadChunkHeader(PngDecContainer *dec_cont) {
  u32 length = 0;
  u32 chunk_type = 0;
  u32 ret = 0;
  StreamStorage *strm = &dec_cont->stream;
  strm->stream_info = &dec_cont->llstrminfo.stream_info;

 /* Read the length and the chunk name. */
  length = PngDecGet4Bytes(strm);

/* Reset crc, and calc crc from the chunk name*/
  dec_cont->chunk_info.crc = PngCalcCrc32(strm, 4, 0);
  chunk_type = PngDecGet4Bytes(strm);

  STREAMTRACE_I("Reading %lx chunk, length = %lu\n",
                dec_cont->chunk_info.chunk_type, (unsigned long)length);

   /* Check to see if chunk name is valid. */
   ret = PngCheckIsValidChunk(dec_cont, chunk_type, length);
   if (ret == HANTRO_NOK) {
     return -1;
   }

   return HANTRO_OK;
}
u32 PngDecodeIHDR(PngDecContainer *dec_cont) {
/* TODO crc check */
  u32 crc = 0;
  StreamStorage *strm = &dec_cont->stream;
  strm->stream_info = &dec_cont->llstrminfo.stream_info;
  StreamStorage stream = dec_cont->stream;

  dec_cont->Hdrs.pic_width = PngDecGet4Bytes(strm);
  dec_cont->Hdrs.pic_height = PngDecGet4Bytes(strm);
  dec_cont->Hdrs.bit_depth = PngDecGetByte(strm);
  dec_cont->Hdrs.color_type = PngDecGetByte(strm);
  dec_cont->Hdrs.compression_method = PngDecGetByte(strm);
  dec_cont->Hdrs.filter_method = PngDecGetByte(strm);
  dec_cont->Hdrs.interlaced = PngDecGetByte(strm);
  if(dec_cont->Hdrs.interlaced) {
    STREAMTRACE_E("%s", "Interlaced Not Supported\n");
    return HANTRO_NOK;
  }
  crc = PngDecGet4Bytes(strm);
  dec_cont->chunk_info.crc = PngCalcCrc32(&stream, dec_cont->chunk_info.length, dec_cont->chunk_info.crc);
  if(crc != dec_cont->chunk_info.crc) {
    STREAMTRACE_E("%s", "CRC Not Correct\n");
    return HANTRO_NOK;
  }
  if(dec_cont->Hdrs.bit_depth > 8) {
    STREAMTRACE_E("%s","Bit Depth is Not Supported!\n");
    return HANTRO_NOK;
  }
  if(dec_cont->Hdrs.bit_depth < 8 && dec_cont->Hdrs.color_type != PNG_COLOR_TYPE_INDEX) {
    STREAMTRACE_E("%s","Stream Not Supported!\n");
    return HANTRO_NOK;
  }
  return HANTRO_OK;
}
#ifdef SUPPORT_LZ77
u32 PngDecodeLZ77(PngDecContainer *dec_cont) {
/* TODO crc check */
  StreamStorage *strm = &dec_cont->stream;
  strm->stream_info = &dec_cont->llstrminfo.stream_info;
  StreamStorage stream = dec_cont->stream;
  u32 lz77_flag = PngDecShow4Bytes(strm, 0);
  if(lz77_flag != 0x4C5A3737){
    STREAMTRACE_E("%s", "Illegal private chunk.(Not LZ77)\n");
    return HANTRO_NOK;
  }
  u32 crc = 0;
  dec_cont->Hdrs.pic_width = 1280;
  dec_cont->Hdrs.pic_height = 1280;
  dec_cont->Hdrs.bit_depth = 8;
  dec_cont->Hdrs.color_type = 0;
  dec_cont->Hdrs.compression_method = 0;
  dec_cont->Hdrs.filter_method = 0;
  dec_cont->Hdrs.interlaced = 0;

  lz77_flag = PngDecGet4Bytes(strm);
  if(dec_cont->Hdrs.interlaced) {
    STREAMTRACE_E("%s", "Interlaced Not Supported\n");
    return HANTRO_NOK;
  }
  u32 pic_mode = PngDecGetByte(&dec_cont->stream);// bit[7:4] color_type 0b'0000; bit[3:1] reserved 0b'000; bit[0] 0b'0
  u32 original_data_size_msb = PngDecGet2Bytes(&dec_cont->stream);
  u32 original_data_size_lsb = PngDecGet2Bytes(&dec_cont->stream);

  if(pic_mode == 0){
    dec_cont->lz77_mode = LZ77_UNIVERSAL_MODE;
    u32 original_data_size = original_data_size_lsb + (original_data_size_msb << 16);
    dec_cont->Hdrs.pic_height = (original_data_size + 1280 - 1)/ 1280;
    dec_cont->lz77_tail_size = (original_data_size % 1280) ? (original_data_size % 1280) : 1280;
  } else {
    u8 lz77_color_type = pic_mode >> 4;//0b'00:RGB888; 0b'01:ARGB8888; 0b'10: RGB565; 0b'11: ARGB8565
    dec_cont->Hdrs.color_type = lz77_color_type == 0 ? PNG_COLOR_TYPE_RGB :
                                (lz77_color_type == 1 ? PNG_COLOR_TYPE_RGB_ALPHA:
                                 lz77_color_type == 2 ? LZ77_COLOR_TYPE_RGB565 : LZ77_COLOR_TYPE_RGB565_ALPHA);
    dec_cont->Hdrs.pic_width = original_data_size_msb;
    dec_cont->Hdrs.pic_height = original_data_size_lsb;
    dec_cont->lz77_mode = LZ77_PIC_MODE;
  }

  crc = PngDecGet4Bytes(&dec_cont->stream);
  dec_cont->chunk_info.crc = PngCalcCrc32(&stream, dec_cont->chunk_info.length, dec_cont->chunk_info.crc);
  if(crc != dec_cont->chunk_info.crc) {
    STREAMTRACE_E("%s", "CRC Not Correct\n");
    //return HANTRO_NOK;
  }
  if(dec_cont->Hdrs.bit_depth > 8) {
    STREAMTRACE_E("%s","Bit Depth is Not Supported!\n");
    return HANTRO_NOK;
  }
  if(dec_cont->Hdrs.bit_depth < 8 && dec_cont->Hdrs.color_type != PNG_COLOR_TYPE_INDEX) {
    STREAMTRACE_E("%s","Stream Not Supported!\n");
    return HANTRO_NOK;
  }
  return HANTRO_OK;
}
#endif

u32 PngReadNextChunk(PngDecContainer *dec_cont) {
  u32 ret = 0;
  // PngReadNextChunkHeader(dec_cont);
  //dec_cont->idat_size += dec_cont->chunk_info.length + 12;  /* length + type + data + crc */
  //dec_cont->chunk_info.crc = PngCalcCrc32(&dec_cont->stream, dec_cont->chunk_info.length, dec_cont->chunk_info.crc);
  ret = PngDecFlushBits(&dec_cont->stream, dec_cont->chunk_info.length * 8 + 32);

  /* check crc32 of idat chunk in cmodle */
  //crc = PngDecGet4Bytes(&dec_cont->stream);
  // if(crc != dec_cont->chunk_info.crc) {
  //   STREAMTRACE_E("%s", "CRC Not Correct\n");
  //   return HANTRO_NOK;
  // }
  dec_cont->last_idata_pos = dec_cont->stream.p_curr_pos;
  if(ret == HANTRO_OK)
    ret = PngReadChunkHeader(dec_cont);
  return ret;
}

u32 PngDecodeIDAT(PngDecContainer *dec_cont) {
  u32 ret = 0;

  if (dec_cont->llstrminfo.low_latency) {
    return ret; /* parse in send data thread */
  } else {
    while(dec_cont->chunk_info.chunk_type == PNG_IDAT) {
      ret = PngReadNextChunk(dec_cont);
      if(ret != HANTRO_OK) break;
    }
  }
  dec_cont->stream.p_curr_pos = dec_cont->last_idata_pos;
  dec_cont->last_idata_pos = NULL;
  if(ret != HANTRO_OK){
    STREAMTRACE_E("%s", "Warning: There is no data left after IDAT Chunk!\n");
    return ret;
  }
  dec_cont->stream.read_bits -= 8 * 8;
  return ret;
}

u32 PngDecodeIEND(PngDecContainer *dec_cont) {
  u32 ret = HANTRO_OK;
  u32 crc;
  StreamStorage *strm = &dec_cont->stream;
  strm->stream_info = &dec_cont->llstrminfo.stream_info;
  dec_cont->chunk_info.crc = PngCalcCrc32(strm, dec_cont->chunk_info.length, dec_cont->chunk_info.crc);
  ret = PngDecFlushBits(strm, dec_cont->chunk_info.length * 8);
  crc = PngDecGet4Bytes(strm);
  if(crc != dec_cont->chunk_info.crc) {
    STREAMTRACE_E("%s", "CRC Not Correct\n");
    return HANTRO_NOK;
  }
  return ret;
}

u32 PngDecodePLTE(PngDecContainer *dec_cont) {
  u32 crc = 0, num = 0;
  u32 max_pal_len, skip_len = 0;
  u32 ret = HANTRO_OK;
  u32 i, j = 0;
  StreamStorage *strm = &dec_cont->stream;
  strm->stream_info = &dec_cont->llstrminfo.stream_info;

  if ((dec_cont->decode_stat & PNG_HAVE_IHDR) == 0) {
    STREAMTRACE_E("%s", "Missing IHDR");
    return HANTRO_NOK;
  } else if ((dec_cont->decode_stat & PNG_HAVE_PALETTE) != 0) {
    STREAMTRACE_E("%s", "Palette Duplicate");
    return HANTRO_NOK;
  } else if ((dec_cont->decode_stat & PNG_HAVE_IDAT) != 0) {
    STREAMTRACE_E("%s", "Palette chunk should be occurred before IDAT chunk");
    return HANTRO_NOK;
  }

  dec_cont->decode_stat |= PNG_HAVE_PALETTE;

  if (dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_GRAY ||
    dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    STREAMTRACE_I("%s", "Ignored in grayscale PNG");
    ret = HANTRO_NOK;
  }

  /* The cast is safe because 'length' is less than 3*PNG_MAX_PALETTE_LENGTH,
     and should be divisible by 3 */
  if (dec_cont->chunk_info.length > 3 * PNG_MAX_PALETTE_LENGTH
      || dec_cont->chunk_info.length % 3) {
    STREAMTRACE_E("%s", "The length of palette chunk is invalid!!!");
    return HANTRO_NOK;
  }
  //printf("wxdebug:PNG Indexed-color:bit_depth:%d\n",dec_cont->Hdrs.bit_depth);
  /* If the palette has 256 or fewer entries but is too large for the bit
     depth for indexed colour, we don't issue an error.
     We silently truncate the unused extra palette entries.
  */
  num = dec_cont->chunk_info.length / 3;
  if (dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_INDEX)
    max_pal_len = (1 << dec_cont->Hdrs.bit_depth);
  else
    max_pal_len = PNG_MAX_PALETTE_LENGTH;

  if (num > max_pal_len)
    num = max_pal_len;

  dec_cont->chunk_info.crc = PngCalcCrc32(strm, dec_cont->chunk_info.length, dec_cont->chunk_info.crc);

  for (i = 0; i < num * 3; i++) {
    dec_cont->palette[j++] = PngDecGetByte(strm);
#ifdef SUPPORT_TRNS
    if (j % 4 == 3) j++;
#endif
  }

  if((num * 3 < dec_cont->chunk_info.length) &&
      dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_INDEX) {
    skip_len = dec_cont->chunk_info.length - num * 3;
    /* update the stream pos */
    ret = PngDecFlushBits(strm, skip_len * 8);
  }

  crc = PngDecGet4Bytes(strm);
  if(crc != dec_cont->chunk_info.crc) {
    STREAMTRACE_E("%s", "CRC Not Correct\n");
    return HANTRO_NOK;
  }

  return ret;
}

//Check tRNS chunk is exist between IHDR Chunk and IDAT Chunk.
u32 PngChecktRNSExist(PngDecContainer *dec_cont){
  u32 ret = HANTRO_OK;

  //save stream info.
  ChunkInfo chunk_info_tmp;
  chunk_info_tmp = dec_cont->chunk_info;
  av_unused StreamStorage strm_tmp;
  strm_tmp = dec_cont->stream;

  ret = PngReadChunkHeader(dec_cont);
  if(ret != HANTRO_OK) {
    //recover stream info.
    dec_cont->chunk_info = chunk_info_tmp;
    dec_cont->stream = strm_tmp;
    return ret;
  }
  while(dec_cont->chunk_info.chunk_type != PNG_IDAT) {
    if(dec_cont->chunk_info.chunk_type == PNG_tRNS){
      dec_cont->has_tRNS = 1;
      break;
    }
    ret = PngReadNextChunk(dec_cont);
    if(ret != HANTRO_OK) break;
  }
  //recover stream info.
  dec_cont->chunk_info = chunk_info_tmp;
  dec_cont->stream = strm_tmp;
  return ret;
}


u32 PngDecodetRNS(PngDecContainer *dec_cont) {
  u32 crc = 0;
  u32 ret = HANTRO_OK;
  StreamStorage *strm = &dec_cont->stream;
  strm->stream_info = &dec_cont->llstrminfo.stream_info;

  if ((dec_cont->decode_stat & PNG_HAVE_IHDR) == 0) {
    STREAMTRACE_E("%s", "Missing IHDR");
    return HANTRO_NOK;
  } else if ((dec_cont->decode_stat & PNG_HAVE_tRNS) != 0) {
    STREAMTRACE_E("%s", "tRNS Chunk Duplicate");
    return HANTRO_NOK;
  } else if ((dec_cont->decode_stat & PNG_HAVE_IDAT) != 0) {
    STREAMTRACE_E("%s", "tRNS chunk should be occurred before IDAT chunk");
    return HANTRO_NOK;
  }

  dec_cont->decode_stat |= PNG_HAVE_tRNS;
  dec_cont->has_tRNS = 1;

  dec_cont->chunk_info.crc = PngCalcCrc32(strm, dec_cont->chunk_info.length, dec_cont->chunk_info.crc);


  if (dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_GRAY){
    if (dec_cont->chunk_info.length != 2){
      STREAMTRACE_E("%s", "tRNS Chunk Size invalid");
      return HANTRO_NOK;
    }
    dec_cont->tRNS_sample = PngDecGet2Bytes(strm);
  } else if (dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_RGB) {
    if (dec_cont->chunk_info.length != 6){
      STREAMTRACE_E("%s", "tRNS Chunk Size invalid");
      return HANTRO_NOK;
    }
    u16 Red = PngDecGet2Bytes(strm) & 0xFF;
    u16 Green = PngDecGet2Bytes(strm) & 0xFF;
    u16 Blue = PngDecGet2Bytes(strm) & 0xFF;
    dec_cont->tRNS_sample = Blue << 16 | Green << 8 | Red;
  } else if (dec_cont->Hdrs.color_type == PNG_COLOR_TYPE_INDEX) {
    if (dec_cont->chunk_info.length > (1 << dec_cont->Hdrs.bit_depth)){
      STREAMTRACE_E("%s", "tRNS Chunk Size invalid");
      return HANTRO_NOK;
    }
    if (dec_cont->palette_buf.virtual_address == 0 || (dec_cont->decode_stat & PNG_HAVE_PALETTE) == 0){
      STREAMTRACE_E("%s","PNG ERROR:for INDEXED-COLOR with tRNS case, need PLTE chunk before tRNS chunk.\n");
      return HANTRO_NOK;
    }
    for(int i = 0; i < dec_cont->chunk_info.length; i++){
      dec_cont->palette[4 * i + 3] = PngDecGetByte(strm);
    }
    for(int i = dec_cont->chunk_info.length; i < (1 << dec_cont->Hdrs.bit_depth); i++){
      dec_cont->palette[4 * i + 3] = 255;
    }
  } else {
    STREAMTRACE_I("%s", "Unsupported Color Type with alpha channel for tRNS Chunk");
    ret = HANTRO_NOK;
  }

  crc = PngDecGet4Bytes(strm);
  if(crc != dec_cont->chunk_info.crc) {
    STREAMTRACE_E("%s", "CRC Not Correct\n");
    return HANTRO_NOK;
  }

  return ret;
}

u32 PngCheckIsValidChunk(PngDecContainer *dec_cont, u32 chunk_type, u32 len) {
  u32 i;
  u32 cy = chunk_type;

  STREAMTRACE_I("%s\n","Check Chunk Type");

  dec_cont->chunk_info.chunk_type = chunk_type;
  for (i = 0; i < 4; i++)
  {
    u32 c = cy & 0xFF;
    if (c < 65 || c > 122 || (c > 90 && c < 97)) {
      STREAMTRACE_E("%s", "Invalid Chunk Type\n");
      return HANTRO_NOK;
    }
    cy >>= 8;
  }

  STREAMTRACE_I("%s\n","Check Chunk Length");
  if (chunk_type == PNG_IDAT && len > PNG_MAX_LENGTH)
  {
    STREAMTRACE_E("%s", "Invalid Chunk Length\n");
    return HANTRO_NOK;
  }
  if(len > dec_cont->stream.stream_length) {
    STREAMTRACE_E("%s","Chunk Length is Error(bigger than stream length)!\n");
    return HANTRO_NOK;
  }
  dec_cont->chunk_info.length = len;
  return HANTRO_OK;
}