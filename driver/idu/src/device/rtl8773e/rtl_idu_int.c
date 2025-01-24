#include "rtl_idu.h"
#include "rtl_idu_int.h"
#include "stdio.h"
#include "dma_channel.h"

void rtl_idu_channel_init_int(uint8_t *high_speed_channel, uint8_t *low_speed_channel)
{
    *high_speed_channel = 0xA5;
    *low_speed_channel = 0xA5;
    if (!GDMA_channel_request(high_speed_channel, NULL, true))
    {
        assert_param(*high_speed_channel != 0xA5);
        return;
    }
    if (!GDMA_channel_request(low_speed_channel, NULL, true))
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
    IDU_struct_init.head_throw_away_byte_num  = THROW_AWAY_0BYTE;
    IDU_struct_init.pic_pixel_size            = (IDU_PIXEL_SIZE)header->algorithm_type.pixel_bytes;
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
}
