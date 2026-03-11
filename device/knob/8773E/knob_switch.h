/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __KNOB__SWITCH_H__
#define __KNOB__SWITCH_H__

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

// #define P9_6               85        /*!< GPIOB6 */
// #define P6_0               43        /*!< GPIOA25 */



#define GPIO_KNOB_0_APBPeriph                       APBPeriph_GPIOA
#define GPIO_KNOB_0_APBPeriph_CLK                   APBPeriph_GPIOA_CLOCK
#define GPIO_KNOB_0_GROUP                           GPIOA
#define GPIO_KNOB_0_PIN                             P6_0
#define GPIO_KNOB_0_HANDLER                         GPIOA25_Handler
#define GPIO_KNOB_0_IRQ                             GPIO25_IRQn
#define GPIO_KNOB_0_VECTORn                         GPIOA25_VECTORn

#define GPIO_KNOB_1_APBPeriph                       APBPeriph_GPIOB
#define GPIO_KNOB_1_APBPeriph_CLK                   APBPeriph_GPIOB_CLOCK
#define GPIO_KNOB_1_GROUP                           GPIOB
#define GPIO_KNOB_1_PIN                             P9_6
#define GPIO_KNOB_1_HANDLER                         GPIOB6_Handler
#define GPIO_KNOB_1_IRQ                             GPIO38_IRQn
#define GPIO_KNOB_1_VECTORn                         GPIOB6_VECTORn



typedef enum
{
    KNOB_IDLE,
    KNOB_0_UP,
    KNOB_1_UP,
} T_KNOB_STATE;

typedef struct
{
    uint8_t  event;//0:no event//1:button down//2:button up//3:scroll
    int16_t  delta;//positive away from the user and negative toward the user
    uint32_t timestamp_ms;             /* The timestamp when the data was received */
} T_WHEEL_DATA;


typedef enum
{
    GPIO_WHEEL_INVALID,
    GPIO_WHEEL_PRESSED,
    GPIO_WHEEL_RELEASED,
} T_GPIO_WHEEL_STATE;

typedef struct
{
    uint32_t key_id;
    T_GPIO_WHEEL_STATE current_state;
    uint32_t press_timestamp;
    uint32_t release_timestamp;
} T_GPIO_WHEEL;

// void gpio_knob_enter_dlps(void);
// void gpio_knob_exit_dlps(void);
void gpio_knob_init(void);
void gpio_knob_read(T_WHEEL_DATA *data);


#ifdef __cplusplus
}
#endif

#endif /* __KEY_BUTTON_8773_H */
