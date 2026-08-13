/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MODULE_TOUCH_CST820_ZEPHYR_H
#define MODULE_TOUCH_CST820_ZEPHYR_H

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t x_start;
    uint16_t y_start;
    uint32_t timestamp_ms_start;
    uint32_t timestamp_ms_pressing;
    uint16_t count_pressing;
    bool is_press;
} TOUCH_DATA;

TOUCH_DATA get_raw_touch_data(const struct device *dev);

#endif // MODULE_TOUCH_CST820_ZEPHYR_H
