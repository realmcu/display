# Copyright (c) 2024 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

# Build key button driver for RTL8773E
set(SOURCES "")

if(CONFIG_REALTEK_KEY_BUTTON)
    list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/key_button_8773e.c")
endif()

if(SOURCES)
    target_sources(${HW_DISPLAY_TARGET_NAME} PRIVATE ${SOURCES})
    target_include_directories(${HW_DISPLAY_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
