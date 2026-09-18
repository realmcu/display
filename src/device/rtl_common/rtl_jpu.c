/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "stdio.h"
#include "rtl_jpu.h"
#include "trace.h"
#include "string.h"
// #include "rtl_jpu_int.h"


static unsigned char sJpuCompInfoTable[5][24] =
{
    { 00, 02, 02, 00, 00, 00, 01, 01, 01, 01, 01, 01, 02, 01, 01, 01, 01, 01, 03, 00, 00, 00, 00, 00 }, //420
    { 00, 02, 01, 00, 00, 00, 01, 01, 01, 01, 01, 01, 02, 01, 01, 01, 01, 01, 03, 00, 00, 00, 00, 00 }, //422H
    { 00, 01, 02, 00, 00, 00, 01, 01, 01, 01, 01, 01, 02, 01, 01, 01, 01, 01, 03, 00, 00, 00, 00, 00 }, //422V
    { 00, 01, 01, 00, 00, 00, 01, 01, 01, 01, 01, 01, 02, 01, 01, 01, 01, 01, 03, 00, 00, 00, 00, 00 }, //444
    { 00, 01, 01, 00, 00, 00, 01, 00, 00, 00, 00, 00, 02, 00, 00, 00, 00, 00, 03, 00, 00, 00, 00, 00 }, //400
};

/*============================================================================*
 *                           Static Functions
 *============================================================================*/


static int JPU_DecHuffTabSetUp(JPU_DEC_CFG *jpg)
{
    uint32_t i = 0, j = 0;
    int HuffData = 0;   // 16BITS
    uint32_t HuffLength;
    uint32_t temp = 0;


    // MIN Tables
    JPU_HUFF_CTRL_TypeDef jpu_reg_0x80 = {.d32 = 0};
    jpu_reg_0x80.d32 = 0x003;
    JPU->HUFF_CTRL = jpu_reg_0x80.d32;

    //DC Luma
    JPU_HUFF_DATA_TypeDef jpu_reg_0x88 = {.d32 = 0};
    for (j = 0; j < 16; j++)
    {
        HuffData = jpg->huffMin[0][j];
        temp = (HuffData & 0x8000) >> 15;
        temp = (temp << 15) | (temp << 14) | (temp << 13) | (temp << 12) | (temp << 11) | (temp << 10) |
               (temp << 9) | (temp << 8) | (temp << 7) | (temp << 6) | (temp << 5) | (temp << 4) | (temp << 3) |
               (temp << 2) | (temp << 1) | (temp) ;
        jpu_reg_0x88.d32 = (((temp & 0xFFFF) << 16) | HuffData);     // 32-bit
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }

    //DC Chroma
    for (j = 0; j < 16; j++)
    {
        HuffData = jpg->huffMin[2][j];
        temp = (HuffData & 0x8000) >> 15;
        temp = (temp << 15) | (temp << 14) | (temp << 13) | (temp << 12) | (temp << 11) | (temp << 10) |
               (temp << 9) | (temp << 8) | (temp << 7) | (temp << 6) | (temp << 5) | (temp << 4) | (temp << 3) |
               (temp << 2) | (temp << 1) | (temp) ;
        jpu_reg_0x88.d32 = (((temp & 0xFFFF) << 16) | HuffData);     // 32-bit
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }

    //AC Luma
    for (j = 0; j < 16; j++)
    {
        HuffData = jpg->huffMin[1][j];
        temp = (HuffData & 0x8000) >> 15;
        temp = (temp << 15) | (temp << 14) | (temp << 13) | (temp << 12) | (temp << 11) | (temp << 10) |
               (temp << 9) | (temp << 8) | (temp << 7) | (temp << 6) | (temp << 5) | (temp << 4) | (temp << 3) |
               (temp << 2) | (temp << 1) | (temp) ;
        jpu_reg_0x88.d32 = (((temp & 0xFFFF) << 16) | HuffData);     // 32-bit
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }

    //AC Chroma
    for (j = 0; j < 16; j++)
    {
        HuffData = jpg->huffMin[3][j];
        temp = (HuffData & 0x8000) >> 15;
        temp = (temp << 15) | (temp << 14) | (temp << 13) | (temp << 12) | (temp << 11) | (temp << 10) |
               (temp << 9) | (temp << 8) | (temp << 7) | (temp << 6) | (temp << 5) | (temp << 4) | (temp << 3) |
               (temp << 2) | (temp << 1) | (temp) ;
        jpu_reg_0x88.d32 = (((temp & 0xFFFF) << 16) | HuffData);     // 32-bit
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }
    // MAX Tables
    jpu_reg_0x80.d32 = 0x403;
    JPU->HUFF_CTRL = jpu_reg_0x80.d32;

    JPU_HUFF_ADDR_TypeDef jpu_reg_0x84 = {.d32 = 0};
    jpu_reg_0x84.d32 = 0x440;
    JPU->HUFF_ADDR = jpu_reg_0x84.d32;


    //DC Luma
    for (j = 0; j < 16; j++)
    {
        HuffData = jpg->huffMax[0][j];
        temp = (HuffData & 0x8000) >> 15;
        temp = (temp << 15) | (temp << 14) | (temp << 13) | (temp << 12) | (temp << 11) | (temp << 10) |
               (temp << 9) | (temp << 8) | (temp << 7) | (temp << 6) | (temp << 5) | (temp << 4) | (temp << 3) |
               (temp << 2) | (temp << 1) | (temp) ;
        jpu_reg_0x88.d32 = (((temp & 0xFFFF) << 16) | HuffData);     // 32-bit
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }
    //DC Chroma
    for (j = 0; j < 16; j++)
    {
        HuffData = jpg->huffMax[2][j];
        temp = (HuffData & 0x8000) >> 15;
        temp = (temp << 15) | (temp << 14) | (temp << 13) | (temp << 12) | (temp << 11) | (temp << 10) |
               (temp << 9) | (temp << 8) | (temp << 7) | (temp << 6) | (temp << 5) | (temp << 4) | (temp << 3) |
               (temp << 2) | (temp << 1) | (temp) ;
        jpu_reg_0x88.d32 = (((temp & 0xFFFF) << 16) | HuffData);     // 32-bit
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }
    //AC Luma
    for (j = 0; j < 16; j++)
    {
        HuffData = jpg->huffMax[1][j];
        temp = (HuffData & 0x8000) >> 15;
        temp = (temp << 15) | (temp << 14) | (temp << 13) | (temp << 12) | (temp << 11) | (temp << 10) |
               (temp << 9) | (temp << 8) | (temp << 7) | (temp << 6) | (temp << 5) | (temp << 4) | (temp << 3) |
               (temp << 2) | (temp << 1) | (temp) ;
        jpu_reg_0x88.d32 = (((temp & 0xFFFF) << 16) | HuffData);     // 32-bit
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }
    //AC Chroma
    for (j = 0; j < 16; j++)
    {
        HuffData = jpg->huffMax[3][j];
        temp = (HuffData & 0x8000) >> 15;
        temp = (temp << 15) | (temp << 14) | (temp << 13) | (temp << 12) | (temp << 11) | (temp << 10) |
               (temp << 9) | (temp << 8) | (temp << 7) | (temp << 6) | (temp << 5) | (temp << 4) | (temp << 3) |
               (temp << 2) | (temp << 1) | (temp) ;
        jpu_reg_0x88.d32 = (((temp & 0xFFFF) << 16) | HuffData);     // 32-bit
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }

    // PTR Tables
    jpu_reg_0x80.d32 = 0x803;
    JPU->HUFF_CTRL = jpu_reg_0x80.d32;

    jpu_reg_0x84.d32 = 0x880;
    JPU->HUFF_ADDR = jpu_reg_0x84.d32;

    //DC Luma
    for (j = 0; j < 16; j++)
    {
        HuffData = jpg->huffPtr[0][j];
        temp = (HuffData & 0x80) >> 7;
        temp = (temp << 23) | (temp << 22) | (temp << 21) | (temp << 20) | (temp << 19) | (temp << 18) |
               (temp << 17) | (temp << 16) | (temp << 15) | (temp << 14) | (temp << 13) | (temp << 12) |
               (temp << 11) | (temp << 10) | (temp << 9) | (temp << 8) | (temp << 7) | (temp << 6) |
               (temp << 5) | (temp << 4) | (temp << 3) | (temp << 2) | (temp << 1) | (temp);
        jpu_reg_0x88.d32 = (((temp & 0xFFFFFF) << 8) | HuffData);
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }
    //DC Chroma
    for (j = 0; j < 16; j++)
    {
        HuffData = jpg->huffPtr[2][j];
        temp = (HuffData & 0x80) >> 7;
        temp = (temp << 23) | (temp << 22) | (temp << 21) | (temp << 20) | (temp << 19) | (temp << 18) |
               (temp << 17) | (temp << 16) | (temp << 15) | (temp << 14) | (temp << 13) | (temp << 12) |
               (temp << 11) | (temp << 10) | (temp << 9) | (temp << 8) | (temp << 7) | (temp << 6) |
               (temp << 5) | (temp << 4) | (temp << 3) | (temp << 2) | (temp << 1) | (temp);
        jpu_reg_0x88.d32 = (((temp & 0xFFFFFF) << 8) | HuffData);
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }
    //AC Luma
    for (j = 0; j < 16; j++)
    {
        HuffData = jpg->huffPtr[1][j];
        temp = (HuffData & 0x80) >> 7;
        temp = (temp << 23) | (temp << 22) | (temp << 21) | (temp << 20) | (temp << 19) | (temp << 18) |
               (temp << 17) | (temp << 16) | (temp << 15) | (temp << 14) | (temp << 13) | (temp << 12) |
               (temp << 11) | (temp << 10) | (temp << 9) | (temp << 8) | (temp << 7) | (temp << 6) |
               (temp << 5) | (temp << 4) | (temp << 3) | (temp << 2) | (temp << 1) | (temp);
        jpu_reg_0x88.d32 = (((temp & 0xFFFFFF) << 8) | HuffData);
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }
    //AC Chroma
    for (j = 0; j < 16; j++)
    {
        HuffData = jpg->huffPtr[3][j];
        temp = (HuffData & 0x80) >> 7;
        temp = (temp << 23) | (temp << 22) | (temp << 21) | (temp << 20) | (temp << 19) | (temp << 18) |
               (temp << 17) | (temp << 16) | (temp << 15) | (temp << 14) | (temp << 13) | (temp << 12) |
               (temp << 11) | (temp << 10) | (temp << 9) | (temp << 8) | (temp << 7) | (temp << 6) |
               (temp << 5) | (temp << 4) | (temp << 3) | (temp << 2) | (temp << 1) | (temp);
        jpu_reg_0x88.d32 = (((temp & 0xFFFFFF) << 8) | HuffData);
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }

    // VAL Tables
    jpu_reg_0x80.d32 = 0xc03;
    JPU->HUFF_CTRL = jpu_reg_0x80.d32;

    // VAL DC Luma
    HuffLength = 0;
    for (i = 0; i < 12; i++)
    {
        HuffLength += jpg->huffBits[0][i];
    }

    for (i = 0; i < HuffLength; i++) // 8-bit, 12 row, 1 category (DC Luma)
    {
        HuffData = jpg->huffVal[0][i];
        temp = (HuffData & 0x80) >> 7;
        temp = (temp << 23) | (temp << 22) | (temp << 21) | (temp << 20) | (temp << 19) | (temp << 18) |
               (temp << 17) | (temp << 16) | (temp << 15) | (temp << 14) | (temp << 13) | (temp << 12) |
               (temp << 11) | (temp << 10) | (temp << 9) | (temp << 8) | (temp << 7) | (temp << 6) |
               (temp << 5) | (temp << 4) | (temp << 3) | (temp << 2) | (temp << 1) | (temp);
        jpu_reg_0x88.d32 = (((temp & 0xFFFFFF) << 8) | HuffData);
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }

    for (i = 0; i < 12 - HuffLength; i++)
    {
        jpu_reg_0x88.d32 = 0xFFFFFFFF;
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }

    // VAL DC Chroma
    HuffLength = 0;
    for (i = 0; i < 12; i++)
    {
        HuffLength += jpg->huffBits[2][i];
    }
    for (i = 0; i < HuffLength; i++) // 8-bit, 12 row, 1 category (DC Chroma)
    {
        HuffData = jpg->huffVal[2][i];
        temp = (HuffData & 0x80) >> 7;
        temp = (temp << 23) | (temp << 22) | (temp << 21) | (temp << 20) | (temp << 19) | (temp << 18) |
               (temp << 17) | (temp << 16) | (temp << 15) | (temp << 14) | (temp << 13) | (temp << 12) |
               (temp << 11) | (temp << 10) | (temp << 9) | (temp << 8) | (temp << 7) | (temp << 6) |
               (temp << 5) | (temp << 4) | (temp << 3) | (temp << 2) | (temp << 1) | (temp);
        jpu_reg_0x88.d32 = (((temp & 0xFFFFFF) << 8) | HuffData);
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }
    for (i = 0; i < 12 - HuffLength; i++)
    {
        jpu_reg_0x88.d32 = 0xFFFFFFFF;
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }

    // VAL AC Luma
    HuffLength = 0;
    for (i = 0; i < 162; i++)
    {
        HuffLength += jpg->huffBits[1][i];
    }
    for (i = 0; i < HuffLength; i++) // 8-bit, 162 row, 1 category (AC Luma)
    {
        HuffData = jpg->huffVal[1][i];
        temp = (HuffData & 0x80) >> 7;
        temp = (temp << 23) | (temp << 22) | (temp << 21) | (temp << 20) | (temp << 19) | (temp << 18) |
               (temp << 17) | (temp << 16) | (temp << 15) | (temp << 14) | (temp << 13) | (temp << 12) |
               (temp << 11) | (temp << 10) | (temp << 9) | (temp << 8) | (temp << 7) | (temp << 6) |
               (temp << 5) | (temp << 4) | (temp << 3) | (temp << 2) | (temp << 1) | (temp);
        jpu_reg_0x88.d32 = (((temp & 0xFFFFFF) << 8) | HuffData);
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }
    for (i = 0; i < 162 - HuffLength; i++)
    {
        jpu_reg_0x88.d32 = 0xFFFFFFFF;
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }

    // VAL AC Chroma
    HuffLength = 0;
    for (i = 0; i < 162; i++)
    {
        HuffLength += jpg->huffBits[3][i];
    }
    for (i = 0; i < HuffLength; i++) // 8-bit, 162 row, 1 category (AC Chroma)
    {
        HuffData = jpg->huffVal[3][i];
        temp = (HuffData & 0x80) >> 7;
        temp = (temp << 23) | (temp << 22) | (temp << 21) | (temp << 20) | (temp << 19) | (temp << 18) |
               (temp << 17) | (temp << 16) | (temp << 15) | (temp << 14) | (temp << 13) | (temp << 12) |
               (temp << 11) | (temp << 10) | (temp << 9) | (temp << 8) | (temp << 7) | (temp << 6) |
               (temp << 5) | (temp << 4) | (temp << 3) | (temp << 2) | (temp << 1) | (temp);
        jpu_reg_0x88.d32 = (((temp & 0xFFFFFF) << 8) | HuffData);
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }

    for (i = 0; i < 162 - HuffLength; i++)
    {
        jpu_reg_0x88.d32 = 0xFFFFFFFF;
        JPU->HUFF_DATA = jpu_reg_0x88.d32;
    }

    // end SerPeriHuffTab
    jpu_reg_0x80.d32 = 0x000;
    JPU->HUFF_CTRL = jpu_reg_0x80.d32;

    return 1;
}

static int JPU_DecQMatTabSetUp(JPU_DEC_CFG *jpg)
{
    uint32_t i = 0;
    uint32_t table = 0;
    uint32_t val = 0;


    // SetPeriQMatTab
    // Comp 0
    JPU_QMAT_CTRL_TypeDef jpu_reg_0x90 = {.d32 = 0};
    jpu_reg_0x90.d32 = 0x03;
    JPU->QMAT_CTRL = jpu_reg_0x90.d32;

    JPU_QMAT_DATA_TypeDef jpu_reg_0x98 = {.d32 = 0};
    table = jpg->cInfoTab[0][3];
    for (i = 0; i < 64; i++)
    {
        val = jpg->qMatTab[table][i];
        jpu_reg_0x90.d32 = val;
        JPU->QMAT_DATA = jpu_reg_0x90.d32;
    }
    jpu_reg_0x90.d32 = 0x00;
    JPU->QMAT_CTRL = jpu_reg_0x90.d32;

    // Comp 1
    jpu_reg_0x90.d32 = 0x43;
    JPU->QMAT_CTRL = jpu_reg_0x90.d32;
    table = jpg->cInfoTab[1][3];
    for (i = 0; i < 64; i++)
    {
        val = jpg->qMatTab[table][i];
        jpu_reg_0x90.d32 = val;
        JPU->QMAT_DATA = jpu_reg_0x90.d32;
    }
    jpu_reg_0x90.d32 = 0x00;
    JPU->QMAT_CTRL = jpu_reg_0x90.d32;

    // Comp 2
    jpu_reg_0x90.d32 = 0x83;
    JPU->QMAT_CTRL = jpu_reg_0x90.d32;
    table = jpg->cInfoTab[2][3];
    for (i = 0; i < 64; i++)
    {
        val = jpg->qMatTab[table][i];
        jpu_reg_0x90.d32 = val;
        JPU->QMAT_DATA = jpu_reg_0x90.d32;
    }
    jpu_reg_0x90.d32 = 0x00;
    JPU->QMAT_CTRL = jpu_reg_0x90.d32;
    return 1;
}

static void JPU_DecGramSetup(JPU_DEC_CFG *jpg)
{
    int dExtBitBufCurPos;
    int dExtBitBufBaseAddr;
    int dMibStatus;


    dMibStatus          = 1;
    dExtBitBufCurPos    = jpg->pagePtr;
    dExtBitBufBaseAddr  = (int)jpg->bs_buffer_start;

    JPU_BBC_CUR_POS_TypeDef jpu_reg_0x22c = {.d32 = 0};
    jpu_reg_0x22c.d32 = dExtBitBufCurPos;
    JPU->BBC_CUR_POS = jpu_reg_0x22c.d32;

    JPU_BBC_EXT_ADDR_TypeDef jpu_reg_0x214 = {.d32 = 0};
    jpu_reg_0x214.d32 = dExtBitBufBaseAddr + (dExtBitBufCurPos << 8);
    JPU->BBC_EXT_ADDR = jpu_reg_0x214.d32;

    JPU_BBC_INT_ADDR_TypeDef jpu_reg_0x218 = {.d32 = 0};
    jpu_reg_0x218.d32 = (dExtBitBufCurPos & 1) << 6;
    JPU->BBC_INT_ADDR = jpu_reg_0x218.d32;

    JPU_BBC_DATA_CNT_TypeDef jpu_reg_21c = {.d32 = 0};
    jpu_reg_21c.d32 = 256 / 4;
    JPU->BBC_DATA_CNT = jpu_reg_21c.d32;

    JPU_BBC_COMMAND_TypeDef jpu_reg_0x220 = {.d32 = 0};
    jpu_reg_0x220.d32 = (jpg->streamEndian << 1) | 0;
    JPU->BBC_COMMAND = jpu_reg_0x220.d32;


    JPU_BBC_BUSY_TypeDef jpu_reg_0x224;
    while (dMibStatus == 1)
    {
        jpu_reg_0x224.d32 = JPU->BBC_BUSY;
        dMibStatus = jpu_reg_0x224.d32;
    }

    dMibStatus          = 1;
    dExtBitBufCurPos    = dExtBitBufCurPos + 1;


    jpu_reg_0x22c.d32 = dExtBitBufCurPos;
    JPU->BBC_CUR_POS = jpu_reg_0x22c.d32;

    jpu_reg_0x214.d32 = dExtBitBufBaseAddr + (dExtBitBufCurPos << 8);
    JPU->BBC_EXT_ADDR = jpu_reg_0x214.d32;

    jpu_reg_0x218.d32 = (dExtBitBufCurPos & 1) << 6;
    JPU->BBC_INT_ADDR = jpu_reg_0x218.d32;

    jpu_reg_21c.d32 = 256 / 4;
    JPU->BBC_DATA_CNT = jpu_reg_21c.d32;

    jpu_reg_0x220.d32 = (jpg->streamEndian << 1) | 0;
    JPU->BBC_COMMAND = jpu_reg_0x220.d32;

    while (dMibStatus == 1)
    {
        jpu_reg_0x224.d32 = JPU->BBC_BUSY;
        dMibStatus = jpu_reg_0x224.d32;
    }

    dMibStatus          = 1;
    dExtBitBufCurPos    = dExtBitBufCurPos + 1;


    jpu_reg_0x22c.d32 = dExtBitBufCurPos;  // next unit page pointer
    JPU->BBC_CUR_POS = jpu_reg_0x22c.d32;

    JPU_BBC_CTRL_TypeDef jpu_reg_0x228 = {.d32 = 0};
    jpu_reg_0x228.d32 = (jpg->streamEndian << 1) | 1;
    JPU->BBC_CTRL = jpu_reg_0x228.d32;

    JPU_GBU_WD_PTR_TypeDef jpu_reg_0x114 = {.d32 = 0};
    jpu_reg_0x114.d32 = jpg->wordPtr;
    JPU->GBU_WD_PTR = jpu_reg_0x114.d32;

    JPU_GBU_BBSR_TypeDef jpu_reg_0x140 = {.d32 = 0};
    JPU->GBU_BBSR = jpu_reg_0x140.d32;

    JPU_GBU_BBER_TypeDef jpu_reg_0x144 = {.d32 = 0};
    jpu_reg_0x144.d32 = ((256 / 4) * 2) - 1;
    JPU->GBU_BBER = jpu_reg_0x144.d32;


    JPU_GBU_BBIR_TypeDef jpu_reg_0x148 = {.d32 = 0};
    JPU_GBU_BBHR_TypeDef jpu_reg_0x14c = {.d32 = 0};
    if (jpg->pagePtr & 1)
    {
        jpu_reg_0x148.d32 = 0;
        jpu_reg_0x14c.d32 = 0;
    }
    else
    {
        jpu_reg_0x148.d32 = 256 / 4;   // 64 * 4 byte == 32 * 8 byte
        jpu_reg_0x14c.d32 = 256 / 4;   // 64 * 4 byte == 32 * 8 byte
    }
    JPU->GBU_BBIR = jpu_reg_0x148.d32;
    JPU->GBU_BBHR = jpu_reg_0x14c.d32;

    JPU_GBU_CTRL_TypeDef jpu_reg_0x100 = {.d32 = 0};
    jpu_reg_0x100.d32 = 4;
    JPU->GBU_CTRL = jpu_reg_0x100.d32;

    JPU_GBU_FF_RPTR_TypeDef jpu_reg_0x160 = {.d32 = 0};
    jpu_reg_0x160.d32 = jpg->bitPtr;
    JPU->GBU_FF_RPTR = jpu_reg_0x160.d32;
}



static int JPU_EncGenHuffTab(JPU_ENC_CFG *pEncInfo, int tabNum)
{
    int p, i, l, lastp, si, maxsymbol;
    int code;
    uint8_t *bitleng, *huffval;
    unsigned int *ehufco, *ehufsi;

    bitleng = pEncInfo->pHuffBits[tabNum];
    huffval = pEncInfo->pHuffVal[tabNum];
    ehufco  = pEncInfo->huffCode[tabNum];
    ehufsi  = pEncInfo->huffSize[tabNum];

    memset((void *)(pEncInfo->huffSizeTmp), 0x00, sizeof(pEncInfo->huffSizeTmp));
    memset((void *)(pEncInfo->huffCodeTmp), 0x00, sizeof(pEncInfo->huffCodeTmp));

    maxsymbol = tabNum & 1 ? 256 : 16;

    /* Figure C.1: make table of Huffman code length for each symbol */

    p = 0;
    for (l = 1; l <= 16; l++)
    {
        i = bitleng[l - 1];
        if (i < 0 || p + i > maxsymbol)
        {
            return 0;
        }
        while (i--)
        {
            pEncInfo->huffSizeTmp[p++] = l;
        }
    }
    lastp = p;

    /* Figure C.2: generate the codes themselves */
    /* We also validate that the counts represent a legal Huffman code tree. */

    code = 0;
    si = pEncInfo->huffSizeTmp[0];
    p = 0;
    while (pEncInfo->huffSizeTmp[p] != 0)
    {
        while (pEncInfo->huffSizeTmp[p] == si)
        {
            pEncInfo->huffCodeTmp[p++] = code;
            code++;
        }
        if (code >= (1 << si))
        {
            return 0;
        }
        code <<= 1;
        si++;
    }

    /* Figure C.3: generate encoding tables */
    /* These are code and size indexed by symbol value */

    for (i = 0; i < 256; i++)
    {
        ehufsi[i] = 0x00;
    }

    for (i = 0; i < 256; i++)
    {
        ehufco[i] = 0x00;
    }

    for (p = 0; p < lastp; p++)
    {
        i = huffval[p];
        if (i < 0 || i >= maxsymbol || ehufsi[i])
        {
            return 0;
        }
        ehufco[i] = pEncInfo->huffCodeTmp[p];
        ehufsi[i] = pEncInfo->huffSizeTmp[p];
    }

    return 1;
}

static int JPU_EncLoadHuffTab(JPU_ENC_CFG *pEncInfo)
{
    int i, j, t;
    int huffData;

    for (i = 0; i < 4; i++)
    {
        JPU_EncGenHuffTab(pEncInfo, i);
    }

    JPU_HUFF_CTRL_TypeDef jpu_reg_0x80 = {.d32 = 0};
    jpu_reg_0x80.d32 = 0x03;
    JPU->HUFF_CTRL = jpu_reg_0x80.d32;

    JPU_HUFF_DATA_TypeDef jpu_reg_0x88 = {.d32 = 0};
    for (j = 0; j < 4; j++)
    {

        t = (j == 0) ? AC_TABLE_INDEX0 : (j == 1) ? AC_TABLE_INDEX1 : (j == 2) ? DC_TABLE_INDEX0 :
            DC_TABLE_INDEX1;

        for (i = 0; i < 256; i++)
        {
            if ((t == DC_TABLE_INDEX0 || t == DC_TABLE_INDEX1) && (i > 15)) // DC
            {
                break;
            }

            if ((pEncInfo->huffSize[t][i] == 0) && (pEncInfo->huffCode[t][i] == 0))
            {
                huffData = 0;
            }
            else
            {
                huffData = (pEncInfo->huffSize[t][i] - 1);                      // Code length (1 ~ 16), 4-bit
                huffData = (huffData << 16) | (pEncInfo->huffCode[t][i]);       // Code word, 16-bit
            }

            jpu_reg_0x88.d32 = huffData;
            JPU->HUFF_DATA = jpu_reg_0x88.d32;
        }
    }

    jpu_reg_0x80.d32 = 0x0;
    JPU->HUFF_CTRL = jpu_reg_0x80.d32;
    return 1;
}

static int JPU_EncEncodeHeader(JPU_ENC_CFG *pEncInfo)
{
    int i;
    int frameFormat;
    JPU_GBU_PBIT_08_TypeDef jpu_reg_0x120 = {.d32 = 0};
    JPU_GBU_PBIT_16_TypeDef jpu_reg_0x124 = {.d32 = 0};
    JPU_GBU_PBIT_24_TypeDef jpu_reg_0x128 = {.d32 = 0};
    JPU_GBU_PBIT_32_TypeDef jpu_reg_0x12c = {.d32 = 0};

    // SOI Header
    jpu_reg_0x124.b.data_16b = 0xFFD8;
    JPU->GBU_PBIT_16 = jpu_reg_0x124.d32;

    if (!pEncInfo->disableAPPMarker)
    {
        jpu_reg_0x12c.d32 = 0xFFE90004;
        JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
        // APP9 Header
        jpu_reg_0x124.b.data_16b = 0; // pEncInfo->frameIdx
        JPU->GBU_PBIT_16 = jpu_reg_0x124.d32;
    }



    // DRI header
    if (pEncInfo->rstIntval)
    {
        jpu_reg_0x12c.d32 = 0xFFDD0004;
        JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;

        jpu_reg_0x124.b.data_16b = pEncInfo->rstIntval;
        JPU->GBU_PBIT_16 = jpu_reg_0x124.d32;
    }

    // DQT Header
    jpu_reg_0x124.b.data_16b = 0xFFDB;
    JPU->GBU_PBIT_16 = jpu_reg_0x124.d32;

    if (pEncInfo->quantMode == JPG_TBL_NORMAL)
    {
        jpu_reg_0x128.d32 = 0x004300;
        JPU->GBU_PBIT_24 = jpu_reg_0x128.d32;

        for (i = 0; i < 64; i += 4)
        {
            jpu_reg_0x12c.d32 = pEncInfo->pQMatTab[0][i    ] << 24 |
                                pEncInfo->pQMatTab[0][i + 1] << 16 |
                                pEncInfo->pQMatTab[0][i + 2] << 8 |
                                pEncInfo->pQMatTab[0][i + 3];
            JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
        }

        if (pEncInfo->format != JPU_FORMAT_400)
        {
            jpu_reg_0x12c.d32 = 0xFFDB0043;
            JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;

            jpu_reg_0x120.d32 = 0x01;
            JPU->GBU_PBIT_08 = jpu_reg_0x120.d32;

            for (i = 0; i < 64; i += 4)
            {
                jpu_reg_0x12c.d32 = pEncInfo->pQMatTab[1][i    ] << 24 |
                                    pEncInfo->pQMatTab[1][i + 1] << 16 |
                                    pEncInfo->pQMatTab[1][i + 2] << 8 |
                                    pEncInfo->pQMatTab[1][i + 3];
                JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
            }
        }
    }
    else// if (para->quantMode == JPG_TBL_MERGE)
    {
        if (pEncInfo->format != JPU_FORMAT_400)
        {
            jpu_reg_0x128.d32 = 0x008400;
        }
        else
        {
            jpu_reg_0x128.d32 = 0x004300;
        }
        JPU->GBU_PBIT_24 = jpu_reg_0x128.d32;


        for (i = 0; i < 64; i += 4)
        {
            jpu_reg_0x12c.d32 = pEncInfo->pQMatTab[0][i    ] << 24 |
                                pEncInfo->pQMatTab[0][i + 1] << 16 |
                                pEncInfo->pQMatTab[0][i + 2] << 8 |
                                pEncInfo->pQMatTab[0][i + 3];
            JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
        }

        if (pEncInfo->format != JPU_FORMAT_400)
        {
            jpu_reg_0x120.d32 = 0x01;
            JPU->GBU_PBIT_08 = jpu_reg_0x120.d32;

            for (i = 0; i < 64; i += 4)
            {
                jpu_reg_0x12c.d32 = pEncInfo->pQMatTab[1][i    ] << 24 |
                                    pEncInfo->pQMatTab[1][i + 1] << 16 |
                                    pEncInfo->pQMatTab[1][i + 2] << 8 |
                                    pEncInfo->pQMatTab[1][i + 3];
                JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
            }
        }
    }

    // DHT Header
    jpu_reg_0x124.b.data_16b = 0xFFC4;
    JPU->GBU_PBIT_16 = jpu_reg_0x124.d32;

    if (pEncInfo->huffMode == JPG_TBL_NORMAL)
    {
        jpu_reg_0x128.d32 = 0x001F00;
        JPU->GBU_PBIT_24 = jpu_reg_0x128.d32;

        for (i = 0; i < 16; i += 4)
        {
            jpu_reg_0x12c.d32 = pEncInfo->pHuffBits[0][i    ] << 24 |
                                pEncInfo->pHuffBits[0][i + 1] << 16 |
                                pEncInfo->pHuffBits[0][i + 2] << 8 |
                                pEncInfo->pHuffBits[0][i + 3];
            JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
        }

        for (i = 0; i < 12; i += 4)
        {
            jpu_reg_0x12c.d32 = pEncInfo->pHuffVal[0][i    ] << 24 |
                                pEncInfo->pHuffVal[0][i + 1] << 16 |
                                pEncInfo->pHuffVal[0][i + 2] << 8 |
                                pEncInfo->pHuffVal[0][i + 3];
            JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
        }

        jpu_reg_0x12c.d32 = 0xFFC400B5;
        JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
        jpu_reg_0x120.d32 = 0x10;
        JPU->GBU_PBIT_08 = jpu_reg_0x120.d32;

        for (i = 0; i < 16; i += 4)
        {
            jpu_reg_0x12c.d32 = pEncInfo->pHuffBits[1][i    ] << 24 |
                                pEncInfo->pHuffBits[1][i + 1] << 16 |
                                pEncInfo->pHuffBits[1][i + 2] << 8 |
                                pEncInfo->pHuffBits[1][i + 3];
            JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
        }

        for (i = 0; i < 160; i += 4)
        {
            jpu_reg_0x12c.d32 = pEncInfo->pHuffVal[1][i    ] << 24 |
                                pEncInfo->pHuffVal[1][i + 1] << 16 |
                                pEncInfo->pHuffVal[1][i + 2] << 8 |
                                pEncInfo->pHuffVal[1][i + 3];
            JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
        }

        jpu_reg_0x124.b.data_16b = pEncInfo->pHuffVal[1][160] << 8 |
                                   pEncInfo->pHuffVal[1][161];
        JPU->GBU_PBIT_16 = jpu_reg_0x124.d32;

        if (pEncInfo->format != JPU_FORMAT_400)
        {
            jpu_reg_0x12c.d32 = 0xFFC4001F;
            JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
            jpu_reg_0x120.d32 = 0x01;
            JPU->GBU_PBIT_08 = jpu_reg_0x120.d32;

            for (i = 0; i < 16; i += 4)
            {
                jpu_reg_0x12c.d32 = pEncInfo->pHuffBits[2][i    ] << 24 |
                                    pEncInfo->pHuffBits[2][i + 1] << 16 |
                                    pEncInfo->pHuffBits[2][i + 2] << 8 |
                                    pEncInfo->pHuffBits[2][i + 3];
                JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
            }

            for (i = 0; i < 12; i += 4)
            {
                jpu_reg_0x12c.d32 = pEncInfo->pHuffVal[2][i    ] << 24 |
                                    pEncInfo->pHuffVal[2][i + 1] << 16 |
                                    pEncInfo->pHuffVal[2][i + 2] << 8 |
                                    pEncInfo->pHuffVal[2][i + 3];
                JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
            }

            jpu_reg_0x12c.d32 = 0xFFC400B5;
            JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
            jpu_reg_0x120.d32 = 0x11;
            JPU->GBU_PBIT_08 = jpu_reg_0x120.d32;

            for (i = 0; i < 16; i += 4)
            {
                jpu_reg_0x12c.d32 = pEncInfo->pHuffBits[3][i    ] << 24 |
                                    pEncInfo->pHuffBits[3][i + 1] << 16 |
                                    pEncInfo->pHuffBits[3][i + 2] << 8 |
                                    pEncInfo->pHuffBits[3][i + 3];
                JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
            }


            for (i = 0; i < 160; i += 4)
            {
                jpu_reg_0x12c.d32 = pEncInfo->pHuffVal[3][i    ] << 24 |
                                    pEncInfo->pHuffVal[3][i + 1] << 16 |
                                    pEncInfo->pHuffVal[3][i + 2] << 8 |
                                    pEncInfo->pHuffVal[3][i + 3];
                JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
            }

            jpu_reg_0x124.b.data_16b = pEncInfo->pHuffVal[3][160] << 8 |
                                       pEncInfo->pHuffVal[3][161];
            JPU->GBU_PBIT_16 = jpu_reg_0x124.d32;
        }
    }
    else// if (para->huffMode == JPG_TBL_MERGE)
    {
        if (pEncInfo->format != JPU_FORMAT_400)
        {
            jpu_reg_0x128.d32 = 0x01A200;
        }
        else
        {
            jpu_reg_0x128.d32 = 0x00D200;
        }
        JPU->GBU_PBIT_24 = jpu_reg_0x128.d32;


        for (i = 0; i < 16; i += 4)
        {
            jpu_reg_0x12c.d32 = pEncInfo->pHuffBits[0][i    ] << 24 |
                                pEncInfo->pHuffBits[0][i + 1] << 16 |
                                pEncInfo->pHuffBits[0][i + 2] << 8 |
                                pEncInfo->pHuffBits[0][i + 3];
            JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
        }

        for (i = 0; i < 12; i += 4)
        {
            jpu_reg_0x12c.d32 = pEncInfo->pHuffVal[0][i    ] << 24 |
                                pEncInfo->pHuffVal[0][i + 1] << 16 |
                                pEncInfo->pHuffVal[0][i + 2] << 8 |
                                pEncInfo->pHuffVal[0][i + 3];
            JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
        }

        jpu_reg_0x120.d32 = 0x10;
        JPU->GBU_PBIT_08 = jpu_reg_0x120.d32;

        for (i = 0; i < 16; i += 4)
        {
            jpu_reg_0x12c.d32 = pEncInfo->pHuffBits[1][i    ] << 24 |
                                pEncInfo->pHuffBits[1][i + 1] << 16 |
                                pEncInfo->pHuffBits[1][i + 2] << 8 |
                                pEncInfo->pHuffBits[1][i + 3];
            JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
        }


        for (i = 0; i < 160; i += 4)
        {
            jpu_reg_0x12c.d32 = pEncInfo->pHuffVal[1][i    ] << 24 |
                                pEncInfo->pHuffVal[1][i + 1] << 16 |
                                pEncInfo->pHuffVal[1][i + 2] << 8 |
                                pEncInfo->pHuffVal[1][i + 3];
            JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
        }

        jpu_reg_0x124.b.data_16b = pEncInfo->pHuffVal[1][160] << 8 |
                                   pEncInfo->pHuffVal[1][161];
        JPU->GBU_PBIT_16 = jpu_reg_0x124.d32;

        if (pEncInfo->format != JPU_FORMAT_400)
        {
            jpu_reg_0x120.d32 = 0x01;
            JPU->GBU_PBIT_08 = jpu_reg_0x120.d32;

            for (i = 0; i < 16; i += 4)
            {
                jpu_reg_0x12c.d32 = pEncInfo->pHuffBits[2][i    ] << 24 |
                                    pEncInfo->pHuffBits[2][i + 1] << 16 |
                                    pEncInfo->pHuffBits[2][i + 2] << 8 |
                                    pEncInfo->pHuffBits[2][i + 3];
                JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
            }

            for (i = 0; i < 12; i += 4)
            {
                jpu_reg_0x12c.d32 = pEncInfo->pHuffVal[2][i    ] << 24 |
                                    pEncInfo->pHuffVal[2][i + 1] << 16 |
                                    pEncInfo->pHuffVal[2][i + 2] << 8 |
                                    pEncInfo->pHuffVal[2][i + 3];
                JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
            }

            jpu_reg_0x120.d32 = 0x11;
            JPU->GBU_PBIT_08 = jpu_reg_0x120.d32;

            for (i = 0; i < 16; i += 4)
            {
                jpu_reg_0x12c.d32 = pEncInfo->pHuffBits[3][i    ] << 24 |
                                    pEncInfo->pHuffBits[3][i + 1] << 16 |
                                    pEncInfo->pHuffBits[3][i + 2] << 8 |
                                    pEncInfo->pHuffBits[3][i + 3];
                JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
            }


            for (i = 0; i < 160; i += 4)
            {
                jpu_reg_0x12c.d32 = pEncInfo->pHuffVal[3][i    ] << 24 |
                                    pEncInfo->pHuffVal[3][i + 1] << 16 |
                                    pEncInfo->pHuffVal[3][i + 2] << 8 |
                                    pEncInfo->pHuffVal[3][i + 3];
                JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;
            }

            jpu_reg_0x124.b.data_16b = pEncInfo->pHuffVal[3][160] << 8 |
                                       pEncInfo->pHuffVal[3][161];
            JPU->GBU_PBIT_16 = jpu_reg_0x124.d32;
        }
    }

    // SOF header
    jpu_reg_0x124.b.data_16b = 0xFFC0;
    JPU->GBU_PBIT_16 = jpu_reg_0x124.d32;
    jpu_reg_0x124.b.data_16b = (8 + (pEncInfo->compNum * 3));
    JPU->GBU_PBIT_16 = jpu_reg_0x124.d32;
    jpu_reg_0x120.d32 = 0x08;
    JPU->GBU_PBIT_08 = jpu_reg_0x120.d32;

    if (pEncInfo->rotationAngle == 90 || pEncInfo->rotationAngle == 270)
    {
        jpu_reg_0x12c.d32 = pEncInfo->picWidth << 16 | pEncInfo->picHeight;
    }
    else
    {
        jpu_reg_0x12c.d32 = pEncInfo->picHeight << 16 | pEncInfo->picWidth;
    }
    JPU->GBU_PBIT_32 = jpu_reg_0x12c.d32;

    jpu_reg_0x120.d32 = pEncInfo->compNum;
    JPU->GBU_PBIT_08 = jpu_reg_0x120.d32;


    frameFormat = pEncInfo->format;
    // frameFormat = FORMAT_420;
    if (pEncInfo->rotMirEnable && (pEncInfo->rotationAngle == 90 || pEncInfo->rotationAngle == 270))
    {
        frameFormat = (frameFormat == JPU_FORMAT_422) ? JPU_FORMAT_224 : (frameFormat == JPU_FORMAT_224) ?
                      JPU_FORMAT_422 :
                      frameFormat;
    }

    pEncInfo->pCInfoTab[0] = sJpuCompInfoTable[frameFormat];
    pEncInfo->pCInfoTab[1] = pEncInfo->pCInfoTab[0] + 6;
    pEncInfo->pCInfoTab[2] = pEncInfo->pCInfoTab[1] + 6;
    pEncInfo->pCInfoTab[3] = pEncInfo->pCInfoTab[2] + 6;
    for (i = 0; i < pEncInfo->compNum; i++)
    {
        jpu_reg_0x120.d32 = (i + 1);
        JPU->GBU_PBIT_08 = jpu_reg_0x120.d32;
        jpu_reg_0x120.d32 = ((pEncInfo->pCInfoTab[i][1] << 4) & 0xF0) + (pEncInfo->pCInfoTab[i][2] & 0x0F);
        JPU->GBU_PBIT_08 = jpu_reg_0x120.d32;
        jpu_reg_0x120.d32 = pEncInfo->pCInfoTab[i][3];
        JPU->GBU_PBIT_08 = jpu_reg_0x120.d32;
    }
    return 1;
}


static int JPU_EncLoadQMatTab(JPU_ENC_CFG *pEncInfo)
{

    long long int dividend = 0x80000;
    long long int quotient;
    int quantID;
    int divisor;
    int comp;
    int i, t;
    JPU_QMAT_CTRL_TypeDef jpu_reg_0x90 = {.d32 = 0};
    JPU_QMAT_DATA_TypeDef jpu_reg_0x98 = {.d32 = 0};

    for (comp = 0; comp < 3; comp++)
    {
        quantID = pEncInfo->pCInfoTab[comp][3];
        if (quantID >= 4)
        {
            return 0;
        }
        t = (comp == 0) ? Q_COMPONENT0 :
            (comp == 1) ? Q_COMPONENT1 : Q_COMPONENT2;
        jpu_reg_0x90.d32 = 0x3 + t;
        JPU->QMAT_CTRL = jpu_reg_0x90.d32;
        for (i = 0; i < 64; i++)
        {
            divisor = pEncInfo->pQMatTab[quantID][i];
            quotient = dividend / divisor;
            // enhace bit precision & rounding Q
            jpu_reg_0x98.d32 = (int)(divisor << 20) | (int)(quotient & 0xFFFFF);
            JPU->QMAT_DATA = jpu_reg_0x98.d32;
        }
        jpu_reg_0x90.d32 = t;
        JPU->QMAT_CTRL = jpu_reg_0x90.d32;
    }

    return 1;
}
/*============================================================================*
 *                           Public Functions
 *============================================================================*/

void JPU_Clk_Init(void)
{
    RCC_PeriphClockCmd(APBPeriph_JPEG, APBPeriph_JPEG_CLOCK, ENABLE);
}

void JPU_Clk_Deinit(void)
{
    RCC_PeriphClockCmd(APBPeriph_JPEG, APBPeriph_JPEG_CLOCK, DISABLE);
}

void JPU_Reset(void)
{
    JPU_Clk_Init();

    JPU_PIC_START_TypeDef jpu_reg_0x00 = {.d32 = 0};
    JPU_BBC_BAS_ADDR_TypeDef jpu_reg_0x230 = {.d32 = JPU->BBC_BAS_ADDR};
    JPU_BBC_END_ADDR_TypeDef jpu_reg_0x208 = {.d32 = JPU->BBC_END_ADDR};
    JPU_BBC_RD_PTR_TypeDef jpu_reg_0x210 = {.d32 = JPU->BBC_RD_PTR};
    JPU_BBC_WR_PTR_TypeDef jpu_reg_0x20c = {.d32 = JPU->BBC_WR_PTR};

    jpu_reg_0x00.b.init_codec = 1;
    JPU->PIC_START = jpu_reg_0x00.d32;
    do
    {
        jpu_reg_0x00.d32 = JPU->PIC_START;
    }
    while (jpu_reg_0x00.b.init_codec == 1);

    JPU->BBC_BAS_ADDR = jpu_reg_0x230.d32;
    JPU->BBC_END_ADDR = jpu_reg_0x208.d32;
    JPU->BBC_RD_PTR = jpu_reg_0x210.d32;
    JPU->BBC_WR_PTR = jpu_reg_0x20c.d32;
}

uint32_t JPU_Read_test(void)
{
    JPU_OP_INFO_TypeDef jpu_reg_0x02 = {.d32 = 0};
    jpu_reg_0x02.d32 = JPU->OP_INFO;
    return jpu_reg_0x02.d32;
}

void JPU_Write_test(uint32_t val)
{
    JPU_OP_INFO_TypeDef jpu_reg_0x02 = {.d32 = val};
    JPU->OP_INFO = jpu_reg_0x02.d32;
}


static JPU_HW_INFO output_hw_info;

JPU_HW_INFO *JPU_Read_Output(void)
{
    return &output_hw_info;
}

uint32_t JPU_Decode(JPU_DEC_CFG *config)
{
    JPU_Clk_Init();
    // bs
#if 0
    JPU_BBC_BAS_ADDR_TypeDef jpu_reg_0x230 = {.d32 = JPU->BBC_BAS_ADDR};
    jpu_reg_0x230.b.base_addr = config->bs_buffer_start;
    JPU->BBC_BAS_ADDR = jpu_reg_0x230.d32;

    JPU_BBC_END_ADDR_TypeDef jpu_reg_0x208 = {.d32 = JPU->BBC_END_ADDR};
    jpu_reg_0x208.b.int_addr = config->bs_buffer_end;
    JPU->BBC_END_ADDR = jpu_reg_0x208.d32;

    JPU_BBC_RD_PTR_TypeDef jpu_reg_0x210 = {.d32 = JPU->BBC_RD_PTR};
    jpu_reg_0x210.b.rd_ptr = config->bs_rd_ptr;
    JPU->BBC_RD_PTR = jpu_reg_0x210.d32;

    JPU_BBC_WR_PTR_TypeDef jpu_reg_0x20c = {.d32 = JPU->BBC_WR_PTR};
    jpu_reg_0x20c.b.wr_ptr = config->bs_wr_ptr;
    JPU->BBC_WR_PTR = jpu_reg_0x20c.d32;

    JPU_BBC_STRM_CTRL_TypeDef jpu_reg_0x234 = {.d32 = 0};
    JPU->BBC_STRM_CTRL = jpu_reg_0x234.d32;
#endif

    uint32_t val = 0;

    if (config->bs_rd_ptr == config->bs_buffer_end)
    {
        JPU_BBC_CUR_POS_TypeDef jpu_reg_0x22c = {.d32 = 0};
        JPU->BBC_CUR_POS = jpu_reg_0x22c.d32;

        JPU_GBU_TT_CNT_L_TypeDef jpu_reg_0x118 = {.d32 = 0};
        JPU->GBU_TT_CNT_L = jpu_reg_0x118.d32;

        JPU_GBU_TT_CNT_H_TypeDef jpu_reg_0x11c = {.d32 = 0};
        JPU->GBU_TT_CNT_H = jpu_reg_0x11c.d32;
    }

    JPU_BBC_WR_PTR_TypeDef jpu_reg_0x20c = {.d32 = JPU->BBC_WR_PTR};
    jpu_reg_0x20c.b.wr_ptr = (uint32_t)config->bs_wr_ptr;
    JPU->BBC_WR_PTR = jpu_reg_0x20c.d32;

    if (config->bs_wr_ptr == config->bs_buffer_start)
    {
        JPU_BBC_END_ADDR_TypeDef jpu_reg_0x208 = {.d32 = JPU->BBC_END_ADDR};
        jpu_reg_0x208.b.int_addr = (uint32_t)config->bs_buffer_end;
        JPU->BBC_END_ADDR = jpu_reg_0x208.d32;
    }
    else
    {
        JPU_BBC_END_ADDR_TypeDef jpu_reg_0x208 = {.d32 = JPU->BBC_END_ADDR};
        jpu_reg_0x208.b.int_addr = (uint32_t)config->bs_wr_ptr;
        JPU->BBC_END_ADDR = jpu_reg_0x208.d32;
    }
    JPU_BBC_BAS_ADDR_TypeDef jpu_reg_0x230 = {.d32 = JPU->BBC_BAS_ADDR};
    jpu_reg_0x230.b.base_addr = (uint32_t)config->bs_buffer_start;
    JPU->BBC_BAS_ADDR = jpu_reg_0x230.d32;

    // stream is smaller than buffer, set eof, set page counter
    if (config->strm_end_flg)
    {
        val = (config->bs_wr_ptr - config->bs_buffer_start + 255) >> 8;
        JPU_BBC_STRM_CTRL_TypeDef jpu_reg_0x234 = {.d32 = 0};
        jpu_reg_0x234.b.eof = 1;
        jpu_reg_0x234.b.strm_cnt = val;
        JPU->BBC_STRM_CTRL = jpu_reg_0x234.d32;
    }



    JPU_GBU_TT_CNT_L_TypeDef jpu_reg_0x118 = {.d32 = 0};
    JPU->GBU_TT_CNT_L = jpu_reg_0x118.d32;

    JPU_GBU_TT_CNT_H_TypeDef jpu_reg_0x11c = {.d32 = 0};
    JPU->GBU_TT_CNT_H = jpu_reg_0x11c.d32;


    JPU_PIC_CTRL_TypeDef jpu_reg_0x10 = {.d32 = 0};
    jpu_reg_0x10.b.ac_ht_idx = config->huffAcIdx;
    jpu_reg_0x10.b.dc_ht_idx = config->huffDcIdx;
    jpu_reg_0x10.b.usr_ht_en = config->userHuffTab;
    jpu_reg_0x10.b.dma_wr_resp_en = 1; // JPU_DEC_CHECK_WRITE_RESPONSE_BVALID_SIGNAL
    jpu_reg_0x10.b.op_mode = config->usePartial;
    JPU->PIC_CTRL = jpu_reg_0x10.d32;


    JPU_PIC_SIZE_TypeDef jpu_reg_0x14 = {.d32 = 0};
    jpu_reg_0x14.b.pic_width = config->alignedWidth;
    jpu_reg_0x14.b.pic_height = config->alignedHeight;
    JPU->PIC_SIZE = jpu_reg_0x14.d32;


    JPU_ROT_INFO_TypeDef jpu_reg_0x1c = {.d32 = 0};
    JPU->ROT_INFO = jpu_reg_0x1c.d32;

    JPU_OP_INFO_TypeDef jpu_reg_0x2c = {.d32 = 0};
    jpu_reg_0x2c.b.pb_line = config->lineNum;
    jpu_reg_0x2c.b.pb_num = config->bufNum;
    jpu_reg_0x2c.b.mcu_req_num = config->busReqNum;
    JPU->OP_INFO = jpu_reg_0x2c.d32;

    JPU_MCU_INFO_TypeDef jpu_reg_0x18 = {.d32 = 0};
    jpu_reg_0x18.b.block_num = config->mcuBlockNum;
    jpu_reg_0x18.b.cmp_num = config->compNum;
    jpu_reg_0x18.b.cmp0_v_sf = config->compInfo[0];
    jpu_reg_0x18.b.cmp1_v_sf = config->compInfo[1];
    jpu_reg_0x18.b.cmp2_v_sf = config->compInfo[2];
    jpu_reg_0x18.b.cmp0_h_sf = config->compInfo[0] >> 2;
    jpu_reg_0x18.b.cmp1_h_sf = config->compInfo[1] >> 2;
    jpu_reg_0x18.b.cmp2_h_sf = config->compInfo[2] >> 2;
    JPU->MCU_INFO = jpu_reg_0x18.d32;


    JPU_DPB_CONFIG_TypeDef jpu_reg_0x30 = {.d32 = 0};
    jpu_reg_0x30.b.dpb_endianess = config->frameEndian;
    jpu_reg_0x30.b.dpb_interleave_mode = ((config->chromaInterleave == 0) ? 0 :
                                          (config->chromaInterleave == 1) ? 2 : 3);
    jpu_reg_0x30.b.pack_mode = config->req_packed_format;
    JPU->DPB_CONFIG = jpu_reg_0x30.d32;


    JPU_RST_INTERVAL_TypeDef jpu_reg_0xb0 = {.d32 = 0};
    jpu_reg_0xb0.b.restart_interval = config->rstIntval;
    JPU->RST_INTVAL = jpu_reg_0xb0.d32;


    // down scale
    JPU_SCL_INFO_TypeDef jpu_reg_0x20 = {.d32 = 0};
    if (config->iHorScaleFactor || config->iVerScaleFactor)
    {
        jpu_reg_0x20.b.ds_en = 1;
        jpu_reg_0x20.b.h_ds_factor = config->iHorScaleFactor;
        jpu_reg_0x20.b.v_ds_factor = config->iVerScaleFactor;
    }
    JPU->SCL_INFO = jpu_reg_0x20.d32;


    // HuffTab
    if (config->userHuffTab)
    {
        JPU_DecHuffTabSetUp(config);
        // if (!JPU_DecHuffTabSetUp(config))
        // {
        //     return JPU_ERR_INVALID_PARAM;
        // }
    }


    JPU_DecQMatTabSetUp(config);
    // if (!JPU_DecQMatTabSetUp(config))
    // {
    //     return JPU_ERR_INVALID_PARAM;
    // }

    JPU_DecGramSetup(config);



    JPU_RST_INDEX_TypeDef jpu_reg_0xb4 = {.d32 = 0};
    JPU->RST_INDEX = jpu_reg_0xb4.d32; // RST index at the beginning.

    JPU_RST_COUNT_TypeDef jpu_reg_0xb8 = {.d32 = 0};
    JPU->RST_COUNT = jpu_reg_0xb8.d32;

    JPU_DPCM_DIFF_Y_TypeDef jpu_reg_0xf0 = {.d32 = 0};
    JPU->DPCM_DIFF_Y = jpu_reg_0xf0.d32;

    JPU_DPCM_DIFF_CB_TypeDef jpu_reg_0xf4 = {.d32 = 0};
    JPU->DPCM_DIFF_CB = jpu_reg_0xf4.d32;

    JPU_DPCM_DIFF_CR_TypeDef jpu_reg_0xf8 = {.d32 = 0};
    JPU->DPCM_DIFF_CR = jpu_reg_0xf8.d32;

    JPU_GBU_FF_RPTR_TypeDef jpu_reg_0x160 = {.d32 = 0};
    jpu_reg_0x160.d32 = config->bitPtr;
    JPU->GBU_FF_RPTR = jpu_reg_0x160.d32;

    JPU_GBU_CTRL_TypeDef jpu_reg_0x100 = {.d32 = 0};
    jpu_reg_0x100.d32 = 3;
    JPU->GBU_CTRL = jpu_reg_0x100.d32;

    // JPU_ROT_INFO_TypeDef jpu_reg_0x1c = {.d32 = 0};
    jpu_reg_0x1c.d32 = config->rotMir;
    JPU->ROT_INFO = jpu_reg_0x1c.d32;

    if (config->rotMir & 1)
    {
        config->format = (config->format == JPU_FORMAT_422) ? JPU_FORMAT_224 :
                         (config->format == JPU_FORMAT_224) ? JPU_FORMAT_422 : config->format;
    }

    // rotate
    JPU_DPB_BASE00_TypeDef jpu_reg_0x34 = {.d32 = 0};
    JPU_DPB_BASE01_TypeDef jpu_reg_0x38 = {.d32 = 0};
    JPU_DPB_BASE02_TypeDef jpu_reg_0x3c = {.d32 = 0};

    if (config->rotMir & 0x10)
    {
        jpu_reg_0x34.d32 = config->bufY;
        jpu_reg_0x38.d32 = config->bufCb;
        jpu_reg_0x3c.d32 = config->bufCr;
    }
    else if (config->usePartial)
    {
        // TODO
    }
    else
    {
        jpu_reg_0x34.d32 = config->bufY;
        jpu_reg_0x38.d32 = config->bufCb;
        jpu_reg_0x3c.d32 = config->bufCr;
    }
    JPU->DPB_BASE00 = jpu_reg_0x34.d32;
    JPU->DPB_BASE01 = jpu_reg_0x38.d32;
    JPU->DPB_BASE02 = jpu_reg_0x3c.d32;


    JPU_DPB_YSTRIDE_TypeDef jpu_reg_0x64 = {.d32 = 0};
    JPU_DPB_CSTRIDE_TypeDef jpu_reg_0x68 = {.d32 = 0};
    if (config->rotationEnable)
    {
        jpu_reg_0x64.d32 = config->rotatorStride;
        val = (config->format == JPU_FORMAT_420 || config->format == JPU_FORMAT_422 ||
               config->format == JPU_FORMAT_400) ? 2 : 1;
        if (config->chromaInterleave)
        {
            jpu_reg_0x68.d32 = (config->rotatorStride / (int)val) * 2;
        }
        else
        {
            jpu_reg_0x68.d32 = config->rotatorStride / (int)val;
        }
    }
    else
    {
        jpu_reg_0x64.d32 = config->stride;
        val = (config->format == JPU_FORMAT_420 || config->format == JPU_FORMAT_422 ||
               config->format == JPU_FORMAT_400) ? 2 : 1;
        if (config->chromaInterleave)
        {
            jpu_reg_0x68.d32 = (config->stride / (int)val) * 2;
        }
        else
        {
            jpu_reg_0x68.d32 = config->stride / (int)val;
        }
    }
    JPU->DPB_YSTRIDE = jpu_reg_0x64.d32;
    JPU->DPB_CSTRIDE = jpu_reg_0x68.d32;


    // roi
    JPU_CLP_INFO_TypeDef jpu_reg_0x28 = {.d32 = 0};
    if (config->roiEnable)
    {
        jpu_reg_0x28.d32 = 1;

        JPU_CLP_BASE_TypeDef jpu_reg_0x70 = {.d32 = 0};
        jpu_reg_0x70.d32 = config->roiOffsetX << 16 | config->roiOffsetY; // pixel unit
        JPU->CLP_BASE = jpu_reg_0x70.d32;

        JPU_CLP_SIZE_TypeDef jpu_reg_0x74 = {.d32 = 0};
        jpu_reg_0x74.d32 = (config->roiMcuWidth * config->mcuWidth) << 16 |
                           (config->roiMcuHeight * config->mcuHeight); // pixel Unit
        JPU->CLP_SIZE = jpu_reg_0x74.d32;
    }
    else
    {
        jpu_reg_0x28.d32 = 0;
    }
    JPU->CLP_INFO = jpu_reg_0x28.d32;


    // wrapper
    JPU_WRAP_CFG_TypeDef jpu_wrap_reg_0x04 = {.d32 = 0};
    jpu_wrap_reg_0x04.b.axi_wr_mstr = config->wrapper_en ? 0x01 : 0x00;
    JPU_WRAP->CONFIG = jpu_wrap_reg_0x04.d32;

    JPU_WRAP_RGB_PF_TypeDef jpu_wrap_reg_0x00 = {.d32 = 0};
    jpu_wrap_reg_0x00.b.rgb_format = config->rgb_type;
    jpu_wrap_reg_0x00.b.alpha = config->opa;
    JPU_WRAP->RGB_PF = jpu_wrap_reg_0x00.d32;

    // JPU_Dump();

    // launch decoder
    JPU_PIC_START_TypeDef jpu_reg_0x00 = {.d32 = 0};
    jpu_reg_0x00.b.start_codec = 1;
    JPU->PIC_START = jpu_reg_0x00.d32;


    // jpu done
    JPU_PIC_STATUS_TypeDef jpu_reg_0x04 = {.d32 = JPU->PIC_STATUS};
    while (1)
    {
        jpu_reg_0x04.d32 = JPU->PIC_STATUS;
        if (jpu_reg_0x04.d32)
        {
            break;
        }
    }

    // wrapper done
    if (jpu_reg_0x04.b.jpg_done && config->wrapper_en)
    {
        JPU_WRAP_DONE_TypeDef jpu_wrap_reg_0x08 = {.d32 = 0};
        while (1)
        {
            jpu_wrap_reg_0x08.d32 = JPU_WRAP->WRAP_DONE;
            if (jpu_wrap_reg_0x08.d32)
            {
                break;
            }
        }
    }

    uint32_t ret = false;
    ret = jpu_reg_0x04.b.jpg_done ? true : false;

    output_hw_info.status.d32 = jpu_reg_0x04.d32;
    JPU_PIC_ERRMB_TypeDef jpu_reg_0x08 = {.d32 = JPU->PIC_ERRMB};
    output_hw_info.dec_errmb.d32 = jpu_reg_0x08.d32;
    JPU_CYCLE_INFO_TypeDef jpu_reg_0xc8 = {.d32 = JPU->CYCLE_INFO};
    output_hw_info.cycle = jpu_reg_0xc8.d32;

    // clear int
    if (jpu_reg_0x04.d32)
    {
        JPU->PIC_STATUS = jpu_reg_0x04.d32;
    }

    return ret;
}

uint32_t JPU_Encode(JPU_ENC_CFG *config)
{
    JPU_Clk_Init();

    // roi
    JPU_CLP_INFO_TypeDef jpu_reg_0x28 = {.d32 = 0};
    JPU->CLP_INFO = jpu_reg_0x28.d32;

    JPU_BBC_BAS_ADDR_TypeDef jpu_reg_0x230 = {.d32 = JPU->BBC_BAS_ADDR};
    jpu_reg_0x230.b.base_addr = (uint32_t)config->bs_buffer_start;
    JPU->BBC_BAS_ADDR = jpu_reg_0x230.d32;

    JPU_BBC_END_ADDR_TypeDef jpu_reg_0x208 = {.d32 = JPU->BBC_END_ADDR};
    jpu_reg_0x208.b.int_addr = (uint32_t)config->bs_buffer_end;
    JPU->BBC_END_ADDR = jpu_reg_0x208.d32;

    JPU_BBC_WR_PTR_TypeDef jpu_reg_0x20c = {.d32 = JPU->BBC_WR_PTR};
    jpu_reg_0x20c.b.wr_ptr = (uint32_t)config->bs_wr_ptr;
    JPU->BBC_WR_PTR = jpu_reg_0x20c.d32;

    JPU_BBC_RD_PTR_TypeDef jpu_reg_0x210 = {.d32 = JPU->BBC_RD_PTR};
    jpu_reg_0x210.b.rd_ptr = (uint32_t)config->bs_rd_ptr;
    JPU->BBC_RD_PTR = jpu_reg_0x210.d32;

    JPU_BBC_CUR_POS_TypeDef jpu_reg_0x22c = {.d32 = 0};
    JPU->BBC_CUR_POS = jpu_reg_0x22c.d32;

    JPU_BBC_DATA_CNT_TypeDef jpu_reg_21c = {.d32 = 0};
    jpu_reg_21c.d32 = 256 / 4;
    JPU->BBC_DATA_CNT = jpu_reg_21c.d32;

    JPU_BBC_EXT_ADDR_TypeDef jpu_reg_0x214 = {.d32 = 0};
    jpu_reg_0x214.d32 = (uint32_t)config->bs_wr_ptr;
    JPU->BBC_EXT_ADDR = jpu_reg_0x214.d32;

    JPU_BBC_INT_ADDR_TypeDef jpu_reg_0x218 = {.d32 = 0};
    JPU->BBC_INT_ADDR = jpu_reg_0x218.d32;


    JPU_GBU_BT_PTR_TypeDef jpu_reg_0x110 = {.d32 = 0};
    JPU->GBU_BT_PTR = jpu_reg_0x110.d32;

    JPU_GBU_WD_PTR_TypeDef jpu_reg_0x114 = {.d32 = 0};
    JPU->GBU_WD_PTR = jpu_reg_0x114.d32;

    JPU_GBU_BBSR_TypeDef jpu_reg_0x140 = {.d32 = 0};
    JPU->GBU_BBSR = jpu_reg_0x140.d32;

    JPU_GBU_CTRL_TypeDef jpu_reg_0x100 = {.d32 = 0};
    JPU->GBU_CTRL = jpu_reg_0x100.d32;


    JPU_GBU_BBER_TypeDef jpu_reg_0x144 = {.d32 = 0};
    jpu_reg_0x144.d32 = ((256 / 4) * 2) - 1;
    JPU->GBU_BBER = jpu_reg_0x144.d32;


    JPU_GBU_BBIR_TypeDef jpu_reg_0x148 = {.d32 = 0};
    JPU_GBU_BBHR_TypeDef jpu_reg_0x14c = {.d32 = 0};
    jpu_reg_0x148.d32 = 256 / 4;   // 64 * 4 byte == 32 * 8 byte
    jpu_reg_0x14c.d32 = 256 / 4;   // 64 * 4 byte == 32 * 8 byte
    JPU->GBU_BBIR = jpu_reg_0x148.d32;
    JPU->GBU_BBHR = jpu_reg_0x14c.d32;


    JPU_PIC_CTRL_TypeDef jpu_reg_0x10 = {.d32 = 0};
    jpu_reg_0x10.d32 = 0x18 | config->usePartial;
    JPU->PIC_CTRL = jpu_reg_0x10.d32;

    JPU_SCL_INFO_TypeDef jpu_reg_0x20 = {.d32 = 0};
    JPU->SCL_INFO = jpu_reg_0x20.d32;



    JPU_DPB_CONFIG_TypeDef jpu_reg_0x30 = {.d32 = 0};
    jpu_reg_0x30.b.dpb_endianess = config->frameEndian;
    jpu_reg_0x30.b.dpb_interleave_mode = ((config->chromaInterleave == 0) ? 0 :
                                          (config->chromaInterleave == 1) ? 2 : 3);
    jpu_reg_0x30.b.pack_mode = config->frame_packed_format;
    JPU->DPB_CONFIG = jpu_reg_0x30.d32;

    JPU_RST_INTERVAL_TypeDef jpu_reg_0xb0 = {.d32 = 0};
    jpu_reg_0xb0.b.restart_interval = config->rstIntval;
    JPU->RST_INTVAL = jpu_reg_0xb0.d32;

    JPU_BBC_CTRL_TypeDef jpu_reg_0x228 = {.d32 = 0};
    jpu_reg_0x228.d32 = (config->streamEndian << 1) | 1;
    JPU->BBC_CTRL = jpu_reg_0x228.d32;

    JPU_OP_INFO_TypeDef jpu_reg_0x2c = {.d32 = 0};
    jpu_reg_0x2c.b.pb_line = config->lineNum;
    jpu_reg_0x2c.b.pb_num = config->bufNum;
    jpu_reg_0x2c.b.mcu_req_num = config->busReqNum;
    JPU->OP_INFO = jpu_reg_0x2c.d32;

    // Load HUFFTab
    JPU_EncLoadHuffTab(config);
    // if (!JPU_EncLoadHuffTab(config))
    // {
    //     return JPG_RET_INVALID_PARAM;
    // }


    JPU_EncEncodeHeader(config);

    // Load QMATTab
    JPU_EncLoadQMatTab(config);
    // if (!JPU_EncLoadQMatTab(pEncInfo))
    // {
    //     JpgLeaveLock();
    //     return JPG_RET_INVALID_PARAM;
    // }

    //although rotator is enable, this picture size must not be changed from width to height.
    JPU_PIC_SIZE_TypeDef jpu_reg_0x14 = {.d32 = 0};
    jpu_reg_0x14.b.pic_width = config->alignedWidth;
    jpu_reg_0x14.b.pic_height = config->alignedHeight;
    JPU->PIC_SIZE = jpu_reg_0x14.d32;
    JPU_ROT_INFO_TypeDef jpu_reg_0x1c = {.d32 = 0};
    jpu_reg_0x1c.d32 = (config->rotMirEnable | config->rotMirMode);
    JPU->ROT_INFO = jpu_reg_0x1c.d32;



    // MCU setting  EN
    JPU_MCU_INFO_TypeDef jpu_reg_0x18 = {.d32 = 0};
    switch (config->format)
    {
    case JPU_FORMAT_422:
        // DBG_DIRECT("Encode to 422 JPG");
        jpu_reg_0x18.d32 = 0x00043955;
        break;
    case JPU_FORMAT_444:
        // DBG_DIRECT("Encode to 444 JPG");
        // different from sw package original value
        jpu_reg_0x18.d32 = 0x00133555;
        break;
    case JPU_FORMAT_420:
        // DBG_DIRECT("Encode to 420 JPG");
        jpu_reg_0x18.d32 = 0x00063A55;
        break;
    default:
        jpu_reg_0x18.d32 = config->mcuBlockNum << 16 | config->compNum << 12
                           | config->compInfo[0] << 8 | config->compInfo[1] << 4 | config->compInfo[2];
        // DBG_DIRECT("MCU: Ori 0x%x", jpu_reg_0x18.d32);
        break;
    }
    JPU->MCU_INFO = jpu_reg_0x18.d32;

    jpu_reg_0x100.d32 = config->stuffByteEnable << 3; // stuffing "FF" data where frame end
    JPU->GBU_CTRL = jpu_reg_0x100.d32;

    JPU_DPB_BASE00_TypeDef jpu_reg_0x34 = {.d32 = 0};
    JPU_DPB_BASE01_TypeDef jpu_reg_0x38 = {.d32 = 0};
    JPU_DPB_BASE02_TypeDef jpu_reg_0x3c = {.d32 = 0};

    if (config->usePartial)
    {
        // TODO
    }
    else
    {
        jpu_reg_0x34.d32 = config->bufY;
        jpu_reg_0x38.d32 = config->bufCb;
        jpu_reg_0x3c.d32 = config->bufCr;
    }
    JPU->DPB_BASE00 = jpu_reg_0x34.d32;
    JPU->DPB_BASE01 = jpu_reg_0x38.d32;
    JPU->DPB_BASE02 = jpu_reg_0x3c.d32;

    JPU_DPB_YSTRIDE_TypeDef jpu_reg_0x64 = {.d32 = 0};
    JPU_DPB_CSTRIDE_TypeDef jpu_reg_0x68 = {.d32 = 0};
    jpu_reg_0x64.d32 = config->stride;
    JPU->DPB_YSTRIDE = jpu_reg_0x64.d32;

    uint32_t val = (config->format == JPU_FORMAT_420 || config->format == JPU_FORMAT_422 ||
                    config->format == JPU_FORMAT_400) ? 2 : 1;
    if (config->chromaInterleave)
    {
        jpu_reg_0x68.d32 = (config->stride / (int)val) * 2;
    }
    else
    {
        jpu_reg_0x68.d32 = config->stride / (int)val;
    }
    JPU->DPB_CSTRIDE = jpu_reg_0x68.d32;


    // wrapper
    JPU_WRAP_CFG_TypeDef jpu_wrap_reg_0x04 = {.d32 = 0};
    jpu_wrap_reg_0x04.b.axi_wr_mstr = config->wrapper_en ? 0x01 : 0x00;
    JPU_WRAP->CONFIG = jpu_wrap_reg_0x04.d32;

    JPU_WRAP_RGB_PF_TypeDef jpu_wrap_reg_0x00 = {.d32 = 0};
    jpu_wrap_reg_0x00.b.rgb_format = config->rgb_type;
    JPU_WRAP->RGB_PF = jpu_wrap_reg_0x00.d32;


    JPU_PIC_STATUS_TypeDef jpu_reg_0x04 = {.d32 = JPU->PIC_STATUS};
    JPU->PIC_STATUS = jpu_reg_0x04.d32;

    // JPU_Dump();

    // launch encoder
    JPU_PIC_START_TypeDef jpu_reg_0x00 = {.d32 = 0};
    if (config->usePartial)
    {
        // TODO
    }
    else
    {
        jpu_reg_0x00.b.start_codec = 1;
    }
    JPU->PIC_START = jpu_reg_0x00.d32;

    // jpu done
    while (1)
    {
        jpu_reg_0x04.d32 = JPU->PIC_STATUS;
        if (jpu_reg_0x04.d32)
        {
            break;
        }
    }

    uint32_t ret = false;
    ret = jpu_reg_0x04.b.jpg_done ? true : false;

    output_hw_info.status.d32 = jpu_reg_0x04.d32;
    jpu_reg_0x20c.d32 = JPU->BBC_WR_PTR;
    output_hw_info.strmWrPtr = jpu_reg_0x20c.d32;
    JPU_CYCLE_INFO_TypeDef jpu_reg_0xc8 = {.d32 = JPU->CYCLE_INFO};
    output_hw_info.cycle = jpu_reg_0xc8.d32;

    // clear int
    if (jpu_reg_0x04.d32)
    {
        JPU->PIC_STATUS = jpu_reg_0x04.d32;
    }

    if (ret)
    {
        JPU_BBC_FLUSH_CMD_TypeDef jpu_reg_0x238 = {.d32 = 0};
        JPU->BBC_FLUSH_CMD = jpu_reg_0x238.d32;
    }

    return ret;
}

bool JPU_Busy(void)
{
    JPU_BBC_BUSY_TypeDef jpu_reg_0x224 = {.d32 = JPU->BBC_BUSY};
    JPU_GBU_PBIT_BUSY_TypeDef jpu_reg_0x104 = {.d32 = JPU->GBU_PBIT_BUSY};

    return (jpu_reg_0x224.b.busy || jpu_reg_0x104.b.busy);
}


void JPU_Dump(void)
{
    volatile uint32_t addr = (uint32_t)JPU;
    volatile uint32_t val = 0;

    DBG_DIRECT("\n---- Dump JPU Reg ----");
    for (uint32_t i = 0; i <= 0x238; i += 0x4)
    {
        val = *(uint32_t *)(addr + i);
        DBG_DIRECT("0x%x \t 0x%x", (i), val);
    }
    DBG_DIRECT("\n");

    DBG_DIRECT("\n---- Dump Wrapper Reg ----");
    addr = (uint32_t)JPU_WRAP;
    val = 0;
    for (uint32_t i = 0; i <= 0x1c; i += 0x4)
    {
        val = *(uint32_t *)(addr + i);
        DBG_DIRECT("0x%x \t 0x%x", (i), val);
    }
    DBG_DIRECT("\n");
}
