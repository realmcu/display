# Copyright (c) 2024 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

# Dispatcher: general drivers always; Zephyr-style drivers only under Zephyr build.
add_subdirectory(general)

if(ZEPHYR_BASE)
    add_subdirectory(zephyr)
endif()
