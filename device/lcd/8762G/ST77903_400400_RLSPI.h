/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _LCD_ST99703_400_RLSPI_H_
#define _LCD_ST99703_400_RLSPI_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"

#define     DMA_MULTIBLOCK          1
#define     INFINITE_MODE           1

#define PIXEL_BYTES                 3

#define ST77903_WIDTH                   (400U)
#define ST77903_HEIGHT                  (400U)

#define ST77903_VSYNC                   (1U)// for now
#define ST77903_VBP                     (8U)
#define ST77903_VFP                     (8U)

#define ST77903_VSYNC_CMD               0xD8
#define ST77903_VBP_CMD                 0xD8
#define ST77903_VACTIVE_CMD_QSPI        0xDE
#define ST77903_VACTIVE_CMD_DSPI        0xDB
#define ST77903_VACTIVE_CMD_SPI         0xDE
#define ST77903_VFP_CMD                 0xD8
#define ST77903_DUMMY_CMD               0x00

#define ST77903_VSYNC_ADDR              0x006100
#define ST77903_VBP_ADDR                0x006000
#define ST77903_VACTIVE_ADDR            0x006000
#define ST77903_VFP_ADDR                0x006000

#define LCDC_DMA_CHANNEL_NUM              0
#define LCDC_DMA_CHANNEL_INDEX            LCDC_DMA_Channel0



//1 QFN68 -  0 QFN88
#if 0  // GROUP 1
#define LCD_QSPI_RST                     P3_6
// #define LCD_QSPI_BL                      P1_4
#define LCD_QSPI_D0                      P4_2
#define LCD_QSPI_D1                      P4_1
#define LCD_QSPI_D2                      P3_2
#define LCD_QSPI_D3                      P3_3
#define LCD_QSPI_CS                      P4_3
#define LCD_QSPI_CLK                     P4_0
#define LCD_QSPI_TE                      P0_5

//#define LCD_QSPI_IM0                      P0_1
//#define LCD_QSPI_IM1                      P0_1
#else // GROUP 2
#define LCD_QSPI_RST                     P3_6
//#define LCD_QSPI_BL                      P1_2
#define LCD_QSPI_D0                      P4_2
#define LCD_QSPI_D1                      P4_1
#define LCD_QSPI_D2                      P0_6
#define LCD_QSPI_D3                      P0_7
#define LCD_QSPI_CS                      P4_3
#define LCD_QSPI_CLK                     P4_0
#define LCD_QSPI_TE                      P0_1
//#define LCD_QSPI_IM0                      P0_1
//#define LCD_QSPI_IM1                      P0_1

#endif


void rtk_lcd_hal_init(void);
void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len);
#endif
