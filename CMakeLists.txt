# Copyright (c) 2024 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

message(STATUS "CONFIG_REALTEK_DISPLAY is set to: ${CONFIG_REALTEK_DISPLAY}")

list(APPEND soc_series
    CONFIG_SOC_SERIES_RTL87X2G
    CONFIG_SOC_SERIES_RTL8772F
    CONFIG_SOC_SERIES_RTL8773E
    CONFIG_SOC_SERIES_RTL8773G
)

set(any_soc_selected OFF)

foreach(item IN LISTS soc_series)
  if(${item} STREQUAL "y")
     message(STATUS "REALTEK SOC selected: ${item}")
  set(any_soc_selected ON)
  endif()
endforeach()

if(NOT any_soc_selected)
  message(STATUS "No REALTEK SOC selected!!!")
endif()

set(HW_DISPLAY_TARGET_NAME reaktek_display_driver_library)

if(ZEPHYR_BASE)
  message(STATUS "ZEPHYR_BASE is set to: ${ZEPHYR_BASE}")
  add_library(${HW_DISPLAY_TARGET_NAME} STATIC)
  target_link_libraries(${HW_DISPLAY_TARGET_NAME} PRIVATE zephyr_interface)
  add_subdirectory_ifdef(CONFIG_REALTEK_DISPLAY hardware)
else()
  add_library(${HW_DISPLAY_TARGET_NAME} ${SOURCES})
  add_subdirectory_ifdef(CONFIG_REALTEK_DISPLAY hardware)
endif()


