/*
 * Copyright (c) 2024 Nicolas Goualard <nicolas.goualard@sfr.fr>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modifications:
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 */

#define DT_DRV_COMPAT chipsemi_chsc6417

#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include "touch_CHSC6417_zephyr.h"

struct chsc6x_config
{
    struct i2c_dt_spec i2c;
    const struct gpio_dt_spec int_gpio;
    const struct gpio_dt_spec rst_gpio;
    uint32_t timeout_ms;
};

struct chsc6x_data
{
    const struct device *dev;
    struct k_work work;
    struct gpio_callback int_gpio_cb;
    TOUCH_DATA cur_point;
};

union rpt_point_t
{
    struct
    {
        unsigned char x_l8;
        unsigned char y_l8;
        unsigned char z;
        unsigned char x_h4: 4;
        unsigned char y_h4: 4;
        unsigned char id: 4;
        unsigned char event: 4;
    } rp;
    unsigned char data[5];
};

#define CHSC6X_WRITE_ADDR                   0x2c000020
#define CHSC6X_WRITE_LENGTH                 4
#define CHSC6X_READ_LENGTH                  8
#define CHSC6X_DEFAULT_TIMEOUT_MS           30

LOG_MODULE_REGISTER(chsc6417, CONFIG_LOG_DEFAULT_LEVEL);

static void touch_gesture_release_timeout(struct k_timer *timer);

K_TIMER_DEFINE(touch_gesture_release_timer, touch_gesture_release_timeout, NULL);

static int chsc6x_process(const struct device *dev, uint16_t *x, uint16_t *y, bool *pressing)
{
    uint8_t output[CHSC6X_READ_LENGTH], point_num, event_id;
    uint32_t reg_write = CHSC6X_WRITE_ADDR;
    union rpt_point_t *ppt;
    int ret;
    const struct chsc6x_config *cfg = dev->config;

    ret = i2c_write_read_dt(&cfg->i2c,
                            &reg_write, CHSC6X_WRITE_LENGTH,
                            output, CHSC6X_READ_LENGTH);
    if (ret < 0)
    {
        LOG_ERR("Could not read data: %i", ret);
        return -ENODATA;
    }

    point_num = output[1];
    ppt = (union rpt_point_t *)&output[2];

    if (point_num == 0)
    {
        return -ENODATA;
    }

    if (point_num)
    {
        for (uint8_t i = 0; i < point_num; i ++)
        {
            event_id = ppt->rp.event;
            *x = (unsigned int)(ppt->rp.x_h4 << 8) | ppt->rp.x_l8;
            *y = (unsigned int)(ppt->rp.y_h4 << 8) | ppt->rp.y_l8;
        }
        LOG_DBG("event_id = %d, x %d, y %d, point_num %d", event_id, *x, *y, point_num);
    }

    if (event_id == 8)
    {
        *pressing = true;
    }
    else
    {
        *pressing = false;
    }

    return 0;
}

static void chsc6x_work_handler(struct k_work *work)
{
    struct chsc6x_data *data = CONTAINER_OF(work, struct chsc6x_data, work);
    const struct chsc6x_config *cfg = data->dev->config;
    const struct device *dev = data->dev;
    data->cur_point.count_pressing++;
    if (data->cur_point.count_pressing <= 1)
    {
        data->cur_point.timestamp_ms_start = k_uptime_get();
        chsc6x_process(data->dev, &data->cur_point.x_start, &data->cur_point.y_start,
                       &data->cur_point.is_press);
        data->cur_point.x = data->cur_point.x_start;
        data->cur_point.y = data->cur_point.y_start;
        data->cur_point.timestamp_ms_pressing = data->cur_point.timestamp_ms_start;
    }
    else
    {
        data->cur_point.timestamp_ms_pressing = k_uptime_get();
        chsc6x_process(data->dev, &data->cur_point.x, &data->cur_point.y, &data->cur_point.is_press);
    }
    k_timer_start(&touch_gesture_release_timer, K_MSEC(cfg->timeout_ms), K_NO_WAIT);
    k_timer_user_data_set(&touch_gesture_release_timer, data);
}

static void chsc6x_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t mask)
{
    struct chsc6x_data *data = CONTAINER_OF(cb, struct chsc6x_data, int_gpio_cb);
    k_work_submit(&data->work);
}

static int chsc6x_chip_init(const struct device *dev)
{
    const struct chsc6x_config *cfg = dev->config;

    if (!i2c_is_ready_dt(&cfg->i2c))
    {
        LOG_ERR("I2C bus %s not ready", cfg->i2c.bus->name);
        return -ENODEV;
    }

    return 0;
}

static void touch_gesture_release_timeout(struct k_timer *timer)
{
    struct chsc6x_data *data = k_timer_user_data_get(timer);

    data->cur_point.count_pressing = 0;
    data->cur_point.is_press = 0;
}

static int chsc6x_init(const struct device *dev)
{
    struct chsc6x_data *data = dev->data;
    int ret;

    data->dev = dev;

    k_work_init(&data->work, chsc6x_work_handler);

    const struct chsc6x_config *config = dev->config;

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
    /* should be level trigger, if this pin need to wakeup dlps*/
    /* if trigger by falling edge, and wakeup dlps has set, then wakeup will occur continuously*/
    ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_FALLING);
    if (ret < 0)
    {
        LOG_ERR("Could not configure interrupt GPIO interrupt: %d", ret);
        return ret;
    }

    gpio_init_callback(&data->int_gpio_cb, chsc6x_isr_handler, BIT(config->int_gpio.pin));

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

    // Reset the sensor
    gpio_pin_set_dt(&config->rst_gpio, 1);
    k_busy_wait(10 * 1000);
    gpio_pin_set_dt(&config->rst_gpio, 0);
    k_busy_wait(10 * 1000);
    gpio_pin_set_dt(&config->rst_gpio, 1);
    k_busy_wait(50 * 1000);

    LOG_INF("chsc6x_init done");

    return chsc6x_chip_init(dev);
};

TOUCH_DATA get_raw_touch_data(const struct device *dev)
{
    struct chsc6x_data *data = dev->data;

    if (!device_is_ready(dev))
    {
        LOG_ERR("Device %s not ready", dev->name);
        return (TOUCH_DATA) {0};
    }

    return data->cur_point;
}

#define CHSC6X_DEFINE(index)                                                                       \
    static const struct chsc6x_config chsc6x_config_##index = {                                \
        .i2c = I2C_DT_SPEC_INST_GET(index),                                                \
               .int_gpio = GPIO_DT_SPEC_INST_GET(index, irq_gpios),                               \
                           .rst_gpio = GPIO_DT_SPEC_INST_GET(index, rst_gpios),                               \
                                       .timeout_ms = DT_INST_PROP_OR(index, gesture_release_timeout_ms, CHSC6X_DEFAULT_TIMEOUT_MS),            \
    };                                                                                         \
    static struct chsc6x_data chsc6x_data_##index;                                             \
    DEVICE_DT_INST_DEFINE(index, chsc6x_init, NULL, &chsc6x_data_##index,                      \
                          &chsc6x_config_##index, POST_KERNEL, 60,     \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(CHSC6X_DEFINE)
