#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: Apache-2.0
#
# Single build entry point for the VCDJ driver.
#
# The source list is EXPLICIT rather than globbed, and the recursive
# add_subdirectory template used by the other display IPs is deliberately not
# used here: the vendor tree under src/ctrlsw/ must be driven by its own explicit
# import list (see src/ctrlsw/CMakeLists.txt), and a glob would silently pick up
# whatever a vendor upgrade happens to drop in.
#
cmake_minimum_required(VERSION 3.10)

if(CONFIG_REALTEK_DISPLAY_ENABLE_VCDJ)

    # ------------------------------------------------------------------------
    # Public interface: inc/rtl_vcdj.h is the only header an integrator includes.
    # ------------------------------------------------------------------------
    target_include_directories(${HW_DISPLAY_TARGET_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    )

    # ------------------------------------------------------------------------
    # Private include paths: driver internals, the SoC parameters and the vendor
    # Control Software.
    #
    # These stay PRIVATE on purpose: neither the vendor headers nor the driver
    # internals may leak out to whoever links the display library.
    # ------------------------------------------------------------------------
    set(VCDJ_CTRLSW_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src/ctrlsw/software)

    target_include_directories(${HW_DISPLAY_TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common
        ${CMAKE_CURRENT_SOURCE_DIR}/src/device/rtl87x3k
        ${VCDJ_CTRLSW_ROOT}/source/inc
        ${VCDJ_CTRLSW_ROOT}/source/config
        ${VCDJ_CTRLSW_ROOT}/source/common
        ${VCDJ_CTRLSW_ROOT}/source/jpeg
        ${VCDJ_CTRLSW_ROOT}/source/png
    )

    # ------------------------------------------------------------------------
    # Our own sources. Four translation units:
    #
    #   src/common/rtl_vcdj_hal.c            HAL facade, codec dispatch, self test
    #   src/common/rtl_vcdj_dwl.c            everything the vendor CtrlSW calls
    #   src/common/rtl_vcdj_core.c           bounded-wait core + allocator
    #   src/device/rtl87x3k/rtl_vcdj_port.c  OSIF and SoC register access
    # ------------------------------------------------------------------------
    target_sources(${HW_DISPLAY_TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/rtl_vcdj_hal.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/rtl_vcdj_dwl.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/rtl_vcdj_core.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/device/rtl87x3k/rtl_vcdj_port.c
    )

    # ------------------------------------------------------------------------
    # Feature macros
    # ------------------------------------------------------------------------
    # Neutral feature macro for our own layer. If IRQ support is not selected,
    # the core uses bounded polling without requiring a separate polling macro.
    if(CONFIG_REALTEK_VCDJ_IRQ)
        target_compile_definitions(${HW_DISPLAY_TARGET_NAME} PRIVATE VCDJ_USE_IRQ)
    endif()

    if(CONFIG_REALTEK_VCDJ_SELFTEST)
        target_compile_definitions(${HW_DISPLAY_TARGET_NAME} PRIVATE VCDJ_ENABLE_SELFTEST)
    endif()

    foreach(vcdj_opt
            REALTEK_VCDJ_HW_TIMEOUT_MS
            REALTEK_VCDJ_COND_WAIT_TIMEOUT_MS
            REALTEK_VCDJ_OS_MUTEX_COUNT
            REALTEK_VCDJ_OS_COND_COUNT
            REALTEK_VCDJ_OS_SEM_COUNT)
        # CONFIG_REALTEK_VCDJ_FOO -> VCDJ_FOO
        string(REPLACE "REALTEK_" "" vcdj_macro ${vcdj_opt})
        if(CONFIG_${vcdj_opt})
            target_compile_definitions(${HW_DISPLAY_TARGET_NAME} PRIVATE
                ${vcdj_macro}=${CONFIG_${vcdj_opt}}
            )
        endif()
    endforeach()

    # ------------------------------------------------------------------------
    # Force one private entry header into every VCDJ translation unit so the
    # vendor feature macros and the portability shim cannot drift apart between
    # build systems.
    #
    # NOTE for Keil MDK: armcc (AC5) does not accept -include; it needs
    # --preinclude rtl_vcdj_vendor.h instead. The MDK project carries that flag
    # in its own MiscControls.
    # ------------------------------------------------------------------------
    target_compile_options(${HW_DISPLAY_TARGET_NAME} PRIVATE
        -include ${CMAKE_CURRENT_SOURCE_DIR}/inc/rtl_vcdj_vendor.h
    )

    # Vendor Control Software: explicit import list plus its own warning
    # suppressions.
    add_subdirectory(src/ctrlsw)

endif()
