/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include "rtl876x.h"

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
 *              LCDC DSI Registers and Field Descriptions
 *----------------------------------------------------------------------------*
 * Bit fields follow the DWC MIPI DSI Host Controller databook (v1.51a).
 * Each union provides d32 (whole reg) and b.<field> (bitfield) access.
 *============================================================================*/

/* PWR_UP  (0x04) */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        uint32_t shutdownz: 1;
        const uint32_t reserved_0: 31;
        } b;
    } DSI_PWR_UP_TypeDef;

    /* CLKMGR_CFG  (0x08) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t tx_esc_clk_division: 8;
                uint32_t to_clk_division: 8;
                const uint32_t reserved_0: 16;
            } b;
        } DSI_CLKMGR_CFG_TypeDef;

    /* DPI_VCID  (0x0c) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t dpi_vcid: 2;
                const uint32_t reserved_0: 30;
            } b;
        } DSI_DPI_VCID_TypeDef;

    /* DPI_COLOR_CODING  (0x10) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t dpi_color_coding: 4;
                const uint32_t reserved_0: 4;
                uint32_t loosely18_en: 1;
                const uint32_t reserved_1: 23;
            } b;
        } DSI_DPI_COLOR_CODING_TypeDef;

    /* DPI_CFG_POL  (0x14) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t dataen_active_low: 1;
                uint32_t vsync_active_low: 1;
                uint32_t hsync_active_low: 1;
                uint32_t shutd_active_low: 1;
                uint32_t colorm_active_low: 1;
                const uint32_t reserved_0: 27;
            } b;
        } DSI_DPI_CFG_POL_TypeDef;

    /* DPI_LP_CMD_TIM  (0x18) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t invact_lpcmd_time: 8;
                const uint32_t reserved_0: 8;
                uint32_t outvact_lpcmd_time: 8;
                const uint32_t reserved_1: 8;
            } b;
        } DSI_DPI_LP_CMD_TIM_TypeDef;

    /* PCKHDL_CFG  (0x2c) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t eotp_tx_en: 1;
                uint32_t eotp_rx_en: 1;
                uint32_t bta_en: 1;
                uint32_t ecc_rx_en: 1;
                uint32_t crc_rx_en: 1;
                uint32_t eotp_tx_lp_en: 1;
                const uint32_t reserved_0: 26;
            } b;
        } DSI_PCKHDL_CFG_TypeDef;

    /* GEN_VCID  (0x30) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t gen_vcid_rx: 2;
                const uint32_t reserved_0: 6;
                uint32_t gen_vcid_tear_auto: 2;
                const uint32_t reserved_1: 6;
                uint32_t gen_vcid_tx_auto: 2;
                const uint32_t reserved_2: 14;
            } b;
        } DSI_GEN_VCID_TypeDef;

    /* MODE_CFG  (0x34) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t cmd_video_mode: 1;
                const uint32_t reserved_0: 31;
            } b;
        } DSI_MODE_CFG_TypeDef;

    /* VID_MODE_CFG  (0x38) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t vid_mode_type: 2;
                const uint32_t reserved_0: 6;
                uint32_t lp_vsa_en: 1;
                uint32_t lp_vbp_en: 1;
                uint32_t lp_vfp_en: 1;
                uint32_t lp_vact_en: 1;
                uint32_t lp_hbp_en: 1;
                uint32_t lp_hfp_en: 1;
                uint32_t frame_bta_ack_en: 1;
                uint32_t lp_cmd_en: 1;
                uint32_t vpg_en: 1;
                const uint32_t reserved_1: 3;
                uint32_t vpg_mode: 1;
                const uint32_t reserved_2: 3;
                uint32_t vpg_orientation: 1;
                const uint32_t reserved_3: 7;
            } b;
        } DSI_VID_MODE_CFG_TypeDef;

    /* VID_PKT_SIZE  (0x3c) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t vid_pkt_size: 14;
                const uint32_t reserved_0: 18;
            } b;
        } DSI_VID_PKT_SIZE_TypeDef;

    /* VID_NUM_CHUNKS  (0x40) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t vid_num_chunks: 13;
                const uint32_t reserved_0: 19;
            } b;
        } DSI_VID_NUM_CHUNKS_TypeDef;

    /* VID_NULL_SIZE  (0x44) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t vid_null_size: 13;
                const uint32_t reserved_0: 19;
            } b;
        } DSI_VID_NULL_SIZE_TypeDef;

    /* VID_HSA_TIME  (0x48) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t vid_hsa_time: 12;
                const uint32_t reserved_0: 20;
            } b;
        } DSI_VID_HSA_TIME_TypeDef;

    /* VID_HBP_TIME  (0x4c) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t vid_hbp_time: 12;
                const uint32_t reserved_0: 20;
            } b;
        } DSI_VID_HBP_TIME_TypeDef;

    /* VID_HLINE_TIME  (0x50) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t vid_hline_time: 15;
                const uint32_t reserved_0: 17;
            } b;
        } DSI_VID_HLINE_TIME_TypeDef;

    /* VID_VSA_LINES  (0x54) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t vsa_lines: 10;
                const uint32_t reserved_0: 22;
            } b;
        } DSI_VID_VSA_LINES_TypeDef;

    /* VID_VBP_LINES  (0x58) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t vbp_lines: 10;
                const uint32_t reserved_0: 22;
            } b;
        } DSI_VID_VBP_LINES_TypeDef;

    /* VID_VFP_LINES  (0x5c) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t vfp_lines: 10;
                const uint32_t reserved_0: 22;
            } b;
        } DSI_VID_VFP_LINES_TypeDef;

    /* VID_VACTIVE_LINES  (0x60) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t v_active_lines: 14;
                const uint32_t reserved_0: 18;
            } b;
        } DSI_VID_VACTIVE_LINES_TypeDef;

    /* EDPI_CMD_SIZE  (0x64) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t edpi_allowed_cmd_size: 16;
                const uint32_t reserved_0: 16;
            } b;
        } DSI_EDPI_CMD_SIZE_TypeDef;

    /* CMD_MODE_CFG  (0x68) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t tear_fx_en: 1;
                uint32_t ack_rqst_en: 1;
                const uint32_t reserved_0: 6;
                uint32_t gen_sw_0p_tx: 1;
                uint32_t gen_sw_1p_tx: 1;
                uint32_t gen_sw_2p_tx: 1;
                uint32_t gen_sr_0p_tx: 1;
                uint32_t gen_sr_1p_tx: 1;
                uint32_t gen_sr_2p_tx: 1;
                uint32_t gen_lw_tx: 1;
                const uint32_t reserved_1: 1;
                uint32_t dcs_sw_0p_tx: 1;
                uint32_t dcs_sw_1p_tx: 1;
                uint32_t dcs_sr_0p_tx: 1;
                uint32_t dcs_lw_tx: 1;
                const uint32_t reserved_2: 4;
                uint32_t max_rd_pkt_size: 1;
                const uint32_t reserved_3: 7;
            } b;
        } DSI_CMD_MODE_CFG_TypeDef;

    /* GEN_HDR  (0x6c) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t gen_dt: 6;
                uint32_t gen_vc: 2;
                uint32_t gen_wc_lsbyte: 8;
                uint32_t gen_wc_msbyte: 8;
                const uint32_t reserved_0: 8;
            } b;
        } DSI_GEN_HDR_TypeDef;

    /* GEN_PLD_DATA  (0x70) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t gen_pld_b1: 8;
                uint32_t gen_pld_b2: 8;
                uint32_t gen_pld_b3: 8;
                uint32_t gen_pld_b4: 8;
            } b;
        } DSI_GEN_PLD_DATA_TypeDef;

    /* CMD_PKT_STATUS  (0x74) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t gen_cmd_empty: 1;
                const uint32_t gen_cmd_full: 1;
                const uint32_t gen_pld_w_empty: 1;
                const uint32_t gen_pld_w_full: 1;
                const uint32_t gen_pld_r_empty: 1;
                const uint32_t gen_pld_r_full: 1;
                const uint32_t gen_rd_cmd_busy: 1;
                const uint32_t reserved_0: 1;
                const uint32_t dbi_cmd_empy: 1;
                const uint32_t dbi_cmd_full: 1;
                const uint32_t dbi_pld_w_empty: 1;
                const uint32_t dbi_pld_w_full: 1;
                const uint32_t dbi_pld_r_empty: 1;
                const uint32_t dbi_pld_r_full: 1;
                const uint32_t dbi_rd_cmd_busy: 1;
                const uint32_t reserved_1: 1;
                const uint32_t gen_buff_cmd_empty: 1;
                const uint32_t gen_buff_cmd_full: 1;
                const uint32_t gen_buff_pld_empty: 1;
                const uint32_t gen_buff_pld_full: 1;
                const uint32_t reserved_2: 4;
                const uint32_t dbi_buff_cmd_empty: 1;
                const uint32_t dbi_buff_cmd_full: 1;
                const uint32_t dbi_buff_pld_empty: 1;
                const uint32_t dbi_buff_pld_full: 1;
                const uint32_t reserved_3: 4;
            } b;
        } DSI_CMD_PKT_STATUS_TypeDef;

    /* TO_CNT_CFG  (0x78) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t lprx_to_cnt: 16;
                uint32_t hstx_to_cnt: 16;
            } b;
        } DSI_TO_CNT_CFG_TypeDef;

    /* HS_RD_TO_CNT  (0x7c) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t hs_rd_to_cnt: 16;
                const uint32_t reserved_0: 16;
            } b;
        } DSI_HS_RD_TO_CNT_TypeDef;

    /* LP_RD_TO_CNT  (0x80) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t lp_rd_to_cnt: 16;
                const uint32_t reserved_0: 16;
            } b;
        } DSI_LP_RD_TO_CNT_TypeDef;

    /* HS_WR_TO_CNT  (0x84) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t hs_wr_to_cnt: 16;
                const uint32_t reserved_0: 8;
                uint32_t presp_to_mode: 1;
                const uint32_t reserved_1: 7;
            } b;
        } DSI_HS_WR_TO_CNT_TypeDef;

    /* LP_WR_TO_CNT  (0x88) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t lp_wr_to_cnt: 16;
                const uint32_t reserved_0: 16;
            } b;
        } DSI_LP_WR_TO_CNT_TypeDef;

    /* BTA_TO_CNT  (0x8c) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t bta_to_cnt: 16;
                const uint32_t reserved_0: 16;
            } b;
        } DSI_BTA_TO_CNT_TypeDef;

    /* LPCLK_CTRL  (0x94) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t phy_txrequestclkhs: 1;
                uint32_t auto_clklane_ctrl: 1;
                const uint32_t reserved_0: 30;
            } b;
        } DSI_LPCLK_CTRL_TypeDef;

    /* PHY_TMR_LPCLK_CFG  (0x98) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t phy_clklp2hs_time: 10;
                const uint32_t reserved_0: 6;
                uint32_t phy_clkhs2lp_time: 10;
                const uint32_t reserved_1: 6;
            } b;
        } DSI_PHY_TMR_LPCLK_CFG_TypeDef;

    /* PHY_TMR_CFG  (0x9c) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t phy_lp2hs_time: 10;
                const uint32_t reserved_0: 6;
                uint32_t phy_hs2lp_time: 10;
                const uint32_t reserved_1: 6;
            } b;
        } DSI_PHY_TMR_CFG_TypeDef;

    /* PHY_RSTZ  (0xa0) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t phy_shutdownz: 1;
                uint32_t phy_rstz: 1;
                uint32_t phy_enableclk: 1;
                uint32_t phy_forcepll: 1;
                const uint32_t reserved_0: 28;
            } b;
        } DSI_PHY_RSTZ_TypeDef;

    /* PHY_IF_CFG  (0xa4) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t n_lanes: 2;
                const uint32_t reserved_0: 6;
                uint32_t phy_stop_wait_time: 8;
                const uint32_t reserved_1: 16;
            } b;
        } DSI_PHY_IF_CFG_TypeDef;

    /* PHY_STATUS  (0xb0) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t phy_lock: 1;
                const uint32_t phy_direction: 1;
                const uint32_t phy_stopstateclklane: 1;
                const uint32_t phy_ulpsactivenotclk: 1;
                const uint32_t phy_stopstate0lane: 1;
                const uint32_t phy_ulpsactivenot0lane: 1;
                const uint32_t phy_rxulpsesc0lane: 1;
                const uint32_t phy_stopstate1lane: 1;
                const uint32_t phy_ulpsactivenot1lane: 1;
                const uint32_t phy_stopstate2lane: 1;
                const uint32_t phy_ulpsactivenot2lane: 1;
                const uint32_t phy_stopstate3lane: 1;
                const uint32_t phy_ulpsactivenot3lane: 1;
                const uint32_t reserved_0: 19;
            } b;
        } DSI_PHY_STATUS_TypeDef;

    /* PHY_TST_CTRL0  (0xb4) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t phy_testclr: 1;
                uint32_t phy_testclk: 1;
                const uint32_t reserved_0: 30;
            } b;
        } DSI_PHY_TST_CTRL0_TypeDef;

    /* PHY_TST_CTRL1  (0xb8) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t phy_testdin: 8;
                const uint32_t phy_testdout: 8;
                uint32_t phy_testen: 1;
                uint32_t phy_testsel: 1;
                const uint32_t reserved_0: 14;
            } b;
        } DSI_PHY_TST_CTRL1_TypeDef;

    /* AUTO_ULPS_MODE  (0xe0) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t auto_ulps: 1;
                const uint32_t reserved_0: 15;
                uint32_t pll_off_ulps: 1;
                uint32_t pre_pll_off_req: 1;
                const uint32_t reserved_1: 14;
            } b;
        } DSI_AUTO_ULPS_MODE_TypeDef;

    /* AUTO_ULPS_ENTRY_DELAY  (0xe4) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t ulps_entry_delay: 32;
            } b;
        } DSI_AUTO_ULPS_ENTRY_DELAY_TypeDef;

    /* AUTO_ULPS_WAKEUP_TIME  (0xe8) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t twakeup_clk_div: 16;
                uint32_t twakeup_cnt: 16;
            } b;
        } DSI_AUTO_ULPS_WAKEUP_TIME_TypeDef;

    /* PHY_TMR_RD_CFG  (0xf4) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t max_rd_time: 15;
                const uint32_t reserved_0: 17;
            } b;
        } DSI_PHY_TMR_RD_CFG_TypeDef;

    /* AUTO_ULPS_MIN_TIME  (0xf8) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t ulps_min_time: 12;
                const uint32_t reserved_0: 20;
            } b;
        } DSI_AUTO_ULPS_MIN_TIME_TypeDef;

    /* PHY_MODE  (0xfc) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t phy_mode: 1;
                const uint32_t reserved_0: 31;
            } b;
        } DSI_PHY_MODE_TypeDef;

    /* EDPI_TE_HW_CFG  (0x11c) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t hw_tear_effect_on: 1;
                uint32_t hw_tear_effect_gen: 1;
                const uint32_t reserved_0: 2;
                uint32_t hw_set_scan_line: 1;
                const uint32_t reserved_1: 11;
                uint32_t scan_line_parameter: 16;
            } b;
        } DSI_EDPI_TE_HW_CFG_TypeDef;

    /* INT_ST0  (0xbc) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t int_st0: 21;
                const uint32_t reserved_0: 11;
            } b;
        } DSI_INT_ST0_TypeDef;

    /* INT_ST1  (0xc0) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t int_st1: 21;
                const uint32_t reserved_0: 11;
            } b;
        } DSI_INT_ST1_TypeDef;

    /* INT_MSK0  (0xc4) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t int_msk0: 21;
                const uint32_t reserved_0: 11;
            } b;
        } DSI_INT_MSK0_TypeDef;

    /* INT_MSK1  (0xc8) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t int_msk1: 21;
                const uint32_t reserved_0: 11;
            } b;
        } DSI_INT_MSK1_TypeDef;

    /* INT_FORCE0  (0xd8) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t int_force0: 32;
            } b;
        } DSI_INT_FORCE0_TypeDef;

    /* INT_FORCE1  (0xdc) */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t int_force1: 32;
            } b;
        } DSI_INT_FORCE1_TypeDef;


    /*============================================================================*
    *                         DSI Declaration
    *============================================================================*/
#define DSI                            ((DSI_TypeDef           *) MIPI_REG_BASE)

    /** \defgroup DSI         DSI
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
     * \defgroup    DSI_PHY_Mode DSI PHY Mode
     * \{
     * \ingroup     DSI_Exported_Constants
     */
#define DSI_DPHY     0
#define DSI_CPHY     1

#define IS_DSI_PHY_MODE(MODE) (((MODE) == DSI_DPHY) || ((MODE) == DSI_CPHY))

    /** End of DSI_PHY_Mode
         * \}
         */

    /**
     * \defgroup    DSI_N_Lanes DSI N Lanes
     * \{
     * \ingroup     DSI_Exported_Constants
     */
#define DSI_1_LANE     0            /*!< Lane 0 */
#define DSI_2_LANE     1            /*!< Lane 0, lane 1 */
#define DSI_3_LANE     2            /*!< Lane 0, lane 1, lane 2 */
#define DSI_4_LANE     3            /*!< Lane 0, lane 1, lane 2, lane 3 */

#define IS_DSI_N_Lanes(MODE) (((MODE) == DSI_1_LANE) || ((MODE) == DSI_2_LANE) || ((MODE) == DSI_3_LANE) || ((MODE) == DSI_4_LANE))

    /*!< Lane count as written by the caller (1 to 4), not the encoded register value. */
#define IS_DSI_LANE_COUNT(COUNT) (((COUNT) >= 1UL) && ((COUNT) <= 4UL))

    /** End of DSI_N_Lanes
         * \}
         */

    /**
     * \defgroup    DSI_Color_Coding DSI Color Coding
     * \{
     * \ingroup     DSI_Exported_Constants
     */
#define DSI_COLOR_RGB565     0          /*!< RGB565 color coding mode. */
#define DSI_COLOR_RGB888     5          /*!< RGB888 color coding mode. */

    /*!< DPI_COLOR_CODING is 4 bits wide, so any value below 16 fits the register. */
#define IS_DSI_COLOR_CODING(CODING) ((CODING) <= 0xFUL)

    /** End of DSI_Color_Coding
         * \}
         */

    /**
     * \defgroup    DSI_Mode DSI Mode
     * \{
     * \ingroup     DSI_Exported_Constants
     */
#define DSI_MODE_VIDEO       (0)            /*!< Video mode. */
#define DSI_MODE_CMD         (1)            /*!< Command mode. */

#define IS_DSI_MODE(MODE) (((MODE) == DSI_MODE_VIDEO) || ((MODE) == DSI_MODE_CMD))

    /** End of DSI_Mode
         * \}
         */

    /**
     * \defgroup    DSI_HS_POLARITY DSI HS Polarity
     * \{
     * \ingroup     DSI_Exported_Constants
     */
#define DSI_HSPOL_AL                (0)            /*!< Horizontal synchronization polarity is active low. */
#define DSI_HSPOL_AH                (1)            /*!< Horizontal synchronization polarity is active high. */

#define IS_DSI_HS_POLARITY(MODE) (((MODE) == DSI_HSPOL_AL) || ((MODE) == DSI_HSPOL_AH))

    /** End of DSI_HS_POLARITY
         * \}
         */

    /**
     * \defgroup    DSI_VS_POLARITY DSI VS Polarity
     * \{
     * \ingroup     DSI_Exported_Constants
     */
#define DSI_VSPOL_AL                (0)            /*!< Vertical synchronization polarity is active low. */
#define DSI_VSPOL_AH                (1)            /*!< Vertical synchronization polarity is active high. */

#define IS_DSI_VS_POLARITY(MODE) (((MODE) == DSI_VSPOL_AL) || ((MODE) == DSI_VSPOL_AH))

    /** End of DSI_VS_POLARITY
         * \}
         */

    /**
     * \defgroup    DSI_DE_POLARITY DSI DE Polarity
     * \{
     * \ingroup     DSI_Exported_Constants
     */
#define DSI_DEPOL_AL                (0)            /*!< Data data polarity is active low. */
#define DSI_DEPOL_AH                (1)            /*!< Data data polarity is active high. */

#define IS_DSI_DE_POLARITY(MODE) (((MODE) == DSI_DEPOL_AL) || ((MODE) == DSI_DEPOL_AH))

    /** End of DSI_DE_POLARITY
         * \}
         */


    /**
     * \defgroup    DSI_LP DSI LP Command Transport
     *
     *              CMD_MODE_CFG selects, per packet type, whether the command is
     *              sent as a low-power escape (LPDT) or as a high-speed packet.
     *              These masks match the CMD_MODE_CFG bit positions, so
     *              \ref DSI_Init copies the mask straight into the register.
     * \{
     * \ingroup     DSI_Exported_Constants
     */
#define DSI_LP_GEN_SHORT_WRITE_0P   (1UL << 8)   /*!< Generic short write, no parameter.   */
#define DSI_LP_GEN_SHORT_WRITE_1P   (1UL << 9)   /*!< Generic short write, one parameter.  */
#define DSI_LP_GEN_SHORT_WRITE_2P   (1UL << 10)  /*!< Generic short write, two parameters. */
#define DSI_LP_GEN_SHORT_READ_0P    (1UL << 11)  /*!< Generic short read, no parameter.    */
#define DSI_LP_GEN_SHORT_READ_1P    (1UL << 12)  /*!< Generic short read, one parameter.   */
#define DSI_LP_GEN_SHORT_READ_2P    (1UL << 13)  /*!< Generic short read, two parameters.  */
#define DSI_LP_GEN_LONG_WRITE       (1UL << 14)  /*!< Generic long write.                  */
#define DSI_LP_DCS_SHORT_WRITE_0P   (1UL << 16)  /*!< DCS short write, no parameter.       */
#define DSI_LP_DCS_SHORT_WRITE_1P   (1UL << 17)  /*!< DCS short write, one parameter.      */
#define DSI_LP_DCS_SHORT_READ_0P    (1UL << 18)  /*!< DCS short read, no parameter.        */
#define DSI_LP_DCS_LONG_WRITE       (1UL << 19)  /*!< DCS long write.                      */
#define DSI_LP_MAX_READ_PACKET      (1UL << 24)  /*!< Send the max-size read packet in LP. */

    /*!< Every command packet type goes out in low-power. The usual panel setting. */
#define DSI_LP_CMD_ALL              (DSI_LP_GEN_SHORT_WRITE_0P | DSI_LP_GEN_SHORT_WRITE_1P | \
                                     DSI_LP_GEN_SHORT_WRITE_2P | DSI_LP_GEN_SHORT_READ_0P  | \
                                     DSI_LP_GEN_SHORT_READ_1P  | DSI_LP_GEN_SHORT_READ_2P  | \
                                     DSI_LP_GEN_LONG_WRITE     | DSI_LP_DCS_SHORT_WRITE_0P | \
                                     DSI_LP_DCS_SHORT_WRITE_1P | DSI_LP_DCS_SHORT_READ_0P  | \
                                     DSI_LP_DCS_LONG_WRITE)

    /*!< Every bit of CMD_MODE_CFG that a DSI_LP mask can own: DSI_LP_CMD_ALL plus
         DSI_LP_MAX_READ_PACKET. What DSI_ConfigLPM leaves untouched. */
#define DSI_LP_ALL_MASK             (DSI_LP_CMD_ALL | DSI_LP_MAX_READ_PACKET)

    /** End of DSI_LP
         * \}
         */

    /**
     * \defgroup    DSI_Polarity DSI Signal Polarity
     *
     *              The DPI pin polarities are independent bits, so
     *              \ref DSI_ConfigTypeDef::Polarity wants them as one mask.
     *              They match the DPI_CFG_POL bit positions.
     * \{
     * \ingroup     DSI_Exported_Constants
     */
#define DSI_POL_DATAEN_LOW          (1UL << 0)   /*!< dpidataen is active low. */
#define DSI_POL_VSYNC_LOW           (1UL << 1)   /*!< dpivsync is active low.  */
#define DSI_POL_HSYNC_LOW           (1UL << 2)   /*!< dpihsync is active low.  */
#define DSI_POL_SHUTD_LOW           (1UL << 3)   /*!< dpishutdn is active low. */
#define DSI_POL_COLORM_LOW          (1UL << 4)   /*!< dpicolorm is active low. */

#define IS_DSI_POLARITY(POL) (((POL) & ~0x1FUL) == 0UL)

    /** End of DSI_Polarity
         * \}
         */

    /**
     * \defgroup    DSI_FLAG DSI Host Behaviour Flags
     *
     *              The remaining on/off switches the host needs at init, as one
     *              mask in \ref DSI_ConfigTypeDef::Flags.
     * \{
     * \ingroup     DSI_Exported_Constants
     */
#define DSI_FLAG_CLKLANE_STOP_IDLE  (1UL << 0)   /*!< LPCLK_CTRL.auto_clklane_ctrl: park the clock lane in LP while idle. */
#define DSI_FLAG_EOTP_TX            (1UL << 1)   /*!< PCKHDL_CFG.eotp_tx_en: append end-of-transmission to video packets. */
#define DSI_FLAG_BTA                (1UL << 2)   /*!< PCKHDL_CFG.bta_en: allow a bus turnaround, required by \ref DSI_Read. */

#define IS_DSI_FLAGS(FLAG) (((FLAG) & ~0x7UL) == 0UL)

    /** End of DSI_FLAG
         * \}
         */

    /**
     * \defgroup    DSI_PHY_DEFAULT D-PHY Defaults
     *
     *              Values fixed by this chip's D-PHY databook. They are the same
     *              for every panel, so \ref DSI_Init programs them without the
     *              caller having to repeat them.
     * \{
     * \ingroup     DSI_Exported_Constants
     */

    /** PHY_TMR_LPCLK_CFG.phy_clkhs2lp_time. */
#define DSI_PHY_DEFAULT_CLK_HS2LP       (0x24U)
    /** PHY_TMR_LPCLK_CFG.phy_clklp2hs_time. */
#define DSI_PHY_DEFAULT_CLK_LP2HS       (0x3dU)
    /** PHY_TMR_CFG.phy_hs2lp_time. */
#define DSI_PHY_DEFAULT_DATA_HS2LP      (0x26U)
    /** PHY_TMR_CFG.phy_lp2hs_time. */
#define DSI_PHY_DEFAULT_DATA_LP2HS      (0xA1U)
    /** CLKMGR_CFG.to_clk_division: about 90ms of HS/LP timeout at a 50MHz lanebyteclk. */
#define DSI_PHY_DEFAULT_TIMEOUT_CLK_DIV (0x45U)
    /** Target TxClkEsc rate used to derive the divider; the D-PHY allows 2 to 20MHz.
     *  Dividing lanebyteclk by this keeps the escape clock at or below it without
     *  falling under the 2MHz floor. */
#define DSI_PHY_ESC_CLK_TARGET_KHZ      (20000U)

    /** eDPI line buffer depth, the largest command the panel may hand over at once. */
#define DSI_EDPI_DEFAULT_CMD_SIZE       (598U)

    /** EDPI_CMD_SIZE.edpi_allowed_cmd_size is 16 bits wide. */
#define IS_DSI_DPI_COMMAND_SIZE(SIZE)   ((SIZE) <= 0xFFFFUL)

    /** End of DSI_PHY_DEFAULT
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

#define IS_DSI_SHORT_WRITE_PKT(Type) (((Type) == DSI_DCS_SHORT_PKT_WRITE_P0) || ((Type) == DSI_DCS_SHORT_PKT_WRITE_P1) || \
                                      ((Type) == DSI_GEN_SHORT_PKT_WRITE_P0) || ((Type) == DSI_GEN_SHORT_PKT_WRITE_P1) || \
                                      ((Type) == DSI_GEN_SHORT_PKT_WRITE_P2))

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

#define IS_DSI_LONG_WRITE_PKT(Type) (((Type) == DSI_DCS_LONG_PKT_WRITE) || ((Type) == DSI_GEN_LONG_PKT_WRITE))

    /** End of DSI_LONG_WRITE_PKT_Data_Type
         * \}
         */

    /**
     * \defgroup    DSI_SHORT_READ_PKT_Data_Type DSI SHORT READ PKT Data Type
     * \{
     * \ingroup     DSI_Exported_Constants
     */
#define DSI_DCS_SHORT_PKT_READ      0x00000006U /*!< DCS short read                     */
#define DSI_DCS_SHORT_PKT_READ_P0   0x00000006U /*!< DCS short read, no parameters      */
#define DSI_DCS_SHORT_PKT_READ_P1   0x00000016U /*!< DCS short read, one parameter      */
#define DSI_GEN_SHORT_PKT_READ_P0   0x00000004U /*!< Generic short read, no parameters  */
#define DSI_GEN_SHORT_PKT_READ_P1   0x00000014U /*!< Generic short read, one parameter  */
#define DSI_GEN_SHORT_PKT_READ_P2   0x00000024U /*!< Generic short read, two parameters */

#define IS_DSI_READ_PKT(Type) (((Type) == DSI_DCS_SHORT_PKT_READ) || ((Type) == DSI_DCS_SHORT_PKT_READ_P0) || \
                               ((Type) == DSI_DCS_SHORT_PKT_READ_P1) || ((Type) == DSI_GEN_SHORT_PKT_READ_P0) || \
                               ((Type) == DSI_GEN_SHORT_PKT_READ_P1) || ((Type) == DSI_GEN_SHORT_PKT_READ_P2))

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

#define IS_VIDEO_MODE_TYPE(TYPE) (((TYPE) == VIDEO_NON_BURST_WITH_SYNC_PULSES) || ((TYPE) == VIDEO_NON_BURST_WITH_SYNC_EVENTS) || \
                                  ((TYPE) == VIDEO_BURST_MODE_2) || ((TYPE) == VIDEO_BURST_MODE_3))

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

#define IS_VIDEO_VPG_MODE(MODE) (((MODE) == VIDEO_VPG_MODE_V_AND_H) || ((MODE) == VIDEO_VPG_MODE_ONLY_V))
#define IS_VIDEO_VPG_ORIENTATION(ORIENTATION) (((ORIENTATION) == VIDEO_VPG_ORIENTATION_V) || ((ORIENTATION) == VIDEO_VPG_ORIENTATION_H))

    /** End of DSI_VPG
         * \}
         */

    /**
     * \defgroup    DSI_PHY DSI PHY
     * \{
     * \ingroup     DSI_Exported_Constants
     */
#define DSI_STATUS_ERROR        ((FlagStatus)0)
#define DSI_STATUS_OK           ((FlagStatus)1)
#ifndef DSI_POLL_TIMEOUT
#define DSI_POLL_TIMEOUT        0x100000U
#endif

    /*!< CMD_MODE_CFG / PCKHDL_CFG bit 2: enable the Bus Turn-Around request. */
#define DSI_PCKHDL_BTA          (1UL << 2)

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
     *              Holds only what genuinely varies per panel. Everything else
     *              is fixed by the databook and programmed by \ref DSI_Init
     *              without the caller passing it:
     *
     *              | Register field            | Value                              |
     *              |:--------------------------|:-----------------------------------|
     *              | PHY_IF_CFG.stop_wait_time | 0                                  |
     *              | INT_MSK0 / INT_MSK1       | 0x1FFFFF (every source unmasked)   |
     *              | TO_CNT_CFG HS/LP RX       | 0xFFFF (longest timeout)           |
     *              | PIN_LP_CMD_TIM in/out     | 0 (filled in by DSI_ConfigVideoMode)|
     *              | GEN_VCID, DPI_VCID        | 0                                  |
     *              | HS/LP RD/WR/BTA timeouts  | 0 (disabled)                       |
     *              | PHY_TMR_CFG / LPCLK_CFG   | \ref DSI_PHY_DEFAULT group          |
     *              | CLKMGR_CFG divisders      | timeout from \ref DSI_PHY_DEFAULT_TIMEOUT_CLK_DIV,
     *                                           TxClkEsc derived from LaneByteClkKHz |
     *              | CMD_MODE_CFG LP bits      | all set, see \ref DSI_LP_CMD_ALL    |
     *              | PCKHDL_CFG / LPCLK_CTRL   | EoTp and BTA off, clock lane parked in LP |
     *
     * \ingroup     DSI_Exported_Types
     */
    typedef struct
{
    uint32_t LaneCount;         /*!< Active data lanes, 1 to 4. Written as LaneCount - 1. */
    uint32_t ColorCoding;       /*!< \ref DSI_COLOR_RGB565 or \ref DSI_COLOR_RGB888. */
    uint32_t LaneByteClkKHz;    /*!< lanebyteclk = per-lane HS bit rate / 8, in kHz. The
                                     TxClkEsc divider is derived from it. */
    uint32_t Polarity;          /*!< OR of the \ref DSI_Polarity masks. */
    uint32_t Mode;              /*!< \ref DSI_MODE_CMD to send an init table first and switch later
                                     through \ref DSI_StartVideoMode, or \ref DSI_MODE_VIDEO to
                                     start straight in video mode. */
    uint32_t eDPICommandSize;   /*!< EDPI_CMD_SIZE.edpi_allowed_cmd_size: longest command
                                     eDPI will accept, bounded by its line buffer. */
} DSI_ConfigTypeDef;

/**
 * \brief       DSI video mode configuration, in the pixel domain.
 *
 *              Straight off the panel datasheet: \ref DSI_ConfigVideoMode turns
 *              the pixel and line counts into the lane byte clock cycles the
 *              host needs, so nothing here involves lanebyteclk arithmetic.
 *
 *              The low-power blanking enables and the frame BTA acknowledge are
 *              fixed by \ref DSI_ConfigVideoMode; use \ref DSI_ConfigVPG for the
 *              test pattern generator.
 *
 * \ingroup     DSI_Exported_Types
 */
typedef struct
{
    uint32_t Width;             /*!< Horizontal active pixels. */
    uint32_t Height;            /*!< Vertical active lines. */
    uint32_t HSync;             /*!< Horizontal sync width, in pixels. */
    uint32_t HBackPorch;        /*!< Horizontal back porch, in pixels. */
    uint32_t HFrontPorch;       /*!< Horizontal front porch, in pixels. */
    uint32_t VSync;             /*!< Vertical sync width, in lines. */
    uint32_t VBackPorch;        /*!< Vertical back porch, in lines. */
    uint32_t VFrontPorch;       /*!< Vertical front porch, in lines. */

    uint32_t PixelClkKHz;       /*!< DPI pixel clock, in kHz. */
    uint32_t LaneByteClkKHz;    /*!< lanebyteclk, in kHz. */
    uint32_t LaneCount;         /*!< Number of active data lanes, 1 to 4. */
    uint32_t BytesPerPixel;     /*!< Bytes each pixel occupies on the link, usually 3 for RGB888. */

    uint32_t Mode;              /*!< \ref VIDEO_NON_BURST_WITH_SYNC_PULSES,
                                     \ref VIDEO_NON_BURST_WITH_SYNC_EVENTS,
                                     \ref VIDEO_BURST_MODE_2 or \ref VIDEO_BURST_MODE_3. */

    /* Optional, both default to 0, which sends the line as one packet with no
       filler. A null packet pads the link, so a burst-mode line leaves at the
       pixel rate instead of at full link rate: raise NullPacketSize to slow the
       effective pixel rate when the D-PHY runs much faster than the DPI. */
    uint32_t NumberOfChunks;    /*!< Chunks per line; 0 or 1 means a single packet. */
    uint32_t NullPacketSize;    /*!< Payload size of the null packet paired with each packet. */
} DSI_VideoCfgTypeDef;

/**
 * \brief       DSI adapted command mode (eDPI) tearing-effect configuration.
 *
 *              All fields default to 0, which leaves TE off. Pointers this
 *              structure at \ref DSI_ConfigAdaptedCommandMode once a panel needs
 *              the host to track its frame boundary.
 *
 * \ingroup     DSI_Exported_Types
 */
typedef struct
{
    uint32_t TEAcknowledgeRequest;  /*!< 1 enables CMD_MODE_CFG.tear_fx_en and, since the
                                         panel then has to drive the link back, PCKHDL_CFG.bta_en. */
    uint32_t scan_line_parameter;   /*!< EDPI_TE_HW_CFG.scan_line_parameter: the line the TE
                                         fires on when hw_set_scan_line is 1 (16 bits). */
    uint32_t hw_set_scan_line;      /*!< 0 sends set_tear_on, 1 sends set_tear_scanline. */
    uint32_t hw_tear_effect_gen;    /*!< 0 runs the TE protocol right after eDPI data,
                                         1 after eDPI and generic traffic. */
    uint32_t hw_tear_effect_on;     /*!< 1 lets the tear_request pin trigger TE,
                                         0 requires a software command. */
} DSI_AdaptedCmdCfgTypeDef;

/**
 * \brief       DSI D-PHY control structure definition.
 *
 *              Only worth touching on a link that must selectively keep the
 *              PLL alive or the clock lane parked; the ordinary power-up is
 *              \ref DSI_ConfigPhyPins.
 *
 * \ingroup     DSI_Exported_Types
 */
typedef struct
{
    uint32_t ForcePll;       /*!< 1 keeps the D-PHY PLL enabled while the D-PHY is in ULPS. */
    uint32_t EnableClock;    /*!< 1 enables the D-PHY clock lane module. */
    uint32_t ReleaseReset;   /*!< 0 resets the digital section of the D-PHY, 1 releases it. */
    uint32_t PowerUp;        /*!< 0 powers down the complete D-PHY macro, 1 powers it up. */
} DSI_PhyCfgTypeDef;

/**
 * \brief       DSI automatic ULPS control structure definition.
 *
 * \ingroup     DSI_Exported_Types
 */
typedef struct
{
    uint32_t EntryDelay;        /*!< Idle time in lanebyteclk cycles before entering ULPS. */
    uint32_t WakeupClockDiv;    /*!< Twakeup clock divider. Must not be 0. */
    uint32_t WakeupCount;       /*!< Twakeup counter. Twakeup = WakeupClockDiv x WakeupCount. */
    uint32_t MinimumTime;       /*!< Minimum time between ulpsactivenot and ulpsexitreq (12-bit). */
    uint32_t PllOffInUlps;      /*!< 1 turns the D-PHY PLL off during ULPS. */
    uint32_t PrePllOffRequest;  /*!< 1 turns the PLL off before the ULPS entry request. */
} DSI_AutoUlpsCfgTypeDef;

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

/**
 * \brief       De-initialize the DSI host controller.
 *
 *              Clears PWR_UP.shutdownz to put the DSI core into reset /
 *              power-down state, so registers can be safely reconfigured.
 *
 * \param[in]   DSIx : pointer to the DSI peripheral registers.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_restart(void)
 * {
 *     // Put the host into reset before re-programming it.
 *     DSI_DeInit(DSI);
 *
 *     // The whole init sequence (DSI_Init -> mode -> PHY -> DSI_Start) is
 *     // required again afterwards.
 * }
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
void DSI_DeInit(DSI_TypeDef *DSIx);

/**
 * \brief       Initialize the DSI host controller.
 *
 *              Programs lanes, PHY interface, clock manager, DPI and command
 *              mode transmission, then derives the timeouts and PHY timings
 *              from LaneByteClkKHz and PixelClkKHz. Follows the DWC MIPI DSI
 *              Host programming flow (reset -> PHY_IF_CFG -> INT_MSK -> DPI ->
 *              PHY timing).
 *
 * \param[in]   DSIx   : pointer to the DSI peripheral registers.
 * \param[in]   Config : pointer to DSI_ConfigTypeDef that contains the
 *                       panel and D-PHY PLL parameters.
 *
 * \return      DSI_STATUS_OK.
 *
 * \note        Every field is range-checked with assert_param(IS_DSI_*), so a
 *              value out of range trips an assertion rather than returning an
 *              error. Both callers pass compile-time constants; the checks are
 *              there to catch a bad table entry during bring-up.
 *
 * \note        \ref DSI_ConfigPhyPins must run after this to release the PHY.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_init(void)
 * {
 *     static const DSI_ConfigTypeDef cfg =
 *     {
 *         .LaneCount       = 1,
 *         .ColorCoding     = DSI_COLOR_RGB888,
 *         .LaneByteClkKHz  = 50000,   // 400Mbps / 8
 *         .Polarity        = 0,       // every DPI pin active high
 *         .Mode            = DSI_MODE_CMD,
 *         .eDPICommandSize = DSI_EDPI_DEFAULT_CMD_SIZE,
 *     };
 *
 *     DSI_Init(DSI, &cfg);
 *
 *     // Every packet type goes out in low-power by default; narrow it here.
 *     DSI_ConfigLPM(DSI, DSI_LP_CMD_ALL & ~DSI_LP_DCS_LONG_WRITE);
 * }
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
FlagStatus DSI_Init(DSI_TypeDef *DSIx, const DSI_ConfigTypeDef *Config);

/**
 * \brief       Select which packet types go out in low-power.
 *
 *              \ref DSI_Init enables every type in \ref DSI_LP_CMD_ALL; call
 *              this afterwards to keep a single transaction in high-speed.
 *
 * \param[in]   DSIx : pointer to the DSI peripheral registers.
 * \param[in]   Mask : OR of the \ref DSI_LP masks.
 *
 * \return      DSI_STATUS_OK on success, DSI_STATUS_ERROR if a bit outside the
 *              defined masks is set.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * // Send this one command in high-speed while the rest stay in low-power.
 * DSI_ConfigLPM(DSI, DSI_LP_CMD_ALL & ~DSI_LP_DCS_LONG_WRITE);
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
FlagStatus DSI_ConfigLPM(DSI_TypeDef *DSIx, uint32_t Mask);

/**
 * \brief       Configure the DSI to operate in Video mode.
 *
 *              Converts the panel pixel timing into lane byte clock cycles,
 *              works out the largest LP command each blanking window can carry,
 *              then programs the transmission mode (burst / non-burst), the
 *              low-power blanking areas, chunking, the null packet size and both
 *              the horizontal and vertical timings.
 *
 * \param[in]   DSIx   : pointer to the DSI peripheral registers.
 * \param[in]   VidCfg : pointer to DSI_VideoCfgTypeDef, in pixels and lines.
 *
 * \return      DSI_STATUS_OK on success, DSI_STATUS_ERROR on invalid arguments.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_video_mode_init(void)
 * {
 *     const DSI_VideoCfgTypeDef VidCfg =
 *     {
 *         .Width = 454, .Height = 454,
 *         .HSync = 2, .HBackPorch = 10, .HFrontPorch = 10,
 *         .VSync = 2, .VBackPorch = 8, .VFrontPorch = 8,
 *         .PixelClkKHz    = 10000,
 *         .LaneByteClkKHz = 50000,
 *         .LaneCount      = 1,
 *         .BytesPerPixel  = 3,
 *         .Mode           = VIDEO_BURST_MODE_2,
 *         // .NullPacketSize = 0x1e00,  // pad the link to slow the pixel rate
 *     };
 *
 *     DSI_ConfigVideoMode(DSI, &VidCfg);
 * }
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
FlagStatus DSI_ConfigVideoMode(DSI_TypeDef *DSIx, const DSI_VideoCfgTypeDef *VidCfg);

/**
 * \brief       Start the DSI in Video mode.
 *
 * \param[in]   DSIx           : pointer to the DSI peripheral registers.
 *
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_video_mode_start(void)
 * {
 *     // After Config screen in Adapted Command mode, start the DSI in Video mode.
 *     DSI_StartVideoMode(DSI);
 * }
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
void DSI_StartVideoMode(DSI_TypeDef *DSIx);

/**
 * \brief       Configure the adapted command mode tearing-effect handling.
 *
 *              Programmes CMD_MODE_CFG.tear_fx_en and the EDPI_TE_HW_CFG
 *              fields. When TEAcknowledgeRequest is set, PCKHDL_CFG.bta_en is
 *              asserted as well, because the panel has to turn the link around
 *              to send the acknowledge back.
 *
 * \param[in]   DSIx          : pointer to the DSI peripheral registers.
 * \param[in]   AdaptedCmdCfg : pointer to DSI_AdaptedCmdCfgTypeDef. Pass a
 *                              zeroed structure to leave TE off.
 *
 * \return      DSI_STATUS_OK on success, DSI_STATUS_ERROR on invalid arguments.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_cmd_mode_init(void)
 * {
 *     DSI_AdaptedCmdCfgTypeDef AdaptedCmdCfg = {0};   // TE off
 *
 *     // Or, once the panel is wired for it:
 *     AdaptedCmdCfg.TEAcknowledgeRequest = DSI_TE_ACKNOWLEDGE_ENABLE;
 *
 *     // Optional: let the host generate the TE pulse itself.
 *     AdaptedCmdCfg.hw_tear_effect_on    = 1;
 *     AdaptedCmdCfg.hw_tear_effect_gen   = 1;
 *     AdaptedCmdCfg.hw_set_scan_line     = 1;
 *     AdaptedCmdCfg.scan_line_parameter  = 0;
 *
 *     DSI_ConfigAdaptedCommandMode(DSI, &AdaptedCmdCfg);
 * }
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
FlagStatus DSI_ConfigAdaptedCommandMode(DSI_TypeDef *DSIx,
                                        const DSI_AdaptedCmdCfgTypeDef *AdaptedCmdCfg);

/**
 * \brief       Enable or disable the video pattern generator.
 *
 *              With VPG on the host transmits its own test pattern instead of
 *              the eDPI pixels, which is how the link can be brought up without
 *              a panel attached.
 *
 * \param[in]   DSIx         : pointer to the DSI peripheral registers.
 * \param[in]   Enable       : 1 to send the internal pattern, 0 for real pixels.
 * \param[in]   Mode         : \ref VIDEO_VPG_MODE_V_AND_H or \ref VIDEO_VPG_MODE_ONLY_V.
 * \param[in]   Orientation  : \ref VIDEO_VPG_ORIENTATION_V or \ref VIDEO_VPG_ORIENTATION_H.
 *
 * \return      DSI_STATUS_OK on success, DSI_STATUS_ERROR on invalid arguments.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * // Send a horizontal-bar test pattern instead of panel data.
 * DSI_ConfigVPG(DSI, 1, VIDEO_VPG_MODE_V_AND_H, VIDEO_VPG_ORIENTATION_H);
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
FlagStatus DSI_ConfigVPG(DSI_TypeDef *DSIx, uint32_t Enable,
                         uint32_t Mode, uint32_t Orientation);

/**
 * \brief       Power up the D-PHY through the host interface.
 *
 *              Does the D-PHY power-up sequence the databook asks for: drive
 *              PHY_RSTZ low, pulse the PHY test interface clear, release reset
 *              and shutdown, then wait until all active lanes (read back from
 *              PHY_IF_CFG.n_lanes) reach Stop (LP-11) state.
 *
 *              Precondition: the RTL D-PHY PLL must already be configured with
 *              DPHY_Init(), and \ref DSI_Init must have run.
 *
 * \param[in]   DSIx : pointer to the DSI peripheral registers.
 *
 * \return      DSI_STATUS_OK once every active lane is in Stop state,
 *              DSI_STATUS_ERROR on timeout (PHY_STATUS shows which lane stuck).
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_phy_init(void)
 * {
 *     // The RTL D-PHY PLL must be configured first.
 *     DPHY_Init(1, 40, 400);   // 1 lane, 40MHz CKREF, 400MHz HS bit clock
 *
 *     // Returns only once all active lanes are in Stop (LP-11) state.
 *     if (DSI_ConfigPhyPins(DSI) != DSI_STATUS_OK)
 *     {
 *         return;
 *     }
 * }
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
FlagStatus DSI_ConfigPhyPins(DSI_TypeDef *DSIx);

/**
 * \brief       Configure and enable the Auto-ULPS control scheme.
 *
 *              Programs entry delay, wake-up time (divider and count), minimum
 *              ULPS time and PLL-off behavior, then enables the automatic ULPS
 *              mechanism. Only meaningful when eDPI is used in Adapted Command
 *              mode.
 *
 * \param[in]   DSIx        : pointer to the DSI peripheral registers.
 * \param[in]   AutoUlpsCfg : pointer to DSI_AutoUlpsCfgTypeDef structure that
 *                            contains the Auto-ULPS configuration parameters.
 *
 * \return      DSI_STATUS_OK on success, DSI_STATUS_ERROR on invalid arguments.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_auto_ulps_init(void)
 * {
 *     DSI_AutoUlpsCfgTypeDef AutoUlpsCfg = {0};
 *
 *     AutoUlpsCfg.EntryDelay     = 0x1000;
 *     AutoUlpsCfg.WakeupClockDiv = 0x10;    // must not be 0
 *     AutoUlpsCfg.WakeupCount    = 0x20;    // must not be 0
 *     AutoUlpsCfg.MinimumTime    = 0x40;    // 12-bit, max 0x0FFF
 *     AutoUlpsCfg.PllOffInUlps   = 0;
 *     AutoUlpsCfg.PrePllOffRequest = 0;
 *
 *     if (DSI_ConfigAutoUlps(DSI, &AutoUlpsCfg) != DSI_STATUS_OK)
 *     {
 *         // wake-up divider/count left at 0, or MinimumTime out of range
 *         return;
 *     }
 * }
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
FlagStatus DSI_ConfigAutoUlps(DSI_TypeDef *DSIx,
                              const DSI_AutoUlpsCfgTypeDef *AutoUlpsCfg);

/**
 * \brief       Disable the Auto-ULPS control scheme.
 *
 *              Clears AUTO_ULPS_MODE.auto_ulps to turn off the automatic ULPS
 *              mechanism.
 *
 * \param[in]   DSIx : pointer to the DSI peripheral registers.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_suspend(void)
 * {
 *     // Stop entering ULPS automatically before powering the link down.
 *     DSI_DisableAutoUlps(DSI);
 * }
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
void DSI_DisableAutoUlps(DSI_TypeDef *DSIx);

/**
 * \brief       Send a DCS/Generic short packet (0, 1 or 2 parameters).
 *
 *              Waits for the generic command FIFO to have space, then writes
 *              the packet header to GEN_HDR. For 0-parameter commands Param2
 *              is ignored (forced to 0).
 *
 * \param[in]   DSIx      : pointer to the DSI peripheral registers.
 * \param[in]   ChannelID : virtual channel ID of the packet (2 bits).
 * \param[in]   Mode      : DSI data type (e.g. DSI_DCS_SHORT_PKT_WRITE_P0/P1).
 * \param[in]   Param1    : first parameter byte.
 * \param[in]   Param2    : second parameter byte.
 *
 * \return      DSI_STATUS_OK on success, DSI_STATUS_ERROR on invalid arguments
 *              or command FIFO full timeout.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_send_cmds(void)
 * {
 *     // DCS command 0x11 (Sleep Out), no parameter.
 *     DSI_ShortWrite(DSI, 0, DSI_DCS_SHORT_PKT_WRITE_P0, 0x11, 0);
 *
 *     // DCS command 0x36 (Address Set), one parameter: MADCTL = 0x00.
 *     DSI_ShortWrite(DSI, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0x36, 0x00);
 *
 *     // Generic short write with two parameters.
 *     DSI_ShortWrite(DSI, 0, DSI_GEN_SHORT_PKT_WRITE_P2, 0x01, 0x02);
 * }
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
FlagStatus DSI_ShortWrite(DSI_TypeDef *DSIx,
                          uint32_t ChannelID,
                          uint32_t Mode,
                          uint32_t Param1,
                          uint32_t Param2);

/**
 * \brief       Send a DCS/Generic long packet.
 *
 *              Writes the payload into GEN_PLD_DATA (waiting for the payload
 *              FIFO to have space before each 32-bit word), then writes the
 *              packet header to GEN_HDR which triggers transmission.
 *
 * \param[in]   DSIx            : pointer to the DSI peripheral registers.
 * \param[in]   ChannelID       : virtual channel ID of the packet (2 bits).
 * \param[in]   Mode            : DSI data type (e.g. DSI_DCS_LONG_PKT_WRITE).
 * \param[in]   NbParams        : number of payload bytes (<= 199).
 * \param[in]   Param1          : DCS command code (first payload byte).
 * \param[in]   ParametersTable : pointer to remaining payload bytes.
 *
 * \return      DSI_STATUS_OK on success, DSI_STATUS_ERROR on invalid arguments
 *              or payload/command FIFO full timeout.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_set_column_address(void)
 * {
 *     // DCS 0x2A with 4 parameter bytes: x_start = 0x0000, x_end = 0x01C5.
 *     uint8_t params[4] = {0x00, 0x00, 0x01, 0xC5};
 *
 *     if (DSI_LongWrite(DSI, 0, DSI_DCS_LONG_PKT_WRITE, sizeof(params), 0x2A, params)
 *         != DSI_STATUS_OK)
 *     {
 *         // payload/command FIFO never drained
 *         return;
 *     }
 * }
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
FlagStatus DSI_LongWrite(DSI_TypeDef *DSIx,
                         uint32_t ChannelID,
                         uint32_t Mode,
                         uint32_t NbParams,
                         uint32_t Param1,
                         uint8_t *ParametersTable);

/**
 * \brief       Send a DCS/Generic read and collect the payload.
 *
 *              Waits for any earlier read to finish, issues the read command,
 *              then hands back the first NbBytes of the payload.
 *
 *              Only valid in command mode, and the panel must be able to drive
 *              the link back. \ref DSI_Init leaves every packet type in
 *              low-power but keeps PCKHDL_CFG.bta_en clear, so a caller that
 *              reads must first enable the matching \ref DSI_LP read mask
 *              through \ref DSI_ConfigLPM and set \ref DSI_FLAG_BTA in
 *              PCKHDL_CFG.
 *
 * \param[in]   DSIx      : pointer to the DSI peripheral registers.
 * \param[in]   ChannelID : virtual channel ID of the packet (2 bits).
 * \param[in]   Mode      : DSI read data type (e.g. DSI_DCS_SHORT_PKT_READ).
 * \param[in]   Param1    : first parameter byte (the DCS command code).
 * \param[in]   Param2    : second parameter byte, ignored for 0-parameter reads.
 * \param[out]  pBuffer   : destination for the payload; may be NULL to discard.
 * \param[in]   NbBytes   : how many payload bytes to collect.
 *
 * \return      DSI_STATUS_OK on success, DSI_STATUS_ERROR on invalid arguments,
 *              a command FIFO that never drained, or a read that timed out.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_read_id(void)
 * {
 *     uint8_t id[3] = {0};
 *
 *     // DCS 0xDA, one parameter (0x00), three payload bytes expected.
 *     if (DSI_Read(DSI, 0, DSI_DCS_SHORT_PKT_READ, 0xDA, 0x00, id, sizeof(id))
 *         != DSI_STATUS_OK)
 *     {
 *         return;
 *     }
 * }
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
FlagStatus DSI_Read(DSI_TypeDef *DSIx,
                    uint32_t ChannelID,
                    uint32_t Mode,
                    uint32_t Param1,
                    uint32_t Param2,
                    uint8_t *pBuffer,
                    uint32_t NbBytes);

/**
 * \brief       Check whether every queued command packet has been sent.
 *
 *              The command and payload FIFOs being empty is the only reliable
 *              sign that a write really went out on the link, so this is what
 *              to wait on before reconfiguring or powering the host down.
 *
 * \param[in]   DSIx : pointer to the DSI peripheral registers.
 *
 * \return      DSI_STATUS_OK when the command FIFO is empty and idle,
 *              DSI_STATUS_ERROR otherwise.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_before_mode_switch(void)
 * {
 *     // Let the last panel command reach the panel before switching modes.
 *     while (DSI_IsCmdFifoEmpty(DSI) != DSI_STATUS_OK)
 *     {
 *     }
 * }
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
FlagStatus DSI_IsCmdFifoEmpty(DSI_TypeDef *DSIx);

/**
 * \brief       Power-up / start the DSI host controller.
 *
 *              Sets PWR_UP.shutdownz to 1 to wake the core after all
 *              configurations (init, mode, PHY) are applied.
 *
 * \param[in]   DSIx : pointer to the DSI peripheral registers.
 *
 * \return      DSI_STATUS_OK on success, DSI_STATUS_ERROR on invalid arguments.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_power_on(void)
 * {
 *     // Init, mode and PHY configuration must all be applied before this.
 *     DSI_Start(DSI);
 *
 *     // The panel init sequence can be sent as soon as the link is up.
 *     DSI_ShortWrite(DSI, 0, DSI_DCS_SHORT_PKT_WRITE_P0, 0x11, 0);
 * }
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
FlagStatus DSI_Start(DSI_TypeDef *DSIx);
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
