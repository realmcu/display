# Copyright (c) 2024 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

# SOC series to chip directory mapping
# CONFIG_SOC_SERIES_RTL87X3G -> 8773G, CONFIG_SOC_SERIES_RTL87X3E -> 8773E, etc.
set(SOC_CHIP_PAIRS
    "CONFIG_SOC_SERIES_RTL87X3G;8773G"
    "CONFIG_SOC_SERIES_RTL87X3E;8773E"
    "CONFIG_SOC_SERIES_RTL87X2G;8762G"
    "CONFIG_SOC_SERIES_RTL87X2F;8772F"
    "CONFIG_SOC_SERIES_RTL87X2D;8762D"
    "CONFIG_SOC_SERIES_RTL87X2E;8762E"
    "CONFIG_SOC_SERIES_RTL87X2H;8752H"
)

# Device type subdirectories
set(DEVICE_TYPES lcd touch knob key_button)

foreach(pair ${SOC_CHIP_PAIRS})
    list(GET pair 0 soc_config)
    list(GET pair 1 chip_dir)

    if(${soc_config})
        foreach(dev_type ${DEVICE_TYPES})
            # e.g. CONFIG_REALTEK_8773G_DEVICE_LCD
            string(TOUPPER ${dev_type} dev_type_upper)
            set(dev_config "CONFIG_REALTEK_${chip_dir}_DEVICE_${dev_type_upper}")

            if(${dev_config})
                set(sub_path "${CMAKE_CURRENT_SOURCE_DIR}/${dev_type}/${chip_dir}")
                if(EXISTS "${sub_path}/CMakeLists.txt")
                    add_subdirectory("${dev_type}/${chip_dir}")
                endif()
            endif()
        endforeach()
    endif()
endforeach()
