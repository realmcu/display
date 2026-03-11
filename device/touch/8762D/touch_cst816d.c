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
#include "touch_cst816d.h"
#include "drv_gpio.h"
#include "drv_i2c.h"
#include "drv_touch.h"
#include "trace.h"
#include "board.h"
#include "os_mem.h"
#include "cst816d_update.txt"

void touch_write(uint16_t slave_address, uint16_t reg, uint8_t reg_len, uint8_t *data,
                 uint16_t data_len);
void touch_read(uint16_t slave_address, uint16_t reg, uint8_t reg_len, uint8_t *data,
                uint16_t data_len);

static bool cst816d_update_flag = false;

static bool cst816d_enter_bootmode(void)
{
    uint8_t retry_cnt = 50;

    drv_pin_write(TOUCH_CST816D_RST, 0);
    platform_delay_ms(10);
    drv_pin_write(TOUCH_CST816D_RST, 1);
    platform_delay_ms(10);

    while (retry_cnt--)
    {
        uint8_t cmd = 0xab;

        touch_write(TOUCH_UPGRADE_MODE_SLAVE_ADDRESS, 0xa001, 2, &cmd, 1);
        platform_delay_ms(2);
        touch_read(TOUCH_UPGRADE_MODE_SLAVE_ADDRESS, 0xa003, 2, &cmd, 1);
        platform_delay_ms(2);

        if (cmd != 0xc1)
        {
            platform_delay_ms(2);
        }
        else
        {
            APP_PRINT_INFO0("cst816d_enter_bootmode: success!");
            return true;
        }
    }
    return false;
}

static uint16_t cst816d_read_checksum(void)
{
    uint16_t check_sum = 0;
    uint8_t buf[2] = {0, 0};
    uint8_t cmd = 0;

    if (cst816d_enter_bootmode() == false)
    {
        APP_PRINT_ERROR0("cst816d_read_checksum: enter bootmode fail!");
        return 0;
    }

    touch_write(TOUCH_UPGRADE_MODE_SLAVE_ADDRESS, 0xa003, 2, &cmd, 1);
    platform_delay_ms(500);
    touch_read(TOUCH_UPGRADE_MODE_SLAVE_ADDRESS, 0xa008, 2, buf, 2);
    check_sum = buf[1];
    check_sum <<= 8;
    check_sum |= buf[0];
    APP_PRINT_INFO1("cst816d_read_checksum: 0x%x", check_sum);

    return check_sum;
}

static bool cst816d_update(uint16_t start_addr, uint16_t len, uint8_t *data)
{
    uint32_t sum_len = 0;
    uint8_t cmd[2] = {0, 0};
    uint32_t count = len / 512;

    if (cst816d_enter_bootmode() == false)
    {
        APP_PRINT_ERROR0("cst816d_update: enter bootmode fail!");
        return false;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        if (sum_len >= len)
        {
            return false;
        }

        cmd[0] = start_addr & 0xff;
        cmd[1] = start_addr >> 8;
        touch_write(TOUCH_UPGRADE_MODE_SLAVE_ADDRESS, 0xa014, 2, cmd, 2);
        platform_delay_ms(2);
        touch_write(TOUCH_UPGRADE_MODE_SLAVE_ADDRESS, 0xa018, 2, (uint8_t *)data, 512);
        platform_delay_ms(4);

        cmd[0] = 0xee;
        touch_write(TOUCH_UPGRADE_MODE_SLAVE_ADDRESS, 0xa004, 2, cmd, 1);
        platform_delay_ms(100);
        //WDG_Kick();

        uint8_t retry_cnt = 50;
        while (retry_cnt--)
        {
            cmd[0] = 0;
            touch_read(TOUCH_UPGRADE_MODE_SLAVE_ADDRESS, 0xa005, 2, cmd, 1);
            if (cmd[0] == 0x55)
            {
                cmd[0] = 0;
                break;
            }
            platform_delay_ms(10);
        }

        start_addr += 512;
        data += 512;
        sum_len += 512;
    }
    return true;
}

static bool cst816d_upgrade_firmware(void)
{
    if (cst816d_enter_bootmode() == true)
    {
        if (sizeof(cst816d_update_bin) > 10)
        {
            uint16_t start_addr = cst816d_update_bin[1];
            uint16_t length = cst816d_update_bin[3];
            uint16_t check_sum = cst816d_update_bin[5];

            start_addr <<= 8;
            start_addr |= cst816d_update_bin[0];
            length <<= 8;
            length |= cst816d_update_bin[2];
            check_sum <<= 8;
            check_sum |= cst816d_update_bin[4];
            APP_PRINT_INFO1("update check_sum: 0x%x", check_sum);

            if (cst816d_read_checksum() != check_sum)
            {
                cst816d_update(start_addr, length, (uint8_t *)&cst816d_update_bin[6]);
                if (cst816d_read_checksum() == check_sum)
                {
                    APP_PRINT_INFO0("cst816d_upgrade_firmware: upgrade success!");
                    return true;
                }
            }
            else
            {
                APP_PRINT_INFO0("cst816d_upgrade_firmware: checksum consistent, no upgrade required!");
                return false;
            }
        }
    }
    return false;
}


void touch_write(uint16_t slave_address, uint16_t reg, uint8_t reg_len, uint8_t *data,
                 uint16_t data_len)
{
    uint8_t *I2C_WriteBuf = os_mem_alloc(RAM_TYPE_DATA_ON, reg_len + data_len);
    if (reg_len == 2)
    {
        I2C_WriteBuf[0] = reg >> 8;
        I2C_WriteBuf[1] = reg;
    }
    else
    {
        I2C_WriteBuf[0] = reg;
    }
    memcpy(&I2C_WriteBuf[reg_len], data, data_len);
    I2C_SetSlaveAddress(TOUCH_I2C_BUS, slave_address);
    I2C_Status res = I2C_MasterWrite(TOUCH_I2C_BUS, I2C_WriteBuf, reg_len + data_len);
    if (res != I2C_Success)
    {
        APP_PRINT_ERROR1("touch_write: master write fail %d", res);
    }
    os_mem_free(I2C_WriteBuf);
}

void touch_read(uint16_t slave_address, uint16_t reg, uint8_t reg_len, uint8_t *data,
                uint16_t data_len)
{
    uint8_t I2C_WriteBuf[2] = {0, 0};
    if (reg_len == 2)
    {
        I2C_WriteBuf[0] = reg >> 8;
        I2C_WriteBuf[1] = reg;
    }
    else
    {
        I2C_WriteBuf[0] = reg;
    }

    I2C_SetSlaveAddress(TOUCH_I2C_BUS, slave_address);
    I2C_Status res = I2C_MasterWrite(TOUCH_I2C_BUS, I2C_WriteBuf, reg_len);
    if (res != I2C_Success)
    {
        APP_PRINT_ERROR1("touch_read: master write fail error %d", res);
    }
    platform_delay_us(1);
    res = I2C_MasterRead(TOUCH_I2C_BUS, data, data_len);
    if (res != I2C_Success)
    {
        APP_PRINT_ERROR1("touch_read: master read fail error %d", res);
    }
}

bool rtk_touch_hal_read_all(uint16_t *x, uint16_t *y, bool *pressing)
{
    uint8_t iic_write_buf[1] = {0};
    uint8_t buf[24] = {0};

    drv_i2c1_write(TOUCH_WORK_MODE_SLAVE_ADDRESS, iic_write_buf, 1);
    platform_delay_us(100);

    drv_i2c1_read(TOUCH_WORK_MODE_SLAVE_ADDRESS, buf, 24);

    if (buf[3] >> 6 == 2)
    {
        *pressing = true;
    }
    else
    {
        *pressing = false;
    }

    *x = 385 - (((buf[5] & 0x0f) << 8) | buf[6]);
    *y = (((buf[3] & 0x0f) << 8) | buf[4]);

//    *x = (((buf[3] & 0x0f) << 8) | buf[4]);
//    *y = (((buf[5] & 0x0f) << 8) | buf[6]);

    return true;
}

void rtk_touch_hal_set_indicate(void (*indicate)(void *))
{
    drv_pin_mode(TOUCH_CST816D_INT, PIN_MODE_INPUT);
    drv_pin_attach_irq(TOUCH_CST816D_INT, PIN_IRQ_MODE_RISING_FALLING, indicate,
                       NULL);
    drv_pin_irq_enable(TOUCH_CST816D_INT, PIN_IRQ_ENABLE);
}

void rtk_touch_hal_int_config(bool enable)
{
    if (enable == true)
    {
        drv_pin_irq_enable(TOUCH_CST816D_INT, PIN_IRQ_ENABLE);
    }
    else
    {
        drv_pin_irq_enable(TOUCH_CST816D_INT, PIN_IRQ_DISABLE);
    }
}

static bool touch_enter_dlps(void)
{
    drv_pin_mode(TOUCH_CST816D_INT, PIN_MODE_INPUT);
    Pad_Config(TOUCH_CST816D_INT, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    System_WakeUpPinEnable(TOUCH_CST816D_INT, PAD_WAKEUP_POL_LOW, PAD_WK_DEBOUNCE_DISABLE);
    DBG_DIRECT("touch_enter_dlps");
    return true;
}

static bool touch_exit_dlps(void)
{
    drv_i2c1_init(TOUCH_CST816D_SCL, TOUCH_CST816D_SDA);
    Pad_Config(TOUCH_CST816D_INT, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
               PAD_OUT_LOW);
    DBG_DIRECT("touch_exit_dlps");
    return true;
}

bool TOUCH_ALLOW_DLPS = false;
static bool touch_allowed_enter_dlps_check(void)
{
    return TOUCH_ALLOW_DLPS;
}

static bool touch_system_wakeup_dlps_check(void)
{
    if (System_WakeUpInterruptValue(TOUCH_CST816D_INT) == SET)
    {
        Pad_ClearWakeupINTPendingBit(TOUCH_CST816D_INT);
        System_WakeUpPinDisable(TOUCH_CST816D_INT);
        DBG_DIRECT("Touch Wake up");
#ifdef ENABLE_RTK_GUI
        extern void gui_indev_wakeup(void);
        gui_indev_wakeup();
#endif
        return true;
    }
    return false;
}

void drv_touch_dlps_init(void)
{
    System_WakeUpPinEnable(TOUCH_CST816D_INT, PAD_WAKEUP_POL_LOW, PAD_WK_DEBOUNCE_DISABLE);
    drv_dlps_exit_cbacks_register("touch", touch_exit_dlps);
    drv_dlps_enter_cbacks_register("touch", touch_enter_dlps);
    drv_dlps_wakeup_cbacks_register("touch", touch_system_wakeup_dlps_check);
    drv_dlps_check_cbacks_register("touch", touch_allowed_enter_dlps_check);
}

static void touch_get_chip_id(uint8_t chip_id)
{
    touch_read(TOUCH_WORK_MODE_SLAVE_ADDRESS, 0xa7, 1, &chip_id, 1);
    APP_PRINT_INFO1("touch_get_chip_id: 0x%x", chip_id);
}

void rtk_touch_hal_init(void)
{
    drv_pin_mode(TOUCH_CST816D_RST, PIN_MODE_OUTPUT);
    drv_pin_write(TOUCH_CST816D_RST, 0);
    platform_delay_ms(10);
    drv_pin_write(TOUCH_CST816D_RST, 1);
    platform_delay_ms(100);
    drv_i2c1_init(TOUCH_CST816D_SCL, TOUCH_CST816D_SDA);

    if (cst816d_update_flag == false)
    {
        cst816d_upgrade_firmware();
        cst816d_update_flag = true;
    }

    drv_pin_write(TOUCH_CST816D_RST, 0);
    platform_delay_ms(10);
    drv_pin_write(TOUCH_CST816D_RST, 1);
    platform_delay_ms(120);

    uint8_t chip_id = 0;
    touch_get_chip_id(chip_id);

#ifdef RTK_HAL_DLPS
    drv_touch_dlps_init();
#endif
}

