/*------------------------------------------------------------------------------
--       Copyright (c) 2015, VeriSilicon Inc. All rights reserved             --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
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

#include "low_latency.h"
#include "decapicommon.h"
#include "dec_log.h"

/** \brief A function that transfers 32 bits between LSB and MSB.
 *  \ingroup common_group */
#define TRANSE32(x) ((((u32)(x) & 0xff000000) >> 24 ) |\
                     (((u32)(x) & 0x00ff0000) >> 8 ) |\
                     (((u32)(x) & 0x0000ff00) << 8 )|\
                     (((u32)(x) & 0x000000ff) << 24 ))

static u32 CheckLittleEndian(){
  u32 flag = 1;
  if (*((u8 *)&flag))
    return 1;
  else
    return 0;
}

void SwUpdateStrmInfoCtrl(struct LLStrmInfo *llstrminfo, u32 *len_update) {
  u32 strm_status = 0;
  u32 tmp_size = 0;
  u32 pack_size = LOW_LATENCY_PACKET_SIZE;
  addr_t hw_data_end = llstrminfo->stream_info.strm_bus_addr;
  /* stream status buffer */
  u32 *strm_status_addr = llstrminfo->strm_status_addr;
  struct strmInfo *stream_info = &llstrminfo->stream_info;

  APITRACE("start bus addr %08x\n", (u32)llstrminfo->ll_strm_bus_address);
  /* for ringbuffer need update hw_data_end */
  if (hw_data_end < llstrminfo->ll_strm_bus_address)
    hw_data_end = hw_data_end - stream_info->strm_buff_bus_address + stream_info->strm_buff_bus_address + stream_info->buff_size;
  if (hw_data_end - llstrminfo->ll_strm_bus_address >= pack_size){
    tmp_size = ((hw_data_end - llstrminfo->ll_strm_bus_address)/ pack_size) * pack_size;
    llstrminfo->ll_strm_bus_address += tmp_size;
    llstrminfo->ll_strm_len += tmp_size;
  }

  /* the last data is exactly a multiple of the pack size */
  if ((hw_data_end == llstrminfo->ll_strm_bus_address) && (stream_info->last_flag == 1) && *len_update) {
    strm_status = llstrminfo->ll_strm_len & 0x7FFFFFFF;
    strm_status = strm_status | (stream_info->last_flag << 31);
    /* check if the CPU is LSB or MSB, update strm length in ddr */
    if (CheckLittleEndian())
      *strm_status_addr = strm_status;
    else
      *strm_status_addr = TRANSE32(strm_status);
    *len_update = 0;
  /* not the last piece of data */
  } else if (stream_info->last_flag == 0 && *len_update) {
    /* check if the CPU is LSB or MSB, update strm length in ddr */
    strm_status = llstrminfo->ll_strm_len & 0x7FFFFFFF;
    if(CheckLittleEndian())
      *strm_status_addr = strm_status;
    else
      *strm_status_addr = TRANSE32(strm_status);
  }

  /* is the last piece of data but the length is not a multiple of the pack size */
  if ((hw_data_end - llstrminfo->ll_strm_bus_address) < pack_size &&
      (hw_data_end != llstrminfo->ll_strm_bus_address)
      && stream_info->last_flag == 1) {
    llstrminfo->ll_strm_len += (hw_data_end - llstrminfo->ll_strm_bus_address);
    llstrminfo->ll_strm_bus_address = hw_data_end;
    /* enable the last packet flag */
    strm_status = llstrminfo->ll_strm_len & 0x7FFFFFFF;
    /* enable the last buffer flag in ddr */
    strm_status = strm_status | (stream_info->last_flag << 31);
    /* check if the CPU is LSB or MSB, update strm length in ddr */
    if(CheckLittleEndian())
      *strm_status_addr = strm_status;
    else
      *strm_status_addr = TRANSE32(strm_status);
    *len_update = 0;
  }
  APITRACE("end bus addr %08x len update %08x\n", (u32)hw_data_end, *strm_status_addr);
  DWLSyncMemData(llstrminfo->dwl_inst, &llstrminfo->strm_status, 0, 16 * 4, HOST_TO_DEVICE);
}

void SwUpdateTileInfoCtrl(struct LLTileInfo *lltileinfo) {
  u32 tile_status = 0;
  tile_status = lltileinfo->tile_id & 0x7FFFFFFF;
  tile_status = tile_status | (lltileinfo->update_done << 31);
  if(CheckLittleEndian())
    *lltileinfo->tile_status_addr = tile_status;
  else
    *lltileinfo->tile_status_addr = TRANSE32(tile_status);
}
