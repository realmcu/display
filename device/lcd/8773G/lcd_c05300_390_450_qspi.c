/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "rtl_lcdc_dbic.h"
#include "lcd_c05300_390_450_qspi.h"
#include "board.h"
#include "trace.h"
#include "platform_utils.h"
#include "os_mem.h"
#include "os_sched.h"
#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"
#include "clock_manager.h"
#include "rtl876x_nvic.h"
#include "rtl876x_gpio.h"
#include "vector_table.h"

#define LCDC_DMA_CHANNEL_NUM              0
#define LCDC_DMA_CHANNEL_INDEX            LCDC_DMA_Channel0

#define BIT_CMD_CH(x)           (((x) & 0x00000003) << 20)
#define BIT_DATA_CH(x)          (((x) & 0x00000003) << 18)
#define BIT_ADDR_CH(x)          (((x) & 0x00000003) << 16)
#define BIT_TMOD(x)             (((x) & 0x00000003) << 8)

#define BIT_TXSIM               (0x00000001 << 9)
#define BIT_SEQ_EN              (0x00000001 << 3)





static void lcdc_write(uint8_t *buf, uint32_t len)
{
    DBIC->CTRLR0 |= BIT31;
    DBIC->CTRLR0 &= ~(BIT_CMD_CH(3) | BIT_ADDR_CH(3) | BIT_DATA_CH(3));//SET CHANNEL NUM
    DBIC->CTRLR0 &= ~(BIT_TMOD(3)); //tx mode
    DBIC_TX_NDF(len - 4);
    DBIC_CmdLength(1);
    DBIC_AddrLength(3);
    DBIC_USER_LENGTH_TypeDef reg_val = {.d32 = DBIC->USER_LENGTH};
    for (uint32_t i = 0; i < len; i++)
    {
        DBIC->DR[0].byte = buf[i];
    }
    DBIC_Cmd(ENABLE);
    while (DBIC->SR & BIT0);// wait bus busy
}





static void qspi_pad_config(void)
{
    /*QSPI Config*/
    /*QSPI Pad Config, P9_0 -> SIO2, P9_1 -> SIO1, P9_2 -> CS, P9_3 -> SIO0, P9_4 -> CLK, P9_5 -> SIO3*/
    Pad_Config(P9_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(P9_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(P9_2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(P9_3, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(P9_4, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(P9_5, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(P2_2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_HIGH);

    Pad_HighSpeedFuncSel(P9_0, HS_Func0);
    Pad_HighSpeedFuncSel(P9_1, HS_Func0);
    Pad_HighSpeedFuncSel(P9_2, HS_Func0);
    Pad_HighSpeedFuncSel(P9_3, HS_Func0);
    Pad_HighSpeedFuncSel(P9_4, HS_Func0);
    Pad_HighSpeedFuncSel(P9_5, HS_Func0);

    Pad_HighSpeedMuxSel(P9_0, FROM_HS_MUX);
    Pad_HighSpeedMuxSel(P9_1, FROM_HS_MUX);
    Pad_HighSpeedMuxSel(P9_2, FROM_HS_MUX);
    Pad_HighSpeedMuxSel(P9_3, FROM_HS_MUX);
    Pad_HighSpeedMuxSel(P9_4, FROM_HS_MUX);
    Pad_HighSpeedMuxSel(P9_5, FROM_HS_MUX);
    Pad_HighSpeedMuxSel(P2_2, FROM_HS_MUX);


}

static void LCD_CO5300_qspi_cmd_data_byte(uint8_t cmd, uint8_t data)
{
    uint8_t sdat[] = {0x02, 0x00, cmd, 0x00, data};
    lcdc_write(sdat, sizeof(sdat));
}

static void LCD_CO5300_qspi_cmd_data(uint8_t cmd, uint8_t *data, uint32_t len)
{
    uint32_t len_buf = len + 4;
    uint8_t sdat[60] = {0};
    sdat[0] = 0x02;
    sdat[1] = 0x00;
    sdat[2] = cmd;
    sdat[3] = 0x00;

    for (uint8_t i = 0; i < len; i++)
    {
        sdat[4 + i] = data[i];
    }

    lcdc_write(sdat, len_buf);
}

static void LCD_CO5300_qspi_write_cmd(uint8_t cmd)
{
    uint8_t sdat[] = {0x02, 0x00, cmd, 0x00};
    lcdc_write(sdat, sizeof(sdat));
}

//total 4 byte, first byte is 0x32
static void rtl_qspi_enter_data_output_mode(uint32_t len_byte)
{
    LCDC_AXIMUXMode(LCDC_FW_MODE);
    DBIC_SwitchMode(DBIC_USER_MODE);
    DBIC_SwitchDirect(DBIC_TMODE_TX);
    DBIC_CmdLength(1);
    DBIC_AddrLength(3);
    DBIC_TX_NDF(len_byte);

    DBIC->CTRLR0 &= ~(BIT_CMD_CH(3) | BIT_ADDR_CH(3) | BIT_DATA_CH(3));//SET CHANNEL NUM
    DBIC->CTRLR0 |= (BIT_CMD_CH(0) | BIT_ADDR_CH(0) | BIT_DATA_CH(2));

    DBIC->FLUSH_FIFO = 0x01;

    /* must push cmd and address to handler before SPIC enable */
    LCDC_SPICCmd(0x32);
    LCDC_SPICAddr(0x002c00);


    DBIC->DMACR = 2;

    /* change this value can not influence the result. the wave is split into two periods. the first is 32 bytes. */
    DBIC->DMATDLR = 0x20;
//    DBIC_Sequence_Write(ENABLE);
    DBIC->ICR = 1;
    LCDC_AXIMUXMode(LCDC_HW_MODE);
}


static void lcd390X450_co5300_init(void)
{
    uint8_t data_buff[10] = {0};

    LCD_CO5300_qspi_cmd_data_byte(0xFE, 0x00);
    LCD_CO5300_qspi_cmd_data_byte(0xC4, 0x80);
    LCD_CO5300_qspi_cmd_data_byte(0x3A, 0x55);//55:RGE565 77:RGB888
    LCD_CO5300_qspi_cmd_data_byte(0x35, 0x00);//TE
    LCD_CO5300_qspi_cmd_data_byte(0x53, 0x20);
    //LCD_CO5300_qspi_cmd_data_byte(0x51, 0xFF);
    LCD_CO5300_qspi_cmd_data_byte(0x63, 0xFF);

    data_buff[0] = 0x00;
    data_buff[1] = 0x00;
    data_buff[2] = 0x01;
    data_buff[3] = 0x85;
    LCD_CO5300_qspi_cmd_data(0x2A, data_buff, 4);

    data_buff[0] = 0x00;
    data_buff[1] = 0x00;
    data_buff[2] = 0x01;
    data_buff[3] = 0xC1;
    LCD_CO5300_qspi_cmd_data(0x2B, data_buff, 4);

    LCD_CO5300_qspi_write_cmd(0x11);
    platform_delay_ms(60);
    LCD_CO5300_qspi_write_cmd(0x29);
}


void rtk_lcd_hal_init(void)
{
    Pad_Config(P4_4, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW); //for power
    Pad_Config(P0_2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH); //for power
    Pad_Config(P0_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH); //for power
    platform_delay_ms(10);

    LCDC_Clock_Cfg(ENABLE);
    RCC_PeriphClockCmd(APBPeriph_DISP, APBPeriph_DISP_CLOCK, ENABLE);
    //TODO
    qspi_pad_config();
    LCDC_InitTypeDef lcdc_init = {0};
    lcdc_init.LCDC_Interface = LCDC_IF_DBIC;
#if INPUT_PIXEL_BYTES == 4
    lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_ARGB8888;
#elif INPUT_PIXEL_BYTES == 3
    lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_RGB888;
#elif INPUT_PIXEL_BYTES == 2
    lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_RGB565;
#endif
    lcdc_init.LCDC_PixelOutputFormat = LCDC_OUTPUT_RGB565;
    lcdc_init.LCDC_PixelBitSwap = LCDC_SWAP_BYPASS; //lcdc_handler_cfg->LCDC_TeEn = LCDC_TE_DISABLE;
#if TE_VALID
    lcdc_init.LCDC_TeEn = ENABLE;
    lcdc_init.LCDC_TePolarity = LCDC_TE_EDGE_FALLING;
    lcdc_init.LCDC_TeInputMux = LCDC_TE_LCD_INPUT;
#endif
    lcdc_init.LCDC_GroupSel = 1;
    lcdc_init.LCDC_DmaThreshold =
        64;    // MSize + threshold should be no larger than 128
    LCDC_Init(&lcdc_init);
    LCDC_ClearINTPendingBit(LCDC_CLR_WAVEFORM_FINISH);

    RCC_DisplayClockConfig(DISPLAY_CLOCK_SOURCE_PLL1, DISPLAY_CLOCK_DIV_1);

    LCDC_DBICCfgTypeDef dbic_init = {0};
    dbic_init.DBIC_SPEED_SEL         = 2;

    dbic_init.DBIC_TxThr             = 0;
    dbic_init.DBIC_RxThr             = 0;
    dbic_init.SCPOL                  = DBIC_SCPOL_LOW;
    dbic_init.SCPH                   = DBIC_SCPH_1Edge;
    DBIC_Init(&dbic_init);

    LCDC_SwitchMode(LCDC_MANUAL_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    LCDC_Cmd(ENABLE);

    Pad_Config(P4_4, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    platform_delay_ms(20);
    Pad_Config(P4_4, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    platform_delay_ms(20);
    Pad_Config(P4_4, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    platform_delay_ms(120);

    LCDC_AXIMUXMode(LCDC_FW_MODE);
    DBIC_SwitchMode(DBIC_USER_MODE);
    DBIC_SwitchDirect(DBIC_TMODE_TX);

    lcd390X450_co5300_init();

    LCD_CO5300_qspi_cmd_data_byte(0x51, 0xFF);// power or bright

    DBIC_IMR_TypeDef dbic_reg_0x2c = {.d32 = DBIC->IMR};
    dbic_reg_0x2c.b.dreim = 1;
    DBIC->IMR = dbic_reg_0x2c.d32;

    rtk_lcd_hal_clear_screen(0x00FF0000);
}

uint32_t rtk_lcd_hal_get_width(void)
{
    return C05300_LCD_WIDTH;
}
uint32_t rtk_lcd_hal_get_height(void)
{
    return C05300_LCD_HEIGHT;
}

uint32_t rtk_lcd_hal_get_pixel_bits(void)
{
    return C05300_DRV_PIXEL_BITS;
}


void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len)
{
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
    LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = (uint32_t)buf;
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = 0;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);

    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();

    LCDC_SwitchMode(LCDC_AUTO_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    LCDC_ForceBurst(ENABLE);
    LCDC_SetTxPixelLen(len);

    LCDC_Cmd(ENABLE);
    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);
#if (TE_VALID == 1)
    if (use_TE == LCDC_TE_TYPE_HW_TE)
    {
        LCDC_TeCmd(ENABLE);
        LCDC_TeEnableDMA(DISABLE);
    }
    else
    {
        LCDC_AutoWriteCmd(ENABLE);
    }
#endif
#if (TE_VALID == 0)
    LCDC_AutoWriteCmd(ENABLE);
#endif
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
    LCDC_HANDLER_TX_LEN_TypeDef handler_reg_0x28;
    LCDC_HANDLER_TX_CNT_TypeDef handler_reg_0x2c;

    do
    {
        handler_reg_0x14.d32 = LCDC_HANDLER->OPERATE_CTR;
    }
    while (handler_reg_0x14.b.auto_write_start != RESET);
    do
    {
        handler_reg_0x28.d32 = LCDC_HANDLER->TX_LEN;
        handler_reg_0x2c.d32 = LCDC_HANDLER->TX_CNT;
    }
    while (handler_reg_0x2c.b.tx_output_pixel_cnt < handler_reg_0x28.b.tx_output_pixel_num);

    platform_delay_us(1);

#if (TE_VALID == 1)
    if (use_TE == LCDC_TE_TYPE_HW_TE)
    {
        LCDC_TeCmd(DISABLE);
    }
#endif
    LCDC_Cmd(DISABLE);
    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();
    LCDC_AXIMUXMode(LCDC_FW_MODE);
    DBIC_Cmd(DISABLE);

    DBIC_FLUSH_FIFO_TypeDef dbic_reg_0x128 = {.d32 = DBIC->FLUSH_FIFO};
    dbic_reg_0x128.b.flush_dr_fifo = 1;
    DBIC->FLUSH_FIFO = dbic_reg_0x128.d32;
    LCDC_ClearINTPendingBit(LCDC_CLR_WAVEFORM_FINISH);
}


void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h)
{
    uint8_t data[4];
    uint16_t xEnd = xStart + w - 1;
    uint16_t yEnd = yStart + h - 1;
    data[0] = xStart >> 8;
    data[1] = xStart & 0xff;
    data[2] = xEnd >> 8;
    data[3] = xEnd & 0xff;
    LCD_CO5300_qspi_cmd_data(0x2A, data, 4);


    data[0] = yStart >> 8;
    data[1] = yStart & 0xff;
    data[2] = yEnd >> 8;
    data[3] = yEnd & 0xff;
    LCD_CO5300_qspi_cmd_data(0x2B, data, 4);

    uint32_t len_byte = (xEnd - xStart + 1) * (yEnd - yStart + 1) * OUTPUT_PIXEL_BYTES;
    rtl_qspi_enter_data_output_mode(len_byte);
}



void rtk_lcd_hal_clear_screen(uint32_t ARGB_color)
{
    rtk_lcd_hal_set_window(0, 0, C05300_LCD_WIDTH, C05300_LCD_HEIGHT);
    uint8_t *RGB_transfer = (uint8_t *)&ARGB_color;
    uint32_t clear_buf[64] = {0};
#if INPUT_PIXEL_BYTES == 4
    uint32_t rgba8888 = RGB_transfer[3] << 24 | RGB_transfer[0] << 16 | RGB_transfer[1] << 8 |
                        RGB_transfer[2];
    for (int i = 0; i < 64; i++)
    {
        clear_buf[i] = rgba8888;
    }
#elif INPUT_PIXEL_BYTES == 3
    uint8_t *rgb888_buf = (uint8_t *)clear_buf;
    for (int i = 0; i < 64; i++)
    {
        rgb888_buf[i * 3] = RGB_transfer[0];
        rgb888_buf[i * 3 + 1] = RGB_transfer[1];
        rgb888_buf[i * 3 + 2] = RGB_transfer[2];
    }
#elif INPUT_PIXEL_BYTES == 2
    uint16_t color = 0;
    uint16_t *rgb565_buf = (uint16_t *)clear_buf;
    color = (((RGB_transfer[0] & 0xF8) << 8) | ((RGB_transfer[1] & 0xFC) << 3) | ((
            RGB_transfer[2] & 0xF8) >> 3));
    for (int i = 0; i < 64 * 2; i++)
    {
        rgb565_buf[i] = color;
    }
#endif
    LCDC_DMA_InitTypeDef LCDC_DMA_InitStruct = {0};
    LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
    LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum          = LCDC_DMA_CHANNEL_NUM;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceInc           = LCDC_DMA_SourceInc_Fix;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc      = LCDC_DMA_DestinationInc_Fix;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize      = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_64;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_64;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = (uint32_t)clear_buf;
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = 0;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);

    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();

    LCDC_SwitchMode(LCDC_AUTO_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);

    LCDC_SetTxPixelLen(C05300_LCD_WIDTH * C05300_LCD_HEIGHT);

    LCDC_Cmd(ENABLE);

    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);

    LCDC_AutoWriteCmd(ENABLE);

    while ((LCDC_HANDLER->DMA_FIFO_CTRL & LCDC_DMA_ENABLE) != RESET);//wait dma finish


    LCDC_HANDLER_OPERATE_CTR_t handler_reg_0x14;
    do
    {
        handler_reg_0x14.d32 = LCDC_HANDLER->OPERATE_CTR;
    }
    while (handler_reg_0x14.b.auto_write_start != RESET);

    LCDC_Cmd(DISABLE);
    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();
    LCDC_AXIMUXMode(LCDC_FW_MODE);
}

void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len)
{
    LCDC_DMA_InitTypeDef LCDC_DMA_InitStruct = {0};
    LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
    LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum          = LCDC_DMA_CHANNEL_NUM;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceInc           = LCDC_DMA_SourceInc_Inc;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc      = LCDC_DMA_DestinationInc_Fix;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize      = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_64;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_64;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = (uint32_t)buf;
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = 0;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);

    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();

    LCDC_SwitchMode(LCDC_AUTO_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);

    LCDC_SetTxPixelLen(len);

    LCDC_Cmd(ENABLE);
    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);
#if (TE_VALID == 1)
    LCDC_HANDLER_TEAR_CTR_t handler_reg_0x10 = {.d32 = LCDC_HANDLER->TEAR_CTR};
    handler_reg_0x10.b.bypass_t2w_delay = 0;
    handler_reg_0x10.b.t2w_delay = 0xfff;
    LCDC_HANDLER->TEAR_CTR = handler_reg_0x10.d32;
    LCDC_TeCmd(ENABLE);
    LCDC_TeEnableDMA(DISABLE);
#else
    LCDC_AutoWriteCmd(ENABLE);
#endif
    while ((LCDC_HANDLER->DMA_FIFO_CTRL & LCDC_DMA_ENABLE) != RESET)//wait dma finish
    {
        os_delay(1);
    }
    while (((LCDC_HANDLER->DMA_FIFO_OFFSET & LCDC_DMA_TX_FIFO_OFFSET) != RESET) &&
           (LCDC_HANDLER->TX_CNT == LCDC_HANDLER->TX_LEN));//wait lcd tx cnt finish
#if (TE_VALID == 1)
    LCDC_TeCmd(DISABLE);                            // disable Tear trigger auto_write_start
#endif
    LCDC_Cmd(DISABLE);
    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();
    LCDC_AXIMUXMode(LCDC_FW_MODE);
}


void rtk_lcd_hal_lcd_enter_dlps(void)
{
    /*enter dlps*/
    /*QSPI and TE Pad Config */
    Pad_Config(P9_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(P9_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(P9_2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(P9_3, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(P9_4, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(P9_5, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(P2_2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pad_HighSpeedMuxSel(P9_0, FROM_AON_MUX);
    Pad_HighSpeedMuxSel(P9_1, FROM_AON_MUX);
    Pad_HighSpeedMuxSel(P9_2, FROM_AON_MUX);
    Pad_HighSpeedMuxSel(P9_3, FROM_AON_MUX);
    Pad_HighSpeedMuxSel(P9_4, FROM_AON_MUX);
    Pad_HighSpeedMuxSel(P9_5, FROM_AON_MUX);
    Pad_HighSpeedMuxSel(P2_2, FROM_AON_MUX);

}


void rtk_lcd_hal_update_fb_use_up_and_down(uint32_t up,
                                           uint32_t down,
                                           uint32_t w,
                                           uint32_t h,
                                           uint32_t offset,
                                           uint32_t bytes_per_pixel)
{
    DBG_DIRECT("up = 0x%x, down = 0x%x, w = %d ,h = %d, offset = %d, bytes_per_pixel = %d", up,
               down, w, h, offset, bytes_per_pixel);
    if (((uint32_t)up % 4) != 0)
    {
        assert_param("buf not 4byte aligned!");
        while (1);
    }
    if (((uint32_t)down % 4) != 0)
    {
        assert_param("buf not 4byte aligned!");
        while (1);
    }
    if (offset == 0)
    {
        rtk_lcd_hal_start_transfer((uint8_t *)up, w * h);
    }
    else if (offset < h)
    {
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
        LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = (uint32_t)up;


        LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_Mode   =
            LLI_TRANSFER;//LLI_TRANSFER or LLI_WITH_CONTIGUOUS_SAR
        LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = 1;
        LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_Struct  = LCDC_DMA_LINKLIST_REG_BASE + 0x50;
        LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);

        LCDC_SET_GROUP1_BLOCKSIZE((h - offset) * w * bytes_per_pixel);
        LCDC_SET_GROUP2_BLOCKSIZE(offset * w * bytes_per_pixel);


        LCDC_DMALLI_InitTypeDef LCDC_DMA_LLI_Init = {0};
        LCDC_DMA_LLI_Init.g1_source_addr = (uint32_t)up + offset * w * bytes_per_pixel;
        LCDC_DMA_LLI_Init.g1_sar_offset = 0; //offset means interval, no need set here

        LCDC_DMA_LLI_Init.g2_source_addr = (uint32_t)down;
        LCDC_DMA_LLI_Init.g2_sar_offset = 0;  //offset means interval, no need set here
        LCDC_DMA_LinkList_Init(&LCDC_DMA_LLI_Init,
                               &LCDC_DMA_InitStruct);//LLI_TRANSFER or LLI_WITH_CONTIGUOUS_SAR

        LCDC_ClearDmaFifo();
        LCDC_ClearTxPixelCnt();

        LCDC_SwitchMode(LCDC_AUTO_MODE);
        LCDC_SwitchDirect(LCDC_TX_MODE);
        LCDC_ForceBurst(ENABLE);
        LCDC_SetTxPixelLen(w * h);

        LCDC_Cmd(ENABLE);
        LCDC_DMA_MultiBlockCmd(ENABLE);
        LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
        LCDC_DmaCmd(ENABLE);

        LCDC_AutoWriteCmd(ENABLE);
    }
    else if (offset == w)
    {
        rtk_lcd_hal_start_transfer((uint8_t *)down, w * h);
    }
    else
    {
        assert_param("offset error!");
        while (1);
    }

    rtk_lcd_hal_transfer_done();
}

void rtk_lcd_hal_update_fb_use_left_and_right(uint32_t left,
                                              uint32_t right,
                                              uint32_t w,
                                              uint32_t h,
                                              uint32_t offset,
                                              uint32_t bytes_per_pixel)
{
    offset = (offset / 2) * 2;
    DBG_DIRECT("left = 0x%x, right = 0x%x, w = %d ,h = %d, offset = %d, bytes_per_pixel = %d", left,
               right, w, h, offset, bytes_per_pixel);
    if (((uint32_t)left % 4) != 0)
    {
        assert_param("buf not 4byte aligned!");
        while (1);
    }
    if (((uint32_t)right % 4) != 0)
    {
        assert_param("buf not 4byte aligned!");
        while (1);
    }
    if (offset == 0)
    {
        rtk_lcd_hal_start_transfer((uint8_t *)left, w * h);
    }
    else if (offset < w)
    {
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
        LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = (uint32_t)left;


        LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_Mode   =
            LLI_TRANSFER;//LLI_TRANSFER or LLI_WITH_CONTIGUOUS_SAR
        LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = 1;
        LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_Struct  = LCDC_DMA_LINKLIST_REG_BASE + 0x50;
        LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);

        LCDC_SET_GROUP1_BLOCKSIZE((w - offset) * bytes_per_pixel);
        LCDC_SET_GROUP2_BLOCKSIZE(offset * bytes_per_pixel);



        LCDC_DMALLI_InitTypeDef LCDC_DMA_LLI_Init = {0};
        LCDC_DMA_LLI_Init.g1_source_addr = (uint32_t)left + offset * bytes_per_pixel;
        LCDC_DMA_LLI_Init.g1_sar_offset = w * bytes_per_pixel;

        LCDC_DMA_LLI_Init.g2_source_addr = (uint32_t)right;
        LCDC_DMA_LLI_Init.g2_sar_offset = w * bytes_per_pixel;
        LCDC_DMA_LinkList_Init(&LCDC_DMA_LLI_Init,
                               &LCDC_DMA_InitStruct);//LLI_TRANSFER or LLI_WITH_CONTIGUOUS_SAR

        LCDC_ClearDmaFifo();
        LCDC_ClearTxPixelCnt();

        LCDC_SwitchMode(LCDC_AUTO_MODE);
        LCDC_SwitchDirect(LCDC_TX_MODE);
        LCDC_ForceBurst(ENABLE);
        LCDC_SetTxPixelLen(w * h);

        LCDC_Cmd(ENABLE);
        LCDC_DMA_MultiBlockCmd(ENABLE);
        LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
        LCDC_DmaCmd(ENABLE);

        LCDC_AutoWriteCmd(ENABLE);
    }
    else if (offset == w)
    {
        rtk_lcd_hal_start_transfer((uint8_t *)right, w * h);
    }
    else
    {
        assert_param("offset error!");
        while (1);
    }

    rtk_lcd_hal_transfer_done();

}
