/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* \file     rtl_lcdc_dma.h
* \brief    The header file of the peripheral GDMA driver.
* \details  This file provides all GDMA firmware functions.
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

#include "rtl876x.h"
//#include "rtl_lcdc_dma_reg.h"
#include "rtl_lcdc_dma_def.h"
//#if defined (CONFIG_SOC_SERIES_RTL87X2G)
//#include "lcdc/src/rtl87x2g/rtl_lcdc_dma_def.h"
//#endif

/** \defgroup LCDC        LCDC
  * \brief
  * \{
  */

/** \defgroup LCDC_DMA          LCDC_DMA
  * \brief
  * \{
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/
/** \defgroup LCDC_DMA_Exported_Constants LCDC DMA Exported Constants
  * \brief
  * \{
  */

/**
 * \defgroup    LCDC_DMA_Data_Transfer_Direction LCDC DMA Data Transfer Direction
 * \{
 * \ingroup     LCDC_DMA_Exported_Constants
 */
typedef enum
{
    LCDC_DMA_DIR_PeripheralToMemory = 0x4,
    LCDC_DMA_DIR_PeripheralToPeripheral = 0x6,
} LCDC_DMA_DIRECTION_T;

#define IS_LCDC_DMA_DIR(DIR)  ((DIR) == LCDC_DMA_DIR_PeripheralToMemory) || \
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
    LCDC_DMA_SourceInc_Inc = 0x0,
    LCDC_DMA_SourceInc_Dec = 0x1,
    LCDC_DMA_SourceInc_Fix = 0x2,
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
    LCDC_DMA_DestinationInc_Inc = 0x0,
    LCDC_DMA_DestinationInc_Dec = 0x1,
    LCDC_DMA_DestinationInc_Fix = 0x2,
} LCDC_DMA_DEST_INC_T;

#ifndef IS_LCDC_DMA_DestinationInc
#define IS_LCDC_DMA_DestinationInc(STATE) (((STATE) == DMA_DestinationInc_Inc) || \
                                           ((STATE) == DMA_DestinationInc_Dec) || \
                                           ((STATE) == DMA_DestinationInc_Fix))
#endif

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
    LCDC_DMA_DataSize_Byte     = 0x0,
    LCDC_DMA_DataSize_HalfWord = 0x1,
    LCDC_DMA_DataSize_Word     = 0x2,
} LCDC_DMA_DATASIZE_T;

#define IS_GDMA_DATA_SIZE(SIZE) (((SIZE) == GDMA_DataSize_Byte) || \
                                 ((SIZE) == GDMA_DataSize_HalfWord) || \
                                 ((SIZE) == GDMA_DataSize_Word))

/** End of LCDC_DMA_Data_Size
  * \}
  */

/**
 * \brief       LCDC_DMA_Msize LCDC DMA Msize
 *
 * \ingroup     LCDC_DMA_Exported_Constants
 */
typedef enum
{
    LCDC_DMA_Msize_1   = 0x0,
    LCDC_DMA_Msize_4   = 0x1,
    LCDC_DMA_Msize_8   = 0x2,
    LCDC_DMA_Msize_16  = 0x3,
    LCDC_DMA_Msize_32  = 0x4,
    LCDC_DMA_Msize_64  = 0x5,
    LCDC_DMA_Msize_128 = 0x6,
    LCDC_DMA_Msize_256 = 0x7,
} LCDC_DMA_MSIZE_T;

/** End of LCDC_DMA_Msize
  * \}
  */

/**
 * \defgroup    LCDC_DMA_SELECTED LCDC DMA SELECTED
 * \{
 * \ingroup     LCDC_DMA_Exported_Constants
 */
#define LCDC_DMA_AUTO_RELOAD_SELECTED_BIT        (BIT30 | BIT31)
#define LCDC_DMA_LLP_SELECTED_BIT                (BIT27 | BIT28)

/** End of LCDC_DMA_SELECTED
  * \}
  */

/** End of LCDC_DMA_Exported_Constants
  * \}
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
    uint32_t LCDC_DMA_BufferSize;               /*!< Specifies the buffer size(<=65535).
                                                          The data unit is equal to the configuration set in DMA_PeripheralDataSize
                                                          or DMA_MemoryDataSize members depending in the transfer direction. */
    LCDC_DMA_SRC_INC_T
    LCDC_DMA_SourceInc;      /*!< Specifies whether the source address register is incremented or not.
                                                          This parameter can be a value of @ref LCDC_DMA_Source_Incremented_Mode */
    LCDC_DMA_DEST_INC_T
    LCDC_DMA_DestinationInc;/*!< Specifies whether the destination address register is incremented or not.
                                                          This parameter can be a value of @ref LCDC_DMA_Destination_Incremented_Mode */
    LCDC_DMA_DATASIZE_T LCDC_DMA_SourceDataSize;    /*!< Specifies the source data width.
                                                          This parameter can be a value of @ref LCDC_DMA_Data_Size */
    LCDC_DMA_DATASIZE_T LCDC_DMA_DestinationDataSize; /*!< Specifies the destination data width.
                                                            This parameter can be a value of @ref LCDC_DMA_Data_Size */
    LCDC_DMA_MSIZE_T LCDC_DMA_SourceMsize;          /*!< Specifies items number to be transferred.
                                                          This parameter can be a value of @ref LCDC_DMA_Msize */
    LCDC_DMA_MSIZE_T LCDC_DMA_DestinationMsize;     /*!< Specifies items number to be transferred.
                                                          This parameter can be a value of @ref LCDC_DMA_Msize */
    uint32_t LCDC_DMA_SourceAddr;               /*!< Specifies the source base address for LCDC DMA Channelx. */
    uint32_t LCDC_DMA_DestinationAddr;          /*!< Specifies the destination base address for LCDC DMA Channelx. */
    uint32_t LCDC_DMA_ChannelPriority;          /*!< Specifies the software priority for the LCDC DMA Channelx. */
    uint32_t LCDC_DMA_Multi_Block_Mode;         /*!< Specifies the multi block transfer mode.
                                                 This parameter can be a value of \ref LCDC_DMA_Multiblock_Mode. */
    uint32_t LCDC_DMA_Multi_Block_Struct;       /*!< Pointer to the first struct of LLI. */
    uint8_t  LCDC_DMA_Multi_Block_En;           /*!< Enable or disable multi-block function. */
    uint8_t  LCDC_DMA_SourceHandshake;          /*!< Specifies the handshake index in source. */
    uint8_t  LCDC_DMA_DestHandshake;            /*!< Specifies the handshake index in Destination. */
    uint8_t  LCDC_DMA_Gather_En;                /*!< Enable or disable Gather function. NOTE:4 bytes ALIGN.*/
    uint32_t LCDC_DMA_GatherCount;              /*!< Specifies the GatherCount.NOTE:4 bytes ALIGN.*/
    uint32_t LCDC_DMA_GatherInterval;           /*!< Specifies the GatherInterval. */
    uint8_t  LCDC_DMA_Scatter_En;               /*!< Enable or disable Scatter function. */
    uint32_t LCDC_DMA_ScatterCount;             /*!< Specifies the ScatterCount. */
    uint32_t LCDC_DMA_ScatterInterval;          /*!< Specifies the ScatterInterval. */
    uint32_t LCDC_DMA_GatherCircularStreamingNum;  /*!< Specifies the GatherCircularStreamingNum. */
    uint32_t LCDC_DMA_ScatterCircularStreamingNum; /*!< Specifies the ScatterCircularStreamingNum. */
    uint8_t  LCDC_DMA_Secure_En;                /*!< Enable or disable Secure function. */
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
 * rtl_lcdc_dma.h
 *
 * \brief  Initializes the LCDC DMA Channelx according to the specified parameters in the LCDC_DMA_InitStruct.
 *
 * \param[in] LCDC_DMA_Channelx: Where x can be 0 to select the LCDC DMA Channel.
 * \param[in] LCDC_DMA_InitStruct: Pointer to a LCDC_DMA_InitTypeDef structure that contains the configuration information for the specified LCDC DMA Channel.
 *
 * \return None.
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
 * rtl_lcdc_dma.h
 *
 * \brief  Fills each LCDC_DMA_InitStruct member with its default value.
 *
 * \param[in] LCDC_DMA_InitStruct: pointer to a LCDC_DMA_InitTypeDef structure which will be initialized.
 *
 * \return None.
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
 * rtl_lcdc_dma.h
 *
 * \brief  Enable or disable the selected LCDC DMA channel.
 *
 * \param[in]  LCDC_DMA_Channel_Num: LCDC DMA channel number, which can be 0.
 * \param[in]  NewState: New state of the selected LCDC DMA channel.
 *             This parameter can be one of the following values:
 *             \arg ENABLE: Enable the selected LCDC DMA channel.
 *             \arg DISABLE: Disable the selected LCDC DMA channel.
 *
 * \return None.
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

