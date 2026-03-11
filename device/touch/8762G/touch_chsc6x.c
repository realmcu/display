/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "rtl_gpio.h"
#include "rtl_rcc.h"
#include "drv_gpio.h"
#include "drv_i2c.h"
#include "drv_touch.h"
#include "drv_lcd.h"
#include "touch_chsc6x.h"
#include "string.h"
#include "trace.h"
#include "utils.h"


struct ts_event
{
    uint16_t x; /*x coordinate */
    uint16_t y; /*y coordinate */
    int flag; /* touch event flag: 0 -- down; 1-- up; 2 -- contact */
    int id;   /*touch ID */
};
uint32_t chsc6x_read(uint8_t *buf, uint32_t len)
{
#define CHSC6X_MAX_POINTS_NUM     (1)
    int ret;
    int rd_len = 0;
    unsigned char point_num;
    unsigned char read_buf[6];
    struct ts_event events[CHSC6X_MAX_POINTS_NUM];

    if (1 == CHSC6X_MAX_POINTS_NUM)
    {
        rd_len = 3;
    }
    ret = drv_i2c1_read(CHSC6X_I2C_ID, read_buf, rd_len);

    // DBG_DIRECT("read ret %d, 0x%x, 0x%x, 0x%x", ret, read_buf[0], read_buf[1], read_buf[2]);
    if (rd_len == ret)
    {
        point_num = read_buf[0] & 0x03;

        if (1 == CHSC6X_MAX_POINTS_NUM)
        {
            events[0].x = (unsigned short)(((read_buf[0] & 0x40) >> 6) << 8) | (unsigned short)read_buf[1];
            events[0].y = (unsigned short)(((read_buf[0] & 0x80) >> 7) << 8) | (unsigned short)read_buf[2];

            events[0].flag = (read_buf[0] >> 4) & 0x03;
            events[0].id = (read_buf[0] >> 2) & 0x01;
            // DBG_DIRECT("chsc6x:   000  X:%d, Y:%d, point_num:%d,flag:%d, id:%d \r\n", \
            //     events[0].x, events[0].y, point_num, events[0].flag, events[0].id);

        }
    }
    memcpy(buf, read_buf, rd_len);

    return ret;
}



bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing)
{
    // DBG_DIRECT("read");
    *pressing = false;
    uint8_t buf[10] = {0};
    *x = 390;
    *y = 390;
    static uint16_t x_old = 0;
    static uint16_t y_old = 0;


    if (chsc6x_read(buf, 0))
    {
        *pressing = ((buf[0] >> 4) & 0x03)  == 0x02;
        if (*pressing)
        {
            uint16_t tp_x = (uint16_t)(((buf[0] & 0x40) >> 6) << 8) | (uint16_t)buf[1];
            uint16_t tp_y = (uint16_t)(((buf[0] & 0x80) >> 7) << 8) | (uint16_t)buf[2];
            *x = 390 - tp_x;
            *y = 390 - tp_y;
            // DBG_DIRECT("tp x %d y %d", *x, *y);
            x_old = *x;
            y_old = *y;
        }
    }
    else
    {
        *x = x_old;
        *y = y_old;
    }
    return true;


}


void rtk_touch_hal_set_indicate(void (*indicate)(void *))
{
    (void)indicate;
    return;
}
void rtk_touch_hal_int_config(bool enable)
{
    (void)enable;
    return;
}
bool rtk_touch_hal_power_off(void)
{
    // keep holding gpio pin level in sleep
    drv_pin_mode(TOUCH_CHSC6X_INT, PIN_MODE_INPUT);
    Pad_Config(TOUCH_CHSC6X_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(TOUCH_CHSC6X_INT, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_HIGH);

    // GPIO wake-up pin
    System_WakeUpPinEnable(TOUCH_CHSC6X_INT, PAD_WAKEUP_POL_LOW, PAD_WAKEUP_DEB_DISABLE);
    return true;
}

bool rtk_touch_hal_power_on(void)
{
    Pad_Config(TOUCH_CHSC6X_INT, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_HIGH);
    return true;
}

bool rtk_touch_hal_dlps_check(void)
{
    return true;
}

bool rtk_touch_wake_up(void)
{
    if (System_WakeUpInterruptValue(TOUCH_CHSC6X_INT) == SET)
    {
        Pad_ClearWakeupINTPendingBit(TOUCH_CHSC6X_INT);
        System_WakeUpPinDisable(TOUCH_CHSC6X_INT);
        DBG_DIRECT("Touch Wake up");
        // tp_indicate(NULL);
        return true;
    }
    return false;
}

void rtk_touch_dlps_init(void)
{
}


void rtk_touch_hal_init(void)
{
    DBG_DIRECT("rtk_touch_hal_init line = %d\n", __LINE__);
    RCC_PeriphClockCmd(APBPeriph_GPIOA, APBPeriph_GPIOA_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_GPIOB, APBPeriph_GPIOB_CLOCK, ENABLE);

    drv_i2c1_init(TOUCH_CHSC6X_SCL, TOUCH_CHSC6X_SDA);

    drv_pin_mode(TOUCH_CHSC6X_RST, PIN_MODE_OUTPUT);
    drv_pin_write(TOUCH_CHSC6X_RST, 0);
    platform_delay_ms(80);
    drv_pin_write(TOUCH_CHSC6X_RST, 1);
    platform_delay_ms(80);

    drv_pin_mode(TOUCH_CHSC6X_INT, PIN_MODE_INPUT);
    // drv_pin_mode(TOUCH_CHSC6X_INT, PIN_IRQ_MODE_RISING);
    // drv_pin_irq_enable(TOUCH_CHSC6X_INT, false);

    {
#define CHSC6X_MAX_POINTS_NUM     (1)
        int ret;
        int rd_len = 0;
        unsigned char point_num;
        unsigned char read_buf[6];
        struct ts_event events[CHSC6X_MAX_POINTS_NUM];

        if (1 == CHSC6X_MAX_POINTS_NUM)
        {
            rd_len = 3;
        }
        ret = drv_i2c1_read(CHSC6X_I2C_ID, read_buf, rd_len);
        DBG_DIRECT("read ret %d", ret);
        if (rd_len == ret)
        {
            point_num = read_buf[0] & 0x03;

            if (1 == CHSC6X_MAX_POINTS_NUM)
            {
                events[0].x = (unsigned short)(((read_buf[0] & 0x40) >> 6) << 8) | (unsigned short)read_buf[1];
                events[0].y = (unsigned short)(((read_buf[0] & 0x80) >> 7) << 8) | (unsigned short)read_buf[2];

                events[0].flag = (read_buf[0] >> 4) & 0x03;
                events[0].id = (read_buf[0] >> 2) & 0x01;
                DBG_DIRECT("chsc6x:   000  X:%d, Y:%d, point_num:%d,flag:%d, id:%d \r\n", \
                           events[0].x, events[0].y, point_num, events[0].flag, events[0].id);

            }
        }

    }
}


