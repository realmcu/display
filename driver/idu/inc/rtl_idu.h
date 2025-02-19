/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* \file     rtl_idu.h
* \brief    This file provides all the LCDC HANDLER firmware functions.
* \details
* \author   astor zhang
* \date     2023-10-17
* \version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_IDU_H
#define RTL_IDU_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/

#include "rtl_idu_def.h"

/** \defgroup IDU       IDU
  * \brief
  * \{
  */
/*============================================================================*
 *                         IDU Declaration
 *============================================================================*/
/** \defgroup IDU_Exported_Constants IDU Exported Constants
  * \brief
  * \{
  */

/** \defgroup IDU_Declaration IDU Declaration
  * \{
  * \ingroup  IDU_Exported_Constants
  */

#define IDU                           ((IDU_TypeDef *)IDU_REG_BASE)

/** End of IDU_Declaration
  * \}
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/
typedef enum
{
    IDU_RLE = 0x00,
    IDU_FASTLZ = 0x01,
    IDU_YUV_SAMPLE_BLUR_FASTLZ = 0x02,
    IDU_YUV_SAMPLE_BLUR = 0x03,
} IDU_ALGORITHM;

typedef enum
{
    YUV444_SAMPLE = 0x00,
    YUV422_SAMPLE = 0x01,
    YUV411_SAMPLE = 0x02,
} IDU_YUV_SAMPLE_TYPE;

typedef enum
{
    YUV_BLUR_0BIT = 0x00,
    YUV_BLUR_1BIT = 0x01,
    YUV_BLUR_2BIT = 0x02,
    YUV_BLUR_4BIT = 0x03,
} IDU_YUV_BLUR_BIT;

typedef enum
{
#ifdef TARGET_RTL87X3EU
    PIXEL_SIZE_8BIT = 0x00,
    PIXEL_SIZE_16BIT = 0x01,
    PIXEL_SIZE_24BIT = 0x02,
    PIXEL_SIZE_32BIT = 0x03,
#else
    PIXEL_SIZE_16BIT = 0x00,
    PIXEL_SIZE_24BIT = 0x01,
    PIXEL_SIZE_32BIT = 0x02,
#endif
} IDU_PIXEL_SIZE;

typedef enum
{
    THROW_AWAY_0BYTE = 0x00,
    THROW_AWAY_1BYTE = 0x01,
    THROW_AWAY_2BYTE = 0x02,
    THROW_AWAY_3BYTE = 0x03,
} IDU_THROW_AWAY_SIZE;

typedef enum
{
    RUN_LENGTH_SIZE_0BYTE = 0x00,
    RUN_LENGTH_SIZE_1BYTE = 0x01,
    RUN_LENGTH_SIZE_2BYTE = 0x02,
} IDU_RLE_RUNLENGTH_SIZE;

typedef enum
{
    IDU_SUCCESS = 0x0,
    IDU_ERROR_NULL_INPUT,
    IDU_ERROR_DECODE_FAIL,
    IDU_ERROR_START_EXCEED_BOUNDARY,
    IDU_ERROR_END_EXCEED_BOUNDARY,
    IDU_ERROR_START_LARGER_THAN_END,
    IDU_ERROR_INVALID_PARAM,
} IDU_ERROR;

typedef enum
{
    IDU_HW_HANDSHAKE_DMA = 0x0,
    IDU_HW_HANDSHAKE_PPE = 0x1,
} IDU_HW_HANDSHAKE;

typedef struct
{
    struct
    {
        uint8_t algorithm: 2;
        uint8_t feature_1: 2;
        uint8_t feature_2: 2;
        uint8_t pixel_bytes: 2;
    } algorithm_type;
    uint8_t reserved[3];
    uint32_t raw_pic_width;
    uint32_t raw_pic_height;
} IDU_file_header;

typedef struct
{
    uint32_t start_line;
    uint32_t end_line;
    uint32_t start_column;
    uint32_t end_column;
} IDU_decode_range;

typedef struct
{
    uint32_t RX_DMA_channel_num;
    uint32_t RX_FIFO_INT_threshold;
    uint32_t TX_DMA_channel_num;
    uint32_t TX_FIFO_INT_threshold;
    uint32_t *output_buf;
} IDU_DMA_config;

typedef struct
{
    IDU_ALGORITHM algorithm_type;
    IDU_THROW_AWAY_SIZE head_throw_away_byte_num;
    IDU_PIXEL_SIZE pic_pixel_size;
    IDU_RLE_RUNLENGTH_SIZE pic_length2_size;
    IDU_RLE_RUNLENGTH_SIZE pic_length1_size;
    IDU_YUV_BLUR_BIT yuv_blur_bit;
    IDU_YUV_SAMPLE_TYPE yuv_sample_type;
    IDU_HW_HANDSHAKE hw_handshake;
    uint32_t pic_decompress_height;
    uint32_t pic_raw_width;
    uint32_t tx_column_start;
    uint32_t tx_column_end;
    uint32_t compressed_data_size;
    uint32_t rx_fifo_dma_enable;
    uint32_t tx_fifo_dma_enable;
    uint8_t rx_fifo_dma_threshold;
    uint8_t tx_fifo_dma_threshold;
    uint8_t tx_fifo_int_threshold;
    uint8_t rx_fifo_int_threshold;
} IDU_InitTypeDef;

typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t idu_decompress_error_int: 1;
        __IO uint32_t tx_fifo_underflow_int: 1;
        __IO uint32_t tx_fifo_threshold_int: 1;
        __IO uint32_t rx_fifo_overflow_int: 1;
        __IO uint32_t rx_fifo_threshold_int: 1;
        __IO uint32_t line_decompress_finish_int: 1;
        __IO uint32_t idu_decompress_finish_int: 1;
        __I uint32_t reserved_0: 25;
    } b;
} IDU_INT_CFG_t;

/**
 * \defgroup    IDU_FIFO_Width IDU FIFO Width
 * \{
 * \ingroup     IDU_Exported_Constants
 */
#define IDU_TX_FIFO_WIDTH             32
#define IDU_RX_FIFO_WIDTH             32

/** End of IDU_FIFO_Width
  * \}
  */

/**
 * \defgroup    IDU_FIFO_Depth IDU FIFO Depth
 * \{
 * \ingroup     IDU_Exported_Constants
 */
#define IDU_TX_FIFO_DEPTH             16
#define IDU_RX_FIFO_DEPTH             16

/** End of IDU_FIFO_Depth
  * \}
  */

/**
 * \defgroup    IDU_Interrupts_Definition IDU Interrupts Definition
 * \{
 * \ingroup     IDU_Exported_Constants
 */
#define IDU_DECOMPRESS_FINISH_INT                                 ((uint32_t)BIT(6))
#define IDU_LINE_DECOMPRESS_FINISH_INT                            ((uint32_t)BIT(5))
#define IDU_RX_FIFO_THRESHOLD_INT                                 ((uint32_t)BIT(4))
#define IDU_RX_FIFO_OVERFLOW_INT                                  ((uint32_t)BIT(3))
#define IDU_TX_FIFO_THRESHOLD_INT                                 ((uint32_t)BIT(2))
#define IDU_TX_FIFO_UNDERFLOW_INT                                  ((uint32_t)BIT(1))
#define IDU_DECOMPRESS_ERROR_INT                                  ((uint32_t)BIT(0))

#define IS_IDU_INT(INT)              (((INT) == IDU_DECOMPRESS_FINISH_INT)         || \
                                      ((INT) == IDU_LINE_DECOMPRESS_FINISH_INT)  || \
                                      ((INT) == IDU_RX_FIFO_THRESHOLD_INT)         || \
                                      ((INT) == IDU_RX_FIFO_OVERFLOW_INT)  || \
                                      ((INT) == IDU_TX_FIFO_THRESHOLD_INT)      || \
                                      ((INT) == IDU_TX_FIFO_UNDERFLOW_INT)         || \
                                      ((INT) == IDU_DECOMPRESS_ERROR_INT)

/** End of IDU_Interrupts_Definition
  * \}
  */

/**
 * \defgroup    IDU_ALGO IDU ALGO
 * \{
 * \ingroup     IDU_Exported_Constants
 */
#define IDU_ALGO_RLE                      (0)
#define IDU_ALGO_FASTLZ                   (BIT(0))
#define IDU_ALGO_YUV_BLUR_FASTLZ          (BIT(1))
#define IDU_ALGO_YUV_BLUR                 (BIT(1)|BIT(0))
#define IS_IDU_ALGORITHM(ALGO)             (((ALGO) == IDU_ALGO_RLE) || ((ALGO) == IDU_ALGO_FASTLZ)      ||\
                                            ((ALGO) == IDU_ALGO_YUV_BLUR_FASTLZ) || ((ALGO) == IDU_ALGO_YUV_BLUR))

/** End of IDU_ALGO
  * \}
  */

/**
 * \defgroup    IDU_PIXEL_SIZE IDU PIXEL SIZE
 * \{
 * \ingroup     IDU_Exported_Constants
 */
#ifdef TARGET_RTL87X3EU
#define IDU_PIXEL_8BIT                    (0)
#define IDU_PIXEL_16BIT                   (1)
#define IDU_PIXEL_24BIT                   (2)
#define IDU_PIXEL_32BIT                   (3)
#define IS_IDU_PIXEL_BYTES(NUM)           (((NUM) == IDU_PIXEL_8BIT) || ((NUM) == IDU_PIXEL_16BIT) || ((NUM) == IDU_PIXEL_24BIT) || ((NUM) == IDU_PIXEL_32BIT))
#else
#define IDU_PIXEL_16BIT                   (0)
#define IDU_PIXEL_24BIT                   (BIT(0))
#define IDU_PIXEL_32BIT                   (BIT(1))
#define IS_IDU_PIXEL_BYTES(NUM)           (((NUM) == IDU_PIXEL_16BIT) || ((NUM) == IDU_PIXEL_24BIT) || ((NUM) == IDU_PIXEL_32BIT))
#endif

/** End of IDU_PIXEL_SIZE
  * \}
  */

/**
 * \defgroup    IDU_RLE_SIZE_LEN IDU RLE SIZE LEN
 * \{
 * \ingroup     IDU_Exported_Constants
 */
#define IDU_RLE_1BYTE_LEN                 (BIT(0))
#define IDU_RLE_2BYTE_LEN                 (BIT(1))
#define IS_IDU_RLE_BYTE_LEN(LEN)          (((LEN) == IDU_RLE_1BYTE_LEN) || ((LEN) == IDU_RLE_2BYTE_LEN))

/** End of IDU_RLE_SIZE_LEN
  * \}
  */

/**
 * \defgroup    IDU_YUV_BLUR_SIZE IDU YUV BLUR SIZE
 * \{
 * \ingroup     IDU_Exported_Constants
 */
#define IDU_YUV_BLUR_0BIT                 (0)
#define IDU_YUV_BLUR_1BIT                 (BIT(0))
#define IDU_YUV_BLUR_2BIT                 (BIT(1))
#define IDU_YUV_BLUR_4BIT                 (BIT(0)|BIT(1))

#define IS_IDU_YUB_BLUR_BIT(NUM)          (((NUM) == IDU_YUV_BLUR_0BIT) || ((NUM) == IDU_YUV_BLUR_1BIT)      ||\
                                           ((NUM) == IDU_YUV_BLUR_2BIT) || ((NUM) == IDU_YUV_BLUR_4BIT))

/** End of IDU_YUV_BLUR_SIZE
  * \}
  */

/**
 * \defgroup    IDU_YUV_TYPE IDU YUV TYPE
 * \{
 * \ingroup     IDU_Exported_Constants
 */
#define IDU_YUV_TYPE_444                  (0)
#define IDU_YUV_TYPE_422                  (BIT(0))
#define IDU_YUV_TYPE_411                  (BIT(1))

#define IS_IDU_YUV_TYPE(TYPE)             (((TYPE) == IDU_YUV_TYPE_444) || ((TYPE) == IDU_YUV_TYPE_422) || ((TYPE) == IDU_YUV_TYPE_411))

/** End of IDU_YUV_TYPE
  * \}
  */

/** End of IDU_Exported_Constants
  * \}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** \defgroup IDU_Exported_Functions IDU Exported Functions
  * \brief
  * \{
  */

/**
 * \brief  Clear TX FIFO
 */
void IDU_TxFifoClear(void);

/**
 * \brief  Clear RX FIFO
 */
void IDU_RxFifoClear(void);

/**
 * \brief  Clear internal memory
 */
void IDU_AllMemClear(void);

/**
 * \brief  Enable/Disable IDU interrupt
 * \param[in] IDU_INT_EN         interrupt to be operated.
 * \param[in] state               enable/disable interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        IDU_INTConfig(IDU_DECOMPRESS_FINISH_INT, ENABLE);
    }
 * \endcode
 */
void IDU_INTConfig(uint32_t IDU_INT_EN, FunctionalState state);

/**
 * \brief  Get interrupt status of IDU
 * \param[in] IDU_INT         interrupts to be obtained.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        IDU_INT_CFG_t internal_int_cfg = {.d32 = 0};
        internal_int_cfg.b.idu_decompress_finish_int = 1;
        internal_int_cfg.b.idu_decompress_error_int = 1;
        IDU_INT_CFG_t ret_int = IDU_GetINTStatus(internal_int_cfg);
        if(rev.b.idu_decompress_finish_int)
        {
            ...
        }
    }
 * \endcode
 */
ITStatus IDU_GetINTStatus(uint32_t IDU_INT);

/**
 * \brief  Mask/Unmask IDU interrupt
 * \param[in] IDU_INT_MSK         interrupts to be masked.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        IDU_INT_CFG_t internal_int_cfg = {.d32 = 0};
        internal_int_cfg.b.idu_decompress_finish_int = 1;
        internal_int_cfg.b.idu_decompress_error_int = 1;
        IDU_MaskINTConfig(internal_int_cfg);
    }
 * \endcode
 */
void IDU_MaskINTConfig(uint32_t IDU_INT_MSK, FunctionalState state);

/**
 * \brief  Clear pending IDU interrupt
 * \param[in] IDU_INT         interrupts to be cleared.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        IDU_INT_CFG_t internal_int_cfg = {.d32 = 0};
        internal_int_cfg.b.idu_decompress_finish_int = 1;
        internal_int_cfg.b.idu_decompress_error_int = 1;
        IDU_MaskINTConfig(internal_int_cfg);
    }
 * \endcode
 */
void IDU_ClearINTPendingBit(uint32_t IDU_INT);

/**
 * \brief  Get IDU internal buffer state
 * \return buffer state
 * \retval 0        Not empty.
 * \retval 1        Empty.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ITStatus ret = IDU_GetInternalBufferState();
        if(ret)
        {
            ...
        }
    }
 * \endcode
 */
ITStatus IDU_GetInternalBufferState(void);

/**
 * \brief  Get vertical counter of decompress process
 * \return counter value starts from 0
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        uint32_t cnt = IDU_GetDecompressLine();
        DBG_DIRECT("%d lines has been decompressed", cnt + 1);
    }
 * \endcode
 */
uint32_t IDU_GetDecompressLine(void);

/**
 * \brief  Get horizontal counter of decompress process
 * \return counter value starts from 0
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        uint32_t cnt = IDU_GetHorizontalByteNum();
        DBG_DIRECT("%d pixels has been decompressed", cnt + 1);
    }
 * \endcode
 */
uint32_t IDU_GetHorizontalByteNum(void);

/**
 * \brief  the size of compressed data which DMA will transmit from FLASH, measured in byte
 * \return data size
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        uint32_t data_size = IDU_GetCompressedSize();
        DBG_DIRECT("%d bytes will be decompressed", data_size);
    }
 * \endcode
 */
uint32_t IDU_GetCompressedSize(void);

/**
 * \brief  IDU decode function, decode file from memory/flash
 * \param[in] file          address of source.
 * \param[in] range         decode range.
 * \param[in] dma_cfg       determine which DMA channel is selected.
 * \return operation result
 * \retval IDU_SUCCESS    Operation success.
 * \retval others           Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        RCC_PeriphClockCmd(APBPeriph_IDU, APBPeriph_IDU_CLOCK, ENABLE);
        IDU_file_header* header = ( IDU_file_header*)file_data;
        DBG_DIRECT("algo = %d, blur %d, YUV %d", header->algorithm_type.algorithm, header->algorithm_type.feature_2, header->algorithm_type.feature_1);
        IDU_decode_range range = {0, header->raw_pic_height - 1, 0, header->raw_pic_width - 1};
        IDU_DMA_config dma_cfg;
        dma_cfg.output_buf = (uint32_t*)buf;
        dma_cfg.RX_DMA_channel_num = 0;
        dma_cfg.TX_DMA_channel_num = 1;
        IDU_Decode(file_data, range, dma_cfg);
    }
 * \endcode
 */
IDU_ERROR IDU_Decode(uint8_t *file, IDU_decode_range *range, IDU_DMA_config *dma_cfg);

/**
 * \brief  IDU decode function with interrupt, decode file from memory/flash
 * \param[in] file          address of source.
 * \param[in] range         decode range.
 * \param[in] dma_cfg       determine which DMA channel is selected.
 * \return operation result
 * \retval IDU_SUCCESS    Operation started.
 * \retval others           Operation aborted.
 *
 * <b>Example usage</b>
 * \code{.c}
    void IDU_Handler(void){
        if(IDU_GetINTStatus(IDU_DECOMPRESS_FINISH_INT_STATUS_MSK) == SET){
            IDU_ClearINTPendingBit(IDU_DECOMPRESS_FINISH_INT_STATUS_MSK);
            DBG_DIRECT("IDU_IRQHandler line = %d\n",__LINE__);
            for(int i = 0; i < sizeof(decode_file_data); i++){
                if(decode_file_data[i] != buf[i]){

                    DBG_DIRECT("Decode Error at raw[%d] %x || out[%d] %x ",i, decode_file_data[i], i ,buf[i]);
                }
            }
            DBG_DIRECT("Decompress start bit is : %d", IDU->IDU_CTL0 & BIT0);
        }
        if(IDU_GetINTStatus(IDU_DECOMPRESS_ERROR_INT_STATUS_MSK) == SET){
            IDU_ClearINTPendingBit(IDU_DECOMPRESS_ERROR_INT_STATUS_MSK);
            DBG_DIRECT("IDU_IRQHandler line = %d\n",__LINE__);
        }
    }
    void test_code(void){
        RCC_PeriphClockCmd(APBPeriph_IDU, APBPeriph_IDU_CLOCK, ENABLE);
        IDU_file_header* header = ( IDU_file_header*)file_data;
        DBG_DIRECT("algo = %d, blur %d, YUV %d", header->algorithm_type.algorithm, header->algorithm_type.feature_2, header->algorithm_type.feature_1);
        IDU_decode_range range = {0, header->raw_pic_height - 1, 0, header->raw_pic_width - 1};
        IDU_DMA_config dma_cfg;
        dma_cfg.output_buf = (uint32_t*)buf;
        dma_cfg.RX_DMA_channel_num = 0;
        dma_cfg.TX_DMA_channel_num = 1;
        IDU_INT_CFG_t int_cfg = {.d32 = 0};
        int_cfg.b.idu_decompress_finish_int = 1;
        int_cfg.b.idu_decompress_error_int = 1;
        IDU_Decode_Ex(file_data, range, dma_cfg, int_cfg);
    }
 * \endcode
 */
IDU_ERROR IDU_Decode_Ex(uint8_t *file, IDU_decode_range *range, IDU_DMA_config *dma_cfg,
                        IDU_INT_CFG_t int_cfg);

/**
 * \brief  Enable IDU
 * \param[in] state               enable or disable IDU peripheral.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        IDU_Cmd(ENABLE);
        IDU_Run(ENABLE);
    }
 * \endcode
 */
void IDU_Cmd(FunctionalState state);

/**
 * \brief  Start IDU to decode, must envoke after enable IDU
 * \param[in] state               enable or disable IDU decode.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        IDU_Cmd(ENABLE);
        IDU_Run(ENABLE);
    }
 * \endcode
 */
void IDU_Run(FunctionalState state);

/**
 * \brief  Initialize IDU with parameters filled in structure
 * \param[in] IDU_init_struct      pointer to initialize structure
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        IDU_InitTypeDef IDU_struct_init;
        IDU_struct_init.algorithm_type            = IDU_ALGO_RLE;
        IDU_struct_init.head_throw_away_byte_num  = THROW_AWAY_0BYTE;
        IDU_struct_init.pic_pixel_size            = PIXEL_SIZE_16BIT;
        IDU_struct_init.pic_decompress_height     = 100;
        IDU_struct_init.pic_raw_width             = 500;
        IDU_struct_init.tx_column_start           = 15;
        IDU_struct_init.tx_column_end             = 300 ;
        IDU_struct_init.compressed_data_size      = compressed_data_size;
        IDU_struct_init.pic_length2_size          = IDU_RLE_1BYTE_LEN;
        IDU_struct_init.pic_length1_size          = IDU_RLE_1BYTE_LEN;
        IDU_struct_init.yuv_blur_bit              = 0;
        IDU_struct_init.yuv_sample_type           = 0;
        IDU_struct_init.rx_fifo_dma_enable        = (uint32_t)ENABLE;
        IDU_struct_init.tx_fifo_dma_enable        = (uint32_t)ENABLE;
        IDU_struct_init.rx_fifo_dma_threshold     = IDU_RX_FIFO_DEPTH / 2;
        IDU_struct_init.tx_fifo_dma_threshold     = IDU_TX_FIFO_DEPTH / 2;
        IDU_struct_init.rx_fifo_int_threshold     = 8;
        IDU_struct_init.tx_fifo_int_threshold     = 8;
        IDU_struct_init.hw_handshake              = IDU_HW_HANDSHAKE_DMA;
        IDU_Init(&IDU_struct_init);
    }
 * \endcode
 */
void IDU_Init(IDU_InitTypeDef *IDU_init_struct);

/**
 * \brief  Decode compressed data and send directly to PPE
 * \param[in] file          address of source.
 * \param[in] range         decode range.
 * \param[in] dma_cfg       determine which DMA channel is selected.
 * \return operation result
 * \retval IDU_SUCCESS    Operation success.
 * \retval others           Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        RCC_PeriphClockCmd(APBPeriph_IDU, APBPeriph_IDU_CLOCK, ENABLE);
        IDU_file_header* header = ( IDU_file_header*)file_data;
        IDU_decode_range range = {0, header->raw_pic_height - 1, 0, header->raw_pic_width - 1};
        IDU_DMA_config dma_cfg;
        dma_cfg.output_buf = (uint32_t*)buf;
        dma_cfg.RX_DMA_channel_num = 0;
        IDU_Decode_Direct(file_data, range, dma_cfg);
        //Note: Continue with PPE operation
    }
 * \endcode
 */
IDU_ERROR IDU_Decode_Direct(uint8_t *file, IDU_decode_range *range,
                            IDU_DMA_config *dma_cfg);

/**
 * \brief  Get the start address of certain line in compressed file
 * \param[in] compressed_start_address          start address of entire compressed file.
 * \param[in] line_number                       specified line number
 * \return start address of specified line
 * \retval 0                Invalid line number.
 * \retval others           Valid address.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        uint32_t file_address = (uint32_t)SAMPLE_FILE;
        uint32_t line_addr = IDU_Get_Line_Start_Address(file_address, 100);
    }
 * \endcode
 */
uint32_t IDU_Get_Line_Start_Address(uint32_t compressed_start_address,
                                    uint32_t line_number);

/** End of IDU_Exported_Functions
  * \}
  */

/** End of IDU
  * \}
  */

#ifdef __cplusplus
}
#endif

#endif /* RTL_IDU_H */

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/
