/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "board.h"
#include "rtl876x_pinmux.h"
#include "knob_encoder_pulse.h"
#include "vector_table.h"
//#include "app_dlps.h"
#include "gui_api.h"
#include "os_timer.h"
#include "os_sched.h"

static bool key_current_state_raw = SET;
static bool key_1_current_state_raw = SET;
static void *debounce_timer = NULL;
static gui_wheel_port_data_t gpio_wheel;
static T_GPIO_WHEEL gpio_key;

extern uint32_t dlps_bitmap;

volatile static T_ENCODER_STATE encoder_state = ENCODER_IDLE;



static void gui_wake_up_by_button(void)
{
    /*send msg to display on*/
    gui_msg_t p_msg;
//    p_msg.event = GUI_EVENT_DISPLAY_ON;
    extern bool gui_send_msg_to_server(gui_msg_t *msg);
    gui_send_msg_to_server(&p_msg);
}

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
        GPIO_BUTTON_GROUP->INTPOLARITY &= ~ GPIO_GetPin(GPIO_BUTTON_KEY);
        GPIOx_MaskINTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), DISABLE);
        GPIOx_INTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), ENABLE);

        gpio_key.current_state = GPIO_WHEEL_RELEASED;
        gpio_key.key_id = GPIO_BUTTON_KEY;
        gpio_key.release_timestamp = os_sys_time_get();

//        app_dlps_enable(APP_DLPS_ENTER_CHECK_BUTTON);
    }
    /* this means button pressed */
    else
    {
        /* Change GPIO Interrupt Polarity, Enable Interrupt */
        GPIO_BUTTON_GROUP->INTPOLARITY |= GPIO_GetPin(GPIO_BUTTON_KEY);
        GPIOx_MaskINTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), DISABLE);
        GPIOx_INTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), ENABLE);

        gpio_key.current_state = GPIO_WHEEL_PRESSED;
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

static void gpio_button_interrupt_enable(void)
{
    GPIOx_MaskINTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), ENABLE);
    GPIOx_INTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), ENABLE);
    GPIOx_ClearINTPendingBit(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY));
    GPIOx_MaskINTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), DISABLE);

    GPIOx_MaskINTConfig(GPIO_BUTTON_1_GROUP, GPIO_GetPin(GPIO_BUTTON_1_KEY), ENABLE);
    GPIOx_INTConfig(GPIO_BUTTON_1_GROUP, GPIO_GetPin(GPIO_BUTTON_1_KEY), ENABLE);
    GPIOx_ClearINTPendingBit(GPIO_BUTTON_1_GROUP, GPIO_GetPin(GPIO_BUTTON_1_KEY));
    GPIOx_MaskINTConfig(GPIO_BUTTON_1_GROUP, GPIO_GetPin(GPIO_BUTTON_1_KEY), DISABLE);
}
static void gpio_button_interrupt_disable(void)
{
    GPIOx_INTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), DISABLE);
    GPIOx_MaskINTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), ENABLE);
    GPIOx_ClearINTPendingBit(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY));
}

static void gpio_button_1_interrupt_disable(void)
{
    GPIOx_INTConfig(GPIO_BUTTON_1_GROUP, GPIO_GetPin(GPIO_BUTTON_1_KEY), DISABLE);
    GPIOx_MaskINTConfig(GPIO_BUTTON_1_GROUP, GPIO_GetPin(GPIO_BUTTON_1_KEY), ENABLE);
    GPIOx_ClearINTPendingBit(GPIO_BUTTON_1_GROUP, GPIO_GetPin(GPIO_BUTTON_1_KEY));
}

static void gpio_button_pad_init(void)
{
    Pinmux_Config(GPIO_BUTTON_KEY, DWGPIO);
    Pad_Config(GPIO_BUTTON_KEY, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);

    Pinmux_Config(GPIO_BUTTON_1_KEY, DWGPIO);
    Pad_Config(GPIO_BUTTON_1_KEY, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
}
// void gpio_button_enter_dlps(void)
// {
//     Pad_Config(GPIO_BUTTON_KEY, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
// }
// void gpio_button_exit_dlps(void)
// {
//     Pad_Config(GPIO_BUTTON_KEY, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
//                PAD_OUT_LOW);
// }

void gpio_wheel_init(void)
{
    gpio_button_pad_init();
    RCC_PeriphClockCmd(GPIO_BUTTON_APBPeriph,  GPIO_BUTTON_APBPeriph_CLK,  ENABLE);

    RamVectorTableUpdate(GPIO_BUTTON_VECTORn, GPIO_BUTTON_HANDLER);
    GPIO_InitTypeDef GPIO_Param;
    GPIO_StructInit(&GPIO_Param);
    GPIO_Param.GPIO_PinBit = GPIO_GetPin(GPIO_BUTTON_KEY);
    GPIO_Param.GPIO_Mode = GPIO_Mode_IN;
    GPIO_Param.GPIO_ITCmd = ENABLE;
    GPIO_Param.GPIO_ITTrigger = GPIO_INT_Trigger_LEVEL;
    GPIO_Param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
    GPIOx_Init(GPIO_BUTTON_GROUP, &GPIO_Param);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = GPIO_BUTTON_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    {
        RCC_PeriphClockCmd(GPIO_BUTTON_1_APBPeriph,  GPIO_BUTTON_1_APBPeriph_CLK,  ENABLE);

        RamVectorTableUpdate(GPIO_BUTTON_1_VECTORn, GPIO_BUTTON_1_HANDLER);
        GPIO_InitTypeDef GPIO_Param;
        GPIO_StructInit(&GPIO_Param);
        GPIO_Param.GPIO_PinBit = GPIO_GetPin(GPIO_BUTTON_1_KEY);
        GPIO_Param.GPIO_Mode = GPIO_Mode_IN;
        GPIO_Param.GPIO_ITCmd = ENABLE;
        GPIO_Param.GPIO_ITTrigger = GPIO_INT_Trigger_LEVEL;
        GPIO_Param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
        GPIOx_Init(GPIO_BUTTON_1_GROUP, &GPIO_Param);

        NVIC_InitTypeDef NVIC_InitStruct;
        NVIC_InitStruct.NVIC_IRQChannel = GPIO_BUTTON_1_IRQ;
        NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
        NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStruct);
    }

    // os_timer_create(&debounce_timer, "debounce timer", 1, 20, false, debounce_timeout);

    gpio_button_interrupt_enable();

    System_WakeUpPinEnable(GPIO_BUTTON_KEY, PAD_WAKEUP_POL_LOW);
}

void GPIO_BUTTON_HANDLER(void)
{
    gpio_button_interrupt_disable();

    key_current_state_raw = GPIOx_ReadInputDataBit(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY));
    key_1_current_state_raw = GPIOx_ReadInputDataBit(GPIO_BUTTON_1_GROUP,
                                                     GPIO_GetPin(GPIO_BUTTON_1_KEY));

    // DBG_DIRECT("0 GPIO INT %d %d", key_current_state_raw, key_1_current_state_raw);

    if (encoder_state == ENCODER_IDLE)
    {
        if ((key_current_state_raw == RESET) && (key_1_current_state_raw == RESET))
        {
            encoder_state = ENCODER_SCROLL;
            DBG_DIRECT("0 scroll");
            gpio_wheel.delta ++;
            gpio_wheel.event = 3;
            gpio_wheel.timestamp_ms = os_sys_time_get();
        }
    }
    else
    {
        if ((key_current_state_raw == SET) && (key_1_current_state_raw == SET))
        {
            encoder_state = ENCODER_IDLE;
        }
    }


    if (key_current_state_raw == RESET)
    {
        GPIO_BUTTON_GROUP->INTPOLARITY |= GPIO_GetPin(GPIO_BUTTON_KEY);
    }
    else
    {
        GPIO_BUTTON_GROUP->INTPOLARITY &= ~ GPIO_GetPin(GPIO_BUTTON_KEY);
    }

    GPIOx_MaskINTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), DISABLE);
    GPIOx_INTConfig(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY), ENABLE);


}
void GPIO_BUTTON_1_HANDLER(void)
{
    gpio_button_1_interrupt_disable();

    key_current_state_raw = GPIOx_ReadInputDataBit(GPIO_BUTTON_GROUP, GPIO_GetPin(GPIO_BUTTON_KEY));
    key_1_current_state_raw = GPIOx_ReadInputDataBit(GPIO_BUTTON_1_GROUP,
                                                     GPIO_GetPin(GPIO_BUTTON_1_KEY));

    // DBG_DIRECT("1 GPIO INT %d %d", key_current_state_raw, key_1_current_state_raw);

    if (encoder_state == ENCODER_IDLE)
    {
        if ((key_current_state_raw == RESET) && (key_1_current_state_raw == RESET))
        {
            encoder_state = ENCODER_SCROLL;
            DBG_DIRECT("1 scroll");
            gpio_wheel.delta --;
            gpio_wheel.event = 3;
            gpio_wheel.timestamp_ms = os_sys_time_get();
        }
    }
    else
    {
        if ((key_current_state_raw == SET) && (key_1_current_state_raw == SET))
        {
            encoder_state = ENCODER_IDLE;
        }
    }


    if (key_1_current_state_raw == RESET)
    {
        // high
        GPIO_BUTTON_1_GROUP->INTPOLARITY |= GPIO_GetPin(GPIO_BUTTON_1_KEY);
    }
    else
    {
        // low
        GPIO_BUTTON_1_GROUP->INTPOLARITY &= ~ GPIO_GetPin(GPIO_BUTTON_1_KEY);
    }
    GPIOx_MaskINTConfig(GPIO_BUTTON_1_GROUP, GPIO_GetPin(GPIO_BUTTON_1_KEY), DISABLE);
    GPIOx_INTConfig(GPIO_BUTTON_1_GROUP, GPIO_GetPin(GPIO_BUTTON_1_KEY), ENABLE);


}

void gpio_wheel_read(T_WHEEL_DATA *data)
{
    memcpy(data, &gpio_wheel, sizeof(T_WHEEL_DATA));
    memset(&gpio_wheel, 0, sizeof(T_WHEEL_DATA));
}
