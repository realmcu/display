/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _LCD_SH8601Z_466_466_QSPI_H_
#define _LCD_SH8601Z_466_466_QSPI_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"


#define SH8601Z_MAX_PARA_COUNT             (60)


#define TE_VALID                            1

#define SH8601Z_LCD_WIDTH                   466
#define SH8601Z_LCD_HEIGHT                  466

#define INPUT_PIXEL_BYTES                   2
#define OUTPUT_PIXEL_BYTES                  2

#if INPUT_PIXEL_BYTES == 2
#define SH8601Z_DRV_PIXEL_BITS              16
#elif INPUT_PIXEL_BYTES == 3
#define SH8601Z_DRV_PIXEL_BITS              24
#elif INPUT_PIXEL_BYTES == 4
#define SH8601Z_DRV_PIXEL_BITS              32
#endif

typedef struct _SH8601Z_CMD_DESC
{
    uint8_t instruction;
    uint8_t index;
    uint16_t delay;
    uint16_t wordcount;
    uint8_t  payload[SH8601Z_MAX_PARA_COUNT];
} SH8601Z_CMD_DESC;

uint32_t SH8601Z_get_width(void);
uint32_t SH8601Z_get_height(void);
uint32_t SH8601Z_get_pixel_bits(void);

void SH8601Z_init(void);
void SH8601Z_qspi_power_on(void);
void SH8601Z_qspi_power_off(void);
void SH8601Z_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);

typedef enum
{
    LCDC_TE_TYPE_NO_TE = 0x00,
    LCDC_TE_TYPE_HW_TE = 0x01,
    LCDC_TE_TYPE_SW_TE = 0x02,
} T_LCDC_TE_TYPE;

uint32_t rtk_lcd_hal_get_width(void);
uint32_t rtk_lcd_hal_get_height(void);
uint32_t rtk_lcd_hal_get_pixel_bits(void);
bool rtk_lcd_hal_power_off(void);
bool rtk_lcd_hal_power_on(void);

void rtk_lcd_hal_init(void);
void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);
void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_clear_screen(uint32_t ARGB_color);
void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_transfer_done(void);

void rtk_lcd_hal_lcd_enter_dlps(void);
void rtk_lcd_hal_set_TE_type(T_LCDC_TE_TYPE state);
T_LCDC_TE_TYPE rtk_lcd_hal_get_TE_type(void);

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

#endif /* _LCD_SH8601Z_466_466_QSPI_H_ */
