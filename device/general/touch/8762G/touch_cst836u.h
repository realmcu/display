/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2020-08-04     howie  first version
 */

#ifndef __TOUCH_CST836U_H
#define __TOUCH_CST836U_H


#include "board.h"
#include "stdint.h"
#include "stdbool.h"


#ifdef __cplusplus
extern "C" {
#endif

#define TOUCH_CST836U_RST                                     P2_2
#define TOUCH_CST836U_INT                                     P2_3
#define TOUCH_CST836U_SCL                                     P2_4
#define TOUCH_CST836U_SDA                                     P2_5

#define TOUCH_CST836U_ADDR                                    0x15
#define TOUCH_CST836U_CHIP_ID                                 0xAA     //chip ID 


void rtk_touch_hal_init(void);
bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing);

#ifdef __cplusplus
}
#endif

#endif /* __TOUCH_816S_8772F_454_H */
