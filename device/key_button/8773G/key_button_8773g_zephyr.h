/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modifications:
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 */

#ifndef __KEY_BUTTON_8773G_ZEPHYR_H
#define __KEY_BUTTON_8773G_ZEPHYR_H

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    GPIO_KEY_INVALID,
    GPIO_KEY_PRESSED,
    GPIO_KEY_RELEASED,
} T_GPIO_KEY_STATE;

typedef struct
{
    uint32_t key_id;
    T_GPIO_KEY_STATE current_state;
    uint32_t press_timestamp;
    uint32_t release_timestamp;
} T_GPIO_KEY;

typedef void (*T_GPIO_KEY_CALLBACK)(T_GPIO_KEY *state);

/*
 * @brief Read the state of a GPIO key.
 *
 * @param dev The device structure for the driver instance.
 * @param key The key ID to read.
 * @return T_GPIO_KEY The state of the GPIO key.
 */
T_GPIO_KEY gpio_button_read_key(const struct device *dev, uint16_t key);

/**
 * @brief Register a callback for a GPIO key. Callback runs on sys work thread.
 *
 * @param dev The device structure for the driver instance.
 * @param key The key ID to register the callback for.
 * @param callback The callback function to be called when the key state changes.
 * @return int32_t 0 on success, negative error code on failure.
 */
int32_t gpio_button_register_callback(const struct device *dev, uint16_t key,
                                      T_GPIO_KEY_CALLBACK callback);


#ifdef __cplusplus
}
#endif

#endif /* __KEY_BUTTON_8773G_ZEPHYR_H */
