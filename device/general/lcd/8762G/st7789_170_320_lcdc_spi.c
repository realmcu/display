/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "lcd_st7789_170_320_spi.h"
#include "utils.h"
#include "rtl_pinmux.h"
#include "rtl_lcdc.h"
#include "rtl_lcdc_dbic.h"
#include "trace.h"
#include "rtl_rcc.h"
//#include "rtl_spi.h"

#define LCDC_DMA_CHANNEL_NUM              0
#define LCDC_DMA_CHANNEL_INDEX            LCDC_DMA_Channel0

#define BIT_CMD_CH(x)           (((x) & 0x00000003) << 20)
#define BIT_DATA_CH(x)          (((x) & 0x00000003) << 18)
#define BIT_ADDR_CH(x)          (((x) & 0x00000003) << 16)
#define BIT_TMOD(x)             (((x) & 0x00000003) << 8)

#define BIT_TXSIM               (0x00000001 << 9)
#define BIT_SEQ_EN              (0x00000001 << 3)



static void qspi_write(uint8_t *buf, uint32_t len)
{
    DBIC_Cmd(DISABLE);//disable DBIC
    DBIC->CTRLR0 &= ~(BIT_CMD_CH(3) | BIT_ADDR_CH(3) | BIT_DATA_CH(3));//SET CHANNEL NUM
    DBIC->CTRLR0 &= ~(BIT_TMOD(3)); //tx mode

    DBIC_CmdLength(1);
    DBIC_AddrLength(0);
    DBIC_TX_NDF(len - 1);

    DBIC_Cmd(ENABLE);
    for (uint32_t i = 0; i < len; i++)
    {
        DBIC->DR[0].byte = buf[i];
    }
    while (DBIC->SR & BIT0); // wait bus busy
    DBIC_Cmd(DISABLE);//disable DBIC
}

static void st7789_cmd(uint8_t cmd) //total 4 byte, first byte is 0x02
{
    uint8_t sdat[] = {cmd};
    qspi_write(sdat, sizeof(sdat));
}


static void st7789_cmd_param1(uint8_t cmd, uint8_t data) //total 5 byte, first byte is 0x02
{
    uint8_t sdat[] = {cmd, data};
    qspi_write(sdat, sizeof(sdat));
}

static void st7789_cmd_param2(uint8_t cmd, uint8_t *data) //total 6 byte, first byte is 0x02
{
    uint8_t sdat[] = {cmd, data[0], data[1]};
    qspi_write(sdat, sizeof(sdat));
}
static void st7789_cmd_param3(uint8_t cmd, uint8_t *data) //total 7 byte, first byte is 0x02
{
    uint8_t sdat[] = {cmd, data[0], data[1], data[2]};
    qspi_write(sdat, sizeof(sdat));
}
static void st7789_cmd_param4(uint8_t cmd, uint8_t *data) //total 8 byte, first byte is 0x02
{
    uint8_t sdat[] = {cmd,  data[0], data[1], data[2], data[3]};
    qspi_write(sdat, sizeof(sdat));
}

static void st7789_cmd_param5(uint8_t cmd, uint8_t *data) //total 9 byte, first byte is 0x02
{
    uint8_t sdat[] = {cmd,  data[0], data[1], data[2], data[3], data[4]};
    qspi_write(sdat, sizeof(sdat));
}

static void st7789_cmd_param14(uint8_t cmd, uint8_t *data) //total 18 byte, first byte is 0x02
{
    uint8_t sdat[] = {cmd,  data[0], data[1], data[2], data[3], data[4], data[5],
                      data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13]
                     };
    qspi_write(sdat, sizeof(sdat));
}

static void st7789_enter_data_output_mode(uint32_t len_byte) //total 4 byte, first byte is 0x32
{

    LCDC_AXIMUXMode(LCDC_FW_MODE);
    DBIC_SwitchMode(DBIC_USER_MODE);
    DBIC_SwitchDirect(DBIC_TMODE_TX);
    DBIC_CmdLength(1);
    DBIC_AddrLength(0);
    DBIC_TX_NDF(len_byte);

    DBIC->CTRLR0 &= ~(BIT_CMD_CH(3) | BIT_ADDR_CH(3) | BIT_DATA_CH(3));//SET CHANNEL NUM
    DBIC->CTRLR0 |= (BIT_CMD_CH(0) | BIT_ADDR_CH(0) | BIT_DATA_CH(0));

    /* must push cmd and address to handler before SPIC enable */
    LCDC_SPICCmd(0x2C);
//    LCDC_SPICAddr(0x002c00);


    DBIC->DMACR = 2;

    /* change this value can not influence the result. the wave is split into two periods. the first is 32 bytes. */
    DBIC->DMATDLR = 0x20;

    DBIC->FLUSH_FIFO = BIT1;
    LCDC_AXIMUXMode(LCDC_HW_MODE);
}

void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h)
{
    uint8_t data[4];
    uint16_t xEnd = xStart + w - 1;
    uint16_t yEnd = yStart + h - 1;

//    xStart = xStart + 0x1A;
//    xEnd = xEnd + 0x1A;

    data[0] = xStart >> 8;
    data[1] = xStart & 0xff;
    data[2] = xEnd >> 8;
    data[3] = xEnd & 0xff;
    st7789_cmd_param4(0x2A, data);


    data[0] = yStart >> 8;
    data[1] = yStart & 0xff;
    data[2] = yEnd >> 8;
    data[3] = yEnd & 0xff;
    st7789_cmd_param4(0x2B, data);

    uint32_t len_byte = (xEnd - xStart + 1) * (yEnd - yStart + 1) * OUTPUT_PIXEL_BYTES;
//    st7789_cmd(0x32);
//    st7789_cmd(0x2C);
    st7789_enter_data_output_mode(len_byte);

}

uint32_t rtk_lcd_hal_get_width(void)
{
    return ST7789_LCD_WIDTH;
}
uint32_t rtk_lcd_hal_get_height(void)
{
    return ST7789_LCD_HIGHT;
}

uint32_t rtk_lcd_hal_get_pixel_bits(void)
{
    return ST7789_DRV_PIXEL_BITS;
}

void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len)
{

#if (DMA_LINKLIST == 0)
    LCDC_DMA_InitTypeDef LCDC_DMA_InitStruct = {0};
    LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
    LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum          = LCDC_DMA_CHANNEL_NUM;
    LCDC_DMA_InitStruct.LCDC_DMA_DIR                 = 4;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceInc           = LCDC_DMA_SourceInc_Inc;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc      = LCDC_DMA_DestinationInc_Fix;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize      = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_64;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_64;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = (uint32_t)buf;
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = 0;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);
#else
    LCDC_DMA_InitTypeDef LCDC_DMA_InitStruct = {0};
    LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
    LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum          = LCDC_DMA_CHANNEL_NUM;
    LCDC_DMA_InitStruct.LCDC_DMA_DIR                 = 4;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceInc           = LCDC_DMA_SourceInc_Inc;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc      = LCDC_DMA_DestinationInc_Fix;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize      = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_64;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_64;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = 0;

    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_Mode   =
        LLI_TRANSFER;//LLI_TRANSFER or LLI_WITH_CONTIGUOUS_SAR
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = 1;
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_Struct  = LCDC_DMA_LINKLIST_REG_BASE + 0x50;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);

    LCDC_SET_GROUP1_BLOCKSIZE(ST7789_LCD_WIDTH * INPUT_PIXEL_BYTES);
    LCDC_SET_GROUP2_BLOCKSIZE(ST7789_LCD_WIDTH * INPUT_PIXEL_BYTES);

    /*16 pixel aligned for GPU*/
    LCDC_DMALLI_InitTypeDef LCDC_DMA_LLI_Init = {0};
    LCDC_DMA_LLI_Init.g1_source_addr = (uint32_t)buf;
    LCDC_DMA_LLI_Init.g1_sar_offset = ST7789_LCD_WIDTH * INPUT_PIXEL_BYTES * 2;

    LCDC_DMA_LLI_Init.g2_source_addr = (uint32_t)(buf + ST7789_LCD_WIDTH * INPUT_PIXEL_BYTES);
    LCDC_DMA_LLI_Init.g2_sar_offset = ST7789_LCD_WIDTH * INPUT_PIXEL_BYTES * 2;
    LCDC_DMA_LinkList_Init(&LCDC_DMA_LLI_Init,
                           &LCDC_DMA_InitStruct);//LLI_TRANSFER or LLI_WITH_CONTIGUOUS_SAR
#endif

    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();

    LCDC_SwitchMode(LCDC_AUTO_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);

    LCDC_SetTxPixelLen(len);

    LCDC_Cmd(ENABLE);
#if DMA_LINKLIST
    LCDC_DMA_MultiBlockCmd(ENABLE);
#endif
    LCDC_ForceBurst(ENABLE);
    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);
#if (TE_VALID == 1)
    LCDC_TeCmd(ENABLE);
#endif
#if (TE_VALID == 0)
    LCDC_AutoWriteCmd(ENABLE);
#endif

}
void rtk_lcd_hal_transfer_done(void)
{
    LCDC_HANDLER_DMA_FIFO_CTRL_TypeDef handler_reg_0x18;
    do
    {
        handler_reg_0x18.d32 = LCDC_HANDLER->DMA_FIFO_CTRL;
    }
    while (handler_reg_0x18.b.dma_enable != RESET);
    LCDC_HANDLER_OPERATE_CTR_TypeDef handler_reg_0x14;
    do
    {
        handler_reg_0x14.d32 = LCDC_HANDLER->OPERATE_CTR;
    }
    while (handler_reg_0x14.b.auto_write_start != RESET);

    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();
    LCDC_AXIMUXMode(LCDC_FW_MODE);
    LCDC_Cmd(DISABLE);
    LCDC_ClearINTPendingBit(LCDC_CLR_WAVEFORM_FINISH);
}

void rtk_lcd_hal_rect_fill(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h, uint32_t color)
{
    LCDC_DMA_InitTypeDef LCDC_DMA_InitStruct = {0};
    LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
    LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum          = LCDC_DMA_CHANNEL_NUM;
    LCDC_DMA_InitStruct.LCDC_DMA_DIR                 = 4;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceInc           = LCDC_DMA_SourceInc_Fix;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc      = LCDC_DMA_DestinationInc_Fix;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize      = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_1;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_8;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = (uint32_t)(&color);
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = 0;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);

    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();

    LCDC_SwitchMode(LCDC_AUTO_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);

    LCDC_SetTxPixelLen(w * h);

    LCDC_Cmd(ENABLE);

    LCDC_ForceBurst(ENABLE);
    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);

#if (TE_VALID == 1)
    LCDC_TeCmd(ENABLE);
#endif
#if (TE_VALID == 0)
    LCDC_AutoWriteCmd(ENABLE);
#endif
    LCDC_HANDLER_DMA_FIFO_CTRL_TypeDef handler_reg_0x18;
    do
    {
        handler_reg_0x18.d32 = LCDC_HANDLER->DMA_FIFO_CTRL;
    }
    while (handler_reg_0x18.b.dma_enable != RESET);

    LCDC_HANDLER_OPERATE_CTR_TypeDef handler_reg_0x14;
    do
    {
        handler_reg_0x14.d32 = LCDC_HANDLER->OPERATE_CTR;
    }
    while (handler_reg_0x14.b.auto_write_start != RESET);
#if (TE_VALID == 1)
    LCDC_TeCmd(DISABLE);
#endif

    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();
    LCDC_AXIMUXMode(LCDC_FW_MODE);
    LCDC_Cmd(DISABLE);
}

void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len)
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

    LCDC_ForceBurst(ENABLE);
    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);
#if (TE_VALID == 1)
    LCDC_TeCmd(ENABLE);
#endif
#if (TE_VALID == 0)
    LCDC_AutoWriteCmd(ENABLE);
#endif

    LCDC_HANDLER_DMA_FIFO_CTRL_TypeDef handler_reg_0x18;
    do
    {
        handler_reg_0x18.d32 = LCDC_HANDLER->DMA_FIFO_CTRL;
    }
    while (handler_reg_0x18.b.dma_enable != RESET);
    LCDC_HANDLER_OPERATE_CTR_TypeDef handler_reg_0x14;
    do
    {
        handler_reg_0x14.d32 = LCDC_HANDLER->OPERATE_CTR;
    }
    while (handler_reg_0x14.b.auto_write_start != RESET);
#if (TE_VALID == 1)
    LCDC_TeCmd(DISABLE);
#endif

    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();
    LCDC_AXIMUXMode(LCDC_FW_MODE);
    LCDC_Cmd(DISABLE);
}

static void st7789_pad_config(void)
{
//    Pad_Config(LCD_SPI_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_BL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_QSPI_D0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_QSPI_D1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_QSPI_D2, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_QSPI_D3, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_CLK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_TE, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_DC, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);

    Pad_Dedicated_Config(LCD_SPI_BL, ENABLE);
    Pad_Dedicated_Config(LCD_QSPI_D0, ENABLE);
    Pad_Dedicated_Config(LCD_QSPI_D1, ENABLE);
    Pad_Dedicated_Config(LCD_QSPI_D2, ENABLE);
    Pad_Dedicated_Config(LCD_QSPI_D3, ENABLE);
    Pad_Dedicated_Config(LCD_SPI_CS, ENABLE);
    Pad_Dedicated_Config(LCD_SPI_CLK, ENABLE);
    Pad_Dedicated_Config(LCD_SPI_TE, ENABLE);
    Pad_Dedicated_Config(LCD_SPI_DC, ENABLE);

}
static void st7789_chip_reset(void)
{
    //no need HW do it
}

uint32_t rtk_lcd_hal_power_off(void)
{

    return 0;
}
uint32_t rtk_lcd_hal_power_on(void)
{
    st7789_cmd(0x11);
    st7789_cmd(0x29);
    platform_delay_ms(20);
    return 0;
}

uint32_t rtk_lcd_hal_dlps_restore(void)
{
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
    /* config LCD dev info */
    RCC_PeriphClockCmd(APBPeriph_DISP, APBPeriph_DISP_CLOCK, ENABLE);

    //from XTAL SOURCE = 40M
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.disp_ck_en = 1;
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.disp_func_en = 1;
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_mux_clk_cg_en = 1;

    //From PLL1, SOURCE = 100M
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_div_en = 1;
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_clk_src_sel0 = 0; //pll1_peri(0) or pll2(1, pll2 = 160M)
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_clk_src_sel1 = 1; //pll(1) or xtal(0)
    PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_div_sel = 1; //div

    st7789_pad_config();
    st7789_chip_reset();
    LCDC_InitTypeDef lcdc_init = {0};
    lcdc_init.LCDC_Interface = LCDC_IF_DBIC;
    lcdc_init.LCDC_GroupSel = 1; //QFN88 2 - QFN68 1
    lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_RGB565;
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

    DBG_DIRECT("func = %s, line = %d", __func__, __LINE__);


    LCDC_DBICCfgTypeDef dbic_init = {0};
    dbic_init.DBIC_SPEED_SEL         = 1;
    dbic_init.DBIC_TxThr             = 0;
    dbic_init.DBIC_RxThr             = 0;
    dbic_init.SCPOL                  = DBIC_SCPOL_LOW;
    dbic_init.SCPH                   = DBIC_SCPH_1Edge;
    DBIC_Init(&dbic_init);

    LCDC_SwitchMode(LCDC_MANUAL_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    LCDC_Cmd(ENABLE);

    lcd_set_reset(true);
    platform_delay_ms(50);
    lcd_set_reset(false);
    platform_delay_ms(50);


    DBG_DIRECT("func = %s, line = %d, DBIC->FLUSH_FIFO = 0x%x", __func__, __LINE__, &DBIC->FLUSH_FIFO);

    LCDC_AXIMUXMode(LCDC_FW_MODE);
    DBIC_SwitchMode(DBIC_USER_MODE);
    DBIC_SwitchDirect(DBIC_TMODE_TX);

    st7789_cmd_param1(0x35, 0x00);
    uint8_t Tear_Scanline[] = {0x00, 0x50};//Set Tear Scanline
    st7789_cmd_param2(0x44, Tear_Scanline);
#if FEATURE_PSRAM
    uint8_t psram_set[] = {0x00, 0xA0};
    st7789_cmd_param2(0x44, psram_set);
#else
    uint8_t psram_set[] = {0x00, 0x50};
    st7789_cmd_param2(0x44, psram_set); //Set Tear Scanline
#endif
    st7789_cmd_param1(0x36, 0x40); //Memory Data Access Control
    st7789_cmd_param1(0x3A, 0x05); //Interface Pixel Format

    uint8_t PORCTRL[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
    st7789_cmd_param5(0xB2, PORCTRL); //Porch Setting


    st7789_cmd_param1(0xB7, 0x35); //Gate Control
    st7789_cmd_param1(0xBB, 0x19); //VCOM Setting

    st7789_cmd_param1(0xC0, 0x2C); //LCMCTRL
    st7789_cmd_param1(0xC2, 0x01); //LCM Control
    st7789_cmd_param1(0xC3, 0x12); //VRH Set
    st7789_cmd_param1(0xC4, 0x20); // VDV Set
    st7789_cmd_param1(0xC6, 0x1F); //Frame Rate Control in Normal Mode

    uint8_t Power_Control[] = {0xA4, 0xA1};//Power Control 1
    st7789_cmd_param2(0xD0, Power_Control);

    uint8_t Positive_Voltage[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F,
                                  0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23
                                 };//Positive Voltage Gamma Control
    st7789_cmd_param14(0xE0, Positive_Voltage);

    uint8_t Negative_Voltage[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F,
                                  0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23
                                 };//Negative Voltage Gamma Control
    st7789_cmd_param14(0xE1, Positive_Voltage);

    uint8_t Gate_Control[] = {0x27, 0x00, 0x11};//Gate Control
    st7789_cmd_param3(0xE4, Gate_Control);

//    //  st7789_cmd_param1(0x3A,0x77);            /*RGB888*/
    st7789_cmd_param1(0x3A, 0x55);           /*RGB565*/



//    st7789_cmd(0x21); //Display Inversion On
    st7789_cmd(0x11);             /*sleep out*/
    platform_delay_ms(120);

    rtk_lcd_hal_power_on();       /*power on*/


    rtk_lcd_hal_set_window(0, 0, ST7789_LCD_WIDTH, ST7789_LCD_HIGHT);
    rtk_lcd_hal_rect_fill(0, 0, ST7789_LCD_WIDTH, ST7789_LCD_HIGHT, 0xf800f800);
    DBG_DIRECT("[LCD Init Done]func = %s, line = %d", __func__, __LINE__);
}

