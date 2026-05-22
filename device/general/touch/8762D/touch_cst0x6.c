/**
*****************************************************************************************
*     Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.

*
*     SPDX-License-Identifier: Apache-2.0
*****************************************************************************************
* @file    touch_cst816d.c
* @brief   This file provides CST816D touch driver functions
* @author
* @date
* @version v1.0
* *************************************************************************************
*/
#include "string.h"
#include "rtl_hal_peripheral.h"
#include "touch_cst0x6.h"
#include "drv_gpio.h"
#include "drv_i2c.h"
#include "drv_touch.h"
#include "trace.h"
#include "board.h"
#include "os_mem.h"




static uint8_t chip_id = 0x59;


void cst0x6_write(uint8_t reg, uint8_t data)
{
    uint8_t I2C_WriteBuf[2] = {0x0, 0x0};
    I2C_WriteBuf[0] = reg;
    I2C_WriteBuf[1] = data;
    I2C_SetSlaveAddress(TOUCH_I2C_BUS, TOUCH_SLAVE_ADDRESS);
    I2C_MasterWrite(TOUCH_I2C_BUS, I2C_WriteBuf, 2);
}

void cst0x6_read(uint8_t reg, uint8_t *p_data)
{
    uint8_t buf[1] = {reg};
    I2C_SetSlaveAddress(TOUCH_I2C_BUS, TOUCH_SLAVE_ADDRESS);
    I2C_MasterWrite(TOUCH_I2C_BUS, buf, 1);
    platform_delay_us(1);
    I2C_MasterRead(TOUCH_I2C_BUS, p_data, 1);
}

bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing)
{
    uint8_t iic_write_buf[1] = {0};
    uint8_t buf[24] = {0};

    drv_i2c1_write(TOUCH_SLAVE_ADDRESS, iic_write_buf, 1);
    platform_delay_us(100);

    drv_i2c1_read(TOUCH_SLAVE_ADDRESS, buf, 24);

    if (buf[3] >> 6 == 2)
    {
        *pressing = true;
    }
    else
    {
        *pressing = false;
    }

    *x = (((buf[3] & 0x0f) << 8) | buf[4]);
    *y = (((buf[5] & 0x0f) << 8) | buf[6]);

    return true;
}

static void touch_gesture_pad_init(void)
{
    Pinmux_Config(TOUCH_I2C_SCL, TOUCH_I2C_FUNC_SCL);
    Pinmux_Config(TOUCH_I2C_SDA, TOUCH_I2C_FUNC_SDA);
    Pinmux_Config(TOUCH_INT, DWGPIO);

    Pad_Config(TOUCH_I2C_SCL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_I2C_SDA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_INT, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
}

static void cst0x6_device_init(void)
{
    touch_gesture_pad_init();
    /* Enable GPIO and hardware timer's clock */
    RCC_PeriphClockCmd(APBPeriph_GPIO,  APBPeriph_GPIO_CLOCK,  ENABLE);

    /* Initialize GPIO as interrupt mode */
    GPIO_InitTypeDef GPIO_Param;
    GPIO_StructInit(&GPIO_Param);
    GPIO_Param.GPIO_Pin = GPIO_GetPin(TOUCH_INT);
    GPIO_Param.GPIO_Mode = GPIO_Mode_IN;
    GPIO_Param.GPIO_ITCmd = ENABLE;
    GPIO_Param.GPIO_ITTrigger = GPIO_INT_Trigger_EDGE;
    GPIO_Param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
    GPIO_Init(&GPIO_Param);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = TOUCH_INT_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    RCC_I2CClkDivConfig(TOUCH_I2C_BUS, I2C_CLOCK_DIV_2);
    RCC_PeriphClockCmd(TOUCH_I2C_APBPeriph, TOUCH_I2C_APBClock, ENABLE);
    I2C_InitTypeDef  I2C_InitStructure;
    I2C_InitStructure.I2C_Clock = 20000000;
    I2C_InitStructure.I2C_ClockSpeed   = 400000;
    I2C_InitStructure.I2C_DeviveMode   = I2C_DeviveMode_Master;
    I2C_InitStructure.I2C_AddressMode  = I2C_AddressMode_7BIT;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_Init(TOUCH_I2C_BUS, &I2C_InitStructure);
    I2C_Cmd(TOUCH_I2C_BUS, ENABLE);

    System_WakeUpPinEnable(TOUCH_INT, PAD_WAKEUP_POL_LOW, PAD_WK_DEBOUNCE_DISABLE);
}


void rtk_touch_hal_set_indicate(void (*indicate)(void *))
{
    DBG_DIRECT("rtk_touch_hal_set_indicate");
    drv_pin_mode(TOUCH_INT, PIN_MODE_INPUT);
    drv_pin_attach_irq(TOUCH_INT, PIN_IRQ_MODE_RISING_FALLING, indicate,
                       NULL);
    drv_pin_irq_enable(TOUCH_INT, PIN_IRQ_ENABLE);
}

void rtk_touch_hal_int_config(bool enable)
{
    if (enable == true)
    {
        drv_pin_irq_enable(TOUCH_INT, PIN_IRQ_ENABLE);
    }
    else
    {
        drv_pin_irq_enable(TOUCH_INT, PIN_IRQ_DISABLE);
    }
}

static bool touch_enter_dlps(void)
{
    drv_pin_mode(TOUCH_INT, PIN_MODE_INPUT);
    Pad_Config(TOUCH_INT, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    System_WakeUpPinEnable(TOUCH_INT, PAD_WAKEUP_POL_LOW, PAD_WK_DEBOUNCE_DISABLE);
    DBG_DIRECT("touch_enter_dlps");
    return true;
}

static bool touch_exit_dlps(void)
{
    drv_i2c1_init(TOUCH_I2C_SCL, TOUCH_I2C_SDA);
    Pad_Config(TOUCH_INT, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    DBG_DIRECT("touch_exit_dlps");
    return true;
}

static bool touch_allowed_enter_dlps_check(void)
{
    return true;
}

static bool touch_system_wakeup_dlps_check(void)
{
    if (System_WakeUpInterruptValue(TOUCH_INT) == SET)
    {
        Pad_ClearWakeupINTPendingBit(TOUCH_INT);
        System_WakeUpPinDisable(TOUCH_INT);
        DBG_DIRECT("Touch Wake up");
        extern void gui_indev_wakeup(void);
        gui_indev_wakeup();
        return true;
    }
    return false;
}

void drv_touch_dlps_init(void)
{
    System_WakeUpPinEnable(TOUCH_INT, PAD_WAKEUP_POL_LOW, PAD_WK_DEBOUNCE_DISABLE);
    drv_dlps_exit_cbacks_register("touch", touch_exit_dlps);
    drv_dlps_enter_cbacks_register("touch", touch_enter_dlps);
    drv_dlps_wakeup_cbacks_register("touch", touch_system_wakeup_dlps_check);
    drv_dlps_check_cbacks_register("touch", touch_allowed_enter_dlps_check);
}

static void touch_get_chip_id(uint8_t *p_chip_id)
{
    cst0x6_read(FTS_REG_CHIP_ID, p_chip_id);
    APP_PRINT_INFO1("[cst0x6 get chip id] -- %X", *p_chip_id);
}

void rtk_touch_hal_init(void)
{
    cst0x6_device_init();
    drv_i2c1_init(TOUCH_I2C_SCL, TOUCH_I2C_SDA);
    drv_pin_mode(TOUCH_RST, PIN_MODE_OUTPUT);

    drv_pin_write(TOUCH_RST, 0);
    platform_delay_ms(10);
    drv_pin_write(TOUCH_RST, 1);
    platform_delay_ms(100);

    touch_get_chip_id(&chip_id);

#ifdef RTK_HAL_DLPS
    drv_touch_dlps_init();
#endif
}

