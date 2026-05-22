/**
*****************************************************************************************
*     Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.

*
*     SPDX-License-Identifier: Apache-2.0
*****************************************************************************************
* @file    st77916_360_360_dbib_8080.h
* @brief   This file provides LCDC 8080 functions
* @author
* @date
* @version v1.0
* *************************************************************************************
*/
#ifndef _ST77916_360_360_DBIB_8080_H_
#define _ST77916_360_360_DBIB_8080_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"
#include "stdbool.h"
#define INPUT_PIXEL_BYTES                   2

#define ST77916_LCD_WIDTH                   360
#define ST77916_LCD_HEIGHT                  360
#define ST77916_DRV_PIXEL_BITS              16

#if INPUT_PIXEL_BYTES == 3
#error "LCDC DMA doesn't allow 3 bytes input"
#endif

#ifdef ADC_3
#define P0_3               ADC_3     /*!< GPIOA3 */
#endif

#define LCD_8080_RST                     P9_0
#define LCD_8080_D0                      P2_6
#define LCD_8080_D1                      P2_7
#define LCD_8080_D2                      P4_0
#define LCD_8080_D3                      P4_1
#define LCD_8080_D4                      P4_2
#define LCD_8080_D5                      P4_3
#define LCD_8080_D6                      P4_4
#define LCD_8080_D7                      P4_5
#define LCD_8080_CS                      P2_3
#define LCD_8080_DCX                     P2_4
#define LCD_8080_RD                      P0_3
#define LCD_8080_WR                      P2_5
#define LCD_TE_SYNC                      P2_2

void lcd_st77916_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);
void lcd_st77916_seq_init(void);

void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_rect_fill(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h,
                           uint32_t color);
void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len);
uint32_t rtk_lcd_hal_get_width(void);
uint32_t rtk_lcd_hal_get_height(void);
uint32_t rtk_lcd_hal_get_pixel_bits(void);
#ifdef __cplusplus
}
#endif
#endif /* _ST77916_360_360_DBIB_8080_H_ */
