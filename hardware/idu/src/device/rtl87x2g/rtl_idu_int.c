#include "rtl_idu_int.h"
#if 1
#include "os_mem.h"
#endif

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
