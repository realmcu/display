/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* \file     rtl_lcdc.h
* \brief    This file provides all the LCDC HANDLER firmware functions.
* \details  The LCD Controller (LCDC) module provides interfaces to connect with various LCD panels.
*           It supports multiple interfaces including eDPI, DBI-B, DBI-C/SPI, and DPI.
*           This driver provides APIs for configuring and controlling the LCDC module.
*
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
#include "rtl_lcdc_dma.h"
#include "rtl_lcdc_handler_def.h"
#include "rtl_lcdc_dma_ll_def.h"

/*============================================================================*
 *                          Private Macros
 *============================================================================*/
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
  * \brief    LCD Controller driver module
  * \{
  */

/** \defgroup LCDC_BASE        LCDC Base
  * \brief    LCD Controller driver Basic Funtion
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
    LCDC_IF_eDPI = 0x0,                     /*!< Enhanced Display Pixel Interface, need MIPI DSI host */
    LCDC_IF_DBIB = 0x1,                     /*!< Display Bus Interface Type-B */
    LCDC_IF_DBIC = 0x2,                     /*!< Serial Display Interface */
    LCDC_IF_DPI  = 0x3                      /*!< Display Pixel Interface */
} LCDC_IF_T;

#define IS_LCDC_IF_SEL(IF) (((IF) == LCDC_IF_eDPI) || ((IF) == LCDC_IF_DBIB) || \
                            ((IF) == LCDC_IF_DBIC) || ((IF) == LCDC_IF_DPI))

/** End of LCDC_Interface
  * \}
  */

/**
 * \cond        private
 * \brief       Rename DBIC to SDI in the future
 * \defgroup    LCDC_IF_SDI   SDI interface
 * \{
 */
#define LCDC_IF_SDI  LCDC_IF_DBIC
/**
 * \}
 * \endcond
 */

/**
 * \defgroup    LCDC_Pixel_Input_Format LCDC Pixel Input Format
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
typedef enum
{
    LCDC_INPUT_ARGB8888,               /*!< A(bit 31:24) R(bit 23:16) G(bit 15:8) B(bit 7:0). */
    LCDC_INPUT_BGR565,                 /*!< B(bit 15:11) G(bit 10:5) R(bit 4:0). */
    LCDC_INPUT_RGB565,                 /*!< R(bit 15:11) G(bit 10:5) B(bit 4:0). */
    LCDC_INPUT_RGB888,                 /*!< R(bit 23:16) G(bit 15:8) B(bit 7:0). */
    LCDC_INPUT_ABGR8888,               /*!< A(bit 31:24) B(bit 23:16) G(bit 15:8) R(bit 7:0). */
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
    LCDC_OUTPUT_RGB888,           /*!< R(bit 23:16) G(bit 15:8) B(bit 7:0). */
    LCDC_OUTPUT_RGB565,           /*!< R(bit 15:11) G(bit 10:5) B(bit 4:0). */
    LCDC_OUTPUT_BGR888,           /*!< B(bit 23:16) G(bit 15:8) R(bit 7:0). */
    LCDC_OUTPUT_BGR565,           /*!< B(bit 15:11) G(bit 10:5) R(bit 4:0). */
} LCDC_OUTPUT_FORMAT_T;

#define IS_LCDC_OUTPUT_FORMAT(FORMAT) (((FORMAT) == LCDC_OUTPUT_RGB888) || ((FORMAT) == LCDC_OUTPUT_RGB565) || ((FORMAT) == LCDC_OUTPUT_BGR565))

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
    LCDC_SWAP_BYPASS,          /*!< No swap operation is performed. */
    LCDC_SWAP_8BIT,            /*!< Each 4 bytes (byte3 byte2 byte1 byte0) is changed to (byte2 byte3 byte0 byte1). */
    LCDC_SWAP_16BIT,           /*!< Each 4 bytes (byte3 byte2 byte1 byte0) is changed to (byte1 byte0 byte3 byte2). */
    LCDC_SWAP_8BIT_16BIT,      /*!< Each 4 bytes (byte3 byte2 byte1 byte0) is changed to (byte0 byte1 byte2 byte3). */
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
    LCDC_TE_SIGNAL_VALID_LOW,     /*!< Tear signal is now low level. */
    LCDC_TE_SIGNAL_VALID_HIGH,    /*!< Tear signal is now high level. */
} LCDC_TE_SIGNAL_VALID_T;
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
    LCDC_TE_EDGE_RISING,      /*!< Tear signal triggers data transmission on rising edge. */
    LCDC_TE_EDGE_FALLING      /*!< Tear signal triggers data transmission on falling edge. */
} LCDC_TE_EDGE_T;

#define IS_LCDC_TE_POLARITY(POLARITY) (((POLARITY) == LCDC_TE_EDGE_RISING) || ((POLARITY) == LCDC_TE_EDGE_FALLING))

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
    LCDC_TE_LCD_INPUT,        /*!< Tear input signal is from LCD Panel Output. */
    LCDC_TE_DSIHOST_INPUT     /*!< Tear input signal is from DW DSI Host Controller(Only used in eDPI interface). */
} LCDC_INPUT_MUX_T;

#define IS_LCDC_TE_INPUTMUX(INPUT) (((INPUT) == LCDC_TE_LCD_INPUT) || ((INPUT) == LCDC_TE_DSIHOST_INPUT))

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
    LCDC_AUTO_MODE,      /*!< LCDC work in auto mode. */
    LCDC_MANUAL_MODE,    /*!< LCDC work in manual mode, only valid for DBI-B interface. */
} LCDC_MODE_T;

#define IS_LCDC_MODE(MODE) (((MODE) == LCDC_AUTO_MODE) || ((MODE) == LCDC_MANUAL_MODE))

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
    LCDC_TX_MODE,       /*!< LCDC in data transmission mode. */
    LCDC_RX_MODE,       /*!< LCDC in data receiving mode. */
} LCDC_DIRECTION_T;

#define IS_LCDC_DIR(DIR) (((DIR) == LCDC_TX_MODE) || ((DIR) == LCDC_RX_MODE))

/** End of LCDC_Operate_Direction
  * \}
  */

/**
 * \defgroup    LCDC_Clock_Phase_Shift LCDC Clock Phase Shift
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
typedef enum
{
    LCDC_SPI_PHASE_SHIFT_0_DEGREE = 0,      /*!< SPI clock no phase shift. */
    LCDC_SPI_PHASE_SHIFT_90_DEGREE = 2,     /*!< SPI clock phase shift 90 degrees. */
    LCDC_SPI_PHASE_SHIFT_180_DEGREE = 3,    /*!< SPI clock phase shift 180 degrees. */
} LCDC_CLK_PHASE_SHIFT_T;

/** End of LCDC_Clock_Phase_Shift
  * \}
  */

/**
 * \defgroup    LCDC_Interrupt_Status_Definition LCDC Interrupt Status Definition
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
#define LCDC_STATUS_WAVEFORM_FINISH_INT           BIT30 /*!< In DBI-B & SDI interface, the status will be set when CS signal return to inactive level. In eDPI interface, this status will be set when data transmission has been finished and stop state of PHY has been set. */
#define LCDC_STATUS_TEAR_TRIGGER_INT              BIT20 /*!< Tear interrupt status is triggered. */
#define LCDC_STATUS_RX_FIFO_EMPTY                 BIT19 /*!< RX_FIFO offset is 0 and only valid in DBI-B auto receiving. */
#define LCDC_STATUS_RX_FIFO_FULL                  BIT18 /*!< RX_FIFO offset is full and only valid in DBI-B auto receiving. */
#define LCDC_STATUS_RX_AUTO_DONE_INT              BIT17 /*!< RX auto done interrupt is triggered, only valid in DBI-B auto receiving. */
#define LCDC_STATUS_RX_FIFO_OVERFLOW_INT          BIT16 /*!< RX-FIFO has overflowed, only valid in DBI-B auto receiving. */
#define LCDC_STATUS_TX_AUTO_DONE_INT              BIT6  /*!< When output pixel count grows no less than output pixel number. In eDPI mode, it is raised when hole frame has been tranmitted. */
#define LCDC_STATUS_TX_FIFO_EMPTY                 BIT5  /*!< TX_FIFO offset is 0. */
#define LCDC_STATUS_TX_FIFO_FULL                  BIT4  /*!< TX_FIFO is full. */
#define LCDC_STATUS_TX_FIFO_EMPTY_INT             BIT3  /*!< Triggered when TX_FIFO offset is 0. */
#define LCDC_STATUS_TX_FIFO_OVERFLOW_INT          BIT2  /*!< Triggered when TX_FIFO overflows. */
#define LCDC_STATUS_TX_FIFO_THRESHOLD_INT          BIT1  /*!< Triggered when TX_FIFO offset is less than threshold. */

#define IS_LCDC_STATUS(STATUS) (((STATUS) == LCDC_STATUS_WAVEFORM_FINISH_INT) || \
                                ((STATUS) == LCDC_STATUS_TEAR_TRIGGER_INT) || \
                                ((STATUS) == LCDC_STATUS_RX_FIFO_EMPTY) || \
                                ((STATUS) == LCDC_STATUS_RX_FIFO_FULL) || \
                                ((STATUS) == LCDC_STATUS_RX_AUTO_DONE_INT) || \
                                ((STATUS) == LCDC_STATUS_RX_FIFO_OVERFLOW_INT) || \
                                ((STATUS) == LCDC_STATUS_TX_AUTO_DONE_INT) || \
                                ((STATUS) == LCDC_STATUS_TX_FIFO_EMPTY) || \
                                ((STATUS) == LCDC_STATUS_TX_FIFO_FULL) || \
                                ((STATUS) == LCDC_STATUS_TX_FIFO_EMPTY_INT) || \
                                ((STATUS) == LCDC_STATUS_TX_FIFO_OVERFLOW_INT) || \
                                ((STATUS) == LCDC_STATUS_TX_FIFO_THRESHOLD_INT))
/** End of LCDC_Interrupt_Status_Definition
  * \}
  */

/**
 * \defgroup    LCDC_Interrupt_Mask_Definition LCDC Interrupt Mask Definition
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
#define LCDC_INT_MASK_WAVEFORM_FINISH                   BIT30   /*!< Mask interrupt \ref LCDC_STATUS_WAVEFORM_FINISH_INT .*/
#define LCDC_INT_MASK_TEAR_TTRIGGER                     BIT11   /*!< Mask interrupt \ref LCDC_STATUS_TEAR_TRIGGER_INT .*/
#define LCDC_INT_MASK_RX_AUTO_DONE                      BIT10   /*!< Mask interrupt \ref LCDC_STATUS_RX_AUTO_DONE_INT .*/
#define LCDC_INT_MASK_RX_FIFO_OVERFLOW                  BIT9    /*!< Mask interrupt \ref LCDC_STATUS_RX_FIFO_OVERFLOW_INT .*/
#define LCDC_INT_MASK_TX_AUTO_DONE                      BIT8    /*!< Mask interrupt \ref LCDC_STATUS_TX_AUTO_DONE_INT .*/
#define LCDC_INT_MASK_TX_FIFO_EMPTY                     BIT7    /*!< Mask interrupt \ref LCDC_STATUS_TX_FIFO_EMPTY_INT .*/
#define LCDC_INT_MASK_TX_FIFO_OVERFLOW                  BIT6    /*!< Mask interrupt \ref LCDC_STATUS_TX_FIFO_OVERFLOW_INT .*/
#define LCDC_INT_MASK_TX_FIFO_THRESHOLD                 BIT5    /*!< Mask interrupt \ref LCDC_STATUS_TX_FIFO_THRESHOLD_INT .*/

#define IS_LCDC_INT_MSK_CONFIG(MASK)   (((MASK) == LCDC_INT_MASK_TEAR_TTRIGGER)            || \
                                        ((MASK) == LCDC_INT_MASK_WAVEFORM_FINISH)   ||\
                                        ((MASK) == LCDC_INT_MASK_RX_AUTO_DONE)     || \
                                        ((MASK) == LCDC_INT_MASK_RX_FIFO_OVERFLOW)            || \
                                        ((MASK) == LCDC_INT_MASK_TX_AUTO_DONE)     || \
                                        ((MASK) == LCDC_INT_MASK_TX_FIFO_EMPTY)         || \
                                        ((MASK) == LCDC_INT_MASK_TX_FIFO_OVERFLOW)            || \
                                        ((MASK) == LCDC_INT_MASK_TX_FIFO_THRESHOLD))

/** End of LCDC_Interrupt_Mask_Definition
  * \}
  */

/**
 * \defgroup    LCDC_Interrupt_Clear_Status LCDC Interrupt Clear Status
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
#define LCDC_CLR_WAVEFORM_FINISH                                BIT30 /*!< Clear interrupt state \ref LCDC_STATUS_WAVEFORM_FINISH_INT.*/
#define LCDC_CLR_TEAR_TRIGGER                                   BIT10 /*!< Clear interrupt state \ref LCDC_STATUS_TEAR_TRIGGER_INT .*/
#define LCDC_CLR_RX_AUTO_DONE                                   BIT9  /*!< Clear interrupt state \ref LCDC_STATUS_RX_AUTO_DONE_INT .*/
#define LCDC_CLR_RX_FIFO_OVERFLOW                               BIT8  /*!< Clear interrupt state \ref LCDC_STATUS_RX_FIFO_OVERFLOW_INT .*/
#define LCDC_CLR_RX_OUTPUT_CNT                                  BIT7  /*!< Clear RX output counter. */
#define LCDC_CLR_TX_AUTO_DONE                                   BIT6  /*!< Clear interrupt state \ref LCDC_STATUS_TX_AUTO_DONE_INT .*/
#define LCDC_CLR_TX_FIFO_EMPTY                                  BIT5  /*!< Clear interrupt state \ref LCDC_STATUS_TX_FIFO_EMPTY_INT .*/
#define LCDC_CLR_TX_FIFO_OVERFLOW                               BIT4  /*!< Clear interrupt state \ref LCDC_STATUS_TX_FIFO_OVERFLOW_INT .*/
#define LCDC_CLR_TX_FIFO_THRESHOLD                              BIT3  /*!< Clear interrupt state \ref LCDC_STATUS_TX_FIFO_THRESHOLD_INT .*/
#define LCDC_CLR_TX_OUTPUT_CNT                                  BIT2  /*!< Clear TX output counter. */
#define LCDC_CLR_FIFO                                           BIT0  /*!< Forcefully clear the FIFOs. */

#define IS_LCDC_INT_CLEAR(CLR)           (((CLR) == LCDC_CLR_WAVEFORM_FINISH) || \
                                          ((CLR) == LCDC_CLR_TEAR_TRIGGER) || \
                                          ((CLR) == LCDC_CLR_RX_AUTO_DONE) || \
                                          ((CLR) == LCDC_CLR_RX_FIFO_OVERFLOW) || \
                                          ((CLR) == LCDC_CLR_RX_OUTPUT_CNT) || \
                                          ((CLR) == LCDC_CLR_TX_AUTO_DONE) || \
                                          ((CLR) == LCDC_CLR_TX_FIFO_EMPTY) || \
                                          ((CLR) == LCDC_CLR_TX_FIFO_OVERFLOW) || \
                                          ((CLR) == LCDC_CLR_TX_FIFO_THRESHOLD) || \
                                          ((CLR) == LCDC_CLR_TX_OUTPUT_CNT) || \
                                          ((CLR) == LCDC_CLR_FIFO))

/** End of LCDC_Interrupt_Clear_Status
  * \}
  */

/**
 * \defgroup    LCDC_SPIC_RX_FIFO_READ_SIZE LCDC SPIC RX FIFO Read Size
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
typedef enum
{
    SPIC_RX_FIFO_READ_SIZE_BYTE,      /*!< Data size is 1 byte when reading from SDI RX FIFO. */
    SPIC_RX_FIFO_READ_SIZE_HALFWORD,  /*!< Data size is 1 half-word when reading from SDI RX FIFO. */
    SPIC_RX_FIFO_READ_SIZE_WORD       /*!< Data size is 1 word when reading from SDI RX FIFO. */
} LCDC_SPIC_READ_SIZE_T;

#define IS_SPIC_RX_FIFO_READ_SIZE(SIZE) (((SIZE) == SPIC_RX_FIFO_READ_SIZE_BYTE) || \
                                         ((SIZE) == SPIC_RX_FIFO_READ_SIZE_HALFWORD) || \
                                         ((SIZE) == SPIC_RX_FIFO_READ_SIZE_WORD))

/** End of LCDC_SPIC_RX_FIFO_READ_SIZE
  * \}
  */

/**
 * \defgroup    LCDC_AXIMUX_mode LCDC AXIMUX Mode
 * \{
 * \ingroup     LCDC_Exported_Constants
 */
typedef enum
{
    LCDC_FW_MODE,     /*!< AXI MUX is used to set the register of SDI through APB interface. */
    LCDC_HW_MODE,     /*!< AXI MUX is used to tranfer data packet from handler to SDI FIFO, user can not access the register of SDI. */
} LCDC_AXI_MUX_MODE_T;

#define IS_LCDC_AXI_MUX_MODE(Mode) (((Mode) == LCDC_FW_MODE) || ((Mode) == LCDC_HW_MODE))
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
    uint32_t LCDC_PixelInputFormat;    /*!< Configure the input pixel format of pixel converter.
                                              This parameter can be a value of @ref LCDC_Pixel_Input_Format */
    uint32_t LCDC_PixelOutputFormat;    /*!< Configure the output pixel format of pixel converter.
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
    uint32_t LCDC_PhaseShift;       /*!< Phase shift in DBIC mode. */
    uint32_t LCDC_DelayCell;        /*!< Select delay cell after phase shift in DBIC mode. */
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

#ifndef __STATIC_INLINE
#if   defined ( __CC_ARM )
#define __STATIC_INLINE  static __inline
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#define __STATIC_INLINE  static __inline
#elif defined ( __GNUC__ )
#define __STATIC_INLINE  static inline
#else
#error Unknown compiler
#endif
#endif
/*============================================================================*
 *                         Functions
 *============================================================================*/
/** \defgroup LCDC_Exported_Functions LCDC Exported Functions
  * \brief
  * \{
  */

/**
 * \brief  Enable or disable LCD reset signal, which is active low.
 *
 * \param[in] reset: New state of LCD reset signal.
 *            This parameter can be one of the following values:
 *            - true:   Enable LCD reset signal.
 *            - false:  Disable LCD reset signal.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_LCD_SET_RST(true);
 *     platform_delay_ms(120);
 *     LCDC_LCD_SET_RST(false);
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
 * \brief  Enable or disable LCDC.
 *
 * \param[in] NewState: New state of LCDC.
 *            This parameter can be one of the following values:
 *            - ENABLE:   Enable LCDC.
 *            - DISABLE:  Disable LCDC.
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
 * \brief  Selecting the output interface of LCDC.
 *
 * \param[in] InterFace: The output interface of LCDC.
 *            This parameter can be one of the following values:
 *            - LCDC_IF_eDPI:  eDPI interface.
 *            - LCDC_IF_DBIB:  DBIB interface.
 *            - LCDC_IF_DBIC:  DBIC/SPIC interface.
 *            - LCDC_IF_DPI:   DPI interface.
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
__STATIC_INLINE void LCDC_SwitchIF(LCDC_IF_T InterFace)
{
    assert_param(IS_LCDC_IF_SEL(InterFace));

    LCDC_HANDLER_INTERFACE_SELECT_TypeDef handler_reg_0x00 = {.d32 = LCDC_HANDLER->INTERFACE_SELECT};
    handler_reg_0x00.b.interface_select = InterFace;
    LCDC_HANDLER->INTERFACE_SELECT = handler_reg_0x00.d32;
}

/**
 * \brief  Switch LCDC to required mode.
 *
 * \param[in] mode: LCDC work mode.
 *            This parameter can be one of the following values:
 *            - LCDC_AUTO_MODE:    Auto mode.
 *            - LCDC_MANUAL_MODE:  Manual mode.
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
__STATIC_INLINE void LCDC_SwitchMode(LCDC_MODE_T Mode)
{
    assert_param(IS_LCDC_MODE(Mode));
    LCDC_HANDLER_OPERATE_CTR_TypeDef handler_reg_0x14 = {.d32 = LCDC_HANDLER->OPERATE_CTR};
    handler_reg_0x14.b.access_mode = Mode;
    LCDC_HANDLER->OPERATE_CTR = handler_reg_0x14.d32;
}

/**
 * \brief  Swtich LCDC function to write or read.
 *
 * \param[in] dir: The transmission mode of LCDC.
 *            This parameter can be one of the following values:
 *            - LCDC_TX_MODE:  Write pixel data from FIFO to pannel.
 *            - LCDC_RX_MODE:  Read back data from pannel.
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
__STATIC_INLINE void LCDC_SwitchDirect(uint32_t Dir)
{
    assert_param(IS_LCDC_DIR(Dir));
    LCDC_HANDLER_OPERATE_CTR_TypeDef handler_reg_0x14 = {.d32 = LCDC_HANDLER->OPERATE_CTR};
    handler_reg_0x14.b.data_rw = Dir;
    LCDC_HANDLER->OPERATE_CTR = handler_reg_0x14.d32;
}

/**
 * \brief  Start or stop DMA.
 *
 * \param[in] NewState: New state of LCDC DMA.
 *            This parameter can be one of the following values:
 *            - ENABLE:   Enable DMA handshake and internal FIFO control.
 *            - DISABLE:  Disable DMA handshake and internal FIFO control.
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
 * \brief  Enable or disable the function of automatically writing data when the valid edge of the tear signal arrives.
 *
 * \param[in] NewState: Enable or disable the function of automatically writing data when the valid edge of the tear signal arrives.
 *            This parameter can be one of the following values:
 *            - ENABLE:   Enable the function of automatically writing data when the valid edge of the tear signal arrives.
 *            - DISABLE:  Disable the function of automatically writing data when the valid edge of the tear signal arrives.
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
        handler_reg_0x10.b.tear_auto_turn_on_dma_en = 0;
    }
    else
    {
        handler_reg_0x10.b.tear_auto_turn_on_autowritestart = 0;
    }
    LCDC_HANDLER->TEAR_CTR = handler_reg_0x10.d32;
}

/**
 * \brief  Enable or disable the function of automatically turn on DMA when the valid edge of the tear signal arrives.
 *
 * \param[in] NewState: Enable or disable the function of automatically turning on DMA when the valid edge of the tear signal arrives.
 *            This parameter can be one of the following values:
 *            - ENABLE:   Enable the function of automatically turning on DMA when the valid edge of the tear signal arrives.
 *            - DISABLE:  Disable the function of automatically turning on DMA when the valid edge of the tear signal arrives.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_TeEnableDMA(ENABLE);
 * }
 * \endcode
 */
__STATIC_INLINE void LCDC_TeEnableDMA(FunctionalState NewState)
{
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    LCDC_HANDLER_TEAR_CTR_TypeDef handler_reg_0x10 = {.d32 = LCDC_HANDLER->TEAR_CTR};
    if (NewState == ENABLE)
    {
        handler_reg_0x10.b.tear_auto_turn_on_dma_en = 1;
    }
    else
    {
        handler_reg_0x10.b.tear_auto_turn_on_dma_en = 0;
    }
    LCDC_HANDLER->TEAR_CTR = handler_reg_0x10.d32;
}

/**
 * \brief  Start or stop write data in auto mode.
 *
 * \param[in] NewState: Start or stop write data.
 *            This parameter can be one of the following values:
 *            - ENABLE:   Start write data in auto mode.
 *            - DISABLE:  Stop write data in auto mode.
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
 * \brief  Start or stop read data in auto mode, only used in DBIB interface.
 *
 * \param[in] NewState: Start or stop read data.
 *            This parameter can be one of the following values:
 *            - ENABLE:   Start read data in auto mode.
 *            - DISABLE:  Stop read data in auto mode.
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
 * \brief  Write data to DMA FIFO.
 *
 * \param[in] data: Data to be writen.
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
 * \brief  Read data from DMA FIFO.
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
 * \brief  Clear DMA FIFO.
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
 * \brief  Clear pending bit of specified interrupt.
 *
 * \param[in] LCDC_CLEAR_INT: Specified interrupt type.
 *            This parameter can be one of the following values:
 *            - LCDC_CLR_TX_FIFO_THESHOLD:   When FIFO offset smaller than threshold.
 *            - LCDC_CLR_TX_FIFO_OVERFLOW:   TX FIFO overflow.
 *            - LCDC_CLR_TX_FIFO_EMPTY:      TX FIFO is empty.
 *            - LCDC_CLR_TX_AUTO_DONE:       Auto transmit done.
 *            - LCDC_CLR_RX_FIFO_OVERFLOW:   RX FIFO overflow, only in DBIB interface.
 *            - LCDC_CLR_RX_AUTO_DONE:       Auto receive done, only in DBIB interface.
 *            - LCDC_CLR_TEAR_TRIGGER:       Tear signal trigger.
 *            - LCDC_CLR_WAVEFORM_FINISH:    Waveform finish.
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
 * \brief  Get amount of pixels that have been transmitted.
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
 * \brief  Clear transmitted pixel counter.
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
 * \brief  Get the number of byte to be read in auto mode.
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
 * \brief  Get amount of bytes that have been received.
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
 * \brief  Clear recived byte counter.
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
 * \brief  Swtich LCDC function to firmware mode or hardware mode.
 *
 * \param[in] mode: LCDC mode.
 *            This parameter can be one of the following values:
 *            - LCDC_FW_MODE:  Firmware mode. Refer to \ref LCDC_FW_MODE .
 *            - LCDC_HW_MODE:  Hardware mode. Refer to \ref LCDC_HW_MODE .
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
__STATIC_INLINE void LCDC_AXIMUXMode(LCDC_AXI_MUX_MODE_T Mode)
{
    assert_param(IS_LCDC_AXI_MUX_MODE(Mode));
    LCDC_HANDLER_AXI_MUX_MODE_TypeDef handler_reg_0x40 = {.d32 = LCDC_HANDLER->AXI_MUX_MODE};
    handler_reg_0x40.b.axi_mux_mode = Mode;
    LCDC_HANDLER->AXI_MUX_MODE = handler_reg_0x40.d32;
}

/**
 * \brief  Set command in SPIC communication.
 *
 * \param[in] cmd: Value of command.
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
 * \brief  Set address in SPIC communication.
 *
 * \param[in] addr: Value of address.
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
 * \brief  Get data size of receiving.
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
 * \brief  Get count of DMA load configuration times.
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
 * \brief  Configure DMA multi-block mode.
 *
 * \param[in] NewState: New state of DMA multi-block mode.
 *            This parameter can be one of the following values:
 *            - ENABLE:   Enable DMA multi-block mode.
 *            - DISABLE:  Disable DMA multi-block mode.
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
 * \brief  Configure DMA linklist.
 *
 * \param[in] NewState: New state of DMA linklist.
 *            This parameter can be one of the following values:
 *            - ENABLE:   Enable DMA linklist.
 *            - DISABLE:  Disable DMA linklist.
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
 * \brief  Clear DMA load counter.
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
 * \brief  Set block size of group 1.
 *
 * \param[in] size: Block size in bytes.
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
 * \brief  Set block size of group 2.
 *
 * \param[in] size: Block size in bytes.
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
 * \brief  Set source address for group 1 and group 2 in infinite mode.
 *
 * \param[in] addr1: Source address for group 1.
 * \param[in] addr2: Source address for group 2.
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
 * \brief  Set source address for input DMA channel.
 *
 * \param[in] LCDC_DMA_Channelx: Pointer to required DMA channel.
 * \param[in] Address: Source address of DMA.
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
 * \brief  Set buffer size for input DMA channel.
 *
 * \param[in] LCDC_DMA_Channelx: Pointer to required DMA channel.
 * \param[in] buffer_size: The amount of data to be transmitted.
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
 * \brief  Initialize LCDC structure with default values
 * \param  LCDC_InitStruct : Pointer to LCDC_InitTypeDef structure
 *
 * \note   Default values are shown in the table below:
 * | Parameter Name         | Default Value                      | Description                        |
 * |------------------------|------------------------------------|------------------------------------|
 * | LCDC_Interface         | \ref LCDC_IF_DPI                   | Interface type set to DPI          |
 * | LCDC_RamlessEn         | DISABLE                            | Ramless function disabled          |
 * | LCDC_GroupSel          | 1                                  | Pin Group 1 selected               |
 * | LCDC_PixelInputFarmat  | \ref LCDC_INPUT_RGB888             | Input format set to RGB888         |
 * | LCDC_PixelOutpuFarmat  | \ref LCDC_OUTPUT_RGB888            | Output format set to RGB888        |
 * | LCDC_PixelBitSwap      | \ref LCDC_SWAP_BYPASS              | No bit swapping                    |
 * | LCDC_TeEn              | DISABLE                            | Tear Effect function disabled      |
 * | LCDC_TePolarity        | \ref LCDC_TE_EDGE_RISING           | Tear signal rising edge trigger    |
 * | LCDC_TeInputMux        | \ref LCDC_TE_LCD_INPUT             | Tear signal from LCD panel         |
 * | LCDC_DmaThreshold      | 8                                  | DMA threshold set to 8             |
 * | LCDC_InfiniteModeEn    | DISABLE                            | Infinite mode disabled             |
 * | LCDC_PhaseShift        | \ref LCDC_SPI_PHASE_SHIFT_0_DEGREE | No SPI clock phase shift           |
 * | LCDC_DelayCell         | 0                                  | No delay cells                     |
 *
 * Example:
 * \code{.c}
 * // Declare and initialize LCDC configuration structure
 * LCDC_InitTypeDef LCDC_InitStruct;
 * LCDC_StructInit(&LCDC_InitStruct);
 *
 * // Modify default configuration as needed
 * LCDC_InitStruct.LCDC_Interface = LCDC_IF_DBIC;        // Use DBIC interface
 * LCDC_InitStruct.LCDC_PixelInputFarmat = LCDC_INPUT_RGB565;  // Set input format to RGB565
 * LCDC_InitStruct.LCDC_DmaThreshold = 16;              // Modify DMA threshold
 *
 * // Initialize LCDC with modified configuration
 * LCDC_Init(&LCDC_InitStruct);
 * \endcode
 */
void LCDC_StructInit(LCDC_InitTypeDef *LCDC_InitStruct);

/**
 * \brief  Initializes LCDC according to the specified parameters in the LCDC_Init.
 *
 * \param[in] LCDC_InitStructure: Pointer to a LCDC_InitTypeDef structure that contains the configuration information for LCDC.
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
void LCDC_Init(LCDC_InitTypeDef *LCDC_InitStructure);

/**
 * \brief  Configure linklist of LCDC DMA.
 *
 * \param[in] LCDC_DMA_LLIConfig: linklist initialization structure.
 * \param[in] LCDC_DMA_Init: LCDC DMA initialization structure.
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
void LCDC_DMA_LinkList_Init(LCDC_DMALLI_InitTypeDef *LCDC_DMA_LLIConfig,
                            LCDC_DMA_InitTypeDef *LCDC_DMA_Init);

/**
 * \brief  Force DMA use burst data transfer.
 *
 * \param[in] state: DMA burst fixed or not.
 *            This parameter can be one of the following values:
 *            - ENABLE:   Force DMA use burst data transfer.
 *            - DISABLE:  DMA burst is not required.
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

/**
 * \brief  Enable or disable LCDC clock.
 *
 * \param[in] state: New state of LCDC clock.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable LCDC clock.
 *            - DISABLE: Disable LCDC clock.
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
 * \brief  Update source address for group 1 and group 2 in infinite mode.
 *
 * \param[in] G1_SAR_buf: Source address for group 1.
 * \param[in] G2_SAR_buf: Source address for group 2.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     LCDC_DMA_Infinite_Buf_Update(0x4002000, 0x4002000 + 0x2000);
 * }
 * \endcode
 */
void LCDC_DMA_Infinite_Buf_Update(uint8_t *G1_SAR_buf, uint8_t *G2_SAR_buf);

/** End of LCDC_Exported_Functions
  * \}
  */

/** End of LCDC_BASE
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

