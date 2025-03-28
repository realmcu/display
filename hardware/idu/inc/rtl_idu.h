/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* \file     rtl_idu.h
* \brief    This file provides all the IDU firmware functions.
* \details  This module implements the Image Decompressor Unit (IDU) driver,
*           supporting various decompression algorithms including RLE, FastLZ,
*           and YUV sampling with blur effects for efficient image processing.
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
  * \brief    Image Decompressor Unit (IDU) driver for hardware-accelerated image decompression
  * \{
  */
/*============================================================================*
 *                         IDU Declaration
 *============================================================================*/
/** \defgroup IDU_Exported_Constants IDU Exported Constants
  * \brief    Constants and enumerations used by the IDU module
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
/**
 * \defgroup    IDU_FIFO_Width IDU FIFO Width
 * \{
 * \ingroup     IDU_Exported_Constants
 */
#define IDU_TX_FIFO_WIDTH             32      /*!< Width of IDU TX FIFO in bits */
#define IDU_RX_FIFO_WIDTH             32      /*!< Width of IDU RX FIFO in bits */

/** End of IDU_FIFO_Width
  * \}
  */

/**
 * \defgroup    IDU_FIFO_Depth IDU FIFO Depth
 * \{
 * \ingroup     IDU_Exported_Constants
 */
#define IDU_TX_FIFO_DEPTH             16       /*!< Depth of IDU TX FIFO in words */
#define IDU_RX_FIFO_DEPTH             16       /*!< Depth of IDU RX FIFO in words */

/** End of IDU_FIFO_Depth
  * \}
  */

/** \defgroup IDU_ALGORITHM IDU Compress Algorithm
 * \{
 * \ingroup  IDU_Exported_Constants
 */
typedef enum
{
    IDU_RLE = 0x00,                         /*!< The compressed image uses Run Length Encode algorithm. */
    IDU_FASTLZ = 0x01,                      /*!< The compressed image uses FastLZ compression algorithm. */
    IDU_YUV_SAMPLE_BLUR_FASTLZ = 0x02,      /*!< The image is sampled in the YUV color space with a 0-3 bit blur, and then compressed using the FastLZ compression algorithm. */
    IDU_YUV_SAMPLE_BLUR = 0x03,             /*!< The image is sampled in the YUV color space with a 0-3 bit blur. */
} IDU_ALGORITHM;

#define IS_IDU_ALGORITHM(ALGO)       (((ALGO) == IDU_RLE) || \
                                      ((ALGO) == IDU_FASTLZ) || \
                                      ((ALGO) == IDU_YUV_SAMPLE_BLUR_FASTLZ) || \
                                      ((ALGO) == IDU_YUV_SAMPLE_BLUR))
/** End of IDU_ALGORITHM
  * \}
  */

/** \defgroup IDU_YUV_SAMPLE_TYPE IDU YUV Sample Type
  * \{
  * \ingroup  IDU_Exported_Constants
  */
typedef enum
{
    IDU_YUV444_SAMPLE = 0x00,       /*!< YUV444 sampling, data storing format: [Y0 U0 V0 Y1 U1 V1]. */
    IDU_YUV422_SAMPLE = 0x01,       /*!< YUV422 sampling, data storing format: [Y0 U0 Y1 V1]. */
    IDU_YUV411_SAMPLE = 0x02,       /*!< YUV411 sampling, data storing format: [Y0 U0 Y1 Y2 V2 Y3]. */
} IDU_YUV_SAMPLE_TYPE;

#define IS_IDU_YUV_TYPE(TYPE)        (((TYPE) == IDU_YUV444_SAMPLE) || \
                                      ((TYPE) == IDU_YUV422_SAMPLE) || \
                                      ((TYPE) == IDU_YUV411_SAMPLE))

/** End of IDU_YUV_SAMPLE_TYPE
  * \}
  */

/** \defgroup IDU_YUV_BLUR_BIT IDU Blur Bit
  * \{
  * \ingroup  IDU_Exported_Constants
  */
typedef enum
{
    IDU_YUV_BLUR_0BIT = 0x00,       /*!< The compressed image keeps all data after YUV sampling. */
    IDU_YUV_BLUR_1BIT = 0x01,       /*!< The compressed image throws away 1 least significant bit of each byte after YUV sampling. */
    IDU_YUV_BLUR_2BIT = 0x02,       /*!< The compressed image throws away 2 least significant bits of each byte after YUV sampling. */
    IDU_YUV_BLUR_4BIT = 0x03,       /*!< The compressed image throws away 4 least significant bits of each byte after YUV sampling. */
} IDU_YUV_BLUR_BIT;

#define IS_IDU_YUB_BLUR_BIT(NUM)     (((NUM) == IDU_YUV_BLUR_0BIT) || \
                                      ((NUM) == IDU_YUV_BLUR_1BIT) || \
                                      ((NUM) == IDU_YUV_BLUR_2BIT) || \
                                      ((NUM) == IDU_YUV_BLUR_4BIT))
/** End of IDU_YUV_BLUR_BIT
  * \}
  */

/** \defgroup IDU_THROW_AWAY_SIZE IDU Throw Away Byte
  * \{
  * \ingroup  IDU_Exported_Constants
  */
typedef enum
{
    IDU_THROW_AWAY_0BYTE = 0x00,    /*!< IDU keeps entire input data. */
    IDU_THROW_AWAY_1BYTE = 0x01,    /*!< IDU throws away 1 byte at the head of input data. */
    IDU_THROW_AWAY_2BYTE = 0x02,    /*!< IDU throws away 2 bytes at the head of input data. */
    IDU_THROW_AWAY_3BYTE = 0x03,    /*!< IDU throws away 3 bytes at the head of input data. */
} IDU_THROW_AWAY_SIZE;

#define IS_IDU_THROW_AWAY_SIZE(SIZE) (((SIZE) == IDU_THROW_AWAY_0BYTE) || \
                                      ((SIZE) == IDU_THROW_AWAY_1BYTE) || \
                                      ((SIZE) == IDU_THROW_AWAY_2BYTE) || \
                                      ((SIZE) == IDU_THROW_AWAY_3BYTE))
/** End of IDU_THROW_AWAY_SIZE
  * \}
  */

/** \defgroup IDU_RLE_RUNLENGTH_SIZE IDU RLE Run Length
  * \{
  * \ingroup  IDU_Exported_Constants
  */
typedef enum
{
    RUN_LENGTH_SIZE_0BYTE = 0x00,       /*!< The input data is not RLE compressed. */
    RUN_LENGTH_SIZE_1BYTE = 0x01,       /*!< Run length is 1 byte. */
    RUN_LENGTH_SIZE_2BYTE = 0x02,       /*!< Run length is 2 bytes. */
} IDU_RLE_RUNLENGTH_SIZE;

#define IS_IDU_RLE_BYTE_LEN(LEN)          (((LEN) == RUN_LENGTH_SIZE_1BYTE) || ((LEN) == RUN_LENGTH_SIZE_2BYTE))

/** End of IDU_RLE_RUNLENGTH_SIZE
  * \}
  */

/** \defgroup IDU_ERROR IDU Error Code
  * \{
  * \ingroup  IDU_Exported_Constants
  */
typedef enum
{
    IDU_SUCCESS = 0x0,                  /*!< IDU decode procedure successfully finished. */
    IDU_ERROR_NULL_INPUT,               /*!< The pointer points to input data is NULL. */
    IDU_ERROR_DECODE_FAIL,              /*!< IDU decode error interrupt is triggered and detected. */
    IDU_ERROR_START_EXCEED_BOUNDARY,    /*!< Decode start line or column exceeds boundary. */
    IDU_ERROR_END_EXCEED_BOUNDARY,      /*!< Decode end line or column exceeds boundary. */
    IDU_ERROR_START_LARGER_THAN_END,    /*!< Decode start line or column exceeds end line or column. */
    IDU_ERROR_INVALID_PARAM,            /*!< Parameters in DMA setting are invalid. */
    IDU_ERROR_FILE_INVALID,             /*!< Parameters in input compressed file setting are invalid. */
    IDU_ERROR_LINE_NOT_ALIGNED,         /*!< Line width in bytes must be 4-byte aligned when target stride is larger than line width. */
} IDU_ERROR;
/** End of IDU_ERROR
  * \}
  */

/**
 * \defgroup    IDU_Interrupts_Definition IDU Interrupt
 * \{
 * \ingroup     IDU_Exported_Constants
 */
typedef enum
{
    IDU_DECOMPRESS_FINISH_INT,          /*!< Triggered when entire decompress procedure is finished. */
    IDU_LINE_DECOMPRESS_FINISH_INT,     /*!< Triggered when decompress procedure of each line is finished. */
    IDU_RX_FIFO_THRESHOLD_INT,          /*!< Triggered when data level is greater than RX FIFO threshold. */
    IDU_RX_FIFO_OVERFLOW_INT,           /*!< Triggered when overflow occurs in RX FIFO. */
    IDU_TX_FIFO_THRESHOLD_INT,          /*!< Triggered when data level is less than TX FIFO threshold. */
    IDU_TX_FIFO_UNDERFLOW_INT,          /*!< Triggered when underflow occurs in TX FIFO. */
    IDU_DECOMPRESS_ERROR_INT,           /*!< Triggered when input data error is detected. */
} IDU_INTERRUPT_TYPE;

#define IS_IDU_INT(INT)              (((INT) == IDU_DECOMPRESS_FINISH_INT) || \
                                      ((INT) == IDU_LINE_DECOMPRESS_FINISH_INT) || \
                                      ((INT) == IDU_RX_FIFO_THRESHOLD_INT) || \
                                      ((INT) == IDU_RX_FIFO_OVERFLOW_INT) || \
                                      ((INT) == IDU_TX_FIFO_THRESHOLD_INT) || \
                                      ((INT) == IDU_TX_FIFO_UNDERFLOW_INT) || \
                                      ((INT) == IDU_DECOMPRESS_ERROR_INT))
/** End of IDU_Interrupts_Definition
  * \}
  */

/** End of IDU_Exported_Constants
  * \}
  */

/*============================================================================*
 *                             Types
 *============================================================================*/
/** \defgroup IDU_Exported_Types IDU Exported Types
  * \brief    Data structures and enumerations used by the IDU module
  * \{
  */

/**
 * \defgroup    IDU_FILE_HEADER IDU_file_header
 * \{
 * \ingroup     IDU_Exported_Types
 */
typedef struct
{
    struct
    {
        uint8_t algorithm: 2;       /*!< Compression algorithm of input file,
                                        This parameter can be a value of @ref IDU_ALGORITHM. */
        uint8_t feature_1: 2;       /*!< For RLE: Run length size, value of @ref IDU_RLE_RUNLENGTH_SIZE.
                                        For YUV: Sample type, value of @ref IDU_YUV_SAMPLE_TYPE. */
        uint8_t feature_2: 2;       /*!< For RLE: Run length size, value of @ref IDU_RLE_RUNLENGTH_SIZE.
                                        For YUV: Blur bit, value of @ref IDU_YUV_BLUR_BIT. */
        uint8_t pixel_bytes: 2;     /*!< Size of pixel, can be 1-4 bytes. */
    } algorithm_type;               /*!< Compressing information. */
    uint8_t reserved[3];            /*!< Reserved for future use. */
    uint32_t raw_pic_width;         /*!< Width of compressed picture in pixel. */
    uint32_t raw_pic_height;        /*!< Height of compressed picture in pixel. */
} IDU_file_header;
/** End of IDU_FILE_HEADER
  * \}
  */

/**
 * \defgroup    IDU_DECODE_RANGE IDU_decode_range
 * \{
 * \ingroup     IDU_Exported_Types
 */
typedef struct
{
    uint32_t start_line;            /*!< Start line of target decode range. */
    uint32_t end_line;              /*!< End line of target decode range. */
    uint32_t start_column;          /*!< Start column of target decode range. */
    uint32_t end_column;            /*!< End column of target decode range. */
    uint32_t target_stride;         /*!< Data length between 2 contiguous target lines.*/
} IDU_decode_range;
/** End of IDU_DECODE_RANGE
  * \}
  */

/**
 * \defgroup    IDU_DMA_CONFIG IDU_DMA_config
 * \{
 * \ingroup     IDU_Exported_Types
 */
typedef struct
{
    uint32_t RX_DMA_channel_num;            /*!< Number of DMA channel that transmits data to IDU FIFO. */
    uint32_t RX_FIFO_INT_threshold;         /*!< FIFO threshold that triggers \ref IDU_RX_FIFO_THRESHOLD_INT . */
    uint32_t TX_DMA_channel_num;            /*!< Number of DMA channel that receives data from IDU FIFO. */
    uint32_t TX_FIFO_INT_threshold;         /*!< FIFO threshold that triggers \ref IDU_TX_FIFO_THRESHOLD_INT . */
    uint32_t *output_buf;                   /*!< Pointer to buffer that stores decoded data. */
} IDU_DMA_config;
/** End of IDU_DMA_CONFIG
  * \}
  */

/**
 * \defgroup    IDU_INT_CONFIG IDU_interrupt_config
 * \{
 * \ingroup     IDU_Exported_Types
 */
typedef struct
{
    uint32_t idu_decompress_error_int: 1;   /*!< Operate on \ref IDU_DECOMPRESS_ERROR_INT. */
    uint32_t tx_fifo_underflow_int: 1;      /*!< Operate on \ref IDU_TX_FIFO_UNDERFLOW_INT. */
    uint32_t tx_fifo_threshold_int: 1;      /*!< Operate on \ref IDU_TX_FIFO_THRESHOLD_INT. */
    uint32_t rx_fifo_overflow_int: 1;       /*!< Operate on \ref IDU_RX_FIFO_OVERFLOW_INT. */
    uint32_t rx_fifo_threshold_int: 1;      /*!< Operate on \ref IDU_RX_FIFO_THRESHOLD_INT. */
    uint32_t line_decompress_finish_int: 1; /*!< Operate on \ref IDU_LINE_DECOMPRESS_FINISH_INT. */
    uint32_t idu_decompress_finish_int: 1;  /*!< Operate on \ref IDU_DECOMPRESS_FINISH_INT. */
} IDU_interrupt_config;
/** End of IDU_INT_CONFIG
  * \}
  */

/** End of IDU_Exported_Types
  * \}
  */


/*============================================================================*
 *                         Functions
 *============================================================================*/
/** \defgroup IDU_Exported_Functions IDU Exported Functions
  * \brief    Functions for IDU initialization, configuration and operation
  * \{
  */

/**
 * \brief  Enable/Disable IDU interrupt.
 * \param[in] Interrupt: Specified interrupt type from @ref IDU_INTERRUPT_TYPE.
 * \param[in] NewState: New state of the IDU interrupt.
 *            - ENABLE: Enable specified interrupt.
 *            - DISABLE: Disable specified interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
    void demo_code(void){
        IDU_INTConfig(IDU_DECOMPRESS_FINISH_INT, ENABLE);
    }
 * \endcode
 */
void IDU_INTConfig(IDU_INTERRUPT_TYPE Interrupt, FunctionalState NewState);

/**
 * \brief  Get interrupt status of IDU.
 * \param[in] Interrupt: Specified interrupt type from @ref IDU_INTERRUPT_TYPE.
 * \return Interrupt status
 * \retval - SET: Interrupt has been triggered.
 * \retval - RESET: Interrupt has not been triggered yet.
 * <b>Example usage</b>
 * \code{.c}
    void demo_code(void){
        ITStatus status = IDU_GetINTStatus(IDU_DECOMPRESS_FINISH_INT);
    }
 * \endcode
 */
ITStatus IDU_GetINTStatus(IDU_INTERRUPT_TYPE Interrupt);

/**
 * \brief  Mask or unmask specified IDU interrupt.
 * \param[in] Interrupt: Specified interrupt type from @ref IDU_INTERRUPT_TYPE.
 * \param[in] NewState: Mask or unmask the IDU interrupt.
 *            - ENABLE: Mask specified interrupt.
 *            - DISABLE: Unmask specified interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
    void demo_code(void){
        // Mask IDU_DECOMPRESS_FINISH_INT interrupt
        IDU_MaskINTConfig(IDU_DECOMPRESS_FINISH_INT, ENABLE);
    }
 * \endcode
 */
void IDU_MaskINTConfig(IDU_INTERRUPT_TYPE Interrupt, FunctionalState NewState);

/**
 * \brief  Clear status of specified IDU interrupt.
 * \param[in] Interrupt: Specified interrupt type from @ref IDU_INTERRUPT_TYPE.
 *
 * <b>Example usage</b>
 * \code{.c}
    void demo_code(void){
        IDU_ClearINTPendingBit(IDU_DECOMPRESS_FINISH_INT);
    }
 * \endcode
 */
void IDU_ClearINTPendingBit(IDU_INTERRUPT_TYPE Interrupt);

/**
 * \brief  IDU decode function, decode file from memory/flash.
 * \param[in] file: Pointer to header of compressed image.
 * \param[in] range: Decompress range, refer to @ref IDU_DECODE_RANGE.
 * \param[in] dma_cfg: Determine which DMA channel is selected, refer to @ref IDU_DMA_CONFIG.
 * \return Operation result
 * \retval - IDU_SUCCESS: Operation success.
 * \retval - Others: Operation failure, cause refers to @ref IDU_ERROR.
 *
 * <b>Example usage</b>
 * \code{.c}
    void demo_code(void){
        RCC_PeriphClockCmd(APBPeriph_IDU, APBPeriph_IDU_CLOCK, ENABLE);
        IDU_file_header* header = (IDU_file_header*)file_data;
        IDU_decode_range range = {0, header->raw_pic_height - 1, 0, header->raw_pic_width - 1, 204};
        IDU_DMA_config dma_cfg;
        dma_cfg.output_buf = (uint32_t*)buf;
        dma_cfg.RX_DMA_channel_num = 0;
        dma_cfg.TX_DMA_channel_num = 1;
        IDU_ERROR err = IDU_Decode(file_data, &range, &dma_cfg);
    }
 * \endcode
 */
IDU_ERROR IDU_Decode(uint8_t *file, IDU_decode_range *range, IDU_DMA_config *dma_cfg);

/**
 * \brief  IDU decode function with interrupt, decode file from memory/flash.
 * \param[in] file: Pointer to header of compressed image.
 * \param[in] range: Decompress range, refer to @ref IDU_DECODE_RANGE.
 * \param[in] dma_cfg: Determine which DMA channel is selected, refer to @ref IDU_DMA_CONFIG.
 * \param[in] int_cfg: Determine which interrupts are enabled, refer to @ref IDU_INT_CONFIG.
 * \return Operation result
 * \retval - IDU_SUCCESS: Operation success.
 * \retval - Others: Operation failure, cause refers to @ref IDU_ERROR.
 *
 * <b>Example usage</b>
 * \code{.c}
    void IDU_Handler(void){
        if(IDU_GetINTStatus(IDU_DECOMPRESS_FINISH_INT) == SET){
            IDU_ClearINTPendingBit(IDU_DECOMPRESS_FINISH_INT);
            //User operation
        }
        if(IDU_GetINTStatus(IDU_DECOMPRESS_ERROR_INT) == SET){
            IDU_ClearINTPendingBit(IDU_DECOMPRESS_ERROR_INT);
            //User operation
        }
    }
    void demo_code(void){
        RCC_PeriphClockCmd(APBPeriph_IDU, APBPeriph_IDU_CLOCK, ENABLE);
        IDU_file_header* header = (IDU_file_header*)file_data;
        IDU_decode_range range = {0, header->raw_pic_height - 1, 0, header->raw_pic_width - 1, 0};
        IDU_DMA_config dma_cfg;
        dma_cfg.output_buf = (uint32_t*)buf;
        dma_cfg.RX_DMA_channel_num = 0;
        dma_cfg.TX_DMA_channel_num = 1;
        IDU_interrupt_config int_cfg = {0};
        int_cfg.idu_decompress_finish_int = 1;
        int_cfg.idu_decompress_error_int = 1;
        IDU_ERROR err = IDU_Decode_With_Interrupt(file_data, &range, &dma_cfg, int_cfg);
    }
 * \endcode
 */
IDU_ERROR IDU_Decode_With_Interrupt(uint8_t *file, IDU_decode_range *range, IDU_DMA_config *dma_cfg,
                                    IDU_interrupt_config int_cfg);


/**
 * \brief  Get the start address of certain line in compressed file.
 * \param[in] compressed_start_address: start address of entire compressed file.
 * \param[in] line_number: specified line number
 * \return start address of specified line
 * \retval - 0: Invalid line number.
 * \retval - others: Valid address.
 *
 * <b>Example usage</b>
 * \code{.c}
    void demo_code(void){
        uint32_t file_address = (uint32_t)SAMPLE_FILE;
        uint32_t line_addr = IDU_Get_Line_Start_Address(file_address, 100);
    }
 * \endcode
 */
uint32_t IDU_Get_Line_Start_Address(uint32_t compressed_start_address,
                                    uint32_t line_number);

/**
 * \brief  Get the pixel size of compressed image.
 * \param[in] file: Pointer to header of compressed image.
 * \return Pixel size of compressed image
 * \retval - 0xFF: Invalid header.
 * \retval - Others: Actual pixel size.
 *
 * <b>Example usage</b>
 * \code{.c}
    void demo_code(void){
        IDU_file_header* file = (IDU_file_header*)SAMPLE_FILE;
        uint8_t pixel_size = IDU_Get_Pixel_Size(file);
        uint32_t decompressed_image_size = file->raw_pic_width * file->raw_pic_height * pixel_size;
    }
 * \endcode
 */
uint8_t IDU_Get_Pixel_Size(IDU_file_header *file);

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