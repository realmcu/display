/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "NV3030B_76x284_spi.h"
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

static void nv3030b_cmd(uint8_t cmd)
{
    uint8_t sdat[] = {cmd};
    qspi_write(sdat, sizeof(sdat));
}


static void nv3030b_cmd_param1(uint8_t cmd, uint8_t data)
{
    uint8_t sdat[] = {cmd, data};
    qspi_write(sdat, sizeof(sdat));
}

static void nv3030b_cmd_param2(uint8_t cmd, uint8_t *data)
{
    uint8_t sdat[] = {cmd, data[0], data[1]};
    qspi_write(sdat, sizeof(sdat));
}
static void nv3030b_cmd_param3(uint8_t cmd, uint8_t *data)
{
    uint8_t sdat[] = {cmd, data[0], data[1], data[2]};
    qspi_write(sdat, sizeof(sdat));
}
static void nv3030b_cmd_param4(uint8_t cmd, uint8_t *data)
{
    uint8_t sdat[] = {cmd,  data[0], data[1], data[2], data[3]};
    qspi_write(sdat, sizeof(sdat));
}

static void nv3030b_cmd_param5(uint8_t cmd, uint8_t *data)
{
    uint8_t sdat[] = {cmd,  data[0], data[1], data[2], data[3], data[4]};
    qspi_write(sdat, sizeof(sdat));
}

static void nv3030b_cmd_param6(uint8_t cmd, uint8_t *data)
{
    uint8_t sdat[] = {cmd,  data[0], data[1], data[2], data[3], data[4], data[5]};
    qspi_write(sdat, sizeof(sdat));
}

static void nv3030b_cmd_param8(uint8_t cmd, uint8_t *data)
{
    uint8_t sdat[] = {cmd,  data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]};
    qspi_write(sdat, sizeof(sdat));
}


static void nv3030b_cmd_param14(uint8_t cmd, uint8_t *data)
{
    uint8_t sdat[] = {cmd,  data[0], data[1], data[2], data[3], data[4], data[5],
                      data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13]
                     };
    qspi_write(sdat, sizeof(sdat));
}

static void nv3030b_enter_data_output_mode(uint32_t len_byte)
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

    xStart = xStart + 82;
    xEnd = xEnd + 82;

    yStart = yStart + 18;
    yEnd = yEnd + 18;

    data[0] = xStart >> 8;
    data[1] = xStart & 0xff;
    data[2] = xEnd >> 8;
    data[3] = xEnd & 0xff;
    nv3030b_cmd_param4(0x2A, data);


    data[0] = yStart >> 8;
    data[1] = yStart & 0xff;
    data[2] = yEnd >> 8;
    data[3] = yEnd & 0xff;
    nv3030b_cmd_param4(0x2B, data);

    // nv3030b_cmd(0x2C);

    uint32_t len_byte = (xEnd - xStart + 1) * (yEnd - yStart + 1) * OUTPUT_PIXEL_BYTES;
    nv3030b_enter_data_output_mode(len_byte);

}

uint32_t rtk_lcd_hal_get_width(void)
{
    return NV3030B_LCD_WIDTH;
}
uint32_t rtk_lcd_hal_get_height(void)
{
    return NV3030B_LCD_HEIGHT;
}

uint32_t rtk_lcd_hal_get_pixel_bits(void)
{
    return NV3030B_DRV_PIXEL_BITS;
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

    LCDC_SET_GROUP1_BLOCKSIZE(NV3030B_LCD_WIDTH * INPUT_PIXEL_BYTES);
    LCDC_SET_GROUP2_BLOCKSIZE(NV3030B_LCD_WIDTH * INPUT_PIXEL_BYTES);

    /*16 pixel aligned for GPU*/
    LCDC_DMALLI_InitTypeDef LCDC_DMA_LLI_Init = {0};
    LCDC_DMA_LLI_Init.g1_source_addr = (uint32_t)buf;
    LCDC_DMA_LLI_Init.g1_sar_offset = NV3030B_LCD_WIDTH * INPUT_PIXEL_BYTES * 2;

    LCDC_DMA_LLI_Init.g2_source_addr = (uint32_t)(buf + NV3030B_LCD_WIDTH * INPUT_PIXEL_BYTES);
    LCDC_DMA_LLI_Init.g2_sar_offset = NV3030B_LCD_WIDTH * INPUT_PIXEL_BYTES * 2;
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
    LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_8;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_8;
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

static void nv3030b_pad_config(void)
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
static void nv3030b_chip_reset(void)
{
    //no need HW do it
}

bool rtk_lcd_hal_power_off(void)
{
    nv3030b_cmd(0x28);             /*power off*/
    platform_delay_ms(25);
    nv3030b_cmd(0x10);             /*sleep in*/
    return 0;
}
bool rtk_lcd_hal_power_on(void)
{
    nv3030b_cmd(0x11);              /*sleep out*/
    platform_delay_ms(30);
    nv3030b_cmd(0x29);              /*power on*/

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

    nv3030b_pad_config();
    nv3030b_chip_reset();
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

    lcd_set_reset(false);
    platform_delay_ms(200);
    lcd_set_reset(true);
    platform_delay_ms(20);
    lcd_set_reset(false);
    platform_delay_ms(120);


    DBG_DIRECT("func = %s, line = %d, DBIC->FLUSH_FIFO = 0x%x", __func__, __LINE__, &DBIC->FLUSH_FIFO);

    LCDC_AXIMUXMode(LCDC_FW_MODE);
    DBIC_SwitchMode(DBIC_USER_MODE);
    DBIC_SwitchDirect(DBIC_TMODE_TX);


    uint8_t private_access[] = {0x06, 0x08};
    nv3030b_cmd_param2(0xFD, private_access);
    uint8_t dvdd_set[] = {0x07, 0x07};
    nv3030b_cmd_param2(0x61, dvdd_set);
    nv3030b_cmd_param1(0x73, 0x70);
    nv3030b_cmd_param1(0x73, 0x00);
    //bias
    uint8_t bias_set[] = {0x00, 0x44, 0x42};
    nv3030b_cmd_param3(0x62, bias_set);
    //VGL
    uint8_t vgl_set[] = {0x41, 0x02, 0x12, 0x12};
    nv3030b_cmd_param4(0x63, vgl_set);
    //VGH
    nv3030b_cmd_param1(0x64, 0x37);
    //VSP
    uint8_t vsp_set[] = {0x09, 0x10, 0x21};
    nv3030b_cmd_param3(0x65, vsp_set);
    //VSN
    uint8_t vsn_set[] = {0x09, 0x10, 0x21};
    nv3030b_cmd_param3(0x66, vsn_set);
    //add source_neg_time
    uint8_t pump_clock[] = {0x20, 0x20};
    nv3030b_cmd_param2(0x67, pump_clock);
    //gamma vap/van
    uint8_t gamma_ref[] = {0x90, 0x30, 0x21, 0x3D};
    nv3030b_cmd_param4(0x68, gamma_ref);
    //frame rate
    uint8_t frame_rate[] = {0x0f, 0x02, 0x00};
    nv3030b_cmd_param3(0xb1, frame_rate);
    nv3030b_cmd_param1(0xb4, 0x01);
    //blanking porch
    uint8_t blanking_porch[] = {0x02, 0x02, 0x0a, 0x14};
    nv3030b_cmd_param4(0xb5, blanking_porch);
    //display function
    uint8_t display[] = {0x04, 0x01, 0x9f, 0x00, 0x02};
    nv3030b_cmd_param5(0xb6, display);
    //source
    uint8_t src_ctrl1[] = {0x00, 0xff};
    nv3030b_cmd_param2(0xe6, src_ctrl1);
    uint8_t src_ctrl2[] = {0x01, 0x04, 0x03, 0x03, 0x00, 0x12};
    nv3030b_cmd_param6(0xe7, src_ctrl2);
    //gate
    nv3030b_cmd_param1(0xec, 0x52);
    //gamme sel
    nv3030b_cmd_param1(0xdf, 0x11);
    //source
    uint8_t src_ctrl3[] = {0x00, 0x70, 0x00};
    nv3030b_cmd_param3(0xe8, src_ctrl3);
    //gamma positive 3
    uint8_t gamma_p3[] = {0x08, 0x02, 0x03, 0x1c, 0x1b, 0x3a};
    nv3030b_cmd_param6(0xe2, gamma_p3);
    //gamma negative 3
    uint8_t gamma_n3[] = {0x3a, 0x1b, 0x1c, 0x03, 0x01, 0x08};
    nv3030b_cmd_param6(0xe5, gamma_n3);
    //gamma positive 2
    uint8_t gamma_p2[] = {0x2d, 0x60};
    nv3030b_cmd_param2(0xe1, gamma_p2);
    //gamma negative 2
    uint8_t gamma_n2[] = {0x64, 0x1e};
    nv3030b_cmd_param2(0xe4, gamma_n2);
    //gamma positive 1
    uint8_t gamma_p1[] = {0x08, 0x0f, 0x10, 0x12, 0x12, 0x14, 0x12, 0x15};
    nv3030b_cmd_param8(0xe0, gamma_p1);
    //gamma negative 1
    uint8_t gamma_n1[] = {0x15, 0x12, 0x13, 0x17, 0x17, 0x14, 0x08, 0x08};
    nv3030b_cmd_param8(0xe3, gamma_n1);
    //interface control
    uint8_t interface_ctrl[] = {0x01, 0x30, 0x00, 0x00};
    nv3030b_cmd_param4(0xf6, interface_ctrl);
    //tearing effect
    uint8_t tearing_effect[] = {0x01, 0x01, 0x02};
    nv3030b_cmd_param3(0xf1, tearing_effect);
    //private_access
    uint8_t private_access2[] = {0xfa, 0xfc};
    nv3030b_cmd_param2(0xfd, private_access2);
    //pixel format
    nv3030b_cmd_param1(0x3a, 0x55);
    //tearing effect line on
    nv3030b_cmd_param1(0x35, 0x00);
    //memory data access control
    nv3030b_cmd_param1(0x36, 0x00);

    rtk_lcd_hal_power_on();       /*power on*/

    rtk_lcd_hal_set_window(0, 0, NV3030B_LCD_WIDTH, NV3030B_LCD_HEIGHT);
    rtk_lcd_hal_rect_fill(0, 0, NV3030B_LCD_WIDTH, NV3030B_LCD_HEIGHT, 0xf800f800);

    DBG_DIRECT("[LCD Init Done]func = %s, line = %d", __func__, __LINE__);
}

