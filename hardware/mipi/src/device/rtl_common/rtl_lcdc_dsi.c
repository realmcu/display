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

static FlagStatus DSI_WaitCmdFifoEmpty(DSI_TypeDef *DSIx)
{
    uint32_t timeout = DSI_POLL_TIMEOUT;

    while (timeout-- != 0U)
    {
        DSI_CMD_PKT_STATUS_TypeDef status = {.d32 = DSIx->CMD_PKT_STATUS};
        if ((status.b.gen_cmd_empty != 0U) &&
            (status.b.gen_pld_w_empty != 0U) &&
            (status.b.gen_rd_cmd_busy == 0U))
        {
            return DSI_STATUS_OK;
        }
    }

    return DSI_STATUS_ERROR;
}

static uint32_t DSI_PixToLbc(uint32_t pix, uint32_t pix_clk_khz, uint32_t lane_byte_clk_khz)
{
    uint64_t product;

    if (lane_byte_clk_khz == 0U)
    {
        return 0U;
    }

    product = (uint64_t)pix * lane_byte_clk_khz;
    return (uint32_t)((product + pix_clk_khz - 1U) / pix_clk_khz);
}

static uint32_t DSI_EscClkDiv(uint32_t lane_byte_clk_khz)
{
    uint32_t div;

    /* One past the quotient so the clock never lands on the target; dividers 0
       and 1 stop it, so 2 is the floor. */
    div = (lane_byte_clk_khz / DSI_PHY_ESC_CLK_TARGET_KHZ) + 1U;

    if (div < 2U)
    {
        div = 2U;
    }

    return (div > 0xFFU) ? 0xFFU : div;
}

static uint32_t DSI_LpCmdBytes(uint32_t window_lbc, uint32_t lp_overhead_lbc,
                               uint32_t lp_byte_lbc)
{
    if ((window_lbc <= lp_overhead_lbc) || (lp_byte_lbc == 0U))
    {
        return 0U;
    }

    return (window_lbc - lp_overhead_lbc) / lp_byte_lbc;
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

FlagStatus DSI_Init(DSI_TypeDef *DSIx, const DSI_ConfigTypeDef *Config)
{
    assert_param(Config != NULL);
    assert_param(IS_DSI_LANE_COUNT(Config->LaneCount));
    assert_param(IS_DSI_COLOR_CODING(Config->ColorCoding));
    assert_param(IS_DSI_POLARITY(Config->Polarity));
    assert_param(IS_DSI_MODE(Config->Mode));
    assert_param(IS_DSI_DPI_COMMAND_SIZE(Config->eDPICommandSize));

    uint32_t esc_clk_div = DSI_EscClkDiv(Config->LaneByteClkKHz);
    uint32_t to_clk_div = DSI_PHY_DEFAULT_TIMEOUT_CLK_DIV;

    DSI_PWR_UP_TypeDef pwr_up = {.d32 = DSIx->PWR_UP};
    pwr_up.b.shutdownz = 0;                             // (3) reset dsi host controller
    DSIx->PWR_UP = pwr_up.d32;

    DSI_PHY_MODE_TypeDef phy_mode = {.d32 = DSIx->PHY_MODE};
    phy_mode.b.phy_mode = DSI_DPHY;                     // (4) RTL core is D-PHY only
    DSIx->PHY_MODE = phy_mode.d32;

    DSI_PHY_IF_CFG_TypeDef phy_if = {.d32 = DSIx->PHY_IF_CFG};
    phy_if.b.n_lanes = Config->LaneCount - 1U;          // (5) encoded: 0 means 1 lane
    phy_if.b.phy_stop_wait_time = 0U;
    DSIx->PHY_IF_CFG = phy_if.d32;

    DSI_INT_MSK0_TypeDef int_msk0 = {.d32 = DSIx->INT_MSK0};
    int_msk0.b.int_msk0 = 0x1FFFFFU;                    // (6) every source unmasked
    DSIx->INT_MSK0 = int_msk0.d32;

    DSI_INT_MSK1_TypeDef int_msk1 = {.d32 = DSIx->INT_MSK1};
    int_msk1.b.int_msk1 = 0x1FFFFFU;                    // (7)
    DSIx->INT_MSK1 = int_msk1.d32;

    DSI_TO_CNT_CFG_TypeDef to_cnt = {.d32 = DSIx->TO_CNT_CFG};
    to_cnt.b.hstx_to_cnt = 0xFFFFU;                     // (8) HS TX timeout
    to_cnt.b.lprx_to_cnt = 0xFFFFU;                     // (8) LP RX timeout
    DSIx->TO_CNT_CFG = to_cnt.d32;

    DSI_CLKMGR_CFG_TypeDef clkmgr = {.d32 = DSIx->CLKMGR_CFG};
    clkmgr.b.to_clk_division = to_clk_div;              // (9) timeout clock division
    clkmgr.b.tx_esc_clk_division = esc_clk_div;         // (9) TxClkEsc division
    DSIx->CLKMGR_CFG = clkmgr.d32;

    DSI_MODE_CFG_TypeDef mode_cfg = {.d32 = DSIx->MODE_CFG};
    /* Command mode lets a panel send its init table before any video starts. */
    mode_cfg.b.cmd_video_mode = (Config->Mode == DSI_MODE_CMD) ? 1U : 0U; // (10) 0=video / 1=command
    DSIx->MODE_CFG = mode_cfg.d32;

    DSI_DPI_VCID_TypeDef dpi_vcid = {.d32 = DSIx->DPI_VCID};
    dpi_vcid.b.dpi_vcid = 0U;                           // (15) single panel, one virtual channel
    DSIx->DPI_VCID = dpi_vcid.d32;

    DSI_DPI_COLOR_CODING_TypeDef dpi_color = {.d32 = DSIx->DPI_COLOR_CODING};
    dpi_color.b.dpi_color_coding = Config->ColorCoding; // (16)
    DSIx->DPI_COLOR_CODING = dpi_color.d32;

    DSI_DPI_CFG_POL_TypeDef dpi_pol = {.d32 = DSIx->DPI_CFG_POL};
    dpi_pol.b.colorm_active_low = (Config->Polarity & DSI_POL_COLORM_LOW) ? 1U : 0U;
    dpi_pol.b.shutd_active_low = (Config->Polarity & DSI_POL_SHUTD_LOW) ? 1U : 0U;
    dpi_pol.b.hsync_active_low = (Config->Polarity & DSI_POL_HSYNC_LOW) ? 1U : 0U;
    dpi_pol.b.vsync_active_low = (Config->Polarity & DSI_POL_VSYNC_LOW) ? 1U : 0U;
    dpi_pol.b.dataen_active_low = (Config->Polarity & DSI_POL_DATAEN_LOW) ? 1U : 0U;
    DSIx->DPI_CFG_POL = dpi_pol.d32;

    DSI_EDPI_CMD_SIZE_TypeDef edpi_cmd = {.d32 = DSIx->EDPI_CMD_SIZE};
    edpi_cmd.b.edpi_allowed_cmd_size = Config->eDPICommandSize; // (33)
    DSIx->EDPI_CMD_SIZE = edpi_cmd.d32;

    DSIx->CMD_MODE_CFG = DSI_LP_CMD_ALL;                // (34)

    DSIx->HS_RD_TO_CNT = 0U;                            // (35)
    DSIx->LP_RD_TO_CNT = 0U;                            // (36)

    /* presp_to_mode shares this register with the counter, so clear only the
       counter instead of the whole word. */
    DSI_HS_WR_TO_CNT_TypeDef hs_wr_to = {.d32 = DSIx->HS_WR_TO_CNT};
    hs_wr_to.b.hs_wr_to_cnt = 0U;                       // (37)
    DSIx->HS_WR_TO_CNT = hs_wr_to.d32;

    DSIx->LP_WR_TO_CNT = 0U;                            // (38)
    DSIx->BTA_TO_CNT = 0U;                              // (39)

    /* EoTp and BTA stay off: the link only ever goes host -> panel, so there is
       no end-of-transmission to append and no turnaround to allow. */
    DSI_PCKHDL_CFG_TypeDef pckhdl = {.d32 = 0U};        // (42)
    DSIx->PCKHDL_CFG = pckhdl.d32;

    DSI_GEN_VCID_TypeDef gen_vcid = {.d32 = 0U};        // (43)
    DSIx->GEN_VCID = gen_vcid.d32;

    DSI_LPCLK_CTRL_TypeDef lpclk = {.d32 = DSIx->LPCLK_CTRL};
    lpclk.b.auto_clklane_ctrl = 1U;                     // (44) park the clock lane in LP while idle
    /* The host drives txrequestclkhs itself: video mode needs it in HS. */
    lpclk.b.phy_txrequestclkhs = 1U;
    DSIx->LPCLK_CTRL = lpclk.d32;

    DSI_PHY_TMR_LPCLK_CFG_TypeDef tmr_lpclk = {.d32 = DSIx->PHY_TMR_LPCLK_CFG};
    tmr_lpclk.b.phy_clkhs2lp_time = DSI_PHY_DEFAULT_CLK_HS2LP;  // (45)
    tmr_lpclk.b.phy_clklp2hs_time = DSI_PHY_DEFAULT_CLK_LP2HS;
    DSIx->PHY_TMR_LPCLK_CFG = tmr_lpclk.d32;

    DSI_PHY_TMR_CFG_TypeDef tmr = {.d32 = DSIx->PHY_TMR_CFG};
    tmr.b.phy_hs2lp_time = DSI_PHY_DEFAULT_DATA_HS2LP;  // (46)
    tmr.b.phy_lp2hs_time = DSI_PHY_DEFAULT_DATA_LP2HS;
    DSIx->PHY_TMR_CFG = tmr.d32;

    DSI_PHY_TMR_RD_CFG_TypeDef tmr_rd = {.d32 = DSIx->PHY_TMR_RD_CFG};
    tmr_rd.b.max_rd_time = 0U;                          // (47)
    DSIx->PHY_TMR_RD_CFG = tmr_rd.d32;

    return DSI_STATUS_OK;
}

FlagStatus DSI_ConfigLPM(DSI_TypeDef *DSIx, uint32_t Mask)
{
    if ((Mask & ~DSI_LP_ALL_MASK) != 0UL)
    {
        return DSI_STATUS_ERROR;
    }

    /* Read-modify-write rather than a plain store: tear_fx_en and ack_rqst_en
       live in the same register and are not part of any DSI_LP mask. */
    DSI_CMD_MODE_CFG_TypeDef cmd_mode = {.d32 = DSIx->CMD_MODE_CFG};
    cmd_mode.d32 = (cmd_mode.d32 & ~DSI_LP_ALL_MASK) | (Mask & DSI_LP_ALL_MASK);
    DSIx->CMD_MODE_CFG = cmd_mode.d32;

    return DSI_STATUS_OK;
}

FlagStatus DSI_ConfigVideoMode(DSI_TypeDef *DSIx, const DSI_VideoCfgTypeDef *VidCfg)
{
    uint32_t htotal;
    uint32_t hline_lbc;
    uint32_t hsa_lbc;
    uint32_t hbp_lbc;
    uint32_t hact_lbc;
    uint32_t lp_overhead_lbc;
    uint32_t lp_byte_lbc;
    uint32_t esc_clk_div;
    uint32_t line_free_lbc;
    uint32_t invact_lpcmd_size;
    uint32_t outvact_lpcmd_size;

    if ((VidCfg == NULL) || (VidCfg->Mode > VIDEO_BURST_MODE_3) ||
        (VidCfg->NumberOfChunks > 0x1FFFU) || (VidCfg->NullPacketSize > 0x1FFFU) ||
        (VidCfg->PixelClkKHz == 0U) || (VidCfg->LaneCount == 0U) ||
        (VidCfg->LaneCount > 4U) || (VidCfg->BytesPerPixel == 0U) ||
        (VidCfg->LaneByteClkKHz == 0U))
    {
        return DSI_STATUS_ERROR;
    }

    esc_clk_div = DSI_EscClkDiv(VidCfg->LaneByteClkKHz);

    htotal = VidCfg->Width + VidCfg->HSync + VidCfg->HBackPorch + VidCfg->HFrontPorch;
    if (htotal == 0U)
    {
        return DSI_STATUS_ERROR;
    }

    /* Pixels to lane byte clock cycles, rounded up; the active window also
       carries the 6-byte packet header/CRC, spread over the lanes. */
    {
        const uint8_t dsi_minimum_lbcc[4] = {10U, 5U, 4U, 3U};
        uint32_t min_lbcc = dsi_minimum_lbcc[VidCfg->LaneCount - 1U];

        hline_lbc = DSI_PixToLbc(htotal, VidCfg->PixelClkKHz, VidCfg->LaneByteClkKHz);
        hsa_lbc = DSI_PixToLbc(VidCfg->HSync, VidCfg->PixelClkKHz, VidCfg->LaneByteClkKHz);
        hbp_lbc = DSI_PixToLbc(VidCfg->HBackPorch, VidCfg->PixelClkKHz, VidCfg->LaneByteClkKHz);

        hact_lbc = (((VidCfg->Width * VidCfg->BytesPerPixel) + 6U) / VidCfg->LaneCount);

        if (hline_lbc < min_lbcc)
        {
            hline_lbc = min_lbcc;
        }
        if (hsa_lbc < min_lbcc)
        {
            hsa_lbc = min_lbcc;
        }
        if (hbp_lbc < min_lbcc)
        {
            hbp_lbc = min_lbcc;
        }
    }

    /* LP command bytes a blanking window can carry: one LPDT byte costs 16
       TxClkEsc periods, after the transitions and 24 escape periods are paid. */
    lp_byte_lbc = 16U * esc_clk_div;
    lp_overhead_lbc = DSI_PHY_DEFAULT_DATA_HS2LP + DSI_PHY_DEFAULT_DATA_LP2HS +
                      (24U * esc_clk_div);

    line_free_lbc = (hline_lbc > (hsa_lbc + hbp_lbc)) ?
                    (hline_lbc - hsa_lbc - hbp_lbc) : 0U;
    outvact_lpcmd_size = DSI_LpCmdBytes(line_free_lbc, lp_overhead_lbc, lp_byte_lbc);
    invact_lpcmd_size = DSI_LpCmdBytes((line_free_lbc > hact_lbc) ?
                                       (line_free_lbc - hact_lbc) : 0U,
                                       lp_overhead_lbc, lp_byte_lbc);

    DSI_VID_MODE_CFG_TypeDef vid = {.d32 = DSIx->VID_MODE_CFG};
    vid.b.vid_mode_type = VidCfg->Mode;                 // (13) burst / non-burst
    /* LP commands allowed in every blanking area; the link never turns around. */
    vid.b.lp_cmd_en = 1U;                               // (14)
    vid.b.frame_bta_ack_en = 0U;
    vid.b.lp_vsa_en = 1U;
    vid.b.lp_vbp_en = 1U;
    vid.b.lp_vfp_en = 1U;
    vid.b.lp_vact_en = 1U;
    vid.b.lp_hbp_en = 1U;
    vid.b.lp_hfp_en = 1U;
    DSIx->VID_MODE_CFG = vid.d32;

    DSI_DPI_LP_CMD_TIM_TypeDef lp_cmd_tim = {.d32 = DSIx->DPI_LP_CMD_TIM};
    lp_cmd_tim.b.invact_lpcmd_time = invact_lpcmd_size;         // (17)
    lp_cmd_tim.b.outvact_lpcmd_time = outvact_lpcmd_size;
    DSIx->DPI_LP_CMD_TIM = lp_cmd_tim.d32;

    DSI_VID_PKT_SIZE_TypeDef vid_pkt = {.d32 = DSIx->VID_PKT_SIZE};
    vid_pkt.b.vid_pkt_size = VidCfg->Width;             // (18) one packet per line
    DSIx->VID_PKT_SIZE = vid_pkt.d32;

    DSI_VID_NUM_CHUNKS_TypeDef vid_chunks = {.d32 = DSIx->VID_NUM_CHUNKS};
    vid_chunks.b.vid_num_chunks = VidCfg->NumberOfChunks; // (19)
    DSIx->VID_NUM_CHUNKS = vid_chunks.d32;

    DSI_VID_NULL_SIZE_TypeDef vid_null = {.d32 = DSIx->VID_NULL_SIZE};
    vid_null.b.vid_null_size = VidCfg->NullPacketSize;  // (20)
    DSIx->VID_NULL_SIZE = vid_null.d32;

    DSI_VID_HLINE_TIME_TypeDef vid_hline = {.d32 = DSIx->VID_HLINE_TIME};
    vid_hline.b.vid_hline_time = hline_lbc;             // (21)
    DSIx->VID_HLINE_TIME = vid_hline.d32;

    DSI_VID_HSA_TIME_TypeDef vid_hsa = {.d32 = DSIx->VID_HSA_TIME};
    vid_hsa.b.vid_hsa_time = hsa_lbc;                   // (22)
    DSIx->VID_HSA_TIME = vid_hsa.d32;

    DSI_VID_HBP_TIME_TypeDef vid_hbp = {.d32 = DSIx->VID_HBP_TIME};
    vid_hbp.b.vid_hbp_time = hbp_lbc;                   // (23)
    DSIx->VID_HBP_TIME = vid_hbp.d32;

    DSI_VID_VSA_LINES_TypeDef vid_vsa = {.d32 = DSIx->VID_VSA_LINES};
    vid_vsa.b.vsa_lines = VidCfg->VSync;                // (24)
    DSIx->VID_VSA_LINES = vid_vsa.d32;

    DSI_VID_VBP_LINES_TypeDef vid_vbp = {.d32 = DSIx->VID_VBP_LINES};
    vid_vbp.b.vbp_lines = VidCfg->VBackPorch;           // (25)
    DSIx->VID_VBP_LINES = vid_vbp.d32;

    DSI_VID_VFP_LINES_TypeDef vid_vfp = {.d32 = DSIx->VID_VFP_LINES};
    vid_vfp.b.vfp_lines = VidCfg->VFrontPorch;          // (26)
    DSIx->VID_VFP_LINES = vid_vfp.d32;

    DSI_VID_VACTIVE_LINES_TypeDef vid_vact = {.d32 = DSIx->VID_VACTIVE_LINES};
    vid_vact.b.v_active_lines = VidCfg->Height;         // (27)
    DSIx->VID_VACTIVE_LINES = vid_vact.d32;

    return DSI_STATUS_OK;
}

FlagStatus DSI_ConfigVPG(DSI_TypeDef *DSIx, uint32_t Enable,
                         uint32_t Mode, uint32_t Orientation)
{
    assert_param(IS_VIDEO_VPG_MODE(Mode));
    assert_param(IS_VIDEO_VPG_ORIENTATION(Orientation));

    if (!IS_VIDEO_VPG_MODE(Mode) || !IS_VIDEO_VPG_ORIENTATION(Orientation))
    {
        return DSI_STATUS_ERROR;
    }

    DSI_VID_MODE_CFG_TypeDef vid = {.d32 = DSIx->VID_MODE_CFG};
    vid.b.vpg_mode = Mode;                              // (28)
    vid.b.vpg_orientation = Orientation;                // (29)
    vid.b.vpg_en = (Enable != 0U) ? 1U : 0U;            // (30)
    DSIx->VID_MODE_CFG = vid.d32;

    return DSI_STATUS_OK;
}


void DSI_StartVideoMode(DSI_TypeDef *DSIx)
{
    /* Let the last command finish before the mode switch disturbs the link. */
    (void)DSI_WaitCmdFifoEmpty(DSIx);

    DSI_DeInit(DSIx);
    DSI_MODE_CFG_TypeDef mode_cfg = {.d32 = DSIx->MODE_CFG};
    mode_cfg.b.cmd_video_mode = DSI_MODE_VIDEO;
    DSIx->MODE_CFG = mode_cfg.d32;
    DSI_Start(DSIx);
}

FlagStatus DSI_ConfigAdaptedCommandMode(DSI_TypeDef *DSIx,
                                        const DSI_AdaptedCmdCfgTypeDef *AdaptedCmdCfg)
{
    if (AdaptedCmdCfg == NULL)
    {
        return DSI_STATUS_ERROR;
    }

    DSI_CMD_MODE_CFG_TypeDef cmd_mode = {.d32 = DSIx->CMD_MODE_CFG};
    cmd_mode.b.tear_fx_en = (AdaptedCmdCfg->TEAcknowledgeRequest != 0U) ? 1U : 0U; // (34)
    DSIx->CMD_MODE_CFG = cmd_mode.d32;

    /* An acknowledge is a packet coming back, so the link has to turn around. */
    if (AdaptedCmdCfg->TEAcknowledgeRequest != 0U)
    {
        DSI_PCKHDL_CFG_TypeDef pckhdl = {.d32 = DSIx->PCKHDL_CFG};
        pckhdl.b.bta_en = 1U;                           // (42)
        DSIx->PCKHDL_CFG = pckhdl.d32;
    }

    DSI_EDPI_TE_HW_CFG_TypeDef edpi_te = {.d32 = 0U};
    edpi_te.b.scan_line_parameter = (AdaptedCmdCfg->scan_line_parameter & 0xFFFFU); // (40)
    edpi_te.b.hw_set_scan_line = (AdaptedCmdCfg->hw_set_scan_line != 0U) ? 1U : 0U;
    edpi_te.b.hw_tear_effect_gen = (AdaptedCmdCfg->hw_tear_effect_gen != 0U) ? 1U : 0U;
    edpi_te.b.hw_tear_effect_on = (AdaptedCmdCfg->hw_tear_effect_on != 0U) ? 1U : 0U;
    DSIx->EDPI_TE_HW_CFG = edpi_te.d32;

    return DSI_STATUS_OK;
}

FlagStatus DSI_ConfigPhyPins(DSI_TypeDef *DSIx)
{
    DSI_PHY_RSTZ_TypeDef phy_rstz = {.d32 = 0U};

    /* Reset with the PLL and clock lane off, then pulse testclr so the macro
       starts clean instead of inheriting the previous PLL state. */
    phy_rstz.b.phy_shutdownz = 0U;
    phy_rstz.b.phy_rstz = 0U;
    phy_rstz.b.phy_enableclk = 0U;
    phy_rstz.b.phy_forcepll = 0U;
    DSIx->PHY_RSTZ = phy_rstz.d32;

    DSI_PHY_TST_CTRL0_TypeDef tst = {.d32 = 0U};
    tst.b.phy_testclr = 0U;                 /* step 1: clear the test interface */
    DSIx->PHY_TST_CTRL0 = tst.d32;

    tst.b.phy_testclr = 1U;                 /* step 2: assert testclr */
    DSIx->PHY_TST_CTRL0 = tst.d32;

    tst.b.phy_testclr = 0U;                 /* step 3: release it again */
    DSIx->PHY_TST_CTRL0 = tst.d32;

    /* Release reset while keeping the PLL forced off, then power the lane up. */
    phy_rstz.b.phy_enableclk = 1U;
    phy_rstz.b.phy_rstz = 1U;
    phy_rstz.b.phy_shutdownz = 1U;
    DSIx->PHY_RSTZ = phy_rstz.d32;

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

    /* Writing GEN_HDR arms the transmission: returning before the FIFOs drain
       lets a following reconfiguration cut the packet short. */
    return DSI_WaitCmdFifoEmpty(DSIx);
}

FlagStatus DSI_IsCmdFifoEmpty(DSI_TypeDef *DSIx)
{
    return DSI_WaitCmdFifoEmpty(DSIx);
}

FlagStatus DSI_Read(DSI_TypeDef *DSIx,
                    uint32_t ChannelID,
                    uint32_t Mode,
                    uint32_t Param1,
                    uint32_t Param2,
                    uint8_t *pBuffer,
                    uint32_t NbBytes)
{
    uint32_t i, j;
    uint32_t val;

    assert_param(IS_DSI_READ_PKT(Mode));

    if (DSI_WaitCmdFifoEmpty(DSIx) != DSI_STATUS_OK)
    {
        return DSI_STATUS_ERROR;
    }

    /* Only a short packet carries a read request. */
    if ((Mode == DSI_DCS_SHORT_PKT_READ_P0) || (Mode == DSI_GEN_SHORT_PKT_READ_P0))
    {
        Param2 = 0U;
    }

    if (DSI_WaitCmdFifoNotFull(DSIx) != DSI_STATUS_OK)
    {
        return DSI_STATUS_ERROR;
    }

    DSI_GEN_HDR_TypeDef gen_hdr = {.d32 = DSIx->GEN_HDR};
    gen_hdr.b.gen_dt = Mode;
    gen_hdr.b.gen_vc = ChannelID;
    gen_hdr.b.gen_wc_lsbyte = Param1;
    gen_hdr.b.gen_wc_msbyte = Param2;
    DSIx->GEN_HDR = gen_hdr.d32;

    /* Wait for the turnaround to complete and the request to leave. */
    {
        uint32_t timeout = DSI_POLL_TIMEOUT;
        while (timeout-- != 0U)
        {
            DSI_CMD_PKT_STATUS_TypeDef status = {.d32 = DSIx->CMD_PKT_STATUS};
            if ((status.b.gen_rd_cmd_busy == 0U) && (status.b.gen_cmd_empty != 0U))
            {
                break;
            }
        }
        if (timeout == 0U)
        {
            return DSI_STATUS_ERROR;
        }
    }

    /* Collect the payload one FIFO word at a time. */
    for (i = 0U; i < NbBytes; i += 4U)
    {
        uint32_t timeout = DSI_POLL_TIMEOUT;
        while (timeout-- != 0U)
        {
            DSI_CMD_PKT_STATUS_TypeDef status = {.d32 = DSIx->CMD_PKT_STATUS};
            if (status.b.gen_pld_r_empty == 0U)
            {
                break;
            }
        }
        if (timeout == 0U)
        {
            return DSI_STATUS_ERROR;
        }

        val = DSIx->GEN_PLD_DATA;
        if (pBuffer != NULL)
        {
            for (j = 0U; (j < 4U) && ((j + i) < NbBytes); j++)
            {
                pBuffer[i + j] = (uint8_t)(val >> (8U * j));
            }
        }
    }

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

    /* Same drain reasoning as DSI_ShortWrite: the header is the trigger. */
    return DSI_WaitCmdFifoEmpty(DSIx);
}

FlagStatus DSI_Start(DSI_TypeDef *DSIx)
{
    DSI_PWR_UP_TypeDef pwr_up = {.d32 = DSIx->PWR_UP};
    pwr_up.b.shutdownz = 1;             // power-up the DW DSI controller
    DSIx->PWR_UP = pwr_up.d32;
    return DSI_STATUS_OK;
}
