#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: Apache-2.0
#
# This is the ONLY Realtek owned build file inside the vendor tree. The vendor
# subdirectories deliberately have no CMakeLists.txt, so the recursive
# add_subdirectory template used elsewhere in this repository skips them.
#
# The import under src/ctrlsw/ is a SELECTIVE subset of the release archive: only
# the 24 compiled .c files and the headers the compiler actually opens are
# imported (67 files, 42,281 lines). The Linux DWL reference implementation, the
# PC testbench, the vendor GNU-make files and the non-compiled sources are NOT in
# this repository at all - read them in the release archive. See README.md
# section 2 for the dropped set and the recovery path.
#
# The source list below is EXPLICIT, not globbed, and covers every .c file that
# is imported. It is kept explicit so that a new or renamed vendor file after an
# upgrade fails loudly instead of being picked up silently.
#
# Why these 24 and not fewer:
#   ppu.c               mandatory, not optional. The decoder core emits YUV and
#                       the RGB colour space conversion HoneyGUI needs is here.
#   sw_buf_protect.c    sw_stream.c calls SwReadByte / SwPrivateAreaReadByte.
#   error.c             compiles to an EMPTY object: its whole body is inside
#                       #ifdef ERROR_BUILD_SUPPORT, which this build does not
#                       define, and nothing calls Error(). Kept only because the
#                       vendor Makefile lists it in COMMON_SRCS, so dropping it
#                       would be a deviation to re-litigate on every upgrade.
#                       Costs 0 bytes of flash.
#
# The multi-codec dispatcher vcdecapi.c (6528 lines) is deliberately bypassed:
# it calls into JpegDec*/PngDec*, never the other way round, so calling
# JpegDec*/PngDec* directly from src/common/rtl_hal_vcdj.c cuts it off cleanly.
# parselogmsg.c was the single symbol that would have dragged it back in (it
# calls VCDecRetStr); ppu.c and vpufeature.c only include its header, which IS
# imported.
#
# When upgrading the vendor drop, follow README.md section 4 - the import list
# must be re-derived from the build's own dependency data, not hand-edited.
#
cmake_minimum_required(VERSION 3.10)

if(CONFIG_REALTEK_DISPLAY_ENABLE_VCDJ)

    set(VCDJ_CTRLSW_COMMON_SRCS
        software/source/common/case_info.c
        software/source/common/commonconfig.c
        software/source/common/commonfunction.c
        software/source/common/dec_log.c
        software/source/common/delogo.c
        software/source/common/error.c
        software/source/common/errorhandling.c
        software/source/common/fifo.c
        software/source/common/input_queue.c
        software/source/common/low_latency.c
        software/source/common/ppu.c
        software/source/common/regdrv.c
        software/source/common/sw_buf_protect.c
        software/source/common/sw_stream.c
        software/source/common/sw_util.c
        software/source/common/vpufeature.c
    )

    set(VCDJ_CTRLSW_JPEG_SRCS
        software/source/jpeg/jpegdecapi.c
        software/source/jpeg/jpegdechdrs.c
        software/source/jpeg/jpegdecinternal.c
        software/source/jpeg/jpegdecutils.c
    )

    set(VCDJ_CTRLSW_PNG_SRCS
        software/source/png/png_decoder.c
        software/source/png/pngdecapi.c
        software/source/png/pngdecutils.c
        software/source/png/pnghwd_asic.c
    )

    # JPEG and PNG are the two formats this IP decodes; both are always compiled
    # in, exactly like the JPEG path. There is no switch to drop either one.
    set(VCDJ_CTRLSW_SRCS
        ${VCDJ_CTRLSW_COMMON_SRCS}
        ${VCDJ_CTRLSW_JPEG_SRCS}
        ${VCDJ_CTRLSW_PNG_SRCS}
    )

    list(LENGTH VCDJ_CTRLSW_SRCS VCDJ_CTRLSW_SRC_COUNT)
    message(STATUS "[VCDJ] compiling ${VCDJ_CTRLSW_SRC_COUNT} vendor CtrlSW source files")

    # Absolute paths: the target is defined in the repository root CMakeLists.txt,
    # so relative entries would be resolved against the wrong directory
    # (CMake policy CMP0076).
    set(VCDJ_CTRLSW_ABS_SRCS "")
    foreach(vcdj_src ${VCDJ_CTRLSW_SRCS})
        if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${vcdj_src})
            message(FATAL_ERROR
                "[VCDJ] vendor source ${vcdj_src} is missing. The tree under "
                "src/ctrlsw/ must be an unmodified extraction of the release "
                "archive; see src/ctrlsw/README.md.")
        endif()
        list(APPEND VCDJ_CTRLSW_ABS_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/${vcdj_src})
    endforeach()

    target_sources(${HW_DISPLAY_TARGET_NAME} PRIVATE ${VCDJ_CTRLSW_ABS_SRCS})

    # Warning suppressions that apply to the VENDOR FILES ONLY.
    #
    # TARGET_DIRECTORY is what makes this work: source file properties are scoped
    # to the directory that defines the target, which is the repository root, not
    # this file.
    #
    # -Wno-format          the vendor trace strings use %lu for u64, which is
    #                      correct on 64-bit Linux and wrong on a 32-bit target.
    #                      Those are vendor strings in vendor files; they get
    #                      reported upstream, not patched here.
    # -Wno-unused-variable static objects that only the disabled trace builds use
    #                      (e.g. dec_log.c log_mutex under VCD_LOGMSG).
    # -Wno-unused-function static helpers reachable only from disabled features.
    set_source_files_properties(${VCDJ_CTRLSW_ABS_SRCS}
        TARGET_DIRECTORY ${HW_DISPLAY_TARGET_NAME}
        PROPERTIES COMPILE_OPTIONS "-Wno-format;-Wno-format-security;-Wno-unused-variable;-Wno-unused-function"
    )

    # The vendor include directories are added PRIVATE in hardware/vcdj/CMakeLists.txt
    # together with the vendor feature macros, so that both the vendor sources and
    # our own vendor facing sources see exactly the same contract.

endif()
