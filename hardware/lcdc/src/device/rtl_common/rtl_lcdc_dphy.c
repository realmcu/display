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
 *                        Public Functions
 *============================================================================*/
void DPHY_Init(uint32_t NumberOfLanes)
{
    /* clock lane plus the data lanes actually in use */
    uint32_t lane_en = REG_CLK_LANE_EN_MSK | REG_LANE0_EN_MSK;
    if (NumberOfLanes > 1)
    {
        lane_en |= REG_LANE1_EN_MSK;
    }
    if (NumberOfLanes > 2)
    {
        lane_en |= REG_LANE2_EN_MSK;
    }
    if (NumberOfLanes > 3)
    {
        lane_en |= REG_LANE3_EN_MSK;
    }

    /* hold the host side of the D-PHY in reset while the analog part is configured */
    DSI->PHY_RSTZ = (0 << DSI_PHY_FORCEPLL_POS
                     | 0 << DSI_PHY_ENABLECLK_POS
                     | 0 << DSI_PHY_RSTZ_POS
                     | 1 << DSI_PHY_SHUTDOWNZ_POS);

    /*Set D-PHY bit clock*/
    /*source 800MHz*/
    DPHY->SSC3  = (
                      0x28    <<  REG_OC_STEP_SET_POS
                      | 17        <<  NCODE_T_SCPU_POS
                      | 17        <<  NCODE_SSC_POS
                  );
    DPHY->SSC2  = (
                      0           <<  FCODE_T_SCPU_POS
                      | 0         <<  FCODE_SSC_POS
                  );
    /*enable DIV2 400MHz*/
    DPHY->DF        = (DPHY->DF | REG_EN_ALL_DIV2_MSK);

    DPHY->ESCAPE_TX_CLK_0 &= REG_RQST_LP_MASK_ENABLE_CLR;

    DPHY->ESCAPE_TX_DATA_1 = (
                                 TX_LPX_TIME   << REG_TX_LPX_TIME_POS                  // reg_tx_lpx_time   65ns
                                 | HS_EXIT_TIME  << REG_HS_EXIT_TIME_POS                 // reg_hs_exit_time 100ns
                                 | 0x1100 << 0                                                                   // tx_init_time
                             );
    DPHY->ESCAPE_TX_DATA_2 = (
                                 CLK_PRE_TIME    << REG_CLK_PRE_TIME_POS                 // clk_pre_time 8UI
                                 | CLK_ZERO_TIME     << REG_CLK_ZERO_TIME_POS                // clk_zero_time  300ns
                                 | CLK_PRPR_TIME     << REG_CLK_PRPR_TIME_POS                // clk_prpr_time
                                 | CLK_POST_TIME     << REG_CLK_POST_TIME_POS                // clk_post_time
                             );
    DPHY->ESCAPE_TX_DATA_3 = (
                                 5  << REG_TX_VLD_TIME_POS                                               // tx_vld_time default
                                 | TX_TAIL_TIME      << REG_TX_TAIL_TIME_POS                 // tx_tail_time
                                 | HS_ZERO_TIME      << REG_HS_ZERO_TIME_POS                 // hs_zero_time
                                 | HS_PRPR_TIME      << REG_HS_PRPR_TIME_POS                 // hs_prpr_time
                             );
    DPHY->MPLL                              &=      REG_MPLL_PDIV_CLR;
    DPHY->WATCHDOG                      = (1 << REG_TIME0_CK_POS) | (DPHY->WATCHDOG & REG_TIME0_CK_CLR);

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

