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
#include "touch_cst836u.h"
#include "string.h"
#include "trace.h"
#include "utils.h"
#include "board.h"


void (*touch_wakeup_indicate)(void) = NULL;

#define X_LEN      (360)
#define Y_LEN      640

#include "trace.h"
bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing)
{
    uint8_t iic_write_buf[1] = {0x03};
    uint8_t buf[4] = {0};

    drv_i2c0_write(TOUCH_CST836U_ADDR, iic_write_buf, 1);
    platform_delay_us(100);

    drv_i2c0_read(TOUCH_CST836U_ADDR, buf, 4);

    if ((buf[0] & (3 << 6)) >> 6 == 0x02)
    {
        *pressing = true;
    }
    else
    {
        *pressing = false;
    }

    uint16_t x_raw = (((buf[0] & 0x0f) << 8) | buf[1]);
    uint16_t y_raw = (((buf[2] & 0x0f) << 8) | buf[3]);


//    *x = y_raw;
//    *y = X_LEN - x_raw;

    *x = x_raw;
    *y = y_raw;

//    if (*pressing)
//    {
//        DBG_DIRECT("raw: x %d y %d\n", x_raw, y_raw);
//        DBG_DIRECT("tp: x %d y %d\n", *x, *y);
//    }

    return true;
}


void rtk_touch_hal_set_indicate(void (*indicate)(void *))
{
    drv_pin_mode(TOUCH_CST836U_INT, PIN_MODE_INPUT);
    drv_pin_attach_irq(TOUCH_CST836U_INT, PIN_IRQ_MODE_RISING_FALLING, indicate,
                       NULL);
    drv_pin_irq_enable(TOUCH_CST836U_INT, PIN_IRQ_DISABLE);
}

void rtk_touch_hal_int_config(bool enable)
{
    if (enable == true)
    {
        drv_pin_irq_enable(TOUCH_CST836U_INT, PIN_IRQ_ENABLE);
    }
    else
    {
        drv_pin_irq_enable(TOUCH_CST836U_INT, PIN_IRQ_DISABLE);
    }
}


bool rtk_touch_hal_power_off(void)
{
    drv_pin_mode(TOUCH_CST836U_INT, PIN_MODE_INPUT);
    Pad_Config(TOUCH_CST836U_INT, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    System_WakeUpPinEnable(TOUCH_CST836U_INT, PAD_WAKEUP_POL_LOW, PAD_WAKEUP_DEB_DISABLE);
    return false;
}

bool rtk_touch_hal_power_on(void)
{
    Pad_Config(TOUCH_CST836U_INT, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(TOUCH_CST836U_SCL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    Pad_Config(TOUCH_CST836U_SDA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    return false;
}

bool rtk_touch_hal_dlps_check(void)
{
    return true;
}

bool rtk_touch_wake_up(void)
{
    if (System_WakeUpInterruptValue(TOUCH_CST836U_INT) == SET)
    {
        Pad_ClearWakeupINTPendingBit(TOUCH_CST836U_INT);
        System_WakeUpPinDisable(TOUCH_CST836U_INT);
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
    System_WakeUpPinEnable(TOUCH_CST836U_INT, PAD_WAKEUP_POL_LOW, PAD_WAKEUP_DEB_DISABLE);
#ifdef RTK_HAL_DLPS
//    drv_dlps_exit_cbacks_register("touch", rtk_touch_hal_power_on);
//    drv_dlps_enter_cbacks_register("touch", rtk_touch_hal_power_off);
//    drv_dlps_wakeup_cbacks_register("touch", rtk_touch_wake_up);
//    drv_dlps_check_cbacks_register("touch", rtk_touch_hal_dlps_check);
#endif
}

void drv_touch_set_wakeup_indicate(void (*wakeup_ind))
{
    touch_wakeup_indicate = wakeup_ind;
}

void rtk_touch_hal_init(void)
{
    drv_i2c0_init(TOUCH_CST836U_SCL, TOUCH_CST836U_SDA);
    drv_pin_mode(TOUCH_CST836U_RST, PIN_MODE_OUTPUT);
    drv_pin_write(TOUCH_CST836U_RST, 0);
    platform_delay_ms(10);
    drv_pin_write(TOUCH_CST836U_RST, 1);
    platform_delay_ms(120);

    uint8_t iic_write_buf[1] = {TOUCH_CST836U_CHIP_ID};
    uint8_t iic_read_buf[1] = {0x33};

    drv_i2c0_write(TOUCH_CST836U_ADDR, iic_write_buf, 1);
    platform_delay_us(100);

    drv_i2c0_read(TOUCH_CST836U_ADDR, iic_read_buf, 1);

    DBG_DIRECT("CTP ID:0x%x\r\n", iic_read_buf[0]);

#ifdef RTK_HAL_DLPS
//    drv_touch_dlps_init();
#endif
}

