/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "rtl_jdi.h"

/*============================================================================*
 *                          Private Macros
 *============================================================================*/

/*============================================================================*
 *                           Public Functions
 *============================================================================*/

void JDI_Init(JDI_SrtuctInit_TypeDef *JDI_InitStruct)
{
    assert_param(IS_JDI_INPUT_PIXEL_FORMAT(JDI_InitStruct->JDI_Input_Pixel_Format));
    assert_param(IS_JDI_OUT_PIXEL_FORMAT(JDI_InitStruct->JDI_Output_Pixel_Format));
    assert_param(IS_JDI_LINE_ADDR_INCR_TYPE(JDI_InitStruct->JDI_Gate_Line_Address_Incr));
    JDI_Cmd(DISABLE);
    JDI_Wait_Idle();

    JDI_CTL1_TypeDef jdi_reg_0x404 = {.d32 = JDI->CTL1};
    jdi_reg_0x404.b.dummy_bit_for_4bit = JDI_InitStruct->JDI_Dummy_Bit_For_4bit_Output;
    jdi_reg_0x404.b.input_format = JDI_InitStruct->JDI_Input_Pixel_Format;
    jdi_reg_0x404.b.output_format = JDI_InitStruct->JDI_Output_Pixel_Format;
    JDI->CTL1 = jdi_reg_0x404.d32;

    JDI_MODE_SEL_TypeDef jdi_reg_0x408 = {.d32 = JDI->MODE_SEL};
    jdi_reg_0x408.b.mode_select = JDI_InitStruct->JDI_Mode_Select;
    JDI->MODE_SEL = jdi_reg_0x408.d32;

    JDI_DATA_TRANSFER_DUMMY_TypeDef jdi_reg_0x40c = {.d32 = JDI->DATA_TRANSFER_DUMMY};
    jdi_reg_0x40c.b.data_transfer_dummy_end = JDI_InitStruct->JDI_Dummy_Value_Tail;
    jdi_reg_0x40c.b.data_transfer_dummy_mid = JDI_InitStruct->JDI_Dummy_Value_Mid;
    JDI->DATA_TRANSFER_DUMMY = jdi_reg_0x40c.d32;

    JDI_DATA_WRITTEN_HORIZONTAL_TypeDef jdi_reg_0x410 = {.d32 = JDI->DATA_WRITTEN_HORIZONTAL};
    jdi_reg_0x410.b.jdi_mip_lcd_width = JDI_InitStruct->JDI_LCD_Width;
    JDI->DATA_WRITTEN_HORIZONTAL = jdi_reg_0x410.d32;

    JDI_DATA_WRITTEN_VERTICAL_TypeDef jdi_reg_0x414 = {.d32 = JDI->DATA_WRITTEN_VERTICAL};
    jdi_reg_0x414.b.refresh_line_addr_incr = JDI_InitStruct->JDI_Gate_Line_Address_Incr;
    if (JDI_InitStruct->JDI_Gate_Line_Address_Incr_Num > 7)
    {
        jdi_reg_0x414.b.refresh_line_addr_incr_num = 7;
    }
    else
    {
        jdi_reg_0x414.b.refresh_line_addr_incr_num = JDI_InitStruct->JDI_Gate_Line_Address_Incr_Num;
    }
    jdi_reg_0x414.b.refresh_line_num = JDI_InitStruct->JDI_Refresh_Line_Num;
    jdi_reg_0x414.b.refresh_start_line_addr = JDI_InitStruct->JDI_Start_Gate_Line_Address;
    JDI->DATA_WRITTEN_VERTICAL = jdi_reg_0x414.d32;

    JDI_RX_FIFO_DMA_THRESHOLD_TypeDef jdi_reg_0x430 = {.d32 = JDI->RX_FIFO_DMA_THRESHOLD};
    jdi_reg_0x430.b.rx_dma_enable = JDI_InitStruct->RX_DMA_Enable;
    jdi_reg_0x430.b.rx_fifo_dma_threshold = JDI_InitStruct->RX_FIFO_DMA_Threshold;
    JDI->RX_FIFO_DMA_THRESHOLD = jdi_reg_0x430.d32;

    JDI_TX_FIFO_DMA_THRESHOLD_TypeDef jdi_reg_0x434 = {.d32 = JDI->TX_FIFO_DMA_THRESHOLD};
    jdi_reg_0x434.b.tx_dma_enable = JDI_InitStruct->TX_DMA_Enable;
    jdi_reg_0x434.b.tx_fifo_dma_threshold = JDI_InitStruct->TX_FIFO_DMA_Threshold;
    JDI->TX_FIFO_DMA_THRESHOLD = jdi_reg_0x434.d32;

    JDI_RX_FIFO_INT_THRESHOLD_TypeDef jdi_reg_0x438 = {.d32 = JDI->RX_FIFO_INT_THRESHOLD};
    jdi_reg_0x438.b.rx_fifo_int_threshold = JDI_InitStruct->RX_FIFO_Int_Threshold;
    JDI->RX_FIFO_INT_THRESHOLD = jdi_reg_0x438.d32;

    JDI_TX_FIFO_INT_THRESHOLD_TypeDef jdi_reg_0x43c = {.d32 = JDI->TX_FIFO_INT_THRESHOLD};
    jdi_reg_0x43c.b.tx_fifo_int_threshold = JDI_InitStruct->TX_FIFO_Int_Threshold;
    JDI->TX_FIFO_INT_THRESHOLD = jdi_reg_0x43c.d32;
}

void JDI_Structure_Init(JDI_SrtuctInit_TypeDef *JDI_InitStruct)
{
    JDI_InitStruct->JDI_Dummy_Bit_For_4bit_Output = 0;
    JDI_InitStruct->JDI_Dummy_Value_Mid = 0;
    JDI_InitStruct->JDI_Dummy_Value_Tail = 0;
    JDI_InitStruct->JDI_Gate_Line_Address_Incr = JDI_LINE_ADDR_FIXED;
    JDI_InitStruct->JDI_Gate_Line_Address_Incr_Num = 0;
    JDI_InitStruct->JDI_Refresh_Line_Num = 0;
    JDI_InitStruct->JDI_Input_Pixel_Format = JDI_IN_RGB565;
    JDI_InitStruct->JDI_Output_Pixel_Format = JDI_OUT_3BIT;
    JDI_InitStruct->JDI_LCD_Width = 0;
    JDI_InitStruct->JDI_LCD_Height = 0;
    JDI_InitStruct->JDI_Mode_Select = 0;
    JDI_InitStruct->JDI_Start_Gate_Line_Address = 0;
    JDI_InitStruct->RX_DMA_Enable = 0;
    JDI_InitStruct->TX_DMA_Enable = 0;
    JDI_InitStruct->RX_FIFO_DMA_Threshold = JDI_RX_FIFO_DEPTH / 2;
    JDI_InitStruct->TX_FIFO_DMA_Threshold = JDI_TX_FIFO_DEPTH / 2;
    JDI_InitStruct->RX_FIFO_Int_Threshold = JDI_RX_FIFO_DEPTH / 2;
    JDI_InitStruct->TX_FIFO_Int_Threshold = JDI_TX_FIFO_DEPTH / 2;
}

void JDI_Cmd(FunctionalState NewState)
{
    JDI_CTL0_TypeDef jdi_reg_0x400 = {.d32 = JDI->CTL0};
    if (NewState == ENABLE)
    {
        jdi_reg_0x400.b.rt_jdi_mip_start = 1;
    }
    else
    {
        jdi_reg_0x400.b.rt_jdi_mip_start = 0;
    }
    JDI->CTL0 = jdi_reg_0x400.d32;
}

void JDI_Wait_Idle(void)
{
    JDI_CTL0_TypeDef jdi_reg_0x400 = {.d32 = JDI->CTL0};
    while (jdi_reg_0x400.b.rt_jdi_mip_start)
    {
        jdi_reg_0x400.d32 = JDI->CTL0;
    }
}

void JDI_Interrupt_Enable(JDI_INTERRUPT_TYPE Interrupt, FunctionalState NewState)
{
    assert_param(IS_JDI_INT(Interrupt));
    uint32_t new_value;
    if (NewState == ENABLE)
    {
        new_value = 1;
    }
    else
    {
        new_value = 0;
    }
    JDI_INT_ENABLE_TypeDef jdi_reg_0x440 = {.d32 = JDI->INT_ENABLE};
    switch (Interrupt)
    {
    case JDI_TX_FIFO_UNDERFLOW_INT:
        jdi_reg_0x440.b.tx_fifo_underflow_int_en = new_value;
        break;
    case JDI_TX_FIFO_THRESHOLD_INT:
        jdi_reg_0x440.b.tx_fifo_threshold_int_en = new_value;
        break;
    case JDI_RX_FIFO_OVERFLOW_INT:
        jdi_reg_0x440.b.rx_fifo_overflow_int_en = new_value;
        break;
    case JDI_RX_FIFO_THRESHOLD_INT:
        jdi_reg_0x440.b.rx_fifo_threshold_int_en = new_value;
        break;
    case JDI_OUTPUT_FINISH_INT:
        jdi_reg_0x440.b.output_finish_int_en = new_value;
        break;
    }
    JDI->INT_ENABLE = jdi_reg_0x440.d32;
}

void JDI_Interrupt_Mask(JDI_INTERRUPT_TYPE Interrupt, FunctionalState NewState)
{
    assert_param(IS_JDI_INT(Interrupt));
    uint32_t new_value;
    if (NewState == ENABLE)
    {
        new_value = 1;
    }
    else
    {
        new_value = 0;
    }
    JDI_INT_MASK_TypeDef jdi_reg_0x444 = {.d32 = JDI->INT_MASK};
    switch (Interrupt)
    {
    case JDI_TX_FIFO_UNDERFLOW_INT:
        jdi_reg_0x444.b.tx_fifo_underflow_int_msk = new_value;
        break;
    case JDI_TX_FIFO_THRESHOLD_INT:
        jdi_reg_0x444.b.tx_fifo_threshold_int_msk = new_value;
        break;
    case JDI_RX_FIFO_OVERFLOW_INT:
        jdi_reg_0x444.b.rx_fifo_overflow_int_msk = new_value;
        break;
    case JDI_RX_FIFO_THRESHOLD_INT:
        jdi_reg_0x444.b.rx_fifo_threshold_int_msk = new_value;
        break;
    case JDI_OUTPUT_FINISH_INT:
        jdi_reg_0x444.b.output_finish_int_msk = new_value;
        break;
    }
    JDI->INT_MASK = jdi_reg_0x444.d32;
}

void JDI_Interrupt_Clear(JDI_INTERRUPT_TYPE Interrupt)
{
    assert_param(IS_JDI_INT(Interrupt));
    JDI_INT_CLEAR_TypeDef jdi_reg_0x450 = {.d32 = JDI->INT_CLEAR};
    switch (Interrupt)
    {
    case JDI_TX_FIFO_UNDERFLOW_INT:
        jdi_reg_0x450.b.tx_fifo_underflow_int_clr = 1;
        break;
    case JDI_TX_FIFO_THRESHOLD_INT:
        jdi_reg_0x450.b.tx_fifo_threshold_int_clr = 1;
        break;
    case JDI_RX_FIFO_OVERFLOW_INT:
        jdi_reg_0x450.b.rx_fifo_overflow_int_clr = 1;
        break;
    case JDI_RX_FIFO_THRESHOLD_INT:
        jdi_reg_0x450.b.rx_fifo_threshold_int_clr = 1;
        break;
    case JDI_OUTPUT_FINISH_INT:
        jdi_reg_0x450.b.output_finish_int_clr = 1;
        break;
    }
    JDI->INT_CLEAR = jdi_reg_0x450.d32;
}

ITStatus JDI_Get_Interrupt_Status(JDI_INTERRUPT_TYPE Interrupt)
{
    assert_param(IS_JDI_INT(Interrupt));
    JDI_INT_STATUS_TypeDef jdi_reg_0x44c = {.d32 = JDI->INT_STATUS};
    uint32_t value = 0;
    switch (Interrupt)
    {
    case JDI_TX_FIFO_UNDERFLOW_INT:
        value = jdi_reg_0x44c.b.tx_fifo_underflow_int_status;
        break;
    case JDI_TX_FIFO_THRESHOLD_INT:
        value = jdi_reg_0x44c.b.tx_fifo_threshold_int_status;
        break;
    case JDI_RX_FIFO_OVERFLOW_INT:
        value = jdi_reg_0x44c.b.rx_fifo_overflow_int_status;
        break;
    case JDI_RX_FIFO_THRESHOLD_INT:
        value = jdi_reg_0x44c.b.rx_fifo_threshold_int_status;
        break;
    case JDI_OUTPUT_FINISH_INT:
        value = jdi_reg_0x44c.b.output_finish_int_status;
        break;
    }
    if (value)
    {
        return SET;
    }
    else
    {
        return RESET;
    }
}

ITStatus JDI_Get_Interrupt_Raw_Status(JDI_INTERRUPT_TYPE Interrupt)
{
    assert_param(IS_JDI_INT(Interrupt));
    JDI_INT_RAW_STATUS_TypeDef jdi_reg_0x448 = {.d32 = JDI->INT_RAW_STATUS};
    uint32_t value = 0;
    switch (Interrupt)
    {
    case JDI_TX_FIFO_UNDERFLOW_INT:
        value = jdi_reg_0x448.b.tx_fifo_underflow_int_raw_status;
        break;
    case JDI_TX_FIFO_THRESHOLD_INT:
        value = jdi_reg_0x448.b.tx_fifo_threshold_int_raw_status;
        break;
    case JDI_RX_FIFO_OVERFLOW_INT:
        value = jdi_reg_0x448.b.rx_fifo_overflow_int_raw_status;
        break;
    case JDI_RX_FIFO_THRESHOLD_INT:
        value = jdi_reg_0x448.b.rx_fifo_threshold_int_raw_status;
        break;
    case JDI_OUTPUT_FINISH_INT:
        value = jdi_reg_0x448.b.output_finish_int_raw_status;
        break;
    }
    if (value)
    {
        return SET;
    }
    else
    {
        return RESET;
    }
}
