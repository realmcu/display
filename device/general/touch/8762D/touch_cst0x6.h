/**
*****************************************************************************************
*     Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.

*
*     SPDX-License-Identifier: Apache-2.0
*****************************************************************************************
* @file    touch_cst0x6.h
* @brief   This file provides CST0X6 touch driver functions
* @author
* @date
* @version v1.0
* *************************************************************************************
*/

#ifndef __TOUCH_CST0X6_H
#define __TOUCH_CST0X6_H

#include "board.h"
#include "stdint.h"
#include "stdbool.h"

#define FTS_REG_CHIP_ID           0xA3     //chip ID 


void rtk_touch_hal_init(void);
bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing);
void rtk_touch_hal_int_config(bool enable);
void rtk_touch_hal_set_indicate(void (*indicate)(void *));

#endif // __TOUCH_CST0X6_H

