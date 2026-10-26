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

#include "deccfg.h"
#include "ppu.h"
#include "regdrv.h"
#include "commonconfig.h"
#include "sw_util.h"
#include "sw_debug.h"
#include "input_queue.h"
#include "dec_log.h"
#include "parselogmsg.h"
#define _USE_MATH_DEFINES
#include <math.h>
#define PP_MAX_STRIDE 65536
#define DOWN_SCALE_SIZE(w, ds,interlace) (interlace) ? ((((w)/2/(ds)) & ~0x1)<<1) : (((w)/(ds)) & ~0x1)
#define TOFIX(d, q) ((u64)( (d) * ((u64)1<<(q)) ))
#define FDIVI(a, b) ((a)/(b))
#define ABS(a) (((a) < 0) ? -(a) : (a))
#define JPEG 3
#define COEFF_WIDTH 3
#define ALIGN_UP_TO_8(num) ((num >> 3) + 1) << 3
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MAX4(a, b, c, d) MAX(MAX(a, b), MAX(c, d))

// do not change the order in coeff
static const u32 coeff[10][5] = {{16384,22970,11700,5638,29032}, // BT601, full yuv -> full rgb
                                {19077,26149,13320,6419,33050}, // BT601_L, limited yuv -> limited rgb
                                {16384,25802,7670,3069,30402}, // BT709, full yuv -> full rgb
                                {19077,29372,8731,3494,34610}, // BT709_L, limited yuv -> limited rgb
                                {16384,24160,9361,2696,30825}, // BT2020, full yuv -> full rgb
                                {19077,27504,10646,3069,35091}, // BT2020_L, limited yuv -> limited rgb
                                {14071,14392,0,0,0}, // 8bit full yuv -> limited yuv
                                {19077,18651,0,0,0}, // 8bit limited yuv -> full yuv
                                {14030,14350,0,0,0}, // 10bit full yuv -> limited yuv
                                {19133,18706,0,0,0}}; // 10bit limited yuv -> full yuv

/*norm*/
av_unused static u32 FP32TOU32(float f)
{
    union
    {
        float f;
        unsigned int u;
    } usf;
    usf.f = f;
    return usf.u;
}

static double nResult(double x, double n) {
  return (n == 1) ? x : nResult(x, n - 1) * x / n;
}


static double my_sin(double x) {
  u32 i = 1;
  double result = 0, n = 0;
  while(ABS(n = nResult(x, 2 * i - 1)) > 1e-7) {
    result += (i%2 == 1) ? n : -n;
    i++;
  }
  return result;
}

static const u32 stride_unit[] = {1, 2, 4, 8, 16, 3, 64, 512};

static enum StrideUnit AdjustStrideUnit(u32 ystride, u32 cstride) {
  enum StrideUnit unit;
  for (unit = STRIDE_UNIT_1B; unit < STRIDE_UNIT_NOT_SUPPORTED; unit++) {
    if (ystride % stride_unit[unit] == 0 &&
        ystride/stride_unit[unit] < PP_MAX_STRIDE &&
        cstride % stride_unit[unit] == 0 &&
        cstride/stride_unit[unit] < PP_MAX_STRIDE) {
      break;
    }
  }
  return unit;
}

static void CalOutTableIndex(u32 table_in_index, u32 coeff_in_index, u32 filter_size,
                             u32* table_out_index, u32* coeff_out_index) {
#ifdef USE_COEFF_MIRROR
  if (table_in_index <= 16) {
    *table_out_index = table_in_index;
    *coeff_out_index = coeff_in_index;
  } else {
    *table_out_index = 32 - table_in_index;
    *coeff_out_index = filter_size - 1 - coeff_in_index;
  }
#else
  *table_out_index = table_in_index;
  *coeff_out_index = coeff_in_index;
#endif
}

#define ROUNDED_DIV(a,b) (((a)>=0 ? (a) + ((b)>>1) : (a) - ((b)>>1))/(b))
static unsigned av_log2(unsigned x) {
  x =  ((x)|1);
  unsigned i = sizeof(x) * 8;
  while (x) {
      x >>= 1;
      i--;
  }
  i = (31 -i);
  return i;
}

static double getSplineCoeff(double a, double b, double c,
                             double d, double dist) {
  if (dist <= 1.0)
    return ((d * dist + c) * dist + b) * dist + a;
  else
    return getSplineCoeff(0.0, b + 2.0 * c + 3.0 * d,
                          c + 3.0 * d,
                          -b - 3.0 * c - 6.0 * d,
                          dist - 1.0);
}

static void CheckLanFilterSize(const struct DecHwFeatures *hw_feature, PpUnitIntConfig *ppu_cfg, u32 *x_flag, u32 *y_flag) {
  u32 j = 0;
  i32 xDstInSrc, yDstInSrc;
  i32 scale_ratio_x_inv, scale_ratio_y_inv, scale_ratio_x_inv_ch, scale_ratio_y_inv_ch;
  i32 start_pos[5] = {0, 0, 0, 0, 0};
  i32 end_pos[5] = {0, 0, 0, 0, 0};
  i32 pos, x_pos, y_pos;
  u32 crop_width_ch, scale_width_ch, crop_height_ch, scale_height_ch;
  u32 x_filter_size = ppu_cfg->x_filter_size;
  u32 y_filter_size = ppu_cfg->y_filter_size;
  *x_flag = *y_flag = 0;
  /* horizontal*/
  crop_width_ch = ppu_cfg->sub_x == 2 ? (ppu_cfg->crop.width + 1) / 2 : ppu_cfg->crop.width;
  scale_width_ch = ppu_cfg->sub_x == 2 ? (ppu_cfg->scale.width + 1) / 2 : ppu_cfg->scale.width;
  scale_ratio_x_inv = FDIVI(TOFIX(ppu_cfg->crop.width, 16) + ppu_cfg->scale.width / 2, ppu_cfg->scale.width);
  scale_ratio_x_inv_ch = FDIVI(TOFIX(crop_width_ch, 16) + scale_width_ch / 2, scale_width_ch);
  if (scale_ratio_x_inv % 2) scale_ratio_x_inv++;
  if (scale_ratio_x_inv_ch % 2) scale_ratio_x_inv_ch++;
  /*check luma*/
  if (ppu_cfg->src_sel_mode == DOWN_ROUND) {
    xDstInSrc = scale_ratio_x_inv - 65536;
  } else if (ppu_cfg->src_sel_mode == NO_ROUND) {
    xDstInSrc = 0;
  } else {
    xDstInSrc = scale_ratio_x_inv;
  }
  pos = xDstInSrc / 2 - scale_ratio_x_inv;
  x_pos = 0;
  if (ppu_cfg->crop.width > ppu_cfg->scale.width) {
    for (j = 0; j < ppu_cfg->scale.width; j++) {
      pos += scale_ratio_x_inv;
      x_pos = FLOOR(pos+32768);
      start_pos[j % 5] = x_pos - x_filter_size / 2;
      end_pos[j % 5] = x_pos + x_filter_size / 2;
      if ((j >= 4) && (start_pos[j % 5] < end_pos[(j + 1) % 5])) {
        *x_flag += 1;
        break;
      }
    }
  }

  if (*x_flag == 1)
    x_filter_size -= 2;

  /*check chroma*/
  //if (!hw_feature->crop_step_rshift) {
    // if (*x_flag == 0) {
      //if (!ppu_cfg->monochrome && !(ppu_cfg->chroma_format == PP_CHROMA_444) && !(ppu_cfg->out_format == PP_OUT_FMT_RGB))
      if ((ppu_cfg->chroma_format == PP_CHROMA_420 ||
           ppu_cfg->chroma_format == PP_CHROMA_422) &&
          !ppu_cfg->monochrome) {
        if (ppu_cfg->src_sel_mode == DOWN_ROUND) {
          xDstInSrc = scale_ratio_x_inv_ch - 65536;
        } else if (ppu_cfg->src_sel_mode == NO_ROUND) {
          xDstInSrc = 0;
        } else {
          xDstInSrc = scale_ratio_x_inv_ch;
        }
        pos = xDstInSrc / 2 - scale_ratio_x_inv_ch;
        x_pos = 0;
        if (crop_width_ch > scale_width_ch) {
          for (j = 0; j < scale_width_ch; j++) {
            pos += scale_ratio_x_inv_ch;
            x_pos = FLOOR(pos+32768);
            start_pos[j % 5] = x_pos - x_filter_size / 2;
            end_pos[j % 5] = x_pos + x_filter_size / 2;
            if ((j >= 4) && (start_pos[j % 5] < end_pos[(j + 1) % 5])) {
              *x_flag += 1;
              break;
            }
          }
        }
      }
    // }
  //}
  //when filter_size = 8 * n + 1, HW performace will drop heavy, convert to 8 * n - 1
   if (*x_flag == 2)
    x_filter_size -= 2;

  if ((x_filter_size / 8) && (x_filter_size % 8) == 1)
    *x_flag += 1;

  /*vertical*/
  for (j = 0; j < 5; j++) {
    start_pos[j] = end_pos[j] = 0;
  }
  crop_height_ch = ppu_cfg->sub_y == 2 ? (ppu_cfg->crop.height + 1) / 2 : ppu_cfg->crop.height;
  scale_height_ch = ppu_cfg->sub_y == 2 ? (ppu_cfg->scale.height + 1) / 2 : ppu_cfg->scale.height;
  scale_ratio_y_inv = FDIVI(TOFIX(ppu_cfg->crop.height, 16) + ppu_cfg->scale.height / 2, ppu_cfg->scale.height);
  scale_ratio_y_inv_ch = FDIVI(TOFIX(crop_height_ch, 16) + scale_height_ch / 2, scale_height_ch);
  if (scale_ratio_y_inv % 2) scale_ratio_y_inv++;
  if (scale_ratio_y_inv_ch % 2) scale_ratio_y_inv_ch++;
  /*check luma*/
  if (ppu_cfg->src_sel_mode == DOWN_ROUND) {
    yDstInSrc = scale_ratio_y_inv - 65536;
  } else if (ppu_cfg->src_sel_mode == NO_ROUND) {
    yDstInSrc = 0;
  } else {
    yDstInSrc = scale_ratio_y_inv;
  }
  pos = yDstInSrc / 2 - scale_ratio_y_inv;
  y_pos = 0;
  if (ppu_cfg->crop.height > ppu_cfg->scale.height) {
    for (j = 0; j<ppu_cfg->scale.height; j++) {
      pos += scale_ratio_y_inv;
      y_pos = FLOOR(pos+32768);
      start_pos[j % 5] = y_pos - y_filter_size / 2;
      end_pos[j % 5] = y_pos + y_filter_size / 2;
      if ((j >= 4) && (start_pos[j % 5] < end_pos[(j + 1) % 5])) {
        *y_flag += 1;
        break;
      }
    }
  }

  if (*y_flag == 1)
    y_filter_size -= 2;

  /*check chroma*/
  //if (!hw_feature->crop_step_rshift) {
    // if(*y_flag == 0) {
      if (ppu_cfg->chroma_format == PP_CHROMA_420 && !ppu_cfg->monochrome){
        if (ppu_cfg->src_sel_mode == DOWN_ROUND) {
          yDstInSrc = scale_ratio_y_inv_ch - 65536;
        } else if (ppu_cfg->src_sel_mode == NO_ROUND) {
          yDstInSrc = 0;
        } else {
          yDstInSrc = scale_ratio_y_inv_ch;
        }
        pos = yDstInSrc / 2 - scale_ratio_y_inv_ch;
        y_pos = 0;
        if (crop_height_ch > scale_height_ch) {
          for (j = 0; j< scale_height_ch; j++) {
            pos += scale_ratio_y_inv_ch;
            y_pos = FLOOR(pos+32768);
            start_pos[j % 5] = y_pos - y_filter_size / 2;
            end_pos[j % 5] = y_pos + y_filter_size / 2;
            if ((j >= 4) && (start_pos[j % 5] < end_pos[(j + 1) % 5])) {
              *y_flag += 1;
              break;
            }
          }
        }
      }
    // }
  //}
  if (*y_flag == 2)
    y_filter_size -= 2;

  if ((y_filter_size / 8) && (y_filter_size % 8) == 1)
    *y_flag += 1;
}

void InitPpUnitBoundCoeff(const struct DecHwFeatures *hw_feature,
                                  u32 field_pic,
                                  PpUnitIntConfig *ppu_cfg) {
  u32 i = 0, j;
  i32 k;
  i32 yDstInSrc, scale_ratio_y_inv, scale_ratio_y_inv_ch;
  i32 start_pos, end_pos, table_index;
  i32 pos, y_pos;
  u32 start_count, end_count;
  u32 out_index, coeff_index;
  i16 value;
  i16 *table_data;
  u32 shift = field_pic ? 2 : 1;
  u32 phase_repet_num = (1 << (4 - ppu_cfg->y_phase_num));
  i16 start_value[3]={0,0,0};
  i16 end_value[3]={0,0,0};
  i16 chroma_end_value[3]={0,0,0};
  u32 y_filter_stride;
  u32 sub_y = 0;
  u32 crop_h_ch = 0, scale_h_ch = 0;
  u8 bound_count = 2;
  u8 TableHLength = 0;
  for (i = 0; i < hw_feature->max_ppu_count; i++, ppu_cfg++) {
    if (!ppu_cfg->enabled) continue;
    /* low cost alg version bilinear/bicubic/NEAREST not need to calculate coeff for interlaced case */
    if (hw_feature->scale_cost_support[i] != MIDDLE_SCALER && ppu_cfg->lanczos_table.virtual_address) {
      if (ppu_cfg->rgb) {
        sub_y = 1;
      } else {
        sub_y = (ppu_cfg->chroma_format == PP_CHROMA_422 || ppu_cfg->chroma_format == PP_CHROMA_444) ? 1 : 2;
      }
      crop_h_ch = sub_y == 2 ? (ppu_cfg->crop.height + 1) / sub_y : ppu_cfg->crop.height / sub_y;
      scale_h_ch = sub_y == 2 ? (ppu_cfg->scale.height + 1) / sub_y : ppu_cfg->scale.height / sub_y;
      scale_ratio_y_inv = FDIVI(TOFIX(ppu_cfg->crop.height, 16) + ppu_cfg->scale.height / 2, ppu_cfg->scale.height);
      scale_ratio_y_inv_ch = FDIVI(TOFIX(crop_h_ch, 16) + scale_h_ch / 2, scale_h_ch);
      if(hw_feature->scale_cost_support[i] == ULTRA_SCALER) {
        table_data = (i16*)ppu_cfg->lanczos_table.virtual_address + 32*32*2 + 16;
        yDstInSrc = scale_ratio_y_inv - 65536;
        bound_count = 3;
        TableHLength = ppu_cfg->y_phase_num + 1;
      } else {
        table_data = (i16*)ppu_cfg->lanczos_table.virtual_address + X_COEFF_OFFSET + Y_COEFF_OFFSET + 16;
        /*luma*/
        if (ppu_cfg->src_sel_mode == DOWN_ROUND) {
          yDstInSrc = scale_ratio_y_inv - 65536;
        } else if (ppu_cfg->src_sel_mode == NO_ROUND) {
          yDstInSrc = 0;
        } else {
          yDstInSrc = scale_ratio_y_inv;
        }
        if (scale_ratio_y_inv % 2) scale_ratio_y_inv++;
        if (scale_ratio_y_inv_ch % 2) scale_ratio_y_inv_ch++;
      }
      pos = yDstInSrc / 2 - scale_ratio_y_inv;
      y_pos = 0;
      start_count = 0;
      end_count = 0;
      phase_repet_num = (1 << (4 - ppu_cfg->y_phase_num));
      if (ppu_cfg->crop.height > ppu_cfg->scale.height)
        y_filter_stride = NEXT_MULTIPLE((ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset), 8);
      else
        y_filter_stride = 8;
      if (ppu_cfg->scale.height < ppu_cfg->crop.height) {
        for (j = 0; j< ppu_cfg->scale.height / shift; j++) {
          pos += scale_ratio_y_inv;
          y_pos = FLOOR(pos+32768);
          start_pos = y_pos - (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset)/2;
          end_pos = y_pos + (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset)/2;
          if(hw_feature->scale_cost_support[i] == ULTRA_SCALER)
            table_index = ((pos+32768 - MAKE_FIXED_UNI(y_pos)) >> (FIXED_BITS_UNI-TableHLength));
          else
            table_index = ((pos+32768 - MAKE_FIXED_UNI(y_pos)) >> (FIXED_BITS_UNI-TABLE_LENGTH));
          value = 0;
          if (start_pos < 0) {
            for (k = start_pos; k <= 0; k++) {
              CalOutTableIndex(table_index, k-start_pos, (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset), &out_index, &coeff_index);
              if(hw_feature->scale_cost_support[i] == ULTRA_SCALER)
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + 32 * 32 + out_index * y_filter_stride + coeff_index);
              else
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + X_COEFF_OFFSET + out_index / phase_repet_num * y_filter_stride + coeff_index);
            }
            start_value[start_count++] = value;
          }
          // else {
          //   if (start_count < 2) {
          //     CalOutTableIndex(table_index, 0, (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset), &out_index, &coeff_index);
          //     value += *((i16*)ppu_cfg->lanczos_table.virtual_address + X_COEFF_OFFSET + out_index / phase_repet_num * y_filter_stride + coeff_index);
          //     start_value[start_count] = value;
          //     start_count++;
          //   }
          // }
          value = 0;
          if ((end_pos >= ppu_cfg->crop.height / shift)) {
            for (k = ppu_cfg->crop.height / shift - 1; k <= end_pos; k++) {
              if (start_pos == end_pos && k == end_pos)
                continue;
              CalOutTableIndex(table_index, k-start_pos > 0 ? k - start_pos : 0, (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset), &out_index, &coeff_index);
              if(hw_feature->scale_cost_support[i] == ULTRA_SCALER)
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + 32 * 32 + out_index  * y_filter_stride + coeff_index);
              else
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + X_COEFF_OFFSET + out_index / phase_repet_num * y_filter_stride + coeff_index);
            }
            end_value[end_count++] = value;
            // end_count++;
          }
        }
        for (j = 0; j < bound_count; j++) {
          *table_data++ = start_value[j];
          #if ULTRA_SCALE_DEBUG
          printf("interlace case, luma vertical j: %u, start_value: %d\n", j, start_value[j]);
          #endif
        }
        for (j = 0; j < bound_count; j++) {
          *table_data++ = end_value[j];
          #if ULTRA_SCALE_DEBUG
            printf("interlace case, luma vertical j: %u, end_value: %d\n", j, end_value[j]);
          #endif
        }
      }
      /*chroma*/
      if(hw_feature->scale_cost_support[i] == ULTRA_SCALER) {
        table_data = (i16*)ppu_cfg->lanczos_table.virtual_address + 32*32*2 + 24;
        yDstInSrc = scale_ratio_y_inv_ch - 65536;
      } else {
        if (ppu_cfg->src_sel_mode == DOWN_ROUND) {
          yDstInSrc = scale_ratio_y_inv_ch - 65536;
        } else if (ppu_cfg->src_sel_mode == NO_ROUND) {
          yDstInSrc = 0;
        } else {
          yDstInSrc = scale_ratio_y_inv_ch;
        }
      }
      pos = yDstInSrc / 2 - scale_ratio_y_inv_ch;
      y_pos = 0;
      end_count = 0;
      if (ppu_cfg->scale.height < ppu_cfg->crop.height) {
        for (j = 0; j < (sub_y == 2 ? (ppu_cfg->scale.height + 1) / 2 : ppu_cfg->scale.height) / shift; j++) {
          pos += scale_ratio_y_inv_ch;
          y_pos = FLOOR(pos+32768);
          start_pos = y_pos - (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset)/2;
          end_pos = y_pos + (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset)/2;
          if(hw_feature->scale_cost_support[i] == ULTRA_SCALER)
            table_index = ((pos+32768 - MAKE_FIXED_UNI(y_pos)) >> (FIXED_BITS_UNI-TableHLength));
          else
            table_index = ((pos+32768 - MAKE_FIXED_UNI(y_pos)) >> (FIXED_BITS_UNI-TABLE_LENGTH));
          value = 0;
          if ((end_pos >= (sub_y == 2 ? (ppu_cfg->crop.height + 1) / 2 : ppu_cfg->crop.height) / shift) &&
              end_count < bound_count) {
            for (k = (sub_y == 2 ? (ppu_cfg->crop.height + 1) / 2 : ppu_cfg->crop.height)/ shift - 1; k <= end_pos; k++) {
              if (start_pos == end_pos && k == end_pos)
                continue;
              CalOutTableIndex(table_index, k-start_pos > 0 ? k - start_pos : 0, (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset), &out_index, &coeff_index);
              if(hw_feature->scale_cost_support[i] == ULTRA_SCALER)
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + 32 * 32 + out_index  * y_filter_stride + coeff_index);
              else
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + X_COEFF_OFFSET + out_index / phase_repet_num * y_filter_stride + coeff_index);
            }
            chroma_end_value[end_count++] = value;
          }
        }
        table_data += bound_count; /*only need modify bottom bound coeff*/
        for (j = 0; j < bound_count; j++) {
          *table_data++ = chroma_end_value[j];
          #if ULTRA_SCALE_DEBUG
            printf("interlace case, chroma vertical j: %u, end_value: %d\n", j, chroma_end_value[j]);
          #endif
        }
      }
    }
    u32 n_cores = ppu_cfg->lanczos_table.size / LANCZOS_COEFF_BUFFER_SIZE / sizeof(i16);
    ASSERT(ppu_cfg->dwl);
    for (j = 0; j < n_cores; j++) {
      DWLSyncMemData(ppu_cfg->dwl, &ppu_cfg->lanczos_table, j * LANCZOS_COEFF_BUFFER_SIZE * sizeof(i16),
                      LANCZOS_COEFF_BUFFER_SIZE * sizeof(i16), HOST_TO_DEVICE);
    }
  }
}

static void InitPpUnitLanczosData(const struct DecHwFeatures *hw_feature,
                                  PpUnitIntConfig *ppu_cfg) {
  u32 i = 0, j, m;
  i32 k;
  i32 xDstInSrc, yDstInSrc;
  i32 scale_ratio_x_inv, scale_ratio_y_inv;
  i32 scale_ratio_x_inv_ch, scale_ratio_y_inv_ch;
  u32 crop_w_ch, crop_h_ch, scale_w_ch, scale_h_ch;
  i32 start_pos, end_pos, table_index;
  i32 pos, x_pos, y_pos;
  u32 start_count, end_count;
  u32 out_index, coeff_index;
  i32 total;
  i16 value;
  u32 x_flag = 0;
  u32 y_flag = 0;
  /*support odd crop*/
  u32 sub_x;
  u32 sub_y;
  u32 pp_area = 0;
  u32 heif_mode = ppu_cfg->heif_mode;
  for (i = 0; i < hw_feature->max_ppu_count; i++, ppu_cfg++) {
    // u32 shift_bit = hw_feature->pp_area_optimize ? 10 : 7;
    u32 shift_bit = 10;
    if (!ppu_cfg->enabled) continue;
    if (heif_mode &&
        ppu_cfg->pre_lanczos_info.crop_w == ppu_cfg->crop.width &&
        ppu_cfg->pre_lanczos_info.crop_h == ppu_cfg->crop.height &&
        ppu_cfg->pre_lanczos_info.scale_w == ppu_cfg->scale.width &&
        ppu_cfg->pre_lanczos_info.scale_h == ppu_cfg->scale.height &&
        ppu_cfg->pre_lanczos_info.pp_filter == ppu_cfg->pp_filter)
      continue;

    u32 least_coeff = (1 << shift_bit);
    if (ppu_cfg->rgb) {
      sub_x = sub_y = 1;
    } else {
      sub_x = ppu_cfg->chroma_format == PP_CHROMA_444 ? 1 : 2;
      sub_y = (ppu_cfg->chroma_format == PP_CHROMA_422 || ppu_cfg->chroma_format == PP_CHROMA_444) ? 1 : 2;
    }
    if (!ppu_cfg->enabled) continue;

    if ((ppu_cfg->crop.width >= ppu_cfg->scale.width) && (ppu_cfg->crop.height >= ppu_cfg->scale.height)
        && hw_feature->scale_cost_support[i] && ppu_cfg->pp_filter == PP_AREA)
      pp_area = 1;
    else
      pp_area = 0;
    if (!pp_area && ppu_cfg->lanczos_table.virtual_address) {
      u32 x_filter_size, y_filter_size;
      u32 base_x_filter_size, base_y_filter_size;
      u32 x_filter_stride, y_filter_stride;
      u32 x_coeff_base, y_coeff_base;
      if (ppu_cfg->crop.width > ppu_cfg->scale.width) {
        if (ppu_cfg->pp_filter == PP_VSI_LINEAR) {
          x_filter_size = 2 * ((ppu_cfg->crop.width) / ppu_cfg->scale.width);
        } else if (ppu_cfg->pp_filter == PP_FAST_LINEAR) {
          x_filter_size = 2;
        } else if (ppu_cfg->pp_filter == PP_FAST_BICUBIC) {
          x_filter_size = 4;
        } else if (ppu_cfg->pp_filter == PP_LANCZOS || ppu_cfg->pp_filter == PP_BILINEAR ||
                 ppu_cfg->pp_filter == PP_BICUBIC || ppu_cfg->pp_filter == PP_SPLINE) {
          x_filter_size = (2 * ppu_cfg->x_filter_param * ppu_cfg->crop.width + ppu_cfg->scale.width - 1) / ppu_cfg->scale.width;
        } else if (ppu_cfg->pp_filter == PP_BOX) {
          x_filter_size = (ppu_cfg->x_filter_param * ppu_cfg->crop.width + ppu_cfg->scale.width - 1) / ppu_cfg->scale.width;
        } else { /*PP_NEAREST*/
          x_filter_size = 1;
        }
      } else if (ppu_cfg->crop.width < ppu_cfg->scale.width) {
        if (ppu_cfg->pp_filter == PP_FAST_LINEAR || ppu_cfg->pp_filter == PP_BILINEAR) {
          x_filter_size = 2;
        } else if (ppu_cfg->pp_filter == PP_FAST_BICUBIC || ppu_cfg->pp_filter == PP_BICUBIC) {
          x_filter_size = 4;
        } else if (ppu_cfg->pp_filter == PP_LANCZOS) {
          x_filter_size = (LAN_SIZE_HOR - 1);
        } else { /*PP_NEAREST*/
          x_filter_size = 1;
        }
      } else {
        x_filter_size = 0;
      }
      if (ppu_cfg->crop.height > ppu_cfg->scale.height) {
        if (ppu_cfg->pp_filter == PP_VSI_LINEAR) {
          y_filter_size = 2 * ((ppu_cfg->crop.height) / ppu_cfg->scale.height);
        } else if (ppu_cfg->pp_filter == PP_FAST_LINEAR) {
          y_filter_size = 2;
        } else if (ppu_cfg->pp_filter == PP_FAST_BICUBIC) {
          if (hw_feature->scale_cost_support[i] == MIDDLE_SCALER){
            y_filter_size = 2;
          } else {
            y_filter_size = 4;
          }
        } else if (ppu_cfg->pp_filter == PP_LANCZOS || ppu_cfg->pp_filter == PP_BILINEAR ||
                 ppu_cfg->pp_filter == PP_BICUBIC || ppu_cfg->pp_filter == PP_SPLINE) {
          y_filter_size = (2 * ppu_cfg->y_filter_param * ppu_cfg->crop.height + ppu_cfg->scale.height - 1) / ppu_cfg->scale.height;
        } else if (ppu_cfg->pp_filter == PP_BOX) {
          y_filter_size = (ppu_cfg->y_filter_param * ppu_cfg->crop.height + ppu_cfg->scale.height - 1) / ppu_cfg->scale.height;
        } else { /*PP_NEAREST*/
          y_filter_size = 1;
        }
      } else if (ppu_cfg->crop.height < ppu_cfg->scale.height) {
        if (ppu_cfg->pp_filter == PP_FAST_LINEAR || ppu_cfg->pp_filter == PP_BILINEAR) {
          y_filter_size = 2;
        } else if (ppu_cfg->pp_filter == PP_FAST_BICUBIC || ppu_cfg->pp_filter == PP_BICUBIC) {
          if (hw_feature->scale_cost_support[i] == MIDDLE_SCALER){
            y_filter_size = 2;
          } else {
            y_filter_size = 4;
          }
        } else if (ppu_cfg->pp_filter == PP_LANCZOS) {
          y_filter_size = (LAN_SIZE_VER - 1);
        } else { /*PP_NEAREST*/
          y_filter_size = 1;
        }
      } else {
        y_filter_size = 0;
      }
      if (ppu_cfg->crop.width != ppu_cfg->scale.width)
        x_filter_size = 1 + x_filter_size / 2 * 2;
      else
        x_filter_size = 0;
      if (ppu_cfg->crop.height != ppu_cfg->scale.height)
        y_filter_size = 1 + y_filter_size / 2 * 2;
      else
        y_filter_size = 0;
      if (x_filter_size > MIN(ppu_cfg->crop.width - 2, MAX_COEFF_SIZE_HOR_REAL))
        x_filter_size = MIN(ppu_cfg->crop.width - 2, MAX_COEFF_SIZE_HOR_REAL);
      if (y_filter_size > MIN(ppu_cfg->crop.height - 2, MAX_COEFF_SIZE_VER_REAL))
        y_filter_size = MIN(ppu_cfg->crop.height - 2, MAX_COEFF_SIZE_VER_REAL);

      if (ppu_cfg->crop.width > ppu_cfg->scale.width)
        base_x_filter_size =((LAN_SIZE_HOR - 1) * ppu_cfg->crop.width + ppu_cfg->scale.width - 1) / ppu_cfg->scale.width;
      else if (ppu_cfg->crop.width < ppu_cfg->scale.width)
        base_x_filter_size = (LAN_SIZE_HOR - 1);
      else
        base_x_filter_size = 0;
      if (ppu_cfg->crop.height > ppu_cfg->scale.height)
        base_y_filter_size = ((LAN_SIZE_VER - 1) * ppu_cfg->crop.height + ppu_cfg->scale.height - 1) / ppu_cfg->scale.height;
      else if (ppu_cfg->crop.height < ppu_cfg->scale.height)
        base_y_filter_size = (LAN_SIZE_VER - 1);
      else
        base_y_filter_size = 0;
      if (ppu_cfg->crop.width != ppu_cfg->scale.width)
        base_x_filter_size = 1 + base_x_filter_size / 2 * 2;
      else
        base_x_filter_size = 0;
      if (ppu_cfg->crop.height != ppu_cfg->scale.height)
        base_y_filter_size = 1 + base_y_filter_size / 2 * 2;
      else
        base_y_filter_size = 0;
      if (base_x_filter_size > MIN(ppu_cfg->crop.width - 2, MAX_COEFF_SIZE_HOR_REAL))
        base_x_filter_size = MIN(ppu_cfg->crop.width - 2, MAX_COEFF_SIZE_HOR_REAL);
      if (base_y_filter_size > MIN(ppu_cfg->crop.height - 2, MAX_COEFF_SIZE_VER_REAL))
        base_y_filter_size = MIN(ppu_cfg->crop.height - 2, MAX_COEFF_SIZE_VER_REAL);

#ifdef SUPPORT_FLEXIBLE_FILTER
      ppu_cfg->x_filter_size = x_filter_size;
      ppu_cfg->y_filter_size = y_filter_size;
      x_coeff_base = y_coeff_base = 0;
#else
      ppu_cfg->x_filter_size = base_x_filter_size;
      ppu_cfg->y_filter_size = base_y_filter_size;
      x_coeff_base = base_x_filter_size / 2 - x_filter_size / 2;
      y_coeff_base = base_y_filter_size / 2 - y_filter_size / 2;
#endif

      CheckLanFilterSize(hw_feature, ppu_cfg, &x_flag, &y_flag);

      if(hw_feature->scale_cost_support[i] == ULTRA_SCALER) {

        ppu_cfg->x_filter_offset = 0;
        ppu_cfg->y_filter_offset = 0;

        if (ppu_cfg->crop.width > ppu_cfg->scale.width)
          x_filter_stride = NEXT_MULTIPLE(ppu_cfg->x_filter_size, 8);
        else
          x_filter_stride = 8;
        if (ppu_cfg->crop.height > ppu_cfg->scale.height)
          y_filter_stride = NEXT_MULTIPLE(ppu_cfg->y_filter_size, 8);
        else
          y_filter_stride = 8;

        if(x_filter_stride <= 32)
          ppu_cfg->x_phase_num = 4;  // 32
        else if(32 < x_filter_stride  && x_filter_stride <= 64)
          ppu_cfg->x_phase_num = 3;  // 16
        else if(64 < x_filter_stride  && x_filter_stride <= 128)
          ppu_cfg->x_phase_num = 2;  // 8
        else if(128 < x_filter_stride && x_filter_stride <= 256)
          ppu_cfg->x_phase_num = 1;  // 4
        else if(256 < x_filter_stride && x_filter_stride <= 512)
          ppu_cfg->x_phase_num = 0;  // 2
        else if(512 < x_filter_stride && x_filter_stride <= 1024)
          ppu_cfg->x_phase_num = -1;  // 2

        if(y_filter_stride <= 32)
          ppu_cfg->y_phase_num = 4;
        else if(32  < y_filter_stride && y_filter_stride <= 64)
          ppu_cfg->y_phase_num = 3;
        else if(64 < y_filter_stride  && y_filter_stride <= 128)
          ppu_cfg->y_phase_num = 2;
        else if(128 < y_filter_stride && y_filter_stride <= 256)
          ppu_cfg->y_phase_num = 1;
        else if(256 < y_filter_stride && y_filter_stride <= 512)
          ppu_cfg->y_phase_num = 0;
        else if(512 < y_filter_stride && y_filter_stride <= 1024)
          ppu_cfg->y_phase_num = -1;

      } else {
        if (x_flag == 1)
          ppu_cfg->x_filter_offset = 2;
        else if (x_flag == 2)
          ppu_cfg->x_filter_offset = 4;
        else if (x_flag == 3)
          ppu_cfg->x_filter_offset = 6;
        else
          ppu_cfg->x_filter_offset = 0;
        if (y_flag == 1)
          ppu_cfg->y_filter_offset = 2;
        else if (y_flag == 2)
          ppu_cfg->y_filter_offset = 4;
        else if (y_flag == 3)
          ppu_cfg->y_filter_offset = 6;
        else
          ppu_cfg->y_filter_offset = 0;
        if (ppu_cfg->crop.width > ppu_cfg->scale.width)
          x_filter_stride = NEXT_MULTIPLE((ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset), 8);
        else
          x_filter_stride = 8;
        if (ppu_cfg->crop.height > ppu_cfg->scale.height)
          y_filter_stride = NEXT_MULTIPLE((ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset), 8);
        else
          y_filter_stride = 8;
      }

      /*calculate horizontal scalar filter coefficient*/
      i16 *table_data = (i16*)ppu_cfg->lanczos_table.virtual_address;
      int TableWLength = 5;
      int TableHLength = 5;

#if VCDJ_ENABLE_ULTRA_SCALER
      if(hw_feature->scale_cost_support[i] == ULTRA_SCALER) {

        int xx = 0;
        i64 d = 0;
        int64_t coeff = 0;
        i64 error = 0;
        i64 sum   = 0;

        int filterSize = 0;

        int xInc = 0;
        i64 xDst2InSrc = 0;

        int xInc_ch = 0;
        int in_ch = 0, out_ch = 0;
        int pos_ch = 0, x_pos_ch = 0;
        int table_index_ch = 0;
        i64 xDst2InSrc_ch = 0;

        i64 B =  0  * (1 << 24);
        i64 C = 0.6 * (1 << 24);
        i64 dd = 0;
        i64 ddd = 0;

        int64_t fone = 0;
        int one = (1 << 14);

        u32 tableCountX[TABLE_SIZE];
        DWLmemset(tableCountX, 0, sizeof(tableCountX));

        u32 tableCountXch[TABLE_SIZE];
        DWLmemset(tableCountXch, 0, sizeof(tableCountXch));

        u32 tableCountY[TABLE_SIZE];
        DWLmemset(tableCountY, 0, sizeof(tableCountY));

        u32 tableCountYch[TABLE_SIZE];
        DWLmemset(tableCountYch, 0, sizeof(tableCountYch));

        i64 horizontal_weight2[TABLE_SIZE][2 * 2 * 32];
        i64 vertical_weight2[TABLE_SIZE][2 * 2 * 32];

        DWLmemset(horizontal_weight2, 0, sizeof(horizontal_weight2));
        DWLmemset(vertical_weight2, 0, sizeof(vertical_weight2));

        int64_t v = 0;

        TableWLength = ppu_cfg->x_phase_num + 1;
        int TableW = 1 << (TableWLength);

        double floatd = 0;

        /* ffmpeg libswscale/utils.c initFilter */
        int srcW = ppu_cfg->crop.width;
        int dstW = ppu_cfg->scale.width;
        fone = 1LL << (54 - MIN(av_log2(srcW/dstW), 8));

        filterSize = ppu_cfg->x_filter_size;

        xInc = FDIVI(TOFIX(ppu_cfg->crop.width, 16) + ppu_cfg->scale.width / 2, ppu_cfg->scale.width);
        xDst2InSrc = ((128*(i64)xInc)>>7) - ((128*0x10000LL)>>7);

        in_ch  = ppu_cfg->sub_y == 2 ? (srcW + 1) / 2 : srcW;
        out_ch = ppu_cfg->sub_y == 2 ? (dstW + 1) / 2 : dstW;
        xInc_ch = FDIVI(TOFIX(in_ch, 16) + out_ch / 2, out_ch);

        long *filter;
        filter = (long *)DWLcalloc(dstW * filterSize, sizeof(long));

        if(ppu_cfg->crop.width != ppu_cfg->scale.width) {
          if(ppu_cfg->crop.width < ppu_cfg->scale.width) {
            if (xInc % 2)  xInc += 1;
            if (xInc_ch % 2)  xInc_ch += 1;
          }

          for (m = 0; m < dstW; m++) {
            xx = (xDst2InSrc - (filterSize - 2) * (1LL<<16)) / (1 << 17);
            for (j = 0; j < filterSize; j++) {
              d = (ABS(((int64_t)xx * (1 << 17)) - xDst2InSrc)) << 13;
              if (xInc > 1 << 16)
                d = d * dstW / srcW;
              floatd = d * (1.0 / (1 << 30));
              if (ppu_cfg->pp_filter == PP_BICUBIC) {
                if (d >= 1LL << 31) {
                    coeff = 0.0;
                } else {
                    dd  = (d  * d) >> 30;
                    ddd = (dd * d) >> 30;
                    if (d < 1LL << 30)
                        coeff =  (12 * (1 << 24) -  9 * B - 6 * C) * ddd +
                                (-18 * (1 << 24) + 12 * B + 6 * C) *  dd +
                                  (6 * (1 << 24) -  2 * B)         * (1 << 30);
                    else
                        coeff =      (-B -  6 * C) * ddd +
                                  (6 * B + 30 * C) * dd  +
                                (-12 * B - 48 * C) * d   +
                                  (8 * B + 24 * C) * (1 << 30);
                }
                coeff /= (1LL<<54)/fone;
              } else if (ppu_cfg->pp_filter == PP_LANCZOS) {
                double p = 2;
                if(ppu_cfg->x_filter_param == 3)
                  p = 3;
                coeff = (d ? my_sin(floatd * M_PI) * my_sin(floatd * M_PI / p) /
                          (floatd * floatd * M_PI * M_PI / p) : 1.0) * fone;
                if (floatd > p)
                    coeff = 0;
              } else if (ppu_cfg->pp_filter == PP_BILINEAR) {
                coeff = (1 << 30) - d;
                if (coeff < 0)
                    coeff = 0;
                coeff *= fone >> 30;
              }
              filter[m * filterSize + j] = coeff;
              xx++;
            }
            xDst2InSrc += 2 * xInc;
          }

          /* normalize & store in outFilter */
          xDst2InSrc = ((128*(i64)xInc)>>7) - ((128*0x10000LL)>>7);
          pos =  xDst2InSrc / 2 - xInc;

          xDst2InSrc_ch = ((128*(i64)xInc_ch)>>7) - ((128*0x10000LL)>>7);
          pos_ch =  xDst2InSrc_ch / 2 - xInc_ch;

          for (m = 0; m < dstW; m++) {
            error = 0;
            sum   = 0;

            for (j = 0; j < filterSize; j++) {
                sum += filter[m * filterSize + j];
            }
            sum = (sum + one / 2) / one;
            if (!sum) {
                sum = 1;
            }
            for (j = 0; j < filterSize; j++) {
                v = filter[m * filterSize + j] + error;
                int intV  = ROUNDED_DIV(v, sum);
                filter[m * filterSize + j] = intV;
                error                      = v - intV * sum;
            }

            pos += xInc;
            x_pos = FLOOR(pos+32768);
            table_index = ((pos+32768 - MAKE_FIXED_UNI(x_pos)) >> (FIXED_BITS_UNI-TableWLength));

            pos_ch += xInc_ch;
            x_pos_ch = FLOOR(pos_ch+32768);
            table_index_ch = ((pos_ch+32768 - MAKE_FIXED_UNI(x_pos_ch)) >> (FIXED_BITS_UNI-TableWLength));

            if(tableCountXch[table_index_ch] <= 5) {
              for(j = 0; j < filterSize; j++)
                horizontal_weight2[table_index_ch][j] = filter[m * filterSize + j];
            }
            tableCountXch[table_index_ch] += 1;

            if(tableCountX[table_index] <= 5) {
              for(j = 0; j < filterSize; j++)
                horizontal_weight2[table_index][j] = filter[m * filterSize + j];
            }
            tableCountX[table_index] += 1;

            // #ifdef ULTRA_SCALE_DEBUG
            // printf("dstW: %d, table_index: %d, fiterSize: %d  ", m, table_index, filterSize);
            // for (j = 0; j < filterSize; j++) {
            //   printf("%ld ", filter[m * filterSize + j]);
            // }
            // printf("\n");
            // #endif
          }

          DWLfree(filter);
        }

        for (k = 0; k < TableW; k++) {
          // #ifdef ULTRA_SCALE_DEBUG
          // printf("\nWk: %d, offset: %d\n", k, k*x_filter_stride);
          // #endif
          for (m = 0; m < filterSize; m++) {
            *table_data++ = tableCountX[k] == 0 ? 0: (i16)((horizontal_weight2[k][m]));

            #ifdef ULTRA_SCALE_DEBUG
            printf("%d ", tableCountX[k] == 0 ? 0: (i16)( (horizontal_weight2[k][m])));
            #endif
          }
          for (m = 0; m < x_filter_stride - filterSize; m++)
            *table_data++ = 0;
        }

        /*calculate vertical scalar filter coefficient*/
        filterSize = 0;
        TableHLength = ppu_cfg->y_phase_num + 1;
        int TableH = 1 << (TableHLength);

        int srcH = ppu_cfg->crop.height;
        int dstH = ppu_cfg->scale.height;
        fone = 1LL << (54 - MIN(av_log2(srcH/dstH), 8));

        filterSize = ppu_cfg->y_filter_size;

        xInc = FDIVI(TOFIX(ppu_cfg->crop.height, 16) + ppu_cfg->scale.height / 2, ppu_cfg->scale.height);
        in_ch  = ppu_cfg->sub_y == 2 ? (srcH + 1) / 2 : srcH;
        out_ch = ppu_cfg->sub_y == 2 ? (dstH + 1) / 2 : dstH;
        xInc_ch = FDIVI(TOFIX(in_ch, 16) + out_ch / 2, out_ch);
        xDst2InSrc = ((128*(i64)xInc)>>7) - ((128*0x10000LL)>>7);

        long *filterY;
        filterY = (long *)DWLcalloc(dstH * filterSize, sizeof(long));

        if(ppu_cfg->crop.height != ppu_cfg->scale.height) {
          if(ppu_cfg->crop.height < ppu_cfg->scale.height) {
            if (xInc % 2)  xInc += 1;
            if (xInc_ch % 2)  xInc_ch += 1;
          }
          for (m = 0; m < dstH; m++) {
            xx = (xDst2InSrc - (filterSize - 2) * (1LL<<16)) / (1 << 17);
            for (j = 0; j < filterSize; j++) {
              d = (ABS(((int64_t)xx * (1 << 17)) - xDst2InSrc)) << 13;
              if (xInc > 1 << 16)
                d = d * dstH / srcH;
              floatd = d * (1.0 / (1 << 30));
              if (ppu_cfg->pp_filter == PP_BICUBIC) {
                if (d >= 1LL << 31) {
                    coeff = 0.0;
                } else {
                    dd  = (d  * d) >> 30;
                    ddd = (dd * d) >> 30;
                    if (d < 1LL << 30)
                        coeff =  (12 * (1 << 24) -  9 * B - 6 * C) * ddd +
                                (-18 * (1 << 24) + 12 * B + 6 * C) *  dd +
                                  (6 * (1 << 24) -  2 * B)         * (1 << 30);
                    else
                        coeff =      (-B -  6 * C) * ddd +
                                  (6 * B + 30 * C) * dd  +
                                (-12 * B - 48 * C) * d   +
                                  (8 * B + 24 * C) * (1 << 30);
                }
                coeff /= (1LL<<54)/fone;
              } else if (ppu_cfg->pp_filter == PP_LANCZOS) {
                double p = 2;
                if(ppu_cfg->y_filter_param == 3)
                  p = 3;
                coeff = (d ? my_sin(floatd * M_PI) * my_sin(floatd * M_PI / p) /
                          (floatd * floatd * M_PI * M_PI / p) : 1.0) * fone;
                if (floatd > p)
                    coeff = 0;
              } else if (ppu_cfg->pp_filter == PP_BILINEAR) {
                coeff = (1 << 30) - d;
                if (coeff < 0)
                    coeff = 0;
                coeff *= fone >> 30;
              }
              filterY[m * filterSize + j] = coeff;
              xx++;
            }
            xDst2InSrc += 2 * xInc;
          }

          /* normalize & store in outFilter */
          xDst2InSrc = ((128*(i64)xInc)>>7) - ((128*0x10000LL)>>7);
          pos =  xDst2InSrc / 2 - xInc;

          for (m = 0; m < dstH; m++) {
            error = 0;
            sum   = 0;

            for (j = 0; j < filterSize; j++) {
                sum += filterY[m * filterSize + j];
            }
            sum = (sum + one / 2) / one;
            if (!sum) {
                sum = 1;
            }
            for (j = 0; j < filterSize; j++) {
                v = filterY[m * filterSize + j] + error;
                int intV  = ROUNDED_DIV(v, sum);
                filterY[m * filterSize + j] = intV;
                error                      = v - intV * sum;
            }

            pos += xInc;
            x_pos = FLOOR(pos+32768);
            table_index = ((pos+32768 - MAKE_FIXED_UNI(x_pos)) >> (FIXED_BITS_UNI-TableHLength));

            pos_ch += xInc_ch;
            x_pos_ch = FLOOR(pos_ch+32768);
            table_index_ch = ((pos_ch+32768 - MAKE_FIXED_UNI(x_pos_ch)) >> (FIXED_BITS_UNI-TableHLength));

            if(tableCountYch[table_index_ch] <= 5) {
              for(j = 0; j < filterSize; j++)
                vertical_weight2[table_index_ch][j] = filterY[m * filterSize + j];
            }
            tableCountYch[table_index_ch] += 1;

            if(tableCountY[table_index] <= 5) {
              for(j = 0; j < filterSize; j++)
                vertical_weight2[table_index][j] = filterY[m * filterSize + j];
            }
            tableCountY[table_index] += 1;

            // #ifdef ULTRA_SCALE_DEBUG
            // printf("dstH: %d, table_index: %d, fiterSize: %d  ", m, table_index, filterSize);
            // for (j = 0; j < filterSize; j++) {
            //   printf("%ld ", filterY[m * filterSize + j]);
            // }
            // printf("\n");
            // #endif
          }

          DWLfree(filterY);
        }

        table_data = (i16*)ppu_cfg->lanczos_table.virtual_address + 32*32;
        for (k = 0; k < TableH; k++) {
          // #ifdef ULTRA_SCALE_DEBUG
          // printf("\nHk: %d, offset: %d\n", k, y_filter_stride);
          // #endif
          for (m = 0; m < filterSize; m++) {
            *table_data++ = tableCountY[k] == 0 ? 0: (i16)(vertical_weight2[k][m]);

            #ifdef ULTRA_SCALE_DEBUG
            printf("%d ", tableCountY[k] == 0 ? 0: (i16)(vertical_weight2[k][m]));
            #endif
          }
          for (m = 0; m < y_filter_stride - filterSize; m++)
            *table_data++ = 0;
        }
      } else
#endif
      {
        if (ppu_cfg->scale.width < ppu_cfg->crop.width) {
          if ((ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset) <= 7)
            ppu_cfg->x_phase_num = 4;
          else if ((ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset) > 7 &&
                  (ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset) <= 15)
            ppu_cfg->x_phase_num = 3;
          else if ((ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset) > 15 &&
                  (ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset) <= 31)
            ppu_cfg->x_phase_num = 2;
          else if ((ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset) > 31 &&
                  (ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset) <= 63)
            ppu_cfg->x_phase_num = 1;
          else
            ppu_cfg->x_phase_num = 0;
        } else {
          ppu_cfg->x_phase_num = 4;
        }
        if (ppu_cfg->scale.height < ppu_cfg->crop.height) {
          if ((ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset) <= 7)
            ppu_cfg->y_phase_num = 4;
          else if ((ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset) > 7 &&
                  (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset) <= 15)
            ppu_cfg->y_phase_num = 3;
          else if ((ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset) > 15 &&
                  (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset) <= 31)
            ppu_cfg->y_phase_num = 2;
          else if ((ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset) > 31 &&
                  (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset) <= 63)
            ppu_cfg->y_phase_num = 1;
          else
            ppu_cfg->y_phase_num = 0;
        } else {
          ppu_cfg->y_phase_num = 4;
        }
        /*calculate horizontal scalar filter coefficient*/
        double ratio = 0;
        double distance = ppu_cfg->x_filter_param;
        i32 horizontal_weight[MAX_COEFF_SIZE_HOR] = {0};
        i32 vertical_weight[MAX_COEFF_SIZE_VER] = {0};
        i16 *table_data = (i16*)ppu_cfg->lanczos_table.virtual_address;
        if (ppu_cfg->scale.width != ppu_cfg->crop.width) {
          if (ppu_cfg->scale.width > ppu_cfg->crop.width)
            ratio = 1.0;
          else
            ratio = (double)ppu_cfg->scale.width / ppu_cfg->crop.width;
          for (k = 0; k < TABLE_SIZE; k++) {
            for (m = 0; m < x_filter_size; m++) {
              if (ppu_cfg->pp_filter == PP_NEAREST)
                horizontal_weight[m + x_coeff_base] = 65535;
              else if (ppu_cfg->pp_filter == PP_VSI_LINEAR) {
                if (((x_filter_size / 2) % 2) == 1) {
                  if (m == (x_filter_size / 2 - (x_filter_size / 2 + 1) / 2) ||
                      m == (x_filter_size / 2 + (x_filter_size / 2 + 1) / 2)) {
                    horizontal_weight[m + x_coeff_base] = (65536 - (i32)((double)ppu_cfg->scale.width / ppu_cfg->crop.width * 65536)  * (x_filter_size / 2)) / 2;
                  } else if ((m > (x_filter_size / 2 - (x_filter_size / 2 + 1) / 2)) &&
                            (m < (x_filter_size / 2 + (x_filter_size / 2 + 1) / 2))) {
                    horizontal_weight[m + x_coeff_base] = (i32)((double)ppu_cfg->scale.width / ppu_cfg->crop.width * 65536);
                  } else {
                    horizontal_weight[m + x_coeff_base] = 0;
                  }
                } else {
                  if (m >= x_filter_size / 4 && m < 3 * x_filter_size / 4)
                    horizontal_weight[m + x_coeff_base] = (i32)((double)ppu_cfg->scale.width / ppu_cfg->crop.width * 65536);
                  else if (m == 3 * x_filter_size / 4)
                    horizontal_weight[m + x_coeff_base] = 65536 - (i32)((double)ppu_cfg->scale.width / ppu_cfg->crop.width * 65536) * (x_filter_size / 2);
                  else
                    horizontal_weight[m + x_coeff_base] = 0;
                }
              } else if (ppu_cfg->pp_filter == PP_FAST_LINEAR) {
                double value = (double)(m)-x_filter_size/2 - ((double)k - TABLE_SIZE/2)/TABLE_SIZE;
                if (value < 0)
                  value *= -1.0;
                else
                  value *= 1.0;
                double d = value;
                if (d >= 1)
                  horizontal_weight[m + x_coeff_base] = 0;
                else
                  horizontal_weight[m + x_coeff_base] = (i32)((1 - d) * 65536);
              } else if (ppu_cfg->pp_filter == PP_FAST_BICUBIC) {
                double value = (double)(m)-x_filter_size/2 - ((double)k - TABLE_SIZE/2)/TABLE_SIZE;
                if (value < 0)
                  value *= -1.0;
                else
                  value *= 1.0;
                double d = value;
                if (d >= 2)
                  horizontal_weight[m + x_coeff_base] = 0;
                else if (d < 1)
                  horizontal_weight[m + x_coeff_base] = (i32)((1.25 * d * d * d - 2.25 * d * d + 1) * 65536);
                else
                  horizontal_weight[m + x_coeff_base] = (i32)((-0.75 * d * d * d + 3.75 * d * d - 6 * d + 3) * 65536);
              } else {
                double value = (double)(m)-x_filter_size/2 - ((double)k - TABLE_SIZE/2)/TABLE_SIZE;
                if (value < 0)
                  value *= -1.0;
                else
                  value *= 1.0;
                double d = value * ratio;
                if (ppu_cfg->pp_filter == PP_LANCZOS) {
                  if (d==0) {
                    horizontal_weight[m + x_coeff_base] = 65535;
                  } else if (d > distance || d < -distance) {
                    horizontal_weight[m + x_coeff_base] = 0;
                  } else {
                    horizontal_weight[m + x_coeff_base]=
                    (i32)((distance*my_sin(PI*d)*my_sin(PI*d/distance)/(PI*PI*d*d))*65536);
                  }
                } else if (ppu_cfg->pp_filter == PP_BILINEAR) {
                  if (d >= 1)
                    horizontal_weight[m + x_coeff_base] = 0;
                  else
                    horizontal_weight[m + x_coeff_base] = (i32)((1 - d) * 65536);
                } else if (ppu_cfg->pp_filter == PP_BICUBIC) {
                  if (d >= 2)
                    horizontal_weight[m + x_coeff_base] = 0;
                  else if (d < 1)
                    horizontal_weight[m + x_coeff_base] = (i32)((3 * d * d * d - 5 * d * d + 2) / 2 * 65536);
                  else
                    horizontal_weight[m + x_coeff_base] = (i32)((5 * d * d - d * d * d - 8 * d + 4) / 2 * 65536);
                } else if (ppu_cfg->pp_filter == PP_SPLINE) {
                  double p = -2.196152422706632;
                  horizontal_weight[m + x_coeff_base] = (i32)(getSplineCoeff(1.0, 0.0, p, -p - 1.0, d) * 65536);
                  if (m == 0 || m == x_filter_size - 1)
                    horizontal_weight[m + x_coeff_base] = 0;
                } else if (ppu_cfg->pp_filter == PP_BOX) {
                  horizontal_weight[m + x_coeff_base] = (i32)(65536.0 / x_filter_size);
                } else {
                  APITRACEERR("%s", "InitPpUnitLanczosData# ERROR: Not Supported\n");
                }
              }
            }
            total = 0;
            for (m = 0; m < ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset; m++) {
              total += horizontal_weight[m + ppu_cfg->x_filter_offset/2];
            }
            /*interval selection phase ensure to maximum filter coefficients 256 and repeat them to 32 phases*/
            if (k % (1 << (4 - ppu_cfg->x_phase_num)) == 0) {
              i32 x_ratio[MAX_COEFF_SIZE_HOR] = {0};
              for (m = 0; m < ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset; m++) {
                i32 ratio = (i32) ((double)horizontal_weight[m + ppu_cfg->x_filter_offset/2] / total * 65536);
                u32 ratio_bac = (ratio >= 0) ? ratio : -ratio;
                if (ratio > 0)
                  x_ratio[m] = (((ratio_bac >> shift_bit) + ((ratio_bac & (least_coeff / 2)) ? 1 : 0)) << shift_bit);
                else
                  x_ratio[m] = -(i32)(((ratio_bac >> shift_bit) + ((ratio_bac & (least_coeff / 2)) ? 1 : 0)) << shift_bit);
              }
              total = 0;
              for (m = 0; m < ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset; m++) {
                total += x_ratio[m];
              }
              if (total < 65536) {
                for (m = 0; m < (65536 - total) / least_coeff; m++)
                  x_ratio[ppu_cfg->x_filter_size/2 - ppu_cfg->x_filter_offset/2 - (65536 - total)/(2 * least_coeff) + m] += least_coeff;
              } else {
                for (m = 0; m < (total - 65536) / least_coeff; m++)
                  x_ratio[ppu_cfg->x_filter_size/2 - ppu_cfg->x_filter_offset/2 - (total - 65536)/(2 * least_coeff) + m] -= least_coeff;
              }
              if (ppu_cfg->pp_filter != PP_NEAREST) {
                for (m = 0; m < ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset; m++) {
                  if (x_ratio[m] == 65536) {
                    x_ratio[m] -= least_coeff;
                    x_ratio[m - 1] += least_coeff;
                  }
                }
              }
              for (m = 0; m < ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset; m++) {
                if(hw_feature->scale_cost_support[i] && ppu_cfg->pp_filter != PP_NEAREST) {
                  if(k > 16) {
                    *(table_data-1) = x_ratio[m]  >> shift_bit;
                    table_data++;
                  } else {
                    *table_data++ = x_ratio[m]  >> shift_bit;
                  }
                } else
                  *table_data++ = x_ratio[m]  >> shift_bit;
              }
              for (m = 0; m < x_filter_stride - (ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset); m++) {
                *table_data++ = 0;
              }
            }
          }
        }
        /*calculate vertical scalar filter coefficient*/
        distance = ppu_cfg->y_filter_param;
        table_data = (i16*)ppu_cfg->lanczos_table.virtual_address + X_COEFF_OFFSET;
        if (ppu_cfg->scale.height != ppu_cfg->crop.height) {
          if (ppu_cfg->scale.height > ppu_cfg->crop.height)
            ratio = 1.0;
          else
            ratio = (double)ppu_cfg->scale.height / ppu_cfg->crop.height;
          for (k = 0; k < TABLE_SIZE; k++) {
            for (m = 0; m < y_filter_size; m++) {
              if (ppu_cfg->pp_filter == PP_NEAREST)
                vertical_weight[m + y_coeff_base] = 65535;
              else if (ppu_cfg->pp_filter == PP_VSI_LINEAR) {
                if (((y_filter_size / 2) % 2) == 1) {
                  if (m == (y_filter_size / 2 - (y_filter_size / 2 + 1) / 2) ||
                      m == (y_filter_size / 2 + (y_filter_size / 2 + 1) / 2)) {
                    vertical_weight[m + y_coeff_base] = (65536 - (i32)((double)ppu_cfg->scale.height / ppu_cfg->crop.height * 65536)  * (y_filter_size / 2)) / 2;
                  } else if ((m > (y_filter_size / 2 - (y_filter_size / 2 + 1) / 2)) &&
                            (m < (y_filter_size / 2 + (y_filter_size / 2 + 1) / 2))) {
                    vertical_weight[m + y_coeff_base] = (i32)((double)ppu_cfg->scale.height / ppu_cfg->crop.height * 65536);
                  } else {
                    vertical_weight[m + y_coeff_base] = 0;
                  }
                } else {
                  if (m >= y_filter_size / 4 && m < 3 * y_filter_size / 4)
                    vertical_weight[m + y_coeff_base] = (i32)((double)ppu_cfg->scale.height / ppu_cfg->crop.height * 65536);
                  else if (m == 3 * y_filter_size / 4)
                    vertical_weight[m + y_coeff_base] = 65536 - (i32)((double)ppu_cfg->scale.height / ppu_cfg->crop.height * 65536) * (y_filter_size / 2);
                  else
                    vertical_weight[m + y_coeff_base] = 0;
                }
              } else if (ppu_cfg->pp_filter == PP_FAST_LINEAR) {
                double value = (double)(m)-y_filter_size/2 - ((double)k - TABLE_SIZE/2)/TABLE_SIZE;
                if (value < 0)
                  value *= -1.0;
                else
                  value *= 1.0;
                double d = value;
                if (d >= 1)
                  vertical_weight[m + y_coeff_base] = 0;
                else
                  vertical_weight[m + y_coeff_base] = (i32)((1 - d) * 65536);
              } else if (ppu_cfg->pp_filter == PP_FAST_BICUBIC) {
                double value = (double)(m)-y_filter_size/2 - ((double)k - TABLE_SIZE/2)/TABLE_SIZE;
                if (value < 0)
                  value *= -1.0;
                else
                  value *= 1.0;
                double d = value;
                u8 bd = 2;
                if (hw_feature->scale_cost_support[i] == MIDDLE_SCALER) {
                  bd = 1;
                }
                if (d >= bd)
                  vertical_weight[m + y_coeff_base] = 0;
                else if (d < 1)
                  vertical_weight[m + y_coeff_base] = (i32)((1.25 * d * d * d - 2.25 * d * d + 1) * 65536);
                else
                  vertical_weight[m + y_coeff_base] = (i32)((-0.75 * d * d * d + 3.75 * d * d - 6 * d + 3) * 65536);
              } else {
                double value = (double)(m)-y_filter_size/2 - ((double)k - TABLE_SIZE/2)/TABLE_SIZE;
                if (value < 0)
                  value *= -1.0;
                else
                  value *= 1.0;
                double d = value * ratio;
                if (ppu_cfg->pp_filter == PP_LANCZOS) {
                  if (d==0) {
                    vertical_weight[m + y_coeff_base] = 65535;
                  } else if (d > distance || d < -distance) {
                    vertical_weight[m + y_coeff_base] = 0;
                  } else {
                    vertical_weight[m + y_coeff_base]=
                    (i32)((distance*my_sin(PI*d)*my_sin(PI*d/distance)/(PI*PI*d*d))*65536);
                  }
                } else if (ppu_cfg->pp_filter == PP_BILINEAR) {
                  if (d >= 1)
                    vertical_weight[m + y_coeff_base] = 0;
                  else
                    vertical_weight[m + y_coeff_base] = (i32)((1 - d) * 65536);
                } else if (ppu_cfg->pp_filter == PP_BICUBIC) {
                  if (d >= 2)
                    vertical_weight[m + y_coeff_base] = 0;
                  else if (d < 1)
                    vertical_weight[m + y_coeff_base] = (i32)((3 * d * d * d - 5 * d * d + 2) / 2 * 65536);
                  else
                    vertical_weight[m + y_coeff_base] = (i32)((5 * d * d - d * d * d - 8 * d + 4) / 2 * 65536);
                } else if (ppu_cfg->pp_filter == PP_SPLINE) {
                  double p = -2.196152422706632;
                  vertical_weight[m + y_coeff_base] = (i32)(getSplineCoeff(1.0, 0.0, p, -p - 1.0, d) * 65536);
                  if (m == 0 || m == y_filter_size - 1)
                    vertical_weight[m + y_coeff_base] = 0;
                } else if (ppu_cfg->pp_filter == PP_BOX) {
                  vertical_weight[m + y_coeff_base] = (i32)(65536.0 / y_filter_size);
                } else {
                  APITRACEERR("%s", "InitPpUnitLanczosData# ERROR: Not Supported\n");
                }
              }
            }
            total = 0;
            for (m = 0; m < ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset; m++) {
              total += vertical_weight[m + ppu_cfg->y_filter_offset/2];
            }
            /*interval selection phase ensure to maximum filter coefficients 256 and repeat them to 32 phases*/
            if (k % (1 << (4 - ppu_cfg->y_phase_num)) == 0) {
              i32 y_ratio[MAX_COEFF_SIZE_HOR] = {0};
              for (m = 0; m < ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset; m++) {
                i32 ratio = (i32) ((double)vertical_weight[m + ppu_cfg->y_filter_offset/2] / total * 65536);
                u32 ratio_bac = (ratio >= 0) ? ratio : -ratio;
                if (ratio > 0)
                  y_ratio[m] = (((ratio_bac >> shift_bit) + ((ratio_bac & (least_coeff / 2)) ? 1 : 0)) << shift_bit);
                else
                  y_ratio[m] = -(i32)(((ratio_bac >> shift_bit) + ((ratio_bac & (least_coeff / 2)) ? 1 : 0)) << shift_bit);
              }
              total = 0;
              for (m = 0; m < ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset; m++) {
                total += y_ratio[m];
              }
              if (total < 65536) {
                for (m = 0; m < (65536 - total) / least_coeff; m++)
                  y_ratio[ppu_cfg->y_filter_size/2 - ppu_cfg->y_filter_offset/2 - (65536 - total)/(2 * least_coeff) + m] += least_coeff;
              } else {
                for (m = 0; m < (total - 65536) / least_coeff; m++)
                  y_ratio[ppu_cfg->y_filter_size/2 - ppu_cfg->y_filter_offset/2 - (total - 65536)/(2 * least_coeff) + m] -= least_coeff;
              }
              if (ppu_cfg->pp_filter != PP_NEAREST) {
                for (m = 0; m < ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset; m++) {
                  if (y_ratio[m] == 65536) {
                    y_ratio[m] -= least_coeff;
                    y_ratio[m - 1] += least_coeff;
                  }
                }
              }
              for (m = 0; m < ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset; m++) {
                if(hw_feature->scale_cost_support[i] == MIDDLE_SCALER && ppu_cfg->pp_filter != PP_NEAREST) {
                  if(k > 16) {
                    *(table_data-1) = y_ratio[m]  >> shift_bit;
                    table_data++;
                  } else {
                    *table_data++ = y_ratio[m]  >> shift_bit;
                  }
                } else
                  *table_data++ = y_ratio[m]  >> shift_bit;
              }
              for (m = 0; m < y_filter_stride - (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset); m++) {
                *table_data++ = 0;
              }
            }
          }
        }
      }
      /*calculate horizontal downscale frame edge coeffcient, no need calculate upscale edge coefficient, upscale filter size is always no more than 5, HW will calculate directly */
      u8 bound_count = 2;
      table_data = (i16*)ppu_cfg->lanczos_table.virtual_address;
      scale_ratio_x_inv = FDIVI(TOFIX(ppu_cfg->crop.width, 16) + ppu_cfg->scale.width / 2, ppu_cfg->scale.width);
      scale_ratio_y_inv = FDIVI(TOFIX(ppu_cfg->crop.height, 16) + ppu_cfg->scale.height / 2, ppu_cfg->scale.height);
      if(hw_feature->scale_cost_support[i] == ULTRA_SCALER) {
        table_data = (i16*)ppu_cfg->lanczos_table.virtual_address + 32*32*2;
        bound_count = 3;
      } else {
        table_data = (i16*)ppu_cfg->lanczos_table.virtual_address + X_COEFF_OFFSET + Y_COEFF_OFFSET;
        if (scale_ratio_x_inv % 2) scale_ratio_x_inv++;
        if (scale_ratio_y_inv % 2) scale_ratio_y_inv++;
      }
      crop_w_ch = sub_x == 2 ? (ppu_cfg->crop.width + 1) / sub_x : ppu_cfg->crop.width / sub_x;
      crop_h_ch = sub_y == 2 ? (ppu_cfg->crop.height + 1) / sub_y : ppu_cfg->crop.height / sub_y;
      scale_w_ch = sub_x == 2 ? (ppu_cfg->scale.width + 1) / sub_x : ppu_cfg->scale.width / sub_x;
      scale_h_ch = sub_y == 2 ? (ppu_cfg->scale.height + 1) / sub_y : ppu_cfg->scale.height / sub_y;
      scale_ratio_x_inv_ch = FDIVI(TOFIX(crop_w_ch, 16) + scale_w_ch / 2, scale_w_ch);
      scale_ratio_y_inv_ch = FDIVI(TOFIX(crop_h_ch, 16) + scale_h_ch / 2, scale_h_ch);
      if(hw_feature->scale_cost_support[i] == ULTRA_SCALER) {
        xDstInSrc = scale_ratio_x_inv - 65536;
      } else {
        if (scale_ratio_x_inv_ch % 2) scale_ratio_x_inv_ch++;
        if (scale_ratio_y_inv_ch % 2) scale_ratio_y_inv_ch++;
        /*luma: calculate edge coeff at frame left and right edge*/
        if (ppu_cfg->src_sel_mode == DOWN_ROUND) {
          xDstInSrc = scale_ratio_x_inv - 65536;
        } else if (ppu_cfg->src_sel_mode == NO_ROUND) {
          xDstInSrc = 0;
        } else {
          xDstInSrc = scale_ratio_x_inv;
        }
      }
      pos = xDstInSrc / 2 - scale_ratio_x_inv;
      x_pos = 0;
      start_count = 0;
      end_count = 0;
      i16 start_value[3]={0,0,0};
      i16 end_value[3]={0,0,0};
      u32 phase_repet_num = (1 << (4 - ppu_cfg->x_phase_num));
      if (ppu_cfg->crop.width > ppu_cfg->scale.width) {
        for (j = 0; j < ppu_cfg->scale.width; j++) {
          pos += scale_ratio_x_inv;
          x_pos = FLOOR(pos+32768);
          start_pos = x_pos - (ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset)/2;
          end_pos = x_pos + (ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset)/2;
          if(hw_feature->scale_cost_support[i] == ULTRA_SCALER)
            table_index = ((pos+32768 - MAKE_FIXED_UNI(x_pos)) >> (FIXED_BITS_UNI-TableWLength));
          else
            table_index = ((pos+32768 - MAKE_FIXED_UNI(x_pos)) >> (FIXED_BITS_UNI-TABLE_LENGTH));
          value = 0;
          if (start_pos < 0) {
            for (k = start_pos; k <= 0; k++) {
              CalOutTableIndex(table_index, k-start_pos, (ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset), &out_index, &coeff_index);
              if(hw_feature->scale_cost_support[i] == ULTRA_SCALER)
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + out_index * x_filter_stride + coeff_index);
              else
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + out_index / phase_repet_num * x_filter_stride + coeff_index);
            }
            start_value[start_count++] = value;
          } /*else {
            if (start_count < 2) {
              CalOutTableIndex(table_index, 0, (ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset), &out_index, &coeff_index);
              value += *((i16*)ppu_cfg->lanczos_table.virtual_address + out_index / phase_repet_num * x_filter_stride + coeff_index);
              start_value[start_count++] = value;
            }
          }*/
          value = 0;
          if (end_pos >= ppu_cfg->crop.width) {
            for (k = ppu_cfg->crop.width - 1; k <= end_pos; k++) {
              if(start_pos == end_pos && k == end_pos)
                continue;
              CalOutTableIndex(table_index, k - start_pos > 0 ? k - start_pos : 0, (ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset), &out_index, &coeff_index);
              if(hw_feature->scale_cost_support[i] == ULTRA_SCALER)
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + out_index  * x_filter_stride + coeff_index);
              else
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + out_index / phase_repet_num * x_filter_stride + coeff_index);
            }
            end_value[end_count++] = value;
          }
        }
        for (j = 0; j < bound_count; j++) {
          *table_data++ = start_value[j];
          #if ULTRA_SCALE_DEBUG
            printf("luma horital j: %u, start_value: %d\n", j, start_value[j]);
          #endif
          start_value[j] = 0;
        }
        for (j = 0; j < bound_count; j++) {
          *table_data++ = end_value[j];
          #if ULTRA_SCALE_DEBUG
            printf("luma hortical j: %u, end_value: %d\n", j, end_value[j]);
          #endif
          end_value[j] = 0;
        }
      }
      /*chroma:
      before: just only calculate edge coeff at frame right edge, left edge coeff is identical with luma
      now: calculate edge coeff at frame left and right edge**/
      if(hw_feature->scale_cost_support[i] == ULTRA_SCALER) {
        xDstInSrc = scale_ratio_x_inv_ch - 65536;
        table_data = (i16*)ppu_cfg->lanczos_table.virtual_address + 32*32*2 + 8;
      } else {
        if (ppu_cfg->src_sel_mode == DOWN_ROUND) {
          xDstInSrc = scale_ratio_x_inv_ch - 65536;
        } else if (ppu_cfg->src_sel_mode == NO_ROUND) {
          xDstInSrc = 0;
        } else {
          xDstInSrc = scale_ratio_x_inv_ch;
        }
      }
      pos = xDstInSrc / 2 - scale_ratio_x_inv_ch;
      x_pos = 0;
      start_count = 0;
      end_count = 0;
      i16 chroma_start_value[3]={0,0,0};
      i16 chroma_end_value[3]={0,0,0};
      /*support odd crop*/
      //if (ppu_cfg->crop.width > ppu_cfg->scale.width) {
      //  for (j = 0; j < ppu_cfg->scale.width / 2; j++) {
      if (ppu_cfg->crop.width > ppu_cfg->scale.width) {
        for (j = 0; j < (sub_x == 2 ? (ppu_cfg->scale.width + 1) / 2 : ppu_cfg->scale.width); j++) {
          pos += scale_ratio_x_inv_ch;
          x_pos = FLOOR(pos+32768);
          start_pos = x_pos - (ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset)/2;
          end_pos = x_pos + (ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset)/2;
          if(hw_feature->scale_cost_support[i] == ULTRA_SCALER)
            table_index = ((pos+32768 - MAKE_FIXED_UNI(x_pos)) >> (FIXED_BITS_UNI-TableWLength));
          else
            table_index = ((pos+32768 - MAKE_FIXED_UNI(x_pos)) >> (FIXED_BITS_UNI-TABLE_LENGTH));
          value = 0;
          //if (!hw_feature->crop_step_rshift) {
            if (start_pos < 0) {
              for (k = start_pos; k <= 0; k++) {
                CalOutTableIndex(table_index, k-start_pos, (ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset), &out_index, &coeff_index);
                if(hw_feature->scale_cost_support[i] == ULTRA_SCALER)
                  value += *((i16*)ppu_cfg->lanczos_table.virtual_address + out_index * x_filter_stride + coeff_index);
                else
                  value += *((i16*)ppu_cfg->lanczos_table.virtual_address + out_index / phase_repet_num * x_filter_stride + coeff_index);
              }
              chroma_start_value[start_count++] = value;
            }/* else {
              if (start_count < 2) {
                CalOutTableIndex(table_index, 0, (ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset), &out_index, &coeff_index);
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + out_index / phase_repet_num * x_filter_stride + coeff_index);
                chroma_start_value[start_count++] = value;
              }
            }*/
          //}
          value = 0;
          /*support odd crop*/
          //if (end_pos >= ppu_cfg->crop.width / 2) {
          //  for (k = ppu_cfg->crop.width / 2 - 1; k <= end_pos; k++) {
          if (end_pos >= (sub_x == 2 ? (ppu_cfg->crop.width + 1) / 2 :  ppu_cfg->crop.width) && end_pos != start_pos) {
            for (k = (sub_x == 2 ? (ppu_cfg->crop.width + 1) / 2 : ppu_cfg->crop.width) - 1; k <= end_pos; k++) {
              if ( start_pos == end_pos && k == end_pos)
                continue;
              //CalOutTableIndex(table_index, k-start_pos, (ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset), &out_index, &coeff_index);
              CalOutTableIndex(table_index, k - start_pos > 0 ? k - start_pos : 0, (ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset), &out_index, &coeff_index);
              if(hw_feature->scale_cost_support[i] == ULTRA_SCALER)
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + out_index  * x_filter_stride + coeff_index);
              else
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + out_index / phase_repet_num * x_filter_stride + coeff_index);

            }
            chroma_end_value[end_count++] = value;
          }
        }
        //if (!hw_feature->crop_step_rshift) {
          for (j = 0; j < bound_count; j++) {
            *table_data++ = chroma_start_value[j];
            #if ULTRA_SCALE_DEBUG
              printf("chroma hortical j: %u, start_value: %d\n", j, chroma_start_value[j]);
            #endif
            chroma_start_value[j] = 0;
          }
        //}
        for (j = 0; j < bound_count; j++) {
          *table_data++ = chroma_end_value[j];
          #if ULTRA_SCALE_DEBUG
            printf("chroma hortical j: %u, end_value: %d\n", j, chroma_end_value[j]);
          #endif
          chroma_end_value[j] = 0;
        }
      }
      /*calculate vertical downscale edge coefficient, no need calculate upscale edge coefficient, upscale filter size is always no more than 5, HW will calculate directly */
      /*luma: calcuate edge coeff at frame top and bottom edge*/
      if(hw_feature->scale_cost_support[i] == ULTRA_SCALER) {
        table_data = (i16*)ppu_cfg->lanczos_table.virtual_address + 32*32*2 + 16;
        yDstInSrc = scale_ratio_y_inv - 65536;
      } else {
        table_data = (i16*)ppu_cfg->lanczos_table.virtual_address + X_COEFF_OFFSET + Y_COEFF_OFFSET + 16;
        if (ppu_cfg->src_sel_mode == DOWN_ROUND) {
          yDstInSrc = scale_ratio_y_inv - 65536;
        } else if (ppu_cfg->src_sel_mode == NO_ROUND) {
          yDstInSrc = 0;
        } else {
          yDstInSrc = scale_ratio_y_inv;
        }
      }
      pos = yDstInSrc / 2 - scale_ratio_y_inv;
      y_pos = 0;
      start_count = 0;
      end_count = 0;
      phase_repet_num = (1 << (4 - ppu_cfg->y_phase_num));
      if (ppu_cfg->scale.height < ppu_cfg->crop.height) {
        for (j = 0; j<ppu_cfg->scale.height; j++) {
          pos += scale_ratio_y_inv;
          y_pos = FLOOR(pos+32768);
          start_pos = y_pos - (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset)/2;
          end_pos = y_pos + (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset)/2;
          if(hw_feature->scale_cost_support[i] == ULTRA_SCALER)
            table_index = ((pos+32768 - MAKE_FIXED_UNI(y_pos)) >> (FIXED_BITS_UNI-TableHLength));
          else
            table_index = ((pos+32768 - MAKE_FIXED_UNI(y_pos)) >> (FIXED_BITS_UNI-TABLE_LENGTH));
          value = 0;
          if (start_pos < 0) {
            for (k = start_pos; k <= 0; k++) {
              CalOutTableIndex(table_index, k-start_pos, (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset), &out_index, &coeff_index);
              if(hw_feature->scale_cost_support[i] == ULTRA_SCALER)
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + 32 * 32 + out_index * y_filter_stride + coeff_index);
              else
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + X_COEFF_OFFSET + out_index / phase_repet_num * y_filter_stride + coeff_index);
            }
            start_value[start_count++] = value;
          }/* else {
            if (start_count < 2) {
              CalOutTableIndex(table_index, 0, (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset), &out_index, &coeff_index);
              value = *((i16*)ppu_cfg->lanczos_table.virtual_address + X_COEFF_OFFSET + out_index / phase_repet_num * y_filter_stride + coeff_index);
              start_value[start_count++] = value;
            }
          }*/
          value = 0;
          if (end_pos >= ppu_cfg->crop.height) {
            for (k = ppu_cfg->crop.height - 1; k <= end_pos; k++) {
              if (start_pos == end_pos && k == end_pos)
                continue;
              //CalOutTableIndex(table_index, k-start_pos, (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset), &out_index, &coeff_index);
              CalOutTableIndex(table_index, k - start_pos > 0 ? k -start_pos : 0, (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset), &out_index, &coeff_index);
              if(hw_feature->scale_cost_support[i] == ULTRA_SCALER)
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + 32 * 32 + out_index  * y_filter_stride + coeff_index);
              else
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + X_COEFF_OFFSET + out_index / phase_repet_num * y_filter_stride + coeff_index);
            }
            end_value[end_count++] = value;
          }
        }
        for (j = 0; j < bound_count; j++) {
          *table_data++ = start_value[j];
          #if ULTRA_SCALE_DEBUG
            printf("luma vertical j: %u, start_value: %d\n", j, start_value[j]);
          #endif
        }
        for (j = 0; j < bound_count; j++) {
          *table_data++ = end_value[j];
          #if ULTRA_SCALE_DEBUG
            printf("luma vertical j: %u, end_value: %d\n", j, end_value[j]);
          #endif
        }
      }
      /*chroma:
        before: just only calculate at frame bottom edge, top edge coeff is identical with luma
        now: calculate edge coeff at frame top and bottom edge*/

      if(hw_feature->scale_cost_support[i] == ULTRA_SCALER) {
        table_data = (i16*)ppu_cfg->lanczos_table.virtual_address + 32*32*2 + 24;
        yDstInSrc = scale_ratio_y_inv_ch - 65536;
      } else{
        if (ppu_cfg->src_sel_mode == DOWN_ROUND) {
          yDstInSrc = scale_ratio_y_inv_ch - 65536;
        } else if (ppu_cfg->src_sel_mode == NO_ROUND) {
          yDstInSrc = 0;
        } else {
          yDstInSrc = scale_ratio_y_inv_ch;
        }
      }
      pos = yDstInSrc / 2 - scale_ratio_y_inv_ch;
      y_pos = 0;
      start_count = 0;
      end_count = 0;
      /*support odd crop*/
      //if (ppu_cfg->scale.height < ppu_cfg->crop.height) {
      //  for (j = 0; j<ppu_cfg->scale.height / 2; j++) {
      if (ppu_cfg->scale.height < ppu_cfg->crop.height) {
        for (j = 0; j < (sub_y == 2 ? (ppu_cfg->scale.height + 1) / 2 : ppu_cfg->scale.height); j++) {
          pos += scale_ratio_y_inv_ch;
          y_pos = FLOOR(pos+32768);
          start_pos = y_pos - (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset)/2;
          end_pos = y_pos + (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset)/2;
          if(hw_feature->scale_cost_support[i] == ULTRA_SCALER)
            table_index = ((pos+32768 - MAKE_FIXED_UNI(y_pos)) >> (FIXED_BITS_UNI-TableHLength));
          else
            table_index = ((pos+32768 - MAKE_FIXED_UNI(y_pos)) >> (FIXED_BITS_UNI-TABLE_LENGTH));
          value = 0;
          //if (!hw_feature->crop_step_rshift) {
            if (start_pos < 0) {
              for (k = start_pos; k <= 0; k++) {
                CalOutTableIndex(table_index, k-start_pos, (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset), &out_index, &coeff_index);
                if(hw_feature->scale_cost_support[i] == ULTRA_SCALER)
                  value += *((i16*)ppu_cfg->lanczos_table.virtual_address + 32 * 32 + out_index * y_filter_stride + coeff_index);
                else
                  value += *((i16*)ppu_cfg->lanczos_table.virtual_address + X_COEFF_OFFSET + out_index / phase_repet_num * y_filter_stride + coeff_index);
              }
              chroma_start_value[start_count++] = value;
            }/* else {
              if (start_count < 2) {
                CalOutTableIndex(table_index, 0, (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset), &out_index, &coeff_index);
                value = *((i16*)ppu_cfg->lanczos_table.virtual_address + X_COEFF_OFFSET + out_index / phase_repet_num * y_filter_stride + coeff_index);
                chroma_start_value[start_count++] = value;
              }
            }*/
          //}
          value = 0;
          /*support odd crop*/
          //if (end_pos >= ppu_cfg->crop.height / 2) {
          //  for (k = ppu_cfg->crop.height / 2 - 1; k <= end_pos; k++) {
          if (end_pos >= (sub_y == 2 ? (ppu_cfg->crop.height + 1) / 2 :  ppu_cfg->crop.height) && end_pos != start_pos) {
            for (k = (sub_y == 2 ? (ppu_cfg->crop.height + 1) / 2 : ppu_cfg->crop.height) - 1; k <= end_pos; k++) {
              if (start_pos == end_pos && k == end_pos)
                continue;
              //CalOutTableIndex(table_index, k-start_pos, (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset), &out_index, &coeff_index);
              CalOutTableIndex(table_index, k - start_pos > 0 ? k - start_pos : 0, (ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset), &out_index, &coeff_index);
              if(hw_feature->scale_cost_support[i] == ULTRA_SCALER)
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + 32 * 32 + out_index  * y_filter_stride + coeff_index);
              else
                value += *((i16*)ppu_cfg->lanczos_table.virtual_address + X_COEFF_OFFSET + out_index / phase_repet_num * y_filter_stride + coeff_index);
            }
            chroma_end_value[end_count++] = value;
          }
        }
        //if (!hw_feature->crop_step_rshift) {
          for (j = 0; j < bound_count; j++) {
            *table_data++ = chroma_start_value[j];
            #if ULTRA_SCALE_DEBUG
              printf("chroma vertical j: %u, start_value: %d\n", j, chroma_start_value[j]);
            #endif
          }
        //}
        for (j = 0; j < bound_count; j++) {
          *table_data++ = chroma_end_value[j];
          #if ULTRA_SCALE_DEBUG
            printf("chroma vertical j: %u, end_value: %d\n", j, chroma_end_value[j]);
          #endif
        }
      }
      if(hw_feature->id == 0x2070) {
        u32 src_len = 32 * 8 * 2;
        u32 group = 8;
        u32 keep  = 4;
        u32 dst_len = src_len / group * keep;
        i16 *ptr = (i16*)ppu_cfg->lanczos_table.virtual_address;
        u32 dst_idx = 0;
        for (u32 src_idx = 0; src_idx < src_len; src_idx += group) {
          if (dst_idx != src_idx) {
              memmove(ptr + dst_idx, ptr + src_idx, keep * sizeof(i16));
          }
          dst_idx += keep;
        }
        memset(ptr + dst_len, 0, (src_len - dst_len) * sizeof(i16));
      }
    }
    u32 n_cores = ppu_cfg->lanczos_table.size / LANCZOS_COEFF_BUFFER_SIZE / sizeof(i16);
    ASSERT(ppu_cfg->dwl);
    for (j = 0; j < n_cores; j++) {
      DWLSyncMemData(ppu_cfg->dwl, &ppu_cfg->lanczos_table, j * LANCZOS_COEFF_BUFFER_SIZE * sizeof(i16),
                      LANCZOS_COEFF_BUFFER_SIZE * sizeof(i16), HOST_TO_DEVICE);
                      /* 512KB + tile_edge_size */
    }
  }
}

#if 0
void InitFbcParams(PpUnitIntConfig *ppu_cfg, u32 db_filter, addr_t bus_address, u32 all_ppu) {
  u32 i;
  u32 ratio_w, ratio_h;
  u32 begin_offset = 0, hdr_size = 0;
  for (i = 0; i < (all_ppu ? DEC_MAX_PPU_COUNT : 1); i++, ppu_cfg++) {
    if (!ppu_cfg->enabled) continue;
    ppu_cfg->bus_address = bus_address;
    if (ppu_cfg->tiled_e && ppu_cfg->tile_mode) {
      ratio_w = ppu_cfg->crop.width / ppu_cfg->scale.width;
      ratio_h = ppu_cfg->crop.height / ppu_cfg->scale.height;
      ppu_cfg->vir_left = (ppu_cfg->crop.x / ratio_w) % 16;
      if (((ppu_cfg->vir_left + ppu_cfg->scale.width) % 16))
        ppu_cfg->vir_right = 16 - ((ppu_cfg->vir_left + ppu_cfg->scale.width) % 16);
      else
        ppu_cfg->vir_right = 0;
      if (db_filter) {
        if (ppu_cfg->crop.y <= 56) {
          if (((56 - ppu_cfg->crop.y) / ratio_h) % 16)
            ppu_cfg->vir_top = 16 - (((56 - ppu_cfg->crop.y) / ratio_h) % 16);
          else
            ppu_cfg->vir_top = 0;
          if (((ppu_cfg->vir_top + ppu_cfg->scale.height) % 16))
            ppu_cfg->vir_bottom = 16 - ((ppu_cfg->vir_top + ppu_cfg->scale.height) % 16);
          else
            ppu_cfg->vir_bottom = 0;
        } else {
          if (((64 - ((ppu_cfg->crop.y - 56) % 64)) / ratio_h) % 16)
            ppu_cfg->vir_top = 16 - (((64 - ((ppu_cfg->crop.y - 56) % 64)) / ratio_h) % 16);
          else
            ppu_cfg->vir_top = 0;
          if (((ppu_cfg->vir_top + ppu_cfg->scale.height) % 16))
            ppu_cfg->vir_bottom = 16 - ((ppu_cfg->vir_top + ppu_cfg->scale.height) % 16);
          else
            ppu_cfg->vir_bottom = 0;
        }
      } else {
        if (((64 - (ppu_cfg->crop.y % 64)) / ratio_h) % 16)
          ppu_cfg->vir_top = 16 - (((64 - (ppu_cfg->crop.y % 64)) / ratio_h) % 16);
        else
          ppu_cfg->vir_top = 0;
        if (((ppu_cfg->vir_top + ppu_cfg->scale.height) % 16))
          ppu_cfg->vir_bottom = 16 - ((ppu_cfg->vir_top + ppu_cfg->scale.height) % 16);
        else
          ppu_cfg->vir_bottom = 0;
      }
      if (ppu_cfg->tile_mode == TILED16x16) {
        begin_offset = ppu_cfg->hdr_offset;
#ifdef ASIC_TRACE_SUPPORT
        hdr_size = 0;
#else
        hdr_size = NEXT_MULTIPLE((ppu_cfg->vir_left + ppu_cfg->scale.width + ppu_cfg->vir_right) * (ppu_cfg->vir_top + ppu_cfg->scale.height + ppu_cfg->vir_bottom) / 16, 64);
#endif
        ppu_cfg->luma_offset = begin_offset + hdr_size;
        ppu_cfg->chroma_offset = ppu_cfg->luma_offset + NEXT_MULTIPLE((ppu_cfg->vir_left + ppu_cfg->scale.width + ppu_cfg->vir_right) *
                                 (ppu_cfg->vir_top + ppu_cfg->scale.height + ppu_cfg->vir_bottom) * (ppu_cfg->stream_pixel_width == 10 ? 2 : 1),
                                  ALIGN(ppu_cfg->align));
      }
    }
  }
}
#endif

void calSecondUpScaleRatio(const struct DecHwFeatures *hw_feature, PpUnitIntConfig *ppu_cfg, u32 interlace) {
  u32 i = 0;
  u32 found = 0;
  if (!hw_feature->pp_support_6x) {
    if (ppu_cfg->scale.width > 3 * ppu_cfg->crop.width) {
      for (i = 3 * ppu_cfg->crop.width; i > ppu_cfg->crop.width; i-=2) {
        if ((ppu_cfg->scale.width % i) == 0) {
          ppu_cfg->out_width = i;
          ppu_cfg->out_ratio_x = ppu_cfg->scale.width / i;
          found = 1;
          break;
        }
      }

      if (!found) {
        for (i = 3 * ppu_cfg->crop.width; i > ppu_cfg->crop.width; i-=2) {
          if ((ppu_cfg->scale.width / i) != (ppu_cfg->scale.width / (i - 2))) {
            ppu_cfg->out_width = i;
            ppu_cfg->out_ratio_x = ppu_cfg->scale.width / (i - 2);
            break;
          }
        }
      }
    } else {
      ppu_cfg->out_width = ppu_cfg->scale.width;
      ppu_cfg->out_ratio_x = 1;
    }
    found = 0;
    if (ppu_cfg->scale.height > 3 * ppu_cfg->crop.height) {
      for (i = 3 * ppu_cfg->crop.height; i > ppu_cfg->crop.height; i-=(interlace ? 4 : 2)) {
        if ((ppu_cfg->scale.height % i) == 0) {
          ppu_cfg->out_height = i;
          ppu_cfg->out_ratio_y = ppu_cfg->scale.height / i;
          found = 1;
          break;
        }
      }
      if (!found) {
        for (i = 3 * ppu_cfg->crop.height; i > ppu_cfg->crop.height; i-=(interlace ? 4 : 2)) {
          if ((ppu_cfg->scale.height / i) != (ppu_cfg->scale.height / (i - 2))) {
            ppu_cfg->out_height = i;
            ppu_cfg->out_ratio_y = ppu_cfg->scale.height / (i - (interlace ? 4 : 2));
            break;
          }
        }
      }
    } else {
      ppu_cfg->out_height =  ppu_cfg->scale.height;
      ppu_cfg->out_ratio_y = 1;
    }
  } else {
    ppu_cfg->out_width = ppu_cfg->scale.width;
    ppu_cfg->out_ratio_x = 1;
    ppu_cfg->out_height =  ppu_cfg->scale.height;
    ppu_cfg->out_ratio_y = 1;
  }
}
/* Print Out format */

#ifdef VCD_LOGMSG
char *PrintOutFormat(u16 i){
  switch(i){
    case PP_OUT_FMT_YUV420PACKED10: return "PP_OUT_FMT_YUV420PACKED10"; break;
    case PP_OUT_FMT_YUV420_P010: return "PP_OUT_FMT_YUV420_P010"; break;
    case PP_OUT_FMT_YUV420_BIGE: return "PP_OUT_FMT_YUV420_BIGE"; break;
    case PP_OUT_FMT_YUV420_8BIT: return "PP_OUT_FMT_YUV420_8BIT"; break;
    case PP_OUT_FMT_YUV400: return "PP_OUT_FMT_YUV400"; break;
    case PP_OUT_FMT_YUV400_P010: return "PP_OUT_FMT_YUV400_P010"; break;
    case PP_OUT_FMT_YUV400_8BIT: return "PP_OUT_FMT_YUV400_8BIT"; break;
    case PP_OUT_FMT_IYUVPACKED10_420: return "PP_OUT_FMT_IYUVPACKED10_420"; break;
    case PP_OUT_FMT_IYUV_420_P010: return "PP_OUT_FMT_IYUV_420_P010"; break;
    case PP_OUT_FMT_IYUV_420_8BIT: return "PP_OUT_FMT_IYUV_420_8BIT"; break;
    case PP_OUT_FMT_YUV420_10: return "PP_OUT_FMT_YUV420_10"; break;
    case PP_OUT_FMT_RGB: return "PP_OUT_FMT_RGB"; break;
    case PP_OUT_FMT_YUV422_10: return "PP_OUT_FMT_YUV422_10"; break;
    case PP_OUT_FMT_YUV422_8BIT: return "PP_OUT_FMT_YUV422_8BIT"; break;
    case PP_OUT_FMT_YUV422_P010: return " PP_OUT_FMT_YUV422_P010"; break;
    case PP_OUT_FMT_YUV422_P012: return "PP_OUT_FMT_YUV422_P012"; break;
    case PP_OUT_FMT_IYUV_422_8BIT: return "PP_OUT_FMT_IYUV_422_8BIT"; break;
    case PP_OUT_FMT_IYUV_422_P010: return "PP_OUT_FMT_IYUV_422_P010"; break;
    case PP_OUT_FMT_IYUV_422_P012: return "PP_OUT_FMT_IYUV_422_P012"; break;
    case PP_OUT_FMT_YUV422_YUYV_8BIT: return "PP_OUT_FMT_YUV422_YUYV_8BIT"; break;
    case PP_OUT_FMT_YUV422_UYVY_8BIT: return "PP_OUT_FMT_YUV422_UYVY_8BIT"; break;
    case PP_OUT_FMT_YUV422_YUYV_P010: return "PP_OUT_FMT_YUV422_YUYV_P010"; break;
    case PP_OUT_FMT_YUV422_UYVY_P010: return "PP_OUT_FMT_YUV422_UYVY_P010"; break;
    case PP_OUT_FMT_YUV400_P012: return "PP_OUT_FMT_YUV400_P012"; break;
    case PP_OUT_FMT_YUV420_P012: return "PP_OUT_FMT_YUV420_P012"; break;
    case PP_OUT_FMT_IYUV_420_P012: return "PP_OUT_FMT_IYUV_420_P012"; break;
    case PP_OUT_FMT_IYUV_444_8BIT: return "PP_OUT_FMT_IYUV_444_8BIT"; break;
    case PP_OUT_FMT_IYUV_444_P010: return "PP_OUT_FMT_IYUV_444_P010"; break;
    case PP_OUT_FMT_IYUV_444_P012: return "PP_OUT_FMT_IYUV_444_P012"; break;
    case PP_OUT_FMT_YUV400_WITH_ALPHA: return "PP_OUT_FMT_YUV400_WITH_ALPHA"; break;
    default: return "Illegal OutFormat";
  }

}

/* Print RGB format */
char *PrintRGBFormat(u16 i) {
  return "1";
}

/* Print ppu_cfg */
int num_pp = 0, num_pp_bitmap;
int num_pp_flag = 0;
void PrintPpunitConfig(PpUnitIntConfig *ppu_cfg) {
  u32 i;
  for (i = 0; i < DEC_MAX_PPU_COUNT; i++, ppu_cfg++){
    if (!ppu_cfg->enabled) continue;
    num_pp = i+1;
    num_pp_bitmap |= (1 << i);
    num_pp_flag = 1;
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Enabled: %d\n",i, ppu_cfg->enabled);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Tiled_e: %d\n",i, ppu_cfg->tiled_e);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# RGB: %d\n",i, ppu_cfg->rgb);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# RGB Planar: %d\n",i, ppu_cfg->rgb_planar);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Cr first: %d\n",i, ppu_cfg->cr_first);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Luma offset: 0x%x, Luma size: %d\n",i, ppu_cfg->luma_offset, ppu_cfg->luma_size);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Chroma offset: 0x%x, Chroma size: %d\n",i, ppu_cfg->chroma_offset, ppu_cfg->chroma_size);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Hdr offset: 0x%x\n",i, ppu_cfg->hdr_offset);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Pixel Width: %d\n",i, ppu_cfg->pixel_width);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Stream Pixel Width: %d\n",i, ppu_cfg->stream_pixel_width);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Shaper Enabled: %d\n",i, ppu_cfg->shaper_enabled);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# At Frame Right Edge Shaper Pad Disabled: %d\n",i, ppu_cfg->shaper_no_pad);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# At Stream Tile Right Edge Shaper Pad Enabled: %d\n",i, ppu_cfg->shaper_tile_edge_pad);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Dec400 Enabled: %d\n",i, ppu_cfg->dec400_enabled);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Planar: %d\n",i, ppu_cfg->planar);
    if (ppu_cfg->align == DEC_ALIGN_1B)
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Align: 1B\n",i);
    if (ppu_cfg->align == DEC_ALIGN_8B)
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Align: 8B\n",i);
    if (ppu_cfg->align == DEC_ALIGN_16B)
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Align: 16B\n",i);
    if (ppu_cfg->align == DEC_ALIGN_32B)
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Align: 32B\n",i);
    if (ppu_cfg->align == DEC_ALIGN_64B)
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Align: 64B\n",i);
    if (ppu_cfg->align == DEC_ALIGN_128B)
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Align: 128B\n",i);
    if (ppu_cfg->align == DEC_ALIGN_256B)
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Align: 256B\n",i);
    if (ppu_cfg->align == DEC_ALIGN_512B)
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Align: 512B\n",i);
    if (ppu_cfg->align == DEC_ALIGN_1024B)
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Align: 1024B\n",i);
    if (ppu_cfg->align == DEC_ALIGN_2048B)
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Align: 2048B\n",i);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Tile coded image: %d\n",i, ppu_cfg->tile_coded_image);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Ystride: %d\n",i, ppu_cfg->ystride);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Cstride: %d\n",i, ppu_cfg->cstride);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# False_ystride: %d\n",i, ppu_cfg->false_ystride);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# False_cstride: %d\n",i, ppu_cfg->false_cstride);
    if (ppu_cfg->crop.enabled){
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Crop# Enabled: %d\n",i, ppu_cfg->crop.enabled);
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Crop# Setbyuser: %d\n",i, ppu_cfg->crop.set_by_user);
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Crop# x: %d, y: %d, width: %d, height: %d\n",i, ppu_cfg->crop.x, ppu_cfg->crop.y, ppu_cfg->crop.width, ppu_cfg->crop.height);
    }

    if (ppu_cfg->crop2.enabled){
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Crop2# Enabled: %d\n",i, ppu_cfg->crop2.enabled);
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Crop2# x: %d, y: %d, width: %d, height: %d\n",i, ppu_cfg->crop2.x, ppu_cfg->crop2.y, ppu_cfg->crop2.width, ppu_cfg->crop2.height);
    }

    if (ppu_cfg->scale.enabled) {
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Scale# Enabled: %d\n",i, ppu_cfg->scale.enabled);
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Scale# Scale by ratio# x: %d\n",i, ppu_cfg->scale.scale_by_ratio);
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Scale# Ratio x: %d\n",i, ppu_cfg->scale.ratio_x);
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Scale# Ratio y: %d\n",i, ppu_cfg->scale.ratio_y);
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Scale# Width: %d, Height: %d\n",i, ppu_cfg->scale.width, ppu_cfg->scale.height);
    }

    APITRACEDEBUG("PpUnitCfg PPU[%d]# Monochrome: %d\n",i, ppu_cfg->monochrome);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Out format: %s\n",i, PrintOutFormat(ppu_cfg->out_format));
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Out P010: %d\n",i, ppu_cfg->out_p010);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Out 1010: %d\n",i, ppu_cfg->out_1010);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Out I010: %d\n",i, ppu_cfg->out_I010);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Out L010: %d\n",i, ppu_cfg->out_L010);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Out P012: %d\n",i, ppu_cfg->out_p012);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Out I012: %d\n",i, ppu_cfg->out_I012);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Out cut 8bits: %d\n",i, ppu_cfg->out_cut_8bits);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Video Range: %d\n",i, ppu_cfg->video_range);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Range max: %d\n",i, ppu_cfg->video_range);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# RGB format: %s\n",i, PrintRGBFormat(ppu_cfg->rgb_format));
    APITRACEDEBUG("PpUnitCfg PPU[%d]# RGB format: %s\n",i, PrintRGBFormat(ppu_cfg->rgb_format));
    if (ppu_cfg->rgb_stan == BT601)
      APITRACEDEBUG("PpUnitCfg PPU[%d]# RGB conversion standard: %s\n",i, "BT601");
    if (ppu_cfg->rgb_stan == BT601_L)
      APITRACEDEBUG("PpUnitCfg PPU[%d]# RGB conversion standard: %s\n",i, "BT601_L");
    if (ppu_cfg->rgb_stan == BT709)
      APITRACEDEBUG("PpUnitCfg PPU[%d]# RGB conversion standard: %s\n",i, "BT709");
    if (ppu_cfg->rgb_stan == BT709_L)
      APITRACEDEBUG("PpUnitCfg PPU[%d]# RGB conversion standard: %s\n",i, "BT709_L");
    if (ppu_cfg->rgb_stan == BT2020)
      APITRACEDEBUG("PpUnitCfg PPU[%d]# RGB conversion standard: %s\n",i, "BT2020");
    if (ppu_cfg->rgb_stan == BT2020_L)
      APITRACEDEBUG("PpUnitCfg PPU[%d]# RGB conversion standard: %s\n",i, "BT2020_L");
    APITRACEDEBUG("PpUnitCfg PPU[%d]# RGB alpha: %d\n",i, ppu_cfg->rgb_alpha);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# X filter size: %d, X filter offset\n",i, ppu_cfg->x_filter_size,ppu_cfg->x_filter_offset);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Y filter size: %d, Y filter offset\n",i, ppu_cfg->y_filter_size,ppu_cfg->y_filter_offset);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Outwidth: %d, Outheight\n",i, ppu_cfg->out_width,ppu_cfg->out_height);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Out ratio x: %d, Out ratio y\n",i, ppu_cfg->out_ratio_x,ppu_cfg->out_ratio_y);
    switch(ppu_cfg->pp_filter){
      case PP_VSI_LINEAR: APITRACEDEBUG("PpUnitCfg PPU[%d]# PP filter: %s\n",i, "PP_VSI_LINEAR"); break;
      case PP_LANCZOS: APITRACEDEBUG("PpUnitCfg PPU[%d]# PP filter: %s\n",i, "PP_LANCZOS"); break;
      case PP_NEAREST: APITRACEDEBUG("PpUnitCfg PPU[%d]# PP filter: %s\n",i, "PP_NEAREST"); break;
      case PP_BILINEAR: APITRACEDEBUG("PpUnitCfg PPU[%d]# PP filter: %s\n",i, "PP_BILINEAR"); break;
      case PP_BICUBIC: APITRACEDEBUG("PpUnitCfg PPU[%d]# PP filter: %s\n",i, "PP_BICUBIC"); break;
      case PP_SPLINE: APITRACEDEBUG("PpUnitCfg PPU[%d]# PP filter: %s\n",i, "PP_SPLINE"); break;
      case PP_BOX: APITRACEDEBUG("PpUnitCfg PPU[%d]# PP filter: %s\n",i, "PP_BOX"); break;
      case PP_FAST_LINEAR: APITRACEDEBUG("PpUnitCfg PPU[%d]# PP filter: %s\n",i, "PP_FAST_LINEAR"); break;
      case PP_FAST_BICUBIC: APITRACEDEBUG("PpUnitCfg PPU[%d]# PP filter: %s\n",i, "PP_FAST_BICUBIC"); break;
      case PP_AREA: APITRACEDEBUG("PpUnitCfg PPU[%d]# PP filter: %s\n",i, "PP_AREA"); break;
      default:
        ASSERT(0);
        break;
    }
    APITRACEDEBUG("PpUnitCfg PPU[%d]# X filter param: %d\n",i, ppu_cfg->x_filter_param);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Y filter param: %d\n",i, ppu_cfg->y_filter_param);
    switch(ppu_cfg->tile_mode){
      case TILED4x4: APITRACEDEBUG("PpUnitCfg PPU[%d]# TIELD mode: %s\n",i, "TILED4x4"); break;
      case TILED8x8: APITRACEDEBUG("PpUnitCfg PPU[%d]# TIELD mode: %s\n",i, "TILED8x8"); break;
      case TILED16x16: APITRACEDEBUG("PpUnitCfg PPU[%d]# TIELD mode: %s\n",i, "TILED16x16"); break;
      case TILED128x2: APITRACEDEBUG("PpUnitCfg PPU[%d]# TIELD mode: %s\n",i, "TILED128x2"); break;
      case TILED64x64: APITRACEDEBUG("PpUnitCfg PPU[%d]# TIELD mode: %s\n",i, "TILED64x64"); break;
      case TILED32x8: APITRACEDEBUG("PpUnitCfg PPU[%d]# TIELD mode: %s\n",i, "TILED32x8"); break;
      case RECONTILED8x8: APITRACEDEBUG("PpUnitCfg PPU[%d]# TIELD mode: %s\n",i, "RECONTILED8x8"); break;
      case TILED2x4: APITRACEDEBUG("PpUnitCfg PPU[%d]# TIELD mode: %s\n",i, "TILED2x4"); break;
      case PACKED422_TILED8x8: APITRACEDEBUG("PpUnitCfg PPU[%d]# TIELD mode: %s\n",i, "PACKED422_TILED8x8"); break;

      default:
        ASSERT(0);
        break;
    }
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Vir left: %d\n",i, ppu_cfg->vir_left);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Vir right: %d\n",i, ppu_cfg->vir_right);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Vir top: %d\n",i, ppu_cfg->vir_top);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Vir bottom: %d\n",i, ppu_cfg->vir_bottom);
    switch(ppu_cfg->src_sel_mode){
      case DOWN_ROUND: APITRACEDEBUG("PpUnitCfg PPU[%d]# Center point select algorithm: %s\n",i, "DOWN_ROUND"); break;
      case NO_ROUND: APITRACEDEBUG("PpUnitCfg PPU[%d]# Center point select algorithm: %s\n",i, "NO_ROUND"); break;
      case UP_ROUND: APITRACEDEBUG("PpUnitCfg PPU[%d]# Center point select algorithm: %s\n",i, "UP_ROUND"); break;
      default:
        ASSERT(0);
        break;
    }
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Pad sel: %d\n",i, ppu_cfg->pad_sel);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Luma pad value: %d\n",i, ppu_cfg->pad_Y);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Cb pad value: %d\n",i, ppu_cfg->pad_U);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Cr pad value: %d\n",i, ppu_cfg->pad_V);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Sub x: %d\n",i, ppu_cfg->sub_x);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Sub y: %d\n",i, ppu_cfg->sub_y);
    switch(ppu_cfg->chroma_format){
      case PP_YUV400: APITRACEDEBUG("PpUnitCfg PPU[%d]# Chroma format: %s\n",i, "PP_YUV400"); break;
      case PP_YUV420: APITRACEDEBUG("PpUnitCfg PPU[%d]# Chroma format: %s\n",i, "PP_YUV420"); break;
      case PP_YUV422: APITRACEDEBUG("PpUnitCfg PPU[%d]# Chroma format: %s\n",i, "PP_YUV422"); break;
      case PP_YUV444: APITRACEDEBUG("PpUnitCfg PPU[%d]# Chroma format: %s\n",i, "PP_YUV444"); break;
      default:
        ASSERT(0);
        break;
    }
    APITRACEDEBUG("PpUnitCfg PPU[%d]# X phase num: %d\n",i, ppu_cfg->x_phase_num);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Y phase num: %d\n",i, ppu_cfg->y_phase_num);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# LC stride: %d\n",i, ppu_cfg->lc_stripe);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# lanczos_table# Virtual address: 0x%x\n",i, ppu_cfg->lanczos_table.virtual_address);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# lanczos_table# Bus address: 0x%x\n",i, ppu_cfg->lanczos_table.bus_address);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# lanczos_table# Size: %d\n",i, ppu_cfg->lanczos_table.size);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# lanczos_table# Logical: %d\n",i, ppu_cfg->lanczos_table.logical_size);
    switch(ppu_cfg->lanczos_table.mem_type){
      case DWL_MEM_TYPE_CPU: APITRACEDEBUG("PpUnitCfg PPU[%d]# lanczos_table# MEM type: %s\n",i, "DWL_MEM_TYPE_CPU"); break;
      case DWL_MEM_TYPE_SLICE: APITRACEDEBUG("PpUnitCfg PPU[%d]# lanczos_table# MEM type: %s\n",i, "DWL_MEM_TYPE_SLICE"); break;
      case DWL_MEM_TYPE_DPB: APITRACEDEBUG("PpUnitCfg PPU[%d]# lanczos_table# MEM type: %s\n",i, "DWL_MEM_TYPE_DPB"); break;
      case DWL_MEM_TYPE_VPU_WORKING: APITRACEDEBUG("PpUnitCfg PPU[%d]# lanczos_table# MEM type: %s\n",i, "DWL_MEM_TYPE_VPU_WORKING"); break;
      case DWL_MEM_TYPE_VPU_WORKING_SPECIAL: APITRACEDEBUG("PpUnitCfg PPU[%d]# lanczos_table# MEM type: %s\n",i, "DWL_MEM_TYPE_VPU_WORKING_SPECIAL"); break;

      case DWL_MEM_TYPE_VPU_ONLY: APITRACEDEBUG("PpUnitCfg PPU[%d]# lanczos_table# MEM type: %s\n",i, "DWL_MEM_TYPE_VPU_ONLY"); break;
      case DWL_MEM_TYPE_VPU_CPU: APITRACEDEBUG("PpUnitCfg PPU[%d]# lanczos_table# MEM type: %s\n",i, "DWL_MEM_TYPE_VPU_CPU"); break;
      case DWL_MEM_TYPE_SYNC_DEVICE_ONLY: APITRACEDEBUG("PpUnitCfg PPU[%d]# lanczos_table# MEM type: %s\n",i, "DWL_MEM_TYPE_SYNC_DEVICE_ONLY"); break;
      case DWL_MEM_TYPE_SYNC_HOST_TO_DEVICE: APITRACEDEBUG("PpUnitCfg PPU[%d]# lanczos_table# MEM type: %s\n",i, "DWL_MEM_TYPE_SYNC_HOST_TO_DEVICE"); break;
      case DWL_MEM_TYPE_SYNC_DEVICE_TO_HOST: APITRACEDEBUG("PpUnitCfg PPU[%d]# lanczos_table# MEM type: %s\n",i, "DWL_MEM_TYPE_SYNC_DEVICE_TO_HOST"); break;
      case DWL_MEM_TYPE_SYNC_HOST_AND_DEVICE: APITRACEDEBUG("PpUnitCfg PPU[%d]# lanczos_table# MEM type: %s\n",i, "DWL_MEM_TYPE_SYNC_HOST_AND_DEVICE"); break;
      default:
        APITRACEDEBUG("PpUnitCfg PPU[%d]# lanczos_table# MEM type: %s\n",i, "Illegal type");
        break;

    }
    APITRACEDEBUG("PpUnitCfg PPU[%d]# FBC tile# Virtual address: 0x%x\n",i, ppu_cfg->fbc_tile.virtual_address);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# FBC tile# Bus address: 0x%x\n",i, ppu_cfg->fbc_tile.bus_address);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# FBC tile# Size: 0x%x\n",i, ppu_cfg->fbc_tile.size);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# FBC tile# Offset: 0x%x\n",i, ppu_cfg->fbc_tile_offset);

    switch(ppu_cfg->fbc_tile.mem_type){
      case DWL_MEM_TYPE_CPU: APITRACEDEBUG("PpUnitCfg PPU[%d]# FBC tile# MEM type: %s\n",i, "DWL_MEM_TYPE_CPU"); break;
      case DWL_MEM_TYPE_SLICE: APITRACEDEBUG("PpUnitCfg PPU[%d]# FBC tile# MEM type: %s\n",i, "DWL_MEM_TYPE_SLICE"); break;
      case DWL_MEM_TYPE_DPB: APITRACEDEBUG("PpUnitCfg PPU[%d]# FBC tile# MEM type: %s\n",i, "DWL_MEM_TYPE_DPB"); break;
      case DWL_MEM_TYPE_VPU_WORKING: APITRACEDEBUG("PpUnitCfg PPU[%d]# FBC tile# MEM type: %s\n",i, "DWL_MEM_TYPE_VPU_WORKING"); break;
      case DWL_MEM_TYPE_VPU_WORKING_SPECIAL: APITRACEDEBUG("PpUnitCfg PPU[%d]# FBC tile# MEM type: %s\n",i, "DWL_MEM_TYPE_VPU_WORKING_SPECIAL"); break;

      case DWL_MEM_TYPE_VPU_ONLY: APITRACEDEBUG("PpUnitCfg PPU[%d]# FBC tile# MEM type: %s\n",i, "DWL_MEM_TYPE_VPU_ONLY"); break;
      case DWL_MEM_TYPE_VPU_CPU: APITRACEDEBUG("PpUnitCfg PPU[%d]# FBC tile# MEM type: %s\n",i, "DWL_MEM_TYPE_VPU_CPU"); break;
      case DWL_MEM_TYPE_SYNC_DEVICE_ONLY: APITRACEDEBUG("PpUnitCfg PPU[%d]# FBC tile# MEM type: %s\n",i, "DWL_MEM_TYPE_SYNC_DEVICE_ONLY"); break;
      case DWL_MEM_TYPE_SYNC_HOST_TO_DEVICE: APITRACEDEBUG("PpUnitCfg PPU[%d]# FBC tile# MEM type: %s\n",i, "DWL_MEM_TYPE_SYNC_HOST_TO_DEVICE"); break;
      case DWL_MEM_TYPE_SYNC_DEVICE_TO_HOST: APITRACEDEBUG("PpUnitCfg PPU[%d]# FBC tile# MEM type: %s\n",i, "DWL_MEM_TYPE_SYNC_DEVICE_TO_HOST"); break;
      case DWL_MEM_TYPE_SYNC_HOST_AND_DEVICE: APITRACEDEBUG("PpUnitCfg PPU[%d]# FBC tile# MEM type: %s\n",i, "DWL_MEM_TYPE_SYNC_HOST_AND_DEVICE"); break;
      // default:
      //   ASSERT(0);
      //   break;
    }
    u16 j;
    for (j = 0; j < MAX_ASIC_CORES; j++){
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Reorder buf bus[%d]: %d\n",i, j, ppu_cfg->reorder_buf_bus[j]);
    }
    for (j = 0; j < MAX_ASIC_CORES; j++){
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Scale buf bus[%d]: %d\n",i, j, ppu_cfg->scale_buf_bus[j]);
    }

    for (j = 0; j < MAX_ASIC_CORES; j++){
      APITRACEDEBUG("PpUnitCfg PPU[%d]# Scale out buf bus[%d]: %d\n",i, j, ppu_cfg->scale_out_buf_bus[j]);
    }

    APITRACEDEBUG("PpUnitCfg PPU[%d]# Reorder size: %d\n",i, ppu_cfg->reorder_size);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Scale size: %d\n",i, ppu_cfg->scale_size);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Scale out size: %d\n",i, ppu_cfg->scale_out_size);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Out YUYV/YVYU: %d\n",i, ppu_cfg->out_yuyv);
    APITRACEDEBUG("PpUnitCfg PPU[%d]# Out UYVY/UYVY: %d\n",i, ppu_cfg->out_uyvy);
    }
  }

#else
#define PrintPpunitConfig(cfg)
#endif //#ifdef VCD_LOGMSG

static u32 PPGetUnitStride(PpUnitIntConfig *ppu_cfg) {
  u32 pp_width, ystride = 0, cstride = 0;

  if (ppu_cfg->crop2.enabled)
    pp_width = ppu_cfg->crop2.width;
  else
    pp_width = ppu_cfg->frm_width;

  if (ppu_cfg->tiled_e && !ppu_cfg->tile_mode) {
    pp_width = NEXT_MULTIPLE(pp_width, 4);
    if(ppu_cfg->rgb) {
      if (IS_3PLAN_FORMAT(ppu_cfg->rgb_format))
        pp_width *= 3;
      else if (IS_4PLAN_FORMAT(ppu_cfg->rgb_format))
        pp_width *= 4;
      else if(ppu_cfg->rgb_format == PP_OUT_RGB565 || ppu_cfg->rgb_format == PP_OUT_BGR565)
        pp_width *= 2;
      else if(ppu_cfg->rgb_format == PP_OUT_ARGB8565)
        pp_width *= 4;
    }
    ystride = NEXT_MULTIPLE(4 * pp_width * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
    cstride = NEXT_MULTIPLE(4 * (ppu_cfg->sub_x == 1 ? 2 : 1) * pp_width * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
    if(ppu_cfg->rgb)
      cstride = ystride;
  } else if ((ppu_cfg->out_yuyv || ppu_cfg->out_uyvy) && !ppu_cfg->tiled_e) {/*not tile2x4*/
  /*this cstride didn't use*/
    ystride = cstride = NEXT_MULTIPLE(2 * pp_width * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
  } else if (ppu_cfg->out_1010) {
    pp_width = ((pp_width + 2) / 3) * 3;
    ystride = NEXT_MULTIPLE(4 * pp_width / 3, ALIGN(ppu_cfg->align));
    cstride = NEXT_MULTIPLE(4 * (ppu_cfg->sub_x == 1 ? 2 : 1) * pp_width / 3, ALIGN(ppu_cfg->align));
  } else if (ppu_cfg->tiled_e && ppu_cfg->tile_mode) {
    if (ppu_cfg->tile_mode == TILED16x16) {
      pp_width = NEXT_MULTIPLE(pp_width, 16);
      ystride = cstride = NEXT_MULTIPLE(pp_width * ppu_cfg->pixel_width * 24, ALIGN(ppu_cfg->align) * 8) / 8;
    } else if (ppu_cfg->tile_mode == TILED8x8  || ppu_cfg->tile_mode == RECONTILED8x8) {
      pp_width = ppu_cfg->tile_mode == TILED8x8 ? NEXT_MULTIPLE(pp_width, 8) : NEXT_MULTIPLE(pp_width, 16);
      ystride = NEXT_MULTIPLE(8 * pp_width * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
      cstride = NEXT_MULTIPLE(4 * (ppu_cfg->sub_x == 1 ? 2 : 1) * pp_width * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
      if (ppu_cfg->dec400_enabled) {
        ystride = NEXT_MULTIPLE(8 * NEXT_MULTIPLE(pp_width, ppu_cfg->align_pixel_w) * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
        cstride = NEXT_MULTIPLE(4 * (ppu_cfg->sub_x == 1 ? 2 : 1) * NEXT_MULTIPLE(pp_width, ppu_cfg->align_pixel_w) * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
      }
    } else if (ppu_cfg->tile_mode == TILED64x64) {
      /* TILED64x64 only support 4 plan RGB formats. */
      if (IS_4PLAN_FORMAT(ppu_cfg->rgb_format)) {
        // pp_width *= 4;
        // ystride = cstride = pp_width;
        pp_width = NEXT_MULTIPLE(pp_width, 64);
        ystride = cstride = NEXT_MULTIPLE(pp_width * 64 * 4, ALIGN(ppu_cfg->align));
      } else {//yuv420
        pp_width = NEXT_MULTIPLE(pp_width, 64);
        // ystride = cstride = NEXT_MULTIPLE(pp_width * 64, ALIGN(ppu_cfg->align));
        // pp_width = NEXT_MULTIPLE(pp_width * ppu_cfg->pixel_width, 64) / 8;
        ystride = NEXT_MULTIPLE(pp_width * ppu_cfg->pixel_width * 64, ALIGN(ppu_cfg->align)) / 8;
        cstride = ystride / 2;
        // ystride = NEXT_MULTIPLE(pp_width * 64 * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
        // cstride = NEXT_MULTIPLE(pp_width * 32 * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
      }
    } else if (ppu_cfg->tile_mode == TILED2x4 && (ppu_cfg->out_yuyv || ppu_cfg->out_uyvy)) {
      ystride = cstride = NEXT_MULTIPLE(2 * NEXT_MULTIPLE(pp_width, 4) * 4 * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
    } else if (ppu_cfg->tile_mode == PACKED422_TILED8x8 && (ppu_cfg->out_yuyv || ppu_cfg->out_uyvy)) {
      if (ppu_cfg->pixel_width == 8) {
        ystride = NEXT_MULTIPLE(2/*YUYUV*/ * NEXT_MULTIPLE(pp_width, 16) * 8/*h*/ * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
        cstride = 0; /*here cstride didn't use*/
      } else { // only support P010
        // when P010, pixel_width = 16
        ystride = NEXT_MULTIPLE(2/*YUYUV*/ * NEXT_MULTIPLE(pp_width, 8) * 8 * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
        cstride = 0;
      }
    } else {
      pp_width = NEXT_MULTIPLE(pp_width, 256);
      ystride = cstride = NEXT_MULTIPLE(2 * pp_width * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
    }
  } else if (ppu_cfg->rgb) {
    if (IS_3PLAN_FORMAT(ppu_cfg->rgb_format)) {
      pp_width *= 3;
      ystride = cstride = NEXT_MULTIPLE(pp_width * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
    } else if (IS_4PLAN_FORMAT(ppu_cfg->rgb_format)) {
      pp_width *= 4;
      ystride = cstride = NEXT_MULTIPLE(pp_width * 8, ALIGN(ppu_cfg->align) * 8) / 8;
    } else if(ppu_cfg->rgb_format == PP_OUT_RGB565 || ppu_cfg->rgb_format == PP_OUT_BGR565) {
      pp_width *= 2;
      ystride = cstride = NEXT_MULTIPLE(pp_width * 8, ALIGN(ppu_cfg->align) * 8) / 8;
    } else if(ppu_cfg->rgb_format == PP_OUT_ARGB8565) {
      pp_width *= 3;
      ystride = cstride = NEXT_MULTIPLE(pp_width * 8, ALIGN(ppu_cfg->align) * 8) / 8;
    }
  } else if (ppu_cfg->rgb_planar) {
    ystride = cstride = NEXT_MULTIPLE(pp_width * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
  } else if (ppu_cfg->pp_pvfbc_bypass) {
    //pvfbc bypass tiled 32x8 8bit 32x8 10bit 16x8
      if(ppu_cfg->pp_pvfbc_tile_type == PVFBC_TILED8x8)
        ystride = cstride = NEXT_MULTIPLE(8 * pp_width * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align > DEC_ALIGN_256B ? ppu_cfg->align : DEC_ALIGN_256B) * 8) / 8;
      else if(ppu_cfg->pp_pvfbc_tile_type == PVFBC_TILED16x4)
        ystride = cstride = NEXT_MULTIPLE(4 * pp_width * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align > DEC_ALIGN_256B ? ppu_cfg->align : DEC_ALIGN_256B) * 8) / 8;
  } else {
    if (ppu_cfg->out_format == PP_OUT_FMT_YUV400_WITH_ALPHA)
      pp_width *= 2;
    ystride = NEXT_MULTIPLE(pp_width * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
    if (ppu_cfg->planar) {
      /*support odd crop*/
      //cstride = NEXT_MULTIPLE((ppu_cfg->sub_x == 1 ? 2 : 1) * pp_width / 2 * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
      cstride = NEXT_MULTIPLE((ppu_cfg->sub_x == 2 ? (pp_width + 1) / 2 : pp_width) * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
    } else {
      /*support odd crop*/
      //cstride = NEXT_MULTIPLE((ppu_cfg->sub_x == 1 ? 2 : 1) * pp_width * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
      cstride = NEXT_MULTIPLE((ppu_cfg->sub_x == 2 ? ((pp_width + 1) & ~0x1) : 2 * pp_width) * ppu_cfg->pixel_width, ALIGN(ppu_cfg->align) * 8) / 8;
    }
  }

  /* for dec400 use tile 256x1, need set stride align 256 */
  if (ppu_cfg->dec400_enabled) {
    ystride = NEXT_MULTIPLE(ystride, 256);
    cstride = NEXT_MULTIPLE(cstride, 256);
  }
  if (ppu_cfg->ystride == 0) {
#if 0
    if ((ppu_cfg->tiled_e && !ppu_cfg->tile_mode && ((ystride >> hw_feature->pp_tiled_stride_shift) >= PP_MAX_STRIDE)) ||
        (ppu_cfg->tile_mode && ((ystride >> hw_feature->pp_tiled_stride_shift) >= PP_MAX_STRIDE)) ||
        (!ppu_cfg->tile_mode && !ppu_cfg->tiled_e && (ystride >= PP_MAX_STRIDE))) {
      APITRACEERR("PPGetUnitStride# Illegal param[PP %u]: Too large Y stride.\n", i);
      return 1;
    }
#endif

    ppu_cfg->ystride = ystride;
    ppu_cfg->false_ystride = 1;
  } else {
    if (ppu_cfg->false_ystride && !ppu_cfg->stride_set_by_user) {
      ppu_cfg->ystride = ystride;
    } else {
      if (ppu_cfg->ystride < ystride) {
          APITRACEERR("PPGetUnitStride# Illegal param: Too large Y stride, ppu_cfg->ystride %d ystirde %d.\n",
                ppu_cfg->ystride, ystride);
        return 1;
      }
    }
  }

  if (ppu_cfg->cstride == 0) {
#if 0
    if ((ppu_cfg->tiled_e && !ppu_cfg->tile_mode && ((cstride >> hw_feature->pp_tiled_stride_shift) >= PP_MAX_STRIDE)) ||
        (ppu_cfg->tile_mode && ((cstride >> hw_feature->pp_tiled_stride_shift) >= PP_MAX_STRIDE)) ||
        (!ppu_cfg->tile_mode && !ppu_cfg->tiled_e && (cstride >= PP_MAX_STRIDE))) {
      APITRACEERR("PPGetUnitStride# Illegal param[PP %u]: Too large C stride.\n", i);
      return 1;
    }
#endif
    ppu_cfg->cstride = cstride;
    ppu_cfg->false_cstride = 1;
  } else {
    if (ppu_cfg->false_cstride && !ppu_cfg->stride_set_by_user) {
      ppu_cfg->cstride = cstride;
    } else {
      if (ppu_cfg->cstride < cstride) {
          APITRACEERR("PPGetUnitStride# Illegal param: Too large C stride, ppu_cfg->cstride %d cstirde %d.\n",
                ppu_cfg->cstride, cstride);
        return 1;
      }
    }
  }

  return 0;
}

void UpdatePpUnitStride(PpUnitIntConfig *ppu_cfg) {
  ppu_cfg->frm_width = ppu_cfg->scale.width;
  ppu_cfg->frm_height = ppu_cfg->scale.height;

  PPGetUnitStride(ppu_cfg);
}

/* Set ppu_int_cfg by hw_feature . */
void SetPpByReleaseHwFeatures(PpUnitIntConfig *ppu_int_cfg, PpUnitConfig *ppu_ext_cfg,
                              const struct DecHwFeatures *hw_feature) {
  for (u32 i = 0; i < DEC_MAX_PPU_COUNT; i++, ppu_int_cfg++, ppu_ext_cfg++) {
    if (!ppu_ext_cfg->enabled) continue;
    if (ppu_ext_cfg->comp_enabled) {
      switch (hw_feature->pp_comp_support[i]) {
        case 1: ppu_int_cfg->pp_comp = 1; break;
        case 2: ppu_int_cfg->dec400_enabled = 1; break;
        case 3: ppu_int_cfg->pp_pvfbc = 1; break;
        default:
          APITRACEERR("The HW ppu[%u] don't support --pp-comp.\n", i);
      }
    }
    if ((u32)ppu_ext_cfg->align < (u32)hw_feature->shaper_alignment) {
      if (ppu_int_cfg->dec400_enabled) {
        ppu_ext_cfg->align = ppu_int_cfg->align = (u32)hw_feature->shaper_alignment;
      } else {
        ppu_ext_cfg->shaper_no_pad = ppu_int_cfg->shaper_no_pad = 1;
        ppu_ext_cfg->shaper_tile_edge_pad = ppu_int_cfg->shaper_tile_edge_pad = 0;
      }
    }
    if(!hw_feature->yuv420_8b_to_p010)
      ppu_ext_cfg->Explicit_Specified_010 = 0;
  }
}

/* To be moved to a comomn source file for all formats. */
u32 CheckPpUnitConfig(const struct DecHwFeatures *hw_feature,
                            u32 in_width,
                            u32 in_height,
                            u32 interlace,
                            u32 pixel_width,
                            u32 in_chroma_format,
                            PpUnitIntConfig *ppu_cfg) {
  u32 i = 0;
  PpUnitIntConfig *ppu_cfg_temp = ppu_cfg;
  PrintPpunitConfig(ppu_cfg_temp);
  /*support odd crop*/ /*using hw feature 'crop_step_rshift'*/
  u32 crop_align = (1 << hw_feature->crop_step_rshift) - 1;
  for (i = 0; i < DEC_MAX_PPU_COUNT; i++, ppu_cfg++) {
#if !VCDJ_ENABLE_ULTRA_SCALER
    if (ppu_cfg->enabled &&
        hw_feature->scale_cost_support[i] == ULTRA_SCALER) {
      APITRACEERR("CheckPpUnitConfig# ULTRA_SCALER is disabled in this build [PP %u].\n", i);
      return 1;
    }
#endif

    if (i >= hw_feature->max_ppu_count && ppu_cfg->enabled) {
      APITRACEERR("The count(%u) of PPU exceeded the max PPU count.\n", i);
      return 1;
      // ppu_cfg->enabled = 0;
    }

    // checker for ringbuffer and flexa
    if (ppu_cfg->rb_en && !ppu_cfg->enabled) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Ringbuffer should enable pp\n", i);
      return 1;
    }
    if (ppu_cfg->sbi_en && !ppu_cfg->enabled) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Sbi should enable pp\n", i);
      return 1;
    }

    if (!ppu_cfg->enabled) continue;

    if (in_width == 0 && in_height == 0)
      return 0;

    /*only support YUV420 case to YUYV/YVYU*/
    #if 0
    if ((in_chroma_format < ppu_cfg->chroma_format) && !(in_chroma_format == PP_CHROMA_420 && (ppu_cfg->out_yuyv || ppu_cfg->out_uyvy))) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: upsample to 422P or 422SP is not supuported in HW feature, only support YUYV and UYVY.\n", i);
      return 1;
    }
    #endif

    if(!ppu_cfg->planar_f) {
      if (!hw_feature->pp_planar_support[i] && ppu_cfg->planar) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: PLANAR output is not supuported.\n", i);
        return 1;
      }
    }

#if 0 /*off checker*/
    if (ppu_cfg->rgb || ppu_cfg->rgb_planar) {
      rgb_range_map_enable = 1;
    }
    if (ppu_cfg->set_yuv_target_enable) {
      yuv_range_map_enable = 1;
      if (ppu_cfg->yuv_target_range) {
        yuv_full_range++;
      } else {
        yuv_limited_range++;
      }
    }
    // video_range represents rgb range (1 means full, 0 means limited)
    // yuv_target_range represents yuv range (1 means full, 0 means limited)

    if (rgb_range_map_enable && yuv_range_map_enable) {
      if (ppu_cfg->video_range) {
        if (yuv_limited_range) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Diffrent range setting is not supuported.\n", i);
          return 1;
        }
      } else {
        if (yuv_full_range) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Diffrent range setting is not supuported.\n", i);
          return 1;
        }
      }
    }
#endif

    if (!(ppu_cfg->rgb || ppu_cfg->rgb_planar) && ppu_cfg->enable_3dlut) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: 3dlut only supuported when rgb output.\n", i);
      return 1;
    }

    if (!hw_feature->fmt_tile_support[i] && ppu_cfg->tiled_e && !ppu_cfg->tile_mode) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: TILE4x4 output is not supuported.\n", i);
      return 1;
    }

    if (!hw_feature->fmt_output_tile8x8_support[i] && ppu_cfg->tiled_e && ppu_cfg->tile_mode == TILED8x8) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: TILE8x8 output is not supuported.\n", i);
      return 1;
    }

    if (interlace && ppu_cfg->tiled_e) {
      APITRACEERR("%s","CheckPpUnitConfig# Illegal param: TILED output not support interlace frame\n");
      return 1;
    }

    if (!hw_feature->fmt_supertile_support[i] && ppu_cfg->tile_mode == TILED64x64 && ppu_cfg->rgb) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Super_tile output is not supuported.\n", i);
      return 1;
    }

    if (!hw_feature->pp_yuv420_101010_support && ppu_cfg->out_1010) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: 101010 output is not supuported.\n", i);
      return 1;
    }

    if (!hw_feature->fmt_p010_support &&
        (ppu_cfg->out_p010 || ppu_cfg->out_I010 || ppu_cfg->out_L010)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: P010 output is not supuported.\n", i);
      return 1;
    }
    if (!hw_feature->second_crop_support && ppu_cfg->crop2.enabled) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Second crop is not supuported.\n", i);
      return 1;
    }
    if (!hw_feature->pp_3dlut[i] && ppu_cfg->enable_3dlut) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: 3dlut is not supuported.\n", i);
      return 1;
    }

#if 0
    if (!hw_feature->fmt_rgb_support[i] && (ppu_cfg->rgb || ppu_cfg->rgb_planar)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: RGB is not supported.\n", i);
      return 1;
    }
#endif
    if (ppu_cfg->monochrome) {
      if (ppu_cfg->rgb || ppu_cfg->rgb_planar)
        ppu_cfg->out_format = PP_OUT_FMT_RGB;
      // else if (ppu_cfg->out_yuyv)
      //   ppu_cfg->out_format = PP_OUT_FMT_YUV422_YUYV_8BIT;
      // else if (ppu_cfg->out_uyvy)
      //   ppu_cfg->out_format = PP_OUT_FMT_YUV422_UYVY_8BIT;
      else if (ppu_cfg->out_p010 || ppu_cfg->out_I010 || ppu_cfg->out_L010)
        ppu_cfg->out_format = PP_OUT_FMT_YUV400_P010;
      else if (ppu_cfg->out_p012 || ppu_cfg->out_I012)
        ppu_cfg->out_format = PP_OUT_FMT_YUV400_P012;
      else if (ppu_cfg->out_cut_8bits)
        ppu_cfg->out_format = PP_OUT_FMT_YUV400_8BIT;
      else
        ppu_cfg->out_format = ppu_cfg->yuv400_with_alpha ? PP_OUT_FMT_YUV400_WITH_ALPHA : PP_OUT_FMT_YUV400;
      if (ppu_cfg->planar)
        ppu_cfg->planar = 0;
    }
    if (crop_align) {
      ppu_cfg->crop.width = (ppu_cfg->crop.width +1) & ~0x1;
      ppu_cfg->crop.height = (ppu_cfg->crop.height +1) & ~0x1;
    }
    if (!ppu_cfg->crop.width || !ppu_cfg->crop.height) {
      ppu_cfg->crop.enabled = 0;
    }
    if (!ppu_cfg->crop.enabled) {
      ppu_cfg->crop.x = ppu_cfg->crop.y = 0;
      ppu_cfg->crop.width = in_width;
      ppu_cfg->crop.height = in_height;
      ppu_cfg->crop.set_by_user = 0;
      ppu_cfg->crop.enabled = 1;
    }

    if (!ppu_cfg->scale.width || !ppu_cfg->scale.height) {
      ppu_cfg->scale.enabled = 0;
    }

    /* reset PP0 scaling w/h when enable '-d' and tb.cfg*/
    if (ppu_cfg->scale.ratio_x && ppu_cfg->scale.ratio_y &&
        !ppu_cfg->scale.width && !ppu_cfg->scale.height) {
      /*support odd crop*/
      if (crop_align) {
        ppu_cfg->scale.width = DOWN_SCALE_SIZE(ppu_cfg->crop.width, ppu_cfg->scale.ratio_x, 0);
        ppu_cfg->scale.height = DOWN_SCALE_SIZE(ppu_cfg->crop.height, ppu_cfg->scale.ratio_y, interlace);
      } else {
        ppu_cfg->scale.width = ppu_cfg->crop.width / ppu_cfg->scale.ratio_x;
        ppu_cfg->scale.height = ppu_cfg->crop.height / ppu_cfg->scale.ratio_y;
      }
      /* To resolve last 'if' which sets ppu_cfg->scale.enabled as 0 */
      ppu_cfg->scale.enabled = 1;
    }

    if (!ppu_cfg->scale.enabled) {
      ppu_cfg->scale.width = ppu_cfg->crop.width;
      ppu_cfg->scale.height = ppu_cfg->crop.height;
      ppu_cfg->scale.ratio_x = 1;
      ppu_cfg->scale.ratio_y = 1;
      ppu_cfg->scale.scale_by_ratio = 1;
      ppu_cfg->scale.enabled = 1;
    }
    ppu_cfg->frm_width = ppu_cfg->scale.width;
    ppu_cfg->frm_height = ppu_cfg->scale.height;


    /* hist constraint  */
    if (i == 2) {
      if (ppu_cfg->luma_sum_e && !ppu_cfg->hist_global_e && !ppu_cfg->hist_local_e && !ppu_cfg->hist_hsv_e) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d], which can't be enabled if not enable any one feature of luma_sum_e, hist_global_e, hist_local_e and hist_hsv_e.\n", i);
        return 1;
      }
      if (ppu_cfg->hist_local_e) {
        u8 idx = 0;
        for (idx = 0; idx < 4; idx++) {
          if(ppu_cfg->hist_offset_x[idx] >= ppu_cfg->hist_offset_x[idx + 1]) {
            APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d], hist_local_e x offset error. adjacent regions cannot be configured with the same value\n", i);
            return 1;
          }
        }

        for (idx = 0; idx < 2 ; idx++) {
          if(ppu_cfg->hist_offset_y[idx] >= ppu_cfg->hist_offset_y[idx + 1]) {
            APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d], hist_local_e y offset error. adjacent regions cannot be configured with the same value\\n", i);
            return 1;
          }
        }

        if(ppu_cfg->hist_offset_x[0] == 0 || ppu_cfg->hist_offset_y[0] == 0 || ppu_cfg->hist_offset_y[0] == 1) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d], hist_local_e x/y offset error. x offset can't be setted to zero, y offset can't be setted to zero/one \\n", i);
          return 1;
        }

        for(idx = 0; idx < 5; idx ++) {
          if ((ppu_cfg->crop.x + ppu_cfg->hist_offset_x[idx]) % 8 != 0) {
            APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: Local Hist offset [%d] add Crop.x must align to 8.\n", i, ppu_cfg->hist_offset_x[idx]);
            return 1;
          }
        }

        for(idx = 0; idx < 3; idx ++) {
          if(interlace) {
            if ((ppu_cfg->crop.y / 2 + ppu_cfg->hist_offset_y[idx]) % 8 != 0) {
              APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: For interlace stream, Local Hist offset [%d] add Crop.y must align to 16.\n", i, ppu_cfg->hist_offset_y[idx]);
              return 1;
            }
          } else {
            if ((ppu_cfg->crop.y + ppu_cfg->hist_offset_y[idx]) % 8 != 0) {
              APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: Local Hist offset [%d] add Crop.y must align to 8.\n", i, ppu_cfg->hist_offset_y[idx]);
              return 1;
            }
          }
        }
      }
    }

    if(i == 2 && in_chroma_format == PP_CHROMA_444 && !(ppu_cfg->rgb || ppu_cfg->rgb_planar) && !ppu_cfg->monochrome && (ppu_cfg->chroma_format == PP_CHROMA_420 || ppu_cfg->chroma_format == PP_CHROMA_422)
      && ((ppu_cfg->crop.x & 0x1) || (ppu_cfg->crop.y & 0x1))) {
        if(ppu_cfg->hist_hsv_e == 0) {
          ppu_cfg->monochrome = 1;
          ppu_cfg->hist_hsv_e = 0;
          ppu_cfg->out_format = PP_OUT_FMT_YUV400;
        } else {
          ppu_cfg->rgb = 1;
        }
    }


    if (ppu_cfg->tile_mode == TILED64x64 && !(ppu_cfg->rgb || ppu_cfg->rgb_planar)
      && (ppu_cfg->out_p010 || ppu_cfg->out_I010 || ppu_cfg->out_L010 || ppu_cfg->out_1010 || ppu_cfg->out_p012 || ppu_cfg->out_I012)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: yuv supertile output only supuport 8bit / pack10 output.\n", i);
      return 1;
    }

    if (ppu_cfg->tiled_e && (pixel_width != 8 && ppu_cfg->out_cut_8bits) && ppu_cfg->tile_mode == TILED64x64 &&
        !(ppu_cfg->rgb || ppu_cfg->rgb_planar)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: When 10bit stream outputs in yuv TILED64x64, should not cut_8_bit.\n", i);
      return 1;
    }


    if (ppu_cfg->planar && pixel_width != 8 &&
        !(ppu_cfg->out_cut_8bits | ppu_cfg->out_p010 | ppu_cfg->out_I010 | ppu_cfg->out_L010)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: When 10bit stream outputs planar, must open one of cut_8_bit/P010/I010/L010.\n", i);
      return 1;
    }

    if (!ppu_cfg->tiled_e && ppu_cfg->tile_mode == TILED64x64) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: When output format is TILED64x64, must enable tiled flag.\n", i);
      return 1;
    }

    if (ppu_cfg->tiled_e && (pixel_width != 8 && !ppu_cfg->out_cut_8bits) && ppu_cfg->tile_mode && ppu_cfg->tile_mode != RECONTILED8x8 && ppu_cfg->tile_mode != TILED64x64 &&
        !(ppu_cfg->out_p010 | ppu_cfg->out_I010 | ppu_cfg->out_L010)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: When 10bit stream outputs in tile mode(except TILED64x64 & RECONTILED8x8), must open one of P010/I010/L010.\n", i);
      return 1;
    }

    if (ppu_cfg->tiled_e && ppu_cfg->tile_mode == TILED8x8 && (pixel_width != 8 && !ppu_cfg->out_cut_8bits) && !ppu_cfg->out_p010) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: When 10bit stream outputs in tile mode TILED8x8, must open P010, not support I010/L010.\n", i);
      return 1;
    }
    /* Realtek: enabled, the release ships it as #if 0. 0x2070 RTL has no
     * 420 -> 422 up-conversion; unchecked, the core writes half the chroma. */
    if (in_chroma_format == PP_CHROMA_420 && ppu_cfg->chroma_format == PP_CHROMA_422 && hw_feature->pp_420to422_support[i] != 1) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: PP don't support 420 to 422.\n", i);
      return 1;
    }

    if(ppu_cfg->tile_mode == TILED2x4 && !(ppu_cfg->out_yuyv || ppu_cfg->out_uyvy)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: only YUYV and UYVY support TILE2x4 output format.\n", i);
      return 1;
    }
    if((ppu_cfg->out_yuyv || ppu_cfg->out_uyvy) && ppu_cfg->tiled_e && !(ppu_cfg->tile_mode == TILED2x4 || ppu_cfg->tile_mode == PACKED422_TILED8x8)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: YUYV and UYVY with tile output only support TILE2x4 or PACKED422_TILED8x8.\n", i);
      return 1;
    }
    if(ppu_cfg->tile_mode == PACKED422_TILED8x8 && !(ppu_cfg->out_yuyv || ppu_cfg->out_uyvy)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: only YUYV and UYVY support PACKED422_TILED8x8 output format.\n", i);
      return 1;
      if ((pixel_width != 8 && !ppu_cfg->out_cut_8bits) && !ppu_cfg->out_p010) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: When 10bit stream outputs in tile mode PACKED422_TILED8x8, must open P010, not support others.\n", i);
        return 1;
      }
    }

    if ((ppu_cfg->out_yuyv || ppu_cfg->out_uyvy) && (!((ppu_cfg->sub_x == 2) && (ppu_cfg->sub_y == 1)))) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: only 422 support YUYV/YVYU and UYVY/VYUY format.\n", i);
      return 1;
    }
    // if ((ppu_cfg->out_yuyv || ppu_cfg->out_uyvy) && (pixel_width != 8)) {
    //   APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: only 8bit support YUYV and UYVY format.\n", i);
    //   return 1;
    // }
    // if ((ppu_cfg->out_yuyv || ppu_cfg->out_uyvy) && (pixel_width != 8) && (!ppu_cfg->out_cut_8bits)) {
    //   APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: 422packed must force 8bit output.\n", i);
    //   return 1;
    // }
    if (ppu_cfg->chroma_format >= PP_CHROMA_422 && ppu_cfg->monochrome) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: 444/422 output conflict luma only.\n", i);
      return 1;
    }
    if (ppu_cfg->chroma_format >= PP_CHROMA_422 && (pixel_width == 10) &&
        !ppu_cfg->out_p010 && !ppu_cfg->out_I010 && !ppu_cfg->out_L010 &&
        !ppu_cfg->out_cut_8bits) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: 444/422 output doesn't support pack10 output, please use '-f' option.\n", i);
      return 1;
    }
    if (ppu_cfg->chroma_format >= PP_CHROMA_422 && (ppu_cfg->rgb || ppu_cfg->rgb_planar)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: yuv output and rgb output can not both enabled\n", i);
      return 1;
    }
    if (ppu_cfg->chroma_format == PP_CHROMA_444 && !ppu_cfg->planar) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: 444 only support planar output\n", i);
      return 1;
    }
    // if (ppu_cfg->chroma_format == PP_CHROMA_422 && ppu_cfg->planar) {
    //   APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: 422 can not support planar output\n", i);
    //   return 1;
    // }
    if (ppu_cfg->chroma_format == PP_CHROMA_422 && ppu_cfg->tiled_e && !(ppu_cfg->out_yuyv || ppu_cfg->out_uyvy)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: 422 can not support tiled output except YUYV/UYVY only support TILED2x4 or PACKED422_TILED8x8\n", i);
      return 1;
    }
    if (ppu_cfg->out_1010 && (ppu_cfg->chroma_format != PP_CHROMA_420 || ppu_cfg->planar)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: only YUV420SP support p32('101010') output\n", i);
      return 1;
    }
    if(ppu_cfg->lc_stripe && (ppu_cfg->rgb || ppu_cfg->rgb_planar)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: line counter doesn't support rgb output\n", i);
      return 1;
    }
    if(ppu_cfg->lc_stripe && ppu_cfg->scale.width % 2) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: line counter doesn't support odd width output\n", i);
      return 1;
    }
#if 0
    if (ppu_cfg->tile_mode && (ppu_cfg->tile_mode != TILED128x2))
      InitFbcParams(ppu_cfg, 0, 0, 0);
#endif

    /*support decnano*/
    if(ppu_cfg->decnano_e) {
      if(ppu_cfg->tile_mode == TILED4x4) {
        if(ppu_cfg->rgb_format != PP_OUT_BGR888 && ppu_cfg->rgb_format != PP_OUT_ARGB888 && ppu_cfg->rgb_format != PP_OUT_RGB565  && ppu_cfg->rgb_format != PP_OUT_ARGB8565) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: TILE4x4 + DECNANO only support RGB888/ARGB888/RGB565/ARGB8565)\n", i);
          return 1;
        }
      }
    }

    /*pp comp checker*/
    if (ppu_cfg->pp_pvfbc){
      if (in_chroma_format == PP_CHROMA_400) {
        APITRACEERR("CheckPpUnitConfig# Illegal pp comp param[PP %d]: pp comp 'pvfbc' dosen't support YUV400 case.\n", i);
          return 1;
      }
      if (!(ppu_cfg->out_format == PP_OUT_FMT_YUV420_8BIT || (ppu_cfg->out_format == PP_OUT_FMT_YUV420_P010 && ppu_cfg->out_p010))) {
        APITRACEERR("CheckPpUnitConfig# Illegal pp comp param[PP %u]: pp comp 'pvfbc' only support YUV420 semiplanar (NV12_8it/NV21_8bit/NV12_P010) compression and dosen't support 400/422/RGB\n", i);
          return 1;
      }
      if (ppu_cfg->out_format == PP_OUT_FMT_YUV420_P010 && ppu_cfg->out_p010 && ppu_cfg->cr_first) {
        APITRACEERR("CheckPpUnitConfig# Illegal pp comp param[PP %u]: pp comp 'pvfbc' 420sp p010 output format dosen't support 'cr-first'\n", i);
          return 1;
      }
      if (ppu_cfg->tiled_e) {
        APITRACEERR("CheckPpUnitConfig# Illegal pp comp param[PP %u]: pp comp 'pvfbc' dosen't support tiled output format\n", i);
          return 1;
      }
      if (interlace) {
        APITRACEERR("CheckPpUnitConfig# Illegal pp comp param[PP %u]: interlace case dosen't support pp comp 'pvfbc'\n", i);
          return 1;
      }
      if (ppu_cfg->scale.width > 8192 || ppu_cfg->scale.height > 8192) {
        APITRACEERR("CheckPpUnitConfig# Illegal pp comp param[PP %u]: pp comp 'pvfbc' with output width or height should no more than 8192\n", i);
          return 1;
      }

    }
    if (ppu_cfg->pp_pvfbc_bypass && ppu_cfg->pp_pvfbc == 0) {
      APITRACEERR("CheckPpUnitConfig# Illegal pp comp param[PP %u]: must use pvfbc bypass with --pp-comp\n", i);
        return 1;
    }

    if (CheckPpStride(ppu_cfg)) return 1;

    if (pixel_width > 8 && ppu_cfg->dec400_enabled && !ppu_cfg->out_p010 && !ppu_cfg->out_cut_8bits && !ppu_cfg->rgb) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: please open p010 or ARGB when enables dec400 and output 10bit formats.\n", i);
      return 1;
    }
    /*support PP DEC400*/
    if (ppu_cfg->dec400_enabled && interlace) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: DEC400 doesn't support interlace stream.\n", i);
      return 1;
    }
    if (ppu_cfg->dec400_enabled && ppu_cfg->rgb_planar) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: PP DEC400 input doesn't support RGB planar.\n", i);
      return 1;
    }
    if (ppu_cfg->dec400_enabled && !ppu_cfg->shaper_enabled && ppu_cfg->tiled_e && (ppu_cfg->tile_mode != TILED64x64)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: PP DEC400 must enable shaper, DEC400 is after shaper module.\n", i);
      return 1;
    }
    if (ppu_cfg->dec400_enabled && ppu_cfg->tile_mode == TILED64x64 && !ppu_cfg->rgb) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: PP DEC400 doesn't support YUV supertile.\n", i);
      return 1;
    }
    if (ppu_cfg->dec400_enabled && ppu_cfg->chroma_format == PP_YUV422 && !(ppu_cfg->out_yuyv == 1 || ppu_cfg->out_uyvy == 1)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: When PP DEC400 outputs 422, must set '--pp-packed-mode' as one of YUYV/UYVY.\n", i);
      return 1;
    }
    if (ppu_cfg->dec400_enabled && ppu_cfg->chroma_format == PP_YUV444) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: PP DEC400 doesn't support 444 output.\n", i);
      return 1;
    }
    if ((ppu_cfg->tile_mode == TILED64x64 && ppu_cfg->rgb && IS_4PLAN_FORMAT(ppu_cfg->rgb_format)) ||
        (!hw_feature->pp_support_6x && (ppu_cfg->scale.width > ppu_cfg->crop.width*3 || ppu_cfg->scale.height > ppu_cfg->crop.height*3))) {
      ppu_cfg->shaper_enabled = 0;
    }
    if (ppu_cfg->dec400_enabled && ppu_cfg->shaper_enabled && ALIGN(ppu_cfg->align) < 256) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: shaper algin doesn't less than 256 when enable PP DEC400.\n", i);
      return 1;
    }
    if (ppu_cfg->dec400_enabled && !(ppu_cfg->dec400_align == DEC_ALIGN_32B || ppu_cfg->dec400_align == DEC_ALIGN_64B)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: should configure '--dec400-align=32' or '--dec400-align=64' when enable PP DEC400.\n", i);
      return 1;
    }

    if (pixel_width < 12 && (ppu_cfg->out_p012 || ppu_cfg->out_I012)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: 12bit output is not supported for less 12 bit stream.\n", i);
      return 1;
    }

    if (ppu_cfg->dec400_enabled && ppu_cfg->norm_enable) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: PP DEC400 doesn't support normalization and quantization.\n", i);
      return 1;
    }

    if (ppu_cfg->dec400_enabled && ppu_cfg->rgb && (!IS_4PLAN_FORMAT(ppu_cfg->rgb_format) ||
       ppu_cfg->rgb_format == PP_OUT_R10G10B10A2 ||  ppu_cfg->rgb_format == PP_OUT_B10G10R10A2)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal RGB parameters with DEC400 enabled.\n", i);
      return 1;
    }
    if (ppu_cfg->dec400_conformance_test_en && !ppu_cfg->dec400_enabled) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: dec400 conformance test must enable dec400.\n", i);
      return 1;
    }
#if 0
    if (pixel_width == 12 && !ppu_cfg->out_p012 && !ppu_cfg->out_I012) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: pp 12bit input only can output 16bit.\n", i);
      return 1;
    }
#endif
    if ((ppu_cfg->rgb || ppu_cfg->rgb_planar)) {
      if (ppu_cfg->stream_pixel_width == 8 &&
         (ppu_cfg->rgb_format == PP_OUT_R16G16B16 ||
          ppu_cfg->rgb_format == PP_OUT_R16G16B16_P ||
          ppu_cfg->rgb_format == PP_OUT_A2R10G10B10 ||
          ppu_cfg->rgb_format == PP_OUT_A2B10G10R10 ||
          ppu_cfg->rgb_format == PP_OUT_X2R10G10B10 ||
          ppu_cfg->rgb_format == PP_OUT_X2B10G10R10 ||
          ppu_cfg->rgb_format == PP_OUT_R10G10B10A2 ||
          ppu_cfg->rgb_format == PP_OUT_B10G10R10A2 ||
          ppu_cfg->rgb_format == PP_OUT_B16G16R16   ||
          ppu_cfg->rgb_format == PP_OUT_B16G16R16_P)) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal RGB parameters. Input bitwidth conflict with output format bitwidth \n", i);
        return 1;
      }
    }
    if (ppu_cfg->rgb_planar) {
      if (ppu_cfg->rgb_format == PP_OUT_ARGB888 || ppu_cfg->rgb_format == PP_OUT_ABGR888 ||
          ppu_cfg->rgb_format == PP_OUT_RGBA888 || ppu_cfg->rgb_format == PP_OUT_BGRA888 ||
          ppu_cfg->rgb_format == PP_OUT_A2R10G10B10 || ppu_cfg->rgb_format == PP_OUT_A2B10G10R10 ||
          ppu_cfg->rgb_format == PP_OUT_X2R10G10B10 || ppu_cfg->rgb_format == PP_OUT_X2B10G10R10 ||
          ppu_cfg->rgb_format == PP_OUT_R10G10B10A2 || ppu_cfg->rgb_format == PP_OUT_B10G10R10A2 ||
          ppu_cfg->rgb_format == PP_OUT_XRGB888 || ppu_cfg->rgb_format == PP_OUT_XBGR888) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal RGB parameters. RGB planar conflict with XRGB/XBGR/RGBX/BGRX in output format\n", i);
        return 1;
      }
    }
    if (!ppu_cfg->dis_checker && ((ppu_cfg->crop.x & crop_align) ||
        (ppu_cfg->crop.y & crop_align) ||
        (ppu_cfg->crop.width & crop_align) ||
        (ppu_cfg->crop.height & crop_align) ||
      	(ppu_cfg->crop.x > PP_CROP_MAX_X) ||
        (ppu_cfg->crop.y > PP_CROP_MAX_Y) ||
        (ppu_cfg->crop.width  < PP_CROP_MIN_WIDTH && ppu_cfg->crop.width != in_width) ||
        (ppu_cfg->crop.height < PP_CROP_MIN_HEIGHT && ppu_cfg->crop.height != in_height) ||
        (ppu_cfg->crop.x + ppu_cfg->crop.width > in_width) ||
        (ppu_cfg->crop.y + ppu_cfg->crop.height > in_height))) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal cropping parameters.\n", i);
      return 1;
    }
    /*support odd crop*/
    if(in_chroma_format == PP_CHROMA_420 || in_chroma_format == PP_CHROMA_422 || in_chroma_format == PP_CHROMA_440 || in_chroma_format == PP_CHROMA_411){

      #ifdef SUPPORT_EVEN_CROP
      ppu_cfg->crop.x = ppu_cfg->crop.x & ~1;
      ppu_cfg->crop.y = ppu_cfg->crop.y & ~1;
      #endif
      if ((ppu_cfg->crop.x & 0x1) || (ppu_cfg->crop.y & 0x1)) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal cropping parameters, input YUV420 format or YUV422 format or YUV411/440 format must crop at even position.\n", i);
        return 1;
      }
    }
    /*if input RGB, it will set 'in_chroma_format = PP_CHROMA_444'*/
    if((in_chroma_format == PP_CHROMA_444) && !(ppu_cfg->rgb || ppu_cfg->rgb_planar) && !ppu_cfg->monochrome && (ppu_cfg->chroma_format == PP_CHROMA_420 || ppu_cfg->chroma_format == PP_CHROMA_422)){
      #ifdef SUPPORT_EVEN_CROP
      ppu_cfg->crop.x = ppu_cfg->crop.x & ~1;
      ppu_cfg->crop.y = ppu_cfg->crop.y & ~1;
      #endif
      if ((ppu_cfg->crop.x & 0x1) || (ppu_cfg->crop.y & 0x1)) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal cropping parameters, input YUV444 or RGB format and output YUV420 or YUV422 format must crop at even position.\n", i);
        return 1;
      }
    }

    //if (IS_PIC_422PACKED(ppu_cfg->out_format) && (ppu_cfg->scale.width & 0x1)){
    if ((ppu_cfg->out_yuyv || ppu_cfg->out_uyvy) && (ppu_cfg->scale.width & 0x1)){
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: output width should be even when output 422packed(YUYV/UYVY) format.\n", i);
      return 1;
    }


    if ((ppu_cfg->crop2.x & crop_align) ||
        (ppu_cfg->crop2.y & crop_align) ||
        (ppu_cfg->crop2.width & crop_align) ||
        (ppu_cfg->crop2.height & crop_align) ||
        (ppu_cfg->crop2.x + ppu_cfg->crop2.width > ppu_cfg->scale.width) ||
        (ppu_cfg->crop2.y + ppu_cfg->crop2.height > ppu_cfg->scale.height) ||
        (ppu_cfg->crop2.enabled && ((ppu_cfg->crop2.width < 48) || (ppu_cfg->scale.height < 48)))) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal cropping parameters.\n", i);
      return 1;
    }
    if (interlace && (ppu_cfg->crop.x % 2 || ppu_cfg->crop.y % 4 || ppu_cfg->crop.width % 2 || ppu_cfg->crop.height % 4)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal cropping parameters in interlace case, crop x and width must be even; crop y and height must be multiples of 4.\n", i);
      return 1;
    }
    if (interlace && (ppu_cfg->scale.width % 2 || ppu_cfg->scale.height % 2)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal output parameters, interlace case doesn't support odd size output.\n", i);
      return 1;
    }
    #if 0
    if (ppu_cfg->tiled_e && (ppu_cfg->crop.x % 2 || ppu_cfg->crop.y % 2 || ppu_cfg->crop.width % 2 || ppu_cfg->crop.height % 2)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal cropping parameters, tiled output doesn't support odd crop.\n", i);
      return 1;
    }
    if (ppu_cfg->tiled_e && (ppu_cfg->scale.width % 2 || ppu_cfg->scale.height % 2)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal output parameters, tiled output doesn't support odd size.\n", i);
      return 1;
    }
    #endif
    /* If cropping output greater thatn SCALE_IN_MAX size, no scaling. */
    if (ppu_cfg->crop.width  <= hw_feature->max_pp_out_pic_width[i] &&
        ppu_cfg->crop.height <= hw_feature->max_pp_out_pic_height[i]) {
      if (crop_align) {
        if (!ppu_cfg->scale.width || !ppu_cfg->scale.height ||
            (!hw_feature->uscale_support[i] &&
             (ppu_cfg->scale.width > ppu_cfg->crop.width ||
              ppu_cfg->scale.height > ppu_cfg->crop.height)) ||
            (!hw_feature->dscale_support[i] &&
             (ppu_cfg->scale.width < ppu_cfg->crop.width ||
              ppu_cfg->scale.height < ppu_cfg->crop.height)) ||
            ((ppu_cfg->scale.width  > MIN(hw_feature->max_pp_out_pic_width[i],  (hw_feature->pp_support_6x ? 6 : 3) * ppu_cfg->crop.width))) ||
            ((ppu_cfg->scale.height > MIN(hw_feature->max_pp_out_pic_height[i], (hw_feature->pp_support_6x ? 6 : 3) * ppu_cfg->crop.height))) ||
            (ppu_cfg->scale.width  & 1) ||
            (ppu_cfg->scale.height & (interlace ? 3 : 1)) ||
            (interlace && ((ppu_cfg->crop.y & 3) || (ppu_cfg->crop.height & 3))) /*||
            (ppu_cfg->scale.width  > ppu_cfg->crop.width &&
             ppu_cfg->scale.height < ppu_cfg->crop.height) ||
            (ppu_cfg->scale.width  < ppu_cfg->crop.width &&
             ppu_cfg->scale.height > ppu_cfg->crop.height)*/) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal scaling output parameters.\n", i);
          return 1;
        }
      } else {
        if (!ppu_cfg->scale.width || !ppu_cfg->scale.height ||
            (!hw_feature->uscale_support[i] &&
             (ppu_cfg->scale.width > ppu_cfg->crop.width ||
              ppu_cfg->scale.height > ppu_cfg->crop.height)) ||
            (!hw_feature->dscale_support[i] &&
             (ppu_cfg->scale.width < ppu_cfg->crop.width ||
              ppu_cfg->scale.height < ppu_cfg->crop.height)) ||
            ((ppu_cfg->scale.width  > MIN(hw_feature->max_pp_out_pic_width[i],  (hw_feature->pp_support_6x ? 6 : 3) * ppu_cfg->crop.width))) ||
            ((ppu_cfg->scale.height > MIN(hw_feature->max_pp_out_pic_height[i], (hw_feature->pp_support_6x ? 6 : 3) * ppu_cfg->crop.height))) /*||
            support odd crop
            (ppu_cfg->scale.width  & 1) ||
            (ppu_cfg->scale.height & (interlace ? 3 : 1))*/) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal scaling output parameters.\n", i);
          return 1;
        }
      }
    } else {
      if (crop_align) {
        if ((((ppu_cfg->crop.width != ppu_cfg->scale.width) || (ppu_cfg->crop.height != ppu_cfg->scale.height)) &&
            ((ppu_cfg->scale.width  > MIN(hw_feature->max_pp_out_pic_width[i],  (hw_feature->pp_support_6x ? 6 : 3) * ppu_cfg->crop.width)) ||
            (ppu_cfg->scale.height > MIN(hw_feature->max_pp_out_pic_height[i], (hw_feature->pp_support_6x ? 6 : 3) * ppu_cfg->crop.height)))) ||
            (ppu_cfg->scale.width  & 1) ||
            (ppu_cfg->scale.height & (interlace ? 3 : 1)) ||
            (interlace && ((ppu_cfg->crop.y & 3) || (ppu_cfg->crop.height & 3)))) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal scaling output parameters.\n", i);
          return 1;
        }
      } else {
        if ((((ppu_cfg->crop.width != ppu_cfg->scale.width) || (ppu_cfg->crop.height != ppu_cfg->scale.height)) &&
            ((ppu_cfg->scale.width  > MIN(hw_feature->max_pp_out_pic_width[i],  (hw_feature->pp_support_6x ? 6 : 3) * ppu_cfg->crop.width)) ||
            (ppu_cfg->scale.height > MIN(hw_feature->max_pp_out_pic_height[i], (hw_feature->pp_support_6x ? 6 : 3) * ppu_cfg->crop.height)))) /*||
            support odd crop
            (ppu_cfg->scale.width  & 1) ||
            (ppu_cfg->scale.height & (interlace ? 3 : 1))*/) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal scaling output parameters.\n", i);
          return 1;
        }
      }
    }

    if (hw_feature->pp_half_scale_support[i] &&
        (ppu_cfg->scale.ratio_x == 2) && (ppu_cfg->scale.ratio_y == 2)) {
      if (ppu_cfg->crop.width > hw_feature->max_pp_out_pic_width[i] ||
          ppu_cfg->crop.height > hw_feature->max_pp_out_pic_height[i]) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: half scale supported input max width is %d and input max height is %d.\n",
                    i, hw_feature->max_pp_out_pic_width[i], hw_feature->max_pp_out_pic_height[i]);
        return 1;
      }
      if ((ppu_cfg->crop.width & 1) ||
          (ppu_cfg->crop.height & 1) ||
          (ppu_cfg->scale.width & 1) ||
          (ppu_cfg->scale.height & 1)) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: half scale not support odd pp input size or odd pp output size.\n", i);
        return 1;
      }
    }

    // #if 0
    if (!ppu_cfg->dis_checker && ((ppu_cfg->scale.width && ppu_cfg->scale.width < 48) || (ppu_cfg->scale.height && ppu_cfg->scale.height < 48))
        && ppu_cfg->lz77_mode != 1) { //for LZ77 mode LZ77_UNIVERSAL_MODE,  dont need check resolution.
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal scaling output parameters, scale output width and height must be more than 48.\n", i);
      return 1;
    }
    // #endif

    if(IS_PP_OUT_PACKED10(ppu_cfg->out_format) && ppu_cfg->crop.width == ppu_cfg->scale.width && ppu_cfg->crop.height == ppu_cfg->scale.height) {
      if((ppu_cfg->scale.width  > hw_feature->max_pp_out_pic_width[i]) || (ppu_cfg->scale.height  > hw_feature->max_pp_out_pic_height[i])) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: Illegal output parameters.\n", i);
        return 1;
      }
    }

#if 0
    if ((ppu_cfg->crop.width > JPEG_PP_SCALE_IN_MAX_WIDTH || ppu_cfg->crop.height > JPEG_PP_SCALE_IN_MAX_HEIGHT) &&
        (ppu_cfg->scale.width != ppu_cfg->crop.width || ppu_cfg->scale.height != ppu_cfg->crop.height)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal scaling output parameters.\n", i);
      return 1;
    }
#endif
    if (((ppu_cfg->scale.width  > ppu_cfg->crop.width) || (ppu_cfg->scale.height > ppu_cfg->crop.height)) &&
       ((ppu_cfg->pp_filter == PP_VSI_LINEAR) || (ppu_cfg->pp_filter == PP_SPLINE) || (ppu_cfg->pp_filter == PP_BOX))) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal filter type, the filter type can not support upscale.\n", i);
      return 1;
    }

    if (hw_feature->scale_cost_support[i] < ULTRA_SCALER && (((FDIVI(TOFIX(ppu_cfg->crop.width, 16) + ppu_cfg->scale.width  / 2, ppu_cfg->scale.width)) > TOFIX(1, 21)) ||
        ((FDIVI(TOFIX(ppu_cfg->crop.height, 16) + ppu_cfg->scale.height  / 2, ppu_cfg->scale.height)) > TOFIX(1, 21)))) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: too large DS ratio due to max supported ratio is 32.\n", i);
      return 1;
    }
    if (ppu_cfg->tile_mode == RECONTILED8x8  || ppu_cfg->tile_mode == TILED8x8 || ppu_cfg->tile_mode == TILED64x64) {
      if (ppu_cfg->scale.width > 8192 || ppu_cfg->scale.height > 8192) {
        APITRACEERR("%s","CheckPpUnitConfig# Illegal param: TILED8x8 and TILED64x64 with output width or height should no more than 8192\n");
          return 1;
      }
    }


    if (ppu_cfg->tile_mode == TILED64x64 && ppu_cfg->rgb) {
      if (!IS_4PLAN_FORMAT(ppu_cfg->rgb_format)) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: TILED64x64 only support 4 plan RGB formats.\n", i);
        return 1;
      }
//      if (!(((ppu_cfg->scale.width == ppu_cfg->crop.width) || (ppu_cfg->scale.width == ppu_cfg->crop.width / 2)) &&
//            ((ppu_cfg->scale.height == ppu_cfg->crop.height) || (ppu_cfg->scale.height == ppu_cfg->crop.height / 2)))) {
//        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: TILED64x64 only support no scale or 2:1 scale.\n", i);
//        return 1;
//      }
      ppu_cfg->shaper_enabled = 0;
    }
#if 0
    if (ppu_cfg->tiled_e && (ppu_cfg->tile_mode != TILED128x2)) {
      if ((ppu_cfg->crop.x & 3) || (ppu_cfg->crop.width & 3) ||
          (ppu_cfg->crop.y & 3) || (ppu_cfg->crop.height & 3)) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: FBC cropping must align to 4.\n", i);
        return 1;
      }
      if ((ppu_cfg->tile_mode == TILED16x16) &&
          ((ppu_cfg->scale.width != ppu_cfg->crop.width) || (ppu_cfg->scale.height != ppu_cfg->crop.height))) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: FBC TILED16x16 only support no scale.\n", i);
        return 1;
      }
      if ((ppu_cfg->tile_mode == TILED8x8) &&
          (!(((ppu_cfg->scale.width == ppu_cfg->crop.width) || (ppu_cfg->scale.width == ppu_cfg->crop.width / 2)) &&
            ((ppu_cfg->scale.height == ppu_cfg->crop.height) || (ppu_cfg->scale.height == ppu_cfg->crop.height / 2))))) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: FBC TILED8x8 only support no scale or 2:1 scale.\n", i);
        return 1;
      }
      if (ppu_cfg->tile_mode && !hw_feature->fbc_support) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: FBC not support.\n", i);
        return 1;
      }
    }
#endif
    if (ppu_cfg->tiled_e && !ppu_cfg->tile_mode && ppu_cfg->planar) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: PLANAR is not supported with TILED4x4 output.\n", i);
      return 1;
    }
    if ((ppu_cfg->tile_mode == TILED128x2) && (ppu_cfg->scale.height & 3)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: TILED128x2, height must align to 4.\n", i);
      return 1;
    }
    if (ppu_cfg->tiled_e && !ppu_cfg->tile_mode && ppu_cfg->scale.width > hw_feature->max_pp_out_pic_width[i]) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: HW limit: TILED4x4 output size can't be bigger than PP max size.\n", i);
      return 1;
    }
    if ((ppu_cfg->tiled_e && (ppu_cfg->crop.width == ppu_cfg->scale.width) && (ppu_cfg->crop.height == ppu_cfg->scale.height)) &&
      ((ppu_cfg->scale.width  > hw_feature->max_pp_out_pic_width[i]) || (ppu_cfg->scale.height > hw_feature->max_pp_out_pic_height[i]) ) ) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal scaling output parameters when enable TILED output.\n", i);
      return 1;
    }

#if 0
    if (ppu_cfg->tiled_e && (ppu_cfg->scale.height%16) != 0) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: PP height must be 16 alignment with TILED4x4 output.\n", i);
      return 1;
    }
#endif

    if (ppu_cfg->pp_comp && (!hw_feature->pp_comp_support[i])) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Don't support enable PP compression.\n", i);
      return 1;
    }
    if (!(ppu_cfg->pp_comp && hw_feature->pp_comp_support[i] == 1) && (ppu_cfg->tile_mode == TILED16x16 || ppu_cfg->tile_mode == TILED32x8)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: When set '--tiled-mode' as TILED16x16/TILED32x8, must enable PFC compression.\n", i);
      return 1;
    }

    if (ppu_cfg->pp_comp && hw_feature->pp_comp_support[i] == 1) {
      if (!(ppu_cfg->tile_mode == TILED16x16 || ppu_cfg->tile_mode == TILED32x8)) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: When enable PFC compression, must set '--tiled-mode' as one of TILED16x16/TILED32x8.\n", i);
        return 1;
      }
      if(!ppu_cfg->shaper_enabled) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]:  --pp-shaper-dis conflic with AFBC.\n", i);
        return 1;
      }
      if (ppu_cfg->scale.width > 8192) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: FBC max width is 8192, cur width %d.\n", i, ppu_cfg->scale.width);
        return 1;
      }
      if (in_chroma_format == PP_CHROMA_400) {
        APITRACEERR("CheckPpUnitConfig# Illegal pp comp param[PP %u]: AFBC not support YUV400 case.\n", i);
          return 1;
      }

      if (hw_feature->scale_cost_support[i] == HIGH_SCALER) {
        if ((ppu_cfg->pp_filter == BICUBIC && ppu_cfg->antialias == 0) && ppu_cfg->src_sel_mode != NO_ROUND) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Channel that support both PFC and PFC compression with High 1/2 SCALE, BICUBIC(antilias=0) only support NO_ROUND mode for src_sel.\n", i);
          return 1;
        }
      }

      if (hw_feature->scale_cost_support[i] == MIDDLE_SCALER) {
        if ( (ppu_cfg->pp_filter == LANCZOS || (ppu_cfg->pp_filter == BICUBIC && ppu_cfg->antialias == 1)) && ppu_cfg->src_sel_mode == NO_ROUND) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Channel that support both PFC and PFC compression with Medium 1/2 SCALE, LANCZOS/BICUBIC(antialias=1) only support DOWN_ROUND/UP_ROUND mode for src_sel.\n", i);
          return 1;
        } else if(( (ppu_cfg->pp_filter == BI_LINEAR || ppu_cfg->antialias == 0) || ppu_cfg->pp_filter == BOX) && ppu_cfg->src_sel_mode != NO_ROUND) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Channel that support both PFC and PFC compression with Medium 1/2 SCALE, BILINEAR(antialias=0)/ BOX only support NO_ROUND mode for src_sel.\n", i);
          return 1;
        } else if( (ppu_cfg->pp_filter == BI_LINEAR && ppu_cfg->antialias == 1) || (ppu_cfg->pp_filter == BICUBIC && ppu_cfg->antialias == 0)) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Channel that support both PFC and PFC compression with Medium 1/2 SCALE, Don't support BILINEAR(antialias=1)/ BICUBC(antialias=0).\n", i);
          return 1;
        }
      }

      if (hw_feature->scale_cost_support[i] == ULTRA_SCALER) {
        if (( ppu_cfg->pp_filter == LANCZOS || ppu_cfg->pp_filter == BICUBIC) && ppu_cfg->src_sel_mode != DOWN_ROUND) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Channel that support both PFC and PFC compression with ULTRA 1/2 SCALE, LANCZOS/BICUBIC only support DOWN_ROUND mode for src_sel.\n", i);
          return 1;
        } else if(ppu_cfg->pp_filter == BI_LINEAR) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Channel that support both PFC and PFC compression with ULTRA 1/2 SCALE, Don't support BILINEAR.\n", i);
          return 1;
        }
      }

      if ((ppu_cfg->out_format == PP_OUT_FMT_YUV400 || ppu_cfg->out_format == PP_OUT_FMT_YUV400_P010 ||
          ppu_cfg->out_format == PP_OUT_FMT_YUV400_8BIT || ppu_cfg->out_format == PP_OUT_FMT_YUV400_P012)) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: PFC compression not support monochrome output format.\n", i);
        return 1;
      }

      if (ppu_cfg->crop2.enabled) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: PFC compression not support crop2.\n", i);
        return 1;
      }
      if (interlace) {
        APITRACEERR("%s","CheckPpUnitConfig# Illegal param: AFBC not support interlace frame\n");
        return 1;
      }
      if((ppu_cfg->scale.ratio_x == 2 || ppu_cfg->scale.ratio_y == 2) && ((ppu_cfg->crop.x & 7) || (ppu_cfg->crop.y & 7) ||
        (ppu_cfg->crop.width & 3) || (ppu_cfg->crop.height & 3))) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: PFC compression cropping [W %d]/[H %d] must align to 4. size must align to 4, when downscale 1/2 \n", i, ppu_cfg->crop.width, ppu_cfg->crop.height);
        return 1;
      }

      if((ppu_cfg->scale.ratio_x != 2 || ppu_cfg->scale.ratio_y != 2) && ((ppu_cfg->crop.x & 7) || (ppu_cfg->crop.y & 7))) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: PFC compression cropping x/y must align to 8. \n", i);
        return 1;
      }

      if(((ppu_cfg->scale.ratio_x > 1)  && (ppu_cfg->scale.ratio_x != 2)) || ((ppu_cfg->scale.ratio_y > 1)  && (ppu_cfg->scale.ratio_y != 2))) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: PFC compression only support downscale 1/2 \n", i);
        return 1;
      }

      if ((ppu_cfg->scale.width != ppu_cfg->crop.width) ||
          (ppu_cfg->scale.height != ppu_cfg->crop.height)) {
        if (((FDIVI(TOFIX(ppu_cfg->crop.width, 16) + ppu_cfg->scale.width / 2, ppu_cfg->scale.width)) != 0x20000) ||
            ((FDIVI(TOFIX(ppu_cfg->crop.height, 16) + ppu_cfg->scale.height / 2, ppu_cfg->scale.height)) != 0x20000)) {
          if ((ppu_cfg->crop.width % 4) || (ppu_cfg->crop.height % 4)) {
            APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Channel that support both PFC and PFC compression, crop size must align to 4.\n", i);
          } else {
            APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Channel that support both PFC and PFC compression, only support 1:1 and 1:2 dscale ratio.\n", i);
          }
          return 1;
        }
      }
    }

    if (ppu_cfg->tile_mode == TILED64x64 || (!hw_feature->pp_support_6x && (ppu_cfg->scale.width > ppu_cfg->crop.width*3 ||
        ppu_cfg->scale.height > ppu_cfg->crop.height*3))) {
      ppu_cfg->shaper_enabled = 0;
    }

    // checker for ringbuffer and flexa
    if (ppu_cfg->rb_en && !ppu_cfg->rb_mode && (ppu_cfg->rb_num <= 1 || ppu_cfg->rb_num >= 16)) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Not satisfy 1 < rb_num < 16 condition\n", i);
      return 1;
    }

    // if (ppu_cfg->rb_en && (ppu_cfg->rb_height < (u32)ceil((double)((16 + 8) * 1 + 2) / (double)16))) { // here only restrict h264+AREA+dowanscale
    //   // (max_ctb_height + luch_offset from In-Loop filter) * scale_hratio + Y direction luch_offset from pp scale filter
    //   APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: rb_height * 16 should >= pp output ctb height\n", i);
    //   return 1;
    // }

    if (ppu_cfg->sbi_en) {
      if (ppu_cfg->sbi_lu_segmentbuffer_offset >= ppu_cfg->sbi_lu_segmentbuffer_entry_count ||
        (!IS_PP_OUT_MONOCHROME(ppu_cfg->out_format) && ppu_cfg->sbi_ch_segmentbuffer_offset >= ppu_cfg->sbi_ch_segmentbuffer_entry_count)) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Sbi segment count must bigger than segment offset.\n", i);
        return 1;
      }
      if (ppu_cfg->sbi_lu_segmentbuffer_entry_size * ppu_cfg->sbi_lu_segmentbuffer_entry_count > 0xFFFFFFFF ||
          (!IS_PP_OUT_MONOCHROME(ppu_cfg->out_format) && (ppu_cfg->sbi_ch_segmentbuffer_entry_size * ppu_cfg->sbi_ch_segmentbuffer_entry_count > 0xFFFFFFFF))) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Sbi segment size * segment count out of range.\n", i);
        return 1;
      }
      if (ppu_cfg->sbi_lu_segmentbuffer_entry_count < 2 || ppu_cfg->sbi_lu_segmentbuffer_entry_count > 0xFFFF ||
          (!IS_PP_OUT_MONOCHROME(ppu_cfg->out_format) && (ppu_cfg->sbi_ch_segmentbuffer_entry_count < 2 || ppu_cfg->sbi_ch_segmentbuffer_entry_count > 0xFFFF))) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Sbi segment size * segment count out of range.\n", i);
        return 1;
      }
      if (!(IS_PP_OUT_MONOCHROME(ppu_cfg->out_format) || IS_PP_OUT_PACKED422(ppu_cfg->out_format)
      || (ppu_cfg->rgb && IS_PACKED_RGB(ppu_cfg->rgb_format))
      || (ppu_cfg->tiled_e && (!ppu_cfg->tile_mode || ppu_cfg->tile_mode == TILED2x4)))
      && ppu_cfg->sbi_lu_stream_id == ppu_cfg->sbi_ch_stream_id && !ppu_cfg->sbi_coupled) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Sbi chroma stream id should not equal to luma stream id.\n", i);
        return 1;
      }
      if (ppu_cfg->sbi_lu_stream_id > 0xFF || ppu_cfg->sbi_ch_stream_id > 0xFF) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Sbi stream id out of range.\n", i);
        return 1;
      }
      u32 sbi_lu_segment_size = ppu_cfg->sbi_lu_segmentbuffer_entry_size * ppu_cfg->ystride * 16;
      u32 segment_height = sbi_lu_segment_size / ppu_cfg->ystride / 16;
      if (segment_height > 4096) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Sbi segment height out of range.\n", i);
        return 1;
      }
    }

    if(ppu_cfg->slice_text.slice_txt_cnt > 0 && i == 0) {
      for (int i = 0; i < 4; i++)
      {
        for (int j = 0;j < ppu_cfg->slice_text.slice_txt_cnt; j++) {
          if(ppu_cfg->slice_text.slice_params[i].offset[j] > 255) {
            APITRACEERR("%s","CheckPpUnitConfig# Illegal param: The offset of any slice txt should not be more than 255.\n");
            return 1;
          }
        }
      }
    }

    /* low cost scale */
    if ((!hw_feature->scale_cost_support[i] && !hw_feature->only_area_scale_alg_support[i]) && ppu_cfg->pp_filter == AREA) {
      APITRACEERR("CheckPpUnitConfig# Illegal param[PP %u]: Illegal DOWNSCALE filter type in this HwBuildId, AREA can not support, please check POF \n", i);
      return 1;
    }

    if (hw_feature->scale_cost_support[i] == MIDDLE_SCALER) {
      u32 input_h = ppu_cfg->crop.height;
      u32 input_w = ppu_cfg->crop.width;
      u32 output_h = ppu_cfg->scale.height;
      u32 output_w = ppu_cfg->scale.width;
      if ((input_h > output_h && input_w > output_w) || (input_h == output_h && input_w > output_w) || (input_h > output_h && input_w == output_w) ) { /* DOWNSCALE */
        if (ppu_cfg->pp_filter == VSI_LINEAR || ppu_cfg->pp_filter == LANCZOS || ppu_cfg->pp_filter == SPLINE || ppu_cfg->pp_filter == BOX) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: Illegal DOWNSCALE filter type in low cost alg, please indicate --pp-filter= with NEAREST/BILINEAR/BICUBIC/AREA \n", i);
          return 1;
        }
        if (ppu_cfg->pp_filter == AREA && ppu_cfg->antialias == 0) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: Illegal DOWNSCALE filter type in low cost alg, AREA can not support antialias=0 \n", i);
          return 1;
        }
        if (ppu_cfg->pp_filter == BI_LINEAR && ppu_cfg->antialias == 1) {
          ppu_cfg->pp_filter = FAST_LINEAR;
        }
        if (ppu_cfg->pp_filter == BICUBIC && ppu_cfg->antialias == 1) {
          ppu_cfg->pp_filter = FAST_BICUBIC;
        }
      } else if ((input_h < output_h && input_w < output_w) || (input_h == output_h && input_w < output_w) || (input_h < output_h && input_w == output_w)) { /* UPSCALE */
        if (ppu_cfg->pp_filter == VSI_LINEAR || ppu_cfg->pp_filter == LANCZOS || ppu_cfg->pp_filter == SPLINE
          || ppu_cfg->pp_filter == BOX || ppu_cfg->pp_filter == AREA) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: Illegal UPSCALE filter type in low cost alg, please indicate --pp-filter= with NEAREST/BILINEAR(antialias=0)/BICUBIC(antialias=0) \n", i);
          return 1;
        }
        if (ppu_cfg->pp_filter == BI_LINEAR && ppu_cfg->antialias == 1) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: Illegal UPSCALE filter type in low cost alg, BILINEAR can not support antialias=1 \n", i);
          return 1;
        }
        if (ppu_cfg->pp_filter == BICUBIC && ppu_cfg->antialias == 1) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: Illegal UPSCALE filter type in low cost alg, BICUBIC can not support antialias=1 \n", i);
          return 1;
        }
      } else if ((input_h > output_h && input_w < output_w) || (input_h < output_h && input_w > output_w)) {
        if (ppu_cfg->pp_filter == VSI_LINEAR || ppu_cfg->pp_filter == LANCZOS || ppu_cfg->pp_filter == SPLINE
          || ppu_cfg->pp_filter == BOX || ppu_cfg->pp_filter == AREA) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: Illegal filter type in low cost alg, UPSCALE and DOWNSCALE at the same time, please indicate --pp-filter= with NEAREST/BILINEAR/BICUBIC \n", i);
          return 1;
        }
        if (ppu_cfg->pp_filter == BI_LINEAR && ppu_cfg->antialias == 1) {
          ppu_cfg->pp_filter = FAST_LINEAR;
        }
        if (ppu_cfg->pp_filter == BICUBIC && ppu_cfg->antialias == 1) {
          ppu_cfg->pp_filter = FAST_BICUBIC;
        }
      }
    }

    if(hw_feature->scale_cost_support[i] == ULTRA_SCALER) {

      u32 input_h = ppu_cfg->crop.height;
      u32 input_w = ppu_cfg->crop.width;
      u32 output_h = ppu_cfg->scale.height;
      u32 output_w = ppu_cfg->scale.width;

      if(hw_feature->pp_area_optimize == 1) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: Illegal feature configuration in ULTRA SCALE, only support pp_area_optimize = 0\n", i);
        return 1;
      }

      if (ppu_cfg->pp_filter == VSI_LINEAR || ppu_cfg->pp_filter == NEAREST || ppu_cfg->pp_filter == SPLINE || ppu_cfg->pp_filter == BOX || ppu_cfg->pp_filter == AREA) {
        APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: Illegal filter type in ULTRA SCALE , Only Support --pp-filter= with LANCZOS/BILINEAR/BICUBIC \n", i);
        return 1;
      }

      if ((input_h > output_h && input_w > output_w) || (input_h == output_h && input_w > output_w) || (input_h > output_h && input_w == output_w) ) { /* DOWNSCALE */
        if(ppu_cfg->antialias == 0) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: Illegal --antialias option in DOWNSCALER ULTRA SCALER. ULTRA SCALE DOWNSCALER only support -- antialia=1 \n", i);
          return 1;
        }
        if (ppu_cfg->src_sel_mode != DOWN_ROUND) {
          APITRACEERR("CheckPpUnitConfig# Illegal param[PP %d]: Illegal --src_sel_mode option in ULTRA SCALE. ULTRA SCALE only support --pp-src-sel=DOWN_ROUND\n", i);
          return 1;
        }
      }
    }
  }
  InitPpUnitLanczosData(hw_feature, ppu_cfg_temp);
  if (ppu_cfg_temp->heif_mode) {
    for (i=0; i<DEC_MAX_PPU_COUNT; i++) {
      if (!ppu_cfg_temp[i].enabled) continue;

      ppu_cfg_temp[i].pre_lanczos_info.crop_w = ppu_cfg_temp[i].crop.width;
      ppu_cfg_temp[i].pre_lanczos_info.crop_h = ppu_cfg_temp[i].crop.height;
      ppu_cfg_temp[i].pre_lanczos_info.scale_w = ppu_cfg_temp[i].scale.width;
      ppu_cfg_temp[i].pre_lanczos_info.scale_h = ppu_cfg_temp[i].scale.height;
      ppu_cfg_temp[i].pre_lanczos_info.pp_filter = ppu_cfg_temp[i].pp_filter;
    }
  }

  return 0; //return DEC_OK;
}

void PPSetSliceTextParams(u32 *pp_regs, struct DecSliceTextInfo *slice_text) {

  SetPpuRegister(pp_regs, HWIF_PPX_LC_TXT_E_U, 0, slice_text->slice_txt_cnt > 0);
  SetPpuRegister(pp_regs, HWIF_PPX_LC_TXT_SEL_U, 0, 0);

  /*slice 0*/
#define SET_PP_SLICE_PARAMS(n) do { \
    SetPpuRegister(pp_regs, HWIF_PPX_LC_CNT##n##_U, 0, slice_text->slice_params[n].lines); \
    SetPpuRegister(pp_regs, HWIF_PPX_LC_CNT##n##_TXT0_U, 0, slice_text->slice_params[n].text[0]); \
    SetPpuRegister(pp_regs, HWIF_PPX_LC_CNT##n##_TXT1_U, 0, slice_text->slice_params[n].text[1]); \
    SetPpuRegister(pp_regs, HWIF_PPX_LC_CNT##n##_TXT2_U, 0, slice_text->slice_params[n].text[2]); \
    SetPpuRegister(pp_regs, HWIF_PPX_LC_CNT##n##_TXT3_U, 0, slice_text->slice_params[n].text[3]); \
    SetPpuRegister(pp_regs, HWIF_PPX_LC_CNT##n##_OFFSET0_U, 0, slice_text->slice_params[n].offset[0]); \
    SetPpuRegister(pp_regs, HWIF_PPX_LC_CNT##n##_OFFSET1_U, 0, slice_text->slice_params[n].offset[1]); \
    SetPpuRegister(pp_regs, HWIF_PPX_LC_CNT##n##_OFFSET2_U, 0, slice_text->slice_params[n].offset[2]); \
    SetPpuRegister(pp_regs, HWIF_PPX_LC_CNT##n##_OFFSET3_U, 0, slice_text->slice_params[n].offset[3]); \
    SetPpuRegister(pp_regs, HWIF_PPX_LC_CNT##n##_BASE_U_LSB, 0, ((u32)slice_text->slice_params[n].text_base)); \
    SetPpuRegister(pp_regs, HWIF_PPX_LC_CNT##n##_BASE_U_MSB, 0, ((u32)(slice_text->slice_params[n].text_base >> 32))); \
  } while (0)

  SET_PP_SLICE_PARAMS(0);
  SET_PP_SLICE_PARAMS(1);
  SET_PP_SLICE_PARAMS(2);
  SET_PP_SLICE_PARAMS(3);

}

void PPSetOneChannelRegs(u32 *pp_regs, const struct PpParams *pp_args, u32 channel_id) {
  u32 i = channel_id;
  u32 ppb_mode = DEC_DPB_FRAME;
  u32 ppw, ppw_c;
  u32 pp_field_offset = 0;
  u32 pp_field_offset_ch = 0;
  u32 coversion_type = 0;
  const struct DecHwFeatures *p_hw_feature = pp_args->p_hw_feature;
  PpUnitIntConfig *ppu_cfg = &pp_args->ppu_cfg[channel_id];
  addr_t ppu_out_bus_addr = pp_args->ppu_out_addr->bus_address;
  u8 *ppu_out_vir_addr = (u8 *)pp_args->ppu_out_addr->virtual_address;
  u32 bottom_field_flag = pp_args->bottom_field_flag;
#ifndef MODEL_SIMULATION
  const struct DecHwFeatures *hw_feature = p_hw_feature;
#endif
  /* pp common registers*/
  if (!IS_PP_IN_RGB(ppu_cfg->pp_in_format) && (ppu_cfg->rgb || ppu_cfg->rgb_planar)) {
    /* default video_range is 1 (full) and rgb_stan is BT601,
    after overwritten video_range by stream info (get from sps),
    here rgb_stan should also change */
    if (ppu_cfg->video_range == 0 && IS_FULL_RANGE(ppu_cfg->rgb_stan)) {
      ppu_cfg->rgb_stan += 1;
    } else if (ppu_cfg->video_range == 1 && !(IS_FULL_RANGE(ppu_cfg->rgb_stan))) {
      ppu_cfg->rgb_stan -= 1;
    }

    if(p_hw_feature->pp_standalone) {
      SetDecRegister(pp_regs, HWIF_PP_COLOR_COEFFA2_U, coeff[ppu_cfg->rgb_stan][0]);
      SetDecRegister(pp_regs, HWIF_PP_COLOR_COEFFA1_U, coeff[ppu_cfg->rgb_stan][0]);
      SetDecRegister(pp_regs, HWIF_PP_COLOR_COEFFB_U, coeff[ppu_cfg->rgb_stan][1]);
      SetDecRegister(pp_regs, HWIF_PP_COLOR_COEFFC_U, coeff[ppu_cfg->rgb_stan][2]);
      SetDecRegister(pp_regs, HWIF_PP_COLOR_COEFFD_U, coeff[ppu_cfg->rgb_stan][3]);
      SetDecRegister(pp_regs, HWIF_PP_COLOR_COEFFE_U, coeff[ppu_cfg->rgb_stan][4]);
    } else {
      if(ppu_cfg->rgb_stan == BT601 || ppu_cfg->rgb_stan == BT601_L) {
        coversion_type = BT601_TYPE;
      } else if(ppu_cfg->rgb_stan == BT709 || ppu_cfg->rgb_stan == BT709_L) {
        coversion_type = BT709_TYPE;
      } else if (ppu_cfg->rgb_stan == BT2020 || ppu_cfg->rgb_stan == BT2020_L) {
        coversion_type = BT2020_TYPE;
      }
      SetDecRegister(pp_regs, HWIF_COLOR_CONVERSION_TYPE_U, coversion_type);
    }
    SetDecRegister(pp_regs, HWIF_PP_VIDEO_RANGE_U, ppu_cfg->video_range);
    SetDecRegister(pp_regs, HWIF_PP_RGB_RANGE_MAX_U, ppu_cfg->range_max);
    SetDecRegister(pp_regs, HWIF_PP_RGB_RANGE_MIN_U, ppu_cfg->range_min);
  }

  SetDecRegister(pp_regs, HWIF_PP_IN_ORG_WIDTH_U, ppu_cfg->pp_in_org_width);
  SetDecRegister(pp_regs, HWIF_PP_IN_ORG_HEIGHT_U, ppu_cfg->pp_in_org_height);

  /* Set SBI register. */
  PPSetSBIRegs(pp_regs, i, ppu_cfg, pp_args->sbi_pic_start);

  /* Set ringbuffer register. */
  if (ppu_cfg->rb_en) {
    SetPpuRegister(pp_regs, HWIF_PP_OUT_RB_EN_U, i, ppu_cfg->rb_en);
    SetPpuRegister(pp_regs, HWIF_PP_OUT_SEGMENT_ENTRY_HEIGHT_U, i, ppu_cfg->rb_height);
    if (!ppu_cfg->rb_mode)
      SetPpuRegister(pp_regs, HWIF_PP_OUT_SEGMENT_ENTRY_NUM_U, i, ppu_cfg->rb_num);
    SetPpuRegister(pp_regs, HWIF_PP_OUT_RB_MODE_U, i, ppu_cfg->rb_mode);

    if (ppu_cfg->rb_mode) {
      ppu_cfg->rb_num = (u32)ceil((double)(ppu_cfg->scale.height) / (double)((ppu_cfg->rb_height) * 16));
    }

    u32 lu_rb_buf_size = 16 * ppu_cfg->rb_height * ppu_cfg->ystride * ppu_cfg->rb_num;
    u32 ch_rb_buf_size = 16 * ppu_cfg->rb_height * ppu_cfg->cstride * ppu_cfg->rb_num;

    if (!ppu_cfg->rb_mode) {
      if (IS_PP_OUT_420(ppu_cfg->out_format)) {
        SetPpuRegister(pp_regs, HWIF_PP_OUT_LU_SEGMENT_BUF_SIZE_U, i, lu_rb_buf_size);
        SetPpuRegister(pp_regs, HWIF_PP_OUT_CH_SEGMENT_BUF_SIZE_U, i, ch_rb_buf_size / 2); // 420 half height
      } else if (IS_PP_OUT_422(ppu_cfg->out_format)) {
        SetPpuRegister(pp_regs, HWIF_PP_OUT_LU_SEGMENT_BUF_SIZE_U, i, lu_rb_buf_size);
        SetPpuRegister(pp_regs, HWIF_PP_OUT_CH_SEGMENT_BUF_SIZE_U, i, ch_rb_buf_size);
      } else if (IS_PP_OUT_MONOCHROME(ppu_cfg->out_format)) {
        SetPpuRegister(pp_regs, HWIF_PP_OUT_LU_SEGMENT_BUF_SIZE_U, i, lu_rb_buf_size);
        SetPpuRegister(pp_regs, HWIF_PP_OUT_CH_SEGMENT_BUF_SIZE_U, i, 0);
      }
    }

  #ifndef MODEL_SIMULATION
    ppu_cfg->chroma_offset = lu_rb_buf_size;
    // store ringbuffer small buff size in HWIF_PPX_OUT_CH_BASE for FPGA
  #endif
  }

  /*set norm registers*/
  if (ppu_cfg->norm_enable) {
    if (ppu_cfg->norm_filter == ZSCORE) {
      SetDecRegister(pp_regs, HWIF_PP_NORM_SCALE_FACTOR, FP32TOU32(1.0));
    } else if (ppu_cfg->norm_filter == VSI_ZSCORE) {
      u32 pp_in_bit_depth = 8, pp_out_bit_depth = 0;
      if (IS_PP_IN_10BIT(ppu_cfg->pp_in_format)) {
        pp_in_bit_depth = 10;
      }
      pp_out_bit_depth = FP32TOU32(1.0 / ((1<< pp_in_bit_depth) -1));
      SetDecRegister(pp_regs, HWIF_PP_NORM_SCALE_FACTOR, pp_out_bit_depth);
    }
    SetDecRegister(pp_regs, HWIF_PP_NORM_TYPE_U, ppu_cfg->norm_filter);

    SetDecRegister(pp_regs, HWIF_PP_NORM_R_MEAN_VALUE, ppu_cfg->norm_mean_r);
    SetDecRegister(pp_regs, HWIF_PP_NORM_G_MEAN_VALUE, ppu_cfg->norm_mean_g);
    SetDecRegister(pp_regs, HWIF_PP_NORM_B_MEAN_VALUE, ppu_cfg->norm_mean_b);
    SetDecRegister(pp_regs, HWIF_PP_NORM_R_STD_VALUE, ppu_cfg->norm_std_r);
    SetDecRegister(pp_regs, HWIF_PP_NORM_G_STD_VALUE, ppu_cfg->norm_std_g);
    SetDecRegister(pp_regs, HWIF_PP_NORM_B_STD_VALUE, ppu_cfg->norm_std_b);

    SetPpuRegister(pp_regs, HWIF_PPX_NORM_EN_U, i, ppu_cfg->norm_enable);
    SetPpuRegister(pp_regs, HWIF_PPX_NORM_OUT_FORMAT_U, i, ppu_cfg->norm_fmat);
    SetPpuRegister(pp_regs, HWIF_PPX_NORM_QUANT_STEP_U, i, ppu_cfg->norm_quant_step);
  }

  /* for color remapping (3dlut) */
  if (ppu_cfg->enable_3dlut && (ppu_cfg->rgb || ppu_cfg->rgb_planar)) {
    SetPpuRegister(pp_regs, HWIF_PPX_3DLUT_ENABLE_U, i, ppu_cfg->enable_3dlut);
    SET_PP_ADDR_REG2(pp_regs,
                    HWIF_PP_3DLUT_TBL_BASE_U_LSB,
                    HWIF_PP_3DLUT_TBL_BASE_U_MSB,
                    0,
                    ppu_cfg->table_3dlut_buffer.bus_address);
  }

  /* range mapping */
  SetPpuRegister(pp_regs, HWIF_PPX_RANGE_MAP_FLAG_U, i, ppu_cfg->range_map_flag);
  if (ppu_cfg->range_map_flag) {
    if (ppu_cfg->range_map_flag == 2) { // full to limited
      if (ppu_cfg->stream_pixel_width == 8) {
        SetPpuRegister(pp_regs, HWIF_PPX_RANGE_MAP_COEFF1_U, i, coeff[6][0]); // for luma/R/G/B
        SetPpuRegister(pp_regs, HWIF_PPX_RANGE_MAP_COEFF2_U, i, coeff[6][1]); // for chroma
        SetPpuRegister(pp_regs, HWIF_PPX_RANGE_MAX_U, i, ppu_cfg->range_map_max);
        SetPpuRegister(pp_regs, HWIF_PPX_RANGE_MIN_U, i, ppu_cfg->range_map_min);
      } else { // ppu_cfg->stream_pixel_width == 10
        SetPpuRegister(pp_regs, HWIF_PPX_RANGE_MAP_COEFF1_U, i, coeff[8][0]);
        SetPpuRegister(pp_regs, HWIF_PPX_RANGE_MAP_COEFF2_U, i, coeff[8][1]);
        SetPpuRegister(pp_regs, HWIF_PPX_RANGE_MAX_U, i, ppu_cfg->range_map_max);
        SetPpuRegister(pp_regs, HWIF_PPX_RANGE_MIN_U, i, ppu_cfg->range_map_min);
      }
    } else { // ppu_cfg->range_map_flag == 1 limited to full
      if (ppu_cfg->stream_pixel_width == 8) {
        SetPpuRegister(pp_regs, HWIF_PPX_RANGE_MAP_COEFF1_U, i, coeff[7][0]);
        SetPpuRegister(pp_regs, HWIF_PPX_RANGE_MAP_COEFF2_U, i, coeff[7][1]);
        SetPpuRegister(pp_regs, HWIF_PPX_RANGE_MAX_U, i, ppu_cfg->range_map_max);
        SetPpuRegister(pp_regs, HWIF_PPX_RANGE_MIN_U, i, ppu_cfg->range_map_min);
      } else { // ppu_cfg->stream_pixel_width == 10
        SetPpuRegister(pp_regs, HWIF_PPX_RANGE_MAP_COEFF1_U, i, coeff[9][0]);
        SetPpuRegister(pp_regs, HWIF_PPX_RANGE_MAP_COEFF2_U, i, coeff[9][1]);
        SetPpuRegister(pp_regs, HWIF_PPX_RANGE_MAX_U, i, ppu_cfg->range_map_max);
        SetPpuRegister(pp_regs, HWIF_PPX_RANGE_MIN_U, i, ppu_cfg->range_map_min);
      }
    }
  }

  SetPpuRegister(pp_regs, HWIF_PPX_DITHER_U, i, ppu_cfg->dither_enable);

  SET_ADDR_REG(pp_regs, HWIF_PP_REORDER_TILE_WR_BASE_U, ppu_cfg->reorder_buf_bus[0]);

  SetPpuRegister(pp_regs, HWIF_PPX_CR_FIRST, i, ppu_cfg->cr_first);
  SetPpuRegister(pp_regs, HWIF_PPX_OUT_FORMAT_U, i, ppu_cfg->out_format);
  SetPpuRegister(pp_regs, HWIF_PPX_OUT_RGB_FMT_U, i, ppu_cfg->rgb_format);
  SetPpuRegister(pp_regs, HWIF_PPX_RGB_PLANAR_U, i, ppu_cfg->rgb_planar);
  SetPpuRegister(pp_regs, HWIF_PPX_OUT_ALPHA_U, i, ppu_cfg->rgb_alpha);
  SetPpuRegister(pp_regs, HWIF_PPX_SRC_SEL_MODE_U, i, ppu_cfg->src_sel_mode);
  SetPpuRegister(pp_regs, HWIF_PPX_SHAPER_DIS_U, i, !ppu_cfg->shaper_enabled);
  SetPpuRegister(pp_regs, HWIF_PPX_SHAPER_PAD_E_U, i, !ppu_cfg->shaper_no_pad);
  SetPpuRegister(pp_regs, HWIF_PPX_SHAPER_TILE_PAD_E_U, i, ppu_cfg->shaper_tile_edge_pad);
  SetPpuRegister(pp_regs, HWIF_PPX_X_PHASE_NUM_U, i, (ppu_cfg->x_phase_num + 1));
  SetPpuRegister(pp_regs, HWIF_PPX_Y_PHASE_NUM_U, i, (ppu_cfg->y_phase_num + 1));
  SetPpuRegister(pp_regs, HWIF_PPX_OUT_P010_FMT_U, i, (ppu_cfg->out_L010 ? 2 :
                                                    ((ppu_cfg->out_I010 || ppu_cfg->out_I012) ? 1 : 0)));
  SetPpuRegister(pp_regs, HWIF_PPX_LINE_CNT_STRIPE_U, i, ppu_cfg->lc_stripe);
  SetPpuRegister(pp_regs, HWIF_PPX_LINE_CNT_E_U, i, ppu_cfg->lc_stripe > 0);

  {
    /* flexible scale ratio */
    if ((ppu_cfg->scale.width  > ppu_cfg->crop.width) || (ppu_cfg->scale.height > ppu_cfg->crop.height))
      calSecondUpScaleRatio(p_hw_feature, ppu_cfg, GetDecRegister(pp_regs, HWIF_PIC_FIELDMODE_E));
    u32 in_width = ppu_cfg->crop.width;
    u32 in_height = ppu_cfg->crop.height;
    u32 out_width = ppu_cfg->scale.width;
    u32 out_height = ppu_cfg->scale.height;
    u32 out_ratio_x = 1;
    u32 out_ratio_y = 1;
    if ((in_width < out_width) || (in_height < out_height)) {
      out_width = ppu_cfg->out_width;
      out_height = ppu_cfg->out_height;
      out_ratio_x = ppu_cfg->out_ratio_x;
      out_ratio_y = ppu_cfg->out_ratio_y;
    }
    ppw = ppu_cfg->ystride;
    ppw_c = ppu_cfg->cstride;

    SetPpuRegister(pp_regs, HWIF_PPX_CROP_STARTX_U, i,
                    ppu_cfg->crop.x >> p_hw_feature->crop_step_rshift);
    SetPpuRegister(pp_regs, HWIF_PPX_CROP_STARTY_U, i,
                    ppu_cfg->crop.y >> p_hw_feature->crop_step_rshift);
    SetPpuRegister(pp_regs, HWIF_PPX_IN_WIDTH_U, i,
                    ppu_cfg->crop.width >> p_hw_feature->crop_step_rshift);
    SetPpuRegister(pp_regs, HWIF_PPX_IN_HEIGHT_U, i,
                    ppu_cfg->crop.height >> p_hw_feature->crop_step_rshift);

    if (ppu_cfg->crop2.enabled) {
      SetPpuRegister(pp_regs, HWIF_PPX_CROP2_STARTX_U, i,
                      ppu_cfg->crop2.x >> p_hw_feature->crop_step_rshift);
      SetPpuRegister(pp_regs, HWIF_PPX_CROP2_STARTY_U, i,
                      ppu_cfg->crop2.y >> p_hw_feature->crop_step_rshift);
      SetPpuRegister(pp_regs, HWIF_PPX_CROP2_OUT_WIDTH_U, i,
                      ppu_cfg->crop2.width >> p_hw_feature->crop_step_rshift);
      SetPpuRegister(pp_regs, HWIF_PPX_CROP2_OUT_HEIGHT_U, i,
                      ppu_cfg->crop2.height >> p_hw_feature->crop_step_rshift);
    } else {
      SetPpuRegister(pp_regs, HWIF_PPX_CROP2_STARTX_U, i, 0);
      SetPpuRegister(pp_regs, HWIF_PPX_CROP2_STARTY_U, i, 0);
      SetPpuRegister(pp_regs, HWIF_PPX_CROP2_OUT_WIDTH_U, i,
                      ppu_cfg->scale.width >> p_hw_feature->crop_step_rshift);
      SetPpuRegister(pp_regs, HWIF_PPX_CROP2_OUT_HEIGHT_U, i,
                      ppu_cfg->scale.height >> p_hw_feature->crop_step_rshift);
    }

    SetPpuRegister(pp_regs, HWIF_PPX_OUT_WIDTH_U, i,
                    ppu_cfg->scale.width);
    SetPpuRegister(pp_regs, HWIF_PPX_OUT_HEIGHT_U, i,
                    ppu_cfg->scale.height);

    /*support low-cost scale */
    u32 area_enable = 0;
    if(p_hw_feature->scale_cost_support[i] == 1) {
      if (ppu_cfg->pp_filter == PP_NEAREST) {
        SetPpuRegister(pp_regs, HWIF_PPX_HOR_ALG_FLAG_U, i, 0);
        SetPpuRegister(pp_regs, HWIF_PPX_VER_ALG_FLAG_U, i, 0);
      } else if (ppu_cfg->pp_filter == PP_FAST_LINEAR) {
        SetPpuRegister(pp_regs, HWIF_PPX_HOR_ALG_FLAG_U, i, 1);
        SetPpuRegister(pp_regs, HWIF_PPX_VER_ALG_FLAG_U, i, 1);
      } else if (ppu_cfg->pp_filter == PP_FAST_BICUBIC) {
        SetPpuRegister(pp_regs, HWIF_PPX_HOR_ALG_FLAG_U, i, 2);
        SetPpuRegister(pp_regs, HWIF_PPX_VER_ALG_FLAG_U, i, 2);
      } else if (ppu_cfg->pp_filter == AREA) {
        SetPpuRegister(pp_regs, HWIF_PPX_HOR_ALG_FLAG_U, i, 3);
        SetPpuRegister(pp_regs, HWIF_PPX_VER_ALG_FLAG_U, i, 3);
      }
    } else if (p_hw_feature->only_area_scale_alg_support[i]) {
      SetPpuRegister(pp_regs, HWIF_PPX_HOR_ALG_FLAG_U, i, 0);
      SetPpuRegister(pp_regs, HWIF_PPX_VER_ALG_FLAG_U, i, 3);
    } else {
      if(ppu_cfg->pp_filter == PP_NEAREST) {
        SetPpuRegister(pp_regs, HWIF_PPX_HOR_ALG_FLAG_U, i, 0);
        SetPpuRegister(pp_regs, HWIF_PPX_VER_ALG_FLAG_U, i, 0);
      } else if (ppu_cfg->pp_filter == PP_FAST_LINEAR) {
        SetPpuRegister(pp_regs, HWIF_PPX_HOR_ALG_FLAG_U, i, 0);
        SetPpuRegister(pp_regs, HWIF_PPX_VER_ALG_FLAG_U, i, 1);
      } else if (ppu_cfg->pp_filter == PP_FAST_BICUBIC) {
        SetPpuRegister(pp_regs, HWIF_PPX_HOR_ALG_FLAG_U, i, 0);
        SetPpuRegister(pp_regs, HWIF_PPX_VER_ALG_FLAG_U, i, 2);
      } else if (ppu_cfg->pp_filter == PP_AREA) {
        SetPpuRegister(pp_regs, HWIF_PPX_HOR_ALG_FLAG_U, i, 0);
        SetPpuRegister(pp_regs, HWIF_PPX_VER_ALG_FLAG_U, i, 3);
      } else if (ppu_cfg->pp_filter == PP_LANCZOS) {
        SetPpuRegister(pp_regs, HWIF_PPX_HOR_ALG_FLAG_U, i, 1);
        SetPpuRegister(pp_regs, HWIF_PPX_VER_ALG_FLAG_U, i, 0);
      } else if (ppu_cfg->pp_filter == PP_BILINEAR) {
        SetPpuRegister(pp_regs, HWIF_PPX_HOR_ALG_FLAG_U, i, 1);
        SetPpuRegister(pp_regs, HWIF_PPX_VER_ALG_FLAG_U, i, 1);
      } else if (ppu_cfg->pp_filter == PP_BICUBIC) {
        SetPpuRegister(pp_regs, HWIF_PPX_HOR_ALG_FLAG_U, i, 1);
        SetPpuRegister(pp_regs, HWIF_PPX_VER_ALG_FLAG_U, i, 2);
      }  else if (ppu_cfg->pp_filter == PP_VSI_LINEAR) {
        SetPpuRegister(pp_regs, HWIF_PPX_HOR_ALG_FLAG_U, i, 1);
        SetPpuRegister(pp_regs, HWIF_PPX_VER_ALG_FLAG_U, i, 3);
      }
    }

    /* hist */
    if (ppu_cfg->hist_global_e) {
      SetPpuRegister(pp_regs, HWIF_PPX_HIST_GLOBAL_LUMA_EN_U, i, 1);
    } else {
      SetPpuRegister(pp_regs, HWIF_PPX_HIST_GLOBAL_LUMA_EN_U, i, 0);
    }

    if (ppu_cfg->hist_local_e){
        SetPpuRegister(pp_regs, HWIF_PPX_HIST_LOCAL_LUMA_EN_U, i, 1);
        SetPpuRegister(pp_regs, HWIF_PPX_HIST_LOCAL_LUMA_X1_OFFSET_U, i, ppu_cfg->hist_offset_x[0]);
        SetPpuRegister(pp_regs, HWIF_PPX_HIST_LOCAL_LUMA_X2_OFFSET_U, i, ppu_cfg->hist_offset_x[1]);
        SetPpuRegister(pp_regs, HWIF_PPX_HIST_LOCAL_LUMA_X3_OFFSET_U, i, ppu_cfg->hist_offset_x[2]);
        SetPpuRegister(pp_regs, HWIF_PPX_HIST_LOCAL_LUMA_X4_OFFSET_U, i, ppu_cfg->hist_offset_x[3]);
        SetPpuRegister(pp_regs, HWIF_PPX_HIST_LOCAL_LUMA_X5_OFFSET_U, i, ppu_cfg->hist_offset_x[4]);
        SetPpuRegister(pp_regs, HWIF_PPX_HIST_LOCAL_LUMA_Y1_OFFSET_U, i, ppu_cfg->hist_offset_y[0]);
        SetPpuRegister(pp_regs, HWIF_PPX_HIST_LOCAL_LUMA_Y2_OFFSET_U, i, ppu_cfg->hist_offset_y[1]);
        SetPpuRegister(pp_regs, HWIF_PPX_HIST_LOCAL_LUMA_Y3_OFFSET_U, i, ppu_cfg->hist_offset_y[2]);
    } else {
      SetPpuRegister(pp_regs, HWIF_PPX_HIST_LOCAL_LUMA_EN_U, i, 0);
    }

    if (ppu_cfg->hist_hsv_e) {
      SetPpuRegister(pp_regs, HWIF_PPX_HIST_HSV_EN_U, i, 1);
    } else {
      SetPpuRegister(pp_regs, HWIF_PPX_HIST_HSV_EN_U, i, 0);
    }

    if (ppu_cfg->luma_sum_e) {
      SetPpuRegister(pp_regs, HWIF_PPX_HIST_GLOBAL_LUMA_SUM_EN_U, i, 1);
      SetPpuRegister(pp_regs, HWIF_PPX_HIST_GLOBAL_LUMA_SUM_SHIFT_U, i, ppu_cfg->luma_sum_shift);
      // SetPpuRegister(pp_regs, HWIF_PPX_HIST_GLOBAL_LUMA_SUM_U, i, 0);
    } else {
      SetPpuRegister(pp_regs, HWIF_PPX_HIST_GLOBAL_LUMA_SUM_EN_U, i, 0);
    }

    if(in_width && out_width && in_width < out_width) {
      /* upscale */
      u32 W, W_ch, inv_w_lu;
      /*support odd crop*/
      u32 inv_w_ch, in_width_ch, out_width_ch;
      u32 out422packed = (ppu_cfg->out_yuyv || ppu_cfg->out_uyvy);
      if (out422packed) {
        in_width_ch = in_width;
        out_width_ch = out_width;
      } else {
        in_width_ch = ppu_cfg->sub_x == 2 ? (in_width + 1) / 2 : in_width;
        out_width_ch = ppu_cfg->sub_x == 2 ? (out_width + 1) / 2 : out_width;
      }

      SetPpuRegister(pp_regs, HWIF_PPX_HOR_SCALE_MODE_U, i, 1);

      W = FDIVI(TOFIX(out_width, 32), (TOFIX(in_width, 16) + out_width / 2));
      W_ch = FDIVI(TOFIX(out_width_ch, 32), (TOFIX(in_width_ch, 16) + out_width_ch / 2));
      inv_w_lu = FDIVI(TOFIX(in_width, 16) + out_width / 2, out_width);
      inv_w_ch = FDIVI(TOFIX(in_width_ch, 16) + out_width_ch / 2, out_width_ch);
      // if(p_hw_feature->scale_cost_support[i] < ULTRA_SCALER) {
        if (inv_w_lu % 2) inv_w_lu++;
        if (inv_w_ch % 2) inv_w_ch++;
      // }

      SetPpuRegister(pp_regs, HWIF_PPX_LU_WSCALE_RATIO_U, i, W);
      SetPpuRegister(pp_regs, HWIF_PPX_CH_WSCALE_RATIO_U, i, W_ch);
      SetPpuRegister(pp_regs, HWIF_PPX_LU_WSCALE_INVRA_U, i, inv_w_lu);
      // if(p_hw_feature->crop_step_rshift)
      //   SetPpuRegister(pp_regs, HWIF_PPX_CH_WSCALE_INVRA_U, i, inv_w_lu);
      // else
        SetPpuRegister(pp_regs, HWIF_PPX_CH_WSCALE_INVRA_U, i, inv_w_ch);

      SetPpuRegister(pp_regs, HWIF_PPX_DUP_HOR_U, i,  out_ratio_x);
    } else if(in_width && out_width && in_width > out_width) {
      /* downscale */
      u32 W, W_ch, inv_w_lu;
      /*support odd crop*/
      u32 inv_w_ch, in_width_ch, out_width_ch;
      u32 out422packed = (ppu_cfg->out_yuyv || ppu_cfg->out_uyvy);
      if (out422packed) {
        in_width_ch = in_width;
        out_width_ch = out_width;
      } else {
        in_width_ch = ppu_cfg->sub_x == 2 ? (in_width + 1) / 2: in_width;
        out_width_ch = ppu_cfg->sub_x == 2 ? (out_width + 1) / 2 : out_width;
      }

      SetPpuRegister(pp_regs, HWIF_PPX_HOR_SCALE_MODE_U, i, 2);

      /*support low-cost scale */
      if (p_hw_feature->scale_cost_support[i] == MIDDLE_SCALER && (ppu_cfg->pp_filter == AREA || area_enable)) {
        W = (u32)ceil(((double)out_width / (double)in_width) * (1<<16));
        W_ch = (u32)ceil(((double)out_width_ch / (double)in_width_ch) * (1<<16));
      } else {
        W = FDIVI(TOFIX(out_width, 32), (TOFIX(in_width, 16) + out_width / 2));
        W_ch = FDIVI(TOFIX(out_width_ch, 32), (TOFIX(in_width_ch, 16) + out_width_ch / 2));
      }
      inv_w_lu = FDIVI(TOFIX(in_width, 16) + out_width / 2, out_width);
      inv_w_ch = FDIVI(TOFIX(in_width_ch, 16) + out_width_ch / 2, out_width_ch);
      if(p_hw_feature->scale_cost_support[i] != ULTRA_SCALER) {
        if (inv_w_lu % 2) inv_w_lu++;
        if (inv_w_ch % 2) inv_w_ch++;
      }

      SetPpuRegister(pp_regs, HWIF_PPX_LU_WSCALE_RATIO_U, i, W);
      SetPpuRegister(pp_regs, HWIF_PPX_CH_WSCALE_RATIO_U, i, W_ch);
      SetPpuRegister(pp_regs, HWIF_PPX_LU_WSCALE_INVRA_U, i, inv_w_lu);
      // if(p_hw_feature->crop_step_rshift)
      //   SetPpuRegister(pp_regs, HWIF_PPX_CH_WSCALE_INVRA_U, i, inv_w_lu);
      // else
        SetPpuRegister(pp_regs, HWIF_PPX_CH_WSCALE_INVRA_U, i, inv_w_ch);
      SetPpuRegister(pp_regs, HWIF_PPX_DUP_HOR_U, i,  0);
    } else {
      SetPpuRegister(pp_regs, HWIF_PPX_HOR_SCALE_MODE_U, i, 0);
      SetPpuRegister(pp_regs, HWIF_PPX_LU_WSCALE_RATIO_U, i, 0);
      SetPpuRegister(pp_regs, HWIF_PPX_CH_WSCALE_RATIO_U, i, 0);
      SetPpuRegister(pp_regs, HWIF_PPX_DUP_HOR_U, i, 1);
      SetPpuRegister(pp_regs, HWIF_PPX_LU_WSCALE_INVRA_U, i, 0);
      SetPpuRegister(pp_regs, HWIF_PPX_CH_WSCALE_INVRA_U, i, 0);
    }

    if(in_height && out_height && in_height < out_height) {
      /* upscale */
      u32 H, H_ch, inv_h_lu;
      /*support odd crop*/
      u32 inv_h_ch, in_height_ch, out_height_ch;
      in_height_ch = ppu_cfg->sub_y == 2 ? (in_height + 1) / 2 : in_height;
      out_height_ch = ppu_cfg->sub_y == 2 ? (out_height + 1) / 2 : out_height;

      SetPpuRegister(pp_regs, HWIF_PPX_VER_SCALE_MODE_U, i, 1);

      H = FDIVI(TOFIX(out_height, 32), (TOFIX(in_height, 16) + out_height / 2));
      H_ch = FDIVI(TOFIX(out_height_ch, 32), (TOFIX(in_height_ch, 16) + out_height_ch / 2));
      inv_h_lu = FDIVI(TOFIX(in_height, 16) + out_height / 2, out_height);
      inv_h_ch = FDIVI(TOFIX(in_height_ch, 16) + out_height_ch / 2, out_height_ch);
      // if(p_hw_feature->scale_cost_support[i] < ULTRA_SCALER) {
        if (inv_h_lu % 2) inv_h_lu++;
        if (inv_h_ch % 2) inv_h_ch++;
      // }

      SetPpuRegister(pp_regs, HWIF_PPX_LU_HSCALE_RATIO_U, i, H);
      SetPpuRegister(pp_regs, HWIF_PPX_CH_HSCALE_RATIO_U, i, H_ch);
      SetPpuRegister(pp_regs, HWIF_PPX_LU_HSCALE_INVRA_U, i, inv_h_lu);
      // if(p_hw_feature->crop_step_rshift)
      //   SetPpuRegister(pp_regs, HWIF_PPX_CH_HSCALE_INVRA_U, i, inv_h_lu);
      // else
        SetPpuRegister(pp_regs, HWIF_PPX_CH_HSCALE_INVRA_U, i, inv_h_ch);
      SetPpuRegister(pp_regs, HWIF_PPX_DUP_VER_U, i, out_ratio_y);
    } else if(in_height && out_height && in_height > out_height) {
      /* downscale */
      u32 H, H_ch, inv_h_lu;
      /*support odd crop*/
      u32 inv_h_ch, in_height_ch, out_height_ch;
      in_height_ch = ppu_cfg->sub_y == 2 ? (in_height + 1) / 2 : in_height;
      out_height_ch = ppu_cfg->sub_y == 2 ? (out_height + 1) / 2 : out_height;

      if(pp_args->interlace && ppu_cfg->pp_filter == AREA ) {
        in_height_ch  =  ppu_cfg->sub_y == 2 ? (in_height_ch + 1) / 2 : in_height_ch;
        out_height_ch = ppu_cfg->sub_y == 2 ? (out_height_ch + 1) / 2 : out_height_ch;
      }
      SetPpuRegister(pp_regs, HWIF_PPX_VER_SCALE_MODE_U, i, 2);

      if (p_hw_feature->scale_cost_support[i] == MIDDLE_SCALER && (ppu_cfg->pp_filter == AREA || area_enable)) {
        H = (u32)ceil(((double)out_height / (double)in_height) * (1<<16));
        H_ch = (u32)ceil(((double)out_height_ch / (double)in_height_ch) * (1<<16));
      } else {
        H = FDIVI(TOFIX(out_height, 32), (TOFIX(in_height, 16) + out_height / 2));
        H_ch = FDIVI(TOFIX(out_height_ch, 32), (TOFIX(in_height_ch, 16) + out_height_ch / 2));
      }
      inv_h_lu = FDIVI(TOFIX(in_height, 16) + out_height / 2, out_height);
      inv_h_ch = FDIVI(TOFIX(in_height_ch, 16) + out_height_ch / 2, out_height_ch);

      if(p_hw_feature->scale_cost_support[i] != ULTRA_SCALER) {
        if (inv_h_lu % 2) inv_h_lu++;
        if (inv_h_ch % 2) inv_h_ch++;
      }

      SetPpuRegister(pp_regs, HWIF_PPX_LU_HSCALE_RATIO_U, i, H);
      SetPpuRegister(pp_regs, HWIF_PPX_CH_HSCALE_RATIO_U, i, H_ch);
      SetPpuRegister(pp_regs, HWIF_PPX_LU_HSCALE_INVRA_U, i, inv_h_lu);
      SetPpuRegister(pp_regs, HWIF_PPX_DUP_VER_U, i, 0);
      // if(p_hw_feature->crop_step_rshift)
      //   SetPpuRegister(pp_regs, HWIF_PPX_CH_HSCALE_INVRA_U, i, inv_h_lu);
      // else
        SetPpuRegister(pp_regs, HWIF_PPX_CH_HSCALE_INVRA_U, i, inv_h_ch);
    } else {
      SetPpuRegister(pp_regs, HWIF_PPX_VER_SCALE_MODE_U, i, 0);
      SetPpuRegister(pp_regs, HWIF_PPX_LU_HSCALE_INVRA_U, i, 0);
      SetPpuRegister(pp_regs, HWIF_PPX_CH_HSCALE_INVRA_U, i, 0);
      SetPpuRegister(pp_regs, HWIF_PPX_CH_HSCALE_RATIO_U, i, 0);
      SetPpuRegister(pp_regs, HWIF_PPX_DUP_VER_U, i, 1);
      SetPpuRegister(pp_regs, HWIF_PPX_LU_HSCALE_RATIO_U, i, 0);
    }
  }
  SetPpuRegister(pp_regs, HWIF_PPX_OUT_TILE_E_U, i, ppu_cfg->tiled_e);
  if (ppu_cfg->tiled_e && ppu_cfg->tile_mode) {
    if (ppu_cfg->tile_mode != PACKED422_TILED8x8)
      SetPpuRegister(pp_regs, HWIF_PPX_TILE_SIZE_U, i, ppu_cfg->tile_mode);
    else
      SetPpuRegister(pp_regs, HWIF_PPX_OUT_YUV422_TILE_E_U, i, 1);
    SetPpuRegister(pp_regs, HWIF_PPX_YUV_SUPERTILE_PAD_COPY_E_U, i, ppu_cfg->yuv_supertile_pad_copy_e);
    /*
    SetPpuRegister(pp_regs, VIRTUAL_LEFT_U, i, ppu_cfg->vir_left);
    SetPpuRegister(pp_regs, VIRTUAL_RIGHT_U, i, ppu_cfg->vir_right);
    SetPpuRegister(pp_regs, VIRTUAL_TOP_U, i, ppu_cfg->vir_top);
    SetPpuRegister(pp_regs, VIRTUAL_BOTTOM_U, i, ppu_cfg->vir_bottom);
    SET_PP_ADDR_REG2(pp_regs,
                      PP_FBC_TILE_BASE_U_LSB,
                      PP_FBC_TILE_BASE_U_MSB,
                      i,
                      ppu_cfg->fbc_tile.bus_address + ppu_cfg->fbc_tile_offset);
    */
    //TODO(min): FBC to be supported.
  }

  if (ppu_cfg->pp_comp && (ppu_cfg->tile_mode == TILED16x16 || ppu_cfg->tile_mode == TILED32x8)) {
    SetPpuRegister(pp_regs, HWIF_PPX_COMP_E_U, i, ppu_cfg->pp_comp);
    SetPpuRegister(pp_regs, HWIF_PPX_FBC_TILE_E_U, i, ppu_cfg->pp_fbc_tile_enable);
  } else if(ppu_cfg->pp_pvfbc && p_hw_feature->pp_comp_support[i] == 3) {/*1: fbc, 2:dec400, 3:pvfbc*/
    SetPpuRegister(pp_regs, HWIF_PPX_COMP_E_U, i, ppu_cfg->pp_pvfbc);
  } else {
    SetPpuRegister(pp_regs, HWIF_PPX_COMP_E_U, i, 0);
  }
  if (ppu_cfg->pp_pvfbc && p_hw_feature->pp_comp_support[i] == 3) {
    u32 pp_comp_ctb_height, pp_comp_chroma_ctb_height, luma_ram_size, chroma_ram_size, luma_direct_map, chroma_direct_map;
    u32 pp_width, pp_height, pp_width_ch, pp_height_ch, plane0_header_size, plane1_header_size, plane0_payload_size, plane1_payload_size;
    //u32 plane0_offset, plane1_offset;
    if (p_hw_feature->av1_support || p_hw_feature->vvc_support) {
      pp_comp_ctb_height = 144;
    } else if (p_hw_feature->hevc_support || p_hw_feature->h264_adv_support || p_hw_feature->vp9_support || p_hw_feature->avs2_support) {
      pp_comp_ctb_height = 88;
    } else /*jpeg, TODO g1*/
      pp_comp_ctb_height = 24;
    pp_comp_chroma_ctb_height = pp_comp_ctb_height / 2;
    luma_ram_size = pp_comp_ctb_height * 32 / 4; // TODO need know actual scale ratio
    if (ppu_cfg->crop2.enabled){
      pp_width = ppu_cfg->crop2.width;
      pp_height = ppu_cfg->crop2.height;
    } else {
      pp_width = ppu_cfg->scale.width;
      pp_height = ppu_cfg->scale.height;
    }
    pp_width_ch = ppu_cfg->sub_x == 2 ? NEXT_MULTIPLE(pp_width, 2) : pp_width;
    pp_height_ch = ppu_cfg->sub_y == 2 ? (pp_height+1) / 2 : pp_height;
    pp_width = ppu_cfg->out_format == PP_OUT_FMT_YUV420_P010 ? pp_width * 2 : pp_width;
    pp_width_ch = ppu_cfg->out_format == PP_OUT_FMT_YUV420_P010 ? pp_width_ch * 2 : pp_width_ch;
    // pp_width = ppu_cfg->out_format == PP_OUT_FMT_PVFBC_YUV420_P010 ? pp_width * 2 : pp_width;
    // pp_width_ch = ppu_cfg->out_format == PP_OUT_FMT_PVFBC_YUV420_P010 ? pp_width_ch * 2 : pp_width_ch;
    if(ppu_cfg->pp_pvfbc_tile_type == PVFBC_TILED16x4) {
      pp_width = NEXT_MULTIPLE(pp_width, 64);
      pp_width_ch = NEXT_MULTIPLE(pp_width_ch, 64);
      pp_height = NEXT_MULTIPLE(pp_height, 4);
      pp_height_ch = NEXT_MULTIPLE(pp_height_ch, 4);
    } else if(ppu_cfg->pp_pvfbc_tile_type == PVFBC_TILED8x8) {
      pp_width = NEXT_MULTIPLE(pp_width, 32);
      pp_width_ch = NEXT_MULTIPLE(pp_width_ch, 32);
      pp_height = NEXT_MULTIPLE(pp_height, 8);
      pp_height_ch = NEXT_MULTIPLE(pp_height_ch, 8);
    }
    plane0_payload_size = pp_width * pp_height;
    plane0_header_size = NEXT_MULTIPLE(plane0_payload_size / 256, 256);
    plane1_payload_size = pp_width_ch * pp_height_ch;
    plane1_header_size = NEXT_MULTIPLE(plane1_payload_size / 256, 256);
    luma_direct_map = (luma_ram_size >= plane0_header_size);
    chroma_ram_size = pp_comp_chroma_ctb_height * 32 / 4;
    chroma_direct_map = (chroma_ram_size >= plane1_header_size);
    if (luma_direct_map && chroma_direct_map)
      SetPpuRegister(pp_regs, HWIF_PPX_PP_COMP_DIRECT_MAP_U, i, 1);
    else
      SetPpuRegister(pp_regs, HWIF_PPX_PP_COMP_DIRECT_MAP_U, i, 0);
    //plane0_offset = plane0_payload_size - plane0_header_size;
    //plane1_offset = plane1_payload_size - plane1_header_size;

    SetPpuRegister(pp_regs, HWIF_PPX_PVFBC_PLN0_PLD_OFFSET_U, i, plane0_header_size);
    SetPpuRegister(pp_regs, HWIF_PPX_PVFBC_PLN1_PLD_OFFSET_U, i, plane1_header_size);
    SetPpuRegister(pp_regs, HWIF_PPX_PVFBC_LU_VAL0_U, i, ppu_cfg->pp_pvfbc_lu_const0);
    SetPpuRegister(pp_regs, HWIF_PPX_PVFBC_LU_VAL1_U, i, ppu_cfg->pp_pvfbc_lu_const1);
    SetPpuRegister(pp_regs, HWIF_PPX_PVFBC_CH_VAL0_U, i, ppu_cfg->pp_pvfbc_ch_const0);
    SetPpuRegister(pp_regs, HWIF_PPX_PVFBC_CH_VAL1_U, i, ppu_cfg->pp_pvfbc_ch_const1);
    SetPpuRegister(pp_regs, HWIF_PPX_PVFBC_TILE_TYPE_U, i, ppu_cfg->pp_pvfbc_tile_type);
    SetPpuRegister(pp_regs, HWIF_PPX_PVFBC_BYPASS_U, i, ppu_cfg->pp_pvfbc_bypass);
  }

  SetPpuRegister(pp_regs, HWIF_PPX_DECNANO_E_U, i, ppu_cfg->decnano_e);
  SetPpuRegister(pp_regs, HWIF_PPX_DECNANO_MODE_U, i, ppu_cfg->decnano_mode);

  if(bottom_field_flag && ppb_mode == DEC_DPB_FRAME) {
    if(ppu_cfg->luma_sum_e || ppu_cfg->hist_local_e || ppu_cfg->hist_global_e || ppu_cfg->hist_hsv_e) {
      pp_field_offset = ppw;
      pp_field_offset_ch = ppw_c + (256 * 32 + 24 * 128 * 32 +2  * 16 * 32 + 256 * 32) / 8;
    } else {
      pp_field_offset = ppw;
      pp_field_offset_ch = ppw_c;
    }
  }
  /*support PP DEC400*/
  if (ppu_cfg->dec400_enabled) {
    u32 dec400_ctb_height, dec400_ctb_ram_size_lu, dec400_ctb_ram_size_cb, dec400_ctb_ram_size_cr;
    u32 act_compressed_Y_ts_size, act_compressed_UV_ts_size, act_compressed_U_ts_size, act_compressed_V_ts_size, act_compressed_ARGB_ts_size;
    u8 dec400_direct_mapping = 0;
    if (p_hw_feature->av1_support || p_hw_feature->vvc_support) {
      dec400_ctb_height = 144 * (p_hw_feature->uscale_support[i] ? (p_hw_feature->pp_support_6x ? 6 : 3) : 1);
    } else if (p_hw_feature->hevc_support || p_hw_feature->h264_adv_support || p_hw_feature->vp9_support || p_hw_feature->avs2_support) {
      dec400_ctb_height = 88 * (p_hw_feature->uscale_support[i] ? (p_hw_feature->pp_support_6x ? 6 : 3) : 1);
    } else if (p_hw_feature->mpeg4_support || p_hw_feature->custom_mpeg4_support || p_hw_feature->mpeg2_support ||
               p_hw_feature->sorenson_spark_support || p_hw_feature->vc1_support || p_hw_feature->rv_support ||
               p_hw_feature->vp8_support || p_hw_feature->webp_support || p_hw_feature->vp7_support ||
               p_hw_feature->vp6_support || p_hw_feature->avs_support || p_hw_feature->avs_plus_support) {/*g1*/
      dec400_ctb_height = 56 * (p_hw_feature->uscale_support[i] ? (p_hw_feature->pp_support_6x ? 6 : 3) : 1);
    } else { /* jpeg only */
      if (!p_hw_feature->jpeg_support && p_hw_feature->png_support) { /* png only */
        dec400_ctb_height = 8 * (p_hw_feature->uscale_support[i] ? (p_hw_feature->pp_support_6x ? 6 : 3) : 1);
      } else {
        dec400_ctb_height = 16 * (p_hw_feature->uscale_support[i] ? (p_hw_feature->pp_support_6x ? 6 : 3) : 1);
      }
      if (p_hw_feature->uscale_support[i] || p_hw_feature->dscale_support[i]) {
        dec400_ctb_height += 8;
      }
    }

    dec400_ctb_height = 1 << (CeilLog2(dec400_ctb_height));
    act_compressed_ARGB_ts_size = ppu_cfg->luma_size / 256 / 2;
    if (p_hw_feature->pp_comp_support[i] == 2 && p_hw_feature->pp_comp_version[i] == 2 && ppu_cfg->planar) {
      dec400_ctb_ram_size_lu = dec400_ctb_height * 32;
      dec400_ctb_ram_size_cb =  dec400_ctb_ram_size_lu / 2;
      dec400_ctb_ram_size_cr = dec400_ctb_ram_size_cb;
      act_compressed_Y_ts_size = ppu_cfg->luma_size / 256 / 2;
      act_compressed_U_ts_size = ppu_cfg->chroma_size / 2 / 256 / 2;
      act_compressed_V_ts_size = ppu_cfg->chroma_size / 2 / 256 / 2;
      if (!(ppu_cfg->rgb || ppu_cfg->rgb_planar) && ppu_cfg->planar &&
        (NEXT_MULTIPLE(act_compressed_Y_ts_size, 32) <= dec400_ctb_ram_size_lu && NEXT_MULTIPLE(act_compressed_U_ts_size, 32) <= dec400_ctb_ram_size_cb && NEXT_MULTIPLE(act_compressed_V_ts_size, 32) <= dec400_ctb_ram_size_cr)) {
        dec400_direct_mapping = 1;
      }
    } else {
      if (p_hw_feature->pp_comp_support[i] == 2 && p_hw_feature->pp_comp_version[i] == 2) {
        dec400_ctb_ram_size_lu = dec400_ctb_height * 32;
        dec400_ctb_ram_size_cb =  dec400_ctb_ram_size_lu / 2;
      } else {
        dec400_ctb_ram_size_lu = dec400_ctb_height * 32 / 2;
        dec400_ctb_ram_size_cb =  dec400_ctb_ram_size_lu / 2;
      }
      if (p_hw_feature->pp_planar_support[i] || ppu_cfg->planar_f) {
        dec400_ctb_ram_size_cr = dec400_ctb_ram_size_cb;
      } else
        dec400_ctb_ram_size_cr = 0;
      act_compressed_Y_ts_size = ppu_cfg->luma_size / 256 / 2;
      act_compressed_UV_ts_size = ppu_cfg->chroma_size / 256 / 2;
      if((!(ppu_cfg->rgb || ppu_cfg->rgb_planar) && act_compressed_Y_ts_size <= dec400_ctb_ram_size_lu && act_compressed_UV_ts_size <= (dec400_ctb_ram_size_cb + dec400_ctb_ram_size_cr))
            || ((ppu_cfg->rgb || ppu_cfg->rgb_planar) && act_compressed_ARGB_ts_size <= (dec400_ctb_ram_size_lu + dec400_ctb_ram_size_cb + dec400_ctb_ram_size_cr))) {
        dec400_direct_mapping = 1;
      }
    }

    SetPpuRegister(pp_regs, HWIF_PPX_PP_COMP_DIRECT_MAP_U, i, dec400_direct_mapping);
    SetPpuRegister(pp_regs, HWIF_PPX_COMP_E_U, i, ppu_cfg->dec400_enabled);
    if (ppu_cfg->dec400_align == DEC_ALIGN_32B) {
      SetPpuRegister(pp_regs, HWIF_PPX_DEC400_ALIGN_U, i, 0);
    } else if (ppu_cfg->dec400_align == DEC_ALIGN_64B) {
      SetPpuRegister(pp_regs, HWIF_PPX_DEC400_ALIGN_U, i, 1);
    }
    DWLmemset(ppu_out_vir_addr + ppu_cfg->dec400_pln0_tile_status_offset + pp_field_offset, 0, ppu_cfg->dec400_luma_table.size);
    if (ppu_cfg->chroma_size) {
      DWLmemset(ppu_out_vir_addr + ppu_cfg->dec400_pln1_tile_status_offset + pp_field_offset_ch, 0, ppu_cfg->dec400_chroma_table.size);
    }
    SET_PP_ADDR_REG2(pp_regs,
                    HWIF_PPX_DEC400_PLN0_TS_BASE_U_LSB,
                    HWIF_PPX_DEC400_PLN0_TS_BASE_U_MSB,
                    i,
                    ppu_out_bus_addr + ppu_cfg->dec400_pln0_tile_status_offset + pp_field_offset);
    SET_PP_ADDR_REG2(pp_regs,
                    HWIF_PPX_DEC400_PLN1_TS_BASE_U_LSB,
                    HWIF_PPX_DEC400_PLN1_TS_BASE_U_MSB,
                    i,
                    ppu_out_bus_addr + ppu_cfg->dec400_pln1_tile_status_offset + pp_field_offset_ch);
    if (p_hw_feature->pp_comp_support[i] == 2 && p_hw_feature->pp_comp_version[i] == 2 && ppu_cfg->planar) {
      SET_PP_ADDR_REG2(pp_regs,
                      HWIF_PPX_DEC400_PLN2_TS_BASE_U_LSB,
                      HWIF_PPX_DEC400_PLN2_TS_BASE_U_MSB,
                      i,
                      ppu_out_bus_addr + ppu_cfg->dec400_pln1_tile_status_offset + pp_field_offset_ch + ppu_cfg->dec400_chroma_table.size / 2); // for planar cr plane when dec400 out
    }
  }
  SET_PP_ADDR_REG2(pp_regs,
                  HWIF_PPX_LANCZOS_TBL_BASE_U_LSB,
                  HWIF_PPX_LANCZOS_TBL_BASE_U_MSB,
                  i,
                  ppu_cfg->lanczos_table.bus_address);

  if(p_hw_feature->pp_comp_support[i] == 1 && ((ppu_cfg->crop.width >> 1) == ppu_cfg->scale.width) && ((ppu_cfg->crop.height >> 1) == ppu_cfg->scale.height) && ppu_cfg->pp_filter != AREA) {
    /*width and height downscale 1/2: set 7 filter weight factors*/
    SetPpuRegister(pp_regs, HWIF_PPX_DS_COEFF0_U, i, (i8)*ppu_cfg->lanczos_table.virtual_address); /*low 16 bit*/
    SetPpuRegister(pp_regs, HWIF_PPX_DS_COEFF1_U, i, (i8)(*(ppu_cfg->lanczos_table.virtual_address) >> 16)); /*high 16 bit*/
    SetPpuRegister(pp_regs, HWIF_PPX_DS_COEFF2_U, i, (i8)*(ppu_cfg->lanczos_table.virtual_address + 1));
    SetPpuRegister(pp_regs, HWIF_PPX_DS_COEFF3_U, i, (i8)(*(ppu_cfg->lanczos_table.virtual_address + 1) >> 16));
    SetPpuRegister(pp_regs, HWIF_PPX_DS_COEFF4_U, i, (i8)*(ppu_cfg->lanczos_table.virtual_address + 2));
    SetPpuRegister(pp_regs, HWIF_PPX_DS_COEFF5_U, i, (i8)(*(ppu_cfg->lanczos_table.virtual_address + 2) >> 16));
    SetPpuRegister(pp_regs, HWIF_PPX_DS_COEFF6_U, i, (i8)*(ppu_cfg->lanczos_table.virtual_address + 3));
  }
  SET_PP_ADDR_REG2(pp_regs,
                  HWIF_PPX_SCALE_COLBUF_WR_BASE_U_LSB,
                  HWIF_PPX_SCALE_COLBUF_WR_BASE_U_MSB,
                  i,
                  ppu_cfg->scale_buf_bus[0]);
  if(ppu_cfg->crop.width != ppu_cfg->scale.width || ppu_cfg->crop.height != ppu_cfg->scale.height) {
    if (p_hw_feature->scale_cost_support[i] == MIDDLE_SCALER) {
      if (ppu_cfg->pp_filter == PP_NEAREST) {
        SetPpuRegister(pp_regs, HWIF_PPX_Y_FILTER_SIZE_U, i, ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset);
        SetPpuRegister(pp_regs, HWIF_PPX_X_FILTER_SIZE_U, i, ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset);
      } else if (ppu_cfg->pp_filter == PP_FAST_LINEAR) {
        SetPpuRegister(pp_regs, HWIF_PPX_Y_FILTER_SIZE_U, i, 2);
        SetPpuRegister(pp_regs, HWIF_PPX_X_FILTER_SIZE_U, i, 2);
      } else if (ppu_cfg->pp_filter == PP_FAST_BICUBIC) {
        SetPpuRegister(pp_regs, HWIF_PPX_Y_FILTER_SIZE_U, i, 2);
        SetPpuRegister(pp_regs, HWIF_PPX_X_FILTER_SIZE_U, i, 4);
      } else if (ppu_cfg->pp_filter == AREA) { // MEANINGLESS NUM. JUST FOR HW
        SetPpuRegister(pp_regs, HWIF_PPX_Y_FILTER_SIZE_U, i, 6);
        SetPpuRegister(pp_regs, HWIF_PPX_X_FILTER_SIZE_U, i, 6);
      }
    } else {
      SetPpuRegister(pp_regs, HWIF_PPX_Y_FILTER_SIZE_U, i, ppu_cfg->y_filter_size - ppu_cfg->y_filter_offset);
      SetPpuRegister(pp_regs, HWIF_PPX_X_FILTER_SIZE_U, i, ppu_cfg->x_filter_size - ppu_cfg->x_filter_offset);
    }
  }

#if 0
  if (ppu_cfg->tiled_e) {
    SetPpuRegister(pp_regs, HWIF_PPX_OUT_Y_STRIDE, i, (ppw >> p_hw_feature->pp_tiled_stride_shift));
    SetPpuRegister(pp_regs, HWIF_PPX_OUT_C_STRIDE, i, (ppw_c >> p_hw_feature->pp_tiled_stride_shift));
  } else {
    SetPpuRegister(pp_regs, HWIF_PPX_OUT_Y_STRIDE, i, ppw);
    SetPpuRegister(pp_regs, HWIF_PPX_OUT_C_STRIDE, i, ppw_c);
  }
#else
  SetPpuRegister(pp_regs, HWIF_PPX_OUT_Y_STRIDE, i, ppw / stride_unit[ppu_cfg->unit]);
  SetPpuRegister(pp_regs, HWIF_PPX_OUT_C_STRIDE, i, ppw_c / stride_unit[ppu_cfg->unit]);
  SetPpuRegister(pp_regs, HWIF_PPX_STRIDE_UNIT, i, ppu_cfg->unit);
#endif

  addr_t addr_plane_lu_r, addr_plane_ch_g, addr_plane_b;
  u8 *addr_plane_lu_r_vir, *addr_plane_ch_g_vir;
  u32 valid_2nd_addr = 0;
  struct DWLLinearMem *cur_pp_out_addr;

  if (pp_args->use_separated_pp == 0)
    cur_pp_out_addr = ((struct DWLDecBuffers *)pp_args->ppu_out_addr)->dec_buffer[0];
  else
    cur_pp_out_addr = ((struct DWLDecBuffers *)pp_args->ppu_out_addr)->dec_buffer[channel_id];

  if (MAX_OUTPUT_PLANE >= 3 && DWL_DEVMEM_VAILD(cur_pp_out_addr[1])) {
    valid_2nd_addr = 1;

    addr_plane_lu_r = cur_pp_out_addr[0].bus_address;
    addr_plane_ch_g = cur_pp_out_addr[1].bus_address;
    addr_plane_b = cur_pp_out_addr[2].bus_address;
    if (ppu_cfg->buf_off_set_by_user) {
      addr_plane_lu_r += ppu_cfg->luma_offset;
      addr_plane_ch_g += ppu_cfg->chroma_offset;
    }
  }

  if (!ppu_cfg->rgb_planar) {
    if (i == 0 && ppu_cfg->pp_comp && (ppu_cfg->tile_mode == TILED16x16 || ppu_cfg->tile_mode == TILED32x8)) {
      addr_plane_lu_r = cur_pp_out_addr->bus_address + ppu_cfg->header_offset + pp_field_offset;
      addr_plane_ch_g = cur_pp_out_addr->bus_address + ppu_cfg->payload_offset + pp_field_offset_ch;

      SET_PP_ADDR_REG2(pp_regs,
                  HWIF_PPX_OUT_LU_BASE_U_LSB,
                  HWIF_PPX_OUT_LU_BASE_U_MSB,
                  0,
                  addr_plane_lu_r);
      SET_PP_ADDR_REG2(pp_regs,
                  HWIF_PPX_OUT_CH_BASE_U_LSB,
                  HWIF_PPX_OUT_CH_BASE_U_MSB,
                  0,
                  addr_plane_ch_g);
    } else if (ppu_cfg->pp_pvfbc) {
      addr_plane_lu_r = cur_pp_out_addr->bus_address + ppu_cfg->plane0_offset + pp_field_offset;
      addr_plane_ch_g = cur_pp_out_addr->bus_address + ppu_cfg->plane1_offset + pp_field_offset_ch;
      addr_plane_lu_r_vir = (u8*)cur_pp_out_addr->virtual_address + ppu_cfg->plane0_offset + pp_field_offset;
      addr_plane_ch_g_vir = (u8*)cur_pp_out_addr->virtual_address + ppu_cfg->plane1_offset + pp_field_offset_ch;
      if (!ppu_cfg->pp_pvfbc_bypass) {
        DWLmemset(addr_plane_lu_r_vir, 0, ppu_cfg->pvfbc_plane0_headr_size);
        DWLmemset(addr_plane_ch_g_vir, 0, ppu_cfg->pvfbc_plane1_headr_size);
      }
      // #ifdef MODEL_SIMULATION
      // SET_PP_ADDR_REG2(pp_regs,
      //             HWIF_PPX_OUT_LU_BASE_U_LSB,
      //             HWIF_PPX_OUT_LU_BASE_U_MSB,
      //             0,
      //             ppu_out_bus_addr + MAX(ppu_cfg->plane0_offset, ppu_cfg->luma_offset) + pp_field_offset);
      // SET_PP_ADDR_REG2(pp_regs,
      //             HWIF_PPX_OUT_CH_BASE_U_LSB,
      //             HWIF_PPX_OUT_CH_BASE_U_MSB,
      //             0,
      //             ppu_out_bus_addr + MAX(ppu_cfg->plane1_offset, ppu_cfg->chroma_offset) + pp_field_offset_ch);
      // #else
      SET_PP_ADDR_REG2(pp_regs,
                  HWIF_PPX_OUT_LU_BASE_U_LSB,
                  HWIF_PPX_OUT_LU_BASE_U_MSB,
                  0,
                  addr_plane_lu_r);
      SET_PP_ADDR_REG2(pp_regs,
                  HWIF_PPX_OUT_CH_BASE_U_LSB,
                  HWIF_PPX_OUT_CH_BASE_U_MSB,
                  0,
                  addr_plane_ch_g);
      //#endif
    }
    else {
#ifdef ASIC_TRACE_SUPPORT
        if (!valid_2nd_addr) {
          addr_plane_lu_r = cur_pp_out_addr->bus_address + ppu_cfg->luma_offset + pp_field_offset;
          addr_plane_ch_g = cur_pp_out_addr->bus_address + ppu_cfg->chroma_offset + pp_field_offset_ch;
        }
        else {
          if (pp_field_offset)
            addr_plane_lu_r += pp_field_offset;
          if (pp_field_offset_ch)
            addr_plane_ch_g += pp_field_offset_ch;
        }
        SET_PP_ADDR_REG2(pp_regs,
                    HWIF_PPX_OUT_LU_BASE_U_LSB,
                    HWIF_PPX_OUT_LU_BASE_U_MSB,
                    i,
                    addr_plane_lu_r);
        SET_PP_ADDR_REG2(pp_regs,
                      HWIF_PPX_OUT_CH_BASE_U_LSB,
                      HWIF_PPX_OUT_CH_BASE_U_MSB,
                      i,
                      addr_plane_ch_g);
#else
        if (ppu_cfg->sbi_en) {
          if (!ppu_cfg->sbi_lu_addr) {
            SET_PP_ADDR_REG2(pp_regs,
                          HWIF_PPX_OUT_LU_BASE_U_LSB,
                          HWIF_PPX_OUT_LU_BASE_U_MSB,
                          i,
                          ppu_out_bus_addr);
            addr_t ch_addr = NEXT_MULTIPLE(ppu_out_bus_addr +
                            ppu_cfg->sbi_lu_segmentbuffer_entry_size *
                            ppu_cfg->sbi_lu_segmentbuffer_entry_count *
                            ppu_cfg->ystride * 16, PAGE_SIZE);
            SET_PP_ADDR_REG2(pp_regs,
                          HWIF_PPX_OUT_CH_BASE_U_LSB,
                          HWIF_PPX_OUT_CH_BASE_U_MSB,
                          i,
                          ch_addr);
          } else {
            SET_PP_ADDR_REG2(pp_regs,
                          HWIF_PPX_OUT_LU_BASE_U_LSB,
                          HWIF_PPX_OUT_LU_BASE_U_MSB,
                          i,
                          ppu_cfg->sbi_lu_addr);
            SET_PP_ADDR_REG2(pp_regs,
                          HWIF_PPX_OUT_CH_BASE_U_LSB,
                          HWIF_PPX_OUT_CH_BASE_U_MSB,
                          i,
                          ppu_cfg->sbi_ch_addr);
          }
        } else {
          if (!valid_2nd_addr) {
            addr_plane_lu_r = cur_pp_out_addr->bus_address + ppu_cfg->luma_offset + pp_field_offset;
            addr_plane_ch_g = cur_pp_out_addr->bus_address + ppu_cfg->chroma_offset + pp_field_offset_ch;
          }
          else {
            if (pp_field_offset)
              addr_plane_lu_r += pp_field_offset;
            if (pp_field_offset_ch)
              addr_plane_ch_g += pp_field_offset_ch;
          }
          SET_PP_ADDR_REG2(pp_regs,
                      HWIF_PPX_OUT_LU_BASE_U_LSB,
                      HWIF_PPX_OUT_LU_BASE_U_MSB,
                      i,
                      addr_plane_lu_r);
          SET_PP_ADDR_REG2(pp_regs,
                        HWIF_PPX_OUT_CH_BASE_U_LSB,
                        HWIF_PPX_OUT_CH_BASE_U_MSB,
                        i,
                        addr_plane_ch_g);
        }
#endif
    }

    u32 pp_height;
    pp_height = ppu_cfg->crop2.enabled ? ppu_cfg->crop2.height : ppu_cfg->scale.height;
    if (ppu_cfg->planar) {
      if (!valid_2nd_addr) {
        if (0)
          addr_plane_b = cur_pp_out_addr->bus_address + ppu_cfg->chroma_offset + pp_field_offset_ch +
                         ppu_cfg->cstride * pp_height / ppu_cfg->sub_y;
        else /*support odd crop*/
          addr_plane_b = cur_pp_out_addr->bus_address + ppu_cfg->chroma_offset + pp_field_offset_ch +
                         ppu_cfg->cstride * (ppu_cfg->sub_y == 2 ? (pp_height + 1) / 2 : pp_height);
      }
      else {
        if (pp_field_offset_ch)
          addr_plane_b += pp_field_offset_ch;
      }
      SET_PP_ADDR_REG2(pp_regs,
                  HWIF_PPX_OUT_B_BASE_U_LSB,
                  HWIF_PPX_OUT_B_BASE_U_MSB,
                  i,
                  addr_plane_b);
    }
  } else {
    //CalcOnePpUnitLumaSize think rgb_planar only one plane
    u32 offset = 0;
    u32 pp_height = ppu_cfg->crop2.enabled ? ppu_cfg->crop2.height : ppu_cfg->scale.height;
    offset = NEXT_MULTIPLE(ppw * pp_height, PLANE_ALIGNMENT);

    if (IS_BGR_P_FORMAT(ppu_cfg->rgb_format)) {
      SET_PP_ADDR_REG2(pp_regs,
                    HWIF_PPX_OUT_B_BASE_U_LSB,
                    HWIF_PPX_OUT_B_BASE_U_MSB,
                    i,
                    ppu_out_bus_addr + ppu_cfg->luma_offset + pp_field_offset);
      SET_PP_ADDR_REG2(pp_regs,
                    HWIF_PPX_OUT_G_BASE_U_LSB,
                    HWIF_PPX_OUT_G_BASE_U_MSB,
                    i,
                    ppu_out_bus_addr + ppu_cfg->luma_offset + offset + pp_field_offset);
      SET_PP_ADDR_REG2(pp_regs,
                    HWIF_PPX_OUT_R_BASE_U_LSB,
                    HWIF_PPX_OUT_R_BASE_U_MSB,
                    i,
                    ppu_out_bus_addr + ppu_cfg->luma_offset + 2 * offset + pp_field_offset);
    } else {
      if (!valid_2nd_addr) {
        addr_plane_lu_r = cur_pp_out_addr->bus_address + ppu_cfg->luma_offset + pp_field_offset;
        addr_plane_ch_g = cur_pp_out_addr->bus_address + ppu_cfg->luma_offset + offset + pp_field_offset;
        addr_plane_b = cur_pp_out_addr->bus_address + ppu_cfg->luma_offset + 2 * offset + pp_field_offset;
      }
      SET_PP_ADDR_REG2(pp_regs,
                    HWIF_PPX_OUT_R_BASE_U_LSB,
                    HWIF_PPX_OUT_R_BASE_U_MSB,
                    i,
                    addr_plane_lu_r);
      SET_PP_ADDR_REG2(pp_regs,
                    HWIF_PPX_OUT_G_BASE_U_LSB,
                    HWIF_PPX_OUT_G_BASE_U_MSB,
                    i,
                    addr_plane_ch_g);
      SET_PP_ADDR_REG2(pp_regs,
                    HWIF_PPX_OUT_B_BASE_U_LSB,
                    HWIF_PPX_OUT_B_BASE_U_MSB,
                    i,
                    addr_plane_b);
    }
  }
  if (channel_id == 0)
    PPSetSliceTextParams(pp_regs, &ppu_cfg->slice_text);
}



/*------------------------------------------------------------------------------
    Function name :
    Description   :

    Return type   : None
    Argument      : DecAsicBuffers_t * p_asic_buff
------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------

    Function: PPSetRegs

        Functional description:
            Set registers based on pp configurations.

        Input:
            pp_regs     register array base pointer (swreg0)
            p_hw_feature  hw feature struct pointer
            ppu_out_bus_addr  base address of pp buffer
            mono_chrome   whether input picture is mono-chrome
            bottom_field_flag set 1 if current is the bottom field of a picture,
                              otherwise (top field or frame) 0
            align       alignment setting

        Output:

        Returns:

------------------------------------------------------------------------------*/
void PPSetRegs(u32 *pp_regs, const struct PpParams *pp_args) {
  u32 i;
  u32 pp_enabled = 0;
  const struct DecHwFeatures *hw_feature = pp_args->p_hw_feature;
  PpUnitIntConfig *ppu_cfg = pp_args->ppu_cfg;

  /* registers for each pp unit */
  for (i = 0; i < hw_feature->max_ppu_count; i++, ppu_cfg++) {
    if (!ppu_cfg->enabled || !PP_CHANNEL_OUT_ENABLE(pp_args->pp_out_ctrl, i)) continue;

    pp_enabled |= 1 << i;

    PPSetOneChannelRegs(pp_regs, pp_args, i);
  }
  SetDecRegister(pp_regs, HWIF_PP_OUT_E_U, pp_enabled);
  /* Set default value 1 for input format in pipeline mode. */
  /* For standalone pp, remember to overwrite it to correct value after PPSetRegs. */
  // SetDecRegister(pp_regs, HWIF_PP_IN_FORMAT_U, 1);
}

void PPSetFbcRegs(u32 *pp_regs, const struct PpParams *pp_args) {

  u32 i;
  const struct DecHwFeatures *p_hw_feature = pp_args->p_hw_feature;
  PpUnitIntConfig *ppu_cfg = pp_args->ppu_cfg;
  u32 tile_enable = pp_args->tile_enable;
  /* registers for each pp unit */
  for (i = 0; i < p_hw_feature->max_ppu_count; i++, ppu_cfg++) {
    if (!ppu_cfg->enabled) continue;
    if (ppu_cfg->tile_mode == TILED128x2) {
      if (tile_enable) {
        SetPpuRegister(pp_regs, HWIF_PPX_OUT_TILE_E_U, i, 0);
        SetPpuRegister(pp_regs, HWIF_PPX_TILE_SIZE_U, i, 0);
      } else {
        SetPpuRegister(pp_regs, HWIF_PPX_OUT_TILE_E_U, i, ppu_cfg->tiled_e);
        SetPpuRegister(pp_regs, HWIF_PPX_TILE_SIZE_U, i, ppu_cfg->tile_mode);
      }
      SetPpuRegister(pp_regs, HWIF_PPX_OUT_Y_STRIDE, i, ppu_cfg->ystride / stride_unit[ppu_cfg->unit]);
      SetPpuRegister(pp_regs, HWIF_PPX_OUT_C_STRIDE, i, ppu_cfg->cstride / stride_unit[ppu_cfg->unit]);
      SetPpuRegister(pp_regs, HWIF_PPX_STRIDE_UNIT, i, ppu_cfg->unit);
    }
  }
}

u32 PPGetLancozsColumnBufferSize(PpUnitIntConfig *ppu_cfg,
                                 u32 pic_height,
                                 u32 pixel_width,
                                 u32 num_tile_cols) {
  u32 pp_reorder_size = NEXT_MULTIPLE(pic_height, 16) * pixel_width * 36;
  u32 pp_scale_size = LANCZOS_TILE_EDGE_SIZE * sizeof(i32);
  u32 pp_scale_out_size = 0;
  u32 size = 0;
  /* pp reorder col buffer: only one */
  size += pp_reorder_size * num_tile_cols;
  for (u32 i = 0; i < DEC_MAX_PPU_COUNT; i++) {
    ppu_cfg[i].reorder_size = pp_reorder_size;
    ppu_cfg[i].scale_size = pp_scale_size;
    if (ppu_cfg[i].enabled) {
      /* pp scale col buffer: each pp channel */
      size += pp_scale_size * num_tile_cols;
      /*pp scale out col buffer: each pp channel */
      switch (ppu_cfg[i].tiled_e ? ppu_cfg[i].tile_mode : 0) {
        case TILED16x16:
          if (ppu_cfg[i].pp_comp) {
            /*support max_pic_height: 8K,"* (8192 + 8192 / 2)": luma + chroma, '*2': if can't write done in HW */
            pp_scale_out_size = 3 * LANCZOS_TILE_PPOUT_SIZE;
          } else {
            pp_scale_out_size = 3 * LANCZOS_TILE_PPOUT_SIZE;
          }
          break;
        case TILED64x64:
          /* 8(column) x 8192(max height) x 4(r g b occupies 32bits) x 2(prevent overwrite) : for 8x8 bolck */
          if (IS_PP_OUT_RGB(ppu_cfg[i].out_format))
            pp_scale_out_size = 4 * LANCZOS_TILE_PPOUT_SIZE;
          else
            pp_scale_out_size = 8 * LANCZOS_TILE_PPOUT_SIZE;
          break;
        case TILED8x8:
          /* 8(column) x 8192(max height) * 2 : luma,  : for 8x8 bolck */
          pp_scale_out_size = 3 * LANCZOS_TILE_PPOUT_SIZE / 2;
          break;
        default: /*TILED32x8*/
          if (ppu_cfg[i].pp_comp) {
            /*support max_pic_height: 8K,"* (8192 + 8192 / 2)": luma + chroma, '*2': if can't write done in HW */
            pp_scale_out_size = 6 * LANCZOS_TILE_PPOUT_SIZE;
          } else {
            pp_scale_out_size = 1 * LANCZOS_TILE_PPOUT_SIZE;
          }
          break;
      }
      if (ppu_cfg[i].pp_pvfbc) {/*only support 420SP*/
        if(ppu_cfg[i].pp_pvfbc_tile_type == PVFBC_TILED16x4)
          pp_scale_out_size = (8192 * 64 + 4096 * 64) * 2;
        else if(ppu_cfg[i].pp_pvfbc_tile_type == PVFBC_TILED8x8)
          pp_scale_out_size = (8192 * 32 + 4096 * 32) * 2;
      }
      ppu_cfg[i].scale_out_size = pp_scale_out_size;
      size += pp_scale_out_size * num_tile_cols;
    }
  }
  return size;
}

void PPSetLancozsScaleRegs(u32 *pp_regs,
                           const struct DecHwFeatures *hw_feature,
                           PpUnitIntConfig *ppu_cfg,
                           u32 core_id) {
  u32 i;
  u32 reorder_set = 0;

  for (i = 0; i < hw_feature->max_ppu_count; i++, ppu_cfg++) {
    if (!ppu_cfg->enabled) continue;
    if (!reorder_set) {
      SET_ADDR_REG(pp_regs, HWIF_PP_REORDER_TILE_WR_BASE_U, ppu_cfg->reorder_buf_bus[core_id]);
      SET_ADDR_REG(pp_regs, HWIF_PP_REORDER_TILE_RD_BASE_U, ppu_cfg->reorder_buf_bus[core_id]);
      reorder_set = 1;
    }
    SET_PP_ADDR_REG2(pp_regs,
                    HWIF_PPX_SCALE_COLBUF_WR_BASE_U_LSB,
                    HWIF_PPX_SCALE_COLBUF_WR_BASE_U_MSB,
                    i,
                    ppu_cfg->scale_buf_bus[core_id]);
    SET_PP_ADDR_REG2(pp_regs,
                    HWIF_PPX_SCALE_COLBUF_RD_BASE_U_LSB,
                    HWIF_PPX_SCALE_COLBUF_RD_BASE_U_MSB,
                    i,
                    ppu_cfg->scale_buf_bus[core_id]);
    SET_PP_ADDR_REG2(pp_regs,
                    HWIF_PPX_SCALE_OUT_COLBUF_WR_BASE_U_LSB,
                    HWIF_PPX_SCALE_OUT_COLBUF_WR_BASE_U_MSB,
                    i,
                    ppu_cfg->scale_out_buf_bus[core_id]);
    SET_PP_ADDR_REG2(pp_regs,
                    HWIF_PPX_SCALE_OUT_COLBUF_RD_BASE_U_LSB,
                    HWIF_PPX_SCALE_OUT_COLBUF_RD_BASE_U_MSB,
                    i,
                    ppu_cfg->scale_out_buf_bus[core_id]);
  }
}

void PPSetLancozsMutiCoreScaleRegs(u32 *pp_regs,
                                   const struct DecHwFeatures *hw_feature,
                                   PpUnitIntConfig *ppu_cfg, u32 tile_id) {
  u32 i;
  u32 reorder_set = 0;
  u32 tile_id_wr = 0;
  u32 tile_id_rd = 0;

  if (hw_feature->max_ppu_count) {
    for (i = 0; i < DEC_MAX_PPU_COUNT; i++, ppu_cfg++) {
      if (!ppu_cfg->enabled) continue;
      tile_id_wr = tile_id;
      tile_id_rd = tile_id == 0 ? 0 : (tile_id - 1);
      if (!reorder_set) {
        SET_ADDR_REG(pp_regs, HWIF_PP_REORDER_TILE_WR_BASE_U, ppu_cfg->reorder_buf_bus[0] + tile_id_wr * ppu_cfg->reorder_size);
        SET_ADDR_REG(pp_regs, HWIF_PP_REORDER_TILE_RD_BASE_U, ppu_cfg->reorder_buf_bus[0] + tile_id_rd * ppu_cfg->reorder_size);
        reorder_set = 1;
      }
      SET_PP_ADDR_REG2(pp_regs,
                      HWIF_PPX_SCALE_COLBUF_WR_BASE_U_LSB,
                      HWIF_PPX_SCALE_COLBUF_WR_BASE_U_MSB,
                      i,
                      ppu_cfg->scale_buf_bus[0] + tile_id_wr * ppu_cfg->scale_size);
      SET_PP_ADDR_REG2(pp_regs,
                      HWIF_PPX_SCALE_COLBUF_RD_BASE_U_LSB,
                      HWIF_PPX_SCALE_COLBUF_RD_BASE_U_MSB,
                      i,
                      ppu_cfg->scale_buf_bus[0] + tile_id_rd * ppu_cfg->scale_size);
      SET_PP_ADDR_REG2(pp_regs,
                      HWIF_PPX_SCALE_OUT_COLBUF_WR_BASE_U_LSB,
                      HWIF_PPX_SCALE_OUT_COLBUF_WR_BASE_U_MSB,
                      i,
                      ppu_cfg->scale_out_buf_bus[0] + tile_id_wr * ppu_cfg->scale_out_size);
      SET_PP_ADDR_REG2(pp_regs,
                      HWIF_PPX_SCALE_OUT_COLBUF_RD_BASE_U_LSB,
                      HWIF_PPX_SCALE_OUT_COLBUF_RD_BASE_U_MSB,
                      i,
                      ppu_cfg->scale_out_buf_bus[0] + tile_id_rd * ppu_cfg->scale_out_size);
    }
  }
}

u32 PPCheckMutiCoreSupport(PpUnitIntConfig *ppu_cfg, u32 filter_bypass, u32 sb_size, u32 tile_cols, u8* tile_col_mem) {
  u32 i, j;
  u32 start_tile = 0;
  i32 xDstInSrc;
  i32 scale_ratio_x_inv, scale_ratio_x_inv_ch;
  i32 pos, x_pos;
  u16 xRow[8320] = {0}, xRow_ch[8320] = {0};//8192+128 = 8320
  u32 index = 0;
  u32 last_index = 0;
  u32 first_flag = 1;
  u32 sub_x;
  u32 crop_w_ch, scale_w_ch;
  if (ppu_cfg->scale.width >= ppu_cfg->crop.width)
    return 1;
  if (ppu_cfg->pixel_width != 10 && !ppu_cfg->tiled_e)
    return 1;
  u32 *tile_size_start = (u32*)DWLmalloc((tile_cols - 1) * sizeof(u32));
  if (tile_size_start == NULL)
    return 0;
  u32 *tile_size_end = (u32*)DWLmalloc((tile_cols - 1) * sizeof(u32));
  if (tile_size_end == NULL) {
    DWLfree(tile_size_start);
    return 0;
  }
  for (i = 0; i < tile_cols - 1; i++) {
    tile_size_start[i] = (tile_col_mem[i] << sb_size) / 2 - (i == 0 ? 0 : filter_bypass ? 0 : 8);
    tile_size_end[i] = (tile_col_mem[i + 1] << sb_size) / 2 - ((i + 1) == 0 ? 0 : filter_bypass ? 0 : 8);
  }
  for (i = 0; i < tile_cols - 1; i++) {
    if ((ppu_cfg->crop.x / 2 >= tile_size_start[i]) && (ppu_cfg->crop.x / 2 < tile_size_end[i])) {
      start_tile = i + 1;
      break;
    }
  }
  /* x_pos*/
  for (i = 0; i < 8320; i++) {
    xRow[i] = 0;
    xRow_ch[i] = 0;
  }
  scale_ratio_x_inv = FDIVI(TOFIX(ppu_cfg->crop.width, 16) + ppu_cfg->scale.width / 2, ppu_cfg->scale.width);
  if (scale_ratio_x_inv % 2) scale_ratio_x_inv++;

  if (ppu_cfg->src_sel_mode == DOWN_ROUND) {
    xDstInSrc = scale_ratio_x_inv - 65536;
  } else if (ppu_cfg->src_sel_mode == NO_ROUND) {
    xDstInSrc = 0;
  } else {
    xDstInSrc = scale_ratio_x_inv;
  }
  pos = xDstInSrc / 2 - scale_ratio_x_inv;
  x_pos = 0;
  for (i = 0; i < ppu_cfg->scale.width; i++) {
    pos += scale_ratio_x_inv;
    x_pos = FLOOR(pos+32768);
    index = x_pos + ppu_cfg->x_filter_size/2;
    if (index == last_index) {
      if (first_flag) {
        first_flag = 0;
        xRow[index] = 0;
      } else {
        xRow[index]++;
      }
      last_index = index;
    } else {
      for (j = last_index + 1; j < index; j++)
        xRow[j] = xRow[last_index];
      if (first_flag) {
        first_flag = 0;
        xRow[index] = 0;
      } else {
        xRow[index] = xRow[last_index] + 1;
      }
      last_index = index;
    }
  }

  for (i = start_tile; i < tile_cols - 1; i++) {
    if (xRow[tile_size_start[i]] == xRow[tile_size_end[i]]) {
      DWLfree(tile_size_start);
      DWLfree(tile_size_end);
      return 0;
    }
  }

  sub_x = ppu_cfg->chroma_format == PP_CHROMA_444 ? 1 : 2;
  crop_w_ch = sub_x == 2 ? (ppu_cfg->crop.width + 1) / sub_x : ppu_cfg->crop.width / sub_x;
  scale_w_ch = sub_x == 2 ? (ppu_cfg->scale.width + 1) / sub_x : ppu_cfg->scale.width / sub_x;

  scale_ratio_x_inv_ch = FDIVI(TOFIX(crop_w_ch, 16) + scale_w_ch / 2, scale_w_ch);
  if (scale_ratio_x_inv_ch % 2) scale_ratio_x_inv_ch++;
  if (ppu_cfg->src_sel_mode == DOWN_ROUND) {
    xDstInSrc = scale_ratio_x_inv_ch - 65536;
  } else if (ppu_cfg->src_sel_mode == NO_ROUND) {
    xDstInSrc = 0;
  } else {
    xDstInSrc = scale_ratio_x_inv_ch;
  }
  pos = xDstInSrc / 2 - scale_ratio_x_inv_ch;
  x_pos = 0;
  for (i = 0; i < scale_w_ch; i++) {
    pos += scale_ratio_x_inv_ch;
    x_pos = FLOOR(pos+32768);
    index = x_pos + ppu_cfg->x_filter_size/2;
    if (index == last_index) {
      if (first_flag) {
        first_flag = 0;
        xRow_ch[index] = 0;
      } else {
        xRow_ch[index]++;
      }
      last_index = index;
    } else {
      for (j = last_index + 1; j < index; j++)
        xRow_ch[j] = xRow_ch[last_index];
      if (first_flag) {
        first_flag = 0;
        xRow_ch[index] = 0;
      } else {
        xRow_ch[index] = xRow_ch[last_index] + 1;
      }
      last_index = index;
    }
  }
  for (i = start_tile; i < tile_cols - 1; i++) {
    if (xRow_ch[tile_size_start[i]] == xRow_ch[tile_size_end[i]]) {
      DWLfree(tile_size_start);
      DWLfree(tile_size_end);
      return 0;
    }
  }
  DWLfree(tile_size_start);
  DWLfree(tile_size_end);;
  return 1;
}

u32 CalcOnePpUnitFBCHeaderPayloadSize(PpUnitIntConfig *ppu_cfg, u32 header) {
  u32 width_in_block = 0, height_in_block = 0;
  u32 block_payload_size = 0;
  u32 header_size = 0, payload_size = 0;
  u32 pp_width = ppu_cfg->crop2.enabled ? ppu_cfg->crop2.width : ppu_cfg->scale.width;
  u32 pp_height = ppu_cfg->crop2.enabled ? ppu_cfg->crop2.height : ppu_cfg->scale.height;
  if (ppu_cfg->tile_mode == TILED16x16) {
    width_in_block = NEXT_MULTIPLE(pp_width, 16) >> 4;
    height_in_block = NEXT_MULTIPLE(pp_height, 16) >> 4;
    if(ppu_cfg->pp_fbc_tile_enable) {
      width_in_block = NEXT_MULTIPLE(width_in_block, 8);
      height_in_block = NEXT_MULTIPLE(height_in_block, 8);
    }
  } else if (ppu_cfg->tile_mode == TILED32x8) {
    width_in_block = NEXT_MULTIPLE(pp_width, 32) >> 5;
    height_in_block = NEXT_MULTIPLE(pp_height, 8) >> 3;
    if(ppu_cfg->pp_fbc_tile_enable) {
      width_in_block = NEXT_MULTIPLE(width_in_block, 8);
      height_in_block = NEXT_MULTIPLE(height_in_block, 8);
    }
  }
  if (ppu_cfg->pixel_width == 8)
    block_payload_size = 384;
  else
    block_payload_size = 512;
  if(ppu_cfg->pp_fbc_tile_enable) {
    header_size = NEXT_MULTIPLE(width_in_block * 16 * height_in_block, 4096);
    payload_size = NEXT_MULTIPLE(block_payload_size * width_in_block *height_in_block, 4096);
  } else {
    header_size = NEXT_MULTIPLE(width_in_block * 16 * height_in_block, 128);
    payload_size = NEXT_MULTIPLE(block_payload_size * width_in_block *height_in_block, 128);
  }
  if (header)
    return header_size;
  else
    return payload_size;
}
u64 CalcOnePpUnitPVFBCPlane0Size(PpUnitIntConfig *ppu_cfg) {
  u64 pp_height = 0, pp_width = 0;
  u64 payload_size = 0, header_size = 0, plane0_size = 0;
  if (ppu_cfg->crop2.enabled){
    pp_width = ppu_cfg->crop2.width;
    pp_height = ppu_cfg->crop2.height;
  } else {
    pp_width = ppu_cfg->scale.width;
    pp_height = ppu_cfg->scale.height;
  }
  pp_width = ppu_cfg->out_format == PP_OUT_FMT_YUV420_P010 ? pp_width * 2 : pp_width;
  //pp_width = ppu_cfg->out_format == PP_OUT_FMT_PVFBC_YUV420_P010 ? pp_width * 2 : pp_width;
  if(ppu_cfg->pp_pvfbc_tile_type == PVFBC_TILED16x4)
    payload_size = NEXT_MULTIPLE(pp_width, 64) * NEXT_MULTIPLE(pp_height, 4);
  else if(ppu_cfg->pp_pvfbc_tile_type == PVFBC_TILED8x8)
    payload_size = NEXT_MULTIPLE(pp_width, 32) * NEXT_MULTIPLE(pp_height, 8);
  header_size = NEXT_MULTIPLE(payload_size / 256, 256);
  plane0_size = payload_size + header_size;
  if(ppu_cfg->pp_pvfbc_bypass==1){
    if(ppu_cfg->pp_pvfbc_tile_type == PVFBC_TILED8x8)
      plane0_size = ppu_cfg->ystride * (NEXT_MULTIPLE(pp_height, 8) / 8);
    else if(ppu_cfg->pp_pvfbc_tile_type == PVFBC_TILED16x4)
      plane0_size = ppu_cfg->ystride * (NEXT_MULTIPLE(pp_height, 4) / 4);
  } else {
    ppu_cfg->pvfbc_plane0_headr_size = header_size;
  }
  return plane0_size;
}
u64 CalcOnePpUnitPVFBCPlane1Size(PpUnitIntConfig *ppu_cfg, u32 mono_chrome) {
  u64 pp_height = 0, pp_width = 0;
  u64 payload_size = 0, header_size = 0, plane1_size = 0;
  if (!ppu_cfg->monochrome && !mono_chrome && !ppu_cfg->rgb &&
    !ppu_cfg->rgb_planar && !ppu_cfg->out_yuyv && !ppu_cfg->out_uyvy &&
    (ppu_cfg->tile_mode != TILED16x16)) {
    if (ppu_cfg->crop2.enabled) {/*semiplanar: only support 420 semiplanar*/
      pp_width = ppu_cfg->sub_x == 2 ? NEXT_MULTIPLE(ppu_cfg->crop2.width, 2) : ppu_cfg->crop2.width * 2;
      pp_height = ppu_cfg->sub_y == 2 ? (ppu_cfg->crop2.height + 1) / 2 : ppu_cfg->crop2.height;
    } else {
      pp_width = ppu_cfg->sub_x == 2 ? NEXT_MULTIPLE(ppu_cfg->scale.width , 2) : ppu_cfg->scale.width * 2;
      pp_height = ppu_cfg->sub_y == 2 ? (ppu_cfg->scale.height + 1) / 2 : ppu_cfg->scale.height;
    }
    pp_width = ppu_cfg->out_format == PP_OUT_FMT_YUV420_P010 ? pp_width * 2 : pp_width;
    //pp_width = ppu_cfg->out_format == PP_OUT_FMT_PVFBC_YUV420_P010 ? pp_width * 2 : pp_width;
    if(ppu_cfg->pp_pvfbc_tile_type == PVFBC_TILED16x4)
      payload_size = NEXT_MULTIPLE(pp_width, 64) * NEXT_MULTIPLE(pp_height, 4);
    else if(ppu_cfg->pp_pvfbc_tile_type == PVFBC_TILED8x8)
      payload_size = NEXT_MULTIPLE(pp_width, 32) * NEXT_MULTIPLE(pp_height, 8);
    header_size = NEXT_MULTIPLE(payload_size / 256, 256);
    plane1_size = payload_size + header_size;
  }
  if(ppu_cfg->pp_pvfbc_bypass==1){
    if(ppu_cfg->pp_pvfbc_tile_type == PVFBC_TILED8x8){
      u64 ds_aln_h_chroma = NEXT_MULTIPLE(pp_height, 8);
      plane1_size = ppu_cfg->cstride * (ds_aln_h_chroma / 8);
    }else if(ppu_cfg->pp_pvfbc_tile_type == PVFBC_TILED16x4){
      u64 ds_aln_h_chroma = NEXT_MULTIPLE(pp_height, 4);
      plane1_size = ppu_cfg->cstride * (ds_aln_h_chroma / 4);
    }
  } else {
    ppu_cfg->pvfbc_plane1_headr_size = header_size;
  }
  return plane1_size;
}

void CalcOnePpUnitLumaSize(PpUnitIntConfig *ppu_cfg, u64* size_array) {
  u64 pp_height = 0, pp_stride = 0;
  u64 luma_size = 0;

  ppu_cfg->frm_width = ppu_cfg->scale.width;
  ppu_cfg->frm_height = ppu_cfg->scale.height;

  if (ppu_cfg->tiled_e)
    pp_height = NEXT_MULTIPLE(ppu_cfg->scale.height, 4) / 4;
  else {
    if (ppu_cfg->crop2.enabled)
      pp_height = ppu_cfg->crop2.height;
    else
      pp_height = ppu_cfg->frm_height;
  }
  pp_stride = ppu_cfg->ystride;

  if (ppu_cfg->tiled_e) {
    if(ppu_cfg->tile_mode == TILED4x4) {
      luma_size = pp_stride * (NEXT_MULTIPLE(ppu_cfg->scale.height, 4) / 4);
    } else if (ppu_cfg->tile_mode == TILED16x16) {
      luma_size = pp_stride * (NEXT_MULTIPLE(ppu_cfg->scale.height, 16) / 16);
    } else if (ppu_cfg->tile_mode == TILED8x8 || ppu_cfg->tile_mode == RECONTILED8x8) {
      luma_size = pp_stride * (NEXT_MULTIPLE(ppu_cfg->scale.height, 8) / 8);
    } else if (ppu_cfg->tile_mode == TILED64x64) {
      luma_size = pp_stride * (NEXT_MULTIPLE(ppu_cfg->scale.height, 64) / 64);
    } else if (ppu_cfg->tile_mode == TILED2x4) {/*YUYV/UYVY*/
      luma_size = pp_stride * (NEXT_MULTIPLE(ppu_cfg->scale.height, 4) / 4);
    } else if (ppu_cfg->tile_mode == PACKED422_TILED8x8) {/*YUYV/UYVY*/
      luma_size = pp_stride * (NEXT_MULTIPLE(ppu_cfg->scale.height, 8) / 8);
    } else {
      luma_size = pp_stride * ppu_cfg->scale.height;
    }
  } else if (ppu_cfg->rgb) {
    luma_size = NEXT_MULTIPLE(pp_stride * pp_height, PLANE_ALIGNMENT);
  } else if (ppu_cfg->rgb_planar) {
    luma_size = 3 * NEXT_MULTIPLE(pp_stride * pp_height, PLANE_ALIGNMENT);
  } else {
    luma_size = pp_stride * pp_height;
  }
  size_array[0] = luma_size;
}

void CalcOnePpUnitChromaSize(PpUnitIntConfig *ppu_cfg, u32 mono_chrome, u64* size_array) {
  u32 pp_height = 0, pp_stride = 0;
  u64 chroma_size = 0;

  /* for dec400 + supertile optimization */
  if(ppu_cfg->tile_mode && ppu_cfg->tile_mode == TILED64x64 && ppu_cfg->dec400_enabled)
    chroma_size = NEXT_MULTIPLE(NEXT_MULTIPLE(ppu_cfg->scale.width, 8) / 8, 64) * NEXT_MULTIPLE(ppu_cfg->scale.height,8) / 8;
  if (!ppu_cfg->monochrome && !mono_chrome && !ppu_cfg->rgb &&
      !ppu_cfg->rgb_planar && !ppu_cfg->out_yuyv && !ppu_cfg->out_uyvy &&
      (ppu_cfg->tile_mode != TILED16x16)) {
    if (ppu_cfg->tiled_e && !ppu_cfg->tile_mode) {
      /*support odd crop*/
      //pp_height = NEXT_MULTIPLE(ppu_cfg->scale.height/ppu_cfg->sub_y, 4) / 4;
      pp_height = NEXT_MULTIPLE((ppu_cfg->sub_y == 2 ? (ppu_cfg->scale.height + 1) / 2 : ppu_cfg->scale.height), 4) / 4;
    } else if (ppu_cfg->crop2.enabled) {
      if (ppu_cfg->planar)
        /*support odd crop*/
        //pp_height = ppu_cfg->crop2.height * (ppu_cfg->sub_y == 1 ? 2 : 1);
        pp_height = ppu_cfg->sub_y == 2 ? (ppu_cfg->crop2.height + 1) & ~0x1 : ppu_cfg->crop2.height * 2;
      else
        /*support odd crop*/
        //pp_height = ppu_cfg->crop2.height / ppu_cfg->sub_y;
        pp_height = ppu_cfg->sub_y == 2 ? (ppu_cfg->crop2.height + 1)/ 2 : ppu_cfg->crop2.height;
    } else {
      if (ppu_cfg->planar) {
        /*support odd crop*/
        //pp_height = ppu_cfg->scale.height * (ppu_cfg->sub_y == 1 ? 2 : 1);
        pp_height = ppu_cfg->sub_y == 2 ? (ppu_cfg->scale.height + 1) & ~0x1 : ppu_cfg->scale.height * 2;
      } else {
        /*support odd crop*/
        //pp_height = ppu_cfg->scale.height / ppu_cfg->sub_y;
        pp_height = ppu_cfg->sub_y == 2 ? (ppu_cfg->scale.height + 1) / 2 : ppu_cfg->scale.height;
      }
    }
    pp_stride = ppu_cfg->cstride;
    if (ppu_cfg->tiled_e && ppu_cfg->tile_mode) {
      if (ppu_cfg->tile_mode == TILED8x8  || ppu_cfg->tile_mode == RECONTILED8x8)
        chroma_size = ppu_cfg->cstride * (ppu_cfg->sub_y == 2 ? NEXT_MULTIPLE((ppu_cfg->scale.height + 1) / 2, 4) / 4 : NEXT_MULTIPLE(ppu_cfg->scale.height, 4) / 4);
      else if (ppu_cfg->tile_mode == TILED64x64 && !(ppu_cfg->rgb || ppu_cfg->rgb_planar)) {
        chroma_size = ppu_cfg->cstride * (NEXT_MULTIPLE((ppu_cfg->scale.height + 1) / 2, 32) / 32); //only 420
      } else {
        /*support odd crop*/
        //chroma_size = ppu_cfg->cstride * ppu_cfg->scale.height / 2;
        chroma_size = ppu_cfg->cstride * (ppu_cfg->sub_y == 2 ? (ppu_cfg->scale.height + 1) / 2 : ppu_cfg->scale.height);
      }
    } else {
      chroma_size = pp_stride * pp_height;
    }
  }
  if (ppu_cfg->luma_sum_e || ppu_cfg->hist_local_e || ppu_cfg->hist_global_e || ppu_cfg->hist_hsv_e) {
    chroma_size = (256 * 32 + 24 * 128 * 32 +2  * 16 * 32 + 256 * 32) / 4;
  }
  size_array[4] = chroma_size;
}

u64 CalcOnePpUnitDec400TblSize(const struct DecHwFeatures *hw_feature, PpUnitIntConfig *ppu_cfg, u64 luma_size, u64 chroma_size, u32 i) {
  u64 dec400_table_size = 0;
    if(ppu_cfg->dec400_enabled){
      //luma
      if(ppu_cfg->dec400_conformance_test_en)
        ppu_cfg->dec400_luma_table.logical_size = NEXT_MULTIPLE(DEC400_IN_HEADER_SIZE + (luma_size / DEC400_TILE_SIZE * 4 + 7) / 8, DEC400_TILE_SIZE);
      else
        ppu_cfg->dec400_luma_table.logical_size = NEXT_MULTIPLE((luma_size / DEC400_TILE_SIZE * 4 + 7) / 8, 16);
      /*always align DEC400_TBL_ALIGN_FACTOR bytes for luma/chroma table to avoid table pading data covers previous data,
      and uv start address align*/
      if (hw_feature->pp_comp_support[i] == 2 && hw_feature->pp_comp_version[i] == 2 && ppu_cfg->planar) {
        ppu_cfg->dec400_luma_table.size = NEXT_MULTIPLE(DEC400_IN_HEADER_SIZE + NEXT_MULTIPLE(luma_size / DEC400_TILE_SIZE / 2, 128), 256);
      } else {
        ppu_cfg->dec400_luma_table.size = NEXT_MULTIPLE(DEC400_IN_HEADER_SIZE + NEXT_MULTIPLE(luma_size / DEC400_TILE_SIZE, 128), 256);
      }
      dec400_table_size += ppu_cfg->dec400_luma_table.size;
      ppu_cfg->total_buffer_size[0] += ppu_cfg->dec400_luma_table.size;

      //chroma
      if(chroma_size && ppu_cfg->tile_mode != TILED64x64) {
        if(ppu_cfg->dec400_conformance_test_en) {
          if (hw_feature->pp_comp_support[i] == 2 && hw_feature->pp_comp_version[i] == 2 && ppu_cfg->planar) {
            ppu_cfg->dec400_chroma_table.logical_size = 2 * NEXT_MULTIPLE(DEC400_IN_HEADER_SIZE + (chroma_size / 2 / DEC400_TILE_SIZE * 4 + 7) / 8, DEC400_TILE_SIZE);
          } else {
            ppu_cfg->dec400_chroma_table.logical_size = NEXT_MULTIPLE(DEC400_IN_HEADER_SIZE + (chroma_size / DEC400_TILE_SIZE * 4 + 7) / 8, DEC400_TILE_SIZE);
          }
        } else {
          if (hw_feature->pp_comp_support[i] == 2 && hw_feature->pp_comp_version[i] == 2 && ppu_cfg->planar) {
            ppu_cfg->dec400_chroma_table.logical_size = 2 * NEXT_MULTIPLE((chroma_size / 2 / DEC400_TILE_SIZE * 4 + 7) / 8, 16);
          } else {
            ppu_cfg->dec400_chroma_table.logical_size = NEXT_MULTIPLE((chroma_size / DEC400_TILE_SIZE * 4 + 7) / 8, 16);
          }
        }
        /*always align DEC400_TBL_ALIGN_FACTOR bytes for luma/chroma table to avoid table pading data covers previous data,
          and uv start address align*/
        if (hw_feature->pp_comp_support[i] == 2 && hw_feature->pp_comp_version[i] == 2 && ppu_cfg->planar) {
          ppu_cfg->dec400_chroma_table.size = 2 * NEXT_MULTIPLE(DEC400_IN_HEADER_SIZE + NEXT_MULTIPLE(chroma_size / 2 / DEC400_TILE_SIZE / 2, 128), 256);
        } else {
          ppu_cfg->dec400_chroma_table.size = NEXT_MULTIPLE(DEC400_IN_HEADER_SIZE + NEXT_MULTIPLE(chroma_size / DEC400_TILE_SIZE, 128), 256);
        }
      }
      dec400_table_size += ppu_cfg->dec400_chroma_table.size;
      ppu_cfg->total_buffer_size[1] += ppu_cfg->dec400_chroma_table.size;
    }
  return dec400_table_size;
}

/* INTERNAL_DEC400: Embed into VCD IPS, one PP buffer layout:
  | luma header |     luma ts       | luma compressed data  |  chroma header  |     chroma ts     | chroma compressed data  |
  |--128 Bytes--|-------------------|  >= shaper_alignment  |----128 Bytes----|-------------------|    >= shaper_alignment  |
  |<---------256 bytes align------->|                       |<-----------256 bytes align--------->|
*/
u64 CalcPpUnitBufferSize(const struct DecHwFeatures *hw_feature, PpUnitIntConfig *ppu_cfg, u32 mono_chrome, u32 use_separated_pp) {
  u32 i = 0;
  u64 luma_size = 0,chroma_size = 0;
  u64 luma_size_1 = 0,chroma_size_1 = 0;
  u64 luma_size_2 = 0,chroma_size_2 = 0;
  u64 luma_size_3 = 0,chroma_size_3 = 0;
  u64 size_array[8];
  u64 ext_buffer_size = 0, dec400_table_size = 0;
  u32 header_size = 0, payload_size = 0;
  u32 dec400_pln0_tile_status_size = 0, dec400_pln1_tile_status_size = 0;
  u64 plane0_size = 0, plane1_size = 0;
  for (i = 0; i < DEC_MAX_PPU_COUNT; i++, ppu_cfg++) {
    DWLmemset(ppu_cfg->total_buffer_size, 0, sizeof(ppu_cfg->total_buffer_size));
    if (!ppu_cfg->enabled) continue;
    //only pp0 support embedded FBC core
    if (i == 0 && ppu_cfg->pp_comp && (ppu_cfg->tile_mode == TILED16x16 || ppu_cfg->tile_mode == TILED32x8)) {
      header_size = CalcOnePpUnitFBCHeaderPayloadSize(ppu_cfg, 1);
      payload_size = CalcOnePpUnitFBCHeaderPayloadSize(ppu_cfg, 0);
      ppu_cfg->header_offset = ext_buffer_size;
      ppu_cfg->payload_offset = ext_buffer_size + header_size;
    } else if (ppu_cfg->pp_pvfbc) {
      plane0_size = CalcOnePpUnitPVFBCPlane0Size(ppu_cfg);
      plane1_size = CalcOnePpUnitPVFBCPlane1Size(ppu_cfg, mono_chrome);
      ppu_cfg->plane0_offset = ext_buffer_size;
      ppu_cfg->plane1_offset = ext_buffer_size + plane0_size;
    }
    CalcOnePpUnitLumaSize(ppu_cfg, size_array);
    if (ppu_cfg->dec400_enabled) size_array[0] = NEXT_MULTIPLE(size_array[0], 256);
    luma_size = size_array[0];
    luma_size_1 = size_array[1];
    luma_size_2 = size_array[2];
    luma_size_3 = size_array[3];

    if (ppu_cfg->hist_local_e || ppu_cfg->hist_global_e || ppu_cfg->hist_hsv_e)
      luma_size = 0;

    ppu_cfg->total_buffer_size[0] = luma_size;
    ppu_cfg->luma_size = luma_size;
    ppu_cfg->luma_size_1 = luma_size_1;
    ppu_cfg->luma_size_2 = luma_size_2;
    ppu_cfg->luma_size_3 = luma_size_3;
    ppu_cfg->hdr_offset = ext_buffer_size;

    CalcOnePpUnitChromaSize(ppu_cfg, mono_chrome, size_array);
    if (ppu_cfg->dec400_enabled) size_array[4] = NEXT_MULTIPLE(size_array[4], 256);
    chroma_size = size_array[4];
    chroma_size_1 = size_array[5];
    chroma_size_2 = size_array[6];
    chroma_size_3 = size_array[7];
    ppu_cfg->chroma_size = chroma_size;
    ppu_cfg->chroma_size_1 = chroma_size_1;
    ppu_cfg->chroma_size_2 = chroma_size_2;
    ppu_cfg->chroma_size_3 = chroma_size_3;
    if (MAX_OUTPUT_PLANE >= 3) {
      if (ppu_cfg->planar) {
        ppu_cfg->total_buffer_size[1] = ppu_cfg->total_buffer_size[2] = chroma_size / 2;
      } else {
        ppu_cfg->total_buffer_size[1] = chroma_size;
        ppu_cfg->total_buffer_size[2] = 0;
      }
    }

    dec400_pln0_tile_status_size = 0;
    dec400_pln1_tile_status_size = 0;
    if (ppu_cfg->dec400_enabled) {/*suport PP DEC400*/
      dec400_table_size += CalcOnePpUnitDec400TblSize(hw_feature, ppu_cfg, luma_size, chroma_size, i);
      dec400_pln0_tile_status_size = ppu_cfg->dec400_luma_table.size;
      if(chroma_size && ppu_cfg->tile_mode != TILED64x64)
        dec400_pln1_tile_status_size = ppu_cfg->dec400_chroma_table.size;
      ppu_cfg->dec400_pln0_tile_status_offset = ext_buffer_size;
      ppu_cfg->dec400_pln1_tile_status_offset = ext_buffer_size + dec400_pln0_tile_status_size + luma_size;
    }
    //buf_off_set_by_user only used in heif format
    if (!ppu_cfg->buf_off_set_by_user) {
      if(ppu_cfg->pp_pvfbc) {
        // ppu_cfg->luma_offset = ext_buffer_size;
        // ppu_cfg->chroma_offset = ext_buffer_size + plane0_size;
        ppu_cfg->luma_offset = ppu_cfg->plane0_offset;
        ppu_cfg->chroma_offset = ppu_cfg->plane1_offset;
      } else if(ppu_cfg->dec400_enabled) {
	      ppu_cfg->luma_offset = dec400_pln0_tile_status_size + ppu_cfg->dec400_pln0_tile_status_offset;
        ppu_cfg->chroma_offset = dec400_pln1_tile_status_size + ppu_cfg->dec400_pln1_tile_status_offset;
      } else {
        u32 base_offset = ext_buffer_size;
        if (use_separated_pp) {
          base_offset = 0;
          dec400_table_size = 0;
        }
        ppu_cfg->luma_offset = base_offset;
        ppu_cfg->chroma_offset = base_offset + luma_size;
      }
    }
    //only pp0 support embedded FBC core
    if (i == 0 && ppu_cfg->pp_comp && (ppu_cfg->tile_mode == TILED16x16 || ppu_cfg->tile_mode == TILED32x8)) {
#ifdef MODEL_SIMULATION
      ppu_cfg->total_buffer_size[0] = MAX(NEXT_MULTIPLE(header_size + payload_size, 16), NEXT_MULTIPLE(luma_size + chroma_size, 16));
#else
      ppu_cfg->total_buffer_size[0] = NEXT_MULTIPLE(header_size + payload_size, 16);
#endif
      ext_buffer_size += ppu_cfg->total_buffer_size[0];
      if (MAX_OUTPUT_PLANE >= 3) {
        ppu_cfg->total_buffer_size[1] = ppu_cfg->total_buffer_size[2] = 0;
      }
    } else if (ppu_cfg->dec400_enabled) { /*supoort PP DEC400*/
      ext_buffer_size += luma_size + chroma_size + dec400_pln0_tile_status_size + dec400_pln1_tile_status_size;
    } else if (ppu_cfg->pp_pvfbc) {
      // #ifdef MODEL_SIMULATION
      // ext_buffer_size += NEXT_MULTIPLE(MAX(plane0_size, luma_size) + MAX(plane1_size, chroma_size), 16);
      // #else
      // ext_buffer_size += plane0_size + plane1_size; /*always 16byte align*/
      // #endif
      ext_buffer_size += plane0_size + plane1_size; /*always 16byte align*/
    }  else {
      ext_buffer_size += NEXT_MULTIPLE(luma_size + chroma_size, 16);
    }
    if (i < DEC_MAX_PPU_COUNT && ppu_cfg[1].enabled && ppu_cfg[1].dec400_enabled) {
      /* offset need align to 256 before dec400 channel */
      ext_buffer_size = NEXT_MULTIPLE(ext_buffer_size, 256);
    }
  }
  return ext_buffer_size;
}

void PpFillDec400TblInfo(PpUnitIntConfig *ppu_cfg,
                         const u32 *pp_start_vir_addr,
                         addr_t pp_start_bus_addr,
                         struct DWLLinearMem *luma_tbl,
                         struct DWLLinearMem *chroma_tbl) {

  addr_t dec400_table_luma_offset = 0;
  addr_t dec400_table_chroma_offset = 0;
  dec400_table_luma_offset = ppu_cfg->dec400_pln0_tile_status_offset + DEC400_IN_HEADER_SIZE;
  dec400_table_chroma_offset = ppu_cfg->dec400_pln1_tile_status_offset + DEC400_IN_HEADER_SIZE;
  luma_tbl->virtual_address = (u32*)((u8*)pp_start_vir_addr + dec400_table_luma_offset);
  luma_tbl->bus_address = pp_start_bus_addr + dec400_table_luma_offset;
  luma_tbl->logical_size = ppu_cfg->dec400_luma_table.logical_size;
  luma_tbl->size = ppu_cfg->dec400_luma_table.size;
  if (!ppu_cfg->monochrome) {
    chroma_tbl->virtual_address = (u32*)((u8*)pp_start_vir_addr + dec400_table_chroma_offset);
    chroma_tbl->bus_address = pp_start_bus_addr + dec400_table_chroma_offset;
    chroma_tbl->logical_size = ppu_cfg->dec400_chroma_table.logical_size;
    chroma_tbl->size = ppu_cfg->dec400_chroma_table.size;
  }
}

void PpUnitSetIntConfig(PpUnitIntConfig *ppu_int_cfg,
                        PpUnitConfig *ppu_ext_cfg,
                        const struct DecHwFeatures *hw_feature,
                        u32 pixel_width, u32 frame_only,
                        u32 mono_chrome) {
  u32 i, idx;
  u32 chroma_format = 0;
  u8 flag_3dlut = 0;
  if (ppu_int_cfg == NULL || ppu_ext_cfg == NULL)
    return;

  SetPpByReleaseHwFeatures(ppu_int_cfg, ppu_ext_cfg, hw_feature);
  for (i = 0; i < DEC_MAX_PPU_COUNT; i++) {
    ppu_int_cfg->enabled = ppu_ext_cfg->enabled;
    if (!ppu_int_cfg->enabled) {
      ppu_ext_cfg++;
      ppu_int_cfg++;
      continue;
    }
    ppu_int_cfg->dis_checker = ppu_ext_cfg->dis_checker;
    ppu_int_cfg->tiled_e = ppu_ext_cfg->tiled_e;
    ppu_int_cfg->rgb = ppu_ext_cfg->rgb;
    ppu_int_cfg->rgb_planar = ppu_ext_cfg->rgb_planar;
    if (ppu_int_cfg->rgb_planar)
      ppu_int_cfg->rgb = 0;
    ppu_int_cfg->cr_first = ppu_ext_cfg->cr_first;
    ppu_int_cfg->shaper_enabled = ppu_ext_cfg->shaper_enabled;
    ppu_int_cfg->shaper_no_pad = ppu_ext_cfg->shaper_no_pad;
    ppu_int_cfg->shaper_tile_edge_pad = ppu_ext_cfg->shaper_tile_edge_pad;
    ppu_int_cfg->dec400_align = ppu_ext_cfg->dec400_align;
    ppu_int_cfg->dec400_conformance_test_en = ppu_ext_cfg->dec400_conformance_test_en;
    ppu_int_cfg->crop.enabled = ppu_ext_cfg->crop.enabled;
    ppu_int_cfg->crop.set_by_user = ppu_ext_cfg->crop.set_by_user;
    ppu_int_cfg->crop.x = ppu_ext_cfg->crop.x;
    ppu_int_cfg->crop.y = ppu_ext_cfg->crop.y;
    ppu_int_cfg->crop.width = ppu_ext_cfg->crop.width;
    ppu_int_cfg->crop.height = ppu_ext_cfg->crop.height;
    ppu_int_cfg->crop2.enabled = ppu_ext_cfg->crop2.enabled;
    ppu_int_cfg->crop2.x = ppu_ext_cfg->crop2.x;
    ppu_int_cfg->crop2.y = ppu_ext_cfg->crop2.y;
    ppu_int_cfg->crop2.width = ppu_ext_cfg->crop2.width;
    ppu_int_cfg->crop2.height = ppu_ext_cfg->crop2.height;
    ppu_int_cfg->scale.enabled = ppu_ext_cfg->scale.enabled;
    ppu_int_cfg->scale.scale_by_ratio = ppu_ext_cfg->scale.scale_by_ratio;
    ppu_int_cfg->scale.ratio_x = ppu_ext_cfg->scale.ratio_x;
    ppu_int_cfg->scale.ratio_y = ppu_ext_cfg->scale.ratio_y;
    ppu_int_cfg->scale.width = ppu_ext_cfg->scale.width;
    ppu_int_cfg->scale.height = ppu_ext_cfg->scale.height;
    ppu_int_cfg->pp_filter = ppu_ext_cfg->pp_filter;
    ppu_int_cfg->frm_width = ppu_int_cfg->scale.width;
    ppu_int_cfg->frm_height = ppu_int_cfg->scale.height;
    ppu_int_cfg->monochrome = ppu_ext_cfg->monochrome;
    ppu_int_cfg->Explicit_Specified_010 = ppu_ext_cfg->Explicit_Specified_010;
    ppu_int_cfg->out_p010 = ppu_ext_cfg->out_p010;
    ppu_int_cfg->out_I010 = ppu_ext_cfg->out_I010;
    ppu_int_cfg->out_L010 = ppu_ext_cfg->out_L010;
    ppu_int_cfg->out_1010 = ppu_ext_cfg->out_1010;
    ppu_int_cfg->out_p012 = ppu_ext_cfg->out_p012;
    ppu_int_cfg->out_I012 = ppu_ext_cfg->out_I012;
    ppu_int_cfg->out_uyvy = ppu_ext_cfg->out_uyvy;
    ppu_int_cfg->out_yuyv = ppu_ext_cfg->out_yuyv;
    ppu_int_cfg->out_cut_8bits = ppu_ext_cfg->out_cut_8bits;
    ppu_int_cfg->planar = ppu_ext_cfg->planar;
    ppu_int_cfg->planar_f = ppu_ext_cfg->planar_f;
    ppu_int_cfg->align = ppu_ext_cfg->align;
    ppu_int_cfg->align_h = ppu_ext_cfg->align_h;
    ppu_int_cfg->align_pixel_w = ppu_ext_cfg->align_pixel_w;
    ppu_int_cfg->ystride = ppu_ext_cfg->ystride;
    ppu_int_cfg->cstride = ppu_ext_cfg->cstride;
    ppu_int_cfg->stride_set_by_user = ppu_ext_cfg->stride_set_by_user;
    ppu_int_cfg->video_range = ppu_ext_cfg->video_range;
    ppu_int_cfg->tile_mode = ppu_ext_cfg->tile_mode;
    ppu_int_cfg->yuv_supertile_pad_copy_e = ppu_ext_cfg->yuv_supertile_pad_copy_e;
    //ppu_int_cfg->in_lib_name = ppu_ext_cfg->in_lib_name;

    ppu_int_cfg->sbi_en = ppu_ext_cfg->sbi_en;
    ppu_int_cfg->sbi_lu_stream_id = ppu_ext_cfg->sbi_lu_stream_id;
    ppu_int_cfg->sbi_lu_segmentbuffer_entry_size = ppu_ext_cfg->sbi_lu_segmentbuffer_entry_size;
    ppu_int_cfg->sbi_lu_segmentbuffer_entry_count = ppu_ext_cfg->sbi_lu_segmentbuffer_entry_count;
    ppu_int_cfg->sbi_lu_segmentbuffer_offset = ppu_ext_cfg->sbi_lu_segmentbuffer_offset;
    ppu_int_cfg->sbi_ch_stream_id = ppu_ext_cfg->sbi_ch_stream_id;
    ppu_int_cfg->sbi_ch_segmentbuffer_entry_size = ppu_ext_cfg->sbi_ch_segmentbuffer_entry_size;
    ppu_int_cfg->sbi_ch_segmentbuffer_entry_count = ppu_ext_cfg->sbi_ch_segmentbuffer_entry_count;
    ppu_int_cfg->sbi_ch_segmentbuffer_offset = ppu_ext_cfg->sbi_ch_segmentbuffer_offset;
    ppu_int_cfg->sbi_coupled = ppu_ext_cfg->sbi_coupled;
    ppu_int_cfg->sbi_lu_reset_offset = ppu_ext_cfg->sbi_lu_reset_offset;
    ppu_int_cfg->sbi_ch_reset_offset = ppu_ext_cfg->sbi_ch_reset_offset;
    ppu_int_cfg->sbi_lu_addr = ppu_ext_cfg->sbi_lu_addr;
    ppu_int_cfg->sbi_ch_addr = ppu_ext_cfg->sbi_ch_addr;

    ppu_int_cfg->rb_en = ppu_ext_cfg->rb_en;
    ppu_int_cfg->rb_height = ppu_ext_cfg->rb_height;
    ppu_int_cfg->rb_num = ppu_ext_cfg->rb_num;
    ppu_int_cfg->rb_mode = ppu_ext_cfg->rb_mode;

    ppu_int_cfg->pp_pvfbc_lu_const0 = ppu_ext_cfg->pp_pvfbc_lu_const0;
    ppu_int_cfg->pp_pvfbc_lu_const1 = ppu_ext_cfg->pp_pvfbc_lu_const1;
    ppu_int_cfg->pp_pvfbc_ch_const0 = ppu_ext_cfg->pp_pvfbc_ch_const0;
    ppu_int_cfg->pp_pvfbc_ch_const1 = ppu_ext_cfg->pp_pvfbc_ch_const1;
    ppu_int_cfg->pp_pvfbc_tile_type = ppu_ext_cfg->pp_pvfbc_tile_type;
    ppu_int_cfg->pp_pvfbc_bypass = ppu_ext_cfg->pp_pvfbc_bypass;
    ppu_int_cfg->pp_fbc_tile_enable = ppu_ext_cfg->pp_fbc_tile_enable;
    ppu_int_cfg->pad_sel = ppu_ext_cfg->pad_sel;
    ppu_int_cfg->src_sel_mode = ppu_ext_cfg->src_sel_mode;
    ppu_int_cfg->pad_Y = ppu_ext_cfg->pad_Y;
    ppu_int_cfg->pad_U = ppu_ext_cfg->pad_U;
    ppu_int_cfg->pad_V = ppu_ext_cfg->pad_V;
    ppu_int_cfg->lc_stripe = ppu_ext_cfg->lc_stripe;
    ppu_int_cfg->lc_int_enabled = ppu_ext_cfg->lc_int_enabled;
    /* hist */
    ppu_int_cfg->hist_local_e = ppu_ext_cfg->hist_local_e;
    ppu_int_cfg->hist_global_e = ppu_ext_cfg->hist_global_e;
    ppu_int_cfg->hist_hsv_e = ppu_ext_cfg->hist_hsv_e;
    ppu_int_cfg->luma_sum_e = ppu_ext_cfg->luma_sum_e;
    ppu_int_cfg->luma_sum_shift = ppu_ext_cfg->luma_sum_shift;
    for(idx = 0; idx < 5; idx ++)
      ppu_int_cfg->hist_offset_x[idx] = ppu_ext_cfg->hist_offset_x[idx];
    for(idx = 0; idx < 3; idx ++)
      ppu_int_cfg->hist_offset_y[idx] = ppu_ext_cfg->hist_offset_y[idx];
    /* norm */
    ppu_int_cfg->norm_enable = ppu_ext_cfg->norm.enable;
    ppu_int_cfg->norm_mean_r = ppu_ext_cfg->norm.mean_r;
    ppu_int_cfg->norm_mean_g = ppu_ext_cfg->norm.mean_g;
    ppu_int_cfg->norm_mean_b = ppu_ext_cfg->norm.mean_b;
    ppu_int_cfg->norm_std_r = ppu_ext_cfg->norm.std_r;
    ppu_int_cfg->norm_std_g = ppu_ext_cfg->norm.std_g;
    ppu_int_cfg->norm_std_b = ppu_ext_cfg->norm.std_b;
    ppu_int_cfg->norm_quant_step = ppu_ext_cfg->norm.quant_step;
    ppu_int_cfg->decnano_e = ppu_ext_cfg->decnano_e;
    ppu_int_cfg->decnano_mode = ppu_ext_cfg->decnano_mode;
#ifdef SUPPORT_LZ77
    ppu_int_cfg->lz77_mode = ppu_ext_cfg->lz77_mode;
#endif
    ppu_int_cfg->yuv400_with_alpha = ppu_ext_cfg->yuv400_with_alpha;
    if(ppu_ext_cfg->norm.enable) {
      switch(ppu_ext_cfg->norm.fmat) {
        case NORM_OUT_FP32:
          ppu_int_cfg->norm_fmat = NORM_OUT_FP32;
          break;
        case NORM_OUT_FP16:
          ppu_int_cfg->norm_fmat = NORM_OUT_FP16;
          break;
        case NORM_OUT_INT16:
          ppu_int_cfg->norm_fmat = NORM_OUT_INT16;
          break;
        case NORM_OUT_INT8:
          ppu_int_cfg->norm_fmat = NORM_OUT_INT8;
          break;
        case NORM_OUT_UINT8:
          ppu_int_cfg->norm_fmat = NORM_OUT_UINT8;
          break;
      }
      switch(ppu_ext_cfg->norm.filter) {
        case ZSCORE:
          ppu_int_cfg->norm_filter = ZSCORE;
          break;
        case VSI_ZSCORE:
          ppu_int_cfg->norm_filter = VSI_ZSCORE;
          break;
      }
    }

    if (ppu_ext_cfg->buf_off_set_by_user) {
      ppu_int_cfg->luma_offset = ppu_ext_cfg->luma_offset;
      ppu_int_cfg->chroma_offset = ppu_ext_cfg->chroma_offset;
    }
    ppu_int_cfg->buf_off_set_by_user = ppu_ext_cfg->buf_off_set_by_user;

    if (i == 0) {
      ppu_int_cfg->slice_text.slice_txt_cnt = ppu_ext_cfg->slice_text.slice_txt_cnt;
      DWLmemcpy((void *)ppu_int_cfg->slice_text.slice_params,
                (void *)ppu_ext_cfg->slice_text.slice_params,
                sizeof(ppu_ext_cfg->slice_text.slice_params));
    }

    if (mono_chrome)
      ppu_int_cfg->chroma_format = chroma_format = 0;
    else
      ppu_int_cfg->chroma_format = chroma_format = ppu_ext_cfg->chroma_format;
    // if (ppu_int_cfg->out_uyvy || ppu_int_cfg->out_yuyv)
    //   ppu_int_cfg->cr_first = 0;
    ppu_int_cfg->rgb_format = PP_OUT_RGB888;
    ppu_int_cfg->rgb_stan = BT601;
    ppu_int_cfg->rgb_alpha = 0;
    if (ppu_ext_cfg->rgb || ppu_ext_cfg->rgb_planar) {
      ppu_int_cfg->cr_first = 0;
      switch (ppu_ext_cfg->rgb_format) {
      case DEC_OUT_FRM_RGB888:
        ppu_int_cfg->rgb_format = ppu_ext_cfg->rgb_planar ? PP_OUT_RGB888_P : PP_OUT_BGR888;
        break;
      case DEC_OUT_FRM_BGR888:
        ppu_int_cfg->rgb_format = ppu_ext_cfg->rgb_planar ? PP_OUT_BGR888_P : PP_OUT_RGB888;
        break;
      case DEC_OUT_FRM_R16G16B16:
        ppu_int_cfg->rgb_format = ppu_ext_cfg->rgb_planar ? PP_OUT_R16G16B16_P : PP_OUT_B16G16R16;
        break;
      case DEC_OUT_FRM_B16G16R16:
        ppu_int_cfg->rgb_format = ppu_ext_cfg->rgb_planar ? PP_OUT_B16G16R16_P : PP_OUT_R16G16B16;
        break;
      case DEC_OUT_FRM_ABGR888:
        ppu_int_cfg->rgb_format = PP_OUT_ABGR888;
        break;
      case DEC_OUT_FRM_ARGB888:
        ppu_int_cfg->rgb_format = PP_OUT_ARGB888;
        break;
      case DEC_OUT_FRM_A2B10G10R10:
        ppu_int_cfg->rgb_format = PP_OUT_A2B10G10R10;
        break;
      case DEC_OUT_FRM_A2R10G10B10:
        ppu_int_cfg->rgb_format = PP_OUT_A2R10G10B10;
        break;
      case DEC_OUT_FRM_X2R10G10B10:
        ppu_int_cfg->rgb_format = PP_OUT_X2R10G10B10;
        break;
      case DEC_OUT_FRM_X2B10G10R10:
        ppu_int_cfg->rgb_format = PP_OUT_X2B10G10R10;
        break;
      case DEC_OUT_FRM_XBGR888:
        ppu_int_cfg->rgb_format = PP_OUT_XBGR888;
        break;
      case DEC_OUT_FRM_XRGB888:
        ppu_int_cfg->rgb_format = PP_OUT_XRGB888;
        break;
      case DEC_OUT_FRM_BGRA888:
        ppu_int_cfg->rgb_format = PP_OUT_BGRA888;
        break;
      case DEC_OUT_FRM_RGBA888:
        ppu_int_cfg->rgb_format = PP_OUT_RGBA888;
        break;
      case DEC_OUT_FRM_B10G10R10A2:
        ppu_int_cfg->rgb_format = PP_OUT_B10G10R10A2;
        break;
      case DEC_OUT_FRM_R10G10B10A2:
        ppu_int_cfg->rgb_format = PP_OUT_R10G10B10A2;
        break;
      case DEC_OUT_FRM_RGB565:
        ppu_int_cfg->rgb_format = PP_OUT_RGB565;
        break;
      case DEC_OUT_FRM_BGR565:
        ppu_int_cfg->rgb_format = PP_OUT_BGR565;
        break;
      case DEC_OUT_FRM_ARGB8565:
        ppu_int_cfg->rgb_format = PP_OUT_ARGB8565;
        break;
      default:
        ASSERT(0);
        break;
      }
      ppu_int_cfg->rgb_stan = ppu_ext_cfg->rgb_stan;
      ppu_int_cfg->rgb_alpha = ppu_ext_cfg->rgb_alpha;
#if 0
      if (ppu_ext_cfg->video_range) {
        if (pixel_width == 8) {
          ppu_int_cfg->range_max = 255;
          ppu_int_cfg->range_min = 0;
        } else {
          ppu_int_cfg->range_max = 1023;
          ppu_int_cfg->range_min = 0;
        }
      } else {
        if (pixel_width == 8) {
          ppu_int_cfg->range_max = 235;
          ppu_int_cfg->range_min = 16;
        } else {
          ppu_int_cfg->range_max = 940;
          ppu_int_cfg->range_min = 64;
        }
      }
#endif
      if (pixel_width == 8) {
        ppu_int_cfg->range_max = 255;
        ppu_int_cfg->range_min = 0;
      } else {
        ppu_int_cfg->range_max = 1023;
        ppu_int_cfg->range_min = 0;
      }
    }
    ppu_int_cfg->dither_enable = ppu_ext_cfg->dither_enable;

    /* for range mapping */
    ppu_int_cfg->set_target_range_enable = ppu_ext_cfg->set_target_range_enable;
    ppu_int_cfg->target_range = ppu_ext_cfg->target_range;
    ppu_int_cfg->range_map_flag = 0;
    /* range_map_flag: need 2 bits to decide whether to do range mapping or not*/
    if (ppu_ext_cfg->set_target_range_enable) {
      if (ppu_int_cfg->source_range == ppu_ext_cfg->target_range) {
        ppu_int_cfg->range_map_flag = 0; // bypass
      } else if ((ppu_int_cfg->source_range != ppu_ext_cfg->target_range) && ppu_ext_cfg->target_range == 1) {
        ppu_int_cfg->range_map_flag = 1; // to full
      } else if ((ppu_int_cfg->source_range != ppu_ext_cfg->target_range) && ppu_ext_cfg->target_range == 0) {
        ppu_int_cfg->range_map_flag = 2; // to limited
      }
      // set the min/max for final clip
      if (ppu_int_cfg->range_map_flag) {
        if (ppu_int_cfg->range_map_flag == 1) { // to full
          if (pixel_width == 8) { // 8 bit
            ppu_int_cfg->range_map_max = 255;
            ppu_int_cfg->range_map_min = 0;
          } else { // 10 bit
            ppu_int_cfg->range_map_max = 1023;
            ppu_int_cfg->range_map_min = 0;
          }
        } else { // ppu_int_cfg->range_map_flag == 2 to limited
          if (pixel_width == 8) { // 8 bit
            ppu_int_cfg->range_map_max = 235; // for luma
            ppu_int_cfg->range_map_min = 16;
          } else { // 10 bit
            ppu_int_cfg->range_map_max = 940; // for luma
            ppu_int_cfg->range_map_min = 64;
          }
        }
      }
    }

    /* for color remapping (3dlut) */
    ppu_int_cfg->enable_3dlut = ppu_ext_cfg->enable_3dlut;
    /* 3dlut: addr adustment for hw*/
    if (ppu_int_cfg->enable_3dlut) {
      u32 *temp_addr;
      temp_addr = (u32*)DWLmalloc(18 * 18 * 18 * sizeof(u32));
      if (temp_addr == NULL)
        return;
      DWLmemset(temp_addr, 0, 18 * 18 * 18 * sizeof(u32));
      // addr adustment
      if (!flag_3dlut) {
        int j = 0;
        if (pixel_width == 8) {
          for (int i = 0; i < 18 * 18 * 18 * 3; i++) { // i < number of 16bit
            DWLmemcpy(((u8*)temp_addr) + j, ((u16*)ppu_int_cfg->table_3dlut_buffer.virtual_address) + i, sizeof(u16));
            if ((j + 2) % 4 == 0) j++; // B8G8R808
            j++;
          }
          DWLmemcpy((u8 *)ppu_int_cfg->table_3dlut_buffer.virtual_address, temp_addr, 18 * 18 * 18 * sizeof(u32));
        } else {
          int k = 0;
          u32 threed_lut_table_value;
          for (int i = 0; i < 18 * 18 * 18 * 3; i+=3) {
            threed_lut_table_value = (*((u16*)ppu_int_cfg->table_3dlut_buffer.virtual_address + i + 2) & 0x3FF) << 20
                                    | (*((u16*)ppu_int_cfg->table_3dlut_buffer.virtual_address + i + 1) & 0x3FF) << 10
                                    | (*((u16*)ppu_int_cfg->table_3dlut_buffer.virtual_address + i) & 0x3FF); // B is lsb, 0+R is msb
            *((u32*)ppu_int_cfg->table_3dlut_buffer.virtual_address + k) = threed_lut_table_value;
            k++;
          }
        }
      }
      DWLfree(temp_addr);
      flag_3dlut = 1;
    }

    ppu_int_cfg->antialias = ppu_ext_cfg->antialias;
    switch (ppu_ext_cfg->pp_filter) {
    case VSI_LINEAR:
      ppu_int_cfg->pp_filter = PP_VSI_LINEAR;
      break;
    case LANCZOS:
      ppu_int_cfg->pp_filter = PP_LANCZOS;
      ppu_int_cfg->x_filter_param = ppu_ext_cfg->x_filter_param;
      ppu_int_cfg->y_filter_param = ppu_ext_cfg->y_filter_param;
      break;
    case NEAREST:
      ppu_int_cfg->pp_filter = PP_NEAREST;
      break;
    case BI_LINEAR:
      ppu_int_cfg->pp_filter = PP_BILINEAR;
      ppu_int_cfg->x_filter_param = 1;
      ppu_int_cfg->y_filter_param = 1;
      break;
    case BICUBIC:
      ppu_int_cfg->pp_filter = PP_BICUBIC;
      ppu_int_cfg->x_filter_param = 2;
      ppu_int_cfg->y_filter_param = 2;
      break;
    case SPLINE:
      ppu_int_cfg->pp_filter = PP_SPLINE;
      ppu_int_cfg->x_filter_param = 2;
      ppu_int_cfg->y_filter_param = 2;
      break;
    case BOX:
      ppu_int_cfg->pp_filter = PP_BOX;
      ppu_int_cfg->x_filter_param = 1;
      ppu_int_cfg->y_filter_param = 1;
      break;
    case FAST_LINEAR:
      ppu_int_cfg->pp_filter = FAST_LINEAR;
      break;
    case FAST_BICUBIC:
      ppu_int_cfg->pp_filter = FAST_BICUBIC;
      break;
    case AREA:
      ppu_int_cfg->pp_filter = AREA;
      break;
    default:
      ASSERT(0);
      break;
    }
    if (chroma_format == PP_CHROMA_420 || chroma_format == PP_CHROMA_400) {
      ppu_int_cfg->sub_x = ppu_int_cfg->sub_y = 2;
    } else if (chroma_format == PP_CHROMA_422) {
      ppu_int_cfg->sub_x = 2;
      ppu_int_cfg->sub_y = 1;
    } else {
      ppu_int_cfg->sub_x = ppu_int_cfg->sub_y = 1;
    }
#if 0
    if (ppu_ext_cfg->tiled_e && pixel_width > 8) {
      if (!ppu_int_cfg->out_p010 &&
          !ppu_int_cfg->out_I010 &&
          !ppu_int_cfg->out_L010) {
        ppu_int_cfg->out_p010 = 1;  /* P010 by default for tile output of 10-bit cases */
      }
      ppu_int_cfg->out_cut_8bits = 0;
    }
#endif
    if ((pixel_width == 8 && !ppu_int_cfg->Explicit_Specified_010) || ppu_int_cfg->rgb || ppu_int_cfg->rgb_planar) {
      ppu_int_cfg->out_cut_8bits = 0;
      ppu_int_cfg->out_p010 = 0;
      ppu_int_cfg->out_1010 = 0;
      ppu_int_cfg->out_I010 = 0;
      ppu_int_cfg->out_L010 = 0;
      ppu_int_cfg->out_p012 = 0;
      ppu_int_cfg->out_I012 = 0;
    }
    if (ppu_int_cfg->out_cut_8bits) {
      ppu_int_cfg->out_p010 = 0;
      ppu_int_cfg->out_1010 = 0;
      ppu_int_cfg->out_I010 = 0;
      ppu_int_cfg->out_L010 = 0;
      ppu_int_cfg->out_p012 = 0;
      ppu_int_cfg->out_I012 = 0;
    }
    if (!IS_PLANAR_RGB(ppu_int_cfg->rgb_format)) {
      ppu_int_cfg->rgb_planar = 0;
    }
    if ((ppu_int_cfg->sub_x == 1) && (ppu_int_cfg->sub_y == 1) && !ppu_int_cfg->planar) {
      ppu_int_cfg->planar = 1;
    }
    if (ppu_int_cfg->rgb || ppu_int_cfg->rgb_planar) {
      ppu_int_cfg->out_format = PP_OUT_FMT_RGB;
    } else if (ppu_int_cfg->out_yuyv) {
      if (ppu_int_cfg->out_cut_8bits || pixel_width == 8)
        ppu_int_cfg->out_format = PP_OUT_FMT_YUV422_YUYV_8BIT;
      else if (ppu_int_cfg->out_p010)
        ppu_int_cfg->out_format = PP_OUT_FMT_YUV422_YUYV_P010;
    } else if (ppu_int_cfg->out_uyvy) {
      if (ppu_int_cfg->out_cut_8bits || pixel_width == 8)
        ppu_int_cfg->out_format = PP_OUT_FMT_YUV422_UYVY_8BIT;
      else
        ppu_int_cfg->out_format = PP_OUT_FMT_YUV422_UYVY_P010;
    } else if (ppu_int_cfg->monochrome || mono_chrome) {
      if (ppu_int_cfg->out_p010 || ppu_int_cfg->out_I010 || ppu_int_cfg->out_L010)
        ppu_int_cfg->out_format = PP_OUT_FMT_YUV400_P010;
      else if (ppu_int_cfg->out_p012 || ppu_int_cfg->out_I012)
        ppu_int_cfg->out_format = PP_OUT_FMT_YUV400_P012;
      else if (ppu_int_cfg->out_cut_8bits)
        ppu_int_cfg->out_format = PP_OUT_FMT_YUV400_8BIT;
      else
        ppu_int_cfg->out_format = ppu_ext_cfg->yuv400_with_alpha ? PP_OUT_FMT_YUV400_WITH_ALPHA : PP_OUT_FMT_YUV400;
      ppu_int_cfg->monochrome = 1;
    } else if (ppu_int_cfg->planar) {
      if (ppu_int_cfg->out_p010 || ppu_int_cfg->out_I010 || ppu_int_cfg->out_L010) {
        if (chroma_format == PP_CHROMA_420)
          ppu_int_cfg->out_format = PP_OUT_FMT_IYUV_420_P010;
        else if (chroma_format == PP_CHROMA_422)
          ppu_int_cfg->out_format = PP_OUT_FMT_IYUV_422_P010;
        else
          ppu_int_cfg->out_format = PP_OUT_FMT_IYUV_444_P010;
      } else if (ppu_int_cfg->out_p012 || ppu_int_cfg->out_I012) {
        if (chroma_format == PP_CHROMA_420)
          ppu_int_cfg->out_format = PP_OUT_FMT_IYUV_420_P012;
        else if (chroma_format == PP_CHROMA_422)
          ppu_int_cfg->out_format = PP_OUT_FMT_IYUV_422_P012;
        else
          ppu_int_cfg->out_format = PP_OUT_FMT_IYUV_444_P012;
      } else if (ppu_int_cfg->out_cut_8bits || pixel_width == 8) {
        if (chroma_format == PP_CHROMA_420)
          ppu_int_cfg->out_format = PP_OUT_FMT_IYUV_420_8BIT;
        else if (chroma_format == PP_CHROMA_422)
          ppu_int_cfg->out_format = PP_OUT_FMT_IYUV_422_8BIT;
        else
          ppu_int_cfg->out_format = PP_OUT_FMT_IYUV_444_8BIT;
      } else {
        if (chroma_format == PP_CHROMA_420)
          ppu_int_cfg->out_format = PP_OUT_FMT_IYUVPACKED10_420; /*422 and 444 not defined packed mode*/
      }
    } else if (ppu_int_cfg->out_1010) {
      if (chroma_format == PP_CHROMA_420)
        ppu_int_cfg->out_format = PP_OUT_FMT_YUV420_10;
      else if (chroma_format == PP_CHROMA_422)
        ppu_int_cfg->out_format = PP_OUT_FMT_YUV422_10;
    } else {
      //444 only support planar
      if (ppu_int_cfg->out_p010 || ppu_int_cfg->out_I010 || ppu_int_cfg->out_L010) {
        if (chroma_format == PP_CHROMA_420) {
          // if(ppu_int_cfg->pp_pvfbc)
          //   ppu_int_cfg->out_format = PP_OUT_FMT_PVFBC_YUV420_P010;
          // else
            ppu_int_cfg->out_format = PP_OUT_FMT_YUV420_P010;
        } else if (chroma_format == PP_CHROMA_422)
          ppu_int_cfg->out_format = PP_OUT_FMT_YUV422_P010;
      } else if (ppu_int_cfg->out_cut_8bits || pixel_width == 8) {
        if (chroma_format == PP_CHROMA_420) {
          // if(ppu_int_cfg->pp_pvfbc)
          //   ppu_int_cfg->out_format = PP_OUT_FMT_PVFBC_YUV420_8BIT;
          // else
            ppu_int_cfg->out_format = PP_OUT_FMT_YUV420_8BIT;
        }else if (chroma_format == PP_CHROMA_422)
          ppu_int_cfg->out_format = PP_OUT_FMT_YUV422_8BIT;
      } else if (ppu_int_cfg->out_p012 || ppu_int_cfg->out_I012) {
        if (chroma_format == PP_CHROMA_420)
          ppu_int_cfg->out_format = PP_OUT_FMT_YUV420_P012;
        else if (chroma_format == PP_CHROMA_422)
          ppu_int_cfg->out_format = PP_OUT_FMT_YUV422_P012;
      } else {
        if (chroma_format == PP_CHROMA_420)
          ppu_int_cfg->out_format = PP_OUT_FMT_YUV420PACKED10; /*422 and 444 not defined packed mode*/
      }
    }
    #if 0
    if (!frame_only)
      ppu_int_cfg->tiled_e = 0;
    #endif
    ppu_int_cfg->pixel_width = (ppu_int_cfg->out_cut_8bits || ((ppu_int_cfg->rgb || ppu_int_cfg->rgb_planar) &&
                                (IS_4PLAN_FORMAT(ppu_int_cfg->rgb_format) || IS_8BITS_FORMAT(ppu_int_cfg->rgb_format) || IS_5_6_5_FORMAT(ppu_int_cfg->rgb_format) )) ||
                                (pixel_width == 8 && (!ppu_int_cfg->out_p010 && !ppu_int_cfg->out_I010 && !ppu_int_cfg->out_L010))) ? 8 :
                               ((ppu_int_cfg->out_p010 || ppu_int_cfg->out_I010 || ppu_int_cfg->out_L010 ||
                                 ppu_int_cfg->out_p012 || ppu_int_cfg->out_I012 ||
                                ((ppu_int_cfg->rgb || ppu_int_cfg->rgb_planar) && IS_16BITS_FORMAT(ppu_int_cfg->rgb_format))) ? 16 : pixel_width);
    ppu_int_cfg->stream_pixel_width = pixel_width;
    ppu_ext_cfg++;
    ppu_int_cfg++;
  }
}

enum DecPictureFormat TransUnitConfig2Format(PpUnitIntConfig *ppu_int_cfg) {
  enum DecPictureFormat output_format = DEC_OUT_FRM_TILED_4X4;

  if (ppu_int_cfg->hist_global_e || ppu_int_cfg->hist_local_e || ppu_int_cfg->hist_hsv_e) {
    output_format = DEC_OUT_FRM_HIST;
  } else if (ppu_int_cfg->dec400_enabled) {
    /* DEC400 */
    if (ppu_int_cfg->rgb) {
      switch (ppu_int_cfg->rgb_format) {
      case PP_OUT_RGB888:
        output_format = DEC_OUT_FRM_DEC400_RGB888;
        break;
      case PP_OUT_BGR888:
        output_format = DEC_OUT_FRM_DEC400_BGR888;
        break;
      case PP_OUT_ABGR888:
        if (ppu_int_cfg->tiled_e)
          output_format = DEC_OUT_FRM_DEC400_ABGR888_TILED64X64;
        else
          output_format = DEC_OUT_FRM_DEC400_ABGR888;
        break;
      case PP_OUT_ARGB888:
        if (ppu_int_cfg->tiled_e)
          output_format = DEC_OUT_FRM_DEC400_ARGB888_TILED64X64;
        else
          output_format = DEC_OUT_FRM_DEC400_ARGB888;
        break;
      case PP_OUT_BGRA888:
        if (ppu_int_cfg->tiled_e)
          output_format = DEC_OUT_FRM_DEC400_BGRA888_TILED64X64;
        else
          output_format = DEC_OUT_FRM_DEC400_BGRA888;
        break;
      case PP_OUT_RGBA888:
        if (ppu_int_cfg->tiled_e)
          output_format = DEC_OUT_FRM_DEC400_RGBA888_TILED64X64;
        else
          output_format = DEC_OUT_FRM_DEC400_RGBA888;
        break;
      case PP_OUT_A2B10G10R10:
        if (ppu_int_cfg->tiled_e)
          output_format = DEC_OUT_FRM_DEC400_A2B10G10R10_TILED64X64;
        else
          output_format = DEC_OUT_FRM_DEC400_A2B10G10R10;
        break;
      case PP_OUT_A2R10G10B10:
        if (ppu_int_cfg->tiled_e)
          output_format = DEC_OUT_FRM_DEC400_A2R10G10B10_TILED64X64;
        else
          output_format = DEC_OUT_FRM_DEC400_A2R10G10B10;
        break;
      case PP_OUT_B10G10R10A2:
        if (ppu_int_cfg->tiled_e)
          output_format = DEC_OUT_FRM_DEC400_B10G10R10A2_TILED64X64;
        else
          output_format = DEC_OUT_FRM_DEC400_B10G10R10A2;
        break;
      case PP_OUT_R10G10B10A2:
        if (ppu_int_cfg->tiled_e)
          output_format = DEC_OUT_FRM_DEC400_R10G10B10A2_TILED64X64;
        else
          output_format = DEC_OUT_FRM_DEC400_R10G10B10A2;
        break;
      case PP_OUT_X2R10G10B10:
        if (ppu_int_cfg->tiled_e)
          output_format = DEC_OUT_FRM_DEC400_X2R10G10B10_TILED64X64;
        else
          output_format = DEC_OUT_FRM_DEC400_X2R10G10B10;
        break;
      case PP_OUT_X2B10G10R10:
        if (ppu_int_cfg->tiled_e)
          output_format = DEC_OUT_FRM_DEC400_X2B10G10R10_TILED64X64;
        else
          output_format = DEC_OUT_FRM_DEC400_X2B10G10R10;
        break;
      case PP_OUT_XBGR888:
        if (ppu_int_cfg->tiled_e)
          output_format = DEC_OUT_FRM_DEC400_XBGR888_TILED64X64;
        else
          output_format = DEC_OUT_FRM_DEC400_XBGR888;
        break;
      case PP_OUT_XRGB888:
        if (ppu_int_cfg->tiled_e)
          output_format = DEC_OUT_FRM_DEC400_XRGB888_TILED64X64;
        else
          output_format = DEC_OUT_FRM_DEC400_XRGB888;
        break;
      default:
        ASSERT(0);
        break;
      }
    } else if (ppu_int_cfg->monochrome) {
      if (ppu_int_cfg->tiled_e && !ppu_int_cfg->tile_mode)
        output_format = DEC_OUT_FRM_DEC400_400TILE;
      else if (ppu_int_cfg->tiled_e && ppu_int_cfg->tile_mode == TILED8x8) {
        if (ppu_int_cfg->pixel_width != 8)
          output_format = DEC_OUT_FRM_DEC400_400TILED8x8_P010;
        else
          output_format = DEC_OUT_FRM_DEC400_400TILED8x8;
      } else {
        if (ppu_int_cfg->pixel_width != 8)
          output_format = DEC_OUT_FRM_DEC400_400_P010;
        else
          output_format = DEC_OUT_FRM_DEC400_400;
      }
    } else if(ppu_int_cfg->chroma_format < PP_CHROMA_422){
      if (ppu_int_cfg->tiled_e && !ppu_int_cfg->tile_mode ) {
        output_format = DEC_OUT_FRM_DEC400_420TILE;
      } else if (ppu_int_cfg->tiled_e && ppu_int_cfg->tile_mode == TILED8x8) {
        if (ppu_int_cfg->pixel_width != 8)
          output_format = ppu_int_cfg->cr_first ? DEC_OUT_FRM_DEC400_NV21TILED8x8_P010 : DEC_OUT_FRM_DEC400_420TILED8x8_P010;
        else
          output_format = ppu_int_cfg->cr_first ? DEC_OUT_FRM_DEC400_NV21TILED8x8 : DEC_OUT_FRM_DEC400_420TILED8x8;
      } else if (ppu_int_cfg->planar) {
        if (ppu_int_cfg->pixel_width != 8)
          output_format = DEC_OUT_FRM_DEC400_420P_P010;
        else
          output_format = DEC_OUT_FRM_DEC400_420P;
      } else {
        if (ppu_int_cfg->pixel_width != 8)
          output_format = ppu_int_cfg->cr_first ? DEC_OUT_FRM_DEC400_NV21SP_P010 : DEC_OUT_FRM_DEC400_420SP_P010;
        else
          output_format = ppu_int_cfg->cr_first ? DEC_OUT_FRM_DEC400_NV21SP : DEC_OUT_FRM_DEC400_420SP;
      }
    } else if(ppu_int_cfg->out_yuyv) {
      output_format = DEC_OUT_FRM_DEC400_YUV422_YUYV;
    } else if(ppu_int_cfg->out_uyvy) {
      output_format = DEC_OUT_FRM_DEC400_YUV422_UYVY;
    }
  } else if (ppu_int_cfg->pp_pvfbc) {/*only support 420SP/420SP_P010*/
    if(ppu_int_cfg->pp_pvfbc_bypass){
      if (ppu_int_cfg->pixel_width == 8) {
        if (ppu_int_cfg->chroma_format == PP_CHROMA_420) {
          if(ppu_int_cfg->pp_pvfbc_tile_type == PVFBC_TILED16x4)
            output_format = DEC_OUT_FRM_PVFBC_BYPASS_TILED16X4_420SP;
          else if(ppu_int_cfg->pp_pvfbc_tile_type == PVFBC_TILED8x8)
            output_format = DEC_OUT_FRM_PVFBC_BYPASS_TILED8X8_420SP;
        }
      } else {
        if (ppu_int_cfg->chroma_format == PP_CHROMA_420) {
          if(ppu_int_cfg->pp_pvfbc_tile_type == PVFBC_TILED16x4)
            output_format = DEC_OUT_FRM_PVFBC_BYPASS_TILED16X4_420SP_P010;
          else if(ppu_int_cfg->pp_pvfbc_tile_type == PVFBC_TILED8x8)
            output_format = DEC_OUT_FRM_PVFBC_BYPASS_TILED8X8_420SP_P010;
        }
      }
    }else{
      if (ppu_int_cfg->pixel_width == 8) {
        if (ppu_int_cfg->chroma_format == PP_CHROMA_420) {
          if(ppu_int_cfg->pp_pvfbc_tile_type == PVFBC_TILED16x4)
            output_format = DEC_OUT_FRM_PVFBC_TILED16X4_420SP;
          else if(ppu_int_cfg->pp_pvfbc_tile_type == PVFBC_TILED8x8)
            output_format = DEC_OUT_FRM_PVFBC_TILED8X8_420SP;
        }
      } else {
        if (ppu_int_cfg->chroma_format == PP_CHROMA_420) {
          if(ppu_int_cfg->pp_pvfbc_tile_type == PVFBC_TILED16x4)
            output_format = DEC_OUT_FRM_PVFBC_TILED16X4_420SP_P010;
          else if(ppu_int_cfg->pp_pvfbc_tile_type == PVFBC_TILED8x8)
            output_format = DEC_OUT_FRM_PVFBC_TILED8X8_420SP_P010;
        }
      }
    }
  } else if (ppu_int_cfg->tiled_e) {
    /* FBC style */
    if (ppu_int_cfg->tile_mode == TILED4x4) {
      if (ppu_int_cfg->rgb_format == PP_OUT_BGR888) {
        if(ppu_int_cfg->decnano_e)
          output_format = DEC_OUT_FRM_DECNANO_MODE0_RGB888_TILED4X4;
        else
          output_format = DEC_OUT_FRM_RGB888_TILED4X4;
      } else if (ppu_int_cfg->rgb_format == PP_OUT_ARGB888) {
        if(ppu_int_cfg->decnano_e)
          output_format = DEC_OUT_FRM_DECNANO_MODE0_ARGB888_TILED4X4;
        else
          output_format = DEC_OUT_FRM_ARGB888_TILED4X4;
      } else if (ppu_int_cfg->rgb_format == PP_OUT_RGB565) {
        if(ppu_int_cfg->decnano_e)
          output_format = DEC_OUT_FRM_DECNANO_MODE0_RGB565_TILED4X4;
        else
          output_format = DEC_OUT_FRM_RGB565_TILED4X4;
      } else if (ppu_int_cfg->rgb_format == PP_OUT_ARGB8565) {
        if(ppu_int_cfg->decnano_e)
          output_format = DEC_OUT_FRM_DECNANO_MODE0_ARGB8565_TILED4X4;
        else
          output_format = DEC_OUT_FRM_ARGB565_TILED4X4;
      }
    } else if (ppu_int_cfg->tile_mode == TILED8x8) {
      if (!ppu_int_cfg->monochrome) {
        if (ppu_int_cfg->pixel_width != 8)
          output_format = DEC_OUT_FRM_TILED_8X8_P010;
        else
          output_format = DEC_OUT_FRM_TILED_8X8;
      } else {
        if (ppu_int_cfg->pixel_width != 8)
          output_format = DEC_OUT_FRM_YUV400TILE8x8_P010;
        else
          output_format = DEC_OUT_FRM_YUV400TILE8x8;
      }
    } else if (ppu_int_cfg->tile_mode == RECONTILED8x8) {
      if (!ppu_int_cfg->monochrome) {
        if (ppu_int_cfg->pixel_width != 8)
          output_format = DEC_OUT_FRM_YUV420RECONTILE8X8_PACK10;
        else
          output_format = DEC_OUT_FRM_YUV420RECONTILE8X8;
      }
    } else if (ppu_int_cfg->tile_mode == TILED16x16 && !ppu_int_cfg->pp_comp) {
      if (ppu_int_cfg->pixel_width != 8)
        output_format = DEC_OUT_FRM_TILED_16X16_P010;
      else
        output_format = DEC_OUT_FRM_TILED_16X16;
    } else if (ppu_int_cfg->tile_mode == TILED16x16 && ppu_int_cfg->pp_comp) {
      if (ppu_int_cfg->pixel_width != 8)
        output_format = DEC_OUT_FRM_TILED_16X16_P010_COMP;
      else
        output_format = DEC_OUT_FRM_TILED_16X16_COMP;
    } else if (ppu_int_cfg->tile_mode == TILED32x8 && ppu_int_cfg->pp_comp) {
      if (ppu_int_cfg->pixel_width != 8)
        output_format = DEC_OUT_FRM_TILED_32X8_P010_COMP;
      else
        output_format = DEC_OUT_FRM_TILED_32X8_COMP;
    } else if (ppu_int_cfg->tile_mode == TILED128x2) {
      if (!ppu_int_cfg->tile_coded_image) {
        if (ppu_int_cfg->pixel_width != 8) {
          if (ppu_int_cfg->monochrome)
            output_format = DEC_OUT_FRM_YUV400TILE_128X2_P010;
          else
            output_format = DEC_OUT_FRM_YUV420TILE_128X2_P010;
        } else {
          if (ppu_int_cfg->monochrome)
            output_format = DEC_OUT_FRM_YUV400TILE_128X2;
          else
            output_format = DEC_OUT_FRM_YUV420TILE_128X2;
        }
      } else {
        if (ppu_int_cfg->pixel_width != 8) {
          if (ppu_int_cfg->monochrome)
            output_format = DEC_OUT_FRM_YUV400_P010;
          else
            output_format = DEC_OUT_FRM_YUV420SP_P010;
        } else {
          if (ppu_int_cfg->monochrome)
            output_format = DEC_OUT_FRM_YUV400;
          else
            output_format = DEC_OUT_FRM_YUV420SP;
        }
      }
    } else if (ppu_int_cfg->tile_mode == TILED64x64) {
      if (!ppu_int_cfg->rgb) {
        if (ppu_int_cfg->pixel_width == 8)
          output_format = DEC_OUT_FRM_TILED_64X64;
        else
          output_format = DEC_OUT_FRM_TILED_64X64_PACK10;
      } else {
        switch (ppu_int_cfg->rgb_format) {
        case PP_OUT_ABGR888:
          output_format = DEC_OUT_FRM_ABGR888_TILED64X64;
          break;
        case PP_OUT_ARGB888:
          output_format = DEC_OUT_FRM_ARGB888_TILED64X64;
          break;
        case PP_OUT_A2B10G10R10:
          output_format = DEC_OUT_FRM_A2B10G10R10_TILED64X64;
          break;
        case PP_OUT_A2R10G10B10:
          output_format = DEC_OUT_FRM_A2R10G10B10_TILED64X64;
          break;
        case PP_OUT_X2B10G10R10:
          output_format = DEC_OUT_FRM_X2B10G10R10_TILED64X64;
          break;
        case PP_OUT_X2R10G10B10:
          output_format = DEC_OUT_FRM_X2R10G10B10_TILED64X64;
          break;
        case PP_OUT_XBGR888:
          output_format = DEC_OUT_FRM_XBGR888_TILED64X64;
          break;
        case PP_OUT_XRGB888:
          output_format = DEC_OUT_FRM_XRGB888_TILED64X64;
          break;
        case PP_OUT_BGRA888:
          output_format = DEC_OUT_FRM_BGRA888_TILED64X64;
          break;
        case PP_OUT_RGBA888:
          output_format = DEC_OUT_FRM_RGBA888_TILED64X64;
          break;
        case PP_OUT_B10G10R10A2:
          output_format = DEC_OUT_FRM_B10G10R10A2_TILED64X64;
          break;
        case PP_OUT_R10G10B10A2:
          output_format = DEC_OUT_FRM_R10G10B10A2_TILED64X64;
          break;
        default:
          ASSERT(0);
          break;
        }
      }
    } else if(ppu_int_cfg->tile_mode == TILED2x4) {
      if (ppu_int_cfg->out_yuyv) {
        if(ppu_int_cfg->decnano_e)
          output_format = DEC_OUT_FRM_DECNANO_MODE0_YUV422_YUYV_TILED2X4;
        else
          output_format = DEC_OUT_FRM_YUV422_YUYV_TILED2X4;
      } else if(ppu_int_cfg->out_uyvy) {
        if(ppu_int_cfg->decnano_e)
          output_format = DEC_OUT_FRM_DECNANO_MODE0_YUV422_UYVY_TILED2X4;
        else
          output_format = DEC_OUT_FRM_YUV422_UYVY_TILED2X4;
      }
    } else if(ppu_int_cfg->tile_mode == PACKED422_TILED8x8) {
      if (ppu_int_cfg->out_yuyv) {
        output_format = DEC_OUT_FRM_YUV422_YUYV_TILED8X8;
      } else if(ppu_int_cfg->out_uyvy) {
        output_format = DEC_OUT_FRM_YUV422_UYVY_TILED8X8;
      }
    }
  } else if (ppu_int_cfg->rgb || ppu_int_cfg->rgb_planar) {
    /* RGB */
    switch (ppu_int_cfg->rgb_format) {
    case PP_OUT_RGB888: /* sw_ppx_out_rgb_fmt == 0 */
        output_format = DEC_OUT_FRM_BGR888;
      break;
    case PP_OUT_BGR888: /* sw_ppx_out_rgb_fmt == 1 */
        output_format = DEC_OUT_FRM_RGB888;
      break;
    case PP_OUT_R16G16B16: /* sw_ppx_out_rgb_fmt == 2 */
        output_format = DEC_OUT_FRM_B16G16R16;
      break;
    case PP_OUT_B16G16R16: /* sw_ppx_out_rgb_fmt == 3 */
        output_format = DEC_OUT_FRM_R16G16B16;
      break;
    case PP_OUT_ABGR888: /* sw_ppx_out_rgb_fmt == 4 */
        output_format = DEC_OUT_FRM_ABGR888;
      break;
    case PP_OUT_ARGB888: /* sw_ppx_out_rgb_fmt == 5 */
        output_format = DEC_OUT_FRM_ARGB888;
      break;
    case PP_OUT_A2B10G10R10: /* sw_ppx_out_rgb_fmt == 6 */
        output_format = DEC_OUT_FRM_A2B10G10R10;
      break;
    case PP_OUT_A2R10G10B10: /* sw_ppx_out_rgb_fmt == 7 */
        output_format = DEC_OUT_FRM_A2R10G10B10;
      break;
    case PP_OUT_XBGR888: /* sw_ppx_out_rgb_fmt == 8 */
        output_format = DEC_OUT_FRM_XBGR888;
      break;
    case PP_OUT_XRGB888: /* sw_ppx_out_rgb_fmt == 9 */
        output_format = DEC_OUT_FRM_XRGB888;
      break;
    case PP_OUT_RGB888_P: /* sw_ppx_out_rgb_fmt == 10 */
        output_format = DEC_OUT_FRM_RGB888_P;
      break;
    case PP_OUT_BGR888_P: /* sw_ppx_out_rgb_fmt == 11 */
        output_format = DEC_OUT_FRM_BGR888_P;
      break;
    case PP_OUT_R16G16B16_P: /* sw_ppx_out_rgb_fmt == 12 */
        output_format = DEC_OUT_FRM_R16G16B16_P;
      break;
    case PP_OUT_B16G16R16_P: /* sw_ppx_out_rgb_fmt == 13 */
        output_format = DEC_OUT_FRM_B16G16R16_P;
      break;
    case PP_OUT_BGRA888: /* sw_ppx_out_rgb_fmt == 14 */
        output_format = DEC_OUT_FRM_BGRA888;
      break;
    case PP_OUT_RGBA888: /* sw_ppx_out_rgb_fmt == 15 */
        output_format = DEC_OUT_FRM_RGBA888;
      break;
    case PP_OUT_B10G10R10A2: /* sw_ppx_out_rgb_fmt == 16 */
        output_format = DEC_OUT_FRM_B10G10R10A2;
      break;
    case PP_OUT_R10G10B10A2: /* sw_ppx_out_rgb_fmt == 17 */
        output_format = DEC_OUT_FRM_R10G10B10A2;
      break;
    case PP_OUT_X2B10G10R10: /* sw_ppx_out_rgb_fmt == 18 */
        output_format = DEC_OUT_FRM_X2B10G10R10;
      break;
    case PP_OUT_X2R10G10B10: /* sw_ppx_out_rgb_fmt == 19 */
        output_format = DEC_OUT_FRM_X2R10G10B10;
      break;
    case PP_OUT_RGB565:
        output_format = DEC_OUT_FRM_RGB565;
      break;
    case PP_OUT_BGR565:
        output_format = DEC_OUT_FRM_BGR565;
      break;
    case PP_OUT_ARGB8565:
        output_format = DEC_OUT_FRM_ARGB8565;
      break;
    default:
      ASSERT(0);
      break;
    }
  } else if (ppu_int_cfg->out_yuyv) {
    output_format = DEC_OUT_FRM_YUV422_YUYV;
  } else if (ppu_int_cfg->out_uyvy) {
    output_format = DEC_OUT_FRM_YUV422_UYVY;
  } else { /* YUV */
    if (ppu_int_cfg->monochrome) { /* YUV400 */
      if (ppu_int_cfg->tiled_e) {
        output_format = DEC_OUT_FRM_YUV400TILE;
        if (ppu_int_cfg->pixel_width != 8) {
          if (ppu_int_cfg->out_cut_8bits)
            output_format = DEC_OUT_FRM_YUV400TILE;
          else if (ppu_int_cfg->out_p010)
            output_format = DEC_OUT_FRM_YUV400TILE_P010;
          else if (ppu_int_cfg->out_I010)
            output_format = DEC_OUT_FRM_YUV400TILE_I010;
          else if (ppu_int_cfg->out_L010)
            output_format = DEC_OUT_FRM_YUV400TILE_L010;
          else if (ppu_int_cfg->out_p012)
            output_format = DEC_OUT_FRM_YUV400TILE_P012;
          else if (ppu_int_cfg->out_I012)
            output_format = DEC_OUT_FRM_YUV400TILE_I012;
          else
            output_format = DEC_OUT_FRM_YUV400TILE_PACK10;
        }
      } else {
        output_format = ppu_int_cfg->yuv400_with_alpha ? DEC_OUT_FRM_YUV400_WITH_ALPHA : DEC_OUT_FRM_YUV400;
        if (ppu_int_cfg->pixel_width != 8) {
          if (ppu_int_cfg->out_cut_8bits) {
            output_format = DEC_OUT_FRM_YUV400;
          } else if(ppu_int_cfg->out_p010) {
            output_format = DEC_OUT_FRM_YUV400_P010;
          } else if (ppu_int_cfg->out_I010) {
            output_format = DEC_OUT_FRM_YUV400_I010;
          } else if (ppu_int_cfg->out_L010) {
            output_format = DEC_OUT_FRM_YUV400_L010;
          } else if(ppu_int_cfg->out_p012) {
            output_format = DEC_OUT_FRM_YUV400_P012;
          } else if (ppu_int_cfg->out_I012) {
            output_format = DEC_OUT_FRM_YUV400_I012;
          } else {
            if (ppu_int_cfg->pixel_width == 10)
              output_format = DEC_OUT_FRM_YUV400_PACK10;
            else
              output_format = DEC_OUT_FRM_YUV400_PACK12;
          }
        }
      }
    } else {
      /* YUV420 */
      u32 cr_first = ppu_int_cfg->cr_first;
      if (ppu_int_cfg->tiled_e) {
        output_format = cr_first ? DEC_OUT_FRM_NV21TILE : DEC_OUT_FRM_YUV420TILE;
        if (ppu_int_cfg->pixel_width != 8) {
          if (ppu_int_cfg->out_cut_8bits)
            output_format = cr_first ? DEC_OUT_FRM_NV21TILE : DEC_OUT_FRM_YUV420TILE;
          else if (ppu_int_cfg->out_p010)
            output_format = cr_first ? DEC_OUT_FRM_NV21TILE_P010 : DEC_OUT_FRM_YUV420TILE_P010;
          else if (ppu_int_cfg->out_I010)
            output_format = cr_first ? DEC_OUT_FRM_NV21TILE_I010 : DEC_OUT_FRM_YUV420TILE_I010;
          else if (ppu_int_cfg->out_L010)
            output_format = cr_first ? DEC_OUT_FRM_NV21TILE_L010 : DEC_OUT_FRM_YUV420TILE_L010;
          else if (ppu_int_cfg->out_p012)
            output_format = cr_first ? DEC_OUT_FRM_NV21TILE_P012 : DEC_OUT_FRM_YUV420TILE_P012;
          else if (ppu_int_cfg->out_I012)
            output_format = cr_first ? DEC_OUT_FRM_NV21TILE_I012 : DEC_OUT_FRM_YUV420TILE_I012;
          else
            output_format = cr_first ? DEC_OUT_FRM_NV21TILE_PACK10 : DEC_OUT_FRM_YUV420TILE_PACK10;
        }
      } else if (ppu_int_cfg->planar) {
        if (ppu_int_cfg->chroma_format == PP_CHROMA_420)
          output_format = cr_first ? DEC_OUT_FRM_NV21P : DEC_OUT_FRM_YUV420P;
        else if (ppu_int_cfg->chroma_format == PP_CHROMA_422)
          output_format = cr_first ? DEC_OUT_FRM_422_NV21P : DEC_OUT_FRM_YUV422P;
        else
          output_format = cr_first ? DEC_OUT_FRM_444_NV21P : DEC_OUT_FRM_YUV444P;
        if (ppu_int_cfg->pixel_width != 8) {
          if (ppu_int_cfg->out_cut_8bits) {
            if (ppu_int_cfg->chroma_format == PP_CHROMA_420)
              output_format = cr_first ? DEC_OUT_FRM_NV21P : DEC_OUT_FRM_YUV420P;
            else if (ppu_int_cfg->chroma_format == PP_CHROMA_422)
              output_format = cr_first ? DEC_OUT_FRM_422_NV21P : DEC_OUT_FRM_YUV422P;
            else
              output_format = cr_first ? DEC_OUT_FRM_444_NV21P : DEC_OUT_FRM_YUV444P;
          } else if (ppu_int_cfg->out_p010) {
            if (ppu_int_cfg->chroma_format == PP_CHROMA_420)
              output_format = cr_first ? DEC_OUT_FRM_NV21P_P010 : DEC_OUT_FRM_YUV420P_P010;
            else if (ppu_int_cfg->chroma_format == PP_CHROMA_422)
              output_format = cr_first ? DEC_OUT_FRM_422_NV21P_P010 : DEC_OUT_FRM_YUV422P_P010;
            else
              output_format = cr_first ? DEC_OUT_FRM_444_NV21P_P010 : DEC_OUT_FRM_YUV444P_P010;
          } else if (ppu_int_cfg->out_I010) {
            if (ppu_int_cfg->chroma_format == PP_CHROMA_420)
              output_format = cr_first ? DEC_OUT_FRM_NV21P_I010 : DEC_OUT_FRM_YUV420P_I010;
            else if (ppu_int_cfg->chroma_format == PP_CHROMA_422)
              output_format = cr_first ? DEC_OUT_FRM_422_NV21P_I010 : DEC_OUT_FRM_YUV422P_I010;
            else
              output_format = cr_first ? DEC_OUT_FRM_444_NV21P_I010 : DEC_OUT_FRM_YUV444P_I010;
          } else if (ppu_int_cfg->out_L010) {
            if (ppu_int_cfg->chroma_format == PP_CHROMA_420)
              output_format = cr_first ? DEC_OUT_FRM_NV21P_L010 : DEC_OUT_FRM_YUV420P_L010;
            else if (ppu_int_cfg->chroma_format == PP_CHROMA_422)
              output_format = cr_first ? DEC_OUT_FRM_422_NV21P_L010 : DEC_OUT_FRM_YUV422P_L010;
            else
              output_format = cr_first ? DEC_OUT_FRM_444_NV21P_L010 : DEC_OUT_FRM_YUV444P_L010;
           } else if (ppu_int_cfg->out_1010) {
            if (ppu_int_cfg->chroma_format == PP_CHROMA_420)
              output_format = cr_first ? DEC_OUT_FRM_NV21P_1010 : DEC_OUT_FRM_YUV420P_1010;
            //422.444 not support planar 1010.
          } else if (ppu_int_cfg->out_p012) {
            if (ppu_int_cfg->chroma_format == PP_CHROMA_420)
              output_format = cr_first ? DEC_OUT_FRM_NV21P_P012 : DEC_OUT_FRM_YUV420P_P012;
            else if (ppu_int_cfg->chroma_format == PP_CHROMA_422)
              output_format = cr_first ? DEC_OUT_FRM_422_NV21P_P012 : DEC_OUT_FRM_YUV422P_P012;
            else
              output_format = cr_first ? DEC_OUT_FRM_444_NV21P_P012 : DEC_OUT_FRM_YUV444P_P012;
          } else if (ppu_int_cfg->out_I012) {
            if (ppu_int_cfg->chroma_format == PP_CHROMA_420)
              output_format = cr_first ? DEC_OUT_FRM_NV21P_I012 : DEC_OUT_FRM_YUV420P_I012;
            else if (ppu_int_cfg->chroma_format == PP_CHROMA_422)
              output_format = cr_first ? DEC_OUT_FRM_422_NV21P_I012 : DEC_OUT_FRM_YUV422P_I012;
            else
              output_format = cr_first ? DEC_OUT_FRM_444_NV21P_I012 : DEC_OUT_FRM_YUV444P_I012;
          } else {
            if (ppu_int_cfg->chroma_format == PP_CHROMA_420) {
              if (ppu_int_cfg->pixel_width == 10)
                output_format = cr_first ? DEC_OUT_FRM_NV21P_PACK10 : DEC_OUT_FRM_YUV420P_PACK10;
              else
                output_format = cr_first ? DEC_OUT_FRM_NV21P_PACK12 : DEC_OUT_FRM_YUV420P_PACK12;
            } else if (ppu_int_cfg->chroma_format == PP_CHROMA_422) {
              if (ppu_int_cfg->pixel_width == 10)
                output_format = cr_first ? DEC_OUT_FRM_422_NV21P_PACK10 : DEC_OUT_FRM_YUV422P_PACK10;
              else
                output_format = cr_first ? DEC_OUT_FRM_422_NV21P_PACK12 : DEC_OUT_FRM_YUV422P_PACK12;
            } else {
              if (ppu_int_cfg->pixel_width == 10)
                output_format = cr_first ? DEC_OUT_FRM_444_NV21P_PACK10 : DEC_OUT_FRM_YUV444P_PACK10;
              else
                output_format = cr_first ? DEC_OUT_FRM_444_NV21P_PACK12 : DEC_OUT_FRM_YUV444P_PACK12;
            }
          }
        }
      } else { /* semi-planar */
        if (ppu_int_cfg->chroma_format == PP_CHROMA_420)
          output_format = cr_first ? DEC_OUT_FRM_NV21SP : DEC_OUT_FRM_YUV420SP;
        else if (ppu_int_cfg->chroma_format == PP_CHROMA_422)
          output_format = cr_first ? DEC_OUT_FRM_422_NV21SP : DEC_OUT_FRM_YUV422SP;
        else
          output_format = cr_first ? DEC_OUT_FRM_444_NV21SP : DEC_OUT_FRM_YUV444SP;
        if (ppu_int_cfg->pixel_width != 8) {
          if (ppu_int_cfg->out_cut_8bits) {
            if (ppu_int_cfg->chroma_format == PP_CHROMA_420)
              output_format = cr_first ? DEC_OUT_FRM_NV21SP : DEC_OUT_FRM_YUV420SP;
            else if (ppu_int_cfg->chroma_format == PP_CHROMA_422)
              output_format = cr_first ? DEC_OUT_FRM_422_NV21SP : DEC_OUT_FRM_YUV422SP;
            else
              output_format = cr_first ? DEC_OUT_FRM_444_NV21SP : DEC_OUT_FRM_YUV444SP;
          } else if (ppu_int_cfg->out_p010) {
            if (ppu_int_cfg->chroma_format == PP_CHROMA_420)
              output_format = cr_first ? DEC_OUT_FRM_NV21SP_P010 : DEC_OUT_FRM_YUV420SP_P010;
            else if (ppu_int_cfg->chroma_format == PP_CHROMA_422)
              output_format = cr_first ? DEC_OUT_FRM_422_NV21SP_P010 : DEC_OUT_FRM_YUV422SP_P010;
            else
              output_format = cr_first ? DEC_OUT_FRM_444_NV21SP_P010 : DEC_OUT_FRM_YUV444SP_P010;
          } else if (ppu_int_cfg->out_I010) {
            if (ppu_int_cfg->chroma_format == PP_CHROMA_420)
              output_format = cr_first ? DEC_OUT_FRM_NV21SP_I010 : DEC_OUT_FRM_YUV420SP_I010;
            else if (ppu_int_cfg->chroma_format == PP_CHROMA_422)
              output_format = cr_first ? DEC_OUT_FRM_422_NV21SP_I010 : DEC_OUT_FRM_YUV422SP_I010;
            else
              output_format = cr_first ? DEC_OUT_FRM_444_NV21SP_I010 : DEC_OUT_FRM_YUV444SP_I010;
          } else if (ppu_int_cfg->out_L010) {
            if (ppu_int_cfg->chroma_format == PP_CHROMA_420)
              output_format = cr_first ? DEC_OUT_FRM_NV21SP_L010 : DEC_OUT_FRM_YUV420SP_L010;
            else if (ppu_int_cfg->chroma_format == PP_CHROMA_422)
              output_format = cr_first ? DEC_OUT_FRM_422_NV21SP_L010 : DEC_OUT_FRM_YUV422SP_L010;
            else
              output_format = cr_first ? DEC_OUT_FRM_444_NV21SP_L010 : DEC_OUT_FRM_YUV444SP_L010;
          } else if (ppu_int_cfg->out_1010) {
            if (ppu_int_cfg->chroma_format == PP_CHROMA_420)
              output_format = cr_first ? DEC_OUT_FRM_NV21SP_1010 : DEC_OUT_FRM_YUV420SP_1010;
            else if (ppu_int_cfg->chroma_format == PP_CHROMA_422)
              output_format = cr_first ? DEC_OUT_FRM_422_NV21SP_1010 : DEC_OUT_FRM_YUV422SP_1010;
          } else if (ppu_int_cfg->out_p012) {
            if (ppu_int_cfg->chroma_format == PP_CHROMA_420)
              output_format = cr_first ? DEC_OUT_FRM_NV21SP_P012 : DEC_OUT_FRM_YUV420SP_P012;
            else if (ppu_int_cfg->chroma_format == PP_CHROMA_422)
              output_format = cr_first ? DEC_OUT_FRM_422_NV21SP_P012 : DEC_OUT_FRM_YUV422SP_P012;
            else
              output_format = cr_first ? DEC_OUT_FRM_444_NV21SP_P012 : DEC_OUT_FRM_YUV444SP_P012;
          } else if (ppu_int_cfg->out_I012) {
            if (ppu_int_cfg->chroma_format == PP_CHROMA_420)
              output_format = cr_first ? DEC_OUT_FRM_NV21SP_I012 : DEC_OUT_FRM_YUV420SP_I012;
            else if (ppu_int_cfg->chroma_format == PP_CHROMA_422)
              output_format = cr_first ? DEC_OUT_FRM_422_NV21SP_I012 : DEC_OUT_FRM_YUV422SP_I012;
            else
              output_format = cr_first ? DEC_OUT_FRM_444_NV21SP_I012 : DEC_OUT_FRM_YUV444SP_I012;
          } else {
            if (ppu_int_cfg->chroma_format == PP_CHROMA_420) {
              if (ppu_int_cfg->pixel_width == 10)
                output_format = cr_first ? DEC_OUT_FRM_NV21SP_PACK10 : DEC_OUT_FRM_YUV420SP_PACK10;
              else
                output_format = cr_first ? DEC_OUT_FRM_NV21SP_PACK12 : DEC_OUT_FRM_YUV420SP_PACK12;
            } else if (ppu_int_cfg->chroma_format == PP_CHROMA_422) {
              if (ppu_int_cfg->pixel_width == 10)
                output_format = cr_first ? DEC_OUT_FRM_422_NV21SP_PACK10 : DEC_OUT_FRM_YUV422SP_PACK10;
              else
                output_format = cr_first ? DEC_OUT_FRM_422_NV21SP_PACK12 : DEC_OUT_FRM_YUV422SP_PACK12;
            } else {
              if (ppu_int_cfg->pixel_width == 10)
                output_format = cr_first ? DEC_OUT_FRM_444_NV21SP_PACK10 : DEC_OUT_FRM_YUV444SP_PACK10;
              else
                output_format = cr_first ? DEC_OUT_FRM_444_NV21SP_PACK12 : DEC_OUT_FRM_YUV444SP_PACK12;
            }
          }
        }
      }
    }
  }
  return (output_format);
}

void PPSetSBIRegs(u32 *pp_regs, u32 i, PpUnitIntConfig *ppu_cfg, u32 sbi_pic_start) {
  u32 sbi_lu_segment_size, sbi_ch_segment_size, sbi_segment_num_tot;
  u32 ystride = ppu_cfg->ystride, cstride = ppu_cfg->cstride;

  if (!ppu_cfg->enabled || !ppu_cfg->sbi_en)
    return;
  /* set SBI register */
  if (ppu_cfg->tiled_e) {
    if (!ppu_cfg->tile_mode || ppu_cfg->tile_mode == TILED2x4) {
      ystride /= 4; // tiled4x4/tiled2x4 stride in one row
      cstride /= 4;
    }
  }

  sbi_lu_segment_size = ppu_cfg->sbi_lu_segmentbuffer_entry_size * ystride * 16;
  if (ppu_cfg->out_format == PP_OUT_FMT_YUV400_WITH_ALPHA)
    sbi_lu_segment_size *= 2;

  /* luma */
  SetPpuRegister(pp_regs, HWIF_SBIP_LU_ENABLE_U, i, ppu_cfg->sbi_en);
  SetPpuRegister(pp_regs, HWIF_SBIP_SW_RESET_FLAG_U, i, 0);
  SetPpuRegister(pp_regs, HWIF_SBIP_LU_SEGMENT_ENTRY_COUNT_U, i, ppu_cfg->sbi_lu_segmentbuffer_entry_count);
  SetPpuRegister(pp_regs, HWIF_SBIP_LU_SEGMENT_ENTRY_SIZE_U, i, sbi_lu_segment_size);

  if (sbi_pic_start == 0) {
    SetPpuRegister(pp_regs, HWIF_SBIP_LU_NEW_START_FLAG_U, i, 1);
    SetPpuRegister(pp_regs, HWIF_SBIP_LU_RESET_OFFSET_FLAG_U, i, 1);
  }
  else
    SetPpuRegister(pp_regs, HWIF_SBIP_LU_RESET_OFFSET_FLAG_U, i, ppu_cfg->sbi_lu_reset_offset);
  SetPpuRegister(pp_regs, HWIF_SBIP_LU_SEGMENT_OFFSET_U_LSB, i, ppu_cfg->sbi_lu_segmentbuffer_offset * sbi_lu_segment_size);
  SetPpuRegister(pp_regs, HWIF_SBIP_LU_TIMEOUT_MASK_U, i, 0);
  SetPpuRegister(pp_regs, HWIF_PP_OUT_LU_SEGMENT_BUF_SIZE_U, i, sbi_lu_segment_size * ppu_cfg->sbi_lu_segmentbuffer_entry_count);
  SetPpuRegister(pp_regs, HWIF_PP_OUT_SEGMENT_ENTRY_HEIGHT_U, i, sbi_lu_segment_size / ystride / 16);
  SetPpuRegister(pp_regs, HWIF_SBIP_LU_CH_COUPLED_EN_U, i, ppu_cfg->sbi_coupled);
  SetPpuRegister(pp_regs, HWIF_SBIP_LU_STREAM_ID_U, i, ppu_cfg->sbi_lu_stream_id);

  /* chroma */
  if (!(IS_PP_OUT_MONOCHROME(ppu_cfg->out_format) || IS_PP_OUT_PACKED422(ppu_cfg->out_format)
      || (ppu_cfg->rgb && IS_PACKED_RGB(ppu_cfg->rgb_format))
      || (ppu_cfg->tiled_e && (!ppu_cfg->tile_mode || ppu_cfg->tile_mode == TILED2x4))
      || ppu_cfg->sbi_coupled || ppu_cfg->out_format == PP_OUT_FMT_YUV400_WITH_ALPHA)) {
    sbi_ch_segment_size = ppu_cfg->sbi_ch_segmentbuffer_entry_size * cstride * 8;
    if (IS_PP_OUT_422(ppu_cfg->out_format)) sbi_ch_segment_size = sbi_lu_segment_size;
    SetPpuRegister(pp_regs, HWIF_SBIP_CH_ENABLE_U, i, ppu_cfg->sbi_en);
    SetPpuRegister(pp_regs, HWIF_SBIP_CH_SEGMENT_ENTRY_COUNT_U, i, ppu_cfg->sbi_ch_segmentbuffer_entry_count);
    SetPpuRegister(pp_regs, HWIF_SBIP_CH_SEGMENT_ENTRY_SIZE_U, i, sbi_ch_segment_size);
    if (sbi_pic_start == 0) {
      SetPpuRegister(pp_regs, HWIF_SBIP_CH_NEW_START_FLAG_U, i, 1);
      SetPpuRegister(pp_regs, HWIF_SBIP_CH_RESET_OFFSET_FLAG_U, i, 1);
    }
    else
      SetPpuRegister(pp_regs, HWIF_SBIP_CH_RESET_OFFSET_FLAG_U, i, ppu_cfg->sbi_ch_reset_offset);
    SetPpuRegister(pp_regs, HWIF_SBIP_CH_SEGMENT_OFFSET_U_LSB, i, ppu_cfg->sbi_ch_segmentbuffer_offset * sbi_ch_segment_size);
    SetPpuRegister(pp_regs, HWIF_SBIP_CH_TIMEOUT_MASK_U, i, 0);
    SetPpuRegister(pp_regs, HWIF_PP_OUT_CH_SEGMENT_BUF_SIZE_U, i, sbi_ch_segment_size * ppu_cfg->sbi_ch_segmentbuffer_entry_count);
    SetPpuRegister(pp_regs, HWIF_SBIP_CH_STREAM_ID_U, i, ppu_cfg->sbi_ch_stream_id);

#ifndef MODEL_SIMULATION
      ppu_cfg->chroma_offset = sbi_lu_segment_size * ppu_cfg->sbi_lu_segmentbuffer_entry_count;
      // store flexa small buff size in HWIF_PPX_OUT_CH_BASE for FPGA
#endif
  }

  sbi_segment_num_tot = (u32)ceil((double)(ppu_cfg->luma_size) / (double)sbi_lu_segment_size);
  SetPpuRegister(pp_regs, HWIF_SBIP_SEGMENT_NUM_TOT_U, i, sbi_segment_num_tot);
}

u32 CheckPpStride(PpUnitIntConfig *ppu_cfg) {
  enum StrideUnit unit;

  ppu_cfg->frm_width = ppu_cfg->scale.width + ppu_cfg->pad.l_off + ppu_cfg->pad.r_off;
  ppu_cfg->frm_height = ppu_cfg->scale.height + ppu_cfg->pad.t_off + ppu_cfg->pad.b_off;

  if (ppu_cfg->norm_enable) {
    if (ppu_cfg->norm_fmat == NORM_OUT_FP32)
      ppu_cfg->pixel_width = 32;
    if (ppu_cfg->norm_fmat == NORM_OUT_FP16 || ppu_cfg->norm_fmat == NORM_OUT_INT16)
      ppu_cfg->pixel_width = 16;
    if (ppu_cfg->norm_fmat == NORM_OUT_UINT8 || ppu_cfg->norm_fmat == NORM_OUT_INT8)
      ppu_cfg->pixel_width = 8;
  }

  if (PPGetUnitStride(ppu_cfg))
    return 1;

  /* Validate the stride */
  unit = AdjustStrideUnit(ppu_cfg->ystride, ppu_cfg->cstride);
  if (unit == STRIDE_UNIT_NOT_SUPPORTED) {
    APITRACEERR("%s", "CheckPpStride# Stride not supported.\n");
    return 1;
  }
  // for yuv420 supertile
  if (ppu_cfg->tile_mode == TILED64x64 && !ppu_cfg->rgb)
    unit = 7; // STRIDE_UNIT_512B

  ppu_cfg->unit = unit;

  return 0;
}
