# Copyright (c) 2024 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

# SOC series to chip directory mapping
set(SOC_CHIP_PAIRS
    "CONFIG_SOC_SERIES_RTL87X3G|8773G"
    "CONFIG_SOC_SERIES_RTL8773E|8773E"
    "CONFIG_SOC_SERIES_RTL87X3E|8773E"
    "CONFIG_SOC_SERIES_RTL87X2G|8762G"
    "CONFIG_SOC_SERIES_RTL87X2F|8772F"
    "CONFIG_SOC_SERIES_RTL87X2D|8762D"
    "CONFIG_SOC_SERIES_RTL87X2E|8762E"
    "CONFIG_SOC_SERIES_RTL87X2H|8752H"
)

# Build list of chip directories that match the current SOC
set(ACTIVE_CHIP_DIRS "")
foreach(pair IN LISTS SOC_CHIP_PAIRS)
    string(REPLACE "|" ";" pair_list "${pair}")
    list(GET pair_list 0 soc_config)
    list(GET pair_list 1 chip_dir)
    if(${soc_config})
        list(APPEND ACTIVE_CHIP_DIRS ${chip_dir})
    endif()
endforeach()

# LCD type config to chip directory mapping (general drivers only)
set(LCD_CONFIG_CHIP_PAIRS
    "CONFIG_REALTEK_LCD_SH8601Z_410_502_QSPI|8773G"
    "CONFIG_REALTEK_LCD_SH8601Z_410_502_QSPI|8773E"
    "CONFIG_REALTEK_LCD_ST77916_360_360_QSPI|8773G"
    "CONFIG_REALTEK_LCD_ST77916_360_360_QSPI|8773E"
    "CONFIG_REALTEK_LCD_ICNA3310_466_466_QSPI|8773G"
    "CONFIG_REALTEK_LCD_ICNA3310_466_466_QSPI|8773E"
    "CONFIG_REALTEK_LCD_ST7801N_466_466_QSPI|8773G"
    "CONFIG_REALTEK_SH8601Z_454454_QSPI|8773G"
    "CONFIG_REALTEK_SH8601Z_454454_QSPI|8773E"
    "CONFIG_REALTEK_LCD_SH8601Z_466_466_QSPI|8773G"
    "CONFIG_REALTEK_LCD_ST7265_800_480_RGB|8773G"
    "CONFIG_REALTEK_LCD_ST7265_800_480_RGB|8773E"
    "CONFIG_REALTEK_LCD_EK9716_800_480_RGB|8773G"
    "CONFIG_REALTEK_LCD_ST77916_360_360_8080|8773G"
    "CONFIG_REALTEK_LCD_ST77916_360_360_8080|8773E"
    "CONFIG_REALTEK_LCD_ST7789_170_320_SPI|8773G"
    "CONFIG_REALTEK_LCD_C05300_390_450_QSPI|8773G"
    "CONFIG_REALTEK_LCD_C05300_390_450_QSPI|8773E"
    "CONFIG_REALTEK_LCD_NV3041A_480_272_QSPI|8773G"
    "CONFIG_REALTEK_LCD_NV3041A_480_272_QSPI|8773E"
    "CONFIG_REALTEK_LCD_HX8369_480_480_RGB|8773E"
    "CONFIG_REALTEK_ST7701S_480_480_RGB|8773G"
    "CONFIG_REALTEK_ST7701S_480_480_RGB|8773E"
)

# Touch config to chip directory mapping (general drivers only -- _ZEPHYR variants live in device/zephyr/)
set(TOUCH_CONFIG_CHIP_PAIRS
    "CONFIG_REALTEK_TOUCH_CHSC6417|8773G"
    "CONFIG_REALTEK_TOUCH_CHSC6417|8773E"
    "CONFIG_REALTEK_TOUCH_CHSC6413|8773G"
    "CONFIG_REALTEK_TOUCH_CST820|8773G"
    "CONFIG_REALTEK_TOUCH_GT911|8773G"
    "CONFIG_REALTEK_TOUCH_GT911|8773E"
    "CONFIG_REALTEK_TOUCH_CST816D|8773E"
)

set(KEY_CONFIG_CHIP_PAIRS
    "CONFIG_REALTEK_KEY_BUTTON|8773G"
    "CONFIG_REALTEK_KEY_BUTTON|8773E"
    "CONFIG_REALTEK_KEY_BUTTON|8772F"
)

# Process LCD configs to add chip subdirectories.
# Multiple LCD configs may map to the same chip_dir (e.g. SH8601Z and ST77916 both -> 8773G);
# track already-added dirs to avoid CMake duplicate-binary-directory errors.
set(LCD_ADDED_CHIP_DIRS "")
foreach(pair IN LISTS LCD_CONFIG_CHIP_PAIRS)
    string(REPLACE "|" ";" pair_list "${pair}")
    list(GET pair_list 0 lcd_config)
    list(GET pair_list 1 chip_dir)
    if(${lcd_config} AND (chip_dir IN_LIST ACTIVE_CHIP_DIRS) AND NOT (chip_dir IN_LIST LCD_ADDED_CHIP_DIRS))
        set(sub_path "${CMAKE_CURRENT_SOURCE_DIR}/lcd/${chip_dir}")
        if(EXISTS "${sub_path}/CMakeLists.txt")
            add_subdirectory("lcd/${chip_dir}")
            list(APPEND LCD_ADDED_CHIP_DIRS ${chip_dir})
        endif()
    endif()
endforeach()

# Process touch configs to add chip subdirectories (deduplicated by chip_dir).
set(TOUCH_ADDED_CHIP_DIRS "")
foreach(pair IN LISTS TOUCH_CONFIG_CHIP_PAIRS)
    string(REPLACE "|" ";" pair_list "${pair}")
    list(GET pair_list 0 touch_config)
    list(GET pair_list 1 chip_dir)
    if(${touch_config} AND (chip_dir IN_LIST ACTIVE_CHIP_DIRS) AND NOT (chip_dir IN_LIST TOUCH_ADDED_CHIP_DIRS))
        set(sub_path "${CMAKE_CURRENT_SOURCE_DIR}/touch/${chip_dir}")
        if(EXISTS "${sub_path}/CMakeLists.txt")
            add_subdirectory("touch/${chip_dir}")
            list(APPEND TOUCH_ADDED_CHIP_DIRS ${chip_dir})
        endif()
    endif()
endforeach()

# Process key_button configs (deduplicated by chip_dir).
set(KEY_ADDED_CHIP_DIRS "")
foreach(pair IN LISTS KEY_CONFIG_CHIP_PAIRS)
    string(REPLACE "|" ";" pair_list "${pair}")
    list(GET pair_list 0 key_config)
    list(GET pair_list 1 chip_dir)
    if(${key_config} AND (chip_dir IN_LIST ACTIVE_CHIP_DIRS) AND NOT (chip_dir IN_LIST KEY_ADDED_CHIP_DIRS))
        set(sub_path "${CMAKE_CURRENT_SOURCE_DIR}/key_button/${chip_dir}")
        if(EXISTS "${sub_path}/CMakeLists.txt")
            add_subdirectory("key_button/${chip_dir}")
            list(APPEND KEY_ADDED_CHIP_DIRS ${chip_dir})
        endif()
    endif()
endforeach()
