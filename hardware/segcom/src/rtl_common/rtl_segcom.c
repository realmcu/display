/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* \file     rtl_segcom.c
* \brief    This file provides all the SEG_COM controller firmware functions.
* \details
* \author   astor zhang
* \date     2023-10-17
* \version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "rtl_segcom.h"
#include "aon_reg.h"

/*============================================================================*
 *                           Public Functions
 *============================================================================*/
void SEGCOM_clk_config(FunctionalState state)
{
    uint32_t *reg = (uint32_t *)(0x40002000 + 0x31C);
    uint32_t reg_value = *reg;
    if (state)
    {
        reg_value |= (BIT19 | BIT18);
        uint32_t *LDO_reg = (uint32_t *)0x40000C14;
        *LDO_reg |= BIT7;

        uint32_t *reg = (uint32_t *)0x40000800;
        *reg &= (~BIT10);
        *reg |= BIT9;

        uint32_t *LCD_reg = (uint32_t *)0x40000C18;
        *LCD_reg |= BIT1;
    }
    else
    {
        reg_value &= (~(BIT19 | BIT18));
    }
    *reg = reg_value;
}

void SEGCOM_UpdateClk(void)
{
    SEGCOM_LCD_CR_TypeDef segcom_reg_0x00 = {.d32 = SEGCOM->LCD_CR};
    segcom_reg_0x00.b.update = 1;
    SEGCOM->LCD_CR = segcom_reg_0x00.d32;
}

void SEGCOM_CMD(FunctionalState state)
{
    SEGCOM_LCD_CR_TypeDef segcom_reg_0x00 = {.d32 = SEGCOM->LCD_CR};
    if (state)
    {
        segcom_reg_0x00.b.en_lcd = 1;
    }
    else
    {
        segcom_reg_0x00.b.en_lcd = 0;
    }
    SEGCOM->LCD_CR = segcom_reg_0x00.d32;
}

void SEGCOM_StructInit(SEGCOM_InitTypeDef *cfg)
{
    assert_param(cfg);
    cfg->SEG_bias = BIAS_STATIC;
    cfg->SEG_duty = DUTY_STATIC;
    cfg->SEG_blink_mode = BLINK_DISABLE;
    cfg->SEG_blink_freq = BLINK_FREQ_8;
    cfg->SEG_dead_time = DEAD_TIME_NONE;
    cfg->SEG_set_manual_mode = SEGCOM_LOW_DRIVING;
    cfg->SEG_driv_ctl_mode = SEGCOM_AUTO_MODE;
    cfg->SEG_high_driv_duration = HIGH_DURATION_ALL;
    cfg->SEG_clk_divider = CLK_DIV_16;
    cfg->SEG_clk_prescaler = CLK_PRESCALE_1;
}

void SEGCOM_IntConfig(SEGCOM_INTERRUPT interrupt, FunctionalState state)
{
    assert_param(IS_SEGCOM_INT(interrupt));
    SEGCOM_LCD_FCR_TypeDef segcom_reg_0x04 = {.d32 = SEGCOM->LCD_FCR};
    if (state)
    {
        if (interrupt == SEGCOM_UPDATE_DISPLAY_DONE_INT)
        {
            segcom_reg_0x04.b.uddie = ENABLE;
        }
        if (interrupt == SEGCOM_START_OF_FRAME_INT)
        {
            segcom_reg_0x04.b.sofie = ENABLE;
        }
    }
    else
    {
        if (interrupt == SEGCOM_UPDATE_DISPLAY_DONE_INT)
        {
            segcom_reg_0x04.b.uddie = DISABLE;
        }
        if (interrupt == SEGCOM_START_OF_FRAME_INT)
        {
            segcom_reg_0x04.b.sofie = DISABLE;
        }
    }
    SEGCOM->LCD_FCR = segcom_reg_0x04.d32;
}

uint32_t SEGCOM_GetStatus(SEGCOM_STATUS flag)
{
    SEGCOM_LCD_SR_TypeDef segcom_reg_0x08 = {.d32 = SEGCOM->LCD_SR};
    uint32_t status = (segcom_reg_0x08.d32 & (1 << flag));
    return status;
}

void SEGCOM_ClearStatus(SEGCOM_CLEAR_STATUS status)
{
    SEGCOM_LCD_SR_TypeDef segcom_reg_0x08 = {.d32 = SEGCOM->LCD_SR};
    if (status == SEGCOM_CLEAR_START_OF_FRAME)
    {
        segcom_reg_0x08.b.sofc = 1;
    }
    else if (status == SEGCOM_CLEAR_UPDATE_DISPLAY_DONE)
    {
        segcom_reg_0x08.b.uddc = 1;
    }
    SEGCOM->LCD_SR = segcom_reg_0x08.d32;
}

void SEGCOM_UpdateDisplayRequest(void)
{
    SEGCOM_LCD_SR_TypeDef segcom_reg_0x08 = {.d32 = SEGCOM->LCD_SR};
    segcom_reg_0x08.b.udr = ENABLE;
    SEGCOM->LCD_SR = segcom_reg_0x08.d32;
}

void SEGCOM_Write(SEGCOM_SEG_CHANNEL channel, uint32_t value)
{
    assert_param(channel <= SEGCOM_DR_3);
    switch (channel)
    {
    case SEGCOM_DR_0:
        SEGCOM->LCD_RAM_COM0 = value;
        break;
    case SEGCOM_DR_1:
        SEGCOM->LCD_RAM_COM1 = value;
        break;
    case SEGCOM_DR_2:
        SEGCOM->LCD_RAM_COM2 = value;
        break;
    case SEGCOM_DR_3:
        SEGCOM->LCD_RAM_COM3 = value;
    }
}

void SEGCOM_Pin_Init(SEGCOM_com_pin_cfg_Typedef com_cfg, SEGCOM_seg_pin_cfg_Typedef seg_cfg)
{
    AON_NS_P3_P1_PAD_E_LCD_CFG_TYPE aon_reg_0x1d80 = {.d32 = *(uint32_t *)0x40001d80};
    AON_NS_P3_P1_PAD_SHDN_LCD_CFG_TYPE aon_reg_0x1d8c = {.d32 = *(uint32_t *)0x40001d8c};
    aon_reg_0x1d80.PAD_P0_E_LCD_7_0 = (com_cfg.pin.com2_p_0_0 | (com_cfg.pin.com1_p_0_1 << 1) | \
                                       (com_cfg.pin.com0_p_0_2 << 2) | (seg_cfg.pin.seg4_p_0_4 << 4) | (seg_cfg.pin.seg2_p_0_5 << 5) | \
                                       (seg_cfg.pin.seg1_p_0_6 << 6) | (seg_cfg.pin.seg0_p_0_7 << 7));
    aon_reg_0x1d8c.PAD_P0_SHDN_LCD_7_0 = aon_reg_0x1d80.PAD_P0_E_LCD_7_0;
    aon_reg_0x1d80.PAD_P1_E_LCD_7_2 = (seg_cfg.pin.seg3_p_1_2 | (seg_cfg.pin.seg26_p_1_3 << 1) | \
                                       (seg_cfg.pin.seg25_p_1_4 << 2) | (seg_cfg.pin.seg5_p_1_5 << 3) | (seg_cfg.pin.seg6_p_1_6 << 4) | \
                                       (seg_cfg.pin.seg7_p_1_7 << 5));
    aon_reg_0x1d8c.PAD_P1_SHDN_LCD_7_2 = aon_reg_0x1d80.PAD_P1_E_LCD_7_2;
    aon_reg_0x1d80.PAD_P3_E_LCD_7_2 = (com_cfg.pin.com3_p_3_2 | (seg_cfg.pin.seg11_p_3_3 << 1) | \
                                       (seg_cfg.pin.seg10_p_3_4 << 2) | (seg_cfg.pin.seg9_p_3_5 << 3) | (seg_cfg.pin.seg8_p_3_6 << 4) | \
                                       (seg_cfg.pin.seg13_p_3_7 << 5));
    aon_reg_0x1d8c.PAD_P3_SHDN_LCD_7_2 = aon_reg_0x1d80.PAD_P3_E_LCD_7_2;
    *(uint32_t *)0x40001d80 = aon_reg_0x1d80.d32;
    *(uint32_t *)0x40001d8C = aon_reg_0x1d8c.d32;

    AON_NS_P7_P4_PAD_E_LCD_CFG_TYPE aon_reg_0x1d84 = {.d32 = *(uint32_t *)0x40001d84};
    AON_NS_P7_P4_PAD_SHDN_LCD_CFG_TYPE aon_reg_0x1d90 = {.d32 = *(uint32_t *)0x40001d90};
    aon_reg_0x1d84.PAD_P4_E_LCD_7_0 = (seg_cfg.pin.seg14_p_4_0 | (seg_cfg.pin.seg15_p_4_1 << 1) | \
                                       (seg_cfg.pin.seg16_p_4_2 << 2) | (seg_cfg.pin.seg17_p_4_3 << 3) | (seg_cfg.pin.seg24_p_4_4 << 4) | \
                                       (seg_cfg.pin.seg23_p_4_5 << 5) | (seg_cfg.pin.seg22_p_4_6 << 6) | (seg_cfg.pin.seg21_p_4_7 << 7));
    aon_reg_0x1d90.PAD_P4_SHDN_LCD_7_0 = aon_reg_0x1d84.PAD_P4_E_LCD_7_0;
    aon_reg_0x1d84.PAD_P5_E_LCD_5_0 = (seg_cfg.pin.seg27_p_5_0 | (seg_cfg.pin.seg28_p_5_1 << 1) | \
                                       (seg_cfg.pin.seg29_p_5_2 << 2) | (seg_cfg.pin.seg30_p_5_3 << 3) | (seg_cfg.pin.seg31_p_5_4 << 4) | \
                                       (seg_cfg.pin.seg12_p_5_5 << 5));
    aon_reg_0x1d90.PAD_P5_SHDN_LCD_5_0 = aon_reg_0x1d84.PAD_P5_E_LCD_5_0;
    *(uint32_t *)0x40001d84 = aon_reg_0x1d84.d32;
    *(uint32_t *)0x40001d90 = aon_reg_0x1d90.d32;

    AON_NS_P11_P8_PAD_E_LCD_CFG_TYPE aon_reg_0x1d88 = {.d32 = *(uint32_t *)0x40001d88};
    AON_NS_P11_P8_PAD_SHDN_LCD_CFG_TYPE aon_reg_0x1d94 = {.d32 = *(uint32_t *)0x40001d94};
    aon_reg_0x1d88.PAD_P9_E_LCD_2_0 = (seg_cfg.pin.seg18_p_9_0 | (seg_cfg.pin.seg19_p_9_1 << 1) | \
                                       (seg_cfg.pin.seg20_p_9_2 << 2));
    aon_reg_0x1d94.PAD_P9_SHDN_LCD_2_0 = aon_reg_0x1d88.PAD_P9_E_LCD_2_0;
    *(uint32_t *)0x40001d88 = aon_reg_0x1d88.d32;
    *(uint32_t *)0x40001d94 = aon_reg_0x1d94.d32;
}

void SEGCOM_init(SEGCOM_InitTypeDef *cfg)
{
    SEGCOM_LCD_CR_TypeDef segcom_reg_0x00 = {.d32 = SEGCOM->LCD_CR};
    segcom_reg_0x00.b.duty = cfg->SEG_duty;
    segcom_reg_0x00.b.bias = cfg->SEG_bias;
    SEGCOM->LCD_CR = segcom_reg_0x00.d32;

    SEGCOM_LCD_FCR_TypeDef segcom_reg_0x04 = {.d32 = SEGCOM->LCD_FCR};
    segcom_reg_0x04.b.blink = cfg->SEG_blink_mode;
    segcom_reg_0x04.b.dead = cfg->SEG_dead_time;
    segcom_reg_0x04.b.div = cfg->SEG_clk_divider;
    segcom_reg_0x04.b.ps = cfg->SEG_clk_prescaler;
    segcom_reg_0x04.b.en_res_sel = cfg->SEG_driv_ctl_mode;
    segcom_reg_0x04.b.res_l_en_set = cfg->SEG_set_manual_mode;
    segcom_reg_0x04.b.pon = cfg->SEG_high_driv_duration;
    segcom_reg_0x04.b.blinkf = cfg->SEG_blink_freq;
    if (cfg->SEG_driv_ctl_mode == SEGCOM_AUTO_MODE)
    {
        segcom_reg_0x04.b.pon = cfg->SEG_high_driv_duration;
    }
    SEGCOM->LCD_FCR = segcom_reg_0x04.d32;
}

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/
