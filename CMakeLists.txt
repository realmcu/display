# Copyright (c) 2024 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0


# 输出用于调试的信息，例如项目路径和 CMake 版本

message(STATUS "ZEPHYR_BASE is set to: ${ZEPHYR_BASE}")

message(STATUS "CONFIG_REALTEK_DISPLAY is set to: ${CONFIG_REALTEK_DISPLAY}")

set(LIBRARY_NAME reaktek_display_driver_library)

if(ZEPHYR_BASE)
  add_library(${LIBRARY_NAME} STATIC)
  target_link_libraries(${LIBRARY_NAME} PRIVATE zephyr_interface)
  add_subdirectory_ifdef(CONFIG_REALTEK_DISPLAY driver)
  #add_subdirectory_ifdef(CONFIG_REALTEK_DISPLAY zephyr-driver)
else()
  add_library(${LIBRARY_NAME} ${SOURCES})
  add_subdirectory_ifdef(CONFIG_REALTEK_DISPLAY driver)
endif()


