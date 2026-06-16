#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: Apache-2.0
#
message(STATUS "CONFIG_REALTEK_DISPLAY is set to: ${CONFIG_REALTEK_DISPLAY}")

list(APPEND soc_series
    CONFIG_SOC_SERIES_RTL87X2G
    CONFIG_SOC_SERIES_RTL8772F
    CONFIG_SOC_SERIES_RTL8773E
    CONFIG_SOC_SERIES_RTL87X3E
    CONFIG_SOC_SERIES_RTL87X3G
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

set(HW_DISPLAY_TARGET_NAME realtek_display_driver_library)
set(LIBRARY_NAME ${HW_DISPLAY_TARGET_NAME})

if(ZEPHYR_BASE)
  if(CONFIG_REALTEK_DISPLAY)
	  message(STATUS "ZEPHYR_BASE is set to: ${ZEPHYR_BASE}")
    zephyr_library_named(${HW_DISPLAY_TARGET_NAME})
	  if(CONFIG_REALTEK_DISPLAY_GUI_ACC)
		target_link_libraries(${HW_DISPLAY_TARGET_NAME} PRIVATE guiLib)
	  endif()
	  add_subdirectory(hardware)
	  add_subdirectory(device)
	  add_subdirectory(bindings)
  endif()
else()
  if(CONFIG_REALTEK_DISPLAY)
    message(STATUS "[Display] Building for non-Zephyr environment")
    add_library(${LIBRARY_NAME} STATIC)
    add_subdirectory(hardware)
    add_subdirectory(device)
    add_subdirectory(bindings)
    target_link_libraries(${LIBRARY_NAME} PUBLIC halLib)
    if(CONFIG_REALTEK_DISPLAY_GUI_ACC)
      target_link_libraries(${LIBRARY_NAME} PUBLIC guiLib)
    endif()
  endif()
endif()


