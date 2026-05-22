/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _LCD_ST7789_170_320_SPI_H_
#define _LCD_ST7789_170_320_SPI_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"
#include "stdbool.h"

#define DMA_LINKLIST                        0
//#define TE_VALID                            1
#define ST7789_LCD_WIDTH                  240
#define ST7789_LCD_HIGHT                  320

#define INPUT_PIXEL_BYTES                   2
#define OUTPUT_PIXEL_BYTES                  2
#define ST7789_DRV_PIXEL_BITS             16


#define LCD_BACKLIGHT_PIN P9_6  // Low level turns on backlight power

#define LCD_QSPI_D0                     P9_3
#define LCD_SPI_CS                      P9_2
#define LCD_SPI_CLK                     P9_4
#define LCD_SPI_DC                      P4_3
#define LCD_SPI_RST                     P9_1




typedef struct
{
    int dc3v3;           // Pin for 3.3V power
    int power;           // Pin for LCD module power
    int backlight;       // Pin for backlight power
} LcdPowerPins;

void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);
void rtk_lcd_hal_init(void);
void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_rect_fill(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h,
                           uint32_t color);
uint32_t rtk_lcd_hal_get_width(void);
uint32_t rtk_lcd_hal_get_height(void);
uint32_t rtk_lcd_hal_get_pixel_bits(void);
uint32_t rtk_lcd_hal_power_off(void);
uint32_t rtk_lcd_hal_power_on(void);
uint32_t rtk_lcd_hal_dlps_restore(void);

void power_for_lcd(bool flg);
void backlight_for_lcd(bool flg);
#ifdef __cplusplus
}
#endif

#endif /* _LCD_ST7789_170_320_SPI_H_ */