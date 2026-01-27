/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef RTK_DISPLAY_MALLOC
#define RTK_DISPLAY_MALLOC(size) malloc(size)
//#define RTK_DISPLAY_MALLOC(size) os_mem_alloc(RAM_TYPE_DATA_ON, size)
#endif

#ifndef RTK_DISPLAY_FREE
#define RTK_DISPLAY_FREE free
#endif

void *rtk_display_malloc(size_t size)
{
    return RTK_DISPLAY_MALLOC(size);
}


void rtk_display_free(void *ptr)
{
    RTK_DISPLAY_FREE(ptr);
}
