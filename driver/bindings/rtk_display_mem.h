/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* \file     rtk_display_mem.h
* \brief    This file provides display repo bindings API.
* \details
* \author   realmcu
* \date     2024-12-77
* \version  v1.0
*********************************************************************************************************
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

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/
