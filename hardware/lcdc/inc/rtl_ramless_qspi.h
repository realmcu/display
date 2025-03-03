/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* \file     rtl_ramless_qspi.h
* \brief    This file provides all the LCDC Ramless QSPI firmware functions.
* \details
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
  * \brief
  * \{
  */

/** \defgroup 87X2G_RAMLESS_QSPI  RAMLESS_QSPI
  * \brief
  * \{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/
/** \defgroup RAMLESS_QSPI_Exported_Types RAMLESS_QSPI Exported Types
  * \brief
  * \{
  */

/**
 * \brief       SYNC_CMD_TYPE SYNC CMD TYPE
 *
 * \ingroup     RAMLESS_QSPI_Exported_Types
 */
typedef enum
{
    VSYNC_CMD,
    VSYNC_CMD_ADDR,
    HSYNC_CMD_VBP,
    HSYNC_CMD_VBP_ADDR,
    HSYNC_CMD_VACTIVE,
    HSYNC_CMD_VACTIVE_ADDR,
    HSYNC_CMD_VFP,
    HSYNC_CMD_VFP_ADDR,
} LCDC_Ramless_QSPI_Reg_Enum;

typedef struct
{
    uint32_t VSA;       /* vertical synchronization active height (in units of horizontal scan line)*/
    uint32_t VBP;     /* accumulated Vertical back porch (in units of horizontal scan line) */
    uint32_t VFP;
    uint32_t tear_input_mux;
    uint32_t VSYNC_CMD;
    uint32_t VSYNC_CMD_ADDR;
    uint32_t HSYNC_CMD_VBP;
    uint32_t HSYNC_CMD_VBP_ADDR;
    uint32_t HSYNC_CMD_VACTIVE;
    uint32_t HSYNC_CMD_VACTIVE_ADDR;
    uint32_t HSYNC_CMD_VFP;
    uint32_t HSYNC_CMD_VFP_ADDR;
    uint32_t DUMMY_CMD;
    uint32_t width;
    uint32_t height;
    uint32_t line_delay_in_vactive;
} LCDC_RLSPI_initTypeDef;

/** End of RAMLESS_QSPI_Exported_Types
  * \}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** \defgroup RAMLESS_QSPI_Exported_Functions RAMLESS_QSPI Exported Functions
  * \brief
  * \{
  */

/**
 * \brief   Configure command data for certian synchronize signal.
 *
 * \param[in] cmd: specified command type. \ref SYNC_CMD_TYPE.
 *
 * \param[in] value: data of command
 *
 * \return    None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void demo(void)
 * {
 *     RLSPI_SetCmd(VSYNC_CMD, 0x80DE00);
 * }
 * \endcode
 */
void RLSPI_SetCmd(LCDC_Ramless_QSPI_Reg_Enum cmd, uint32_t value);

/**
 * \brief   Fill the structure with default value.
 *
 * \param[in] obj: pointer to structure variable.
 *
 * \return    None.
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
void RLSPI_StructInit(LCDC_RLSPI_initTypeDef *obj);

/**
 * \brief   Initialize Ramless QSPI peripheral with value filled in input variable.
 *
 * \param[in] obj: pointer to structure variable.
 *
 * \return    None.
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
void RLSPI_Init(LCDC_RLSPI_initTypeDef *obj);

/**
 * \brief     Enable or disable Ramless QSPI peripheral.
 *
 * \param[in] value: This parameter enable or disable the the Ramless QSPI peripheral.
 *            This parameter can be the following:
 *            \arg DISABLE: Disable function.
 *            \arg ENABLE: Enable function.
 *
 * \return    None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void demo(void)
 * {
 *     RLSPI_Cmd(ENABLE);
 * }
 * \endcode
 */
void RLSPI_Cmd(FunctionalState state);

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
