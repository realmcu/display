/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "hx8369_480480_rgb.h"
#include "rtl_lcdc_edpi.h"
#include "rtl_lcdc.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gpio.h"
#include "rtl876x_rcc.h"
//#include "utils.h"
#include "mem_config.h"
//#include "string.h"
#include "rtl876x_gdma.h"
#include "trace.h"
#include "platform_utils.h"


#include "rtl876x_spi.h"
#include "trace.h"



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

#define PSRAM_FRAME_BUF1_ADDR 0x4000000

#define LCDC_DATA15         P8_5
#define LCDC_DATA14         P8_4
#define LCDC_DATA13         P8_3
#define LCDC_DATA12         P8_2
#define LCDC_DATA11         P8_1
#define LCDC_DATA10         P8_0
#define LCDC_DATA9          P4_7
#define LCDC_DATA8          P4_6

#define LCDC_DATA7          P4_5
#define LCDC_DATA6          P4_4
#define LCDC_DATA5          P4_3
#define LCDC_DATA4          P4_2
#define LCDC_DATA3          P4_1
#define LCDC_DATA2          P4_0
#define LCDC_DATA1          P2_7
#define LCDC_DATA0          P2_6

#define LCDC_RGB_WRCLK      P2_5
#define LCDC_HSYNC          P2_4
#define LCDC_CSN_DE         P2_3
#define LCDC_VSYNC          P2_2
#define LCDC_RESET          P9_1

#define SPI2_SCK_PIN                               P9_4
#define SPI2_MOSI_PIN                              P9_3
#define SPI2_MISO_PIN                              P9_5
#define SPI2_CS_PIN                                P9_2


// PWM
#define PWREN            MIC1_P
#define LCD_PWREN        ADC_1
#define LCD_ST7701_BL    P9_0

#define LCDC_DMA_CHANNEL_NUM              0
#define LCDC_DMA_CHANNEL_INDEX            LCDC_DMA_Channel0

#define TRANSFER_DMA_CHANNEL_NUM              0
#define TRANSFER_DMA_CHANNEL_INDEX            GDMA_Channel0


static void write_command(uint32_t cmd)
{
    platform_delay_us(200);
    SPI_SendData(SPI2, cmd);
}
static void write_data(uint32_t data)
{
    uint32_t send = (data | 0x100);
    SPI_SendData(SPI2, send);
}

static void hx8369_reset_high(void)
{
    // GPIO_WriteBit(GPIO_GetPort(LCDC_RESET), GPIO_GetPin(LCDC_RESET), 1);
    Pad_Config(LCDC_RESET, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
}

static void hx8369_reset_low(void)
{
    // GPIO_WriteBit(GPIO_GetPort(LCDC_RESET), GPIO_GetPin(LCDC_RESET), 0);
    Pad_Config(LCDC_RESET, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
}

//--------------------initial code-----------------------------------------//

#include "hw_tim.h"
#include "pwm.h"
// higher than 20k
// 20  50k
#define PWM_LOW_LEVEL_CNT                       4    //LOW LEVEL count ,count frequnce is 1Mhz
#define PWM_HIGH_LEVEL_CNT                      16    //High LEVEL count ,count frequnce is 1Mhz
#include "hal_gpio.h"
#include "rtl876x_pinmux.h"
static void hx8369_gpio_init(void)
{
    Pad_AnalogMode(MIC1_P, PAD_DIGITAL_MODE);
    Pad_Config(MIC1_P, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_PWREN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);




    // PWM BL
    if (1)
    {
        Pad_Config(LCD_ST7701_BL, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
    else
    {
        static T_PWM_HANDLE demo_pwm_handle;
        demo_pwm_handle = pwm_create("BL_pwm", PWM_HIGH_LEVEL_CNT, PWM_LOW_LEVEL_CNT, false);
        if (!demo_pwm_handle)
        {
            DBG_DIRECT("driver_pwm_init: Fail to create pwm handle");
            return;
        }

        pwm_pin_config(demo_pwm_handle, LCD_ST7701_BL, PWM_FUNC);
        pwm_start(demo_pwm_handle);


    }





    Pad_Config(LCDC_RESET, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);

    Pinmux_Config(LCDC_RESET, DWGPIO);

    RCC_PeriphClockCmd(APBPeriph_GPIOA, APBPeriph_GPIOA_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_GPIOB, APBPeriph_GPIOB_CLOCK, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_PinBit    = GPIO_GetPin(LCDC_RESET);
    GPIO_InitStruct.GPIO_Mode   = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_ITCmd  = DISABLE;
    // TO BE CHECK
    // GPIO_Init(GPIO_GetPort(LCDC_RESET), &GPIO_InitStruct);
    hx8369_reset_high();


}

static void hx8369_spi_init(void)
{
    Pad_Config(SPI2_MOSI_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SPI2_SCK_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(SPI2_CS_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);

    Pinmux_Deinit(SPI2_MOSI_PIN);
    Pinmux_Deinit(SPI2_SCK_PIN);
    Pinmux_Deinit(SPI2_CS_PIN);

    Pinmux_Config(SPI2_CS_PIN, SPI2_SS_N_0_MASTER);
    Pinmux_Config(SPI2_SCK_PIN, SPI2_CLK_MASTER);
    Pinmux_Config(SPI2_MOSI_PIN, SPI2_MO_MASTER);

    RCC_PeriphClockCmd(APBPeriph_SPI2, APBPeriph_SPI2_CLOCK, ENABLE);

    SPI_InitTypeDef spi_init_struct;
    SPI_StructInit(&spi_init_struct);
    // spi_init_struct.SPI_CPOL = SPI_CPOL_Low;
    // spi_init_struct.SPI_CPHA = SPI_CPHA_1Edge;
    // spi_init_struct.SPI_Direction = SPI_Direction_TxOnly;
    // spi_init_struct.SPI_DataSize = SPI_DataSize_9b;
    // spi_init_struct.SPI_BaudRatePrescaler = 64;
    // spi_init_struct.SPI_FrameFormat = SPI_Frame_Motorola;

    spi_init_struct.SPI_Direction   = SPI_Direction_TxOnly;
    spi_init_struct.SPI_Mode        = SPI_Mode_Master;
    spi_init_struct.SPI_DataSize    = SPI_DataSize_9b;
    spi_init_struct.SPI_CPOL        = SPI_CPOL_Low;
    spi_init_struct.SPI_CPHA        = SPI_CPHA_1Edge;
    spi_init_struct.SPI_BaudRatePrescaler  = 64;
    spi_init_struct.SPI_FrameFormat = SPI_Frame_Motorola;

    SPI_Init(SPI2, &spi_init_struct);

    SPI_Cmd(SPI2, ENABLE);

}

static void hx8369_dma_init(uint8_t *init_buffer)
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

    LCDC_SET_GROUP1_BLOCKSIZE(HX8369_480480_LCD_WIDTH * HX8369_DRV_PIXEL_BITS / 8);
    LCDC_SET_GROUP2_BLOCKSIZE(HX8369_480480_LCD_WIDTH * HX8369_DRV_PIXEL_BITS / 8);

    LCDC_DMALLI_InitTypeDef LCDC_DMA_LLI_Init = {0};

    LCDC_DMA_LLI_Init.g1_source_addr = (uint32_t)init_buffer;
    LCDC_DMA_LLI_Init.g2_source_addr = (uint32_t)((uint32_t)init_buffer + HX8369_480480_LCD_WIDTH *
                                                  HX8369_DRV_PIXEL_BITS / 8);
    LCDC_DMA_LLI_Init.g1_sar_offset = HX8369_480480_LCD_WIDTH * HX8369_DRV_PIXEL_BITS / 8 * 2;
    LCDC_DMA_LLI_Init.g2_sar_offset = HX8369_480480_LCD_WIDTH * HX8369_DRV_PIXEL_BITS / 8 * 2;

    LCDC_DMA_Infinite_Buf_Update((uint8_t *)init_buffer,
                                 (uint8_t *)init_buffer + HX8369_480480_LCD_WIDTH *
                                 HX8369_DRV_PIXEL_BITS / 8);
    LCDC_DMA_LinkList_Init(&LCDC_DMA_LLI_Init,
                           &LCDC_DMA_InitStruct);//LLI_TRANSFER or LLI_WITH_CONTIGUOUS_SAR

    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();

    LCDC_SwitchMode(LCDC_AUTO_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);

    LCDC_SetTxPixelLen(HX8369_480480_LCD_WIDTH * HX8369_480480_LCD_HEIGHT);
#if LV_USE_GPU_RTK_PPE
    LCDC_ForceBurst(ENABLE);
#endif
    LCDC_ForceBurst(ENABLE);
    LCDC_Cmd(ENABLE);

    LCDC_DMA_SetSourceAddress(LCDC_DMA_CHANNEL_INDEX, (uint32_t)init_buffer);

    LCDC_DMA_MultiBlockCmd(ENABLE);

    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);

    LCDC_DmaCmd(ENABLE);
    LCDC_AutoWriteCmd(ENABLE);
}

static void lcd_pad_and_clk_init(void)
{
    LCDC_Clock_Cfg(ENABLE);
    RCC_PeriphClockCmd(APBPeriph_DISP, APBPeriph_DISP_CLOCK, ENABLE);

    // //from XTAL SOURCE = 40M
    // PERIBLKCTRL_PERI_CLK->u_324.BITS_324.disp_ck_en = 1;
    // PERIBLKCTRL_PERI_CLK->u_324.BITS_324.disp_func_en = 1;
    // PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_mux_clk_cg_en = 1;

    // //From PLL1, SOURCE = 125M
    // PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_div_en = 1;
    // PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_clk_src_sel0 = 0; //pll1_peri(0) or pll2(1, pll2 = 160M)
    // PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_clk_src_sel1 = 1; //pll(1) or xtal(0)
    // PERIBLKCTRL_PERI_CLK->u_324.BITS_324.r_disp_div_sel = 1; //div




    Pad_Config(LCDC_DATA0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA2, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA3, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA4, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA5, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA6, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA7, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);

    Pad_Config(LCDC_DATA8, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA9, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA10, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA11, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA12, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA13, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);

    Pad_Config(LCDC_DATA14, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_DATA15, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);


    Pad_Config(LCDC_RGB_WRCLK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);
    Pad_Config(LCDC_HSYNC, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_VSYNC, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCDC_CSN_DE, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);




    Pad_HighSpeedFuncSel(LCDC_DATA0, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_DATA1, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_DATA2, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_DATA3, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_DATA4, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_DATA5, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_DATA6, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_DATA7, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_DATA8, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_DATA9, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_DATA10, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_DATA11, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_DATA12, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_DATA13, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_DATA14, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_DATA15, HS_Func0);


    Pad_HighSpeedFuncSel(LCDC_RGB_WRCLK, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_HSYNC, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_VSYNC, HS_Func0);
    Pad_HighSpeedFuncSel(LCDC_CSN_DE, HS_Func0);

    Pad_HighSpeedMuxSel(LCDC_DATA0, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_DATA1, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_DATA2, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_DATA3, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_DATA4, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_DATA5, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_DATA6, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_DATA7, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_DATA8, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_DATA9, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_DATA10, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_DATA11, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_DATA12, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_DATA13, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_DATA14, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_DATA15, FROM_CORE_DOMAIN);


    Pad_HighSpeedMuxSel(LCDC_RGB_WRCLK, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_HSYNC, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_VSYNC, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCDC_CSN_DE, FROM_CORE_DOMAIN);


    DBG_DIRECT("%s: %d", __FUNCTION__, __LINE__);
    hx8369_gpio_init();

    DBG_DIRECT("%s: %d", __FUNCTION__, __LINE__);


    platform_delay_ms(1);

    DBG_DIRECT("%s: %d", __FUNCTION__, __LINE__);
    hx8369_spi_init();

    DBG_DIRECT("%s: %d", __FUNCTION__, __LINE__);
}

#include "wdg.h"
void rtk_lcd_hal_init(void)
{
    DBG_DIRECT("%s: %d", __FUNCTION__, __LINE__);

    lcd_pad_and_clk_init();
#if 1
    DBG_DIRECT("%s: %d", __FUNCTION__, __LINE__);

    LCDC_InitTypeDef lcdc_init = {0};
    lcdc_init.LCDC_Interface = LCDC_IF_DPI;
    lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_RGB565;
    lcdc_init.LCDC_PixelOutputFormat = LCDC_OUTPUT_RGB565;
    lcdc_init.LCDC_PixelBitSwap = LCDC_SWAP_BYPASS; //lcdc_handler_cfg->LCDC_TeEn = LCDC_TE_DISABLE;
    lcdc_init.LCDC_GroupSel = 0;

    lcdc_init.LCDC_DmaThreshold =
        64;    // MSize + threshold should be no larger than 128
    lcdc_init.LCDC_InfiniteModeEn = 1;
    LCDC_Init(&lcdc_init);
    LCDC_ClearINTPendingBit(LCDC_CLR_WAVEFORM_FINISH);
    RCC_DisplayClockConfig(DISPLAY_CLOCK_DIV_1, ENABLE);//PLL1:200M

    uint32_t HSA = 32, HFP = 58, HBP = 58, HACT = HX8369_480480_LCD_WIDTH;
    uint32_t VSA = 6, VFP = 12, VBP = 5, VACT = HX8369_480480_LCD_HEIGHT;

    LCDC_eDPICfgTypeDef eDPICfg;//480*640  ---->   500 * 660
    eDPICfg.eDPI_ClockDiv = 0x4;

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
    eDPICfg.eDPI_ColorMap = EDPI_PIXELFORMAT_RGB565_1;
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
    // EDPI OFFSET 0x38 bit8 set 0
    EDPI_VIDEO_CTL_TypeDef edpi_reg_0x38 = {.d32 = EDPI->EDPI_VIDEO_CTL};
    edpi_reg_0x38.d32 &= 0xfffffeff;
    EDPI->EDPI_VIDEO_CTL = edpi_reg_0x38.d32;

    DBG_DIRECT("%s: %d", __FUNCTION__, __LINE__);

    hx8369_reset_high();
    platform_delay_ms(4);
    hx8369_reset_low();
    platform_delay_ms(20);
    hx8369_reset_high();
    platform_delay_ms(130);
    //*******************************/
#include "hx8369_rgb.txt"
    uint8_t *buf = (uint8_t *)PSRAM_FRAME_BUF1_ADDR;
    for (int i = 0; i < 480 * 480 * HX8369_DRV_PIXEL_BITS / 8; i = i + HX8369_DRV_PIXEL_BITS / 8)
    {
        if (i < 480 * 480 * HX8369_DRV_PIXEL_BITS / 8 / 4 - 1)
        {
            buf[i] = 0x00;
            buf[i + 1] = 0xf8;
        }
        else if (i < 480 * 480 * HX8369_DRV_PIXEL_BITS / 8 / 2 - 1)
        {
            buf[i] = 0xe0;
            buf[i + 1] = 0x07;
        }
        else if (i < 480 * 480 * HX8369_DRV_PIXEL_BITS / 8 / 4 * 3 - 1)
        {
            buf[i] = 0x1f;
            buf[i + 1] = 0x00;
        }
        else
        {
            buf[i] = 0xff;
            buf[i + 1] = 0xff;
        }
    }

    rtk_lcd_hal_update_framebuffer((uint8_t *)PSRAM_FRAME_BUF1_ADDR,
                                   HX8369_480480_LCD_WIDTH * HX8369_480480_LCD_HEIGHT);
#endif
    DBG_DIRECT("%s: %d", __FUNCTION__, __LINE__);
    // WDG_Disable();
    // while(1);
}

static bool flush_first = true;

void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len)
{
    if (flush_first == true)
    {
        hx8369_dma_init(buf);
        flush_first = false;
    }
    else
    {
        LCDC_DMA_Infinite_Buf_Update(buf,
                                     buf + HX8369_480480_LCD_WIDTH * HX8369_DRV_PIXEL_BITS / 8);
    }
}

void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len)
{
    return;
}

void rtk_lcd_hal_transfer_done(void)
{
    LCDC_ClearINTPendingBit(LCDC_CLR_WAVEFORM_FINISH);
    return;
}

void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h)
{
    return;
}

uint32_t rtk_lcd_hal_get_width(void)
{
    return HX8369_480480_LCD_WIDTH;
}

uint32_t rtk_lcd_hal_get_height(void)
{
    return HX8369_480480_LCD_HEIGHT;
}

uint32_t rtk_lcd_hal_get_pixel_bits(void)
{
    return HX8369_DRV_PIXEL_BITS;
}
bool rtk_lcd_hal_power_off(void)
{
    return 0;
}

bool rtk_lcd_hal_power_on(void)
{
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
