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

# The vendor Control Software is compiled as the generated, pruned
# src/ctrlsw/vcdj_ctrlsw.c/.h (src/ctrlsw/README.md section 8), with no vendor
# include path. The DWL in src/common/rtl_vcdj.c implements only what that
# pruned code references, so the vendor files cannot be compiled directly.
include_path = [
    cwd + '/inc',
    cwd + '/src/common',
    cwd + '/src/test',
    cwd + '/' + PLATFORM,
]

# Selects vcdj_ctrlsw.h in rtl_vcdj_vendor.h and src/common/rtl_vcdj_ctrlsw.h.
ctrlsw_defines = ['VCDJ_CTRLSW_MERGED']

# Vendor compile contract. Must stay in step with CMake's
# target_compile_options(-include ...) and Keil's --preinclude.
VENDOR_PREINCLUDE = ' -include ' + cwd + '/inc/rtl_vcdj_vendor.h'

src = [
    'src/common/rtl_vcdj.c',
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

# Not compiled: the generator's input, checked here so a vendor upgrade that
# adds or renames a file fails loudly. Explicit, not globbed, and mirrors
# src/ctrlsw/CMakeLists.txt one for one.
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

# vcdj_ctrlsw_gen.py reads the source order from the unity wrapper.
CTRLSW_UNITY = 'src/ctrlsw/vcdj_ctrlsw_unity.c'
with open(os.path.join(cwd, CTRLSW_UNITY)) as f:
    unity_src = ['src/ctrlsw/' + m.group(1) for m in
                 re.finditer(r'^#include "(software/source/[^"]+\.c)"', f.read(), re.M)]
if unity_src != ctrlsw_src:
    raise RuntimeError(
        '[VCDJ] the #include list in %s differs from ctrlsw_src (content or '
        'order).' % CTRLSW_UNITY)

ctrlsw_build_src = ['src/ctrlsw/vcdj_ctrlsw.c']

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
