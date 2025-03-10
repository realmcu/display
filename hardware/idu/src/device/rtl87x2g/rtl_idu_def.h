/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* \file     rtl_idu_def.h
* \brief    GPIO related definitions for RTL8762G
* \details
* \author
* \date     2023-11-15
* \version  v1.1
* *********************************************************************************************************
*/

#ifndef RTL_IDU_REG_H
#define RTL_IDU_REG_H

#include "utils/rtl_utils.h"
#include "address_map.h"

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

/*============================================================================*
 *                         IDU Defines
 *============================================================================*/

/*============================================================================*
 *                         IDU Registers Memory Map
 *============================================================================*/
typedef struct
{
    __IO uint32_t IDU_CTL0;                                    /*< 0x00 */
    __IO uint32_t IDU_CTL1;                                    /*< 0x04 */
    __IO uint32_t PIC_DECOMPRESS_TOTAL_PIXEL;                   /*< 0x08 */
    __IO uint32_t PIC_RAW_WIDTH;                                /*< 0x0C */
    __IO uint32_t TX_COLUMN_START;                              /*< 0x10 */
    __IO uint32_t TX_COLUMN_END;                                /*< 0x14 */
    __IO uint32_t DECOMPRESS_OUTPUT_PIXEL;                      /*< 0x18 */
    __IO uint32_t RLE_FASTLZ_CTL;                               /*< 0x1C */
    __IO uint32_t YUV_SBF_CTL;                                  /*< 0x20 */
    __IO uint32_t VERTICAL_COUNTER;                             /*< 0x24 */
    __IO uint32_t HORIZONTAL_COUNTER;                           /*< 0x28 */
    __IO uint32_t IDU_OUTPUT_COUNTER;                          /*< 0x2C */
    __IO uint32_t RESERVED1;                                    /*< 0x30 */
    __IO uint32_t COMPRESSED_DATA_SIZE;                         /*< 0x34 */
    __IO uint32_t RX_FIFO;                                      /*< 0x38 */
    __IO uint32_t TX_FIFO;                                      /*< 0x3C */
    __IO uint32_t RX_FIFO_OFFSET;                               /*< 0x40 */
    __IO uint32_t TX_FIFO_OFFSET;                               /*< 0x44 */
    __IO uint32_t RX_FIFO_DMA_THRESHOLD;                        /*< 0x48 */
    __IO uint32_t TX_FIFO_DMA_THRESHOLD;                        /*< 0x4C */
    __IO uint32_t RX_FIFO_INT_THRESHOLD;                        /*< 0x50 */
    __IO uint32_t TX_FIFO_INT_THRESHOLD;                        /*< 0x54 */
    __IO uint32_t INT_ENABLE;                                   /*< 0x58 */
    __IO uint32_t INT_MASK;                                     /*< 0x5C */
    __IO uint32_t INT_RAW_STATUS;                               /*< 0x60 */
    __IO uint32_t INT_STATUS;                                   /*< 0x64 */
    __IO uint32_t INT_CLEAR;                                    /*< 0x68 */
} IDU_TypeDef;

/*============================================================================*
 *                         IDU Declaration
 *============================================================================*/

/*============================================================================*
 *                         IDU Private Types
 *============================================================================*/

/*============================================================================*
 *                         IDU Registers and Field Descriptions
 *============================================================================*/
/* 0x00
    0       R/W    idu_reset_decompress_start           0x0
    1       R/W    idu_enable                           0x0
    7:2     R      reserved2                               0x0
    9:8     R/W    idu_algorithm                        0x0
    31:10   R      reserved1                               0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        uint32_t idu_reset_decompress_start: 1;
        uint32_t idu_enable: 1;
        const uint32_t reserved_1: 6;
            uint32_t idu_algorithm: 2;
            const uint32_t reserved_0: 22;
        } b;
    } IDU_CTL0_TypeDef;



    /* 0x04
        0       R      reserved                                0x0
        1       W1C    tx_fifo_clear                           0x0
        2       W1C    rx_fifo_clear                           0x0
        25:3    R      reserved2                               0x0
        27:26   R/W    pic_pixel_size                          0x0
        28      R      reserved1                               0x0
        29      R      idu_internal_buffer_empty             0x0
        31:30   R/W    head_throw_away_byte_num                0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t reserved_2: 1;
                uint32_t tx_fifo_clear: 1;
                uint32_t rx_fifo_clear: 1;
                const uint32_t reserved_1: 23;
                uint32_t pic_pixel_size: 2;
#if defined(RTL8763EP)
                uint32_t hw_handshake_mux: 1;
#else
                const uint32_t reserved: 1;
#endif
                const uint32_t idu_internal_buffer_empty: 1;
                uint32_t head_throw_away_byte_num: 2;
            } b;
        } IDU_CTL1_TypeDef;



    /* 0x08
        31:0    R/W    pic_decompress_total_pixel              0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t pic_decompress_total_pixel: 32;
            } b;
        } PIC_DECOMPRESS_TOTAL_PIXEL_TypeDef;



    /* 0x0C
        31:0    R/W    pic_raw_width                           0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t pic_raw_width: 32;
            } b;
        } PIC_RAW_WIDTH_TypeDef;



    /* 0x10
        31:0    R/W    tx_column_start                         0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t tx_column_start: 32;
            } b;
        } TX_COLUMN_START_TypeDef;



    /* 0x14
        31:0    R/W    tx_column_end                           0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t tx_column_end: 32;
            } b;
        } TX_COLUMN_END_TypeDef;



    /* 0x18
        31:0    R/W    decompress_output_pixel                 0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t decompress_output_pixel: 32;
            } b;
        } DECOMPRESS_OUTPUT_PIXEL_TypeDef;



    /* 0x1C
        1:0     R/W    pic_length1_size                        0x0
        3:2     R/W    pic_length2_size                        0x0
        31:4    R      reserved                                0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t pic_length1_size: 2;
                uint32_t pic_length2_size: 2;
                const uint32_t reserved_0: 28;
            } b;
        } RLE_FASTLZ_CTL_TypeDef;



    /* 0x20
        1:0     R/W    yuv_sample_type                         0x0
        3:2     R/W    yuv_blur_bit                            0x0
        31:4    R      reserved                                0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t yuv_sample_type: 2;
                uint32_t yuv_blur_bit: 2;
                const uint32_t reserved_0: 28;
            } b;
        } YUV_SBF_CTL_TypeDef;



    /* 0x24
        31:0    R      vertical_counter                        0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t vertical_counter: 32;
            } b;
        } VERTICAL_COUNTER_TypeDef;



    /* 0x28
        31:0    R      horizontal_counter                      0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t horizontal_counter: 32;
            } b;
        } HORIZONTAL_COUNTER_TypeDef;



    /* 0x2C
        31:0    R      idu_output_counter                    0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t idu_output_counter: 32;
            } b;
        } IDU_OUTPUT_COUNTER_TypeDef;


    /* 0x34
        31:0    R/W    compressed_data_size                    0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t compressed_data_size: 32;
            } b;
        } COMPRESSED_DATA_SIZE_TypeDef;



    /* 0x38
        31:0    R/W    rx_fifo                                 0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t rx_fifo: 32;
            } b;
        } IDU_RX_FIFO_TypeDef;



    /* 0x3C
        31:0    R/W    tx_fifo                                 0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t tx_fifo: 32;
            } b;
        } IDU_TX_FIFO_TypeDef;



    /* 0x40
        7:0     R      rx_fifo_offset                          0x0
        31:8    R      reserved                                0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t rx_fifo_offset: 8;
                const uint32_t reserved_0: 24;
            } b;
        } IDU_RX_FIFO_OFFSET_TypeDef;



    /* 0x44
        7:0     R      tx_fifo_offset                          0x0
        31:8    R      reserved                                0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t tx_fifo_offset: 8;
                const uint32_t reserved_0: 24;
            } b;
        } IDU_TX_FIFO_OFFSET_TypeDef;



    /* 0x48
        7:0     R/W    rx_fifo_dma_threshold                   0x0
        30:8    R      reserved                                0x0
        31      R/W    rx_dma_enable                           0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t rx_fifo_dma_threshold: 8;
                const uint32_t reserved_0: 23;
                uint32_t rx_dma_enable: 1;
            } b;
        } IDU_RX_FIFO_DMA_THRESHOLD_TypeDef;



    /* 0x4C
        7:0     R/W    tx_fifo_dma_threshold                   0x0
        30:8    R      reserved                                0x0
        31      R/W    tx_dma_enable                           0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t tx_fifo_dma_threshold: 8;
                const uint32_t reserved_0: 23;
                uint32_t tx_dma_enable: 1;
            } b;
        } IDU_TX_FIFO_DMA_THRESHOLD_TypeDef;



    /* 0x50
        7:0     R/W    rx_fifo_int_threshold                   0x0
        31:8    R      reserved                                0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t rx_fifo_int_threshold: 8;
                const uint32_t reserved_0: 24;
            } b;
        } IDU_RX_FIFO_INT_THRESHOLD_TypeDef;



    /* 0x54
        7:0     R/W    tx_fifo_int_threshold                   0x0
        31:8    R      reserved                                0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t tx_fifo_int_threshold: 8;
                const uint32_t reserved_0: 24;
            } b;
        } IDU_TX_FIFO_INT_THRESHOLD_TypeDef;



    /* 0x58
        0       R/W    idu_decompress_error_int_en          0x0
        1       R/W    tx_fifo_underflow_int_en                0x0
        2       R/W    tx_fifo_threshold_int_en                0x0
        3       R/W    rx_fifo_overflow_int_en                 0x0
        4       R/W    rx_fifo_threshold_int_en                0x0
        5       R/W    line_decompress_finish_int_en           0x0
        6       R/W    idu_decompress_finish_int_en          0x0
        31:7    R      reserved                                0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t idu_decompress_error_int_en: 1;
                uint32_t tx_fifo_underflow_int_en: 1;
                uint32_t tx_fifo_threshold_int_en: 1;
                uint32_t rx_fifo_overflow_int_en: 1;
                uint32_t rx_fifo_threshold_int_en: 1;
                uint32_t line_decompress_finish_int_en: 1;
                uint32_t idu_decompress_finish_int_en: 1;
                const uint32_t reserved_0: 25;
            } b;
        } IDU_INT_ENABLE_TypeDef;



    /* 0x5C
        0       R/W    idu_decompress_error_int_msk         0x1
        1       R/W    tx_fifo_underflow_int_msk               0x1
        2       R/W    tx_fifo_threshold_int_msk               0x1
        3       R/W    rx_fifo_overflow_int_msk                0x1
        4       R/W    rx_fifo_threshold_int_msk               0x1
        5       R/W    line_decompress_finish_int_msk          0x1
        6       R/W    idu_decompress_finish_int_msk         0x1
        31:7    R      reserved                                0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t idu_decompress_error_int_msk: 1;
                uint32_t tx_fifo_underflow_int_msk: 1;
                uint32_t tx_fifo_threshold_int_msk: 1;
                uint32_t rx_fifo_overflow_int_msk: 1;
                uint32_t rx_fifo_threshold_int_msk: 1;
                uint32_t line_decompress_finish_int_msk: 1;
                uint32_t idu_decompress_finish_int_msk: 1;
                const uint32_t reserved_0: 25;
            } b;
        } IDU_INT_MASK_TypeDef;



    /* 0x60
        0       R      idu_decompress_error_int_raw_status  0x0
        1       R      tx_fifo_underflow_int_raw_status        0x0
        2       R      tx_fifo_threshold_int_raw_status        0x0
        3       R      rx_fifo_overflow_int_raw_status         0x0
        4       R      rx_fifo_threshold_int_raw_status        0x0
        5       R      line_decompress_finish_int_raw_status   0x0
        6       R      idu_decompress_finish_int_raw_status  0x0
        31:7    R      reserved                                0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t idu_decompress_error_int_raw_status: 1;
                const uint32_t tx_fifo_underflow_int_raw_status: 1;
                const uint32_t tx_fifo_threshold_int_raw_status: 1;
                const uint32_t rx_fifo_overflow_int_raw_status: 1;
                const uint32_t rx_fifo_threshold_int_raw_status: 1;
                const uint32_t line_decompress_finish_int_raw_status: 1;
                const uint32_t idu_decompress_finish_int_raw_status: 1;
                const uint32_t reserved_0: 25;
            } b;
        } IDU_INT_RAW_STATUS_TypeDef;



    /* 0x64
        0       R      idu_decompress_error_int_status      0x0
        1       R      tx_fifo_underflow_int_status            0x0
        2       R      tx_fifo_threshold_int_status            0x0
        3       R      rx_fifo_overflow_int_status             0x0
        4       R      rx_fifo_threshold_int_status            0x0
        5       R      line_decompress_finish_int_status       0x0
        6       R      idu_decompress_finish_int_status      0x0
        31:7    R      reserved                                0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t idu_decompress_error_int_status: 1;
                const uint32_t tx_fifo_underflow_int_status: 1;
                const uint32_t tx_fifo_threshold_int_status: 1;
                const uint32_t rx_fifo_overflow_int_status: 1;
                const uint32_t rx_fifo_threshold_int_status: 1;
                const uint32_t line_decompress_finish_int_status: 1;
                const uint32_t idu_decompress_finish_int_status: 1;
                const uint32_t reserved_0: 25;
            } b;
        } IDU_INT_STATUS_TypeDef;



    /* 0x68
        0       W1C    idu_decompress_error_int_clr         0x0
        1       W1C    tx_fifo_underflow_int_clr               0x0
        2       W1C    tx_fifo_threshold_int_clr               0x0
        3       W1C    rx_fifo_overflow_int_clr                0x0
        4       W1C    rx_fifo_threshold_int_clr               0x0
        5       W1C    line_decompress_finish_int_clr          0x0
        6       W1C    idu_decompress_finish_int_clr         0x0
        31:7    R      reserved                                0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t idu_decompress_error_int_clr: 1;
                uint32_t tx_fifo_underflow_int_clr: 1;
                uint32_t tx_fifo_threshold_int_clr: 1;
                uint32_t rx_fifo_overflow_int_clr: 1;
                uint32_t rx_fifo_threshold_int_clr: 1;
                uint32_t line_decompress_finish_int_clr: 1;
                uint32_t idu_decompress_finish_int_clr: 1;
                const uint32_t reserved_0: 25;
            } b;
        } IDU_INT_CLEAR_TypeDef;


#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* RTL_IDU_REG_H */
