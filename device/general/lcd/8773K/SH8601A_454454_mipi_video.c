/**
*********************************************************************************************************
*               Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file
* @brief
* @details
* @author   howie wang
* @date     2022-02-16
* @version  v0.1
*********************************************************************************************************
*/
#include "SH8601A_454454_mipi_video.h"
#include "rtl_lcdc.h"
#include "rtl_pinmux.h"
#include "rtl_rcc.h"
#include "rtl_lcdc_dsi.h"
#include "rtl_lcdc_dphy.h"
#include "rtl_lcdc_edpi.h"
#include "os_sched.h"
#include "utils.h"
#include "drv_gpio.h"
#include "trace.h"
#include "rtl_nvic.h"

#define LCDC_DMA_CHANNEL_NUM              0
#define LCDC_DMA_CHANNEL_INDEX            LCDC_DMA_Channel0

#define SH8601A_EDPI_CLOCK_DIV            (8U)
#define SH8601A_EDPI_SRC_CLK_KHZ          (80000U)
#define SH8601A_LANE_BYTE_CLK_KHZ         (50000U)
#define SH8601A_DPHY_SRC_CLK_MHZ          (40U)     /* D-PHY PLL reference (CKREF) */
#define SH8601A_LCD_CLK_KHZ               (SH8601A_EDPI_SRC_CLK_KHZ / SH8601A_EDPI_CLOCK_DIV)

#define SH8601A_DSI_LANES                 (1U)
/* Set to 1 to make the DSI host emit its own test pattern instead of eDPI pixels. */
#define SH8601A_DSI_VPG_ENABLE            (0U)

#define SH8601A_DSI_CMD_DONE_TIMEOUT_MS   (100U)

__weak void DSI_IO_WriteCmd(uint32_t NbrParams, uint8_t *pParams)
{
    DSI_TypeDef *DSIx = DSI;
    if (NbrParams == 0)
    {
        DSI_ShortWrite(DSIx, 0, DSI_DCS_SHORT_PKT_WRITE_P0, pParams[0], 0);
    }
    else if (NbrParams == 1)
    {
        DSI_ShortWrite(DSIx, 0, DSI_DCS_SHORT_PKT_WRITE_P1, pParams[0], pParams[1]);
    }
    else
    {
        DSI_LongWrite(DSIx,  0, DSI_DCS_LONG_PKT_WRITE, NbrParams, pParams[NbrParams], pParams);
    }
}

uint32_t rtk_lcd_hal_get_width(void)
{
    return SH8601A_LCD_WIDTH;
}
uint32_t rtk_lcd_hal_get_height(void)
{
    return SH8601A_LCD_HEIGHT;
}

void rtk_lcd_hal_clear_screen(uint32_t ARGB_color)
{
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
    GDMA_InitTypeDef LCDC_DMA_InitStruct = {0};
    LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
    LCDC_DMA_InitStruct.GDMA_ChannelNum          = LCDC_DMA_CHANNEL_NUM;
    LCDC_DMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Fix;
    LCDC_DMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;
    LCDC_DMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Word;
    LCDC_DMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Word;
    LCDC_DMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_1;
    LCDC_DMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_1;
    LCDC_DMA_InitStruct.GDMA_SourceAddr          = (uint32_t)clear_buf;
    LCDC_DMA_InitStruct.GDMA_Multi_Block_En     = 0;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);

    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();

    LCDC_SwitchMode(LCDC_AUTO_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);

    LCDC_SetTxPixelLen(rtk_lcd_hal_get_height() * rtk_lcd_hal_get_width());

    LCDC_Cmd(ENABLE);

    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);

    LCDC_AutoWriteCmd(ENABLE);

    while ((LCDC_HANDLER->DMA_FIFO_CTRL & LCDC_DMA_ENABLE) != RESET);//wait dma finish


    OPERATE_CTR_t handler_reg_0x14;
    do
    {
        handler_reg_0x14.d32 = LCDC_HANDLER->OPERATE_CTR;
    }
    while (handler_reg_0x14.b.auto_write_start != RESET);
    platform_delay_us(10);
    LCDC_Cmd(DISABLE);
    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();
    LCDC_AutoWriteCmd(DISABLE);
}

static void driver_ic_init(void)
{
    Pad_Config(P17_7, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Dedicated_Config(P17_7, ENABLE);
    LCDC_LCD_SET_RST(false);
    platform_delay_ms(50);
    LCDC_LCD_SET_RST(true);
    platform_delay_ms(200);
    LCDC_LCD_SET_RST(false);
    platform_delay_ms(50);
    Pad_Config(P17_7, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Dedicated_Config(P17_7, DISABLE);

    uint8_t pass_word_0[] = {0x5a, 0x5a, 0xC0};
    DSI_IO_WriteCmd(2, (uint8_t *)pass_word_0);
    uint8_t pass_word_1[] = {0x5a, 0x5a, 0xC1};
    DSI_IO_WriteCmd(2, (uint8_t *)pass_word_1);

    uint8_t ShortRegDataE4[] = {0xE4, 0x01};
    DSI_IO_WriteCmd(1, (uint8_t *)ShortRegDataE4);
    uint8_t ShortRegDataB1[] = {0xB1, 0xF0}; //0XF0:video mode, 0XC0:CMD mode
    DSI_IO_WriteCmd(1, (uint8_t *)ShortRegDataB1);


    uint8_t ShortRegData36[] = {0x11, 0x00};
    DSI_IO_WriteCmd(0, (uint8_t *)ShortRegData36);
    platform_delay_ms(5);

    //for bist mode
    // uint8_t bist[] = {0xBA, 0X81};
    // DSI_IO_WriteCmd(1, (uint8_t *)bist);
    // platform_delay_ms(5);

    uint8_t lcdRegData27[] = {0, 0, 0x01, 0xC5, 0x2A};
    DSI_IO_WriteCmd(4, (uint8_t *)lcdRegData27);
    uint8_t lcdRegData28[] = {0, 0, 0x01, 0xC5, 0x2B};
    DSI_IO_WriteCmd(4, (uint8_t *)lcdRegData28);
    uint8_t lcdRegData44[] = {0x01, 0xC5, 0x44};
    DSI_IO_WriteCmd(2, (uint8_t *)lcdRegData44);
    uint8_t lcdRegData35[] = {0x35, 0x00};
    DSI_IO_WriteCmd(0, (uint8_t *)lcdRegData35);
    uint8_t lcdRegData3A[] = {0x3A, 0x77}; //0x77:RGB888, 0X55:RGB565
    DSI_IO_WriteCmd(1, (uint8_t *)lcdRegData3A);
    uint8_t ShortRegData51[] = {0x51, 0x00};
    DSI_IO_WriteCmd(0, (uint8_t *)ShortRegData51);
    uint8_t ShortRegData53[] = {0x53, 0x20};//Write CTRL Display
    DSI_IO_WriteCmd(1, (uint8_t *)ShortRegData53);

    platform_delay_ms(80);
    uint8_t ShortRegData44[] = {0x29, 0x00};
    DSI_IO_WriteCmd(0, (uint8_t *)ShortRegData44);
}

/* Wait until every queued command packet has left the host FIFOs. */
static bool sh8601a_wait_cmd_fifo_empty(uint32_t timeout_ms)
{
    while (timeout_ms-- != 0)
    {
        DSI_CMD_PKT_STATUS_TypeDef status = {.d32 = DSI->CMD_PKT_STATUS};
        if ((status.b.gen_cmd_empty != 0) && (status.b.gen_pld_w_empty != 0)
            && (status.b.gen_rd_cmd_busy == 0))
        {
            return true;
        }
        platform_delay_ms(1);
    }
    return false;
}

/*
 * Init sequence (same order as the Linux dw-mipi-dsi bridge):
 *   1. host in command mode, every packet type in LP  -> driver_ic_init()
 *   2. reset host, switch MODE_CFG to video, power up -> sh8601a_video_mode_init()
 */
void sh8601a_video_mode_init(void)
{
    /* No leftover init packet may collide with the pixel stream. */
    if (!sh8601a_wait_cmd_fifo_empty(SH8601A_DSI_CMD_DONE_TIMEOUT_MS))
    {
        DBG_DIRECT("SH8601A: cmd fifo not empty, status 0x%08x\n", DSI->CMD_PKT_STATUS);
    }

    /* MODE_CFG may only change while the host is held in reset. The eDPI side
       was already configured for video mode by EDPI_Init and stays idle until
       LCDC_Cmd, so only this register has to change. */
    DSI_DeInit(DSI);
    DSI_MODE_CFG_TypeDef mode_cfg = {.d32 = DSI->MODE_CFG};
    mode_cfg.b.cmd_video_mode = DSI_MODE_VIDEO;
    DSI->MODE_CFG = mode_cfg.d32;
    DSI_Start(DSI);
}

void rtk_lcd_hal_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_DISP, APBPeriph_DISP_CLOCK_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_MIPI_HOST, APBPeriph_MIPI_HOST_CLOCK, ENABLE);

    Pad_Config(P17_2, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Dedicated_Config(P17_2, ENABLE);
    drv_pin_mode(P13_7, PIN_MODE_OUTPUT);
    drv_pin_write(P13_7, 0); //im0
    drv_pin_mode(P13_6, PIN_MODE_OUTPUT);
    drv_pin_write(P13_6, 1); //im1

    LCDC_InitTypeDef lcdc_init = {0};
    lcdc_init.LCDC_Interface = LCDC_IF_eDPI;
    lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_RGB888;
    lcdc_init.LCDC_PixelOutputFormat = LCDC_OUTPUT_RGB888;
    lcdc_init.LCDC_PixelBitSwap = LCDC_SWAP_BYPASS; //lcdc_handler_cfg->LCDC_TeEn = DISABLE;

    // lcdc_init.LCDC_InfiniteModeEn = ENABLE;
    lcdc_init.LCDC_DmaThreshold = 64;    // MSize + threshold should be no larger than 128
    LCDC_Init(&lcdc_init);

    // LCDC_MaskINTConfig(LCDC_INT_MASK_TX_AUTO_DONE, DISABLE);
    // LCDC_ClearINTPendingBit(LCDC_CLR_TX_AUTO_DONE);
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = Display_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    LCDC_eDPICfgTypeDef eDPICfg = {0};//836 * 550, 42FPS == 19311600
    eDPICfg.eDPI_ClockDiv = SH8601A_EDPI_CLOCK_DIV;
    eDPICfg.eDPI_HoriSyncWidth = SH8601A_LCD_HSYNC;
    eDPICfg.eDPI_VeriSyncHeight = SH8601A_LCD_VSYNC;
    eDPICfg.eDPI_AccumulatedHBP = SH8601A_LCD_HSYNC + SH8601A_LCD_HBP;
    eDPICfg.eDPI_AccumulatedVBP = SH8601A_LCD_VSYNC + SH8601A_LCD_VBP;
    eDPICfg.eDPI_AccumulatedActiveW = SH8601A_LCD_HSYNC + SH8601A_LCD_HBP + SH8601A_LCD_WIDTH;
    eDPICfg.eDPI_AccumulatedActiveH = SH8601A_LCD_VSYNC + SH8601A_LCD_VBP + SH8601A_LCD_HEIGHT;
    eDPICfg.eDPI_TotalWidth = SH8601A_LCD_WIDTH + SH8601A_LCD_HSYNC + SH8601A_LCD_HBP +
                              SH8601A_LCD_HFP;
    eDPICfg.eDPI_TotalHeight = SH8601A_LCD_HEIGHT + SH8601A_LCD_VSYNC + SH8601A_LCD_VBP +
                               SH8601A_LCD_VFP;
    eDPICfg.eDPI_HoriSyncPolarity = 1;
    eDPICfg.eDPI_VeriSyncPolarity = 1;
    eDPICfg.eDPI_DataEnPolarity = 1;
    eDPICfg.eDPI_LineIntMask = 1;
    eDPICfg.eDPI_ColorMap = 0;//for RGB888
    eDPICfg.eDPI_OperateMode = EDPI_STANDARD_VIDEO_MODE;
    eDPICfg.eDPI_LcdArc = 0;
    eDPICfg.eDPI_ShutdnPolarity = 0;
    eDPICfg.eDPI_ColorModePolarity = 0;
    eDPICfg.eDPI_ShutdnEn = 0;
    eDPICfg.eDPI_ColorModeEn = 0;
    eDPICfg.eDPI_UpdateCfgEn = 0;
    eDPICfg.eDPI_TearReq = 0;
    eDPICfg.eDPI_Halt = 0;
    eDPICfg.eDPI_CmdMaxLatency = 0x39d23;//todo
    eDPICfg.eDPI_LineBufferPixelThreshold = SH8601A_LCD_WIDTH;//eDPICfg.eDPI_TotalWidth / 2;
    EDPI_Init(&eDPICfg);

    DSI_DeInit(DSI);

    const DSI_ConfigTypeDef dsi_cfg =
    {
        .LaneCount           = SH8601A_DSI_LANES,
        .ColorCoding         = DSI_COLOR_RGB888,
        .LaneByteClkKHz      = SH8601A_LANE_BYTE_CLK_KHZ,
        .Polarity            = 0,                   /* all sync and DE active high */
        /* Command mode first: the panel init table goes out before video. */
        .Mode                = DSI_MODE_CMD,
        .eDPICommandSize     = DSI_EDPI_DEFAULT_CMD_SIZE,
    };

    DSI_Init(DSI, &dsi_cfg);
    /* DCS long writes such as 0x2A/0x2B also go out in LP, as DSI_Init left
       every packet type there. */

    {
        const DSI_VideoCfgTypeDef vid_cfg =
        {
            .Width              = SH8601A_LCD_WIDTH,
            .Height             = SH8601A_LCD_HEIGHT,
            .HSync              = SH8601A_LCD_HSYNC,
            .HBackPorch         = SH8601A_LCD_HBP,
            .HFrontPorch        = SH8601A_LCD_HFP,
            .VSync              = SH8601A_LCD_VSYNC,
            .VBackPorch         = SH8601A_LCD_VBP,
            .VFrontPorch        = SH8601A_LCD_VFP,
            .PixelClkKHz        = SH8601A_LCD_CLK_KHZ,
            .LaneByteClkKHz     = SH8601A_LANE_BYTE_CLK_KHZ,
            .LaneCount          = SH8601A_DSI_LANES,
            .BytesPerPixel      = OUTPUT_PIXEL_BYTES,
            .Mode               = VIDEO_BURST_MODE_2,
            /* One packet per line, no filler. A non-zero null size pads the
               link, which is only wanted when the D-PHY runs much faster than
               the DPI and the effective pixel rate has to come down. */
            .NumberOfChunks     = 0,
            .NullPacketSize     = 0,
        };

        if (DSI_ConfigVideoMode(DSI, &vid_cfg) != DSI_STATUS_OK)
        {
            DBG_DIRECT("SH8601A: video timing rejected\n");
        }

        DSI_ConfigVPG(DSI, SH8601A_DSI_VPG_ENABLE,
                      VIDEO_VPG_MODE_V_AND_H, VIDEO_VPG_ORIENTATION_H);
    }

    /* CKREF = 40MHz, HS bit clock = 8 x lane byte clock = 400MHz */
    DPHY_Init(SH8601A_DSI_LANES,
              SH8601A_DPHY_SRC_CLK_MHZ,
              (SH8601A_LANE_BYTE_CLK_KHZ * 8U) / 1000U);

    /* Wait for PLL lock and all lanes in stop state (LP-11) before any command. */
    if (DSI_ConfigPhyPins(DSI) != DSI_STATUS_OK)
    {
        DBG_DIRECT("SH8601A: dphy not in stop state, status 0x%08x\n", DSI->PHY_STATUS);
    }

    DSI_Start(DSI);

    /* Stage 1: command mode, the whole init table in LP. */
    driver_ic_init();

    /* Stage 2: switch the host to video mode, then start the pixel stream. */
    sh8601a_video_mode_init();
    platform_delay_ms(5);
    rtk_lcd_hal_clear_screen(0xFFFF0000);
}

uint32_t rtk_lcd_hal_power_on(void)
{
    uint8_t ShortRegData44[] = {0x29, 0x00};
    DSI_IO_WriteCmd(0, (uint8_t *)ShortRegData44);
    uint8_t ShortRegData36[] = {0x11, 0x00};
    DSI_IO_WriteCmd(0, (uint8_t *)ShortRegData36);
    return 0;
}

uint32_t rtk_lcd_hal_power_off(void)
{
    uint8_t ShortRegData35[] = {0x10, 0x00};
    DSI_IO_WriteCmd(0, (uint8_t *)ShortRegData35);
    uint8_t ShortRegData43[] = {0x28, 0x00};
    DSI_IO_WriteCmd(0, (uint8_t *)ShortRegData43);
    return 0;
}

static uint32_t framebuffer = 0;

void lcd_sh8601a_mipi_video_update_framebuffer(uint8_t *buf, uint32_t len)
{
    framebuffer = (uint32_t)buf;
}

void lcd_sh8601a_mipi_video_init_framebuffer(uint8_t *buf, uint32_t len)
{
#define DMA_LINKLIST 0
#if (DMA_LINKLIST == 0)
    LCDC_DMA_InitTypeDef LCDC_DMA_InitStruct = {0};
    LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
    LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum          = LCDC_DMA_CHANNEL_NUM;
    LCDC_DMA_InitStruct.LCDC_DMA_DIR                 = LCDC_DMA_DIR_PeripheralToMemory;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceInc           = LCDC_DMA_SourceInc_Inc;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc      = LCDC_DMA_DestinationInc_Fix;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize      = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_1;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_1;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = (uint32_t)buf;
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = 0;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);
#else
    GDMA_InitTypeDef LCDC_DMA_InitStruct = {0};
    LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
    LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum          = LCDC_DMA_CHANNEL_NUM;
    LCDC_DMA_InitStruct.LCDC_DMA_DIR                 = LCDC_DMA_DIR_PeripheralToMemory;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceInc           = LCDC_DMA_SourceInc_Inc;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc      = LCDC_DMA_DestinationInc_Fix;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize      = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_1;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_1;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = 0;

    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_Mode   =
        LLI_TRANSFER;//LLI_TRANSFER or LLI_WITH_CONTIGUOUS_SAR
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = 1;
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_Struct  = LCDC_DMA_LINKLIST_REG_BASE + 0x50;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);

    LCDC_SET_GROUP1_BLOCKSIZE(SH8601A_LCD_WIDTH * INPUT_PIXEL_BYTES);
    LCDC_SET_GROUP2_BLOCKSIZE(SH8601A_LCD_WIDTH * INPUT_PIXEL_BYTES);

    /*16 pixel aligned for GPU*/
    uint32_t gpu_width = ((SH8601A_LCD_WIDTH + 15) >> 4) << 4;
    /*16 pixel aligned for GPU*/
    LCDC_DMALLI_InitTypeDef LCDC_DMA_LLI_Init = {0};
    LCDC_DMA_LLI_Init.g1_source_addr = (uint32_t)buf;
    LCDC_DMA_LLI_Init.g1_sar_offset = gpu_width * INPUT_PIXEL_BYTES * 2;

    LCDC_DMA_LLI_Init.g2_source_addr = (uint32_t)(buf + gpu_width * INPUT_PIXEL_BYTES);
    LCDC_DMA_LLI_Init.g2_sar_offset = gpu_width * INPUT_PIXEL_BYTES * 2;
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
    LCDC_SET_INFINITE_ADDR(LCDC_DMA_LLI_Init.g1_source_addr, LCDC_DMA_LLI_Init.g2_source_addr);
    LCDC_DMA_MultiBlockCmd(ENABLE);
#else
    LCDC_DMA_SetSourceAddress(LCDC_DMA_CHANNEL_INDEX, (uint32_t)buf);
#endif


    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);
    LCDC_AutoWriteCmd(ENABLE);
}

void Display_Handler(void)
{
    if (LCDC_GetINTStatus(LCDC_STATUS_TX_AUTO_DONE_INT) == SET)
    {
        LCDC_ClearINTPendingBit(LCDC_CLR_TX_AUTO_DONE);

        DBG_DIRECT("SH8601A[2] INT_ST0 0x%x INT_ST1 0x%x VID_PKT_STATUS 0x%08x\n",
                   DSI->INT_ST0, DSI->INT_ST1, DSI->VID_PKT_STATUS);
    }
}

