/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef TOUCH_CST820_H
#define TOUCH_CST820_H

#include "rtl876x.h"

#define TOUCH_I2C_SCL                             P2_3
#define TOUCH_I2C_SDA                             P2_4
#define TOUCH_I2C_BUS                             I2C1
#define TOUCH_I2C_FUNC_SCL                        I2C1_CLK
#define TOUCH_I2C_FUNC_SDA                        I2C1_DAT
#define TOUCH_I2C_APBPeriph                       APBPeriph_I2C1
#define TOUCH_I2C_APBClock                        APBPeriph_I2C1_CLOCK

#define TOUCH_INT_APBPeriph                       APBPeriph_GPIOB
#define TOUCH_INT_APBPeriph_CLK                   APBPeriph_GPIOB_CLOCK
#define TOUCH_INT_GROUP                           GPIOB
#define TOUCH_INT                                 P3_5
#define TOUCH_INT_HANDLER                         GPIOB25_Handler
#define TOUCH_INT_IRQ                             GPIOB25_IRQn
#define TOUCH_INT_VECTORn                         GPIOB25_VECTORn

#define TOUCH_RST                                 P3_4
#define TOUCH_SLAVE_ADDRESS                       0x15

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t x_start;
    uint16_t y_start;
    uint32_t timestamp_ms_start;
    uint32_t timestamp_ms_pressing;
    uint16_t count_pressing;
    bool is_press;
} TOUCH_DATA;

bool touch_cst820_read_all(uint16_t *x, uint16_t *y, bool *pressing);
bool touch_cst820_read_chip_id(void);

#endif // TOUCH_CST820_H