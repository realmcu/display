/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "board.h"
#include "app_section.h"
#include "rtl876x_spi.h"
#include "trace.h"
#include "lcd_nv3023b_132x162_spi.h"
#include "platform_utils.h"
#include "rtl876x_rcc.h"
#include "rtl876x_tim.h"
#include "rtl876x_gpio.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gdma.h"
#include "rtl876x_nvic.h"
#include "flash_device.h"
#include "trace.h"

bool te_ready;
bool allowed_lcd_backlight_enter_dlps;
/* Config PWM_PERIOD and PWM_DUTY_CYCLE */
#define PWM_PERIOD              1000000 //uint:us
#define PWM_DUTY_CYCLE          50      //uint:percent
/* PWM_HIGH_COUNT =  */
#define PWM_HIGH_COUNT          ((((PWM_PERIOD)*(PWM_DUTY_CYCLE*40))/100)-1)    //PWM CLOCK = 40000000
#define PWM_LOW_COUNT           ((((PWM_PERIOD)*((100-PWM_DUTY_CYCLE)*40))/100)-1)
void lcd_set_backlight(uint32_t percent)
{
    Pad_Config(LCD_SPI_BL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    if (percent != 0)
    {
        TIM_Cmd(BL_PWM_TIM, DISABLE);
        TIM_PWMChangeFreqAndDuty(BL_PWM_TIM, (percent) * 10, (100 - percent) * 10);
        TIM_Cmd(BL_PWM_TIM, ENABLE);
        Pad_Config(LCD_SPI_BL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
        allowed_lcd_backlight_enter_dlps = false;
    }
    else
    {
        TIM_Cmd(BL_PWM_TIM, DISABLE);
        Pad_Config(LCD_SPI_BL, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
        allowed_lcd_backlight_enter_dlps = true;
    }
}
DATA_RAM_FUNCTION
void LCD_NV3023_CMD(uint8_t command)
{
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_BUSY));
    /* switch to CS0 signal */
    SPI_SetCSNumber(LCD_SPI_BUS, 0);
    SPI_Cmd(LCD_SPI_BUS, DISABLE);
    SPI_Change_CLK(LCD_SPI_BUS, 2);
    SPI_Cmd(LCD_SPI_BUS, ENABLE);
    GPIO_ResetBits(LCD_SPI_DC_PIN);
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_TFE) == false);
    SPI_SendData(LCD_SPI_BUS, command);

    /* Waiting for SPI data transfer to end */
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_BUSY));
    GPIO_SetBits(LCD_SPI_DC_PIN);
}

DATA_RAM_FUNCTION
void LCD_NV3023_Parameter(uint8_t data)
{
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_TFE) == false);
    SPI_SendData(LCD_SPI_BUS, data);

    /* Waiting for SPI data transfer to end */
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_BUSY)); //Howie todo, remove later
}

void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd)
{
    /*this is a bug here if lcd size bigger than uint8_t*/

    LCD_NV3023_CMD(0x2A); //Set Column Address
    while (!(LCD_SPI_BUS->SR & BIT(1)));
    LCD_SPI_BUS->DR[0] = xStart >> 8;
    while (!(LCD_SPI_BUS->SR & BIT(1)));
    LCD_SPI_BUS->DR[0] = xStart;
    while (!(LCD_SPI_BUS->SR & BIT(1)));
    LCD_SPI_BUS->DR[0] = xEnd >> 8;
    while (!(LCD_SPI_BUS->SR & BIT(1)));
    LCD_SPI_BUS->DR[0] = xEnd;

    LCD_NV3023_CMD(0x2B); //Set Page Address

    LCD_SPI_BUS->DR[0] = yStart >> 8;
    while (!(LCD_SPI_BUS->SR & BIT(1)));
    LCD_SPI_BUS->DR[0] = yStart;
    while (!(LCD_SPI_BUS->SR & BIT(1)));
    LCD_SPI_BUS->DR[0] = yEnd >> 8;
    while (!(LCD_SPI_BUS->SR & BIT(1)));
    LCD_SPI_BUS->DR[0] = yEnd;

    LCD_NV3023_CMD(0x2C);

}



void lcd_st7789_init(void)
{





    //----------------Star Initial Sequence-------//
    LCD_NV3023_CMD(0xff);
    LCD_NV3023_Parameter(0xa5);
    LCD_NV3023_CMD(0x3E);
    LCD_NV3023_Parameter(0x08);
    LCD_NV3023_CMD(0x3A);
    LCD_NV3023_Parameter(0x55);
    LCD_NV3023_CMD(0x82);
    LCD_NV3023_Parameter(0x00);
    LCD_NV3023_CMD(0x98);
    LCD_NV3023_Parameter(0x00);
    LCD_NV3023_CMD(0x63);
    LCD_NV3023_Parameter(0x0f);
    LCD_NV3023_CMD(0x64);
    LCD_NV3023_Parameter(0x0f);
    LCD_NV3023_CMD(0xB4);
    LCD_NV3023_Parameter(0x24);
    LCD_NV3023_CMD(0xB5);
    LCD_NV3023_Parameter(0x30);
    LCD_NV3023_CMD(0x83);
    LCD_NV3023_Parameter(0x03);
    LCD_NV3023_CMD(0x86);//
    LCD_NV3023_Parameter(0x04);
    LCD_NV3023_CMD(0x86);//frc
    LCD_NV3023_Parameter(0x04);
    LCD_NV3023_CMD(0x87);
    LCD_NV3023_Parameter(0x16);
    LCD_NV3023_CMD(0x88);//VCOMH
    LCD_NV3023_Parameter(0x34);
    LCD_NV3023_CMD(0x89);//
    LCD_NV3023_Parameter(0x2f);//2F
    LCD_NV3023_CMD(0x93); //
    LCD_NV3023_Parameter(0x63);
    LCD_NV3023_CMD(0x96);
    LCD_NV3023_Parameter(0x81);
    LCD_NV3023_CMD(0xC3);
    LCD_NV3023_Parameter(0x11);
    LCD_NV3023_CMD(0xE6);
    LCD_NV3023_Parameter(0x00);
////////////////////////gamma_set//////////////////////////////////////
    LCD_NV3023_CMD(0x70); LCD_NV3023_Parameter(0x07); //VRP 0 1
    LCD_NV3023_CMD(0x71); LCD_NV3023_Parameter(0x2B); //VRP 1 3
    LCD_NV3023_CMD(0x72); LCD_NV3023_Parameter(0x1A); //VRP 2 7
//24
//Newvision NV3023B
//25
    LCD_NV3023_CMD(0x73); LCD_NV3023_Parameter(0x12); //VRP 3 9
    LCD_NV3023_CMD(0x74); LCD_NV3023_Parameter(0x18); //VRP 6 11
    LCD_NV3023_CMD(0x75); LCD_NV3023_Parameter(0x1b); //VRP 8 13
    LCD_NV3023_CMD(0x76); LCD_NV3023_Parameter(0x4B); //VRP 10 5
    LCD_NV3023_CMD(0x77); LCD_NV3023_Parameter(0x06); //VRP 14 15
    LCD_NV3023_CMD(0x78); LCD_NV3023_Parameter(0x04); //VRP 17 16
    LCD_NV3023_CMD(0x79); LCD_NV3023_Parameter(0x49); //VRP 21 6
    LCD_NV3023_CMD(0x7a); LCD_NV3023_Parameter(0x04); //VRP 23 14
    LCD_NV3023_CMD(0x7b); LCD_NV3023_Parameter(0x08); //VRP 25 12
    LCD_NV3023_CMD(0x7c); LCD_NV3023_Parameter(0x0E); //VRP 28 10
    LCD_NV3023_CMD(0x7d); LCD_NV3023_Parameter(0x0C); //VRP 29 8
    LCD_NV3023_CMD(0x7e); LCD_NV3023_Parameter(0x1C); //VRP 30 4
    LCD_NV3023_CMD(0x7f); LCD_NV3023_Parameter(0x08); //VRP 31 2
    LCD_NV3023_CMD(0xa0); LCD_NV3023_Parameter(0x0b); //VRN 0 1
    LCD_NV3023_CMD(0xa1); LCD_NV3023_Parameter(0x24); //VRN 1 3
    LCD_NV3023_CMD(0xa2); LCD_NV3023_Parameter(0x07); //VRN 2 7
    LCD_NV3023_CMD(0xa3); LCD_NV3023_Parameter(0x09); //VRN 3 9
    LCD_NV3023_CMD(0xa4); LCD_NV3023_Parameter(0x08); //VRN 6 11
    LCD_NV3023_CMD(0xa5); LCD_NV3023_Parameter(0x22); //VRN 8 13
    LCD_NV3023_CMD(0xa6); LCD_NV3023_Parameter(0x2F); //VRN 10 5
    LCD_NV3023_CMD(0xa7); LCD_NV3023_Parameter(0x04); //VRN 14 15
    LCD_NV3023_CMD(0xa8); LCD_NV3023_Parameter(0x06); //VRN 17 16
    LCD_NV3023_CMD(0xa9); LCD_NV3023_Parameter(0x32); //VRN 21 6
    LCD_NV3023_CMD(0xaa); LCD_NV3023_Parameter(0x0D); //VRN 23 14
    LCD_NV3023_CMD(0xab); LCD_NV3023_Parameter(0x13); //VRN 25 12
    LCD_NV3023_CMD(0xac); LCD_NV3023_Parameter(0x10); //VRN 28 10
    LCD_NV3023_CMD(0xad); LCD_NV3023_Parameter(0x08); //VRN 29 8
    LCD_NV3023_CMD(0xae); LCD_NV3023_Parameter(0x39); //VRN 30 4
    LCD_NV3023_CMD(0xaf); LCD_NV3023_Parameter(0x07); //VRN 31 2
//////////////////////////////////////////////////////////////////




}





bool rtl_gui_lcd_te_get_status(void)
{
    return te_ready;
}
//static void lcd_change_te_status(bool status)
//{
//    te_ready = status;
//}



void lcd_pad_init(void)
{
    Pinmux_Config(LCD_SPI_CLK, LCD_SPI_FUNC_CLK);
    Pinmux_Config(LCD_SPI_MOSI, LCD_SPI_FUNC_MOSI);
    Pinmux_Config(LCD_SPI_CS, LCD_SPI_FUNC_CS);

    Pinmux_Config(LCD_SPI_DC, DWGPIO);

    Pad_Config(LCD_SPI_CLK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_DC, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);

    /*BL AND RESET ARE NOT FIX*/
    //Pad_Config(LCD_SPI_BL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
//    Pinmux_Config(LCD_SPI_BL, BL_PWM_TIM);
//Pad_Config(H_2, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(H_2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(LCD_SPI_BL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pinmux_Config(LCD_SPI_BL, timer_pwm4);
}
void lcd_enter_dlps(void)
{
    Pad_Config(LCD_SPI_CLK, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(LCD_SPI_MOSI, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pad_Config(LCD_SPI_CS, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(LCD_SPI_DC, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pad_Config(LCD_SPI_BL, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);
}
void lcd_exit_dlps(void)
{
    Pad_Config(LCD_SPI_CLK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_SPI_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_DC, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);

    Pad_Config(LCD_SPI_BL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_HIGH);
}


/**
  * @brief  writband lcd device init set IO config here
  * @param  None
  * @retval None
  */
void lcd_device_init(void)
{
    lcd_pad_init();

    /* Open timer clock */
    RCC_PeriphClockCmd(APBPeriph_TIMER, APBPeriph_TIMER_CLOCK, ENABLE);

    TIM_TimeBaseInitTypeDef TIM_InitStruct;

    TIM_StructInit(&TIM_InitStruct);

    TIM_InitStruct.TIM_PWM_En = PWM_ENABLE;
    /* Set period */
//    TIM_InitStruct.TIM_Period = 10 - 1 ;
    /* Set PWM high count and low count */
    TIM_InitStruct.TIM_PWM_High_Count = PWM_HIGH_COUNT ;
    TIM_InitStruct.TIM_PWM_Low_Count = PWM_LOW_COUNT ;
    /* Set timer mode User_define */
    TIM_InitStruct.TIM_Mode = TIM_Mode_UserDefine;
    /* Set timer clock source divider 40, timer pclk = 40M/4 */
    TIM_InitStruct.TIM_SOURCE_DIV = TIM_CLOCK_DIVIDER_40;
    TIM_TimeBaseInit(BL_PWM_TIM, &TIM_InitStruct);
    TIM_Cmd(BL_PWM_TIM, ENABLE);

    RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, ENABLE);
    SPI_DeInit(LCD_SPI_BUS);
    RCC_PeriphClockCmd(LCD_SPI_APBPeriph, LCD_SPI_APBClock, ENABLE);
    SPI_InitTypeDef  SPI_InitStructure;

    SPI_StructInit(&SPI_InitStructure);
    SPI_InitStructure.SPI_Direction   = SPI_Direction_FullDuplex;
    SPI_InitStructure.SPI_Mode        = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler  = SPI_BaudRatePrescaler_8;
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola;
    SPI_InitStructure.SPI_NDF         = 0;
    SPI_InitStructure.SPI_TxWaterlevel = 30;
    SPI_InitStructure.SPI_TxDmaEn = ENABLE;
    //SPI_InitStructure.SPI_SwapTxByteEn = 1;


    SPI_Init(LCD_SPI_BUS, &SPI_InitStructure);
    SPI_Cmd(LCD_SPI_BUS, ENABLE);

    RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_ControlMode = GPIO_SOFTWARE_MODE;
    GPIO_InitStruct.GPIO_Pin  = GPIO_GetPin(LCD_SPI_DC);
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_ITCmd = DISABLE;
    GPIO_Init(&GPIO_InitStruct);
    //GPIO_SetBits(GPIO_GetPin(LCD_SPI_BL));
}

void rtl_gui_dma_single_block_init(uint32_t dir_type)
{
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = LCD_DMA_CHANNEL_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStruct);
    /* Initialize GDMA peripheral */
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum          = LCD_DMA_CHANNEL_NUM;

    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;


    if (dir_type == GDMA_DIR_MemoryToMemory)
    {
        GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_1;
        GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_1;
        GDMA_InitStruct.GDMA_DIR                 = dir_type;
        GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
        GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Inc;
    }
    else if (dir_type == GDMA_DIR_MemoryToPeripheral)
    {
        GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_4;
        GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_4;
        GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_SPI0_TX;
        GDMA_InitStruct.GDMA_DIR                 = dir_type;
        GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
        GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;
    }

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

}

void lcd_dma_single_block_start(uint32_t destination_addr, uint32_t source_addr, uint32_t len)
{
    GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, len);
    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)(LCD_SPI_BUS->DR));
    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)source_addr);
    GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
    SPI_GDMACmd(LCD_SPI_BUS, SPI_GDMAReq_Tx, ENABLE);
}

void lcd_wait_lcd_control_transfer(uint32_t count)
{
    while (GDMA_GetTransferINTStatus(LCD_DMA_CHANNEL_NUM) != SET)
    {
        platform_delay_us(10);
    }
    GDMA_ClearINTPendingBit(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
}

void rtk_lcd_hal_transfer_done(void)
{
    while (GDMA_GetTransferINTStatus(LCD_DMA_CHANNEL_NUM) != SET);
    GDMA_ClearINTPendingBit(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_TFE) == false);
}

void rtl_gui_lcd_clear(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd,
                       uint32_t color)
{
#if 0
    rtk_lcd_hal_set_window(xStart, yStart, xEnd - 1, yEnd - 1);
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_BUSY));
    for (uint16_t i = xStart; i < xEnd; i++)
    {
        for (uint16_t j = yStart; j < yEnd; j++)
        {
            while (!(LCD_SPI_BUS->SR & BIT(1)));
            LCD_SPI_BUS->DR[0] = color >> 8;
            while (!(LCD_SPI_BUS->SR & BIT(1)));
            LCD_SPI_BUS->DR[0] = color;
        }
    }
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_BUSY));
#else
    static uint32_t color_buf = 0x24212421;

    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = LCD_DMA_CHANNEL_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStruct);

    /* Initialize GDMA peripheral */
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum          = LCD_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_4;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_4;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_SPI0_TX;

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    rtk_lcd_hal_set_window(xStart, yStart, xEnd - 1, yEnd - 1);
    for (uint16_t i = 0; i < TOTAL_SECTION_COUNT - 1; i++)
    {
        lcd_dma_single_block_start(NULL, (uint32_t)&color_buf, NV3023B_LCD_WIDTH * NV3023B_SEC_HEIGHT);
        rtk_lcd_hal_transfer_done();
    }
    uint32_t last_len = 0;
    if (NV3023B_LCD_HEIGHT % NV3023B_SEC_HEIGHT == 0)
    {
        last_len = NV3023B_SEC_HEIGHT * NV3023B_LCD_WIDTH;
    }
    else
    {
        last_len = (NV3023B_LCD_HEIGHT % NV3023B_SEC_HEIGHT) * NV3023B_LCD_WIDTH;
    }
    lcd_dma_single_block_start(NULL, (uint32_t)&color_buf, last_len);
    lcd_wait_lcd_control_transfer(NV3023B_LCD_WIDTH * NV3023B_LCD_HEIGHT * INPUT_PIXEL_BYTES);

#endif
}
void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len)
{
#if 1
    GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, len);
    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)(LCD_SPI_BUS->DR));
    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)buf);
    GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
    SPI_GDMACmd(LCD_SPI_BUS, SPI_GDMAReq_Tx, ENABLE);
#else
    DBG_DIRECT("rtk_lcd_hal_start_transfer %d ", __LINE__);
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = LCD_DMA_CHANNEL_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStruct);
    DBG_DIRECT("rtk_lcd_hal_start_transfer %d ", __LINE__);
    /* Initialize GDMA peripheral */
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum          = LCD_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_BufferSize          = NV3023B_LCD_WIDTH * NV3023B_SEC_HEIGHT *
                                               INPUT_PIXEL_BYTES;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_4;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_4;
    GDMA_InitStruct.GDMA_SourceAddr          = 0;
    GDMA_InitStruct.GDMA_DestinationAddr     = (uint32_t)(LCD_SPI_BUS->DR);
    GDMA_InitStruct.GDMA_DestHandshake       = LCD_SPI_DMA_TX_HANDSHAKE;
    DBG_DIRECT("rtk_lcd_hal_start_transfer %d ", __LINE__);

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);
    DBG_DIRECT("rtk_lcd_hal_start_transfer %d ", __LINE__);

#endif
}

uint32_t rtk_lcd_hal_get_width(void)
{
    return NV3023B_LCD_WIDTH;
}
uint32_t rtk_lcd_hal_get_height(void)
{
    return NV3023B_LCD_HEIGHT;
}
uint32_t rtk_lcd_hal_get_pixel_bits(void)
{
    return NV3023B_DRV_PIXEL_BITS;
}


void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len)
{

}
bool turn_off_screen;
uint32_t rtk_lcd_hal_power_on(void)
{
    turn_off_screen = 0;
    //Pad_Config(LCD_SPI_BL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);

    Pad_Config(H_2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pinmux_Config(LCD_SPI_CLK, LCD_SPI_FUNC_CLK);
    Pinmux_Config(LCD_SPI_MOSI, LCD_SPI_FUNC_MOSI);
    Pinmux_Config(LCD_SPI_CS, LCD_SPI_FUNC_CS);

    Pinmux_Config(LCD_SPI_DC, DWGPIO);

    Pad_Config(LCD_SPI_CLK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_DC, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    LCD_NV3023_CMD(0x11);
    platform_delay_ms(150);
    LCD_NV3023_CMD(0x36);
    LCD_NV3023_Parameter(0x0);

    LCD_NV3023_CMD(0x29);
    platform_delay_ms(15);
//lcd_set_backlight(100);
    return 0;
}

uint32_t rtk_lcd_hal_power_off(void)
{
    DBG_DIRECT("rtk_lcd_hal_power_off %d ", __LINE__);
    turn_off_screen = 1;
    Pad_Config(LCD_SPI_BL, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(H_2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    // Send Display Off command
//    LCD_NV3023_CMD(0x28);
//    platform_delay_ms(10); // Wait for display to turn off

//    // Send Sleep In command to enter low-power mode
//    LCD_NV3023_CMD(0x10);
//    platform_delay_ms(120); // Wait for sleep mode to stabilize (typical requirement)
    Pad_Config(LCD_SPI_CLK, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_SPI_MOSI, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);

    Pad_Config(LCD_SPI_CS, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_SPI_DC, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_SPI_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);

    DBG_DIRECT("rtk_lcd_hal_power_off %d ", __LINE__);
    return 0;
}


void lcd_set_reset(bool reset)
{
    if (reset)
    {
        Pad_Config(LCD_SPI_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }
    else
    {
        Pad_Config(LCD_SPI_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
}

void rtk_lcd_hal_init(void)
{
    lcd_device_init();
    lcd_set_reset(false); platform_delay_ms(10);
    lcd_set_reset(true); platform_delay_ms(200);
    lcd_set_reset(false); platform_delay_ms(120);
    //lcd_te_device_init();
    lcd_st7789_init();
    DBG_DIRECT("rtk_lcd_hal_init DONE");
    rtk_lcd_hal_power_on();
    rtl_gui_lcd_clear(0, 0, NV3023B_LCD_WIDTH, NV3023B_LCD_HEIGHT, 0x2124);
    turn_off_screen = 0;
}



