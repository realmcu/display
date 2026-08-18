/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "stdio.h"
#include "rtl_idu.h"
#include "rtl_idu_int.h"


/*============================================================================*
 *                           Busy-wait Timeout
 *============================================================================*/
/* Bounded busy-wait used by the polled IDU paths. (timed_out) is set to true
 * if the wait exceeds IDU_WAIT_MAX_ITER iterations (defined per chip in
 * rtl_idu_def.h, derived from that chip's CPU clock). This is a calibrated
 * loop-count safety net to prevent a hardware hang from wedging the CPU
 * forever, NOT a precise deadline. */
#define IDU_BUSY_WAIT(cond, timed_out)                  \
    do {                                                \
        uint32_t _cnt = 0;                              \
        (timed_out) = false;                            \
        while (cond) {                                  \
            if (++_cnt > IDU_WAIT_MAX_ITER) {           \
                (timed_out) = true;                     \
                break;                                  \
            }                                           \
        }                                               \
    } while (0)

/*============================================================================*
 *                           Static Functions
 *============================================================================*/
static void IDU_Init(IDU_InitTypeDef *IDU_init_struct)
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

    IDU_RX_FIFO_DMA_THRESHOLD_TypeDef idu_reg_0x48 = {.d32 = IDU->RX_FIFO_DMA_THRESHOLD};
    idu_reg_0x48.b.rx_fifo_dma_threshold = IDU_struct_init->rx_fifo_dma_threshold;
    idu_reg_0x48.b.rx_dma_enable = IDU_struct_init->rx_fifo_dma_enable;
    IDU->RX_FIFO_DMA_THRESHOLD = idu_reg_0x48.d32;

    IDU_TX_FIFO_DMA_THRESHOLD_TypeDef idu_reg_0x4c = {.d32 = IDU->TX_FIFO_DMA_THRESHOLD};
    idu_reg_0x4c.b.tx_fifo_dma_threshold = IDU_struct_init->tx_fifo_dma_threshold;
    idu_reg_0x4c.b.tx_dma_enable = IDU_struct_init->tx_fifo_dma_enable;
    IDU->TX_FIFO_DMA_THRESHOLD = idu_reg_0x4c.d32;

    IDU_RX_FIFO_INT_THRESHOLD_TypeDef idu_reg_0x50 = {.d32 = IDU->RX_FIFO_INT_THRESHOLD};
    idu_reg_0x50.b.rx_fifo_int_threshold = IDU_struct_init->rx_fifo_int_threshold;
    IDU->RX_FIFO_INT_THRESHOLD = idu_reg_0x50.d32;

    IDU_TX_FIFO_INT_THRESHOLD_TypeDef idu_reg_0x54 = {.d32 = IDU->TX_FIFO_INT_THRESHOLD};
    idu_reg_0x54.b.tx_fifo_int_threshold = IDU_struct_init->tx_fifo_int_threshold;
    IDU->TX_FIFO_INT_THRESHOLD = idu_reg_0x54.d32;
}

static void IDU_TxFifoClear(void)
{
    IDU_CTL1_TypeDef idu_reg_0x04 = {.d32 = IDU->IDU_CTL1};
    idu_reg_0x04.b.tx_fifo_clear = 1;
    IDU->IDU_CTL1 = idu_reg_0x04.d32;
}

static void IDU_RxFifoClear(void)
{
    IDU_CTL1_TypeDef idu_reg_0x04 = {.d32 = IDU->IDU_CTL1};
    idu_reg_0x04.b.rx_fifo_clear = 1;
    IDU->IDU_CTL1 = idu_reg_0x04.d32;
}

static void IDU_Cmd(FunctionalState state)
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

static void IDU_Run(FunctionalState state)
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

/*============================================================================*
 *                           Public Functions
 *============================================================================*/

ITStatus IDU_GetINTStatus(IDU_INTERRUPT_TYPE Interrupt)
{
    /* Check the parameters */
    assert_param(IS_IDU_INT(Interrupt));
    uint32_t int_val = 0;
    IDU_INT_STATUS_TypeDef idu_reg_0x64 = {.d32 = IDU->INT_STATUS};
    switch (Interrupt)
    {
    case IDU_DECOMPRESS_FINISH_INT:
        int_val = idu_reg_0x64.b.idu_decompress_finish_int_status;
        break;
    case IDU_LINE_DECOMPRESS_FINISH_INT:
        int_val = idu_reg_0x64.b.line_decompress_finish_int_status;
        break;
    case IDU_DECOMPRESS_ERROR_INT:
        int_val = idu_reg_0x64.b.idu_decompress_error_int_status;
        break;
    case IDU_RX_FIFO_OVERFLOW_INT:
        int_val = idu_reg_0x64.b.rx_fifo_overflow_int_status;
        break;
    case IDU_RX_FIFO_THRESHOLD_INT:
        int_val = idu_reg_0x64.b.rx_fifo_threshold_int_status;
        break;
    case IDU_TX_FIFO_UNDERFLOW_INT:
        int_val = idu_reg_0x64.b.tx_fifo_underflow_int_status;
        break;
    case IDU_TX_FIFO_THRESHOLD_INT:
        int_val = idu_reg_0x64.b.tx_fifo_threshold_int_status;
        break;
    }
    if (int_val)
    {
        return SET;
    }
    else
    {
        return RESET;
    }
}

ITStatus IDU_GetRawINTStatus(IDU_INTERRUPT_TYPE Interrupt)
{
    /* Check the parameters */
    assert_param(IS_IDU_INT(Interrupt));
    uint32_t int_val = 0;
    IDU_INT_RAW_STATUS_TypeDef idu_reg_0x60 = {.d32 = IDU->INT_RAW_STATUS};
    switch (Interrupt)
    {
    case IDU_DECOMPRESS_FINISH_INT:
        int_val = idu_reg_0x60.b.idu_decompress_finish_int_raw_status;
        break;
    case IDU_LINE_DECOMPRESS_FINISH_INT:
        int_val = idu_reg_0x60.b.line_decompress_finish_int_raw_status;
        break;
    case IDU_DECOMPRESS_ERROR_INT:
        int_val = idu_reg_0x60.b.idu_decompress_error_int_raw_status;
        break;
    case IDU_RX_FIFO_OVERFLOW_INT:
        int_val = idu_reg_0x60.b.rx_fifo_overflow_int_raw_status;
        break;
    case IDU_RX_FIFO_THRESHOLD_INT:
        int_val = idu_reg_0x60.b.rx_fifo_threshold_int_raw_status;
        break;
    case IDU_TX_FIFO_UNDERFLOW_INT:
        int_val = idu_reg_0x60.b.tx_fifo_underflow_int_raw_status;
        break;
    case IDU_TX_FIFO_THRESHOLD_INT:
        int_val = idu_reg_0x60.b.tx_fifo_threshold_int_raw_status;
        break;
    }
    if (int_val)
    {
        return SET;
    }
    else
    {
        return RESET;
    }
}


void IDU_INTConfig(IDU_INTERRUPT_TYPE Interrupt, FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_IDU_INT(IDU_INT));
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    IDU_INT_ENABLE_TypeDef idu_reg_0x58 = {.d32 = IDU->INT_ENABLE};
    uint32_t new_int_status = 0;
    if (NewState == ENABLE)
    {
        new_int_status = 1;
    }
    else
    {
        new_int_status = 0;
    }
    switch (Interrupt)
    {
    case IDU_DECOMPRESS_FINISH_INT:
        idu_reg_0x58.b.idu_decompress_finish_int_en = new_int_status;
        break;
    case IDU_LINE_DECOMPRESS_FINISH_INT:
        idu_reg_0x58.b.line_decompress_finish_int_en = new_int_status;
        break;
    case IDU_DECOMPRESS_ERROR_INT:
        idu_reg_0x58.b.idu_decompress_error_int_en = new_int_status;
        break;
    case IDU_RX_FIFO_OVERFLOW_INT:
        idu_reg_0x58.b.rx_fifo_overflow_int_en = new_int_status;
        break;
    case IDU_RX_FIFO_THRESHOLD_INT:
        idu_reg_0x58.b.rx_fifo_threshold_int_en = new_int_status;
        break;
    case IDU_TX_FIFO_UNDERFLOW_INT:
        idu_reg_0x58.b.tx_fifo_underflow_int_en = new_int_status;
        break;
    case IDU_TX_FIFO_THRESHOLD_INT:
        idu_reg_0x58.b.tx_fifo_threshold_int_en = new_int_status;
        break;
    }
    IDU->INT_ENABLE = idu_reg_0x58.d32;
}

void IDU_MaskINTConfig(IDU_INTERRUPT_TYPE Interrupt, FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_IDU_INT(IDU_INT_MSK));
    assert_param(IS_FUNCTIONAL_STATE(NewState));

    IDU_INT_MASK_TypeDef idu_reg_0x5c = {.d32 = IDU->INT_MASK};
    uint32_t new_int_status = 0;
    if (NewState == ENABLE)
    {
        new_int_status = 1;
    }
    else
    {
        new_int_status = 0;
    }
    switch (Interrupt)
    {
    case IDU_DECOMPRESS_FINISH_INT:
        idu_reg_0x5c.b.idu_decompress_finish_int_msk = new_int_status;
        break;
    case IDU_LINE_DECOMPRESS_FINISH_INT:
        idu_reg_0x5c.b.line_decompress_finish_int_msk = new_int_status;
        break;
    case IDU_DECOMPRESS_ERROR_INT:
        idu_reg_0x5c.b.idu_decompress_error_int_msk = new_int_status;
        break;
    case IDU_RX_FIFO_OVERFLOW_INT:
        idu_reg_0x5c.b.rx_fifo_overflow_int_msk = new_int_status;
        break;
    case IDU_RX_FIFO_THRESHOLD_INT:
        idu_reg_0x5c.b.rx_fifo_threshold_int_msk = new_int_status;
        break;
    case IDU_TX_FIFO_UNDERFLOW_INT:
        idu_reg_0x5c.b.tx_fifo_underflow_int_msk = new_int_status;
        break;
    case IDU_TX_FIFO_THRESHOLD_INT:
        idu_reg_0x5c.b.tx_fifo_threshold_int_msk = new_int_status;
        break;
    }
    IDU->INT_MASK = idu_reg_0x5c.d32;
}

void IDU_ClearINTPendingBit(IDU_INTERRUPT_TYPE Interrupt)
{
    assert_param(IS_IDU_INT(Interrupt));
    IDU_INT_CLEAR_TypeDef idu_reg_0x68 = {.d32 = IDU->INT_CLEAR};
    switch (Interrupt)
    {
    case IDU_DECOMPRESS_FINISH_INT:
        idu_reg_0x68.b.idu_decompress_finish_int_clr = 1;
        break;
    case IDU_LINE_DECOMPRESS_FINISH_INT:
        idu_reg_0x68.b.line_decompress_finish_int_clr = 1;
        break;
    case IDU_DECOMPRESS_ERROR_INT:
        idu_reg_0x68.b.idu_decompress_error_int_clr = 1;
        break;
    case IDU_RX_FIFO_OVERFLOW_INT:
        idu_reg_0x68.b.rx_fifo_overflow_int_clr = 1;
        break;
    case IDU_RX_FIFO_THRESHOLD_INT:
        idu_reg_0x68.b.rx_fifo_threshold_int_clr = 1;
        break;
    case IDU_TX_FIFO_UNDERFLOW_INT:
        idu_reg_0x68.b.tx_fifo_underflow_int_clr = 1;
        break;
    case IDU_TX_FIFO_THRESHOLD_INT:
        idu_reg_0x68.b.tx_fifo_threshold_int_clr = 1;
        break;
    }
    IDU->INT_CLEAR = idu_reg_0x68.d32;
}


uint32_t IDU_Get_Line_Start_Address(uint32_t compressed_start_address,
                                    uint32_t line_number)
{
    IDU_file_header *rle_file_header = ((IDU_file_header *)(compressed_start_address));
    if (line_number > rle_file_header->raw_pic_height)
    {
        return 0;
    }
    else
    {
        uint32_t *line_address = (uint32_t *)(compressed_start_address + 12 + line_number * 4);
        return (line_address[0] + compressed_start_address);
    }
}

IDU_ERROR IDU_Decode(uint8_t *file, IDU_decode_range *range, IDU_DMA_config *dma_cfg)
{
    IDU_ERROR err = IDU_SUCCESS;
    uint32_t decompress_start_line;
    uint32_t decompress_end_line;
    uint32_t decompress_start_column;
    uint32_t decompress_end_column;
    uint32_t decompress_target_stride;
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
        decompress_target_stride = range->target_stride;
    }
    else
    {
        return IDU_ERROR_INVALID_PARAM;
    }
    if ((!IS_IDU_ALGORITHM(header->algorithm_type.algorithm)) ||
        (!IS_IDU_PIXEL_BYTES(header->algorithm_type.pixel_bytes)))
    {
        return IDU_ERROR_FILE_INVALID;
    }
    uint8_t pixel_size = rtl_idu_get_actual_pixel_length((IDU_PIXEL_SIZE_INT)
                                                         header->algorithm_type.pixel_bytes);
    if (pixel_size == 0xFF)
    {
        return IDU_ERROR_FILE_INVALID;
    }
    uint32_t decompress_line_size = (decompress_end_column - decompress_start_column + 1) * pixel_size;
    if (decompress_line_size % 4 && decompress_line_size != decompress_target_stride)
    {
        return IDU_ERROR_LINE_NOT_ALIGNED;
    }
    uint32_t decompress_height = (decompress_end_line - decompress_start_line + 1);
    uint32_t start_line_address = IDU_Get_Line_Start_Address(compressed_data_start_address,
                                                             decompress_start_line);
    uint32_t compressed_data_size = IDU_Get_Line_Start_Address(compressed_data_start_address,
                                                               decompress_end_line + 1) - start_line_address;
    uint32_t decompressed_data_size = decompress_height * decompress_line_size;
    uint8_t rx_dma_depth = rtl_idu_get_dma_depth(dma_cfg->RX_DMA_channel_num);
    uint8_t tx_dma_depth = rtl_idu_get_dma_depth(dma_cfg->TX_DMA_channel_num);
    if (tx_dma_depth == 0xFF || rx_dma_depth == 0xFF)
    {
        return IDU_ERROR_INVALID_PARAM;
    }
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
    IDU_struct_init.rx_fifo_dma_threshold     = 8;
    IDU_struct_init.tx_fifo_dma_threshold     = 8;
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
    uint32_t s = rtl_idu_lock_int();
    GDMA_LLIDef *RX_GDMA_LLIStruct = NULL;
    rtl_idu_int_rx_dma_setting(start_line_address, compressed_data_size, dma_cfg->RX_DMA_channel_num,
                               &RX_GDMA_LLIStruct);

    GDMA_LLIDef *TX_GDMA_LLIStruct = NULL;
    rtl_idu_int_tx_dma_setting(dma_cfg->TX_DMA_channel_num, tx_dma_depth,
                               (uint32_t)dma_cfg->output_buf, DMA_decompressed_data_size_word,
                               decompress_target_stride, decompress_line_size,
                               decompress_height, &TX_GDMA_LLIStruct);
    IDU_ClearINTPendingBit(IDU_DECOMPRESS_FINISH_INT);
    IDU_INTConfig(IDU_DECOMPRESS_FINISH_INT, ENABLE);
    IDU_MaskINTConfig(IDU_DECOMPRESS_FINISH_INT, DISABLE);

    IDU_ClearINTPendingBit(IDU_DECOMPRESS_ERROR_INT);
    IDU_INTConfig(IDU_DECOMPRESS_ERROR_INT, ENABLE);
    IDU_MaskINTConfig(IDU_DECOMPRESS_ERROR_INT, DISABLE);

    IDU_Cmd(ENABLE);
    IDU_Run(ENABLE);
    rtl_idu_unlock_int(s);
    bool timed_out;
    IDU_BUSY_WAIT(IDU->IDU_CTL0 & BIT0, timed_out);
    if (timed_out)
    {
        err = IDU_ERROR_TIMEOUT;
        goto cleanup;
    }
    if (IDU_GetINTStatus(IDU_DECOMPRESS_ERROR_INT))
    {
        err =  IDU_ERROR_DECODE_FAIL;
        goto cleanup;
    }
    IDU_BUSY_WAIT(!rtl_idu_int_is_rx_dma_done(), timed_out);
    if (timed_out)
    {
        err = IDU_ERROR_TIMEOUT;
        goto cleanup;
    }
    IDU_BUSY_WAIT(!rtl_idu_int_is_tx_dma_done(), timed_out);
    if (timed_out)
    {
        err = IDU_ERROR_TIMEOUT;
        goto cleanup;
    }
    IDU_RxFifoClear();
    IDU_Cmd(DISABLE);
    IDU_BUSY_WAIT(!(IDU->IDU_CTL1 & BIT29), timed_out);
    if (timed_out)
    {
        err = IDU_ERROR_TIMEOUT;
        goto cleanup;
    }
    IDU_TxFifoClear();

cleanup:
    if (err != IDU_SUCCESS)
    {
        /* Decode failed or the engine/DMA never signalled completion. Force-stop
         * IDU and abort both GDMA channels (GDMA_Cmd performs an internal safe
         * suspend) so we leave no transfer in flight and do not wedge the next
         * operation. */
        IDU_Cmd(DISABLE);
        GDMA_Cmd(rtl_idu_int_get_rx_dma_channel(), DISABLE);
        GDMA_Cmd(rtl_idu_int_get_tx_dma_channel(), DISABLE);
    }
    if (TX_GDMA_LLIStruct)
    {
        rtl_idu_free_int(TX_GDMA_LLIStruct);
    }
    if (RX_GDMA_LLIStruct)
    {
        rtl_idu_free_int(RX_GDMA_LLIStruct);
    }

    return err;
}

IDU_ERROR IDU_Decode_With_Interrupt(uint8_t *file, IDU_decode_range *range, IDU_DMA_config *dma_cfg,
                                    IDU_interrupt_config int_cfg, IDU_State_Record *record)
{
    IDU_ERROR err = IDU_SUCCESS;
    uint32_t decompress_start_line;
    uint32_t decompress_end_line;
    uint32_t decompress_start_column;
    uint32_t decompress_end_column;
    uint32_t decompress_target_stride;
    RCC_PeriphClockCmd(APBPeriph_IDU, APBPeriph_IDU_CLOCK, ENABLE);
    if (record == NULL)
    {
        return IDU_ERROR_NULL_INT_REPORT;
    }
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
        decompress_target_stride = range->target_stride;
    }
    else
    {
        return IDU_ERROR_INVALID_PARAM;
    }
    if ((!IS_IDU_ALGORITHM(header->algorithm_type.algorithm)) ||
        (!IS_IDU_PIXEL_BYTES(header->algorithm_type.pixel_bytes)))
    {
        return IDU_ERROR_FILE_INVALID;
    }
    uint8_t pixel_size = rtl_idu_get_actual_pixel_length((IDU_PIXEL_SIZE_INT)
                                                         header->algorithm_type.pixel_bytes);
    if (pixel_size == 0xFF)
    {
        return IDU_ERROR_FILE_INVALID;
    }
    uint32_t decompress_line_size = (decompress_end_column - decompress_start_column + 1) * pixel_size;
    if (decompress_line_size % 4 && decompress_line_size != decompress_target_stride)
    {
        return IDU_ERROR_LINE_NOT_ALIGNED;
    }
    uint32_t decompress_height = (decompress_end_line - decompress_start_line + 1);
    uint32_t start_line_address = IDU_Get_Line_Start_Address(compressed_data_start_address,
                                                             decompress_start_line);
    uint32_t compressed_data_size = IDU_Get_Line_Start_Address(compressed_data_start_address,
                                                               decompress_end_line + 1) - start_line_address;
    uint32_t decompressed_data_size = decompress_height * decompress_line_size;
    uint8_t rx_dma_depth = rtl_idu_get_dma_depth(dma_cfg->RX_DMA_channel_num);
    uint8_t tx_dma_depth = rtl_idu_get_dma_depth(dma_cfg->TX_DMA_channel_num);
    if (tx_dma_depth == 0xFF || rx_dma_depth == 0xFF)
    {
        return IDU_ERROR_INVALID_PARAM;
    }
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
    if (rx_dma_depth == 4)
    {
        IDU_struct_init.rx_fifo_dma_threshold     = 12;
    }
    else
    {
        IDU_struct_init.rx_fifo_dma_threshold     = 8;
    }
    IDU_struct_init.tx_fifo_dma_threshold = 8;
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
    uint32_t s = rtl_idu_lock_int();
    GDMA_LLIDef *RX_GDMA_LLIStruct = NULL;
    rtl_idu_int_rx_dma_setting(start_line_address, compressed_data_size, dma_cfg->RX_DMA_channel_num,
                               &RX_GDMA_LLIStruct);

    GDMA_LLIDef *TX_GDMA_LLIStruct = NULL;
    rtl_idu_int_tx_dma_setting(dma_cfg->TX_DMA_channel_num, tx_dma_depth,
                               (uint32_t)dma_cfg->output_buf, DMA_decompressed_data_size_word,
                               decompress_target_stride, decompress_line_size,
                               decompress_height, &TX_GDMA_LLIStruct);
    if (int_cfg.idu_decompress_finish_int)
    {
        IDU_ClearINTPendingBit(IDU_DECOMPRESS_FINISH_INT);
        IDU_INTConfig(IDU_DECOMPRESS_FINISH_INT, ENABLE);
        IDU_MaskINTConfig(IDU_DECOMPRESS_FINISH_INT, DISABLE);
    }
    else
    {
        IDU_ClearINTPendingBit(IDU_DECOMPRESS_FINISH_INT);
        IDU_INTConfig(IDU_DECOMPRESS_FINISH_INT, DISABLE);
        IDU_MaskINTConfig(IDU_DECOMPRESS_FINISH_INT, ENABLE);
    }
    if (int_cfg.idu_decompress_error_int)
    {
        IDU_ClearINTPendingBit(IDU_DECOMPRESS_ERROR_INT);
        IDU_INTConfig(IDU_DECOMPRESS_ERROR_INT, ENABLE);
        IDU_MaskINTConfig(IDU_DECOMPRESS_ERROR_INT, DISABLE);
    }
    else
    {
        IDU_ClearINTPendingBit(IDU_DECOMPRESS_ERROR_INT);
        IDU_INTConfig(IDU_DECOMPRESS_ERROR_INT, DISABLE);
        IDU_MaskINTConfig(IDU_DECOMPRESS_ERROR_INT, ENABLE);
    }
    if (int_cfg.line_decompress_finish_int)
    {
        IDU_ClearINTPendingBit(IDU_LINE_DECOMPRESS_FINISH_INT);
        IDU_INTConfig(IDU_LINE_DECOMPRESS_FINISH_INT, ENABLE);
        IDU_MaskINTConfig(IDU_LINE_DECOMPRESS_FINISH_INT, DISABLE);
    }
    if (int_cfg.rx_fifo_overflow_int)
    {
        IDU_ClearINTPendingBit(IDU_RX_FIFO_OVERFLOW_INT);
        IDU_INTConfig(IDU_RX_FIFO_OVERFLOW_INT, ENABLE);
        IDU_MaskINTConfig(IDU_RX_FIFO_OVERFLOW_INT, DISABLE);
    }
    if (int_cfg.rx_fifo_threshold_int)
    {
        IDU_ClearINTPendingBit(IDU_RX_FIFO_THRESHOLD_INT);
        IDU_INTConfig(IDU_RX_FIFO_THRESHOLD_INT, ENABLE);
        IDU_MaskINTConfig(IDU_RX_FIFO_THRESHOLD_INT, DISABLE);
    }
    if (int_cfg.tx_fifo_threshold_int)
    {
        IDU_ClearINTPendingBit(IDU_TX_FIFO_THRESHOLD_INT);
        IDU_INTConfig(IDU_TX_FIFO_THRESHOLD_INT, ENABLE);
        IDU_MaskINTConfig(IDU_TX_FIFO_THRESHOLD_INT, DISABLE);
    }
    if (int_cfg.tx_fifo_underflow_int)
    {
        IDU_ClearINTPendingBit(IDU_TX_FIFO_UNDERFLOW_INT);
        IDU_INTConfig(IDU_TX_FIFO_UNDERFLOW_INT, ENABLE);
        IDU_MaskINTConfig(IDU_TX_FIFO_UNDERFLOW_INT, DISABLE);
    }

    record->rx_lli = RX_GDMA_LLIStruct;
    record->tx_lli = TX_GDMA_LLIStruct;
    record->rx_dma_num = rtl_idu_int_get_rx_dma_channel();
    record->tx_dma_num = rtl_idu_int_get_tx_dma_channel();
    IDU_Cmd(ENABLE);
    IDU_Run(ENABLE);
    rtl_idu_unlock_int(s);
    return IDU_SUCCESS;
}

void IDU_Clean_State(IDU_State_Record *record)
{
    if (record == NULL)
    {
        return;
    }
    bool rx_timed_out, tx_timed_out, fifo_timed_out;
    IDU_BUSY_WAIT(!rtl_idu_int_is_rx_dma_done(), rx_timed_out);
    IDU_BUSY_WAIT(!rtl_idu_int_is_tx_dma_done(), tx_timed_out);
    if (rx_timed_out || tx_timed_out)
    {
        /* This is the recovery path: the DMA never signalled completion, so we
         * must abort the channels (safe suspend) instead of waiting forever -
         * otherwise the very condition that triggered cleanup would hang here. */
        GDMA_Cmd(record->rx_dma_num, DISABLE);
        GDMA_Cmd(record->tx_dma_num, DISABLE);
    }
    IDU_RxFifoClear();
    IDU_Cmd(DISABLE);
    IDU_BUSY_WAIT(!(IDU->IDU_CTL1 & BIT29), fifo_timed_out);
    (void)fifo_timed_out;
    IDU_TxFifoClear();
    /* LLIs are freed unconditionally so a wedged engine can never leak them. */
    if (record->rx_lli)
    {
        rtl_idu_free_int(record->rx_lli);
    }
    if (record->tx_lli)
    {
        rtl_idu_free_int(record->tx_lli);
    }
}

IDU_ERROR IDU_Decode_Direct(uint8_t *file, IDU_decode_range *range, IDU_DMA_config *dma_cfg)
{
    GDMA_LLIDef *RX_GDMA_LLIStruct = NULL;
    return rtl_idu_decode_direct_int(file, range, dma_cfg, RX_GDMA_LLIStruct);
}

uint8_t IDU_Get_Pixel_Size(IDU_file_header *file)
{
    return rtl_idu_get_actual_pixel_length((IDU_PIXEL_SIZE_INT)file->algorithm_type.pixel_bytes);
}
