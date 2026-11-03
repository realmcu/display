#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: Apache-2.0
#
# SCons build for the VCDJ (VeriSilicon VC9000DJ DecoderJ) driver.
#
import os
import re
from building import *

cwd = GetCurrentDir()

CTRLSW = 'src/ctrlsw/software/source'

# One directory per OS under src/device/, exactly one compiled:
#   rtl87x3k      FreeRTOS + OSIF, built by Keil MDK
#   rtl87x3k_rtt  RT-Thread, built here (also supplies a trace.h stand-in)
PLATFORM = 'src/device/rtl87x3k_rtt'

# How the vendor Control Software is compiled:
#   split   the 24 vendor .c files one by one
#   unity   one TU, src/ctrlsw/vcdj_ctrlsw_unity.c, that #includes all 24
#   merged  src/ctrlsw/vcdj_ctrlsw.c/.h from vcdj_ctrlsw_gen.py; no vendor
#           include path
VCDJ_CTRLSW = 'merged'

include_path = [
    cwd + '/inc',
    cwd + '/src/common',
    cwd + '/src/test',
    cwd + '/' + PLATFORM,
]
if VCDJ_CTRLSW != 'merged':
    include_path += [
        cwd + '/' + CTRLSW + '/inc',
        cwd + '/' + CTRLSW + '/config',
        cwd + '/' + CTRLSW + '/common',
        cwd + '/' + CTRLSW + '/jpeg',
        cwd + '/' + CTRLSW + '/png',
    ]

# Selects vcdj_ctrlsw.h in rtl_vcdj_vendor.h and src/common/rtl_vcdj_ctrlsw.h.
ctrlsw_defines = ['VCDJ_CTRLSW_MERGED'] if VCDJ_CTRLSW == 'merged' else []

# Vendor compile contract. Must stay in step with CMake's
# target_compile_options(-include ...) and Keil's --preinclude.
VENDOR_PREINCLUDE = ' -include ' + cwd + '/inc/rtl_vcdj_vendor.h'

src = [
    'src/common/rtl_vcdj_hal.c',
    'src/common/rtl_vcdj_dwl.c',
    'src/common/rtl_vcdj_core.c',
    PLATFORM + '/rtl_vcdj_port.c',
    'src/test/vcdj_test_stream.c',
    'src/test/vcdj_selftest.c',
]

# Two groups because SCons has no per-source-file compile option: vendor sources
# need their own warning suppressions.
group = DefineGroup(
    'vcdj', src, depend = ['REALTEK_DISPLAY_ENABLE_VCDJ'],
    CPPPATH          = include_path,
    LOCAL_CCFLAGS    = VENDOR_PREINCLUDE,
    LOCAL_CPPDEFINES = ctrlsw_defines,
)

# Explicit, not globbed, and mirrors src/ctrlsw/CMakeLists.txt one for one, so a
# vendor upgrade that adds or renames a file fails loudly.
ctrlsw_common_src = [
    CTRLSW + '/common/case_info.c',
    CTRLSW + '/common/commonconfig.c',
    CTRLSW + '/common/commonfunction.c',
    CTRLSW + '/common/dec_log.c',
    CTRLSW + '/common/delogo.c',
    CTRLSW + '/common/error.c',
    CTRLSW + '/common/errorhandling.c',
    CTRLSW + '/common/fifo.c',
    CTRLSW + '/common/input_queue.c',
    CTRLSW + '/common/low_latency.c',
    CTRLSW + '/common/ppu.c',
    CTRLSW + '/common/regdrv.c',
    CTRLSW + '/common/sw_buf_protect.c',
    CTRLSW + '/common/sw_stream.c',
    CTRLSW + '/common/sw_util.c',
    CTRLSW + '/common/vpufeature.c',
]

ctrlsw_jpeg_src = [
    CTRLSW + '/jpeg/jpegdecapi.c',
    CTRLSW + '/jpeg/jpegdechdrs.c',
    CTRLSW + '/jpeg/jpegdecinternal.c',
    CTRLSW + '/jpeg/jpegdecutils.c',
]

ctrlsw_png_src = [
    CTRLSW + '/png/png_decoder.c',
    CTRLSW + '/png/pngdecapi.c',
    CTRLSW + '/png/pngdecutils.c',
    CTRLSW + '/png/pnghwd_asic.c',
]

ctrlsw_src = ctrlsw_common_src + ctrlsw_jpeg_src + ctrlsw_png_src

for vcdj_src in ctrlsw_src:
    if not os.path.isfile(os.path.join(cwd, vcdj_src)):
        raise RuntimeError(
            '[VCDJ] vendor source %s is missing. The tree under src/ctrlsw/software/ '
            'must hold the release archive files; see src/ctrlsw/README.md.' % vcdj_src)

# Checked in every mode so the unity list cannot drift while split is in use.
CTRLSW_UNITY = 'src/ctrlsw/vcdj_ctrlsw_unity.c'
with open(os.path.join(cwd, CTRLSW_UNITY)) as f:
    unity_src = ['src/ctrlsw/' + m.group(1) for m in
                 re.finditer(r'^#include "(software/source/[^"]+\.c)"', f.read(), re.M)]
if unity_src != ctrlsw_src:
    raise RuntimeError(
        '[VCDJ] the #include list in %s differs from ctrlsw_src (content or '
        'order).' % CTRLSW_UNITY)

if VCDJ_CTRLSW == 'split':
    ctrlsw_build_src = ctrlsw_src
elif VCDJ_CTRLSW == 'unity':
    ctrlsw_build_src = [CTRLSW_UNITY]
elif VCDJ_CTRLSW == 'merged':
    ctrlsw_build_src = ['src/ctrlsw/vcdj_ctrlsw.c']
else:
    raise RuntimeError('[VCDJ] unknown VCDJ_CTRLSW %r' % VCDJ_CTRLSW)

# Vendor code only; the warnings come from the vendor sources.
CTRLSW_CCFLAGS = (' -Wno-format -Wno-format-security'
                  ' -Wno-unused-variable -Wno-unused-function')

group = group + DefineGroup(
    'vcdj_ctrlsw', ctrlsw_build_src, depend = ['REALTEK_DISPLAY_ENABLE_VCDJ'],
    CPPPATH          = include_path,
    LOCAL_CCFLAGS    = VENDOR_PREINCLUDE + CTRLSW_CCFLAGS,
    LOCAL_CPPDEFINES = ctrlsw_defines,
)

Return('group')
