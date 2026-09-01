/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "board.h"
#include "icna3311_368448_qspi.h"
#include "rtl_lcdc_dbic.h"
#include "utils.h"
#include "trace.h"
#include "os_sched.h"
#include "rtl_pinmux.h"
#include "drv_gpio.h"
#include "os_mem.h"

#define LCDC_DMA_CHANNEL_NUM              0
#define LCDC_DMA_CHANNEL_INDEX            LCDC_DMA_Channel0

#define BIT_CMD_CH(x)           (((x) & 0x00000003) << 20)
#define BIT_DATA_CH(x)          (((x) & 0x00000003) << 18)
#define BIT_ADDR_CH(x)          (((x) & 0x00000003) << 16)
#define BIT_TMOD(x)             (((x) & 0x00000003) << 8)

#define BIT_TXSIM               (0x00000001 << 9)
#define BIT_SEQ_EN              (0x00000001 << 3)


static void spic3_spi_write(uint8_t *buf, uint32_t len)
{
    DBIC->CTRLR0 |= BIT31;
    DBIC->CTRLR0 &= ~(BIT_CMD_CH(3) | BIT_ADDR_CH(3) | BIT_DATA_CH(3));//SET CHANNEL NUM
    DBIC->CTRLR0 &= ~(BIT_TMOD(3)); //tx mode

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


/* ICNA3311 QSPI Instruction Code */
#define ICNA3311_QSPI_INST_CMD_WRITE                     (0x02)
#define ICNA3311_QSPI_INST_CMD_READ                      (0x03)
#define ICNA3311_QSPI_INST_1WIRE_PIXEL_WRITE             (0x02)
#define ICNA3311_QSPI_INST_4WIRE_PIXEL_WRITE_TYPE1       (0x32)
#define ICNA3311_QSPI_INST_4WIRE_PIXEL_WRITE_TYPE2       (0x12)
#define ICNA3311_QSPI_SEQ_FINISH_CODE                    (0x00)



const ICNA3311_CMD_DESC ICNA3311_POST_OTP_POWERON_SEQ_CMD[] =
{
    {ICNA3311_QSPI_INST_CMD_WRITE, 0xFE, 1,  1,  {0x00}},
    {ICNA3311_QSPI_INST_CMD_WRITE, 0xC4, 1,  1,  {0x80}},
    {ICNA3311_QSPI_INST_CMD_WRITE, 0x3A, 1,  1,  {0x77}},////0x55 -->565, 0x77 -->888
    {ICNA3311_QSPI_INST_CMD_WRITE, 0x35, 1,  1,  {0x00}},
    {ICNA3311_QSPI_INST_CMD_WRITE, 0x53, 1,  1,  {0x20}},
    {ICNA3311_QSPI_INST_CMD_WRITE, 0x51, 1,  1,  {0xFF}},
    {ICNA3311_QSPI_INST_CMD_WRITE, 0x63, 1,  1,  {0xFF}},
    {ICNA3311_QSPI_INST_CMD_WRITE, 0x2A, 1,  4,  {0x00, 0x10, 0x01, 0x7F}},
    {ICNA3311_QSPI_INST_CMD_WRITE, 0x2B, 1,  4,  {0x00, 0x00, 0x01, 0xBF}},
    {ICNA3311_QSPI_SEQ_FINISH_CODE,  0,  0,  0,  {0}},
};

const ICNA3311_CMD_DESC ICNA3311_OTP_WRITE_POWERON[] =
{
    {ICNA3311_QSPI_INST_CMD_WRITE, 0x29, 0, 0, {0}},
    {ICNA3311_QSPI_SEQ_FINISH_CODE,  0,  0,  0,  {0}},
};

const ICNA3311_CMD_DESC ICNA3311_POWEROFF_SEQ_CMD[] =
{
    {ICNA3311_QSPI_INST_CMD_WRITE, 0x28, 25, 0,  {0}},
    {ICNA3311_QSPI_INST_CMD_WRITE, 0x10, 50, 0,  {0}},
    {ICNA3311_QSPI_SEQ_FINISH_CODE,  0,  0,  0,  {0}},
};

const ICNA3311_CMD_DESC ICNA3311_SET_BACKLIGHT[] =
{
    {ICNA3311_QSPI_INST_CMD_WRITE, 0x51, 0,  1,  {0xff}},
    {ICNA3311_QSPI_SEQ_FINISH_CODE,  0,  0,  0,  {0}},
};

const ICNA3311_CMD_DESC ICNA3311_OTP_WRITE[] =
{
    {ICNA3311_QSPI_INST_CMD_WRITE, 0x11, 1,   0,  {0x00}},
    {ICNA3311_QSPI_SEQ_FINISH_CODE,  0,  0,  0,  {0}},
};

static void ICNA3311_Reg_Write(const ICNA3311_CMD_DESC *cmd)
{
    uint16_t idx = 0;

    while (cmd[idx].instruction != ICNA3311_QSPI_SEQ_FINISH_CODE)
    {
        uint8_t sdat[cmd[idx].wordcount + 4];

        sdat[0] = cmd[idx].instruction;

        sdat[1] = 0;
        sdat[2] = cmd[idx].index;        // Set in the middle 8 bits ADDR[15:8] of the 24 bits ADDR[23:0]
        sdat[3] = 0;

        for (uint16_t i = 0; i < cmd[idx].wordcount; i++)
        {
            sdat[i + 4] = cmd[idx].payload[i];
        }

        spic3_spi_write(sdat, sizeof(sdat));
        if (cmd[idx].delay != 0)
        {
#ifdef __RTTHREAD__
            rt_thread_mdelay(cmd[idx].delay);
#else
            platform_delay_ms(cmd[idx].delay);
#endif
        }

        idx++;
    }
}

void ICNA3311_Init_Post_OTP(void)
{
    //pull low RESX
    //power on VBAT: VBAT = 3.7V
    //power on VDDI: VDDI = 1.8V
    //pull high VCI_EN: enable VCI = 3.3V
    //delay 10ms
    //pull high RESX: IC reset
    //delay 10ms
    ICNA3311_Reg_Write(ICNA3311_POST_OTP_POWERON_SEQ_CMD);
}
void ICNA3311_qspi_power_on(void)
{
    //pull low RESX
    //power on VBAT: VBAT = 3.7V
    //power on VDDI: VDDI = 1.8V
    //pull high VCI_EN: enable VCI = 3.3V
    //delay 10ms
    //pull high RESX: IC reset
    //delay 10ms
    ICNA3311_Reg_Write(ICNA3311_OTP_WRITE_POWERON);
}
void ICNA3311_qspi_power_off(void)
{
    ICNA3311_Reg_Write(ICNA3311_POWEROFF_SEQ_CMD);
    //delay 100ms
    //pull low RESX
    //delay 10ms
    //pull low VCI_EN: disable VCI
    //power off VDDI
    //power off VBAT
}
void ICNA3311_qspi_set_backlight(void)
{
    ICNA3311_Reg_Write(ICNA3311_SET_BACKLIGHT);
    //delay 100ms
    //pull low RESX
    //delay 10ms
    //pull low VCI_EN: disable VCI
    //power off VDDI
    //power off VBAT
}
void ICNA3311_qspi_otp_write(void)
{
    ICNA3311_Reg_Write(ICNA3311_OTP_WRITE);
    //delay 100ms
    //pull low RESX
    //delay 10ms
    //pull low VCI_EN: disable VCI
    //power off VDDI
    //power off VBAT
}

static void rtl_ICNA3311_qspi_cmd_param4(uint8_t cmd,
                                         uint8_t *data) //total 8 byte, first byte is 0x02
{
    uint8_t sdat[] = {0x02, 0x00, cmd, 0x00, data[0], data[1], data[2], data[3]};
    spic3_spi_write(sdat, sizeof(sdat));
}

static void rtl_ICNA3311_qspi_enter_data_output_mode(uint32_t
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

uint32_t rtk_lcd_hal_power_on(void)
{
    ICNA3311_qspi_power_on();
    return 0;
}

uint32_t rtk_lcd_hal_power_off(void)
{
    ICNA3311_qspi_power_off();
    return 0;
}

void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h)
{
    uint8_t data[4];
    uint16_t xEnd = xStart + w - 1;
    uint16_t yEnd = yStart + h - 1;
    data[0] = (xStart + 0x10) >> 8;
    data[1] = (xStart + 0x10) & 0xff;
    data[2] = (xEnd + 0x10) >> 8;
    data[3] = (xEnd + 0x10) & 0xff;
    rtl_ICNA3311_qspi_cmd_param4(0x2A, data);


    data[0] = yStart >> 8;
    data[1] = yStart & 0xff;
    data[2] = yEnd >> 8;
    data[3] = yEnd & 0xff;
    rtl_ICNA3311_qspi_cmd_param4(0x2B, data);

    uint32_t len_byte = (xEnd - xStart + 1) * (yEnd - yStart + 1) * OUTPUT_PIXEL_BYTES;
    rtl_ICNA3311_qspi_enter_data_output_mode(len_byte);
}

void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len)
{
    GDMA_InitTypeDef LCDC_DMA_InitStruct = {0};
    LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
    LCDC_DMA_InitStruct.GDMA_ChannelNum          = LCDC_DMA_CHANNEL_NUM;
    LCDC_DMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    LCDC_DMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;
    LCDC_DMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Word;
    LCDC_DMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Word;
    LCDC_DMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_8;
    LCDC_DMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_8;
    LCDC_DMA_InitStruct.GDMA_SourceAddr          = (uint32_t)buf;
    LCDC_DMA_InitStruct.GDMA_Multi_Block_En     = 0;
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
    TEAR_CTR_t handler_reg_0x10 = {.d32 = LCDC_HANDLER->TEAR_CTR};
    handler_reg_0x10.b.bypass_t2w_delay = 0;
    handler_reg_0x10.b.t2w_delay = 0xfff;
    LCDC_HANDLER->TEAR_CTR = handler_reg_0x10.d32;
    LCDC_TeCmd(ENABLE);
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

void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len)
{
    GDMA_InitTypeDef LCDC_DMA_InitStruct = {0};
    LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
    LCDC_DMA_InitStruct.GDMA_ChannelNum          = LCDC_DMA_CHANNEL_NUM;
    LCDC_DMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    LCDC_DMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;
    LCDC_DMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Word;
    LCDC_DMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Word;
    LCDC_DMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_8;
    LCDC_DMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_8;
    LCDC_DMA_InitStruct.GDMA_SourceAddr          = (uint32_t)buf;
    LCDC_DMA_InitStruct.GDMA_Multi_Block_En     = 0;
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
    TEAR_CTR_t handler_reg_0x10 = {.d32 = LCDC_HANDLER->TEAR_CTR};
    handler_reg_0x10.b.bypass_t2w_delay = 0;
    handler_reg_0x10.b.t2w_delay = 0xfff;
    LCDC_HANDLER->TEAR_CTR = handler_reg_0x10.d32;
    LCDC_TeCmd(ENABLE);
#else
    LCDC_AutoWriteCmd(ENABLE);
#endif
}
void rtk_lcd_hal_transfer_done(void)
{
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

static void ICNA3311_pad_config(void)
{
    Pad_Config(P16_6, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);//D3
    Pad_Config(P16_7, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);//D2
    Pad_Config(P17_0, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);//D1
    Pad_Config(P17_1, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);//D0
    Pad_Config(P17_2, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);//TE
    Pad_Config(P17_3, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);//
    Pad_Config(P17_4, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);//CS
    Pad_Config(P17_5, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);//CLK
    Pad_Config(P17_6, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(P17_7, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);//RST

    Pad_Dedicated_Config(P16_6, ENABLE);
    Pad_Dedicated_Config(P16_7, ENABLE);
    Pad_Dedicated_Config(P17_0, ENABLE);
    Pad_Dedicated_Config(P17_1, ENABLE);
    Pad_Dedicated_Config(P17_2, ENABLE);
    Pad_Dedicated_Config(P17_3, ENABLE);
    Pad_Dedicated_Config(P17_4, ENABLE);
    Pad_Dedicated_Config(P17_5, ENABLE);
    Pad_Dedicated_Config(P17_6, ENABLE);
    Pad_Dedicated_Config(P17_7, ENABLE);

    drv_pin_mode(P13_7, PIN_MODE_OUTPUT);
    drv_pin_write(P13_7, 0); //im0
    drv_pin_mode(P13_6, PIN_MODE_OUTPUT);
    drv_pin_write(P13_6, 1); //im1

    drv_pin_mode(P10_7, PIN_MODE_OUTPUT);
    drv_pin_mode(P12_6, PIN_MODE_OUTPUT);
    drv_pin_write(P10_7, 0); //power enable
    drv_pin_write(P12_6, 1); //power enable
}

uint32_t rtk_lcd_hal_get_width(void)
{
    return ICNA3311_LCD_WIDTH;
}
uint32_t rtk_lcd_hal_get_height(void)
{
    return ICNA3311_LCD_HIGHT;
}

uint32_t rtk_lcd_hal_get_pixel_bits(void)
{
    return ICNA3311_LCD_BITS_PIXEL;
}

void lcd_clear_test(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h)
{
    uint32_t gpu_width = ((ICNA3311_LCD_WIDTH + 15) >> 4) << 4;

    uint8_t *buffer0 = os_mem_alloc(RAM_TYPE_EXT_DATA_SRAM,
                                    (h * gpu_width * ICNA3311_LCD_BITS_PIXEL / 8) + 63);

    uint8_t *ptr = buffer0;

    for (uint32_t i = 0; i < (gpu_width * h * ICNA3311_LCD_BITS_PIXEL / 8);
         i += (ICNA3311_LCD_BITS_PIXEL / 8))
    {
        ptr[i] = 0xFF;
        ptr[i + 1] = 0x0;
        ptr[i + 2] = 0x0;
    }

    rtk_lcd_hal_set_window(xStart, yStart, w, h);
    rtk_lcd_hal_update_framebuffer(ptr, w * h);
}

static void driver_ic_init(void)
{
    ICNA3311_pad_config();

    LCDC_LCD_SET_RST(false);
    platform_delay_ms(50);
    LCDC_LCD_SET_RST(true);
    platform_delay_ms(120);
    LCDC_LCD_SET_RST(false);
    platform_delay_ms(50);

    LCDC_AXIMUXMode(LCDC_FW_MODE);
    DBIC_SwitchMode(DBIC_USER_MODE);
    DBIC_SwitchDirect(DBIC_TMODE_TX);

    ICNA3311_Init_Post_OTP();
    ICNA3311_qspi_otp_write();
    platform_delay_ms(60);
    ICNA3311_qspi_power_on();

    //lcd_clear_test(0, 0, ICNA3311_LCD_WIDTH, ICNA3311_LCD_HIGHT);
    //while(1);
    //ICNA3311_qspi_set_backlight();
}

uint32_t rtk_lcd_hal_dlps_restore(void)
{
    return 0;
}

void rtk_lcd_hal_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_DISP, APBPeriph_DISP_CLOCK_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_MIPI_HOST, APBPeriph_MIPI_HOST_CLOCK, ENABLE);

    LCDC_InitTypeDef lcdc_init = {0};
    lcdc_init.LCDC_Interface = LCDC_IF_DBIC;
    lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_ARGB8888;
    lcdc_init.LCDC_PixelOutputFormat = LCDC_OUTPUT_RGB888;
    lcdc_init.LCDC_PixelBitSwap = LCDC_SWAP_BYPASS; //lcdc_handler_cfg->LCDC_TeEn = LCDC_TE_DISABLE;
#if TE_VALID
    lcdc_init.LCDC_TeEn = DISABLE;
    lcdc_init.LCDC_TePolarity = LCDC_TE_EDGE_FALLING;
    lcdc_init.LCDC_TeInputMux = LCDC_TE_LCD_INPUT;
#endif
    lcdc_init.LCDC_DmaThreshold =
        8;    // MSize + threshold should be no larger than 128
    LCDC_Init(&lcdc_init);

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

    driver_ic_init();
}


