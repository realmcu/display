/*
 * Copyright(c) 2026, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LCD_ST7801N_390_450_QSPI_H__
#define __LCD_ST7801N_390_450_QSPI_H__

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

#define ST7801N_390_450_LCD_WIDTH           390
#define ST7801N_390_450_LCD_HEIGHT          450
#define ST7801N_390_450_DRV_PIXEL_BITS      (INPUT_PIXEL_BYTES * 8)

#define ST7801N_MAX_PARA_COUNT              20

typedef struct _ST7801N_CMD_DESC
{
    uint8_t instruction;
    uint8_t index;
    uint16_t delay;
    uint16_t wordcount;
    uint8_t  payload[ST7801N_MAX_PARA_COUNT];
} ST7801N_CMD_DESC;

typedef enum
{
    LCDC_TE_TYPE_NO_TE = 0x00,
    LCDC_TE_TYPE_HW_TE = 0x01,
    LCDC_TE_TYPE_SW_TE = 0x02,
} T_LCDC_TE_TYPE;

void rtk_lcd_hal_init(void);
void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);
void rtk_lcd_hal_clear_screen(uint32_t ARGB_color);
void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_transfer_done(void);
bool rtk_lcd_hal_power_on(void);
bool rtk_lcd_hal_power_off(void);
void rtk_lcd_hal_lcd_enter_dlps(void);

void rtk_lcd_hal_set_brightness(uint8_t brightness);
void rtk_lcd_hal_set_display(bool on);
void rtk_lcd_hal_set_partial_mode(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);
void rtk_lcd_hal_exit_partial_mode(void);

void rtk_lcd_hal_set_TE_type(T_LCDC_TE_TYPE state);
T_LCDC_TE_TYPE rtk_lcd_hal_get_TE_type(void);
uint32_t rtk_lcd_hal_get_width(void);
uint32_t rtk_lcd_hal_get_height(void);
uint32_t rtk_lcd_hal_get_pixel_bits(void);

#ifdef __cplusplus
}
#endif
#endif /* __LCD_ST7801N_390_450_QSPI_H__ */
