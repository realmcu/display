#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: Apache-2.0
#
# SCons build for the JPU (Realtek JPEG codec) driver.
#
from building import *

cwd = GetCurrentDir()

# Register and address-map definitions live per SoC under src/device/.
# Only one is compiled; rtl_common/ holds the SoC independent driver.
PLATFORM = 'src/device/rtl8773g'

include_path = [
    cwd + '/inc',
    cwd + '/' + PLATFORM,
]

src = [
    'src/device/rtl_common/rtl_jpu.c',
    'src/device/rtl_common/rtl_hal_jpu.c',
]

# depend = [] keeps the whole group out of the build when the switch is off,
# so nothing here is read unless REALTEK_DISPLAY_ENABLE_JPU is set.
group = DefineGroup(
    'jpu', src, depend = ['REALTEK_DISPLAY_ENABLE_JPU'],
    CPPPATH = include_path,
)

Return('group')
