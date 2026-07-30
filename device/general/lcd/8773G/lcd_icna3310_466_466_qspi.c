/**
*****************************************************************************************
*     Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.

*
*     SPDX-License-Identifier: Apache-2.0
*****************************************************************************************
* @file    lcd_st77916_360_360_qspi.c
* @brief   This file provides ST77916 LCD driver functions
* @author
* @date
* @version v1.0
* *************************************************************************************
*/
#include "rtl_lcdc_dbic.h"
#include "lcd_icna3310_466_466_qspi.h"
// #include "board.h"
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

#define LCD_QSPI_CS                       P9_2
#define LCD_QSPI_CLK                      P9_4
#define LCD_QSPI_D0                       P9_3
#define LCD_QSPI_D1                       P9_1
#define LCD_QSPI_D2                       P9_0
#define LCD_QSPI_D3                       P9_5
#define LCD_QSPI_RST                      P5_2
#define LCD_QSPI_TE                       P3_1
#define LCD_QSPI_RS                       P2_4

#define LCD_PIN_BL                        P5_0



#define ST77916_MAX_PARA_COUNT              20





typedef struct _ST77916_CMD_DESC
{
    uint8_t instruction;
    uint8_t index;
    uint16_t delay;
    uint16_t wordcount;
    uint8_t  payload[ST77916_MAX_PARA_COUNT];
} ST77916_CMD_DESC;



static void qspi_pad_config(void)
{
    // power en
    Pad_Config(P5_1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_QSPI_RS, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);

    /*QSPI Config*/
    /*QSPI Pad Config, P9_0 -> SIO2, P9_1 -> SIO1, P9_2 -> CS, P9_3 -> SIO0, P9_4 -> CLK, P9_5 -> SIO3*/
    Pad_Config(LCD_QSPI_CS, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_QSPI_CLK, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_QSPI_D0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_QSPI_D1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_QSPI_D2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_QSPI_D3, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    // Pad_Config(LCD_QSPI_TE, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_HIGH);

    Pad_HighSpeedFuncSel(LCD_QSPI_CS, HS_Func0);
    Pad_HighSpeedFuncSel(LCD_QSPI_CLK, HS_Func0);
    Pad_HighSpeedFuncSel(LCD_QSPI_D0, HS_Func0);
    Pad_HighSpeedFuncSel(LCD_QSPI_D1, HS_Func0);
    Pad_HighSpeedFuncSel(LCD_QSPI_D2, HS_Func0);
    Pad_HighSpeedFuncSel(LCD_QSPI_D3, HS_Func0);

    Pad_HighSpeedMuxSel(LCD_QSPI_CS, 1); //1: FROM_CORE_DOMAIN & FROM_HS_MUX
    Pad_HighSpeedMuxSel(LCD_QSPI_CLK, 1);
    Pad_HighSpeedMuxSel(LCD_QSPI_D0, 1);
    Pad_HighSpeedMuxSel(LCD_QSPI_D1, 1);
    Pad_HighSpeedMuxSel(LCD_QSPI_D2, 1);
    Pad_HighSpeedMuxSel(LCD_QSPI_D3, 1);
    // Pad_HighSpeedMuxSel(LCD_QSPI_TE, 1);
}

#if 0
#include "pwm.h"
#define PWM_OUT_PIN              ADC_3
#define PWM_LOW_LEVEL_CNT                       2000    //LOW LEVEL count ,count frequnce is 1Mhz
#define PWM_HIGH_LEVEL_CNT                      2000    //High LEVEL count ,count frequnce is 1Mhz

static T_PWM_HANDLE demo_pwm_handle;
static T_PWM_HANDLE demo_pwm_deadzone_handle;
static void driver_pwm_init(void)
{
    T_PWM_CONFIG demo_pwm_deadzone_para;

    demo_pwm_handle = pwm_create("demo_pwm", PWM_HIGH_LEVEL_CNT, PWM_LOW_LEVEL_CNT, false);
    if (demo_pwm_handle == NULL)
    {
        IO_PRINT_ERROR0("driver_pwm_init: Fail to create pwm handle");
        return;
    }

    pwm_pin_config(demo_pwm_handle, PWM_OUT_PIN, PWM_FUNC);
    pwm_start(demo_pwm_handle);
}
#endif
void lcd_set_backlight(bool set)
{
    if (set)
    {
        // driver_pwm_init();
        Pad_Config(LCD_PIN_BL, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
    else
    {
        Pad_Config(LCD_PIN_BL, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }
}

static void lcdc_dbic_write(uint8_t *buf, uint32_t len)
{
    //DBIC_Cmd(DISABLE);
    DBIC->CTRLR0 |= BIT31;
    DBIC->CTRLR0 &= ~(BIT_CMD_CH(3) | BIT_ADDR_CH(3) | BIT_DATA_CH(3));//SET CHANNEL NUM
    DBIC->CTRLR0 &= ~(BIT_TMOD(3)); //tx mode
    DBIC_TX_NDF(len - 4);
    DBIC_CmdLength(1);
    DBIC_AddrLength(3);
    // DBIC_USER_LENGTH_t reg_val = {.d32 = DBIC->USER_LENGTH};

    for (uint32_t i = 0; i < len; i++)
    {
        DBIC->DR[0].byte = buf[i];
    }

    DBIC_Cmd(ENABLE);
    while (DBIC->SR & BIT0);// wait bus busy
    //DBIC_Cmd(DISABLE);
}

/* ST77916 QSPI Instruction Code */
#define ST77916_QSPI_INST_CMD_WRITE                     (0x02)
#define ST77916_QSPI_SEQ_FINISH_CODE                    (0x00)

static const ST77916_CMD_DESC ST77916_POST_OTP_POWERON_SEQ_CMD[] =
{
    {ST77916_QSPI_INST_CMD_WRITE, 0xFE, 0, 1, {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE, 0xC4, 0, 1, {0x80}},
    {ST77916_QSPI_INST_CMD_WRITE, 0x3A, 0, 1, {0x55}},  // 565
    {ST77916_QSPI_INST_CMD_WRITE, 0x35, 0, 1, {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE, 0x53, 0, 1, {0x20}},
    {ST77916_QSPI_INST_CMD_WRITE, 0x51, 0, 1, {0xFF}},  // 0x00~0xFF lightning
    {ST77916_QSPI_INST_CMD_WRITE, 0x63, 0, 1, {0xFF}},

    {ST77916_QSPI_INST_CMD_WRITE, 0x2A, 0, 4, {0x00, 0x06, 0x01, 0xD7}},
    {ST77916_QSPI_INST_CMD_WRITE, 0x2B, 0, 4, {0x00, 0x00, 0x01, 0xD1}},

    {ST77916_QSPI_INST_CMD_WRITE, 0x11, 60, 0, {0x00}},
    {ST77916_QSPI_INST_CMD_WRITE, 0x29, 20, 0, {0x00}},

//      {ST77916_QSPI_INST_CMD_WRITE, 0x1C, 0, 0, {0x00}},



// #if (ENABLE_TE_FOR_LCD == 1)
//     {ST77916_QSPI_INST_CMD_WRITE,  0x44, 0,  2,  {0x00, 0x00}},
//     {ST77916_QSPI_INST_CMD_WRITE,  0x35, 0,  1,  {0x00}},
// #else
//     {ST77916_QSPI_INST_CMD_WRITE,  0x34, 0,  0,  {0x00}},
// #endif
//     {ST77916_QSPI_INST_CMD_WRITE,  0x3A, 0,  1,  {0x55}},
//     {ST77916_QSPI_INST_CMD_WRITE,  0x11, 120, 0,  {0x00}},
//     {ST77916_QSPI_INST_CMD_WRITE,  0x29, 20, 0,  {0x00}},
//     {ST77916_QSPI_INST_CMD_WRITE,  0x36, 0,  1,  {0xA0}},


    {ST77916_QSPI_SEQ_FINISH_CODE, 0,    0,  0,  {0}},
};

static void ST77916_Reg_Write(const ST77916_CMD_DESC *cmd)
{
    uint16_t idx = 0;

    // uint8_t *sdat = os_mem_alloc(RAM_TYPE_DATA_ON, 50);
    uint8_t sdat[50] = {0};
    while (cmd[idx].instruction != ST77916_QSPI_SEQ_FINISH_CODE)
    {
        sdat[0] = cmd[idx].instruction;

        sdat[1] = 0;
        sdat[2] = cmd[idx].index; // Set in the middle 8 bits ADDR[15:8] of the 24 bits ADDR[23:0]
        sdat[3] = 0;

        //APP_PRINT_INFO1("cmd[idx].index: 0x%x", cmd[idx].index);

        for (uint16_t i = 0; i < cmd[idx].wordcount; i++)
        {
            sdat[i + 4] = cmd[idx].payload[i];
        }

        lcdc_dbic_write(sdat, cmd[idx].wordcount + 4);
        if (cmd[idx].delay != 0)
        {
            platform_delay_ms(cmd[idx].delay);
        }

        idx++;
    }
    // os_mem_free(sdat);
}

void ST77916_Init_Post_OTP(void)
{
    ST77916_Reg_Write(ST77916_POST_OTP_POWERON_SEQ_CMD);
}

static void lcdc_dbic_write_cmd_param4(uint8_t cmd, uint8_t *data)
{
    uint8_t sdat[] = {ST77916_QSPI_INST_CMD_WRITE, 0x00, cmd, 0x00, data[0], data[1], data[2], data[3]};
    lcdc_dbic_write(sdat, sizeof(sdat));
}

static void lcdc_dbic_enter_data_output_mode(uint32_t len_byte)
{
//    DBIC_Cmd(DISABLE);
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
    DBIC->DMATDLR = 32; /* no any influence. */
//    DBIC_Sequence_Write(ENABLE);
    DBIC->ICR = 1;
    LCDC_AXIMUXMode(LCDC_HW_MODE);
//    DBIC_Cmd(ENABLE);
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
    lcdc_dbic_write_cmd_param4(0x2A, data);


    data[0] = yStart >> 8;
    data[1] = yStart & 0xff;
    data[2] = yEnd >> 8;
    data[3] = yEnd & 0xff;
    lcdc_dbic_write_cmd_param4(0x2B, data);

    uint32_t len_byte = (xEnd - xStart + 1) * (yEnd - yStart + 1) * OUTPUT_PIXEL_BYTES;
    lcdc_dbic_enter_data_output_mode(len_byte);
}



static T_LCDC_TE_TYPE use_TE = LCDC_TE_TYPE_HW_TE;
void rtk_lcd_hal_set_TE_type(T_LCDC_TE_TYPE state)
{
    use_TE = state;
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
    LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_16;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_16;
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
        handler_reg_0x28.d32 = LCDC_HANDLER->TX_LEN;
        handler_reg_0x2c.d32 = LCDC_HANDLER->TX_CNT;
    }
    while (handler_reg_0x14.b.auto_write_start != RESET &&
           handler_reg_0x2c.b.tx_output_pixel_cnt < handler_reg_0x28.b.tx_output_pixel_num);
    platform_delay_us(20);

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
    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);
#if (TE_VALID == 1)
    LCDC_HANDLER_TEAR_CTR_TypeDef handler_reg_0x10 = {.d32 = LCDC_HANDLER->TEAR_CTR};
    handler_reg_0x10.b.bypass_t2w_delay = 0;
    handler_reg_0x10.b.t2w_delay = 0xfff;
    LCDC_HANDLER->TEAR_CTR = handler_reg_0x10.d32;
    LCDC_TeCmd(ENABLE);
    LCDC_TeEnableDMA(DISABLE);
#else
    LCDC_AutoWriteCmd(ENABLE);
#endif

    DBG_DIRECT("%s %d", __FUNCTION__, __LINE__);

    while ((LCDC_HANDLER->DMA_FIFO_CTRL & LCDC_DMA_ENABLE) != RESET)//wait dma finish
    {
        os_delay(1);
    }
    while (((LCDC_HANDLER->DMA_FIFO_OFFSET & LCDC_DMA_TX_FIFO_OFFSET) != RESET) &&
           (LCDC_HANDLER->TX_CNT == LCDC_HANDLER->TX_LEN));//wait lcd tx cnt finish
#if (TE_VALID == 1)
    LCDC_TeCmd(DISABLE);                            // disable Tear trigger auto_write_start
#endif

    DBG_DIRECT("%s %d", __FUNCTION__, __LINE__);
    LCDC_Cmd(DISABLE);
    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();
    LCDC_AXIMUXMode(LCDC_FW_MODE);
}

void rtk_lcd_hal_clear_screen(uint32_t ARGB_color)
{
    rtk_lcd_hal_set_window(0, 0, ICNA3310_466_466_LCD_WIDTH, ICNA3310_466_466_LCD_HEIGHT);
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
    LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_8;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_8;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = (uint32_t)clear_buf;
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = 0;
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

#include "wdg.h"
void rtk_lcd_hal_init(void)
{
    // return;
    DBG_DIRECT("%s %d", __FUNCTION__, __LINE__);
    LCDC_Clock_Sel(LCDC_BUS_CLK_200M);
    //TODO
    qspi_pad_config();

    Pad_Config(LCD_PIN_BL, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_QSPI_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    platform_delay_ms(100);
    Pad_Config(LCD_QSPI_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    platform_delay_ms(50);
    Pad_Config(LCD_QSPI_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    platform_delay_ms(130);

    // lcd_set_backlight(true);


#if 1
    LCDC_InitTypeDef lcdc_init = {0};
    lcdc_init.LCDC_Interface = LCDC_IF_DBIC;
#if INPUT_PIXEL_BYTES == 4
    lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_ARGB8888;
#elif INPUT_PIXEL_BYTES == 3
    lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_RGB888;
#elif INPUT_PIXEL_BYTES == 2
    lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_RGB565;
#endif

#if OUTPUT_PIXEL_BYTES == 2
    lcdc_init.LCDC_PixelOutputFormat = LCDC_OUTPUT_RGB565;
#elif OUTPUT_PIXEL_BYTES == 3
    lcdc_init.LCDC_PixelOutputFormat = LCDC_OUTPUT_RGB888;
#endif


    lcdc_init.LCDC_PixelBitSwap = LCDC_SWAP_BYPASS; //lcdc_handler_cfg->LCDC_TeEn = LCDC_TE_DISABLE;
#if TE_VALID
    lcdc_init.LCDC_TeEn = ENABLE;
    lcdc_init.LCDC_TePolarity = LCDC_TE_EDGE_FALLING;
    lcdc_init.LCDC_TeInputMux = LCDC_TE_LCD_INPUT;
#endif
    lcdc_init.LCDC_GroupSel = 1;
    lcdc_init.LCDC_DmaThreshold =
        64;    //only support threshold = 8 for DMA MSIZE = 8; the other threshold setting will be support later
    lcdc_init.LCDC_PhaseShift = 0x2; // set PHI delay
//    LCDC_AXIMUXMode(LCDC_FW_MODE);
//      DBG_DIRECT("PHI b = 0x%x", *((volatile uint32_t*)0x40017960)); // set PHI delay
//      *((volatile uint32_t*)0x40017960) = 0x2;
//      DBG_DIRECT("PHI a = 0x%x", *((volatile uint32_t*)0x40017960));
    LCDC_Init(&lcdc_init);

    LCDC_ClearINTPendingBit(LCDC_CLR_WAVEFORM_FINISH);//add

    LCDC_DBICCfgTypeDef dbic_init = {0};
    dbic_init.DBIC_SPEED_SEL         = 2;
    // dbic_init.DBIC_SPEED_SEL         = 4;

    dbic_init.DBIC_TxThr             = 0;//0 or 4
    dbic_init.DBIC_RxThr             = 0;
    dbic_init.SCPOL                  = DBIC_SCPOL_LOW;
    dbic_init.SCPH                   = DBIC_SCPH_1Edge;
    DBIC_Init(&dbic_init);

    LCDC_SwitchMode(LCDC_MANUAL_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    LCDC_Cmd(ENABLE);

    // lcdc_dbib_lcd_reset
    // lcd_set_backlight(true);

    LCDC_AXIMUXMode(LCDC_FW_MODE);
    DBIC_SwitchMode(DBIC_USER_MODE);
    DBIC_SwitchDirect(DBIC_TMODE_TX);

    ST77916_Init_Post_OTP();

    DBIC_IMR_TypeDef dbic_reg_0x2c = {.d32 = DBIC->IMR};
    dbic_reg_0x2c.b.dreim = 1;
    DBIC->IMR = dbic_reg_0x2c.d32;
    // rtk_lcd_hal_clear_screen(0x00FF0000);  // blue
    rtk_lcd_hal_clear_screen(0x000000FF);    // red
    // rtk_lcd_hal_clear_screen(0x00FFFFFF);
    // rtk_lcd_hal_clear_screen(0x00000000);

#endif
    DBG_DIRECT("%s %d", __FUNCTION__, __LINE__);
    // while(1)
    // {
    //     wdg_kick();
    // };
}

uint32_t rtk_lcd_hal_get_width(void)
{
    return ICNA3310_466_466_LCD_WIDTH;
}

uint32_t rtk_lcd_hal_get_height(void)
{
    return ICNA3310_466_466_LCD_HEIGHT;
}

uint32_t rtk_lcd_hal_get_pixel_bits(void)
{
    return ICNA3310_466_466_DRV_PIXEL_BITS;
}

/*
 * The ST77916 panel driver does not implement a dedicated power_on/off
 * sequence (only init + transfer). Provide weak stubs so the upper
 * blanking_on/off callers have symbols to link against. To add real
 * sleep support later (e.g. 0x10 sleep-in / 0x11 sleep-out + backlight
 * toggle), define same-named strong functions in this file and they
 * will automatically override these weak defaults.
 */
__attribute__((weak)) bool rtk_lcd_hal_power_on(void)  { return true; }
__attribute__((weak)) bool rtk_lcd_hal_power_off(void) { return true; }