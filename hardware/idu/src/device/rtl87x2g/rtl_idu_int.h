/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtl_gdma.h"
#include "rtl_rcc.h"
#include "rtl_nvic.h"
#include "rtl_idu.h"
#include "utils/rtl_utils.h"

#define MAX_DMA_BLOCK_SIZE 65535

typedef enum
{
    IDU_HW_HANDSHAKE_DMA = 0x0,     /*!< Handshake with DMA. */
    IDU_HW_HANDSHAKE_PPE = 0x1,     /*!< Handshake with PPE. */
} IDU_HW_HANDSHAKE;

typedef struct
{
    IDU_ALGORITHM algorithm_type;                       /*!< Encode algorithm obtain from file header.
                                                             This parameter can be a value of @ref IDU_ALGORITHMS */
    IDU_THROW_AWAY_SIZE
    head_throw_away_byte_num;       /*!< Number of bytes to be thrown away at head of input data.
                                                             This parameter can be a value of @ref IDU_THROW_AWAY_SIZE */
    uint8_t pic_pixel_size;                             /*!< Length of decoded pixel in byte. */
    IDU_RLE_RUNLENGTH_SIZE
    pic_length2_size;            /*!< Run length of RLE encoeded image in stage 1.
                                                             This parameter can be a value of @ref IDU_RLE_RUNLENGTH_SIZE */
    IDU_RLE_RUNLENGTH_SIZE
    pic_length1_size;            /*!< Run length of RLE encoeded image in stage 2.
                                                             This parameter can be a value of @ref IDU_RLE_RUNLENGTH_SIZE */
    IDU_YUV_BLUR_BIT
    yuv_blur_bit;                      /*!< Number of bits blured in YUV sampled image.
                                                             This parameter can be a value of @ref IDU_YUV_BLUR_BIT */
    IDU_YUV_SAMPLE_TYPE yuv_sample_type;                /*!< YUV sampling type.
                                                             This parameter can be a value of @ref IDU_YUV_SAMPLE_TYPE */
    uint32_t pic_decompress_height;                     /*!< Number of lines to be decoded. */
    uint32_t pic_raw_width;                             /*!< Actual width of encoded image. */
    uint32_t tx_column_start;                           /*!< Start column of target decoding range. */
    uint32_t tx_column_end;                             /*!< End column of target decoding range. */
    uint32_t compressed_data_size;                      /*!< Size of encoded data. */
    uint8_t rx_fifo_dma_enable;                        /*!< Enable or disable RX FIFO DMA transmission. */
    uint8_t tx_fifo_dma_enable;                        /*!< Enable or disable TX FIFO DMA transmission. */
    uint8_t rx_fifo_dma_threshold;                     /*!< RX FIFO threshold that triggers data receiving. */
    uint8_t tx_fifo_dma_threshold;                     /*!< TX FIFO threshold that triggers data transmitting. */
    uint8_t tx_fifo_int_threshold;                     /*!< RX FIFO threshold that triggers RX FIFO threshold interrupt. */
    uint8_t rx_fifo_int_threshold;                     /*!< TX FIFO threshold that triggers TX FIFO threshold interrupt. */
} IDU_InitTypeDef;

typedef enum
{
    PIXEL_SIZE_16BIT = 0x00,
    PIXEL_SIZE_24BIT = 0x01,
    PIXEL_SIZE_32BIT = 0x02,
} IDU_PIXEL_SIZE_INT;

#define IS_IDU_PIXEL_BYTES(SIZE)            (SIZE == PIXEL_SIZE_16BIT || SIZE == PIXEL_SIZE_24BIT || SIZE == PIXEL_SIZE_32BIT)

uint32_t rtl_idu_get_dma_ctl_low_int(GDMA_ChannelTypeDef *dma);

GDMA_ChannelTypeDef *rtl_idu_get_dma_channel_int(uint8_t channel_num);

bool rtl_idu_get_dma_busy_state(uint8_t channel_num);

void rtl_idu_wait_dma_idle(GDMA_ChannelTypeDef *dma);

void rtl_idu_rx_handshake_init(GDMA_InitTypeDef *init_struct);

void rtl_idu_tx_handshake_init(GDMA_InitTypeDef *init_struct);

void rtl_idu_hw_handshake_init(IDU_InitTypeDef *init_struct);

void rtl_idu_fill_hw_hs_reg_int(IDU_CTL1_TypeDef *reg, IDU_InitTypeDef *IDU_init_struct);

uint8_t rtl_idu_get_dma_depth(uint8_t channel_num);

IDU_ERROR rtl_idu_decode_direct_int(uint8_t *file, IDU_decode_range *range,
                                    IDU_DMA_config *dma_cfg, GDMA_LLIDef *RX_LLI);

uint8_t *rtl_idu_malloc_int(uint32_t size);

void rtl_idu_free_int(void *p_mem);

uint8_t rtl_idu_get_decode_pixel_length(IDU_PIXEL_SIZE_INT pixel_size);

uint8_t rtl_idu_get_actual_pixel_length(IDU_PIXEL_SIZE_INT pixel_size);

bool rtl_idu_int_is_rx_dma_done(void);

bool rtl_idu_int_is_tx_dma_done(void);

uint8_t rtl_idu_int_get_rx_dma_channel(void);
uint8_t rtl_idu_int_get_tx_dma_channel(void);

void rtl_idu_int_rx_dma_setting(uint32_t src, uint32_t len, uint8_t dma_ch,
                                GDMA_LLIDef **RX_GDMA_LLIStruct);

void rtl_idu_int_tx_dma_setting(uint8_t tx_dma_ch, uint8_t tx_dma_depth,
                                uint32_t dst_addr, uint32_t data_size_word,
                                uint32_t line_stride, uint32_t line_size,
                                uint32_t line_num, GDMA_LLIDef **TX_GDMA_LLIStruct);

uint32_t rtl_idu_lock_int(void);

void rtl_idu_unlock_int(uint32_t s);
