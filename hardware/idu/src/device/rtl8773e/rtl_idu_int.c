/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtl_idu.h"
#include "rtl_idu_int.h"
#include "dma_channel.h"
#include "os_mem.h"
#include "rtl876x_nvic.h"
#include "vector_table.h"
#include "fmc_platform.h"
#include "section.h"
#include "os_sync.h"

uint32_t rtl_idu_lock_int(void)
{
    uint32_t s = os_lock();
    return s;
}

void rtl_idu_unlock_int(uint32_t s)
{
    os_unlock(s);
}

void rtl_idu_channel_init_int(uint8_t *high_speed_channel, uint8_t *low_speed_channel)
{
    *high_speed_channel = 0xA5;
    *low_speed_channel = 0xA5;
    if (!GDMA_channel_request(high_speed_channel, NULL, true))
    {
        assert_param(*high_speed_channel != 0xA5);
        return;
    }
    if (!GDMA_channel_request(low_speed_channel, NULL, false))
    {
        assert_param(*low_speed_channel != 0xA5);
        return;
    }
    return;
}

GDMA_ChannelTypeDef *rtl_idu_get_dma_channel_int(uint8_t channel_num)
{
    return DMA_CH_BASE(channel_num);
}

uint32_t rtl_idu_get_dma_ctl_low_int(GDMA_ChannelTypeDef *dma)
{
    return dma->CTL_LOW;
}

bool rtl_idu_get_dma_busy_state(uint8_t channel_num)
{
    return (GDMA0->ChEnReg & (0x01 << channel_num));
}

void rtl_idu_wait_dma_idle(GDMA_ChannelTypeDef *dma)
{
    GDMA_SuspendCmd(dma, ENABLE);
    while (!(dma->CFG_LOW & BIT0));
}

void rtl_idu_rx_handshake_init(GDMA_InitTypeDef *init_struct)
{
    init_struct->GDMA_DestHandshake = GDMA_Handshake_RTZIP_RX;
}

void rtl_idu_tx_handshake_init(GDMA_InitTypeDef *init_struct)
{
    init_struct->GDMA_SourceHandshake = GDMA_Handshake_RTZIP_TX;
}

void rtl_idu_hw_handshake_init(IDU_InitTypeDef *init_struct)
{
    init_struct->hw_handshake = IDU_HW_HANDSHAKE_DMA;
}

void rtl_idu_fill_hw_hs_reg_int(IDU_CTL1_TypeDef *reg, IDU_InitTypeDef *IDU_init_struct)
{
    reg->b.hw_handshake_mux = IDU_init_struct->hw_handshake;
}

uint8_t rtl_idu_get_dma_depth(uint8_t channel_num)
{
    if (channel_num <= 1)
    {
        return 16;
    }
    else if (channel_num <= 5)
    {
        return 8;
    }
    else if (channel_num <= 11)
    {
        return 4;
    }
    else
    {
        return 0xFF;
    }
}

IDU_ERROR rtl_idu_decode_direct_int(uint8_t *file, IDU_decode_range *range,
                                    IDU_DMA_config *dma_cfg, GDMA_LLIDef *RX_LLI)
{
#if SUPPORT_HS_PPE
    uint32_t decompress_start_line;
    uint32_t decompress_end_line;
    uint32_t decompress_start_column;
    uint32_t decompress_end_column;
    RCC_PeriphClockCmd(APBPeriph_IDU, APBPeriph_IDU_CLOCK, ENABLE);
    if (file == NULL)
    {
        return IDU_ERROR_NULL_INPUT;
    }
    if (dma_cfg == NULL)
    {
        return IDU_ERROR_INVALID_PARAM;
    }
    uint32_t compressed_data_start_address = (uint32_t)file;
    IDU_file_header *header = (IDU_file_header *)compressed_data_start_address;
    if (range != NULL)
    {
        if ((range->start_line >= header->raw_pic_height) || (range->start_column >= header->raw_pic_width))
        {
            return IDU_ERROR_START_EXCEED_BOUNDARY;
        }
        if ((range->start_line > range->end_line) || (range->start_column > range->end_column))
        {
            return IDU_ERROR_START_LARGER_THAN_END;
        }
        if ((range->end_line >= header->raw_pic_height) || (range->end_column >= header->raw_pic_width))
        {
            return IDU_ERROR_END_EXCEED_BOUNDARY;
        }
        decompress_start_line = range->start_line;
        decompress_end_line = range->end_line;
        decompress_start_column = range->start_column;
        decompress_end_column = range->end_column;
    }
    else
    {
        decompress_start_line = 0;
        decompress_end_line = header->raw_pic_height - 1;
        decompress_start_column = 0;
        decompress_end_column = header->raw_pic_width - 1;
    }
    if ((!IS_IDU_ALGORITHM(header->algorithm_type.algorithm)) ||
        (!IS_IDU_PIXEL_BYTES(header->algorithm_type.pixel_bytes)))
    {
        return IDU_ERROR_INVALID_PARAM;
    }

    uint32_t start_line_address = IDU_Get_Line_Start_Address(compressed_data_start_address,
                                                             decompress_start_line);
    uint32_t compressed_data_size = IDU_Get_Line_Start_Address(compressed_data_start_address,
                                                               decompress_end_line + 1) - start_line_address;

    IDU_InitTypeDef IDU_struct_init;
    IDU_struct_init.algorithm_type            = (IDU_ALGORITHM)header->algorithm_type.algorithm;
    IDU_struct_init.head_throw_away_byte_num  = IDU_THROW_AWAY_0BYTE;
    IDU_struct_init.pic_pixel_size            = rtl_idu_get_decode_pixel_length(
                                                    (IDU_PIXEL_SIZE_INT)header->algorithm_type.pixel_bytes);
    IDU_struct_init.pic_decompress_height     = (decompress_end_line - decompress_start_line + 1);
    IDU_struct_init.pic_raw_width             = header->raw_pic_width;
    IDU_struct_init.tx_column_start           = decompress_start_column;
    IDU_struct_init.tx_column_end             = decompress_end_column ;
    IDU_struct_init.compressed_data_size      = compressed_data_size;
    IDU_struct_init.pic_length2_size          = (IDU_RLE_RUNLENGTH_SIZE)
                                                header->algorithm_type.feature_2;
    IDU_struct_init.pic_length1_size          = (IDU_RLE_RUNLENGTH_SIZE)
                                                header->algorithm_type.feature_1;
    IDU_struct_init.yuv_blur_bit              = (IDU_YUV_BLUR_BIT)header->algorithm_type.feature_2;
    IDU_struct_init.yuv_sample_type           = (IDU_YUV_SAMPLE_TYPE)
                                                header->algorithm_type.feature_1;
    IDU_struct_init.rx_fifo_dma_enable        = (uint32_t)ENABLE;
    IDU_struct_init.tx_fifo_dma_enable        = (uint32_t)ENABLE;
    IDU_struct_init.rx_fifo_dma_threshold     = IDU_RX_FIFO_DEPTH / 2;
    IDU_struct_init.tx_fifo_dma_threshold     = IDU_TX_FIFO_DEPTH / 2;
    IDU_struct_init.rx_fifo_int_threshold     = dma_cfg->RX_FIFO_INT_threshold;
    IDU_struct_init.tx_fifo_int_threshold     = dma_cfg->TX_FIFO_INT_threshold;
    IDU_struct_init.hw_handshake              = IDU_HW_HANDSHAKE_PPE;
    extern void IDU_Init(IDU_InitTypeDef * IDU_init_struct);
    IDU_Init(&IDU_struct_init);


    uint32_t DMA_compressed_data_size_word = ((compressed_data_size % 4) ?
                                              (compressed_data_size / 4 + 1) : (compressed_data_size / 4));
    /* Configure DMA */
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    GDMA_InitTypeDef RX_GDMA_InitStruct;
    GDMA_ChannelTypeDef *RX_DMA = rtl_idu_get_dma_channel_int(dma_cfg->RX_DMA_channel_num);
    GDMA_ChannelTypeDef *TX_DMA = rtl_idu_get_dma_channel_int(dma_cfg->TX_DMA_channel_num);
    uint32_t rx_block_num = 0;
    if (DMA_compressed_data_size_word >= 65535)
    {
        rx_block_num = DMA_compressed_data_size_word / 65535;
        if (DMA_compressed_data_size_word % 65535)
        {
            rx_block_num = rx_block_num + 1;
        }
    }
    /*--------------GDMA init-----------------------------*/
    GDMA_StructInit(&RX_GDMA_InitStruct);
    RX_GDMA_InitStruct.GDMA_ChannelNum          = dma_cfg->RX_DMA_channel_num;
    RX_GDMA_InitStruct.GDMA_BufferSize          = DMA_compressed_data_size_word;
    RX_GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    RX_GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    RX_GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;
    RX_GDMA_InitStruct.GDMA_SourceMsize         =
        GDMA_Msize_16;                         // 8 msize for source msize
    RX_GDMA_InitStruct.GDMA_DestinationMsize    =
        GDMA_Msize_8;                         // 8 msize for destiantion msize
    RX_GDMA_InitStruct.GDMA_DestinationDataSize =
        GDMA_DataSize_Word;                   // 32 bit width for destination transaction
    RX_GDMA_InitStruct.GDMA_SourceDataSize      =
        GDMA_DataSize_Word;                   // 32 bit width for source transaction
    RX_GDMA_InitStruct.GDMA_SourceAddr          = (uint32_t)start_line_address;
    RX_GDMA_InitStruct.GDMA_DestinationAddr     = (uint32_t)(&IDU->RX_FIFO);
    rtl_idu_rx_handshake_init(&RX_GDMA_InitStruct);
    if (rx_block_num)
    {
        RX_GDMA_InitStruct.GDMA_BufferSize = 65535;
        RX_GDMA_InitStruct.GDMA_Multi_Block_Mode = LLI_TRANSFER;
        RX_GDMA_InitStruct.GDMA_Multi_Block_En = 1;
        RX_GDMA_InitStruct.GDMA_Multi_Block_Struct = (uint32_t)RX_LLI;
    }
    GDMA_Init(RX_DMA, &RX_GDMA_InitStruct);
    if (rx_block_num)
    {
        for (int i = 0; i < rx_block_num; i++)
        {
            if (i == rx_block_num - 1)
            {
                RX_LLI[i].SAR = RX_GDMA_InitStruct.GDMA_SourceAddr + 65535 * 4 * i;
                RX_LLI[i].DAR = (uint32_t)(&IDU->RX_FIFO);
                RX_LLI[i].LLP = 0;
                /* configure low 32 bit of CTL register */
                RX_LLI[i].CTL_LOW = (BIT(0)
                                     | (RX_GDMA_InitStruct.GDMA_DestinationDataSize << 1)
                                     | (GDMA_DataSize_Byte << 4)
                                     | (RX_GDMA_InitStruct.GDMA_DestinationInc << 7)
                                     | (RX_GDMA_InitStruct.GDMA_SourceInc << 9)
                                     | (RX_GDMA_InitStruct.GDMA_DestinationMsize << 11)
                                     | (RX_GDMA_InitStruct.GDMA_SourceMsize << 14)
                                     | (RX_GDMA_InitStruct.GDMA_DIR << 20));
                /* configure high 32 bit of CTL register */
                uint32_t block_size = compressed_data_size - 65535 * 4 * i;
                RX_LLI[i].CTL_HIGH = block_size;
            }
            else
            {
                RX_LLI[i].SAR = RX_GDMA_InitStruct.GDMA_SourceAddr + 65535 * 4 * i;
                RX_LLI[i].DAR = (uint32_t)(&IDU->RX_FIFO);
                RX_LLI[i].LLP = (uint32_t)&RX_LLI[i + 1];
                /* configure low 32 bit of CTL register */
                RX_LLI[i].CTL_LOW = rtl_idu_get_dma_ctl_low_int(RX_DMA);
                /* configure high 32 bit of CTL register */
                RX_LLI[i].CTL_HIGH = 65535;
            }
        }
    }

    GDMA_Cmd(dma_cfg->RX_DMA_channel_num, ENABLE);

    IDU_ClearINTPendingBit(IDU_DECOMPRESS_FINISH_INT);
    IDU_INTConfig(IDU_DECOMPRESS_FINISH_INT, ENABLE);
    IDU_MaskINTConfig(IDU_DECOMPRESS_FINISH_INT, DISABLE);

    IDU_ClearINTPendingBit(IDU_DECOMPRESS_ERROR_INT);
    IDU_INTConfig(IDU_DECOMPRESS_ERROR_INT, ENABLE);
    IDU_MaskINTConfig(IDU_DECOMPRESS_ERROR_INT, DISABLE);
    return IDU_SUCCESS;
#else
    return IDU_ERROR_DECODE_FAIL;
#endif
}

uint8_t *rtl_idu_malloc_int(uint32_t size)
{
    if (size == 0)
    {
        return NULL;
    }
#if 1
    return os_mem_alloc(RAM_TYPE_DATA_ON, size);
#else
#endif
}

void rtl_idu_free_int(void *p)
{
    if (p == 0)
    {
        return;
    }
#if 1
    return os_mem_free(p);
#else
#endif
}

uint8_t rtl_idu_get_decode_pixel_length(IDU_PIXEL_SIZE_INT pixel_size)
{
    switch (pixel_size)
    {
    case PIXEL_SIZE_16BIT: return 0;
    case PIXEL_SIZE_24BIT: return 1;
    case PIXEL_SIZE_32BIT: return 2;
    default: return 0xFF;
    }
}

uint8_t rtl_idu_get_actual_pixel_length(IDU_PIXEL_SIZE_INT pixel_size)
{
    switch (pixel_size)
    {
    case PIXEL_SIZE_16BIT: return 2;
    case PIXEL_SIZE_24BIT: return 3;
    case PIXEL_SIZE_32BIT: return 4;
    default: return 0xFF;
    }
}

typedef enum
{
    FLASH_NOR_RET_SUCCESS = 0,
} FLASH_NOR_RET_TYPE;

typedef FLASH_NOR_RET_TYPE(*FLASH_NOR_DMA_SETTING_FUNC)(uint32_t src, uint32_t dst, uint32_t len,
                                                        uint8_t dma_ch);
typedef void (*FLASH_NOR_ASYNC_CB)(void);

extern FLASH_NOR_RET_TYPE(*flash_nor_auto_dma_read_locked)(uint32_t src, uint32_t dst, uint32_t len,
                                                           FLASH_NOR_DMA_SETTING_FUNC dma_setting_func,
                                                           FLASH_NOR_ASYNC_CB cb);
static volatile bool idu_rx_dma_done = false;
static volatile bool idu_tx_dma_done = false;
uint8_t current_rx_dma;
uint8_t current_tx_dma;
static GDMA_LLIDef *g_rx_dma_lli_struct = NULL;

RAM_TEXT_SECTION
void idu_rx_dma_callback(void)
{
    GDMA_ClearAllTypeINT(current_rx_dma);
    idu_rx_dma_done = true;
}



FLASH_NOR_RET_TYPE idu_rx_dma_setting(uint32_t src, uint32_t dst, uint32_t len, uint8_t dma_ch)
{
    g_rx_dma_lli_struct = NULL;
    idu_rx_dma_done = false;
    uint32_t DMA_data_size_word = ((len % 4) ? (len / 4 + 1) : (len / 4));
    current_rx_dma = dma_ch;
    /* Configure DMA */
    GDMA_InitTypeDef RX_GDMA_InitStruct;
    GDMA_ChannelTypeDef *RX_DMA = rtl_idu_get_dma_channel_int(dma_ch);

    uint32_t rx_block_num = 0;
    if (DMA_data_size_word >= MAX_DMA_BLOCK_SIZE)
    {
        rx_block_num = DMA_data_size_word / MAX_DMA_BLOCK_SIZE;
        if (DMA_data_size_word % MAX_DMA_BLOCK_SIZE)
        {
            rx_block_num = rx_block_num + 1;
        }
    }

    /*--------------GDMA init-----------------------------*/
    GDMA_StructInit(&RX_GDMA_InitStruct);
    RX_GDMA_InitStruct.GDMA_ChannelNum          = dma_ch;
    RX_GDMA_InitStruct.GDMA_BufferSize          = DMA_data_size_word;
    RX_GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    RX_GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    RX_GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;

    uint8_t rx_dma_depth = rtl_idu_get_dma_depth(dma_ch);
    if (rx_dma_depth == 16 || rx_dma_depth == 32)
    {
        RX_GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_8;
        RX_GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_8;
    }
    else if (rx_dma_depth == 8)
    {
        RX_GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_8;
        RX_GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_8;
    }
    else
    {
        RX_GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_4;
        RX_GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_4;
    }

    RX_GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Word;
    RX_GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Word;
    RX_GDMA_InitStruct.GDMA_SourceAddr          = (uint32_t)src;
    RX_GDMA_InitStruct.GDMA_DestinationAddr     = (uint32_t)(&IDU->RX_FIFO);
    rtl_idu_rx_handshake_init(&RX_GDMA_InitStruct);

    if (rx_block_num)
    {
        g_rx_dma_lli_struct = (GDMA_LLIDef *)rtl_idu_malloc_int(rx_block_num * sizeof(GDMA_LLIDef));
        RX_GDMA_InitStruct.GDMA_BufferSize = MAX_DMA_BLOCK_SIZE;
        RX_GDMA_InitStruct.GDMA_Multi_Block_Mode = LLI_TRANSFER;
        RX_GDMA_InitStruct.GDMA_Multi_Block_En = 1;
        RX_GDMA_InitStruct.GDMA_Multi_Block_Struct = (uint32_t)g_rx_dma_lli_struct;
    }

    GDMA_Init(RX_DMA, &RX_GDMA_InitStruct);

    if (rx_block_num && g_rx_dma_lli_struct != NULL)
    {
        for (int i = 0; i < rx_block_num; i++)
        {
            if (i == rx_block_num - 1)
            {
                g_rx_dma_lli_struct[i].SAR = (uint32_t)src + MAX_DMA_BLOCK_SIZE * 4 * i;
                g_rx_dma_lli_struct[i].DAR = (uint32_t)(&IDU->RX_FIFO);
                g_rx_dma_lli_struct[i].LLP = 0;
                g_rx_dma_lli_struct[i].CTL_LOW = (BIT(0)
                                                  | (RX_GDMA_InitStruct.GDMA_DestinationDataSize << 1)
                                                  | (GDMA_DataSize_Word << 4)
                                                  | (RX_GDMA_InitStruct.GDMA_DestinationInc << 7)
                                                  | (RX_GDMA_InitStruct.GDMA_SourceInc << 9)
                                                  | (RX_GDMA_InitStruct.GDMA_DestinationMsize << 11)
                                                  | (RX_GDMA_InitStruct.GDMA_SourceMsize << 14)
                                                  | (RX_GDMA_InitStruct.GDMA_DIR << 20));
                uint32_t block_size = DMA_data_size_word - MAX_DMA_BLOCK_SIZE * i;
                g_rx_dma_lli_struct[i].CTL_HIGH = block_size;
            }
            else
            {
                g_rx_dma_lli_struct[i].SAR = (uint32_t)src + MAX_DMA_BLOCK_SIZE * 4 * i;
                g_rx_dma_lli_struct[i].DAR = (uint32_t)(&IDU->RX_FIFO);
                g_rx_dma_lli_struct[i].LLP = (uint32_t)&g_rx_dma_lli_struct[i + 1];
                g_rx_dma_lli_struct[i].CTL_LOW = rtl_idu_get_dma_ctl_low_int(RX_DMA);
                g_rx_dma_lli_struct[i].CTL_HIGH = MAX_DMA_BLOCK_SIZE;
            }
        }
    }

    GDMA_INTConfig(dma_ch, GDMA_INT_Transfer, ENABLE);
    NVIC_InitTypeDef GDMA_NVIC_InitStruct;
    GDMA_NVIC_InitStruct.NVIC_IRQChannel = DMA_CH_IRQ(dma_ch);
    GDMA_NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    GDMA_NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&GDMA_NVIC_InitStruct);
    GDMA_ClearAllTypeINT(dma_ch);
    GDMA_Cmd(dma_ch, ENABLE);
    return FLASH_NOR_RET_SUCCESS;
}

void rtl_idu_int_rx_dma_setting(uint32_t src, uint32_t len, uint8_t dma_ch,
                                GDMA_LLIDef **RX_GDMA_LLIStruct)
{
    (void)dma_ch;
    *RX_GDMA_LLIStruct = NULL;
    g_rx_dma_lli_struct = NULL;
    idu_rx_dma_done = false;
    if (FMC_IS_SPIC0_ADDR(src))
    {
        flash_nor_auto_dma_read_locked(src, (uint32_t)(&IDU->RX_FIFO), len, idu_rx_dma_setting,
                                       idu_rx_dma_callback);
    }
    else
    {
        RamVectorTableUpdate(DMA_CH_VECTOR(dma_ch), (IRQ_Fun)idu_rx_dma_callback);
        idu_rx_dma_setting(src, (uint32_t)(&IDU->RX_FIFO), len, dma_ch);
    }
    *RX_GDMA_LLIStruct = g_rx_dma_lli_struct;
}


bool rtl_idu_int_is_rx_dma_done(void)
{
    return idu_rx_dma_done;
}

bool rtl_idu_int_is_tx_dma_done(void)
{
    return idu_tx_dma_done;
}

RAM_TEXT_SECTION
void idu_tx_dma_callback(void)
{
    GDMA_ClearAllTypeINT(current_tx_dma);
    idu_tx_dma_done = true;
}

void rtl_idu_int_tx_dma_setting(uint8_t tx_dma_ch, uint8_t tx_dma_depth,
                                uint32_t dst_addr, uint32_t data_size_word,
                                uint32_t line_stride, uint32_t line_size,
                                uint32_t line_num, GDMA_LLIDef **TX_GDMA_LLIStruct)
{
    idu_tx_dma_done = false;
    RamVectorTableUpdate(DMA_CH_VECTOR(tx_dma_ch), (IRQ_Fun)idu_tx_dma_callback);
    GDMA_ChannelTypeDef *TX_DMA = rtl_idu_get_dma_channel_int(tx_dma_ch);
    GDMA_InitTypeDef TX_GDMA_InitStruct;
    uint32_t tx_block_num = 0;
    uint32_t tx_block_size = data_size_word;
    *TX_GDMA_LLIStruct = NULL;

    if (line_stride != line_size)
    {
        tx_block_num = line_num;
        tx_block_size = line_size / 4;
        line_stride = line_stride / 4;
    }
    else if (data_size_word >= MAX_DMA_BLOCK_SIZE)
    {
        tx_block_num = data_size_word / MAX_DMA_BLOCK_SIZE;
        if (data_size_word % MAX_DMA_BLOCK_SIZE)
        {
            tx_block_num = tx_block_num + 1;
        }
        line_stride = MAX_DMA_BLOCK_SIZE;
        tx_block_size = MAX_DMA_BLOCK_SIZE;
    }

    GDMA_StructInit(&TX_GDMA_InitStruct);
    TX_GDMA_InitStruct.GDMA_ChannelNum          = tx_dma_ch;
    TX_GDMA_InitStruct.GDMA_BufferSize          = tx_block_size;
    TX_GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_PeripheralToMemory;
    TX_GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Fix;
    TX_GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Inc;
    TX_GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Word;
    TX_GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Word;

    if (tx_dma_depth == 16 || tx_dma_depth == 32)
    {
        TX_GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_8;
        TX_GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_16;
    }
    else if (tx_dma_depth == 8)
    {
        TX_GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_8;
        TX_GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_8;
    }
    else
    {
        TX_GDMA_InitStruct.GDMA_SourceMsize      = GDMA_Msize_8;
        TX_GDMA_InitStruct.GDMA_DestinationMsize = GDMA_Msize_4;
        TX_GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_HalfWord;
    }

    TX_GDMA_InitStruct.GDMA_SourceAddr      = (uint32_t)(&IDU->TX_FIFO);
    TX_GDMA_InitStruct.GDMA_DestinationAddr = (uint32_t)dst_addr;
    rtl_idu_tx_handshake_init(&TX_GDMA_InitStruct);

    if (tx_block_num)
    {
        *TX_GDMA_LLIStruct = (GDMA_LLIDef *)rtl_idu_malloc_int(tx_block_num * sizeof(GDMA_LLIDef));
        TX_GDMA_InitStruct.GDMA_BufferSize = tx_block_size;
        TX_GDMA_InitStruct.GDMA_Multi_Block_Mode = LLI_TRANSFER;
        TX_GDMA_InitStruct.GDMA_Multi_Block_En = 1;
        TX_GDMA_InitStruct.GDMA_Multi_Block_Struct = (uint32_t)(*TX_GDMA_LLIStruct);
    }

    GDMA_Init(TX_DMA, &TX_GDMA_InitStruct);

    if (tx_block_num && *TX_GDMA_LLIStruct != NULL)
    {
        for (int i = 0; i < tx_block_num; i++)
        {
            if (i == tx_block_num - 1)
            {
                (*TX_GDMA_LLIStruct)[i].SAR = (uint32_t)(&IDU->TX_FIFO);
                (*TX_GDMA_LLIStruct)[i].DAR = (uint32_t)dst_addr + line_stride * 4 * i;
                (*TX_GDMA_LLIStruct)[i].LLP = 0;
                (*TX_GDMA_LLIStruct)[i].CTL_LOW = (BIT(0)
                                                   | (TX_GDMA_InitStruct.GDMA_DestinationDataSize << 1)
                                                   | (GDMA_DataSize_Word << 4)
                                                   | (TX_GDMA_InitStruct.GDMA_DestinationInc << 7)
                                                   | (TX_GDMA_InitStruct.GDMA_SourceInc << 9)
                                                   | (TX_GDMA_InitStruct.GDMA_DestinationMsize << 11)
                                                   | (TX_GDMA_InitStruct.GDMA_SourceMsize << 14)
                                                   | (TX_GDMA_InitStruct.GDMA_DIR << 20));
                uint32_t block_size = data_size_word - tx_block_size * i;
                (*TX_GDMA_LLIStruct)[i].CTL_HIGH = block_size;
            }
            else
            {
                (*TX_GDMA_LLIStruct)[i].SAR = (uint32_t)(&IDU->TX_FIFO);
                (*TX_GDMA_LLIStruct)[i].DAR = (uint32_t)dst_addr + line_stride * 4 * i;
                (*TX_GDMA_LLIStruct)[i].LLP = (uint32_t) & ((*TX_GDMA_LLIStruct)[i + 1]);
                (*TX_GDMA_LLIStruct)[i].CTL_LOW = rtl_idu_get_dma_ctl_low_int(TX_DMA);
                (*TX_GDMA_LLIStruct)[i].CTL_HIGH = tx_block_size;
            }
        }
    }
    GDMA_INTConfig(tx_dma_ch, GDMA_INT_Transfer, ENABLE);
    NVIC_InitTypeDef GDMA_NVIC_InitStruct;
    GDMA_NVIC_InitStruct.NVIC_IRQChannel = DMA_CH_IRQ(tx_dma_ch);
    GDMA_NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    GDMA_NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&GDMA_NVIC_InitStruct);
    GDMA_ClearAllTypeINT(tx_dma_ch);
    GDMA_Cmd(tx_dma_ch, ENABLE);
    current_tx_dma = tx_dma_ch;
}

uint8_t rtl_idu_int_get_dma_ch(void)
{
    return current_rx_dma;
}

uint8_t rtl_idu_int_get_rx_dma_channel(void)
{
    return current_rx_dma;
}

uint8_t rtl_idu_int_get_tx_dma_channel(void)
{
    return current_tx_dma;
}
