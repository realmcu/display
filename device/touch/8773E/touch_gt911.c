/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

//#include "wristband_sdk_config.h"

#if 1
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "rtl876x_i2c.h"
#include "touch_gt911.h"
#include "platform_utils.h"
#include "string.h"
#include "trace.h"






const uint8_t GT911_CFG_TBL[] =
{
    0X60, 0XE0, 0X01, 0XE0, 0X01, 0X05, 0X35, 0X00, 0X02, 0X08,
    0X1E, 0X08, 0X05, 0X3C, 0X0F, 0X05, 0X00, 0X00, 0XFF, 0X67,
    0X50, 0X00, 0X00, 0X18, 0X1A, 0X1E, 0X14, 0X89, 0X28, 0X0A,
    0X30, 0X2E, 0XBB, 0X0A, 0X03, 0X00, 0X00, 0X02, 0X33, 0X1D,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X32, 0X00, 0X00,
    0X2A, 0X1C, 0X5A, 0X94, 0XC5, 0X02, 0X07, 0X00, 0X00, 0X00,
    0XB5, 0X1F, 0X00, 0X90, 0X28, 0X00, 0X77, 0X32, 0X00, 0X62,
    0X3F, 0X00, 0X52, 0X50, 0X00, 0X52, 0X00, 0X00, 0X00, 0X00,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X0F,
    0X0F, 0X03, 0X06, 0X10, 0X42, 0XF8, 0X0F, 0X14, 0X00, 0X00,
    0X00, 0X00, 0X1A, 0X18, 0X16, 0X14, 0X12, 0X10, 0X0E, 0X0C,
    0X0A, 0X08, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
    0X00, 0X00, 0X29, 0X28, 0X24, 0X22, 0X20, 0X1F, 0X1E, 0X1D,
    0X0E, 0X0C, 0X0A, 0X08, 0X06, 0X05, 0X04, 0X02, 0X00, 0XFF,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
    0X00, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF,
    0XFF, 0XFF, 0XFF, 0XFF,
};

static void board_i2c_init(void)
{
    Pinmux_Config(TOUCH_GT911_SDA, I2C1_DAT);
    Pinmux_Config(TOUCH_GT911_SCL, I2C1_CLK);
    Pad_Config(TOUCH_GT911_SDA, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(TOUCH_GT911_SCL, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);


    Pad_Config(TOUCH_GT911_INT, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(TOUCH_GT911_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    platform_delay_ms(10);
    Pad_Config(TOUCH_GT911_INT, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    platform_delay_ms(10);
    Pad_Config(TOUCH_GT911_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    platform_delay_ms(30);
    Pad_Config(TOUCH_GT911_INT, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_DISABLE, PAD_OUT_LOW);
}

/**
  * @brief  Initialize ADC peripheral.
  * @param   No parameter.
  * @return  void
  */
static void driver_i2c_init(void)
{
    RCC_PeriphClockCmd(APBPeriph_I2C1, APBPeriph_I2C1_CLOCK, ENABLE);

    I2C_InitTypeDef  I2C_InitStructure;
    I2C_StructInit(&I2C_InitStructure);

    I2C_InitStructure.I2C_ClockSpeed = 400000;
    I2C_InitStructure.I2C_DeviveMode = I2C_DeviveMode_Master;
    I2C_InitStructure.I2C_AddressMode = I2C_AddressMode_7BIT;
    I2C_InitStructure.I2C_SlaveAddress = TOUCH_GT911_ADDR;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;

    I2C_Init(I2C1, &I2C_InitStructure);

    I2C_Cmd(I2C1, ENABLE);
}



uint8_t GT911_WR_Reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t send[198];
    send[0] = reg >> 8;
    send[1] = reg & 0xff;
    if (len > 200)
    {
        return 0;
    }
    else
    {
        memcpy(send + 2, buf, len);
        // drv_i2c0_write(TOUCH_GT911_ADDR, send, 2 + len);
        I2C_MasterWrite(I2C1, send, 2 + len);
    }

    return 0;
}

uint32_t GT911_RD_Reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t iic_write_buf[2] = {0};
    iic_write_buf[0] = reg >> 8;
    iic_write_buf[1] = reg & 0xff;

    // drv_i2c0_write(TOUCH_GT911_ADDR, iic_write_buf, 2);
    I2C_MasterWrite(I2C1, iic_write_buf, 2);

//    return drv_i2c0_read(TOUCH_GT911_ADDR, buf, len);
    if (I2C_MasterRead(I2C1, buf, len) == I2C_Success)
    {
        return len;
    }
    else
    {
        return 0;
    }
}

uint8_t GT911_Send_Cfg(uint8_t mode)
{
    uint8_t buf[2];
    uint8_t i = 0;
    buf[0] = 0;
    buf[1] = mode;
    for (i = 0; i < sizeof(GT911_CFG_TBL); i++)
    {
        buf[0] += GT911_CFG_TBL[i];
    }
    buf[0] = (~buf[0]) + 1;
    GT911_WR_Reg(GT_CFGS_REG, (uint8_t *)GT911_CFG_TBL, sizeof(GT911_CFG_TBL));
    GT911_WR_Reg(GT_CHECK_REG, buf, 2);
    return 0;
}

uint32_t GT911_read(uint8_t *buf, uint32_t len)
{
    uint8_t *read = buf;
    uint8_t temp[10] = {0};

    uint8_t mode = 0;
    uint8_t point_num = 0;
    uint8_t reset = 0;
    GT911_RD_Reg(GT_GSTID_REG, &mode, 1);
    // DBG_DIRECT("mode:%x",mode);
    read[0] = mode;
    if (mode & 0x80)
    {
        GT911_WR_Reg(GT_GSTID_REG, &reset, 1);// clear flags
    }
    else
    {
        // DBG_DIRECT("GT911_read");
        return 0;
    }
    point_num = mode & 0x0F;
    if (point_num == 0)
    {
        // DBG_DIRECT("GT911_read point_num");
        return 0;
    }

    len = GT911_RD_Reg(0x814F, read + 1, 7);

    GT911_WR_Reg(GT_GSTID_REG, &reset, 1);
    // DBG_DIRECT("len:%d",len);
    return len;
}

bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing)
{
    *pressing = false;
    uint8_t buf[10] = {0};
    *x = 480;
    *y = 480;
    static uint16_t x_old = 0;
    static uint16_t y_old = 0;
    if (GT911_read(buf, 0))
    {
        *pressing = true;
        *x = (buf[2] | (buf[3] << 8));
        *y = (buf[4] | (buf[5] << 8));

#if (TOUCH_SCREEN_TYPE == 2)
        *x = TOUCH_SCREEN_WIDTH - *x;
        *y = TOUCH_SCREEN_HIGHT - *y;
#endif

        x_old = *x;
        y_old = *y;
//        DBG_DIRECT("x:%d", x_old);
//        DBG_DIRECT("y:%d", y_old);

        return true;
    }
    else
    {
        *x = x_old;
        *y = y_old;
        return true;
    }

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
void rtk_touch_hal_init(void)
{
    DBG_DIRECT("touch_gt911_init line = %d\n", __LINE__);


//    drv_i2c0_init(TOUCH_GT911_SCL, TOUCH_GT911_SDA);
    board_i2c_init();
    driver_i2c_init();


    uint8_t iic_write_buf[2] = {0x81, 0x40};
    uint8_t iic_read_buf[5] = {0};

    // drv_i2c0_write(TOUCH_GT911_ADDR, iic_write_buf, 2);
    I2C_MasterWrite(I2C1, iic_write_buf, 2);

    // drv_i2c0_read(TOUCH_GT911_ADDR, iic_read_buf, 4);
    I2C_MasterRead(I2C1, iic_read_buf, 4);

    DBG_DIRECT("CTP ID:%x %x %x %x\r\n", iic_read_buf[0], iic_read_buf[1], iic_read_buf[2],
               iic_read_buf[3]);

    uint8_t cmd_init[1] = {2};
    GT911_WR_Reg(GT_CTRL_REG, (uint8_t *)cmd_init, 1);
    GT911_RD_Reg(GT_CFGS_REG, (uint8_t *)cmd_init, 1);
    if (cmd_init[0] < 0x60)
    {
        GT911_Send_Cfg(1);
    }
    platform_delay_us(200);
    cmd_init[0] = 0;
    GT911_WR_Reg(GT_CTRL_REG, (uint8_t *)cmd_init, 1);

    uint8_t x_low = 0;
    uint8_t x_high = 0;
    uint8_t y_low = 0;
    uint8_t y_high = 0;

    GT911_RD_Reg(0x8146, &x_low, 1);
    GT911_RD_Reg(0x8147, &x_high, 1);
    GT911_RD_Reg(0x8148, &y_low, 1);
    GT911_RD_Reg(0x8149, &y_high, 1);
    uint16_t x = x_low + (x_high << 8);
    uint16_t y = y_low + (y_high << 8);
    DBG_DIRECT("x = %d\r\n", x);
    DBG_DIRECT("y = %d\r\n", y);
}

#endif /* BSP_USING_TOUCH */
