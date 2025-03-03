/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* \file     rtl_lcdc_edpi.h
* \brief    This file provides all the eDPI firmware functions.
* \details
* \author   boris yue
* \date     2023-10-17
* \version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_EDPI_H
#define RTL_EDPI_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "rtl_lcdc.h"
#include "rtl_lcdc_edpi_def.h"

/** \defgroup LCDC        LCDC
  * \brief
  * \{
  */

/** \defgroup LCDC_EDPI        LCDC EDPI
  * \brief
  * \{
  */

/*============================================================================*
 *                         Types
 *============================================================================*/
/** \defgroup LCDC_EDPI_Exported_Types LCDC EDPI Exported Types
  * \brief
  * \{
  */

/**
 * \brief       LCDC EDPI Init structure definition
 *
 * \ingroup     LCDC_EDPI_Exported_Types
 */
typedef struct
{
    uint32_t eDPI_ClockDiv;            /*!< EDPI interface clock divider parameter.
                                              This parameter can be a value of @ref LCDC_EDPI_Clock_Divider */
    uint32_t eDPI_HoriSyncWidth;       /*!< The number of Horizontal Synchronization pixel. */
    uint32_t eDPI_VeriSyncHeight;      /*!< The vertical Synchronization height, it represents the number of horizontal synchronization lines. */
    uint32_t eDPI_AccumulatedHBP;      /*!< The accumulated horizontal back porch width that includes the horizontal synchronization and horizontal back porch pixels. */
    uint32_t eDPI_AccumulatedVBP;      /*!< The accumulated vertical back porch width that includes the vertical synchronization and vertical back porch lines. */
    uint32_t eDPI_AccumulatedActiveW;  /*!< The accumulated active width which includes the horizontal synchronization, horizontal back porch and active pixels. */
    uint32_t eDPI_AccumulatedActiveH;  /*!< The accumulated height which includes the vertical synchronization, vertical back porch and the active height lines. The active height is the number of activelines in the panel. */
    uint32_t eDPI_TotalWidth;          /*!< The accumulated total width which includes the horizontal synchronization, horizontal back porch, active width and horizontal front porch pixels */
    uint32_t eDPI_TotalHeight;         /*!< The accumulated height which includes the vertical synchronization, vertical back porch, the active height and vertical front porch height lines. */
    uint32_t eDPI_HoriSyncPolarity;    /*!< Horizontal synchronization polarity.
                                              This parameter can be a value of @ref LCDC_EDPI_HS_POLARITY */
    uint32_t eDPI_VeriSyncPolarity;    /*!< Vertical synchronization polarity.
                                              This parameter can be a value of @ref LCDC_EDPI_VS_POLARITY */
    uint32_t eDPI_DataEnPolarity;      /*!< Data enable polarity.
                                              This parameter can be a value of @ref LCDC_EDPI_DE_POLARITY */
    uint32_t eDPI_LineIntMask;         /*!< Determines if line interrupt is masked.
                                              This parameter can be a value of @ref LCDC_EDPI_Line_INT_MASK */
    uint32_t eDPI_LineIntFlag;         /*!< Line interrupt generated when a programmed line is reached. */
    uint32_t eDPI_LineIntClr;          /*!< Clears the line interrupt flag. */
    uint32_t eDPI_ColorMap;            /*!< The pixel color map.
                                              This parameter can be a value of @ref LCDC_EDPI_Pixel_Format */
    uint32_t eDPI_OperateMode;         /*!< Operating mode of eDPI.
                                              This parameter can be a value of @ref LCDC_EDPI_OP_MODE */
    uint32_t eDPI_LcdArc;              /*!< LCD display driver module architecture.
                                              This parameter can be a value of @ref LCDC_EDPI_LCD_ARC */
    uint32_t eDPI_ShutdnPolarity;      /*!< Shutdown polarity setting.
                                              This parameter can be a value of @ref LCDC_EDPI_Shutdown_Polarity */
    uint32_t eDPI_ColorModePolarity;   /*!< EDPI Color Mode Polarity.
                                              This parameter can be a value of @ref LCDC_EDPI_Color_Mode_Polarity */
    uint32_t eDPI_ShutdnEn;            /*!< EDPI shut down signal control.
                                              This parameter can be a value of @ref LCDC_EDPI_Shutdown_Signal_Enable */
    uint32_t eDPI_ColorModeEn;         /*!< EDPI color mode signal control.
                                              This parameter can be a value of @ref LCDC_EDPI_Color_Mode_Signal_Enable */
    uint32_t eDPI_UpdateCfgEn;         /*!< EDPI update config pulse control, DPI outputs a pulse of 1 period width, and return to 0 automatically.
                                              This parameter can be a value of @ref LCDC_EDPI_Update_Config_Pulse_Enable */
    uint32_t eDPI_TearReq;             /*!< Tear request pulse enable, outputs a pulse of 1 period width, and return to 0 automatically.
                                              This parameter can be a value of @ref LCDC_EDPI_Tear_Request_Pulse_Enable */
    uint32_t eDPI_Halt;                /*!< Halt signal valid level.
                                              This parameter can be a value of @ref LCDC_EDPI_Halt_Signal_Valid */
    uint32_t eDPI_CmdMaxLatency;       /*!< Max time of DE holding transfer. After max latency, if DE is still invalid, eDPI would be pulled down to trigger the transmission of rest pixels. */
    uint32_t eDPI_LineBufferPixelThreshold;  /*!< Pixel threshold in line buffer to start transmit data after auto-write starts. */
} LCDC_eDPICfgTypeDef;

/** End of LCDC_EDPI_Exported_Types
  * \}
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/
/** \defgroup LCDC_EDPI_Exported_Constants LCDC EDPI Exported Constants
  * \brief
  * \{
  */

/**
 * \defgroup    LCDC_EDPI_Clock_Divider LCDC EDPI Clock Divider
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_CLOCKDIV2                             ((uint32_t)0x1)
#define EDPI_CLOCKDIV3                             ((uint32_t)0x2)
#define EDPI_CLOCKDIV4                             ((uint32_t)0x3)
#define EDPI_CLOCKDIV5                             ((uint32_t)0x4)
#define EDPI_CLOCKDIV6                             ((uint32_t)0x5)
#define EDPI_CLOCKDIV7                             ((uint32_t)0x6)
#define EDPI_CLOCKDIV8                             ((uint32_t)0x7)

#define IS_EDPI_CLOCKDIV(DIV)   (((DIV) == EDPI_CLOCKDIV2) || ((DIV) == EDPI_CLOCKDIV3) || \
                                 ((DIV) == EDPI_CLOCKDIV4) || ((DIV) == EDPI_CLOCKDIV5) || \
                                 ((DIV) == EDPI_CLOCKDIV6) || ((DIV) == EDPI_CLOCKDIV7) || \
                                 ((DIV) == EDPI_CLOCKDIV8))

/** End of LCDC_EDPI_Clock_Divider
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_HS_POLARITY LCDC EDPI Horizontal Synchronization Polarity
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_HSPOL_AL                (0)            /*!< Horizontal synchronization polarity is active low. */
#define EDPI_HSPOL_AH                (1)            /*!< Horizontal synchronization polarity is active high. */

#define IS_EDPI_HSPOL(POLARITY)   ((POLARITY) == EDPI_HSPOL_AL) || ((POLARITY) == EDPI_HSPOL_AH)

/** End of LCDC_EDPI_HS_POLARITY
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_VS_POLARITY LCDC EDPI Vertical Synchronization Polarity
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_VSPOL_AL                (0)            /*!< Vertical Synchronization is active low. */
#define EDPI_VSPOL_AH                (1)            /*!< Vertical Synchronization is active high. */

#define IS_EDPI_VSPOL(POLARITY)   ((POLARITY) == EDPI_VSPOL_AL) || ((POLARITY) == EDPI_VSPOL_AH)

/** End of LCDC_EDPI_VS_POLARITY
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_DE_POLARITY LCDC EDPI Data Enable Polarity
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_DEPOL_AL                (0)            /*!< Data enable polarity is active low. */
#define EDPI_DEPOL_AH                (1)            /*!< Data enable polarity is active high. */

#define IS_EDPI_DEPOL(POLARITY)   ((POLARITY) == EDPI_DEPOL_AL) || ((POLARITY) == EDPI_DEPOL_AH)

/** End of LCDC_EDPI_DE_POLARITY
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_Line_INT_MASK LCDC EDPI Line Interrupt Mask
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_LIM_UNMASK            (0)              /*!< Line interrupt is unmasked */
#define EDPI_LIM_MASK              (1)              /*!< Line interrupt is masked */

#define IS_EDPI_LIM(LIM)   ((LIM) == EDPI_LIM_UNMASK) || ((LIM) == EDPI_LIM_MASK)

/** End of LCDC_EDPI_Line_INT_MASK
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_Pixel_Format LCDC EDPI Pixel Format
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_PIXELFORMAT_RGB888            (0)               /*!< RGB888 */
#define EDPI_PIXELFORMAT_RGB565_1          ((uint32_t)0x1)   /*!<RGB565_3(R[D15:D11]G[D10:D5]B[D4:D0])*/
#define EDPI_PIXELFORMAT_RGB565_2          ((uint32_t)0x2)   /*!<RGB565_2(R[D20:D16]G[D13:D8]B[D4:D0])*/
#define EDPI_PIXELFORMAT_RGB565_3          ((uint32_t)0x3)   /*!<RGB565_1(R[D21:D17]G[D13:D8]B[D5:D1])*/

#define IS_EDPI_PIXELFORMAT(FORMAT)   (((FORMAT) == EDPI_PIXELFORMAT_RGB888) || ((FORMAT) == EDPI_PIXELFORMAT_RGB565_1) || \
                                       ((FORMAT) == EDPI_PIXELFORMAT_RGB565_2) || ((FORMAT) == EDPI_PIXELFORMAT_RGB565_3))

/** End of LCDC_EDPI_Pixel_Format
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_OP_MODE LCDC EDPI Operating Mode
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_STANDARD_VIDEO_MODE            (0)             /*!< Standard Video mode. */
#define EDPI_ADAPTED_COMMAND_MODE           (1)             /*!< Adapted Command mode. */

#define IS_EDPI_OP_MODE(MDOE)   ((MODE) == EDPI_STANDARD_VIDEO_MODE) || ((MODE) == EDPI_ADAPTED_COMMAND_MODE)

/** End of LCDC_EDPI_OP_MODE
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_LCD_ARC LCDC EDPI LCD Display Driver Module Architecture
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_LCD_ARC_TYPE123            (0)             /*!< LCD Display Driver is of Type 1 or 2 or 3. */
#define EDPI_LCD_ARC_TYPE4              (1)             /*!< LCD Display Driver is of Type 4. */

#define IS_EDPI_LCD_ARC_TYPE(TYPE)   ((TYPE) == EDPI_LCD_ARC_TYPE123) || ((TYPE) == EDPI_LCD_ARC_TYPE4)

/** End of LCDC_EDPI_LCD_ARC
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_SD_EN_FCOLOR LCDC EDPI SD EN FCOLOR
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_SD_EN_FCOLOR_BLACK            (0)   /*!< this bit defines the color of the 4 frames is black. */
#define EDPI_SD_EN_FCOLOR_WHITE            (1)   /*!< this bit defines the color of the 4 frames is white. */

#define IS_EDPI_SD_EN_FCOLOR(FCOLOR)   ((FCOLOR) == EDPI_SD_EN_FCOLOR_BLACK) || ((FCOLOR) == EDPI_SD_EN_FCOLOR_WHITE)

/** End of LCDC_EDPI_SD_EN_FCOLOR
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_SD_DIS_FCOLOR LCDC EDPI SD DIS FCOLOR
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_SD_DIS_FCOLOR_BLACK              (0)           /*!< this bit defines the color of the 12 frames is black. */
#define EDPI_SD_DIS_FCOLOR_WHITE              (1)           /*!< this bit defines the color of the 12 frames is white. */

#define IS_EDPI_SD_DIS_FCOLOR(FCOLOR)   ((FCOLOR) == EDPI_SD_DIS_FCOLOR_BLACK) || ((FCOLOR) == EDPI_SD_DIS_FCOLOR_WHITE)

/** End of LCDC_EDPI_SD_DIS_FCOLOR
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_Shutdown_Polarity LCDC EDPI Shutdown Polarity
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_SDPOL_AL                (0)            /*!< ShutDown is active low. */
#define EDPI_SDPOL_AH                (1)            /*!< ShutDown is active high. */

#define IS_EDPI_SDPOL(POLARITY)   ((POLARITY) == EDPI_SDPOL_AL) || ((POLARITY) == EDPI_SDPOL_AH)

/** End of LCDC_EDPI_Shutdown_Polarity
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_Color_Mode_Polarity LCDC EDPI Color Mode Polarity
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_CLMPOL_AL                (0)               /*!< Falling edge. */
#define EDPI_CLMPOL_AH                (1)               /*!< Rising edge. */

#define IS_EDPI_CLMPOL(POLARITY)   ((POLARITY) == EDPI_CLMPOL_AL) || ((POLARITY) == EDPI_CLMPOL_AH)

/** End of LCDC_EDPI_Color_Mode_Polarity
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_Shutdown_Signal_Enable LCDC EDPI Shutdown Signal Enable
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_SD_DISABLE                (0)            /*!< Shutdown signal DISABLE. */
#define EDPI_SD_ENABLE                 (1)            /*!< Shutdown signal ENABLE. */

#define IS_EDPI_SD_CMD(CMD)             ((CMD) == EDPI_SD_DISABLE) || ((CMD) == EDPI_SD_ENABLE)

/** End of LCDC_EDPI_Shutdown_Signal_Enable
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_Color_Mode_Signal_Enable LCDC EDPI Color Mode Signal Enable
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_CLM_DISABLE                (0)             /*!< Color Mode Signal DISABLE. */
#define EDPI_CLM_ENABLE                 (1)             /*!< Color Mode Signal ENABLE. */

#define IS_EDPI_CLM_CMD(CMD)   ((CMD) == EDPI_CLM_DISABLE) || ((CMD) == EDPI_CLM_ENABLE)

/** End of LCDC_EDPI_Color_Mode_Signal_Enable
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_Update_Config_Pulse_Enable LCDC EDPI Update Config Pulse Enable
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_UPEN_DISABLE                (0)            /*!< Update Config Pulse DISABLE. */
#define EDPI_UPEN_ENABLE                 (1)            /*!< Update Config Pulse ENABLE. */

#define IS_EDPI_UPEN_CMD(CMD)   ((CMD) == EDPI_UPEN_DISABLE) || ((CMD) == EDPI_UPEN_ENABLE)

/** End of LCDC_EDPI_Update_Config_Pulse_Enable
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_Tear_Request_Pulse_Enable LCDC EDPI Tear Request Pulse Enable
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_TEARREQ_AL                (0)              /*!< Tear Request Pulse DISABLE. */
#define EDPI_TEARREQ_AH                (1)              /*!< Tear Request Pulse ENABLE. */

#define IS_EDPI_TEARREQ_POLARITY(POLARITY)   ((POLARITY) == EDPI_TEARREQ_AL) || ((POLARITY) == EDPI_TEARREQ_AH)

/** End of LCDC_EDPI_Tear_Request_Pulse_Enable
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_Halt_Signal_Valid LCDC EDPI Halt Signal Valid
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_HALT_AL                (0)             /*!< halt signal is low. */
#define EDPI_HALT_AH                (1)             /*!< halt signal is high. */

#define IS_EDPI_HALT_POLARITY(POLARITY)   ((POLARITY) == EDPI_HALT_AL) || ((POLARITY) == EDPI_HALT_AH)

/** End of LCDC_EDPI_Halt_Signal_Valid
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_DE_CONSTANT LCDC EDPI DE Signal Constant Value
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_DE_CONSTANT_NONE                (0x1)      /*!< No constant value for DE, it can be drive by display controller*/
#define EDPI_DE_CONSTANT_LOW                 (0x0)      /*!< constant low for DE. */
#define EDPI_DE_CONSTANT_HIGH                (0x3)      /*!< constant high for DE */

#define IS_EDPI_DE_CONSTANT_VALUE(VALUE)   ((VALUE) == EDPI_DE_CONSTANT_NONE) || ((VALUE) == EDPI_DE_CONSTANT_LOW) || ((VALUE) == EDPI_DE_CONSTANT_HIGH)

/** End of LCDC_EDPI_DE_CONSTANT
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_VSYNC_CONSTANT LCDC EDPI VSYNC Signal Constant Value
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_VSYNC_CONSTANT_NONE                (0x1)      /*!< No constant value for VSYNC, it can be drive by display controller*/
#define EDPI_VSYNC_CONSTANT_LOW                 (0x0)      /*!< constant low for VSYNC. */
#define EDPI_VSYNC_CONSTANT_HIGH                (0x3)      /*!< constant high for VSYNC */

#define IS_EDPI_VSYNC_CONSTANT_VALUE(VALUE)   ((VALUE) == EDPI_VSYNC_CONSTANT_NONE) || ((VALUE) == EDPI_VSYNC_CONSTANT_LOW) || ((VALUE) == EDPI_VSYNC_CONSTANT_HIGH)

/** End of LCDC_EDPI_VSYNC_CONSTANT
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_HSYNC_CONSTANT LCDC EDPI HSYNC Signal Constant Value
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
#define EDPI_HSYNC_CONSTANT_NONE                (0x1)      /*!< No constant value for HSYNC, it can be drive by display controller*/
#define EDPI_HSYNC_CONSTANT_LOW                 (0x0)      /*!< constant low for HSYNC. */
#define EDPI_HSYNC_CONSTANT_HIGH                (0x3)      /*!< constant high for HSYNC */

#define IS_EDPI_HSYNC_CONSTANT_VALUE(VALUE)   ((VALUE) == EDPI_HSYNC_CONSTANT_NONE) || ((VALUE) == EDPI_HSYNC_CONSTANT_LOW)  || ((VALUE) == EDPI_HSYNC_CONSTANT_HIGH)

/** End of LCDC_EDPI_HSYNC_CONSTANT
  * \}
  */

/**
 * \defgroup    LCDC_EDPI_Synchronization_Signal LCDC EDPI Synchronization Signal
 * \{
 * \ingroup     LCDC_EDPI_Exported_Constants
 */
typedef enum
{
    EDPI_HSYNCS = 0,
    EDPI_VSYNCS,
    EDPI_HDES,
    EDPI_VDES,
} EDPI_SIGNAL_t;

/** End of LCDC_EDPI_Synchronization_Signal
  * \}
  */

/** End of LCDC_EDPI_Exported_Constants
  * \}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** \defgroup LCDC_EDPI_Exported_Functions LCDC EDPI Exported Functions
  * \brief
  * \{
  */

/**
 * rtl_lcdc_edpi.h
 *
 * \brief  Mask or unmask line interrupt.
 *
 * \param[in] state: New state of line interrupt.
 *            This parameter can be one of the following values:
 *            \arg ENABLE: Mask line interrupt.
 *            \arg DISABLE: Unmask line interrupt.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_edpi_init(void)
 * {
 *     EDPI_MaskLineINTConfig(ENABLE);
 * }
 * \endcode
 */
void EDPI_MaskLineINTConfig(FunctionalState state);

/**
 * rtl_lcdc_edpi.h
 *
 * \brief  Clear line interrupt flag.
 *
 * \param None
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_edpi_init(void)
 * {
 *     LCDC_ClearLineINTPendingBit();
 * }
 * \endcode
 */
void LCDC_ClearLineINTPendingBit(void);

/**
 * rtl_lcdc_edpi.h
 *
 * \brief  Get line interrupt position.
 *
 * \param None
 *
 * \return Line interrupt position.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_edpi_init(void)
 * {
 *     uint16_t position = EDPI_GetLineINTPos();
 * }
 * \endcode
 */
uint16_t EDPI_GetLineINTPos(void);

/**
 * rtl_lcdc_edpi.h
 *
 * \brief  Get current X position.
 *
 * \param None
 *
 * \return Current X position.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_edpi_init(void)
 * {
 *     uint16_t pos = EDPI_GetXPos();
 * }
 * \endcode
 */
uint16_t EDPI_GetXPos(void);

/**
 * rtl_lcdc_edpi.h
 *
 * \brief  Get current Y position.
 *
 * \param None
 *
 * \return Current Y position.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_edpi_init(void)
 * {
 *     uint16_t pos = EDPI_GetYPos();
 * }
 * \endcode
 */
uint16_t EDPI_GetYPos(void);

/**
 * rtl_lcdc_edpi.h
 *
 * \brief  Config operating mode of edpi Interface.
 *
 * \param[in] mode: Operating mode of edpi Interface.
 *            This parameter can be one of the following values:
 *            \arg EDPI_STANDARD_VIDEO_MODE: Standard Video mode.
 *            \arg EDPI_ADAPTED_COMMAND_MODE: Adapted Command mode.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_edpi_init(void)
 * {
 *     EDPI_OPMODE_CONFIG(EDPI_ADAPTED_COMMAND_MODE);
 * }
 * \endcode
 */
void EDPI_OPMODE_CONFIG(uint32_t mode);

/**
 * rtl_lcdc_edpi.h
 *
 * \brief  Initializes EDPI interface according to the specified parameters in the eDPICfg.
 *
 * \param[in] eDPICfg: Pointer to a LCDC_eDPICfgTypeDef structure that contains the configuration information for EDPI interface.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_edpi_init(void)
 * {
 *     LCDC_eDPICfgTypeDef eDPICfg
 *     eDPICfg.eDPI_ClockDiv = 4;
 *     eDPICfg.eDPI_HoriSyncWidth = SH8601A_LCD_HSYNC;
 *     eDPICfg.eDPI_VeriSyncHeight = SH8601A_LCD_VSYNC;
 *     eDPICfg.eDPI_AccumulatedHBP = SH8601A_LCD_HSYNC + SH8601A_LCD_HBP;
 *     eDPICfg.eDPI_AccumulatedVBP = SH8601A_LCD_VSYNC + SH8601A_LCD_VBP;
 *     eDPICfg.eDPI_AccumulatedActiveW = SH8601A_LCD_HSYNC + SH8601A_LCD_HBP + SH8601A_LCD_WIDTH;
 *     eDPICfg.eDPI_AccumulatedActiveH = SH8601A_LCD_VSYNC + SH8601A_LCD_VBP + SH8601A_LCD_HEIGHT;
 *     eDPICfg.eDPI_TotalWidth = SH8601A_LCD_WIDTH + SH8601A_LCD_HSYNC + SH8601A_LCD_HBP + SH8601A_LCD_HFP;
 *     eDPICfg.eDPI_TotalHeight = SH8601A_LCD_HEIGHT + SH8601A_LCD_VSYNC + SH8601A_LCD_VBP + SH8601A_LCD_VFP;
 *     eDPICfg.eDPI_HoriSyncPolarity = EDPI_HSPOL_AH;
 *     eDPICfg.eDPI_VeriSyncPolarity = EDPI_VSPOL_AH;
 *     eDPICfg.eDPI_DataEnPolarity = EDPI_DEPOL_AH;
 *     eDPICfg.eDPI_LineIntMask = ENABLE;
 *     eDPICfg.eDPI_ColorMap = EDPI_PIXELFORMAT_RGB888;
 *     eDPICfg.eDPI_OperateMode = EDPI_ADAPTED_COMMAND_MODE;
 *     eDPICfg.eDPI_LcdArc = EDPI_LCD_ARC_TYPE123;
 *     eDPICfg.eDPI_ShutdnPolarity = EDPI_SDPOL_AL;
 *     eDPICfg.eDPI_ShutdnEn = EDPI_SD_DISABLE;
 *     eDPICfg.eDPI_ColorModeEn = EDPI_CLM_DISABLE;
 *     eDPICfg.eDPI_UpdateCfgEn = 0;
 *     eDPICfg.eDPI_TearReq = 0;
 *     eDPICfg.eDPI_Halt = EDPI_HALT_AL;
 *     eDPICfg.eDPI_CmdMaxLatency = 0x39d23;
 *     eDPICfg.eDPI_LineBufferPixelThreshold = eDPICfg.eDPI_TotalWidth / 2;
 *     EDPI_Init(&eDPICfg);
 * }
 * \endcode
 */
void EDPI_Init(LCDC_eDPICfgTypeDef *eDPICfg);

/** End of LCDC_EDPI_Exported_Functions
  * \}
  */

/** End of LCDC_EDPI
  * \}
  */

/** End of LCDC
  * \}
  */

#ifdef __cplusplus
}
#endif

#endif /*RTL_EDPI_H*/


/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/

