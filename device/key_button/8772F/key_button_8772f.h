/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2020-08-04     howie  first version
 */

#ifndef __KEY_BUTTON_8772F_H
#define __KEY_BUTTON_8772F_H


#include "board.h"
#include "stdint.h"
#include "stdbool.h"


#ifdef __cplusplus
extern "C" {
#endif

#define KEY_BUTTON                                     P4_7



void rtk_key_button_init(void);


#ifdef __cplusplus
}
#endif

#endif /* __KEY_BUTTON_8772F_H */
