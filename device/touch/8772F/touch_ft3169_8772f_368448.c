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
#include "drv_i2c.h"
#include "drv_touch.h"
#include "touch_ft3169_8772f_368448.h"
#include "string.h"
#include "trace.h"
#include "utils.h"
#include "board.h"


void (*touch_wakeup_indicate)(void) = NULL;


bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing)
{
    uint8_t iic_write_buf[1] = {0x01};
    uint8_t buf[24] = {0};

    drv_i2c0_write(TOUCH_FT3169_ADDR, iic_write_buf, 1);
    platform_delay_us(100);

    drv_i2c0_read(TOUCH_FT3169_ADDR, buf, 24);

    if ((buf[1] & 0x0F) == 1)
    {
        *pressing = true;
    }
    else
    {
        *pressing = false;
    }

    *x = (((buf[2] & 0x0f) << 8) | buf[3]);
    *y = (((buf[4] & 0x0f) << 8) | buf[5]);

    return true;
}


void rtk_touch_hal_set_indicate(void (*indicate)(void *))
{
    drv_pin_mode(TOUCH_FT3169_INT, PIN_MODE_INPUT);
    drv_pin_attach_irq(TOUCH_FT3169_INT, PIN_IRQ_MODE_RISING_FALLING, indicate,
                       NULL);
    drv_pin_irq_enable(TOUCH_FT3169_INT, PIN_IRQ_DISABLE);
}

void rtk_touch_hal_int_config(bool enable)
{
    if (enable == true)
    {
        drv_pin_irq_enable(TOUCH_FT3169_INT, PIN_IRQ_ENABLE);
    }
    else
    {
        drv_pin_irq_enable(TOUCH_FT3169_INT, PIN_IRQ_DISABLE);
    }
}


static bool touch_enter_dlps(void)
{
    Pad_Config(TOUCH_FT3169_INT, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    System_WakeUpPinEnable(TOUCH_FT3169_INT, PAD_WAKEUP_POL_LOW, PAD_WAKEUP_DEB_DISABLE);
    return false;
}

static bool touch_exit_dlps(void)
{
    Pad_Config(TOUCH_FT3169_INT, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(TOUCH_FT3169_SCL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(TOUCH_FT3169_SDA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    return false;
}

static bool touch_allowed_enter_dlps_check(void)
{
    return true;
}

static bool touch_system_wakeup_dlps_check(void)
{
    if (System_WakeUpInterruptValue(TOUCH_FT3169_INT) == SET)
    {
        Pad_ClearWakeupINTPendingBit(TOUCH_FT3169_INT);
        System_WakeUpPinDisable(TOUCH_FT3169_INT);
        DBG_DIRECT("Touch Wake up");
        if (touch_wakeup_indicate != NULL)
        {
            touch_wakeup_indicate();
        }
        return true;
    }
    return false;
}

void drv_touch_dlps_init(void)
{
    System_WakeUpPinEnable(TOUCH_FT3169_INT, PAD_WAKEUP_POL_LOW, PAD_WAKEUP_DEB_DISABLE);
#ifdef RTK_HAL_DLPS
    drv_dlps_exit_cbacks_register("touch", touch_exit_dlps);
    drv_dlps_enter_cbacks_register("touch", touch_enter_dlps);
    drv_dlps_wakeup_cbacks_register("touch", touch_system_wakeup_dlps_check);
    drv_dlps_check_cbacks_register("touch", touch_allowed_enter_dlps_check);
#endif
}

void drv_touch_set_wakeup_indicate(void (*wakeup_ind))
{
    touch_wakeup_indicate = wakeup_ind;
}

void rtk_touch_hal_init(void)
{
    drv_i2c0_init(TOUCH_FT3169_SCL, TOUCH_FT3169_SDA);
    drv_pin_mode(TOUCH_FT3169_RST, PIN_MODE_OUTPUT);
    drv_pin_write(TOUCH_FT3169_RST, 0);
    platform_delay_ms(10);
    drv_pin_write(TOUCH_FT3169_RST, 1);
    platform_delay_ms(100);



    uint8_t iic_write_buf[1] = {TOUCH_FT3169_CHIP_ID};
    uint8_t iic_read_buf[1] = {0x33};

    drv_i2c0_write(TOUCH_FT3169_ADDR, iic_write_buf, 1);
    platform_delay_us(100);

    drv_i2c0_read(TOUCH_FT3169_ADDR, iic_read_buf, 1);

    DBG_DIRECT("CTP ID:0x%x\r\n", iic_read_buf[0]);

    drv_touch_dlps_init();

}

