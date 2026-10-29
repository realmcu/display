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
#define PHY_FORCEPLL_ENABLE     1
#define PHY_FORCEPLL_DISABLE    0
#define PHY_CLK_ENABLE     1
#define PHY_CLK_DISABLE    0
#define PHY_RST_ENABLE     1
#define PHY_RST_DISABLE    0
#define PHY_SHUTDOWN_ENABLE     1
#define PHY_SHUTDOWN_DISABLE    0

#define DSI_STATUS_ERROR        ((FlagStatus)0)
#define DSI_STATUS_OK           ((FlagStatus)1)
#ifndef DSI_POLL_TIMEOUT
#define DSI_POLL_TIMEOUT        0x100000U
#endif

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
     *              Used by \ref DSI_Init. Call \ref DSI_StructInit first to load the
     *              default values, then overwrite the fields that depend on the
     *              panel and the D-PHY PLL.
     *
     * \ingroup     DSI_Exported_Types
     */
    typedef struct
{
    uint32_t phy_mode;                  /*!< Selects a D-PHY (\ref DSI_DPHY) or a C-PHY (\ref DSI_CPHY) core. */
    uint32_t ColorCoding;               /*!< Pixel color coding, \ref DSI_COLOR_RGB565 or \ref DSI_COLOR_RGB888. */
    uint32_t NumberOfLanes;             /*!< Active data lanes, 1 to 4. The driver writes NumberOfLanes - 1 to the register. */
    uint32_t StopWaitTime;              /*!< Minimum time the PHY stays in StopState before requesting an HS transfer. */
    uint32_t InterruptMask0;            /*!< Mask of the INT_ST0 error sources. A 1 un-masks (enables) the source, a 0 masks it. */
    uint32_t InterruptMask1;            /*!< Mask of the INT_ST1 error sources, same polarity as INT_MSK0. */
    uint32_t HS_TX_CNT;                 /*!< High-speed transmit timeout, in to_clk_division cycles. */
    uint32_t LP_RX_CNT;                 /*!< Low-power receive timeout, in to_clk_division cycles. */
    uint32_t TOCkdiv;                   /*!< Divides lanebyteclk to produce the time-out clock used by the counters. */
    uint32_t TXEscapeCkdiv;             /*!< Divides lanebyteclk for the TX escape clock. 0 and 1 stop the clock. */
    uint32_t ModeCfg;                   /*!< \ref DSI_MODE_VIDEO or \ref DSI_MODE_CMD, the operating mode of the host. */

    uint32_t PeripherCharacteristics;   /*!< eotp_tx_lp_en, crc_rx_en, ecc_rx_en, bta_en, eotp_rx_en and eotp_tx_en. */
    uint32_t GenericPacketVCID;         /*!< Virtual channel of generic packets generated and sent by hardware. */
    uint32_t GenericTearVCID;           /*!< Virtual channel used for the hardware generated tearing effect. */
    uint32_t GenericReadBackVCID;       /*!< Virtual channel expected on generic read-back packets. */

    uint32_t AutoHS2LPEnable;           /*!< Lets the clock lane stop automatically when time allows. */
    uint32_t TxRequestCLKEnable;        /*!< Drives the D-PHY PPI txrequestclkhs signal from the controller. */

    uint32_t ClockLaneHS2LPTime;        /*!< Max clock-lane HS to LP time, in lanebyteclk cycles. */
    uint32_t ClockLaneLP2HSTime;        /*!< Max clock-lane LP to HS time, in lanebyteclk cycles. */
    uint32_t DataLaneHS2LPTime;         /*!< Max data-lane HS to LP time, in lanebyteclk cycles. */
    uint32_t DataLaneLP2HSTime;         /*!< Max data-lane LP to HS time, in lanebyteclk cycles. */
    uint32_t DataLaneMaxReadTime;       /*!< Max time allowed for a read command, in lanebyteclk cycles. */
    uint32_t VirtualChannelID;          /*!< Virtual channel indexed into the Video mode and eDPI packets. */


    uint8_t phy_shutdownz;              /*!< 0 puts the complete D-PHY macro in power-down, 1 powers it up. */
    uint8_t phy_enableclk;              /*!< 1 enables the D-PHY clock lane module. */
    uint8_t phy_forcepll;               /*!< 1 keeps the D-PHY PLL enabled while the D-PHY is in ULPS. */
    uint8_t phy_rstz;                   /*!< 0 resets the digital section of the D-PHY, 1 releases it. */

    uint32_t ColorModePolarity;         /*!< 1 makes the dpicolorm pin active low, 0 active high. */
    uint32_t ShutDownPolarity;          /*!< 1 makes the dpishutdn pin active low, 0 active high. */
    uint32_t HSPolarity;                /*!< \ref DSI_HSPOL_AL or \ref DSI_HSPOL_AH for the dpihsync pin. */
    uint32_t VSPolarity;                /*!< \ref DSI_VSPOL_AL or \ref DSI_VSPOL_AH for the dpivsync pin. */
    uint32_t DEPolarity;                /*!< \ref DSI_DEPOL_AL or \ref DSI_DEPOL_AH for the dpidataen pin. */
    uint32_t CommandSize;               /*!< Pixels the eDPI may send before a WMS command, so both FIFOs fill evenly. */

    uint32_t LPMaxReadPacket;           /*!< Transmission type (HS or LP) of the maximum-size read packet command. */
    uint32_t LPGenReadTime;             /*!< PRESP_TO counter for a generic read in LP. 0 disables the timeout. */
    uint32_t HSGenReadTime;             /*!< PRESP_TO counter for a generic read in HS. 0 disables the timeout. */
    uint32_t LPGenWriteTime;            /*!< PRESP_TO counter for a generic write in LP. 0 disables the timeout. */
    uint32_t HSGenWriteTime;            /*!< PRESP_TO counter for a generic write in HS. 0 disables the timeout. */
    uint32_t BTATime;                   /*!< PRESP_TO counter for a Bus Turn-Around. 0 disables the timeout. */
    uint32_t AcknowledgeRequest;        /*!< 1 requests an acknowledge after every packet transmission. */

    uint32_t LPGenShortWriteNoP;        /*!< 1 sends a generic short write, no parameter, in low-power. */
    uint32_t LPGenShortWriteOneP;       /*!< 1 sends a generic short write, one parameter, in low-power. */
    uint32_t LPGenShortWriteTwoP;       /*!< 1 sends a generic short write, two parameters, in low-power. */
    uint32_t LPGenShortReadNoP;         /*!< 1 sends a generic short read, no parameter, in low-power. */
    uint32_t LPGenShortReadOneP;        /*!< 1 sends a generic short read, one parameter, in low-power. */
    uint32_t LPGenShortReadTwoP;        /*!< 1 sends a generic short read, two parameters, in low-power. */
    uint32_t LPGenLongWrite;            /*!< 1 sends a generic long write in low-power. */
    uint32_t LPDcsShortWriteNoP;        /*!< 1 sends a DCS short write, no parameter, in low-power. */
    uint32_t LPDcsShortWriteOneP;       /*!< 1 sends a DCS short write, one parameter, in low-power. */
    uint32_t LPDcsShortReadNoP;         /*!< 1 sends a DCS short read, no parameter, in low-power. */
    uint32_t LPDcsLongWrite;            /*!< 1 sends a DCS long write in low-power. */

} DSI_InitTypeDef;

/**
 * \brief       DSI video mode configuration structure definition.
 *
 * \ingroup     DSI_Exported_Types
 */
typedef struct
{
    uint32_t Mode;                         /*!< \ref VIDEO_NON_BURST_WITH_SYNC_PULSES, \ref VIDEO_NON_BURST_WITH_SYNC_EVENTS, */
    /*!<   \ref VIDEO_BURST_MODE_2 or \ref VIDEO_BURST_MODE_3. */
    uint32_t LPCommandEnable;              /*!< 1 sends non-video packets in low-power during the blanking periods. */
    uint32_t FrameBTAAcknowledgeEnable;    /*!< 1 requests an acknowledge at the end of every frame. */
    uint32_t LPHorizontalFrontPorchEnable; /*!< 1 allows a return to low-power inside the HFP period. */
    uint32_t LPHorizontalBackPorchEnable;  /*!< 1 allows a return to low-power inside the HBP period. */
    uint32_t LPVerticalActiveEnable;       /*!< 1 allows a return to low-power inside the VACT period. */
    uint32_t LPVerticalFrontPorchEnable;   /*!< 1 allows a return to low-power inside the VFP period. */
    uint32_t LPVerticalBackPorchEnable;    /*!< 1 allows a return to low-power inside the VBP period. */
    uint32_t LPVerticalSyncActiveEnable;   /*!< 1 allows a return to low-power inside the VSA period. */

    uint32_t invact_lpcmd_size;            /*!< Bytes available for a low-power command inside VACT. */
    uint32_t outvact_lpcmd_size;           /*!< Bytes available for a low-power command outside VACT. */

    uint32_t PacketSize;                   /*!< Pixels per video packet; equals the panel width for one packet per line. */
    uint32_t NumberOfChunks;               /*!< Chunks per line, 0 or 1 sends the line as a single packet. */
    uint32_t NullPacketSize;               /*!< Payload size of the null packet paired with each video packet. */

    uint32_t HorizontalSyncActive;         /*!< Horizontal synchronism active duration (in lane byte clock cycles) */
    uint32_t HorizontalBackPorch;          /*!< Horizontal back-porch duration (in lane byte clock cycles)        */
    uint32_t HorizontalLine;               /*!< Horizontal line duration (in lane byte clock cycles)              */
    uint32_t VerticalSyncActive;           /*!< Vertical synchronism active duration, in lines                           */
    uint32_t VerticalBackPorch;            /*!< Vertical back-porch duration, in lines                                   */
    uint32_t VerticalFrontPorch;           /*!< Vertical front-porch duration, in lines                                  */
    uint32_t VerticalActive;               /*!< Vertical active duration, in lines                              */

    uint32_t VPGOrientation;               /*!< \ref VIDEO_VPG_ORIENTATION_V or \ref VIDEO_VPG_ORIENTATION_H. */
    uint32_t VPGMode;                      /*!< \ref VIDEO_VPG_MODE_V_AND_H or \ref VIDEO_VPG_MODE_ONLY_V. */
    uint32_t VPGEnable;                    /*!< 1 makes the host send its internal test pattern instead of eDPI pixels. */

} DSI_VidCfgTypeDef;

/**
 * \brief       DSI adapted command mode (eDPI) configuration structure definition.
 *
 * \ingroup     DSI_Exported_Types
 */
typedef struct
{
    uint32_t TEAcknowledgeRequest;  /*!< 1 enables the tearing effect acknowledge request. */
    uint32_t scan_line_parameter;   /*!< TE output line mode parameter; byte 1 in [31:24], byte 2 in [23:16]. */
    uint32_t hw_set_scan_line;      /*!< 0 sends set_tear_on, 1 sends set_tear_scanline on a TE request. */
    uint32_t hw_tear_effect_gen;    /*!< 0 runs the TE protocol right after eDPI data, 1 after eDPI and generic traffic. */
    uint32_t hw_tear_effect_on;     /*!< 1 lets the tear_request pin trigger TE, 0 requires a software command. */

} DSI_AdaptedCmdCfgTypeDef;

/**
 * \brief       DSI D-PHY control structure definition.
 *
 * \ingroup     DSI_Exported_Types
 */
typedef struct
{
    uint32_t phy_forcepll;   /*!< 1 keeps the D-PHY PLL enabled while the D-PHY is in ULPS. */
    uint32_t phy_enableclk;  /*!< 1 enables the D-PHY clock lane module. */
    uint32_t phy_rstz;       /*!< 0 resets the digital section of the D-PHY, 1 releases it. */
    uint32_t phy_shutdownz;  /*!< 0 powers down the complete D-PHY macro, 1 powers it up. */
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
 * \brief       Fills each DSI_InitTypeDef member with its default value.
 *
 *              Gives panel-independent defaults so the caller only has to
 *              overwrite the fields that depend on the panel and the D-PHY PLL.
 *
 * \param[in]   DSI_Init : pointer to a DSI_InitTypeDef structure which will be
 *                         initialized.
 *
 * \note The default settings for the DSI_InitTypeDef members are shown in the
 *       following table. The register and bit names refer to the DWC MIPI DSI
 *       Host Controller databook (v1.51a).
 *       | DSI_InitTypeDef Member  | Default Value                | Description                                                                     |
 *       |:------------------------|:-----------------------------|:--------------------------------------------------------------------------------|
 *       | phy_mode                | \ref DSI_DPHY                | Selects a D-PHY (DSI_DPHY) or a C-PHY (DSI_CPHY) core.                          |
 *       | ColorCoding             | \ref DSI_COLOR_RGB888        | Pixel color coding for Video mode and eDPI packets.                             |
 *       | NumberOfLanes           | \ref DSI_1_LANE              | Active data lanes. The driver writes NumberOfLanes - 1, so 1 means lane 0 only. |
 *       | StopWaitTime            | 0                            | Minimum time the PHY stays in StopState before requesting an HS TX.             |
 *       | InterruptMask0          | 0x1FFFFF                     | INT_ST0 error source mask. A 1 un-masks (enables) the source, a 0 masks it.     |
 *       | InterruptMask1          | 0x1FFFFF                     | INT_ST1 error source mask, same polarity as INT_MSK0.                           |
 *       | HS_TX_CNT               | 0xFFFF                       | HS transmit timeout, in to_clk_division cycles (16-bit, max defers the reset).  |
 *       | LP_RX_CNT               | 0xFFFF                       | LP receive timeout, in to_clk_division cycles (16-bit).                         |
 *       | TOCkdiv                 | 0                            | Divides lanebyteclk to produce the time-out clock used by the counters.         |
 *       | TXEscapeCkdiv           | 0                            | Divides lanebyteclk for the TX escape clock (0 and 1 stop the clock).           |
 *       | ModeCfg                 | \ref DSI_MODE_CMD            | 0 = video mode, 1 = command mode.                                               |
 *       | PeripherCharacteristics | 0                            | EoTp TX/RX, BTA, ECC RX and CRC RX packet handling enables.                     |
 *       | GenericPacketVCID       | 0                            | Virtual channel of generic packets generated and transmitted by hardware.       |
 *       | GenericTearVCID         | 0                            | Virtual channel used for the hardware generated tearing effect.                 |
 *       | GenericReadBackVCID     | 0                            | Virtual channel expected on generic read-back packets.                          |
 *       | AutoHS2LPEnable         | ENABLE                       | Lets the controller stop the clock lane automatically when time allows.         |
 *       | TxRequestCLKEnable      | ENABLE                       | Drives the D-PHY PPI txrequestclkhs signal from the controller.                 |
 *       | ClockLaneHS2LPTime      | 0                            | Max clock-lane HS to LP time, in lanebyteclk cycles.                            |
 *       | ClockLaneLP2HSTime      | 0                            | Max clock-lane LP to HS time, in lanebyteclk cycles.                            |
 *       | DataLaneHS2LPTime       | 0                            | Max data-lane HS to LP time, in lanebyteclk cycles.                             |
 *       | DataLaneLP2HSTime       | 0                            | Max data-lane LP to HS time, in lanebyteclk cycles.                             |
 *       | DataLaneMaxReadTime     | 0                            | Max time allowed for a read command, in lanebyteclk cycles.                     |
 *       | VirtualChannelID        | 0                            | Virtual channel indexed into the Video mode / eDPI packets.                     |
 *       | phy_shutdownz           | 1                            | 0 puts the complete D-PHY macro in power-down, 1 powers it up.                  |
 *       | phy_enableclk           | 1                            | 1 enables the D-PHY clock lane module.                                          |
 *       | phy_forcepll            | 0                            | 1 keeps the D-PHY PLL enabled while the D-PHY is in ULPS.                       |
 *       | phy_rstz                | 1                            | 0 resets the digital section of the D-PHY, 1 releases it.                       |
 *       | ColorModePolarity       | 0                            | 1 makes the dpicolorm pin active low (0 = active high).                         |
 *       | ShutDownPolarity        | 0                            | 1 makes the dpishutdn pin active low (0 = active high).                         |
 *       | HSPolarity              | \ref DSI_HSPOL_AH            | 1 makes the dpihsync pin active low (0 = active high).                          |
 *       | VSPolarity              | \ref DSI_VSPOL_AH            | 1 makes the dpivsync pin active low (0 = active high).                          |
 *       | DEPolarity              | \ref DSI_DEPOL_AH            | 1 makes the dpidataen pin active low (0 = active high).                         |
 *       | CommandSize             | 0                            | Pixels the eDPI may send before a WMS command, so both FIFOs fill evenly.       |
 *       | LPMaxReadPacket         | \ref DSI_LP_MRDP_DISABLE     | Transmission type (HS/LP) of the maximum-size read packet command.              |
 *       | LPGenReadTime           | 0                            | PRESP_TO counter for generic read in LP, 0 disables it.                         |
 *       | HSGenReadTime           | 0                            | PRESP_TO counter for generic read in HS, 0 disables it.                         |
 *       | LPGenWriteTime          | 0                            | PRESP_TO counter for generic write in LP, 0 disables it.                        |
 *       | HSGenWriteTime          | 0                            | PRESP_TO counter for generic write in HS, 0 disables it.                        |
 *       | BTATime                 | 0                            | PRESP_TO counter for Bus Turn-Around, 0 disables it.                            |
 *       | AcknowledgeRequest      | \ref DSI_ACKNOWLEDGE_DISABLE | 1 requests an acknowledge after every packet transmission.                      |
 *       | LPGenShortWriteNoP      | \ref DSI_LP_GSW0P_ENABLE     | 1 sends generic short write, no parameter, in low-power.                        |
 *       | LPGenShortWriteOneP     | \ref DSI_LP_GSW1P_ENABLE     | 1 sends generic short write, one parameter, in low-power.                       |
 *       | LPGenShortWriteTwoP     | \ref DSI_LP_GSW2P_ENABLE     | 1 sends generic short write, two parameters, in low-power.                      |
 *       | LPGenShortReadNoP       | \ref DSI_LP_GSR0P_ENABLE     | 1 sends generic short read, no parameter, in low-power.                         |
 *       | LPGenShortReadOneP      | \ref DSI_LP_GSR1P_ENABLE     | 1 sends generic short read, one parameter, in low-power.                        |
 *       | LPGenShortReadTwoP      | \ref DSI_LP_GSR2P_ENABLE     | 1 sends generic short read, two parameters, in low-power.                       |
 *       | LPGenLongWrite          | \ref DSI_LP_GLW_ENABLE       | 1 sends generic long write in low-power.                                        |
 *       | LPDcsShortWriteNoP      | \ref DSI_LP_DSW0P_ENABLE     | 1 sends DCS short write, no parameter, in low-power.                            |
 *       | LPDcsShortWriteOneP     | \ref DSI_LP_DSW1P_ENABLE     | 1 sends DCS short write, one parameter, in low-power.                           |
 *       | LPDcsShortReadNoP       | \ref DSI_LP_DSR0P_ENABLE     | 1 sends DCS short read, no parameter, in low-power.                             |
 *       | LPDcsLongWrite          | \ref DSI_LP_DLW_ENABLE       | 1 sends DCS long write in low-power.                                            |
 *
 * \note TOCkdiv, TXEscapeCkdiv, CommandSize and the PHY timing fields are left
 *       at 0 because they depend on the lane byte clock and the D-PHY PLL of the
 *       actual panel; the caller must set them before \ref DSI_Init.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_init(void)
 * {
 *     DSI_InitTypeDef DSI_InitStruct = {0};
 *
 *     // Start from the default values.
 *     DSI_StructInit(&DSI_InitStruct);
 *
 *     // Overwrite the panel / PLL dependent fields.
 *     DSI_InitStruct.NumberOfLanes      = 1;
 *     DSI_InitStruct.TOCkdiv            = 0x14;
 *     DSI_InitStruct.TXEscapeCkdiv      = 0x03;
 *     DSI_InitStruct.CommandSize        = 598;
 *     DSI_InitStruct.ClockLaneHS2LPTime = 0x24;
 *     DSI_InitStruct.ClockLaneLP2HSTime = 0x3D;
 *     DSI_InitStruct.DataLaneHS2LPTime  = 0x26;
 *     DSI_InitStruct.DataLaneLP2HSTime  = 0xA1;
 *
 *     DSI_Init(DSI, &DSI_InitStruct);
 * }
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
void DSI_StructInit(DSI_InitTypeDef *DSI_Init);

/**
 * \brief       Initialize/clk-config the DSI host controller.
 *
 *              Programs lanes, PHY interface, clock manager, DPI/eDPI, command
 *              mode transmission and PHY timing registers, following the
 *              DWC MIPI DSI Host programming flow (reset -> PHY_IF_CFG ->
 *              INT_MSK -> DPI/DBI -> PHY timing).
 *
 * \param[in]   DSIx     : pointer to the DSI peripheral registers.
 * \param[in]   DSI_Init : pointer to DSI_InitTypeDef structure that contains
 *                         all the initialization parameters.
 *
 * \return      DSI_STATUS_OK on success, DSI_STATUS_ERROR on invalid arguments.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_init(void)
 * {
 *     DSI_InitTypeDef DSI_InitStruct = {0};
 *
 *     DSI_InitStruct.VirtualChannelID   = 0;
 *     DSI_InitStruct.phy_mode           = 0;            // D-PHY
 *     DSI_InitStruct.NumberOfLanes      = 1;
 *     DSI_InitStruct.ColorCoding        = 5;            // RGB888
 *     DSI_InitStruct.ModeCfg            = DSI_MODE_CMD; // start in command mode
 *     DSI_InitStruct.TOCkdiv            = 0x14;
 *     DSI_InitStruct.TXEscapeCkdiv      = 0x03;         // TxClkEsc must be 2-20MHz
 *     DSI_InitStruct.CommandSize        = 598;
 *
 *     DSI_InitStruct.AutoHS2LPEnable    = ENABLE;
 *     DSI_InitStruct.TxRequestCLKEnable = ENABLE;
 *     DSI_InitStruct.ClockLaneHS2LPTime = 0x24;
 *     DSI_InitStruct.ClockLaneLP2HSTime = 0x3D;
 *     DSI_InitStruct.DataLaneHS2LPTime  = 0x26;
 *     DSI_InitStruct.DataLaneLP2HSTime  = 0xA1;
 *
 *     // Send every command packet type in low-power mode.
 *     DSI_InitStruct.LPGenShortWriteNoP  = DSI_LP_GSW0P_ENABLE;
 *     DSI_InitStruct.LPGenShortWriteOneP = DSI_LP_GSW1P_ENABLE;
 *     DSI_InitStruct.LPGenShortWriteTwoP = DSI_LP_GSW2P_ENABLE;
 *     DSI_InitStruct.LPGenShortReadNoP   = DSI_LP_GSR0P_ENABLE;
 *     DSI_InitStruct.LPGenShortReadOneP  = DSI_LP_GSR1P_ENABLE;
 *     DSI_InitStruct.LPGenShortReadTwoP  = DSI_LP_GSR2P_ENABLE;
 *     DSI_InitStruct.LPGenLongWrite      = DSI_LP_GLW_ENABLE;
 *     DSI_InitStruct.LPDcsShortWriteNoP  = DSI_LP_DSW0P_ENABLE;
 *     DSI_InitStruct.LPDcsShortWriteOneP = DSI_LP_DSW1P_ENABLE;
 *     DSI_InitStruct.LPDcsShortReadNoP   = DSI_LP_DSR0P_ENABLE;
 *     DSI_InitStruct.LPDcsLongWrite      = DSI_LP_DLW_ENABLE;
 *
 *     if (DSI_Init(DSI, &DSI_InitStruct) != DSI_STATUS_OK)
 *     {
 *         // invalid lane count or color coding
 *         return;
 *     }
 * }
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
FlagStatus DSI_Init(DSI_TypeDef *DSIx, const DSI_InitTypeDef *DSI_Init);

/**
 * \brief       Configure the DSI to operate in Video mode.
 *
 *              Programs video transmission mode (burst / non-burst), low-power
 *              transition areas, horizontal/vertical timing and the video
 *              pattern generator.
 *
 * \param[in]   DSIx   : pointer to the DSI peripheral registers.
 * \param[in]   VidCfg : pointer to DSI_VidCfgTypeDef structure that contains
 *                       the video mode configuration parameters.
 *
 * \return      DSI_STATUS_OK on success, DSI_STATUS_ERROR on invalid arguments.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_video_mode_init(void)
 * {
 *     DSI_VidCfgTypeDef VidCfg = {0};
 *
 *     VidCfg.Mode            = VIDEO_BURST_MODE_2;
 *     VidCfg.LPCommandEnable = ENABLE;
 *     // Allow the host to insert LP commands in every blanking period.
 *     VidCfg.LPVerticalSyncActiveEnable    = ENABLE;
 *     VidCfg.LPVerticalBackPorchEnable     = ENABLE;
 *     VidCfg.LPVerticalFrontPorchEnable    = ENABLE;
 *     VidCfg.LPVerticalActiveEnable        = ENABLE;
 *     VidCfg.LPHorizontalBackPorchEnable   = ENABLE;
 *     VidCfg.LPHorizontalFrontPorchEnable  = ENABLE;
 *
 *     // Largest LP command the line timing can carry, in bytes.
 *     VidCfg.invact_lpcmd_size  = 16;
 *     VidCfg.outvact_lpcmd_size = 45;
 *
 *     // One video packet per line, no chunking, no null packets.
 *     VidCfg.PacketSize     = 454;
 *     VidCfg.NumberOfChunks = 0;
 *     VidCfg.NullPacketSize = 0;
 *
 *     // Horizontal timings are in lane byte clock cycles.
 *     VidCfg.HorizontalSyncActive = 10;
 *     VidCfg.HorizontalBackPorch  = 50;
 *     VidCfg.HorizontalLine       = 2380;
 *     // Vertical timings are in lines.
 *     VidCfg.VerticalSyncActive = 2;
 *     VidCfg.VerticalBackPorch  = 8;
 *     VidCfg.VerticalFrontPorch = 8;
 *     VidCfg.VerticalActive     = 454;
 *
 *     // Feed real pixels from eDPI, not the internal test pattern.
 *     VidCfg.VPGEnable      = 0;
 *     VidCfg.VPGMode        = VIDEO_VPG_MODE_V_AND_H;
 *     VidCfg.VPGOrientation = VIDEO_VPG_ORIENTATION_H;
 *
 *     DSI_ConfigVideoMode(DSI, &VidCfg);
 * }
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
FlagStatus DSI_ConfigVideoMode(DSI_TypeDef *DSIx, const DSI_VidCfgTypeDef *VidCfg);

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
 * \brief       Configure the DSI to operate in Adapted Command mode.
 *
 *              Sets up Tearing Effect (TE) acknowledge and the eDPI hardware TE
 *              registers. When TE acknowledge is enabled, PCKHDL_CFG.bta_en is
 *              automatically asserted so the BTA request can be issued.
 *
 * \param[in]   DSIx           : pointer to the DSI peripheral registers.
 * \param[in]   AdaptedCmdCfg  : pointer to DSI_AdaptedCmdCfgTypeDef structure
 *                               that contains the TE configuration parameters.
 *
 * \return      DSI_STATUS_OK on success, DSI_STATUS_ERROR on invalid arguments.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_cmd_mode_init(void)
 * {
 *     DSI_AdaptedCmdCfgTypeDef AdaptedCmdCfg = {0};
 *
 *     // Enable TE so the host knows when the panel has finished a frame.
 *     AdaptedCmdCfg.TEAcknowledgeRequest = DSI_TE_ACKNOWLEDGE_ENABLE;
 *
 *     // Optional: let the host generate the TE pulse itself.
 *     AdaptedCmdCfg.hw_tear_effect_on  = 1;
 *     AdaptedCmdCfg.hw_tear_effect_gen = 1;
 *     AdaptedCmdCfg.hw_set_scan_line   = 1;
 *     AdaptedCmdCfg.scan_line_parameter = 0;
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
 * \brief       Configure and power-up the D-PHY through the host interface.
 *
 *              Brings the D-PHY out of shutdown/reset using PHY_RSTZ and waits
 *              until all active lanes (read from PHY_IF_CFG.n_lanes) reach Stop
 *              state. Precondition: the RTL-specific D-PHY PLL must already be
 *              configured.
 *
 * \param[in]   DSIx   : pointer to the DSI peripheral registers.
 * \param[in]   PhyCfg : pointer to DSI_PhyCfgTypeDef structure that contains
 *                       the PHY control parameters.
 *
 * \return      DSI_STATUS_OK on success, DSI_STATUS_ERROR on invalid arguments
 *              or timeout waiting for Stop state.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_dsi_phy_init(void)
 * {
 *     // The RTL D-PHY PLL must be configured first.
 *     DPHY_Init(1, 40, 400);   // 1 lane, 40MHz CKREF, 400MHz HS bit clock
 *
 *     DSI_PhyCfgTypeDef PhyCfg = {0};
 *     PhyCfg.phy_forcepll  = PHY_FORCEPLL_DISABLE;
 *     PhyCfg.phy_enableclk = PHY_CLK_ENABLE;
 *     PhyCfg.phy_rstz      = PHY_RST_ENABLE;
 *     PhyCfg.phy_shutdownz = PHY_SHUTDOWN_ENABLE;
 *
 *     // Returns only once all active lanes are in Stop (LP-11) state.
 *     if (DSI_ConfigPhy(DSI, &PhyCfg) != DSI_STATUS_OK)
 *     {
 *         // panel link not ready, PHY_STATUS tells which lane is stuck
 *         return;
 *     }
 * }
 * \endcode
 *
 * \ingroup     DSI_Exported_Functions
 */
FlagStatus DSI_ConfigPhy(DSI_TypeDef *DSIx, const DSI_PhyCfgTypeDef *PhyCfg);

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
