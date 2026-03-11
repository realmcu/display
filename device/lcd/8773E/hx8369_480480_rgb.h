/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _LCD_HX8369_480480_RGB_H_
#define _LCD_HX8369_480480_RGB_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"
#include "stdbool.h"

#define HX8369_480480_LCD_WIDTH                   480
#define HX8369_480480_LCD_HEIGHT                  480

#define HX8369_DRV_PIXEL_BITS                     16 // 24



void hx8369_init(void);
void lcd_hx8369_init_framebuffer(uint8_t *buf, uint32_t len);
void lcd_hx8369_update_framebuffer(uint8_t *buf, uint32_t len);

void rtk_lcd_hal_init(void);
void rtk_lcd_hal_update_framebuffer(uint8_t *p_buf, uint32_t size);
void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);
void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_transfer_done(void);
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

#endif /* _LCD_HX8369_480480_RGB_H_ */
