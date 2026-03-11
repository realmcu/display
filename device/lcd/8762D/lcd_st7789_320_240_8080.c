/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "board.h"
#include "lcd_st7789_320_240_8080.h"
#include "app_section.h"
#include "flash_device.h"
#include "rtl_hal_peripheral.h"
#include "trace.h"
#include "rtl876x_tim.h"

DATA_RAM_FUNCTION
void st7789_write_cmd(uint8_t command)
{
    IF8080_SetCS();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    IF8080_ResetCS();
    IF8080_SendCommand(command);
}

DATA_RAM_FUNCTION
void st7789_write_data(uint8_t data)
{
    IF8080_SendData(&data, 1);
}

static void lcd_set_reset(bool reset)
{
    if (reset)
    {
        Pad_Config(LCD_8080_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }
    else
    {
        Pad_Config(LCD_8080_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
}

void lcd_set_backlight(uint32_t percent)
{
    if (percent)
    {
        if (percent > 100)
        {
            percent = 100;
        }
        TIM_Cmd(BL_PWM_TIM, DISABLE);
        TIM_PWMChangeFreqAndDuty(BL_PWM_TIM, percent * 10, (100 - percent) * 10);
        TIM_Cmd(BL_PWM_TIM, ENABLE);
        Pad_Config(LCD_8080_BL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE,
                   PAD_OUT_HIGH);
    }
    else
    {
        TIM_Cmd(BL_PWM_TIM, DISABLE);
        Pad_Config(LCD_8080_BL, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }
    return;
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
    Pad_Config(LCD_8080_WR, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);

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
    Pad_Config(LCD_8080_BL, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
}

void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h)
{
    IF8080_SwitchMode(IF8080_MODE_MANUAL);
    uint16_t xEnd = xStart + w - 1;
    uint16_t yEnd = yStart + h - 1;
#if(LCD_DIRECTION_SELECTION_90_READINGS == 1)
    xStart += 80;
    xEnd += 80;
#else
    // yStart = yStart + 80;
    // yEnd = yEnd + 80;
#endif

    st7789_write_cmd(0x2a);
    st7789_write_data(xStart >> 8);
    st7789_write_data(xStart & 0xff);
    st7789_write_data(xEnd >> 8);
    st7789_write_data(xEnd & 0xff);

    st7789_write_cmd(0x2b);
    st7789_write_data(yStart >> 8);
    st7789_write_data(yStart & 0xff);
    st7789_write_data(yEnd >> 8);
    st7789_write_data(yEnd & 0xff);
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
    uint32_t len = (xEnd - xStart + 1) * (yEnd - yStart + 1) * RGB16BIT_565 >> 3;
    IF8080_SetTxDataLen(len);

    /* Start output */
    IF8080_AutoModeCmd(IF8080_Auto_Mode_Direction_WRITE, ENABLE);
}

uint32_t rtk_lcd_hal_power_off(void)
{
    Pad_Config(LCD_8080_D0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D3, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D4, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D5, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D6, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(LCD_8080_D7, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    /* CS */
    Pad_Config(LCD_8080_CS, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    /* DCX */
    Pad_Config(LCD_8080_DCX, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    /* RD */
    Pad_Config(LCD_8080_RD, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    /* WR */
    Pad_Config(LCD_8080_WR, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    /* BL */
    Pad_Config(LCD_8080_BL, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);

    extern void drv_touch_int_config(bool enable);
    drv_touch_int_config(true);
    st7789_write_cmd(0x28);
    st7789_write_cmd(0x10);
    lcd_set_backlight(0);
    return 0;
}
uint32_t rtk_lcd_hal_power_on(void)
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

    Pad_Config(LCD_8080_BL, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);

    st7789_write_cmd(0x11);
    st7789_write_cmd(0x29);
    //lcd_set_backlight(100);
    return 0;
}

void lcd_device_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_IF8080, APBPeriph_IF8080_CLOCK, DISABLE);
    RCC_PeriphClockCmd(APBPeriph_IF8080, APBPeriph_IF8080_CLOCK, ENABLE);
#if IF8080_PINGROUP_1
    IF8080_PinGroupConfig(IF8080_PinGroup_1);
#else
    IF8080_PinGroupConfig(IF8080_PinGroup_2);
#endif
    IF8080_InitTypeDef IF8080_InitStruct;
    IF8080_StructInit(&IF8080_InitStruct);
    extern uint32_t SystemCpuClock;

    if (SystemCpuClock == 100000000)
    {
        IF8080_InitStruct.IF8080_ClockDiv          = IF8080_CLOCK_DIV_5;
    }
    else if (SystemCpuClock == 90000000)
    {
        IF8080_InitStruct.IF8080_ClockDiv          = IF8080_CLOCK_DIV_6;
    }
    else if (SystemCpuClock == 8000000)
    {
        IF8080_InitStruct.IF8080_ClockDiv          = IF8080_CLOCK_DIV_4;
    }
    else if (SystemCpuClock == 6000000)
    {
        IF8080_InitStruct.IF8080_ClockDiv          = IF8080_CLOCK_DIV_5;
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
    lcd_pad_init();
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
    GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, len >> 2);
    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)(&(IF8080->FIFO)));
    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)source_addr);
    GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
}

// section height default 10 pixel
#define GDMA_MULTIBLOCK_SIZE            (LCD_HEIGHT / 10)
#define GDMA_MULTIBLOCK_MODE            LLI_TRANSFER
GDMA_LLIDef LCD_GDMA_LLIStruct[GDMA_MULTIBLOCK_SIZE];
void rtl_gui_dma_multi_block_init(uint8_t *source_addr, uint32_t len)
{
    uint32_t GDMA_BufferSize = LCD_WIDTH * 10 * 2 >> 2;

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
    GDMA_InitStruct.GDMA_BufferSize          = GDMA_BufferSize;
    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_8080_TX;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;

    GDMA_InitStruct.GDMA_Multi_Block_Mode   = GDMA_MULTIBLOCK_MODE;//LLI_TRANSFER;
    GDMA_InitStruct.GDMA_Multi_Block_En     = 1;
    GDMA_InitStruct.GDMA_Multi_Block_Struct = (uint32_t)LCD_GDMA_LLIStruct;

    // source_addr += 3 * GDMA_BufferSize;
    for (uint32_t i = 0; i < GDMA_MULTIBLOCK_SIZE; i++)
    {
        LCD_GDMA_LLIStruct[i].SAR = (uint32_t)source_addr + i * GDMA_BufferSize * 4;
        LCD_GDMA_LLIStruct[i].DAR = (uint32_t)(&(IF8080->FIFO));
        if (i == GDMA_MULTIBLOCK_SIZE - 1)
        {
            //GDMA_LLIStruct[i].LLP=0;
            LCD_GDMA_LLIStruct[i].LLP = 0;
            /* Configure low 32 bit of CTL register */
            LCD_GDMA_LLIStruct[i].CTL_LOW = BIT(0)
                                            | (GDMA_InitStruct.GDMA_DestinationDataSize << 1)
                                            | (GDMA_InitStruct.GDMA_SourceDataSize << 4)
                                            | (GDMA_InitStruct.GDMA_DestinationInc << 7)
                                            | (GDMA_InitStruct.GDMA_SourceInc << 9)
                                            | (GDMA_InitStruct.GDMA_DestinationMsize << 11)
                                            | (GDMA_InitStruct.GDMA_SourceMsize << 14)
                                            | (GDMA_InitStruct.GDMA_DIR << 20);
            /* Configure high 32 bit of CTL register */
            LCD_GDMA_LLIStruct[i].CTL_HIGH = GDMA_InitStruct.GDMA_BufferSize;
        }
        else
        {
            LCD_GDMA_LLIStruct[i].LLP = (uint32_t)&LCD_GDMA_LLIStruct[i + 1];
            /* Configure low 32 bit of CTL register */
            LCD_GDMA_LLIStruct[i].CTL_LOW = BIT(0)
                                            | (GDMA_InitStruct.GDMA_DestinationDataSize << 1)
                                            | (GDMA_InitStruct.GDMA_SourceDataSize << 4)
                                            | (GDMA_InitStruct.GDMA_DestinationInc << 7)
                                            | (GDMA_InitStruct.GDMA_SourceInc << 9)
                                            | (GDMA_InitStruct.GDMA_DestinationMsize << 11)
                                            | (GDMA_InitStruct.GDMA_SourceMsize << 14)
                                            | (GDMA_InitStruct.GDMA_DIR << 20)
                                            | (GDMA_InitStruct.GDMA_Multi_Block_Mode & LLP_SELECTED_BIT);
        }
        /* Configure high 32 bit of CTL register */
        LCD_GDMA_LLIStruct[i].CTL_HIGH = GDMA_InitStruct.GDMA_BufferSize;
    }

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);
}


void lcd_dma_multi_block_start()
{
    GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
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

    static uint32_t color_buf[64];
    // static uint32_t color_buf;
    for (int i = 0; i < 64; i++)
    {
        color_buf[i] = color;
    }
//    static uint32_t color_buf = 0;
//    color_buf = (color >> 8) | (color << 8);
//    color_buf = color_buf | color_buf << 16;

    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)color_buf);
    // GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)&color);
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

static uint32_t pfb_tx_xnt = 0;
void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len)
{
    pfb_tx_xnt = len;
    // RGB16BIT_565 16
    if (len <= 4095 * 2) // Max buffer size   word
    {
        rtl_gui_dma_single_block_init();
        lcd_dma_single_block_start(buf, len * 2);
    }
    else
    {
        rtl_gui_dma_multi_block_init(buf, len * 2);
        lcd_dma_multi_block_start();
    }
}

void rtk_lcd_hal_transfer_done(void)
{
    while (GDMA_GetTransferINTStatus(LCD_DMA_CHANNEL_NUM) != SET);
    GDMA_ClearINTPendingBit(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    uint32_t counter = 0;
    while (counter != (pfb_tx_xnt * 2)) //LCD_SECTION_HEIGHT
    {
        counter = IF8080_GetTxCounter();
    }
    IF8080_SwitchMode(IF8080_MODE_MANUAL);
}
uint32_t rtk_lcd_hal_get_width(void)
{
    return LCD_WIDTH;
}
uint32_t rtk_lcd_hal_get_height(void)
{
    return LCD_HEIGHT;
}
uint32_t rtk_lcd_hal_get_pixel_bits(void)
{
    return RGB16BIT_565;
}

void lcd_seq_init(void)
{
#if 1
    st7789_write_cmd(0x35);
    st7789_write_data(0x00);

    st7789_write_cmd(0x44);
    st7789_write_data(0x00);
    st7789_write_data(0x50);
#if FEATURE_PSRAM
    st7789_write_cmd(0x44);
    st7789_write_data(0x00);
    st7789_write_data(0xA0); // if use psram set tear scanline here
#else
    st7789_write_cmd(0x44);
    st7789_write_data(0x00);
    st7789_write_data(0x50);
#endif

    st7789_write_cmd(0x36);
    st7789_write_data(0x60);

    st7789_write_cmd(0x3A);
    st7789_write_data(0x05);

    st7789_write_cmd(0xB2);
    st7789_write_data(0x0C);
    st7789_write_data(0x0C);
    st7789_write_data(0x00);
    st7789_write_data(0x33);
    st7789_write_data(0x33);

    st7789_write_cmd(0xB7);
    st7789_write_data(0x35);


//-----------ST7789V Power setting---------------//
    st7789_write_cmd(0xbb);
    st7789_write_data(0x28);
    st7789_write_cmd(0xc0);
    st7789_write_data(0x2c);
    st7789_write_cmd(0xc2);
    st7789_write_data(0x01);
    st7789_write_cmd(0xc3);
    st7789_write_data(0x0b);
    st7789_write_cmd(0xc4);
    st7789_write_data(0x20);
    st7789_write_cmd(0xc6);
    st7789_write_data(0x0f);
    st7789_write_cmd(0xd0);
    st7789_write_data(0xa4);
    st7789_write_data(0xa1);
//------------ST7789V gamma setting-------------//
    st7789_write_cmd(0xe0);
    st7789_write_data(0xd0);
    st7789_write_data(0x01);
    st7789_write_data(0x08);
    st7789_write_data(0x0f);
    st7789_write_data(0x11);
    st7789_write_data(0x2a);
    st7789_write_data(0x36);
    st7789_write_data(0x55);
    st7789_write_data(0x44);
    st7789_write_data(0x3a);
    st7789_write_data(0x0b);
    st7789_write_data(0x06);
    st7789_write_data(0x11);
    st7789_write_data(0x20);
    st7789_write_cmd(0xe1);
    st7789_write_data(0xd0);
    st7789_write_data(0x02);
    st7789_write_data(0x07);
    st7789_write_data(0x0a);
    st7789_write_data(0x0b);
    st7789_write_data(0x18);
    st7789_write_data(0x34);
    st7789_write_data(0x43);
    st7789_write_data(0x4a);
    st7789_write_data(0x2b);
    st7789_write_data(0x1b);
    st7789_write_data(0x1c);
    st7789_write_data(0x22);
    st7789_write_data(0x1f);
    st7789_write_cmd(0x29);
#endif

#if 0

    st7789_write_cmd(0x35);
    st7789_write_data(0x00);

//    st7789_write_cmd(0x44);
//    st7789_write_data(0x00);
//    st7789_write_data(0x50);

    st7789_write_cmd(0x36);
    st7789_write_data(0x00);

    st7789_write_cmd(0x3A);
    st7789_write_data(0x05);

    st7789_write_cmd(0xB2);
    st7789_write_data(0x0C);
    st7789_write_data(0x0C);
    st7789_write_data(0x00);
    st7789_write_data(0x33);
    st7789_write_data(0x33);

    st7789_write_cmd(0xB7);
    st7789_write_data(0x35);

    st7789_write_cmd(0xBB);
    st7789_write_data(0x19);

    st7789_write_cmd(0xC0);
    st7789_write_data(0x2C);

    st7789_write_cmd(0xC2);
    st7789_write_data(0x01);

    st7789_write_cmd(0xC3);
    st7789_write_data(0x12);

    st7789_write_cmd(0xC4);
    st7789_write_data(0x20);

    st7789_write_cmd(0xC6);
    st7789_write_data(0x1F);

    st7789_write_cmd(0xD0);
    st7789_write_data(0xA4);
    st7789_write_data(0xA1);

    st7789_write_cmd(0xE0);
    st7789_write_data(0xD0);
    st7789_write_data(0x04);
    st7789_write_data(0x0D);
    st7789_write_data(0x11);
    st7789_write_data(0x13);
    st7789_write_data(0x2B);
    st7789_write_data(0x3F);
    st7789_write_data(0x54);
    st7789_write_data(0x4C);
    st7789_write_data(0x18);
    st7789_write_data(0x0D);
    st7789_write_data(0x0B);
    st7789_write_data(0x1F);
    st7789_write_data(0x23);

    st7789_write_cmd(0xE1);
    st7789_write_data(0xD0);
    st7789_write_data(0x04);
    st7789_write_data(0x0C);
    st7789_write_data(0x11);
    st7789_write_data(0x13);
    st7789_write_data(0x2C);
    st7789_write_data(0x3F);
    st7789_write_data(0x44);
    st7789_write_data(0x51);
    st7789_write_data(0x2F);
    st7789_write_data(0x1F);
    st7789_write_data(0x1F);
    st7789_write_data(0x20);
    st7789_write_data(0x23);

//    st7789_write_cmd(0xE4);
//    st7789_write_data(0x27);
//    st7789_write_data(0x00);
//    st7789_write_data(0x11);

    st7789_write_cmd(0x21);

    st7789_write_cmd(0x11);
    platform_delay_ms(120);
#endif
}
void rtk_lcd_hal_init(void)
{
    lcd_device_init();

    lcd_set_reset(false);
    platform_delay_ms(1);
    lcd_set_reset(true);
    platform_delay_ms(20);
    lcd_set_reset(false);
    platform_delay_ms(140);

    st7789_write_cmd(0x11);
    platform_delay_ms(120);
    lcd_seq_init();


    // st7789_write_cmd(0x29);
    //lcd_set_backlight(100);

    rtk_lcd_hal_rect_fill(0, 0, LCD_WIDTH, LCD_HEIGHT, 0xF81FF81F);
}


