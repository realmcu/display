/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "rtl_hal_peripheral.h"
#include "st7789_320_240_8080.h"
#include "drv_gpio.h"
#include "trace.h"

#define LCDC_DMA_CHANNEL_NUM              0
#define LCDC_DMA_CHANNEL_INDEX            LCDC_DMA_Channel0

#define DMA_MULTIBLOCK 0


void st7789_write_cmd(uint8_t command)
{
    DBIB_SetCS();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    DBIB_ResetCS();

    DBIB_SendCmd(command);
}



void st7789_write_data(uint8_t data)
{
    uint8_t data_buf[1];
    data_buf[0] = data;
    DBIB_SendData(data_buf, 1);
}

void lcd_lcd7796_power_on(void)
{
    DBG_DIRECT("lcd_lcd7796_power_on !!!\n");
}

void lcd_lcd7796_power_off(void)
{
    st7789_write_cmd(0x28);
    st7789_write_cmd(0x10);
    //lcd_set_backlight(0);
}

static void lcd7796_seq_init(void)
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
}


// uint8_t array_t[320*240*2];
void lcd_lcd7796_clear(uint8_t *buf, uint32_t len)
{
    rtk_lcd_hal_set_window(0, 0, ST7789_LCD_WIDTH, ST7789_LCD_HEIGHT);

    LCDC_DMA_InitTypeDef LCDC_DMA_InitStruct = {0};
    LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
    LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum          = LCDC_DMA_CHANNEL_NUM;
    LCDC_DMA_InitStruct.LCDC_DMA_DIR                 = LCDC_DMA_DIR_PeripheralToMemory;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceInc           = LCDC_DMA_SourceInc_Inc;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc      = LCDC_DMA_DestinationInc_Fix;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize      = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_64;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_64;
    // LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = 0x8000000;
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En      = 0;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);


    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();

    LCDC_SwitchMode(LCDC_AUTO_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    DBIB_BypassCmdByteCmd(DISABLE);

    uint8_t cmd[1] = {0x2C};
    LCDC_DBIB_SetCmdSequence(cmd, 1);

    len = ST7789_LCD_WIDTH * ST7789_LCD_HEIGHT;
    LCDC_SetTxPixelLen(len);

    LCDC_Cmd(ENABLE);


    LCDC_DMA_SetSourceAddress(LCDC_DMA_CHANNEL_INDEX, (uint32_t)0x8000000);
    // LCDC_DMA_SetSourceAddress(LCDC_DMA_CHANNEL_INDEX, (uint32_t)buf);
    LCDC_ForceBurst(ENABLE);
    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);
    LCDC_AutoWriteCmd(ENABLE);

    LCDC_HANDLER_DMA_FIFO_CTRL_t handler_reg_0x18;
    do
    {
        handler_reg_0x18.d32 = LCDC_HANDLER->DMA_FIFO_CTRL;
    }
    while (handler_reg_0x18.b.dma_enable != RESET);
    LCDC_HANDLER_OPERATE_CTR_t handler_reg_0x14;
    do
    {
        handler_reg_0x14.d32 = LCDC_HANDLER->OPERATE_CTR;
    }
    while (handler_reg_0x14.b.auto_write_start != RESET);

    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();

    LCDC_Cmd(DISABLE);
}

void lcd7796_read(uint16_t command, uint8_t *pBuf, uint32_t len)
{
    LCDC_Cmd(DISABLE);
    LCDC_SwitchMode(LCDC_MANUAL_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    LCDC_Cmd(ENABLE);
    st7789_write_cmd(command);

    LCDC_Cmd(DISABLE);
    LCDC_SwitchMode(LCDC_MANUAL_MODE);
    LCDC_SwitchDirect(LCDC_RX_MODE);
    LCDC_Cmd(ENABLE);
    /* Read data */
    DBIB_ReceiveData(pBuf, len);

    DBIB_SetCS();
}

void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h)
{
    LCDC_Cmd(DISABLE);
    LCDC_SwitchMode(LCDC_MANUAL_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    LCDC_Cmd(ENABLE);
    uint16_t xEnd = xStart + w - 1;
    uint16_t yEnd = yStart + h - 1;

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

    DBIB_SetCS();
}

void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len)
{
    if (((uint32_t)buf % 4) != 0)
    {
        while (1);
    }

    LCDC_DMA_InitTypeDef LCDC_DMA_InitStruct = {0};
    LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
    LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum          = LCDC_DMA_CHANNEL_NUM;
    LCDC_DMA_InitStruct.LCDC_DMA_DIR                 = LCDC_DMA_DIR_PeripheralToMemory;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceInc           = LCDC_DMA_SourceInc_Inc;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc      = LCDC_DMA_DestinationInc_Fix;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize      = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_64;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_64;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = 0x8000000;
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En      = 0;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);


    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();

    LCDC_SwitchMode(LCDC_AUTO_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    DBIB_BypassCmdByteCmd(DISABLE);

    uint8_t cmd[1] = {0x2C};
    LCDC_DBIB_SetCmdSequence(cmd, 1);

    LCDC_SetTxPixelLen(len);

    LCDC_Cmd(ENABLE);


    LCDC_DMA_SetSourceAddress(LCDC_DMA_CHANNEL_INDEX, (uint32_t)buf);

    LCDC_ForceBurst(ENABLE);
    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);
    LCDC_AutoWriteCmd(ENABLE);


}
void rtk_lcd_hal_transfer_done(void)
{
    LCDC_HANDLER_DMA_FIFO_CTRL_t handler_reg_0x18;
    do
    {
        handler_reg_0x18.d32 = LCDC_HANDLER->DMA_FIFO_CTRL;
    }
    while (handler_reg_0x18.b.dma_enable != RESET);
    LCDC_HANDLER_OPERATE_CTR_t handler_reg_0x14;
    do
    {
        handler_reg_0x14.d32 = LCDC_HANDLER->OPERATE_CTR;
    }
    while (handler_reg_0x14.b.auto_write_start != RESET);


    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();

    LCDC_Cmd(DISABLE);
    LCDC_ClearINTPendingBit(LCDC_CLR_WAVEFORM_FINISH);
}


void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len)
{
    if (((uint32_t)buf % 4) != 0)
    {
        while (1);
    }

    LCDC_DMA_InitTypeDef LCDC_DMA_InitStruct = {0};
    LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
    LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum          = LCDC_DMA_CHANNEL_NUM;
    LCDC_DMA_InitStruct.LCDC_DMA_DIR                 = LCDC_DMA_DIR_PeripheralToMemory;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceInc           = LCDC_DMA_SourceInc_Inc;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc      = LCDC_DMA_DestinationInc_Fix;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize      = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_64;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_64;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = 0x8000000;
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En      = 0;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);


    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();

    LCDC_SwitchMode(LCDC_AUTO_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    DBIB_BypassCmdByteCmd(DISABLE);

    uint8_t cmd[1] = {0x2C};
    LCDC_DBIB_SetCmdSequence(cmd, 1);

    LCDC_SetTxPixelLen(len);

    LCDC_Cmd(ENABLE);


    LCDC_DMA_SetSourceAddress(LCDC_DMA_CHANNEL_INDEX, (uint32_t)buf);

    LCDC_ForceBurst(ENABLE);
    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);
    LCDC_AutoWriteCmd(ENABLE);

    LCDC_HANDLER_DMA_FIFO_CTRL_t handler_reg_0x18;
    do
    {
        handler_reg_0x18.d32 = LCDC_HANDLER->DMA_FIFO_CTRL;
    }
    while (handler_reg_0x18.b.dma_enable != RESET);
    LCDC_HANDLER_OPERATE_CTR_t handler_reg_0x14;
    do
    {
        handler_reg_0x14.d32 = LCDC_HANDLER->OPERATE_CTR;
    }
    while (handler_reg_0x14.b.auto_write_start != RESET);


    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();

    LCDC_Cmd(DISABLE);
}


static void st7789_pad_config(void)
{
    Pad_Config(LCD_8080_D0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_D1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_D2, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_D3, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_D4, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_D5, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_D6, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_D7, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_DCX, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_RD, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_WR, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    // Pad_Config(LCD_TE_SYNC, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);

    Pad_Dedicated_Config(LCD_8080_D0, ENABLE);
    Pad_Dedicated_Config(LCD_8080_D1, ENABLE);
    Pad_Dedicated_Config(LCD_8080_D2, ENABLE);
    Pad_Dedicated_Config(LCD_8080_D3, ENABLE);
    Pad_Dedicated_Config(LCD_8080_D4, ENABLE);
    Pad_Dedicated_Config(LCD_8080_D5, ENABLE);
    Pad_Dedicated_Config(LCD_8080_D6, ENABLE);
    Pad_Dedicated_Config(LCD_8080_D7, ENABLE);
    Pad_Dedicated_Config(LCD_8080_CS, ENABLE);
    Pad_Dedicated_Config(LCD_8080_DCX, ENABLE);
    Pad_Dedicated_Config(LCD_8080_RD, ENABLE);
    Pad_Dedicated_Config(LCD_8080_WR, ENABLE);
    // Pad_Dedicated_Config(LCD_TE_SYNC, ENABLE);

}
static void st7789_chip_reset(void)
{
    //no need HW do it
    // Pad_Config(LCD_8080_D0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    drv_pin_mode(LCD_8080_RST, PIN_MODE_OUTPUT);
    drv_pin_write(LCD_8080_RST, 1);
    platform_delay_ms(10);
    drv_pin_write(LCD_8080_RST, 0);
    platform_delay_ms(50);
    drv_pin_write(LCD_8080_RST, 1);
    platform_delay_ms(140);

    drv_pin_mode(LCD_8080_BL, PIN_MODE_OUTPUT);
    drv_pin_write(LCD_8080_BL, 1);
}


uint32_t rtk_lcd_hal_get_width(void)
{
    return ST7789_LCD_WIDTH;
}
uint32_t rtk_lcd_hal_get_height(void)
{
    return ST7789_LCD_HEIGHT;
}
uint32_t rtk_lcd_hal_get_pixel_bits(void)
{
    return ST7789_DRV_PIXEL_BITS;
}

void rtk_lcd_hal_init(void)
{
    /* config LCD dev info */
    RCC_PeriphClockCmd(APBPeriph_DISP, APBPeriph_DISP_CLOCK, ENABLE);

    //from XTAL SOURCE = 40M
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.disp_ck_en = 1;
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.disp_func_en = 1;
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_mux_clk_cg_en = 1;

    //From PLL1, SOURCE = 125M
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_div_en = 1;
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_clk_src_sel0 = 0; //pll1_peri(0) or pll2(1, pll2 = 160M)
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_clk_src_sel1 = 1; //pll(1) or xtal(0)
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_div_sel = 1; //div

    st7789_pad_config();
    // st7789_chip_reset();
    LCDC_InitTypeDef lcdc_init = {0};
    lcdc_init.LCDC_Interface = LCDC_IF_DBIB;
    lcdc_init.LCDC_GroupSel =
        2;    // I8080 Group2 pins (D0-7=P0_4-7/P4_0-3, CS#=P0_0, WR#=P0_2, RD#=P1_6, DCX=P1_5), per RTL87x2G QFN88 pinmux
    lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_RGB565;
    lcdc_init.LCDC_PixelOutputFormat = LCDC_OUTPUT_RGB565;
    lcdc_init.LCDC_PixelBitSwap = LCDC_SWAP_BYPASS; //lcdc_handler_cfg->LCDC_TeEn = LCDC_TE_DISABLE;
    lcdc_init.LCDC_TePolarity = LCDC_TE_EDGE_RISING;
    lcdc_init.LCDC_DmaThreshold =
        64;    // MSize + threshold should be no larger than 128
    LCDC_Init(&lcdc_init);
    LCDC_ClearINTPendingBit(LCDC_CLR_WAVEFORM_FINISH);


    LCDC_DBIBCfgTypeDef dbib_init = {0};
    dbib_init.DBIB_SPEED_SEL         = DBIB_CLOCK_DIV_3;
    dbib_init.DBIB_InitGuardTimeCmd  = DBIB_INIT_GUARD_TIME_DISABLE;
    dbib_init.DBIB_InitGuardTime     = DBIB_INIT_GUARD_TIME_1T;
    dbib_init.DBIB_CmdGuardTimeCmd   = DBIB_CMD_GUARD_TIME_DISABLE;
    dbib_init.DBIB_CmdGuardTime      = DBIB_CMD_GUARD_TIME_1T;
    dbib_init.DBIB_GuardTimeCmd      = DBIB_GUARD_TIME_DISABLE;
    dbib_init.DBIB_GuardTime         = DBIB_GUARD_TIME_1T;
    dbib_init.DBIB_WRDelay           = DBIB_WR_HALF_DELAY;
    DBIB_Init(&dbib_init);

    //lcdc_dbib_init_lcd();
    LCDC_SwitchMode(LCDC_MANUAL_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    LCDC_Cmd(ENABLE);

//    LCDC_LCD_SET_RST(false);
//    platform_delay_ms(10);
//    LCDC_LCD_SET_RST(true);
//    platform_delay_ms(20);
//    LCDC_LCD_SET_RST(false);
//    platform_delay_ms(140);
    st7789_chip_reset();

    st7789_write_cmd(0x11);
    platform_delay_ms(120);
    lcd7796_seq_init();

    uint8_t color[4] = {0x1F, 0xF8, 0x1F, 0XF8};//B G R A

    uint8_t *pd = (uint8_t *)0x8000000;
    for (uint32_t i = 0; i < 320 * 240; i++)
    {
        if (i % 2 == 0)
        {
            pd[i] = 0x1F;
        }
        else
        {
            pd[i] = 0xF8;
        }
    }
    pd += 320 * 240;
    for (uint32_t i = 0; i < 320 * 240; i++)
    {
        if (i % 2 == 0)
        {
            pd[i] = 0xFF;
        }
        else
        {
            pd[i] = 0xFF;
        }
    }
    lcd_lcd7796_clear(pd, ST7789_LCD_WIDTH * ST7789_LCD_HEIGHT);

    // while(1);
}

