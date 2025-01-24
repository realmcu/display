#include "rtl_gdma.h"
#include "rtl_rcc.h"
#include "rtl_nvic.h"
#include "rtl_idu.h"
#include "utils/rtl_utils.h"

void rtl_idu_channel_init_int(uint8_t *high_speed_channel, uint8_t *low_speed_channel);

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
