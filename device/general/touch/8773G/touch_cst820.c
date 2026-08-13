/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "touch_cst820.h"
#include "os_timer.h"
#include "app_msg.h"
//#include "hub_task.h"
#include "platform_utils.h"
#include "vector_table.h"
#include "rtl876x_i2c.h"
//#include "app_dlps.h"
#include "os_sched.h"
#include "stdio.h"
#include "os_timer.h"
#include "trace.h"
#include "rtl876x_gpio.h"
#include "rtl876x_rcc.h"
#include "rtl876x_tim.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x.h"

static bool touch_cst820_write(uint8_t *p_data, uint8_t len)
{
    I2C_SetSlaveAddress(TOUCH_I2C_BUS, TOUCH_SLAVE_ADDRESS);
    I2C_Status res = I2C_MasterWrite(TOUCH_I2C_BUS, p_data, len);
    if (res != I2C_Success)
    {
        APP_PRINT_INFO1("ERROR! touch_write I2C_MasterWrite: %d", res);
        return false;
    }
    return true;
}

static bool touch_cst820_read(uint8_t *p_data, uint8_t len)
{
    I2C_SetSlaveAddress(TOUCH_I2C_BUS, TOUCH_SLAVE_ADDRESS);
    I2C_Status res = I2C_MasterRead(TOUCH_I2C_BUS, p_data, len);
    if (res != I2C_Success)
    {
        APP_PRINT_INFO1("ERROR! touch_read I2C_MasterRead: %d", res);
        return false;
    }
    return true;
}

bool touch_cst820_read_all(uint16_t *x, uint16_t *y, bool *pressing)
{
    uint8_t read_buf[24] = {0};
    uint8_t read_reg = 0x00;

    touch_cst820_write(&read_reg, 1);
    touch_cst820_read(read_buf, sizeof(read_buf));

    if (read_buf[3] >> 6 == 2)
    {
        *pressing = true;
    }
    else
    {
        *pressing = false;
    }
//    *pressing = true;
    // *x = 466 - (((read_buf[3] & 0x0f) << 8) | read_buf[4]);
    // *y = 466 - (((read_buf[5] & 0x0f) << 8) | read_buf[6]);
    *x = (((read_buf[3] & 0x0f) << 8) | read_buf[4]);
    *y = (((read_buf[5] & 0x0f) << 8) | read_buf[6]);

    return true;
}

bool touch_cst820_read_chip_id(void)
{
    uint8_t read_reg = 0xa7;
    uint8_t chip_id[4] = {0};

    if (touch_cst820_write(&read_reg, 1) == false)
    {
        DBG_DIRECT("ERROR touch_cst820_read_chip_id!!");
        return false;
    }
    if (touch_cst820_read(chip_id, 1) == false)
    {
        DBG_DIRECT("ERROR touch_cst820_read_chip_id!!");
        return false;
    }

    DBG_DIRECT("[cst820 get chip id] -- 0x%x", chip_id[0]);
    return true;
}


static void touch_gesture_release_timeout(void *pxTimer);
static void *touch_gesture_release_timer = NULL;
void touch_get_chip_id(uint8_t *p_chip_id);
static TOUCH_DATA cur_point = {0};
static uint32_t touch_timeout_ms = 30;

static void touch_gesture_release_timeout(void *pxTimer)
{
//    cur_point.x = 0;
//    cur_point.y = 0;
//    cur_point.x_start = 0;
//    cur_point.y_start = 0;
//    cur_point.timestamp_ms_start = 0;
//    cur_point.timestamp_ms_pressing = 0;
    cur_point.count_pressing = 0;
    cur_point.is_press = 0;
}

/**
  * @brief  Initialize touch device
  * @param  None
  *
  * @retval None
  */
static void touch_device_init(void)
{
    const uint8_t pull_strength = 1;
    Pad_PullConfigValue(TOUCH_I2C_SCL, pull_strength);
    Pad_PullConfigValue(TOUCH_I2C_SDA, pull_strength);

    Pinmux_Config(TOUCH_I2C_SCL, TOUCH_I2C_FUNC_SCL);
    Pinmux_Config(TOUCH_I2C_SDA, TOUCH_I2C_FUNC_SDA);
    Pinmux_Config(TOUCH_INT, DWGPIO);

    Pad_Config(TOUCH_I2C_SCL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_I2C_SDA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_INT, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    /* Enable GPIO and hardware timer's clock */
    RCC_PeriphClockCmd(TOUCH_INT_APBPeriph,  TOUCH_INT_APBPeriph_CLK,  ENABLE);

    RamVectorTableUpdate(TOUCH_INT_VECTORn, TOUCH_INT_HANDLER);
    /* Initialize GPIO as interrupt mode */
    GPIO_InitTypeDef GPIO_Param;
    GPIO_StructInit(&GPIO_Param);
    GPIO_Param.GPIO_PinBit = GPIO_GetPin(TOUCH_INT);
    GPIO_Param.GPIO_Mode = GPIO_Mode_IN;
    GPIO_Param.GPIO_ITCmd = ENABLE;
    GPIO_Param.GPIO_ITTrigger = GPIO_INT_Trigger_EDGE;
    GPIO_Param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW; //GPIO_INT_POLARITY_ACTIVE_HIGH;
    GPIOx_Init(TOUCH_INT_GROUP, &GPIO_Param);

    RCC_PeriphClockCmd(TOUCH_I2C_APBPeriph, TOUCH_I2C_APBClock, DISABLE);
    RCC_PeriphClockCmd(TOUCH_I2C_APBPeriph, TOUCH_I2C_APBClock, ENABLE);
    I2C_InitTypeDef  I2C_InitStructure;
    I2C_StructInit(&I2C_InitStructure);
    I2C_InitStructure.I2C_Clock = 40000000;
    I2C_InitStructure.I2C_ClockSpeed   = 400000;
    I2C_InitStructure.I2C_DeviveMode   = I2C_DeviveMode_Master;
    I2C_InitStructure.I2C_AddressMode  = I2C_AddressMode_7BIT;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_Init(TOUCH_I2C_BUS, &I2C_InitStructure);
    I2C_Cmd(TOUCH_I2C_BUS, ENABLE);
    GPIOx_MaskINTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), ENABLE);
    GPIOx_INTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), DISABLE);
}

static void touch_device_reset(void)
{
    /* Reference: 8762D rtk_touch_hal_init() -- double-pulse reset.
     * 1st pulse (low 10ms / high 100ms): power-on / wake before I2C access.
     * 2nd pulse (low 10ms / high 120ms): bring the controller into work mode
     *                                    before reading chip id.
     */
    Pad_Config(TOUCH_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    platform_delay_ms(10);
    Pad_Config(TOUCH_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    platform_delay_ms(100);

    Pad_Config(TOUCH_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    platform_delay_ms(10);
    Pad_Config(TOUCH_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    platform_delay_ms(120);

    if (touch_gesture_release_timer == NULL)
    {
        os_timer_create(&touch_gesture_release_timer, "touch gesture release timer", 1, touch_timeout_ms,
                        false,
                        touch_gesture_release_timeout);
    }

    GPIOx_MaskINTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), ENABLE);
    GPIOx_INTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), ENABLE);
    GPIOx_ClearINTPendingBit(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT));
    GPIOx_MaskINTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), DISABLE);
}

static void touch_device_read_id(void)
{
    touch_cst820_read_chip_id();
}

void touch_gesture_exit_dlps(void)
{
//    touch_device_init();
    Pad_Config(TOUCH_I2C_SCL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_I2C_SDA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_INT, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
}

void touch_gesture_enter_dlps(void)
{
//    Pad_Config(TOUCH_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_I2C_SCL, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_I2C_SDA, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_INT, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_DISABLE, PAD_OUT_LOW);
}

void touch_driver_init(void)
{
    touch_device_init();
    touch_device_reset();
    touch_device_read_id();
}

bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing)
{
    bool res = touch_cst820_read_all(x, y, pressing);

    if (res)
    {
        return true;
    }
    else
    {
        APP_PRINT_ERROR0("ERROR! rtk_touch_hal_read_all");
        return false;
    }
}

void TOUCH_INT_HANDLER(void)
{
    /*  Mask GPIO interrupt */
    //DBG_DIRECT("TOUCH_INT_HANDLER!!");
    GPIOx_INTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), DISABLE);
    GPIOx_MaskINTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), ENABLE);
    GPIOx_ClearINTPendingBit(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT));

    cur_point.count_pressing++;
    if (cur_point.count_pressing <= 1)
    {
        cur_point.timestamp_ms_start = os_sys_time_get();
        rtk_touch_hal_read_all(&cur_point.x_start, &cur_point.y_start, &cur_point.is_press);
        cur_point.x = cur_point.x_start;
        cur_point.y = cur_point.y_start;
        cur_point.timestamp_ms_pressing = cur_point.timestamp_ms_start;
    }
    else
    {
        cur_point.timestamp_ms_pressing = os_sys_time_get();
        rtk_touch_hal_read_all(&cur_point.x, &cur_point.y, &cur_point.is_press);
    }
    os_timer_restart(&touch_gesture_release_timer, touch_timeout_ms);

    GPIOx_INTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), ENABLE);
    GPIOx_MaskINTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), DISABLE);
}

TOUCH_DATA get_raw_touch_data(void)
{
    return cur_point;
}

bool touch_set_timeout_ms(uint32_t time)
{
    if (time > 0)
    {
        touch_timeout_ms = time;
        if (touch_gesture_release_timer != NULL)
        {
            os_timer_delete(&touch_gesture_release_timer);
            touch_gesture_release_timer = NULL;
        }
        os_timer_create(&touch_gesture_release_timer, "touch gesture release timer", 1, touch_timeout_ms,
                        false, touch_gesture_release_timeout);
        return true;
    }
    return false;
}

void rtk_touch_hal_set_indicate(void (*indicate)(void *))
{
    (void)indicate;
    return;
}
void rtk_touch_hal_int_config(bool enable)
{
    if (enable)
    {
        NVIC_InitTypeDef NVIC_InitStruct;
        NVIC_InitStruct.NVIC_IRQChannel = TOUCH_INT_IRQ;
        NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
        NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStruct);
        GPIOx_MaskINTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), ENABLE);
        GPIOx_INTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), ENABLE);
        GPIOx_ClearINTPendingBit(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT));
        GPIOx_MaskINTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), DISABLE);
    }
    else
    {
        NVIC_InitTypeDef NVIC_InitStruct;
        NVIC_InitStruct.NVIC_IRQChannel = TOUCH_INT_IRQ;
        NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
        NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
        NVIC_Init(&NVIC_InitStruct);
        GPIOx_MaskINTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), ENABLE);
        GPIOx_INTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), DISABLE);
        GPIOx_ClearINTPendingBit(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT));
    }
    return;
}

void rtk_touch_hal_init(void)
{
    touch_driver_init();
}


