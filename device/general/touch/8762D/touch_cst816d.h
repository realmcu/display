/**
*****************************************************************************************
*     Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.

*
*     SPDX-License-Identifier: Apache-2.0
*****************************************************************************************
* @file    touch_cst816d.h
* @brief   This file provides CST816D touch driver functions
* @author
* @date
* @version v1.0
* *************************************************************************************
*/

#ifndef __TOUCH_CST816D_H
#define __TOUCH_CST816D_H

#include "board.h"
#include "stdint.h"
#include "stdbool.h"
#include "menu_config.h"


#if USING_8762CK_VD_DEVICE
#define TOUCH_CST816D_RST                                     P2_2
#define TOUCH_CST816D_INT                                     P2_1
#define TOUCH_CST816D_SCL                                     H_2
#define TOUCH_CST816D_SDA                                     H_1
#else
#define TOUCH_CST816D_RST                                     P2_3
#define TOUCH_CST816D_INT                                     P0_4
#define TOUCH_CST816D_SCL                                     P0_0
#define TOUCH_CST816D_SDA                                     P0_2
#endif

#define TOUCH_WORK_MODE_SLAVE_ADDRESS             0x15
#define TOUCH_UPGRADE_MODE_SLAVE_ADDRESS          0x6a

void rtk_touch_hal_init(void);
bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing);
void rtk_touch_hal_int_config(bool enable);
void rtk_touch_hal_set_indicate(void (*indicate)(void *));

#endif // __TOUCH_CST816T_H

