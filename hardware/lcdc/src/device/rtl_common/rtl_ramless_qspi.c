/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* \file     rtl_ramless_qspi.c
* \brief    This file provides all the Ramless QSPI firmware functions.
* \details
* \author   astor zhang
* \date     2023-10-17
* \version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "rtl_ramless_qspi.h"

/*============================================================================*
 *                           Public Functions
 *============================================================================*/
void RLSPI_StructInit(LCDC_RLSPI_initTypeDef *RLSPI_InitStruct)
{
    assert_param(RLSPI_InitStruct);
    RLSPI_InitStruct->VSA = 0;
    RLSPI_InitStruct->VBP = 0;
    RLSPI_InitStruct->VFP = 0;
    RLSPI_InitStruct->VSYNC_CMD = 0;
    RLSPI_InitStruct->VSYNC_CMD_ADDR = 0;
    RLSPI_InitStruct->HSYNC_CMD_VBP = 0;
    RLSPI_InitStruct->HSYNC_CMD_VBP_ADDR = 0;
    RLSPI_InitStruct->HSYNC_CMD_VACTIVE = 0;
    RLSPI_InitStruct->HSYNC_CMD_VACTIVE_ADDR = 0;
    RLSPI_InitStruct->HSYNC_CMD_VFP = 0;
    RLSPI_InitStruct->HSYNC_CMD_VFP_ADDR = 0;
    RLSPI_InitStruct->DUMMY_CMD = 0;
    RLSPI_InitStruct->width = 0;
    RLSPI_InitStruct->height = 0;
    RLSPI_InitStruct->line_delay_in_vactive = 0;
    RLSPI_InitStruct->line_delay_out_vactive = 0;
    RLSPI_InitStruct->frame_delay = 0;
}

void RLSPI_Init(LCDC_RLSPI_initTypeDef *RLSPI_InitStruct)
{
    assert_param(RLSPI_InitStruct);
    assert_param((RLSPI_InitStruct->VSA & RLSPI_VERTICAL_SYNC_WIDTH_CLR) == 0);
    assert_param((RLSPI_InitStruct->VBP & RLSPI_VERTICAL_ABACK_PORCH_CLR) == 0);
    RAMLESS_QSPI->RLSPI_VERTICAL_SYNC_WIDTH = RLSPI_InitStruct->VSA;
    RAMLESS_QSPI->RLSPI_VERTICAL_ABACK_PORCH = RLSPI_InitStruct->VSA + RLSPI_InitStruct->VBP;
    RAMLESS_QSPI->RLSPI_VERTICAL_AACTIVE = RLSPI_InitStruct->VSA + RLSPI_InitStruct->VBP +
                                           RLSPI_InitStruct->height;
    RAMLESS_QSPI->RLSPI_VERTICAL_TOTAL_HEIGHT = RLSPI_InitStruct->VSA + RLSPI_InitStruct->VBP +
                                                RLSPI_InitStruct->height + RLSPI_InitStruct->VFP;
    RAMLESS_QSPI->RLSPI_HORIZONTAL_WIDTH = RLSPI_InitStruct->width;
    RAMLESS_QSPI->RLSPI_VSYNC_CMD = RLSPI_InitStruct->VSYNC_CMD;
    RAMLESS_QSPI->RLSPI_VSYNC_CMD_ADDR = RLSPI_InitStruct->VSYNC_CMD_ADDR;
    RAMLESS_QSPI->RLSPI_HSYNC_CMD_VFP = RLSPI_InitStruct->HSYNC_CMD_VFP;
    RAMLESS_QSPI->RLSPI_HSYNC_CMD_ADDR_VFP = RLSPI_InitStruct->HSYNC_CMD_VFP_ADDR;
    RAMLESS_QSPI->RLSPI_HSYNC_CMD_VBPORCH = RLSPI_InitStruct->HSYNC_CMD_VBP;
    RAMLESS_QSPI->RLSPI_HSYNC_CMD_ADDR_VBPORCH = RLSPI_InitStruct->HSYNC_CMD_VBP_ADDR;
    RAMLESS_QSPI->RLSPI_HSYNC_CMD_VACTIVE = RLSPI_InitStruct->HSYNC_CMD_VACTIVE;
    RAMLESS_QSPI->RLSPI_HSYNC_CMD_ADDR_VACTIVE = RLSPI_InitStruct->HSYNC_CMD_VACTIVE_ADDR;
    RAMLESS_QSPI->RLSPI_LINE_DELAY_IN_VACTIVE = RLSPI_InitStruct->line_delay_in_vactive;
    RAMLESS_QSPI->RLSPI_LINE_DELAY_OUT_VACTIVE = RLSPI_InitStruct->line_delay_out_vactive;
    RAMLESS_QSPI->RLSPI_FRAME_DELAY_INFINITE = RLSPI_InitStruct->frame_delay;
}

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/