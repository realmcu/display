/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2020-08-04     howie  first version
 */

#ifndef __TOUCH_CHSC6417EU24_8772F_368448_H
#define __TOUCH_CHSC6417EU24_8772F_368448_H


#include "board.h"
#include "stdint.h"
#include "stdbool.h"


#ifdef __cplusplus
extern "C" {
#endif

#define TOUCH_CHSC6417EU24_RST                                     P12_2
#define TOUCH_CHSC6417EU24_INT                                     P12_4
#define TOUCH_CHSC6417EU24_SCL                                     P10_5
#define TOUCH_CHSC6417EU24_SDA                                     P10_3


#define TOUCH_CHSC6417EU24_ADDR                                    0x2E


void rtk_touch_hal_init(void);
bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing);

#ifdef __cplusplus
}
#endif

#endif /* __TOUCH_CHSC6417EU24_8772F_368448_H */
