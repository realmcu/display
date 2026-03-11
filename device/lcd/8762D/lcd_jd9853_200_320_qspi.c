/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_section.h"
#include "rtl876x_rcc.h"
#include "rtl876x_tim.h"
#include "rtl876x_gpio.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gdma.h"
#include "rtl876x_nvic.h"
#include "trace.h"
#include "lcd_jd9853_200_320_qspi.h"
#include "platform_utils.h"
#include "qspi_lcd_platform.h"
#include <string.h>
#include <drv_gpio.h>


#define BIT_CMD_CH(x)           (((x) & 0x00000003) << 20)
#define BIT_DATA_CH(x)          (((x) & 0x00000003) << 18)
#define BIT_ADDR_CH(x)          (((x) & 0x00000003) << 16)
#define BIT_TMOD(x)             (((x) & 0x00000003) << 8)

#define BIT_TXSIM               (0x00000001 << 9)
#define BIT_SEQ_EN              (0x00000001 << 3)


static bool rtk_lcd_te_sigal = false;


DATA_RAM_FUNCTION
static void spic2_spi_write(uint8_t *buf, uint32_t len)
{
    SPIC2->ser = BIT0;
    SPIC2->ssienr = 0x00;//disable SPIC2

    SPIC2->ctrlr0 &= ~(BIT_CMD_CH(3) | BIT_ADDR_CH(3) | BIT_DATA_CH(3));//SET CHANNEL NUM
    SPIC2->ctrlr0 &= ~(BIT_TMOD(3)); //tx mode

    for (uint32_t i = 0; i < len; i++)
    {
        SPIC2->dr[0].byte = buf[i];
    }
    SPIC2->ssienr = 0x01;//enable SPIC2

    while (SPIC2->sr & BIT0); // wait bus busy

    SPIC2->ssienr = 0x00;//disable SPIC2
}


DATA_RAM_FUNCTION
static void rtl_lcd_qspi_write_cmd(uint16_t cmd) //total 4 byte, first byte is 0x02
{
    uint8_t sdat[] = {0x02, 0x00, cmd, 0x00};
    spic2_spi_write(sdat, sizeof(sdat));
}



DATA_RAM_FUNCTION
static void rtl_lcd_qspi_cmd_param4(uint8_t cmd, uint8_t *data) //total 8 byte, first byte is 0x02
{
    uint8_t sdat[] = {0x02, 0x00, cmd, 0x00, data[0], data[1], data[2], data[3]};
    spic2_spi_write(sdat, sizeof(sdat));
}


DATA_RAM_FUNCTION
static void rtl_lcd_qspi_enter_data_output_mode(void) //total 4 byte, first byte is 0x32
{
    SPIC2->ser = BIT0;//select CS0
    SPIC2->ssienr = 0x00;//disable SPIC2

    SPIC2->ctrlr0 &= ~(BIT_CMD_CH(3) | BIT_ADDR_CH(3) | BIT_DATA_CH(3));//SET CHANNEL NUM
    SPIC2->ctrlr0 |= (BIT_CMD_CH(0) | BIT_ADDR_CH(0) | BIT_DATA_CH(2));
    SPIC2->ctrlr0 &= ~(BIT_TMOD(3)); //set tx mode

    SPIC2->imr |= BIT_TXSIM;
    SPIC2->ctrlr2 |= BIT_SEQ_EN;

    /* must push cmd and address before SPIC enable */
    uint32_t first_word = 0x32 | __REV(0x002c00);
    SPIC2->dr[0].word = first_word;

    SPIC2->dmacr = 2;

    /* change this value can not influence the result. the wave is split into two periods. the first is 32 bytes. */
    SPIC2->dmatdlr = 4; /* no any influence. */



    SPIC2->ssienr = 0x01;//enable SPIC2
    /*then , we can push data to FIFO*/


    //SPIC2->ctrlr2 &= ~ BIT_SEQ_EN;
    //SPIC2->imr &= ~ BIT_TXSIM;

    //SPIC2->ssienr = 0x00;//disable SPIC2
}

void rtl_lcd_qspi_cmd_param_n(uint8_t cmd, uint8_t *data,
                              uint8_t dataLength) //total 8 byte, first byte is 0x02
{
    uint8_t sdat[50] = {0x02, 0x00, cmd, 0x00};
    memcpy(&sdat[4], &data[0], dataLength);
    spic2_spi_write(sdat, (4 + dataLength));
}

static bool first_section_begin = false;
static bool last_section_done = false;

void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h)
{

    if ((xStart == 0) && (yStart == 0))
    {
        first_section_begin = true;
    }
    else
    {
        first_section_begin = false;
    }

    if (\
        ((xStart + w) == ST77916_LCD_WIDTH) && \
        ((yStart + h) == ST77916_LCD_HEIGHT)
       )
    {
        last_section_done = true;
    }
    else
    {
        last_section_done = false;
    }

    uint8_t data[4];
    uint16_t xEnd = xStart + w - 1;
    uint16_t yEnd = yStart + h - 1;
    yStart = yStart + 20;
    yEnd = yEnd + 20;

    data[0] = xStart >> 8;
    data[1] = xStart & 0xff;
    data[2] = xEnd >> 8;
    data[3] = xEnd & 0xff;
    rtl_lcd_qspi_cmd_param4(0x2A, data);


    data[0] = yStart >> 8;
    data[1] = yStart & 0xff;
    data[2] = yEnd >> 8;
    data[3] = yEnd & 0xff;
    rtl_lcd_qspi_cmd_param4(0x2B, data);

    rtl_lcd_qspi_enter_data_output_mode();
}

void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len)
{
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = LCD_DMA_CHANNEL_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStruct);
    /* Initialize GDMA peripheral */
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum          = LCD_DMA_CHANNEL_NUM;

    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_1;

    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_SPIC2_TX;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, len >> 2);

    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)(&(SPIC2->dr[0].word)));
    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)buf);
    GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
}

void rtk_lcd_hal_rect_fill(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h,
                           uint32_t color)
{
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = LCD_DMA_CHANNEL_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStruct);
    /* Initialize GDMA peripheral */
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum          = LCD_DMA_CHANNEL_NUM;

    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_1;

    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_SPIC2_TX;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);


    rtk_lcd_hal_set_window(xStart, yStart, w, h);

    static uint32_t color_buf = 0;
    color_buf = __REV16(color);

    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)&color_buf);
    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)(&(SPIC2->dr[0].word)));


    uint32_t section_hight = 10;
    uint32_t left_line = h % section_hight;

    for (uint32_t i = 0; i < h / section_hight; i++)
    {
        GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, (w * section_hight * 2) >> 2);
        GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
        rtk_lcd_hal_transfer_done();
    }
    if (left_line != 0)
    {
        GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, (w * left_line * 2) >> 2);
        GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
        rtk_lcd_hal_transfer_done();
    }

}



void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len)
{
    uint32_t *p32 = (uint32_t *)buf;
    for (uint32_t i = 0; i < len / 2; i++)
    {
        p32[i] = __REV16(p32[i]);
    }

    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = LCD_DMA_CHANNEL_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStruct);
    /* Initialize GDMA peripheral */
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum          = LCD_DMA_CHANNEL_NUM;

    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Word;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_1;

    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_SPIC2_TX;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    GDMA_SetBufferSize(LCD_DMA_CHANNEL_INDEX, len >> 1);

    GDMA_SetDestinationAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)(&(SPIC2->dr[0].word)));
    GDMA_SetSourceAddress(LCD_DMA_CHANNEL_INDEX, (uint32_t)buf);

    if (first_section_begin == true)
    {
        while (rtk_lcd_te_sigal == false)
        {
            platform_delay_us(10);
        }
        rtk_lcd_te_sigal = false;
        drv_pin_irq_enable(QSPI_LCD_TE, PIN_IRQ_DISABLE);
    }
    GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
}
void rtk_lcd_hal_transfer_done(void)
{
    while (GDMA_GetTransferINTStatus(LCD_DMA_CHANNEL_NUM) != SET);
    GDMA_ClearINTPendingBit(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    if (last_section_done == true)
    {
        drv_pin_irq_enable(QSPI_LCD_TE, PIN_IRQ_ENABLE);
    }
}

uint32_t rtk_lcd_hal_get_width(void)
{
    return ST77916_LCD_WIDTH;
}
uint32_t rtk_lcd_hal_get_height(void)
{
    return ST77916_LCD_HEIGHT;
}
uint32_t rtk_lcd_hal_get_pixel_bits(void)
{
    return ST77916_DRV_PIXEL_BITS;
}


static void lcd_pad_init(void)
{

    /*BL AND RESET ARE NOT FIX*/
    /*BL AND RESET ARE NOT FIX*/

    Pad_Config(QSPI_LCD_POWER, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);

}

static void lcd_set_reset(bool reset)
{
    if (reset)
    {
        Pad_Config(H_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }
    else
    {
        Pad_Config(H_0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
}

static void lcd_seq_init(void)
{
    DBG_DIRECT("lcd_rm69330_qspi_390_init");
    uint8_t data[40] = {0};

    data[0] = 0x98;
    data[1] = 0x53;
    rtl_lcd_qspi_cmd_param_n(0xDF, data, 2);

    data[0] = 0x00;
    rtl_lcd_qspi_cmd_param_n(0xDE, data, 1);

    data[0] = 0x23;
    rtl_lcd_qspi_cmd_param_n(0xB2, data, 1);

    data[0] = 0x00;
    data[1] = 0x35;
    data[2] = 0x00;
    data[3] = 0x5D;
    rtl_lcd_qspi_cmd_param_n(0xB7, data, 4);

    data[0] = 0x4C;
    data[1] = 0x2F;
    data[2] = 0x55;
    data[3] = 0x73;
    data[4] = 0x6F;
    data[5] = 0xF0;
    rtl_lcd_qspi_cmd_param_n(0xBB, data, 6);

    data[0] = 0x66;
    data[1] = 0xE6;
    rtl_lcd_qspi_cmd_param_n(0xC0, data, 2);

    data[0] = 0x12;
    rtl_lcd_qspi_cmd_param_n(0xC1, data, 1);

    data[0] = 0x7D;
    data[1] = 0x07;
    data[2] = 0x14;
    data[3] = 0x06;
    data[4] = 0xCC;
    data[5] = 0x71;
    data[6] = 0x72;
    data[7] = 0x77;
    rtl_lcd_qspi_cmd_param_n(0xC3, data, 8);

//    {
//        data[0] = 0x00;//00=60Hz 04=57Hz 08=51Hz
//        data[1] = 0x00;
//        data[2] = 0xA0;//LN=320 Line
//        data[3] = 0x79;
//        data[4] = 0x0A;
//        data[5] = 0x0B;
//        data[6] = 0x16;
//        data[7] = 0x79;
//        data[8] = 0x0A;
//        data[9] = 0x0B;
//        data[10] = 0x16;
//        data[11] = 0x82;
//    }
    {
        ////Real framerate=41.5Hz
        data[0] = 0x08;//Porchx4
        data[1] = 0x00;
        data[2] = 0xA0;//LN=320  Line
        data[3] = 0x79;//13ms for display
        data[4] = 0x42;//42h*2=66*4=264*0.04=11.1ms
        data[5] = 0x03;//3h*4=12*0.04=0.5ms
        data[6] = 0x16;
        data[7] = 0x79;
        data[8] = 0x0A;
        data[9] = 0x0B;
        data[10] = 0x16;
        data[11] = 0x82;
    }
    rtl_lcd_qspi_cmd_param_n(0xC4, data, 12);

    data[0] = 0x3F;
    data[1] = 0x2F;
    data[2] = 0x26;
    data[3] = 0x1F;
    data[4] = 0x26;
    data[5] = 0x28;
    data[6] = 0x24;
    data[7] = 0x24;
    data[8] = 0x24;
    data[9] = 0x23;
    data[10] = 0x21;
    data[11] = 0x16;
    data[12] = 0x12;
    data[13] = 0x0D;
    data[14] = 0x07;
    data[15] = 0x02;
    data[16] = 0x3F;
    data[17] = 0x2F;
    data[18] = 0x26;
    data[19] = 0x1F;
    data[20] = 0x26;
    data[21] = 0x28;
    data[22] = 0x24;
    data[23] = 0x24;
    data[24] = 0x24;
    data[25] = 0x23;
    data[26] = 0x21;
    data[27] = 0x16;
    data[28] = 0x12;
    data[29] = 0x0D;
    data[30] = 0x07;
    data[31] = 0x02;
    rtl_lcd_qspi_cmd_param_n(0xC8, data, 32);

    data[0] = 0x04;
    data[1] = 0x06;
    data[2] = 0x6B;
    data[3] = 0x0F;
    data[4] = 0x00;
    rtl_lcd_qspi_cmd_param_n(0xD0, data, 5);

    data[0] = 0x00;
    data[1] = 0x30;
    rtl_lcd_qspi_cmd_param_n(0xD7, data, 2);

    data[0] = 0x14;
    rtl_lcd_qspi_cmd_param_n(0xE6, data, 1);

    data[0] = 0x01;
    rtl_lcd_qspi_cmd_param_n(0xDE, data, 1);

    data[0] = 0x04;//TE_SEL
    rtl_lcd_qspi_cmd_param_n(0xBB, data, 1);

    data[0] = 0x12;//INH_CHKSUM_SEL
    rtl_lcd_qspi_cmd_param_n(0xD7, data, 1);

    data[0] = 0x03;
    data[1] = 0x13;
    data[2] = 0xEF;
    data[3] = 0x38;
    data[4] = 0x38;
    rtl_lcd_qspi_cmd_param_n(0xB7, data, 5);

    data[0] = 0x14;
    data[1] = 0x15;
    data[2] = 0xC0;
    rtl_lcd_qspi_cmd_param_n(0xC1, data, 3);

    data[0] = 0x06;
    data[1] = 0x3A;
    rtl_lcd_qspi_cmd_param_n(0xC2, data, 2);

    data[0] = 0x72;
    data[1] = 0x12;
    rtl_lcd_qspi_cmd_param_n(0xC4, data, 2);

    data[0] = 0x00;
    rtl_lcd_qspi_cmd_param_n(0xBE, data, 1);

    data[0] = 0x02;
    rtl_lcd_qspi_cmd_param_n(0xDE, data, 1);

    data[0] = 0x00;
    data[1] = 0x02;
    rtl_lcd_qspi_cmd_param_n(0xE5, data, 2);

    data[0] = 0x01;
    data[1] = 0x02;
    rtl_lcd_qspi_cmd_param_n(0xE5, data, 2);

    data[0] = 0x00;
    rtl_lcd_qspi_cmd_param_n(0xDE, data, 1);

    data[0] = 0x00;
    data[1] = 0x00;
    rtl_lcd_qspi_cmd_param_n(0x44, data, 2);

    data[0] = 0x00;
    rtl_lcd_qspi_cmd_param_n(0x35, data, 1);

    {
        //add by wanghao
        data[0] = 0x60;
        rtl_lcd_qspi_cmd_param_n(0x36, data, 1);
    };

    data[0] = 0x05;//06=RGB666??05=RGB565
    rtl_lcd_qspi_cmd_param_n(0x3A, data, 1);

    {
        //change by wanghao
        data[0] = 0x00;
        data[1] = 0x00;//Start_X=0
        data[2] = 0x01;
        data[3] = 0x3f;//End_X=319
        rtl_lcd_qspi_cmd_param_n(0x2A, data, 4);

        data[0] = 0x00;
        data[1] = 0x14;//Start_Y=20
        data[2] = 0x00;
        data[3] = 0xDB;//End_Y=219
        rtl_lcd_qspi_cmd_param_n(0x2B, data, 4);
    };

    rtl_lcd_qspi_write_cmd(0x11);
    platform_delay_ms(120);

    data[0] = 0x02;
    rtl_lcd_qspi_cmd_param_n(0xDE, data, 1);

    data[0] = 0x00;
    data[1] = 0x02;
    rtl_lcd_qspi_cmd_param_n(0xE5, data, 2);

    data[0] = 0x00;
    rtl_lcd_qspi_cmd_param_n(0xDE, data, 1);

    rtl_lcd_qspi_write_cmd(0x29);
    platform_delay_ms(10);
}


uint32_t rtk_lcd_hal_power_on(void)
{
    lcd_pad_init();
    qspi_lcd_platform_init();

    rtl_lcd_qspi_write_cmd(0x11);   /*sleep out*/
    platform_delay_ms(120);
    rtl_lcd_qspi_write_cmd(0x29);   /*power on*/
    platform_delay_ms(20);
    return 0;
}

uint32_t rtk_lcd_hal_power_off(void)
{
    extern void drv_touch_int_config(bool enable);
    drv_touch_int_config(true);
    rtl_lcd_qspi_write_cmd(0x28);   /*sleep in*/
    platform_delay_ms(120);

    rtl_lcd_qspi_write_cmd(0x10);   /*power off*/
    platform_delay_ms(20);
    return 0;
}


static void qspi_lcd_te_handle(void *data)
{
    rtk_lcd_te_sigal = true;
}


void rtk_lcd_hal_init(void)
{
    lcd_pad_init();
    qspi_lcd_platform_init();

    lcd_set_reset(true);
    platform_delay_ms(120);
    lcd_set_reset(false);
    platform_delay_ms(50);

    lcd_seq_init();



    drv_pin_mode(QSPI_LCD_TE, PIN_MODE_INPUT);
    drv_pin_attach_irq(QSPI_LCD_TE, PIN_IRQ_MODE_RISING, qspi_lcd_te_handle,
                       NULL);
    drv_pin_irq_enable(QSPI_LCD_TE, PIN_IRQ_ENABLE);

    rtk_lcd_hal_rect_fill(0, 0, ST77916_LCD_WIDTH, ST77916_LCD_HEIGHT, 0xF800F800);
}






