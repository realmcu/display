/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hynitron_cst820

#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include "touch_cst820_zephyr.h"

struct cst820_config
{
    struct i2c_dt_spec i2c;
    const struct gpio_dt_spec int_gpio;
    const struct gpio_dt_spec rst_gpio;
    const struct gpio_dt_spec pwr_en_gpio;
    const struct gpio_dt_spec pwr_gpio;
    uint32_t timeout_ms;
};

struct cst820_data
{
    const struct device *dev;
    struct k_work work;
    struct gpio_callback int_gpio_cb;
    TOUCH_DATA cur_point;
};

#define CST820_REG_DATA             0x00
#define CST820_REG_CHIP_ID          0xA7
#define CST820_READ_LENGTH          24
#define CST820_DEFAULT_TIMEOUT_MS   30
/* Settle time after asserting the supply rail, before touching reset. */
#define CST820_PWR_ON_DELAY_MS      10
/* Settle time after asserting the board-level rail, before the sensor rail. */
#define CST820_PWR_EN_DELAY_MS      5

LOG_MODULE_REGISTER(cst820, CONFIG_LOG_DEFAULT_LEVEL);

static void touch_gesture_release_timeout(struct k_timer *timer);

K_TIMER_DEFINE(touch_gesture_release_timer, touch_gesture_release_timeout, NULL);

static int cst820_process(const struct device *dev, uint16_t *x, uint16_t *y, bool *pressing)
{
    uint8_t reg = CST820_REG_DATA;
    uint8_t data[CST820_READ_LENGTH];
    int ret;
    const struct cst820_config *cfg = dev->config;

    ret = i2c_write_read_dt(&cfg->i2c, &reg, 1, data, CST820_READ_LENGTH);
    if (ret < 0)
    {
        LOG_ERR("Could not read data: %i", ret);
        return ret;
    }

    *pressing = (data[3] >> 6 == 2);
    *x = (uint16_t)(((data[3] & 0x0f) << 8) | data[4]);
    *y = (uint16_t)(((data[5] & 0x0f) << 8) | data[6]);

    LOG_DBG("pressing %d, x %d, y %d", *pressing, *x, *y);

    return 0;
}

static int cst820_read_chip_id(const struct device *dev)
{
    uint8_t reg = CST820_REG_CHIP_ID;
    uint8_t chip_id = 0;
    int ret;
    const struct cst820_config *cfg = dev->config;

    ret = i2c_write_read_dt(&cfg->i2c, &reg, 1, &chip_id, 1);
    if (ret < 0)
    {
        LOG_ERR("Could not read chip id: %i", ret);
        return ret;
    }

    LOG_INF("cst820 chip id: 0x%02x", chip_id);
    return 0;
}

static void cst820_work_handler(struct k_work *work)
{
    struct cst820_data *data = CONTAINER_OF(work, struct cst820_data, work);
    const struct cst820_config *cfg = data->dev->config;

    data->cur_point.count_pressing++;
    if (data->cur_point.count_pressing <= 1)
    {
        data->cur_point.timestamp_ms_start = k_uptime_get();
        cst820_process(data->dev, &data->cur_point.x_start, &data->cur_point.y_start,
                       &data->cur_point.is_press);
        data->cur_point.x = data->cur_point.x_start;
        data->cur_point.y = data->cur_point.y_start;
        data->cur_point.timestamp_ms_pressing = data->cur_point.timestamp_ms_start;
    }
    else
    {
        data->cur_point.timestamp_ms_pressing = k_uptime_get();
        cst820_process(data->dev, &data->cur_point.x, &data->cur_point.y,
                       &data->cur_point.is_press);
    }
    k_timer_start(&touch_gesture_release_timer, K_MSEC(cfg->timeout_ms), K_NO_WAIT);
    k_timer_user_data_set(&touch_gesture_release_timer, data);
}

static void cst820_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t mask)
{
    struct cst820_data *data = CONTAINER_OF(cb, struct cst820_data, int_gpio_cb);
    k_work_submit(&data->work);
}

static void touch_gesture_release_timeout(struct k_timer *timer)
{
    struct cst820_data *data = k_timer_user_data_get(timer);

    data->cur_point.count_pressing = 0;
    data->cur_point.is_press = 0;
}

static int cst820_init(const struct device *dev)
{
    struct cst820_data *data = dev->data;
    const struct cst820_config *config = dev->config;
    int ret;

    data->dev = dev;
    k_work_init(&data->work, cst820_work_handler);

    /* Power the sensor rail before anything else: this init runs at
     * POST_KERNEL prio 60, i.e. before main(), so nothing in the
     * application has enabled the supply yet. pwr-en is the board-level
     * rail and comes up first, then the sensor's own supply.
     */
    if (config->pwr_en_gpio.port != NULL)
    {
        if (!gpio_is_ready_dt(&config->pwr_en_gpio))
        {
            LOG_ERR("GPIO port %s not ready", config->pwr_en_gpio.port->name);
            return -ENODEV;
        }

        ret = gpio_pin_configure_dt(&config->pwr_en_gpio, GPIO_OUTPUT_ACTIVE);
        if (ret < 0)
        {
            LOG_ERR("Could not configure main supply GPIO pin: %d", ret);
            return ret;
        }

        k_busy_wait(CST820_PWR_EN_DELAY_MS * 1000);
    }

    if (config->pwr_gpio.port != NULL)
    {
        if (!gpio_is_ready_dt(&config->pwr_gpio))
        {
            LOG_ERR("GPIO port %s not ready", config->pwr_gpio.port->name);
            return -ENODEV;
        }

        ret = gpio_pin_configure_dt(&config->pwr_gpio, GPIO_OUTPUT_ACTIVE);
        if (ret < 0)
        {
            LOG_ERR("Could not configure power GPIO pin: %d", ret);
            return ret;
        }

        k_busy_wait(CST820_PWR_ON_DELAY_MS * 1000);
    }

    if (!i2c_is_ready_dt(&config->i2c))
    {
        LOG_ERR("I2C bus %s not ready", config->i2c.bus->name);
        return -ENODEV;
    }

    if (!gpio_is_ready_dt(&config->int_gpio))
    {
        LOG_ERR("GPIO port %s not ready", config->int_gpio.port->name);
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT | GPIO_PULL_UP);
    if (ret < 0)
    {
        LOG_ERR("Could not configure interrupt GPIO pin: %d", ret);
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_FALLING);
    if (ret < 0)
    {
        LOG_ERR("Could not configure interrupt GPIO interrupt: %d", ret);
        return ret;
    }

    gpio_init_callback(&data->int_gpio_cb, cst820_isr_handler, BIT(config->int_gpio.pin));

    ret = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
    if (ret < 0)
    {
        LOG_ERR("Could not set gpio callback: %d", ret);
        return ret;
    }

    if (!gpio_is_ready_dt(&config->rst_gpio))
    {
        LOG_ERR("GPIO port %s not ready", config->rst_gpio.port->name);
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&config->rst_gpio, GPIO_OUTPUT_ACTIVE);
    if (ret < 0)
    {
        LOG_ERR("Could not configure reset GPIO pin: %d", ret);
        return ret;
    }


    gpio_pin_set_dt(&config->rst_gpio, 1);
    k_busy_wait(120 * 1000);

    gpio_pin_set_dt(&config->rst_gpio, 0);
    k_busy_wait(10 * 1000);
    gpio_pin_set_dt(&config->rst_gpio, 1);
    k_busy_wait(120 * 1000);



    (void)cst820_read_chip_id(dev);

    LOG_INF("cst820_init done");

    return 0;
}

TOUCH_DATA get_raw_touch_data(const struct device *dev)
{
    struct cst820_data *data = dev->data;

    if (!device_is_ready(dev))
    {
        LOG_ERR("Device %s not ready", dev->name);
        return (TOUCH_DATA) {0};
    }

    return data->cur_point;
}

#define CST820_DEFINE(index)                                                                       \
    static const struct cst820_config cst820_config_##index = {                                    \
        .i2c = I2C_DT_SPEC_INST_GET(index),                                                        \
               .int_gpio = GPIO_DT_SPEC_INST_GET(index, irq_gpios),                                       \
                           .rst_gpio = GPIO_DT_SPEC_INST_GET(index, rst_gpios),                                       \
                                       .pwr_en_gpio = GPIO_DT_SPEC_INST_GET_OR(index, pwr_en_gpios, {0}),                         \
                                                      .pwr_gpio = GPIO_DT_SPEC_INST_GET_OR(index, pwr_gpios, {0}),                               \
                                                                  .timeout_ms = DT_INST_PROP_OR(index, gesture_release_timeout_ms,                           \
                                                                                CST820_DEFAULT_TIMEOUT_MS),                                  \
    };                                                                                             \
    static struct cst820_data cst820_data_##index;                                                 \
    DEVICE_DT_INST_DEFINE(index, cst820_init, NULL, &cst820_data_##index,                          \
                          &cst820_config_##index, POST_KERNEL, 60, NULL);

DT_INST_FOREACH_STATUS_OKAY(CST820_DEFINE)
