/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* \file     rtl_dsi.c
* \brief    This file provides all the I2C firmware functions.
* \details
* \author   howie wang
* \date     2023-10-17
* \version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "rtl_lcdc_dsi.h"

/*============================================================================*
 *                          Private Macros
 *============================================================================*/
#define DSI_READ32(addr)               HAL_READ32(0, addr)
#define DSI_WRITE32(addr, value32)     HAL_WRITE32(0, addr, value32)

#define DSI_UPDATE32(addr, mask, offset, value)  DSI_WRITE32(addr, (DSI_READ32(addr) &(~(mask<<offset))) | ((value<<offset)&(mask<<offset))   )

/*============================================================================*
 *                           Public Functions
 *============================================================================*/
void DSI_DeInit(DSI_TypeDef *DSIx)
{
    DSIx->PWR_UP = 0x00;    //reset dsi host controller
}
void DSI_Init(DSI_TypeDef *DSIx,  DSI_InitTypeDef *DSI_Init)
{
    DSIx->PWR_UP = 0x00;//3
    DSIx->PHY_MODE = DSI_Init->phy_mode;//4

    DSI_UPDATE32(&DSIx->PHY_IF_CFG, 0x3, 0, (DSI_Init->NumberOfLanes - 1));//5

    DSI_UPDATE32(&DSIx->PHY_IF_CFG, 0xff, 8, DSI_Init->StopWaitTime);//5
    DSI_UPDATE32(&DSIx->INT_MSK0, 0x1fffff, 0, DSI_Init->InterruptMask0);//6
    DSI_UPDATE32(&DSIx->INT_MSK1, 0x1fffff, 0, DSI_Init->InterruptMask1);//7


    DSI_UPDATE32(&DSIx->TO_CNT_CFG, (uint32_t)0xffff, 16, DSI_Init->HS_TX_CNT);//8
    DSI_UPDATE32(&DSIx->TO_CNT_CFG, 0xffff, 0, DSI_Init->LP_RX_CNT);//8

    DSI_UPDATE32(&DSIx->CLKMGR_CFG, 0xff, 8, DSI_Init->TOCkdiv);//9

    DSI_UPDATE32(&DSIx->CLKMGR_CFG, 0xff, 0, DSI_Init->TXEscapeCkdiv);//9

    DSI_UPDATE32(&DSIx->MODE_CFG, 0x1, 0, DSI_Init->ModeCfg);//10

    DSI_UPDATE32(&DSIx->DPI_VCID, 0x3, 0, DSI_Init->VirtualChannelID);//15

    DSI_UPDATE32(&DSIx->DPI_COLOR_CODING, 0x3, 0, DSI_Init->ColorCoding);//16

    DSI_UPDATE32(&DSIx->DPI_CFG_POL, 0x1, 4, DSI_Init->ColorModePolarity);//31
    DSI_UPDATE32(&DSIx->DPI_CFG_POL, 0x1, 3, DSI_Init->ShutDownPolarity);//31
    DSI_UPDATE32(&DSIx->DPI_CFG_POL, 0x1, 2, DSI_Init->HSPolarity);//31
    DSI_UPDATE32(&DSIx->DPI_CFG_POL, 0x1, 1, DSI_Init->VSPolarity);//31
    DSI_UPDATE32(&DSIx->DPI_CFG_POL, 0x1, 0, DSI_Init->DEPolarity);//31
    DSI_UPDATE32(&DSIx->EDPI_CMD_SIZE, 0xFFFF, 0, DSI_Init->CommandSize);//33

    DSI_UPDATE32(&DSIx->CMD_MODE_CFG, 1, 1, DSI_Init->AcknowledgeRequest);//34
    DSI_UPDATE32(&DSIx->HS_RD_TO_CNT, 0xFFFF, 0, DSI_Init->HSGenReadTime);//35
    DSI_UPDATE32(&DSIx->LP_RD_TO_CNT, 0xFFFF, 0, DSI_Init->LPGenReadTime);//36
    DSI_UPDATE32(&DSIx->HS_WR_TO_CNT, 0xFFFF, 0, DSI_Init->HSGenWriteTime);//37
    DSI_UPDATE32(&DSIx->LP_WR_TO_CNT, 0xFFFF, 0, DSI_Init->LPGenWriteTime);//38
    DSI_UPDATE32(&DSIx->BTA_TO_CNT, 0xFFFF, 0, DSI_Init->BTATime);//39
    DSI_UPDATE32(&DSIx->EDPI_TE_HW_CFG, (uint32_t)0xFFFF, 16, DSI_Init->BTATime);//41

    DSI_UPDATE32(&DSIx->PCKHDL_CFG, 0x3F, 0, DSI_Init->PeripherCharacteristics);//42
    DSI_UPDATE32(&DSIx->GEN_VCID, 0x3, 16, DSI_Init->GenericPacketVCID);//43
    DSI_UPDATE32(&DSIx->GEN_VCID, 0x3, 8, DSI_Init->GenericTearVCID);//43
    DSI_UPDATE32(&DSIx->GEN_VCID, 0x3, 0, DSI_Init->GenericReadBackVCID);//43

    DSI_UPDATE32(&DSIx->LPCLK_CTRL, 0x1, 1, DSI_Init->AutoHS2LPEnable);//44
    DSI_UPDATE32(&DSIx->LPCLK_CTRL, 0x1, 0, DSI_Init->TxRequestCLKEnable);//44


    DSI_UPDATE32(&DSIx->PHY_TMR_LPCLK_CFG, 0x3FF, 16, DSI_Init->ClockLaneHS2LPTime);//45
    DSI_UPDATE32(&DSIx->PHY_TMR_LPCLK_CFG, 0x3FF, 0, DSI_Init->ClockLaneLP2HSTime);//45
    DSI_UPDATE32(&DSIx->PHY_TMR_CFG, 0x3FF, 16, DSI_Init->DataLaneHS2LPTime);//46
    DSI_UPDATE32(&DSIx->PHY_TMR_CFG, 0x3FF, 0, DSI_Init->DataLaneLP2HSTime);//46
    DSI_UPDATE32(&DSIx->PHY_TMR_RD_CFG, 0x7FFF, 0, DSI_Init->DataLaneMaxReadTime);//47

    DSI_UPDATE32(&DSIx->PHY_RSTZ, 0x1, 0, DSI_Init->phy_shutdownz);//48
    DSI_UPDATE32(&DSIx->PHY_TST_CTRL0, 0x1, 0, DSI_Init->phy_testclr);//49
    DSI_UPDATE32(&DSIx->PHY_TST_CTRL1, 0x1, 16, DSI_Init->phy_testen);//50
    DSI_UPDATE32(&DSIx->PHY_RSTZ, 0x1, 2, DSI_Init->phy_enableclk);//51
    DSI_UPDATE32(&DSIx->PHY_TST_CTRL1, 0xFF, 8, DSI_Init->phy_testdout);//52
    DSI_UPDATE32(&DSIx->PHY_TST_CTRL1, 0xFF, 0, DSI_Init->phy_testin);//52
    DSI_UPDATE32(&DSIx->PHY_TST_CTRL0, 0x1, 1, DSI_Init->phy_testclk);//53
    DSI_UPDATE32(&DSIx->PHY_RSTZ, 0x1, 0, DSI_Init->phy_shutdownz);//48

    DSI_UPDATE32(&DSIx->PHY_RSTZ, 0x1, 3, DSI_Init->phy_forcepll);//54
    DSI_UPDATE32(&DSIx->PHY_RSTZ, 0x1, 1, DSI_Init->phy_rstz);//55

//    //do verify //57

//    DSI_UPDATE32(&DSIx->AUTO_ULPS_MODE, 0x1, 0, DSI_Init->phy_auto_ulps);//58
//    DSI_UPDATE32(&DSIx->AUTO_ULPS_MODE, 0x1, 16, DSI_Init->phy_pll_off_ulps);//59
//    DSI_UPDATE32(&DSIx->AUTO_ULPS_MODE, 0x1, 17, DSI_Init->phy_pre_pll_off_req);//59

//    DSI_UPDATE32(&DSIx->AUTO_ULPS_ENTRY_DELAY, 0xFFFF, 0, DSI_Init->phy_auto_ulps_delay);//60
//    DSI_UPDATE32(&DSIx->AUTO_ULPS_WAKEUP_TIME, 0xFFFF, 0, DSI_Init->phy_auto_ulps_time);//61
    DSI_UPDATE32(&DSIx->DPI_COLOR_CODING, 0xFFFF, 0, 0x5);

    DSI_UPDATE32(&DSIx->CMD_MODE_CFG, 1, 8, DSI_Init->LPGenShortWriteNoP);//34
    DSI_UPDATE32(&DSIx->CMD_MODE_CFG, 1, 9, DSI_Init->LPGenShortWriteOneP);//34
    DSI_UPDATE32(&DSIx->CMD_MODE_CFG, 1, 10, DSI_Init->LPGenShortWriteTwoP);//34
    DSI_UPDATE32(&DSIx->CMD_MODE_CFG, 1, 11, DSI_Init->LPGenShortReadNoP);//34
    DSI_UPDATE32(&DSIx->CMD_MODE_CFG, 1, 12, DSI_Init->LPGenShortReadOneP);//34
    DSI_UPDATE32(&DSIx->CMD_MODE_CFG, 1, 13, DSI_Init->LPGenShortReadTwoP);//34
    DSI_UPDATE32(&DSIx->CMD_MODE_CFG, 1, 14, DSI_Init->LPGenLongWrite);//34
    DSI_UPDATE32(&DSIx->CMD_MODE_CFG, 1, 16, DSI_Init->LPDcsShortWriteNoP);//34
    DSI_UPDATE32(&DSIx->CMD_MODE_CFG, 1, 17, DSI_Init->LPDcsShortWriteOneP);//34
    DSI_UPDATE32(&DSIx->CMD_MODE_CFG, 1, 18, DSI_Init->LPDcsShortReadNoP);//34
    DSI_UPDATE32(&DSIx->CMD_MODE_CFG, 1, 19, DSI_Init->LPDcsLongWrite);//34

}



void DSI_ConfigVideoMode(DSI_TypeDef *DSIx,  DSI_VidCfgTypeDef *VidCfg)
{
    DSI_UPDATE32(&DSIx->VID_MODE_CFG, 0x3, 0, VidCfg->Mode);//13

    DSI_UPDATE32(&DSIx->VID_MODE_CFG, 0x1, 15, VidCfg->LPCommandEnable);//14
    DSI_UPDATE32(&DSIx->VID_MODE_CFG, 0x1, 14, VidCfg->FrameBTAAcknowledgeEnable);//14
    DSI_UPDATE32(&DSIx->VID_MODE_CFG, 0x1, 8, VidCfg->LPVerticalSyncActiveEnable);//14
    DSI_UPDATE32(&DSIx->VID_MODE_CFG, 0x1, 9, VidCfg->LPVerticalBackPorchEnable);//14
    DSI_UPDATE32(&DSIx->VID_MODE_CFG, 0x1, 10, VidCfg->LPVerticalFrontPorchEnable);//14
    DSI_UPDATE32(&DSIx->VID_MODE_CFG, 0x1, 11, VidCfg->LPVerticalActiveEnable);//14
    DSI_UPDATE32(&DSIx->VID_MODE_CFG, 0x1, 12, VidCfg->LPHorizontalBackPorchEnable);//14
    DSI_UPDATE32(&DSIx->VID_MODE_CFG, 0x1, 13, VidCfg->LPHorizontalFrontPorchEnable);//14

    DSI_UPDATE32(&DSIx->DPI_LP_CMD_TIM, 0xff, 0, VidCfg->invact_lpcmd_size);//17
    DSI_UPDATE32(&DSIx->DPI_LP_CMD_TIM, 0xff, 16, VidCfg->outvact_lpcmd_size);//17

    DSI_UPDATE32(&DSIx->VID_PKT_SIZE, 0x3fff, 0, VidCfg->PacketSize);//18
    DSI_UPDATE32(&DSIx->VID_NUM_CHUNKS, 0x1fff, 0, VidCfg->NumberOfChunks);//19
    DSI_UPDATE32(&DSIx->VID_NULL_SIZE, 0x1fff, 0, VidCfg->NullPacketSize);//20

    DSI_UPDATE32(&DSIx->VID_HLINE_TIME, 0x7fff, 0, VidCfg->HorizontalLine);//21
    DSI_UPDATE32(&DSIx->VID_HSA_TIME, 0xfff, 0, VidCfg->HorizontalSyncActive);//22
    DSI_UPDATE32(&DSIx->VID_HBP_TIME, 0xfff, 0, VidCfg->HorizontalBackPorch);//23
    DSI_UPDATE32(&DSIx->VID_VSA_LINES, 0x3ff, 0, VidCfg->VerticalSyncActive);//24
    DSI_UPDATE32(&DSIx->VID_VBP_LINES, 0x3ff, 0, VidCfg->VerticalBackPorch);//25
    DSI_UPDATE32(&DSIx->VID_VFP_LINES, 0x3ff, 0, VidCfg->VerticalFrontPorch);//26
    DSI_UPDATE32(&DSIx->VID_VACTIVE_LINES, 0x3fff, 0, VidCfg->VerticalActive);//27

    DSI_UPDATE32(&DSIx->VID_MODE_CFG, 0x1, 20, VidCfg->VPGMode);//28
    DSI_UPDATE32(&DSIx->VID_MODE_CFG, 0x1, 24, VidCfg->VPGOrientation);//29
    DSI_UPDATE32(&DSIx->VID_MODE_CFG, 0x1, 16, VidCfg->VPGEnable);//30
}

void DSI_ConfigAdaptedCommandMode(DSI_TypeDef *DSIx,  DSI_AdaptedCmdCfgTypeDef *AdaptedCmdCfg)
{

    DSI_UPDATE32(&DSIx->CMD_MODE_CFG, 1, 0, AdaptedCmdCfg->TEAcknowledgeRequest);//34


    DSI_UPDATE32(&DSIx->EDPI_TE_HW_CFG, (uint32_t)0xFFFF, 16, AdaptedCmdCfg->scan_line_parameter);//40
    DSI_UPDATE32(&DSIx->EDPI_TE_HW_CFG, 0x1, 4, AdaptedCmdCfg->hw_set_scan_line);//40
    DSI_UPDATE32(&DSIx->EDPI_TE_HW_CFG, 0x1, 1, AdaptedCmdCfg->hw_tear_effect_gen);//40
    DSI_UPDATE32(&DSIx->EDPI_TE_HW_CFG, 0x1, 0, AdaptedCmdCfg->hw_tear_effect_on);//40



}

void DSI_ConfigPhy(DSI_TypeDef *DSIx,  DSI_PhyCfgTypeDef *PhyCfg)
{
//    DSI_UPDATE32(&DSIx->PHY_RSTZ, 0x1, 3, PhyCfg->phy_forepll);
//    DSI_UPDATE32(&DSIx->PHY_RSTZ, 0x1, 2, PhyCfg->phy_enableclk);
//    DSI_UPDATE32(&DSIx->PHY_RSTZ, 0x1, 1, PhyCfg->phy_rstz);
//    DSI_UPDATE32(&DSIx->PHY_RSTZ, 0x1, 0, PhyCfg->phy_shutdownz);

    DSI_UPDATE32(&DSIx->PHY_RSTZ, 0xF, 0, 0x04);

    DSI_UPDATE32(&DSIx->PHY_RSTZ, 0xF, 0,
                 PhyCfg->phy_shutdownz << 3 | PhyCfg->phy_enableclk << 2 | PhyCfg->phy_rstz << 1 |
                 PhyCfg->phy_shutdownz);

    while ((DSI->PHY_STATUS & BIT2) == 0U);
    while ((DSI->PHY_STATUS & BIT4) == 0U);
    while ((DSI->PHY_STATUS & BIT7) == 0U);

}

static void DSI_ConfigPacketHeader(DSI_TypeDef *DSIx,
                                   uint32_t ChannelID,
                                   uint32_t DataType,
                                   uint32_t Data0,
                                   uint32_t Data1)
{
    /* Update the DSI packet header with new information */
    DSIx->GEN_HDR = (DataType | (ChannelID << 6U) | (Data0 << 8U) | (Data1 << 16U));
}

FlagStatus DSI_ShortWrite(DSI_TypeDef *DSIx,
                          uint32_t ChannelID,
                          uint32_t Mode,
                          uint32_t Param1,
                          uint32_t Param2)
{
    FlagStatus status = 0;

    /* Wait for Command FIFO Empty */
    while ((DSIx->CMD_PKT_STATUS & BIT0) == 0U); //refs databook page 230

    /* Configure the packet to send a short DCS command with 0 or 1 parameter */
    /* Update the DSI packet header with new information */
    DSIx->GEN_HDR = (Mode | (ChannelID << 6U) | (Param1 << 8U) | (Param2 << 16U));
    return status;
}

FlagStatus DSI_LongWrite(DSI_TypeDef *DSIx,
                         uint32_t ChannelID,
                         uint32_t Mode,
                         uint32_t NbParams,
                         uint32_t Param1,
                         uint8_t *ParametersTable)
{
    FlagStatus status = 0;
    uint32_t uicounter, nbBytes, count;
    uint32_t tickstart;
    uint32_t fifoword;
    uint8_t *pparams = ParametersTable;

    if (NbParams > (200 - 1))
    {
        return 0;
    }
    /* Wait for Command FIFO Empty */
    while ((DSIx->CMD_PKT_STATUS & BIT0) == 0U);

    /* Set the DCS code on payload byte 1, and the other parameters on the write FIFO command*/
    fifoword = Param1;
    nbBytes = (NbParams < 3U) ? NbParams : 3U;

    for (count = 0U; count < nbBytes; count++)
    {
        fifoword |= (((uint32_t)(*(pparams + count))) << (8U + (8U * count)));
    }
    DSIx->GEN_PLD_DATA = fifoword;

    uicounter = NbParams - nbBytes;
    pparams += nbBytes;
    /* Set the Next parameters on the write FIFO command*/
    while (uicounter != 0U)
    {
        nbBytes = (uicounter < 4U) ? uicounter : 4U;
        fifoword = 0U;
        for (count = 0U; count < nbBytes; count++)
        {
            fifoword |= (((uint32_t)(*(pparams + count))) << (8U * count));
        }
        DSIx->GEN_PLD_DATA = fifoword;

        uicounter -= nbBytes;
        pparams += nbBytes;
    }

    /* Configure the packet to send a long DCS command */
    DSI_ConfigPacketHeader(DSIx,
                           ChannelID,
                           Mode,
                           ((NbParams + 1U) & 0x00FFU),
                           (((NbParams + 1U) & 0xFF00U) >> 8U));

    return status;
}

FlagStatus DSI_Start(DSI_TypeDef *DSIx)
{
    FlagStatus status = 0;
    DSIx->PWR_UP = 0x01;//3

    return status;
}

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/

