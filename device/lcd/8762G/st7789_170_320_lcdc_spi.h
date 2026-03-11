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


#define DMA_LINKLIST                        0
//#define TE_VALID                            1
#define ST7789_LCD_WIDTH                  170
#define ST7789_LCD_HIGHT                  320

#define INPUT_PIXEL_BYTES                   2
#define OUTPUT_PIXEL_BYTES                  2
#define ST7789_DRV_PIXEL_BITS             16

#define LCD_SPI_RST                     P3_6
#define LCD_SPI_BL                      P1_4
#define LCD_QSPI_D0                     P4_2
#define LCD_QSPI_D1                     P4_1
#define LCD_QSPI_D2                     P3_2
#define LCD_QSPI_D3                     P3_3
#define LCD_SPI_CS                      P4_3
#define LCD_SPI_CLK                     P4_0
#define LCD_SPI_TE                      P0_5
#define LCD_SPI_DC                      P3_4





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
#ifdef __cplusplus
}
#endif

#endif /* _LCD_ST7789_170_320_SPI_H_ */
