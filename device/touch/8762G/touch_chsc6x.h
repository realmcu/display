/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TOUCH_CHSC6X_H
#define __TOUCH_CHSC6X_H


#include "board.h"
#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif


#define CHSC6X_I2C_ID      0x2e

#define TOUCH_CHSC6X_RST             P1_2
#define TOUCH_CHSC6X_INT             P3_5
#define TOUCH_CHSC6X_SCL             P3_3
#define TOUCH_CHSC6X_SDA             P3_2

bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing);


#ifdef __cplusplus
}
#endif

#endif /* __TOUCH_CHSC6X_H */
