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
#include "touch_chsc6417eu24_8772f_368448.h"
#include "string.h"
#include "trace.h"
#include "utils.h"
#include "board.h"


void (*touch_wakeup_indicate)(void) = NULL;

uint8_t last_buf[3] = {0x1};


bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing)
{
    uint8_t iic_write_buf[1] = {0x00};
    uint8_t buf[3] = {0};
    uint8_t press = 0;

    drv_i2c0_write(TOUCH_CHSC6417EU24_ADDR, iic_write_buf, 1);
    platform_delay_us(100);

    drv_i2c0_read(TOUCH_CHSC6417EU24_ADDR, buf, 3);

    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && last_buf[0] != 0 && last_buf[1] != 0 &&
        last_buf[2] != 0)
    {
        *x = ((((last_buf[0] & 0x40) >> 6) << 8) | last_buf[1]);
        *y = (((last_buf[0] & 0x80) >> 7) << 8) | last_buf[2];
        press = (last_buf[0] & 0x03) ? 1 : 0;
    }
    else
    {
        *x = ((((buf[0] & 0x40) >> 6) << 8) | buf[1]);
        *y = (((buf[0] & 0x80) >> 7) << 8) | buf[2];
        press = (buf[0] & 0x03) ? 1 : 0;
    }

    if (press == 1)
    {
        *pressing = true;
    }
    else
    {
        *pressing = false;
    }

    memcpy(last_buf, buf, 3);

    //DBG_DIRECT("rtk_touch_hal_read_all buf[0] 0x%x buf[1] 0x%x buf[2] 0x%x", buf[0], buf[1], buf[2]);
    //DBG_DIRECT("rtk_touch_hal_read_all x = %d y = %d press 0x%x", *x, *y, press);
    return true;
}


void rtk_touch_hal_set_indicate(void (*indicate)(void *))
{
    drv_pin_mode(TOUCH_CHSC6417EU24_INT, PIN_MODE_INPUT);
    drv_pin_attach_irq(TOUCH_CHSC6417EU24_INT, PIN_IRQ_MODE_RISING_FALLING, indicate,
                       NULL);
    drv_pin_irq_enable(TOUCH_CHSC6417EU24_INT, PIN_IRQ_DISABLE);
}

void rtk_touch_hal_int_config(bool enable)
{
    if (enable == true)
    {
        drv_pin_irq_enable(TOUCH_CHSC6417EU24_INT, PIN_IRQ_ENABLE);
    }
    else
    {
        drv_pin_irq_enable(TOUCH_CHSC6417EU24_INT, PIN_IRQ_DISABLE);
    }
}


static bool touch_enter_dlps(void)
{
    Pad_Config(TOUCH_CHSC6417EU24_INT, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    System_WakeUpPinEnable(TOUCH_CHSC6417EU24_INT, PAD_WAKEUP_POL_LOW, PAD_WAKEUP_DEB_DISABLE);
    return false;
}

static bool touch_exit_dlps(void)
{
    Pad_Config(TOUCH_CHSC6417EU24_INT, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(TOUCH_CHSC6417EU24_SCL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(TOUCH_CHSC6417EU24_SDA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    return false;
}

static bool touch_allowed_enter_dlps_check(void)
{
    return true;
}

static bool touch_system_wakeup_dlps_check(void)
{
    if (System_WakeUpInterruptValue(TOUCH_CHSC6417EU24_INT) == SET)
    {
        Pad_ClearWakeupINTPendingBit(TOUCH_CHSC6417EU24_INT);
        System_WakeUpPinDisable(TOUCH_CHSC6417EU24_INT);
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
    System_WakeUpPinEnable(TOUCH_CHSC6417EU24_INT, PAD_WAKEUP_POL_LOW, PAD_WAKEUP_DEB_DISABLE);
    drv_dlps_exit_cbacks_register("touch", touch_exit_dlps);
    drv_dlps_enter_cbacks_register("touch", touch_enter_dlps);
    drv_dlps_wakeup_cbacks_register("touch", touch_system_wakeup_dlps_check);
    drv_dlps_check_cbacks_register("touch", touch_allowed_enter_dlps_check);
}

void drv_touch_set_wakeup_indicate(void (*wakeup_ind))
{
    touch_wakeup_indicate = wakeup_ind;
}

void rtk_touch_hal_init(void)
{
    drv_i2c0_init(TOUCH_CHSC6417EU24_SCL, TOUCH_CHSC6417EU24_SDA);
    drv_pin_mode(TOUCH_CHSC6417EU24_RST, PIN_MODE_OUTPUT);
    drv_pin_write(TOUCH_CHSC6417EU24_RST, 1);
    platform_delay_ms(30);
    drv_pin_write(TOUCH_CHSC6417EU24_RST, 0);
    platform_delay_ms(30);
    drv_pin_write(TOUCH_CHSC6417EU24_RST, 1);
    platform_delay_ms(30);

    drv_touch_dlps_init();

}

