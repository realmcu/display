/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _SH8601Z_368448_QSPI_H_
#define _SH8601Z_368448_QSPI_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"

#define SH8601Z_MAX_PARA_COUNT                          (300)

#define TE_VALID                            1
#define SH8601Z_LCD_WIDTH                   368
#define SH8601Z_LCD_HIGHT                   448
#define SH8601Z_LCD_BITS_PIXEL              32
#define INPUT_PIXEL_BYTES                   4
#define OUTPUT_PIXEL_BYTES                  3
#define RED                                 0xf800



typedef struct _SH8601Z_CMD_DESC
{
    uint8_t instruction;
    uint8_t index;
    uint16_t delay;
    uint16_t wordcount;
    uint8_t  payload[SH8601Z_MAX_PARA_COUNT];
} SH8601Z_CMD_DESC;


void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);
void rtk_lcd_hal_init(void);
void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* _SH8601Z_368448_QSPI_H_ */
