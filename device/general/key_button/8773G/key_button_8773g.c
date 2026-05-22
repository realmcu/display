/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include "board.h"
#include "rtl876x_pinmux.h"
#include "key_button_8773g.h"
#include "vector_table.h"
//#include "app_dlps.h"
#include "os_timer.h"
#include "os_sched.h"

static bool key_current_state_raw = SET;
static void *debounce_timer = NULL;
static T_GPIO_KEY gpio_key;

static void debounce_timeout(void *pxTimer)
{
    if (key_current_state_raw != GPIOx_ReadInputDataBit(GPIO_BUTTON_GROUP,
                                                        GPIO_GetPin(GPIO_BUTTON_KEY)))
    {
        GPIOx_MaskINTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), DISABLE);
        GPIOx_INTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), ENABLE);
        APP_PRINT_ERROR0("GPIO button debounce Fail!");
        return;
    }

    /* this means button released */
    if (key_current_state_raw == SET)
    {
        /* Change GPIO Interrupt Polarity, Enable Interrupt */
        // GPIO_BUTTON_GROUP->GPIO_INT_POL &= ~ GPIO_GetPin(GPIO_BUTTON_KEY);
        GPIOx_IntPolaritySet(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), GPIO_INT_POLARITY_ACTIVE_LOW);
        GPIOx_MaskINTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), DISABLE);
        GPIOx_INTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), ENABLE);

        gpio_key.current_state = GPIO_KEY_RELEASED;
        gpio_key.key_id = GPIO_BUTTON_KEY;
        gpio_key.release_timestamp = os_sys_time_get();

//        app_dlps_enable(APP_DLPS_ENTER_CHECK_BUTTON);
    }
    /* this means button pressed */
    else
    {
        /* Change GPIO Interrupt Polarity, Enable Interrupt */
        // GPIO_BUTTON_GROUP->GPIO_INT_POL |= GPIO_GetPin(GPIO_BUTTON_KEY);
        GPIOx_IntPolaritySet(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY),
                             GPIO_INT_POLARITY_ACTIVE_HIGH);
        GPIOx_MaskINTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), DISABLE);
        GPIOx_INTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), ENABLE);

        gpio_key.current_state = GPIO_KEY_PRESSED;
        gpio_key.key_id = GPIO_BUTTON_KEY;
        gpio_key.press_timestamp = os_sys_time_get();
    }

    /*check if lcd has powered off, then send msg to gui task*/
//    if (!app_dlps_check_enter_bits(APP_DLPS_ENTER_CHECK_DISPLAY | APP_DLPS_ENTER_CHECK_BUTTON))
//    {
//        APP_PRINT_INFO1("[Key Button][key_wakeup_flag] key_current_state_raw = %d", key_current_state_raw);
//        gui_wake_up_by_button();
//    }
}

void gpio_button_interrupt_enable(void)
{
    GPIOx_MaskINTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), ENABLE);
    GPIOx_INTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), ENABLE);
    GPIOx_ClearINTPendingBit(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY));
    GPIOx_MaskINTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), DISABLE);
}
void gpio_button_interrupt_disable(void)
{
    GPIOx_INTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), DISABLE);
    GPIOx_MaskINTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), ENABLE);
    GPIOx_ClearINTPendingBit(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY));
}

void gpio_button_pad_init(void)
{
    Pinmux_Config(GPIO_BUTTON_KEY, DWGPIO);
    Pad_Config(GPIO_BUTTON_KEY, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
}
void gpio_button_enter_dlps(void)
{
    Pad_Config(GPIO_BUTTON_KEY, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
}
void gpio_button_exit_dlps(void)
{
    Pad_Config(GPIO_BUTTON_KEY, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
}

static void GPIO_BUTTON_HANDLER(void)
{
    gpio_button_interrupt_disable();

    key_current_state_raw = GPIOx_ReadInputDataBit(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY));
    //APP_PRINT_INFO1("[Key Button] key_current_state_raw = %d", key_current_state_raw);

//    app_dlps_disable(APP_DLPS_ENTER_CHECK_BUTTON);
    if (os_timer_start(&debounce_timer) == false)
    {
        GPIOx_INTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), ENABLE);
        GPIOx_MaskINTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), DISABLE);
//        app_dlps_enable(APP_DLPS_ENTER_CHECK_BUTTON);
    }

}

void gpio_button_init(void)
{
    extern void gpio_int_init(void);
    gpio_int_init();
    gpio_button_pad_init();
    RCC_PeriphClockCmd(GPIO_BUTTON_APBPeriph,  GPIO_BUTTON_APBPeriph_CLK,  ENABLE);

    RamVectorTableUpdate(GPIOA23_VECTORn, GPIO_BUTTON_HANDLER);

    GPIO_InitTypeDef GPIO_Param;
    GPIO_StructInit(&GPIO_Param);
    GPIO_Param.GPIO_PinBit = GPIO_GetPin(GPIO_BUTTON_KEY);
    GPIO_Param.GPIO_Mode = GPIO_Mode_IN;
    GPIO_Param.GPIO_ITCmd = ENABLE;
    GPIO_Param.GPIO_ITTrigger = GPIO_INT_Trigger_LEVEL;
    GPIO_Param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
    GPIOx_Init(GPIO_BUTTON_GROUP, &GPIO_Param);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = GPIO_A16_23_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    os_timer_create(&debounce_timer, "debounce timer", 1, 20, false, debounce_timeout);

    gpio_button_interrupt_enable();

    System_WakeUpPinEnable(GPIO_BUTTON_KEY, PAD_WAKEUP_POL_LOW);
}



T_GPIO_KEY gpio_button_read_key(void)
{
    return gpio_key;
}
