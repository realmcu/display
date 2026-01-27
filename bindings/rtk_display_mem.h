/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTK_DISPLAY_H
#define RTK_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/

void *rtk_display_malloc(size_t size);
void rtk_display_free(void *ptr);


#ifdef __cplusplus
}
#endif

#endif /* RTK_DISPLAY_H */
