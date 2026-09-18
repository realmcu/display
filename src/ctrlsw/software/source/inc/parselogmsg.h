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

#include "dectypes.h"
#include "dec_log.h"
#include "vpufeature.h"

#ifdef VCD_LOGMSG
char* ParseDecPictureFormat(enum DecPictureFormat output_format);
char* ParseDecRet(enum DecRet rv);
char *ParseDWLMemType(int i);
void ParseDecInitConfig(struct DecInitConfig *cfg);
void ParseDecSequenceInfo(struct DecSequenceInfo *seq);
char* ParseDecPicAlignment(int i);
void ParseDecConfig(struct DecConfig *config);
void ParseDecInputParameters(struct DecInputParameters* param);
void ParseDecBufferInfo(struct DecBufferInfo *buf_info);
void ParseFeatureList(struct DecHwFeatures *cfg);

#else //#ifdef VCD_LOGMSG

#define ParseDecPictureFormat(output_format) (NULL)
#define ParseDecRet(rv) (NULL)
#define ParseDWLMemType(i) (NULL)
#define ParseDecInitConfig(cfg)
#define ParseDecSequenceInfo(seq)
#define ParseDecPicAlignment(i) (NULL)
#define ParseDecConfig(config)
#define ParseDecInputParameters(param)
#define ParseDecBufferInfo(buf_info)
#define ParseFeatureList(cfg)

#endif //#ifdef VCD_LOGMSG

#if 0
#define RETURN_LOG() do { \
  return; \
} while (0)
#define RETURN_LOG_1() do { \
  return NULL; \
} while (0)
#else
#define RETURN_LOG()
#define RETURN_LOG_1()
#endif