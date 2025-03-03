/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     rtl_lcdc_dbib.h
* @brief    The header file of the peripheral DBIB driver
* @details
* @author   boris yue
* @date     2023-10-17
* @version  v1.0
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
  * \brief
  * \{
  */

/** \defgroup LCDC_DBIB        LCDC DBIB
  * \brief
  * \{
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/
/** \defgroup LCDC_DBIB_Exported_Constants LCDC DBIB Exported Constants
  * \brief
  * \{
  */

/**
 * \defgroup    LCDC_DBIB_Clock_Divider LCDC DBIB Clock Divider
 * \{
 * \ingroup     LCDC_DBIB_Exported_Constants
 */
#define DBIB_CLOCK_DIV_2                             ((uint32_t)0x1)
#define DBIB_CLOCK_DIV_3                             ((uint32_t)0x2)
#define DBIB_CLOCK_DIV_4                             ((uint32_t)0x3)
#define DBIB_CLOCK_DIV_5                             ((uint32_t)0x4)
#define DBIB_CLOCK_DIV_6                             ((uint32_t)0x5)
#define DBIB_CLOCK_DIV_7                             ((uint32_t)0x6)
#define DBIB_CLOCK_DIV_8                             ((uint32_t)0x7)
#define DBIB_CLOCK_DIV_64                            ((uint32_t)0x3F)

#define IS_DBIB_CLOCK_DIV(DIV)   (((DIV) == DBIB_CLOCK_DIV_2) || ((DIV) == DBIB_CLOCK_DIV_3) || \
                                  ((DIV) == DBIB_CLOCK_DIV_4) || ((DIV) == DBIB_CLOCK_DIV_5) || \
                                  ((DIV) == DBIB_CLOCK_DIV_6) || ((DIV) == DBIB_CLOCK_DIV_7) || \
                                  ((DIV) == DBIB_CLOCK_DIV_8))

/** End of LCDC_DBIB_Clock_Divider
  * \}
  */

/**
 * \defgroup    LCDC_DBIB_InitGuardTime_Cmd LCDC DBIB Init Guard Time Cmd
 * \{
 * \ingroup     LCDC_DBIB_Exported_Constants
 */
#define DBIB_INIT_GUARD_TIME_ENABLE                   (0)
#define DBIB_INIT_GUARD_TIME_DISABLE                  (1)

#define IS_DBIB_INIT_GUARD_TIME_CMD(CMD) (((CMD) == DBIB_INIT_GUARD_TIME_ENABLE) || ((CMD) == DBIB_INIT_GUARD_TIME_DISABLE))

/** End of LCDC_DBIB_InitGuardTime_Cmd
  * \}
  */

/**
 * \defgroup    LCDC_DBIB_InitGuardTime LCDC DBIB Init Guard Time
 * \{
 * \ingroup     LCDC_DBIB_Exported_Constants
 */
#define DBIB_INIT_GUARD_TIME_1T                       (0)
#define DBIB_INIT_GUARD_TIME_2T                       ((uint32_t)0x1)
#define DBIB_INIT_GUARD_TIME_3T                       ((uint32_t)0x2)
#define DBIB_INIT_GUARD_TIME_4T                       ((uint32_t)0x3)

#define IS_DBIB_INIT_GUARD_TIME(TIME)   (((TIME) == DBIB_INIT_GUARD_TIME_1T) || ((TIME) == DBIB_INIT_GUARD_TIME_2T) || \
                                         ((TIME) == DBIB_INIT_GUARD_TIME_3T) || ((TIME) == DBIB_INIT_GUARD_TIME_4T))

/** End of LCDC_DBIB_InitGuardTime
  * \}
  */

/**
 * \defgroup    LCDC_DBIB_CmdGuardTime_Cmd LCDC DBIB Cmd Guard Time Cmd
 * \{
 * \ingroup     LCDC_DBIB_Exported_Constants
 */
#define DBIB_CMD_GUARD_TIME_ENABLE                   (0)
#define DBIB_CMD_GUARD_TIME_DISABLE                  (1)

#define IS_DBIB_CMD_GUARD_TIME_CMD(CMD) (((CMD) == DBIB_CMD_GUARD_TIME_ENABLE) || ((CMD) == DBIB_CMD_GUARD_TIME_DISABLE))

/** End of LCDC_DBIB_CmdGuardTime_Cmd
  * \}
  */

/**
 * \defgroup    LCDC_DBIB_CmdGuardTime LCDC DBIB Cmd Guard Time
 * \{
 * \ingroup     LCDC_DBIB_Exported_Constants
 */
#define DBIB_CMD_GUARD_TIME_1T                       (0)
#define DBIB_CMD_GUARD_TIME_2T                       ((uint32_t)0x1)
#define DBIB_CMD_GUARD_TIME_3T                       ((uint32_t)0x2)
#define DBIB_CMD_GUARD_TIME_4T                       ((uint32_t)0x3)

#define IS_DBIB_CMD_GUARD_TIME(TIME)   (((TIME) == DBIB_CMD_GUARD_TIME_1T) || ((TIME) == DBIB_CMD_GUARD_TIME_2T) || \
                                        ((TIME) == DBIB_CMD_GUARD_TIME_3T) || ((TIME) == DBIB_CMD_GUARD_TIME_4T))

/** End of LCDC_DBIB_CmdGuardTime
  * \}
  */

/**
 * \defgroup   LCDC_DBIB_GuardTime_Cmd LCDC DBIB Guard Time Cmd
 * \{
 * \ingroup    LCDC_DBIB_Exported_Constants
 */

#define DBIB_GUARD_TIME_ENABLE                       (0)
#define DBIB_GUARD_TIME_DISABLE                      (1)

#define IS_DBIB_GUARD_TIME_CMD(CMD) (((CMD) == DBIB_GUARD_TIME_ENABLE) || ((CMD) == DBIB_GUARD_TIME_DISABLE))

/** End of LCDC_DBIB_GuardTime_Cmd
  * \}
  */

/**
 * \defgroup    LCDC_DBIB_GuardTime LCDC DBIB Guard Time
 * \{
 * \ingroup     LCDC_DBIB_Exported_Constants
 */

#define DBIB_GUARD_TIME_1T                           (0)
#define DBIB_GUARD_TIME_2T                           ((uint32_t)0x1)
#define DBIB_GUARD_TIME_3T                           ((uint32_t)0x2)
#define DBIB_GUARD_TIME_4T                           ((uint32_t)0x3)

#define IS_DBIB_GUARD_TIME(TIME) (((TIME) == DBIB_GUARD_TIME_1T) || ((TIME) == DBIB_GUARD_TIME_2T) || \
                                  ((TIME) == DBIB_GUARD_TIME_3T) || ((TIME) == DBIB_GUARD_TIME_4T))

/** End of LCDC_DBIB_GuardTime
  * \}
  */

/**
 * \defgroup    LCDC_DBIB_WRDelay LCDC DBIB WR Delay
 * \{
 * \ingroup     LCDC_DBIB_Exported_Constants
 */
#define DBIB_WR_HALF_DELAY          ((uint32_t)0x1)
#define DBIB_WR_ONE_DELAY           ((uint32_t)0x2)

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
  * \brief
  * \{
  */

/**
 * \brief       LCDC DBIB init structure definition.
 *
 * \ingroup     LCDC_DBIB_Exported_Types
 */
typedef struct
{
    uint32_t DBIB_SPEED_SEL;              /*!< Specifies the DBIB clock divider value.
                                                 This parameter can be a value of \ref LCDC_DBIB_Clock_Divider */
    uint32_t DBIB_InitGuardTimeCmd;       /*!< Specifies the init guard time function.
                                                 This parameter can be a value of \ref LCDC_DBIB_InitGuardTime_Cmd */
    uint32_t DBIB_InitGuardTime;          /*!< Specifies the init guard time.
                                                 This parameter can be a value of \ref LCDC_DBIB_InitGuardTime */
    uint32_t DBIB_CmdGuardTimeCmd;        /*!< Specifies the command guard time function.
                                                 This parameter can be a value of \ref LCDC_DBIB_CmdGuardTime_Cmd */
    uint32_t DBIB_CmdGuardTime;           /*!< Specifies the command guard time.
                                                 This parameter can be a value of \ref LCDC_DBIB_CmdGuardTime */
    uint32_t DBIB_GuardTimeCmd;           /*!< Specifies the guard time function. The guard time only avaliable for hardware continuously wtite mode.*/
    uint32_t DBIB_GuardTime;              /*!< Specifies the guard time. This parameter is 0~3T of divider clock. */
    uint32_t DBIB_WRDelay;                /*!< Specifies the delay time for WR. This parameter is half or one bus clock cycle. */
    uint32_t DBIB_8BitSwap;               /*!< Specifies the FIFO data format. */
    uint32_t DBIB_16BitSwap;              /*!< Specifies the FIFO data format. */
    uint32_t DBIB_TxThr;                  /*!< Specifies the TX FIFO threshold value. This value can be from 0 to 16. */
    uint32_t DBIB_TxDMACmd;               /*!< Specifies the TX DMA status in auto mode. */
    uint32_t DBIB_VsyncCmd;               /*!< Specifies the Vsync signal. */
    uint32_t DBIB_VsyncPolarity;          /*!< Specifies the Vsync trigger polarity. */
} LCDC_DBIBCfgTypeDef;

/** End of LCDC_DBIB_Exported_Types
  * \}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** \defgroup LCDC_DBIB_Exported_Functions LCDC DBIB Exported Functions
  * \brief
  * \{
  */

/**
 * rtl_lcdc_dbib.h
 *
 * \brief  Enable or disable command byte bypass in auto mode.
 *
 * \param[in] NewState: New state of command byte bypass.
 *            This parameter can be one of the following values:
 *            \arg ENABLE: Enable command byte bypass.
 *            \arg DISABLE: Disable command byte bypass.
 *
 * \return None.
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
 * rtl_lcdc_dbib.h
 *
 * \brief  Pull down CS to active state.
 *
 * \param  None.
 *
 * \return None.
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
 * rtl_lcdc_dbib.h
 *
 * \brief  Pull up CS to idle state.
 *
 * \param  None.
 *
 * \return None.
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
 * rtl_lcdc_dbib.h
 *
 * \brief  Put DBIB in command mode.
 *
 * \param  None.
 *
 * \return None.
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
 * rtl_lcdc_dbib.h
 *
 * \brief  Put DBIB in data mode.
 *
 * \param  None.
 *
 * \return None.
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
 * rtl_lcdc_dbib.h
 *
 * \brief  Initializes DBIB interface according to the specified parameters in the DBIBCfg.
 *
 * \param[in] DBIBCfg: Pointer to a LCDC_DBIBCfgTypeDef structure that contains the configuration information for DBIB interface.
 *
 * \return None.
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
 * rtl_lcdc_dbib.h
 *
 * \brief  Send command in manual mode.
 *
 * \param[in] cmd: Command which to be sent.
 *
 * \return None.
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
 * rtl_lcdc_dbib.h
 *
 * \brief  Send data in manual mode.
 *
 * \param[in] pBuf: Data buffer for sending.
 * \param[in] len: The length of the data to be sent.
 *
 * \return None.
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
 * rtl_lcdc_dbib.h
 *
 * \brief  Receive data in manual mode.
 *
 * \param[in] pBuf: Data buffer for receiving.
 * \param[in] len: The length of the data to be received.
 *
 * \return None.
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
 * rtl_lcdc_dbib.h
 *
 * \brief  Send command and data in manual mode.
 *
 * \param[in] cmd: Command which to be sent.
 * \param[in] pBuf: Data buffer for sending.
 * \param[in] len: The length of the data to be sent.
 *
 * \return None.
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
 * rtl_lcdc_dbib.h
 *
 * \brief  Send command and read data in manual mode.
 *
 * \param[in] cmd: Command which to be sent.
 * \param[in] pBuf: Data buffer for receiving.
 * \param[in] len: The length of the data to be received.
 *
 * \return None.
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
 * rtl_lcdc_dbib.h
 *
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

