/*
 * Copyright (c) 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modifications:
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 */

#define DT_DRV_COMPAT realtek_gpio_keys

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include "key_button_8773g_zephyr.h"

LOG_MODULE_REGISTER(gpio_keys, CONFIG_LOG_DEFAULT_LEVEL);

typedef enum
{
    GPIO_KEY_PULL_NONE,
    GPIO_KEY_PULL_UP,
    GPIO_KEY_PULL_DOWN,
} gpio_keys_pull;

struct gpio_keys_callback
{
    struct gpio_callback gpio_cb;
};

struct gpio_keys_pin_config
{
    /** GPIO specification from devicetree */
    struct gpio_dt_spec spec;
    /** Zephyr code from devicetree */
    uint32_t key_id;
};

struct gpio_keys_pin_data
{
    const struct device *dev;
    struct gpio_keys_callback cb_data;
    struct k_work_delayable work;
    T_GPIO_KEY report_pin_state;
    T_GPIO_KEY_CALLBACK user_callback;
};

struct gpio_keys_config
{
    /** Debounce interval in milliseconds from devicetree */
    uint32_t debounce_interval_ms;
    const int num_keys;
    const struct gpio_keys_pin_config *pin_cfg;
    struct gpio_keys_pin_data *pin_data;
    k_work_handler_t handler;
};

/**
 * Handle debounced gpio pin state.
 */
static void gpio_keys_poll_pin(const struct device *dev, int key_index)
{
    const struct gpio_keys_config *cfg = dev->config;
    const struct gpio_keys_pin_config *pin_cfg = &cfg->pin_cfg[key_index];
    struct gpio_keys_pin_data *pin_data = &cfg->pin_data[key_index];
    int pressed;

    pressed = gpio_pin_get_dt(&pin_cfg->spec);

    LOG_DBG("%s: pressed=%d, key_index=%d, pin=%d", dev->name,
            pressed, key_index, pin_cfg->spec.pin);

    /* RELEASED */
    if (pressed <= 0)
    {
        pin_data->report_pin_state.current_state = GPIO_KEY_RELEASED;
        pin_data->report_pin_state.release_timestamp = k_uptime_get_32();

        gpio_pin_interrupt_configure_dt(&pin_cfg->spec, GPIO_INT_LEVEL_ACTIVE);

        if (pin_data->user_callback != NULL)
        {
            pin_data->user_callback(&pin_data->report_pin_state);
        }

        LOG_DBG("Report event %s pressed=%d, code=%d, release_timestamp=%d", dev->name, pressed,
                pin_cfg->key_id, pin_data->report_pin_state.release_timestamp);
    }
    /* PRESSED */
    else
    {
        pin_data->report_pin_state.current_state = GPIO_KEY_PRESSED;
        pin_data->report_pin_state.press_timestamp = k_uptime_get_32();

        if (pin_data->user_callback != NULL)
        {
            pin_data->user_callback(&pin_data->report_pin_state);
        }

        gpio_pin_interrupt_configure_dt(&pin_cfg->spec, GPIO_INT_LEVEL_INACTIVE);
        LOG_DBG("Report event %s pressed=%d, code=%d, press_timestamp=%d", dev->name, pressed,
                pin_cfg->key_id, pin_data->report_pin_state.press_timestamp);
    }

    gpio_pin_interrupt_configure_dt(&pin_cfg->spec, GPIO_INT_MODE_ENABLE_ONLY);
}

static __maybe_unused void gpio_keys_change_deferred(struct k_work *work)
{
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct gpio_keys_pin_data *pin_data = CONTAINER_OF(dwork, struct gpio_keys_pin_data, work);
    const struct device *dev = pin_data->dev;
    const struct gpio_keys_config *cfg = dev->config;
    int key_index = pin_data - (struct gpio_keys_pin_data *)cfg->pin_data;

    gpio_keys_poll_pin(dev, key_index);
}

static void gpio_keys_interrupt(const struct device *dev, struct gpio_callback *cbdata,
                                uint32_t pins)
{
    struct gpio_keys_callback *keys_cb = CONTAINER_OF(
                                             cbdata, struct gpio_keys_callback, gpio_cb);
    struct gpio_keys_pin_data *pin_data = CONTAINER_OF(
                                              keys_cb, struct gpio_keys_pin_data, cb_data);
    const struct gpio_keys_config *cfg = pin_data->dev->config;
    int key_index = pin_data - (struct gpio_keys_pin_data *)cfg->pin_data;

    gpio_pin_interrupt_configure_dt(&cfg->pin_cfg[key_index].spec, GPIO_INT_MODE_DISABLE_ONLY);

    ARG_UNUSED(dev); /* GPIO device pointer. */
    ARG_UNUSED(pins);

    LOG_DBG("dev 0x%x pin_data->dev 0x%x key_index %d", (uint32_t)dev, (uint32_t)pin_data->dev,
            key_index);

    k_work_reschedule(&pin_data->work, K_MSEC(cfg->debounce_interval_ms));
}


static int gpio_keys_init(const struct device *dev)
{
    const struct gpio_keys_config *cfg = dev->config;
    struct gpio_keys_pin_data *pin_data = cfg->pin_data;
    int ret;

    for (int i = 0; i < cfg->num_keys; i++)
    {
        const struct gpio_dt_spec *gpio = &cfg->pin_cfg[i].spec;

        if (!gpio_is_ready_dt(gpio))
        {
            LOG_ERR("%s is not ready", gpio->port->name);
            return -ENODEV;
        }

        ret = gpio_pin_configure_dt(gpio, GPIO_INPUT);
        if (ret != 0)
        {
            LOG_ERR("Pin %d configuration failed: %d", i, ret);
            return ret;
        }

        pin_data[i].dev = dev;
        k_work_init_delayable(&pin_data[i].work, cfg->handler);

        gpio_init_callback(&pin_data[i].cb_data.gpio_cb, gpio_keys_interrupt, BIT(gpio->pin));

        ret = gpio_add_callback(gpio->port, &pin_data[i].cb_data.gpio_cb);
        if (ret < 0)
        {
            LOG_ERR("Could not set gpio callback");
            return ret;
        }

        pin_data[i].report_pin_state.key_id = cfg->pin_cfg[i].key_id;
        pin_data[i].report_pin_state.current_state = GPIO_KEY_INVALID;
        pin_data[i].report_pin_state.press_timestamp = 0;
        pin_data[i].report_pin_state.release_timestamp = 0;

        LOG_DBG("port=%s, pin=%d, key_id=%d", gpio->port->name, gpio->pin,
                cfg->pin_cfg[i].key_id);

        ret = gpio_pin_interrupt_configure_dt(gpio, GPIO_INT_LEVEL_ACTIVE);
        if (ret < 0)
        {
            LOG_ERR("interrupt configuration failed: %d", ret);
            return ret;
        }
    }

    return 0;
}

T_GPIO_KEY gpio_button_read_key(const struct device *dev, uint16_t key)
{
    const struct gpio_keys_config *cfg = dev->config;
    struct gpio_keys_pin_data *pin_data = cfg->pin_data;
    T_GPIO_KEY key_state = {0};

    if (!device_is_ready(dev))
    {
        return key_state;
    }

    for (uint8_t i = 0; i < cfg->num_keys; i++)
    {
        if (pin_data[i].report_pin_state.key_id == key)
        {
            return pin_data[i].report_pin_state;
        }
    }

    return key_state;
}

int32_t gpio_button_register_callback(const struct device *dev, uint16_t key,
                                      T_GPIO_KEY_CALLBACK callback)
{
    const struct gpio_keys_config *cfg = dev->config;
    struct gpio_keys_pin_data *pin_data = cfg->pin_data;

    if (!device_is_ready(dev))
    {
        return -ENODEV;
    }

    for (uint8_t i = 0; i < cfg->num_keys; i++)
    {
        if (pin_data[i].report_pin_state.key_id == key)
        {
            pin_data[i].user_callback = callback;
            return 0;
        }
    }

    return -EINVAL;
}

#define GPIO_KEYS_CFG_CHECK(node_id)                                                               \
    BUILD_ASSERT(DT_NODE_HAS_PROP(node_id, key_id),                                       \
                 "zephyr-code must be specified to use the input-gpio-keys driver");

#define GPIO_KEYS_CFG_DEF(node_id)                                                                 \
    {                                                                                          \
        .spec = GPIO_DT_SPEC_GET(node_id, gpios),                                          \
                .key_id = DT_PROP(node_id, key_id),                                      \
    }

#define GPIO_KEYS_INIT(i)                                                                          \
    DT_INST_FOREACH_CHILD_STATUS_OKAY(i, GPIO_KEYS_CFG_CHECK);                                 \
    \
    static const struct gpio_keys_pin_config gpio_keys_pin_config_##i[] = {                    \
        DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(i, GPIO_KEYS_CFG_DEF, (,))};                 \
    \
    static struct gpio_keys_pin_data                                                           \
    gpio_keys_pin_data_##i[ARRAY_SIZE(gpio_keys_pin_config_##i)];                      \
    \
    static const struct gpio_keys_config gpio_keys_config_##i = {                              \
        .debounce_interval_ms = DT_INST_PROP(i, debounce_interval_ms),                     \
                                .num_keys = ARRAY_SIZE(gpio_keys_pin_config_##i),                                  \
                                            .pin_cfg = gpio_keys_pin_config_##i,                                               \
                                                       .pin_data = gpio_keys_pin_data_##i,                                                \
                                                                   .handler = gpio_keys_change_deferred,                                              \
    };                                                                                         \
    \
    DEVICE_DT_INST_DEFINE(i, &gpio_keys_init, NULL,                        \
                          NULL, &gpio_keys_config_##i,                          \
                          POST_KERNEL, 60, NULL);

DT_INST_FOREACH_STATUS_OKAY(GPIO_KEYS_INIT)


