/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* \file     rtl_lcdc_dbic.h
* \brief    The header file of the peripheral DBIB driver
* \details
* \author   boris yue
* \date     2023-10-17
* \version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_DBIC_H
#define RTL_DBIC_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "lcdc/device/inc/rtl_lcdc.h"
#include "rtl_lcdc_dbic_def.h"


/** \defgroup LCDC        LCDC
  * \brief
  * \{
  */

/** \defgroup LCDC_DBIC        LCDC DBIC
  * \brief
  * \{
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/
/** \defgroup LCDC_DBIC_Exported_Constants LCDC DBIC Exported Constants
  * \brief
  * \{
  */

/**
 * \defgroup    LCDC_DBIC_BIT LCDC DBIC BIT
 * \{
 * \ingroup     LCDC_DBIC_Exported_Constants
 */
#define BIT_CK_MTIMES(x)        (((x) & 0x0000001F) << 23)
#define BIT_FAST_RD(x)          (((x) & 0x00000001) << 22)
#define BIT_CMD_CH(x)           (((x) & 0x00000003) << 20)
#define BIT_DATA_CH(x)          (((x) & 0x00000003) << 18)
#define BIT_ADDR_CH(x)          (((x) & 0x00000003) << 16)
#define BIT_TMOD(x)             (((x) & 0x00000003) << 8)

/** End of LCDC_DBIC_BIT
  * \}
  */

/**
 * \defgroup    LCDC_DBIC_Mode LCDC DBIC Mode
 * \{
 * \ingroup     LCDC_DBIC_Exported_Constants
 */
#define DBIC_AUTO_MODE              ((uint32_t)0x0)
#define DBIC_USER_MODE              ((uint32_t)0x1)
#define IS_DBIC_MODE(mode)          ((mode == DBIC_AUTO_MODE) || (mode == DBIC_USER_MODE))

/** End of LCDC_DBIC_Mode
  * \}
  */

/**
 * \defgroup    LCDC_DBIC_CMD_CHANNEL LCDC DBIC Command Channel
 * \{
 * \ingroup     LCDC_DBIC_Exported_Constants
 */
#define DBIC_CMD_CH_SINGLE          ((uint32_t)0x0)
#define DBIC_CMD_CH_DUAL            ((uint32_t)0x1)
#define DBIC_CMD_CH_QUAD            ((uint32_t)0x2)
#define DBIC_CMD_CH_OCTAL           ((uint32_t)0x3)
#define IS_DBIC_CMD_CH_NUM(num)     ((num == DBIC_CMD_CH_SINGLE) || (num == DBIC_CMD_CH_DUAL) ||\
                                     (num == DBIC_CMD_CH_QUAD) || (num == DBIC_CMD_CH_OCTAL))

/** End of LCDC_DBIC_CMD_CHANNEL
  * \}
  */

/**
 * \defgroup    LCDC_DBIC_DATA_CHANNEL LCDC DBIC Data Channel
 * \{
 * \ingroup     LCDC_DBIC_Exported_Constants
 */
#define DBIC_DATA_CH_SINGLE         ((uint32_t)0x0)
#define DBIC_DATA_CH_DUAL           ((uint32_t)0x1)
#define DBIC_DATA_CH_QUAD           ((uint32_t)0x2)
#define DBIC_DATA_CH_OCTAL          ((uint32_t)0x3)
#define IS_DBIC_DATA_CH_NUM(num)    ((num == DBIC_DATA_CH_SINGLE) || (num == DBIC_DATA_CH_DUAL) ||\
                                     (num == DBIC_DATA_CH_QUAD) || (num == DBIC_DATA_CH_OCTAL))

/** End of LCDC_DBIC_DATA_CHANNEL
  * \}
  */

/**
 * \defgroup    LCDC_DBIC_ADDR_CHANNEL LCDC DBIC Address Channel
 * \{
 * \ingroup     LCDC_DBIC_Exported_Constants
 */
#define DBIC_ADDR_CH_SINGLE         ((uint32_t)0x0)
#define DBIC_ADDR_CH_DUAL           ((uint32_t)0x1)
#define DBIC_ADDR_CH_QUAD           ((uint32_t)0x2)
#define DBIC_ADDR_CH_OCTAL          ((uint32_t)0x3)
#define IS_DBIC_DATA_CH_NUM(num)    ((num == DBIC_DATA_CH_SINGLE) || (num == DBIC_DATA_CH_DUAL) ||\
                                     (num == DBIC_DATA_CH_QUAD) || (num == DBIC_DATA_CH_OCTAL))

/** End of LCDC_DBIC_ADDR_CHANNEL
  * \}
  */

/**
 * \defgroup    LCDC_DBIC_TMODE LCDC DBIC Transfer Mode
 * \{
 * \ingroup     LCDC_DBIC_Exported_Constants
 */
#define DBIC_TMODE_TX               ((uint32_t)0x0)
#define DBIC_TMODE_RX               ((uint32_t)0x3)
#define IS_DBIC_DIR(dir)            ((dir == DBIC_TMODE_TX) || (dir == DBIC_TMODE_RX))

/** End of LCDC_DBIC_TMODE
  * \}
  */

/**
 * \defgroup    LCDC_DBIC_SCPOL LCDC DBIC Serial Clock Polarity
 * \{
 * \ingroup     LCDC_DBIC_Exported_Constants
 */
#define DBIC_SCPOL_LOW              ((uint32_t)0x0)
#define DBIC_SCPOL_HIGH             ((uint32_t)0x1)
#define IS_DBIC_SCPOL(pol)          ((pol == DBIC_SCPOL_LOW) || (pol == DBIC_SCPOL_HIGH))

/** End of LCDC_DBIC_SCPOL
  * \}
  */

/**
 * \defgroup    LCDC_DBIC_SCPH_Edge LCDC DBIC Serial Clock Phase
 * \{
 * \ingroup     LCDC_DBIC_Exported_Constants
 */
#define DBIC_SCPH_1Edge             ((uint32_t)0x0)
#define DBIC_SCPH_2Edge             ((uint32_t)0x1)
#define IS_DBIC_SCPH(phase)         ((phase == DBIC_SCPH_1Edge) || (phase == DBIC_SCPH_2Edge))

/** End of LCDC_DBIC_SCPH_Edge
  * \}
  */

/** End of LCDC_DBIC_Exported_Constants
  * \}
  */

/*============================================================================*
 *                         Types
 *============================================================================*/
/** \defgroup LCDC_DBIC_Exported_Types LCDC DBIC Exported Types
  * \brief
  * \{
  */

/**
 * \brief       LCDC DBIC init structure definition.
 *
 * \ingroup     LCDC_DBIC_Exported_Types
 */
typedef struct
{
    uint32_t DBIC_SPEED_SEL;              /*!< Specifies the DBIC clock divider value.
                                                 The frequency of DBIC clock = The frequency of DBIC clock source / (2 * DBIC_SPEED_SEL) */
    uint32_t DBIC_TxThr;                  /*!< Specifies the TX FIFO threshold value.
                                                  This value can be from 0 to 16. */
    uint32_t DBIC_RxThr;                  /*!< Specifies the RX FIFO threshold value.
                                                  This value can be from 0 to 16. */
    uint32_t SCPOL;                       /*!< Specifies the serial clock polarity.
                                                  This parameter can be a value of @ref LCDC_DBIC_SCPOL */
    uint32_t SCPH;                        /*!< Specifies the serial clock phase.
                                                  This parameter can be a value of @ref LCDC_DBIC_SCPH_Edge */
} LCDC_DBICCfgTypeDef;

/** End of LCDC_DBIC_Exported_Types
  * \}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** \defgroup LCDC_DBIC_Exported_Functions LCDC DBIC Exported Functions
  * \brief
  * \{
  */

/**
 * rtl_lcdc_dbic.h
 *
 * \brief  Enable or disable DBIC interface.
 *
 * \param[in] NewState: New state of DBIC interface.
 *            This parameter can be one of the following values:
 *            \arg ENABLE: Enable DBIC interface.
 *            \arg DISABLE: Disable DBIC interface.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbic_init(void)
 * {
 *     DBIC_Cmd(ENABLE);
 * }
 * \endcode
 */
void DBIC_Cmd(FunctionalState NewState);

/**
 * rtl_lcdc_dbic.h
 *
 * \brief   Switch DBIC interface to user mode or auto mode. It can not be changed while SPIC is busy.
 *
 * \param[in] mode: The mode of DBIC interface.
 *            This parameter can be one of the following values:
 *            \arg DBIC_AUTO_MODE: Auto mode. User can't push data to FIFO and Enable DBIC interface. SPIC will accept auto read/write command.
 *            \arg DBIC_USER_MODE: User mode. User can push data to FIFO and Enable DBIC interface. SPIC won't accept auto read/write command.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbic_init(void)
 * {
 *     DBIC_SwitchMode(DBIC_USER_MODE);
 * }
 * \endcode
 */
void DBIC_SwitchMode(uint32_t mode);

/**
 * rtl_lcdc_dbic.h
 *
 * \brief  Switch the transfer mode.
 *
 * \param[in] dir: The transmission mode of DBIC.
 *            This parameter can be one of the following values:
 *            \arg DBIC_TMODE_TX: Transmit mode.
 *            \arg DBIC_TMODE_RX: Receive mode.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbic_init(void)
 * {
 *     DBIC_SwitchDirect(DBIC_TMODE_TX);
 * }
 * \endcode
 */
void DBIC_SwitchDirect(uint32_t dir);

/**
 * rtl_lcdc_dbic.h
 *
 * \brief  Configure number of bytes in command phase in user mode.
 *
 * \param[in] len: Length of command in bytes.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbic_init(void)
 * {
 *     DBIC_CmdLength(1);
 * }
 * \endcode
 */
void DBIC_CmdLength(uint32_t len);

/**
 * rtl_lcdc_dbic.h
 *
 * \brief  Configure number of bytes in address phase (between command phase and write/read phase) in user mode.
 *
 * \param[in] len: Length of address in bytes.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbic_init(void)
 * {
 *     DBIC_AddrLength(3);
 * }
 * \endcode
 */
void DBIC_AddrLength(uint32_t len);

/**
 * rtl_lcdc_dbic.h
 *
 * \brief  Select current SPIC.
 *
 * \param  None.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbic_init(void)
 * {
 *     DBIC_Select();
 * }
 * \endcode
 */
void DBIC_Select(void);

/**
 * rtl_lcdc_dbic.h
 *
 * \brief  Configure the number of data frames in bytes. After transmitting command and address,
 *         SPIC transmits data continuously until data frames are equal to the set length.
 *
 * \param[in] len: Length of data frames in bytes.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbic_init(void)
 * {
 *     DBIC_TX_NDF(466);
 * }
 * \endcode
 */
void DBIC_TX_NDF(uint32_t len);

/**
 * rtl_lcdc_dbic.h
 *
 * \brief  Initializes DBIC interface according to the specified parameters in the DBICCfg.
 *
 * \param[in] DBICCfg: Pointer to a LCDC_DBICCfgTypeDef structure that contains the configuration information for DBIC interface.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbic_init(void)
 * {
 *     LCDC_DBICCfgTypeDef dbic_init = {0};
 *     dbic_init.DBIC_SPEED_SEL         = 2;
 *     dbic_init.DBIC_TxThr             = 0;
 *     dbic_init.DBIC_RxThr             = 0;
 *     dbic_init.SCPOL                  = DBIC_SCPOL_LOW;
 *     dbic_init.SCPH                   = DBIC_SCPH_1Edge;
 *     DBIC_Init(&dbic_init);
 * }
 * \endcode
 */
void DBIC_Init(LCDC_DBICCfgTypeDef *DBICCfg);

/**
 * rtl_lcdc_dbic.h
 *
 * \brief  Send data through DBIC interface.
 *
 * \param[in] buf: Data buffer for sending.
 * \param[in] len: The length of the data to be sent.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbic_init(void)
 * {
 *     uint8_t buf[100];
 *     memset(buf, 100, 0xA5);
 *     DBIC_SendBuf(buf, 100);
 * }
 * \endcode
 */
void DBIC_SendBuf(uint8_t *buf, uint32_t len);

/**
 * rtl_lcdc_dbic.h
 *
 * \brief  Read data through DBIC interface.
 *
 * \param[in] addr: The address to be read.
 * \param[in] data_len: The length of the data to be received.
 * \param[in] data: Data buffer for receiving.
 * \param[in] rd_dummy_len: Delay cycles for receiving data in user mode.
 *
 * \return None.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbic_init(void)
 * {
 *     uint8_t buf[4] = {0, 0, 0, 0};
 *     DBIC_ReceiveBuf(0x09, 4, buf, 0);
 * }
 * \endcode
 */
void DBIC_ReceiveBuf(uint16_t addr, uint16_t data_len, uint8_t *data, uint16_t rd_dummy_len);

/** End of LCDC_DBIC_Exported_Functions
  * \}
  */

/** End of LCDC_DBIC
  * \}
  */

/** End of LCDC
  * \}
  */

#ifdef __cplusplus
}
#endif

#endif /*RTL_DBIC_H*/

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/

