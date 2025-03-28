/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* \file     rtl_lcdc_dma.h
* \brief    The header file of the peripheral driver for DMA inside LCDC.
* \details  This file provides all LCDC DMA firmware functions.
* \author   HOWIE
* \date     2023-10-17
* \version  v1.0
* *********************************************************************************************************
*/

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_LCDC_DMA_H
#define RTL_LCDC_DMA_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "rtl_lcdc_dma_def.h"

/** \defgroup LCDC        LCDC
  * \brief    LCD Controller driver module
  * \{
  */

/** \defgroup LCDC_DMA          LCDC DMA
  * \brief    LCD Controller internal DMA driver
  * \{
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/
/** \defgroup LCDC_DMA_Exported_Constants LCDC DMA Exported Constants
  * \brief    Constants used by the LCDC DMA driver
  * \{
  */

/**
 * \defgroup    LCDC_DMA_Data_Transfer_Direction LCDC DMA Data Transfer Direction
 * \{
 * \ingroup     LCDC_DMA_Exported_Constants
 */
typedef enum
{
    LCDC_DMA_DIR_PeripheralToMemory = 0x4,        /*!< Data transmission from peripheral to memory. */
    LCDC_DMA_DIR_PeripheralToPeripheral = 0x6,    /*!< Data transmission from peripheral to peripheral. */
} LCDC_DMA_DIRECTION_T;

#define IS_LCDC_DMA_DIR(DIR)  (((DIR) == LCDC_DMA_DIR_PeripheralToMemory) || \
                               ((DIR) == LCDC_DMA_DIR_PeripheralToPeripheral))

/** End of LCDC_DMA_Data_Transfer_Direction
  * \}
  */

/**
 * \defgroup    LCDC_DMA_Source_Incremented_Mode LCDC DMA Source Incremented Mode
 * \{
 * \ingroup     LCDC_DMA_Exported_Constants
 */
typedef enum
{
    LCDC_DMA_SourceInc_Inc = 0x0,    /*!< Source address increases. */
    LCDC_DMA_SourceInc_Dec = 0x1,    /*!< Source address decreases. */
    LCDC_DMA_SourceInc_Fix = 0x2,    /*!< Source address is fixed. */
} LCDC_DMA_SRC_INC_T;

#define IS_LCDC_DMA_SourceInc(STATE) (((STATE) == LCDC_DMA_SourceInc_Inc) || \
                                      ((STATE) == LCDC_DMA_SourceInc_Dec) || \
                                      ((STATE) == LCDC_DMA_SourceInc_Fix))

/** End of LCDC_DMA_Source_Incremented_Mode
  * \}
  */

/**
 * \defgroup    LCDC_DMA_Destination_Incremented_Mode LCDC DMA Destination Incremented Mode
 * \{
 * \ingroup     LCDC_DMA_Exported_Constants
 */
typedef enum
{
    LCDC_DMA_DestinationInc_Fix = 0x2,   /*!< Destination address is fixed. */
} LCDC_DMA_DEST_INC_T;

#define IS_LCDC_DMA_DestinationInc(STATE) (((STATE) == LCDC_DMA_DestinationInc_Fix))

/** End of LCDC_DMA_Destination_Incremented_Mode
  * \}
  */

/**
 * \defgroup    LCDC_DMA_Data_Size LCDC DMA Data Size
 * \{
 * \ingroup     LCDC_DMA_Exported_Constants
 */
typedef enum
{
    LCDC_DMA_DataSize_Byte     = 0x0,    /*!< Data size is 8 bits. */
    LCDC_DMA_DataSize_HalfWord = 0x1,    /*!< Data size is 16 bits. */
    LCDC_DMA_DataSize_Word     = 0x2,    /*!< Data size is 32 bits. */
} LCDC_DMA_DATASIZE_T;

#define IS_LCDC_DMA_DATA_SIZE(SIZE) (((SIZE) == LCDC_DMA_DataSize_Byte) || \
                                     ((SIZE) == LCDC_DMA_DataSize_HalfWord) || \
                                     ((SIZE) == LCDC_DMA_DataSize_Word))

/** End of LCDC_DMA_Data_Size
  * \}
  */

/**
 * \defgroup    LCDC_DMA_Msize LCDC DMA Msize
 * \{
 * \ingroup     LCDC_DMA_Exported_Constants
 */
typedef enum
{
    LCDC_DMA_Msize_1   = 0x0,    /*!< Transmit 1 item in a burst. */
    LCDC_DMA_Msize_4   = 0x1,    /*!< Transmit 4 items in a burst. */
    LCDC_DMA_Msize_8   = 0x2,    /*!< Transmit 8 items in a burst. */
    LCDC_DMA_Msize_16  = 0x3,    /*!< Transmit 16 items in a burst. */
    LCDC_DMA_Msize_32  = 0x4,    /*!< Transmit 32 items in a burst. */
    LCDC_DMA_Msize_64  = 0x5,    /*!< Transmit 64 items in a burst. */
    LCDC_DMA_Msize_128 = 0x6,    /*!< Transmit 128 items in a burst. */
} LCDC_DMA_MSIZE_T;

#define IS_LCDC_DMA_Msize(SIZE) (((SIZE) == LCDC_DMA_Msize_1) || \
                                 ((SIZE) == LCDC_DMA_Msize_4) || \
                                 ((SIZE) == LCDC_DMA_Msize_8) || \
                                 ((SIZE) == LCDC_DMA_Msize_16) || \
                                 ((SIZE) == LCDC_DMA_Msize_32) || \
                                 ((SIZE) == LCDC_DMA_Msize_64) || \
                                 ((SIZE) == LCDC_DMA_Msize_128))
/** End of LCDC_DMA_Msize
  * \}
  */

/**
 * \defgroup    LCDC_DMA_Multiblock_Mode LCDC DMA Multi-block Mode
 * \{
 * \ingroup     LCDC_DMA_Exported_Constants
 */
#define AUTO_RELOAD_WITH_CONTIGUOUS_SAR     (BIT31)              /*!< The DARx register can be automatically reloaded from its initial value at the end of every block for multi-block transfers. */
#define AUTO_RELOAD_TRANSFER                (BIT30 | BIT31)      /*!< Both DARx and SARx registers can be automatically reloaded from its initial value at the end of every block for multi-block transfers. */
#define LLI_TRANSFER                        (BIT27 | BIT28)      /*!< Multi-block with Linklist module. */

#define IS_DMA_MULTIBLOCKMODE(MODE) (((MODE) == AUTO_RELOAD_WITH_CONTIGUOUS_SAR) || ((MODE) == LLI_TRANSFER)\
                                     ||((MODE) == AUTO_RELOAD_TRANSFER))

/** End of LCDC_DMA_Multiblock_Mode
  * \}
  */

/** End of LCDC_DMA_Exported_Constants
  * \}
  */

/**
 * \cond    private
 * \brief   DMA mode description
 * \defgroup DMA_MODE_BIT_FIELD LCDC DMA Mode Bit Field
 * \{
 */
#define LCDC_DMA_AUTO_RELOAD_SELECTED_BIT        (BIT30 | BIT31)  /*!< SARx and DARx are auto reloaded. */
#define LCDC_DMA_LLP_SELECTED_BIT                (BIT27 | BIT28)  /*!< Use internal Linklist module. */
/**
 * \}
 * \endcond
 */

/*============================================================================*
 *                         Types
 *============================================================================*/
/** \defgroup LCDC_DMA_Exported_Types LCDC_DMA Exported Types
  * \brief
  * \{
  */

/**
 * \brief       LCDC DMA init structure definition.
 *
 * \ingroup     LCDC_DMA_Exported_Types
 */
typedef struct
{
    uint8_t  LCDC_DMA_ChannelNum;               /*!< Specifies channel number for LCDC DMA. */
    LCDC_DMA_DIRECTION_T  LCDC_DMA_DIR;         /*!< Specifies transfer direction.
                                                           This parameter can be a value of @ref LCDC_DMA_Data_Transfer_Direction */
    uint32_t LCDC_DMA_BufferSize;               /*!< Specifies the buffer size. */
    LCDC_DMA_SRC_INC_T
    LCDC_DMA_SourceInc;      /*!< Specifies whether the source address register is incremented or not.
                                                          This parameter can be a value of @ref LCDC_DMA_Source_Incremented_Mode */
    LCDC_DMA_DEST_INC_T
    LCDC_DMA_DestinationInc;/*!< Specifies whether the destination address register is incremented or not.
                                                          This parameter can be a value of @ref LCDC_DMA_Destination_Incremented_Mode. */
    LCDC_DMA_DATASIZE_T LCDC_DMA_SourceDataSize;    /*!< Specifies the source data width.
                                                          This parameter can be a value of @ref LCDC_DMA_Data_Size. */
    LCDC_DMA_DATASIZE_T LCDC_DMA_DestinationDataSize; /*!< Specifies the destination data width.
                                                            This parameter can be a value of @ref LCDC_DMA_Data_Size. */
    LCDC_DMA_MSIZE_T LCDC_DMA_SourceMsize;          /*!< Specifies items number to be transferred.
                                                          This parameter can be a value of @ref LCDC_DMA_Msize. */
    LCDC_DMA_MSIZE_T LCDC_DMA_DestinationMsize;     /*!< Specifies items number to be transferred.
                                                          This parameter can be a value of @ref LCDC_DMA_Msize */
    uint32_t LCDC_DMA_SourceAddr;               /*!< Specifies the source base address for LCDC DMA Channelx. */
    uint32_t LCDC_DMA_DestinationAddr;          /*!< Specifies the destination base address for LCDC DMA Channelx. */
    uint32_t LCDC_DMA_ChannelPriority;          /*!< Specifies the software priority for the LCDC DMA Channelx. */
    uint32_t LCDC_DMA_Multi_Block_Mode;         /*!< Specifies the multi block transfer mode.
                                                 This parameter can be a value of \ref LCDC_DMA_Multiblock_Mode. */
    uint32_t LCDC_DMA_Multi_Block_Struct;       /*!< Pointer to the first struct of LLI. */
    uint8_t  LCDC_DMA_Multi_Block_En;           /*!< Enable or disable multi-block function. */
    uint8_t  LCDC_DMA_Secure_En;                /*!< Enable or disable Secure DMA function. */
} LCDC_DMA_InitTypeDef;

/** End of LCDC_DMA_Exported_Types
  * \}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** \defgroup LCDC_DMA_Exported_Functions LCDC_DMA Exported Functions
  * \brief
  * \{
  */

/**
 * \brief  Initializes the LCDC DMA Channelx according to the specified parameters in the LCDC_DMA_InitStruct.
 *
 * \param[in] LCDC_DMA_Channelx: Where x can be 0 to select the LCDC DMA Channel.
 * \param[in] LCDC_DMA_InitStruct: Pointer to a LCDC_DMA_InitTypeDef structure that contains the configuration information for the specified LCDC DMA Channel.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dma_init(void)
 * {
 *     LCDC_DMA_InitTypeDef LCDC_DMA_InitStruct = {0};
 *     LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
 *     LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum          = 0;
 *     LCDC_DMA_InitStruct.LCDC_DMA_SourceInc           = LCDC_DMA_SourceInc_Inc;
 *     LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc      = LCDC_DMA_DestinationInc_Fix;
 *     LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize      = LCDC_DMA_DataSize_Word;
 *     LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
 *     LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_8;
 *     LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_8;
 *     LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = (uint32_t)buf;
 *     LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = 0;
 *     LCDC_DMA_Init(LCDC_DMA_Channel0, &LCDC_DMA_InitStruct);
 * }
 * \endcode
 */
void LCDC_DMA_Init(LCDC_DMA_ChannelTypeDef *LCDC_DMA_Channelx,
                   LCDC_DMA_InitTypeDef *LCDC_DMA_InitStruct);

/**
 * \brief  Fills each LCDC_DMA_InitStruct member with its default value.
 *
 * \param[in] LCDC_DMA_InitStruct: pointer to a LCDC_DMA_InitTypeDef structure which will be initialized.
 *
 * \note The default settings for the LCDC_DMA_InitStruct member are shown in the following table:
 *       | LCDC_DMA_InitStruct member     | Default value                        |
 *       |:------------------------------:|:------------------------------------:|
 *       | LCDC_DMA_ChannelNum            | 0                                    |
 *       | LCDC_DMA_DIR                   | \ref LCDC_DMA_DIR_PeripheralToMemory |
 *       | LCDC_DMA_BufferSize            | 0                                    |
 *       | LCDC_DMA_SourceInc             | \ref LCDC_DMA_SourceInc_Fix          |
 *       | LCDC_DMA_DestinationInc        | \ref LCDC_DMA_DestinationInc_Fix     |
 *       | LCDC_DMA_SourceDataSize        | \ref LCDC_DMA_DataSize_Byte          |
 *       | LCDC_DMA_DestinationDataSize   | \ref LCDC_DMA_DataSize_Byte          |
 *       | LCDC_DMA_SourceMsize           | \ref LCDC_DMA_Msize_1                |
 *       | LCDC_DMA_DestinationMsize      | \ref LCDC_DMA_Msize_1                |
 *       | LCDC_DMA_SourceAddr            | 0                                    |
 *       | LCDC_DMA_DestinationAddr       | 0                                    |
 *       | LCDC_DMA_Multi_Block_En        | DISABLE                              |
 *       | LCDC_DMA_Multi_Block_Mode      | \ref LLI_TRANSFER                    |
 *       | LCDC_DMA_Multi_Block_Struct    | 0                                    |
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dma_init(void)
 * {
 *     LCDC_DMA_InitTypeDef LCDC_DMA_InitStruct = {0};
 *     LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
 *     LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum          = 0;
 *     LCDC_DMA_InitStruct.LCDC_DMA_SourceInc           = LCDC_DMA_SourceInc_Inc;
 *     LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc      = LCDC_DMA_DestinationInc_Fix;
 *     LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize      = LCDC_DMA_DataSize_Word;
 *     LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
 *     LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_8;
 *     LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_8;
 *     LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = (uint32_t)buf;
 *     LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = 0;
 *     LCDC_DMA_Init(LCDC_DMA_Channel0, &LCDC_DMA_InitStruct);
 * }
 * \endcode
 */
void LCDC_DMA_StructInit(LCDC_DMA_InitTypeDef *LCDC_DMA_InitStruct);

/**
 * \brief  Enable or disable the selected LCDC DMA channel.
 *
 * \param[in]  LCDC_DMA_Channel_Num: LCDC DMA channel number, which can be 0.
 * \param[in]  NewState: New state of the selected LCDC DMA channel.
 *             This parameter can be one of the following values:
 *             - ENABLE: Enable the selected LCDC DMA channel.
 *             - DISABLE: Disable the selected LCDC DMA channel.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dma_init(void)
 * {
 *     LCDC_DMAChannelCmd(0, ENABLE);
 * }
 * \endcode
 */
void LCDC_DMAChannelCmd(uint8_t LCDC_DMA_Channel_Num, FunctionalState NewState);

/** End of LCDC_DMA_Exported_Functions
  * \}
  */

/** End of LCDC_DMA
  * \}
  */

/** End of LCDC
  * \}
  */

#ifdef __cplusplus
}
#endif

#endif /* RTL_LCDC_DMA_H */


/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/

