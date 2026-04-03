/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "st7701s_480480_rgb.h"
#include "rtl_lcdc_edpi.h"
#include "rtl_spi.h"
#include "rtl_pinmux.h"
#include "rtl_gpio.h"
#include "rtl_rcc.h"
#include "utils.h"
#include "mem_config.h"
#include "rtl_gdma.h"

#define RTL87x2G_TYPEB      1

/**************************************
NOTE:VCI=3.3V,IOVCC=1.8V,
Display resolution:480*640
params->vertical_sync_active=2
params->vertical_backporch=10
params->vertical_frontporch=8
params->horizontal_sync_active=2
params->horizontal_backporch=10
params->horizontal_frontporch=8
params->RGB.PLL_CLOCK=(19.8)
**************************************/

#define LCDC_DATA23         P5_0
#define LCDC_DATA22         P5_1
#define LCDC_DATA21         P5_2
#define LCDC_DATA20         P5_3
#define LCDC_DATA19         P5_4
#define LCDC_DATA18         P5_5
#define LCDC_DATA17         P3_5
#define LCDC_DATA16         P3_4
#define LCDC_DATA15         P3_3
#define LCDC_DATA14         P3_2
#define LCDC_DATA13         P9_2
#define LCDC_DATA12         P4_7
#define LCDC_DATA11         P4_6
#define LCDC_DATA10         P4_5
#define LCDC_DATA9          P4_4
#define LCDC_DATA8          P1_2
#define LCDC_DATA7          P4_3
#define LCDC_DATA6          P4_2
#define LCDC_DATA5          P4_1
#define LCDC_DATA4          P4_0
#define LCDC_DATA3          P0_7
#define LCDC_DATA2          P0_6
#define LCDC_DATA1          P0_5
#define LCDC_DATA0          P0_4

#define LCDC_RGB_WRCLK      P0_2
#define LCDC_HSYNC          P1_5
#define LCDC_VSYNC          P0_1
#define LCDC_CSN_DE         P0_0

#if RTL87x2G_TYPEB
#define SPI0_SCK_PIN                               P9_0
#define SPI0_MOSI_PIN                              P1_3
#define SPI0_MISO_PIN                              P1_6
#define SPI0_CS_PIN                                P9_1
#define LCDC_RESET                                 P3_6
#else
#define GPD0                P2_5
#define GPD1                P2_7
#define GPD2                P9_0
#define GPD3                P2_6
#define SPI0_SCK_PIN        GPD0
#define SPI0_MOSI_PIN       P3_6
#define SPI0_CS_PIN         GPD3
#define LCDC_RESET          GPD2
#endif


#define LCDC_DMA_CHANNEL_NUM              0
#define LCDC_DMA_CHANNEL_INDEX            LCDC_DMA_Channel0

#define TRANSFER_DMA_CHANNEL_NUM              0
#define TRANSFER_DMA_CHANNEL_INDEX            GDMA_Channel0


static void write_command(uint32_t cmd)
{
    platform_delay_us(200);
    SPI_SendData(SPI0, cmd);
}
static void write_data(uint32_t data)
{
    uint32_t send = (data | 0x100);
    SPI_SendData(SPI0, send);
}

static void st7701s_reset_high(void)
{
    GPIO_WriteBit(GPIO_GetPort(LCDC_RESET), GPIO_GetPin(LCDC_RESET), 1);
}

static void st7701s_reset_low(void)
{
    GPIO_WriteBit(GPIO_GetPort(LCDC_RESET), GPIO_GetPin(LCDC_RESET), 0);
}

//--------------------initial code-----------------------------------------//
static void st7701s_gpio_init(void)
{
    Pad_Config(LCDC_RESET, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);

    Pinmux_Config(LCDC_RESET, DWGPIO);

    RCC_PeriphClockCmd(APBPeriph_GPIOA, APBPeriph_GPIOA_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_GPIOB, APBPeriph_GPIOB_CLOCK, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin    = GPIO_GetPin(LCDC_RESET);
    GPIO_InitStruct.GPIO_Mode   = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_ITCmd  = DISABLE;
    GPIO_Init(GPIO_GetPort(LCDC_RESET), &GPIO_InitStruct);
    st7701s_reset_high();
}

static void st7701s_spi_init(void)
{
    Pad_Config(SPI0_MOSI_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_SCK_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SPI0_CS_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);

    Pinmux_Deinit(SPI0_MOSI_PIN);
    Pinmux_Deinit(SPI0_SCK_PIN);
    Pinmux_Deinit(SPI0_CS_PIN);

    Pinmux_Config(SPI0_CS_PIN, SPI0_CSN_0_MASTER);
    Pinmux_Config(SPI0_SCK_PIN, SPI0_CLK_MASTER);
    Pinmux_Config(SPI0_MOSI_PIN, SPI0_MO_MASTER);

    RCC_PeriphClockCmd(APBPeriph_SPI0, APBPeriph_SPI0_CLOCK, ENABLE);

    SPI_InitTypeDef spi_init_struct;
    SPI_StructInit(&spi_init_struct);
    spi_init_struct.SPI_CPOL = SPI_CPOL_Low;
    spi_init_struct.SPI_CPHA = SPI_CPHA_1Edge;
    spi_init_struct.SPI_Direction = SPI_Direction_TxOnly;
    spi_init_struct.SPI_DataSize = SPI_DataSize_9b;
    spi_init_struct.SPI_BaudRatePrescaler = 64;
    spi_init_struct.SPI_FrameFormat = SPI_Frame_Motorola;
    SPI_Init(SPI0, &spi_init_struct);

    SPI_Cmd(SPI0, ENABLE);

}

static void st7701s_dma_init(uint8_t *init_buffer)
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
    LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = 0;

    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_Mode   =
        LLI_TRANSFER;//LLI_TRANSFER or LLI_WITH_CONTIGUOUS_SAR
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = 1;
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_Struct  = LCDC_DMA_LINKLIST_REG_BASE + 0x50;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);

    LCDC_SET_GROUP1_BLOCKSIZE(ST7701S_480480_LCD_WIDTH * ST7701S_DRV_PIXEL_BITS / 8);
    LCDC_SET_GROUP2_BLOCKSIZE(ST7701S_480480_LCD_WIDTH * ST7701S_DRV_PIXEL_BITS / 8);

    LCDC_DMALLI_InitTypeDef LCDC_DMA_LLI_Init = {0};

    LCDC_DMA_LLI_Init.g1_source_addr = (uint32_t)init_buffer;
    LCDC_DMA_LLI_Init.g2_source_addr = (uint32_t)((uint32_t)init_buffer + ST7701S_480480_LCD_WIDTH *
                                                  ST7701S_DRV_PIXEL_BITS / 8);
    LCDC_DMA_LLI_Init.g1_sar_offset = ST7701S_480480_LCD_WIDTH * ST7701S_DRV_PIXEL_BITS / 8 * 2;
    LCDC_DMA_LLI_Init.g2_sar_offset = ST7701S_480480_LCD_WIDTH * ST7701S_DRV_PIXEL_BITS / 8 * 2;

    LCDC_DMA_Infinite_Buf_Update((uint8_t *)init_buffer,
                                 (uint8_t *)init_buffer + ST7701S_480480_LCD_WIDTH *
                                 ST7701S_DRV_PIXEL_BITS / 8);
    LCDC_DMA_LinkList_Init(&LCDC_DMA_LLI_Init,
                           &LCDC_DMA_InitStruct);//LLI_TRANSFER or LLI_WITH_CONTIGUOUS_SAR
#if LV_USE_GPU_RTK_PPE
    LCDC_ForceBurst(ENABLE);
#endif
    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();

    LCDC_SwitchMode(LCDC_AUTO_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);

    LCDC_SetTxPixelLen(ST7701S_480480_LCD_WIDTH * ST7701S_480480_LCD_HEIGHT);

    LCDC_Cmd(ENABLE);

    LCDC_DMA_SetSourceAddress(LCDC_DMA_CHANNEL_INDEX, (uint32_t)init_buffer);

    LCDC_DMA_MultiBlockCmd(ENABLE);

    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);

    LCDC_DmaCmd(ENABLE);
    LCDC_AutoWriteCmd(ENABLE);
}

static void lcd_pad_and_clk_init(void)
{
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

    Pad_Config(LCDC_DATA0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA2, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA3, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA4, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA8, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA9, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA10, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA11, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA12, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA13, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA16, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA17, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA18, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA19, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA20, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
#if ST7701S_DRV_PIXEL_BITS == 24
    Pad_Config(LCDC_DATA5, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA6, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA7, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA14, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA15, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA21, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA22, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA23, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
#endif

    Pad_Config(LCDC_RGB_WRCLK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pad_Config(LCDC_HSYNC, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_VSYNC, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_CSN_DE, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);

    extern void Pad_Dedicated_Config(uint8_t Pin_Num, FunctionalState Status);
    Pad_Dedicated_Config(LCDC_DATA0, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA1, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA2, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA3, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA4, ENABLE);

    Pad_Dedicated_Config(LCDC_DATA8, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA9, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA10, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA11, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA12, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA13, ENABLE);

    Pad_Dedicated_Config(LCDC_DATA16, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA17, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA18, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA19, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA20, ENABLE);
#if ST7701S_DRV_PIXEL_BITS == 24
    Pad_Dedicated_Config(LCDC_DATA5, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA6, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA7, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA14, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA15, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA21, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA22, ENABLE);
    Pad_Dedicated_Config(LCDC_DATA23, ENABLE);
#endif

    Pad_Dedicated_Config(LCDC_RGB_WRCLK, ENABLE);
    Pad_Dedicated_Config(LCDC_HSYNC, ENABLE);
    Pad_Dedicated_Config(LCDC_VSYNC, ENABLE);
    Pad_Dedicated_Config(LCDC_CSN_DE, ENABLE);

    st7701s_gpio_init();
    platform_delay_ms(1);
    st7701s_spi_init();
}

void rtk_lcd_hal_init(void)
{

    lcd_pad_and_clk_init();

    LCDC_InitTypeDef lcdc_init = {0};
    lcdc_init.LCDC_Interface = LCDC_IF_DPI;
#if ST7701S_DRV_PIXEL_BITS == 16
    lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_RGB565;
    lcdc_init.LCDC_PixelOutputFormat = LCDC_OUTPUT_RGB565;
#else
    lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_RGB888;
    lcdc_init.LCDC_PixelOutputFormat = LCDC_OUTPUT_RGB888;
#endif
    lcdc_init.LCDC_PixelBitSwap = LCDC_SWAP_BYPASS; //lcdc_handler_cfg->LCDC_TeEn = LCDC_TE_DISABLE;
    lcdc_init.LCDC_GroupSel = 1;

    lcdc_init.LCDC_DmaThreshold =
        64;    // MSize + threshold should be no larger than 128
    lcdc_init.LCDC_InfiniteModeEn = 1;
    LCDC_Init(&lcdc_init);
    LCDC_ClearINTPendingBit(LCDC_CLR_WAVEFORM_FINISH);

    uint32_t HSA = 10, HFP = 50, HBP = 50, HACT = ST7701S_480480_LCD_WIDTH;
    uint32_t VSA = 10, VFP = 20, VBP = 20, VACT = ST7701S_480480_LCD_HEIGHT;

    LCDC_eDPICfgTypeDef eDPICfg;//480*640  ---->   500 * 660
    eDPICfg.eDPI_ClockDiv = 0x3;

    eDPICfg.eDPI_HoriSyncWidth = HSA;
    eDPICfg.eDPI_VeriSyncHeight = VSA;
    eDPICfg.eDPI_AccumulatedHBP = HSA + HBP;
    eDPICfg.eDPI_AccumulatedVBP = VSA + VBP;
    eDPICfg.eDPI_AccumulatedActiveW = HSA + HBP + HACT;
    eDPICfg.eDPI_AccumulatedActiveH = VSA + VBP + VACT;
    eDPICfg.eDPI_TotalWidth = HSA + HBP + HACT + HFP;
    eDPICfg.eDPI_TotalHeight = VSA + VBP + VACT + VFP;
    eDPICfg.eDPI_HoriSyncPolarity = 0;
    eDPICfg.eDPI_VeriSyncPolarity = 0;
    eDPICfg.eDPI_DataEnPolarity = 1;
    eDPICfg.eDPI_LineIntMask = 1;
#if ST7701S_DRV_PIXEL_BITS == 16
    eDPICfg.eDPI_ColorMap = EDPI_PIXELFORMAT_RGB565_2;
#else
    eDPICfg.eDPI_ColorMap = EDPI_PIXELFORMAT_RGB888;
#endif
    eDPICfg.eDPI_OperateMode = 0;//video mode
    eDPICfg.eDPI_LcdArc = 0;
    eDPICfg.eDPI_ShutdnPolarity = 0;
    eDPICfg.eDPI_ColorModePolarity = 0;
    eDPICfg.eDPI_ShutdnEn = 0;
    eDPICfg.eDPI_ColorModeEn = 0;
    eDPICfg.eDPI_UpdateCfgEn = 0;
    eDPICfg.eDPI_TearReq = 0;
    eDPICfg.eDPI_Halt = 0;
    eDPICfg.eDPI_CmdMaxLatency = 0;//todo
    eDPICfg.eDPI_LineBufferPixelThreshold = eDPICfg.eDPI_TotalWidth / 2;

    EDPI_Init(&eDPICfg);

    st7701s_reset_high();
    platform_delay_ms(4);
    st7701s_reset_low();
    platform_delay_ms(30);
    st7701s_reset_high();
    platform_delay_ms(120);
    //*******************************/
#include "st7701s_rgb.txt"
    uint8_t *buf = (uint8_t *)SPIC1_ADDR;
    for (int i = 0; i < 480 * 480 * ST7701S_DRV_PIXEL_BITS / 8; i = i + ST7701S_DRV_PIXEL_BITS / 8)
    {
        buf[i] = 0xF8;
        buf[i + 1] = 0;
        // buf[i + 2] = 0;
    }
    rtk_lcd_hal_update_framebuffer((uint8_t *)SPIC1_ADDR,
                                   ST7701S_480480_LCD_WIDTH * ST7701S_480480_LCD_HEIGHT);

}

static bool flush_first = true;
static void *framebuffer = NULL;

void *rtk_lcd_hal_get_framebuffer(void)
{
    return framebuffer;
}

void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len)
{
    framebuffer = buf;
    if (flush_first == true)
    {
        st7701s_dma_init(buf);
        flush_first = false;
    }
    else
    {
        LCDC_DMA_Infinite_Buf_Update(buf,
                                     (buf + ST7701S_480480_LCD_WIDTH * ST7701S_DRV_PIXEL_BITS / 8));
    }
}

void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len)
{
    return;//todo by howie
}

void rtk_lcd_hal_transfer_done(void)
{
    LCDC_ClearINTPendingBit(LCDC_CLR_WAVEFORM_FINISH);
    return;//todo by howie
}

void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h)
{
    return;//todo by howie
}

uint32_t rtk_lcd_hal_get_width(void)
{
    return ST7701S_480480_LCD_WIDTH;
}

uint32_t rtk_lcd_hal_get_height(void)
{
    return ST7701S_480480_LCD_HEIGHT;
}

uint32_t rtk_lcd_hal_get_pixel_bits(void)
{
    return ST7701S_DRV_PIXEL_BITS;
}
bool rtk_lcd_hal_power_off(void)
{
    st7701s_reset_low();
    RCC_PeriphClockCmd(APBPeriph_DISP, APBPeriph_DISP_CLOCK, DISABLE);
    return 0;
}

bool rtk_lcd_hal_power_on(void)
{


    lcd_pad_and_clk_init();

    LCDC_InitTypeDef lcdc_init = {0};
    lcdc_init.LCDC_Interface = LCDC_IF_DPI;
#if ST7701S_DRV_PIXEL_BITS == 16
    lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_RGB565;
    lcdc_init.LCDC_PixelOutputFormat = LCDC_OUTPUT_RGB565;
#else
    lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_RGB888;
    lcdc_init.LCDC_PixelOutputFormat = LCDC_OUTPUT_RGB888;
#endif
    lcdc_init.LCDC_PixelBitSwap = LCDC_SWAP_BYPASS; //lcdc_handler_cfg->LCDC_TeEn = LCDC_TE_DISABLE;
    lcdc_init.LCDC_GroupSel = 1;

    lcdc_init.LCDC_DmaThreshold =
        8;    // MSize + threshold should be no larger than 128
    lcdc_init.LCDC_InfiniteModeEn = 1;
    LCDC_Init(&lcdc_init);

    uint32_t HSA = 10, HFP = 50, HBP = 50, HACT = ST7701S_480480_LCD_WIDTH;
    uint32_t VSA = 10, VFP = 20, VBP = 20, VACT = ST7701S_480480_LCD_HEIGHT;

    LCDC_eDPICfgTypeDef eDPICfg;//480*640  ---->   500 * 660
    eDPICfg.eDPI_ClockDiv = 0x3;

    eDPICfg.eDPI_HoriSyncWidth = HSA;
    eDPICfg.eDPI_VeriSyncHeight = VSA;
    eDPICfg.eDPI_AccumulatedHBP = HSA + HBP;
    eDPICfg.eDPI_AccumulatedVBP = VSA + VBP;
    eDPICfg.eDPI_AccumulatedActiveW = HSA + HBP + HACT;
    eDPICfg.eDPI_AccumulatedActiveH = VSA + VBP + VACT;
    eDPICfg.eDPI_TotalWidth = HSA + HBP + HACT + HFP;
    eDPICfg.eDPI_TotalHeight = VSA + VBP + VACT + VFP;
    eDPICfg.eDPI_HoriSyncPolarity = 0;
    eDPICfg.eDPI_VeriSyncPolarity = 0;
    eDPICfg.eDPI_DataEnPolarity = 1;
    eDPICfg.eDPI_LineIntMask = 1;
#if ST7701S_DRV_PIXEL_BITS == 16
    eDPICfg.eDPI_ColorMap = EDPI_PIXELFORMAT_RGB565_2;
#else
    eDPICfg.eDPI_ColorMap = EDPI_PIXELFORMAT_RGB888;
#endif
    eDPICfg.eDPI_OperateMode = 0;//video mode
    eDPICfg.eDPI_LcdArc = 0;
    eDPICfg.eDPI_ShutdnPolarity = 0;
    eDPICfg.eDPI_ColorModePolarity = 0;
    eDPICfg.eDPI_ShutdnEn = 0;
    eDPICfg.eDPI_ColorModeEn = 0;
    eDPICfg.eDPI_UpdateCfgEn = 0;
    eDPICfg.eDPI_TearReq = 0;
    eDPICfg.eDPI_Halt = 0;
    eDPICfg.eDPI_CmdMaxLatency = 0;//todo
    eDPICfg.eDPI_LineBufferPixelThreshold = eDPICfg.eDPI_TotalWidth / 2;

    EDPI_Init(&eDPICfg);

    st7701s_reset_high();
    platform_delay_ms(4);
    st7701s_reset_low();
    platform_delay_ms(30);
    st7701s_reset_high();
    platform_delay_ms(120);
    //*******************************/
#include "st7701s_rgb.txt"
    flush_first = true;

    return 0;
}

bool rtk_lcd_hal_dlps_check(void)
{
    return true;
}

bool rtk_lcd_wake_up(void)
{
    return 0;
}

uint32_t rtk_lcd_hal_dlps_restore(void)
{
    return 0;
}

void rtk_lcd_dlps_init(void)
{

}
