/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __KEY_BUTTON_8773E_H
#define __KEY_BUTTON_8773E_H

#include "rtl876x_gpio.h"
#include "rtl876x_rcc.h"
#include "rtl876x_tim.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x.h"
#include "trace.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_BUTTON_APBPeriph                       APBPeriph_GPIOA
#define GPIO_BUTTON_APBPeriph_CLK                   APBPeriph_GPIOA_CLOCK
#define GPIO_BUTTON_GROUP                           GPIOA
#define GPIO_BUTTON_KEY                             P3_0




typedef enum
{
    GPIO_KEY_INVALID,
    GPIO_KEY_PRESSED,
    GPIO_KEY_RELEASED,
} T_GPIO_KEY_STATE;

typedef struct
{
    uint32_t key_id;
    T_GPIO_KEY_STATE current_state;
    uint32_t press_timestamp;
    uint32_t release_timestamp;
} T_GPIO_KEY;

void gpio_button_init(void);
void gpio_button_enter_dlps(void);
void gpio_button_exit_dlps(void);
T_GPIO_KEY gpio_button_read_key(void);


#ifdef __cplusplus
}
#endif

#endif /* __KEY_BUTTON_8773_H */
