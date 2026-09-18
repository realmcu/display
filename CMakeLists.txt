#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: Apache-2.0
#
# Build entry point for the VCDJ (VeriSilicon VC9000DJ DecoderJ) driver.
#
cmake_minimum_required(VERSION 3.10)

if(CONFIG_REALTEK_DISPLAY_ENABLE_VCDJ)

    set(HW_DISPLAY_TARGET_NAME realtek_vcdj_driver_library)

    add_library(${HW_DISPLAY_TARGET_NAME} STATIC)

    # inc/rtl_vcdj.h is the only header an integrator includes.
    target_include_directories(${HW_DISPLAY_TARGET_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    )

    set(VCDJ_CTRLSW_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src/ctrlsw/software)

    # PRIVATE: driver internals and vendor headers must not leak to linkers.
    target_include_directories(${HW_DISPLAY_TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common
        ${CMAKE_CURRENT_SOURCE_DIR}/src/device/rtl87x3k
        ${VCDJ_CTRLSW_ROOT}/source/inc
        ${VCDJ_CTRLSW_ROOT}/source/config
        ${VCDJ_CTRLSW_ROOT}/source/common
        ${VCDJ_CTRLSW_ROOT}/source/jpeg
        ${VCDJ_CTRLSW_ROOT}/source/png
    )

    target_sources(${HW_DISPLAY_TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/rtl_vcdj_hal.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/rtl_vcdj_dwl.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/rtl_vcdj_core.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/device/rtl87x3k/rtl_vcdj_port.c
    )

    # Vendor compile contract. Must stay in step with the SConscript's
    # LOCAL_CCFLAGS and Keil's --preinclude.
    target_compile_options(${HW_DISPLAY_TARGET_NAME} PRIVATE
        -include ${CMAKE_CURRENT_SOURCE_DIR}/inc/rtl_vcdj_vendor.h
    )

    # Vendor Control Software: explicit import list plus its own warning
    # suppressions.
    add_subdirectory(src/ctrlsw)

endif()
