/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_JDI_H
#define RTL_JDI_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/

#include "rtl_jdi_reg.h"

/** \defgroup JDI       JDI
  * \brief    JDI (Japan Display Inc.) LCD Controller Interface
  * \{
  */
/*============================================================================*
 *                             Constants
 *============================================================================*/
/*!< JDI FIFO configuration parameters */
#define JDI_TX_FIFO_WIDTH       8    /*!< Width of TX FIFO in bits */
#define JDI_RX_FIFO_WIDTH       8    /*!< Width of RX FIFO in bits */
#define JDI_TX_FIFO_DEPTH       8    /*!< Depth of TX FIFO */
#define JDI_RX_FIFO_DEPTH       64   /*!< Depth of RX FIFO */
/** \defgroup JDI_Exported_Constants JDI Exported Constants
  * \brief    Constants and macros used by the JDI module
  * \{
  */

/** \defgroup JDI_Declaration JDI Declaration
  * \{
  * \ingroup  JDI_Exported_Constants
  */

#define JDI                           ((JDI_TypeDef *)(JDI_REG_BASE + 0x400))
/** End of JDI_Declaration
  * \}
  */

/**
 * \defgroup    JDI_INPUT_PIXEL_FORMAT JDI input pixel format
 * \{
 * \ingroup     JDI_Exported_Constants
 */
typedef enum
{
    JDI_IN_ARGB2222 = 0x00,      /*!< A(bit 7:6) R(bit 5:4) G(bit 3:2) B(bit 1:0). */
    JDI_IN_RGB222   = 0x01,      /*!< R1(bit 11:10) G1(bit 9:8) B1(bit 7:6) R0(bit 5:4) G0(bit 3:2) B0(bit 1:0).  */
    JDI_IN_RGB565   = 0x02,      /*!< R(bit 15:11) G(bit 10:5) B(bit 4:0). */
    JDI_IN_RGB888   = 0x03,      /*!< R(bit 23:16) G(bit 15:8) B(bit 7:0). */
} JDI_INPUT_PIXEL_FORMAT;

#define IS_JDI_INPUT_PIXEL_FORMAT(FORMAT)   (FORMAT == JDI_IN_ARGB2222 || FORMAT == JDI_IN_RGB222\
                                             || FORMAT == JDI_IN_RGB565 || FORMAT == JDI_IN_RGB888)
/** End of JDI_INPUT_PIXEL_FORMAT
  * \}
  */

/**
 * \defgroup    JDI_OUTPUT_PIXEL_FORMAT JDI output pixel format
 * \{
 * \ingroup     JDI_Exported_Constants
 */
typedef enum
{
    JDI_OUT_1BIT    = 0x00,      /*!< If any of the highest bits of any color channel of RGB input is 1, the output pixel is represented as 1; Otherwise, the output pixel is represented as 0. */
    JDI_OUT_3BIT    = 0x01,      /*!< R1(bit 5) G1(bit 4) B1(bit 3) R0(bit 2) G0(bit 1) B0(bit 0). */
    JDI_OUT_4BIT    = 0x02,      /*!< R0(bit 3) G0(bit 2) B0(bit 1) dummy(bit 0). */
} JDI_OUTPUT_PIXEL_FORMAT;

#define IS_JDI_OUT_PIXEL_FORMAT(FORMAT)   (FORMAT == JDI_OUT_1BIT || FORMAT == JDI_OUT_3BIT\
                                           || FORMAT == JDI_OUT_4BIT)
/** End of JDI_OUTPUT_PIXEL_FORMAT
  * \}
  */

/**
 * \defgroup    JDI_LINE_ADDRESS_INCR_TYPE JDI line address increase type
 * \{
 * \ingroup     JDI_Exported_Constants
 */
typedef enum
{
    JDI_LINE_ADDR_FIXED         = 0x00,      /*!< line address is fixed during the data transmission process. */
    JDI_LINE_ADDR_INCREASED     = 0x01,      /*!< line address is gradually increasing during the data transmission process. */
} JDI_LINE_ADDRESS_INCR_TYPE;

#define IS_JDI_LINE_ADDR_INCR_TYPE(TYPE)   (TYPE == JDI_LINE_ADDR_FIXED || TYPE == JDI_LINE_ADDR_INCREASED)
/** End of JDI_LINE_ADDRESS_INCR_TYPE
  * \}
  */

/**
 * \defgroup    JDI_INTERRUPT_TYPE Enumeration of All JDI Interrupts
 * \{
 * \ingroup     JDI_Exported_Constants
 */
typedef enum
{
    JDI_TX_FIFO_UNDERFLOW_INT   = 0x00,      /*!< Triggered when underflow in TX FIFO occurs. */
    JDI_TX_FIFO_THRESHOLD_INT   = 0x01,      /*!< Triggered when data level reaches TX FIFO threshold. */
    JDI_RX_FIFO_OVERFLOW_INT    = 0x02,      /*!< Triggered when overflow in RX FIFO occurs. */
    JDI_RX_FIFO_THRESHOLD_INT   = 0x03,      /*!< Triggered when data level is less than RX FIFO threshold. */
    JDI_OUTPUT_FINISH_INT       = 0x04,      /*!< Triggered when the output procedure is complete and all pixel data has been transferred to the TX FIFO. */
} JDI_INTERRUPT_TYPE;

#define IS_JDI_INT(INT)   (INT == JDI_TX_FIFO_UNDERFLOW_INT || INT == JDI_TX_FIFO_THRESHOLD_INT ||\
                           INT == JDI_RX_FIFO_OVERFLOW_INT || INT == JDI_RX_FIFO_THRESHOLD_INT ||\
                           INT == JDI_OUTPUT_FINISH_INT)
/** End of JDI_INTERRUPT_TYPE
  * \}
  */
/** End of JDI_Exported_Constants
  * \}
  */
/*============================================================================*
 *                             Types
 *============================================================================*/
/** \defgroup JDI_Exported_Types JDI Exported Types
  * \brief    Data structures and enumerations used by the JDI module
  * \{
  */

/**
 * \brief       JDI initialization structure definition.
 *
 * \ingroup     JDI_Exported_Types
 */
typedef struct
{
    JDI_INPUT_PIXEL_FORMAT JDI_Input_Pixel_Format;      /*!< Determine input pixel format sent to JDI.
                                                             This parameter can be a value of @ref JDI_INPUT_PIXEL_FORMAT */
    JDI_OUTPUT_PIXEL_FORMAT
    JDI_Output_Pixel_Format;    /*!< Determine output pixel format received from JDI.
                                                             This parameter can be a value of @ref JDI_OUTPUT_PIXEL_FORMAT */
    JDI_LINE_ADDRESS_INCR_TYPE
    JDI_Gate_Line_Address_Incr; /*!< Determine line address is increased for fixed.
                                                                This parameter can be a value of @ref JDI_LINE_ADDRESS_INCR_TYPE */
    uint32_t JDI_Gate_Line_Address_Incr_Num;            /*!< Line increase, should be less than 7. */
    uint32_t JDI_Mode_Select;                           /*!< Select JDI screen working mode, such as update and blink, referring to screen specification. */
    uint32_t JDI_Dummy_Bit_For_4bit_Output;             /*!< The value of top bit of 4-bit pixel output, can be 0 or 1. */
    uint32_t JDI_LCD_Width;                             /*!< Width of each line. */
    uint32_t JDI_LCD_Height;                            /*!< Height of LCD panel. */
    uint32_t JDI_Start_Gate_Line_Address;               /*!< Start line address for data transmission. */
    uint32_t JDI_Refresh_Line_Num;                      /*!< Number of lines to be refreshed. */
    uint32_t JDI_Dummy_Value_Mid;                       /*!< Value of 6-bit dummy data between lines. */
    uint32_t JDI_Dummy_Value_Tail;                      /*!< Value of 16-bit dummy data at the end of transmission. */
    uint32_t RX_DMA_Enable;                             /*!< Enable/Disable RX DMA. */
    uint32_t TX_DMA_Enable;                             /*!< Enable/Disable TX DMA. */
    uint32_t RX_FIFO_DMA_Threshold;                     /*!< RX FIFO threshold that triggers data receiving. */
    uint32_t TX_FIFO_DMA_Threshold;                     /*!< TX FIFO threshold that triggers data transmitting. */
    uint32_t RX_FIFO_Int_Threshold;                     /*!< RX FIFO threshold that triggers RX FIFO threshold interrupt. */
    uint32_t TX_FIFO_Int_Threshold;                     /*!< TX FIFO threshold that triggers TX FIFO threshold interrupt. */
} JDI_SrtuctInit_TypeDef;
/** End of JDI_Exported_Types
  * \}
  */
/*============================================================================*
 *                            Functions
 *============================================================================*/
/** \defgroup JDI_Exported_Functions JDI Exported Functions
  * \brief    API functions for controlling JDI LCD interface
  * \{
  */

/**
* \brief   Initialize JDI according to the specified parameters in JDI_InitStruct.
*
* \param[in] JDI_InitStruct: Pointer to a JDI_SrtuctInit_TypeDef structure which has been initialized.
*
* <b>Example usage</b>
* \code{.c}
*
* void driver_jdi_init(void)
* {
*     RCC_PeriphClockCmd(APBPeriph_SPI1, APBPeriph_SPI1_CLOCK, ENABLE);
*
*     JDI_SrtuctInit_TypeDef JDI_InitStruct;
*     JDI_Structure_Init(&JDI_InitStruct);
*     JDI_InitStruct.LCD_Width = 256;
*     JDI_InitStruct.LCD_Height = 200;
*     JDI_InitStruct.JDI_Dummy_Bit_For_4bit_Output = 0;
*     JDI_InitStruct.JDI_Dummy_Value_Mid = 0x0;
*     JDI_InitStruct.JDI_Dummy_Value_Tail = 0xFF;
*     JDI_InitStruct.JDI_Gate_Line_Address_Incr = JDI_LINE_ADDR_INCREASED;
*     JDI_InitStruct.JDI_Gate_Line_Address_Incr_Num = 1;
*     JDI_InitStruct.JDI_Input_Pixel_Format = JDI_IN_RGB565;
*     JDI_InitStruct.JDI_Output_Pixel_Format = JDI_OUT_3BIT;
*     JDI_InitStruct.JDI_Mode_Select = 0;
*     JDI_InitStruct.JDI_Start_Gate_Line_Address = 0;
*     JDI_InitStruct.RX_DMA_Enable = 1;
*     JDI_InitStruct.TX_DMA_Enable = 1;
*     JDI_InitStruct.JDI_Refresh_Line_Num = 200;
*     JDI_Init(&JDI_InitStruct);
* }
* \endcode
*/
void JDI_Init(JDI_SrtuctInit_TypeDef *JDI_InitStruct);

/**
 * \brief   Fill each JDI_InitStruct member with default value.
 *
 * \note The default settings for the JDI_InitStruct member are shown in the following table:
 *       | Member                          | Default Value                        |
 *       |:-------------------------------:|:------------------------------------:|
 *       | JDI_Input_Pixel_Format          | \ref JDI_IN_RGB565                   |
 *       | JDI_Output_Pixel_Format         | \ref JDI_OUT_3BIT                    |
 *       | JDI_Gate_Line_Address_Incr      | \ref JDI_LINE_ADDR_FIXED             |
 *       | JDI_Gate_Line_Address_Incr_Num  | 0                                    |
 *       | JDI_Mode_Select                 | 0                                    |
 *       | JDI_Dummy_Bit_For_4bit_Output   | 0                                    |
 *       | JDI_LCD_Width                   | 0                                    |
 *       | JDI_LCD_Height                  | 0                                    |
 *       | JDI_Start_Gate_Line_Address     | 0                                    |
 *       | JDI_Refresh_Line_Num            | 0                                    |
 *       | JDI_Dummy_Value_Mid             | 0                                    |
 *       | JDI_Dummy_Value_Tail            | 0                                    |
 *       | RX_DMA_Enable                   | 0                                    |
 *       | TX_DMA_Enable                   | 0                                    |
 *       | RX_FIFO_DMA_Threshold           | 32                                   |
 *       | TX_FIFO_DMA_Threshold           | 4                                    |
 *       | RX_FIFO_Int_Threshold           | 32                                   |
 *       | TX_FIFO_Int_Threshold           | 4                                    |
 *
 * \param[in] JDI_InitStruct: Pointer to a JDI_SrtuctInit_TypeDef structure which will be initialized.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_jdi_init(void)
 * {
 *     RCC_PeriphClockCmd(APBPeriph_SPI1, APBPeriph_SPI1_CLOCK, ENABLE);
 *
 *     JDI_SrtuctInit_TypeDef JDI_InitStruct;
 *     JDI_Structure_Init(&JDI_InitStruct);
 *     JDI_InitStruct.LCD_Width = 256;
 *     JDI_InitStruct.LCD_Height = 200;
 *     JDI_InitStruct.JDI_Gate_Line_Address_Incr = JDI_LINE_ADDR_INCREASED;
 *     JDI_InitStruct.JDI_Gate_Line_Address_Incr_Num = 1;
 *     JDI_InitStruct.JDI_Input_Pixel_Format = JDI_IN_RGB888;
 *     JDI_InitStruct.JDI_Output_Pixel_Format = JDI_OUT_1BIT;
 *     JDI_InitStruct.RX_DMA_Enable = 1;
 *     JDI_InitStruct.TX_DMA_Enable = 1;
 *     JDI_InitStruct.JDI_Refresh_Line_Num = 200;
 *     JDI_Init(&JDI_InitStruct);
 * }
 * \endcode
 */
void JDI_Structure_Init(JDI_SrtuctInit_TypeDef *JDI_InitStruct);

/**
 * \brief   Enable or disable the JDI peripheral.
 *
 * \param[in] NewState: New state of the JDI peripheral.
 *            This parameter can be: ENABLE or DISABLE.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_jdi_init(void)
 * {
 *     RCC_PeriphClockCmd(APBPeriph_SPI1, APBPeriph_SPI1_CLOCK, ENABLE);
 *
 *     JDI_SrtuctInit_TypeDef JDI_InitStruct;
 *     JDI_Structure_Init(&JDI_InitStruct);
 *     JDI_InitStruct.LCD_Width = 256;
 *     JDI_InitStruct.LCD_Height = 200;
 *     JDI_InitStruct.JDI_Gate_Line_Address_Incr = JDI_LINE_ADDR_INCREASED;
 *     JDI_InitStruct.JDI_Gate_Line_Address_Incr_Num = 1;
 *     JDI_InitStruct.JDI_Input_Pixel_Format = JDI_IN_RGB888;
 *     JDI_InitStruct.JDI_Output_Pixel_Format = JDI_OUT_1BIT;
 *     JDI_InitStruct.RX_DMA_Enable = 1;
 *     JDI_InitStruct.TX_DMA_Enable = 1;
 *     JDI_InitStruct.JDI_Refresh_Line_Num = 200;
 *     JDI_Init(&JDI_InitStruct);
 *     JDI_Cmd(ENABLE);
 * }
 * \endcode
 */
void JDI_Cmd(FunctionalState NewState);

/**
 * \brief   Wait till the peripheral is in idle state.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_jdi_deinit(void)
 * {
 *     JDI_Cmd(DISABLE);
 *     JDI_Wait_Idle();
 *     RCC_PeriphClockCmd(APBPeriph_SPI1, APBPeriph_SPI1_CLOCK, DISABLE);
 * }
 * \endcode
 */
void JDI_Wait_Idle(void);

/**
 * \brief   Enable or disable certain JDI interrupt.
 *
 * \param[in] Interrupt: Specified interrupt type from @ref JDI_INTERRUPT_TYPE
 * \param[in] NewState: New state of the JDI interrupt type.
 *            This parameter can be: ENABLE or DISABLE.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void jdi_interrupt_set(void)
 * {
 *     JDI_Interrupt_Enable(JDI_OUTPUT_FINISH_INT, ENABLE);
 *     JDI_Interrupt_Mask(JDI_OUTPUT_FINISH_INT, DISABLE);
 * }
 * \endcode
 */
void JDI_Interrupt_Enable(JDI_INTERRUPT_TYPE Interrupt, FunctionalState NewState);

/**
 * \brief   Mask or unmask certain JDI interrupt.
 *
 * \param[in] Interrupt: Specified interrupt type from @ref JDI_INTERRUPT_TYPE
 * \param[in] NewState: New state of the JDI interrupt type.
 *            This parameter can be: ENABLE or DISABLE.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void jdi_interrupt_mask(void)
 * {
 *     // Mask JDI_RX_FIFO_THRESHOLD_INT interrupt.
 *     JDI_Interrupt_Mask(JDI_RX_FIFO_THRESHOLD_INT, ENABLE);
 * }
 * \endcode
 */
void JDI_Interrupt_Mask(JDI_INTERRUPT_TYPE Interrupt, FunctionalState NewState);

/**
 * \brief   Get specified interrupt status.
 *
 * \param[in] Interrupt: Specified interrupt type from @ref JDI_INTERRUPT_TYPE
 *
 * \return  Status of input interrupt type.
 * \retval  - SET: The interrupt has been triggered and not masked.
 * \retval  - RESET: The interrupt has not been triggered yet or has been masked.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void jdi_interrupt_get(void)
 * {
 *     ITStatus status = JDI_Get_Interrupt_Status(JDI_RX_FIFO_THRESHOLD_INT);
 * }
 * \endcode
 */
ITStatus JDI_Get_Interrupt_Status(JDI_INTERRUPT_TYPE Interrupt);

/**
 * \brief   Get specified interrupt status before mask.
 *
 * \param[in] Interrupt: Specified interrupt type from @ref JDI_INTERRUPT_TYPE
 *
 * \return  Status of input interrupt type.
 * \retval  - SET: The interrupt has been triggered.
 * \retval  - RESET: The interrupt has not been triggered yet.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void jdi_interrupt_get_raw(void)
 * {
 *     ITStatus status = JDI_Get_Interrupt_Raw_Status(JDI_RX_FIFO_THRESHOLD_INT);
 * }
 * \endcode
 */
ITStatus JDI_Get_Interrupt_Raw_Status(JDI_INTERRUPT_TYPE Interrupt);

/**
 * \brief   Clear specified interrupt status.
 *
 * \param[in] Interrupt: Specified interrupt type from @ref JDI_INTERRUPT_TYPE
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void jdi_interrupt_clear(void)
 * {
 *     JDI_Interrupt_Clear(JDI_RX_FIFO_OVERFLOW_INT);
 * }
 * \endcode
 */
void JDI_Interrupt_Clear(JDI_INTERRUPT_TYPE Interrupt);

/** End of JDI_Exported_Functions
  * \}
  */

/** End of JDI
  * \}
  */



#ifdef __cplusplus
}
#endif

#endif /* RTL_JDI_H */
