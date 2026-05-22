/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "touch_lw.h"
#include "string.h"
#include "trace.h"
#include "board.h"
#include "platform_utils.h"
#include "rtl876x_i2c.h"

#define DBP_TOUCH(...)   //DBG_DIRECT(__VA_ARGS__)

#define TP_ID_ADDR                 (0xA7)
#define TP_DATA_ADDR               (0x00)
#define TP_DATA_LEN                (8)



void touch_get_chip_id(uint8_t *p_chip_id);




/**
  * @brief  Initialize touch device
  * @param  None
  *
  * @retval None
  */
static void touch_pad_init(void)
{
    Pinmux_Config(TOUCH_I2C_SCL, TOUCH_I2C_FUNC_SCL);
    Pinmux_Config(TOUCH_I2C_SDA, TOUCH_I2C_FUNC_SDA);
    Pinmux_Config(TOUCH_INT, DWGPIO);
    Pinmux_Config(TOUCH_RST, IDLE_MODE);

    Pad_Config(TOUCH_I2C_SCL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_I2C_SDA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_INT, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
}

static void TOUCH_INT_HANDLER(void)
{
    /*  Mask GPIO interrupt */
    DBP_TOUCH("TOUCH_INT_HANDLER!!");
}

static void touch_device_init(void)
{
    touch_pad_init();

    /* Enable GPIO and hardware timer's clock */
    RCC_PeriphClockCmd(TOUCH_INT_APBPeriph,  TOUCH_INT_APBPeriph_CLK,  DISABLE);
    RCC_PeriphClockCmd(TOUCH_INT_APBPeriph,  TOUCH_INT_APBPeriph_CLK,  ENABLE);

    RamVectorTableUpdate(TOUCH_INT_VECTORn, TOUCH_INT_HANDLER);
    /* Initialize GPIO as interrupt mode */
    GPIO_InitTypeDef GPIO_Param;
    GPIO_StructInit(&GPIO_Param);
    GPIO_Param.GPIO_PinBit = GPIO_GetPin(TOUCH_INT);
    GPIO_Param.GPIO_Mode = GPIO_Mode_IN;
    GPIO_Param.GPIO_ITCmd = ENABLE;
    GPIO_Param.GPIO_ITTrigger = GPIO_INT_Trigger_EDGE;
    GPIO_Param.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
    GPIOx_Init(TOUCH_INT_GROUP, &GPIO_Param);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = TOUCH_INT_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);


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

void touch_gesture_exit_dlps(void)
{
    Pad_Config(TOUCH_I2C_SCL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_I2C_SDA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_INT, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_RST, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
}

void touch_gesture_enter_dlps(void)
{
    GPIOx_INTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), DISABLE);
    GPIOx_MaskINTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), ENABLE);

    Pad_Config(TOUCH_I2C_SCL, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_I2C_SDA, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_INT, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
}

void touch_driver_init(void)
{
    DBP_TOUCH("------------------> touch_driver_init LW;");
    touch_device_init();
    Pad_Config(TOUCH_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    platform_delay_ms(10);
    Pad_Config(TOUCH_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    platform_delay_ms(10);
    Pad_Config(TOUCH_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    platform_delay_ms(50);

//    GPIOx_MaskINTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), ENABLE);
//    GPIOx_INTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), ENABLE);
//    GPIOx_ClearINTPendingBit(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT));
//    GPIOx_MaskINTConfig(TOUCH_INT_GROUP, GPIO_GetPin(TOUCH_INT), DISABLE);

    uint8_t chip_id;
    touch_get_chip_id(&chip_id);
    DBP_TOUCH("--- chip_id : 0x%X ;", chip_id);
}

void touch_write(uint8_t reg, uint8_t data)
{
    uint8_t I2C_WriteBuf[2] = {reg, data};
    I2C_SetSlaveAddress(TOUCH_I2C_BUS, TOUCH_SLAVE_ADDRESS);
    I2C_Status res = I2C_MasterWrite(TOUCH_I2C_BUS, I2C_WriteBuf, 2);
    if (res != I2C_Success)
    {
        DBP_TOUCH("ERROR! touch_write I2C_MasterWrite: %d", res);
    }
}

void touch_read(uint8_t reg, uint8_t *p_data, uint8_t len)
{
    I2C_SetSlaveAddress(TOUCH_I2C_BUS, TOUCH_SLAVE_ADDRESS);
    I2C_Status res = I2C_MasterWrite(TOUCH_I2C_BUS, &reg, 1);
    if (res != I2C_Success)
    {
        DBP_TOUCH("ERROR! touch_read I2C_MasterWrite: %d", res);
    }
    platform_delay_us(1);
    res = I2C_MasterRead(TOUCH_I2C_BUS, p_data, len);
    if (res != I2C_Success)
    {
        DBP_TOUCH("ERROR! touch_read I2C_MasterRead: %d", res);
    }
}

void touch_get_chip_id(uint8_t *p_chip_id)
{
    uint8_t TP_ID = 0;
    uint8_t TP_ID_NUM = 0;
    do
    {
        touch_read(TP_ID_ADDR, &TP_ID, 1);
        DBP_TOUCH("--- touch_get_chip_id : 0x%X ;", TP_ID);
        TP_ID_NUM ++;
        if (TP_ID_NUM > 10)
        {
            break;
        }
    }
    while (TP_ID != 0xB7);

    *p_chip_id = TP_ID;
}

bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing)
{
    uint8_t buf[24] = {0};
    touch_read(TP_DATA_ADDR, buf, TP_DATA_LEN);

    uint8_t point_num = buf[2] & 0x0f;

    int16_t x_buff = ((buf[3] & 0x0f) << 8) | buf[4];
    int16_t y_buff = ((buf[5] & 0x0f) << 8) | buf[6];

    if ((buf[3] & 0x80) == 0x80)
    {
        *pressing = true;
    }
    else if ((buf[3] & 0x40) == 0x40 || point_num == 0)
    {
        *pressing = false;
    }

    *x = x_buff;
    *y = y_buff;

    DBP_TOUCH("x_buff = %d ; ", x_buff);
    DBP_TOUCH("y_buff = %d ; ", y_buff);
    DBP_TOUCH("pressing = %d ; ", *pressing);

    return true;
}





