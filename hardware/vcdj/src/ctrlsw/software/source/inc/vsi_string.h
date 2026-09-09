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
#ifndef _VSI_STRING_H_
#define _VSI_STRING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>

#undef av_unused
#if defined(__GNUC__) || defined(__clang__)
#define av_unused __attribute__((unused))
#else
#define av_unused
#endif
#ifdef _USE_VSI_STRING_
#define atoi    vsi_atoi
#define strcat  vsi_strcat
#define strcmp  vsi_strcmp
#define strcpy  vsi_strcpy
#define strlen  vsi_strlen
#define strncmp vsi_strncmp
#define strncpy vsi_strncpy
#define strtok  vsi_strtok
#define strtol  vsi_strtol

extern char *vsi_strcat (char *dest, const char *src);
extern int vsi_strcmp (const char *s1, const char *s2);
extern int vsi_strncmp (const char *s1, const char *s2, unsigned int n);
extern char *vsi_strcpy (char *dest, const char *src);
extern char *vsi_strncpy (char *dest, const char *src, unsigned int n);
extern unsigned int vsi_strlen (const char *s);
extern char *vsi_strtok (char *s, const char *delim);
long int vsi_strtol(const char *nptr,char **endptr,int base);
int vsi_atoi (const char *nptr);
#elif defined(SAFESTRING)
//safestring imp for Intel
#include "safe_lib.h"
#include "snprintf_s.h"

#define strlen(dest) strnlen_s(dest, 512)
//#define wcslen(dest)              wcsnlen_s(dest, 512)

#define strcat(dest, src) strcat_s(dest, 512, src)
//#define strncat(dest,src,n)       strncat_s(dest, 512, src, n)

#define strcpy(dest, src) strcpy_s(dest, 512, src)
#define strncpy(dest, src, n) strncpy_s(dest, 512, src, n)

#define memcpy(dest, src, n) memcpy_s(dest, n, src, n)
#define memmove(dest, src, n) memmove_s(dest, n, src, n)
/* #define memcmp(s1,s2,n)           int ind = 0;\
                                  int rc = 0;\
                                  if (rc = memcmp_s(s1, 512, s2, n, &ind) != EOK ) {\
    	                            printf("%s %u  Ind=%d  Error rc=%u \n", __FUNCTION__, __LINE__, ind, rc);\
									assert(0);\
                                  }\
	                                return ind; */

#define memset(dest, value, n) memset_s(dest, n, value)

typedef errno_t mem_ret;
av_unused static mem_ret vsi_strcmp(const char *s1, const char *s2) {
  int ind = 0;
  int rc = 0;
  if ((rc = strcmp_s(s1, RSIZE_MAX_STR, s2, &ind)) != EOK) {
    printf("%s %d  Ind=%d  Error rc=%d \n", __FUNCTION__, __LINE__, ind, rc);
    assert(0);
  }

  return ind;
}

#define strcmp vsi_strcmp
//...
#else
/* For memset, strcpy and strlen */
#include <string.h>

#define snprintf_s_i(dest, n, format, a) snprintf(dest, n, format, a)
#define snprintf_s_si(dest, dmax, format, s, a) \
  snprintf(dest, dmax, format, s, a)
#define snprintf_s_l(dest, dmax, format, a) snprintf(dest, dmax, format, a)
#define snprintf_s_sl(dest, dmax, format, s, a) \
  snprintf(dest, dmax, format, s, a)
#define snprintf_s_s(dest, dmax, format, s) snprintf(dest, dmax, format, s)
#define snprintf_s_sssiii(dest, dmax, format, s1, s2, s3, inta1, inta2, inta3) \
  snprintf(dest, dmax, format, s1, s2, s3, inta1, inta2, inta3)

typedef void *mem_ret;

av_unused static int vsi_strcmp(const char *s1, const char *s2) {
  return strcmp(s1, s2);
}

#endif

#ifdef __cplusplus
}
#endif

#endif /* _VSI_STRING_H_ */
