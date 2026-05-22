/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2020-08-04     howie  first version
 */


#include "rtl_gpio.h"
#include "rtl_rcc.h"
#include "drv_gpio.h"
#include "key_button_8772f.h"
#include "string.h"
#include "trace.h"
#include "utils.h"
#include "board.h"


void (*key_button_wakeup_indicate)(void) = NULL;

static void button_cb(void *this)
{
    DBG_DIRECT("button_cb");
}


void rtk_key_button_hal_set_indicate(void (*indicate)(void *))
{
    drv_pin_mode(KEY_BUTTON, PIN_MODE_INPUT);
    drv_pin_attach_irq(KEY_BUTTON, PIN_IRQ_MODE_RISING_FALLING, indicate,
                       NULL);
    drv_pin_irq_enable(KEY_BUTTON, PIN_IRQ_ENABLE);
}

void rtk_key_button_hal_int_config(bool enable)
{
    if (enable == true)
    {
        drv_pin_irq_enable(KEY_BUTTON, PIN_IRQ_ENABLE);
    }
    else
    {
        drv_pin_irq_enable(KEY_BUTTON, PIN_IRQ_DISABLE);
    }
}


static bool key_button_enter_dlps(void)
{
    Pad_Config(KEY_BUTTON, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    System_WakeUpPinEnable(KEY_BUTTON, PAD_WAKEUP_POL_LOW, PAD_WAKEUP_DEB_DISABLE);
    return false;
}

static bool key_button_exit_dlps(void)
{
    Pad_Config(KEY_BUTTON, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    return false;
}

static bool key_button_allowed_enter_dlps_check(void)
{
    return true;
}

static bool key_button_system_wakeup_dlps_check(void)
{
    if (System_WakeUpInterruptValue(KEY_BUTTON) == SET)
    {
        Pad_ClearWakeupINTPendingBit(KEY_BUTTON);
        System_WakeUpPinDisable(KEY_BUTTON);
        DBG_DIRECT("key button Wake up");
        if (key_button_wakeup_indicate != NULL)
        {
            key_button_wakeup_indicate();
        }
        return true;
    }
    return false;
}

void drv_key_button_dlps_init(void)
{
    System_WakeUpPinEnable(KEY_BUTTON, PAD_WAKEUP_POL_LOW, PAD_WAKEUP_DEB_DISABLE);
#ifdef RTK_HAL_DLPS
    drv_dlps_exit_cbacks_register("key", key_button_exit_dlps);
    drv_dlps_enter_cbacks_register("key", key_button_enter_dlps);
    drv_dlps_wakeup_cbacks_register("key", key_button_system_wakeup_dlps_check);
    drv_dlps_check_cbacks_register("key", key_button_allowed_enter_dlps_check);
#endif
}

void drv_key_button_set_wakeup_indicate(void (*wakeup_ind))
{
    key_button_wakeup_indicate = wakeup_ind;
}

bool drv_key_button_read(bool *pressing)
{
    uint8_t value = drv_pin_read(KEY_BUTTON);

    if (value == 1)
    {
        *pressing = false;
    }
    else
    {
        *pressing = true;
    }

    return true;
}

void rtk_key_button_hal_init(void)
{
    drv_pin_mode(KEY_BUTTON, PIN_MODE_INPUT);

    drv_key_button_dlps_init();

}

