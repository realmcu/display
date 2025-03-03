/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* \file     rtl_dsi.h
* \brief    This file provides all the I2C firmware functions.
* \details
* \author   howie wang
* \date     2023-10-17
* \version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_DSI_H
#define RTL_DSI_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/

/*============================================================================*
 *                         DSI Registers Memory Map
 *============================================================================*/
typedef struct
{
    __IO uint32_t VERSION;
    __IO uint32_t PWR_UP;
    __IO uint32_t CLKMGR_CFG;
    __IO uint32_t DPI_VCID;
    __IO uint32_t DPI_COLOR_CODING ;
    __IO uint32_t DPI_CFG_POL ;
    __IO uint32_t DPI_LP_CMD_TIM ;
    uint32_t RESERVED0[4];/*!< Reserved, 0x1c - 0x28 */
    __IO uint32_t PCKHDL_CFG ;
    __IO uint32_t GEN_VCID ;
    __IO uint32_t MODE_CFG ;
    __IO uint32_t VID_MODE_CFG ;
    __IO uint32_t VID_PKT_SIZE ;
    __IO uint32_t VID_NUM_CHUNKS ;
    __IO uint32_t VID_NULL_SIZE ;
    __IO uint32_t VID_HSA_TIME ;
    __IO uint32_t VID_HBP_TIME ;
    __IO uint32_t VID_HLINE_TIME ;
    __IO uint32_t VID_VSA_LINES ;
    __IO uint32_t VID_VBP_LINES ;
    __IO uint32_t VID_VFP_LINES ;
    __IO uint32_t VID_VACTIVE_LINES ;
    __IO uint32_t EDPI_CMD_SIZE ;
    __IO uint32_t CMD_MODE_CFG ;
    __IO uint32_t GEN_HDR ;
    __IO uint32_t GEN_PLD_DATA ;
    __IO uint32_t CMD_PKT_STATUS ;
    __IO uint32_t TO_CNT_CFG ;
    __IO uint32_t HS_RD_TO_CNT ;
    __IO uint32_t LP_RD_TO_CNT ;
    __IO uint32_t HS_WR_TO_CNT ;
    __IO uint32_t LP_WR_TO_CNT ;
    __IO uint32_t BTA_TO_CNT ;
    uint32_t RESERVED1;/*!< Reserved, 0x90 */
    __IO uint32_t LPCLK_CTRL ;
    __IO uint32_t PHY_TMR_LPCLK_CFG ;
    __IO uint32_t PHY_TMR_CFG ;
    __IO uint32_t PHY_RSTZ ;
    __IO uint32_t PHY_IF_CFG ;
    __IO uint32_t PHY_ULPS_CTRL ;
    __IO uint32_t PHY_TX_TRIGGERS ;
    __IO uint32_t PHY_STATUS ;
    __IO uint32_t PHY_TST_CTRL0 ;
    __IO uint32_t PHY_TST_CTRL1 ;
    __IO uint32_t INT_ST0 ;
    __IO uint32_t INT_ST1 ;
    __IO uint32_t INT_MSK0 ;
    __IO uint32_t INT_MSK1 ;
    __IO uint32_t PHY_CAL ;
    uint32_t RESERVED2[2];/*!< Reserved, 0xC0,0XD4 */
    __IO uint32_t INT_FORCE0 ;
    __IO uint32_t INT_FORCE1 ;
    __IO uint32_t AUTO_ULPS_MODE ;
    __IO uint32_t AUTO_ULPS_ENTRY_DELAY ;
    __IO uint32_t AUTO_ULPS_WAKEUP_TIME ;
    uint32_t RESERVED3;/*!< Reserved, 0xec */
    __IO uint32_t DSC_PARAMETER ;
    __IO uint32_t PHY_TMR_RD_CFG ;
    __IO uint32_t AUTO_ULPS_MIN_TIME ;
    __IO uint32_t PHY_MODE ;
    __IO uint32_t VID_SHADOW_CTRL ;
    uint32_t RESERVED4[2];/*!< Reserved, 0x104,0x108 */
    __IO uint32_t DPI_VCID_ACT ;
    __IO uint32_t DPI_COLOR_CODING_ACT ;
    uint32_t RESERVED5;/*!< Reserved, 0x114 */
    __IO uint32_t DPI_LP_CMD_TIM_ACT ;
    __IO uint32_t EDPI_TE_HW_CFG ;
    uint32_t RESERVED6[6];/*!< Reserved, 0x120-0x134 */
    __IO uint32_t VID_MODE_CFG_ACT ;
    __IO uint32_t VID_PKT_SIZE_ACT ;
    __IO uint32_t VID_NUM_CHUNKS_ACT ;
    __IO uint32_t VID_NULL_SIZE_ACT ;
    __IO uint32_t VID_HSA_TIME_ACT ;
    __IO uint32_t VID_HBP_TIME_ACT ;
    __IO uint32_t VID_HLINE_TIME_ACT ;
    __IO uint32_t VID_VSA_LINES_ACT ;
    __IO uint32_t VID_VBP_LINES_ACT ;
    __IO uint32_t VID_VFP_LINES_ACT ;
    __IO uint32_t VID_VACTIVE_LINES_ACT ;
    uint32_t RESERVED7;/*!< Reserved, 0x164 */
    __IO uint32_t VID_PKT_STATUS ;
    uint32_t RESERVED8[74]; /*!< Reserved, 0x190 - 0x2B4 */
} DSI_TypeDef;

/*============================================================================*
 *                         DSI Declaration
 *============================================================================*/
#define DSI                            ((DSI_TypeDef           *) MIPI_REG_BASE)

/** \defgroup 87X2G_DSI         DSI
  * \brief
  * \{
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/
/** \defgroup DSI_Exported_Constants DSI Exported Constants
  * \brief
  * \{
  */

/**
 * \defgroup    DSI_Mode DSI Mode
 * \{
 * \ingroup     DSI_Exported_Constants
 */
#define DSI_MODE_VIDEO      0
#define DSI_MODE_CMD        1

/** End of DSI_Mode
  * \}
  */

/**
 * \defgroup    DSI_LP DSI LP
 * \{
 * \ingroup     DSI_Exported_Constants
 */
#define DSI_LP_GSW0P_DISABLE     0
#define DSI_LP_GSW0P_ENABLE      1
#define DSI_LP_GSW1P_DISABLE     0
#define DSI_LP_GSW1P_ENABLE      1
#define DSI_LP_GSW2P_DISABLE     0
#define DSI_LP_GSW2P_ENABLE      1
#define DSI_LP_GSR0P_DISABLE     0
#define DSI_LP_GSR0P_ENABLE      1
#define DSI_LP_GSR1P_DISABLE     0
#define DSI_LP_GSR1P_ENABLE      1
#define DSI_LP_GSR2P_DISABLE     0
#define DSI_LP_GSR2P_ENABLE      1
#define DSI_LP_GLW_DISABLE       0
#define DSI_LP_GLW_ENABLE        1
#define DSI_LP_DSW0P_DISABLE     0
#define DSI_LP_DSW0P_ENABLE      1
#define DSI_LP_DSW1P_DISABLE     0
#define DSI_LP_DSW1P_ENABLE      1
#define DSI_LP_DSR0P_DISABLE     0
#define DSI_LP_DSR0P_ENABLE      1
#define DSI_LP_DLW_DISABLE       0
#define DSI_LP_DLW_ENABLE        1
#define DSI_LP_MRDP_DISABLE      0
#define DSI_LP_MRDP_ENABLE       1

/** End of DSI_LP
  * \}
  */

/**
 * \defgroup    DSI_ACKNOWLEDGE DSI ACKNOWLEDGE
 * \{
 * \ingroup     DSI_Exported_Constants
 */
#define DSI_TE_ACKNOWLEDGE_DISABLE 0
#define DSI_TE_ACKNOWLEDGE_ENABLE  1
#define DSI_ACKNOWLEDGE_DISABLE   0
#define DSI_ACKNOWLEDGE_ENABLE    1

/** End of DSI_ACKNOWLEDGE
  * \}
  */

/**
 * \defgroup    DSI_SHORT_WRITE_PKT_Data_Type DSI SHORT WRITE PKT Data Type
 * \{
 * \ingroup     DSI_Exported_Constants
 */
#define DSI_DCS_SHORT_PKT_WRITE_P0  0x00000005U /*!< DCS short write, no parameters      */
#define DSI_DCS_SHORT_PKT_WRITE_P1  0x00000015U /*!< DCS short write, one parameter      */
#define DSI_GEN_SHORT_PKT_WRITE_P0  0x00000003U /*!< Generic short write, no parameters  */
#define DSI_GEN_SHORT_PKT_WRITE_P1  0x00000013U /*!< Generic short write, one parameter  */
#define DSI_GEN_SHORT_PKT_WRITE_P2  0x00000023U /*!< Generic short write, two parameters */

/** End of DSI_SHORT_WRITE_PKT_Data_Type
  * \}
  */

/**
 * \defgroup    DSI_LONG_WRITE_PKT_Data_Type DSI LONG WRITE PKT Data Type
 * \{
 * \ingroup     DSI_Exported_Constants
 */
#define DSI_DCS_LONG_PKT_WRITE      0x00000039U /*!< DCS long write     */
#define DSI_GEN_LONG_PKT_WRITE      0x00000029U /*!< Generic long write */

/** End of DSI_LONG_WRITE_PKT_Data_Type
  * \}
  */

/**
 * \defgroup    DSI_SHORT_READ_PKT_Data_Type DSI SHORT READ PKT Data Type
 * \{
 * \ingroup     DSI_Exported_Constants
 */
#define DSI_DCS_SHORT_PKT_READ      0x00000006U /*!< DCS short read                     */
#define DSI_GEN_SHORT_PKT_READ_P0   0x00000004U /*!< Generic short read, no parameters  */
#define DSI_GEN_SHORT_PKT_READ_P1   0x00000014U /*!< Generic short read, one parameter  */
#define DSI_GEN_SHORT_PKT_READ_P2   0x00000024U /*!< Generic short read, two parameters */

/** End of DSI_SHORT_READ_PKT_Data_Type
  * \}
  */

/**
 * \defgroup    DSI_BURST DSI BURST
 * \{
 * \ingroup     DSI_Exported_Constants
 */
#define VIDEO_NON_BURST_WITH_SYNC_PULSES    0
#define VIDEO_NON_BURST_WITH_SYNC_EVENTS    1
#define VIDEO_BURST_MODE_2    2
#define VIDEO_BURST_MODE_3    3

/** End of DSI_BURST
  * \}
  */

/**
 * \defgroup    DSI_VPG DSI VPG
 * \{
 * \ingroup     DSI_Exported_Constants
 */
#define VIDEO_VPG_MODE_V_AND_H    0
#define VIDEO_VPG_MODE_ONLY_V     1

#define VIDEO_VPG_ORIENTATION_V    0
#define VIDEO_VPG_ORIENTATION_H    1

/** End of DSI_VPG
  * \}
  */

/**
 * \defgroup    DSI_PHY DSI PHY
 * \{
 * \ingroup     DSI_Exported_Constants
 */
#define PHY_FORCEPLL_ENABLE     1
#define PHY_FORCEPLL_DISABLE    0
#define PHY_CLK_ENABLE     1
#define PHY_CLK_DISABLE    0
#define PHY_RST_ENABLE     1
#define PHY_RST_DISABLE    0
#define PHY_SHUTDOWN_ENABLE     1
#define PHY_SHUTDOWN_DISABLE    0

/** End of DSI_PHY
  * \}
  */

/** End of DSI_Exported_Constants
  * \}
  */

/*============================================================================*
 *                         Types
 *============================================================================*/
/** \defgroup DSI_Exported_Types DSI Exported Types
  * \brief
  * \{
  */

/**
 * \brief       DSI init structure definition.
 *
 * \ingroup     DSI_Exported_Types
 */
typedef struct
{
    uint32_t phy_mode;
    uint32_t ColorCoding;
    uint32_t NumberOfLanes;
    uint32_t StopWaitTime;
    uint32_t InterruptMask0;
    uint32_t InterruptMask1;
    uint32_t HS_TX_CNT;                 //for trigger timeout error measured in TO_CLK_DIVISION
    uint32_t LP_RX_CNT;                 //for trigger timeout error measured in TO_CLK_DIVISION
    uint32_t TOCkdiv;
    uint32_t TXEscapeCkdiv;
    uint32_t ModeCfg;                   //config for video mode or cmd mode

    uint32_t PeripherCharacteristics;
    uint32_t GenericPacketVCID;
    uint32_t GenericTearVCID;
    uint32_t GenericReadBackVCID;

    uint32_t AutoHS2LPEnable;
    uint32_t TxRequestCLKEnable;

    uint32_t ClockLaneHS2LPTime;
    uint32_t ClockLaneLP2HSTime;
    uint32_t DataLaneHS2LPTime;
    uint32_t DataLaneLP2HSTime;
    uint32_t DataLaneMaxReadTime;
    uint32_t VirtualChannelID;


    uint8_t phy_shutdownz;
    uint8_t phy_testclr;
    uint8_t phy_testen;
    uint8_t phy_enableclk;
    uint8_t phy_forcepll;

    uint8_t phy_rstz;
    uint8_t phy_testclk;

    uint8_t phy_testdout;
    uint8_t phy_testin;

    uint8_t phy_auto_ulps;
    uint8_t phy_pll_off_ulps;
    uint8_t phy_pre_pll_off_req;
    uint8_t phy_auto_ulps_delay;
    uint8_t phy_auto_ulps_time;

    uint32_t ColorModePolarity;
    uint32_t ShutDownPolarity;
    uint32_t HSPolarity;
    uint32_t VSPolarity;
    uint32_t DEPolarity;
    uint32_t CommandSize;

    uint32_t LPMaxReadPacket;
    uint32_t LPGenReadTime;
    uint32_t HSGenReadTime;
    uint32_t LPGenWriteTime;
    uint32_t HSGenWriteTime;
    uint32_t BTATime;
    uint32_t AcknowledgeRequest;

    uint32_t LPGenShortWriteNoP;
    uint32_t LPGenShortWriteOneP;
    uint32_t LPGenShortWriteTwoP;
    uint32_t LPGenShortReadNoP;
    uint32_t LPGenShortReadOneP;
    uint32_t LPGenShortReadTwoP;
    uint32_t LPGenLongWrite;
    uint32_t LPDcsShortWriteNoP;
    uint32_t LPDcsShortWriteOneP;
    uint32_t LPDcsShortReadNoP;
    uint32_t LPDcsLongWrite;

} DSI_InitTypeDef;

typedef struct
{
    uint32_t Mode;//for brust or none brust
    uint32_t LPCommandEnable;
    uint32_t FrameBTAAcknowledgeEnable;
    uint32_t LPHorizontalFrontPorchEnable;
    uint32_t LPHorizontalBackPorchEnable;
    uint32_t LPVerticalActiveEnable;
    uint32_t LPVerticalFrontPorchEnable;
    uint32_t LPVerticalBackPorchEnable;
    uint32_t LPVerticalSyncActiveEnable;

    uint32_t invact_lpcmd_size;
    uint32_t outvact_lpcmd_size;

    uint32_t PacketSize;
    uint32_t NumberOfChunks;               /*!< Number of chunks                                                   */
    uint32_t NullPacketSize;               /*!< Null packet size                                                   */

    uint32_t HorizontalSyncActive;         /*!< Horizontal synchronism active duration (in lane byte clock cycles) */
    uint32_t HorizontalBackPorch;          /*!< Horizontal back-porch duration (in lane byte clock cycles)         */
    uint32_t HorizontalLine;               /*!< Horizontal line duration (in lane byte clock cycles)               */
    uint32_t VerticalSyncActive;           /*!< Vertical synchronism active duration                               */
    uint32_t VerticalBackPorch;            /*!< Vertical back-porch duration                                       */
    uint32_t VerticalFrontPorch;           /*!< Vertical front-porch duration                                      */
    uint32_t VerticalActive;               /*!< Vertical active duration                                           */

    uint32_t VPGOrientation;
    uint32_t VPGMode;
    uint32_t VPGEnable;

} DSI_VidCfgTypeDef;

typedef struct
{
    uint32_t TEAcknowledgeRequest;

    uint32_t scan_line_parameter;//for te
    uint32_t hw_set_scan_line;//for te
    uint32_t hw_tear_effect_gen;
    uint32_t hw_tear_effect_on;

} DSI_AdaptedCmdCfgTypeDef;

typedef struct
{
    uint32_t phy_forepll;
    uint32_t phy_enableclk;
    uint32_t phy_rstz;
    uint32_t phy_shutdownz;

//    uint32_t ClockLaneHS2LPTime;
//    uint32_t ClockLaneLP2HSTime;
//    uint32_t DataLaneHS2LPTime;
//    uint32_t DataLaneLP2HSTime;
//    uint32_t DataLaneMaxReadTime;
//    uint32_t StopWaitTime;
} DSI_PhyCfgTypeDef;

/** End of DSI_Exported_Types
  * \}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** \defgroup DSI_Exported_Functions DSI Exported Functions
  * \brief
  * \{
  */

void DSI_Init(DSI_TypeDef *DSIx,  DSI_InitTypeDef *DSI_Init);
void DSI_ConfigVideoMode(DSI_TypeDef *DSIx,  DSI_VidCfgTypeDef *VidCfg);

/** End of DSI_Exported_Functions
  * \}
  */

/** End of DSI
  * \}
  */

#ifdef __cplusplus
}
#endif

#endif /*RTL_DSI_H*/


/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/

