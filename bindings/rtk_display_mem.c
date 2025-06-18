/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* \file     rtk_display_mem.c
* \brief    This file provides display repo bindings API.
* \details
* \author   realmcu
* \date     2025-03-05
* \version  v1.0
*********************************************************************************************************
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
