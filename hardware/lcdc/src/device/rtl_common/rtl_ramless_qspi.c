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
void RLSPI_StructInit(LCDC_RLSPI_initTypeDef *init_struct)
{
    assert_param(obj);
    init_struct->VBP = 0;
    init_struct->VFP = 0;
    init_struct->VSA = 0;
}

void RLSPI_Init(LCDC_RLSPI_initTypeDef *init_struct)
{
    assert_param(init_struct);
    assert_param((init_struct->VSA & RLSPI_VERTICAL_SYNC_WIDTH_CLR) == 0);
    assert_param((init_struct->VBP & RLSPI_VERTICAL_ABACK_PORCH_CLR) == 0);
    RAMLESS_QSPI->RLSPI_VERTICAL_SYNC_WIDTH = init_struct->VSA;
    RAMLESS_QSPI->RLSPI_VERTICAL_ABACK_PORCH = init_struct->VSA + init_struct->VBP;
    RAMLESS_QSPI->RLSPI_VERTICAL_AACTIVE = init_struct->VSA + init_struct->VBP + init_struct->height;
    RAMLESS_QSPI->RLSPI_VERTICAL_TOTAL_HEIGHT = init_struct->VSA + init_struct->VBP +
                                                init_struct->height + init_struct->VFP;
    RAMLESS_QSPI->RLSPI_HORIZONTAL_WIDTH = init_struct->width;
    RAMLESS_QSPI->RLSPI_VSYNC_CMD = init_struct->VSYNC_CMD;
    RAMLESS_QSPI->RLSPI_VSYNC_CMD_ADDR = init_struct->VSYNC_CMD_ADDR;
    RAMLESS_QSPI->RLSPI_HSYNC_CMD_VFP = init_struct->HSYNC_CMD_VFP;
    RAMLESS_QSPI->RLSPI_HSYNC_CMD_ADDR_VFP = init_struct->HSYNC_CMD_VFP_ADDR;
    RAMLESS_QSPI->RLSPI_HSYNC_CMD_VBPORCH = init_struct->HSYNC_CMD_VBP;
    RAMLESS_QSPI->RLSPI_HSYNC_CMD_ADDR_VBPORCH = init_struct->HSYNC_CMD_VBP_ADDR;
    RAMLESS_QSPI->RLSPI_HSYNC_CMD_VACTIVE = init_struct->HSYNC_CMD_VACTIVE;
    RAMLESS_QSPI->RLSPI_HSYNC_CMD_ADDR_VACTIVE = init_struct->HSYNC_CMD_VACTIVE_ADDR;
//    LCDC_HANDLER->TEAR_CTR = ((LCDC_HANDLER->TEAR_CTR & LCDC_TEAR_INPUT_MUX_CLR) | init_struct->tear_input_mux);
}

void RLSPI_Cmd(FunctionalState state)
{

    if (state == ENABLE)
    {
        LCDC_HANDLER_TEAR_CTR_TypeDef lcdc_reg_0x10 = {.d32 = LCDC_HANDLER->TEAR_CTR};
        lcdc_reg_0x10.b.tear_auto_turn_on_dma_en = 1;
        lcdc_reg_0x10.b.tear_auto_turn_on_autowritestart = 1;
        LCDC_HANDLER->TEAR_CTR = lcdc_reg_0x10.d32;
    }
    else
    {
        LCDC_HANDLER_TEAR_CTR_TypeDef lcdc_reg_0x10 = {.d32 = LCDC_HANDLER->TEAR_CTR};
        lcdc_reg_0x10.b.tear_auto_turn_on_dma_en = 0;
        lcdc_reg_0x10.b.tear_auto_turn_on_autowritestart = 0;
        LCDC_HANDLER->TEAR_CTR = lcdc_reg_0x10.d32;
    }
}

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/