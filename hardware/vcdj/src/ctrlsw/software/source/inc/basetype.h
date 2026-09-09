/*
  * Copyright 2014-2025 Verisilicon Inc.
  *
  * The software contained herein may be directly re-distributed or may be adapted for redistribution
  * by SoC vendors who have licensed the related VeriSilicon/Hantro core IP, with the understanding
  * that the re-distribution is limited to the users who have purchased the SoC product which contains
  * the related VeriSilicon/Hantro IP. Otherwise, the information contained herein is not to be used by
  * or disclosed to the third parties without the express written permission of an officer of VeriSilicon Holdings Co., Ltd.
  *
  * The above copyright notice and this permission notice (including the
  * next paragraph) shall be included in all copies or substantial
  * portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
  * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
  * IN NO EVENT SHALL VERISILICON AND/OR ITS SUPPLIERS BE LIABLE FOR ANY
  * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef BASE_TYPE_H
#define BASE_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#ifndef NDEBUG
#include <assert.h>
#endif
#include <stdbool.h>
#include <inttypes.h>

#ifndef SYSTEMSHARED
#define VSIAPI(x) VSIAPI##x /* redefine API with prefix "VSIAPI" */
#else
#define VSIAPI(x) VSISYS##x /* redefine API with prefix "VSISYS" */
#endif

/**
 * \defgroup base_type Numeric Data Types
 *
 * \brief This section provides the definitions of numeric data types.
 *
 * @{
 */
/** \brief 8-bit signed integer. */
typedef int8_t i8;
/** \brief 8-bit unsigned integer. */
typedef uint8_t u8;
/** \brief 16-bit signed integer. */
typedef int16_t i16;
/** \brief 16-bit unsigned integer. */
typedef uint16_t u16;
/** \brief 32-bit signed integer. */
typedef int32_t i32;
/** \brief 32-bit unsigned integer. */
typedef uint32_t u32;
/** \brief 64-bit signed integer. */
typedef int64_t i64;
/** \brief 64-bit unsigned integer. */
typedef uint64_t u64;

/**@}*/

#ifndef INLINE
#define INLINE inline
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

#ifndef OK
#define OK 0
#endif
#ifndef NOK
#define NOK -1
#endif

/** \brief The macro that signals unused parameters.
 *  \ingroup common_group */
#define UNUSED(x) (void)(x)

typedef signed char        s8;
typedef signed short       s16;
typedef signed int         s32;
typedef signed short       pel; /* pixel type */
typedef long long          s64;

typedef short Short;
typedef int Int;
typedef unsigned int UInt;


/** \brief The Boolean value, false.
 *  \ingroup common_group */
#ifndef FALSE
#define FALSE  0
#endif
/** \brief The Boolean value, true.
 *  \ingroup common_group */
#ifndef TRUE
#define TRUE 1
#endif

#ifndef HANTRO_FALSE
#define HANTRO_FALSE 0
#endif

#ifndef HANTRO_TRUE
#define HANTRO_TRUE 1
#endif

typedef enum { False, True } vsi_bool;

/* ASSERT */
/* macro for assertion, used only when _ASSERT_USED is defined */
#if !defined(NDEBUG) || defined (_ASSERT_USED)
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif

/** \brief The address.
 *  \ingroup common_group */
typedef u64 addr_t;

/** \brief The buffer address.
 *  \ingroup common_group */
typedef u64 ptr_t;

#ifdef USE_LIBVA
#define STATIC
#else
#define STATIC static
#endif

#ifdef __cplusplus
}
#endif

#endif
