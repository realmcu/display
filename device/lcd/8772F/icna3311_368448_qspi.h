/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _ICNA3311_368448_QSPI_H_
#define _ICNA3311_368448_QSPI_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"

#define ICNA3311_MAX_PARA_COUNT                          (300)

#define TE_VALID                            1
#define ICNA3311_LCD_WIDTH                   368
#define ICNA3311_LCD_HIGHT                   448
#define ICNA3311_LCD_BITS_PIXEL              32
#define INPUT_PIXEL_BYTES                   4
#define OUTPUT_PIXEL_BYTES                  3
#define RED                                 0xf800



typedef struct _ICNA3311_CMD_DESC
{
    uint8_t instruction;
    uint8_t index;
    uint16_t delay;
    uint16_t wordcount;
    uint8_t  payload[ICNA3311_MAX_PARA_COUNT];
} ICNA3311_CMD_DESC;


void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);
void rtk_lcd_hal_init(void);
void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* _ICNA3311_368448_QSPI_H_ */
