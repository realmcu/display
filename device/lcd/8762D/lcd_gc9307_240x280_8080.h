/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _LCD_GC9307_240_280_8080_H_
#define _LCD_GC9307_240_280_8080_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"

#define LCD_8080_D0                      P0_4
#define LCD_8080_D1                      P0_5
#define LCD_8080_D2                      P0_6
#define LCD_8080_D3                      P0_7
#define LCD_8080_D4                      P4_0
#define LCD_8080_D5                      P4_1
#define LCD_8080_D6                      P4_2
#define LCD_8080_D7                      P4_3

#define LCD_8080_CS                      P0_0
#define LCD_8080_DCX                     P1_5
#define LCD_8080_RD                      P1_6
#define LCD_8080_WR                      P0_2
#define LCD_DMA_CHANNEL_NUM              1
#define LCD_DMA_CHANNEL_INDEX            GDMA_Channel1
#define LCD_DMA_CHANNEL_IRQ              GDMA0_Channel1_IRQn

#define GC9307_LCD_WIDTH                  240
#define GC9307_LCD_HIGHT                  280
#define INPUT_PIXEL_BYTES                   2
#define OUTPUT_PIXEL_BYTES                  2
#define GC9307_DRV_PIXEL_BITS               16


void rtk_lcd_hal_init(void);
void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);

void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_rect_fill(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h,
                           uint32_t color);
void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_transfer_done(void);
uint32_t rtk_lcd_hal_get_width(void);
uint32_t rtk_lcd_hal_get_height(void);
uint32_t rtk_lcd_hal_get_pixel_bits(void);
uint32_t rtk_lcd_hal_power_on(void);
uint32_t rtk_lcd_hal_power_off(void);

#ifdef __cplusplus
}
#endif

#endif /* _LCD_ST7796_320_H_ */
