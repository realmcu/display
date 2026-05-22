# Copyright (c) 2024 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

# Build general key button driver for RTL8773G.
# When CONFIG_REALTEK_KEY_BUTTON_8773G_ZEPHYR is set, the Zephyr-style
# variant is built from device/zephyr/key_button/8773G/ instead (mutually exclusive).
set(SOURCES "")

if(CONFIG_REALTEK_KEY_BUTTON_8773G_ZEPHYR)
    # Yield to device/zephyr/key_button/8773G/.
elseif(CONFIG_REALTEK_KEY_BUTTON)
    list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/key_button_8773g.c")
endif()

if(SOURCES)
    target_sources(${HW_DISPLAY_TARGET_NAME} PRIVATE ${SOURCES})
    target_include_directories(${HW_DISPLAY_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
