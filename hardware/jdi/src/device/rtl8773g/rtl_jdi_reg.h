/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RTL_JDI_REG_H
#define RTL_JDI_REG_H

#include <stdint.h>
#include <stdbool.h>
#include "rtl876x.h"

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

#define JDI_REG_BASE      0x40019400
#define __IO volatile
#define __I  volatile
/*============================================================================*
 *                         JDI Registers Memory Map
 *============================================================================*/
typedef struct
{
    __IO uint32_t  CTL0;                                                                 //0X400
    __IO uint32_t  CTL1;                                                                 //0X404
    __IO uint32_t  MODE_SEL;                                                             //0X408
    __IO uint32_t  DATA_TRANSFER_DUMMY;                                                  //0X40C
    __IO uint32_t  DATA_WRITTEN_HORIZONTAL;                                              //0X410
    __IO uint32_t  DATA_WRITTEN_VERTICAL;                                                //0X414
    __IO uint32_t  OUTPUT_COUNTER;                                                       //0X418
    __IO uint32_t  OUTPUT_LINE_COUNTER;                                                  //0X41C
    __IO uint32_t  RX_FIFO;                                                              //0X420
    __IO uint32_t  TX_FIFO;                                                              //0X424
    __IO uint32_t  RX_FIFO_OFFSET;                                                       //0X428
    __IO uint32_t  TX_FIFO_OFFSET;                                                       //0X42C
    __IO uint32_t  RX_FIFO_DMA_THRESHOLD;                                                //0X430
    __IO uint32_t  TX_FIFO_DMA_THRESHOLD;                                                //0X434
    __IO uint32_t  RX_FIFO_INT_THRESHOLD;                                                //0X438
    __IO uint32_t  TX_FIFO_INT_THRESHOLD;                                                //0X43C
    __IO uint32_t  INT_ENABLE;                                                           //0X440
    __IO uint32_t  INT_MASK;                                                             //0X444
    __IO uint32_t  INT_RAW_STATUS;                                                       //0X448
    __IO uint32_t  INT_STATUS;                                                           //0X44C
    __IO uint32_t  INT_CLEAR;                                                            //0X450
} JDI_TypeDef;

/* 0x400
    0       R/WAC  rt_jdi_mip_start                    1'h0
    31:1    R      reserved_0                          31'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t rt_jdi_mip_start: 1;
        __I uint32_t reserved_0: 31;
    } b;
} JDI_CTL0_TypeDef;



/* 0x404
    0       W1C    output_counter_clear                1'h0
    1       W1C    output_line_counter_clear           1'h0
    2       W1C    tx_fifo_clear                       1'h0
    3       W1C    rx_fifo_clear                       1'h0
    5:4     R/W    output_format                       2'h1
    6       R/W    dummy_bit_for_4bit                  1'h0
    8:7     R/W    input_format                        2'h0
    9       R/W    ext_tx_fifo_en                      1'h0
    31:10   R      reserved_0                          23'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t output_counter_clear: 1;
        __IO uint32_t output_line_counter_clear: 1;
        __IO uint32_t tx_fifo_clear: 1;
        __IO uint32_t rx_fifo_clear: 1;
        __IO uint32_t output_format: 2;
        __IO uint32_t dummy_bit_for_4bit: 1;
        __IO uint32_t input_format: 2;
        __IO uint32_t ext_tx_fifo_en: 1;
        __I uint32_t reserved_0: 22;
    } b;
} JDI_CTL1_TypeDef;



/* 0x408
    5:0     R/W    mode_select                         6'h0
    31:6    R      reserved_0                          26'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t mode_select: 6;
        __I uint32_t reserved_0: 26;
    } b;
} JDI_MODE_SEL_TypeDef;



/* 0x40C
    15:0    R/W    data_transfer_dummy_end             16'h0
    21:16   R/W    data_transfer_dummy_mid             6'h0
    31:22   R      reserved_0                          10'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t data_transfer_dummy_end: 16;
        __IO uint32_t data_transfer_dummy_mid: 6;
        __I uint32_t reserved_0: 10;
    } b;
} JDI_DATA_TRANSFER_DUMMY_TypeDef;



/* 0x410
    11:0    R/W    jdi_mip_lcd_width                   12'h0
    31:12   R      reserved_0                          20'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t jdi_mip_lcd_width: 12;
        __I uint32_t reserved_0: 20;
    } b;
} JDI_DATA_WRITTEN_HORIZONTAL_TypeDef;



/* 0x414
    9:0     R/W    refresh_start_line_addr             10'h0
    15:10   R      reserved_0                          6'h0
    25:16   R/W    refresh_line_num                    10'h0
    27:26   R      reserved_1                          2'h0
    30:28   R/W    refresh_line_addr_incr_num          3'h0
    31      R/W    refresh_line_addr_incr              1'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t refresh_start_line_addr: 10;
        __I uint32_t reserved_1: 6;
        __IO uint32_t refresh_line_num: 10;
        __I uint32_t reserved_0: 2;
        __IO uint32_t refresh_line_addr_incr_num: 3;
        __IO uint32_t refresh_line_addr_incr: 1;
    } b;
} JDI_DATA_WRITTEN_VERTICAL_TypeDef;



/* 0x418
    31:0    R      output_counter                      32'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t output_counter: 32;
    } b;
} JDI_OUTPUT_COUNTER_TypeDef;



/* 0x41C
    9:0     R      output_line_counter                 10'h0
    31:10   R      reserved_0                          22'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t output_line_counter: 10;
        __I uint32_t reserved_0: 22;
    } b;
} JDI_OUTPUT_LINE_COUNTER_TypeDef;



/* 0x420
    7:0     R/W    rx_fifo_data                        8'h0
    31:8    R      reserved_0                          24'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t rx_fifo_data: 8;
        __I uint32_t reserved_0: 24;
    } b;
} JDI_RX_FIFO_TypeDef;



/* 0x424
    7:0     R/W    tx_fifo_data                        8'h0
    31:8    R      reserved_0                          24'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t tx_fifo_data: 8;
        __I uint32_t reserved_0: 24;
    } b;
} JDI_TX_FIFO_TypeDef;



/* 0x428
    7:0     R      rx_fifo_offset                      8'h0
    31:8    R      reserved_0                          24'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t rx_fifo_offset: 8;
        __I uint32_t reserved_0: 24;
    } b;
} JDI_RX_FIFO_OFFSET_TypeDef;



/* 0x42C
    7:0     R      tx_fifo_offset                      8'h0
    31:8    R      reserved_0                          24'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t tx_fifo_offset: 8;
        __I uint32_t reserved_0: 24;
    } b;
} JDI_TX_FIFO_OFFSET_TypeDef;



/* 0x430
    7:0     R/W    rx_fifo_dma_threshold               8'h0
    30:8    R      reserved_0                          23'h0
    31      R/W    rx_dma_enable                       1'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t rx_fifo_dma_threshold: 8;
        __I uint32_t reserved_0: 23;
        __IO uint32_t rx_dma_enable: 1;
    } b;
} JDI_RX_FIFO_DMA_THRESHOLD_TypeDef;



/* 0x434
    7:0     R/W    tx_fifo_dma_threshold               8'h0
    30:8    R      reserved_0                          23'h0
    31      R/W    tx_dma_enable                       1'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t tx_fifo_dma_threshold: 8;
        __I uint32_t reserved_0: 23;
        __IO uint32_t tx_dma_enable: 1;
    } b;
} JDI_TX_FIFO_DMA_THRESHOLD_TypeDef;



/* 0x438
    7:0     R/W    rx_fifo_int_threshold               8'h0
    31:8    R      reserved_0                          24'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t rx_fifo_int_threshold: 8;
        __I uint32_t reserved_0: 24;
    } b;
} JDI_RX_FIFO_INT_THRESHOLD_TypeDef;



/* 0x43C
    7:0     R/W    tx_fifo_int_threshold               8'h0
    31:8    R      reserved_0                          24'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t tx_fifo_int_threshold: 8;
        __I uint32_t reserved_0: 24;
    } b;
} JDI_TX_FIFO_INT_THRESHOLD_TypeDef;



/* 0x440
    0       R/W    tx_fifo_underflow_int_en            1'h0
    1       R/W    tx_fifo_threshold_int_en            1'h0
    2       R/W    rx_fifo_overflow_int_en             1'h0
    3       R/W    rx_fifo_threshold_int_en            1'h0
    4       R/W    output_finish_int_en                1'h0
    31:5    R      reserved_0                          27'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t tx_fifo_underflow_int_en: 1;
        __IO uint32_t tx_fifo_threshold_int_en: 1;
        __IO uint32_t rx_fifo_overflow_int_en: 1;
        __IO uint32_t rx_fifo_threshold_int_en: 1;
        __IO uint32_t output_finish_int_en: 1;
        __I uint32_t reserved_0: 27;
    } b;
} JDI_INT_ENABLE_TypeDef;



/* 0x444
    0       R/W    tx_fifo_underflow_int_msk           1'h1
    1       R/W    tx_fifo_threshold_int_msk           1'h1
    2       R/W    rx_fifo_overflow_int_msk            1'h1
    3       R/W    rx_fifo_threshold_int_msk           1'h1
    4       R/W    output_finish_int_msk               1'h1
    31:5    R      reserved_0                          27'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t tx_fifo_underflow_int_msk: 1;
        __IO uint32_t tx_fifo_threshold_int_msk: 1;
        __IO uint32_t rx_fifo_overflow_int_msk: 1;
        __IO uint32_t rx_fifo_threshold_int_msk: 1;
        __IO uint32_t output_finish_int_msk: 1;
        __I uint32_t reserved_0: 27;
    } b;
} JDI_INT_MASK_TypeDef;



/* 0x448
    0       R      tx_fifo_underflow_int_raw_status    1'h0
    1       R      tx_fifo_threshold_int_raw_status    1'h0
    2       R      rx_fifo_overflow_int_raw_status     1'h0
    3       R      rx_fifo_threshold_int_raw_status    1'h0
    4       R      output_finish_int_raw_status        1'h0
    31:5    R      reserved_0                          27'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t tx_fifo_underflow_int_raw_status: 1;
        __I uint32_t tx_fifo_threshold_int_raw_status: 1;
        __I uint32_t rx_fifo_overflow_int_raw_status: 1;
        __I uint32_t rx_fifo_threshold_int_raw_status: 1;
        __I uint32_t output_finish_int_raw_status: 1;
        __I uint32_t reserved_0: 27;
    } b;
} JDI_INT_RAW_STATUS_TypeDef;



/* 0x44C
    0       R      tx_fifo_underflow_int_status        1'h0
    1       R      tx_fifo_threshold_int_status        1'h0
    2       R      rx_fifo_overflow_int_status         1'h0
    3       R      rx_fifo_threshold_int_status        1'h0
    4       R      output_finish_int_status            1'h0
    31:5    R      reserved_0                          27'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t tx_fifo_underflow_int_status: 1;
        __I uint32_t tx_fifo_threshold_int_status: 1;
        __I uint32_t rx_fifo_overflow_int_status: 1;
        __I uint32_t rx_fifo_threshold_int_status: 1;
        __I uint32_t output_finish_int_status: 1;
        __I uint32_t reserved_0: 27;
    } b;
} JDI_INT_STATUS_TypeDef;



/* 0x450
    0       W1C    tx_fifo_underflow_int_clr           1'h0
    1       W1C    tx_fifo_threshold_int_clr           1'h0
    2       W1C    rx_fifo_overflow_int_clr            1'h0
    3       W1C    rx_fifo_threshold_int_clr           1'h0
    4       W1C    output_finish_int_clr               1'h0
    31:5    R      reserved_0                          27'h0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t tx_fifo_underflow_int_clr: 1;
        __IO uint32_t tx_fifo_threshold_int_clr: 1;
        __IO uint32_t rx_fifo_overflow_int_clr: 1;
        __IO uint32_t rx_fifo_threshold_int_clr: 1;
        __IO uint32_t output_finish_int_clr: 1;
        __I uint32_t reserved_0: 27;
    } b;
} JDI_INT_CLEAR_TypeDef;

#ifdef  __cplusplus
}
#endif /* __cplusplus */
#endif /* RTL_JDI_REG_H */
