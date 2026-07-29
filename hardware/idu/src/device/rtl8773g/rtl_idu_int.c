/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtl_idu_int.h"
#include "os_mem.h"
#include "dma_channel.h"
#include "trace.h"
#include "rtl876x_nvic.h"
#include "rtl876x_gdma.h"
#include "section.h"
#include "fmc_platform.h"
#include "vector_table.h"
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

GDMA_ChannelTypeDef *rtl_idu_get_dma_channel_int(uint8_t channel_num)
{
    return GDMA_GetGDMAChannelx(channel_num);
}

uint32_t rtl_idu_get_dma_ctl_low_int(GDMA_ChannelTypeDef *dma)
{
    return dma->GDMA_CTL_LOWx;
}

bool rtl_idu_get_dma_busy_state(uint8_t channel_num)
{
    return GDMA_GetChannelStatus(channel_num);
}

void rtl_idu_wait_dma_idle(GDMA_ChannelTypeDef *dma)
{
    GDMA_SafeSuspend(dma);
}

void rtl_idu_rx_handshake_init(GDMA_InitTypeDef *init_struct)
{
    init_struct->GDMA_DestHandshake = GDMA_Handshake_IDU_RX;
}

void rtl_idu_tx_handshake_init(GDMA_InitTypeDef *init_struct)
{
    init_struct->GDMA_SourceHandshake = GDMA_Handshake_IDU_TX;
}

void rtl_idu_hw_handshake_init(IDU_InitTypeDef *init_struct)
{
    return;
}

void rtl_idu_fill_hw_hs_reg_int(IDU_CTL1_TypeDef *reg, IDU_InitTypeDef *IDU_init_struct)
{
    return;
}

IDU_ERROR rtl_idu_decode_direct_int(uint8_t *file, IDU_decode_range *range,
                                    IDU_DMA_config *dma_cfg, GDMA_LLIDef *RX_LLI)
{
    return IDU_ERROR_INVALID_PARAM;
}

uint8_t *rtl_idu_malloc_int(uint32_t size)
{
    if (size == 0)
    {
        return NULL;
    }
    return os_mem_alloc(OS_MEM_TYPE_DATA, size);
}

void rtl_idu_free_int(void *p)
{
    if (p != NULL)
    {
        os_mem_free(p);
    }
}

uint8_t rtl_idu_get_decode_pixel_length(IDU_PIXEL_SIZE_INT pixel_size)
{
    switch (pixel_size)
    {
    case PIXEL_SIZE_8BIT: return 0;
    case PIXEL_SIZE_16BIT: return 1;
    case PIXEL_SIZE_24BIT: return 2;
    case PIXEL_SIZE_32BIT: return 3;
    default: return 0xFF;
    }
}

uint8_t rtl_idu_get_actual_pixel_length(IDU_PIXEL_SIZE_INT pixel_size)
{
    switch (pixel_size)
    {
    case PIXEL_SIZE_8BIT: return 1;
    case PIXEL_SIZE_16BIT: return 2;
    case PIXEL_SIZE_24BIT: return 3;
    case PIXEL_SIZE_32BIT: return 4;
    default: return 0xFF;
    }
}

uint8_t rtl_idu_get_dma_depth(uint8_t channel_num)
{
    if (channel_num <= 1)
    {
        return 32;
    }
    else if (channel_num <= 5)
    {
        return 8;
    }
    else if (channel_num <= 15)
    {
        return 4;
    }
    else
    {
        return 0xFF;
    }
}

typedef enum
{
    FLASH_NOR_RET_SUCCESS = 0,
} FLASH_NOR_RET_TYPE;

typedef FLASH_NOR_RET_TYPE(*FLASH_NOR_DMA_SETTING_FUNC)(uint32_t src, uint32_t dst, uint32_t len,
                                                        uint8_t dma_ch);
typedef void (*FLASH_NOR_ASYNC_CB)(void);

extern FLASH_NOR_RET_TYPE flash_nor_auto_dma_read_locked(uint32_t src, uint32_t dst, uint32_t len,
                                                         FLASH_NOR_DMA_SETTING_FUNC dma_setting_func,
                                                         FLASH_NOR_ASYNC_CB cb);

static volatile bool idu_rx_dma_done = false;
static volatile bool idu_tx_dma_done = false;
uint8_t current_rx_dma;
uint8_t current_tx_dma;
static GDMA_LLIDef *g_rx_dma_lli_struct = NULL;

RAM_TEXT_SECTION
static void idu_dma_cleanup(uint8_t dma_channel)
{
    GDMA_Cmd(dma_channel, DISABLE);
    GDMA_INTConfig(dma_channel, GDMA_INT_Transfer, DISABLE);
    NVIC_DisableIRQ(DMA_CH_IRQ(dma_channel));
    NVIC_ClearPendingIRQ(DMA_CH_IRQ(dma_channel));
    GDMA_ClearAllTypeINT(dma_channel);
}

FLASH_NOR_RET_TYPE idu_rx_dma_setting(uint32_t src, uint32_t dst, uint32_t len, uint8_t dma_ch)
{
    (void)dst;  // dst not used, target is fixed to IDU RX_FIFO

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

RAM_TEXT_SECTION
void idu_rx_dma_callback(void)
{
    idu_dma_cleanup(current_rx_dma);
    idu_rx_dma_done = true;
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
    idu_dma_cleanup(current_tx_dma);
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

uint8_t rtl_idu_int_get_rx_dma_channel(void)
{
    return current_rx_dma;
}

uint8_t rtl_idu_int_get_tx_dma_channel(void)
{
    return current_tx_dma;
}
