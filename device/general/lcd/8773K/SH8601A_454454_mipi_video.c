/**
*********************************************************************************************************
*     Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.

*
*     SPDX-License-Identifier: Apache-2.0
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
#include "rtl_lcdc_dsi.h"
#include "rtl_lcdc_dphy.h"
#include "rtl_lcdc_edpi.h"
#include "rtl_nvic.h"


#define LCDC_DMA_CHANNEL_NUM              0
#define LCDC_DMA_CHANNEL_INDEX            LCDC_DMA_Channel0


__weak void DSI_IO_WriteCmd(uint32_t NbrParams, uint8_t *pParams)
{
    DSI_TypeDef *DSIx = DSI;
    if (NbrParams == 0)
    {
        DSI_ShortWrite(DSIx, 0, DSI_DCS_SHORT_PKT_WRITE_P0, pParams[0], pParams[1]);
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

    /* Unlock the SH8601A command pages before sending panel settings. */
    uint8_t pass_word_0[] = {0x5a, 0x5a, 0xC0};
    DSI_IO_WriteCmd(2, (uint8_t *)pass_word_0);
    uint8_t pass_word_1[] = {0x5a, 0x5a, 0xC1};
    DSI_IO_WriteCmd(2, (uint8_t *)pass_word_1);
    platform_delay_ms(5);

    uint8_t ShortRegData36[] = {0x11, 0x00};
    DSI_IO_WriteCmd(0, (uint8_t *)ShortRegData36);
    /* Sleep Out needs the internal power-on settling time before more setup. */
    platform_delay_ms(120);

    /* Select the timing parameter page, then program the video-mode timing
       register (0xB1 carries NOR_VBP/VFP per the SH8601A Table 35 note). */
    uint8_t paramter_select[] = {0xB0, 0x16};
    DSI_IO_WriteCmd(1, (uint8_t *)paramter_select);
    platform_delay_ms(5);
    uint8_t video[] = {0xB1, 0XF0};
    DSI_IO_WriteCmd(1, (uint8_t *)video);
    platform_delay_ms(5);

    /* Address the complete 454 x 454 active area, coordinates 0..453. */
    uint8_t lcdRegData27[] = {0, 0, 0x01, 0xC5, 0x2A};
    DSI_IO_WriteCmd(4, (uint8_t *)lcdRegData27);
    platform_delay_ms(5);

    uint8_t lcdRegData28[] = {0, 0, 0x01, 0xC5, 0x2B};
    DSI_IO_WriteCmd(4, (uint8_t *)lcdRegData28);
    platform_delay_ms(5);

    /* WRCTRLD: enable the panel brightness control path. */
    uint8_t ShortRegData41[] = {0x53, 0x20};
    DSI_IO_WriteCmd(1, (uint8_t *)ShortRegData41);

    platform_delay_ms(60);
    /* Display On is sent only after the internal power-on settling delay. */
    uint8_t ShortRegData44[] = {0x29, 0x00};
    DSI_IO_WriteCmd(0, (uint8_t *)ShortRegData44);
    platform_delay_ms(20);

//      //for bist mode
//      uint8_t bist[] = {0xBA, 0X81};
//      DSI_IO_WriteCmd(1, (uint8_t *)bist);


}

void sh8601a_video_mode_init(void)
{
    /* Let every queued command packet leave the FIFO before the link changes
       mode, so no leftover init packet collides with the pixel stream. */
    uint32_t time_out_ms = 100;
    while (time_out_ms-- != 0)
    {
        DSI_CMD_PKT_STATUS_TypeDef status = {.d32 = DSI->CMD_PKT_STATUS};
        if ((status.b.gen_cmd_empty != 0) && (status.b.gen_pld_w_empty != 0))
        {
            break;
        }
        platform_delay_ms(1);
    }

    /* DSI host: command mode -> standard video mode. The eDPI side was already
       configured for video mode by EDPI_Init and stays idle until LCDC_Cmd, so
       only this register has to change. */
    DSI_MODE_CFG_TypeDef mode_cfg = {.d32 = DSI->MODE_CFG};
    mode_cfg.b.cmd_video_mode = DSI_MODE_VIDEO;
    DSI->MODE_CFG = mode_cfg.d32;
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
    lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_ARGB8888;
    //lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_RGB565;
    lcdc_init.LCDC_PixelOutputFormat = LCDC_OUTPUT_RGB888;
    lcdc_init.LCDC_PixelBitSwap = LCDC_SWAP_BYPASS; //lcdc_handler_cfg->LCDC_TeEn = LCDC_TE_DISABLE;

    lcdc_init.LCDC_InfiniteModeEn = ENABLE;
    lcdc_init.LCDC_DmaThreshold =
        64;    // MSize + threshold should be no larger than 128
    LCDC_Init(&lcdc_init);

    LCDC_MaskINTConfig(LCDC_INT_MASK_TX_AUTO_DONE, DISABLE);
    LCDC_ClearINTPendingBit(LCDC_CLR_TX_AUTO_DONE);
    NVIC_InitTypeDef NVIC_InitStruct;
    /* Infinite-mode refresh depends on LCDC_IRQHandler rearming the DMA. */
    NVIC_InitStruct.NVIC_IRQChannel = Display_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    LCDC_eDPICfgTypeDef eDPICfg = {0};
    /* The current 8773K eDPI API has one divider field. */
    eDPICfg.eDPI_ClockDiv = 8;
    uint32_t laneByteClk_kHz = 50000;
    uint32_t LcdClock_kHz  = (80000 / eDPICfg.eDPI_ClockDiv);
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
    eDPICfg.eDPI_HoriSyncPolarity = 0;
    eDPICfg.eDPI_VeriSyncPolarity = 0;
    eDPICfg.eDPI_DataEnPolarity = 0;
    eDPICfg.eDPI_LineIntMask = 1;

    eDPICfg.eDPI_ColorMap = 0;//for RGB888
    eDPICfg.eDPI_OperateMode = EDPI_STANDARD_VIDEO_MODE;//for adpated VIDEO mode
    eDPICfg.eDPI_LcdArc = 0;
    eDPICfg.eDPI_ShutdnPolarity = 0;
    eDPICfg.eDPI_ColorModePolarity = 0;
    eDPICfg.eDPI_ShutdnEn = 0;
    eDPICfg.eDPI_ColorModeEn = 0;
    eDPICfg.eDPI_UpdateCfgEn = 0;
    eDPICfg.eDPI_TearReq = 0;
    eDPICfg.eDPI_Halt = 0;
    eDPICfg.eDPI_CmdMaxLatency = 0x39d23;//todo
    eDPICfg.eDPI_LineBufferPixelThreshold = eDPICfg.eDPI_TotalWidth / 2;
    EDPI_Init(&eDPICfg);


    uint32_t HSA = SH8601A_LCD_HSYNC, HFP = SH8601A_LCD_HFP, HBP = SH8601A_LCD_HBP,
             HACT = SH8601A_LCD_WIDTH;
    uint32_t VSA = SH8601A_LCD_VSYNC, VFP = SH8601A_LCD_VFP, VBP = SH8601A_LCD_VBP,
             VACT = SH8601A_LCD_HEIGHT;

    (void)HSA;
    (void)HFP;
    (void)HBP;
    (void)HACT;
    (void)VSA;
    (void)VFP;
    (void)VBP;
    (void)VACT;

    LCDC_LCD_SET_RST(false);
    platform_delay_ms(100);
    LCDC_LCD_SET_RST(true);
    platform_delay_ms(50);
    LCDC_LCD_SET_RST(false);
    platform_delay_ms(50);

    DSI_DeInit(DSI);
    DSI_InitTypeDef   DSI_InitStruct = {0};
    DSI_InitStruct.VirtualChannelID             = 0;

    /* DSI color code 5 is the host RGB888 packet format. */
    DSI_InitStruct.ColorCoding                  = 5;
    DSI_InitStruct.LPMaxReadPacket              = DSI_LP_MRDP_DISABLE;
    DSI_InitStruct.DEPolarity                   = 1;
    DSI_InitStruct.HSPolarity                   = 1;
    DSI_InitStruct.VSPolarity                   = 1;

    DSI_InitStruct.phy_mode = 0x0;  //fix
    /* The MIPI module specification calls for one data lane. */
    DSI_InitStruct.NumberOfLanes = 0x01;
    DSI_InitStruct.StopWaitTime = 0;    //skip
    DSI_InitStruct.InterruptMask0 = 0x1FFFFF;
    DSI_InitStruct.InterruptMask1 = 0x1FFFFF;
    DSI_InitStruct.HS_TX_CNT =
        0xFFFF;                 //for trigger timeout error measured in TO_CLK_DIVISION
    DSI_InitStruct.LP_RX_CNT =
        0xFFFF;                 //for trigger timeout error measured in TO_CLK_DIVISION
    DSI_InitStruct.TOCkdiv = 0x05;
    DSI_InitStruct.TXEscapeCkdiv = 0x05;
    DSI_InitStruct.ModeCfg = DSI_MODE_CMD;                   //config for video mode or cmd mode
    DSI_InitStruct.CommandSize                  = 598;
    DSI_InitStruct.PeripherCharacteristics = 0;
    DSI_InitStruct.GenericPacketVCID = 0;
    DSI_InitStruct.GenericTearVCID = 0;
    DSI_InitStruct.GenericReadBackVCID = 0;
    DSI_InitStruct.AutoHS2LPEnable = 0;
    DSI_InitStruct.TxRequestCLKEnable = 1;

    DSI_InitStruct.ClockLaneHS2LPTime = 0x0E;
    DSI_InitStruct.ClockLaneLP2HSTime = 0x27;
    DSI_InitStruct.DataLaneHS2LPTime = 0x10;
    DSI_InitStruct.DataLaneLP2HSTime = 0x37;

    DSI_InitStruct.DataLaneMaxReadTime = 0;
    DSI_InitStruct.phy_shutdownz = 1;
//    DSI_InitStruct.phy_testclr = 0;
//    DSI_InitStruct.phy_testen = 0;
    DSI_InitStruct.phy_enableclk = 1;
//    DSI_InitStruct.phy_forcepll = 0;
    DSI_InitStruct.phy_rstz = 1;
//    DSI_InitStruct.phy_testclk = 0;
//    DSI_InitStruct.phy_testdout = 0;
//    DSI_InitStruct.phy_testin = 0;
//    DSI_InitStruct.phy_auto_ulps = 0;
//    DSI_InitStruct.phy_pll_off_ulps = 0;
//    DSI_InitStruct.phy_pre_pll_off_req = 0;
//    DSI_InitStruct.phy_auto_ulps_delay = 0;
//    DSI_InitStruct.phy_auto_ulps_time = 0;

    DSI_InitStruct.AcknowledgeRequest           = DSI_ACKNOWLEDGE_DISABLE;
    DSI_InitStruct.LPGenReadTime                = 0;
    DSI_InitStruct.HSGenReadTime                = 0;
    DSI_InitStruct.LPGenWriteTime               = 0;
    DSI_InitStruct.HSGenWriteTime               = 0;
    DSI_InitStruct.BTATime                      = 0;

    DSI_InitStruct.LPGenShortWriteNoP           = DSI_LP_GSW0P_ENABLE;
    DSI_InitStruct.LPGenShortWriteOneP          = DSI_LP_GSW1P_ENABLE;
    DSI_InitStruct.LPGenShortWriteTwoP          = DSI_LP_GSW2P_ENABLE;
    DSI_InitStruct.LPGenShortReadNoP            = DSI_LP_GSR0P_ENABLE;
    DSI_InitStruct.LPGenShortReadOneP           = DSI_LP_GSR1P_ENABLE;
    DSI_InitStruct.LPGenShortReadTwoP           = DSI_LP_GSR2P_ENABLE;
    DSI_InitStruct.LPGenLongWrite               = DSI_LP_GLW_ENABLE;
    DSI_InitStruct.LPDcsShortWriteNoP           = DSI_LP_DSW0P_ENABLE;
    DSI_InitStruct.LPDcsShortWriteOneP          = DSI_LP_DSW1P_ENABLE;
    DSI_InitStruct.LPDcsShortReadNoP            = DSI_LP_DSR0P_ENABLE;
    DSI_InitStruct.LPDcsLongWrite               = DSI_LP_DLW_ENABLE;
    DSI_Init(DSI, &DSI_InitStruct);


    DSI_VidCfgTypeDef   VideoCmd_InitStruct = {0};
    VideoCmd_InitStruct.Mode = VIDEO_BURST_MODE_3;
    VideoCmd_InitStruct.LPCommandEnable = ENABLE;
    VideoCmd_InitStruct.FrameBTAAcknowledgeEnable = DISABLE;
    VideoCmd_InitStruct.LPVerticalSyncActiveEnable = ENABLE;
    VideoCmd_InitStruct.LPVerticalBackPorchEnable = ENABLE;
    VideoCmd_InitStruct.LPVerticalFrontPorchEnable = ENABLE;
    VideoCmd_InitStruct.LPVerticalActiveEnable = ENABLE;
    VideoCmd_InitStruct.LPHorizontalBackPorchEnable = ENABLE;
    VideoCmd_InitStruct.LPHorizontalFrontPorchEnable = ENABLE;


    VideoCmd_InitStruct.invact_lpcmd_size = 0;//todo
    VideoCmd_InitStruct.outvact_lpcmd_size = 16;//todo
    VideoCmd_InitStruct.PacketSize = HACT;//todo
    VideoCmd_InitStruct.NumberOfChunks = 0;//todo
    VideoCmd_InitStruct.NullPacketSize = 0;//todo

    VideoCmd_InitStruct.HorizontalLine = (SH8601A_LCD_WIDTH + SH8601A_LCD_HSYNC + SH8601A_LCD_HBP +
                                          SH8601A_LCD_HFP) * laneByteClk_kHz / LcdClock_kHz;
    VideoCmd_InitStruct.HorizontalSyncActive = SH8601A_LCD_HSYNC * laneByteClk_kHz / LcdClock_kHz;
    VideoCmd_InitStruct.HorizontalBackPorch = SH8601A_LCD_HBP * laneByteClk_kHz / LcdClock_kHz;
    VideoCmd_InitStruct.VerticalSyncActive = SH8601A_LCD_VSYNC;
    VideoCmd_InitStruct.VerticalBackPorch = SH8601A_LCD_VBP;
    VideoCmd_InitStruct.VerticalFrontPorch = SH8601A_LCD_VFP;
    VideoCmd_InitStruct.VerticalActive = SH8601A_LCD_HEIGHT;

    VideoCmd_InitStruct.VPGMode = VIDEO_VPG_MODE_V_AND_H;
    VideoCmd_InitStruct.VPGOrientation = VIDEO_VPG_ORIENTATION_V;
    VideoCmd_InitStruct.VPGEnable = DISABLE;


    DSI_ConfigVideoMode(DSI, &VideoCmd_InitStruct);

    DPHY_Init(DSI_InitStruct.NumberOfLanes);

    DSI_PhyCfgTypeDef dsi_phy = {0};
    dsi_phy.phy_forcepll  = PHY_FORCEPLL_DISABLE;
    dsi_phy.phy_enableclk  = PHY_CLK_ENABLE;
    dsi_phy.phy_rstz   = PHY_RST_ENABLE;
    dsi_phy.phy_shutdownz   = PHY_SHUTDOWN_ENABLE;
    DSI_ConfigPhy(DSI, &dsi_phy);

    DSI_Start(DSI);

    /* Step 1: initialize the panel while the host is still in command mode. */
    driver_ic_init();

    /* Step 2: hand the link over to standard video mode. */
    sh8601a_video_mode_init();

    /* Step 3: start the pixel stream with a full-screen solid color. */
    lcd_sh8601a_mipi_video_fill_screen(0xFF00FF00);
}

static uint32_t framebuffer = 0;

void lcd_sh8601a_mipi_video_update_framebuffer(uint8_t *buf, uint32_t len)
{
    framebuffer = (uint32_t)buf;
}

/* Scroll context shared with LCDC_IRQHandler. */
static uint32_t scroll_src_up = 0;      /* Upper/left part source address. */
static uint32_t scroll_src_down = 0;    /* Lower/right part source address. */
static uint32_t scroll_w = 0;
static uint32_t scroll_h = 0;
static uint32_t scroll_offset = 0;      /* Current scroll position (rows or cols). */
static uint32_t scroll_bpp = 0;
static uint32_t scroll_dir = 0;         /* 0 = vertical, 1 = horizontal. */
static uint32_t scroll_enable = 0;      /* Non-zero while a scroll is running. */

/*
 * Frame buffer for the solid-color test pattern. The same external buffer the
 * other 8773K video drivers stream from (otm8009a / ST7701S both use
 * 0x00500000); a static array is not an option because one ARGB8888 frame at
 * 454 x 454 is ~824 KB.
 */
#define SH8601A_VIDEO_FB_ADDR   (0x00500000)

void lcd_sh8601a_mipi_video_fill_screen(uint32_t color)
{
    uint32_t pixels = SH8601A_LCD_WIDTH * SH8601A_LCD_HEIGHT;
    uint32_t *fb = (uint32_t *)SH8601A_VIDEO_FB_ADDR;

    /* INPUT_PIXEL_BYTES is 4 (ARGB8888), so one word per pixel. */
    for (uint32_t i = 0; i < pixels; i++)
    {
        fb[i] = color;
    }

    /* A scroll must not re-arm over this frame from the interrupt handler. */
    scroll_enable = 0;

    /* Point the infinite-mode re-arm at this buffer, then start streaming it
       through the proven single-block path. */
    lcd_sh8601a_mipi_video_update_framebuffer((uint8_t *)fb, pixels);
    lcd_sh8601a_mipi_video_init_framebuffer((uint8_t *)fb, pixels);
}

/**
 * \brief  Program the LCDC DMA source to push one (possibly split) frame and
 *         (re-)arm the infinite-mode refresh.
 *
 * \param  g1_sar: Source address for group1.
 * \param  g1_bytes: Group1 transfer size in bytes. When it is 0 a single-block
 *                   transfer is used (only the "full" buffer is pushed).
 * \param  g2_sar: Source address for group2 (used together with g1).
 * \param  g2_bytes: Group2 transfer size in bytes.
 * \param  g1_sar_off: Source-address step for group1 (per-LLI row stride; 0 for
 *                     a contiguous block, w*bytes_per_pixel for a horizontal cut).
 * \param  g2_sar_off: Source-address step for group2.
 * \param  total_pixels: Total pixels to output for this frame.
 *
 * \note   Shared by the vertical/horizontal scroll entry points and by
 *         LCDC_IRQHandler. The function always configures the DMA channel so
 *         that after the interrupt handler re-arms, the next frame source is
 *         correct. Multi-block (LLI) is used only when a split is requested.
 */
static void lcd_scroll_arm_frame(uint32_t g1_sar, uint32_t g1_bytes,
                                 uint32_t g2_sar, uint32_t g2_bytes,
                                 uint32_t g1_sar_off, uint32_t g2_sar_off,
                                 uint32_t total_pixels)
{
    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();

    if ((g1_bytes == 0) || (g2_bytes == 0))
    {
        /* Whole picture is a single buffer: plain single-block transfer. */
        LCDC_DMA_InitTypeDef LCDC_DMA_InitStruct = {0};
        LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
        LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum          = LCDC_DMA_CHANNEL_NUM;
        LCDC_DMA_InitStruct.LCDC_DMA_SourceInc           = LCDC_DMA_SourceInc_Inc;
        LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc      = LCDC_DMA_DestinationInc_Fix;
        LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize      = LCDC_DMA_DataSize_Word;
        LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
        LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_64;
        LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_64;
        LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = g2_sar ? g2_sar : g1_sar;
        LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = 0;
        LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);

        LCDC_SetTxPixelLen(total_pixels);
        LCDC_Cmd(ENABLE);
        LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
        LCDC_DmaCmd(ENABLE);
    }
    else
    {
        /* Split frame: G1 + G2 concatenated (LLI two-block transfer). */
        LCDC_DMA_InitTypeDef LCDC_DMA_InitStruct = {0};
        LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
        LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum          = LCDC_DMA_CHANNEL_NUM;
        LCDC_DMA_InitStruct.LCDC_DMA_SourceInc           = LCDC_DMA_SourceInc_Inc;
        LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc      = LCDC_DMA_DestinationInc_Fix;
        LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize      = LCDC_DMA_DataSize_Word;
        LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
        LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_64;
        LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_64;
        LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = g1_sar;
        LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_Mode    = LLI_TRANSFER;
        LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = 1;
        LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_Struct  = LCDC_DMA_LINKLIST_REG_BASE + 0x50;
        LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);

        LCDC_SET_GROUP1_BLOCKSIZE(g1_bytes);
        LCDC_SET_GROUP2_BLOCKSIZE(g2_bytes);

        LCDC_DMALLI_InitTypeDef LCDC_DMA_LLI_Init = {0};
        LCDC_DMA_LLI_Init.g1_source_addr = g1_sar;
        LCDC_DMA_LLI_Init.g1_sar_offset = g1_sar_off;
        LCDC_DMA_LLI_Init.g2_source_addr = g2_sar;
        LCDC_DMA_LLI_Init.g2_sar_offset = g2_sar_off;
        LCDC_DMA_LinkList_Init(&LCDC_DMA_LLI_Init, &LCDC_DMA_InitStruct);

        LCDC_SetTxPixelLen(total_pixels);
        LCDC_Cmd(ENABLE);
        LCDC_DMA_MultiBlockCmd(ENABLE);
        LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
        LCDC_DmaCmd(ENABLE);
    }

    LCDC_AutoWriteCmd(ENABLE);
}

/* Compute the G1/G2 split for the current vertical scroll state and arm a frame. */
static void lcd_scroll_arm_vertical(void)
{
    uint32_t stride = scroll_w * scroll_bpp;
    uint32_t off = scroll_offset;

    if (off == 0)
    {
        /* Top of the picture: whole frame is the upper buffer. */
        lcd_scroll_arm_frame(scroll_src_up, 0, 0, 0, 0, 0, scroll_w * scroll_h);
    }
    else if (off < scroll_h)
    {
        /* G1 = rows [off..h-1] of "up", G2 = rows [0..off-1] of "down". */
        lcd_scroll_arm_frame(scroll_src_up + off * stride,
                             (scroll_h - off) * stride,
                             scroll_src_down,
                             off * stride,
                             0, 0,
                             scroll_w * scroll_h);
    }
    else /* off == h */
    {
        /* Bottom of the picture: whole frame is the lower buffer. */
        lcd_scroll_arm_frame(0, 0, scroll_src_down, 0, 0, 0, scroll_w * scroll_h);
    }
}

/* Compute the G1/G2 split for the current horizontal scroll state and arm a frame. */
static void lcd_scroll_arm_horizontal(void)
{
    uint32_t stride = scroll_w * scroll_bpp;
    uint32_t off = scroll_offset;

    if (off == 0)
    {
        /* Leftmost of the picture: whole frame is the left buffer. */
        lcd_scroll_arm_frame(scroll_src_up, 0, 0, 0, 0, 0, scroll_w * scroll_h);
    }
    else if (off < scroll_w)
    {
        /* G1 = cols [off..w-1] of "left" each row, G2 = cols [0..off-1] of "right"
           each row. Both groups must step a full row (stride) in SAR per line. */
        lcd_scroll_arm_frame(scroll_src_up + off * scroll_bpp,
                             (scroll_w - off) * scroll_bpp,
                             scroll_src_down,
                             off * scroll_bpp,
                             stride, stride,
                             scroll_w * scroll_h);
    }
    else /* off == w */
    {
        /* Rightmost of the picture: whole frame is the right buffer. */
        lcd_scroll_arm_frame(0, 0, scroll_src_down, 0, 0, 0, scroll_w * scroll_h);
    }
}

void rtk_lcd_hal_update_fb_use_up_and_down(uint32_t up,
                                           uint32_t down,
                                           uint32_t w,
                                           uint32_t h,
                                           uint32_t offset,
                                           uint32_t bytes_per_pixel)
{
    /* Both source buffers must be 4-byte aligned for the LCDC DMA. */
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
    if (offset > h)
    {
        assert_param("offset error!");
        while (1);
    }

    scroll_src_up = up;
    scroll_src_down = down;
    scroll_w = w;
    scroll_h = h;
    scroll_offset = offset;
    scroll_bpp = bytes_per_pixel;
    scroll_dir = 0;              /* vertical scroll. */
    scroll_enable = 1;

    lcd_scroll_arm_vertical();
}

void rtk_lcd_hal_update_fb_use_left_and_right(uint32_t left,
                                              uint32_t right,
                                              uint32_t w,
                                              uint32_t h,
                                              uint32_t offset,
                                              uint32_t bytes_per_pixel)
{
    /* Both source buffers must be 4-byte aligned for the LCDC DMA. */
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
    if (offset > w)
    {
        assert_param("offset error!");
        while (1);
    }

    scroll_src_up = left;
    scroll_src_down = right;
    scroll_w = w;
    scroll_h = h;
    scroll_offset = offset;
    scroll_bpp = bytes_per_pixel;
    scroll_dir = 1;              /* horizontal scroll. */
    scroll_enable = 1;

    lcd_scroll_arm_horizontal();
}

/* Advance the current scroll (one row or one column) after each finished frame. */
void lcd_scroll_arm_step(void)
{
    if (scroll_dir == 0)
    {
        scroll_offset++;
        if (scroll_offset > scroll_h)
        {
            scroll_offset = 0;
        }
        lcd_scroll_arm_vertical();
    }
    else
    {
        scroll_offset++;
        if (scroll_offset > scroll_w)
        {
            scroll_offset = 0;
        }
        lcd_scroll_arm_horizontal();
    }
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
    LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_64;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_64;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = (uint32_t)buf;
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = 0;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);

//      Test_SH8601A_LCD_DPI_OUTPUT_RTKDMAC_SETUP();
//      LCDC_DMA_Channel0 ->SAR                     =       (uint32_t)buf;

#else
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
    LCDC_DMA_MultiBlockCmd(ENABLE);
#endif

    LCDC_DMA_SetSourceAddress(LCDC_DMA_CHANNEL_INDEX, (uint32_t)buf);

    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);
    LCDC_AutoWriteCmd(ENABLE);
}

void LCDC_IRQHandler(void)
{
    if (LCDC_GetINTStatus(LCDC_STATUS_TX_AUTO_DONE_INT) == SET)
    {
        LCDC_ClearINTPendingBit(LCDC_CLR_TX_AUTO_DONE);
        //rt_kprintf("LCDC_IRQHandler line = %d\n",__LINE__);
        LCDC_ClearDmaFifo();
        LCDC_ClearTxPixelCnt();

        if (scroll_enable)
        {
            /* Continuous scroll: advance the offset and re-arm the split frame. */
            lcd_scroll_arm_step();
        }
        else
        {
            /* Normal infinite-mode refresh: re-arm from the static framebuffer. */
            LCDC_DMA_SetSourceAddress(LCDC_DMA_CHANNEL_INDEX, (uint32_t)framebuffer);
            LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
            LCDC_DmaCmd(ENABLE);
            LCDC_AutoWriteCmd(ENABLE);
        }
    }

}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
