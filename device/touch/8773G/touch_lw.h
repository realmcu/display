/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _TOUCH_LW_H_
#define _TOUCH_LW_H_


#include "rtl876x_gpio.h"
#include "rtl876x_rcc.h"
#include "rtl876x_tim.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "vector_table.h"
#include "rtl876x.h"


#define TOUCH_I2C_SCL                             P4_2
#define TOUCH_I2C_SDA                             P4_3
#define TOUCH_I2C_BUS                             I2C1
#define TOUCH_I2C_FUNC_SCL                        I2C1_CLK
#define TOUCH_I2C_FUNC_SDA                        I2C1_DAT
#define TOUCH_I2C_APBPeriph                       APBPeriph_I2C1
#define TOUCH_I2C_APBClock                        APBPeriph_I2C1_CLOCK
#define TOUCH_SLAVE_ADDRESS                       (0x15)//(0x14)

#define TOUCH_INT_APBPeriph                       APBPeriph_GPIOB
#define TOUCH_INT_APBPeriph_CLK                   APBPeriph_GPIOB_CLOCK
#define TOUCH_INT_GROUP                           GPIOB
#define TOUCH_INT                                 P4_1
//#define TOUCH_INT_HANDLER                         GPIOB8_Handler  //GPIOA2_Handler
#define TOUCH_INT_IRQ                             GPIO40_IRQn      //GPIO2_IRQn
#define TOUCH_INT_VECTORn                         GPIOB8_VECTORn  //GPIOA2_VECTORn

#define TOUCH_RST                                 P4_0



void touch_driver_init(void);
bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing);
void touch_gesture_enter_dlps(void);
void touch_gesture_exit_dlps(void);


#endif // _TOUCH_LW_H_
