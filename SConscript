#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: Apache-2.0
#
# SCons build for the JPU (Realtek JPEG codec) driver.
#
from building import *

cwd = GetCurrentDir()

# Register definitions live per SoC under src/device/; rtl_common/ holds the
# OS and SoC independent driver. The OS itself is selected by JPU_OS_* below.
PLATFORM = 'src/device/rtl87x3k'

include_path = [
    cwd + '/inc',
    cwd + '/' + PLATFORM,
]

src = [
    'src/device/rtl_common/rtl_jpu.c',
    'src/device/rtl_common/rtl_hal_jpu.c',
]

# OS selection for inc/rtl_jpu_port.h. No predefined macro distinguishes
# RT-Thread from OSIF under SCons, so it has to be stated here; CMake and Keil
# must state it too.
OS_DEFINE = ['JPU_OS_RTTHREAD']

# depend = [] keeps the whole group out of the build when the switch is off,
# so nothing here is read unless REALTEK_DISPLAY_ENABLE_JPU is set.
group = DefineGroup(
    'jpu', src, depend = ['REALTEK_DISPLAY_ENABLE_JPU'],
    CPPPATH     = include_path,
    CPPDEFINES  = OS_DEFINE,
)

Return('group')
