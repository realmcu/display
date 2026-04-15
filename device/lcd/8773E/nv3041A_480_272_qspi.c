/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "board.h"
#include "rtl_lcdc_dbic.h"
#include "platform_utils.h"
#include "trace.h"
#include "os_sched.h"
#include "rtl876x_rcc.h"
#include "nv3041A_480_272_qspi.h"
#include "rtl_lcdc_dbic.h"
#include "rtl876x_pinmux.h"
#include "rtl_lcdc.h"

#define LCDC_DMA_CHANNEL_NUM              0
#define LCDC_DMA_CHANNEL_INDEX            LCDC_DMA_Channel0


#define BIT_CMD_CH(x)           (((x) & 0x00000003) << 20)
#define BIT_DATA_CH(x)          (((x) & 0x00000003) << 18)
#define BIT_ADDR_CH(x)          (((x) & 0x00000003) << 16)
#define BIT_TMOD(x)             (((x) & 0x00000003) << 8)

#define BIT_TXSIM               (0x00000001 << 9)
#define BIT_SEQ_EN              (0x00000001 << 3)


#define QSPI_LCD_TE             P2_2
#define QSPI_CSN                P2_3
#define QSPI_DCX                P2_4
#define QSPI_CLK                P2_5
#define QSPI_SIO0               P2_6
#define QSPI_SIO1               P2_7
#define QSPI_SIO2               P4_0
#define QSPI_SIO3               P4_1

#define QSPI_SIO4               P4_2
#define QSPI_SIO5               P4_3
#define QSPI_SIO6               P4_4
#define QSPI_SIO7               P4_5
#define QSPI_LCD_RESX           P9_0

#define NV3041A_LCD_BL                  ADC_3
#define NV3041A_LCD_IM0                 P3_2
#define NV3041A_LCD_IM1                 P3_3

static void spic3_spi_write(uint8_t *buf, uint32_t len)
{
    DBIC->CTRLR0 |= BIT31;
    DBIC->CTRLR0 &= ~(BIT_CMD_CH(3) | BIT_ADDR_CH(3) | BIT_DATA_CH(3));//SET CHANNEL NUM
    DBIC->CTRLR0 &= ~(BIT_TMOD(3)); //tx mode

    DBIC_CmdLength(0);
    DBIC_AddrLength(0);
    DBIC_TX_NDF(len);
    for (uint32_t i = 0; i < len; i++)
    {
        DBIC->DR[0].byte = buf[i];
    }
    DBIC_Cmd(ENABLE);
    while (DBIC->SR & BIT0);// wait bus busy
    //DBIC_Cmd(DISABLE);//disable DBIC
    DBIC->CTRLR0 &= ~BIT31;
}

//
//static void rtl_lcd_qspi_write_cmd(uint16_t cmd) //total 4 byte, first byte is 0x02
//{
//    uint8_t sdat[] = {0x02, 0x00, cmd, 0x00};
//    spic3_spi_write(sdat, sizeof(sdat));
//}



static void rtl_lcd_qspi_cmd_param(uint8_t cmd, uint8_t data) //total 5 byte, first byte is 0x02
{
    uint8_t sdat[] = {0x02, 0x00, cmd, 0x00, data};
    spic3_spi_write(sdat, sizeof(sdat));
}


static void rtl_lcd_qspi_cmd_param4(uint8_t cmd, uint8_t *data) //total 8 byte, first byte is 0x02
{
    uint8_t sdat[] = {0x02, 0x00, cmd, 0x00, data[0], data[1], data[2], data[3]};
    spic3_spi_write(sdat, sizeof(sdat));
}



static void rtl_lcd_qspi_enter_data_output_mode(uint32_t
                                                len_byte) //total 4 byte, first byte is 0x32
{
    LCDC_AXIMUXMode(LCDC_FW_MODE);
    DBIC_SwitchMode(DBIC_USER_MODE);
    DBIC_SwitchDirect(DBIC_TMODE_TX);
    DBIC_CmdLength(1);
    DBIC_AddrLength(3);
    DBIC_TX_NDF(len_byte);

    DBIC->CTRLR0 &= ~(BIT_CMD_CH(3) | BIT_ADDR_CH(3) | BIT_DATA_CH(3));//SET CHANNEL NUM
    DBIC->CTRLR0 |= (BIT_CMD_CH(0) | BIT_ADDR_CH(0) | BIT_DATA_CH(2));

    /* must push cmd and address to handler before SPIC enable */
    LCDC_SPICCmd(0x32);
    LCDC_SPICAddr(0x002c00);


    DBIC->DMACR = 2;

    /* change this value can not influence the result. the wave is split into two periods. the first is 32 bytes. */
    DBIC->DMATDLR = 0x20;

    LCDC_AXIMUXMode(LCDC_HW_MODE);
}


void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h)
{
    uint8_t data[4];
    uint16_t xEnd = xStart + w - 1;
    uint16_t yEnd = yStart + h - 1;
    xStart = xStart + 0;
    xEnd = xEnd + 0;
    yStart = yStart + 0;
    yEnd = yEnd + 0;

    data[0] = xStart >> 8;
    data[1] = xStart & 0xff;
    data[2] = xEnd >> 8;
    data[3] = xEnd & 0xff;
    // DBG_DIRECT("0x2A, 0x%x,0x%x,0x%x,0x%x", data[0], data[1], data[2], data[3]);
    rtl_lcd_qspi_cmd_param4(0x2A, data);


    data[0] = yStart >> 8;
    data[1] = yStart & 0xff;
    data[2] = yEnd >> 8;
    data[3] = yEnd & 0xff;
    rtl_lcd_qspi_cmd_param4(0x2B, data);

    uint32_t len_byte = (xEnd - xStart + 1) * (yEnd - yStart + 1) * OUTPUT_PIXEL_BYTES;
    rtl_lcd_qspi_enter_data_output_mode(len_byte);
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
    LCDC_ForceBurst(true);
    LCDC_Cmd(ENABLE);
    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);
#if (TE_VALID == 1)
    // LCDC_HANDLER_TEAR_CTR_t handler_reg_0x10 = {.d32 = LCDC_HANDLER->TEAR_CTR};
    // handler_reg_0x10.b.bypass_t2w_delay = 0;
    // handler_reg_0x10.b.t2w_delay = 0xfff;
    // LCDC_HANDLER->TEAR_CTR = handler_reg_0x10.d32;
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
    DBIC_Cmd(DISABLE);

    DBIC_FLUSH_FIFO_t dbic_reg_0x128 = {.d32 = DBIC->FLUSH_FIFO};
    dbic_reg_0x128.b.flush_dr_fifo = 1;
    DBIC->FLUSH_FIFO = dbic_reg_0x128.d32;
}

void rtk_lcd_hal_clear_screen(uint32_t ARGB_color)
{
    rtk_lcd_hal_set_window(0, 0, NV3041A_LCD_WIDTH, NV3041A_LCD_HIGHT);
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
    uint16_t color_swap = (color >> 8) | (color << 8);
    for (int i = 0; i < 64 * 2; i++)
    {
        rgb565_buf[i] = color;
    }
    // color high byte send first, which is B (ARGB), so swap it
    // -> ABGR
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

    LCDC_SetTxPixelLen(NV3041A_LCD_WIDTH * NV3041A_LCD_HIGHT);

    LCDC_Cmd(ENABLE);

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
    LCDC_AXIMUXMode(LCDC_FW_MODE);
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

    LCDC_SetTxPixelLen(len);

    LCDC_Cmd(ENABLE);
    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);
#if (TE_VALID == 1)
    LCDC_TeCmd(ENABLE);
    LCDC_TeEnableDMA(DISABLE);
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
    LCDC_HANDLER_TX_LEN_t handler_reg_0x28;
    LCDC_HANDLER_TX_CNT_t handler_reg_0x2c;

    do
    {
        handler_reg_0x14.d32 = LCDC_HANDLER->OPERATE_CTR;
        handler_reg_0x28.d32 = LCDC_HANDLER->TX_LEN;
        handler_reg_0x2c.d32 = LCDC_HANDLER->TX_CNT;
    }
    while (handler_reg_0x14.b.auto_write_start != RESET &&
           handler_reg_0x2c.b.tx_output_pixel_cnt < handler_reg_0x28.b.tx_output_pixel_num);

#if (TE_VALID == 1)
    LCDC_TeCmd(DISABLE);
#endif
    LCDC_Cmd(DISABLE);
    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();
    LCDC_AXIMUXMode(LCDC_FW_MODE);
    DBIC_Cmd(DISABLE);

    DBIC_FLUSH_FIFO_t dbic_reg_0x128 = {.d32 = DBIC->FLUSH_FIFO};
    dbic_reg_0x128.b.flush_dr_fifo = 1;
    DBIC->FLUSH_FIFO = dbic_reg_0x128.d32;
    LCDC_ClearINTPendingBit(LCDC_CLR_WAVEFORM_FINISH);
}

uint32_t rtk_lcd_hal_get_width(void)
{
    return NV3041A_LCD_WIDTH;
}
uint32_t rtk_lcd_hal_get_height(void)
{
    return NV3041A_LCD_HIGHT;
}
uint32_t rtk_lcd_hal_get_pixel_bits(void)
{
    return NV3041A_DRV_PIXEL_BITS;
}


static void lcd_pad_init(void)
{

    /*BL AND RESET ARE NOT FIX*/
    // BL
    Pad_Config(NV3041A_LCD_BL, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    // RESET
    Pad_Config(QSPI_LCD_RESX, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    // 0b'11 for QSPI
    Pad_Config(NV3041A_LCD_IM0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(NV3041A_LCD_IM1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);


    // QSPI
    Pad_Config(QSPI_LCD_TE, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(QSPI_CSN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    // Pad_Config(QSPI_DCX, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(QSPI_CLK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);

    Pad_Config(QSPI_SIO0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(QSPI_SIO1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(QSPI_SIO2, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(QSPI_SIO3, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);

    Pad_HighSpeedFuncSel(QSPI_LCD_TE, HS_Func0);
    Pad_HighSpeedFuncSel(QSPI_CSN, HS_Func0);
    Pad_HighSpeedFuncSel(QSPI_CLK, HS_Func0);
    Pad_HighSpeedFuncSel(QSPI_SIO0, HS_Func0);
    Pad_HighSpeedFuncSel(QSPI_SIO1, HS_Func0);
    Pad_HighSpeedFuncSel(QSPI_SIO2, HS_Func0);
    Pad_HighSpeedFuncSel(QSPI_SIO3, HS_Func0);

    Pad_HighSpeedMuxSel(QSPI_LCD_TE, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(QSPI_CSN, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(QSPI_CLK, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(QSPI_SIO0, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(QSPI_SIO1, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(QSPI_SIO2, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(QSPI_SIO3, FROM_CORE_DOMAIN);

}

static void lcd_set_reset(bool reset)
{
    // LCDC_LCD_SET_RST(reset);
    if (reset)
    {
        Pad_Config(QSPI_LCD_RESX, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }
    else
    {
        Pad_Config(QSPI_LCD_RESX, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
}

static void lcd_seq_init(void)
{


//************* Start Initial Sequence **********//
    rtl_lcd_qspi_cmd_param(0xff, 0xa5);

    rtl_lcd_qspi_cmd_param(0xE7, 0x10);//TE_output_en

    rtl_lcd_qspi_cmd_param(0x35, 0x00);//TE_ interface_en

    // rtl_lcd_qspi_cmd_param(0x36, 0xc0);
    rtl_lcd_qspi_cmd_param(0x36, 0x00);

#if 1
    rtl_lcd_qspi_cmd_param(0x38, 0xff);//idle off
#else
    rtl_lcd_qspi_cmd_param(0x39, 0xff);//idle on
#endif

    rtl_lcd_qspi_cmd_param(0x3A, 0x01);//01---565/00---666

    rtl_lcd_qspi_cmd_param(0x40, 0x01);//01:IPS/00:TN

    rtl_lcd_qspi_cmd_param(0x41, 0x03);//01--8bit//03--16bit

    rtl_lcd_qspi_cmd_param(0x43, 0x00);// 00 msb/01 lsb

    rtl_lcd_qspi_cmd_param(0x44, 0x15);//VBP

    rtl_lcd_qspi_cmd_param(0x45, 0x15); //VFP

    rtl_lcd_qspi_cmd_param(0x7d, 0x03);//vdds_trim[2:0]

    rtl_lcd_qspi_cmd_param(0xc1,
                           0xbb);//avdd_clp_en avdd_clp[1:0] avcl_clp_en avcl_clp[1:0]  //0xbb    88  a2

    rtl_lcd_qspi_cmd_param(0xc2, 0x05);//vgl_clp_en vgl_clp[2:0]

    rtl_lcd_qspi_cmd_param(0xc3, 0x10);//vgl_clp_en vgl_clp[2:0]

    rtl_lcd_qspi_cmd_param(0xc6,
                           0x3e);//avdd_ratio_sel avcl_ratio_sel vgh_ratio_sel[1:0] vgl_ratio_sel[1:0]

    rtl_lcd_qspi_cmd_param(0xc7, 0x25);//mv_clk_sel[1:0] avdd_clk_sel[1:0] avcl_clk_sel[1:0]

    rtl_lcd_qspi_cmd_param(0xc8, 0x21);// VGL_CLK_sel

    rtl_lcd_qspi_cmd_param(0x7a, 0x51);// user_vgsp  //58

    rtl_lcd_qspi_cmd_param(0x6f, 0x49);// user_gvdd  //4F

    rtl_lcd_qspi_cmd_param(0x78, 0x57);// user_gvcl  //70

    rtl_lcd_qspi_cmd_param(0xc9, 0x00);

    rtl_lcd_qspi_cmd_param(0x67, 0x11);

    rtl_lcd_qspi_cmd_param(0x51, 0x0a);//gate_ed //gate_st_o[7:0]

    rtl_lcd_qspi_cmd_param(0x52, 0x7D);

    rtl_lcd_qspi_cmd_param(0x53, 0x0a);

    rtl_lcd_qspi_cmd_param(0x54, 0x7D);

//sorce
    rtl_lcd_qspi_cmd_param(0x46, 0x0a);

    rtl_lcd_qspi_cmd_param(0x47, 0x2a);

    rtl_lcd_qspi_cmd_param(0x48, 0x0a);

    rtl_lcd_qspi_cmd_param(0x49, 0x1a);

    rtl_lcd_qspi_cmd_param(0x44, 0x15);

    rtl_lcd_qspi_cmd_param(0x45, 0x15);

    rtl_lcd_qspi_cmd_param(0x73, 0x08);

    rtl_lcd_qspi_cmd_param(0x74, 0x12);

    rtl_lcd_qspi_cmd_param(0x55, 0x01);

    rtl_lcd_qspi_cmd_param(0x56, 0x43);

    rtl_lcd_qspi_cmd_param(0x57, 0x42);

    rtl_lcd_qspi_cmd_param(0x58, 0x3c);

    rtl_lcd_qspi_cmd_param(0x59, 0x64);

    rtl_lcd_qspi_cmd_param(0x5a, 0x41);

    rtl_lcd_qspi_cmd_param(0x5b, 0x3C);

    rtl_lcd_qspi_cmd_param(0x5c, 0x02);

    rtl_lcd_qspi_cmd_param(0x5d, 0x3c);

    rtl_lcd_qspi_cmd_param(0x5e, 0x1f);

    rtl_lcd_qspi_cmd_param(0x60, 0x80);

    rtl_lcd_qspi_cmd_param(0x61, 0x3f);

    rtl_lcd_qspi_cmd_param(0x62, 0x21);

    rtl_lcd_qspi_cmd_param(0x63, 0x07);

    rtl_lcd_qspi_cmd_param(0x64, 0xe0);

    rtl_lcd_qspi_cmd_param(0x65, 0x02);

    rtl_lcd_qspi_cmd_param(0xca, 0x20);

    rtl_lcd_qspi_cmd_param(0xcb, 0x52);

    rtl_lcd_qspi_cmd_param(0xcc, 0x10);

    rtl_lcd_qspi_cmd_param(0xcD, 0x42);

    rtl_lcd_qspi_cmd_param(0xD0, 0x20);

    rtl_lcd_qspi_cmd_param(0xD1, 0x52);

    rtl_lcd_qspi_cmd_param(0xD2, 0x10);

    rtl_lcd_qspi_cmd_param(0xD3, 0x42);

    rtl_lcd_qspi_cmd_param(0xD4, 0x0a);

    rtl_lcd_qspi_cmd_param(0xD5, 0x32);

//gammma
    rtl_lcd_qspi_cmd_param(0x80, 0x00);

    rtl_lcd_qspi_cmd_param(0xA0, 0x00);

    rtl_lcd_qspi_cmd_param(0x81, 0x06);

    rtl_lcd_qspi_cmd_param(0xA1, 0x08);

    rtl_lcd_qspi_cmd_param(0x82, 0x03);

    rtl_lcd_qspi_cmd_param(0xA2, 0x03);

    rtl_lcd_qspi_cmd_param(0x86, 0x14);

    rtl_lcd_qspi_cmd_param(0xA6, 0x14);

    rtl_lcd_qspi_cmd_param(0x87, 0x2C);

    rtl_lcd_qspi_cmd_param(0xA7, 0x26);

    rtl_lcd_qspi_cmd_param(0x83, 0x37);

    rtl_lcd_qspi_cmd_param(0xA3, 0x37);

    rtl_lcd_qspi_cmd_param(0x84, 0x35);

    rtl_lcd_qspi_cmd_param(0xA4, 0x35);

    rtl_lcd_qspi_cmd_param(0x85, 0x3f);

    rtl_lcd_qspi_cmd_param(0xA5, 0x3f);

    rtl_lcd_qspi_cmd_param(0x88, 0x0A);

    rtl_lcd_qspi_cmd_param(0xA8, 0x0A);

    rtl_lcd_qspi_cmd_param(0x89, 0x13);

    rtl_lcd_qspi_cmd_param(0xA9, 0x12);

    rtl_lcd_qspi_cmd_param(0x8a, 0x18);

    rtl_lcd_qspi_cmd_param(0xAa, 0x19);

    rtl_lcd_qspi_cmd_param(0x8b, 0x0a);

    rtl_lcd_qspi_cmd_param(0xAb, 0x0a);

    rtl_lcd_qspi_cmd_param(0x8c, 0x17);

    rtl_lcd_qspi_cmd_param(0xAc, 0x0B);

    rtl_lcd_qspi_cmd_param(0x8d, 0x1A);

    rtl_lcd_qspi_cmd_param(0xAd, 0x09);

    rtl_lcd_qspi_cmd_param(0x8e, 0x1A);

    rtl_lcd_qspi_cmd_param(0xAe, 0x08);

    rtl_lcd_qspi_cmd_param(0x8f, 0x1F);

    rtl_lcd_qspi_cmd_param(0xAf, 0x00);

    rtl_lcd_qspi_cmd_param(0x90, 0x08);

    rtl_lcd_qspi_cmd_param(0xB0, 0x00);

    rtl_lcd_qspi_cmd_param(0x91, 0x10);

    rtl_lcd_qspi_cmd_param(0xB1, 0x06);

    rtl_lcd_qspi_cmd_param(0x92, 0x19);

    rtl_lcd_qspi_cmd_param(0xB2, 0x15);

    rtl_lcd_qspi_cmd_param(0xff, 0x00);

    rtl_lcd_qspi_cmd_param(0x11, 0x00);

    platform_delay_ms(50);

    rtl_lcd_qspi_cmd_param(0x29, 0x00);

    platform_delay_ms(50);
}

void rtk_lcd_hal_init(void)
{
    // clk source change to pll1
    RCC_DisplayClockConfig(DISPLAY_CLOCK_SOURCE_PLL1, DISPLAY_CLOCK_DIV_1);
    LCDC_Clock_Cfg(ENABLE);
    RCC_PeriphClockCmd(APBPeriph_DISP, APBPeriph_DISP_CLOCK, ENABLE);


    lcd_pad_init();

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
    lcdc_init.LCDC_DmaThreshold =
        64;    // MSize + threshold should be no larger than 128
    LCDC_Init(&lcdc_init);
    LCDC_ClearINTPendingBit(LCDC_CLR_WAVEFORM_FINISH);




    LCDC_DBICCfgTypeDef dbic_init = {0};
    dbic_init.DBIC_SPEED_SEL         = 3;

    dbic_init.DBIC_TxThr             = 0;
    dbic_init.DBIC_RxThr             = 0;
    dbic_init.SCPOL                  = DBIC_SCPOL_LOW;
    dbic_init.SCPH                   = DBIC_SCPH_1Edge;
    DBIC_Init(&dbic_init);

    LCDC_SwitchMode(LCDC_MANUAL_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    LCDC_Cmd(ENABLE);


    lcd_set_reset(true);
    platform_delay_ms(120);
    lcd_set_reset(false);
    platform_delay_ms(50);

    LCDC_AXIMUXMode(LCDC_FW_MODE);
    DBIC_SwitchMode(DBIC_USER_MODE);
    DBIC_SwitchDirect(DBIC_TMODE_TX);


    lcd_seq_init();

//   rtk_lcd_hal_rect_fill(0, 0, NV3041A_LCD_WIDTH, NV3041A_LCD_HIGHT, 0xF0F0F0F0);
    rtk_lcd_hal_clear_screen(0x00FF00f0);

    // uint8_t *pbuffer = 0x4000000;
    // for(uint64_t i=0; i < NV3041A_LCD_WIDTH * NV3041A_LCD_HIGHT * NV3041A_DRV_PIXEL_BITS/8; i+= NV3041A_DRV_PIXEL_BITS/8)
    // {
    //     pbuffer[i] = 0x1f;
    //     pbuffer[i+1] = 0x00;
    // }

    // rtk_lcd_hal_set_window(0, 0, NV3041A_LCD_WIDTH, NV3041A_LCD_HIGHT);
    // rtk_lcd_hal_start_transfer((uint8_t *)0x4000000, NV3041A_LCD_WIDTH * NV3041A_LCD_HIGHT * NV3041A_DRV_PIXEL_BITS/8);

}

uint32_t rtk_lcd_hal_power_on(void)
{
    return 0;
}

uint32_t rtk_lcd_hal_power_off(void)
{
    return 0;
}








