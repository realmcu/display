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

#ifndef _OSAL_H_
#define _OSAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__FREERTOS__) || defined(_WIN32) || defined(__linux__)

#if defined(__GNUC__) || defined(__clang__)
#define av_unused __attribute__((unused))
#else
#define av_unused
#endif

#ifdef __FREERTOS__
#include "osal_freertos.h"
#elif defined(__linux__)
#include "osal_linux.h"
#elif defined(_WIN32)
#include "osal_win32.h"
#endif

#ifdef _HAVE_PTHREAD_H
//For static link pthread in win32, need init some global variables to prevent crash
#ifdef PTW32_STATIC_LIB
static void detach_ptw32(void) {
  pthread_win32_thread_detach_np();
  pthread_win32_process_detach_np();
}
#endif

static av_unused void osal_thread_init()
{
#ifdef PTW32_STATIC_LIB
  pthread_win32_process_attach_np();
  pthread_win32_thread_attach_np();
  atexit(detach_ptw32);
#endif
}
#endif /* _HAVE_PTHREAD_H */

#endif /* defined(__FREERTOS__) || defined(__linux__) || defined(_WIN32) */

#define gettimeofday                osal_gettimeofday
#define usleep                      osal_usleep
#define open_memstream              osal_open_memstream

#ifdef __linux__
#define OSAL_STRM_POS(A) (A).__pos
#else
#define OSAL_STRM_POS(A) (A)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _OSAL_H_ */
