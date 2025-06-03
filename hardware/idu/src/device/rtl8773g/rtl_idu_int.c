#include "rtl_idu_int.h"
#include "os_mem.h"

void rtl_idu_channel_init_int(uint8_t *high_speed_channel, uint8_t *low_speed_channel)
{
    return;
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
