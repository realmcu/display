/**
*********************************************************************************************************
*               Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     rtl_lcdc_dphy.h
* @brief    This file provides all the LCDC HANDLER firmware functions.
* @details
* @author   feihu wang
* @date     2023-02-23
* @version  v0.1
*********************************************************************************************************
*/
#include "utils/rtl_utils.h"

typedef struct
{
    __IO uint32_t CLOCK_GEN;                                        /*0x00*/
    __IO uint32_t ESCAPE_TX_CLK_0;                          /*0x04*/
    __IO uint32_t ESCAPE_TX_DATA_0;                         /*0x08*/
    __IO uint32_t ESCAPE_TX_DATA_1;                         /*0x0C*/
    __IO uint32_t ESCAPE_TX_DATA_2;                         /*0x10*/
    __IO uint32_t ESCAPE_TX_DATA_3;                         /*0x14*/
    __IO uint32_t ESCAPE_TX_DATA_4;                         /*0x18*/
    __IO uint32_t ESCAPE_TX_DATA_5;                         /*0x1c*/
    __IO uint32_t ESCAPE_TX_DATA_6;                         /*0x20*/
    __IO uint32_t ESCAPE_RX_DATA_0;                         /*0x24*/
    __IO uint32_t LANE_SWAP_CTRL;                               /*0x28*/
    __IO uint32_t LANE_LP_BIST_0;                               /*0x2C*/
    __IO uint32_t LANE_HS_BIST;                                 /*0x30*/
    __IO uint32_t CD_SPSDT;                                         /*0x34*/
    __IO uint32_t DEBUG_0;                                          /*0x38*/
    __IO uint32_t DEBUG_1;                                          /*0x3C*/
    __IO uint32_t SSC0;                                                 /*0x40*/
    __IO uint32_t SSC1;                                                 /*0x44*/
    __IO uint32_t SSC2;                                                 /*0x48*/
    __IO uint32_t SSC3;                                                 /*0x4C*/
    __IO uint32_t WATCHDOG;                                         /*0x50*/
    __IO uint32_t RX_ESCAPE_READ_0;                         /*0x54*/
    __IO uint32_t RX_ESCAPE_READ_1;                         /*0x58*/
    __IO uint32_t EGLITCH_LP;                                       /*0x5C*/
    __IO uint32_t LP_BIST_1;                                        /*0x60*/
    __IO uint32_t LP_BIST_2;                                        /*0x64*/
    __IO uint32_t LANE_PN_SWAP_1;                               /*0x68*/
    __IO uint32_t LANE_PN_SWAP_2;                               /*0x6C*/
    __IO uint32_t RESV[20];                                         /*FROM 0x70-0xBC*/
    __IO uint32_t MPLL;                                                 /*0xC0*/
    __IO uint32_t LF;                                                       /*0xC4*/
    __IO uint32_t TXF;                                                  /*0xC8*/
    __IO uint32_t DF;                                                       /*0xCC*/
} DPHY_TypeDef;

#define MIPI_DPHY_REG_BASE                          (MIPI_REG_BASE + 0x400)     // 0x4060_2400
#define DPHY                            ((DPHY_TypeDef           *) MIPI_DPHY_REG_BASE)

/*=========================================================*/
/*REGISTER:CLOCK_GEN,Offset = 0x00*/
/*CLOCK_GEN[25:23]:reg_pll_en_sel*/
#define REG_PLL_EN_SEL_POS                  (23)
#define REG_PLL_EN_SEL_MSK                  (0x3<<REG_PLL_EN_SEL_POS)
#define REG_PLL_EN_SEL_CLR                  (~REG_PLL_EN_SEL_MSK)

/*CLOCK_GEN[22:22]:reg_disable_escape_mode*/
#define REG_DISABLE_ESCAPE_MODE_POS                 (22)
#define REG_DISABLE_ESCAPE_MODE_MSK                 (0x1<<REG_DISABLE_ESCAPE_MODE_POS)
#define REG_DISABLE_ESCAPE_MODE_CLR                 (~REG_DISABLE_ESCAPE_MODE_MSK)

/*CLOCK_GEN[21:21]:reg_mtx_en*/
#define REG_MTX_EN_POS                  (21)
#define REG_MTX_EN_MSK                  (0x1<<REG_MTX_EN_POS)
#define REG_MTX_EN_CLR                  (~REG_MTX_EN_MSK)

/*CLOCK_GEN[20:20]:reg_pll_ck_lane_enable*/
#define REG_PLL_CK_LANE_ENABLE_POS                  (20)
#define REG_PLL_CK_LANE_ENABLE_MSK                  (0x1<<REG_PLL_CK_LANE_ENABLE_POS)
#define REG_PLL_CK_LANE_ENABLE_CLR                  (~REG_PLL_CK_LANE_ENABLE_MSK)

/*CLOCK_GEN[19:19]:reg_pll_lane3_enable*/
#define REG_PLL_LANE3_ENABLE_POS                    (19)
#define REG_PLL_LANE3_ENABLE_MSK                    (0x1<<REG_PLL_LANE3_ENABLE_POS)
#define REG_PLL_LANE3_ENABLE_CLR                    (~REG_PLL_LANE3_ENABLE_MSK)

/*CLOCK_GEN[18:18]:reg_pll_lane2_enable*/
#define REG_PLL_LANE2_ENABLE_POS                    (18)
#define REG_PLL_LANE2_ENABLE_MSK                    (0x1<<REG_PLL_LANE2_ENABLE_POS)
#define REG_PLL_LANE2_ENABLE_CLR                    (~REG_PLL_LANE2_ENABLE_MSK)

/*CLOCK_GEN[17:17]:reg_pll_lane1_enable*/
#define REG_PLL_LANE1_ENABLE_POS                    (17)
#define REG_PLL_LANE1_ENABLE_MSK                    (0x1<<REG_PLL_LANE1_ENABLE_POS)
#define REG_PLL_LANE1_ENABLE_CLR                    (~REG_PLL_LANE1_ENABLE_MSK)

/*CLOCK_GEN[16:16]:reg_pll_lane0_enable*/
#define REG_PLL_LANE0_ENABLE_POS                    (16)
#define REG_PLL_LANE0_ENABLE_MSK                    (0x1<<REG_PLL_LANE0_ENABLE_POS)
#define REG_PLL_LANE0_ENABLE_CLR                    (~REG_PLL_LANE0_ENABLE_MSK)

/*CLOCK_GEN[15:15]:reg_bypass_clkrdy*/
#define REG_BYPASS_CLKRDY_POS                   (15)
#define REG_BYPASS_CLKRDY_MSK                   (0x1<<REG_BYPASS_CLKRDY_POS)
#define REG_BYPASS_CLKRDY_CLR                   (~REG_BYPASS_CLKRDY_MSK)

/*CLOCK_GEN[14:14]:reg_clk_deglitch_pll_en*/
#define REG_CLK_DEGLITCH_PLL_EN_POS                 (14)
#define REG_CLK_DEGLITCH_PLL_EN_MSK                 (0x1<<REG_CLK_DEGLITCH_PLL_EN_POS)
#define REG_CLK_DEGLITCH_PLL_EN_CLR                 (~REG_CLK_DEGLITCH_PLL_EN_MSK)

/*CLOCK_GEN[13:13]:reg_lane_sync_disable*/
#define REG_LANE_SYNC_DISABLE_POS                   (13)
#define REG_LANE_SYNC_DISABLE_MSK                   (0x1<<REG_LANE_SYNC_DISABLE_POS)
#define REG_LANE_SYNC_DISABLE_CLR                   (~REG_LANE_SYNC_DISABLE_MSK)

/*CLOCK_GEN[12:12]:reg_ck_escape_disable*/
#define REG_CK_ESCAPE_DISABLE_POS                   (12)
#define REG_CK_ESCAPE_DISABLE_MSK                   (0x1<<REG_CK_ESCAPE_DISABLE_POS)
#define REG_CK_ESCAPE_DISABLE_CLR                   (~REG_CK_ESCAPE_DISABLE_MSK)

/*CLOCK_GEN[11:11]:reg_tx_ulps_pll_enable*/
#define REG_TX_ULPS_PLL_ENABLE_POS                  (11)
#define REG_TX_ULPS_PLL_ENABLE_MSK                  (0x1<<REG_TX_ULPS_PLL_ENABLE_POS)
#define REG_TX_ULPS_PLL_ENABLE_CLR                  (~REG_TX_ULPS_PLL_ENABLE_MSK)

/*CLOCK_GEN[10:10]:reg_tx_rst_n*/
#define REG_TX_RST_N_POS                    (10)
#define REG_TX_RST_N_MSK                    (0x1<<REG_TX_RST_N_POS)
#define REG_TX_RST_N_CLR                    (~REG_TX_RST_N_MSK)

/*CLOCK_GEN[9:9]:reg_rx_rst_n*/
#define REG_RX_RST_N_POS                    (9)
#define REG_RX_RST_N_MSK                    (0x1<<REG_RX_RST_N_POS)
#define REG_RX_RST_N_CLR                    (~REG_RX_RST_N_MSK)

/*CLOCK_GEN[8:8]:reg_clk_lane_en*/
#define REG_CLK_LANE_EN_POS                 (8)
#define REG_CLK_LANE_EN_MSK                 (0x1<<REG_CLK_LANE_EN_POS)
#define REG_CLK_LANE_EN_CLR                 (~REG_CLK_LANE_EN_MSK)

/*CLOCK_GEN[7:7]:reg_lane3_en*/
#define REG_LANE3_EN_POS                    (7)
#define REG_LANE3_EN_MSK                    (0x1<<REG_LANE3_EN_POS)
#define REG_LANE3_EN_CLR                    (~REG_LANE3_EN_MSK)

/*CLOCK_GEN[6:6]:reg_lane2_en*/
#define REG_LANE2_EN_POS                    (6)
#define REG_LANE2_EN_MSK                    (0x1<<REG_LANE2_EN_POS)
#define REG_LANE2_EN_CLR                    (~REG_LANE2_EN_MSK)

/*CLOCK_GEN[5:5]:reg_lane1_en*/
#define REG_LANE1_EN_POS                    (5)
#define REG_LANE1_EN_MSK                    (0x1<<REG_LANE1_EN_POS)
#define REG_LANE1_EN_CLR                    (~REG_LANE1_EN_MSK)

/*CLOCK_GEN[4:4]:reg_lane0_en*/
#define REG_LANE0_EN_POS                    (4)
#define REG_LANE0_EN_MSK                    (0x1<<REG_LANE0_EN_POS)
#define REG_LANE0_EN_CLR                    (~REG_LANE0_EN_MSK)

/*CLOCK_GEN[3:3]:reg_txbyteclkhs_ckinv*/
#define REG_TXBYTECLKHS_CKINV_POS                   (3)
#define REG_TXBYTECLKHS_CKINV_MSK                   (0x1<<REG_TXBYTECLKHS_CKINV_POS)
#define REG_TXBYTECLKHS_CKINV_CLR                   (~REG_TXBYTECLKHS_CKINV_MSK)

/*CLOCK_GEN[2:2]:reg_txclkesc_ckinv*/
#define REG_TXCLKESC_CKINV_POS                  (2)
#define REG_TXCLKESC_CKINV_MSK                  (0x1<<REG_TXCLKESC_CKINV_POS)
#define REG_TXCLKESC_CKINV_CLR                  (~REG_TXCLKESC_CKINV_MSK)

/*CLOCK_GEN[1:0]:reg_txclkesc_sel*/
#define REG_TXCLKESC_SEL_POS                    (0)
#define REG_TXCLKESC_SEL_MSK                    (0x2<<REG_TXCLKESC_SEL_POS)
#define REG_TXCLKESC_SEL_CLR                    (~REG_TXCLKESC_SEL_MSK)

/*=========================================================*/
/*REGISTER: ESCAPE_TX_CLK_0,Offset = 0x04*/
/* ESCAPE_TX_CLK_0[27:27]:reg_rqst_lp_mask_enable*/
#define REG_RQST_LP_MASK_ENABLE_POS                 (27)
#define REG_RQST_LP_MASK_ENABLE_MSK                 (0x1<<REG_RQST_LP_MASK_ENABLE_POS)
#define REG_RQST_LP_MASK_ENABLE_CLR                 (~REG_RQST_LP_MASK_ENABLE_MSK)

/* ESCAPE_TX_CLK_0[26:26]:reg_lp_clk_cfg*/
#define REG_LP_CLK_CFG_POS                  (26)
#define REG_LP_CLK_CFG_MSK                  (0x1<<REG_LP_CLK_CFG_POS)
#define REG_LP_CLK_CFG_CLR                  (~REG_LP_CLK_CFG_MSK)

/* ESCAPE_TX_CLK_0[25:24]:reg_ulps_entry_sel*/
#define REG_ULPS_ENTRY_SEL_POS                  (24)
#define REG_ULPS_ENTRY_SEL_MSK                  (0x2<<REG_ULPS_ENTRY_SEL_POS)
#define REG_ULPS_ENTRY_SEL_CLR                  (~REG_ULPS_ENTRY_SEL_MSK)

/* ESCAPE_TX_CLK_0[23:20]:reg_ck_tx_10_exit_time*/
#define REG_CK_TX_10_EXIT_TIME_POS                  (20)
#define REG_CK_TX_10_EXIT_TIME_MSK                  (0x4<<REG_CK_TX_10_EXIT_TIME_POS)
#define REG_CK_TX_10_EXIT_TIME_CLR                  (~REG_CK_TX_10_EXIT_TIME_MSK)

/* ESCAPE_TX_CLK_0[19:16]:reg_ck_tx_idle_exit_time*/
#define REG_CK_TX_IDLE_EXIT_TIME_POS                    (16)
#define REG_CK_TX_IDLE_EXIT_TIME_MSK                    (0x4<<REG_CK_TX_IDLE_EXIT_TIME_POS)
#define REG_CK_TX_IDLE_EXIT_TIME_CLR                    (~REG_CK_TX_IDLE_EXIT_TIME_MSK)

/* ESCAPE_TX_CLK_0[15:0]:reg_ck_ulps_exit_time*/
#define REG_CK_ULPS_EXIT_TIME_POS                   (0)
#define REG_CK_ULPS_EXIT_TIME_MSK                   (0x10<<REG_CK_ULPS_EXIT_TIME_POS)
#define REG_CK_ULPS_EXIT_TIME_CLR                   (~REG_CK_ULPS_EXIT_TIME_MSK)

/*=========================================================*/
/*REGISTER:ESCAPE_TX_DATA_0,Offset = 0x08*/
/*ESCAPE_TX_DATA_0[31:31]:reg_data_prpr_mask_enable*/
#define REG_DATA_PRPR_MASK_ENABLE_POS                   (31)
#define REG_DATA_PRPR_MASK_ENABLE_MSK                   (0x1<<REG_DATA_PRPR_MASK_ENABLE_POS)
#define REG_DATA_PRPR_MASK_ENABLE_CLR                   (~REG_DATA_PRPR_MASK_ENABLE_MSK)

/*ESCAPE_TX_DATA_0[30:28]:reg_data_prpr_mask_counter*/
#define REG_DATA_PRPR_MASK_COUNTER_POS                  (28)
#define REG_DATA_PRPR_MASK_COUNTER_MSK                  (0x3<<REG_DATA_PRPR_MASK_COUNTER_POS)
#define REG_DATA_PRPR_MASK_COUNTER_CLR                  (~REG_DATA_PRPR_MASK_COUNTER_MSK)

/*ESCAPE_TX_DATA_0[26:26]:reg_clk_prpr_mask_enable*/
#define REG_CLK_PRPR_MASK_ENABLE_POS                    (26)
#define REG_CLK_PRPR_MASK_ENABLE_MSK                    (0x1<<REG_CLK_PRPR_MASK_ENABLE_POS)
#define REG_CLK_PRPR_MASK_ENABLE_CLR                    (~REG_CLK_PRPR_MASK_ENABLE_MSK)

/*ESCAPE_TX_DATA_0[25:23]:reg_clk_prpr_mask_counter*/
#define REG_CLK_PRPR_MASK_COUNTER_POS                   (23)
#define REG_CLK_PRPR_MASK_COUNTER_MSK                   (0x3<<REG_CLK_PRPR_MASK_COUNTER_POS)
#define REG_CLK_PRPR_MASK_COUNTER_CLR                   (~REG_CLK_PRPR_MASK_COUNTER_MSK)

/*ESCAPE_TX_DATA_0[22:22]:reg_rqst_lp_one_quarter_mask_enable*/
#define REG_RQST_LP_ONE_QUARTER_MASK_ENABLE_POS                 (22)
#define REG_RQST_LP_ONE_QUARTER_MASK_ENABLE_MSK                 (0x1<<REG_RQST_LP_ONE_QUARTER_MASK_ENABLE_POS)
#define REG_RQST_LP_ONE_QUARTER_MASK_ENABLE_CLR                 (~REG_RQST_LP_ONE_QUARTER_MASK_ENABLE_MSK)

/*ESCAPE_TX_DATA_0[21:18]:reg_rqst_lp_one_quarter_mask_counter*/
#define REG_RQST_LP_ONE_QUARTER_MASK_COUNTER_POS                    (18)
#define REG_RQST_LP_ONE_QUARTER_MASK_COUNTER_MSK                    (0x4<<REG_RQST_LP_ONE_QUARTER_MASK_COUNTER_POS)
#define REG_RQST_LP_ONE_QUARTER_MASK_COUNTER_CLR                    (~REG_RQST_LP_ONE_QUARTER_MASK_COUNTER_MSK)

/*ESCAPE_TX_DATA_0[17:17]:reg_taget_timeout_disable*/
#define REG_TAGET_TIMEOUT_DISABLE_POS                   (17)
#define REG_TAGET_TIMEOUT_DISABLE_MSK                   (0x1<<REG_TAGET_TIMEOUT_DISABLE_POS)
#define REG_TAGET_TIMEOUT_DISABLE_CLR                   (~REG_TAGET_TIMEOUT_DISABLE_MSK)

/*ESCAPE_TX_DATA_0[16:12]:reg_time_taget_timeout*/
#define REG_TIME_TAGET_TIMEOUT_POS                  (12)
#define REG_TIME_TAGET_TIMEOUT_MSK                  (0x5<<REG_TIME_TAGET_TIMEOUT_POS)
#define REG_TIME_TAGET_TIMEOUT_CLR                  (~REG_TIME_TAGET_TIMEOUT_MSK)

/*ESCAPE_TX_DATA_0[11:8]:reg_time_taget*/
#define REG_TIME_TAGET_POS                  (8)
#define REG_TIME_TAGET_MSK                  (0x4<<REG_TIME_TAGET_POS)
#define REG_TIME_TAGET_CLR                  (~REG_TIME_TAGET_MSK)

/*ESCAPE_TX_DATA_0[7:4]:reg_time_tago*/
#define REG_TIME_TAGO_POS                   (4)
#define REG_TIME_TAGO_MSK                   (0x4<<REG_TIME_TAGO_POS)
#define REG_TIME_TAGO_CLR                   (~REG_TIME_TAGO_MSK)

/*ESCAPE_TX_DATA_0[3:0]:reg_tx_idle_exit_time*/
#define REG_TX_IDLE_EXIT_TIME_POS                   (0)
#define REG_TX_IDLE_EXIT_TIME_MSK                   (0x4<<REG_TX_IDLE_EXIT_TIME_POS)
#define REG_TX_IDLE_EXIT_TIME_CLR                   (~REG_TX_IDLE_EXIT_TIME_MSK)

/*=========================================================*/
/*REGISTER:ESCAPE_TX_DATA_1,Offset = 0x0C*/
/*ESCAPE_TX_DATA_1[31:24]:reg_tx_lpx_time*/
#define REG_TX_LPX_TIME_POS                 (24)
#define REG_TX_LPX_TIME_MSK                 (0x8<<REG_TX_LPX_TIME_POS)
#define REG_TX_LPX_TIME_CLR                 (~REG_TX_LPX_TIME_MSK)

/*ESCAPE_TX_DATA_1[23:16]:reg_hs_exit_time*/
#define REG_HS_EXIT_TIME_POS                    (16)
#define REG_HS_EXIT_TIME_MSK                    (0x8<<REG_HS_EXIT_TIME_POS)
#define REG_HS_EXIT_TIME_CLR                    (~REG_HS_EXIT_TIME_MSK)

/*ESCAPE_TX_DATA_1[15:0]:reg_tx_init_time*/
#define REG_TX_INIT_TIME_POS                    (0)
#define REG_TX_INIT_TIME_MSK                    (0x10<<REG_TX_INIT_TIME_POS)
#define REG_TX_INIT_TIME_CLR                    (~REG_TX_INIT_TIME_MSK)

/*=========================================================*/
/*REGISTER: ESCAPE_TX_DATA_2,Offset = 0x10*/
/* ESCAPE_TX_DATA_2[31:24]:reg_clk_pre_time*/
#define REG_CLK_PRE_TIME_POS                    (24)
#define REG_CLK_PRE_TIME_MSK                    (0x8<<REG_CLK_PRE_TIME_POS)
#define REG_CLK_PRE_TIME_CLR                    (~REG_CLK_PRE_TIME_MSK)

/* ESCAPE_TX_DATA_2[23:16]:reg_clk_zero_time*/
#define REG_CLK_ZERO_TIME_POS                   (16)
#define REG_CLK_ZERO_TIME_MSK                   (0x8<<REG_CLK_ZERO_TIME_POS)
#define REG_CLK_ZERO_TIME_CLR                   (~REG_CLK_ZERO_TIME_MSK)

/* ESCAPE_TX_DATA_2[15:8]:reg_clk_prpr_time*/
#define REG_CLK_PRPR_TIME_POS                   (8)
#define REG_CLK_PRPR_TIME_MSK                   (0x8<<REG_CLK_PRPR_TIME_POS)
#define REG_CLK_PRPR_TIME_CLR                   (~REG_CLK_PRPR_TIME_MSK)

/* ESCAPE_TX_DATA_2[7:0]:reg_clk_post_time*/
#define REG_CLK_POST_TIME_POS                   (0)
#define REG_CLK_POST_TIME_MSK                   (0x8<<REG_CLK_POST_TIME_POS)
#define REG_CLK_POST_TIME_CLR                   (~REG_CLK_POST_TIME_MSK)

/*=========================================================*/
/*REGISTER: ESCAPE_TX_DATA_3,Offset = 0x14*/
/* ESCAPE_TX_DATA_3[31:24]:reg_tx_vld_time*/
#define REG_TX_VLD_TIME_POS                 (24)
#define REG_TX_VLD_TIME_MSK                 (0x8<<REG_TX_VLD_TIME_POS)
#define REG_TX_VLD_TIME_CLR                 (~REG_TX_VLD_TIME_MSK)

/* ESCAPE_TX_DATA_3[23:16]:reg_tx_tail_time*/
#define REG_TX_TAIL_TIME_POS                    (16)
#define REG_TX_TAIL_TIME_MSK                    (0x8<<REG_TX_TAIL_TIME_POS)
#define REG_TX_TAIL_TIME_CLR                    (~REG_TX_TAIL_TIME_MSK)

/* ESCAPE_TX_DATA_3[15:8]:reg_hs_zero_time*/
#define REG_HS_ZERO_TIME_POS                    (8)
#define REG_HS_ZERO_TIME_MSK                    (0x8<<REG_HS_ZERO_TIME_POS)
#define REG_HS_ZERO_TIME_CLR                    (~REG_HS_ZERO_TIME_MSK)

/* ESCAPE_TX_DATA_3[7:0]:reg_hs_prpr_time*/
#define REG_HS_PRPR_TIME_POS                    (0)
#define REG_HS_PRPR_TIME_MSK                    (0x8<<REG_HS_PRPR_TIME_POS)
#define REG_HS_PRPR_TIME_CLR                    (~REG_HS_PRPR_TIME_MSK)

/*=========================================================*/
/*REGISTER:ESCAPE_TX_DATA_4,Offset = 0x18*/
/*ESCAPE_TX_DATA_4[19:19]:reg_clkedge_cfg_3*/
#define REG_CLKEDGE_CFG_3_POS                   (19)
#define REG_CLKEDGE_CFG_3_MSK                   (0x1<<REG_CLKEDGE_CFG_3_POS)
#define REG_CLKEDGE_CFG_3_CLR                   (~REG_CLKEDGE_CFG_3_MSK)

/*ESCAPE_TX_DATA_4[18:18]:reg_lp_dat_cfg_3*/
#define REG_LP_DAT_CFG_3_POS                    (18)
#define REG_LP_DAT_CFG_3_MSK                    (0x1<<REG_LP_DAT_CFG_3_POS)
#define REG_LP_DAT_CFG_3_CLR                    (~REG_LP_DAT_CFG_3_MSK)

/*ESCAPE_TX_DATA_4[17:17]:reg_hsvld_dat_cfg_3*/
#define REG_HSVLD_DAT_CFG_3_POS                 (17)
#define REG_HSVLD_DAT_CFG_3_MSK                 (0x1<<REG_HSVLD_DAT_CFG_3_POS)
#define REG_HSVLD_DAT_CFG_3_CLR                 (~REG_HSVLD_DAT_CFG_3_MSK)

/*ESCAPE_TX_DATA_4[16:16]:reg_clkedge_cfg_2*/
#define REG_CLKEDGE_CFG_2_POS                   (16)
#define REG_CLKEDGE_CFG_2_MSK                   (0x1<<REG_CLKEDGE_CFG_2_POS)
#define REG_CLKEDGE_CFG_2_CLR                   (~REG_CLKEDGE_CFG_2_MSK)

/*ESCAPE_TX_DATA_4[15:15]:reg_lp_dat_cfg_2*/
#define REG_LP_DAT_CFG_2_POS                    (15)
#define REG_LP_DAT_CFG_2_MSK                    (0x1<<REG_LP_DAT_CFG_2_POS)
#define REG_LP_DAT_CFG_2_CLR                    (~REG_LP_DAT_CFG_2_MSK)

/*ESCAPE_TX_DATA_4[14:14]:reg_hsvld_dat_cfg_2*/
#define REG_HSVLD_DAT_CFG_2_POS                 (14)
#define REG_HSVLD_DAT_CFG_2_MSK                 (0x1<<REG_HSVLD_DAT_CFG_2_POS)
#define REG_HSVLD_DAT_CFG_2_CLR                 (~REG_HSVLD_DAT_CFG_2_MSK)

/*ESCAPE_TX_DATA_4[13:13]:reg_clkedge_cfg_1*/
#define REG_CLKEDGE_CFG_1_POS                   (13)
#define REG_CLKEDGE_CFG_1_MSK                   (0x1<<REG_CLKEDGE_CFG_1_POS)
#define REG_CLKEDGE_CFG_1_CLR                   (~REG_CLKEDGE_CFG_1_MSK)

/*ESCAPE_TX_DATA_4[12:12]:reg_lp_dat_cfg_1*/
#define REG_LP_DAT_CFG_1_POS                    (12)
#define REG_LP_DAT_CFG_1_MSK                    (0x1<<REG_LP_DAT_CFG_1_POS)
#define REG_LP_DAT_CFG_1_CLR                    (~REG_LP_DAT_CFG_1_MSK)

/*ESCAPE_TX_DATA_4[11:11]:reg_hsvld_dat_cfg_1*/
#define REG_HSVLD_DAT_CFG_1_POS                 (11)
#define REG_HSVLD_DAT_CFG_1_MSK                 (0x1<<REG_HSVLD_DAT_CFG_1_POS)
#define REG_HSVLD_DAT_CFG_1_CLR                 (~REG_HSVLD_DAT_CFG_1_MSK)

/*ESCAPE_TX_DATA_4[10:10]:reg_clkedge_cfg_0*/
#define REG_CLKEDGE_CFG_0_POS                   (10)
#define REG_CLKEDGE_CFG_0_MSK                   (0x1<<REG_CLKEDGE_CFG_0_POS)
#define REG_CLKEDGE_CFG_0_CLR                   (~REG_CLKEDGE_CFG_0_MSK)

/*ESCAPE_TX_DATA_4[9:9]:reg_lp_dat_cfg_0*/
#define REG_LP_DAT_CFG_0_POS                    (9)
#define REG_LP_DAT_CFG_0_MSK                    (0x1<<REG_LP_DAT_CFG_0_POS)
#define REG_LP_DAT_CFG_0_CLR                    (~REG_LP_DAT_CFG_0_MSK)

/*ESCAPE_TX_DATA_4[8:8]:reg_hsvld_dat_cfg_0*/
#define REG_HSVLD_DAT_CFG_0_POS                 (8)
#define REG_HSVLD_DAT_CFG_0_MSK                 (0x1<<REG_HSVLD_DAT_CFG_0_POS)
#define REG_HSVLD_DAT_CFG_0_CLR                 (~REG_HSVLD_DAT_CFG_0_MSK)

/*ESCAPE_TX_DATA_4[7:0]:reg_hs_wkup_time*/
#define REG_HS_WKUP_TIME_POS                    (0)
#define REG_HS_WKUP_TIME_MSK                    (0x8<<REG_HS_WKUP_TIME_POS)
#define REG_HS_WKUP_TIME_CLR                    (~REG_HS_WKUP_TIME_MSK)

/*=========================================================*/
/*REGISTER:ESCAPE_TX_DATA_5,Offset = 0x1C*/
/*ESCAPE_TX_DATA_5[31:24]:reg_dig_reserved_3*/
#define REG_DIG_RESERVED_3_POS                  (24)
#define REG_DIG_RESERVED_3_MSK                  (0x8<<REG_DIG_RESERVED_3_POS)
#define REG_DIG_RESERVED_3_CLR                  (~REG_DIG_RESERVED_3_MSK)

/*ESCAPE_TX_DATA_5[23:16]:reg_dig_reserved_2*/
#define REG_DIG_RESERVED_2_POS                  (16)
#define REG_DIG_RESERVED_2_MSK                  (0x8<<REG_DIG_RESERVED_2_POS)
#define REG_DIG_RESERVED_2_CLR                  (~REG_DIG_RESERVED_2_MSK)

/*ESCAPE_TX_DATA_5[15:8]:reg_dig_reserved_1*/
#define REG_DIG_RESERVED_1_POS                  (8)
#define REG_DIG_RESERVED_1_MSK                  (0x8<<REG_DIG_RESERVED_1_POS)
#define REG_DIG_RESERVED_1_CLR                  (~REG_DIG_RESERVED_1_MSK)

/*ESCAPE_TX_DATA_5[7:0]:reg_dig_reserved_0*/
#define REG_DIG_RESERVED_0_POS                  (0)
#define REG_DIG_RESERVED_0_MSK                  (0x8<<REG_DIG_RESERVED_0_POS)
#define REG_DIG_RESERVED_0_CLR                  (~REG_DIG_RESERVED_0_MSK)

/*=========================================================*/
/*REGISTER:ESCAPE_TX_DATA_6,Offset = 0x20*/
/*ESCAPE_TX_DATA_6[31:16]:reg_skew_time*/
#define REG_SKEW_TIME_POS                   (16)
#define REG_SKEW_TIME_MSK                   (0x10<<REG_SKEW_TIME_POS)
#define REG_SKEW_TIME_CLR                   (~REG_SKEW_TIME_MSK)

/*ESCAPE_TX_DATA_6[15:0]:reg_ulps_exit_time*/
#define REG_ULPS_EXIT_TIME_POS                  (0)
#define REG_ULPS_EXIT_TIME_MSK                  (0x10<<REG_ULPS_EXIT_TIME_POS)
#define REG_ULPS_EXIT_TIME_CLR                  (~REG_ULPS_EXIT_TIME_MSK)

/*=========================================================*/
/*REGISTER:ESCAPE_RX_DATA_0,Offset = 0x24*/

/*ESCAPE_RX_DATA_0[30:30]:reg_disable_erresc*/
#define REG_DISABLE_ERRESC_POS                  (30)
#define REG_DISABLE_ERRESC_MSK                  (0x1<<REG_DISABLE_ERRESC_POS)
#define REG_DISABLE_ERRESC_CLR                  (~REG_DISABLE_ERRESC_MSK)

/*ESCAPE_RX_DATA_0[29:29]:reg_disable_errsyncesc*/
#define REG_DISABLE_ERRSYNCESC_POS                  (29)
#define REG_DISABLE_ERRSYNCESC_MSK                  (0x1<<REG_DISABLE_ERRSYNCESC_POS)
#define REG_DISABLE_ERRSYNCESC_CLR                  (~REG_DISABLE_ERRSYNCESC_MSK)

/*ESCAPE_RX_DATA_0[28:28]:reg_disable_errcontrol*/
#define REG_DISABLE_ERRCONTROL_POS                  (28)
#define REG_DISABLE_ERRCONTROL_MSK                  (0x1<<REG_DISABLE_ERRCONTROL_POS)
#define REG_DISABLE_ERRCONTROL_CLR                  (~REG_DISABLE_ERRCONTROL_MSK)

/*ESCAPE_RX_DATA_0[27:24]:reg_rx_lp_delay*/
#define REG_RX_LP_DELAY_POS                 (24)
#define REG_RX_LP_DELAY_MSK                 (0x4<<REG_RX_LP_DELAY_POS)
#define REG_RX_LP_DELAY_CLR                 (~REG_RX_LP_DELAY_MSK)

/*ESCAPE_RX_DATA_0[23:23]:reg_rx_lp_deglitch*/
#define REG_RX_LP_DEGLITCH_POS                  (23)
#define REG_RX_LP_DEGLITCH_MSK                  (0x1<<REG_RX_LP_DEGLITCH_POS)
#define REG_RX_LP_DEGLITCH_CLR                  (~REG_RX_LP_DEGLITCH_MSK)

/*ESCAPE_RX_DATA_0[22:18]:reg_time_rx2tx*/
#define REG_TIME_RX2TX_POS                  (18)
#define REG_TIME_RX2TX_MSK                  (0x5<<REG_TIME_RX2TX_POS)
#define REG_TIME_RX2TX_CLR                  (~REG_TIME_RX2TX_MSK)

/*ESCAPE_RX_DATA_0[17:13]:reg_time_rx_ta_ack*/
#define REG_TIME_RX_TA_ACK_POS                  (13)
#define REG_TIME_RX_TA_ACK_MSK                  (0x5<<REG_TIME_RX_TA_ACK_POS)
#define REG_TIME_RX_TA_ACK_CLR                  (~REG_TIME_RX_TA_ACK_MSK)

/*ESCAPE_RX_DATA_0[12:5]:reg_rx_clock_phase*/
#define REG_RX_CLOCK_PHASE_POS                  (5)
#define REG_RX_CLOCK_PHASE_MSK                  (0x8<<REG_RX_CLOCK_PHASE_POS)
#define REG_RX_CLOCK_PHASE_CLR                  (~REG_RX_CLOCK_PHASE_MSK)

/*ESCAPE_RX_DATA_0[4:0]:reg_deglitch_cfg*/
#define REG_DEGLITCH_CFG_POS                    (0)
#define REG_DEGLITCH_CFG_MSK                    (0x5<<REG_DEGLITCH_CFG_POS)
#define REG_DEGLITCH_CFG_CLR                    (~REG_DEGLITCH_CFG_MSK)

/*=========================================================*/
/*REGISTER:LANE_SWAP_CTRL,Offset = 0x28*/

/*LANE_SWAP_CTRL[16:15]:lane3_sel*/
#define LANE3_SEL_POS                   (15)
#define LANE3_SEL_MSK                   (0x2<<LANE3_SEL_POS)
#define LANE3_SEL_CLR                   (~LANE3_SEL_MSK)

/*LANE_SWAP_CTRL[14:13]:lane2_sel*/
#define LANE2_SEL_POS                   (13)
#define LANE2_SEL_MSK                   (0x2<<LANE2_SEL_POS)
#define LANE2_SEL_CLR                   (~LANE2_SEL_MSK)

/*LANE_SWAP_CTRL[12:11]:lane1_sel*/
#define LANE1_SEL_POS                   (11)
#define LANE1_SEL_MSK                   (0x2<<LANE1_SEL_POS)
#define LANE1_SEL_CLR                   (~LANE1_SEL_MSK)

/*LANE_SWAP_CTRL[10:9]:lane0_sel*/
#define LANE0_SEL_POS                   (9)
#define LANE0_SEL_MSK                   (0x2<<LANE0_SEL_POS)
#define LANE0_SEL_CLR                   (~LANE0_SEL_MSK)

/*LANE_SWAP_CTRL[8:7]:reg_mux_delay_sel*/
#define REG_MUX_DELAY_SEL_POS                   (7)
#define REG_MUX_DELAY_SEL_MSK                   (0x2<<REG_MUX_DELAY_SEL_POS)
#define REG_MUX_DELAY_SEL_CLR                   (~REG_MUX_DELAY_SEL_MSK)

/*LANE_SWAP_CTRL[6:6]:clkedge_cfg*/
#define CLKEDGE_CFG_POS                 (6)
#define CLKEDGE_CFG_MSK                 (0x1<<CLKEDGE_CFG_POS)
#define CLKEDGE_CFG_CLR                 (~CLKEDGE_CFG_MSK)

/*LANE_SWAP_CTRL[5:5]:bit_swp*/
#define BIT_SWP_POS                 (5)
#define BIT_SWP_MSK                 (0x1<<BIT_SWP_POS)
#define BIT_SWP_CLR                 (~BIT_SWP_MSK)

/*LANE_SWAP_CTRL[4:4]:enhst_dat_cfg*/
#define ENHST_DAT_CFG_POS                   (4)
#define ENHST_DAT_CFG_MSK                   (0x1<<ENHST_DAT_CFG_POS)
#define ENHST_DAT_CFG_CLR                   (~ENHST_DAT_CFG_MSK)

/*LANE_SWAP_CTRL[3:3]:wkup_dat_cfg*/
#define WKUP_DAT_CFG_POS                    (3)
#define WKUP_DAT_CFG_MSK                    (0x1<<WKUP_DAT_CFG_POS)
#define WKUP_DAT_CFG_CLR                    (~WKUP_DAT_CFG_MSK)

/*LANE_SWAP_CTRL[2:2]:hsvld_clk_cfg*/
#define HSVLD_CLK_CFG_POS                   (2)
#define HSVLD_CLK_CFG_MSK                   (0x1<<HSVLD_CLK_CFG_POS)
#define HSVLD_CLK_CFG_CLR                   (~HSVLD_CLK_CFG_MSK)

/*LANE_SWAP_CTRL[1:1]:enhst_clk_cfg*/
#define ENHST_CLK_CFG_POS                   (1)
#define ENHST_CLK_CFG_MSK                   (0x1<<ENHST_CLK_CFG_POS)
#define ENHST_CLK_CFG_CLR                   (~ENHST_CLK_CFG_MSK)

/*LANE_SWAP_CTRL[0:0]:wkup_clk_cfg*/
#define WKUP_CLK_CFG_POS                    (0)
#define WKUP_CLK_CFG_MSK                    (0x1<<WKUP_CLK_CFG_POS)
#define WKUP_CLK_CFG_CLR                    (~WKUP_CLK_CFG_MSK)

/*=========================================================*/
/*REGISTER: LANE_LP_BIST_0,Offset = 0x2C*/

/* LANE_LP_BIST_0[20:13]:reg_rx_lp_bist_pattern*/
#define REG_RX_LP_BIST_PATTERN_POS                  (13)
#define REG_RX_LP_BIST_PATTERN_MSK                  (0x8<<REG_RX_LP_BIST_PATTERN_POS)
#define REG_RX_LP_BIST_PATTERN_CLR                  (~REG_RX_LP_BIST_PATTERN_MSK)

/* LANE_LP_BIST_0[12:5]:reg_tx_lp_bist_pattern*/
#define REG_TX_LP_BIST_PATTERN_POS                  (5)
#define REG_TX_LP_BIST_PATTERN_MSK                  (0x8<<REG_TX_LP_BIST_PATTERN_POS)
#define REG_TX_LP_BIST_PATTERN_CLR                  (~REG_TX_LP_BIST_PATTERN_MSK)

/* LANE_LP_BIST_0[4:4]:reg_bist_path*/
#define REG_BIST_PATH_POS                   (4)
#define REG_BIST_PATH_MSK                   (0x1<<REG_BIST_PATH_POS)
#define REG_BIST_PATH_CLR                   (~REG_BIST_PATH_MSK)

/* LANE_LP_BIST_0[3:3]:reg_rx_lp_bist_enable*/
#define REG_RX_LP_BIST_ENABLE_POS                   (3)
#define REG_RX_LP_BIST_ENABLE_MSK                   (0x1<<REG_RX_LP_BIST_ENABLE_POS)
#define REG_RX_LP_BIST_ENABLE_CLR                   (~REG_RX_LP_BIST_ENABLE_MSK)

/* LANE_LP_BIST_0[2:2]:reg_tx_lp_bist_enable*/
#define REG_TX_LP_BIST_ENABLE_POS                   (2)
#define REG_TX_LP_BIST_ENABLE_MSK                   (0x1<<REG_TX_LP_BIST_ENABLE_POS)
#define REG_TX_LP_BIST_ENABLE_CLR                   (~REG_TX_LP_BIST_ENABLE_MSK)

/* LANE_LP_BIST_0[1:0]:reg_bist_pattern_sel*/
#define REG_BIST_PATTERN_SEL_POS                    (0)
#define REG_BIST_PATTERN_SEL_MSK                    (0x2<<REG_BIST_PATTERN_SEL_POS)
#define REG_BIST_PATTERN_SEL_CLR                    (~REG_BIST_PATTERN_SEL_MSK)

/*=========================================================*/
/*REGISTER:LANE_HS_BIST,Offset = 0x30*/
/*LANE_HS_BIST[31:30]:reg_pattern_sel*/
#define REG_PATTERN_SEL_POS                 (30)
#define REG_PATTERN_SEL_MSK                 (0x2<<REG_PATTERN_SEL_POS)
#define REG_PATTERN_SEL_CLR                 (~REG_PATTERN_SEL_MSK)

/*LANE_HS_BIST[29:29]:reg_prbs_invs*/
#define REG_PRBS_INVS_POS                   (29)
#define REG_PRBS_INVS_MSK                   (0x1<<REG_PRBS_INVS_POS)
#define REG_PRBS_INVS_CLR                   (~REG_PRBS_INVS_MSK)

/*LANE_HS_BIST[28:28]:reg_prbs_enable*/
#define REG_PRBS_ENABLE_POS                 (28)
#define REG_PRBS_ENABLE_MSK                 (0x1<<REG_PRBS_ENABLE_POS)
#define REG_PRBS_ENABLE_CLR                 (~REG_PRBS_ENABLE_MSK)

/*LANE_HS_BIST[27:27]:test_en*/
#define TEST_EN_POS                 (27)
#define TEST_EN_MSK                 (0x1<<TEST_EN_POS)
#define TEST_EN_CLR                 (~TEST_EN_MSK)

/*LANE_HS_BIST[26:26]:test_hs_en*/
#define TEST_HS_EN_POS                  (26)
#define TEST_HS_EN_MSK                  (0x1<<TEST_HS_EN_POS)
#define TEST_HS_EN_CLR                  (~TEST_HS_EN_MSK)

/*LANE_HS_BIST[25:25]:test_dat_mode*/
#define TEST_DAT_MODE_POS                   (25)
#define TEST_DAT_MODE_MSK                   (0x1<<TEST_DAT_MODE_POS)
#define TEST_DAT_MODE_CLR                   (~TEST_DAT_MODE_MSK)

/*LANE_HS_BIST[24:24]:test_hsdat_only*/
#define TEST_HSDAT_ONLY_POS                 (24)
#define TEST_HSDAT_ONLY_MSK                 (0x1<<TEST_HSDAT_ONLY_POS)
#define TEST_HSDAT_ONLY_CLR                 (~TEST_HSDAT_ONLY_MSK)

/*LANE_HS_BIST[23:16]:pdat_tst*/
#define PDAT_TST_POS                    (16)
#define PDAT_TST_MSK                    (0x8<<PDAT_TST_POS)
#define PDAT_TST_CLR                    (~PDAT_TST_MSK)

/*LANE_HS_BIST[15:8]:ndat_tst*/
#define NDAT_TST_POS                    (8)
#define NDAT_TST_MSK                    (0x8<<NDAT_TST_POS)
#define NDAT_TST_CLR                    (~NDAT_TST_MSK)

/*LANE_HS_BIST[7:0]:lpx_tst*/
#define LPX_TST_POS                 (0)
#define LPX_TST_MSK                 (0x8<<LPX_TST_POS)
#define LPX_TST_CLR                 (~LPX_TST_MSK)

/*=========================================================*/
/*REGISTER:CONTENTION_DETECTOR_AND_STOPSTATE_DT ,Offset = 0x34*/

/*CONTENTION_DETECTOR_AND_STOPSTATE_DT [25:22]:reg_contention_detect_way*/
#define REG_CONTENTION_DETECT_WAY_POS                   (22)
#define REG_CONTENTION_DETECT_WAY_MSK                   (0x4<<REG_CONTENTION_DETECT_WAY_POS)
#define REG_CONTENTION_DETECT_WAY_CLR                   (~REG_CONTENTION_DETECT_WAY_MSK)

/*CONTENTION_DETECTOR_AND_STOPSTATE_DT [21:20]:reg_stopstate_detect_way_sel*/
#define REG_STOPSTATE_DETECT_WAY_SEL_POS                    (20)
#define REG_STOPSTATE_DETECT_WAY_SEL_MSK                    (0x2<<REG_STOPSTATE_DETECT_WAY_SEL_POS)
#define REG_STOPSTATE_DETECT_WAY_SEL_CLR                    (~REG_STOPSTATE_DETECT_WAY_SEL_MSK)

/*CONTENTION_DETECTOR_AND_STOPSTATE_DT [19:17]:reg_stopstate_deglitch_sel*/
#define REG_STOPSTATE_DEGLITCH_SEL_POS                  (17)
#define REG_STOPSTATE_DEGLITCH_SEL_MSK                  (0x3<<REG_STOPSTATE_DEGLITCH_SEL_POS)
#define REG_STOPSTATE_DEGLITCH_SEL_CLR                  (~REG_STOPSTATE_DEGLITCH_SEL_MSK)

/*CONTENTION_DETECTOR_AND_STOPSTATE_DT [16:14]:reg_tx_detect_delay_sel*/
#define REG_TX_DETECT_DELAY_SEL_POS                 (14)
#define REG_TX_DETECT_DELAY_SEL_MSK                 (0x3<<REG_TX_DETECT_DELAY_SEL_POS)
#define REG_TX_DETECT_DELAY_SEL_CLR                 (~REG_TX_DETECT_DELAY_SEL_MSK)

/*CONTENTION_DETECTOR_AND_STOPSTATE_DT [13:12]:reg_rx_detect_delay_sel*/
#define REG_RX_DETECT_DELAY_SEL_POS                 (12)
#define REG_RX_DETECT_DELAY_SEL_MSK                 (0x2<<REG_RX_DETECT_DELAY_SEL_POS)
#define REG_RX_DETECT_DELAY_SEL_CLR                 (~REG_RX_DETECT_DELAY_SEL_MSK)

/*CONTENTION_DETECTOR_AND_STOPSTATE_DT [11:8]:reg_detect_enable*/
#define REG_DETECT_ENABLE_POS                   (8)
#define REG_DETECT_ENABLE_MSK                   (0x4<<REG_DETECT_ENABLE_POS)
#define REG_DETECT_ENABLE_CLR                   (~REG_DETECT_ENABLE_MSK)

/*CONTENTION_DETECTOR_AND_STOPSTATE_DT [7:0]:reg_detect_idle_exit_time*/
#define REG_DETECT_IDLE_EXIT_TIME_POS                   (0)
#define REG_DETECT_IDLE_EXIT_TIME_MSK                   (0x8<<REG_DETECT_IDLE_EXIT_TIME_POS)
#define REG_DETECT_IDLE_EXIT_TIME_CLR                   (~REG_DETECT_IDLE_EXIT_TIME_MSK)

/*=========================================================*/
/*REGISTER:DEBUG_0,Offset = 0x38*/

/*DEBUG_0[7:0]:reg_dig_debug_sel*/
#define REG_DIG_DEBUG_SEL_POS                   (0)
#define REG_DIG_DEBUG_SEL_MSK                   (0x8<<REG_DIG_DEBUG_SEL_POS)
#define REG_DIG_DEBUG_SEL_CLR                   (~REG_DIG_DEBUG_SEL_MSK)

/*=========================================================*/
/*REGISTER:DEBUG_1,Offset = 0x3C*/

/*DEBUG_1[29:0]:debug_out*/
#define DEBUG_OUT_POS                   (0)
#define DEBUG_OUT_MSK                   (0x1E<<DEBUG_OUT_POS)
#define DEBUG_OUT_CLR                   (~DEBUG_OUT_MSK)

/*=========================================================*/
/*REGISTER: SSC0,Offset = 0x40*/

/* SSC0[13:8]:reg_oc_done_delay*/
#define REG_OC_DONE_DELAY_POS                   (8)
#define REG_OC_DONE_DELAY_MSK                   (0x6<<REG_OC_DONE_DELAY_POS)
#define REG_OC_DONE_DELAY_CLR                   (~REG_OC_DONE_DELAY_MSK)

/* SSC0[7:6]:reg_hs_oc_stop_diff*/
#define REG_HS_OC_STOP_DIFF_POS                 (6)
#define REG_HS_OC_STOP_DIFF_MSK                 (0x2<<REG_HS_OC_STOP_DIFF_POS)
#define REG_HS_OC_STOP_DIFF_CLR                 (~REG_HS_OC_STOP_DIFF_MSK)

/* SSC0[5:4]:reg_sel_oc_mode*/
#define REG_SEL_OC_MODE_POS                 (4)
#define REG_SEL_OC_MODE_MSK                 (0x2<<REG_SEL_OC_MODE_POS)
#define REG_SEL_OC_MODE_CLR                 (~REG_SEL_OC_MODE_MSK)

/* SSC0[3:3]:dpi_ssc_flag_init*/
#define DPI_SSC_FLAG_INIT_POS                   (3)
#define DPI_SSC_FLAG_INIT_MSK                   (0x1<<DPI_SSC_FLAG_INIT_POS)
#define DPI_SSC_FLAG_INIT_CLR                   (~DPI_SSC_FLAG_INIT_MSK)

/* SSC0[2:2]:reg_sdm_order*/
#define REG_SDM_ORDER_POS                   (2)
#define REG_SDM_ORDER_MSK                   (0x1<<REG_SDM_ORDER_POS)
#define REG_SDM_ORDER_CLR                   (~REG_SDM_ORDER_MSK)

/* SSC0[1:1]:en_ssc*/
#define EN_SSC_POS                  (1)
#define EN_SSC_MSK                  (0x1<<EN_SSC_POS)
#define EN_SSC_CLR                  (~EN_SSC_MSK)

/* SSC0[0:0]:reg_bypass_pi*/
#define REG_BYPASS_PI_POS                   (0)
#define REG_BYPASS_PI_MSK                   (0x1<<REG_BYPASS_PI_POS)
#define REG_BYPASS_PI_CLR                   (~REG_BYPASS_PI_MSK)

/*=========================================================*/
/*REGISTER:SSC1,Offset = 0x44*/

/*SSC1[23:23]:reg_gran_auto_rst*/
#define REG_GRAN_AUTO_RST_POS                   (23)
#define REG_GRAN_AUTO_RST_MSK                   (0x1<<REG_GRAN_AUTO_RST_POS)
#define REG_GRAN_AUTO_RST_CLR                   (~REG_GRAN_AUTO_RST_MSK)

/*SSC1[22:20]:reg_dot_gran*/
#define REG_DOT_GRAN_POS                    (20)
#define REG_DOT_GRAN_MSK                    (0x3<<REG_DOT_GRAN_POS)
#define REG_DOT_GRAN_CLR                    (~REG_DOT_GRAN_MSK)

/*SSC1[18:0]:gran_est*/
#define GRAN_EST_POS                    (0)
#define GRAN_EST_MSK                    (0x13<<GRAN_EST_POS)
#define GRAN_EST_CLR                    (~GRAN_EST_MSK)

/*=========================================================*/
/*REGISTER:SSC2,Offset = 0x48*/

/*SSC2[26:16]:fcode_t_scpu*/
#define FCODE_T_SCPU_POS                    (16)
#define FCODE_T_SCPU_MSK                    (0xB<<FCODE_T_SCPU_POS)
#define FCODE_T_SCPU_CLR                    (~FCODE_T_SCPU_MSK)

/*SSC2[10:0]:fcode_ssc*/
#define FCODE_SSC_POS                   (0)
#define FCODE_SSC_MSK                   (0xB<<FCODE_SSC_POS)
#define FCODE_SSC_CLR                   (~FCODE_SSC_MSK)

/*=========================================================*/
/*REGISTER:SSC3,Offset = 0x4C*/

/*SSC3[26:26]:oc_en*/
#define OC_EN_POS                   (26)
#define OC_EN_MSK                   (0x1<<OC_EN_POS)
#define OC_EN_CLR                   (~OC_EN_MSK)

/*SSC3[25:16]:reg_oc_step_set*/
#define REG_OC_STEP_SET_POS                 (16)
#define REG_OC_STEP_SET_MSK                 (0xA<<REG_OC_STEP_SET_POS)
#define REG_OC_STEP_SET_CLR                 (~REG_OC_STEP_SET_MSK)

/*SSC3[15:8]:ncode_t_scpu*/
#define NCODE_T_SCPU_POS                    (8)
#define NCODE_T_SCPU_MSK                    (0x8<<NCODE_T_SCPU_POS)
#define NCODE_T_SCPU_CLR                    (~NCODE_T_SCPU_MSK)

/*SSC3[7:0]:ncode_ssc*/
#define NCODE_SSC_POS                   (0)
#define NCODE_SSC_MSK                   (0x8<<NCODE_SSC_POS)
#define NCODE_SSC_CLR                   (~NCODE_SSC_MSK)

/*=========================================================*/
/*REGISTER:WATCHDOG,Offset = 0x50*/

/*WATCHDOG[13:12]:reg_time2_rst_width*/
#define REG_TIME2_RST_WIDTH_POS                 (12)
#define REG_TIME2_RST_WIDTH_MSK                 (0x2<<REG_TIME2_RST_WIDTH_POS)
#define REG_TIME2_RST_WIDTH_CLR                 (~REG_TIME2_RST_WIDTH_MSK)

/*WATCHDOG[11:10]:dfs_time_rdy_ckout*/
#define DFS_TIME_RDY_CKOUT_POS                  (10)
#define DFS_TIME_RDY_CKOUT_MSK                  (0x2<<DFS_TIME_RDY_CKOUT_POS)
#define DFS_TIME_RDY_CKOUT_CLR                  (~DFS_TIME_RDY_CKOUT_MSK)

/*WATCHDOG[9:8]:reg_time_rdy_ckout*/
#define REG_TIME_RDY_CKOUT_POS                  (8)
#define REG_TIME_RDY_CKOUT_MSK                  (0x2<<REG_TIME_RDY_CKOUT_POS)
#define REG_TIME_RDY_CKOUT_CLR                  (~REG_TIME_RDY_CKOUT_MSK)

/*WATCHDOG[6:4]:reg_time0_ck*/
#define REG_TIME0_CK_POS                    (4)
#define REG_TIME0_CK_MSK                    (0x3<<REG_TIME0_CK_POS)
#define REG_TIME0_CK_CLR                    (~REG_TIME0_CK_MSK)

/*WATCHDOG[3:3]:rstb*/
#define RSTB_POS                    (3)
#define RSTB_MSK                    (0x1<<RSTB_POS)
#define RSTB_CLR                    (~RSTB_MSK)

/*WATCHDOG[2:1]:reg_f390k*/
#define REG_F390K_POS                   (1)
#define REG_F390K_MSK                   (0x2<<REG_F390K_POS)
#define REG_F390K_CLR                   (~REG_F390K_MSK)

/*WATCHDOG[0:0]:reg_en_wdog*/
#define REG_EN_WDOG_POS                 (0)
#define REG_EN_WDOG_MSK                 (0x1<<REG_EN_WDOG_POS)
#define REG_EN_WDOG_CLR                 (~REG_EN_WDOG_MSK)

/*=========================================================*/
/*REGISTER:RX_ESCAPE_READ_0,Offset = 0x54*/

/*RX_ESCAPE_READ_0[17:17]:lane1_esc_vld*/
#define LANE1_ESC_VLD_POS                   (17)
#define LANE1_ESC_VLD_MSK                   (0x1<<LANE1_ESC_VLD_POS)
#define LANE1_ESC_VLD_CLR                   (~LANE1_ESC_VLD_MSK)

/*RX_ESCAPE_READ_0[16:16]:lane0_esc_vld*/
#define LANE0_ESC_VLD_POS                   (16)
#define LANE0_ESC_VLD_MSK                   (0x1<<LANE0_ESC_VLD_POS)
#define LANE0_ESC_VLD_CLR                   (~LANE0_ESC_VLD_MSK)

/*RX_ESCAPE_READ_0[15:8]:lane1_esc_cmd*/
#define LANE1_ESC_CMD_POS                   (8)
#define LANE1_ESC_CMD_MSK                   (0x8<<LANE1_ESC_CMD_POS)
#define LANE1_ESC_CMD_CLR                   (~LANE1_ESC_CMD_MSK)

/*RX_ESCAPE_READ_0[7:0]:lane0_esc_cmd*/
#define LANE0_ESC_CMD_POS                   (0)
#define LANE0_ESC_CMD_MSK                   (0x8<<LANE0_ESC_CMD_POS)
#define LANE0_ESC_CMD_CLR                   (~LANE0_ESC_CMD_MSK)

/*=========================================================*/
/*REGISTER:RX_ESCAPE_READ_1,Offset = 0x58*/

/*RX_ESCAPE_READ_1[17:17]:lane3_esc_vld*/
#define LANE3_ESC_VLD_POS                   (17)
#define LANE3_ESC_VLD_MSK                   (0x1<<LANE3_ESC_VLD_POS)
#define LANE3_ESC_VLD_CLR                   (~LANE3_ESC_VLD_MSK)

/*RX_ESCAPE_READ_1[16:16]:lane2_esc_vld*/
#define LANE2_ESC_VLD_POS                   (16)
#define LANE2_ESC_VLD_MSK                   (0x1<<LANE2_ESC_VLD_POS)
#define LANE2_ESC_VLD_CLR                   (~LANE2_ESC_VLD_MSK)

/*RX_ESCAPE_READ_1[15:8]:lane3_esc_cmd*/
#define LANE3_ESC_CMD_POS                   (8)
#define LANE3_ESC_CMD_MSK                   (0x8<<LANE3_ESC_CMD_POS)
#define LANE3_ESC_CMD_CLR                   (~LANE3_ESC_CMD_MSK)

/*RX_ESCAPE_READ_1[7:0]:lane2_esc_cmd*/
#define LANE2_ESC_CMD_POS                   (0)
#define LANE2_ESC_CMD_MSK                   (0x8<<LANE2_ESC_CMD_POS)
#define LANE2_ESC_CMD_CLR                   (~LANE2_ESC_CMD_MSK)

/*=========================================================*/
/*REGISTER:DEGLITCH_LP,Offset = 0x5C*/

/*DEGLITCH_LP[16:16]:reg_lp_switch_deglitch*/
#define REG_LP_SWITCH_DEGLITCH_POS                  (16)
#define REG_LP_SWITCH_DEGLITCH_MSK                  (0x1<<REG_LP_SWITCH_DEGLITCH_POS)
#define REG_LP_SWITCH_DEGLITCH_CLR                  (~REG_LP_SWITCH_DEGLITCH_MSK)

/*DEGLITCH_LP[15:0]:reg_ph_sel*/
#define REG_PH_SEL_POS                  (0)
#define REG_PH_SEL_MSK                  (0x10<<REG_PH_SEL_POS)
#define REG_PH_SEL_CLR                  (~REG_PH_SEL_MSK)

/*=========================================================*/
/*REGISTER:LP_BIST_1,Offset = 0x60*/
/*LP_BIST_1[31:24]:packet_cnt_lane1*/
#define PACKET_CNT_LANE1_POS                    (24)
#define PACKET_CNT_LANE1_MSK                    (0x8<<PACKET_CNT_LANE1_POS)
#define PACKET_CNT_LANE1_CLR                    (~PACKET_CNT_LANE1_MSK)

/*LP_BIST_1[23:16]:err_cnt_lane1*/
#define ERR_CNT_LANE1_POS                   (16)
#define ERR_CNT_LANE1_MSK                   (0x8<<ERR_CNT_LANE1_POS)
#define ERR_CNT_LANE1_CLR                   (~ERR_CNT_LANE1_MSK)

/*LP_BIST_1[15:8]:packet_cnt_lane0*/
#define PACKET_CNT_LANE0_POS                    (8)
#define PACKET_CNT_LANE0_MSK                    (0x8<<PACKET_CNT_LANE0_POS)
#define PACKET_CNT_LANE0_CLR                    (~PACKET_CNT_LANE0_MSK)

/*LP_BIST_1[7:0]:err_cnt_lane0*/
#define ERR_CNT_LANE0_POS                   (0)
#define ERR_CNT_LANE0_MSK                   (0x8<<ERR_CNT_LANE0_POS)
#define ERR_CNT_LANE0_CLR                   (~ERR_CNT_LANE0_MSK)

/*=========================================================*/
/*REGISTER:LP_BIST_2,Offset = 0x64*/
/*LP_BIST_2[31:24]:packet_cnt_lane3*/
#define PACKET_CNT_LANE3_POS                    (24)
#define PACKET_CNT_LANE3_MSK                    (0x8<<PACKET_CNT_LANE3_POS)
#define PACKET_CNT_LANE3_CLR                    (~PACKET_CNT_LANE3_MSK)

/*LP_BIST_2[23:16]:err_cnt_lane3*/
#define ERR_CNT_LANE3_POS                   (16)
#define ERR_CNT_LANE3_MSK                   (0x8<<ERR_CNT_LANE3_POS)
#define ERR_CNT_LANE3_CLR                   (~ERR_CNT_LANE3_MSK)

/*LP_BIST_2[15:8]:packet_cnt_lane2*/
#define PACKET_CNT_LANE2_POS                    (8)
#define PACKET_CNT_LANE2_MSK                    (0x8<<PACKET_CNT_LANE2_POS)
#define PACKET_CNT_LANE2_CLR                    (~PACKET_CNT_LANE2_MSK)

/*LP_BIST_2[7:0]:err_cnt_lane2*/
#define ERR_CNT_LANE2_POS                   (0)
#define ERR_CNT_LANE2_MSK                   (0x8<<ERR_CNT_LANE2_POS)
#define ERR_CNT_LANE2_CLR                   (~ERR_CNT_LANE2_MSK)

/*=========================================================*/
/*REGISTER:LANE_PN_SWAP_1,Offset = 0x68*/

/*LANE_PN_SWAP_1[26:26]:reg_wkup_inverse*/
#define REG_WKUP_INVERSE_POS                    (26)
#define REG_WKUP_INVERSE_MSK                    (0x1<<REG_WKUP_INVERSE_POS)
#define REG_WKUP_INVERSE_CLR                    (~REG_WKUP_INVERSE_MSK)

/*LANE_PN_SWAP_1[25:25]:reg_enhs_inverse*/
#define REG_ENHS_INVERSE_POS                    (25)
#define REG_ENHS_INVERSE_MSK                    (0x1<<REG_ENHS_INVERSE_POS)
#define REG_ENHS_INVERSE_CLR                    (~REG_ENHS_INVERSE_MSK)

/*LANE_PN_SWAP_1[24:24]:reg_lane4_pn_swap*/
#define REG_LANE4_PN_SWAP_POS                   (24)
#define REG_LANE4_PN_SWAP_MSK                   (0x1<<REG_LANE4_PN_SWAP_POS)
#define REG_LANE4_PN_SWAP_CLR                   (~REG_LANE4_PN_SWAP_MSK)

/*LANE_PN_SWAP_1[23:23]:reg_lane3_pn_swap*/
#define REG_LANE3_PN_SWAP_POS                   (23)
#define REG_LANE3_PN_SWAP_MSK                   (0x1<<REG_LANE3_PN_SWAP_POS)
#define REG_LANE3_PN_SWAP_CLR                   (~REG_LANE3_PN_SWAP_MSK)

/*LANE_PN_SWAP_1[22:22]:reg_lane2_pn_swap*/
#define REG_LANE2_PN_SWAP_POS                   (22)
#define REG_LANE2_PN_SWAP_MSK                   (0x1<<REG_LANE2_PN_SWAP_POS)
#define REG_LANE2_PN_SWAP_CLR                   (~REG_LANE2_PN_SWAP_MSK)

/*LANE_PN_SWAP_1[21:21]:reg_lane1_pn_swap*/
#define REG_LANE1_PN_SWAP_POS                   (21)
#define REG_LANE1_PN_SWAP_MSK                   (0x1<<REG_LANE1_PN_SWAP_POS)
#define REG_LANE1_PN_SWAP_CLR                   (~REG_LANE1_PN_SWAP_MSK)

/*LANE_PN_SWAP_1[20:20]:reg_lane0_pn_swap*/
#define REG_LANE0_PN_SWAP_POS                   (20)
#define REG_LANE0_PN_SWAP_MSK                   (0x1<<REG_LANE0_PN_SWAP_POS)
#define REG_LANE0_PN_SWAP_CLR                   (~REG_LANE0_PN_SWAP_MSK)

/*LANE_PN_SWAP_1[18:16]:reg_tx_lane4_sel*/
#define REG_TX_LANE4_SEL_POS                    (16)
#define REG_TX_LANE4_SEL_MSK                    (0x3<<REG_TX_LANE4_SEL_POS)
#define REG_TX_LANE4_SEL_CLR                    (~REG_TX_LANE4_SEL_MSK)

/*LANE_PN_SWAP_1[14:12]:reg_tx_lane3_sel*/
#define REG_TX_LANE3_SEL_POS                    (12)
#define REG_TX_LANE3_SEL_MSK                    (0x3<<REG_TX_LANE3_SEL_POS)
#define REG_TX_LANE3_SEL_CLR                    (~REG_TX_LANE3_SEL_MSK)

/*LANE_PN_SWAP_1[10:8]:reg_tx_lane2_sel*/
#define REG_TX_LANE2_SEL_POS                    (8)
#define REG_TX_LANE2_SEL_MSK                    (0x3<<REG_TX_LANE2_SEL_POS)
#define REG_TX_LANE2_SEL_CLR                    (~REG_TX_LANE2_SEL_MSK)

/*LANE_PN_SWAP_1[6:4]:reg_tx_lane1_sel*/
#define REG_TX_LANE1_SEL_POS                    (4)
#define REG_TX_LANE1_SEL_MSK                    (0x3<<REG_TX_LANE1_SEL_POS)
#define REG_TX_LANE1_SEL_CLR                    (~REG_TX_LANE1_SEL_MSK)

/*LANE_PN_SWAP_1[2:0]:reg_tx_lane0_sel*/
#define REG_TX_LANE0_SEL_POS                    (0)
#define REG_TX_LANE0_SEL_MSK                    (0x3<<REG_TX_LANE0_SEL_POS)
#define REG_TX_LANE0_SEL_CLR                    (~REG_TX_LANE0_SEL_MSK)

/*=========================================================*/
/*REGISTER:LANE_PN_SWAP_2,Offset = 0x6C*/

/*LANE_PN_SWAP_2[18:16]:reg_rx_lane4_sel*/
#define REG_RX_LANE4_SEL_POS                    (16)
#define REG_RX_LANE4_SEL_MSK                    (0x3<<REG_RX_LANE4_SEL_POS)
#define REG_RX_LANE4_SEL_CLR                    (~REG_RX_LANE4_SEL_MSK)

/*LANE_PN_SWAP_2[14:12]:reg_rx_lane3_sel*/
#define REG_RX_LANE3_SEL_POS                    (12)
#define REG_RX_LANE3_SEL_MSK                    (0x3<<REG_RX_LANE3_SEL_POS)
#define REG_RX_LANE3_SEL_CLR                    (~REG_RX_LANE3_SEL_MSK)

/*LANE_PN_SWAP_2[10:8]:reg_rx_lane2_sel*/
#define REG_RX_LANE2_SEL_POS                    (8)
#define REG_RX_LANE2_SEL_MSK                    (0x3<<REG_RX_LANE2_SEL_POS)
#define REG_RX_LANE2_SEL_CLR                    (~REG_RX_LANE2_SEL_MSK)

/*LANE_PN_SWAP_2[6:4]:reg_rx_lane1_sel*/
#define REG_RX_LANE1_SEL_POS                    (4)
#define REG_RX_LANE1_SEL_MSK                    (0x3<<REG_RX_LANE1_SEL_POS)
#define REG_RX_LANE1_SEL_CLR                    (~REG_RX_LANE1_SEL_MSK)

/*LANE_PN_SWAP_2[2:0]:reg_rx_lane0_sel*/
#define REG_RX_LANE0_SEL_POS                    (0)
#define REG_RX_LANE0_SEL_MSK                    (0x3<<REG_RX_LANE0_SEL_POS)
#define REG_RX_LANE0_SEL_CLR                    (~REG_RX_LANE0_SEL_MSK)

/*=========================================================*/
/*REGISTER:MPLL,Offset = 0xC0*/

/*MPLL[28:28]:reg_hsmode_ldo_en*/
#define REG_HSMODE_LDO_EN_POS                   (28)
#define REG_HSMODE_LDO_EN_MSK                   (0x1<<REG_HSMODE_LDO_EN_POS)
#define REG_HSMODE_LDO_EN_CLR                   (~REG_HSMODE_LDO_EN_MSK)

/*MPLL[27:27]:reg_bypass_8x*/
#define REG_BYPASS_8X_POS                   (27)
#define REG_BYPASS_8X_MSK                   (0x1<<REG_BYPASS_8X_POS)
#define REG_BYPASS_8X_CLR                   (~REG_BYPASS_8X_MSK)

/*MPLL[26:25]:reg_txtst_sel*/
#define REG_TXTST_SEL_POS                   (25)
#define REG_TXTST_SEL_MSK                   (0x2<<REG_TXTST_SEL_POS)
#define REG_TXTST_SEL_CLR                   (~REG_TXTST_SEL_MSK)

/*MPLL[24:24]:reg_txtst_en*/
#define REG_TXTST_EN_POS                    (24)
#define REG_TXTST_EN_MSK                    (0x1<<REG_TXTST_EN_POS)
#define REG_TXTST_EN_CLR                    (~REG_TXTST_EN_MSK)

/*MPLL[18:18]:reg_mpll_cco_band*/
#define REG_MPLL_CCO_BAND_POS                   (18)
#define REG_MPLL_CCO_BAND_MSK                   (0x1<<REG_MPLL_CCO_BAND_POS)
#define REG_MPLL_CCO_BAND_CLR                   (~REG_MPLL_CCO_BAND_MSK)

/*MPLL[17:17]:reg_en_tx_lp_ldo*/
#define REG_EN_TX_LP_LDO_POS                    (17)
#define REG_EN_TX_LP_LDO_MSK                    (0x1<<REG_EN_TX_LP_LDO_POS)
#define REG_EN_TX_LP_LDO_CLR                    (~REG_EN_TX_LP_LDO_MSK)

/*MPLL[16:16]:reg_cmu_pow*/
#define REG_CMU_POW_POS                 (16)
#define REG_CMU_POW_MSK                 (0x1<<REG_CMU_POW_POS)
#define REG_CMU_POW_CLR                 (~REG_CMU_POW_MSK)

/*MPLL[15:15]:reg_mpll_testen*/
#define REG_MPLL_TESTEN_POS                 (15)
#define REG_MPLL_TESTEN_MSK                 (0x1<<REG_MPLL_TESTEN_POS)
#define REG_MPLL_TESTEN_CLR                 (~REG_MPLL_TESTEN_MSK)

/*MPLL[14:13]:reg_mpll_pi_isel*/
#define REG_MPLL_PI_ISEL_POS                    (13)
#define REG_MPLL_PI_ISEL_MSK                    (0x2<<REG_MPLL_PI_ISEL_POS)
#define REG_MPLL_PI_ISEL_CLR                    (~REG_MPLL_PI_ISEL_MSK)

/*MPLL[12:12]:reg_mpll_pi_c*/
#define REG_MPLL_PI_C_POS                   (12)
#define REG_MPLL_PI_C_MSK                   (0x1<<REG_MPLL_PI_C_POS)
#define REG_MPLL_PI_C_CLR                   (~REG_MPLL_PI_C_MSK)

/*MPLL[11:11]:reg_mpll_pi_bypass*/
#define REG_MPLL_PI_BYPASS_POS                  (11)
#define REG_MPLL_PI_BYPASS_MSK                  (0x1<<REG_MPLL_PI_BYPASS_POS)
#define REG_MPLL_PI_BYPASS_CLR                  (~REG_MPLL_PI_BYPASS_MSK)

/*MPLL[10:9]:reg_mpll_pdiv*/
#define REG_MPLL_PDIV_POS                   (9)
#define REG_MPLL_PDIV_MSK                   (0x2<<REG_MPLL_PDIV_POS)
#define REG_MPLL_PDIV_CLR                   (~REG_MPLL_PDIV_MSK)

/*MPLL[8:6]:reg_mpll_lpf_sr*/
#define REG_MPLL_LPF_SR_POS                 (6)
#define REG_MPLL_LPF_SR_MSK                 (0x3<<REG_MPLL_LPF_SR_POS)
#define REG_MPLL_LPF_SR_CLR                 (~REG_MPLL_LPF_SR_MSK)

/*MPLL[5:4]:reg_mpll_ldo_vref*/
#define REG_MPLL_LDO_VREF_POS                   (4)
#define REG_MPLL_LDO_VREF_MSK                   (0x2<<REG_MPLL_LDO_VREF_POS)
#define REG_MPLL_LDO_VREF_CLR                   (~REG_MPLL_LDO_VREF_MSK)

/*MPLL[3:2]:reg_mpll_icp*/
#define REG_MPLL_ICP_POS                    (2)
#define REG_MPLL_ICP_MSK                    (0x2<<REG_MPLL_ICP_POS)
#define REG_MPLL_ICP_CLR                    (~REG_MPLL_ICP_MSK)

/*MPLL[1:1]:reg_mpll_div_cyc*/
#define REG_MPLL_DIV_CYC_POS                    (1)
#define REG_MPLL_DIV_CYC_MSK                    (0x1<<REG_MPLL_DIV_CYC_POS)
#define REG_MPLL_DIV_CYC_CLR                    (~REG_MPLL_DIV_CYC_MSK)

/*MPLL[0:0]:reg_mpll_big_cco*/
#define REG_MPLL_BIG_CCO_POS                    (0)
#define REG_MPLL_BIG_CCO_MSK                    (0x1<<REG_MPLL_BIG_CCO_POS)
#define REG_MPLL_BIG_CCO_CLR                    (~REG_MPLL_BIG_CCO_MSK)

/*=========================================================*/
/*REGISTER:LF,Offset = 0xC4*/

/*LF[29:27]:reg_ck8x_lag*/
#define REG_CK8X_LAG_POS                    (27)
#define REG_CK8X_LAG_MSK                    (0x3<<REG_CK8X_LAG_POS)
#define REG_CK8X_LAG_CLR                    (~REG_CK8X_LAG_MSK)

/*LF[26:26]:reg_ck8x_inv*/
#define REG_CK8X_INV_POS                    (26)
#define REG_CK8X_INV_MSK                    (0x1<<REG_CK8X_INV_POS)
#define REG_CK8X_INV_CLR                    (~REG_CK8X_INV_MSK)

/*LF[25:25]:reg_ck1x_inv*/
#define REG_CK1X_INV_POS                    (25)
#define REG_CK1X_INV_MSK                    (0x1<<REG_CK1X_INV_POS)
#define REG_CK1X_INV_CLR                    (~REG_CK1X_INV_MSK)

/*LF[14:10]:reg_data2_dly*/
#define REG_DATA2_DLY_POS                   (10)
#define REG_DATA2_DLY_MSK                   (0x5<<REG_DATA2_DLY_POS)
#define REG_DATA2_DLY_CLR                   (~REG_DATA2_DLY_MSK)

/*LF[9:5]:reg_data1_dly*/
#define REG_DATA1_DLY_POS                   (5)
#define REG_DATA1_DLY_MSK                   (0x5<<REG_DATA1_DLY_POS)
#define REG_DATA1_DLY_CLR                   (~REG_DATA1_DLY_MSK)

/*LF[4:0]:reg_data0_dly*/
#define REG_DATA0_DLY_POS                   (0)
#define REG_DATA0_DLY_MSK                   (0x5<<REG_DATA0_DLY_POS)
#define REG_DATA0_DLY_CLR                   (~REG_DATA0_DLY_MSK)

/*=========================================================*/
/*REGISTER:TXF,Offset = 0xC8*/

/*TXF[27:26]:reg_ck8x_phs*/
#define REG_CK8X_PHS_POS                    (26)
#define REG_CK8X_PHS_MSK                    (0x2<<REG_CK8X_PHS_POS)
#define REG_CK8X_PHS_CLR                    (~REG_CK8X_PHS_MSK)

/*TXF[25:22]:reg_z0_tx*/
#define REG_Z0_TX_POS                   (22)
#define REG_Z0_TX_MSK                   (0x4<<REG_Z0_TX_POS)
#define REG_Z0_TX_CLR                   (~REG_Z0_TX_MSK)

/*TXF[21:20]:reg_tx_preres_sel*/
#define REG_TX_PRERES_SEL_POS                   (20)
#define REG_TX_PRERES_SEL_MSK                   (0x2<<REG_TX_PRERES_SEL_POS)
#define REG_TX_PRERES_SEL_CLR                   (~REG_TX_PRERES_SEL_MSK)

/*TXF[19:18]:reg_tx_precur_sel*/
#define REG_TX_PRECUR_SEL_POS                   (18)
#define REG_TX_PRECUR_SEL_MSK                   (0x2<<REG_TX_PRECUR_SEL_POS)
#define REG_TX_PRECUR_SEL_CLR                   (~REG_TX_PRECUR_SEL_MSK)

/*TXF[17:16]:reg_sr_ctrl*/
#define REG_SR_CTRL_POS                 (16)
#define REG_SR_CTRL_MSK                 (0x2<<REG_SR_CTRL_POS)
#define REG_SR_CTRL_CLR                 (~REG_SR_CTRL_MSK)

/*TXF[15:15]:reg_range_mode*/
#define REG_RANGE_MODE_POS                  (15)
#define REG_RANGE_MODE_MSK                  (0x1<<REG_RANGE_MODE_POS)
#define REG_RANGE_MODE_CLR                  (~REG_RANGE_MODE_MSK)

/*TXF[14:14]:reg_overshoot_bp*/
#define REG_OVERSHOOT_BP_POS                    (14)
#define REG_OVERSHOOT_BP_MSK                    (0x1<<REG_OVERSHOOT_BP_POS)
#define REG_OVERSHOOT_BP_CLR                    (~REG_OVERSHOOT_BP_MSK)

/*TXF[13:12]:reg_lp_drv*/
#define REG_LP_DRV_POS                  (12)
#define REG_LP_DRV_MSK                  (0x2<<REG_LP_DRV_POS)
#define REG_LP_DRV_CLR                  (~REG_LP_DRV_MSK)

/*TXF[11:9]:reg_lpldo_lvl*/
#define REG_LPLDO_LVL_POS                   (9)
#define REG_LPLDO_LVL_MSK                   (0x3<<REG_LPLDO_LVL_POS)
#define REG_LPLDO_LVL_CLR                   (~REG_LPLDO_LVL_MSK)

/*TXF[8:8]:reg_ib_en*/
#define REG_IB_EN_POS                   (8)
#define REG_IB_EN_MSK                   (0x1<<REG_IB_EN_POS)
#define REG_IB_EN_CLR                   (~REG_IB_EN_MSK)

/*TXF[7:7]:reg_en_lv_ldo*/
#define REG_EN_LV_LDO_POS                   (7)
#define REG_EN_LV_LDO_MSK                   (0x1<<REG_EN_LV_LDO_POS)
#define REG_EN_LV_LDO_CLR                   (~REG_EN_LV_LDO_MSK)

/*TXF[6:4]:reg_emp*/
#define REG_EMP_POS                 (4)
#define REG_EMP_MSK                 (0x3<<REG_EMP_POS)
#define REG_EMP_CLR                 (~REG_EMP_MSK)

/*TXF[3:3]:reg_deemp_en*/
#define REG_DEEMP_EN_POS                    (3)
#define REG_DEEMP_EN_MSK                    (0x1<<REG_DEEMP_EN_POS)
#define REG_DEEMP_EN_CLR                    (~REG_DEEMP_EN_MSK)

/*TXF[2:0]:reg_amp*/
#define REG_AMP_POS                 (0)
#define REG_AMP_MSK                 (0x3<<REG_AMP_POS)
#define REG_AMP_CLR                 (~REG_AMP_MSK)

/*=========================================================*/
/*REGISTER:DF,Offset = 0xCC*/

/*DF[25:25]:reg_pwct*/
#define REG_PWCT_POS                    (25)
#define REG_PWCT_MSK                    (0x1<<REG_PWCT_POS)
#define REG_PWCT_CLR                    (~REG_PWCT_MSK)

/*DF[24:24]:reg_mbias_en*/
#define REG_MBIAS_EN_POS                    (24)
#define REG_MBIAS_EN_MSK                    (0x1<<REG_MBIAS_EN_POS)
#define REG_MBIAS_EN_CLR                    (~REG_MBIAS_EN_MSK)

/*DF[23:21]:reg_lprx_vth_sel*/
#define REG_LPRX_VTH_SEL_POS                    (21)
#define REG_LPRX_VTH_SEL_MSK                    (0x3<<REG_LPRX_VTH_SEL_POS)
#define REG_LPRX_VTH_SEL_CLR                    (~REG_LPRX_VTH_SEL_MSK)

/*DF[20:18]:reg_lpcd_levth_sel*/
#define REG_LPCD_LEVTH_SEL_POS                  (18)
#define REG_LPCD_LEVTH_SEL_MSK                  (0x3<<REG_LPCD_LEVTH_SEL_POS)
#define REG_LPCD_LEVTH_SEL_CLR                  (~REG_LPCD_LEVTH_SEL_MSK)

/*DF[17:15]:reg_lpcd_hevth_sel*/
#define REG_LPCD_HEVTH_SEL_POS                  (15)
#define REG_LPCD_HEVTH_SEL_MSK                  (0x3<<REG_LPCD_HEVTH_SEL_POS)
#define REG_LPCD_HEVTH_SEL_CLR                  (~REG_LPCD_HEVTH_SEL_MSK)

/*DF[12:10]:reg_en_lprx*/
#define REG_EN_LPRX_POS                 (10)
#define REG_EN_LPRX_MSK                 (0x3<<REG_EN_LPRX_POS)
#define REG_EN_LPRX_CLR                 (~REG_EN_LPRX_MSK)

/*DF[7:5]:reg_en_lpcd*/
#define REG_EN_LPCD_POS                 (5)
#define REG_EN_LPCD_MSK                 (0x3<<REG_EN_LPCD_POS)
#define REG_EN_LPCD_CLR                 (~REG_EN_LPCD_MSK)

/*DF[4:4]:reg_postdiv4*/
#define REG_POSTDIV4_POS                    (4)
#define REG_POSTDIV4_MSK                    (0x1<<REG_POSTDIV4_POS)
#define REG_POSTDIV4_CLR                    (~REG_POSTDIV4_MSK)

/*DF[3:3]:reg_postdiv3*/
#define REG_POSTDIV3_POS                    (3)
#define REG_POSTDIV3_MSK                    (0x1<<REG_POSTDIV3_POS)
#define REG_POSTDIV3_CLR                    (~REG_POSTDIV3_MSK)

/*DF[2:2]:reg_postdiv2*/
#define REG_POSTDIV2_POS                    (2)
#define REG_POSTDIV2_MSK                    (0x1<<REG_POSTDIV2_POS)
#define REG_POSTDIV2_CLR                    (~REG_POSTDIV2_MSK)

/*DF[1:1]:reg_en_all_div4*/
#define REG_EN_ALL_DIV4_POS                 (1)
#define REG_EN_ALL_DIV4_MSK                 (0x1<<REG_EN_ALL_DIV4_POS)
#define REG_EN_ALL_DIV4_CLR                 (~REG_EN_ALL_DIV4_MSK)

/*DF[0:0]:reg_en_all_div2*/
#define REG_EN_ALL_DIV2_POS                 (0)
#define REG_EN_ALL_DIV2_MSK                 (0x1<<REG_EN_ALL_DIV2_POS)
#define REG_EN_ALL_DIV2_CLR                 (~REG_EN_ALL_DIV2_MSK)




