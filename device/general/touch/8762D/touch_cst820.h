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

#ifndef __TOUCH_CST820_H
#define __TOUCH_CST820_H

#include "stdint.h"
#include "stdbool.h"



#define TOUCH_CST820_RST                                     P1_0
#define TOUCH_CST820_INT                                     P0_0
#define TOUCH_CST820_SCL                                     P0_1
#define TOUCH_CST820_SDA                                     P0_2

#define TOUCH_I2C_BUS                                   I2C1
#define TOUCH_I2C_FUNC_SCL                              I2C1_CLK
#define TOUCH_I2C_FUNC_SDA                              I2C1_DAT

#define TOUCH_WORK_MODE_SLAVE_ADDRESS             0x15
#define TOUCH_UPGRADE_MODE_SLAVE_ADDRESS          0x6a

void rtk_touch_hal_init(void);
bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing);
void rtk_touch_hal_int_config(bool enable);
void rtk_touch_hal_set_indicate(void (*indicate)(void *));

#endif // __TOUCH_CST816T_H

