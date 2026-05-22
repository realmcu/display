/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _LCD_390_450_LCDC_H_
#define _LCD_390_450_LCDC_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"




#define TE_VALID                            0
#define C05300_LCD_WIDTH                   390
#define C05300_LCD_HEIGHT                  450
#define C05300_DRV_PIXEL_BITS              16
#define INPUT_PIXEL_BYTES                   2
#if INPUT_PIXEL_BYTES == 3
#error "LCDC DMA doesn't allow 3 bytes input"
#endif
#define OUTPUT_PIXEL_BYTES                  2





void rtk_lcd_hal_init(void);
void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);
void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_clear_screen(uint32_t ARGB_color);
void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_transfer_done(void);


uint32_t rtk_lcd_hal_get_width(void);
uint32_t rtk_lcd_hal_get_height(void);
uint32_t rtk_lcd_hal_get_pixel_bits(void);
uint32_t rtk_lcd_hal_power_off(void);
uint32_t rtk_lcd_hal_power_on(void);

void rtk_lcd_hal_lcd_enter_dlps(void);


void rtk_lcd_hal_update_fb_use_left_and_right(uint32_t left,
                                              uint32_t right,
                                              uint32_t w,
                                              uint32_t h,
                                              uint32_t offset,
                                              uint32_t bytes_per_pixel);
void rtk_lcd_hal_update_fb_use_up_and_down(uint32_t up,
                                           uint32_t down,
                                           uint32_t w,
                                           uint32_t h,
                                           uint32_t offset,
                                           uint32_t bytes_per_pixel);

#ifdef __cplusplus
}
#endif

#endif /* _LCD_390_450_LCDC_H_ */
