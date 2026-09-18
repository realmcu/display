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

/* This file is generated. Please don't edit, don't change. */
#define VCDSW_VERSION_MAJOR 4
#define VCDSW_VERSION_MINOR 1
#define VCDSW_VERSION_MICRO 84

#define VCDSW_VESION_CODE (((VCDSW_VERSION_MAJOR & 0xff)<<24) \
                         | ((VCDSW_VERSION_MINOR & 0xff)<<16) \
                         | (VCDSW_VERSION_MICRO & 0xffff))

/**
 * Evaluates to %TRUE if the version of VCD API is greater than
 * @major, @minor and @revision
 */
#define VCDSW_CHECK_VERSION(major,minor,revision) \
        (VCDSW_VERSION_MAJOR > (major) || \
         (VCDSW_VERSION_MAJOR == (major) && VCDSW_VERSION_MINOR > (minor)) || \
         (VCDSW_VERSION_MAJOR == (major) && VCDSW_VERSION_MINOR == (minor) && \
          VCDSW_VERSION_MICRO >= (revision)))

/** Set P4CL# Directly */
#define VCDEC_BUILD_CLNUM 1411136

