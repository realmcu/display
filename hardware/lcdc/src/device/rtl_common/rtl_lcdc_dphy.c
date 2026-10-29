/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *                        Header Files
 *============================================================================*/

#include "rtl_lcdc_dphy.h"
#include "rtl_lcdc_dsi.h"

/*============================================================================*
 *                        Local Macros
 *============================================================================*/
#define DPHY_NS(x)                  ((x) * 10U)                 /* ns   -> 0.1ns */
#define DPHY_UI(n, bit_ns10)        ((n) * (bit_ns10))          /* n UI -> 0.1ns */

/*============================================================================*
 *                        Public Functions
 *============================================================================*/
#if 1
void DPHY_Init(uint32_t lane_number, uint32_t src_clk_mhz, uint32_t dphy_clk_mhz)
{
    /* clock lane plus the data lanes actually in use */
    uint32_t lane_en = REG_CLK_LANE_EN_MSK | REG_LANE0_EN_MSK;
    if (lane_number > 0)
    {
        lane_en |= REG_LANE1_EN_MSK;
    }
    if (lane_number > 1)
    {
        lane_en |= REG_LANE2_EN_MSK;
    }
    if (lane_number > 2)
    {
        lane_en |= REG_LANE3_EN_MSK;
    }

    uint32_t bit_ns10  = 10000U / dphy_clk_mhz;
    uint32_t byte_ns10 = 8U * bit_ns10;

    /* duration in 0.1ns -> lane byte clock cycles */
#define DPHY_CYCLES(ns10)       ((ns10) / byte_ns10)

    uint32_t tx_lpx_time  = DPHY_CYCLES(DPHY_NS(60));                           /* T_LPX, 50-75ns */
    uint32_t hs_exit_time = DPHY_CYCLES(DPHY_NS(
                                            100));                          /* T_HS_EXIT, min 100ns */
    uint32_t clk_pre_time = DPHY_CYCLES(DPHY_UI(8,
                                                bit_ns10));                  /* T_CLK_PREPARE, min 8UI */
    uint32_t clk_zero_time = DPHY_CYCLES(DPHY_NS(
                                             300));                         /* T_CLK_ZERO, min 300ns */
    uint32_t clk_prpr_time = DPHY_CYCLES(DPHY_NS(
                                             70));                          /* T_CLK_PREPARE, 38-95ns */
    uint32_t clk_post_time = DPHY_CYCLES(DPHY_NS(60) + DPHY_UI(52,
                                                               bit_ns10));  /* T_CLK_POST, min 60ns+52UI */
    uint32_t tx_tail_time = DPHY_CYCLES(DPHY_NS(60) + DPHY_UI(4,
                                                              bit_ns10));    /* T_HS_TRAIL, min 60ns+4UI */
    uint32_t hs_zero_time = DPHY_CYCLES(DPHY_NS(
                                            100));                          /* T_HS_ZERO, min 100ns */
    uint32_t hs_prpr_time = DPHY_CYCLES(DPHY_NS(40) + DPHY_UI(4,
                                                              bit_ns10));    /* T_HS_PREPARE, min 40ns+4UI */

    /* VCO = bit clock x 2 because POSTDIV2 is enabled further down.
       F_VCO = CKREF x (NCODE + 3 + FCODE/2048) */
    uint32_t vco_mhz = dphy_clk_mhz * 2U;
    uint32_t ncode   = (vco_mhz / src_clk_mhz) - 3U;
    uint32_t fcode   = 0U;

#undef DPHY_CYCLES

    /* hold the host side of the D-PHY in reset while the analog part is configured */
    DSI->PHY_RSTZ = (0 << DSI_PHY_FORCEPLL_POS
                     | 0 << DSI_PHY_ENABLECLK_POS
                     | 0 << DSI_PHY_RSTZ_POS
                     | 1 << DSI_PHY_SHUTDOWNZ_POS);

    /*Set D-PHY bit clock*/
    /*F_VCO = CKREF x (NCODE + 3 + FCODE/2048); SSC is off, so FCODE stays 0 */
    DPHY->SSC3  = (
                      0x28    << REG_OC_STEP_SET_POS
                      | ncode << NCODE_T_SCPU_POS
                      | ncode << NCODE_SSC_POS
                  );
    DPHY->SSC2  = (
                      fcode << FCODE_T_SCPU_POS
                      | fcode << FCODE_SSC_POS
                  );
    /*enable DIV2: VCO / 2 is the actual bit clock*/
    DPHY->DF        = (DPHY->DF | REG_EN_ALL_DIV2_MSK);

    DPHY->ESCAPE_TX_CLK_0 &= REG_RQST_LP_MASK_ENABLE_CLR;

    DPHY->ESCAPE_TX_DATA_1 = (
                                 tx_lpx_time  << REG_TX_LPX_TIME_POS     /* 60ns    */
                                 | hs_exit_time << REG_HS_EXIT_TIME_POS  /* 100ns   */
                                 | 0x1100 << 0                           /* tx_init_time */
                             );
    DPHY->ESCAPE_TX_DATA_2 = (
                                 clk_pre_time  << REG_CLK_PRE_TIME_POS   /* 8UI  */
                                 | clk_zero_time << REG_CLK_ZERO_TIME_POS /* 300ns */
                                 | clk_prpr_time << REG_CLK_PRPR_TIME_POS /* 38-95ns */
                                 | clk_post_time << REG_CLK_POST_TIME_POS /* 60ns+52UI */
                             );
    DPHY->ESCAPE_TX_DATA_3 = (
                                 5 << REG_TX_VLD_TIME_POS                /* tx_vld_time default */
                                 | tx_tail_time << REG_TX_TAIL_TIME_POS  /* 60ns+4UI */
                                 | hs_zero_time << REG_HS_ZERO_TIME_POS  /* 100ns    */
                                 | hs_prpr_time << REG_HS_PRPR_TIME_POS  /* 40ns+4UI */
                             );
    DPHY->MPLL     &= REG_MPLL_PDIV_CLR;
    DPHY->WATCHDOG = (1 << REG_TIME0_CK_POS) | (DPHY->WATCHDOG & REG_TIME0_CK_CLR);

    /*toogle D-PHY*/
    DPHY->DF                        &= ~(REG_POSTDIV4_MSK | REG_POSTDIV3_MSK | REG_POSTDIV2_MSK);
    DPHY->CLOCK_GEN         &= ~(REG_CLK_LANE_EN_MSK | REG_LANE3_EN_MSK | REG_LANE2_EN_MSK |
                                 REG_LANE1_EN_MSK | REG_LANE0_EN_MSK | REG_RX_RST_N_MSK | REG_TX_RST_N_MSK);
    DPHY->DF                        &= ~(REG_PWCT_MSK | REG_MBIAS_EN_MSK);
    DPHY->WATCHDOG          &= ~RSTB_MSK;
    /*toogle to 1*/
    DPHY->DF                        |= (REG_POSTDIV4_MSK | REG_POSTDIV3_MSK | REG_POSTDIV2_MSK);
    DPHY->CLOCK_GEN         |= (lane_en | REG_RX_RST_N_MSK | REG_TX_RST_N_MSK);
    DPHY->DF                        |= (REG_PWCT_MSK | REG_MBIAS_EN_MSK);
    DPHY->WATCHDOG          |= (RSTB_MSK);

    /*power up the analog front end*/
    DPHY->DF |= REG_PWCT_MSK;
    DPHY->DF |= REG_MBIAS_EN_MSK;

    DPHY->MPLL |= REG_EN_TX_LP_LDO_MSK;

    DPHY->TXF |= REG_EN_LV_LDO_MSK;

    DPHY->CLOCK_GEN |= REG_MTX_EN_MSK;

    DPHY->WATCHDOG |= RSTB_MSK;

    DPHY->CLOCK_GEN |= REG_TX_RST_N_MSK;
    DPHY->CLOCK_GEN |= REG_RX_RST_N_MSK;

    /*only the lanes in use are enabled, the rest stay powered down*/
    DPHY->CLOCK_GEN = (DPHY->CLOCK_GEN & ~(REG_CLK_LANE_EN_MSK | REG_LANE3_EN_MSK |
                                           REG_LANE2_EN_MSK | REG_LANE1_EN_MSK | REG_LANE0_EN_MSK)) | lane_en;
}
#else
void DPHY_Init(uint32_t lane_number, uint32_t src_clk_mhz, uint32_t dphy_clk_mhz)
{
    uint32_t bit_ns10  = 10000U / dphy_clk_mhz;
    uint32_t byte_ns10 = 8U * bit_ns10;

    /* duration in 0.1ns -> lane byte clock cycles */
#define DPHY_CYCLES(ns10)       ((ns10) / byte_ns10)

    uint32_t tx_lpx_time  = DPHY_CYCLES(DPHY_NS(60));                           /* T_LPX, 50-75ns */
    uint32_t hs_exit_time = DPHY_CYCLES(DPHY_NS(
                                            100));                          /* T_HS_EXIT, min 100ns */
    uint32_t clk_pre_time = DPHY_CYCLES(DPHY_UI(8,
                                                bit_ns10));                  /* T_CLK_PREPARE, min 8UI */
    uint32_t clk_zero_time = DPHY_CYCLES(DPHY_NS(
                                             300));                         /* T_CLK_ZERO, min 300ns */
    uint32_t clk_prpr_time = DPHY_CYCLES(DPHY_NS(
                                             70));                          /* T_CLK_PREPARE, 38-95ns */
    uint32_t clk_post_time = DPHY_CYCLES(DPHY_NS(60) + DPHY_UI(52,
                                                               bit_ns10));  /* T_CLK_POST, min 60ns+52UI */
    uint32_t tx_tail_time = DPHY_CYCLES(DPHY_NS(60) + DPHY_UI(4,
                                                              bit_ns10));    /* T_HS_TRAIL, min 60ns+4UI */
    uint32_t hs_zero_time = DPHY_CYCLES(DPHY_NS(
                                            100));                          /* T_HS_ZERO, min 100ns */
    uint32_t hs_prpr_time = DPHY_CYCLES(DPHY_NS(40) + DPHY_UI(4,
                                                              bit_ns10));    /* T_HS_PREPARE, min 40ns+4UI */

    /* VCO = bit clock x 2 because POSTDIV2 is enabled further down.
       F_VCO = CKREF x (NCODE + 3 + FCODE/2048) */
    uint32_t vco_mhz = dphy_clk_mhz * 2U;
    uint32_t ncode   = (vco_mhz / src_clk_mhz) - 3U;
    uint32_t fcode   = 0U;

#undef DPHY_CYCLES

    /* hold the host side of the D-PHY in reset while the analog part is configured */
    DSI_PHY_RSTZ_TypeDef phy_rstz = {.d32 = 0};
    phy_rstz.b.phy_forcepll  = 0;
    phy_rstz.b.phy_enableclk = 0;
    phy_rstz.b.phy_rstz      = 0;
    phy_rstz.b.phy_shutdownz = 1;
    DSI->PHY_RSTZ = phy_rstz.d32;

    /*Set D-PHY bit clock: F_VCO = CKREF x (NCODE + 3 + FCODE/2048), SSC is off */
    DPHY_SSC3_TypeDef ssc3 = {.d32 = 0};
    ssc3.b.oc_step_set  = 0x28;
    ssc3.b.ncode_t_scpu = ncode;
    ssc3.b.ncode_ssc    = ncode;
    ssc3.b.oc_en        = 0;
    DPHY->SSC3 = ssc3.d32;

    DPHY_SSC2_TypeDef ssc2 = {.d32 = 0};
    ssc2.b.fcode_t_scpu = fcode;
    ssc2.b.fcode_ssc    = fcode;
    DPHY->SSC2 = ssc2.d32;

    /*enable DIV2: VCO / 2 is the actual bit clock*/
    DPHY_DF_TypeDef df = {.d32 = DPHY->DF};
    df.b.en_all_div2 = 1;
    DPHY->DF = df.d32;

    DPHY_ESCAPE_TX_CLK_0_TypeDef esc_clk = {.d32 = DPHY->ESCAPE_TX_CLK_0};
    esc_clk.b.rqst_lp_mask_enable = 0;
    DPHY->ESCAPE_TX_CLK_0 = esc_clk.d32;

    DPHY_ESCAPE_TX_DATA_1_TypeDef esc_d1 = {.d32 = 0};
    esc_d1.b.tx_init_time = 0x1100;
    esc_d1.b.hs_exit_time = hs_exit_time;   /* 100ns */
    esc_d1.b.tx_lpx_time  = tx_lpx_time;    /* 60ns  */
    DPHY->ESCAPE_TX_DATA_1 = esc_d1.d32;

    DPHY_ESCAPE_TX_DATA_2_TypeDef esc_d2 = {.d32 = 0};
    esc_d2.b.clk_post_time = clk_post_time; /* 60ns+52UI */
    esc_d2.b.clk_prpr_time = clk_prpr_time; /* 38-95ns   */
    esc_d2.b.clk_zero_time = clk_zero_time; /* 300ns     */
    esc_d2.b.clk_pre_time  = clk_pre_time;  /* 8UI       */
    DPHY->ESCAPE_TX_DATA_2 = esc_d2.d32;

    DPHY_ESCAPE_TX_DATA_3_TypeDef esc_d3 = {.d32 = 0};
    esc_d3.b.hs_prpr_time = hs_prpr_time;   /* 40ns+4UI  */
    esc_d3.b.hs_zero_time = hs_zero_time;   /* 100ns     */
    esc_d3.b.tx_tail_time = tx_tail_time;   /* 60ns+4UI  */
    esc_d3.b.tx_vld_time  = 5;
    DPHY->ESCAPE_TX_DATA_3 = esc_d3.d32;

    /* PDIV = 0 -> CKREF enters the PLL undivided */
    DPHY_MPLL_TypeDef mpll = {.d32 = DPHY->MPLL};
    mpll.b.mpll_pdiv = 0;
    DPHY->MPLL = mpll.d32;

    DPHY_WATCHDOG_TypeDef wdog = {.d32 = DPHY->WATCHDOG};
    wdog.b.time0_ck = 1;
    DPHY->WATCHDOG = wdog.d32;

    /*toogle D-PHY*/
    df.d32 = DPHY->DF;
    df.b.postdiv2 = 0;
    df.b.postdiv3 = 0;
    df.b.postdiv4 = 0;
    DPHY->DF = df.d32;

    DPHY_CLOCK_GEN_TypeDef clk = {.d32 = DPHY->CLOCK_GEN};
    clk.b.lane0_en = 0;
    clk.b.lane1_en = 0;
    clk.b.lane2_en = 0;
    clk.b.lane3_en = 0;
    clk.b.clk_lane_en = 0;
    clk.b.rx_rst_n = 0;
    clk.b.tx_rst_n = 0;
    DPHY->CLOCK_GEN = clk.d32;

    df.d32 = DPHY->DF;
    df.b.pwct = 0;
    df.b.mbias_en = 0;
    DPHY->DF = df.d32;

    wdog.d32 = DPHY->WATCHDOG;
    wdog.b.rstb = 0;
    DPHY->WATCHDOG = wdog.d32;

    /*toogle to 1*/
    df.d32 = DPHY->DF;
    df.b.postdiv2 = 1;
    df.b.postdiv3 = 1;
    df.b.postdiv4 = 1;
    DPHY->DF = df.d32;

    clk.d32 = DPHY->CLOCK_GEN;
    clk.b.lane0_en = 1;
    clk.b.lane1_en = (lane_number > 0);
    clk.b.lane2_en = (lane_number > 1);
    clk.b.lane3_en = (lane_number > 2);
    clk.b.clk_lane_en = 1;
    clk.b.rx_rst_n = 1;
    clk.b.tx_rst_n = 1;
    DPHY->CLOCK_GEN = clk.d32;

    df.d32 = DPHY->DF;
    df.b.pwct = 1;
    df.b.mbias_en = 1;
    DPHY->DF = df.d32;

    wdog.d32 = DPHY->WATCHDOG;
    wdog.b.rstb = 1;
    DPHY->WATCHDOG = wdog.d32;

    /*power up the analog front end*/
    df.d32 = DPHY->DF;
    df.b.pwct = 1;
    df.b.mbias_en = 1;
    DPHY->DF = df.d32;

    mpll.d32 = DPHY->MPLL;
    mpll.b.en_tx_lp_ldo = 1;
    DPHY->MPLL = mpll.d32;

    DPHY_TXF_TypeDef txf = {.d32 = DPHY->TXF};
    txf.b.en_lv_ldo = 1;
    DPHY->TXF = txf.d32;

    clk.d32 = DPHY->CLOCK_GEN;
    clk.b.mtx_en = 1;
    DPHY->CLOCK_GEN = clk.d32;

    wdog.d32 = DPHY->WATCHDOG;
    wdog.b.rstb = 1;
    DPHY->WATCHDOG = wdog.d32;

    clk.d32 = DPHY->CLOCK_GEN;
    clk.b.tx_rst_n = 1;
    clk.b.rx_rst_n = 1;
    DPHY->CLOCK_GEN = clk.d32;

    /*only the lanes in use are enabled, the rest stay powered down*/
    clk.d32 = DPHY->CLOCK_GEN;
    clk.b.lane0_en = 1;
    clk.b.lane1_en = (lane_number > 1);
    clk.b.lane2_en = (lane_number > 2);
    clk.b.lane3_en = (lane_number > 3);
    clk.b.clk_lane_en = 1;
    DPHY->CLOCK_GEN = clk.d32;
}
#endif
