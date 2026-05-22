/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _CO5300_390X390_QSPI_H_
#define _CO5300_390X390_QSPI_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"
#include "stdbool.h"

#define DMA_LINKLIST                        0
#define TE_VALID                            1
#define CO5300_LCD_WIDTH                  390
#define CO5300_LCD_HIGHT                  390
#define INPUT_PIXEL_BYTES                   2
#define OUTPUT_PIXEL_BYTES                  2
#define CO5300_DRV_PIXEL_BITS             16


// #define LCD_QSPI_RST                     P3_6
//#define LCD_QSPI_BL                      P1_2
#define LCD_QSPI_D0                      P5_5
#define LCD_QSPI_D1                      P1_5
#define LCD_QSPI_D2                      P1_6
#define LCD_QSPI_D3                      P3_6
#define LCD_QSPI_CS                      P5_2
#define LCD_QSPI_CLK                     P5_4
#define LCD_QSPI_TE                      P5_3
//#define LCD_QSPI_IM0                      P0_1
//#define LCD_QSPI_IM1                      P0_1




void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);
void rtk_lcd_hal_init(void);
void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_rect_fill(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h,
                           uint32_t color);
uint32_t rtk_lcd_hal_get_width(void);
uint32_t rtk_lcd_hal_get_height(void);
uint32_t rtk_lcd_hal_get_pixel_bits(void);
bool rtk_lcd_hal_power_off(void);
bool rtk_lcd_hal_power_on(void);
bool rtk_lcd_hal_dlps_check(void);
bool rtk_lcd_wake_up(void);
void rtk_lcd_dlps_init(void);
uint32_t rtk_lcd_hal_dlps_restore(void);
#ifdef __cplusplus
}
#endif

#endif /* _ICNA3311_280X456_QSPI_H_ */
