/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"
#include "hal_gpio.h"
#include "platform_utils.h"
//#include "rtk_hal_lcd.h"
#include "rtl_lcdc_dbib.h"
#include "os_sched.h"
#include "st77916_360_360_dbib_8080.h"
#include "trace.h"
#define LCDC_DMA_CHANNEL_NUM              0
#define LCDC_DMA_CHANNEL_INDEX            LCDC_DMA_Channel0


#include "rtl_lcdc_dbib.h"
#include "platform_utils.h"

void st77916_write_cmd(uint8_t command)
{
    LCDC_Cmd(DISABLE);
    LCDC_SwitchMode(LCDC_MANUAL_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    LCDC_Cmd(ENABLE);

    DBIB_SetCS();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    DBIB_ResetCS();

    DBIB_SendCmd(command);
}

void st77916_write_data(uint8_t data)
{
    uint8_t data_buf[1];
    data_buf[0] = data;
    DBIB_SendData(data_buf, 1);
}


void lcd_st77916_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h)
{
    LCDC_Cmd(DISABLE);
    LCDC_SwitchMode(LCDC_MANUAL_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    LCDC_Cmd(ENABLE);
    uint16_t xEnd = xStart + w - 1;
    uint16_t yEnd = yStart + h - 1;

    st77916_write_cmd(0x2a);
    st77916_write_data(xStart >> 8);
    st77916_write_data(xStart & 0xff);
    st77916_write_data(xEnd >> 8);
    st77916_write_data(xEnd & 0xff);

    st77916_write_cmd(0x2b);
    st77916_write_data(yStart >> 8);
    st77916_write_data(yStart & 0xff);
    st77916_write_data(yEnd >> 8);
    st77916_write_data(yEnd & 0xff);

    DBIB_SetCS();
}

void lcd_st77916_seq_init(void)
{
    st77916_write_cmd(0xF0);    st77916_write_data(0x28);

    st77916_write_cmd(0xF2);    st77916_write_data(0x28);

    st77916_write_cmd(0x73);    st77916_write_data(0xF0);

    st77916_write_cmd(0x76);    st77916_write_data(0x0F);

    st77916_write_cmd(0x7C);    st77916_write_data(0xD1);

    st77916_write_cmd(0x83);    st77916_write_data(0xE0);

    st77916_write_cmd(0x84);    st77916_write_data(0x61);

    st77916_write_cmd(0xF2);    st77916_write_data(0x82);

    st77916_write_cmd(0xF0);    st77916_write_data(0x00);

    st77916_write_cmd(0xF0);    st77916_write_data(0x01);

    st77916_write_cmd(0xF1);    st77916_write_data(0x01);

    st77916_write_cmd(0xB0);    st77916_write_data(0x52);

    st77916_write_cmd(0xB1);    st77916_write_data(0x49);

    st77916_write_cmd(0xB2);    st77916_write_data(0x24);

    st77916_write_cmd(0xB3);    st77916_write_data(0x01);

    st77916_write_cmd(0xB4);    st77916_write_data(0x66);

    st77916_write_cmd(0xB5);    st77916_write_data(0x44);

    st77916_write_cmd(0xB6);    st77916_write_data(0xC5);

    st77916_write_cmd(0xB7);    st77916_write_data(0x40);

    st77916_write_cmd(0xB8);    st77916_write_data(0x86);

    st77916_write_cmd(0xB9);    st77916_write_data(0x15);

    st77916_write_cmd(0xBA);    st77916_write_data(0x00);

    st77916_write_cmd(0xBB);     st77916_write_data(0x08);

    st77916_write_cmd(0xBC);    st77916_write_data(0x08);

    st77916_write_cmd(0xBD);    st77916_write_data(0x00);

    st77916_write_cmd(0xBE);    st77916_write_data(0x00);

    st77916_write_cmd(0xBF);    st77916_write_data(0x07);

    st77916_write_cmd(0xC0);    st77916_write_data(0x80);

    st77916_write_cmd(0xC1);    st77916_write_data(0x10);

    st77916_write_cmd(0xC2);    st77916_write_data(0x37);

    st77916_write_cmd(0xC3);    st77916_write_data(0x80);

    st77916_write_cmd(0xC4);    st77916_write_data(0x10);

    st77916_write_cmd(0xC5);    st77916_write_data(0x37);

    st77916_write_cmd(0xC6);    st77916_write_data(0xA9);

    st77916_write_cmd(0xC7);    st77916_write_data(0x41);

    st77916_write_cmd(0xC8);    st77916_write_data(0x01);

    st77916_write_cmd(0xC9);    st77916_write_data(0xA9);

    st77916_write_cmd(0xCA);    st77916_write_data(0x41);

    st77916_write_cmd(0xCB);    st77916_write_data(0x01);

    st77916_write_cmd(0xCC);    st77916_write_data(0x7F);

    st77916_write_cmd(0xCD);    st77916_write_data(0x7F);

    st77916_write_cmd(0xCE);    st77916_write_data(0xFF);

    st77916_write_cmd(0xD0);    st77916_write_data(0x91);

    st77916_write_cmd(0xD1);    st77916_write_data(0x68);

    st77916_write_cmd(0xD2);    st77916_write_data(0x68);

    uint8_t data_F5h[2] = {0x00, 0xA5};
    st77916_write_cmd(0xF5);
    st77916_write_data(data_F5h[0]);
    st77916_write_data(data_F5h[1]);

    st77916_write_cmd(0xF1);    st77916_write_data(0x10);

    st77916_write_cmd(0xF0);    st77916_write_data(0x00);

    st77916_write_cmd(0xF0);    st77916_write_data(0x02);

    uint8_t data_E0h[14] = {0xF0, 0x0E, 0x14, 0x0B, 0x0B, 0x16, 0x3A, 0x44, 0x4E, 0x18, 0x14, 0x13, 0x2F, 0x35};
    uint8_t data_E1h[14] = {0xf0, 0x0D, 0x13, 0x0B, 0x0A, 0x16, 0x39, 0x43, 0x4E, 0x17, 0x13, 0x13, 0x2E, 0x34};
    st77916_write_cmd(0xE0);
    for (int i = 0; i < 14; i++)
    {
        st77916_write_data(data_E0h[i]);
    }

    st77916_write_cmd(0xE1);
    for (int i = 0; i < 14; i++)
    {
        st77916_write_data(data_E1h[i]);
    }

    st77916_write_cmd(0xF0);    st77916_write_data(0x10);

    st77916_write_cmd(0xF3);    st77916_write_data(0x10);

    st77916_write_cmd(0xE0);    st77916_write_data(0x09);

    st77916_write_cmd(0xE1);    st77916_write_data(0x00);

    st77916_write_cmd(0xE2);    st77916_write_data(0x03);

    st77916_write_cmd(0xE3);    st77916_write_data(0x00);

    st77916_write_cmd(0xE4);    st77916_write_data(0xE0);

    st77916_write_cmd(0xE5);    st77916_write_data(0x06);

    st77916_write_cmd(0xE6);    st77916_write_data(0x21);

    st77916_write_cmd(0xE7);    st77916_write_data(0x00);

    st77916_write_cmd(0xE8);    st77916_write_data(0x05);

    st77916_write_cmd(0xE9);    st77916_write_data(0x82);

    st77916_write_cmd(0xEA);    st77916_write_data(0xDE);

    st77916_write_cmd(0xEB);    st77916_write_data(0xC0);

    st77916_write_cmd(0xEC);    st77916_write_data(0x40);

    st77916_write_cmd(0xED);    st77916_write_data(0x84);

    st77916_write_cmd(0xEE);    st77916_write_data(0xFF);

    st77916_write_cmd(0xEF);    st77916_write_data(0x71);

    st77916_write_cmd(0xF8);    st77916_write_data(0xFF);

    st77916_write_cmd(0xF9);    st77916_write_data(0x50);

    st77916_write_cmd(0xFA);    st77916_write_data(0xFF);

    st77916_write_cmd(0xFB);    st77916_write_data(0xF3);

    st77916_write_cmd(0xFC);    st77916_write_data(0x00);

    st77916_write_cmd(0xFD);    st77916_write_data(0x00);
    st77916_write_cmd(0xFE);    st77916_write_data(0x00);
    st77916_write_cmd(0xFF);    st77916_write_data(0x00);
    st77916_write_cmd(0x60);    st77916_write_data(0x42);
    st77916_write_cmd(0x61);    st77916_write_data(0xDF);
    st77916_write_cmd(0x62);    st77916_write_data(0x40);
    st77916_write_cmd(0x63);    st77916_write_data(0x40);
    st77916_write_cmd(0x64);    st77916_write_data(0x02);
    st77916_write_cmd(0x65);    st77916_write_data(0x00);
    st77916_write_cmd(0x66);    st77916_write_data(0x00);
    st77916_write_cmd(0x67);    st77916_write_data(0x00);
    st77916_write_cmd(0x68);    st77916_write_data(0x00);
    st77916_write_cmd(0x69);    st77916_write_data(0x00);
    st77916_write_cmd(0x6A);    st77916_write_data(0x00);
    st77916_write_cmd(0x6B);    st77916_write_data(0x00);
    st77916_write_cmd(0x70);    st77916_write_data(0x42);
    st77916_write_cmd(0x71);    st77916_write_data(0xDF);
    st77916_write_cmd(0x72);    st77916_write_data(0x40);
    st77916_write_cmd(0x73);    st77916_write_data(0x40);
    st77916_write_cmd(0x74);    st77916_write_data(0x01);
    st77916_write_cmd(0x75);    st77916_write_data(0x00);
    st77916_write_cmd(0x76);    st77916_write_data(0x00);
    st77916_write_cmd(0x77);    st77916_write_data(0x00);
    st77916_write_cmd(0x78);    st77916_write_data(0x00);
    st77916_write_cmd(0x79);    st77916_write_data(0x00);
    st77916_write_cmd(0x7A);    st77916_write_data(0x00);
    st77916_write_cmd(0x7B);    st77916_write_data(0x00);
    st77916_write_cmd(0x80);    st77916_write_data(0x48);
    st77916_write_cmd(0x81);    st77916_write_data(0x00);
    st77916_write_cmd(0x82);    st77916_write_data(0x04);
    st77916_write_cmd(0x83);    st77916_write_data(0x02);
    st77916_write_cmd(0x84);    st77916_write_data(0xDC);
    st77916_write_cmd(0x85);    st77916_write_data(0x00);
    st77916_write_cmd(0x86);    st77916_write_data(0x00);
    st77916_write_cmd(0x87);    st77916_write_data(0x00);
    st77916_write_cmd(0x88);    st77916_write_data(0x48);
    st77916_write_cmd(0x89);    st77916_write_data(0x00);
    st77916_write_cmd(0x8A);    st77916_write_data(0x06);
    st77916_write_cmd(0x8B);    st77916_write_data(0x02);
    st77916_write_cmd(0x8C);    st77916_write_data(0xDE);
    st77916_write_cmd(0x8D);    st77916_write_data(0x00);
    st77916_write_cmd(0x8E);    st77916_write_data(0x00);
    st77916_write_cmd(0x8F);    st77916_write_data(0x00);
    st77916_write_cmd(0x90);    st77916_write_data(0x48);
    st77916_write_cmd(0x91);    st77916_write_data(0x00);
    st77916_write_cmd(0x92);    st77916_write_data(0x08);
    st77916_write_cmd(0x93);    st77916_write_data(0x02);
    st77916_write_cmd(0x94);    st77916_write_data(0xE0);
    st77916_write_cmd(0x95);    st77916_write_data(0x00);
    st77916_write_cmd(0x96);    st77916_write_data(0x00);
    st77916_write_cmd(0x97);    st77916_write_data(0x00);
    st77916_write_cmd(0x98);    st77916_write_data(0x48);
    st77916_write_cmd(0x99);    st77916_write_data(0x00);
    st77916_write_cmd(0x9A);    st77916_write_data(0x0A);
    st77916_write_cmd(0x9B);    st77916_write_data(0x02);
    st77916_write_cmd(0x9C);    st77916_write_data(0xE2);
    st77916_write_cmd(0x9D);    st77916_write_data(0x00);
    st77916_write_cmd(0x9E);    st77916_write_data(0x00);
    st77916_write_cmd(0x9F);    st77916_write_data(0x00);
    st77916_write_cmd(0xA0);    st77916_write_data(0x48);
    st77916_write_cmd(0xA1);    st77916_write_data(0x00);
    st77916_write_cmd(0xA2);    st77916_write_data(0x03);
    st77916_write_cmd(0xA3);    st77916_write_data(0x02);
    st77916_write_cmd(0xA4);    st77916_write_data(0xDB);
    st77916_write_cmd(0xA5);    st77916_write_data(0x00);
    st77916_write_cmd(0xA6);    st77916_write_data(0x00);
    st77916_write_cmd(0xA7);    st77916_write_data(0x00);
    st77916_write_cmd(0xA8);    st77916_write_data(0x48);
    st77916_write_cmd(0xA9);    st77916_write_data(0x00);
    st77916_write_cmd(0xAA);    st77916_write_data(0x05);
    st77916_write_cmd(0xAB);    st77916_write_data(0x02);
    st77916_write_cmd(0xAC);    st77916_write_data(0xDD);
    st77916_write_cmd(0xAD);    st77916_write_data(0x00);
    st77916_write_cmd(0xAE);    st77916_write_data(0x00);
    st77916_write_cmd(0xAF);    st77916_write_data(0x00);
    st77916_write_cmd(0xB0);    st77916_write_data(0x48);
    st77916_write_cmd(0xB1);    st77916_write_data(0x00);
    st77916_write_cmd(0xB2);    st77916_write_data(0x07);
    st77916_write_cmd(0xB3);    st77916_write_data(0x02);
    st77916_write_cmd(0xB4);    st77916_write_data(0xDF);
    st77916_write_cmd(0xB5);    st77916_write_data(0x00);
    st77916_write_cmd(0xB6);    st77916_write_data(0x00);
    st77916_write_cmd(0xB7);    st77916_write_data(0x00);
    st77916_write_cmd(0xB8);    st77916_write_data(0x48);
    st77916_write_cmd(0xB9);    st77916_write_data(0x00);
    st77916_write_cmd(0xBA);    st77916_write_data(0x09);
    st77916_write_cmd(0xBB);    st77916_write_data(0x02);
    st77916_write_cmd(0xBC);    st77916_write_data(0xE1);
    st77916_write_cmd(0xBD);    st77916_write_data(0x00);
    st77916_write_cmd(0xBE);    st77916_write_data(0x00);
    st77916_write_cmd(0xBF);    st77916_write_data(0x00);
    st77916_write_cmd(0xC0);    st77916_write_data(0x65);
    st77916_write_cmd(0xC1);    st77916_write_data(0x74);
    st77916_write_cmd(0xC2);    st77916_write_data(0x47);
    st77916_write_cmd(0xC3);    st77916_write_data(0x56);
    st77916_write_cmd(0xC4);    st77916_write_data(0xAA);
    st77916_write_cmd(0xC5);    st77916_write_data(0x11);
    st77916_write_cmd(0xC6);    st77916_write_data(0x00);
    st77916_write_cmd(0xC7);    st77916_write_data(0x2A);
    st77916_write_cmd(0xC8);    st77916_write_data(0xA2);
    st77916_write_cmd(0xC9);    st77916_write_data(0x33);
    st77916_write_cmd(0xD0);    st77916_write_data(0x65);
    st77916_write_cmd(0xD1);    st77916_write_data(0x74);
    st77916_write_cmd(0xD2);    st77916_write_data(0x47);
    st77916_write_cmd(0xD3);    st77916_write_data(0x56);
    st77916_write_cmd(0xD4);    st77916_write_data(0xAA);
    st77916_write_cmd(0xD5);    st77916_write_data(0x11);
    st77916_write_cmd(0xD6);    st77916_write_data(0x00);
    st77916_write_cmd(0xD7);    st77916_write_data(0x2A);
    st77916_write_cmd(0xD8);    st77916_write_data(0xA2);
    st77916_write_cmd(0xD9);    st77916_write_data(0x33);
    st77916_write_cmd(0xF3);    st77916_write_data(0x01);
    st77916_write_cmd(0xF0);    st77916_write_data(0x00);
    st77916_write_cmd(0x21);

#if (ENABLE_TE_FOR_LCD == 1)
    st77916_write_cmd(0x35); //TE mode
    st77916_write_data(0x00); //TE mode
#endif
    st77916_write_cmd(0x3A); //set pixel format
    st77916_write_data(0x55); //16bit RGB565

    st77916_write_cmd(0x11); //exit sleep mode
    platform_delay_ms(120);

    st77916_write_cmd(0x29);
}

static void lcd_reset_init(void)
{
//    hal_gpio_init();
//    hal_gpio_init_pin(LCD_8080_RST, GPIO_TYPE_AON, GPIO_DIR_OUTPUT, GPIO_PULL_UP);
//    hal_gpio_set_level(LCD_8080_RST, GPIO_LEVEL_HIGH);
}
static void lcd_set_reset(bool reset)
{
    if (reset)
    {
        Pad_Config(LCD_8080_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }
    else
    {
        Pad_Config(LCD_8080_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
}
void lcd_pad_init(void)
{
    Pad_Config(LCD_8080_D0, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_D1, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_D2, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_D3, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_D4, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_D5, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_D6, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_D7, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_CS, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_DCX, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_RD, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_8080_WR, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    Pad_HighSpeedFuncSel(LCD_8080_D0, HS_Func0);
    Pad_HighSpeedFuncSel(LCD_8080_D1, HS_Func0);
    Pad_HighSpeedFuncSel(LCD_8080_D2, HS_Func0);
    Pad_HighSpeedFuncSel(LCD_8080_D3, HS_Func0);
    Pad_HighSpeedFuncSel(LCD_8080_D4, HS_Func0);
    Pad_HighSpeedFuncSel(LCD_8080_D5, HS_Func0);
    Pad_HighSpeedFuncSel(LCD_8080_D6, HS_Func0);
    Pad_HighSpeedFuncSel(LCD_8080_D7, HS_Func0);
    Pad_HighSpeedFuncSel(LCD_8080_CS, HS_Func0);
    Pad_HighSpeedFuncSel(LCD_8080_DCX, HS_Func0);
    Pad_HighSpeedFuncSel(LCD_8080_RD, HS_Func0);
    Pad_HighSpeedFuncSel(LCD_8080_WR, HS_Func0);
    Pad_HighSpeedMuxSel(LCD_8080_D0, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCD_8080_D1, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCD_8080_D2, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCD_8080_D3, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCD_8080_D4, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCD_8080_D5, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCD_8080_D6, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCD_8080_D7, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCD_8080_CS, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCD_8080_DCX, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCD_8080_RD, FROM_CORE_DOMAIN);
    Pad_HighSpeedMuxSel(LCD_8080_WR, FROM_CORE_DOMAIN);
    lcd_reset_init();
#if (ENABLE_TE_FOR_LCD == 1)
    /*TE Config*/
    /*TE Type Config*/
    rtk_lcd_hal_set_TE_type(LCDC_TE_TYPE_HW_TE);
    /*TE Pad Config, P2_2 -> Hardware TE*/
    if (rtk_lcd_hal_get_TE_type() == LCDC_TE_TYPE_HW_TE)
    {
        Pad_Config(LCD_TE_SYNC, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_HIGH);
        Pad_HighSpeedMuxSel(LCD_TE_SYNC, FROM_CORE_DOMAIN);
    }
    /*TE Pad Config, P4_5 -> Software TE*/
    else if (rtk_lcd_hal_get_TE_type() == LCDC_TE_TYPE_SW_TE)
    {
        lcd_te_device_init();
    }
#endif
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
    return ST77916_DRV_PIXEL_BITS ;
}
void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h)
{
    lcd_st77916_set_window(xStart, yStart, xStart + w - 1, yStart + h - 1);
}
void lcdc_driver_init(void)
{
    //RCC_DisplayClockConfig(DISPLAY_CLOCK_SOURCE_PLL1, DISPLAY_CLOCK_DIV_1);
    RCC_DisplayClockConfig(DISPLAY_CLOCK_SOURCE_40MHZ, DISPLAY_CLOCK_DIV_1);
    RCC_PeriphClockCmd(APBPeriph_DISP, APBPeriph_DISP_CLOCK, ENABLE);
    LCDC_InitTypeDef lcdc_init = {0};
    lcdc_init.LCDC_GroupSel = 0;
    lcdc_init.LCDC_Interface = LCDC_IF_DBIB;
    lcdc_init.LCDC_PixelInputFormat = LCDC_INPUT_RGB565;
    lcdc_init.LCDC_PixelOutputFormat = LCDC_OUTPUT_RGB565;
    lcdc_init.LCDC_PixelBitSwap = LCDC_SWAP_8BIT;
#if TE_VALID
    lcdc_init.LCDC_TeEn = ENABLE;
    lcdc_init.LCDC_TePolarity = LCDC_TE_EDGE_RISING;
    lcdc_init.LCDC_TeInputMux = LCDC_TE_LCD_INPUT;
#endif
    lcdc_init.LCDC_DmaThreshold = 64; // MSize + threshold should be no larger than 128
    LCDC_Init(&lcdc_init);
    LCDC_ClearINTPendingBit(LCDC_CLR_WAVEFORM_FINISH);
    LCDC_DBIBCfgTypeDef dbib_init = {0};
    dbib_init.DBIB_Clock_Divider         = 2;
    dbib_init.DBIB_InitGuardTimeCmd  = DBIB_INIT_GUARD_TIME_DISABLE;
    dbib_init.DBIB_InitGuardTime     = DBIB_GUARD_TIME_1T;
    dbib_init.DBIB_CmdGuardTimeCmd   = DBIB_CMD_GUARD_TIME_DISABLE;
    dbib_init.DBIB_CmdGuardTime      = DBIB_GUARD_TIME_1T;
    dbib_init.DBIB_GuardTimeCmd      = DBIB_GUARD_TIME_DISABLE;
    dbib_init.DBIB_GuardTime         = DBIB_GUARD_TIME_1T;
    dbib_init.DBIB_WRDelay           = DBIB_WR_HALF_DELAY;
    DBIB_Init(&dbib_init);
    LCDC_SwitchMode(LCDC_MANUAL_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    LCDC_Cmd(ENABLE);

    lcd_set_reset(true);
    platform_delay_ms(120);
    lcd_set_reset(false);
    platform_delay_ms(5);
}
void rtk_lcd_hal_init(void)
{
    lcd_pad_init();
    lcdc_driver_init();
    lcd_st77916_seq_init();

    rtk_lcd_hal_rect_fill(0, 0, ST77916_LCD_WIDTH, ST77916_LCD_HEIGHT, 0xFF00);
}
void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len)
{
    if (((uint32_t)buf % 4) != 0)
    {
        assert_param("buf not 4byte aligned!");
        while (1);
    }
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
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En      = 0;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);
    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();
    LCDC_SwitchMode(LCDC_AUTO_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    DBIB_BypassCmdByteCmd(DISABLE);
    uint8_t cmd[1] = {0x2C};
    LCDC_DBIB_SetCmdSequence(cmd, 1);
    LCDC_SetTxPixelLen(len);
    LCDC_Cmd(ENABLE);
    LCDC_DMA_SetSourceAddress(LCDC_DMA_CHANNEL_INDEX, (uint32_t)buf);
    LCDC_ForceBurst(ENABLE);
    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);
#if (ENABLE_TE_FOR_LCD == 1)
    if (rtk_lcd_hal_get_TE_type() == LCDC_TE_TYPE_HW_TE)
    {
        LCDC_TeCmd(ENABLE);
    }
    else
    {
        LCDC_AutoWriteCmd(ENABLE);
    }
#endif
#if (ENABLE_TE_FOR_LCD == 0)
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
#if (ENABLE_TE_FOR_LCD == 1)
    if (rtk_lcd_hal_get_TE_type() == LCDC_TE_TYPE_HW_TE)
    {
        LCDC_TeCmd(DISABLE);
    }
#endif
    LCDC_Cmd(DISABLE);
    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();
    LCDC_AXIMUXMode(LCDC_FW_MODE);
    LCDC_ClearINTPendingBit(LCDC_CLR_WAVEFORM_FINISH);
}
void rtk_lcd_hal_rect_fill(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h,
                           uint32_t color)
{
    rtk_lcd_hal_set_window(xStart, yStart, w, h);
    uint8_t *RGB_transfer = (uint8_t *)&color;
    uint32_t clear_buf[64] = {0};
    uint16_t rgb_color = 0;
    uint16_t *rgb565_buf = (uint16_t *)clear_buf;
    rgb_color = RGB_transfer[0] << 8 | RGB_transfer[1];
    for (int i = 0; i < 64 * 2; i++)
    {
        rgb565_buf[i] = rgb_color;
    }
    LCDC_DMA_InitTypeDef LCDC_DMA_InitStruct = {0};
    LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
    LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum          = LCDC_DMA_CHANNEL_NUM;
    LCDC_DMA_InitStruct.LCDC_DMA_DIR                 = LCDC_DMA_DIR_PeripheralToMemory;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceInc           = LCDC_DMA_SourceInc_Fix;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc      = LCDC_DMA_DestinationInc_Fix;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize      = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_64;
    LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_64;
    LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = (uint32_t)clear_buf;
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En      = 0;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);
    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();
    LCDC_SwitchMode(LCDC_AUTO_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    DBIB_BypassCmdByteCmd(DISABLE);
    uint8_t cmd[1] = {0x2C};
    LCDC_DBIB_SetCmdSequence(cmd, 1);
    LCDC_SetTxPixelLen(w * h);
    LCDC_Cmd(ENABLE);
    LCDC_DMA_SetSourceAddress(LCDC_DMA_CHANNEL_INDEX, (uint32_t)clear_buf);
    LCDC_ForceBurst(ENABLE);
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
}
void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len)
{
    if (((uint32_t)buf % 4) != 0)
    {
        assert_param("buf not 4byte aligned!");
        while (1);
    }
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
    LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En      = 0;
    LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);
    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();
    LCDC_SwitchMode(LCDC_AUTO_MODE);
    LCDC_SwitchDirect(LCDC_TX_MODE);
    DBIB_BypassCmdByteCmd(DISABLE);
    uint8_t cmd[1] = {0x2C};
    LCDC_DBIB_SetCmdSequence(cmd, 1);
    LCDC_SetTxPixelLen(len);
    LCDC_Cmd(ENABLE);
    LCDC_DMA_SetSourceAddress(LCDC_DMA_CHANNEL_INDEX, (uint32_t)buf);
    LCDC_ForceBurst(ENABLE);
    LCDC_DMAChannelCmd(LCDC_DMA_CHANNEL_NUM, ENABLE);
    LCDC_DmaCmd(ENABLE);
#if (ENABLE_TE_FOR_LCD == 1)
    LCDC_HANDLER_TEAR_CTR_TypeDef handler_reg_0x10 = {.d32 = LCDC_HANDLER->TEAR_CTR};
    handler_reg_0x10.b.bypass_t2w_delay = 0;
    handler_reg_0x10.b.t2w_delay = 0xfff;
    LCDC_HANDLER->TEAR_CTR = handler_reg_0x10.d32;
    LCDC_TeCmd(ENABLE);
#else
    LCDC_AutoWriteCmd(ENABLE);
#endif
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
#if (ENABLE_TE_FOR_LCD == 1)
    LCDC_TeCmd(DISABLE);
#endif
    LCDC_ClearDmaFifo();
    LCDC_ClearTxPixelCnt();
    LCDC_Cmd(DISABLE);
}

