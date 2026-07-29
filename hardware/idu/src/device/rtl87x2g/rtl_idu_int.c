/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtl_idu_int.h"
#include "vector_table.h"
#if 1
#include "os_mem.h"
#endif
uint32_t rtl_idu_lock_int(void)
{
    return 0;
}

void rtl_idu_unlock_int(uint32_t s)
{
    (void)s;
}

GDMA_ChannelTypeDef *rtl_idu_get_dma_channel_int(uint8_t channel_num)
{
    return GDMA_GetGDMAChannelx(channel_num);
}

uint32_t rtl_idu_get_dma_ctl_low_int(GDMA_ChannelTypeDef *dma)
{
    return dma->GDMA_CTLx_L;
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
    init_struct->GDMA_Secure_En = ENABLE;
    init_struct->GDMA_DestHandshake = GDMA_Handshake_IDU_RX;
}

void rtl_idu_tx_handshake_init(GDMA_InitTypeDef *init_struct)
{
    init_struct->GDMA_Secure_En = ENABLE;
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

uint8_t rtl_idu_get_dma_depth(uint8_t channel_num)
{
    if (channel_num <= 1)
    {
        return 16;
    }
    else if (channel_num <= 9)
    {
        return 8;
    }
    else
    {
        return 0xFF;
    }
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
#if 1
    return os_mem_alloc(RAM_TYPE_DATA_ON, size);
#else
#endif
}

void rtl_idu_free_int(void *p_mem)
{
    if (p_mem == NULL)
    {
        return;
    }
#if 1
    os_mem_free(p_mem);
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

static IRQn_Type get_gdma_irq(uint8_t ch)
{
    static IRQn_Type irq_map[] =
    {
        GDMA_Channel0_IRQn,  GDMA_Channel1_IRQn,  GDMA_Channel2_IRQn,  GDMA_Channel3_IRQn,
        GDMA_Channel4_IRQn,  GDMA_Channel5_IRQn,  GDMA_Channel6_IRQn,  GDMA_Channel7_IRQn,
        GDMA_Channel8_IRQn,  GDMA_Channel9_IRQn
    };
    if (ch > 9) { return 0xFF; }
    return irq_map[ch];
}

static VECTORn_Type get_gdma_vector(uint8_t ch)
{
    static VECTORn_Type vector_map[] =
    {
        GDMA0_Channel0_VECTORn,  GDMA0_Channel1_VECTORn,  GDMA0_Channel2_VECTORn,  GDMA0_Channel3_VECTORn,
        GDMA0_Channel4_VECTORn,  GDMA0_Channel5_VECTORn,  GDMA0_Channel6_VECTORn,  GDMA0_Channel7_VECTORn,
        GDMA0_Channel8_VECTORn,  GDMA0_Channel9_VECTORn
    };
    if (ch > 9) { return 0xFF; }
    return vector_map[ch];
}


static volatile bool idu_rx_dma_done = false;
static volatile bool idu_tx_dma_done = false;
uint8_t current_rx_dma;
uint8_t current_tx_dma;

void idu_rx_dma_callback(void)
{
    GDMA_ClearAllTypeINT(current_rx_dma);
    idu_rx_dma_done = true;
    NVIC_InitTypeDef GDMA_NVIC_InitStruct;
    GDMA_NVIC_InitStruct.NVIC_IRQChannel = get_gdma_irq(current_rx_dma);
    GDMA_NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    GDMA_NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&GDMA_NVIC_InitStruct);
}

void rtl_idu_int_rx_dma_setting(uint32_t src, uint32_t len, uint8_t dma_ch,
                                GDMA_LLIDef **RX_GDMA_LLIStruct)
{
    *RX_GDMA_LLIStruct = NULL;
    GDMA_LLIDef *g_rx_dma_lli_struct = NULL;
    idu_rx_dma_done = false;
    RamVectorTableUpdate(get_gdma_vector(dma_ch), (IRQ_Fun)idu_rx_dma_callback);
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
    GDMA_NVIC_InitStruct.NVIC_IRQChannel = get_gdma_irq(dma_ch);
    GDMA_NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    GDMA_NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&GDMA_NVIC_InitStruct);
    GDMA_ClearAllTypeINT(dma_ch);
    GDMA_Cmd(dma_ch, ENABLE);
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

void idu_tx_dma_callback(void)
{
    GDMA_ClearAllTypeINT(current_tx_dma);
    idu_tx_dma_done = true;
    NVIC_InitTypeDef GDMA_NVIC_InitStruct;
    GDMA_NVIC_InitStruct.NVIC_IRQChannel = get_gdma_irq(current_tx_dma);
    GDMA_NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    GDMA_NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&GDMA_NVIC_InitStruct);
}

void rtl_idu_int_tx_dma_setting(uint8_t tx_dma_ch, uint8_t tx_dma_depth,
                                uint32_t dst_addr, uint32_t data_size_word,
                                uint32_t line_stride, uint32_t line_size,
                                uint32_t line_num, GDMA_LLIDef **TX_GDMA_LLIStruct)
{
    idu_tx_dma_done = false;
    VECTORn_Type TX_DMA_IRQn = get_gdma_vector(tx_dma_ch);
    RamVectorTableUpdate(TX_DMA_IRQn, (IRQ_Fun)idu_tx_dma_callback);
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
    GDMA_NVIC_InitStruct.NVIC_IRQChannel = get_gdma_irq(tx_dma_ch);
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
