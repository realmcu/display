#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: Apache-2.0
#
# Build entry point for the JPU (Realtek JPEG codec) driver.
#
cmake_minimum_required(VERSION 3.10)

if(CONFIG_REALTEK_DISPLAY_ENABLE_JPU)

    set(HW_DISPLAY_TARGET_NAME realtek_jpu_driver_library)

    add_library(${HW_DISPLAY_TARGET_NAME} STATIC)

    # inc/rtl_jpu.h and inc/rtl_hal_jpu.h are what an integrator includes.
    target_include_directories(${HW_DISPLAY_TARGET_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    )

    # PRIVATE: register definitions must not leak to linkers.
    target_include_directories(${HW_DISPLAY_TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/device/rtl87x3k
    )

    target_sources(${HW_DISPLAY_TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/device/rtl_common/rtl_jpu.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/device/rtl_common/rtl_hal_jpu.c
    )

    # JPU_OS_OSIF selects the OSIF branch of inc/rtl_jpu_port.h; SCons states
    # JPU_OS_RTTHREAD instead. The rest is the Realtek RTOS SDK memory placement
    # contract, carried over from the per-SoC CMakeLists this file replaced.
    target_compile_definitions(${HW_DISPLAY_TARGET_NAME} PRIVATE
        JPU_OS_OSIF
        T_OS_MEM_TYPE=RAM_TYPE
        OS_MEM_TYPE_DATA=RAM_TYPE_DATA_ON
    )

endif()
