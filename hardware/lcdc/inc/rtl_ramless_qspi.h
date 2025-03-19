/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* \file     rtl_ramless_qspi.h
* \brief    This file provides all the LCDC Ramless QSPI firmware functions.
* \details  The LCDC Ramless QSPI interface provides the following features:
*          - Supports QSPI interface for LCD panels without frame buffer
*          - Configurable vertical timing parameters (VSA, VBP, VFP)
*          - Configurable VSYNC/HSYNC commands and addresses for different display regions
*          - Adjustable line and frame delays for timing control
*          - Programmable display resolution settings
* \author   howie wang
* \date     2023-10-17
* \version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_RAMLESS_QSPI_H
#define RTL_RAMLESS_QSPI_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "rtl_lcdc.h"
#include "rtl_lcdc_rlspi_def.h"

/** \defgroup LCDC        LCDC
  * \brief    LCD Controller driver module
  * \{
  */

/** \defgroup 87X2G_RAMLESS_QSPI  LCDC Ramless QSPI
  * \brief    LCD Controller Ramless QSPI interface driver module
  * \{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/
/** \defgroup RAMLESS_QSPI_Exported_Types RAMLEQSPI Exported Types
  * \brief    Types provided by the Ramless QSPI driver module
  * \{
  */

typedef struct
{
    uint32_t VSA;       /*!< Vertical Synchronization Active height in units of horizontal scan lines. */
    uint32_t VBP;       /*!< Vertical Back Porch in units of horizontal scan lines. */
    uint32_t VFP;       /*!< Vertical Front Porch in units of horizontal scan lines. */
    uint32_t VSYNC_CMD;  /*!< VSYNC command for ramless LCD host controller of SPI interface in vertical sync active period. */
    uint32_t VSYNC_CMD_ADDR;  /*!< Address for VSYNC command in vertical sync active period. */
    uint32_t HSYNC_CMD_VBP;  /*!< HSYNC command for ramless LCD host controller of SPI interface in vertical back porch period. */
    uint32_t HSYNC_CMD_VBP_ADDR;  /*!< Address for HSYNC command in vertical back porch period. */
    uint32_t HSYNC_CMD_VACTIVE;  /*!< HSYNC command for ramless LCD host controller of SPI interface in vertical active period. */
    uint32_t HSYNC_CMD_VACTIVE_ADDR;  /*!< Address for the HSYNC command in active period. */
    uint32_t HSYNC_CMD_VFP;  /*!< HSYNC command byte for ramless LCD host controller of SPI interface in vertical front porch period. */
    uint32_t HSYNC_CMD_VFP_ADDR;  /*!< Address for the HSYNC command in vertical front porch period. */
    uint32_t DUMMY_CMD;  /*!< Dummy command for timing adjustment. */
    uint32_t width;  /*!< Display width in pixels. */
    uint32_t height;  /*!< Display height in pixels. */
    uint32_t line_delay_in_vactive;  /*!< Delay between each line during vertical active period measured in display clock. */
    uint32_t line_delay_out_vactive;  /*!< Delay between each line outside vertical active period measured in display clock. */
    uint32_t frame_delay;  /*!< Delay between frames in infinite mode measured in display clock. */
} LCDC_RLSPI_initTypeDef;

/** End of RAMLESS_QSPI_Exported_Types
  * \}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** \defgroup RAMLESS_QSPI_Exported_Functions RAMLESS_QSPI Exported Functions
  * \brief    Functions provided by the Ramless QSPI driver module
  * \{
  */

/**
* \brief   Initialize the LCDC_RLSPI_initTypeDef structure with default values.
*
* \param[in] RLSPI_InitStruct: Pointer to a LCDC_RLSPI_initTypeDef structure that will be initialized.
*
* \note Default values for each member:
*       | Member                    | Default Value | Description                                                  |
*       |---------------------------|---------------|--------------------------------------------------------------|
*       | VSA                       | 0             | Vertical synchronization active height                       |
*       | VBP                       | 0             | Vertical back porch                                          |
*       | VFP                       | 0             | Vertical front porch                                         |
*       | VSYNC_CMD                 | 0             | VSYNC command                                                |
*       | VSYNC_CMD_ADDR            | 0             | VSYNC command address                                        |
*       | HSYNC_CMD_VBP             | 0             | HSYNC command for VBP                                        |
*       | HSYNC_CMD_VBP_ADDR        | 0             | HSYNC command address for VBP                                |
*       | HSYNC_CMD_VACTIVE         | 0             | HSYNC command for active area                                |
*       | HSYNC_CMD_VACTIVE_ADDR    | 0             | HSYNC command address for active area                        |
*       | HSYNC_CMD_VFP             | 0             | HSYNC command for VFP                                        |
*       | HSYNC_CMD_VFP_ADDR        | 0             | HSYNC command address for VFP                                |
*       | DUMMY_CMD                 | 0             | Dummy command                                                |
*       | width                     | 0             | Display width                                                |
*       | height                    | 0             | Display height                                               |
*       | line_delay_in_vactive     | 0             | Line delay in vertical active period                         |
*       | line_delay_out_vactive     | 0            | Line delay outside vertical active period                    |
*       | frame_delay               | 0             | Frame delay                                                  |
*
* <b>Example usage</b>
* \code{.c}
*
* void demo(void)
* {
*     LCDC_RLSPI_initTypeDef init_struct;
*     RLSPI_StructInit(&init_struct);
* }
* \endcode
*/
void RLSPI_StructInit(LCDC_RLSPI_initTypeDef *RLSPI_InitStrut);

/**
 * \brief   Initialize Ramless QSPI peripheral with value filled in input variable.
 *
 * \param[in] RLSPI_InitStruct: Pointer to a LCDC_RLSPI_initTypeDef structure that contains the configuration information for Ramless QSPI.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void demo(void)
 * {
 *     LCDC_RLSPI_initTypeDef init_struct;
 *     RLSPI_StructInit(&init_struct);
 *     RLSPI_Init(&init_struct);
 * }
 * \endcode
 */
void RLSPI_Init(LCDC_RLSPI_initTypeDef *RLSPI_InitStruct);

/** End of RAMLESS_QSPI_Exported_Functions
  * \}
  */

/** End of RAMLESS_QSPI
  * \}
  */

/** End of LCDC
  * \}
  */

#ifdef __cplusplus
}
#endif

#endif /* RTL_RAMLESS_QSPI_H */

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/
