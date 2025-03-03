/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* \file     rtl_edpi.c
* \brief    This file provides all the eDPI firmware functions.
* \details
* \author   boris yue
* \date     2023-10-17
* \version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "rtl_lcdc_edpi.h"

/*============================================================================*
 *                           Public Functions
 *============================================================================*/
void EDPI_Videocfg()
{

}

void EDPI_Cmdcfg()
{

}

void EDPI_Init(LCDC_eDPICfgTypeDef *eDPICfg)
{
    /* Check the parameters */
    assert_param(IS_EDPI_HSPOL(eDPICfg->eDPI_HoriSyncPolarity));
    assert_param(IS_EDPI_VSPOL(eDPICfg->eDPI_VeriSyncPolarity));
    assert_param(IS_EDPI_DEPOL(eDPICfg->eDPI_DataEnPolarity));
    assert_param(IS_EDPI_PIXELFORMAT(eDPICfg->eDPI_ColorMap));
    assert_param(IS_EDPI_OP_MODE(eDPICfg->eDPI_OperateMode));


    EDPI_DIV_PAR_TypeDef edpi_reg_0x50 = {.d32 = EDPI->EDPI_DIV_PAR};
    edpi_reg_0x50.b.edpi_div_par = eDPICfg->eDPI_ClockDiv;
    EDPI->EDPI_DIV_PAR = edpi_reg_0x50.d32;

    EDPI_SYNC_WIDTH_TypeDef edpi_reg_0x04 = {.d32 = EDPI->EDPI_SYNC_WIDTH};
    edpi_reg_0x04.b.hsw = eDPICfg->eDPI_HoriSyncWidth;
    edpi_reg_0x04.b.vsh = eDPICfg->eDPI_VeriSyncHeight;
    EDPI->EDPI_SYNC_WIDTH = edpi_reg_0x04.d32;

    EDPI_ABACK_PORCH_TypeDef edpi_reg_0x08 = {.d32 = EDPI->EDPI_ABACK_PORCH};
    edpi_reg_0x08.b.ahbp = eDPICfg->eDPI_AccumulatedHBP;
    edpi_reg_0x08.b.avbp = eDPICfg->eDPI_AccumulatedVBP;
    EDPI->EDPI_ABACK_PORCH = edpi_reg_0x08.d32;

    EDPI_AACTIVE_TypeDef edpi_reg_0x0c = {.d32 = EDPI->EDPI_AACTIVE};
    edpi_reg_0x0c.b.aaw = eDPICfg->eDPI_AccumulatedActiveW;
    edpi_reg_0x0c.b.aah = eDPICfg->eDPI_AccumulatedActiveH;
    EDPI->EDPI_AACTIVE = edpi_reg_0x0c.d32;

    EDPI_TOTAL_TypeDef edpi_reg_0x10 = {.d32 = EDPI->EDPI_TOTAL};
    edpi_reg_0x10.b.totalh = eDPICfg->eDPI_TotalHeight;
    edpi_reg_0x10.b.totalw = eDPICfg->eDPI_TotalWidth;
    EDPI->EDPI_TOTAL = edpi_reg_0x10.d32;

    EDPI_SYNC_POL_TypeDef edpi_reg_0x14 = {.d32 = EDPI->EDPI_SYNC_POL};
    edpi_reg_0x14.b.hspol = eDPICfg->eDPI_HoriSyncPolarity;
    edpi_reg_0x14.b.depol = eDPICfg->eDPI_DataEnPolarity;
    edpi_reg_0x14.b.vspol = eDPICfg->eDPI_VeriSyncPolarity;
    EDPI->EDPI_SYNC_POL = edpi_reg_0x14.d32;

    EDPI_COLOR_MAP_TypeDef edpi_reg_0x30 = {.d32 = EDPI->EDPI_COLOR_MAP};
    edpi_reg_0x30.b.color_map = eDPICfg->eDPI_ColorMap;
    EDPI->EDPI_COLOR_MAP = edpi_reg_0x30.d32;

    if (eDPICfg->eDPI_OperateMode == EDPI_ADAPTED_COMMAND_MODE)
    {
        EDPI_CMD_CTL_TypeDef edpi_reg_0x3c = {.d32 = EDPI->EDPI_CMD_CTL};
        edpi_reg_0x3c.b.tear_req = eDPICfg->eDPI_TearReq;
        EDPI->EDPI_CMD_CTL = edpi_reg_0x3c.d32;

        EDPI_CMD_MAX_LATENCY_TypeDef edpi_reg_0x40 = {.d32 = EDPI->EDPI_CMD_MAX_LATENCY};
        edpi_reg_0x40.b.max_latency = eDPICfg->eDPI_CmdMaxLatency;
        EDPI->EDPI_CMD_MAX_LATENCY = edpi_reg_0x40.d32;
    }
    else if (eDPICfg->eDPI_OperateMode == EDPI_STANDARD_VIDEO_MODE)
    {
        EDPI_VIDEO_CTL_TypeDef edpi_reg_0x38 = {.d32 = EDPI->EDPI_VIDEO_CTL};
        edpi_reg_0x38.b.lcd_arc = eDPICfg->eDPI_LcdArc;
        edpi_reg_0x38.b.sd_pol = eDPICfg->eDPI_ShutdnPolarity;
        edpi_reg_0x38.b.sd_en = eDPICfg->eDPI_ShutdnEn;
        edpi_reg_0x38.b.clm_en = eDPICfg->eDPI_ColorModeEn;
        edpi_reg_0x38.b.up_en = eDPICfg->eDPI_UpdateCfgEn;
        EDPI->EDPI_VIDEO_CTL = edpi_reg_0x38.d32;
    }
    EDPI_OP_MODE_TypeDef edpi_reg_0x34 = {.d32 = EDPI->EDPI_OP_MODE};
    edpi_reg_0x34.b.op_mode = eDPICfg->eDPI_OperateMode;
    EDPI->EDPI_OP_MODE = edpi_reg_0x34.d32;

    EDPI_LINE_BUFFER_PIXEL_THRESHOLD_TypeDef edpi_reg_0x48 = {.d32 = EDPI->DPI_LINE_BUFFER_PIXEL_THRESHOLD};
    edpi_reg_0x48.b.line_buffer_pixel_threshold = eDPICfg->eDPI_LineBufferPixelThreshold;
    EDPI->DPI_LINE_BUFFER_PIXEL_THRESHOLD = edpi_reg_0x48.d32;
}

ITStatus EDPI_GetLineINTStatus(void)
{
    ITStatus bit_status = RESET;
    EDPI_INT_FLAG_TypeDef edpi_reg_0x1c = {.d32 = EDPI->EDPI_INT_FLAG};
    if (edpi_reg_0x1c.b.lif)
    {
        bit_status = SET;
    }

    return  bit_status;
}

ITStatus EDPI_GetSyncStatus(EDPI_SIGNAL_t signal)
{
    ITStatus bit_status = RESET;
    EDPI_SYNC_STATUS_TypeDef edpi_reg_0x2c = {.d32 = EDPI->EDPI_SYNC_STATUS};
    switch (signal)
    {
    case EDPI_HSYNCS:
        if (edpi_reg_0x2c.b.hsyncs)
        {
            bit_status = SET;
        }
        break;
    case EDPI_VSYNCS:
        if (edpi_reg_0x2c.b.vsyncs)
        {
            bit_status = SET;
        }
        break;
    case EDPI_VDES:
        if (edpi_reg_0x2c.b.vdes)
        {
            bit_status = SET;
        }
        break;
    case EDPI_HDES:
        if (edpi_reg_0x2c.b.hdes)
        {
            bit_status = SET;
        }
        break;
    }

    /* Return the EDPI_SIGNAL status */
    return  bit_status;
}

void EDPI_MaskLineINTConfig(FunctionalState state)
{
    assert_param(IS_FUNCTIONAL_STATE(state));
    EDPI_INT_MASK_TypeDef edpi_reg_0x18 = {.d32 = EDPI->EDPI_INT_MASK};
    if (state == ENABLE)
    {
        edpi_reg_0x18.b.lim = 1;
    }
    else
    {
        edpi_reg_0x18.b.lim = 0;
    }
    EDPI->EDPI_INT_MASK = edpi_reg_0x18.d32;
}

void LCDC_ClearLineINTPendingBit(void)
{
    EDPI_INT_CLR_TypeDef edpi_reg_0x20 = {.d32 = EDPI->EDPI_INT_CLR};
    edpi_reg_0x20.b.clif = 0;
    EDPI->EDPI_INT_CLR = edpi_reg_0x20.d32;
}

uint16_t EDPI_GetLineINTPos(void)
{
    EDPI_LINE_INT_POS_TypeDef edpi_reg_0x24 = {.d32 = EDPI->EDPI_LINE_INT_POS};
    return edpi_reg_0x24.b.lipos;
}

uint16_t EDPI_GetXPos(void)
{
    EDPI_PIXEL_POS_TypeDef edpi_reg_0x28 = {.d32 = EDPI->EDPI_PIXEL_POS};
    return edpi_reg_0x28.b.cxpos;
}

uint16_t EDPI_GetYPos(void)
{
    EDPI_PIXEL_POS_TypeDef edpi_reg_0x28 = {.d32 = EDPI->EDPI_PIXEL_POS};
    return edpi_reg_0x28.b.cypos;
}

void EDPI_OPMODE_CONFIG(uint32_t mode)
{
    assert_param(IS_EDPI_OP_MODE(mode));
    EDPI_OP_MODE_TypeDef edpi_reg_0x34 = {.d32 = EDPI->EDPI_OP_MODE};
    edpi_reg_0x34.b.op_mode = mode;
    EDPI->EDPI_OP_MODE = edpi_reg_0x34.d32;
}

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/

