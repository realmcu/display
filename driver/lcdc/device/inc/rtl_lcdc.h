/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* \file     rtl_lcdc.h
* \brief    This file provides all the LCDC HANDLER firmware functions.
* \details
* \author   howie wang
* \date     2023-10-17
* \version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef __RTL_LCDC_H
#define __RTL_LCDC_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/

#include "lcdc/device/inc/rtl_lcdc_dma.h"
#include "lcdc/device/src/rtl87x2g/rtl_lcdc_handler_def.h"
#include "lcdc/device/src/rtl87x2g/rtl_lcdc_dma_ll_def.h"
//#if defined (CONFIG_SOC_SERIES_RTL87X2G)
//#include "lcdc/device/src/rtl87x2g/rtl_lcdc_handler_def.h"
//#include "lcdc/device/src/rtl87x2g/rtl_lcdc_dma_ll_def.h"
//#endif

/*============================================================================*
 *                          Private Macros
 *============================================================================*/
#define DISPLAY_CTRL_REG_BASE                0x40017000UL

#if defined (CONFIG_SOC_SERIES_RTL87X2G)

#define DISPLAY_CTRL_REG_BASE              0x40023000UL

#elif defined RTL8763EP

#define DISPLAY_CTRL_REG_BASE                0x40017000UL

#endif
#define LCDC_DMA_CHANNEL_REG_BASE           (DISPLAY_CTRL_REG_BASE + 0)
#define LCDC_DMA_Channel0_BASE              (DISPLAY_CTRL_REG_BASE + 0x000)
#define LCDC_DMA_REG_BASE                   (DISPLAY_CTRL_REG_BASE + 0x2c0)
#define DBIC_REG_BASE                       (DISPLAY_CTRL_REG_BASE + 0x400)
#define LCDC_DMA_LINKLIST_REG_BASE          (DISPLAY_CTRL_REG_BASE + 0x600)
#define EDPI_REG_BASE                       (DISPLAY_CTRL_REG_BASE + 0x700)
#define DBIB_REG_BASE                       (DISPLAY_CTRL_REG_BASE + 0x800)
#define LCDC_HANDLER_REG_BASE               (DISPLAY_CTRL_REG_BASE + 0x900)
#define RAMLESS_QSPI_REG_BASE               (DISPLAY_CTRL_REG_BASE + 0xA00)//0x40023A00UL

#define LCDC_DMA_Channel0                   ((LCDC_DMA_ChannelTypeDef *) LCDC_DMA_Channel0_BASE)
#define LCDC_DMA_BASE                       ((LCDC_DMA_TypeDef *) LCDC_DMA_REG_BASE)
#define DBIC                                ((LCDC_DBIC_TypeDef *)DBIC_REG_BASE)
#define LCDC_DMA_LINKLIST                   ((LCDC_DMA_LinkList_TypeDef *)LCDC_DMA_LINKLIST_REG_BASE)
#define EDPI                                ((LCDC_EDPI_TypeDef *)EDPI_REG_BASE)
#define DBIB                                ((LCDC_DBIB_TypeDef *)DBIB_REG_BASE)
#define LCDC_HANDLER                        ((LCDC_Handler_TypeDef *)LCDC_HANDLER_REG_BASE)
#define RAMLESS_QSPI                        ((LCDC_Ramless_QSPI_TypeDef *)RAMLESS_QSPI_REG_BASE)

#define LCDC_DMA_ENABLE             BIT0

#define LCDC_TX_CNT

#define LCDC_DMA_RX_FIFO_OFFSET             ((uint32_t)(0xFFFF << 16))
#define LCDC_DMA_TX_FIFO_OFFSET             ((uint32_t)(0xFFFF))

typedef struct
{
    __IO uint32_t GRP1_SAR;
    __IO uint32_t GRP1_DAR;
    __IO uint32_t GRP1_LLP;
    __IO uint32_t GRP1_CTL0;
    __IO uint32_t GRP1_CTL1;
    __I  uint32_t RSVD0[3];
    __IO uint32_t GRP1_SAR_OFFSET;
    __IO uint32_t GRP1_DAR_OFFSET;
    __IO uint32_t GRP2_SAR_OFFSET;
    __IO uint32_t GRP2_DAR_OFFSET;
    __IO uint32_t GRP2_SAR;
    __IO uint32_t GRP2_DAR;
    __IO uint32_t GRP2_LLP;
    __IO uint32_t GRP2_CTL0;
    __IO uint32_t GRP2_CTL1;
    __IO uint32_t DMA_CFG;
    __I  uint32_t RSVD1[2];
    __IO uint32_t GRP1_SAR_DMA;
    __IO uint32_t GRP1_DAR_DMA;
    __IO uint32_t GRP1_LLP_DMA;
    __IO uint32_t GRP1_CTL0_DMA;
    __IO uint32_t GRP1_CTL1_DMA;
    __I  uint32_t RSVD2[3];
    __IO uint32_t GRP2_SAR_DMA;
    __IO uint32_t GRP2_DAR_DMA;
    __IO uint32_t GRP2_LLP_DMA;
    __IO uint32_t GRP2_CTL0_DMA;
    __IO uint32_t GRP2_CTL1_DMA;
    __I  uint32_t RSVD3[2];
    __IO uint32_t GRP1_SAR_FOR_INFINITE_MODE;
    __IO uint32_t GRP2_SAR_FOR_INFINITE_MODE;
} LCDC_DMA_LinkListTypeDef;

/** \defgroup LCDC        LCDC
  * \brief
  * \{
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/
/** \defgroup LCDC_Exported_Constants LCDC Exported Constants
  * \brief
  * \{
  */

/**
 * \defgroup    LCDC_Interface LCDC Interface
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
typedef enum
{
    LCDC_IF_eDPI,
    LCDC_IF_DBIB,
    LCDC_IF_DBIC,
    LCDC_IF_DPI
} LCDC_IF_T;

#define IS_LCDC_IF_SEL(IF) (((IF) == LCDC_IF_eDPI) || ((IF) == LCDC_IF_DBIB) || \
                            ((IF) == LCDC_IF_DBIC) || ((IF) == LCDC_IF_DPI))

/** End of LCDC_Interface
  * \}
  */

/**
 * \defgroup    LCDC_Pixel_Input_Format LCDC Pixel Input Format
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
typedef enum
{
    LCDC_INPUT_ARGB8888,
    LCDC_INPUT_BGR565,
    LCDC_INPUT_RGB565,
    LCDC_INPUT_RGB888,
    LCDC_INPUT_ABGR8888,
} LCDC_INPUT_FORMAT_T;

#define IS_LCDC_INPUT_FORMAT(FORMAT) (((FORMAT) == LCDC_INPUT_ARGB8888) || ((FORMAT) == LCDC_INPUT_BGR565) || \
                                      ((FORMAT) == LCDC_INPUT_RGB565) || ((FORMAT) == LCDC_INPUT_RGB888) || \
                                      ((FORMAT) == LCDC_INPUT_ABGR8888))

/** End of LCDC_Pixel_Input_Format
  * \}
  */

/**
 * \defgroup    LCDC_Pixel_Output_Format LCDC Pixel Output Format
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
typedef enum
{
    LCDC_OUTPUT_RGB888,
    LCDC_OUTPUT_RGB565,
    LCDC_OUTPUT_BGR888,
    LCDC_OUTPUT_BGR565,
} LCDC_OUTPUT_FORMAT_T;

#define IS_LCDC_OUTPUT_FORMAT(FORMAT) ((FORMAT) == LCDC_OUTPUT_RGB888) || ((FORMAT) == LCDC_OUTPUT_RGB565) || ((FORMAT) == LCDC_OUTPUT_BGR565)

/** End of LCDC_Pixel_Output_Format
  * \}
  */

/**
 * \defgroup    LCDC_Pixel_Bit_Swap LCDC Pixel Bit Swap
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
typedef enum
{
    LCDC_SWAP_BYPASS,
    LCDC_SWAP_8BIT,
    LCDC_SWAP_16BIT,
    LCDC_SWAP_8BIT_16BIT,
} LCDC_SWAP_BIT_T;

#define IS_LCDC_SWAP(SWAP) (((SWAP) == LCDC_SWAP_BYPASS) || ((SWAP) == LCDC_SWAP_8BIT) || \
                            ((SWAP) == LCDC_SWAP_16BIT) || ((SWAP) == LCDC_SWAP_8BIT_16BIT))

/** End of LCDC_Pixel_Bit_Swap
  * \}
  */

/**
 * \defgroup    LCDC_Tear_Signal_Valid LCDC Tear Signal Valid
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
typedef enum
{
    LCDC_TE_SIGNAL_VALID_LOW,
    LCDC_TE_SIGNAL_VALID_HIGH,
} LCDC_TE_SIGNAL_VALID_T;

#define IS_LCDC_TE_SIGNAL_VALID(VALID) ((VALID) == LCDC_TE_SIGNAL_VALID_HIGH) || ((VALID) == LCDC_TE_SIGNAL_VALID_LOW)

/** End of LCDC_Tear_Signal_Valid
  * \}
  */

/**
 * \defgroup    LCDC_Tear_Polarity LCDC Tear Polarity
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
typedef enum
{
    LCDC_TE_EDGE_RISING,
    LCDC_TE_EDGE_FALLING
} LCDC_TE_EDGE_T;

#define IS_LCDC_TE_POLARITY(POLARITY) ((POLARITY) == LCDC_TE_EDGE_RISING) || ((POLARITY) == LCDC_TE_EDGE_FALLING)

/** End of LCDC_Tear_Polarity
  * \}
  */

/**
 * \defgroup    LCDC_Tear_Input_Mux LCDC Tear Input Mux
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
typedef enum
{
    LCDC_TE_LCD_INPUT,
    LCDC_TE_DSIHOST_INPUT
} LCDC_INPUT_MUX_T;

#define IS_LCDC_TE_INPUTMUX(INPUT) ((INPUT) == LCDC_TE_LCD_INPUT) || ((INPUT) == LCDC_TE_DSIHOST_INPUT)

/** End of LCDC_Tear_Input_Mux
  * \}
  */

/**
 * \defgroup    LCDC_Operate_Mode LCDC Operate Mode
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
typedef enum
{
    LCDC_AUTO_MODE,
    LCDC_MANUAL_MODE,
} LCDC_MODE_T;

#define IS_LCDC_MODE(MODE) ((MODE) == LCDC_AUTO_MODE) || ((MODE) == LCDC_MANUAL_MODE)

/** End of LCDC_Operate_Mode
  * \}
  */

/**
 * \defgroup    LCDC_Operate_Direction LCDC Operate Direction
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
typedef enum
{
    LCDC_TX_MODE,
    LCDC_RX_MODE,
} LCDC_DIRECTION_T;

#define IS_LCDC_DIR(DIR) ((DIR) == LCDC_TX_MODE) || ((DIR) == LCDC_RX_MODE)

/** End of LCDC_Operate_Direction
  * \}
  */

/**
 * \defgroup    LCDC_Interrupt_Mask_Definition LCDC Interrupt Mask Definition
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
#define LCDC_INT_MASK_MULTI_BLOCK_START                 BIT31
#define LCDC_INT_MASK_WAVEFORM_FINISH                   BIT30
#define LCDC_INT_MASK_TEAR_TTRIGGER                     BIT11
#define LCDC_INT_MASK_RX_AUTO_DONE                      BIT10
#define LCDC_INT_MASK_RX_FIFO_OVERFLOW                  BIT9
#define LCDC_INT_MASK_TX_AUTO_DONE                      BIT8
#define LCDC_INT_MASK_TX_FIFO_EMPTY                     BIT7
#define LCDC_INT_MASK_TX_FIFO_OVERFLOW                  BIT6
#define LCDC_INT_MASK_TX_FIFO_THRESHOLDR                BIT5
#define LCDC_INT_MASK_TX_TRANSFER_ERROR                 BIT4

#define IS_LCDC_INT_MSK_CONFIG(MASK)   (((MASK) == LCDC_INT_MASK_TEAR_TTRIGGER)            || \
                                        ((MASK) == LCDC_INT_MASK_RX_AUTO_DONE)     || \
                                        ((MASK) == LCDC_INT_MASK_RX_FIFO_OVERFLOW)            || \
                                        ((MASK) == LCDC_INT_MASK_TX_AUTO_DONE)     || \
                                        ((MASK) == LCDC_INT_MASK_TX_FIFO_EMPTY)         || \
                                        ((MASK) == LCDC_INT_MASK_TX_FIFO_OVERFLOW)            || \
                                        ((MASK) == LCDC_INT_MASK_TX_FIFO_THRESHOLDR)        || \
                                        ((MASK) == LCDC_INT_MASK_TX_TRANSFER_ERROR))

/** End of LCDC_Interrupt_Mask_Definition
  * \}
  */

/**
 * \defgroup    LCDC_Interrupt_Status_Definition LCDC Interrupt Status Definition
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
#define LCDC_INT_STATUS_MULTIBLOCK_LASTBLOCK_START              BIT31
#define LCDC_INT_STATUS_WAVEFORM_FINISH                         BIT30
#define LCDC_INT_STATUS_TEAR_TRIGGER                            BIT20
#define LCDC_INT_STATUS_RX_FIFO_EMPTY                           BIT19
#define LCDC_INT_STATUS_RX_FIFO_FULL                            BIT18
#define LCDC_INT_RX_AUTO_DONE                                   BIT17
#define LCDC_INT_RX_FIFO_OVERFLOW                               BIT16
#define LCDC_INT_TX_AUTO_DONE                                   BIT6
#define LCDC_INT_STATUS_TX_FIFO_EMPTY                           BIT5
#define LCDC_INT_STATUS_TX_FIFO_FULL                            BIT4

#define IS_LCDC_INT_STATUS(STATUS)       (((STATUS) == LCDC_INT_STATUS_MULTIBLOCK_LASTBLOCK_START)         || \
                                          ((STATUS) == LCDC_INT_STATUS_WAVEFORM_FINISH)  || \
                                          ((STATUS) == LCDC_INT_STATUS_TEAR_TRIGGER)         || \
                                          ((STATUS) == LCDC_INT_STATUS_RX_FIFO_EMPTY)  || \
                                          ((STATUS) == LCDC_INT_STATUS_RX_FIFO_FULL)      || \
                                          ((STATUS) == LCDC_INT_RX_AUTO_DONE)         || \
                                          ((STATUS) == LCDC_INT_RX_FIFO_OVERFLOW)           || \
                                          ((STATUS) == LCDC_INT_TX_AUTO_DONE)          || \
                                          ((STATUS) == LCDC_INT_STATUS_TX_FIFO_EMPTY)  || \
                                          ((STATUS) == LCDC_INT_STATUS_TX_FIFO_FULL))

/** End of LCDC_Interrupt_Status_Definition
  * \}
  */

/**
 * \defgroup    LCDC_Interrupt_Flag LCDC Interrupt Flag
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
#define LCDC_INT_FLAG_STATUS_TX_FIFO_EMPTY                      BIT3
#define LCDC_INT_FLAG_STATUS_TX_FIFO_OVERFLOW                   BIT2
#define LCDC_INT_FLAG_STATUS_TX_FIFO_THESHOLD                   BIT1
#define LCDC_INT_FLAG_STATUS_TX_FIFO_TRANSFER_ERROR             BIT0

#define IS_LCDC_FLAG_STATUS(FLAG)       (((FLAG) == LCDC_INT_FLAG_STATUS_TX_FIFO_EMPTY)         || \
                                         ((FLAG) == LCDC_INT_FLAG_STATUS_TX_FIFO_OVERFLOW)  || \
                                         ((FLAG) == LCDC_INT_FLAG_STATUS_TX_FIFO_THESHOLD)         || \
                                         ((FLAG) == LCDC_INT_FLAG_STATUS_TX_FIFO_TRANSFER_ERROR))

/** End of LCDC_Interrupt_Flag
  * \}
  */

/**
 * \defgroup    LCDC_Interrupt_Clear_Status LCDC Interrupt Clear Status
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
#define LCDC_CLR_MULTIBLOCK_LASTBLOCK_START                     BIT31
#define LCDC_CLR_WAVEFORM_FINISH                                BIT30
#define LCDC_CLR_TEAR_TRIGGER                                   BIT10
#define LCDC_CLR_RX_AUTO_DONE                                   BIT9
#define LCDC_CLR_RX_FIFO_OVERFLOW                               BIT8
#define LCDC_CLR_RX_OUTPUT_CNT                                  BIT7
#define LCDC_CLR_TX_AUTO_DONE                                   BIT6
#define LCDC_CLR_TX_FIFO_EMPTY                                  BIT5
#define LCDC_CLR_TX_FIFO_OVERFLOW                               BIT4
#define LCDC_CLR_TX_FIFO_THESHOLD                               BIT3
#define LCDC_CLR_TX_OUTPUT_CNT                                  BIT2
#define LCDC_CLR_TX_FIFO_TRANSFER_ERROR                         BIT1
#define LCDC_CLR_FIFO                                           BIT0

#define IS_LCDC_INT_CLEAR(CLR)           (((CLR) == LCDC_CLR_MULTIBLOCK_LASTBLOCK_START) || \
                                          ((CLR) == LCDC_CLR_WAVEFORM_FINISH) || \
                                          ((CLR) == LCDC_CLR_TEAR_TRIGGER) || \
                                          ((CLR) == LCDC_CLR_RX_AUTO_DONE) || \
                                          ((CLR) == LCDC_CLR_RX_FIFO_OVERFLOW) || \
                                          ((CLR) == LCDC_CLR_RX_OUTPUT_CNT) || \
                                          ((CLR) == LCDC_CLR_TX_AUTO_DONE) || \
                                          ((CLR) == LCDC_CLR_TX_FIFO_EMPTY) || \
                                          ((CLR) == LCDC_CLR_TX_FIFO_OVERFLOW) || \
                                          ((CLR) == LCDC_CLR_TX_FIFO_THESHOLD) || \
                                          ((CLR) == LCDC_CLR_TX_OUTPUT_CNT) || \
                                          ((CLR) == LCDC_CLR_TX_FIFO_TRANSFER_ERROR) || \
                                          ((CLR) == LCDC_CLR_FIFO))

/** End of LCDC_Interrupt_Clear_Status
  * \}
  */

/**
 * \defgroup    LCDC_SPIC_RX_FIFO_READ_SIZE LCDC SPIC RX FIFO READ SIZE
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
typedef enum
{
    SPIC_RX_FIFO_READ_SIZE_BYTE,
    SPIC_RX_FIFO_READ_SIZE_HALFWORD,
    SPIC_RX_FIFO_READ_SIZE_WORD
} LCDC_SPIC_READ_SIZE_T;

#define IS_SPIC_RX_FIFO_READ_SIZE(SIZE) (((SIZE) == SPIC_RX_FIFO_READ_SIZE_BYTE) || \
                                         ((SIZE) == SPIC_RX_FIFO_READ_SIZE_HALFWORD) || \
                                         ((SIZE) == SPIC_RX_FIFO_READ_SIZE_WORD))

/** End of LCDC_SPIC_RX_FIFO_READ_SIZE
  * \}
  */

/**
 * \defgroup    LCDC_DMA_Multiblock_Mode LCDC DMA Multi-block Mode
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
#define AUTO_RELOAD_WITH_CONTIGUOUS_SAR     (BIT31)
#define AUTO_RELOAD_WITH_CONTIGUOUS_DAR     (BIT30)
#define AUTO_RELOAD_TRANSFER                (BIT30 | BIT31)
#define LLI_WITH_CONTIGUOUS_SAR             (BIT27)
#define LLI_WITH_AUTO_RELOAD_SAR            (BIT27 | BIT30)
#define LLI_WITH_CONTIGUOUS_DAR             (BIT28)
#define LLI_WITH_AUTO_RELOAD_DAR            (BIT28 | BIT31)
#define LLI_TRANSFER                        (BIT27 | BIT28)

#define IS_DMA_MULTIBLOCKMODE(MODE) (((MODE) == AUTO_RELOAD_WITH_CONTIGUOUS_SAR) || ((MODE) == AUTO_RELOAD_WITH_CONTIGUOUS_DAR)\
                                     ||((MODE) == AUTO_RELOAD_TRANSFER) || ((MODE) == LLI_WITH_CONTIGUOUS_SAR)\
                                     ||((MODE) == LLI_WITH_AUTO_RELOAD_SAR) || ((MODE) == LLI_WITH_CONTIGUOUS_DAR)\
                                     ||((MODE) == LLI_WITH_AUTO_RELOAD_DAR) || ((MODE) == LLI_TRANSFER))

/** End of LCDC_DMA_Multiblock_Mode
  * \}
  */

/**
 * \defgroup    LCDC_DMA_Multiblock_Select_Bit LCDC DMA Multi-Block Select Bit
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
#define LCDC_DMA_LINK_G1_WP         BIT31
#define LCDC_DMA_LINK_G2_WP         BIT30

/** End of LCDC_DMA_Multiblock_Select_Bit
  * \}
  */

/**
 * \defgroup    LCDC_AXIMUX_mode LCDC AXIMUX Mode
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
typedef enum
{
    LCDC_FW_MODE,
    LCDC_HW_MODE,
} LCDC_AXI_MUX_MODE_T;

/** End of LCDC_AXIMUX_mode
  * \}
  */

/** End of LCDC_Exported_Constants
  * \}
  */

/*============================================================================*
 *                         Types
 *============================================================================*/
/** \defgroup LCDC_Exported_Types LCDC Exported Types
  * \brief
  * \{
  */

typedef enum
{
    CKO1_PLL1_VCORE4 = 0,
    CKO1_PLL4_VCORE4 = 1
} CLK_DISPLAY_SRC_MUX0;

typedef enum
{
    CKO1_PLL2_VCORE4 = 0,
    FROM_CLK_DISPLAY_SRC_MUX0 = 1
} CLK_DISPLAY_SRC_MUX1;

typedef enum
{
    NO_CLOCK = 0,
    FRO_CLK_DISPLAY_SRC_MUX1 = 1
} CLK_DISPLAY_MUX_CG_EN;

typedef enum
{
    LCDC_DIV_DISABLE = 0,
    LCDC_DIV_ENABLE = 1
} LCDC_DIV_EN;

typedef enum
{
    LCDC_DIV_1_DIV = 0,
    LCDC_DIV_2_DIV = 1,
    LCDC_DIV_4_DIV = 2,
    LCDC_DIV_8_DIV = 3,
    LCDC_DIV_16_DIV = 4,
    LCDC_DIV_32_DIV = 5,
    LCDC_DIV_40_DIV = 6,
    LCDC_DIV_64_DIV = 7,
} LCDC_DIV_SEL;

/**
 * \brief       LCDC Init structure definition
 *
 * \ingroup     LCDC_Exported_Types
 */
typedef struct
{
    uint32_t LCDC_Interface;     /*!< Selecting the output interface of LCDC.
                                         This parameter can be a value of @ref LCDC_Interface */
    uint32_t LCDC_RamlessEn;     /*!< Enable or disable ramless qspi, only active when DBIC/SPIC interface is selected.
                                         This parameter can be a value of ENABLE or DISABLE */
    uint32_t LCDC_GroupSel;      /*!< selecting the output pin assignemnt groups.
                                         This parameter can be a value of 1 or 2 */
    uint32_t LCDC_PixelInputFarmat;    /*!< Configure the input pixel format of pixel converter.
                                              This parameter can be a value of @ref LCDC_Pixel_Input_Format */
    uint32_t LCDC_PixelOutpuFarmat;    /*!< Configure the output pixel format of pixel converter.
                                              This parameter can be a value of @ref LCDC_Pixel_Output_Format */
    uint32_t LCDC_PixelBitSwap;     /*!< Input data 8/16bits swap configuration of pixel converter.
                                           This parameter can be a value of @ref LCDC_Pixel_Bit_Swap */
    uint32_t LCDC_TeEn;             /*!< Enable or disable tear function.
                                           This parameter can be a value of ENABLE or DISABLE */
    uint32_t LCDC_TePolarity;       /*!< Configure the edge of tear triggering.
                                           This parameter can be a value of @ref LCDC_Tear_Polarity */
    uint32_t LCDC_TeInputMux;       /*!< Indicate where the tear input signal comes from.
                                           This parameter can be a value of @ref LCDC_Tear_Input_Mux */
    uint32_t LCDC_OperateMode;      /*!< Configure operate mode. */
    uint32_t LCDC_OperateDirection; /*!< Configure operate direction. */
    uint32_t LCDC_DmaThreshold;     /*!< Configure dma handler fifo threshold. */
    uint32_t LCDC_DmaIntMask;       /*!< Dma interrupt mask. */
    uint32_t LCDC_TxLen;            /*!< Tx length */
    uint32_t LCDC_InfiniteModeEn;   /*!< Enable or disable infinite mode. */
} LCDC_InitTypeDef;

/**
 * \brief       LCDC DMA LLI Init structure definition
 *
 * \ingroup     LCDC_Exported_Types
 */
typedef struct
{
    uint32_t g1_source_addr;      /*!< Group1 source address */
    uint32_t g2_source_addr;      /*!< Group2 source address */
    uint32_t g1_destination_addr; /*!< Group1 destination address */
    uint32_t g2_destination_addr; /*!< Group2 destination address */
    uint16_t g1_buffersize;       /*!< Group1 buffer size */
    uint16_t g2_buffersize;       /*!< Group2 buffer size */
    uint32_t g1_sar_offset;       /*!< Group1 SAR offset */
    uint32_t g2_sar_offset;       /*!< Group2 SAR offset */
    uint32_t g1_dar_offset;       /*!< Group1 DAR offset */
    uint32_t g2_dar_offset;       /*!< Group2 DAR offset */
    uint32_t g1_LLP;              /*!< Group1 LLP */
    uint32_t g2_LLP;              /*!< Group2 LLP */
} LCDC_DMALLI_InitTypeDef;

/** End of LCDC_Exported_Types
  * \}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** \defgroup LCDC_Exported_Functions LCDC Exported Functions
  * \brief
  * \{
  */

/**
 * rtl_lcdc.h
 *
 * \brief  Enable or disable LCD reset signal, which is active low.
 *
 * * \param[in] reset: New state of LCD reset signal.
 *            This parameter can be one of the following values:
 *            \arg true: Enable LCD reset signal.
 *            \arg false: Disable LCD reset signal.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_LCD_SET_RST(true);
 *     platform_delay_ms(120);
 *     LCDC_LCD_SET_RST(true);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_LCD_SET_RST(bool reset)
{
    LCDC_HANDLER_INTERFACE_SELECT_TypeDef handler_reg_0x00 = {.d32 = LCDC_HANDLER->INTERFACE_SELECT};
    if (reset)
    {
        handler_reg_0x00.b.reset_lcd_display_module = 0;
    }
    else
    {
        handler_reg_0x00.b.reset_lcd_display_module = 1;
    }
    LCDC_HANDLER->INTERFACE_SELECT = handler_reg_0x00.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Enable or disable LCDC.
 *
 * \param[in] NewState: New state of LCDC.
 *            This parameter can be one of the following values:
 *            \arg ENABLE: Enable LCDC.
 *            \arg DISABLE: Disable LCDC.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_Cmd(ENABLE);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_Cmd(FunctionalState NewState)
{
    LCDC_HANDLER_INTERFACE_SELECT_TypeDef handler_reg_0x00 = {.d32 = LCDC_HANDLER->INTERFACE_SELECT};
    if (NewState)
    {
        handler_reg_0x00.b.display_controller_enable = 1;
    }
    else
    {
        handler_reg_0x00.b.display_controller_enable = 0;
    }
    LCDC_HANDLER->INTERFACE_SELECT = handler_reg_0x00.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Selecting the output interface of LCDC.
 *
 * \param[in] InterFace: The output interface of LCDC.
 *            This parameter can be one of the following values:
 *            \arg LCDC_IF_eDPI: eDPI interface.
 *            \arg LCDC_IF_DBIB: DBIB interface.
 *            \arg LCDC_IF_DBIC: DBIC/SPIC interface.
 *            \arg LCDC_IF_DPI: DPI interface.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_SwitchIF(LCDC_IF_DBIC);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_SwitchIF(uint32_t InterFace)
{
    assert_param(IS_LCDC_IF_SEL(InterFace));

    LCDC_HANDLER_INTERFACE_SELECT_TypeDef handler_reg_0x00 = {.d32 = LCDC_HANDLER->INTERFACE_SELECT};
    handler_reg_0x00.b.interface_select = InterFace;
    LCDC_HANDLER->INTERFACE_SELECT = handler_reg_0x00.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Switch LCDC to required mode.
 *
 * \param[in] mode: LCDC work mode.
 *            This parameter can be one of the following values:
 *            \arg LCDC_AUTO_MODE: Auto mode.
 *            \arg LCDC_MANUAL_MODE: Manual mode.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_SwitchMode(LCDC_AUTO_MODE);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_SwitchMode(uint32_t mode)
{
    assert_param(IS_LCDC_MODE(mode));
    LCDC_HANDLER_OPERATE_CTR_TypeDef handler_reg_0x14 = {.d32 = LCDC_HANDLER->OPERATE_CTR};
    handler_reg_0x14.b.access_mode = mode;
    LCDC_HANDLER->OPERATE_CTR = handler_reg_0x14.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Swtich LCDC function to write or read.
 *
 * \param[in] dir: The transmission mode of LCDC.
 *            This parameter can be one of the following values:
 *            \arg LCDC_TX_MODE: Write pixel data from FIFO to pannel.
 *            \arg LCDC_RX_MODE: Read back data from pannel.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_SwitchDirect(LCDC_TX_MODE);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_SwitchDirect(uint32_t dir)
{
    assert_param(IS_LCDC_DIR(dir));
    LCDC_HANDLER_OPERATE_CTR_TypeDef handler_reg_0x14 = {.d32 = LCDC_HANDLER->OPERATE_CTR};
    handler_reg_0x14.b.data_rw = dir;
    LCDC_HANDLER->OPERATE_CTR = handler_reg_0x14.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Start or stop DMA.
 *
 * \param[in] NewState: New state of LCDC DMA.
 *            This parameter can be one of the following values:
 *            \arg ENABLE: Enable DMA handshake and internal FIFO control.
 *            \arg DISABLE: Disable DMA handshake and internal FIFO control.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_DmaCmd(ENABLE);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_DmaCmd(FunctionalState NewState)
{
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    LCDC_HANDLER_DMA_FIFO_CTRL_TypeDef handler_reg_0x18 = {.d32 = LCDC_HANDLER->DMA_FIFO_CTRL};
    if (NewState == ENABLE)
    {
        handler_reg_0x18.b.dma_enable = 1;
    }
    else
    {
        handler_reg_0x18.b.dma_enable = 0;
    }
    LCDC_HANDLER->DMA_FIFO_CTRL = handler_reg_0x18.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Enable or disable the function of automatically writing data when the valid edge of the tear signal arrives.
 *
 * \param[in] NewState: Enable or disable the function of automatically writing data when the valid edge of the tear signal arrives.
 *            This parameter can be one of the following values:
 *            \arg ENABLE: Enable the function of automatically writing data when the valid edge of the tear signal arrives.
 *            \arg DISABLE: Disable the function of automatically writing data when the valid edge of the tear signal arrives.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_TeCmd(ENABLE);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_TeCmd(FunctionalState NewState)
{
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    LCDC_HANDLER_TEAR_CTR_TypeDef handler_reg_0x10 = {.d32 = LCDC_HANDLER->TEAR_CTR};
    if (NewState == ENABLE)
    {
        handler_reg_0x10.b.tear_auto_turn_on_autowritestart = 1;
    }
    else
    {
        handler_reg_0x10.b.tear_auto_turn_on_autowritestart = 0;
    }
    LCDC_HANDLER->TEAR_CTR = handler_reg_0x10.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Start or stop write data in auto mode.
 *
 * \param[in] NewState: Start or stop write data.
 *            This parameter can be one of the following values:
 *            \arg ENABLE: Start write data in auto mode.
 *            \arg DISABLE: Stop write data in auto mode.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_AutoWriteCmd(ENABLE);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_AutoWriteCmd(FunctionalState NewState)
{
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    LCDC_HANDLER_OPERATE_CTR_TypeDef handler_reg_0x14 = {.d32 = LCDC_HANDLER->OPERATE_CTR};
    if (NewState == ENABLE)
    {
        handler_reg_0x14.b.auto_write_start = 1;
    }
    else
    {
        handler_reg_0x14.b.auto_write_start = 0;
    }
    LCDC_HANDLER->OPERATE_CTR = handler_reg_0x14.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Start or stop read data in auto mode, only used in DBIB interface.
 *
 * \param[in] NewState: Start or stop read data.
 *            This parameter can be one of the following values:
 *            \arg ENABLE: Start read data in auto mode.
 *            \arg DISABLE: Stop read data in auto mode.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_AutoReadCmd(ENABLE);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_AutoReadCmd(FunctionalState NewState)
{
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    LCDC_HANDLER_OPERATE_CTR_TypeDef handler_reg_0x14 = {.d32 = LCDC_HANDLER->OPERATE_CTR};
    if (NewState == ENABLE)
    {
        handler_reg_0x14.b.auto_read_start = 1;
    }
    else
    {
        handler_reg_0x14.b.auto_read_start = 0;
    }
    LCDC_HANDLER->OPERATE_CTR = handler_reg_0x14.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Write data to DMA FIFO.
 *
 * \param[in] data: Data to be writen.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_WriteFIFO(0xFF00FF00);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_WriteFIFO(uint32_t data)
{
    LCDC_HANDLER->DMA_FIFO = data;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Read data from DMA FIFO.
 *
 * \param None.
 *
 * \return Data read from FIFO.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     uint32_t value = LCDC_ReadFIFO();
 * }
 * \endcode
 */
__STATIC_INLINE uint32_t LCDC_ReadFIFO(void)
{
    return LCDC_HANDLER->DMA_FIFO;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Clear DMA FIFO.
 *
 * \param None.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_ClearDmaFifo();
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_ClearDmaFifo(void)
{
    LCDC_HANDLER_DMA_FIFO_ICR_TypeDef handler_reg_0x24 = {.d32 = LCDC_HANDLER->DMA_FIFO_ICR};
    handler_reg_0x24.b.fifo_clr = 1;
    LCDC_HANDLER->DMA_FIFO_ICR = handler_reg_0x24.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Clear pending bit of specified interrupt.
 *
 * \param[in] LCDC_CLEAR_INT: Specified interrupt type.
 *            This parameter can be one of the following values:
 *            \arg LCDC_CLR_TX_FIFO_THESHOLD: When FIFO offset smaller than threshold.
 *            \arg LCDC_CLR_TX_FIFO_OVERFLOW: TX FIFO overflow.
 *            \arg LCDC_CLR_TX_FIFO_EMPTY: TX FIFO is empty.
 *            \arg LCDC_CLR_TX_AUTO_DONE: Auto transmit done.
 *            \arg LCDC_CLR_RX_FIFO_OVERFLOW: RX FIFO overflow, only in DBIB interface.
 *            \arg LCDC_CLR_RX_AUTO_DONE: Auto receive done, only in DBIB interface.
 *            \arg LCDC_CLR_TEAR_TRIGGER: Tear signal trigger.
 *            \arg LCDC_CLR_WAVEFORM_FINISH: Waveform finish.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_ClearINTPendingBit(LCDC_CLR_TX_AUTO_DONE);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_ClearINTPendingBit(uint32_t LCDC_CLEAR_INT)
{
    /* Check the parameters */
    assert_param(IS_LCDC_INT_CLEAR(LCDC_CLEAR_INT));

    //LCDC_HANDLER->DMA_FIFO_ICR &= LCDC_CLEAR_INT;
    LCDC_HANDLER->DMA_FIFO_ICR = LCDC_CLEAR_INT;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Set the number of pixel needed to be writen in auto mode.
 *
 * \param[in] len: The number of pixel needed to be writen in auto mode.
 *
 * \return The number of pixel needed to be writen in auto mode.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_SetTxPixelLen(100);
 * }
 * \endcode
 */
__STATIC_INLINE uint32_t LCDC_SetTxPixelLen(uint32_t len)
{
    return LCDC_HANDLER->TX_LEN = len;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Get amount of pixels that have been transmitted.
 *
 * \param None.
 *
 * \return The amount of pixels that have been transmitted.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     uint32_t pixel_num = LCDC_GetTxPixelCnt();
 * }
 * \endcode
 */
__STATIC_INLINE uint32_t LCDC_GetTxPixelCnt(void)
{
    return LCDC_HANDLER->TX_CNT;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Clear transmitted pixel counter.
 *
 * \param None.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_ClearTxPixelCnt();
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_ClearTxPixelCnt(void)
{
    LCDC_HANDLER_DMA_FIFO_ICR_TypeDef handler_reg_0x24 = {.d32 = LCDC_HANDLER->DMA_FIFO_ICR};
    handler_reg_0x24.b.tx_output_cnt_clr = 1;
    LCDC_HANDLER->DMA_FIFO_ICR = handler_reg_0x24.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Set the number of byte needed to be read in auto mode, only used in DBIB interface.
 *
 * \param[in] len: The number of byte needed to be read in auto mode.
 *
 * \return The number of byte needed to be read in auto mode.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_SetRxByteLen(10);
 * }
 * \endcode
 */
__STATIC_INLINE uint32_t LCDC_SetRxByteLen(uint32_t len)
{
    return LCDC_HANDLER->RX_LEN = len;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Get the number of byte to be read in auto mode.
 *
 * \param None.
 *
 * \return The number of byte to be read in auto mode.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     uint32_t num = LCDC_GetRxByteCnt();
 * }
 * \endcode
 */
__STATIC_INLINE uint32_t LCDC_GetRxByteCnt(void)
{
    return LCDC_HANDLER->RX_LEN;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Get amount of bytes that have been received.
 *
 * \param None.
 *
 * \return The amount of bytes that have been received.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     uint32_t num = LCDC_GetRxCounter();
 * }
 * \endcode
 */
__STATIC_INLINE uint32_t LCDC_GetRxCounter(void)
{
    return LCDC_HANDLER->RX_CNT;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Clear recived byte counter.
 *
 * \param None.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_ClearRxCounter();
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_ClearRxCounter(void)
{
    LCDC_HANDLER_DMA_FIFO_ICR_TypeDef handler_reg_0x24 = {.d32 = LCDC_HANDLER->DMA_FIFO_ICR};
    handler_reg_0x24.b.rx_output_cnt_clr = 1;
    LCDC_HANDLER->DMA_FIFO_ICR = handler_reg_0x24.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Swtich LCDC function to firmware mode or hardware mode.
 *
 * \param[in] mode: LCDC mode.
 *            This parameter can be one of the following values:
 *            \arg LCDC_FW_MODE: Firmware mode. AXI MUX is used to set the register of SPIC through APB interface.
 *            \arg LCDC_HW_MODE: Hardware mode. AXI MUX is used to tranfer data packet from handler to SPIC FIFO(CMD+ADDR+pixel data), user can not access the register of SPIC.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_AXIMUXMode(LCDC_FW_MODE);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_AXIMUXMode(uint32_t mode)
{
    assert_param(IS_LCDC_AXI_MUX_MODE(mode));
    LCDC_HANDLER_AXI_MUX_MODE_TypeDef handler_reg_0x40 = {.d32 = LCDC_HANDLER->AXI_MUX_MODE};
    handler_reg_0x40.b.axi_mux_mode = mode;
    LCDC_HANDLER->AXI_MUX_MODE = handler_reg_0x40.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Set command in SPIC communication.
 *
 * \param[in] cmd: Value of command.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_SPICCmd(0xD8);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_SPICCmd(uint8_t cmd)
{
    LCDC_HANDLER_SPIC_CMD_TypeDef handler_reg_0x44 = {.d32 = LCDC_HANDLER->SPIC_CMD};
    handler_reg_0x44.b.spic_cmd = cmd;
    LCDC_HANDLER->SPIC_CMD = handler_reg_0x44.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Set address in SPIC communication.
 *
 * \param[in] addr: Value of address.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_SPICAddr(0x00DE00);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_SPICAddr(uint32_t addr)
{
    LCDC_HANDLER_SPIC_ADDR_TypeDef handler_reg_0x44 = {.d32 = 0};
    handler_reg_0x44.b.spic_addr = addr;
    LCDC_HANDLER->SPIC_ADDR = handler_reg_0x44.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Get data size of receiving.
 *
 * \param None.
 *
 * \return The data size of receiving.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     uint32_t data_size = LCDC_SPICRXFIFOReadSize();
 * }
 * \endcode
 */
__STATIC_INLINE uint32_t LCDC_SPICRXFIFOReadSize(void)
{
    LCDC_HANDLER_SPIC_RX_FIFO_READ_SIZE_TypeDef handler_reg_0x68 = {.d32 = LCDC_HANDLER->SPIC_RX_FIFO_READ_SIZE};
    return handler_reg_0x68.b.spic_rx_fifo_read_size;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Get count of DMA load configuration times.
 *
 * \param None.
 *
 * \return Times of DMA load settings from G1 and G2.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     uint32_t cnt = LCDC_DMA_LOAD_CNT(true);
 * }
 * \endcode
 */
__STATIC_INLINE uint32_t LCDC_DMA_LOAD_CNT(void)
{
    LCDC_HANDLER_DMA_MULTI_BLOCK_CTRL_TypeDef handler_reg_0x4c = {.d32 = LCDC_HANDLER->DMA_MULTI_BLOCK_CTRL};
    return (handler_reg_0x4c.b.dma_load_cnt);
}

/**
 * rtl_lcdc.h
 *
 * \brief  Configure DMA multi-block mode.
 *
 * \param[in] NewState: New state of DMA multi-block mode.
 *            This parameter can be one of the following values:
 *            \arg ENABLE: Enable DMA multi-block mode.
 *            \arg DISABLE: Disable DMA multi-block mode.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_DMA_MultiBlockCmd(ENABLE);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_DMA_MultiBlockCmd(FunctionalState NewState)
{
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    LCDC_HANDLER_DMA_MULTI_BLOCK_CTRL_TypeDef handler_reg_0x4c = {.d32 = LCDC_HANDLER->DMA_MULTI_BLOCK_CTRL};
    if (NewState == ENABLE)
    {
        handler_reg_0x4c.b.dma_multi_block_en = 1;
    }
    else
    {
        handler_reg_0x4c.b.dma_multi_block_en = 0;
    }
    LCDC_HANDLER->DMA_MULTI_BLOCK_CTRL = handler_reg_0x4c.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Configure DMA linklist.
 *
 * \param[in] NewState: New state of DMA linklist.
 *            This parameter can be one of the following values:
 *            \arg ENABLE: Enable DMA linklist.
 *            \arg DISABLE: Disable DMA linklist.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_DMA_LinkListCmd(ENABLE);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_DMA_LinkListCmd(FunctionalState NewState)
{
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    LCDC_HANDLER_DMA_MULTI_BLOCK_CTRL_TypeDef handler_reg_0x4c = {.d32 = LCDC_HANDLER->DMA_MULTI_BLOCK_CTRL};
    if (NewState == ENABLE)
    {
        handler_reg_0x4c.b.link_list_enable = 1;
    }
    else
    {
        handler_reg_0x4c.b.link_list_enable = 0;
    }
    LCDC_HANDLER->DMA_MULTI_BLOCK_CTRL = handler_reg_0x4c.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Clear DMA load counter.
 *
 * \param None.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_DMA_LOAD_CNT_CLR();
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_DMA_LOAD_CNT_CLR(void)
{
    LCDC_HANDLER_DMA_MULTI_BLOCK_CTRL_TypeDef handler_reg_0x4c = {.d32 = LCDC_HANDLER->DMA_MULTI_BLOCK_CTRL};
    handler_reg_0x4c.b.reg_dma_load_cnt_clr = 1;
    LCDC_HANDLER->DMA_MULTI_BLOCK_CTRL = handler_reg_0x4c.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Set block size of group 1.
 *
 * \param[in] size: Block size in bytes.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_SET_GROUP1_BLOCKSIZE(400);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_SET_GROUP1_BLOCKSIZE(uint32_t size)
{
    LCDC_HANDLER_DMA_MULTI_BLOCK_SIZE1_TypeDef handler_reg_0x50 = {.d32 = LCDC_HANDLER->DMA_MULTI_BLOCK_SIZE1};
    handler_reg_0x50.b.block_size_group_1 = size;
    LCDC_HANDLER->DMA_MULTI_BLOCK_SIZE1 = handler_reg_0x50.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Set block size of group 2.
 *
 * \param[in] size: Block size in bytes.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_SET_GROUP2_BLOCKSIZE(400);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_SET_GROUP2_BLOCKSIZE(uint32_t size)
{
    LCDC_HANDLER_DMA_MULTI_BLOCK_SIZE2_TypeDef handler_reg_0x54 = {.d32 = LCDC_HANDLER->DMA_MULTI_BLOCK_SIZE2};
    handler_reg_0x54.b.block_size_group_2 = size;
    LCDC_HANDLER->DMA_MULTI_BLOCK_SIZE2 = handler_reg_0x54.d32;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Set source address for group 1 and group 2 in infinite mode.
 *
 * \param[in] addr1: Source address for group 1.
 * \param[in] addr2: Source address for group 2.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_SET_INFINITE_ADDR(0x4002000, 0x4002000 + 0x2000);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_SET_INFINITE_ADDR(uint32_t addr1, uint32_t addr2)
{
    LCDC_DMA_LINKLIST->GRP1_SAR_FOR_INFINITE_MODE = addr1;
    LCDC_DMA_LINKLIST->GRP2_SAR_FOR_INFINITE_MODE = addr2;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Set source address for input DMA channel.
 *
 * \param[in] LCDC_DMA_Channelx: Pointer to required DMA channel.
 * \param[in] Address: Source address of DMA.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_DMA_SetSourceAddress(LCDC_DMA_Channel0, 0x8000000);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_DMA_SetSourceAddress(LCDC_DMA_ChannelTypeDef *LCDC_DMA_Channelx,
                                               uint32_t Address)
{
    /* Check the parameters */
    assert_param(IS_DMA_ALL_PERIPH(DMA_Channelx));

    LCDC_DMA_Channelx->LCDC_DMA_SARx = Address;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Set buffer size for input DMA channel.
 *
 * \param[in] LCDC_DMA_Channelx: Pointer to required DMA channel.
 * \param[in] buffer_size: The amount of data to be transmitted.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_DMA_SetBufferSize (LCDC_DMA_Channel0, 5000);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_DMA_SetBufferSize(LCDC_DMA_ChannelTypeDef *LCDC_DMA_Channelx,
                                            uint32_t buffer_size)
{
    /* Check the parameters */
    assert_param(IS_DMA_ALL_PERIPH(DMA_Channelx));

    /* configure high 32 bit of CTL register */
    LCDC_DMA_Channelx->LCDC_DMA_CTL_HIGHx = buffer_size;
}

/**
 * rtl_lcdc.h
 *
 * \brief  Initializes LCDC according to the specified parameters in the LCDC_Init.
 *
 * \param[in] LCDC_Init: Pointer to a LCDC_InitTypeDef structure that contains the configuration information for LCDC.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_InitTypeDef lcdc_init = {0};
 *     lcdc_init.LCDC_Interface = LCDC_IF_DBIC;
 *     lcdc_init.LCDC_PixelInputFarmat = LCDC_INPUT_RGB565;
 *     lcdc_init.LCDC_PixelOutpuFarmat = LCDC_OUTPUT_RGB565;
 *     lcdc_init.LCDC_PixelBitSwap = LCDC_SWAP_BYPASS;
 *     lcdc_init.LCDC_GroupSel = 1;
 *     lcdc_init.LCDC_DmaThreshold = 112;
 *     lcdc_init.LCDC_TeEn = ENABLE;
 *     lcdc_init.LCDC_TePolarity = LCDC_TE_EDGE_FALLING;
 *     lcdc_init.LCDC_TeInputMux = LCDC_TE_LCD_INPUT;
 *     LCDC_Init(&lcdc_init);
 * }
 * \endcode
 */
void LCDC_Init(LCDC_InitTypeDef *LCDC_Init);

/**
 * rtl_lcdc.h
 *
 * \brief  Configure linklist of LCDC DMA.
 *
 * \param[in] LCDC_DMA_LLIConfig: linklist initialization structure.
 * \param[in] LCDC_DMA_Init: LCDC DMA initialization structure.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_DMA_InitTypeDef LCDC_DMA_InitStruct = {0};
 *     LCDC_DMA_StructInit(&LCDC_DMA_InitStruct);
 *     LCDC_DMA_InitStruct.LCDC_DMA_ChannelNum = 0;
 *     LCDC_DMA_InitStruct.LCDC_DMA_DIR  = LCDC_DMA_DIR_PeripheralToMemory;
 *     LCDC_DMA_InitStruct.LCDC_DMA_SourceInc           = LCDC_DMA_SourceInc_Inc;
 *     LCDC_DMA_InitStruct.LCDC_DMA_DestinationInc      = LCDC_DMA_DestinationInc_Fix;
 *     LCDC_DMA_InitStruct.LCDC_DMA_SourceDataSize      = LCDC_DMA_DataSize_Word;
 *     LCDC_DMA_InitStruct.LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Word;
 *     LCDC_DMA_InitStruct.LCDC_DMA_SourceMsize         = LCDC_DMA_Msize_16;
 *     LCDC_DMA_InitStruct.LCDC_DMA_DestinationMsize    = LCDC_DMA_Msize_16;
 *     LCDC_DMA_InitStruct.LCDC_DMA_SourceAddr          = 0;
 *     LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_Mode   = LLI_TRANSFER;
 *     LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_En     = ENABLE;
 *     LCDC_DMA_InitStruct.LCDC_DMA_Multi_Block_Struct = LCDC_DMA_LINKLIST_REG_BASE + 0x50;
 *     LCDC_DMA_Init(LCDC_DMA_CHANNEL_INDEX, &LCDC_DMA_InitStruct);
 *     LCDC_SET_GROUP1_BLOCKSIZE(ST77903_WIDTH * PIXEL_BYTES);
 *     LCDC_SET_GROUP2_BLOCKSIZE(ST77903_WIDTH * PIXEL_BYTES);
 *     LCDC_DMALLI_InitTypeDef LCDC_DMA_LLI_Init = {0};
 *     LCDC_DMA_LLI_Init.g1_source_addr = (uint32_t)buf;
 *     LCDC_DMA_LLI_Init.g2_source_addr = (uint32_t)((uint32_t)buf + ST77903_WIDTH * PIXEL_BYTES);
 *     LCDC_DMA_LLI_Init.g1_sar_offset = ST77903_WIDTH * PIXEL_BYTES * 2;
 *     LCDC_DMA_LLI_Init.g2_sar_offset = ST77903_WIDTH * PIXEL_BYTES * 2;
 *     LCDC_SET_INFINITE_ADDR((uint32_t)buf, (uint32_t)buf + ST77903_WIDTH * PIXEL_BYTES);
 *     LCDC_DMA_LinkList_Init(&LCDC_DMA_LLI_Init, &LCDC_DMA_InitStruct);
 * }
 * \endcode
 */
void LCDC_DMA_LinkList_Init(LCDC_DMALLI_InitTypeDef *LCDC_DMA_LLIConfig, void *LCDC_DMA_Init);

/**
 * rtl_lcdc.h
 *
 * \brief  Enable or disable LCDC clock.
 *
 * \param[in] state: New state of LCDC clock.
 *            This parameter can be one of the following values:
 *            \arg ENABLE: Enable LCDC clock.
 *            \arg DISABLE: Disable LCDC clock.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_Clock_Cfg(ENABLE);
 * }
 * \endcode
 */
void LCDC_Clock_Cfg(FunctionalState state);
/**
 * rtl_lcdc.h
 *
 * \brief  Force DMA to transmit data in a burst, when data left is less than burst size.
 *
 * \param[in] new_state: Force DMA to transmit data in a burst.
 *            This parameter can be one of the following values:
 *            \arg ENABLE: Enable the function that force DMA to transmit data in a burst.
 *            \arg DISABLE: Disable the function that force DMA to transmit data in a burst.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_ForceBurst(ENABLE);
 * }
 * \endcode
 */
void LCDC_ForceBurst(FunctionalState new_state);

/** End of LCDC_Exported_Functions
  * \}
  */

/** End of LCDC
  * \}
  */

#ifdef __cplusplus
}
#endif

#endif /*__RTL_LCDC_H*/


/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/

