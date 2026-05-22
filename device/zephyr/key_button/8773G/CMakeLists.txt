# Copyright (c) 2024 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

# Build Zephyr-style key button driver for RTL8773G.
# Mutually exclusive with the general variant in device/general/key_button/8773G/.
set(SOURCES "")

if(CONFIG_REALTEK_KEY_BUTTON_8773G_ZEPHYR)
    list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/key_button_8773g_zephyr.c")
endif()

if(SOURCES)
    target_sources(${HW_DISPLAY_TARGET_NAME} PRIVATE ${SOURCES})
    target_include_directories(${HW_DISPLAY_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
