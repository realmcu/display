/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* \file     rtl_lcdc_dbib.h
* \brief    The header file of the peripheral DBIB driver
* \details  This file provides all DBIB firmware functions including initialization,
*           configuration, data transmission and reception.
* \author   boris yue
* \date     2023-10-17
* \version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_DBIB_H
#define RTL_DBIB_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "rtl_lcdc.h"
#include "rtl_lcdc_dbib_def.h"

/** \defgroup LCDC        LCDC
  * \brief    LCD Controller driver module
  * \{
  */

/** \defgroup LCDC_DBIB        LCDC DBIB
  * \brief    LCD Controller Display Bus Interface Type-B driver module
  * \{
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/
/** \defgroup LCDC_DBIB_Exported_Constants LCDC DBIB Exported Constants
  * \brief    Constants used by the DBIB driver module
  * \{
  */

/**
 * \defgroup    LCDC_DBIB_InitGuardTime_Cmd LCDC DBIB Init Guard Time Cmd
 * \{
 * \ingroup     LCDC_DBIB_Exported_Constants
 */
#define DBIB_INIT_GUARD_TIME_ENABLE                   (0)   /*!< DBIB Init Guard Time Enable. */
#define DBIB_INIT_GUARD_TIME_DISABLE                  (1)   /*!< DBIB Init Guard Time Disable. */

#define IS_DBIB_INIT_GUARD_TIME_CMD(CMD) (((CMD) == DBIB_INIT_GUARD_TIME_ENABLE) || ((CMD) == DBIB_INIT_GUARD_TIME_DISABLE))

/** End of LCDC_DBIB_InitGuardTime_Cmd
  * \}
  */

/**
 * \defgroup    LCDC_DBIB_GuardTime LCDC DBIB Guard Time Duration
 * \{
 * \ingroup     LCDC_DBIB_Exported_Constants
 */
#define DBIB_GUARD_TIME_1T                       (0)              /*!< The guard time is 1T of divider clock. */
#define DBIB_GUARD_TIME_2T                       ((uint32_t)0x1)  /*!< The guard time is 2T of divider clock. */
#define DBIB_GUARD_TIME_3T                       ((uint32_t)0x2)  /*!< The guard time is 3T of divider clock. */
#define DBIB_GUARD_TIME_4T                       ((uint32_t)0x3)  /*!< The guard time is 4T of divider clock. */

#define IS_DBIB_GUARD_TIME(TIME)   (((TIME) == DBIB_GUARD_TIME_1T) || ((TIME) == DBIB_GUARD_TIME_2T) || \
                                    ((TIME) == DBIB_GUARD_TIME_3T) || ((TIME) == DBIB_GUARD_TIME_4T))

/** End of LCDC_DBIB_GuardTime
  * \}
  */

/**
 * \defgroup    LCDC_DBIB_CmdGuardTime_Cmd LCDC DBIB Cmd Guard Time Cmd
 * \{
 * \ingroup     LCDC_DBIB_Exported_Constants
 */
#define DBIB_CMD_GUARD_TIME_ENABLE                   (0)   /*!< DBIB Cmd Guard Time Enable. */
#define DBIB_CMD_GUARD_TIME_DISABLE                  (1)   /*!< DBIB Cmd Guard Time Disable. */

#define IS_DBIB_CMD_GUARD_TIME_CMD(CMD) (((CMD) == DBIB_CMD_GUARD_TIME_ENABLE) || ((CMD) == DBIB_CMD_GUARD_TIME_DISABLE))

/** End of LCDC_DBIB_CmdGuardTime_Cmd
  * \}
  */


/**
 * \defgroup   LCDC_DBIB_GuardTime_Cmd LCDC DBIB Guard Time Cmd
 * \{
 * \ingroup    LCDC_DBIB_Exported_Constants
 */

#define DBIB_GUARD_TIME_ENABLE                       (0)       /*!< DBIB Guard Time Enable. */
#define DBIB_GUARD_TIME_DISABLE                      (1)       /*!< DBIB Guard Time Disable. */

#define IS_DBIB_GUARD_TIME_CMD(CMD) (((CMD) == DBIB_GUARD_TIME_ENABLE) || ((CMD) == DBIB_GUARD_TIME_DISABLE))

/** End of LCDC_DBIB_GuardTime_Cmd
  * \}
  */

/**
 * \defgroup    LCDC_DBIB_WRDelay LCDC DBIB WR Delay
 * \{
 * \ingroup     LCDC_DBIB_Exported_Constants
 */
#define DBIB_WR_HALF_DELAY          ((uint32_t)0x1)   /*!< DBIB_WRDelay = 1/2 display clock cycle. */
#define DBIB_WR_ONE_DELAY           ((uint32_t)0x2)   /*!< DBIB_WRDelay = 1T of display clock cycle. */

#define IS_DBIB_WR_DELAY_TIME(TIME) (((TIME) == DBIB_WR_HALF_DELAY) || \
                                     ((TIME) == DBIB_WR_ONE_DELAY))

/** End of LCDC_DBIB_WRDelay
  * \}
  */

/** End of LCDC_DBIB_Exported_Constants
  * \}
  */

/*============================================================================*
 *                         Types
 *============================================================================*/
/** \defgroup LCDC_DBIB_Exported_Types LCDC DBIB Exported Types
  * \brief    Types provided by the DBIB driver module
  * \{
  */

/**
 * \brief       LCDC DBIB init structure definition.
 *
 * \ingroup     LCDC_DBIB_Exported_Types
 */
typedef struct
{
    uint32_t DBIB_Clock_Divider;          /*!< Specifies the DBIB clock divider value, ranging from 2 to 64. */
    uint32_t DBIB_InitGuardTimeCmd;       /*!< Controls the init guard time function, only available for hardware continuously write mode.
                                                 This parameter can be a value of \ref LCDC_DBIB_InitGuardTime_Cmd. */
    uint32_t DBIB_InitGuardTime;          /*!< Delay between DBI-B idle state and DCX signal become active.
                                                 This parameter can be a value of \ref LCDC_DBIB_GuardTime . */
    uint32_t DBIB_CmdGuardTimeCmd;        /*!< Controls the command guard time function, only available for hardware continuously write mode.
                                                 This parameter can be a value of \ref LCDC_DBIB_CmdGuardTime_Cmd */
    uint32_t DBIB_CmdGuardTime;           /*!< Delay between CS signal become active and command output.
                                                 This parameter can be a value of \ref LCDC_DBIB_GuardTime. */
    uint32_t DBIB_GuardTimeCmd;           /*!< Controls the guard time function, only available for hardware continuously write mode.*/
    uint32_t DBIB_GuardTime;              /*!< Delay between command output and data input/output.
                                                 This parameter can be a value of \ref LCDC_DBIB_GuardTime. */
    uint32_t DBIB_WRDelay;                /*!< Specifies the delay time for WR. This parameter can be a value of \ref LCDC_DBIB_WRDelay. */
} LCDC_DBIBCfgTypeDef;

/** End of LCDC_DBIB_Exported_Types
  * \}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** \defgroup LCDC_DBIB_Exported_Functions LCDC DBIB Exported Functions
  * \brief    Functions provided by the DBIB driver module
  * \{
  */

/**
 * \brief  Enable or disable command byte bypass in auto mode.
 *
 * \param[in] NewState: New state of command byte bypass.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable command byte bypass.
 *            - DISABLE: Disable command byte bypass.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbib_init(void)
 * {
 *     DBIB_BypassCmdByteCmd(ENABLE);
 * }
 * \endcode
 */
void DBIB_BypassCmdByteCmd(FunctionalState NewState);

/**
 * \brief  Pull down CS to active state.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbib_init(void)
 * {
 *     DBIB_ResetCS();
 * }
 * \endcode
 */
void DBIB_ResetCS(void);

/**
 * \brief  Pull up CS to idle state.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbib_init(void)
 * {
 *     DBIB_SetCS();
 * }
 * \endcode
 */
void DBIB_SetCS(void);

/**
 * \brief  Put DBIB in command mode.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbib_init(void)
 * {
 *     DBIB_ResetDC();
 * }
 * \endcode
 */
void DBIB_ResetDC(void);

/**
 * \brief  Put DBIB in data mode.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbib_init(void)
 * {
 *     DBIB_SetDC();
 * }
 * \endcode
 */
void DBIB_SetDC(void);

/**
 * \brief  Initializes DBIB interface according to the specified parameters in the DBIBCfg.
 *
 * \param[in] DBIBCfg: Pointer to a LCDC_DBIBCfgTypeDef structure that contains the configuration information for DBIB interface.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbib_init(void)
 * {
 *     LCDC_DBIBCfgTypeDef dbib_init = {0};
 *     dbib_init.DBIB_SPEED_SEL         = DBIB_CLOCK_DIV_2;
 *     dbib_init.DBIB_InitGuardTimeCmd  = DBIB_INIT_GUARD_TIME_DISABLE;
 *     dbib_init.DBIB_InitGuardTime     = DBIB_INIT_GUARD_TIME_1T;
 *     dbib_init.DBIB_CmdGuardTimeCmd   = DBIB_CMD_GUARD_TIME_DISABLE;
 *     dbib_init.DBIB_CmdGuardTime      = DBIB_CMD_GUARD_TIME_1T;
 *     dbib_init.DBIB_GuardTimeCmd      = DBIB_GUARD_TIME_DISABLE;
 *     dbib_init.DBIB_GuardTime         = DBIB_GUARD_TIME_1T;
 *     dbib_init.DBIB_WRDelay           = DBIB_WR_HALF_DELAY;
 *     DBIB_Init(&dbib_init);
 * }
 * \endcode
 */
void DBIB_Init(LCDC_DBIBCfgTypeDef *DBIBCfg);

/**
 * \brief  Send command in manual mode.
 *
 * \param[in] cmd: Command which to be sent.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbib_init(void)
 * {
 *     DBIB_SendCmd(0x09);
 * }
 * \endcode
 */
void DBIB_SendCmd(uint8_t cmd);

/**
 * \brief  Send data in manual mode.
 *
 * \param[in] pBuf: Data buffer for sending.
 * \param[in] len: The length of the data to be sent.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbib_init(void)
 * {
 *     uint8_t buf[3] = {0x05, 0x08, 0x10};
 *     DBIB_SendData(buf, 3);
 * }
 * \endcode
 */
void DBIB_SendData(uint8_t *pBuf, uint32_t len);

/**
 * \brief  Receive data in manual mode.
 *
 * \param[in] pBuf: Data buffer for receiving.
 * \param[in] len: The length of the data to be received.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbib_init(void)
 * {
 *     uint8_t buf[3] = {0, 0, 0};
 *     DBIB_ReceiveData(buf, 3);
 * }
 * \endcode
 */
void DBIB_ReceiveData(uint8_t *pBuf, uint32_t len);

/**
 * \brief  Send command and data in manual mode.
 *
 * \param[in] cmd: Command which to be sent.
 * \param[in] pBuf: Data buffer for sending.
 * \param[in] len: The length of the data to be sent.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbib_init(void)
 * {
 *     uint8_t buf[3] = {0x05, 0x08, 0x10};
 *     DBIB_Write(0x09, buf, 3);
 * }
 * \endcode
 */
void DBIB_Write(uint8_t cmd, uint8_t *pBuf, uint32_t len);

/**
 * \brief  Send command and read data in manual mode.
 *
 * \param[in] cmd: Command to be sent.
 * \param[in] pBuf: Data buffer for receiving.
 * \param[in] len: The length of the data to be received.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbib_init(void)
 * {
 *     uint8_t buf[3] = {0, 0, 0};
 *     DBIB_Read(0x10, buf, 3);
 * }
 * \endcode
 */
void DBIB_Read(uint8_t cmd, uint8_t *pBuf, uint32_t len);

/**
 * \brief  Configure command sequences in auto mode.
 *
 * \param[in] pCmdBuf: Buffer address which store command sequence.
 * \param[in] len: Command length.
 *
 * \return  The status of command sequence configuration..
 * \retval SET: Command sequence configuration successful.
 * \retval RESET: Command sequence configuration failed.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbib_init(void)
 * {
 *     uint8_t buf[5] = {0x15, 0x18, 0x11, 0x20, 0x26};
 *     LCDC_DBIB_SetCmdSequence(buf, 5);
 * }
 * \endcode
 */
FlagStatus LCDC_DBIB_SetCmdSequence(uint8_t *pCmdBuf, uint8_t len);

/**
 * \brief  Initialize the LCDC_DBIBCfgTypeDef structure with default values.
 *
 * \param[in] DBIBCfg: Pointer to a LCDC_DBIBCfgTypeDef structure that will be initialized.
 *
 * \note Default values for each member:
 *       | Member                  | Default Value                        |
 *       |-------------------------|--------------------------------------|
 *       | DBIB_Clock_Divider      | 2                                    |
 *       | DBIB_InitGuardTimeCmd   | \ref DBIB_INIT_GUARD_TIME_DISABLE    |
 *       | DBIB_InitGuardTime      | \ref DBIB_GUARD_TIME_1T              |
 *       | DBIB_CmdGuardTimeCmd    | \ref DBIB_CMD_GUARD_TIME_DISABLE     |
 *       | DBIB_CmdGuardTime       | \ref DBIB_GUARD_TIME_1T              |
 *       | DBIB_GuardTimeCmd       | \ref DBIB_GUARD_TIME_DISABLE         |
 *       | DBIB_GuardTime          | \ref DBIB_GUARD_TIME_1T              |
 *       | DBIB_WRDelay            | \ref DBIB_WR_HALF_DELAY              |
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbib_init(void)
 * {
 *     LCDC_DBIBCfgTypeDef dbib_init;
 *     DBIB_StructInit(&dbib_init);
 *     DBIB_Init(&dbib_init);
 * }
 * \endcode
 */
void DBIB_StructInit(LCDC_DBIBCfgTypeDef *DBIBCfg);

/** End of LCDC_DBIB_Exported_Functions
  * \}
  */

/** End of LCDC_DBIB
  * \}
  */

/** End of LCDC
  * \}
  */

#ifdef __cplusplus
}
#endif

#endif /*RTL_DBIB_H*/

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/

