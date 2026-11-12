/**
*****************************************************************************************
*     Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.

*
*     SPDX-License-Identifier: Apache-2.0
*****************************************************************************************
* @file    lcd_auto_detect_360_360_qspi.h
* @brief   This file provides the auto-detect 360x360 QSPI LCD driver API
* @author
* @date
* @version v1.0
* *************************************************************************************
*/

#ifndef __LCD_AUTO_360_360_QSPI_H__
#define __LCD_AUTO_360_360_QSPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define TE_VALID                            0

#define OUTPUT_PIXEL_BYTES                  2

#if OUTPUT_PIXEL_BYTES == 3
#define INPUT_PIXEL_BYTES                   4
#elif OUTPUT_PIXEL_BYTES == 2
#define INPUT_PIXEL_BYTES                   2
#endif

#if INPUT_PIXEL_BYTES == 3
#error "LCDC DMA doesn't allow 3 bytes input"
#endif


#define LCD_AUTO_360_360_WIDTH           360
#define LCD_AUTO_360_360_HEIGHT          360
#define LCD_AUTO_360_360_PIXEL_BITS      (INPUT_PIXEL_BYTES * 8)








typedef enum
{
    LCDC_TE_TYPE_NO_TE = 0x00,
    LCDC_TE_TYPE_HW_TE = 0x01,
    LCDC_TE_TYPE_SW_TE = 0x02,
} T_LCDC_TE_TYPE;


void rtk_lcd_hal_init(void);
void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);
void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_clear_screen(uint32_t ARGB_color);
void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_transfer_done(void);
bool rtk_lcd_hal_power_on(void);
bool rtk_lcd_hal_power_off(void);
uint32_t rtk_lcd_hal_read_id(void);


void rtk_lcd_hal_set_TE_type(T_LCDC_TE_TYPE state);
uint32_t rtk_lcd_hal_get_width(void);
uint32_t rtk_lcd_hal_get_height(void);
uint32_t rtk_lcd_hal_get_pixel_bits(void);


#ifdef __cplusplus
}
#endif
#endif /* __LCD_AUTO_360_360_QSPI_H__ */
