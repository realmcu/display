/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* \file     rtl_idu.c
* \brief    This file provides all the LCDC HANDLER firmware functions.
* \details
* \author   astor zhang
* \date     2023-10-17
* \version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "stdio.h"
#include "rtl_idu.h"
#include "rtl_idu_int.h"

/*============================================================================*
 *                           Public Functions
 *============================================================================*/
GDMA_LLIDef RX_GDMA_LLIStruct[20];
GDMA_LLIDef TX_GDMA_LLIStruct[20];

void IDU_TxFifoClear(void)
{
    IDU_CTL1_TypeDef idu_reg_0x04 = {.d32 = IDU->IDU_CTL1};
    idu_reg_0x04.b.tx_fifo_clear = 1;
    IDU->IDU_CTL1 = idu_reg_0x04.d32;
}

void IDU_RxFifoClear(void)
{
    IDU_CTL1_TypeDef idu_reg_0x04 = {.d32 = IDU->IDU_CTL1};
    idu_reg_0x04.b.rx_fifo_clear = 1;
    IDU->IDU_CTL1 = idu_reg_0x04.d32;
}

ITStatus IDU_GetINTStatus(uint32_t IDU_INT)
{
    ITStatus bit_status = RESET;

    /* Check the parameters */
    //assert_param(IS_IDU_INT(IDU_INT));

    if (IDU->INT_STATUS & IDU_INT)
    {
        bit_status = SET;
    }

    return  bit_status;
}

void IDU_INTConfig(uint32_t IDU_INT, FunctionalState NewState)
{
    /* Check the parameters */
    //assert_param(IS_IDU_INT(IDU_INT_MSK));
    //assert_param(IS_FUNCTIONAL_STATE(NewState));

    if (NewState == ENABLE)
    {
        IDU->INT_ENABLE |= IDU_INT;
    }
    else
    {
        IDU->INT_ENABLE &= ~IDU_INT;
    }
}

void IDU_MaskINTConfig(uint32_t IDU_INT_MSK, FunctionalState NewState)
{
    /* Check the parameters */
    //assert_param(IS_IDU_INT(IDU_INT_MSK));
    //assert_param(IS_FUNCTIONAL_STATE(NewState));

    if (NewState == ENABLE)
    {
        IDU->INT_MASK |= IDU_INT_MSK;
    }
    else
    {
        IDU->INT_MASK &= ~IDU_INT_MSK;
    }
}

void IDU_ClearINTPendingBit(uint32_t IDU_INT)
{
    //assert_param(IS_IDU_INT(IDU_INT));
    IDU->INT_CLEAR |= IDU_INT;
}

void IDU_Cmd(FunctionalState state)
{
    assert_param(IS_FUNCTIONAL_STATE(state));
    IDU_CTL0_TypeDef idu_reg_0x00 = {.d32 = IDU->IDU_CTL0};
    if (state)
    {
        idu_reg_0x00.b.idu_enable = 1;
    }
    else
    {
        idu_reg_0x00.b.idu_enable = 0;
    }
    IDU->IDU_CTL0 = idu_reg_0x00.d32;
}

void IDU_Run(FunctionalState state)
{
    assert_param(IS_FUNCTIONAL_STATE(state));
    IDU_CTL0_TypeDef idu_reg_0x00 = {.d32 = IDU->IDU_CTL0};
    if (state)
    {
        idu_reg_0x00.b.idu_reset_decompress_start = 1;
    }
    else
    {
        idu_reg_0x00.b.idu_reset_decompress_start = 0;
    }
    IDU->IDU_CTL0 = idu_reg_0x00.d32;
}

ITStatus IDU_GetInternalBufferState(void)
{
    ITStatus rv = RESET;
    IDU_CTL1_TypeDef idu_reg_0x04 = {.d32 = IDU->IDU_CTL1};
    if (idu_reg_0x04.b.idu_internal_buffer_empty)
    {
        rv = SET;
    }
    return rv;
}

uint32_t IDU_GetDecompressLine(void)
{
    uint32_t reg_value = IDU->VERTICAL_COUNTER;
    return reg_value;
}

uint32_t IDU_GetHorizontalByteNum(void)
{
    uint32_t reg_value = IDU->HORIZONTAL_COUNTER;
    return reg_value;
}

uint32_t IDU_GetCompressedSize(void)
{
    uint32_t reg_value = IDU->COMPRESSED_DATA_SIZE;
    return reg_value;
}

uint32_t IDU_Get_Line_Start_Address(uint32_t compressed_start_address,
                                    uint32_t line_number)
{
    // line number start from0 and ends to (raw_pic_height-1)
    // return the absolute address of line

    IDU_file_header *rle_file_header = ((IDU_file_header *)(compressed_start_address));
    if (line_number > rle_file_header->raw_pic_height)
    {
//        DBG_DIRECT("[error]line number won't be equals to or bigger than pic height");
        return 0;
    }
    else
    {
        uint32_t *line_address = (uint32_t *)(compressed_start_address + 12 + line_number * 4);
        return (line_address[0] + compressed_start_address);
    }
}


void IDU_Init(IDU_InitTypeDef *IDU_init_struct)
{
    IDU_InitTypeDef *IDU_struct_init = IDU_init_struct;
    IDU_CTL0_TypeDef idu_reg_0x00 = {.d32 = IDU->IDU_CTL0};
    idu_reg_0x00.b.idu_algorithm = IDU_struct_init->algorithm_type;
    IDU->IDU_CTL0 = idu_reg_0x00.d32;

    IDU_CTL1_TypeDef idu_reg_0x04 = {.d32 = IDU->IDU_CTL1};
    idu_reg_0x04.b.head_throw_away_byte_num = IDU_struct_init->head_throw_away_byte_num;
    idu_reg_0x04.b.pic_pixel_size = IDU_struct_init->pic_pixel_size;
    rtl_idu_fill_hw_hs_reg_int(&idu_reg_0x04, IDU_struct_init);
    IDU->IDU_CTL1 = idu_reg_0x04.d32;

    IDU->PIC_RAW_WIDTH = IDU_struct_init->pic_raw_width;
    IDU->TX_COLUMN_START = IDU_struct_init->tx_column_start;
    IDU->TX_COLUMN_END = IDU_struct_init->tx_column_end;
    IDU->DECOMPRESS_OUTPUT_PIXEL = (
                                       IDU_struct_init->pic_decompress_height\
                                       * (IDU_struct_init->tx_column_end - IDU_struct_init->tx_column_start + 1)
                                   );
    IDU->PIC_DECOMPRESS_TOTAL_PIXEL = IDU_struct_init->pic_raw_width *
                                      IDU_struct_init->pic_decompress_height;

    RLE_FASTLZ_CTL_TypeDef idu_reg_0x1c = {.d32 = IDU->RLE_FASTLZ_CTL};
    idu_reg_0x1c.b.pic_length1_size = IDU_struct_init->pic_length1_size;
    idu_reg_0x1c.b.pic_length2_size = IDU_struct_init->pic_length2_size;
    IDU->RLE_FASTLZ_CTL = idu_reg_0x1c.d32;

    YUV_SBF_CTL_TypeDef idu_reg_0x20 = {.d32 = IDU->YUV_SBF_CTL};
    idu_reg_0x20.b.yuv_blur_bit = IDU_struct_init->yuv_blur_bit;
    idu_reg_0x20.b.yuv_sample_type = IDU_struct_init->yuv_sample_type;
    IDU->YUV_SBF_CTL = idu_reg_0x20.d32;

    IDU->COMPRESSED_DATA_SIZE = IDU_struct_init->compressed_data_size;

    RX_FIFO_DMA_THRESHOLD_TypeDef idu_reg_0x48 = {.d32 = IDU->RX_FIFO_DMA_THRESHOLD};
    idu_reg_0x48.b.rx_fifo_dma_threshold = IDU_struct_init->rx_fifo_dma_threshold;
    idu_reg_0x48.b.rx_dma_enable = IDU_struct_init->rx_fifo_dma_enable;
    IDU->RX_FIFO_DMA_THRESHOLD = idu_reg_0x48.d32;

    TX_FIFO_DMA_THRESHOLD_TypeDef idu_reg_0x4c = {.d32 = IDU->TX_FIFO_DMA_THRESHOLD};
    idu_reg_0x4c.b.tx_fifo_dma_threshold = IDU_struct_init->tx_fifo_dma_threshold;
    idu_reg_0x4c.b.tx_dma_enable = IDU_struct_init->tx_fifo_dma_enable;
    IDU->TX_FIFO_DMA_THRESHOLD = idu_reg_0x4c.d32;

    RX_FIFO_INT_THRESHOLD_TypeDef idu_reg_0x50 = {.d32 = IDU->RX_FIFO_INT_THRESHOLD};
    idu_reg_0x50.b.rx_fifo_int_threshold = IDU_struct_init->rx_fifo_int_threshold;
    IDU->RX_FIFO_INT_THRESHOLD = idu_reg_0x50.d32;

    TX_FIFO_INT_THRESHOLD_TypeDef idu_reg_0x54 = {.d32 = IDU->RX_FIFO_INT_THRESHOLD};
    idu_reg_0x54.b.tx_fifo_int_threshold = IDU_struct_init->tx_fifo_int_threshold;
    IDU->TX_FIFO_INT_THRESHOLD = idu_reg_0x54.d32;
}

IDU_ERROR IDU_Decode(uint8_t *file, IDU_decode_range *range, IDU_DMA_config *dma_cfg)
{
    IDU_ERROR err = IDU_SUCCESS;
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
    uint32_t decompressed_data_size = (decompress_end_line - decompress_start_line + 1) *
                                      (decompress_end_column - decompress_start_column + 1)\
                                      * (header->algorithm_type.pixel_bytes + 2);

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
    rtl_idu_hw_handshake_init(&IDU_struct_init);
    IDU_Init(&IDU_struct_init);


    uint32_t DMA_compressed_data_size_word = ((compressed_data_size % 4) ?
                                              (compressed_data_size / 4 + 1) : (compressed_data_size / 4));
    uint32_t DMA_decompressed_data_size_word = ((decompressed_data_size % 4) ?
                                                (decompressed_data_size / 4 + 1) : (decompressed_data_size / 4));
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
        RX_GDMA_InitStruct.GDMA_Multi_Block_Struct = (uint32_t)RX_GDMA_LLIStruct;
    }
    GDMA_Init(RX_DMA, &RX_GDMA_InitStruct);
    if (rx_block_num)
    {
        for (int i = 0; i < rx_block_num; i++)
        {
            if (i == rx_block_num - 1)
            {
                RX_GDMA_LLIStruct[i].SAR = RX_GDMA_InitStruct.GDMA_SourceAddr + 65535 * 4 * i;
                RX_GDMA_LLIStruct[i].DAR = (uint32_t)(&IDU->RX_FIFO);
                RX_GDMA_LLIStruct[i].LLP = 0;
                /* configure low 32 bit of CTL register */
                RX_GDMA_LLIStruct[i].CTL_LOW = (BIT(0)
                                                | (RX_GDMA_InitStruct.GDMA_DestinationDataSize << 1)
                                                | (GDMA_DataSize_Word << 4)
                                                | (RX_GDMA_InitStruct.GDMA_DestinationInc << 7)
                                                | (RX_GDMA_InitStruct.GDMA_SourceInc << 9)
                                                | (RX_GDMA_InitStruct.GDMA_DestinationMsize << 11)
                                                | (RX_GDMA_InitStruct.GDMA_SourceMsize << 14)
                                                | (RX_GDMA_InitStruct.GDMA_DIR << 20));
                /* configure high 32 bit of CTL register */
                uint32_t block_size = DMA_compressed_data_size_word - 65535 * i;
                RX_GDMA_LLIStruct[i].CTL_HIGH = block_size;
            }
            else
            {
                RX_GDMA_LLIStruct[i].SAR = RX_GDMA_InitStruct.GDMA_SourceAddr + 65535 * 4 * i;
                RX_GDMA_LLIStruct[i].DAR = (uint32_t)(&IDU->RX_FIFO);
                RX_GDMA_LLIStruct[i].LLP = (uint32_t)&RX_GDMA_LLIStruct[i + 1];
                /* configure low 32 bit of CTL register */
                RX_GDMA_LLIStruct[i].CTL_LOW = rtl_idu_get_dma_ctl_low_int(RX_DMA);
                /* configure high 32 bit of CTL register */
                RX_GDMA_LLIStruct[i].CTL_HIGH = 65535;
            }
        }
    }

    GDMA_InitTypeDef TX_GDMA_InitStruct;
    uint32_t tx_block_num = 0;
    if (DMA_decompressed_data_size_word >= 65535)
    {
        tx_block_num = DMA_decompressed_data_size_word / 65535;
        if (DMA_decompressed_data_size_word % 65535)
        {
            tx_block_num = tx_block_num + 1;
        }
    }
    /*--------------GDMA init-----------------------------*/
    GDMA_StructInit(&TX_GDMA_InitStruct);
    TX_GDMA_InitStruct.GDMA_ChannelNum          = dma_cfg->TX_DMA_channel_num;
    TX_GDMA_InitStruct.GDMA_BufferSize          = DMA_decompressed_data_size_word;
    TX_GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_PeripheralToMemory;
    TX_GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Fix;
    TX_GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Inc;
    TX_GDMA_InitStruct.GDMA_SourceMsize         =
        GDMA_Msize_8;                         // 8 msize for source msize
    TX_GDMA_InitStruct.GDMA_DestinationMsize    =
        GDMA_Msize_8;                         // 8 msize for destiantion msize
    TX_GDMA_InitStruct.GDMA_DestinationDataSize =
        GDMA_DataSize_Word;                   // 32 bit width for destination transaction
    TX_GDMA_InitStruct.GDMA_SourceDataSize      =
        GDMA_DataSize_Word;                   // 32 bit width for source transaction
    TX_GDMA_InitStruct.GDMA_SourceAddr          = (uint32_t)(&IDU->TX_FIFO);
    TX_GDMA_InitStruct.GDMA_DestinationAddr     = (uint32_t)dma_cfg->output_buf;
    rtl_idu_tx_handshake_init(&TX_GDMA_InitStruct);
    if (tx_block_num)
    {
        TX_GDMA_InitStruct.GDMA_BufferSize = 65535;
        TX_GDMA_InitStruct.GDMA_Multi_Block_Mode = LLI_TRANSFER;
        TX_GDMA_InitStruct.GDMA_Multi_Block_En = 1;
        TX_GDMA_InitStruct.GDMA_Multi_Block_Struct = (uint32_t)TX_GDMA_LLIStruct;
    }
    GDMA_Init(TX_DMA, &TX_GDMA_InitStruct);
    if (tx_block_num)
    {
        for (int i = 0; i < tx_block_num; i++)
        {
            if (i == tx_block_num - 1)
            {
                TX_GDMA_LLIStruct[i].SAR = (uint32_t)(&IDU->TX_FIFO);
                TX_GDMA_LLIStruct[i].DAR = (uint32_t)dma_cfg->output_buf + 65535 * 4 * i;
                TX_GDMA_LLIStruct[i].LLP = 0;
                /* configure low 32 bit of CTL register */
                TX_GDMA_LLIStruct[i].CTL_LOW = (BIT(0)
                                                | (TX_GDMA_InitStruct.GDMA_DestinationDataSize << 1)
                                                | (GDMA_DataSize_Word << 4)
                                                | (TX_GDMA_InitStruct.GDMA_DestinationInc << 7)
                                                | (TX_GDMA_InitStruct.GDMA_SourceInc << 9)
                                                | (TX_GDMA_InitStruct.GDMA_DestinationMsize << 11)
                                                | (TX_GDMA_InitStruct.GDMA_SourceMsize << 14)
                                                | (TX_GDMA_InitStruct.GDMA_DIR << 20));
                /* configure high 32 bit of CTL register */
                uint32_t block_size = DMA_decompressed_data_size_word - 65535 * i;
                TX_GDMA_LLIStruct[i].CTL_HIGH = block_size;
            }
            else
            {
                TX_GDMA_LLIStruct[i].SAR = (uint32_t)(&IDU->TX_FIFO);
                TX_GDMA_LLIStruct[i].DAR = (uint32_t)dma_cfg->output_buf + 65535 * 4 * i;
                TX_GDMA_LLIStruct[i].LLP = (uint32_t)&TX_GDMA_LLIStruct[i + 1];
                /* configure low 32 bit of CTL register */
                TX_GDMA_LLIStruct[i].CTL_LOW = rtl_idu_get_dma_ctl_low_int(TX_DMA);
                /* configure high 32 bit of CTL register */
                TX_GDMA_LLIStruct[i].CTL_HIGH = 65535;
            }
        }
    }
    GDMA_Cmd(dma_cfg->RX_DMA_channel_num, ENABLE);
    GDMA_Cmd(dma_cfg->TX_DMA_channel_num, ENABLE);
    IDU_ClearINTPendingBit(IDU_DECOMPRESS_FINISH_INT);
    IDU_INTConfig(IDU_DECOMPRESS_FINISH_INT, ENABLE);
    IDU_MaskINTConfig(IDU_DECOMPRESS_FINISH_INT, DISABLE);

    IDU_ClearINTPendingBit(IDU_DECOMPRESS_ERROR_INT);
    IDU_INTConfig(IDU_DECOMPRESS_ERROR_INT, ENABLE);
    IDU_MaskINTConfig(IDU_DECOMPRESS_ERROR_INT, DISABLE);

    IDU_Cmd(ENABLE);
    IDU_Run(ENABLE);
    while (IDU->IDU_CTL0 & BIT0);
    if (IDU_GetINTStatus(IDU_DECOMPRESS_ERROR_INT))
    {
        err =  IDU_ERROR_DECODE_FAIL;
    }
    if (rtl_idu_get_dma_busy_state(dma_cfg->RX_DMA_channel_num))
    {
        GDMA_SuspendCmd(TX_DMA, ENABLE);
        GDMA_SuspendCmd(RX_DMA, ENABLE);
        rtl_idu_wait_dma_idle(RX_DMA);
        rtl_idu_wait_dma_idle(TX_DMA);
        GDMA_Cmd(dma_cfg->RX_DMA_channel_num, DISABLE);
        GDMA_Cmd(dma_cfg->TX_DMA_channel_num, DISABLE);
    }
    IDU_RxFifoClear();
    IDU_Cmd(DISABLE);
    while (!(IDU->IDU_CTL1 & BIT29));
    IDU_TxFifoClear();
    return err;
}

IDU_ERROR IDU_Decode_Ex(uint8_t *file, IDU_decode_range *range, IDU_DMA_config *dma_cfg,
                        IDU_INT_CFG_t int_cfg)
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
    uint32_t decompressed_data_size = (decompress_end_line - decompress_start_line + 1) *
                                      (decompress_end_column - decompress_start_column + 1)\
                                      * (header->algorithm_type.pixel_bytes + 2);

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
    rtl_idu_hw_handshake_init(&IDU_struct_init);
    IDU_Init(&IDU_struct_init);


    uint32_t DMA_compressed_data_size_word = ((compressed_data_size % 4) ?
                                              (compressed_data_size / 4 + 1) : (compressed_data_size / 4));
    uint32_t DMA_decompressed_data_size_word = ((decompressed_data_size % 4) ?
                                                (decompressed_data_size / 4 + 1) : (decompressed_data_size / 4));
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
        RX_GDMA_InitStruct.GDMA_Multi_Block_Struct = (uint32_t)RX_GDMA_LLIStruct;
    }
    GDMA_Init(RX_DMA, &RX_GDMA_InitStruct);
    if (rx_block_num)
    {
        for (int i = 0; i < rx_block_num; i++)
        {
            if (i == rx_block_num - 1)
            {
                RX_GDMA_LLIStruct[i].SAR = RX_GDMA_InitStruct.GDMA_SourceAddr + 65535 * 4 * i;
                RX_GDMA_LLIStruct[i].DAR = (uint32_t)(&IDU->RX_FIFO);
                RX_GDMA_LLIStruct[i].LLP = 0;
                /* configure low 32 bit of CTL register */
                RX_GDMA_LLIStruct[i].CTL_LOW = (BIT(0)
                                                | (RX_GDMA_InitStruct.GDMA_DestinationDataSize << 1)
                                                | (GDMA_DataSize_Byte << 4)
                                                | (RX_GDMA_InitStruct.GDMA_DestinationInc << 7)
                                                | (RX_GDMA_InitStruct.GDMA_SourceInc << 9)
                                                | (RX_GDMA_InitStruct.GDMA_DestinationMsize << 11)
                                                | (RX_GDMA_InitStruct.GDMA_SourceMsize << 14)
                                                | (RX_GDMA_InitStruct.GDMA_DIR << 20));
                /* configure high 32 bit of CTL register */
                uint32_t block_size = compressed_data_size - 65535 * 4 * i;
                RX_GDMA_LLIStruct[i].CTL_HIGH = block_size;
            }
            else
            {
                RX_GDMA_LLIStruct[i].SAR = RX_GDMA_InitStruct.GDMA_SourceAddr + 65535 * 4 * i;
                RX_GDMA_LLIStruct[i].DAR = (uint32_t)(&IDU->RX_FIFO);
                RX_GDMA_LLIStruct[i].LLP = (uint32_t)&RX_GDMA_LLIStruct[i + 1];
                /* configure low 32 bit of CTL register */
                RX_GDMA_LLIStruct[i].CTL_LOW = rtl_idu_get_dma_ctl_low_int(RX_DMA);
                /* configure high 32 bit of CTL register */
                RX_GDMA_LLIStruct[i].CTL_HIGH = 65535;
            }
        }
    }

    GDMA_InitTypeDef TX_GDMA_InitStruct;
    uint32_t tx_block_num = 0;
    if (DMA_decompressed_data_size_word >= 65535)
    {
        tx_block_num = DMA_decompressed_data_size_word / 65535;
        if (DMA_decompressed_data_size_word % 65535)
        {
            tx_block_num = tx_block_num + 1;
        }
    }
    /*--------------GDMA init-----------------------------*/
    GDMA_StructInit(&TX_GDMA_InitStruct);
    TX_GDMA_InitStruct.GDMA_ChannelNum          = dma_cfg->TX_DMA_channel_num;
    TX_GDMA_InitStruct.GDMA_BufferSize          = DMA_decompressed_data_size_word;
    TX_GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_PeripheralToMemory;
    TX_GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Fix;
    TX_GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Inc;
    TX_GDMA_InitStruct.GDMA_SourceMsize         =
        GDMA_Msize_8;                         // 8 msize for source msize
    TX_GDMA_InitStruct.GDMA_DestinationMsize    =
        GDMA_Msize_8;                         // 8 msize for destiantion msize
    TX_GDMA_InitStruct.GDMA_DestinationDataSize =
        GDMA_DataSize_Word;                   // 32 bit width for destination transaction
    TX_GDMA_InitStruct.GDMA_SourceDataSize      =
        GDMA_DataSize_Word;                   // 32 bit width for source transaction
    TX_GDMA_InitStruct.GDMA_SourceAddr          = (uint32_t)(&IDU->TX_FIFO);
    TX_GDMA_InitStruct.GDMA_DestinationAddr     = (uint32_t)dma_cfg->output_buf;
    rtl_idu_tx_handshake_init(&TX_GDMA_InitStruct);
    if (tx_block_num)
    {
        TX_GDMA_InitStruct.GDMA_BufferSize = 65535;
        TX_GDMA_InitStruct.GDMA_Multi_Block_Mode = LLI_TRANSFER;
        TX_GDMA_InitStruct.GDMA_Multi_Block_En = 1;
        TX_GDMA_InitStruct.GDMA_Multi_Block_Struct = (uint32_t)TX_GDMA_LLIStruct;
    }
    GDMA_Init(TX_DMA, &TX_GDMA_InitStruct);
    if (tx_block_num)
    {
        for (int i = 0; i < tx_block_num; i++)
        {
            if (i == tx_block_num - 1)
            {
                TX_GDMA_LLIStruct[i].SAR = (uint32_t)(&IDU->TX_FIFO);
                TX_GDMA_LLIStruct[i].DAR = (uint32_t)dma_cfg->output_buf + 65535 * 4 * i;
                TX_GDMA_LLIStruct[i].LLP = 0;
                /* configure low 32 bit of CTL register */
                TX_GDMA_LLIStruct[i].CTL_LOW = (BIT(0)
                                                | (TX_GDMA_InitStruct.GDMA_DestinationDataSize << 1)
                                                | (GDMA_DataSize_Word << 4)
                                                | (TX_GDMA_InitStruct.GDMA_DestinationInc << 7)
                                                | (TX_GDMA_InitStruct.GDMA_SourceInc << 9)
                                                | (TX_GDMA_InitStruct.GDMA_DestinationMsize << 11)
                                                | (TX_GDMA_InitStruct.GDMA_SourceMsize << 14)
                                                | (TX_GDMA_InitStruct.GDMA_DIR << 20));
                /* configure high 32 bit of CTL register */
                uint32_t block_size = DMA_decompressed_data_size_word - 65535 * i;
                TX_GDMA_LLIStruct[i].CTL_HIGH = block_size;
            }
            else
            {
                TX_GDMA_LLIStruct[i].SAR = (uint32_t)(&IDU->TX_FIFO);
                TX_GDMA_LLIStruct[i].DAR = (uint32_t)dma_cfg->output_buf + 65535 * 4 * i;
                TX_GDMA_LLIStruct[i].LLP = (uint32_t)&TX_GDMA_LLIStruct[i + 1];
                /* configure low 32 bit of CTL register */
                TX_GDMA_LLIStruct[i].CTL_LOW = rtl_idu_get_dma_ctl_low_int(TX_DMA);
                /* configure high 32 bit of CTL register */
                TX_GDMA_LLIStruct[i].CTL_HIGH = 65535;
            }
        }
    }
    GDMA_Init(TX_DMA, &TX_GDMA_InitStruct);

    GDMA_Cmd(dma_cfg->RX_DMA_channel_num, ENABLE);
    GDMA_Cmd(dma_cfg->TX_DMA_channel_num, ENABLE);

    if (int_cfg.b.idu_decompress_finish_int)
    {
        IDU_ClearINTPendingBit(IDU_DECOMPRESS_FINISH_INT);
        IDU_INTConfig(IDU_DECOMPRESS_FINISH_INT, ENABLE);
        IDU_MaskINTConfig(IDU_DECOMPRESS_FINISH_INT, DISABLE);
    }
    if (int_cfg.b.idu_decompress_error_int)
    {
        IDU_ClearINTPendingBit(IDU_DECOMPRESS_ERROR_INT);
        IDU_INTConfig(IDU_DECOMPRESS_ERROR_INT, ENABLE);
        IDU_MaskINTConfig(IDU_DECOMPRESS_ERROR_INT, DISABLE);
    }
    if (int_cfg.b.line_decompress_finish_int)
    {
        IDU_ClearINTPendingBit(IDU_LINE_DECOMPRESS_FINISH_INT);
        IDU_INTConfig(IDU_LINE_DECOMPRESS_FINISH_INT, ENABLE);
        IDU_MaskINTConfig(IDU_LINE_DECOMPRESS_FINISH_INT, DISABLE);
    }
    if (int_cfg.b.rx_fifo_overflow_int)
    {
        IDU_ClearINTPendingBit(IDU_RX_FIFO_OVERFLOW_INT);
        IDU_INTConfig(IDU_RX_FIFO_OVERFLOW_INT, ENABLE);
        IDU_MaskINTConfig(IDU_RX_FIFO_OVERFLOW_INT, DISABLE);
    }
    if (int_cfg.b.rx_fifo_threshold_int)
    {
        IDU_ClearINTPendingBit(IDU_RX_FIFO_THRESHOLD_INT);
        IDU_INTConfig(IDU_RX_FIFO_THRESHOLD_INT, ENABLE);
        IDU_MaskINTConfig(IDU_RX_FIFO_THRESHOLD_INT, DISABLE);
    }
    if (int_cfg.b.tx_fifo_threshold_int)
    {
        IDU_ClearINTPendingBit(IDU_TX_FIFO_THRESHOLD_INT);
        IDU_INTConfig(IDU_TX_FIFO_THRESHOLD_INT, ENABLE);
        IDU_MaskINTConfig(IDU_TX_FIFO_THRESHOLD_INT, DISABLE);
    }
    if (int_cfg.b.tx_fifo_underflow_int)
    {
        IDU_ClearINTPendingBit(IDU_TX_FIFO_UNDERFLOW_INT);
        IDU_INTConfig(IDU_TX_FIFO_UNDERFLOW_INT, ENABLE);
        IDU_MaskINTConfig(IDU_TX_FIFO_UNDERFLOW_INT, DISABLE);
    }

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = IDU_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    IDU_Cmd(ENABLE);
    IDU_Run(ENABLE);
    return IDU_SUCCESS;
}

IDU_ERROR IDU_Decode_Direct(uint8_t *file, IDU_decode_range *range, IDU_DMA_config *dma_cfg)
{
    return rtl_idu_decode_direct_int(file, range, dma_cfg, RX_GDMA_LLIStruct);
}
/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/
