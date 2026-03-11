/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "app_section.h"
#include "rtl876x_if8080.h"
#include "trace.h"
#include "lcd_gc9307_240x280_8080.h"
#include "platform_utils.h"
#include "rtl_hal_peripheral.h"


void lcd_gc9307_280_init(void);

/***@display_settings***/
static void gc9307_280_write_cmd(uint8_t command)
{
    IF8080_SetCS();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    IF8080_ResetCS();
    IF8080_SendCommand(command);
}


static void gc9307_280_write_data(uint8_t data)
{
    IF8080_SendData(&data, 1);
}

static void lcd_set_reset(bool reset)
{
    if (reset)
    {
        Pad_Config(P3_3, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }
    else
    {
        Pad_Config(P3_3, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
}



void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h)
{
    IF8080_SwitchMode(IF8080_MODE_MANUAL);
    uint16_t xEnd = xStart + w - 1;
    uint16_t yEnd = yStart + h - 1;
    gc9307_280_write_cmd(0x2a);
    gc9307_280_write_data(xStart >> 8);
    gc9307_280_write_data(xStart & 0xff);
    gc9307_280_write_data(xEnd >> 8);
    gc9307_280_write_data(xEnd & 0xff);

    gc9307_280_write_cmd(0x2b);
    gc9307_280_write_data(yStart >> 8);
    gc9307_280_write_data(yStart & 0xff);
    gc9307_280_write_data(yEnd >> 8);
    gc9307_280_write_data(yEnd & 0xff);
    IF8080_SetCS();
    /* Enable Auto mode */
    IF8080_SwitchMode(IF8080_MODE_AUTO);

    IF8080_ClearTxCounter();

    /* Configure command */
    uint8_t cmd[1] = {0x2c};
    IF8080_SetCmdSequence(cmd, 1);

    /* Enable GDMA TX */
    IF8080_GDMACmd(ENABLE);

    /* Configure pixel number */
    uint32_t len = w * h * GC9307_DRV_PIXEL_BITS >> 3;
    IF8080_SetTxDataLen(len);

    /* Start output */
    IF8080_AutoModeCmd(IF8080_Auto_Mode_Direction_WRITE, ENABLE);
}



static void gc9307_init_1st(void)
{
    //-----------------------------------end Reset Sequence-----------------------------------//
    gc9307_280_write_cmd(0xfe);
    gc9307_280_write_cmd(0xef);
    gc9307_280_write_cmd(0x36);
    gc9307_280_write_data(0x08);//0X00 means RGB for ARM2D, 0x08 means BGR for LVGL
    gc9307_280_write_cmd(0x3a);
    gc9307_280_write_data(0x05);
    //----------------------------------Power Control Registers Initial--------------------------------//
    gc9307_280_write_cmd(0x86);
    gc9307_280_write_data(0x98);
    gc9307_280_write_cmd(0x89);
    gc9307_280_write_data(0x03);
    gc9307_280_write_cmd(0x8b);
    gc9307_280_write_data(0x80);
    gc9307_280_write_cmd(0x8d);
    gc9307_280_write_data(0x22);
    gc9307_280_write_cmd(0x8e);
    gc9307_280_write_data(0x0f);

    gc9307_280_write_cmd(0xe8);//fps
    gc9307_280_write_data(0x12);
    gc9307_280_write_data(0x00);

    gc9307_280_write_cmd(0x35);
    gc9307_280_write_data(0x00);

    gc9307_280_write_cmd(0x44);
    gc9307_280_write_data(0x00);
    gc9307_280_write_data(0x30);

    //gc9307_280_change_disp_mode();

    gc9307_280_write_cmd(0xc3);
    gc9307_280_write_data(0x27);
    gc9307_280_write_cmd(0xc4);
    gc9307_280_write_data(0x18);

    gc9307_280_write_cmd(0xc9);
    gc9307_280_write_data(0x08);

    gc9307_280_write_cmd(0xff);
    gc9307_280_write_data(0x62);
    gc9307_280_write_cmd(0x99);
    gc9307_280_write_data(0x3e);
    gc9307_280_write_cmd(0x9d);
    gc9307_280_write_data(0x4b);

    //---------------------------------display window 240X320-------------------------------------//
    gc9307_280_write_cmd(0x2a);
    gc9307_280_write_data(0x00);
    gc9307_280_write_data(0x00);
    gc9307_280_write_data(0x00);
    gc9307_280_write_data(0Xf0);
    gc9307_280_write_cmd(0x2b);
    gc9307_280_write_data(0x00);
    gc9307_280_write_data(0x00);
    gc9307_280_write_data(0x00);
    gc9307_280_write_data(0xf0);
    gc9307_280_write_cmd(0x2c);
    //-----------------------------end display window 240X320-----------------------------------//
    //---------------------------------------- gamma setting------------------------------------------//
    gc9307_280_write_cmd(0xf0);
    gc9307_280_write_data(0x07);
    gc9307_280_write_data(0x0b);
    gc9307_280_write_data(0xc);
    gc9307_280_write_data(0xa);
    gc9307_280_write_data(0x6);
    gc9307_280_write_data(0x31);

    gc9307_280_write_cmd(0xf2);
    gc9307_280_write_data(0x07);
    gc9307_280_write_data(0x07);
    gc9307_280_write_data(0x4);
    gc9307_280_write_data(0x6);
    gc9307_280_write_data(0x6);
    gc9307_280_write_data(0x21);

    gc9307_280_write_cmd(0xf1);
    gc9307_280_write_data(0x4a);
    gc9307_280_write_data(0x78);
    gc9307_280_write_data(0x76);
    gc9307_280_write_data(0x33);
    gc9307_280_write_data(0x2f);
    gc9307_280_write_data(0xaf);

    gc9307_280_write_cmd(0xf3);
    gc9307_280_write_data(0x38);
    gc9307_280_write_data(0x74);
    gc9307_280_write_data(0x72);
    gc9307_280_write_data(0x22);
    gc9307_280_write_data(0x28);
    gc9307_280_write_data(0x6f);
    //-----------------------------------end gamma setting------------------------------------------//
    platform_delay_ms(20);
    gc9307_280_write_cmd(0x11);
    platform_delay_ms(120);
    gc9307_280_write_cmd(0x29);
    //gc9307_280_write_cmd(0x2c);
}



static void lcd_pad_init(void)
{
    Pad_Config(LCD_8080_D0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D2, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D3, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D4, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D5, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D6, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D7, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);

    /* CS */
    Pad_Config(LCD_8080_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    /* DCX */
    Pad_Config(LCD_8080_DCX, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    /* RD */
    Pad_Config(LCD_8080_RD, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    /* WR */
    Pad_Config(LCD_8080_WR, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);

    /* 8080 interface: D0~D7 */
    Pinmux_Config(LCD_8080_D0, IDLE_MODE);
    Pinmux_Config(LCD_8080_D1, IDLE_MODE);
    Pinmux_Config(LCD_8080_D2, IDLE_MODE);
    Pinmux_Config(LCD_8080_D3, IDLE_MODE);
    Pinmux_Config(LCD_8080_D4, IDLE_MODE);
    Pinmux_Config(LCD_8080_D5, IDLE_MODE);
    Pinmux_Config(LCD_8080_D6, IDLE_MODE);
    Pinmux_Config(LCD_8080_D7, IDLE_MODE);

    /* CS */
    Pinmux_Config(LCD_8080_CS, IDLE_MODE);
    /* DCX */
    Pinmux_Config(LCD_8080_DCX, IDLE_MODE);
    /* RD */
    Pinmux_Config(LCD_8080_RD, IDLE_MODE);
    /* WR */
    Pinmux_Config(LCD_8080_WR, IDLE_MODE);

    /*BL AND RESET ARE NOT FIX*/
    Pad_Config(P3_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);

}

static void lcd_device_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_IF8080, APBPeriph_IF8080_CLOCK, DISABLE);
    RCC_PeriphClockCmd(APBPeriph_IF8080, APBPeriph_IF8080_CLOCK, ENABLE);

    IF8080_PinGroupConfig(IF8080_PinGroup_2);

    IF8080_InitTypeDef IF8080_InitStruct;
    IF8080_StructInit(&IF8080_InitStruct);
    extern uint32_t SystemCpuClock;
    if (SystemCpuClock == 100000000)
    {
        IF8080_InitStruct.IF8080_ClockDiv          = IF8080_CLOCK_DIV_6;
    }
    else if (SystemCpuClock == 90000000)
    {
        IF8080_InitStruct.IF8080_ClockDiv          = IF8080_CLOCK_DIV_6;
    }
    else if (SystemCpuClock == 8000000)
    {
        IF8080_InitStruct.IF8080_ClockDiv          = IF8080_CLOCK_DIV_4;
    }
    else if (SystemCpuClock == 40000000)
    {
        IF8080_InitStruct.IF8080_ClockDiv          = IF8080_CLOCK_DIV_2;
    }
    IF8080_InitStruct.IF8080_Mode              = IF8080_MODE_MANUAL;
    IF8080_InitStruct.IF8080_AutoModeDirection = IF8080_Auto_Mode_Direction_WRITE;
    IF8080_InitStruct.IF8080_GuardTimeCmd      = IF8080_GUARD_TIME_DISABLE;
    IF8080_InitStruct.IF8080_GuardTime         = IF8080_GUARD_TIME_2T;
    IF8080_InitStruct.IF8080_8BitSwap          = IF8080_8BitSwap_ENABLE;
    IF8080_InitStruct.IF8080_16BitSwap         = IF8080_16BitSwap_DISABLE;
    IF8080_InitStruct.IF8080_TxThr             = 10;
    IF8080_InitStruct.IF8080_TxDMACmd          = IF8080_TX_DMA_DISABLE;
    IF8080_InitStruct.IF8080_VsyncCmd          = IF8080_VSYNC_DISABLE;
    IF8080_InitStruct.IF8080_VsyncPolarity     = IF8080_VSYNC_POLARITY_FALLING;
    IF8080_Init(&IF8080_InitStruct);
}

void rtk_lcd_hal_rect_fill(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h,
                           uint32_t color)
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

    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_8;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_8;

    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_8080_TX;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);


    rtk_lcd_hal_set_window(xStart, yStart, w, h);

    //static uint32_t color_buf = color;
    uint32_t fix_buffer[8] = {color, color, color, color, color, color, color, color};


    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)fix_buffer);
    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)(&(IF8080->FIFO)));



    uint32_t section_hight = 10;
    uint32_t left_line = h % section_hight;

    for (uint32_t i = 0; i < h / section_hight; i++)
    {
        GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, (w * section_hight * 2) >> 2);
        GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
        while (GDMA_GetTransferINTStatus(LCD_DMA_CHANNEL_NUM) != SET);
        GDMA_ClearINTPendingBit(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    }
    if (left_line != 0)
    {
        GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, (w * left_line * 2) >> 2);
        GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
        rtk_lcd_hal_transfer_done();
    }

}

void rtk_lcd_hal_init(void)
{
    lcd_pad_init();
    lcd_device_init();
    lcd_set_reset(true);
    platform_delay_ms(120);
    lcd_set_reset(false);
    platform_delay_ms(120);

    gc9307_init_1st();

    //lcd_gc9307_280_power_on();
    gc9307_280_write_cmd(0x21);


    //rtk_lcd_hal_rect_fill(0, 0, 240, 280, 0x001F001F);// RED
    rtk_lcd_hal_rect_fill(0, 0, 240, 280, 0xF800F800);// BLUE
    //rtk_lcd_hal_rect_fill(0, 0, 240, 280, 0x07e007e0);// GREEN

}


void rtl_gui_dma_single_block_init(void)
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

    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Word;

    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_8;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_8;
    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_8080_TX;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);
}


void lcd_dma_single_block_start(uint8_t *source_addr, uint32_t len)
{
    while ((len >> 2) > 4096)
    {
        DBG_DIRECT("BUFFER SIZE ERROR!!");
    }
    GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, len >> 2);
    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)(&(IF8080->FIFO)));
    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)source_addr);
    GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
}


void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len)
{
    rtl_gui_dma_single_block_init();
    lcd_dma_single_block_start(buf, len * 2);
}

void rtk_lcd_hal_transfer_done(void)
{
    while (GDMA_GetTransferINTStatus(LCD_DMA_CHANNEL_NUM) != SET);
    GDMA_ClearINTPendingBit(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer);

    while (IF8080_GetTxDataLen() != IF8080_GetTxCounter())
    {
        //counter = IF8080_GetTxCounter();
        //DBG_DIRECT("LINE = %d, len = %d, tx cnt = %d ", __LINE__, IF8080_GetTxDataLen(), IF8080_GetTxCounter());
    }
    IF8080_ClearFIFO();
    IF8080_SwitchMode(IF8080_MODE_MANUAL);
}
uint32_t rtk_lcd_hal_get_width(void)
{
    return GC9307_LCD_WIDTH;
}
uint32_t rtk_lcd_hal_get_height(void)
{
    return GC9307_LCD_HIGHT;
}
uint32_t rtk_lcd_hal_get_pixel_bits(void)
{
    return GC9307_DRV_PIXEL_BITS;
}
void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len)
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

    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Word;

    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_8;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_8;
    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_8080_TX;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, len >> 2);

    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)(&(IF8080->FIFO)));
    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)buf);
    GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
}
