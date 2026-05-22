/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2024-04-09     roy  first version
 */

#include "rtl_hal_peripheral.h"
#include "drv_gpio.h"
#include "drv_i2c.h"
#include "drv_touch.h"
#include "touch_ft6336.h"
#include "string.h"
#include "trace.h"
#include "board.h"

static void i2c_reg_read8(uint8_t slave_addr, uint8_t register_addr, uint8_t *data_buf,
                          uint32_t size)
{
    // read reg
    drv_i2c0_write(slave_addr, &register_addr, 1);
    platform_delay_us(10);
    drv_i2c0_read(slave_addr, data_buf, size);
}

static void rtk_touch_hal_binding(uint16_t *x, uint16_t *y)
{
    /** binding tp to display coordinate
     *
     * sbee2 86box
     * tp: 240*320,  width-240.  height-320
     * display: 320*240, width-320, height-240
     * scan: left to right, up to down
     * --------------
     * |*(tp)      +| (display)
     * |            |
     * |            |
     * |            |
     * |            |
     * |            |
     * |            |
     * |            |
     * --------------
     *
     */
#define DISPLAY_LCD_WIDTH 320
#define DISPLAY_LCD_HEIGHT 240

    uint16_t x_pos = *x;
    uint16_t y_pos = *y;

    *x = y_pos;
    *y = DISPLAY_LCD_HEIGHT - x_pos;
}


bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing)
{
    // DBG_DIRECT("%s %d", __FUNCTION__, __LINE__);
    uint8_t data_buf[4] = {0};
    uint16_t x_pos = 0;
    uint16_t y_pos = 0;
    bool ispressing = false;

    // get touch number
    // i2c_reg_read8(FT6236_I2C_SLAVE_ADDR, FT6X36_TD_STAT_REG, data_buf, 1);
    // DBG_DIRECT("FT6X36_TD_STAT_REG: 0x%02x", data_buf[0]);
    // if (data_buf[0] != 0)
    {
        i2c_reg_read8(FT6236_I2C_SLAVE_ADDR, FT6X36_P1_XH_REG, &data_buf[0], 2);
        // DBG_DIRECT("FT6X36_P1_XH_REG: 0x%x 0x%x %d %d", data_buf[0], data_buf[1], data_buf[0], data_buf[1]);

        i2c_reg_read8(FT6236_I2C_SLAVE_ADDR, FT6X36_P1_YH_REG, &data_buf[2], 2);
        // DBG_DIRECT("FT6X36_P1_YH_REG: 0x%x 0x%x %d %d", data_buf[2], data_buf[3], data_buf[2], data_buf[3]);

        x_pos = ((data_buf[0] & FT6X36_MSB_MASK) << 8) | (data_buf[1] & FT6X36_LSB_MASK);
        y_pos = ((data_buf[2] & FT6X36_MSB_MASK) << 8) | (data_buf[3] & FT6X36_LSB_MASK);
        // DBG_DIRECT("x: %d, y: %d", x_pos, y_pos);

        ispressing = ((data_buf[0] & FT6X36_TOUCH_EVT_FLAG_MASK) >> FT6X36_TOUCH_EVT_FLAG_SHIFT) ==
                     FT6X36_TOUCH_EVT_FLAG_CONTACT;
        // DBG_DIRECT("event: 0x%x,  pressing: %d",
        //         (data_buf[0] & FT6X36_TOUCH_EVT_FLAG_MASK) >> FT6X36_TOUCH_EVT_FLAG_SHIFT, ispressing);
    }

    // if (ispressing)
    // {
    //     DBG_DIRECT("[TOUCH] x: %d, y: %d", x_pos, y_pos);
    // }

    *x = x_pos;
    *y = y_pos;
    *pressing = ispressing;
    // binding tp to display coordinate
    rtk_touch_hal_binding(x, y);
    if (ispressing)
    {
        // DBG_DIRECT("[TOUCH] x: %d, y: %d", *x, *y);
    }
    return true;
}

/**
  * @brief  Read the FT6x36 gesture ID. Initialize first!
  * @param  dev_addr: I2C FT6x36 Slave address.
  * @retval The gesture ID or 0x00 in case of failure
  */
static uint8_t ft6x36_get_gesture_id(void)
{
    uint8_t data_buf;
    i2c_reg_read8(FT6236_I2C_SLAVE_ADDR, FT6X36_GEST_ID_REG, &data_buf, 1);
    DBG_DIRECT("FT6X36_GEST_ID_REG: 0x%x", data_buf);
    return data_buf;
}

void tptest(void *arg)
{
    DBG_DIRECT("%s %d", __FUNCTION__, __LINE__);
    uint8_t data_buf[4] = {0};
    uint16_t x = 0;
    uint16_t y = 0;
    i2c_reg_read8(FT6236_I2C_SLAVE_ADDR, FT6X36_TD_STAT_REG, data_buf, 1);
    DBG_DIRECT("FT6X36_TD_STAT_REG: 0x%02x", data_buf[0]);
    if (data_buf[0] != 1)
    {
        return;
    }

    i2c_reg_read8(FT6236_I2C_SLAVE_ADDR, FT6X36_P1_XH_REG, &data_buf[0], 2);
    DBG_DIRECT("FT6X36_P1_XH_REG: 0x%x 0x%x %d %d", data_buf[0], data_buf[1], data_buf[0], data_buf[1]);

    i2c_reg_read8(FT6236_I2C_SLAVE_ADDR, FT6X36_P1_YH_REG, &data_buf[2], 2);
    DBG_DIRECT("FT6X36_P1_YH_REG: 0x%x 0x%x %d %d", data_buf[2], data_buf[3], data_buf[2], data_buf[3]);

    x = ((data_buf[0] & FT6X36_MSB_MASK) << 8) | (data_buf[1] & FT6X36_LSB_MASK);
    y = ((data_buf[2] & FT6X36_MSB_MASK) << 8) | (data_buf[3] & FT6X36_LSB_MASK);
    DBG_DIRECT("x: %d, y: %d", x, y);

    bool pressing = ((data_buf[0] & FT6X36_TOUCH_EVT_FLAG_MASK) >> FT6X36_TOUCH_EVT_FLAG_SHIFT) ==
                    FT6X36_TOUCH_EVT_FLAG_CONTACT;
    DBG_DIRECT("event: 0x%x,  pressing: %d",
               (data_buf[0] & FT6X36_TOUCH_EVT_FLAG_MASK) >> FT6X36_TOUCH_EVT_FLAG_SHIFT, pressing);

    ft6x36_get_gesture_id();
}

void rtk_touch_hal_set_indicate(void (*indicate)(void *))
{
    // DBG_DIRECT("%s %d", __FUNCTION__, __LINE__);
    drv_pin_mode(TOUCH_FT6336_INT, PIN_MODE_INPUT);
    drv_pin_attach_irq(TOUCH_FT6336_INT, PIN_IRQ_MODE_RISING_FALLING, indicate,
                       NULL);
    drv_pin_irq_enable(TOUCH_FT6336_INT, PIN_IRQ_DISABLE);
}

void rtk_touch_hal_int_config(bool enable)
{
    // DBG_DIRECT("%s %d", __FUNCTION__, __LINE__);
    if (enable == true)
    {
        drv_pin_irq_enable(TOUCH_FT6336_INT, PIN_IRQ_ENABLE);
    }
    else
    {
        drv_pin_irq_enable(TOUCH_FT6336_INT, PIN_IRQ_DISABLE);
    }
}


static bool touch_enter_dlps(void)
{
    Pad_Config(TOUCH_FT6336_INT, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    System_WakeUpPinEnable(TOUCH_FT6336_INT, PAD_WAKEUP_POL_LOW, PAD_WK_DEBOUNCE_DISABLE);
    return false;
}

static bool touch_exit_dlps(void)
{
    Pad_Config(TOUCH_FT6336_INT, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    return false;
}

static bool touch_allowed_enter_dlps_check(void)
{
    return true;
}

static bool touch_system_wakeup_dlps_check(void)
{
    if (System_WakeUpInterruptValue(TOUCH_FT6336_INT) == SET)
    {
        Pad_ClearWakeupINTPendingBit(TOUCH_FT6336_INT);
        System_WakeUpPinDisable(TOUCH_FT6336_INT);
        DBG_DIRECT("Touch Wake up");
        return true;
    }
    return false;
}

void drv_touch_dlps_init(void)
{
    System_WakeUpPinEnable(TOUCH_FT6336_INT, PAD_WAKEUP_POL_LOW, PAD_WK_DEBOUNCE_DISABLE);
    drv_dlps_exit_cbacks_register("touch", touch_exit_dlps);
    drv_dlps_enter_cbacks_register("touch", touch_enter_dlps);
    drv_dlps_wakeup_cbacks_register("touch", touch_system_wakeup_dlps_check);
    drv_dlps_check_cbacks_register("touch", touch_allowed_enter_dlps_check);
}


void rtk_touch_hal_init(void)
{
    // init i2c io
    drv_i2c0_init(TOUCH_FT6336_SCL, TOUCH_FT6336_SDA);
    drv_pin_mode(TOUCH_FT6336_RST, PIN_MODE_OUTPUT);
    drv_pin_write(TOUCH_FT6336_RST, 0);
    platform_delay_ms(10);
    drv_pin_write(TOUCH_FT6336_RST, 1);
    platform_delay_ms(100);


    // read reg
    uint8_t data_buf = 0;
    DBG_DIRECT("Found touch panel controller");
    i2c_reg_read8(FT6236_I2C_SLAVE_ADDR, FT6X36_PANEL_ID_REG, &data_buf, 1);
    DBG_DIRECT("\tDevice ID: 0x%x", data_buf);

    i2c_reg_read8(FT6236_I2C_SLAVE_ADDR, FT6X36_CHIPSELECT_REG, &data_buf, 1);
    DBG_DIRECT("\tChip ID: 0x%x", data_buf);

    i2c_reg_read8(FT6236_I2C_SLAVE_ADDR, FT6X36_DEV_MODE_REG, &data_buf, 1);
    DBG_DIRECT("\tDevice mode: 0x%x", data_buf);

    i2c_reg_read8(FT6236_I2C_SLAVE_ADDR, FT6X36_FIRMWARE_ID_REG, &data_buf, 1);
    DBG_DIRECT("\tFirmware ID: 0x%x", data_buf);

    i2c_reg_read8(FT6236_I2C_SLAVE_ADDR, FT6X36_RELEASECODE_REG, &data_buf, 1);
    DBG_DIRECT("\tRelease code: 0x%x", data_buf);
}

