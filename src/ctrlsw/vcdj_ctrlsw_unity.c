/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Unity build of the vendor Control Software: SConscript VCDJ_CTRLSW = 'unity'.
 * The #include list must match the SConscript source list in content and order;
 * SConscript checks this in every mode.
 *
 * Each source's file-scope macros are #undef'd before the next source, as they
 * would be at the end of a separate translation unit. Exception: macros that are
 * token-identical to a header definition, because the #undef would also remove
 * the header's copy, which later sources depend on.
 */

/*============================================================================*
 *                        common
 *============================================================================*/
#include "software/source/common/case_info.c"

/* DEC_MODE_* are left defined: identical to regdrv.h. */
#include "software/source/common/commonconfig.c"

#include "software/source/common/commonfunction.c"
#undef LUM_CBS_WIDTH
#undef CH_CBS_WIDTH
#undef LUMA_CACHE_RAM_SIZE
#undef CHROMA_CACHE_RAM_SIZE
#undef CBS_ONE_ROW_PREFTECH_PIXELS
#undef CBS_ONE_ROW_PREFTECH_BYTES_Y
#undef CBS_ONE_ROW_PREFTECH_BYTES_C
#undef TILE_EXTRA_PIXELS
#undef MAX_RFC_TABLE_LINES
#undef UP_ALIGN
#undef DOWN_ALIGN

#include "software/source/common/dec_log.c"
#undef GETPID
#undef VPU_MAX_DEBUG_BUFFER_LEN
#undef FILE_MAX_SIZE
#undef BUF_SIZE
#undef HSWREG

#include "software/source/common/delogo.c"
#include "software/source/common/error.c"

#include "software/source/common/errorhandling.c"
#undef MAGIC_WORD_LENGTH
#undef NUM_OFFSETS

#include "software/source/common/fifo.c"

#include "software/source/common/input_queue.c"
#undef PRINT_COUNTS

#include "software/source/common/low_latency.c"
#undef TRANSE32

/* MAX and ABS are left defined: identical to sw_util.h. */
#include "software/source/common/ppu.c"
#undef _USE_MATH_DEFINES
#undef PP_MAX_STRIDE
#undef DOWN_SCALE_SIZE
#undef TOFIX
#undef FDIVI
#undef JPEG
#undef COEFF_WIDTH
#undef ALIGN_UP_TO_8
#undef MAX4
#undef ROUNDED_DIV
#undef PrintPpunitConfig
#undef SET_PP_SLICE_PARAMS

#include "software/source/common/regdrv.c"
#include "software/source/common/sw_buf_protect.c"
#include "software/source/common/sw_stream.c"

#include "software/source/common/sw_util.c"
#undef STACK_STAT_SIZE
#undef MAGICWORD
#undef SET_CORE_MASK
#undef SET_MAX_WXH

#include "software/source/common/vpufeature.c"

/*============================================================================*
 *                        jpeg
 *============================================================================*/
#include "software/source/jpeg/jpegdecapi.c"
#undef JPEGDEC_CLEAR_IRQ
#undef ABORT_MARKER
#undef JPG_INFO

#include "software/source/jpeg/jpegdechdrs.c"

#include "software/source/jpeg/jpegdecinternal.c"
#undef JPEGDEC_SLICE_START_VALUE
#undef JPEGDEC_VLC_LEN_START_REG
#undef JPEGDEC_VLC_LEN_END_REG
#undef TRACE_COMPONENT_ID

#include "software/source/jpeg/jpegdecutils.c"

/*============================================================================*
 *                        png
 *============================================================================*/
/*
 * pngdeccontainer.h and the PNG sources reuse these JPEG type, struct tag,
 * enumerator and static function names; prefix the PNG copies.
 */
#define StreamStorage           Png_StreamStorage
#define OutElement              Png_OutElement
#define OutElement_             Png_OutElement_
#define FrameBufferList         Png_FrameBufferList
#define FrameBufferList_        Png_FrameBufferList_
#define DEC_IDLE                Png_DEC_IDLE
#define DEC_RUNNING             Png_DEC_RUNNING
#define InitList                Png_InitList
#define ReleaseList             Png_ReleaseList
#define PushOutputPic           Png_PushOutputPic
#define PeekOutputPic           Png_PeekOutputPic
#define SetAbortStatusInList    Png_SetAbortStatusInList
#define ResetOutFifoInList      Png_ResetOutFifoInList
#define ClearAbortStatusInList  Png_ClearAbortStatusInList
#define GetByte                 Png_GetByte

#include "software/source/png/png_decoder.c"

#include "software/source/png/pngdecapi.c"
#undef PNGDEC_CLEAR_IRQ
#undef ABORT_MARKER
#undef PNG_MERGE

#include "software/source/png/pngdecutils.c"
#include "software/source/png/pnghwd_asic.c"
