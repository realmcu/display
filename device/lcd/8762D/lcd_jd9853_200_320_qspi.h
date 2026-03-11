/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _LCD_JD9853_200_320_QSPI_H_
#define _LCD_JD9853_200_320_QSPI_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"

#define TE_VALID                            0

#define ST77916_LCD_WIDTH                  320
#define ST77916_LCD_HEIGHT                 200
#define ST77916_DRV_PIXEL_BITS             16

#define INPUT_PIXEL_BYTES                 2
#define OUTPUT_PIXEL_BYTES                2


#define LCD_SPI_CLK                                 P4_0
#define LCD_SPI_CS                                  P4_3
#define LCD_SPI_MOSI                                P4_2
#define LCD_SPI_BL                                  P2_7
#define LCD_SPI_DC                                  P4_1
#define LCD_SPI_RST                                 P2_0
#define LCD_SPI_RST_PIN                             GPIO_GetPin(LCD_SPI_RST)
#define LCD_SPI_DC_PIN                              GPIO_GetPin(LCD_SPI_DC)
#define LCD_SPI_BL_PIN                              GPIO_GetPin(LCD_SPI_BL)
#define LCD_SPI_BUS                                 SPI0
#define LCD_SPI_FUNC_CLK                            SPI0_CLK_MASTER
#define LCD_SPI_FUNC_MOSI                           SPI0_MO_MASTER
#define LCD_SPI_FUNC_CS                             SPI0_SS_N_0_MASTER
#define LCD_SPI_APBPeriph                           APBPeriph_SPI0
#define LCD_SPI_APBClock                            APBPeriph_SPI0_CLOCK
#define LCD_SPI_DMA_TX_HANDSHAKE                    GDMA_Handshake_SPI0_TX
#define QSPI_LCD_POWER                              P0_4
#define QSPI_LCD_TE                                 P1_2

#define LCD_DMA_CHANNEL_NUM              2
#define LCD_DMA_CHANNEL_INDEX            GDMA_Channel2
#define LCD_DMA_CHANNEL_IRQ              GDMA0_Channel2_IRQn



void rtk_lcd_hal_init(void);
void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);

void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_rect_fill(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h,
                           uint32_t color);
void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_transfer_done(void);


uint32_t rtk_lcd_hal_get_width(void);
uint32_t rtk_lcd_hal_get_height(void);
uint32_t rtk_lcd_hal_get_pixel_bits(void);

#ifdef __cplusplus
}
#endif

#endif /* _LCD_ST77916G5_320_QSPI_H_ */
