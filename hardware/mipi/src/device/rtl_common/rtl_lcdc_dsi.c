/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "rtl_lcdc_dsi.h"

/*============================================================================*
 *                          Private Functions
 *============================================================================*/
static FlagStatus DSI_WaitCmdFifoNotFull(DSI_TypeDef *DSIx)
{
    uint32_t timeout = DSI_POLL_TIMEOUT;

    while (timeout-- != 0U)
    {
        DSI_CMD_PKT_STATUS_TypeDef status = {.d32 = DSIx->CMD_PKT_STATUS};
        if (status.b.gen_cmd_full == 0U)
        {
            return DSI_STATUS_OK;
        }
    }

    return DSI_STATUS_ERROR;
}

static FlagStatus DSI_WaitPayloadFifoNotFull(DSI_TypeDef *DSIx)
{
    uint32_t timeout = DSI_POLL_TIMEOUT;

    while (timeout-- != 0U)
    {
        DSI_CMD_PKT_STATUS_TypeDef status = {.d32 = DSIx->CMD_PKT_STATUS};
        if (status.b.gen_pld_w_full == 0U)
        {
            return DSI_STATUS_OK;
        }
    }

    return DSI_STATUS_ERROR;
}

static FlagStatus DSI_WaitPhyStopState(DSI_TypeDef *DSIx)
{
    uint32_t timeout = DSI_POLL_TIMEOUT;
    uint32_t lane_count;
    DSI_PHY_IF_CFG_TypeDef phy_if = {.d32 = DSIx->PHY_IF_CFG};

    lane_count = phy_if.b.n_lanes + 1U;
    while (timeout-- != 0U)
    {
        DSI_PHY_STATUS_TypeDef status = {.d32 = DSIx->PHY_STATUS};

        if ((status.b.phy_stopstateclklane != 0U) &&
            (status.b.phy_stopstate0lane != 0U) &&
            (status.b.phy_lock != 0U) &&
            ((lane_count < 2U) || (status.b.phy_stopstate1lane != 0U)) &&
            ((lane_count < 3U) || (status.b.phy_stopstate2lane != 0U)) &&
            ((lane_count < 4U) || (status.b.phy_stopstate3lane != 0U)))
        {
            return DSI_STATUS_OK;
        }
    }

    return DSI_STATUS_ERROR;
}

/*============================================================================*
 *                           Public Functions
 *============================================================================*/

void DSI_DeInit(DSI_TypeDef *DSIx)
{
    DSI_PWR_UP_TypeDef reg_0x04 = {.d32 = DSIx->PWR_UP};
    reg_0x04.b.shutdownz = 0;           // reset dsi host controller
    DSIx->PWR_UP = reg_0x04.d32;
}

void DSI_StructInit(DSI_InitTypeDef *DSI_Init)
{
    /*-------------- Reset DSI init structure parameters values -----------------*/
    DSI_Init->phy_mode                = DSI_DPHY;
    DSI_Init->ColorCoding             = DSI_COLOR_RGB888;
    DSI_Init->NumberOfLanes           = DSI_1_LANE;
    DSI_Init->StopWaitTime            = 0;

    /* Unmask every error report in INT_ST0/INT_ST1. */
    DSI_Init->InterruptMask0          = 0x1FFFFF;
    DSI_Init->InterruptMask1          = 0x1FFFFF;

    /* Longest HS-TX / LP-RX timeout. */
    DSI_Init->HS_TX_CNT               = 0xFFFF;
    DSI_Init->LP_RX_CNT               = 0xFFFF;

    /* Clock dividers depend on the lane byte clock, caller must configure. */
    DSI_Init->TOCkdiv                 = 0;
    DSI_Init->TXEscapeCkdiv           = 0;

    /* Start in command mode so the panel init table can be sent first. */
    DSI_Init->ModeCfg                 = DSI_MODE_CMD;

    DSI_Init->PeripherCharacteristics = 0;
    DSI_Init->GenericPacketVCID       = 0;
    DSI_Init->GenericTearVCID         = 0;
    DSI_Init->GenericReadBackVCID     = 0;

    DSI_Init->AutoHS2LPEnable         = ENABLE;
    DSI_Init->TxRequestCLKEnable      = ENABLE;

    /* PHY timings depend on the D-PHY PLL, caller must configure. */
    DSI_Init->ClockLaneHS2LPTime      = 0;
    DSI_Init->ClockLaneLP2HSTime      = 0;
    DSI_Init->DataLaneHS2LPTime       = 0;
    DSI_Init->DataLaneLP2HSTime       = 0;
    DSI_Init->DataLaneMaxReadTime     = 0;
    DSI_Init->VirtualChannelID        = 0;

    DSI_Init->phy_shutdownz           = 1;
    DSI_Init->phy_enableclk           = 1;
    DSI_Init->phy_forcepll            = 0;
    DSI_Init->phy_rstz                = 1;

    DSI_Init->ColorModePolarity       = 0;
    DSI_Init->ShutDownPolarity        = 0;
    DSI_Init->HSPolarity              = DSI_HSPOL_AH;
    DSI_Init->VSPolarity              = DSI_VSPOL_AH;
    DSI_Init->DEPolarity              = DSI_DEPOL_AH;

    /* eDPI allowed command size depends on the line buffer, caller must configure. */
    DSI_Init->CommandSize             = 0;

    DSI_Init->LPMaxReadPacket         = DSI_LP_MRDP_DISABLE;
    DSI_Init->LPGenReadTime           = 0;
    DSI_Init->HSGenReadTime           = 0;
    DSI_Init->LPGenWriteTime          = 0;
    DSI_Init->HSGenWriteTime          = 0;
    DSI_Init->BTATime                 = 0;
    DSI_Init->AcknowledgeRequest      = DSI_ACKNOWLEDGE_DISABLE;

    /* Every command packet type goes out in low-power mode. */
    DSI_Init->LPGenShortWriteNoP      = DSI_LP_GSW0P_ENABLE;
    DSI_Init->LPGenShortWriteOneP     = DSI_LP_GSW1P_ENABLE;
    DSI_Init->LPGenShortWriteTwoP     = DSI_LP_GSW2P_ENABLE;
    DSI_Init->LPGenShortReadNoP       = DSI_LP_GSR0P_ENABLE;
    DSI_Init->LPGenShortReadOneP      = DSI_LP_GSR1P_ENABLE;
    DSI_Init->LPGenShortReadTwoP      = DSI_LP_GSR2P_ENABLE;
    DSI_Init->LPGenLongWrite          = DSI_LP_GLW_ENABLE;
    DSI_Init->LPDcsShortWriteNoP      = DSI_LP_DSW0P_ENABLE;
    DSI_Init->LPDcsShortWriteOneP     = DSI_LP_DSW1P_ENABLE;
    DSI_Init->LPDcsShortReadNoP       = DSI_LP_DSR0P_ENABLE;
    DSI_Init->LPDcsLongWrite          = DSI_LP_DLW_ENABLE;
}

FlagStatus DSI_Init(DSI_TypeDef *DSIx, const DSI_InitTypeDef *DSI_Init)
{
    assert_param(IS_DSI_MODE(DSI_Init->ModeCfg));
    assert_param(IS_DSI_PHY_MODE(DSI_Init->phy_mode));
    assert_param(IS_DSI_N_Lanes(DSI_Init->NumberOfLanes));

    if ((DSI_Init->NumberOfLanes == 0U) || (DSI_Init->NumberOfLanes > 4U) ||
        (DSI_Init->ColorCoding > 0xFU))
    {
        return DSI_STATUS_ERROR;
    }

    DSI_PWR_UP_TypeDef pwr_up = {.d32 = DSIx->PWR_UP};
    pwr_up.b.shutdownz = 0;             // (3) reset dsi host controller
    DSIx->PWR_UP = pwr_up.d32;

    DSI_PHY_MODE_TypeDef phy_mode = {.d32 = DSIx->PHY_MODE};
    phy_mode.b.phy_mode = DSI_Init->phy_mode;           // (4) D-PHY / D-PHY combo
    DSIx->PHY_MODE = phy_mode.d32;

    DSI_PHY_IF_CFG_TypeDef phy_if = {.d32 = DSIx->PHY_IF_CFG};
    phy_if.b.n_lanes = DSI_Init->NumberOfLanes - 1;     // (5) data lane count (encoded: 0 = 1 lane)
    phy_if.b.phy_stop_wait_time = DSI_Init->StopWaitTime;
    DSIx->PHY_IF_CFG = phy_if.d32;

    DSI_INT_MSK0_TypeDef int_msk0 = {.d32 = DSIx->INT_MSK0};
    int_msk0.b.int_msk0 = DSI_Init->InterruptMask0;     // (6)
    DSIx->INT_MSK0 = int_msk0.d32;

    DSI_INT_MSK1_TypeDef int_msk1 = {.d32 = DSIx->INT_MSK1};
    int_msk1.b.int_msk1 = DSI_Init->InterruptMask1;     // (7)
    DSIx->INT_MSK1 = int_msk1.d32;

    DSI_TO_CNT_CFG_TypeDef to_cnt = {.d32 = DSIx->TO_CNT_CFG};
    to_cnt.b.hstx_to_cnt = DSI_Init->HS_TX_CNT;         // (8) HS TX timeout
    to_cnt.b.lprx_to_cnt = DSI_Init->LP_RX_CNT;         // (8) LP RX timeout
    DSIx->TO_CNT_CFG = to_cnt.d32;

    DSI_CLKMGR_CFG_TypeDef clkmgr = {.d32 = DSIx->CLKMGR_CFG};
    clkmgr.b.to_clk_division = DSI_Init->TOCkdiv;       // (9) timeout clock division
    clkmgr.b.tx_esc_clk_division = DSI_Init->TXEscapeCkdiv; // (9) TxClkEsc division (2-20MHz)
    DSIx->CLKMGR_CFG = clkmgr.d32;

    DSI_MODE_CFG_TypeDef mode_cfg = {.d32 = DSIx->MODE_CFG};
    mode_cfg.b.cmd_video_mode = DSI_Init->ModeCfg;      // (10) 0=video / 1=command
    DSIx->MODE_CFG = mode_cfg.d32;

    DSI_DPI_VCID_TypeDef dpi_vcid = {.d32 = DSIx->DPI_VCID};
    dpi_vcid.b.dpi_vcid = DSI_Init->VirtualChannelID;   // (15)
    DSIx->DPI_VCID = dpi_vcid.d32;

    DSI_DPI_COLOR_CODING_TypeDef dpi_color = {.d32 = DSIx->DPI_COLOR_CODING};
    dpi_color.b.dpi_color_coding = DSI_Init->ColorCoding; // (16)
    DSIx->DPI_COLOR_CODING = dpi_color.d32;

    DSI_DPI_CFG_POL_TypeDef dpi_pol = {.d32 = DSIx->DPI_CFG_POL};
    dpi_pol.b.colorm_active_low = DSI_Init->ColorModePolarity; // (31)
    dpi_pol.b.shutd_active_low = DSI_Init->ShutDownPolarity;
    dpi_pol.b.hsync_active_low = DSI_Init->HSPolarity;
    dpi_pol.b.vsync_active_low = DSI_Init->VSPolarity;
    dpi_pol.b.dataen_active_low = DSI_Init->DEPolarity;
    DSIx->DPI_CFG_POL = dpi_pol.d32;

    DSI_EDPI_CMD_SIZE_TypeDef edpi_cmd = {.d32 = DSIx->EDPI_CMD_SIZE};
    edpi_cmd.b.edpi_allowed_cmd_size = DSI_Init->CommandSize; // (33)
    DSIx->EDPI_CMD_SIZE = edpi_cmd.d32;

    DSI_CMD_MODE_CFG_TypeDef cmd_mode = {.d32 = DSIx->CMD_MODE_CFG};
    cmd_mode.b.ack_rqst_en = DSI_Init->AcknowledgeRequest; // (34)
    cmd_mode.b.max_rd_pkt_size = DSI_Init->LPMaxReadPacket;
    cmd_mode.b.gen_sw_0p_tx = DSI_Init->LPGenShortWriteNoP;
    cmd_mode.b.gen_sw_1p_tx = DSI_Init->LPGenShortWriteOneP;
    cmd_mode.b.gen_sw_2p_tx = DSI_Init->LPGenShortWriteTwoP;
    cmd_mode.b.gen_sr_0p_tx = DSI_Init->LPGenShortReadNoP;
    cmd_mode.b.gen_sr_1p_tx = DSI_Init->LPGenShortReadOneP;
    cmd_mode.b.gen_sr_2p_tx = DSI_Init->LPGenShortReadTwoP;
    cmd_mode.b.gen_lw_tx = DSI_Init->LPGenLongWrite;
    cmd_mode.b.dcs_sw_0p_tx = DSI_Init->LPDcsShortWriteNoP;
    cmd_mode.b.dcs_sw_1p_tx = DSI_Init->LPDcsShortWriteOneP;
    cmd_mode.b.dcs_sr_0p_tx = DSI_Init->LPDcsShortReadNoP;
    cmd_mode.b.dcs_lw_tx = DSI_Init->LPDcsLongWrite;
    DSIx->CMD_MODE_CFG = cmd_mode.d32;

    DSI_HS_RD_TO_CNT_TypeDef hs_rd = {.d32 = DSIx->HS_RD_TO_CNT};
    hs_rd.b.hs_rd_to_cnt = DSI_Init->HSGenReadTime;     // (35)
    DSIx->HS_RD_TO_CNT = hs_rd.d32;

    DSI_LP_RD_TO_CNT_TypeDef lp_rd = {.d32 = DSIx->LP_RD_TO_CNT};
    lp_rd.b.lp_rd_to_cnt = DSI_Init->LPGenReadTime;     // (36)
    DSIx->LP_RD_TO_CNT = lp_rd.d32;

    DSI_HS_WR_TO_CNT_TypeDef hs_wr = {.d32 = DSIx->HS_WR_TO_CNT};
    hs_wr.b.hs_wr_to_cnt = DSI_Init->HSGenWriteTime;    // (37)
    DSIx->HS_WR_TO_CNT = hs_wr.d32;

    DSI_LP_WR_TO_CNT_TypeDef lp_wr = {.d32 = DSIx->LP_WR_TO_CNT};
    lp_wr.b.lp_wr_to_cnt = DSI_Init->LPGenWriteTime;    // (38)
    DSIx->LP_WR_TO_CNT = lp_wr.d32;

    DSI_BTA_TO_CNT_TypeDef bta = {.d32 = DSIx->BTA_TO_CNT};
    bta.b.bta_to_cnt = DSI_Init->BTATime;               // (39)
    DSIx->BTA_TO_CNT = bta.d32;

    DSI_PCKHDL_CFG_TypeDef pckhdl = {.d32 = DSIx->PCKHDL_CFG};
    pckhdl.d32 |= (DSI_Init->PeripherCharacteristics & 0x3F); // (42) 0x3F mask
    DSIx->PCKHDL_CFG = pckhdl.d32;

    DSI_GEN_VCID_TypeDef gen_vcid = {.d32 = DSIx->GEN_VCID};
    gen_vcid.b.gen_vcid_tx_auto = DSI_Init->GenericPacketVCID; // (43)
    gen_vcid.b.gen_vcid_tear_auto = DSI_Init->GenericTearVCID;
    gen_vcid.b.gen_vcid_rx = DSI_Init->GenericReadBackVCID;
    DSIx->GEN_VCID = gen_vcid.d32;

    DSI_LPCLK_CTRL_TypeDef lpclk = {.d32 = DSIx->LPCLK_CTRL};
    lpclk.b.auto_clklane_ctrl = DSI_Init->AutoHS2LPEnable; // (44)
    lpclk.b.phy_txrequestclkhs = DSI_Init->TxRequestCLKEnable;
    DSIx->LPCLK_CTRL = lpclk.d32;

    DSI_PHY_TMR_LPCLK_CFG_TypeDef tmr_lpclk = {.d32 = DSIx->PHY_TMR_LPCLK_CFG};
    tmr_lpclk.b.phy_clkhs2lp_time = DSI_Init->ClockLaneHS2LPTime; // (45)
    tmr_lpclk.b.phy_clklp2hs_time = DSI_Init->ClockLaneLP2HSTime;
    DSIx->PHY_TMR_LPCLK_CFG = tmr_lpclk.d32;

    DSI_PHY_TMR_CFG_TypeDef tmr = {.d32 = DSIx->PHY_TMR_CFG};
    tmr.b.phy_hs2lp_time = DSI_Init->DataLaneHS2LPTime; // (46)
    tmr.b.phy_lp2hs_time = DSI_Init->DataLaneLP2HSTime;
    DSIx->PHY_TMR_CFG = tmr.d32;

    DSI_PHY_TMR_RD_CFG_TypeDef tmr_rd = {.d32 = DSIx->PHY_TMR_RD_CFG};
    tmr_rd.b.max_rd_time = DSI_Init->DataLaneMaxReadTime; // (47)
    DSIx->PHY_TMR_RD_CFG = tmr_rd.d32;
    return DSI_STATUS_OK;
}

FlagStatus DSI_ConfigVideoMode(DSI_TypeDef *DSIx, const DSI_VidCfgTypeDef *VidCfg)
{
    assert_param(IS_VIDEO_MODE_TYPE(VidCfg->Mode));
    assert_param(IS_VIDEO_VPG_MODE(VidCfg->VPGMode));
    assert_param(IS_VIDEO_VPG_ORIENTATION(VidCfg->VPGOrientation));

    DSI_VID_MODE_CFG_TypeDef vid = {.d32 = DSIx->VID_MODE_CFG};
    vid.b.vid_mode_type = VidCfg->Mode;                 // (13) burst / non-burst
    vid.b.lp_cmd_en = VidCfg->LPCommandEnable;          // (14)
    vid.b.frame_bta_ack_en = VidCfg->FrameBTAAcknowledgeEnable;
    vid.b.lp_vsa_en = VidCfg->LPVerticalSyncActiveEnable;
    vid.b.lp_vbp_en = VidCfg->LPVerticalBackPorchEnable;
    vid.b.lp_vfp_en = VidCfg->LPVerticalFrontPorchEnable;
    vid.b.lp_vact_en = VidCfg->LPVerticalActiveEnable;
    vid.b.lp_hbp_en = VidCfg->LPHorizontalBackPorchEnable;
    vid.b.lp_hfp_en = VidCfg->LPHorizontalFrontPorchEnable;
    DSIx->VID_MODE_CFG = vid.d32;

    DSI_DPI_LP_CMD_TIM_TypeDef lp_cmd_tim = {.d32 = DSIx->DPI_LP_CMD_TIM};
    lp_cmd_tim.b.invact_lpcmd_time = VidCfg->invact_lpcmd_size;   // (17)
    lp_cmd_tim.b.outvact_lpcmd_time = VidCfg->outvact_lpcmd_size;
    DSIx->DPI_LP_CMD_TIM = lp_cmd_tim.d32;

    DSI_VID_PKT_SIZE_TypeDef vid_pkt = {.d32 = DSIx->VID_PKT_SIZE};
    vid_pkt.b.vid_pkt_size = VidCfg->PacketSize;        // (18)
    DSIx->VID_PKT_SIZE = vid_pkt.d32;

    DSI_VID_NUM_CHUNKS_TypeDef vid_chunks = {.d32 = DSIx->VID_NUM_CHUNKS};
    vid_chunks.b.vid_num_chunks = VidCfg->NumberOfChunks; // (19)
    DSIx->VID_NUM_CHUNKS = vid_chunks.d32;

    DSI_VID_NULL_SIZE_TypeDef vid_null = {.d32 = DSIx->VID_NULL_SIZE};
    vid_null.b.vid_null_size = VidCfg->NullPacketSize;  // (20)
    DSIx->VID_NULL_SIZE = vid_null.d32;

    DSI_VID_HLINE_TIME_TypeDef vid_hline = {.d32 = DSIx->VID_HLINE_TIME};
    vid_hline.b.vid_hline_time = VidCfg->HorizontalLine; // (21)
    DSIx->VID_HLINE_TIME = vid_hline.d32;

    DSI_VID_HSA_TIME_TypeDef vid_hsa = {.d32 = DSIx->VID_HSA_TIME};
    vid_hsa.b.vid_hsa_time = VidCfg->HorizontalSyncActive; // (22)
    DSIx->VID_HSA_TIME = vid_hsa.d32;

    DSI_VID_HBP_TIME_TypeDef vid_hbp = {.d32 = DSIx->VID_HBP_TIME};
    vid_hbp.b.vid_hbp_time = VidCfg->HorizontalBackPorch; // (23)
    DSIx->VID_HBP_TIME = vid_hbp.d32;

    DSI_VID_VSA_LINES_TypeDef vid_vsa = {.d32 = DSIx->VID_VSA_LINES};
    vid_vsa.b.vsa_lines = VidCfg->VerticalSyncActive; // (24)
    DSIx->VID_VSA_LINES = vid_vsa.d32;

    DSI_VID_VBP_LINES_TypeDef vid_vbp = {.d32 = DSIx->VID_VBP_LINES};
    vid_vbp.b.vbp_lines = VidCfg->VerticalBackPorch; // (25)
    DSIx->VID_VBP_LINES = vid_vbp.d32;

    DSI_VID_VFP_LINES_TypeDef vid_vfp = {.d32 = DSIx->VID_VFP_LINES};
    vid_vfp.b.vfp_lines = VidCfg->VerticalFrontPorch; // (26)
    DSIx->VID_VFP_LINES = vid_vfp.d32;

    DSI_VID_VACTIVE_LINES_TypeDef vid_vact = {.d32 = DSIx->VID_VACTIVE_LINES};
    vid_vact.b.v_active_lines = VidCfg->VerticalActive; // (27)
    DSIx->VID_VACTIVE_LINES = vid_vact.d32;

    vid.d32 = DSIx->VID_MODE_CFG;
    vid.b.vpg_mode = VidCfg->VPGMode;                   // (28)
    vid.b.vpg_orientation = VidCfg->VPGOrientation;     // (29)
    vid.b.vpg_en = VidCfg->VPGEnable;                   // (30)
    DSIx->VID_MODE_CFG = vid.d32;

    return DSI_STATUS_OK;
}

void DSI_StartVideoMode(DSI_TypeDef *DSIx)
{
    uint32_t timeout = DSI_POLL_TIMEOUT;
    while (timeout-- != 0)
    {
        DSI_CMD_PKT_STATUS_TypeDef status = {.d32 = DSIx->CMD_PKT_STATUS};
        if ((status.b.gen_cmd_empty != 0) && (status.b.gen_pld_w_empty != 0)
            && (status.b.gen_rd_cmd_busy == 0))
        {
            break;
        }
    }

    DSI_DeInit(DSIx);
    DSI_MODE_CFG_TypeDef mode_cfg = {.d32 = DSIx->MODE_CFG};
    mode_cfg.b.cmd_video_mode = DSI_MODE_VIDEO;
    DSIx->MODE_CFG = mode_cfg.d32;
    DSI_Start(DSIx);
}

FlagStatus DSI_ConfigAdaptedCommandMode(DSI_TypeDef *DSIx,
                                        const DSI_AdaptedCmdCfgTypeDef *AdaptedCmdCfg)
{
    DSI_CMD_MODE_CFG_TypeDef cmd_mode = {.d32 = DSIx->CMD_MODE_CFG};
    cmd_mode.b.tear_fx_en = AdaptedCmdCfg->TEAcknowledgeRequest; // (34)
    DSIx->CMD_MODE_CFG = cmd_mode.d32;

    DSI_PCKHDL_CFG_TypeDef pckhdl = {.d32 = DSIx->PCKHDL_CFG};
    if (AdaptedCmdCfg->TEAcknowledgeRequest != 0U)
    {
        pckhdl.b.bta_en = 1U;
        DSIx->PCKHDL_CFG = pckhdl.d32;
    }

    DSI_EDPI_TE_HW_CFG_TypeDef edpi_te = {.d32 = DSIx->EDPI_TE_HW_CFG};
    edpi_te.b.scan_line_parameter = AdaptedCmdCfg->scan_line_parameter; // (40)
    edpi_te.b.hw_set_scan_line = AdaptedCmdCfg->hw_set_scan_line;
    edpi_te.b.hw_tear_effect_gen = AdaptedCmdCfg->hw_tear_effect_gen;
    edpi_te.b.hw_tear_effect_on = AdaptedCmdCfg->hw_tear_effect_on;
    DSIx->EDPI_TE_HW_CFG = edpi_te.d32;

    return DSI_STATUS_OK;
}

FlagStatus DSI_ConfigPhy(DSI_TypeDef *DSIx, const DSI_PhyCfgTypeDef *PhyCfg)
{
    /* The RTL-specific D-PHY PLL must be configured before this host-side step. */
    {
        DSI_PHY_RSTZ_TypeDef phy_rstz = {.d32 = DSIx->PHY_RSTZ};
        phy_rstz.b.phy_shutdownz = 0;
        phy_rstz.b.phy_rstz = 0;
        phy_rstz.b.phy_enableclk = 1;
        phy_rstz.b.phy_forcepll = 0;
        DSIx->PHY_RSTZ = phy_rstz.d32;
    }

    {
        DSI_PHY_RSTZ_TypeDef phy_rstz = {.d32 = DSIx->PHY_RSTZ};
        phy_rstz.b.phy_forcepll = PhyCfg->phy_forcepll;
        phy_rstz.b.phy_enableclk = PhyCfg->phy_enableclk;
        phy_rstz.b.phy_rstz = PhyCfg->phy_rstz;
        phy_rstz.b.phy_shutdownz = PhyCfg->phy_shutdownz;
        DSIx->PHY_RSTZ = phy_rstz.d32;
    }

    return DSI_WaitPhyStopState(DSIx);
}

FlagStatus DSI_ConfigAutoUlps(DSI_TypeDef *DSIx,
                              const DSI_AutoUlpsCfgTypeDef *AutoUlpsCfg)
{
    if ((AutoUlpsCfg->WakeupClockDiv == 0U) || (AutoUlpsCfg->WakeupCount == 0U) ||
        (AutoUlpsCfg->MinimumTime > 0x0FFFU))
    {
        return DSI_STATUS_ERROR;
    }

    DSI_AUTO_ULPS_ENTRY_DELAY_TypeDef entry_delay = {.d32 = DSIx->AUTO_ULPS_ENTRY_DELAY};
    entry_delay.b.ulps_entry_delay = AutoUlpsCfg->EntryDelay;
    DSIx->AUTO_ULPS_ENTRY_DELAY = entry_delay.d32;

    DSI_AUTO_ULPS_WAKEUP_TIME_TypeDef wakeup_time = {.d32 = DSIx->AUTO_ULPS_WAKEUP_TIME};
    wakeup_time.b.twakeup_clk_div = AutoUlpsCfg->WakeupClockDiv;
    wakeup_time.b.twakeup_cnt = AutoUlpsCfg->WakeupCount;
    DSIx->AUTO_ULPS_WAKEUP_TIME = wakeup_time.d32;

    DSI_AUTO_ULPS_MIN_TIME_TypeDef min_time = {.d32 = DSIx->AUTO_ULPS_MIN_TIME};
    min_time.b.ulps_min_time = AutoUlpsCfg->MinimumTime;
    DSIx->AUTO_ULPS_MIN_TIME = min_time.d32;

    DSI_AUTO_ULPS_MODE_TypeDef mode = {.d32 = DSIx->AUTO_ULPS_MODE};
    mode.b.pll_off_ulps = AutoUlpsCfg->PllOffInUlps;
    mode.b.pre_pll_off_req = AutoUlpsCfg->PrePllOffRequest;
    mode.b.auto_ulps = 1U;
    DSIx->AUTO_ULPS_MODE = mode.d32;

    return DSI_STATUS_OK;
}

void DSI_DisableAutoUlps(DSI_TypeDef *DSIx)
{
    DSI_AUTO_ULPS_MODE_TypeDef mode = {.d32 = DSIx->AUTO_ULPS_MODE};
    mode.b.auto_ulps = 0U;
    DSIx->AUTO_ULPS_MODE = mode.d32;
}

static void DSI_ConfigPacketHeader(DSI_TypeDef *DSIx,
                                   uint32_t ChannelID,
                                   uint32_t DataType,
                                   uint32_t Data0,
                                   uint32_t Data1)
{
    DSI_GEN_HDR_TypeDef gen_hdr = {.d32 = DSIx->GEN_HDR};
    /* Update the DSI packet header with new information */
    gen_hdr.b.gen_dt = DataType;
    gen_hdr.b.gen_vc = ChannelID;
    gen_hdr.b.gen_wc_lsbyte = Data0;
    gen_hdr.b.gen_wc_msbyte = Data1;
    DSIx->GEN_HDR = gen_hdr.d32;
}

FlagStatus DSI_ShortWrite(DSI_TypeDef *DSIx,
                          uint32_t ChannelID,
                          uint32_t Mode,
                          uint32_t Param1,
                          uint32_t Param2)
{
    assert_param(IS_DSI_SHORT_WRITE_PKT(Mode));

    if ((Mode == DSI_DCS_SHORT_PKT_WRITE_P0) || (Mode == DSI_GEN_SHORT_PKT_WRITE_P0))
    {
        Param2 = 0U;
    }

    if (DSI_WaitCmdFifoNotFull(DSIx) != DSI_STATUS_OK)
    {
        return DSI_STATUS_ERROR;
    }

    DSI_GEN_HDR_TypeDef gen_hdr = {.d32 = DSIx->GEN_HDR};
    /* Configure the packet to send a short DCS command with 0 or 1 parameter */
    gen_hdr.b.gen_dt = Mode;
    gen_hdr.b.gen_vc = ChannelID;
    gen_hdr.b.gen_wc_lsbyte = Param1;
    gen_hdr.b.gen_wc_msbyte = Param2;
    DSIx->GEN_HDR = gen_hdr.d32;
    return DSI_STATUS_OK;
}

FlagStatus DSI_LongWrite(DSI_TypeDef *DSIx,
                         uint32_t ChannelID,
                         uint32_t Mode,
                         uint32_t NbParams,
                         uint32_t Param1,
                         uint8_t *ParametersTable)
{
    assert_param(IS_DSI_LONG_WRITE_PKT(Mode));
    if (NbParams > (200U - 1U))
    {
        return DSI_STATUS_ERROR;
    }

    uint32_t uicounter, nbBytes, count;
    uint32_t fifoword;
    uint8_t *pparams = ParametersTable;

    /* Set the DCS code on payload byte 1, and the other parameters on the write FIFO command*/
    fifoword = Param1;
    nbBytes = (NbParams < 3U) ? NbParams : 3U;

    for (count = 0U; count < nbBytes; count++)
    {
        fifoword |= (((uint32_t)(*(pparams + count))) << (8U + (8U * count)));
    }
    if (DSI_WaitPayloadFifoNotFull(DSIx) != DSI_STATUS_OK)
    {
        return DSI_STATUS_ERROR;
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
        if (DSI_WaitPayloadFifoNotFull(DSIx) != DSI_STATUS_OK)
        {
            return DSI_STATUS_ERROR;
        }
        DSIx->GEN_PLD_DATA = fifoword;

        uicounter -= nbBytes;
        pparams += nbBytes;
    }

    if (DSI_WaitCmdFifoNotFull(DSIx) != DSI_STATUS_OK)
    {
        return DSI_STATUS_ERROR;
    }

    /* Configure the packet to send a long DCS command */
    DSI_ConfigPacketHeader(DSIx,
                           ChannelID,
                           Mode,
                           ((NbParams + 1U) & 0x00FFU),
                           (((NbParams + 1U) & 0xFF00U) >> 8U));

    return DSI_STATUS_OK;
}

FlagStatus DSI_Start(DSI_TypeDef *DSIx)
{
    DSI_PWR_UP_TypeDef pwr_up = {.d32 = DSIx->PWR_UP};
    pwr_up.b.shutdownz = 1;             // power-up the DW DSI controller
    DSIx->PWR_UP = pwr_up.d32;
    return DSI_STATUS_OK;
}
