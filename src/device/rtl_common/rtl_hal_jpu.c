/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include <stdio.h>
#include <string.h>
#include "os_mem.h"
#include "rtl_hal_jpu.h"
#include "trace.h"



enum
{
    SAMPLE_420 = 0xA,
    SAMPLE_H422 = 0x9,
    SAMPLE_V422 = 0x6,
    SAMPLE_444 = 0x5,
    SAMPLE_400 = 0x1
};

const static uint8_t cDefHuffBits[4][16] =
{
    {
        // DC index 0 (Luminance DC)
        0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
    ,
    {
        // AC index 0 (Luminance AC)
        0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
        0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D
    }
    ,
    {
        // DC index 1 (Chrominance DC)
        0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00
    }
    ,
    {
        // AC index 1 (Chrominance AC)
        0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04,
        0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77
    }
};

const static uint8_t cDefHuffVal[4][162] =
{
    {
        // DC index 0 (Luminance DC)
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B
    }
    ,
    {
        // AC index 0 (Luminance AC)
        0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
        0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
        0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
        0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0,
        0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
        0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
        0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
        0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
        0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
        0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
        0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5,
        0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
        0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
        0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
        0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
        0xF9, 0xFA
    }
    ,
    {
        // DC index 1 (Chrominance DC)
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B
    }
    ,
    {
        // AC index 1 (Chrominance AC)
        0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
        0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
        0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
        0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0,
        0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34,
        0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
        0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
        0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
        0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96,
        0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5,
        0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4,
        0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3,
        0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2,
        0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
        0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9,
        0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
        0xF9, 0xFA
    }
};



static unsigned char lumaDcBits[16] =
{
    0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static unsigned char lumaDcValue[16] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x00, 0x00, 0x00, 0x00,
};
static unsigned char lumaAcBits[16] =
{
    0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
    0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D,
};
static unsigned char lumaAcValue[168] =
{
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
    0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
    0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
    0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
    0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
    0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5,
    0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
    0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
    0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
    0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
    0xF9, 0xFA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static unsigned char chromaDcBits[16] =
{
    0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static unsigned char chromaDcValue[16] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x00, 0x00, 0x00, 0x00,
};
static unsigned char chromaAcBits[16] =
{
    0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04,
    0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77,
};
static unsigned char chromaAcValue[168] =
{
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
    0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
    0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0,
    0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34,
    0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
    0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96,
    0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5,
    0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4,
    0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3,
    0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2,
    0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
    0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9,
    0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
    0xF9, 0xFA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static unsigned char lumaQ2[64] =
{
    0x06, 0x04, 0x04, 0x04, 0x05, 0x04, 0x06, 0x05,
    0x05, 0x06, 0x09, 0x06, 0x05, 0x06, 0x09, 0x0B,
    0x08, 0x06, 0x06, 0x08, 0x0B, 0x0C, 0x0A, 0x0A,
    0x0B, 0x0A, 0x0A, 0x0C, 0x10, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x10, 0x0C, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
};
static unsigned char chromaBQ2[64] =
{
    0x07, 0x07, 0x07, 0x0D, 0x0C, 0x0D, 0x18, 0x10,
    0x10, 0x18, 0x14, 0x0E, 0x0E, 0x0E, 0x14, 0x14,
    0x0E, 0x0E, 0x0E, 0x0E, 0x14, 0x11, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x11, 0x11, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x11, 0x0C, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
};



static uint8_t jpu_heap_available = false;
static void *(*JPU_MALLOC)(size_t);
static void (*JPU_FREE)(void *);

static JPU_OUTPUT_INFO output_info;
static uint32_t header_sz;
/*============================================================================*
 *                           Static Functions
 *============================================================================*/
// check param valid

static JPU_ERROR JPU_Dec_Prepare(JPU_DEC_PARAM *usr_param, JPU_DEC_PARAM_INT *param)
{
    memset(param, 0, sizeof(JPU_DEC_PARAM_INT));

    param->data = usr_param->data;
    param->size = usr_param->size;

    // default param: not exported to user for now
    {
        // Chroma format type [0](SEPARATED CHROMA) [1](CBCR INTERLEAVED) [2](CRCB INTERLEAVED)
        param->chromaInterleave = JPU_SEPARATED_CHROMA;
        param->StreamEndian = JPU_BBC_64_BIT_LITTLE_ENDIAN;
        param->FrameEndian = JPU_DPB_64_BIT_BIG_ENDIAN;

        //// Can NOT do rotation and mirror, when partial mode enable.
        // rotation angle in degrees(0, 90, 180, 270);
        param->rotAngle = ROT_ANGLE_0;
        // mirror direction(0-no mirror, 1-vertical, 2-horizontal, 3-both);
        param->mirDir = MIRDIR_NONE;
        // scalerMode: 0-no scaler, 1-1/2, 2-1/4, 3-1/8
        param->iHorScaleMode = JPU_DOWN_SCALE_NONE;
        param->iVerScaleMode = JPU_DOWN_SCALE_NONE;

        // partial Mode(0: OFF 1: ON);
        param->usePartialMode = 0;
        // Num of Frame Buffer[ 2 ~ 4 ] ;
        param->partialBufNum = 4;
    }

    // Packed stream format output [0](PLANAR) [1](YUYV) [2](UYVY) [3](YVYU) [4](VYUY) [5](YUV_444 PACKED)
    param->packedFormat = usr_param->frameFormat;
//return JPU_SUCCESS;
    // Wrapper enable: 0-OFF, 1-ON
    param->useWrapper = usr_param->useWrapper;
    //  0-JPG_ARGB8888, 1-JPG_RGB888, 2-JPG_RGB565
    param->rgbType = usr_param->rgbType;
    if (param->useWrapper)
    {
        if (param->packedFormat == PACKED_FORMAT_NONE)
        {
            // DBG_DIRECT("Invalid operation mode : plannar format and wrapper can not be worked\n");
            return JPU_ERR_INVALID_PARAM;
        }
        if (param->rgbType == JPU_ARGB8888)
        {
            param->opacity = usr_param->opacity;
        }
    }

    // param->roiEnable = usr_param->roiEnable;

    if (param->roiEnable)
    {
        param->roiOffsetX = usr_param->roiOffsetX;
        param->roiOffsetY = usr_param->roiOffsetY;
        param->roiWidth  = usr_param->roiWidth;
        param->roiHeight = usr_param->roiHeight;
    }

    if (!param->roiEnable && !param->packedFormat)
    {
        // partial Mode(0: OFF 1: ON);
        // param->usePartialMode = usr_param->usePartialMode;
        if (param->usePartialMode)
        {
            // Num of Frame Buffer[ 2 ~ 4 ] ;
            // param->partialBufNum = usr_param->partialBufNum;
            if (param->partialBufNum > 4)
            {
                param->partialBufNum = 4;
            }
        }
    }

    if (!param->usePartialMode)
    {
        // rotation angle in degrees(0, 90, 180, 270);
        // param->rotAngle = usr_param->rotAngle;
        if (param->rotAngle != ROT_ANGLE_0 && param->rotAngle != ROT_ANGLE_90 &&
            param->rotAngle != ROT_ANGLE_180 &&
            param->rotAngle != ROT_ANGLE_270)
        {
            // DBG_DIRECT("Invalid rotation angle.\n");
            return JPU_ERR_INVALID_PARAM;
        }
        // mirror direction(0-no mirror, 1-vertical, 2-horizontal, 3-both);
        // param->mirDir = usr_param->mirDir;
        if (param->mirDir != MIRDIR_NONE && param->mirDir != MIRDIR_VER && param->mirDir != MIRDIR_HOR &&
            param->mirDir != MIRDIR_HOR_VER)
        {
            // DBG_DIRECT("Invalid mirror direction.\n");
            return JPU_ERR_INVALID_PARAM;
        }
        if (param->rotAngle != ROT_ANGLE_0 || param->mirDir != MIRDIR_NONE)
        {
            param->useRot = 1;
        }
    }

    if (param->usePartialMode && param->roiEnable)
    {
        //- DBG_DIRECT("Invalid operation mode : partial and ROI mode can not be worked\n");
        return JPU_ERR_INVALID_PARAM;
    }
    // if (param->packedFormat && param->roiEnable)
    // {
    //     //- DBG_DIRECT("Invalid operation mode : packed mode and ROI mode can not be worked\n");
    // return JPU_ERR_INVALID_PARAM;
    // }
    if ((param->iHorScaleMode || param->iVerScaleMode) && param->roiEnable)
    {
        //- DBG_DIRECT("Invalid operation mode : Scaler mode and ROI mode can not be worked\n");
        return JPU_ERR_INVALID_PARAM;
    }
    if (param->useRot && param->roiEnable)
    {
        //- DBG_DIRECT("Invalid operation mode : Rotator mode and ROI mode can not be worked\n");
        return JPU_ERR_INVALID_PARAM;
    }
    if ((param->iHorScaleMode || param->iVerScaleMode) && (param->rotAngle ||
                                                           param->mirDir))
    {
        //- DBG_DIRECT("Invalid operation mode : Scaler mode and Rotator mode can not be worked\n");
        return JPU_ERR_INVALID_PARAM;
    }

    return JPU_SUCCESS;
}




// decode header


// buffer bit operation func
#define init_get_bits(CTX, BUFFER, SIZE) JpuGbuInit(CTX, BUFFER, SIZE)
#define show_bits(CTX, NUM) JpuGguShowBit(CTX, NUM)
#define get_bits(CTX, NUM) JpuGbuGetBit(CTX, NUM)
#define get_bits_left(CTX) JpuGbuGetLeftBitCount(CTX)
#define get_bits_count(CTX) JpuGbuGetUsedBitCount(CTX)


static int JpuGbuInit(jpu_getbit_context_t *ctx, uint8_t *buffer, int size)
{
    ctx->buffer = buffer;
    ctx->index = 0;
    ctx->size = size / 8;

    return 1;
}

static int JpuGbuGetUsedBitCount(jpu_getbit_context_t *ctx)
{
    return ctx->index * 8;
}

static int JpuGbuGetLeftBitCount(jpu_getbit_context_t *ctx)
{
    return (ctx->size * 8) - JpuGbuGetUsedBitCount(ctx);
}

static uint32_t JpuGbuGetBit(jpu_getbit_context_t *ctx, int bit_num)
{
    uint8_t *p;
    uint32_t b = 0x0;

    if (bit_num > JpuGbuGetLeftBitCount(ctx))
    {
        return (uint32_t) - 1;
    }

    p = ctx->buffer + ctx->index;

    if (bit_num == 8)
    {
        b = *p;
        ctx->index++;
    }
    else if (bit_num == 16)
    {
        b = *p++ << 8;
        b |= *p++;
        ctx->index += 2;
    }
    else if (bit_num == 32)
    {
        b = *p++ << 24;
        b |= (*p++ << 16);
        b |= (*p++ << 8);
        b |= (*p++ << 0);
        ctx->index += 4;
    }

    return b;
}

static uint32_t JpuGguShowBit(jpu_getbit_context_t *ctx, int bit_num)
{
    uint8_t *p;
    uint32_t b = 0x0;

    if (bit_num > JpuGbuGetLeftBitCount(ctx))
    {
        return (uint32_t) - 1;
    }

    p = ctx->buffer + ctx->index;

    if (bit_num == 8)
    {
        b = *p;
    }
    else if (bit_num == 16)
    {
        b = *p++ << 8;
        b |= *p++;
    }
    else if (bit_num == 32)
    {
        b = *p++ << 24;
        b |= (*p++ << 16);
        b |= (*p++ << 8);
        b |= (*p++ << 0);
    }

    return b;
}


static int find_start_code(JPU_DEC_INFO *jpg)
{
    int word;

    for (;;)
    {
        if (get_bits_left(&jpg->gbc) <= 16)
        {
            ////printf("hit end of stream\n");
            return 0;
        }

        word = show_bits(&jpg->gbc, 16);
        if ((word > 0xFF00) && (word < 0xFFFF))
        {
            break;
        }


        get_bits(&jpg->gbc, 8);
    }

    return word;
}

static int check_start_code(JPU_DEC_INFO *jpg)
{
    if (show_bits(&jpg->gbc, 8) == 0xFF)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static int decode_app_header(JPU_DEC_INFO *jpg)
{
    int length;

    if (get_bits_left(&jpg->gbc) < 16)
    {
        return 0;
    }
    length = get_bits(&jpg->gbc, 16);
    length -= 2;

    while (length-- > 0)
    {
        if (get_bits_left(&jpg->gbc) < 8)
        {
            return 0;
        }
        get_bits(&jpg->gbc, 8);
    }

    return 1;
}


static int decode_dri_header(JPU_DEC_INFO *jpg)
{
    //Length, Lr
    if (get_bits_left(&jpg->gbc) < 16 * 2)
    {
        return 0;
    }
    get_bits(&jpg->gbc, 16);

    jpg->rstIntval = get_bits(&jpg->gbc, 16);


    return 1;
}

static int decode_dqt_header(JPU_DEC_INFO *jpg)
{
    int Pq;
    int Tq;
    int i;
    int tmp;
    if (get_bits_left(&jpg->gbc) < 16)
    {
        return 0;
    }

    // Lq, Length of DQT
    get_bits(&jpg->gbc, 16);

    do
    {
        if (get_bits_left(&jpg->gbc) < 4 + 4 + 8 * 64)
        {
            return 0;
        }

        tmp = get_bits(&jpg->gbc, 8);
        // Pq, Quantization Precision
        Pq = (tmp >> 4) & 0xf;  // H
        // Tq, Quantization table destination identifier
        Tq = tmp & 0xf;         // L

        // Only support Pq = 0, 8bit, 1 byte
        // 64 x 1
        for (i = 0; i < 64; i++)
        {
            jpg->qMatTab[Tq][i] = (uint8_t)get_bits(&jpg->gbc, 8);
        }
    }
    while (!check_start_code(jpg));

    if (Pq != 0) // not 8-bit
    {
        ////printf("pq is not set to zero\n");
        return 0;
    }
    return 1;
}

static int decode_dht_header(JPU_DEC_INFO *jpg)
{
    int Tc;
    int Th;
    int ThTc;
    int bitCnt;
    int i;
    int tmp;
    // Length, Lh
    if (get_bits_left(&jpg->gbc) < 16)
    {
        return 0;
    }

    get_bits(&jpg->gbc, 16);

    do
    {
        if (get_bits_left(&jpg->gbc) < 8 + 8 * 16)
        {
            return 0;
        }

        // Table class - DC, AC
        tmp = get_bits(&jpg->gbc, 8);
        // Table destination identifier
        Tc = (tmp >> 4) & 0xf;
        Th = tmp & 0xf;

        // DC_ID0 (0x00) -> 0
        // AC_ID0 (0x10) -> 1
        // DC_ID1 (0x01) -> 2
        // AC_ID1 (0x11) -> 3
        ThTc = ((Th & 1) << 1) | (Tc & 1);

        // Get Huff Bits list
        bitCnt = 0;
        for (i = 0; i < 16; i++)
        {
            jpg->huffBits[ThTc][i] = (uint8_t)get_bits(&jpg->gbc, 8);
            bitCnt += jpg->huffBits[ThTc][i];

            if (cDefHuffBits[ThTc][i] != jpg->huffBits[ThTc][i])
            {
                jpg->userHuffTab = 1;
            }
        }


        if (get_bits_left(&jpg->gbc) <  8 * bitCnt)
        {
            return 0;
        }

        // Get Huff Val list
        for (i = 0; i < bitCnt; i++)
        {
            jpg->huffVal[ThTc][i] = (uint8_t)get_bits(&jpg->gbc, 8);
            if (cDefHuffVal[ThTc][i] != jpg->huffVal[ThTc][i])
            {
                jpg->userHuffTab = 1;
            }
        }
    }
    while (!check_start_code(jpg));

    return 1;
}
static int decode_sof_header(JPU_DEC_INFO *jpg)
{
    int samplePrecision;
    int sampleFactor;
    int i;
    int Tqi;
    uint8_t compID;
    int hSampFact[3] = {0,};
    int vSampFact[3] = {0,};
    int picX, picY;
    int numComp;
    int tmp;

    if (get_bits_left(&jpg->gbc) < 16 + 8 + 16 + 16 + 8)
    {
        return 0;
    }
    // LF, Length of SOF
    get_bits(&jpg->gbc, 16);

    // Sample Precision: Baseline(8), P
    samplePrecision = get_bits(&jpg->gbc, 8);

    if (samplePrecision != 8)
    {
        //printf("Sample Precision is not 8\n");
        return 0;
    }

    picY = get_bits(&jpg->gbc, 16);
    if (picY > MAX_JPG_PIC_WIDTH)
    {
        //printf("Picture Vertical Size limits Maximum size\n");
        return 0;
    }

    picX = get_bits(&jpg->gbc, 16);
    if (picX > MAX_JPG_PIC_HEIGHT)
    {
        //printf("Picture Horizontal Size limits Maximum size\n");
        return 0;
    }

    //Number of Components in Frame: Nf
    numComp = get_bits(&jpg->gbc, 8);
    if (numComp > 3)
    {
        //printf("Picture Horizontal Size limits Maximum size\n");
    }

    if (get_bits_left(&jpg->gbc) < numComp * (8 + 4 + 4 + 8))
    {
        return 0;
    }
    for (i = 0; i < numComp; i++)
    {
        // Component ID, Ci 0 ~ 255
        compID = (uint8_t)get_bits(&jpg->gbc, 8);
        tmp = get_bits(&jpg->gbc, 8);
        // Horizontal Sampling Factor, Hi
        hSampFact[i] = (tmp >> 4) & 0xf;
        // Vertical Sampling Factor, Vi
        vSampFact[i] = tmp & 0xf;
        // Quantization Table Selector, Tqi
        Tqi = get_bits(&jpg->gbc, 8);

        jpg->cInfoTab[i][0] = compID;
        jpg->cInfoTab[i][1] = (uint8_t)hSampFact[i];
        jpg->cInfoTab[i][2] = (uint8_t)vSampFact[i];
        jpg->cInfoTab[i][3] = (uint8_t)Tqi;
    }

    //if ( hSampFact[0]>2 || vSampFact[0]>2 || ( numComp == 3 && ( hSampFact[1]!=1 || hSampFact[2]!=1 || vSampFact[1]!=1 || vSampFact[2]!=1) ) )
    //printf("Not Supported Sampling Factor\n");

    if (numComp == 1)
    {
        sampleFactor = SAMPLE_400;
    }
    else
    {
        sampleFactor = ((hSampFact[0] & 3) << 2) | (vSampFact[0] & 3);
    }

    switch (sampleFactor)
    {
    case SAMPLE_420:
        jpg->format = JPU_FORMAT_420;
        break;
    case SAMPLE_H422:
        jpg->format = JPU_FORMAT_422;
        break;
    case SAMPLE_V422:
        jpg->format = JPU_FORMAT_224;
        break;
    case SAMPLE_444:
        jpg->format = JPU_FORMAT_444;
        break;
    default:    // 4:0:0
        jpg->format = JPU_FORMAT_400;
    }

    jpg->picWidth = picX;
    jpg->picHeight = picY;



    return 1;
}

static int decode_sos_header(JPU_DEC_INFO *jpg)
{
    int i, j;
    int len;
    int numComp;
    int compID;
    int ecsPtr;
    int ss, se, ah, al;
    int dcHufTblIdx[3] = {0,};
    int acHufTblIdx[3] = {0,};
    int tmp;

    if (get_bits_left(&jpg->gbc) < 8)
    {
        return 0;
    }
    // Length, Ls
    len = get_bits(&jpg->gbc, 16);

    jpg->ecsPtr = get_bits_count(&jpg->gbc) / 8 + len - 2 ;

    ecsPtr = jpg->ecsPtr;

    //printf("ecsPtr=0x%x frameOffset=0x%x, ecsOffset=0x%x, wrPtr=0x%x, rdPtr0x%x\n", jpg->ecsPtr, jpg->frameOffset, ecsPtr, jpg->streamWrPtr, jpg->streamRdPtr);

    // BBC GBU Pointer
    jpg->pagePtr = ecsPtr >> 8;                                  //page unit  ecsPtr/256;
    jpg->wordPtr = (ecsPtr & 0xF0) >>
                   2;                         // word unit ((ecsPtr % 256) & 0xF0) / 4;

    if (jpg->pagePtr & 1)
    {
        jpg->wordPtr += 64;
    }

    jpg->bitPtr = (ecsPtr & 0xF) << 3;                          // bit unit (ecsPtr & 0xF) * 8;

    if (get_bits_left(&jpg->gbc) < 8)
    {
        return 0;
    }
    //Number of Components in Scan: Ns
    numComp = get_bits(&jpg->gbc, 8);

    if (get_bits_left(&jpg->gbc) < numComp * (8 + 4 + 4))
    {
        return 0;
    }
    for (i = 0; i < numComp; i++)
    {
        // Component ID, Csj 0 ~ 255
        compID = get_bits(&jpg->gbc, 8);
        tmp = get_bits(&jpg->gbc, 8);
        // dc entropy coding table selector, Tdj
        dcHufTblIdx[i] = (tmp >> 4) & 0xf;
        // ac entropy coding table selector, Taj
        acHufTblIdx[i] = tmp & 0xf;


        for (j = 0; j < numComp; j++)
        {
            if (compID == jpg->cInfoTab[j][0])
            {
                jpg->cInfoTab[j][4] = (uint8_t)dcHufTblIdx[i];
                jpg->cInfoTab[j][5] = (uint8_t)acHufTblIdx[i];
            }
        }
    }

    if (get_bits_left(&jpg->gbc) < 8 + 8 + 4 + 4)
    {
        return 0;
    }
    // Ss 0
    ss = get_bits(&jpg->gbc, 8);
    // Se 3F
    se = get_bits(&jpg->gbc, 8);
    tmp = get_bits(&jpg->gbc, 8);
    // Ah 0
    ah = (i >> 4) & 0xf;
    // Al 0
    al = tmp & 0xf;

    if ((ss != 0) || (se != 0x3F) || (ah != 0) || (al != 0))
    {
        //printf("The Jpeg Image must be another profile\n");
        return 0;
    }

    return 1;
}

static void genDecHuffTab(JPU_DEC_INFO *jpg, int tabNum)
{
    unsigned char *huffPtr, *huffBits;
    unsigned int *huffMax, *huffMin;

    int ptrCnt = 0;
    int huffCode = 0;
    int zeroFlag = 0;
    int dataFlag = 0;
    int i;

    huffBits    = jpg->huffBits[tabNum];
    huffPtr     = jpg->huffPtr[tabNum];
    huffMax     = jpg->huffMax[tabNum];
    huffMin     = jpg->huffMin[tabNum];

    for (i = 0; i < 16; i++)
    {
        if (huffBits[i]) // if there is bit cnt value
        {
            huffPtr[i] = (uint8_t)ptrCnt;
            ptrCnt += huffBits[i];
            huffMin[i] = huffCode;
            huffMax[i] = huffCode + (huffBits[i] - 1);
            dataFlag = 1;
            zeroFlag = 0;
        }
        else
        {
            huffPtr[i] = 0xFF;
            huffMin[i] = 0xFFFF;
            huffMax[i] = 0xFFFF;
            zeroFlag = 1;
        }

        if (dataFlag == 1)
        {
            if (zeroFlag == 1)
            {
                huffCode <<= 1;
            }
            else
            {
                huffCode = (huffMax[i] + 1) << 1;
            }
        }
    }

}

static int JPU_Dec_Header(JPU_DEC_PARAM_INT *param, JPU_DEC_INFO *jpg)
{
    unsigned int code;
    int ret = 1;
    int i;
    int temp;
    int wrOffset;
    // buffer addr, set in decOpen()
    uint8_t *b = param->data;
    uint32_t size = param->size; // (param->size + 1023) >> 10

    if (!b || !size)
    {
        ret = -1;
        goto DONE_DEC_HEADER;
    }

    memset(jpg, 0, sizeof(JPU_DEC_INFO));
    size = ((size + 1023) >> 10) << 10;
    init_get_bits(&jpg->gbc, b, size * 8);
    // Initialize component information table

    // reference: CSDN: https://blog.csdn.net/yun_hen/article/details/78135122
    for (;;)
    {
        if (find_start_code(jpg) == 0)
        {
            ret = -1;
            goto DONE_DEC_HEADER;
        }

        code = get_bits(&jpg->gbc, 16); //get bits 2byte

//        DBG_DIRECT("code 0x%x\n", code);
        switch (code)
        {
        case JPU_SOI_Marker:
            break;
        case JPU_JFIF_CODE:
        case JPU_EXIF_CODE:
            {
                // not proc, just skip over
                if (!decode_app_header(jpg))
                {
                    ret = -1;
                    goto DONE_DEC_HEADER;
                }
            }

            break;
        case JPU_DRI_Marker:
            if (!decode_dri_header(jpg))
            {
                ret = -1;
                DBG_DIRECT("[INFO] DRI Header parsing error\n");
                goto DONE_DEC_HEADER;
            }
            break;
        case JPU_DQT_Marker:
            if (!decode_dqt_header(jpg))
            {
                ret = -1;
                DBG_DIRECT("[INFO] DQT Header parsing error\n");
                goto DONE_DEC_HEADER;
            }
            break;
        case JPU_DHT_Marker:
            if (!decode_dht_header(jpg))
            {
                ret = -1;
                DBG_DIRECT("[INFO] DHT Header parsing error\n");
                goto DONE_DEC_HEADER;
            }
            break;
        case JPU_SOF_Marker:
            // Component & format, image size
            if (!decode_sof_header(jpg))
            {
                ret = -1;
                DBG_DIRECT("[INFO] SOF Header parsing error\n");
                goto DONE_DEC_HEADER;
            }
            break;

        case JPU_SOF1_Marker:
        case JPU_SOF9_Marker:
            {
                ret = -3;
                DBG_DIRECT("[INFO] This is used Extended sequential DCT.\n");
                goto DONE_DEC_HEADER;
            }
            break;
        case JPU_SOF2_Marker:
        case JPU_SOF10_Marker:
            {
                ret = -3;
                DBG_DIRECT("[INFO] This is used Progressive DCT.\n");
                return -3;
                //goto DONE_DEC_HEADER;
            }

            break;
        case JPU_SOF3_Marker:
        case JPU_SOF11_Marker:
            {
                ret = -3;
                DBG_DIRECT("[INFO] This is used Lossless(sequential).\n");
                goto DONE_DEC_HEADER;
            }

            break;
        case JPU_SOF5_Marker:
        case JPU_SOF13_Marker:
            {
                ret = -3;
                DBG_DIRECT("[INFO] This is used Differential sequential DCT.\n");
                goto DONE_DEC_HEADER;
            }

            break;
        case JPU_SOF6_Marker:
        case JPU_SOF14_Marker:
            {
                ret = -3;
                DBG_DIRECT("[INFO] This is used Differential progressive DCT.\n");
                goto DONE_DEC_HEADER;
            }

            break;
        case JPU_SOF7_Marker:
        case JPU_SOF15_Marker:
            {
                ret = -3;
                DBG_DIRECT("[INFO] This is used Differential lossless.\n");
                goto DONE_DEC_HEADER;
            }

            break;

        case JPU_SOS_Marker:
            // BBC & GBU pointer
            if (!decode_sos_header(jpg))
            {
                ret = -1;
                DBG_DIRECT("[INFO] SOS Header parsing error\n");
                goto DONE_DEC_HEADER;
            }
            if (!jpg->headerSize)
            {
                jpg->headerSize = jpg->ecsPtr;    // we assume header size of all frame is same for mjpeg case
            }
            goto DONE_DEC_HEADER;
            break;
        case JPU_EOI_Marker:
            goto DONE_DEC_HEADER;
        default:
            switch (code & 0xFFF0)
            {
            case 0xFFE0:    // 0xFFEX
            case 0xFFF0:    // 0xFFFX
                if (get_bits_left(&jpg->gbc) <= 0)
                {
                    {
                        ret = -1;
                        goto DONE_DEC_HEADER;
                    }
                }
                else
                {
                    if (!decode_app_header(jpg))
                    {
                        ret = -1;
                        goto DONE_DEC_HEADER;
                    }
                    break;
                }
            default:
                //in case,  restart marker is founded.
                if ((code & 0xFFF0) >= 0xFFD0 && (code & 0xFFF0) <= 0xFFD7)
                {
                    break;
                }
                else
                {
                    return    0;
                }
            }
            break;
        }
    }

DONE_DEC_HEADER:

    if (ret < 0)
    {
        return ret;
    }

    if (!jpg->ecsPtr)
    {
        return 0;
    }

    // Generate Huffman table information
    for (i = 0; i < 4; i++)
    {
        genDecHuffTab(jpg, i);
    }

    // Q Idx
    temp =             jpg->cInfoTab[0][3];
    temp = temp << 1 | jpg->cInfoTab[1][3];
    temp = temp << 1 | jpg->cInfoTab[2][3];
    jpg->Qidx = temp;


    // Huff Idx[DC, AC]
    temp =             jpg->cInfoTab[0][4];
    temp = temp << 1 | jpg->cInfoTab[1][4];
    temp = temp << 1 | jpg->cInfoTab[2][4];
    jpg->huffDcIdx = temp;

    temp =             jpg->cInfoTab[0][5];
    temp = temp << 1 | jpg->cInfoTab[1][5];
    temp = temp << 1 | jpg->cInfoTab[2][5];
    jpg->huffAcIdx = temp;


    // DEC req num,  JPG format
    switch (jpg->format)
    {
    case JPU_FORMAT_420:
        jpg->busReqNum = 2;
        // jpg->busReqNum = 1;
        jpg->mcuBlockNum = 6;
        jpg->compNum = 3;
        jpg->compInfo[0] = 10;
        jpg->compInfo[1] = 5;
        jpg->compInfo[2] = 5;
        jpg->alignedWidth = ((jpg->picWidth + 15) & ~15);
        jpg->alignedHeight = ((jpg->picHeight + 15) & ~15);
        jpg->mcuWidth  = 16;
        jpg->mcuHeight = 16;
        break;
    case JPU_FORMAT_422:
        jpg->busReqNum = 3;
        jpg->mcuBlockNum = 4;
        jpg->compNum = 3;
        jpg->compInfo[0] = 9;
        jpg->compInfo[1] = 5;
        jpg->compInfo[2] = 5;
        jpg->alignedWidth = ((jpg->picWidth + 15) & ~15);
        jpg->alignedHeight = ((jpg->picHeight + 7) & ~7);
        jpg->mcuWidth  = 16;
        jpg->mcuHeight = 8;
        break;
    case JPU_FORMAT_224:
        jpg->busReqNum = 3;
        jpg->mcuBlockNum = 4;
        jpg->compNum = 3;
        jpg->compInfo[0] = 6;
        jpg->compInfo[1] = 5;
        jpg->compInfo[2] = 5;
        jpg->alignedWidth = ((jpg->picWidth + 7) & ~7);
        jpg->alignedHeight = ((jpg->picHeight + 15) & ~15);
        jpg->mcuWidth  = 8;
        jpg->mcuHeight = 16;
        break;
    case JPU_FORMAT_444:
        jpg->busReqNum = 4;
        jpg->mcuBlockNum = 3;
        jpg->compNum = 3;
        jpg->compInfo[0] = 5;
        jpg->compInfo[1] = 5;
        jpg->compInfo[2] = 5;
        jpg->alignedWidth = ((jpg->picWidth + 7) & ~7);
        jpg->alignedHeight = ((jpg->picHeight + 7) & ~7);
        jpg->mcuWidth  = 8;
        jpg->mcuHeight = 8;
        break;
    case JPU_FORMAT_400:
        jpg->busReqNum = 4;
        jpg->mcuBlockNum = 1;
        jpg->compNum = 1;
        jpg->compInfo[0] = 5;
        jpg->compInfo[1] = 0;
        jpg->compInfo[2] = 0;
        jpg->alignedWidth = ((jpg->picWidth + 7) & ~7);
        jpg->alignedHeight = ((jpg->picHeight + 7) & ~7);
        jpg->mcuWidth  = 8;
        jpg->mcuHeight = 8;
        break;
    }

    return 1;
}






static JPU_ERROR JPU_Enc_Prepare(JPU_ENC_PARAM *usr_param, JPU_ENC_PARAM_INT *param)
{
    memset(param, 0, sizeof(JPU_ENC_PARAM_INT));

    param->data = usr_param->data;
    param->size = usr_param->size;

    // picture width
    param->picWidth = usr_param->picWidth;
    // picture height
    param->picHeight = usr_param->picHeight;

    // Jpeg Encoder Quality Percentage [0] not use / [1%% ~ 100%%]
    param->quality = usr_param->quality;
    if (param->quality == 0)
    {
        // Huffman Table file name(0 : use pre-defined table in Ref-S/W)
        // encConfig.huffFileName[0] = 0;
        // Q Matrix Table file name(0 : use pre-defined table in Ref-S/W)
        // encConfig.qMatFileName[0] = 0;
        //- DBG_DIRECT("Invalid param : quality is 0\n");
        return JPU_ERR_INVALID_PARAM;
    }

    // support: packed  422 -> 420, 422, 444;  444 -> 422
    // YUV format
    // packed 422/444 only
    // Frame Format [0](PLANAR) [1](YUYV) [2](UYVY) [3](YVYU) [4](VYUY) [5](YUV_444 PACKED)
    param->frameFormat = usr_param->frameFormat;

    // JPG format
    // Source Chroma Format 0 (4:2:0) / 1 (4:2:2) / 2 (2:2:4 4:2:2 rotated) / 3 (4:4:4) / 4 (4:0:0)
    param->jpgFormat = usr_param->jpgFormat;

    // Wrapper enable: 0-OFF, 1-ON
    param->useWrapper = usr_param->useWrapper;
    //  0-JPG_ARGB8888, 1-JPG_RGB888, 2-JPG_RGB565
    param->rgbType = usr_param->rgbType;

    param->StreamEndian = JPU_BBC_64_BIT_LITTLE_ENDIAN;
    param->FrameEndian = JPU_DPB_64_BIT_BIG_ENDIAN;

    // packed format and interleace is mutually exclusive
    param->chromaInterleave = JPU_SEPARATED_CHROMA;

    param->bEnStuffByte = JPU_STUFFING_BYTE_FF;

    param->buff_sz = (usr_param->jpg_buff_sz) ? usr_param->jpg_buff_sz : DEFAULT_JPG_BUFF_SZ;

    // if (param->frameFormat != PACKED_FORMAT_444 && param->jpgFormat == JPU_FORMAT_444)
    // {
    //     //- DBG_DIRECT("Invalid format convert: must 444 -> 444.\n");
    //     return JPU_ERR_INVALID_PARAM;
    // }

    if (param->frameFormat == PACKED_FORMAT_NONE && param->useWrapper)
    {
        //- DBG_DIRECT("Invalid format convert: wrapper not support plannar.\n");
        return JPU_ERR_INVALID_PARAM;
    }

    // partial Mode(0: OFF 1: ON)
    param->usePartialMode = 0;

    if (param->usePartialMode)
    {
        // Num of Frame Buffer[ 2 ~ 4 ];
        param->partialBufNum = 4;
    }

    if (param->usePartialMode == 0)
    {
        // Rotation parameter
        // rotation angle in degrees(0, 90, 180, 270)
        param->rotAngle = usr_param->rotAngle;
        if (param->rotAngle != ROT_ANGLE_0 && param->rotAngle != ROT_ANGLE_90 &&
            param->rotAngle != ROT_ANGLE_180 &&
            param->rotAngle != ROT_ANGLE_270)
        {
            //- DBG_DIRECT("Invalid rotation angle.\n");
            return JPU_ERR_INVALID_PARAM;
        }
        // Flip parameter
        // mirror direction(0-no mirror, 1-vertical, 2-horizontal, 3-both)
        param->mirDir = usr_param->mirDir;
        if (param->mirDir != MIRDIR_NONE && param->mirDir != MIRDIR_VER && param->mirDir != MIRDIR_HOR &&
            param->mirDir != MIRDIR_HOR_VER)
        {
            //- DBG_DIRECT("Invalid mirror direction.\n");
            return JPU_ERR_INVALID_PARAM;
        }
    }

    if (param->rotAngle != ROT_ANGLE_0 || param->mirDir != MIRDIR_NONE)
    {
        param->useRot = 1;
    }

    return JPU_SUCCESS;
}



static void getJpuEncOpenParamDefault(JPU_ENC_INFO *info)
{
    memset(info, 0x00, sizeof(JPU_ENC_INFO));

    info->rstIntval = 0;

    // Rearrange and insert pre-defined Huffman table to deticated variable.
    memcpy(info->huffBits[DC_TABLE_INDEX0], lumaDcBits, 16);   // Luma DC BitLength
    memcpy(info->huffVal[DC_TABLE_INDEX0], lumaDcValue, 16);   // Luma DC HuffValue

    memcpy(info->huffBits[AC_TABLE_INDEX0], lumaAcBits, 16);   // Luma DC BitLength
    memcpy(info->huffVal[AC_TABLE_INDEX0], lumaAcValue, 162);  // Luma DC HuffValue

    memcpy(info->huffBits[DC_TABLE_INDEX1], chromaDcBits, 16); // Chroma DC BitLength
    memcpy(info->huffVal[DC_TABLE_INDEX1], chromaDcValue, 16); // Chroma DC HuffValue

    memcpy(info->huffBits[AC_TABLE_INDEX1], chromaAcBits, 16); // Chroma AC BitLength
    memcpy(info->huffVal[AC_TABLE_INDEX1], chromaAcValue, 162); // Chorma AC HuffValue

    // Rearrange and insert pre-defined Q-matrix to deticated variable.
    memcpy(info->qMatTab[DC_TABLE_INDEX0], lumaQ2, 64);
    memcpy(info->qMatTab[AC_TABLE_INDEX0], chromaBQ2, 64);

    memcpy(info->qMatTab[DC_TABLE_INDEX1], info->qMatTab[DC_TABLE_INDEX0], 64);
    memcpy(info->qMatTab[AC_TABLE_INDEX1], info->qMatTab[AC_TABLE_INDEX0], 64);
}


static void JpuEncSetupTables(JPU_ENC_INFO *info, uint32_t quality)
{
    int scale_factor;
    int i;
    long temp;
    const int force_baseline = 1;

    /* These are the sample quantization tables given in JPEG spec section K.1.
      * The spec says that the values given produce "good" quality, and
      * when divided by 2, "very good" quality.
      */
    static const unsigned int std_luminance_quant_tbl[64] =
    {
        16,  11,  10,  16,  24,  40,  51,  61,
        12,  12,  14,  19,  26,  58,  60,  55,
        14,  13,  16,  24,  40,  57,  69,  56,
        14,  17,  22,  29,  51,  87,  80,  62,
        18,  22,  37,  56,  68, 109, 103,  77,
        24,  35,  55,  64,  81, 104, 113,  92,
        49,  64,  78,  87, 103, 121, 120, 101,
        72,  92,  95,  98, 112, 100, 103,  99
    };
    static const unsigned int std_chrominance_quant_tbl[64] =
    {
        17,  18,  24,  47,  99,  99,  99,  99,
        18,  21,  26,  66,  99,  99,  99,  99,
        24,  26,  56,  99,  99,  99,  99,  99,
        47,  66,  99,  99,  99,  99,  99,  99,
        99,  99,  99,  99,  99,  99,  99,  99,
        99,  99,  99,  99,  99,  99,  99,  99,
        99,  99,  99,  99,  99,  99,  99,  99,
        99,  99,  99,  99,  99,  99,  99,  99
    };

    if (quality <= 0) { quality = 1; }
    if (quality > 100) { quality = 100; }

    /* The basic table is used as-is (scaling 100) for a quality of 50.
    * Qualities 50..100 are converted to scaling percentage 200 - 2*Q;
    * note that at Q=100 the scaling is 0, which will cause jpeg_add_quant_table
    * to make all the table entries 1 (hence, minimum quantization loss).
    * Qualities 1..50 are converted to scaling percentage 5000/Q.
    */
    if (quality < 50)
    {
        scale_factor = 5000 / quality;
    }
    else
    {
        scale_factor = 200 - quality * 2;
    }


    for (i = 0; i < 64; i++)
    {
        temp = ((long) std_luminance_quant_tbl[i] * scale_factor + 50L) / 100L;
        /* limit the values to the valid range */
        if (temp <= 0L) { temp = 1L; }
        if (temp > 32767L) { temp = 32767L; } /* max quantizer needed for 12 bits */
        if (force_baseline && temp > 255L)
        {
            temp = 255L;    /* limit to baseline range if requested */
        }

        info->qMatTab[DC_TABLE_INDEX0][i] = (uint8_t)temp;
    }

    for (i = 0; i < 64; i++)
    {
        temp = ((long) std_chrominance_quant_tbl[i] * scale_factor + 50L) / 100L;
        /* limit the values to the valid range */
        if (temp <= 0L) { temp = 1L; }
        if (temp > 32767L) { temp = 32767L; } /* max quantizer needed for 12 bits */
        if (force_baseline && temp > 255L)
        {
            temp = 255L;    /* limit to baseline range if requested */
        }

        info->qMatTab[AC_TABLE_INDEX0][i] = (uint8_t)temp;
    }

    //setting of qmatrix table information
//#define USE_CNM_DEFAULT_QMAT_TABLE
#ifdef USE_CNM_DEFAULT_QMAT_TABLE
    memset(&pop->qMatTab[DC_TABLE_INDEX0], 0x00, 64);
    memcpy(&pop->qMatTab[DC_TABLE_INDEX0], lumaQ2, 64);

    memset(&pop->qMatTab[AC_TABLE_INDEX0], 0x00, 64);
    memcpy(&pop->qMatTab[AC_TABLE_INDEX0], chromaBQ2, 64);
#endif

    memcpy(&info->qMatTab[DC_TABLE_INDEX1], &info->qMatTab[DC_TABLE_INDEX0], 64);
    memcpy(&info->qMatTab[AC_TABLE_INDEX1], &info->qMatTab[AC_TABLE_INDEX0], 64);
}


typedef struct
{
    uint8_t *ptr;
    uint32_t sz;
} J_BUFF_t;

static J_BUFF_t jcache_buffer[JPU_BUFF_NUM_MAX];

static void *jpu_malloc(size_t size)
{
    void *ptr = NULL;
    uint8_t i = 0;

    if (jpu_heap_available)
    {
        ptr = JPU_MALLOC(size);
        for (i = 0; i < JPU_BUFF_NUM_MAX; i++)
        {
            if (!jcache_buffer[i].ptr)
            {
                jcache_buffer[i].ptr = (uint8_t *)ptr;
                jcache_buffer[i].sz = size;
                break;
            }
        }
        if (i == JPU_BUFF_NUM_MAX)
        {
            DBG_DIRECT("JPU malloc assert: Buffer num not enough!");
            while (1);
        }
        // DBG_DIRECT("JPU malloc: 0x%x %d", ptr, size);
    }

    return ptr;
}


static void jpu_free(void *ptr, bool need_free)
{
    uint8_t i = 0;

    for (i = 0; i < JPU_BUFF_NUM_MAX; i++)
    {
        // DBG_DIRECT("--jpg free align ck i:%d 0x%x %d sz: %d", i, jcache_buffer[i].ptr, jcache_buffer[i].align,
        //                jcache_buffer[i].sz);
        if ((ptr == jcache_buffer[i].ptr) || (((uint8_t *)ptr > jcache_buffer[i].ptr) &&
                                              ((uint8_t *)ptr < jcache_buffer[i].ptr + jcache_buffer[i].sz)))
        {
            // DBG_DIRECT("---->jpg free i:%d 0x%x sz: %d", i, jcache_buffer[i].ptr, jcache_buffer[i].sz);
            ptr = (void *)jcache_buffer[i].ptr;
            jcache_buffer[i].ptr = 0;
            jcache_buffer[i].sz = 0;
            break;
        }
    }

    if (i < JPU_BUFF_NUM_MAX && need_free)
    {
        JPU_FREE(ptr);
    }
}


/*============================================================================*
 *                           Public Functions
 *============================================================================*/

void *hal_jpu_get_raw_buffer(void *ptr)
{
    uint8_t i = 0;
    for (i = 0; i < JPU_BUFF_NUM_MAX; i++)
    {
        // DBG_DIRECT("--jpg free align ck i:%d 0x%x %d sz: %d", i, jcache_buffer[i].ptr, jcache_buffer[i].align,
        //                jcache_buffer[i].sz);
        if ((ptr == jcache_buffer[i].ptr) || (((uint8_t *)ptr > jcache_buffer[i].ptr) &&
                                              ((uint8_t *)ptr < jcache_buffer[i].ptr + jcache_buffer[i].sz)))
        {
            // DBG_DIRECT("---->buf align i:%d 0x%x  sz: %d", i, jcache_buffer[i].ptr, jcache_buffer[i].sz);
            ptr = (void *)jcache_buffer[i].ptr;
            return ptr;
        }
    }
    return NULL;
}


#if 0
{
    JPU_DEC_PARAM param;
    uint8_t *output = NULL;


    memset((void *)&param, 0, sizeof(param));
    param.data = (uint8_t *)pic;
    param.size = sizeof(pic) / sizeof(pic[0]);

    // Packed stream format output [0](PLANAR) [1](YUYV) [2](UYVY) [3](YVYU) [4](VYUY) [5](YUV_444 PACKED)
    param.packedFormat = PACKED_FORMAT_422_YUYV;

    param.roiEnable = 0;
    {
        param.roiOffsetX = 48;
        param.roiOffsetY = 32;
        param.roiWidth  = 48;
        param.roiHeight = 48;
    }

    // Wrapper enable: 0-OFF, 1-ON
    param.useWrapper = 1;
    //  0-JPG_ARGB8888, 1-JPG_RGB888, 2-JPG_RGB565
    param.rgbType = 0;
    // ARGB: A(8 bit)
    param.opacity = 0xa5;

    hal_jpu_decode(&param, &output);
}
#endif

// decode jpg
void hal_jpu_add_dec_header(uint32_t size)
{
    header_sz = size;
}

void hal_jpu_clean_buffer(void *ptr)
{
    jpu_free(ptr, false);
}

void hal_jpu_free_cache(void *ptr)
{
    if (ptr && jpu_heap_available)
    {
        // DBG_DIRECT("JPU free 0x%x", ptr);
        jpu_free(ptr, true);
    }
}



static void log_dec_param(JPU_DEC_PARAM_INT *param)
{
    DBG_DIRECT("log_param\n");
    DBG_DIRECT("data 0x%x %d -> format %d\n", param->data, param->size, param->packedFormat);
    DBG_DIRECT("useWrapper %d, rgbType %d, opa 0x%x\n", param->useWrapper, param->rgbType,
               param->opacity);
    DBG_DIRECT("iHorScaleMode %d, iVerScaleMode %d\n", param->iHorScaleMode, param->iVerScaleMode);
    DBG_DIRECT("useRot %d, rotAngle %d, mirDir %d\n", param->useRot, param->rotAngle, param->mirDir);
    DBG_DIRECT("roiEnable %d, roiOffsetX %d, roiOffsetY %d, roiWidth %d, roiHeight %d\n",
               param->roiEnable, param->roiOffsetX, param->roiOffsetY, param->roiWidth, param->roiHeight);

}

static void log_dec_info(JPU_DEC_INFO *info)
{
    DBG_DIRECT("JPU_Dec_Header\n");
    DBG_DIRECT("w %d h %d alignW %d, alignH %d\n", info->picWidth, info->picHeight, info->alignedWidth,
               info->alignedHeight);
    DBG_DIRECT("format %d compNum %d \n", info->format, info->compNum);
    DBG_DIRECT("busReqNum %d mcu w %d, h %d, mcuBlockNum %d \n", info->busReqNum, info->mcuWidth,
               info->mcuHeight, info->mcuBlockNum);
    DBG_DIRECT("compInfo: 0x%x  0x%x  0x%x\n", info->compInfo[0], info->compInfo[1], info->compInfo[2]);

    DBG_DIRECT("ecsPtr 0x%x pagePtr 0x%x wordPtr 0x%x, bitPtr 0x%x\n", info->ecsPtr, info->pagePtr,
               info->wordPtr, info->bitPtr);

    DBG_DIRECT("userHuffTab: %d, huffDcIdx %d, huffAcIdx  %d , Qidx  %d \n", info->userHuffTab,
               info->huffDcIdx, info->huffAcIdx, info->Qidx);

#if 0
    DBG_DIRECT("huffVal\n");
    uint8_t *pval = &(info->huffVal[0][0]);
    for (uint32_t i = 0; i < 4 * 162; i++)
    {
        DBG_DIRECT("0x%x  ", *pval);
        pval++;
    }
    DBG_DIRECT("\n");


    DBG_DIRECT("huffBits\n");
    pval = &(info->huffBits[0][0]);
    for (uint32_t i = 0; i < 4 * 256; i++)
    {
        DBG_DIRECT("0x%x  ", *pval);
        pval++;
    }
    DBG_DIRECT("\n");

    DBG_DIRECT("cInfoTab\n");
    pval = &(info->cInfoTab[0][0]);
    for (uint32_t i = 0; i < 4 * 6; i++)
    {
        DBG_DIRECT("%d ", *pval);
        pval++;
    }
    DBG_DIRECT("\n");

    DBG_DIRECT("qMatTab\n");
    pval = (uint8_t *) & (info->qMatTab[0][0]);
    for (uint32_t i = 0; i < 4 * 64; i++)
    {
        DBG_DIRECT("%d ", *pval);
        pval++;
    }
    DBG_DIRECT("\n");

    DBG_DIRECT("huffMin\n");
    pval = (uint8_t *) & (info->huffMin[0][0]);
    for (uint32_t i = 0; i < 4 * 16; i++)
    {
        DBG_DIRECT("%d ", *pval);
        pval++;
    }
    DBG_DIRECT("\n");

    DBG_DIRECT("huffMax\n");
    pval = (uint8_t *) & (info->huffMax[0][0]);
    for (uint32_t i = 0; i < 4 * 16; i++)
    {
        DBG_DIRECT("%d ", *pval);
        pval++;
    }
    DBG_DIRECT("\n");

    DBG_DIRECT("huffPtr\n");
    pval = (uint8_t *) & (info->huffPtr[0][0]);
    for (uint32_t i = 0; i < 4 * 16; i++)
    {
        DBG_DIRECT("%d ", *pval);
        pval++;
    }
#endif
    DBG_DIRECT("\n");
}

JPU_ERROR hal_jpu_decode(JPU_DEC_PARAM *dec_param, uint8_t **frame_buff, uint32_t *frame_size,
                         uint32_t *dec_w, uint32_t *dec_h)
{
    JPU_DEC_PARAM_INT g_dec_param;
    JPU_DEC_INFO g_dec_info;
    JPU_DEC_CFG g_dec_config;
    JPU_ERROR err = JPU_ERR_FAILURE;
    int ret = -1;

    // JPU_DEC_PARAM_INT *param = JPU_MALLOC(sizeof(JPU_DEC_PARAM_INT));
    // JPU_DEC_INFO *info = JPU_MALLOC(sizeof(JPU_DEC_INFO));

    JPU_DEC_PARAM_INT *param = &g_dec_param;
    JPU_DEC_INFO *info = &g_dec_info;
    JPU_DEC_CFG *config = &g_dec_config;

    // check input validate
    if (!dec_param || !dec_param->data || ((uint32_t)dec_param->data & 0x7) || !dec_param->size ||
        !frame_buff || !frame_size || !dec_w || !dec_h)
    {
        return JPU_ERR_INVALID_PARAM;
    }

    memset((void *)param, 0, sizeof(JPU_DEC_PARAM_INT));
    memset((void *)info, 0, sizeof(JPU_DEC_INFO));
    memset((void *)config, 0, sizeof(JPU_DEC_CFG));

    err = JPU_Dec_Prepare(dec_param, param);
    if (err != JPU_SUCCESS)
    {
        DBG_DIRECT("JPU_Dec_Prepare failed %d \n", err);
        return err;
    }
    // log_dec_param(param);

    // decode header
    ret = JPU_Dec_Header(param, info);
    if (ret <= 0)
    {
        DBG_DIRECT("JPU_Dec_Header failed %d \n", ret);
        return JPU_ERR_FAILURE;
    }
    // log_dec_info(info);

    // reset JPU reg
    JPU_Clk_Init();
    JPU_Reset();

    // alloc frame buffer
    uint32_t fb_width = info->alignedWidth;
    uint32_t fb_height = info->alignedHeight;
    uint32_t alignedRoiWidth = param->roiWidth / info->mcuWidth * info->mcuWidth;
    uint32_t alignedRoiHeight = param->roiHeight / info->mcuHeight * info->mcuHeight;

    // roi & scaler
    if (param->roiEnable)
    {
        fb_width  = alignedRoiWidth;
        fb_height = alignedRoiHeight;
    }
    else
    {
        if (fb_width < 128 || fb_height < 128)
        {
            param->iHorScaleMode = 0;
            param->iVerScaleMode = 0;
        }
        fb_width  >>= param->iHorScaleMode;
        fb_height >>= param->iVerScaleMode;
    }

    // rotate
    if (param->rotAngle == 90 || param->rotAngle == 270)
    {
        uint32_t temp = fb_width;
        fb_width = fb_height;
        fb_height = temp;
    }

    if (param->iHorScaleMode || param->iVerScaleMode)
    {
        fb_width = ((fb_width + 15) >> 4) << 4;    // 16
    }

    uint8_t wrapper_factor = 0;
    switch (param->rgbType)
    {
    case JPU_RGB565:
        wrapper_factor = 2;
        break;
    case JPU_RGB888:
        wrapper_factor = 3;
        break;
    case JPU_ARGB8888:
        wrapper_factor = 4;
        break;
    }

    uint32_t packed_factor = 1;
    if (param->packedFormat >= PACKED_FORMAT_422_YUYV && param->packedFormat <= PACKED_FORMAT_422_VYUY)
    {
        packed_factor = 2;
    }
    else if (param->packedFormat == PACKED_FORMAT_444)
    {
        packed_factor = 3;
    }

    wrapper_factor = param->useWrapper ? wrapper_factor : 1;

    uint32_t stride_factor = (wrapper_factor > packed_factor) ? wrapper_factor : packed_factor;
    fb_width *= stride_factor;

    uint32_t fb_size = 0;
    uint32_t lum_size = fb_width * fb_height;
    uint32_t chr_size = 0;
    if (param->packedFormat == PACKED_FORMAT_NONE)
    {
        switch (info->format)
        {
        case JPU_FORMAT_420:
            chr_size = lum_size / 4;
            break;
        case JPU_FORMAT_422:
        case JPU_FORMAT_224:
            chr_size = lum_size / 2 ;
            break;
        case JPU_FORMAT_444:
            chr_size = lum_size;
            break;
        case JPU_FORMAT_400:
            chr_size = 0;
            break;
        }
        fb_size = lum_size + chr_size * 2;
    }
    else
    {
        fb_size = fb_width * fb_height;
    }

    uint8_t *fb_raw = (uint8_t *)jpu_malloc(fb_size + 7 + header_sz);
    if (!fb_raw)
    {
        DBG_DIRECT("fb malloc failed, size %d\n", fb_size + 7 + header_sz);
        return JPU_ERR_MALLOC_FAIL;
    }
    uint8_t *fb = (uint8_t *)((((uint32_t)fb_raw + 7 + header_sz) >> 3) << 3);  // align to 8
    // DBG_DIRECT("fb 0x%x fb_raw 0x%x, *output 0x%x, sz %d \n", fb, fb_raw, *output, fb_size);


    // rotate & mirror
    uint32_t rotMir = 0;
    uint32_t mirrorEnable = (param->mirDir != MIRDIR_NONE);
    if (param->useRot)
    {
        rotMir |= 0x10; // Enable rotator
        switch (param->rotAngle)
        {
        case 0:
            rotMir |= 0x0;
            break;

        case 90:
            rotMir |= 0x1;
            break;

        case 180:
            rotMir |= 0x2;
            break;

        case 270:
            rotMir |= 0x3;
            break;
        }
    }

    if (mirrorEnable)
    {
        rotMir |= 0x10; // Enable rotator
        switch (param->mirDir)
        {
        case MIRDIR_NONE :
            rotMir |= 0x0;
            break;

        case MIRDIR_VER :
            rotMir |= 0x4;
            break;

        case MIRDIR_HOR :
            rotMir |= 0x8;
            break;

        case MIRDIR_HOR_VER :
            rotMir |= 0xc;
            break;
        }
    }

    // construct config
    // JPU_DEC_CFG config;
    config->bs_buffer_start = param->data;
    config->bs_rd_ptr = param->data;
    config->bs_wr_ptr = param->data + param->size;
    config->bs_buffer_end = param->data + (((param->size + 1023) >> 10) << 10);
    config->strm_end_flg = 1;

    // huff table
    config->huffDcIdx = info->huffDcIdx;
    config->huffAcIdx = info->huffAcIdx;
    config->userHuffTab = info->userHuffTab;
    memcpy(config->huffMin, info->huffMin, sizeof(config->huffMin));
    memcpy(config->huffMax, info->huffMax, sizeof(config->huffMax));
    memcpy(config->huffPtr, info->huffPtr, sizeof(config->huffPtr));
    memcpy(config->huffVal, info->huffVal, sizeof(config->huffVal));
    memcpy(config->huffBits, info->huffBits, sizeof(config->huffBits));
    memcpy(config->cInfoTab, info->cInfoTab, sizeof(config->cInfoTab));
    memcpy(config->qMatTab, info->qMatTab, sizeof(config->qMatTab));

    config->ecsPtr = info->ecsPtr;
    config->pagePtr = info->pagePtr;
    config->wordPtr = info->wordPtr;
    config->bitPtr = info->bitPtr;

    config->format = info->format;            // decode from header
    config->alignedWidth = info->alignedWidth;
    config->alignedHeight = info->alignedHeight;
    config->stride = fb_width;

    config->compNum = info->compNum;
    config->mcuBlockNum = info->mcuBlockNum;
    memcpy(config->compInfo, info->compInfo, sizeof(config->compInfo));
    config->busReqNum = info->busReqNum;
    config->rstIntval = info->rstIntval;

    config->req_packed_format = param->packedFormat;   // request output format by user
    config->streamEndian = param->StreamEndian;
    config->frameEndian = param->FrameEndian;
    config->chromaInterleave = param->chromaInterleave;


    // roi
    config->roiEnable = param->roiEnable;
    config->roiOffsetX = param->roiOffsetX / info->mcuWidth * info->mcuWidth;
    config->roiOffsetY = param->roiOffsetY / info->mcuHeight * info->mcuHeight;
    config->roiMcuWidth = (param->roiOffsetX + param->roiWidth + info->mcuWidth - 1) / info->mcuWidth *
                          info->mcuWidth - config->roiOffsetX;
    config->roiMcuHeight = (param->roiOffsetY + param->roiHeight + info->mcuHeight - 1) /
                           info->mcuHeight *
                           info->mcuHeight - config->roiOffsetY;
    config->mcuWidth = info->mcuWidth;
    config->mcuHeight = info->mcuHeight;


    // scaler
    config->iHorScaleFactor = param->iHorScaleMode;
    config->iVerScaleFactor = param->iVerScaleMode;

    // rotator, mirror
    config->rotationEnable = param->useRot;
    config->rotMir = rotMir;
    config->rotatorStride = fb_width;

    // frame buffer
    config->bufY = (uint32_t)fb ;
    if (param->packedFormat == PACKED_FORMAT_NONE)
    {
        config->bufCb = (uint32_t)fb + lum_size;
        config->bufCr = (uint32_t)fb + lum_size + chr_size;
    }

    // wrapper
    config->wrapper_en = param->useWrapper;
    config->rgb_type = param->rgbType;
    config->opa = param->opacity;

    // output info
    JPU_OUTPUT_INFO *poutput_info = hal_jpu_get_info();
    memset((void *)poutput_info, 0, sizeof(JPU_OUTPUT_INFO));
    poutput_info->picWidth = info->picWidth;
    poutput_info->picHeight = info->picHeight;
    poutput_info->alignedWidth = info->alignedWidth;
    poutput_info->alignedHeight = info->alignedHeight;
    poutput_info->buff_raw = fb_raw;
    poutput_info->buff = fb;
    poutput_info->buff_size = fb_size + 7 + header_sz;


    // start decode
    ret = JPU_Decode(config);

    JPU_HW_INFO *hw_info = JPU_Read_Output();
    poutput_info->state = hw_info->status.d32;
    poutput_info->cycle = hw_info->cycle;

    if (!ret)
    {
        DBG_DIRECT("JPU Decode error: 0x%x\n", hw_info->status.d32);
        DBG_DIRECT("JPU Decode error: mcu_x %d mcu_y %d\n", hw_info->dec_errmb.b.err_mcu_x,
                   hw_info->dec_errmb.b.err_mcu_y);
        poutput_info->err_mcu_x = hw_info->dec_errmb.b.err_mcu_x;
        poutput_info->err_mcu_y = hw_info->dec_errmb.b.err_mcu_y;
        hal_jpu_free_cache(fb_raw);
        return JPU_ERR_DECODE;
    }

    *frame_buff = fb - header_sz;
    *frame_size = fb_size;
    *dec_w = info->alignedWidth;
    *dec_h = info->alignedHeight;

    // DBG_DIRECT("INT_JPU_DONE cnt %d\n", hw_info->cycle);

    return JPU_SUCCESS;
}




static void log_enc_param(JPU_ENC_PARAM *param)
{
    DBG_DIRECT("log_param\n");
    DBG_DIRECT("data 0x%x %d -> format %d -> %d\n", param->data, param->size, param->frameFormat,
               param->jpgFormat);
    DBG_DIRECT("useWrapper %d, rgbType %d\n", param->useWrapper, param->rgbType);
    DBG_DIRECT("jpg_buff_sz %d, quality %d\n", param->jpg_buff_sz, param->quality);

}

JPU_ERROR hal_jpu_encode(JPU_ENC_PARAM *enc_param, uint8_t **jpg_buff, uint32_t *jpg_size,
                         uint32_t *enc_w, uint32_t *enc_h)
{
    JPU_ENC_PARAM_INT g_enc_param;
    JPU_ENC_INFO g_enc_info;
    // JPU_ENC_CFG g_enc_config;

    JPU_ERROR err = JPU_ERR_FAILURE;
    int ret = -1;
    uint32_t i = 0;

    JPU_ENC_PARAM_INT *param = &g_enc_param;
    JPU_ENC_INFO *info = &g_enc_info;
    // JPU_ENC_CFG *config = &g_enc_config;
    JPU_ENC_CFG *config = NULL;

    if (!jpu_heap_available)
    {
        return JPU_ERR_HEAP_NOT_AVAILABLE;
    }

    // check input validate
    if (!enc_param || !enc_param->data || ((uint32_t)enc_param->data & 0x7) || !enc_param->size ||
        !jpg_buff || !jpg_size || !enc_w || !enc_h)
    {
        return JPU_ERR_INVALID_PARAM;
    }

    memset((void *)param, 0, sizeof(JPU_ENC_PARAM_INT));
    memset((void *)info, 0, sizeof(JPU_ENC_INFO));
    err = JPU_Enc_Prepare(enc_param, param);
    if (err != JPU_SUCCESS)
    {
        DBG_DIRECT("JPU_Enc_Prepare failed %d \n", err);
        return err;
    }
    // log_enc_param(param);

    // reset JPU reg
    JPU_Clk_Init();
    JPU_Reset();

    uint8_t *jpg_buf_raw = (uint8_t *)jpu_malloc(param->buff_sz + 7);
    if (!jpg_buf_raw)
    {
        DBG_DIRECT("jpg_buf malloc failed, size %d\n", param->buff_sz + 7);
        return JPU_ERR_MALLOC_FAIL;
    }
    uint8_t *jpg_buf = (uint8_t *)((((uint32_t)jpg_buf_raw + 7) >> 3) << 3);  // align to 8
    // DBG_DIRECT("jpg_buf 0x%x jpg_buf_raw 0x%x, sz %d \n", jpg_buf, jpg_buf_raw, param->buff_sz);

    getJpuEncOpenParamDefault(info);

    // quality set quant table
    JpuEncSetupTables(info, param->quality);


    // rotate
    if (param->rotAngle == ROT_ANGLE_90 || param->rotAngle == ROT_ANGLE_270)
    {
        param->jpgFormat = (param->jpgFormat == JPU_FORMAT_422) ? JPU_FORMAT_224 :
                           (param->jpgFormat == JPU_FORMAT_224) ? JPU_FORMAT_422 : param->jpgFormat;
    }

    if (param->usePartialMode) {}

    // image size
    if (param->jpgFormat == JPU_FORMAT_420 || param->jpgFormat == JPU_FORMAT_422)
    {
        info->alignedWidth = ((param->picWidth + 15) >> 4) << 4;
    }
    else
    {
        info->alignedWidth = ((param->picWidth + 7) >> 3) << 3;
    }

    if (param->jpgFormat == JPU_FORMAT_420 || param->jpgFormat == JPU_FORMAT_224)
    {
        info->alignedHeight = ((param->picHeight + 15) >> 4) << 4;
    }
    else
    {
        info->alignedHeight = ((param->picHeight + 7) >> 3) << 3;
    }


    // comp info
    switch (param->jpgFormat)
    {
    case JPU_FORMAT_420:
        {
            info->compNum = 3;
            info->mcuBlockNum = 6;
            info->busReqNum = 2;
            info->compInfo[0] = 10;
            info->compInfo[1] = 5;
            info->compInfo[2] = 5;
        }
        break;
    case JPU_FORMAT_422:
        {
            info->compNum = 3;
            info->mcuBlockNum = 4;
            info->busReqNum = 3;
            info->compInfo[0] = 9;
            info->compInfo[1] = 5;
            info->compInfo[2] = 5;
        }
        break;
    case JPU_FORMAT_224:
        {
            info->compNum = 3;
            info->mcuBlockNum = 4;
            info->busReqNum  = 3;
            info->compInfo[0] = 6;
            info->compInfo[1] = 5;
            info->compInfo[2] = 5;
        }
        break;
    case JPU_FORMAT_444:
        {
            info->compNum = 3;
            info->mcuBlockNum = 3;
            info->busReqNum = 4;
            info->compInfo[0] = 5;
            info->compInfo[1] = 5;
            info->compInfo[2] = 5;
        }
        break;
    case JPU_FORMAT_400:
        {
            info->compNum = 1;
            info->mcuBlockNum = 1;
            info->busReqNum = 4;
            info->compInfo[0] = 5;
            info->compInfo[1] = 0;
            info->compInfo[2] = 0;
        }
        break;
    }



    // frame buffer
    uint8_t wrapper_factor = 1;
    switch (param->rgbType)
    {
    case JPU_RGB565:
        wrapper_factor = 2;
        break;
    case JPU_RGB888:
        wrapper_factor = 3;
        break;
    case JPU_ARGB8888:
        wrapper_factor = 4;
        break;
    }

    uint32_t packed_factor = 1;
    if (param->frameFormat >= PACKED_FORMAT_422_YUYV && param->frameFormat <= PACKED_FORMAT_422_VYUY)
    {
        packed_factor = 2;
    }
    else if (param->frameFormat == PACKED_FORMAT_444)
    {
        packed_factor = 3;
    }

    wrapper_factor = param->useWrapper ? wrapper_factor : 1;

    uint32_t stride_factor = (wrapper_factor > packed_factor) ? wrapper_factor : packed_factor;

    // expected buff size
    uint32_t fb_width = info->alignedWidth;
    uint32_t fb_height = info->alignedHeight;
    uint32_t fb_size = 0;
    uint32_t lum_size = info->alignedWidth * info->alignedHeight;
    uint32_t chr_size = 0;
    if (param->frameFormat == PACKED_FORMAT_NONE)
    {
        switch (param->jpgFormat)
        {
        case JPU_FORMAT_420:
            chr_size = lum_size / 4;
            break;
        case JPU_FORMAT_422:
        case JPU_FORMAT_224:
            chr_size = lum_size / 2 ;
            break;
        case JPU_FORMAT_444:
            chr_size = lum_size;
            break;
        case JPU_FORMAT_400:
            chr_size = 0;
            break;
        }
        fb_size = lum_size + chr_size * 2;
    }
    else
    {
        fb_width *= stride_factor;
        fb_size = fb_width * fb_height;
    }

    uint8_t *enc_fb_buf_raw = NULL;
    uint8_t *enc_fb_buf = NULL;
    if (info->alignedWidth == param->picWidth)
    {
        info->bufY = (uint32_t)param->data;
        if (param->frameFormat == PACKED_FORMAT_NONE)
        {
            info->bufCb = (uint32_t)param->data + lum_size;
            info->bufCr = (uint32_t)param->data + lum_size + chr_size;
        }
    }
    else
    {
        enc_fb_buf_raw = (uint8_t *)jpu_malloc(fb_size + 7);
        if (!enc_fb_buf_raw)
        {
            DBG_DIRECT("jpg_buf malloc failed, size %d\n", fb_size + 7);
            return JPU_ERR_MALLOC_FAIL;
        }
        enc_fb_buf = (uint8_t *)((((uint32_t)enc_fb_buf_raw + 7) >> 3) << 3);  // align to 8
        // DBG_DIRECT("enc_fb_buf 0x%x enc_fb_buf_raw 0x%x, sz %d \n", enc_fb_buf, enc_fb_buf_raw, fb_size);

        // copy frame to fb
        for (i = 0; i < param->picHeight; ++i)
        {
            memcpy(enc_fb_buf + fb_width * i,
                   (uint8_t *)(param->data + i * param->picWidth * stride_factor),
                   param->picWidth * stride_factor);
        }

        if (param->jpgFormat == JPU_FORMAT_400 || param->frameFormat != PACKED_FORMAT_NONE)
        {
            // copy done
        }
        else
        {
            // plannar with Cr Cb
            uint32_t nCb = param->picHeight;
            uint32_t nCr = param->picHeight;
            uint32_t chromaSize = param->picWidth * param->picHeight;
            uint32_t chromaStride = fb_width;
            uint32_t chromaWidth = param->picWidth;

            switch (param->jpgFormat)
            {
            case JPU_FORMAT_420:
                nCb = nCr = param->picHeight / 2;
                chromaSize = param->picWidth * param->picHeight / 4;
                chromaStride = fb_width / 2;
                chromaWidth = param->picWidth / 2;
                break;
            case JPU_FORMAT_224:
                nCb = nCr = param->picHeight / 2;
                chromaSize = param->picWidth * param->picHeight / 2;
                chromaStride = fb_width;
                chromaWidth = param->picWidth;
                break;
            case JPU_FORMAT_422:
                nCb = nCr = param->picHeight;
                chromaSize = param->picWidth * param->picHeight / 2;
                chromaStride = fb_width / 2;
                chromaWidth = param->picWidth / 2;
                break;
            case JPU_FORMAT_444:
                nCb = nCr = param->picHeight;
                chromaSize = param->picWidth * param->picHeight;
                chromaStride = fb_width;
                chromaWidth = param->picWidth;
                break;
            }

            uint32_t puc = (uint32_t)(param->data + param->picWidth * param->picHeight);
            uint32_t addr = (uint32_t)(enc_fb_buf + lum_size);
            for (i = 0; i < nCb; ++i)
            {
                memcpy((void *)(addr + chromaStride * i), (uint8_t *)(puc + i * chromaWidth), chromaWidth);
            }

            puc = puc + chromaSize;
            addr = (uint32_t)(enc_fb_buf + lum_size + chr_size);
            for (i = 0; i < nCr; ++i)
            {
                memcpy((void *)(addr + chromaStride * i), (uint8_t *)(puc + i * chromaWidth), chromaWidth);
            }
        }

        info->bufY = (uint32_t)enc_fb_buf;
        if (param->frameFormat == PACKED_FORMAT_NONE)
        {
            info->bufCb = (uint32_t)enc_fb_buf + lum_size;
            info->bufCr = (uint32_t)enc_fb_buf + lum_size + chr_size;
        }
    }

    uint32_t rotMirEnable = 0;
    uint32_t rotMirMode = 0;
    if (param->useRot)
    {
        rotMirEnable = 0x10; // Enable rotator
        switch (param->rotAngle)
        {
        case 0:
            rotMirMode |= 0x0;
            break;

        case 90:
            rotMirMode |= 0x1;
            break;

        case 180:
            rotMirMode |= 0x2;
            break;

        case 270:
            rotMirMode |= 0x3;
            break;
        }

        switch (param->mirDir)
        {
        case MIRDIR_NONE :
            rotMirMode |= 0x0;
            break;

        case MIRDIR_VER :
            rotMirMode |= 0x4;
            break;

        case MIRDIR_HOR :
            rotMirMode |= 0x8;
            break;

        case MIRDIR_HOR_VER :
            rotMirMode |= 0xc;
            break;

        }
    }


    // construct config
    config = jpu_malloc(sizeof(JPU_ENC_CFG));
    if (!config)
    {
        return JPU_ERR_MALLOC_FAIL;
    }
    memset(config, 0, sizeof(JPU_ENC_CFG));

    config->bs_buffer_start = jpg_buf;
    config->bs_rd_ptr = jpg_buf;
    config->bs_wr_ptr = jpg_buf;
    config->bs_buffer_end = jpg_buf + param->buff_sz;

    // huff table
    for (i = 0; i < 4; i++)
    {
        config->pHuffVal[i] = info->huffVal[i];
    }
    for (i = 0; i < 4; i++)
    {
        config->pHuffBits[i] = info->huffBits[i];
    }
    for (i = 0; i < 4; i++)
    {
        config->pQMatTab[i] = info->qMatTab[i];
    }

    config->picWidth = param->picWidth;
    config->picHeight = param->picHeight;
    config->alignedWidth = info->alignedWidth;
    config->alignedHeight = info->alignedHeight;
    config->stride = fb_width;
    config->format = param->jpgFormat;
    config->frame_packed_format = param->frameFormat;
    config->streamEndian = param->StreamEndian;
    config->frameEndian = param->FrameEndian;
    config->chromaInterleave = param->chromaInterleave;
    config->rstIntval = info->rstIntval;

    config->compNum = info->compNum;
    config->mcuBlockNum = info->mcuBlockNum;
    config->busReqNum = info->busReqNum;
    config->compInfo[0] = info->compInfo[0];
    config->compInfo[1] = info->compInfo[1];
    config->compInfo[2] = info->compInfo[2];

    // wrapper
    config->wrapper_en = param->useWrapper;
    config->rgb_type = param->rgbType;

    // frame buffer
    config->bufY =  info->bufY;
    config->bufCb = info->bufCb;
    config->bufCr = info->bufCr;

    config->stuffByteEnable = param->bEnStuffByte;

    config->size = param->buff_sz;
    config->headerMode =
        ENC_HEADER_MODE_NORMAL;         //Encoder header disable/enable control. Annex:A 1.2.3 item 13
    config->quantMode =
        JPG_TBL_NORMAL; //JPG_TBL_MERGE  // Merge quantization table. Annex:A 1.2.3 item 7
    config->huffMode  = JPG_TBL_NORMAL; // JPG_TBL_MERGE //Merge huffman table. Annex:A 1.2.3 item 6
    config->disableAPPMarker = 0;                        //Remove APPn. Annex:A item 11

    // rotation & mirror
    config->rotMirEnable = rotMirEnable;
    config->rotMirMode = rotMirMode;
    config->rotationAngle = param->rotAngle;

    // expected encode size
    JPU_OUTPUT_INFO *poutput_info = hal_jpu_get_info();
    memset((void *)poutput_info, 0, sizeof(JPU_OUTPUT_INFO));
    poutput_info->picWidth = param->picWidth;
    poutput_info->picHeight = param->picHeight;
    poutput_info->alignedWidth = info->alignedWidth;
    poutput_info->alignedHeight = info->alignedHeight;
    poutput_info->buff_raw = jpg_buf_raw;
    poutput_info->buff = jpg_buf;
    poutput_info->buff_size = param->buff_sz + 7;
    poutput_info->jpg_format = param->jpgFormat;



    // start encode
    ret = JPU_Encode(config);
    hal_jpu_free_cache(enc_fb_buf_raw);
    hal_jpu_free_cache(config);

    JPU_HW_INFO *hw_info = JPU_Read_Output();
    poutput_info->state = hw_info->status.d32;
    poutput_info->cycle = hw_info->cycle;

    if (!ret)
    {
        DBG_DIRECT("JPU Enc error: 0x%x\n", hw_info->status.d32);
        hal_jpu_free_cache(jpg_buf_raw);
        if (poutput_info->state & JPU_STATUS_BBC_INT)
        {
            return JPU_ERR_BBC_INT;
        }
        return JPU_ERR_ENCODE;
    }

    *jpg_buff = jpg_buf;
    *jpg_size = hw_info->strmWrPtr - (uint32_t)jpg_buf;
    *enc_w = info->alignedWidth;
    *enc_h = info->alignedHeight;

    // DBG_DIRECT("INT_JPU_DONE cycle %d\n", hw_info->cycle);
    // DBG_DIRECT("0x%x -> 0x%x, %d\n", jpg_buf, hw_info->strmWrPtr, *wr_size);

    return JPU_SUCCESS;
}


JPU_OUTPUT_INFO *hal_jpu_get_info(void)
{
    return &output_info;
}

void hal_jpu_mem_init(void *(*jmalloc)(size_t), void (*jfree)(void *))
{
    if (jmalloc && jfree)
    {
        JPU_MALLOC = jmalloc;
        JPU_FREE = jfree;
        jpu_heap_available = true;
    }
}
