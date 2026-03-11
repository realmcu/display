/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "board.h"
#include "rtl876x_pinmux.h"
#include "knob_switch.h"
#include "vector_table.h"
//#include "app_dlps.h"
#include "gui_api.h"
#include "os_timer.h"
#include "os_sched.h"

static bool key_current_state_raw = SET;
static bool key_1_current_state_raw = SET;
static void *debounce_timer = NULL;
static gui_wheel_port_data_t gpio_knob;
static T_GPIO_WHEEL gpio_key;

extern uint32_t dlps_bitmap;

static T_KNOB_STATE knob_state = KNOB_IDLE;
#define MAX_PULS_TIME  500 // ms
static uint32_t knob_raise_time;

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
    if (key_current_state_raw != GPIOx_ReadInputDataBit(GPIO_KNOB_0_GROUP,
                                                        GPIO_GetPin(GPIO_KNOB_0_PIN)))
    {
        GPIOx_MaskINTConfig(GPIO_KNOB_0_GROUP, GPIO_GetPin(GPIO_KNOB_0_PIN), DISABLE);
        GPIOx_INTConfig(GPIO_KNOB_0_GROUP, GPIO_GetPin(GPIO_KNOB_0_PIN), ENABLE);
        APP_PRINT_ERROR0("GPIO button debounce Fail!");
        return;
    }

    /* this means button released */
    if (key_current_state_raw == SET)
    {
        /* Change GPIO Interrupt Polarity, Enable Interrupt */
        GPIO_KNOB_0_GROUP->INTPOLARITY &= ~ GPIO_GetPin(GPIO_KNOB_0_PIN);
        GPIOx_MaskINTConfig(GPIO_KNOB_0_GROUP, GPIO_GetPin(GPIO_KNOB_0_PIN), DISABLE);
        GPIOx_INTConfig(GPIO_KNOB_0_GROUP, GPIO_GetPin(GPIO_KNOB_0_PIN), ENABLE);

        gpio_key.current_state = GPIO_WHEEL_RELEASED;
        gpio_key.key_id = GPIO_KNOB_0_PIN;
        gpio_key.release_timestamp = os_sys_time_get();

//        app_dlps_enable(APP_DLPS_ENTER_CHECK_BUTTON);
    }
    /* this means button pressed */
    else
    {
        /* Change GPIO Interrupt Polarity, Enable Interrupt */
        GPIO_KNOB_0_GROUP->INTPOLARITY |= GPIO_GetPin(GPIO_KNOB_0_PIN);
        GPIOx_MaskINTConfig(GPIO_KNOB_0_GROUP, GPIO_GetPin(GPIO_KNOB_0_PIN), DISABLE);
        GPIOx_INTConfig(GPIO_KNOB_0_GROUP, GPIO_GetPin(GPIO_KNOB_0_PIN), ENABLE);

        gpio_key.current_state = GPIO_WHEEL_PRESSED;
        gpio_key.key_id = GPIO_KNOB_0_PIN;
        gpio_key.press_timestamp = os_sys_time_get();
    }

    /*check if lcd has powered off, then send msg to gui task*/
//    if (!app_dlps_check_enter_bits(APP_DLPS_ENTER_CHECK_DISPLAY | APP_DLPS_ENTER_CHECK_BUTTON))
//    {
//        APP_PRINT_INFO1("[Key Button][key_wakeup_flag] key_current_state_raw = %d", key_current_state_raw);
//        gui_wake_up_by_button();
//    }
}

static void gpio_knob_interrupt_enable(void)
{
    GPIOx_MaskINTConfig(GPIO_KNOB_0_GROUP, GPIO_GetPin(GPIO_KNOB_0_PIN), ENABLE);
    GPIOx_INTConfig(GPIO_KNOB_0_GROUP, GPIO_GetPin(GPIO_KNOB_0_PIN), ENABLE);
    GPIOx_ClearINTPendingBit(GPIO_KNOB_0_GROUP, GPIO_GetPin(GPIO_KNOB_0_PIN));
    GPIOx_MaskINTConfig(GPIO_KNOB_0_GROUP, GPIO_GetPin(GPIO_KNOB_0_PIN), DISABLE);

    GPIOx_MaskINTConfig(GPIO_KNOB_1_GROUP, GPIO_GetPin(GPIO_KNOB_1_PIN), ENABLE);
    GPIOx_INTConfig(GPIO_KNOB_1_GROUP, GPIO_GetPin(GPIO_KNOB_1_PIN), ENABLE);
    GPIOx_ClearINTPendingBit(GPIO_KNOB_1_GROUP, GPIO_GetPin(GPIO_KNOB_1_PIN));
    GPIOx_MaskINTConfig(GPIO_KNOB_1_GROUP, GPIO_GetPin(GPIO_KNOB_1_PIN), DISABLE);
}
static void gpio_knob_interrupt_disable(void)
{
    GPIOx_INTConfig(GPIO_KNOB_0_GROUP, GPIO_GetPin(GPIO_KNOB_0_PIN), DISABLE);
    GPIOx_MaskINTConfig(GPIO_KNOB_0_GROUP, GPIO_GetPin(GPIO_KNOB_0_PIN), ENABLE);
    GPIOx_ClearINTPendingBit(GPIO_KNOB_0_GROUP, GPIO_GetPin(GPIO_KNOB_0_PIN));
}

static void gpio_knob_1_interrupt_disable(void)
{
    GPIOx_INTConfig(GPIO_KNOB_1_GROUP, GPIO_GetPin(GPIO_KNOB_1_PIN), DISABLE);
    GPIOx_MaskINTConfig(GPIO_KNOB_1_GROUP, GPIO_GetPin(GPIO_KNOB_1_PIN), ENABLE);
    GPIOx_ClearINTPendingBit(GPIO_KNOB_1_GROUP, GPIO_GetPin(GPIO_KNOB_1_PIN));
}

static void gpio_knob_pad_init(void)
{
    Pinmux_Config(GPIO_KNOB_0_PIN, DWGPIO);
    Pad_Config(GPIO_KNOB_0_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE,
               PAD_OUT_LOW);

    Pinmux_Config(GPIO_KNOB_1_PIN, DWGPIO);
    Pad_Config(GPIO_KNOB_1_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
}
// void gpio_knob_enter_dlps(void)
// {
//     Pad_Config(GPIO_KNOB_0_PIN, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
// }
// void gpio_knob_exit_dlps(void)
// {
//     Pad_Config(GPIO_KNOB_0_PIN, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
//                PAD_OUT_LOW);
// }

void gpio_knob_init(void)
{
    gpio_knob_pad_init();
    RCC_PeriphClockCmd(GPIO_KNOB_0_APBPeriph,  GPIO_KNOB_0_APBPeriph_CLK,  ENABLE);

    RamVectorTableUpdate(GPIO_KNOB_0_VECTORn, GPIO_KNOB_0_HANDLER);
    GPIO_InitTypeDef GPIO_Param;
    GPIO_StructInit(&GPIO_Param);
    GPIO_Param.GPIO_PinBit = GPIO_GetPin(GPIO_KNOB_0_PIN);
    GPIO_Param.GPIO_Mode = GPIO_Mode_IN;
    GPIO_Param.GPIO_ITCmd = ENABLE;
    GPIO_Param.GPIO_ITTrigger = GPIO_INT_Trigger_LEVEL;
    GPIO_Param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
    GPIOx_Init(GPIO_KNOB_0_GROUP, &GPIO_Param);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = GPIO_KNOB_0_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    {
        RCC_PeriphClockCmd(GPIO_KNOB_1_APBPeriph,  GPIO_KNOB_1_APBPeriph_CLK,  ENABLE);

        RamVectorTableUpdate(GPIO_KNOB_1_VECTORn, GPIO_KNOB_1_HANDLER);
        GPIO_InitTypeDef GPIO_Param;
        GPIO_StructInit(&GPIO_Param);
        GPIO_Param.GPIO_PinBit = GPIO_GetPin(GPIO_KNOB_1_PIN);
        GPIO_Param.GPIO_Mode = GPIO_Mode_IN;
        GPIO_Param.GPIO_ITCmd = ENABLE;
        GPIO_Param.GPIO_ITTrigger = GPIO_INT_Trigger_LEVEL;
        GPIO_Param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
        GPIOx_Init(GPIO_KNOB_1_GROUP, &GPIO_Param);

        NVIC_InitTypeDef NVIC_InitStruct;
        NVIC_InitStruct.NVIC_IRQChannel = GPIO_KNOB_1_IRQ;
        NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
        NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStruct);
    }


    gpio_knob_interrupt_enable();

    System_WakeUpPinEnable(GPIO_KNOB_0_PIN, PAD_WAKEUP_POL_LOW);
}

void GPIO_KNOB_0_HANDLER(void)
{
    gpio_knob_interrupt_disable();

    key_current_state_raw = GPIOx_ReadInputDataBit(GPIO_KNOB_0_GROUP, GPIO_GetPin(GPIO_KNOB_0_PIN));

    // DBG_DIRECT("0 GPIO INT %d %d", key_current_state_raw, key_1_current_state_raw);

    if (key_current_state_raw == RESET)
    {
        uint32_t time = os_sys_time_get();
        uint32_t pulse = time - knob_raise_time;
        if (knob_state == KNOB_0_UP)
        {
            DBG_DIRECT("0 scroll ");
            gpio_knob.delta ++;
            gpio_knob.event = 3;
            gpio_knob.timestamp_ms = time;
        }
    }
#if 0
    if (key_current_state_raw == RESET)
    {
        uint32_t time = os_sys_time_get();
        uint32_t pulse = time - knob_raise_time;
        if (knob_state == KNOB_0_UP && (pulse < MAX_PULS_TIME) && (pulse > 0))
        {
            DBG_DIRECT("0 scroll %d", pulse);
            gpio_knob.delta ++;
            gpio_knob.event = 3;
            gpio_knob.timestamp_ms = time;
        }
        else
        {
            DBG_DIRECT("0 p %d", pulse);
        }
        knob_state = KNOB_IDLE;
    }
    else
    {
        knob_state = KNOB_0_UP;
        knob_raise_time = os_sys_time_get();
        DBG_DIRECT("0 up %d", knob_raise_time);
    }
#endif

    if (key_current_state_raw == RESET)
    {
        GPIO_KNOB_0_GROUP->INTPOLARITY |= GPIO_GetPin(GPIO_KNOB_0_PIN);
    }
    else
    {
        GPIO_KNOB_0_GROUP->INTPOLARITY &= ~ GPIO_GetPin(GPIO_KNOB_0_PIN);
    }

    GPIOx_MaskINTConfig(GPIO_KNOB_0_GROUP, GPIO_GetPin(GPIO_KNOB_0_PIN), DISABLE);
    GPIOx_INTConfig(GPIO_KNOB_0_GROUP, GPIO_GetPin(GPIO_KNOB_0_PIN), ENABLE);
}
void GPIO_KNOB_1_HANDLER(void)
{
    gpio_knob_1_interrupt_disable();

    key_1_current_state_raw = GPIOx_ReadInputDataBit(GPIO_KNOB_1_GROUP, GPIO_GetPin(GPIO_KNOB_1_PIN));

    // DBG_DIRECT("1 GPIO INT %d %d", key_current_state_raw, key_1_current_state_raw);

    if (key_1_current_state_raw == RESET)
    {
        uint32_t time = os_sys_time_get();
        uint32_t pulse = time - knob_raise_time;
        if (knob_state == KNOB_1_UP && (pulse < MAX_PULS_TIME) && (pulse > 0))
        {
            DBG_DIRECT("1 scroll %d", pulse);
            gpio_knob.delta --;
            gpio_knob.event = 3;
            gpio_knob.timestamp_ms = time;
        }
        else
        {
            DBG_DIRECT("1 p %d", pulse);
        }
        knob_state = KNOB_IDLE;
    }
    else
    {
        knob_state = KNOB_1_UP;
        knob_raise_time = os_sys_time_get();
        DBG_DIRECT("1 up %d", knob_raise_time);
    }


    if (key_1_current_state_raw == RESET)
    {
        // high
        GPIO_KNOB_1_GROUP->INTPOLARITY |= GPIO_GetPin(GPIO_KNOB_1_PIN);
    }
    else
    {
        // low
        GPIO_KNOB_1_GROUP->INTPOLARITY &= ~ GPIO_GetPin(GPIO_KNOB_1_PIN);
    }
    GPIOx_MaskINTConfig(GPIO_KNOB_1_GROUP, GPIO_GetPin(GPIO_KNOB_1_PIN), DISABLE);
    GPIOx_INTConfig(GPIO_KNOB_1_GROUP, GPIO_GetPin(GPIO_KNOB_1_PIN), ENABLE);
}

void gpio_knob_read(T_WHEEL_DATA *data)
{
    memcpy(data, &gpio_knob, sizeof(T_WHEEL_DATA));
    memset(&gpio_knob, 0, sizeof(T_WHEEL_DATA));
}
