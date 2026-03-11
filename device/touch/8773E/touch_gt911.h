/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TOUCH_GT911_H
#define __TOUCH_GT911_H


#include "board.h"
#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif


#define TOUCH_GT911_ADDR                                      0x14

#define GT_CTRL_REG     0X8040
#define GT_CFGS_REG     0X8047
#define GT_CHECK_REG    0X80FF
#define GT_PID_REG      0X8140

#define GT_GSTID_REG    0X814E
#define GT_TP1_REG      0X8150
#define GT_TP2_REG      0X8158
#define GT_TP3_REG      0X8160
#define GT_TP4_REG      0X8168
#define GT_TP5_REG      0X8170
#define TOUCH_GT911_RST                                          P2_1
#define TOUCH_GT911_INT                                          ADC_2
#define TOUCH_GT911_SCL                                          ADC_0
#define TOUCH_GT911_SDA                                          ADC_1



#define TOUCH_SCREEN_TYPE   1

#if (TOUCH_SCREEN_TYPE  == 1)
#define TOUCH_SCREEN_WIDTH 480
#define TOUCH_SCREEN_HIGHT 480

#elif (TOUCH_SCREEN_TYPE  == 2)
#define TOUCH_SCREEN_WIDTH 480
#define TOUCH_SCREEN_HIGHT 272


#endif





bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing);
#ifdef __cplusplus
}
#endif

#endif /* __TOUCH_GT911_H */
