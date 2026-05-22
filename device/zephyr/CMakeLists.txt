# Copyright (c) 2024 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

# Each subdirectory's CMakeLists.txt guards source files by its own per-driver
# config. Entering with no matching config leaves SOURCES empty -- safe to add
# unconditionally; no compilation cost.
add_subdirectory(lcd/8773G)
add_subdirectory(touch/8773G)
add_subdirectory(key_button/8773G)
