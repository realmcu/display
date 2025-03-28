/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* \file     rtl_lcdc_dbic.h
* \brief    The header file of the peripheral LCDC DBIC driver
* \details  This file provides all LCDC DBIC firmware functions including initialization,
*           configuration, data transmission and reception.
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
#include "rtl_lcdc.h"
#include "rtl_lcdc_dbic_def.h"

/** \defgroup LCDC        LCDC
  * \brief    LCD Controller driver module
  * \{
  */

/** \defgroup LCDC_DBIC        LCDC DBIC
  * \brief    LCD Controller Display Bus Interface Controller
  * \{
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/

/**
 * \cond        private
 * \brief       Bit fields of DBIC.
 * \defgroup    BIT_FIELD_DBIC   DBIC Bit Field
 * \{
 */
#define BIT_CK_MTIMES(x)        (((x) & 0x0000001F) << 23)
#define BIT_FAST_RD(x)          (((x) & 0x00000001) << 22)
#define BIT_CMD_CH(x)           (((x) & 0x00000003) << 20)
#define BIT_DATA_CH(x)          (((x) & 0x00000003) << 18)
#define BIT_ADDR_CH(x)          (((x) & 0x00000003) << 16)
#define BIT_TMOD(x)             (((x) & 0x00000003) << 8)
/**
 * \}
 * \endcond
 */

/** \defgroup LCDC_DBIC_Exported_Constants LCDC DBIC Exported Constants
 * \brief    Constants used by the LCDC DBIC driver
 * \{
 */

/**
 * \defgroup    LCDC_DBIC_Mode LCDC DBIC Mode
 * \{
 * \ingroup     LCDC_DBIC_Exported_Constants
 */
#define DBIC_AUTO_MODE              ((uint32_t)0x0)     /*!< DBIC interface work in auto mode, user can't push data to FIFO and set SSIENR to 1. SPIC will accept auto read/write command. */
#define DBIC_USER_MODE              ((uint32_t)0x1)     /*!< DBIC interface work in user mode, user can push data to FIFO and set SSIENR to 1. SPIC won't accept auto read/write command.*/
#define IS_DBIC_MODE(mode)          (((mode) == DBIC_AUTO_MODE) || ((mode) == DBIC_USER_MODE))

/** End of LCDC_DBIC_Mode
  * \}
  */

/**
 * \defgroup    LCDC_DBIC_CHANNEL_NUM LCDC DBIC Channel Number
 * \{
 * \ingroup     LCDC_DBIC_Exported_Constants
 */
#define DBIC_CH_SINGLE         ((uint32_t)0x0)   /*!< Transmit command/address/data through single data lane. */
#define DBIC_CH_DUAL           ((uint32_t)0x1)   /*!< Transmit command/address/data through dual data lanes. */
#define DBIC_CH_QUAD           ((uint32_t)0x2)   /*!< Transmit command/address/data through quad data lanes. */
#define DBIC_CH_OCTAL          ((uint32_t)0x3)   /*!< Transmit command/address/data through octal data lanes. */
#define IS_DBIC_CH_NUM(num)    (((num) == DBIC_CH_SINGLE) || ((num) == DBIC_CH_DUAL) ||\
                                ((num) == DBIC_CH_QUAD) || ((num) == DBIC_CH_OCTAL))

/** End of LCDC_DBIC_CHANNEL_NUM
  * \}
  */

/**
 * \defgroup    LCDC_DBIC_TMODE LCDC DBIC Transfer Mode
 * \{
 * \ingroup     LCDC_DBIC_Exported_Constants
 */
#define DBIC_TMODE_TX               ((uint32_t)0x0)     /*!< Transmit mode. */
#define DBIC_TMODE_RX               ((uint32_t)0x3)     /*!< Receive mode. */
#define IS_DBIC_DIR(dir)            (((dir) == DBIC_TMODE_TX) || ((dir) == DBIC_TMODE_RX))

/** End of LCDC_DBIC_TMODE
  * \}
  */

/**
 * \defgroup    LCDC_DBIC_SCPOL LCDC DBIC Serial Clock Polarity
 * \{
 * \ingroup     LCDC_DBIC_Exported_Constants
 */
#define DBIC_SCPOL_LOW              ((uint32_t)0x0)     /*!< Inactive state of serial clock is low. */
#define DBIC_SCPOL_HIGH             ((uint32_t)0x1)     /*!< Inactive state of serial clock is high. */
#define IS_DBIC_SCPOL(pol)          (((pol) == DBIC_SCPOL_LOW) || ((pol) == DBIC_SCPOL_HIGH))

/** End of LCDC_DBIC_SCPOL
  * \}
  */

/**
 * \defgroup    LCDC_DBIC_SCPH_Edge LCDC DBIC Serial Clock Phase
 * \{
 * \ingroup     LCDC_DBIC_Exported_Constants
 */
#define DBIC_SCPH_1Edge             ((uint32_t)0x0)    /*!< Serial clock toggles in middle of first data bit. */
#define DBIC_SCPH_2Edge             ((uint32_t)0x1)    /*!< Serial clock toggles at start of first data bit. */
#define IS_DBIC_SCPH(phase)         (((phase) == DBIC_SCPH_1Edge) || ((phase) == DBIC_SCPH_2Edge))

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
  * \brief    Types provided by the LCDC DBIC driver
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

#ifdef TARGET_RTL87x3EU
typedef enum
{
    DCX_IMDEPENDENT = 0x0,
    DCX_CRTL_1BIT,
    DCX_CRTL_2BIT,
    DCX_CRTL_4BIT,
    DCX_CRTL_8BIT,
    DCX_CRTL_16BIT,
} LCDC_DBIC_DCX_CTRL_BIT;

typedef enum
{
    DCX_CTRL_BIT_ONCE = 0x0,
    DCX_CTRL_BIT_EACH_1BYTE,
    DCX_CTRL_BIT_EACH_2BYTE,
    DCX_CTRL_BIT_EACH_4BYTE,
    DCX_CTRL_BIT_EACH_8BYTE,
    DCX_CTRL_BIT_EACH_16BYTE,
} LCDC_DBIC_DCX_CTRL;

typedef struct rtl_lcdc_dbic
{
    FunctionalState             cmd_head_en;
    FunctionalState             cmd_tail_en;
    FunctionalState             addr_head_en;
    FunctionalState             addr_tail_en;
    FunctionalState             data_head_en;
    FunctionalState             data_tail_en;
    LCDC_DBIC_DCX_CTRL_BIT      cmd_ctrl_bit;
    LCDC_DBIC_DCX_CTRL          cmd_ctrl;
    LCDC_DBIC_DCX_CTRL_BIT      addr_ctrl_bit;
    LCDC_DBIC_DCX_CTRL          addr_ctrl;
    LCDC_DBIC_DCX_CTRL_BIT      data_ctrl_bit;
    LCDC_DBIC_DCX_CTRL          data_ctrl;
    uint16_t                    cmd_head_value;
    uint16_t                    cmd_tail_value;
    uint16_t                    addr_head_value;
    uint16_t                    addr_tail_value;
    uint16_t                    data_head_value;
    uint16_t                    data_tail_value;
} LCDC_DBIC_9BIT_TypeDef;

#endif

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** \defgroup LCDC_DBIC_Exported_Functions LCDC DBIC Exported Functions
  * \brief    Functions provided by the LCDC DBIC driver
  * \{
  */

/**
 * \brief  Enable or disable DBIC interface.
 *
 * \param[in] NewState: New state of DBIC interface.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable DBIC interface.
 *            - DISABLE: Disable DBIC interface.
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
 * \brief   Switch DBIC interface to user mode or auto mode. It can not be changed while SPIC is busy.
 *
 * \param[in] mode: The mode of DBIC interface.
 *            This parameter can be one of the following values:
 *            - DBIC_AUTO_MODE: Auto mode. User can't push data to FIFO and Enable DBIC interface. SPIC will accept auto read/write command.
 *            - DBIC_USER_MODE: User mode. User can push data to FIFO and Enable DBIC interface. SPIC won't accept auto read/write command.
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
 * \brief  Switch the transfer mode.
 *
 * \param[in] dir: The transmission mode of DBIC.
 *            This parameter can be one of the following values:
 *            - DBIC_TMODE_TX: Transmit mode.
 *            - DBIC_TMODE_RX: Receive mode.
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
 * \brief  Configure number of bytes in command phase in user mode.
 *
 * \param[in] len: Length of command in bytes.
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
 * \brief  Configure number of bytes in address phase (between command phase and write/read phase) in user mode.
 *
 * \param[in] len: Length of address in bytes.
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
 * \brief  Select current SPIC.
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
 * \brief  Configure the number of data frames in bytes. After transmitting command and address,
 *         SPIC transmits data continuously until data frames are equal to the set length.
 *
 * \param[in] len: Length of data frames in bytes.
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
 * \brief  Initialize the LCDC_DBICCfgTypeDef structure with default values.
 *
 * \param[in] DBIC_InitStruct: Pointer to a LCDC_DBICCfgTypeDef structure that will be initialized.
 *
 * \note Default values for each member:
 *       | Member           | Default Value        | Description                  |
 *       |------------------|----------------------|------------------------------|
 *       | DBIC_SPEED_SEL   | 1                    | Clock divider value          |
 *       | DBIC_TxThr       | 0                    | TX FIFO threshold            |
 *       | DBIC_RxThr       | 0                    | RX FIFO threshold            |
 *       | SCPOL            | \ref DBIC_SCPOL_LOW  | Serial clock polarity        |
 *       | SCPH             | \ref DBIC_SCPH_1Edge | Serial clock phase           |
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_dbic_init(void)
 * {
 *     LCDC_DBICCfgTypeDef dbic_init;
 *     DBIC_StructInit(&dbic_init);
 *     DBIC_Init(&dbic_init);
 * }
 * \endcode
 */
void DBIC_StructInit(LCDC_DBICCfgTypeDef *DBIC_InitStruct);

/**
 * \brief  Initializes DBIC interface according to the specified parameters in the DBICCfg.
 *
 * \param[in] DBIC_InitStruct: Pointer to a LCDC_DBICCfgTypeDef structure that contains the configuration information for DBIC interface.
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
void DBIC_Init(LCDC_DBICCfgTypeDef *DBIC_InitStruct);

/**
 * \brief  Send data through DBIC interface.
 *
 * \param[in] buf: Data buffer for sending.
 * \param[in] len: The length of the data to be sent.
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
 * \brief  Read data through DBIC interface.
 *
 * \param[in] addr: The address to be read.
 * \param[in] data_len: The length of the data to be received.
 * \param[out] data: Data buffer for receiving.
 * \param[in] rd_dummy_len: Delay cycles for receiving data in user mode.
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

