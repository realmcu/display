/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RTL_JPU_REG_H
#define RTL_JPU_REG_H

#ifdef __arm__
#include "address_map.h"
#include "rtl876x.h"
#endif


#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

/*============================================================================*
 *                         JPU Defines
 *============================================================================*/
#define JPU_REG_BASE                  0x4002A000UL
#define JPU_WRAPPER_REG_BASE          0x4002A800UL


#ifdef __WIN32
#define __IO volatile
#define __O  volatile
#define   __I     volatile const

typedef unsigned int  uint32_t;
typedef unsigned char uint8_t;

#define true 1
#define false 0

#endif
/*============================================================================*
 *                         JPU Registers Memory Map
 *============================================================================*/
typedef struct
{
    __IO uint32_t PIC_START;                                    /*< 0x000 */
    __IO uint32_t PIC_STATUS;                                   /*< 0x004 */
    __I  uint32_t PIC_ERRMB;                                    /*< 0x008 */
    __O  uint32_t PIC_SETMB;                                    /*< 0x00C */
    __IO uint32_t PIC_CTRL;                                     /*< 0x010 */
    __IO uint32_t PIC_SIZE;                                     /*< 0x014 */
    __IO uint32_t MCU_INFO;                                     /*< 0x018 */
    __IO uint32_t ROT_INFO;                                     /*< 0x01C */
    __IO uint32_t SCL_INFO;                                     /*< 0x020 */
    __IO uint32_t IF_INFO;                                      /*< 0x024 */
    __IO uint32_t CLP_INFO;                                     /*< 0x028 */
    __IO uint32_t OP_INFO;                                      /*< 0x02C */
    __IO uint32_t DPB_CONFIG;                                   /*< 0x030 */
    __IO uint32_t DPB_BASE00;                                   /*< 0x034 */
    __IO uint32_t DPB_BASE01;                                   /*< 0x038 */
    __IO uint32_t DPB_BASE02;                                   /*< 0x03C */
    __IO uint32_t DPB_BASE10;                                   /*< 0x040 */
    __IO uint32_t DPB_BASE11;                                   /*< 0x044 */
    __IO uint32_t DPB_BASE12;                                   /*< 0x048 */
    __IO uint32_t DPB_BASE20;                                   /*< 0x04C */
    __IO uint32_t DPB_BASE21;                                   /*< 0x050 */
    __IO uint32_t DPB_BASE22;                                   /*< 0x054 */
    __IO uint32_t DPB_BASE30;                                   /*< 0x058 */
    __IO uint32_t DPB_BASE31;                                   /*< 0x05C */
    __IO uint32_t DPB_BASE32;                                   /*< 0x060 */
    __IO uint32_t DPB_YSTRIDE;                                  /*< 0x064 */
    __IO uint32_t DPB_CSTRIDE;                                  /*< 0x068 */
    __I  uint32_t WRESP_CHECK;                                  /*< 0x06C */
    __IO uint32_t CLP_BASE;                                     /*< 0x070 */
    __IO uint32_t CLP_SIZE;                                     /*< 0x074 */
    __I  uint32_t RSVD1[2];                                     /*< 0x078 */
    __IO uint32_t HUFF_CTRL;                                    /*< 0x080 */
    __IO uint32_t HUFF_ADDR;                                    /*< 0x084 */
    __IO uint32_t HUFF_DATA;                                    /*< 0x088 */
    __I  uint32_t RSVD2;                                        /*< 0x08C */
    __IO uint32_t QMAT_CTRL;                                    /*< 0x090 */
    __IO uint32_t QMAT_ADDR;                                    /*< 0x094 */
    __IO uint32_t QMAT_DATA;                                    /*< 0x098 */
    __I  uint32_t RSVD3;                                        /*< 0x09C */
    __IO uint32_t COEF_CTRL;                                    /*< 0x0A0 */
    __IO uint32_t COEF_ADDR;                                    /*< 0x0A4 */
    __IO uint32_t COEF_DATA;                                    /*< 0x0A8 */
    __I  uint32_t RSVD4;                                        /*< 0x0AC */
    __IO uint32_t RST_INTVAL;                                   /*< 0x0B0 */
    __IO uint32_t RST_INDEX;                                    /*< 0x0B4 */
    __IO uint32_t RST_COUNT;                                    /*< 0x0B8 */
    __I  uint32_t RSVD5;                                        /*< 0x0BC */
    __IO uint32_t INTR_MASK;                                    /*< 0x0C0 */
    __I  uint32_t RSVD6;                                        /*< 0x0C4 */
    __I  uint32_t CYCLE_INFO;                                   /*< 0x0C8 */
    __I  uint32_t RSVD7[9];
    __IO uint32_t DPCM_DIFF_Y;                                  /*< 0x0F0 */
    __IO uint32_t DPCM_DIFF_CB;                                 /*< 0x0F4 */
    __IO uint32_t DPCM_DIFF_CR;                                 /*< 0x0F8 */
    __IO uint32_t VERSION_INFO;                                 /*< 0x0FC */

    __IO uint32_t GBU_CTRL;                                     /*< 0x100 */
    __I  uint32_t GBU_PBIT_BUSY;                                /*< 0x104 */
    __I  uint32_t RSVD8[2];
    __IO uint32_t GBU_BT_PTR;                                   /*< 0x110 */
    __IO uint32_t GBU_WD_PTR;                                   /*< 0x114 */
    __IO uint32_t GBU_TT_CNT_L;                                 /*< 0x118 */
    __IO uint32_t GBU_TT_CNT_H;                                 /*< 0x11C */
    __O  uint32_t GBU_PBIT_08;                                  /*< 0x120 */
    __O  uint32_t GBU_PBIT_16;                                  /*< 0x124 */
    __O  uint32_t GBU_PBIT_24;                                  /*< 0x128 */
    __O  uint32_t GBU_PBIT_32;                                  /*< 0x12C */
    __I  uint32_t RSVD9[4];
    __IO uint32_t GBU_BBSR;                                     /*< 0x140 */
    __IO uint32_t GBU_BBER;                                     /*< 0x144 */
    __IO uint32_t GBU_BBIR;                                     /*< 0x148 */
    __IO uint32_t GBU_BBHR;                                     /*< 0x14C */
    __I  uint32_t RSVD10[2];
    __IO uint32_t GBU_BCNT;                                     /*< 0x158 */
    __I  uint32_t RSVD11;                                       /*< 0x15C */
    __IO uint32_t GBU_FF_RPTR;                                  /*< 0x160 */
    __IO uint32_t GBU_FF_WPTR;                                  /*< 0x164 */
    __I  uint32_t RSVD12[40];
    __IO uint32_t BBC_END_ADDR;                                 /*< 0x208 */
    __IO uint32_t BBC_WR_PTR;                                   /*< 0x20C */
    __IO uint32_t BBC_RD_PTR;                                   /*< 0x210 */
    __IO uint32_t BBC_EXT_ADDR;                                 /*< 0x214 */
    __IO uint32_t BBC_INT_ADDR;                                 /*< 0x218 */
    __IO uint32_t BBC_DATA_CNT;                                 /*< 0x21C */
    __IO uint32_t BBC_COMMAND;                                  /*< 0x220 */
    __I  uint32_t BBC_BUSY;                                     /*< 0x224 */
    __IO uint32_t BBC_CTRL;                                     /*< 0x228 */
    __IO uint32_t BBC_CUR_POS;                                  /*< 0x22C */
    __IO uint32_t BBC_BAS_ADDR;                                 /*< 0x230 */
    __IO uint32_t BBC_STRM_CTRL;                                /*< 0x234 */
    __IO uint32_t BBC_FLUSH_CMD;                                /*< 0x238 */
} JPU_TypeDef;

typedef struct
{
    __IO uint32_t RGB_PF;                                       /*< 0x00  */
    __IO uint32_t CONFIG;                                       /*< 0x04  */
    __I  uint32_t WRAP_DONE;                                    /*< 0x08  */
    __IO uint32_t INT_ENABLE;                                   /*< 0x0C  */
    __IO uint32_t INT_MASK;                                     /*< 0x10  */
    __I  uint32_t INT_RAW_STATUS;                               /*< 0x14  */
    __I  uint32_t INT_STATUS;                                   /*< 0x18  */
    __IO uint32_t INT_CLEAR;                                    /*< 0x1C  */
} JPU_WRAPPER_TypeDef;
/*============================================================================*
 *                         JPU Declaration
 *============================================================================*/

/*============================================================================*
 *                         JPU Private Types
 *============================================================================*/

/*============================================================================*
 *                         JPU Registers and Field Descriptions
 *============================================================================*/
/* 0x00
    0       R/W      start_codec                              0x0
    1       R/W      init_codec                               0x0
    2       R/W      stop_codec                               0x0
    3       R/W      start_partial_enc                        0x0
    31:4    R        reserved                                 0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        uint32_t start_codec: 1;
        uint32_t init_codec: 1;
        uint32_t stop_codec: 1;
        uint32_t start_partial_enc: 1;
        const uint32_t reserved_0: 28;
        } b;
    } JPU_PIC_START_TypeDef;



    /* 0x04
        0       R/W      jpg_done                                0x0
        1       R/W      jpg_err                                 0x0
        2       R/W      bbc_int                                 0x0
        3       R/W      jpg_pb_of                               0x0
        4       R/W      jpg_pb_0                                0x0
        5       R/W      jpg_pb_1                                0x0
        6       R/W      jpg_pb_2                                0x0
        7       R/W      jpg_pb_3                                0x0
        8       R/W      jpg_stop                                0x0
        31:9    R        reserved                                0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t jpg_done:  1;
                uint32_t jpg_err:   1;
                uint32_t bbc_int:   1;
                uint32_t jpg_pb_of: 1;
                uint32_t jpg_pb_0:  1;
                uint32_t jpg_pb_1:  1;
                uint32_t jpg_pb_2:  1;
                uint32_t jpg_pb_3:  1;
                uint32_t jpg_stop:  1;
                const uint32_t reserved_0: 23;
            } b;
        } JPU_PIC_STATUS_TypeDef;



    /* 0x08
        11:0     R        err_mcu_y                               0x0
        23:12    R        err_mcu_x                               0x0
        27:24    R        err_restart_idx                         0x0
        31:28    R        reserved                                0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t err_mcu_y: 12;
                const uint32_t err_mcu_x: 12;
                const uint32_t err_restart_idx: 4;
                const uint32_t reserved_0: 4;
            } b;
        } JPU_PIC_ERRMB_TypeDef;



    /* 0x0C
        11:0     W        mcu_y                               0x0
        15:12    R        reserved                            0x0
        27:16    W        mcu_x                               0x0
        31:28    R        reserved                            0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t mcu_y: 12;
                const uint32_t reserved_0: 4;
                uint32_t mcu_x: 12;
                const uint32_t reserved_1: 4;
            } b;
        } JPU_PIC_SETMB_TypeDef;



    /* 0x10
        1:0     R/W      op_mode                             0x0
        2       R/W      dma_wr_resp_en                      0x0
        3       R/W      enc_mode                            0x0
        4       R/W      tc_dir                              0x0
        5       R        reserved                            0x0
        6       R/W      usr_ht_en                           0x0
        9:7     R/W      dc_ht_idx                           0x0
        12:10   R/W      ac_ht_idx                           0x0
        31:13   R        reserved                            0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t op_mode: 2;
                uint32_t dma_wr_resp_en: 1;
                uint32_t enc_mode: 1;
                uint32_t tc_dir: 1;
                const uint32_t reserved_0: 1;
                uint32_t usr_ht_en: 1;
                uint32_t dc_ht_idx: 3;
                uint32_t ac_ht_idx: 3;
                const uint32_t reserved_1: 19;
            } b;
        } JPU_PIC_CTRL_TypeDef;



    /* 0x14
        15:0     R/W    pic_height                           0x0
        31:16    R/W    pic_width                            0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t pic_height: 16;
                uint32_t pic_width: 16;
            } b;
        } JPU_PIC_SIZE_TypeDef;



    /* 0x18
        1:0      R/W    cmp2_v_sf                 0x0
        3:2      R/W    cmp2_h_sf                 0x0
        5:4      R/W    cmp1_v_sf                 0x0
        7:6      R/W    cmp1_h_sf                 0x0
        9:8      R/W    cmp0_v_sf                 0x0
        11:10    R/W    cmp0_h_sf                 0x0
        14:12    R/W    cmp_num                   0x0
        19:16    R/W    block_num                 0x0
        31:20    R      reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t cmp2_v_sf: 2;
                uint32_t cmp2_h_sf: 2;
                uint32_t cmp1_v_sf: 2;
                uint32_t cmp1_h_sf: 2;
                uint32_t cmp0_v_sf: 2;
                uint32_t cmp0_h_sf: 2;
                uint32_t cmp_num: 3;
                const uint32_t reserved_0: 1;
                uint32_t block_num: 4;
                const uint32_t reserved_1: 13;
            } b;
        } JPU_MCU_INFO_TypeDef;



    /* 0x1C
        1:0     R/W    rot_deg_factor                  0x0
        2       R/W    v_mirror                        0x0
        3       R/W    h_mirror                        0x0
        4       R/W    rot_mir_en                      0x0
        31:5    R      reserved                        0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t rot_deg_factor: 2;
                uint32_t v_mirror: 1;
                uint32_t h_mirror: 1;
                uint32_t rot_mir_en: 1;
                const uint32_t reserved_0: 27;
            } b;
        } JPU_ROT_INFO_TypeDef;



    /* 0x20
        1:0     R/W    v_ds_factor                  0x0
        3:2     R/W    h_ds_factor                  0x0
        4       R/W    ds_en                        0x0
        31:5    R      reserved                     0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t v_ds_factor: 2;
                uint32_t h_ds_factor: 2;
                uint32_t ds_en: 1;
                const uint32_t reserved_0: 27;
            } b;
        } JPU_SCL_INFO_TypeDef;



    /* 0x24
        0        R/W      disp_if_clr                  0x0
        1        R/W      sensor_if_clr                0x0
        31:30    R        reserved                     0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t disp_if_clr: 1;
                uint32_t sensor_if_clr: 1;
                const uint32_t reserved_0: 30;
            } b;
        } JPU_IF_INFO_TypeDef;



    /* 0x28
        0        R/W      roi_en                        0x0
        1        R/W      done_int_mode                 0x0
        31:30    R        reserved                      0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t roi_en: 1;
                uint32_t done_int_mode: 1;
                const uint32_t reserved_0: 30;
            } b;
        } JPU_CLP_INFO_TypeDef;



    /* 0x2C
        2:0        R/W      mcu_req_num                  0x1
        5:3        R/W      pb_num                       0x0
        15:6       R        reserved                     0x0
        31:16      R/W      pb_line                      0x10
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t mcu_req_num: 3;
                uint32_t pb_num: 3;
                const uint32_t reserved_0: 10;
                uint32_t pb_line: 16;
            } b;
        } JPU_OP_INFO_TypeDef;



    /* 0x30
        1:0        R/W      dpb_interleave_mode          0x0
        5:2        R/W      pack_mode                    0x0
        7:6        R/W      dpb_endianess                0x0
        31:8       R/W      reserved                     0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t dpb_interleave_mode: 2;
                uint32_t pack_mode: 4;
                uint32_t dpb_endianess: 2;
                const uint32_t reserved_0: 24;
            } b;
        } JPU_DPB_CONFIG_TypeDef;



    /* 0x34
        31:0    R/W    base_addr                    0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t base_addr: 32;
            } b;
        } JPU_DPB_BASE00_TypeDef;



    /* 0x38
        31:0    R/W    base_addr                    0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t base_addr: 32;
            } b;
        } JPU_DPB_BASE01_TypeDef;



    /* 0x3C
        31:0    R/W    base_addr                    0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t base_addr: 32;
            } b;
        } JPU_DPB_BASE02_TypeDef;



    /* 0x40
        31:0    R/W    base_addr                    0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t base_addr: 32;
            } b;
        } JPU_DPB_BASE10_TypeDef;



    /* 0x44
        31:0    R/W    base_addr                    0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t base_addr: 32;
            } b;
        } JPU_DPB_BASE11_TypeDef;



    /* 0x48
        31:0    R/W    base_addr                    0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t base_addr: 32;
            } b;
        } JPU_DPB_BASE12_TypeDef;



    /* 0x4C
        31:0    R/W    base_addr                    0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t base_addr: 32;
            } b;
        } JPU_DPB_BASE20_TypeDef;



    /* 0x50
        31:0    R/W    base_addr                    0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t base_addr: 32;
            } b;
        } JPU_DPB_BASE21_TypeDef;



    /* 0x54
        31:0    R/W    base_addr                    0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t base_addr: 32;
            } b;
        } JPU_DPB_BASE22_TypeDef;



    /* 0x58
        31:0    R/W    base_addr                    0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t base_addr: 32;
            } b;
        } JPU_DPB_BASE30_TypeDef;



    /* 0x5C
        31:0    R/W    base_addr                    0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t base_addr: 32;
            } b;
        } JPU_DPB_BASE31_TypeDef;



    /* 0x60
        31:0    R/W    base_addr                    0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t base_addr: 32;
            } b;
        } JPU_DPB_BASE32_TypeDef;



    /* 0x64
        15:0     R/W      dpb_lum_stride            0x0
        31:16    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t dpb_lum_stride: 16;
                const uint32_t reserved_0: 16;
            } b;
        } JPU_DPB_YSTRIDE_TypeDef;



    /* 0x68
        15:0     R/W      dpb_chrom_stride          0x0
        31:16    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t dpb_chrom_stride: 16;
                const uint32_t reserved_0: 16;
            } b;
        } JPU_DPB_CSTRIDE_TypeDef;

    /* 0x6C
        0       R      axi_wr_resp_check          0x0
        31:1    R      reserved                   0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t axi_wr_resp_check: 1;
                const uint32_t reserved_0: 31;
            } b;
        } JPU_WRESP_CHECK_TypeDef;

    /* 0x70
        15:0     R/W      roi_y_base              0x0
        31:16    R/W      roi_x_base              0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t roi_y_base: 16;
                uint32_t roi_x_base: 16;
            } b;
        } JPU_CLP_BASE_TypeDef;


    /* 0x74
        15:0     R/W      roi_height               0x0
        31:16    R/W      roi_width                0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t roi_height: 16;
                uint32_t roi_width: 16;
            } b;
        } JPU_CLP_SIZE_TypeDef;


    /* 0x80
        0           R/W      mode               0x0
        1           R/W      auto_ctl           0x0
        9:2         R        reserved           0x0
        11:10       R/W      ht_dest            0x0
        31:12       R        reserved           0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t mode: 1;
                uint32_t auto_ctl: 1;
                const uint32_t reserved_0: 8;
                uint32_t ht_dest: 2;
                const uint32_t reserved_1: 20;
            } b;
        } JPU_HUFF_CTRL_TypeDef;


    /* 0x84
        9:0         R/W      ht_base_addr       0x0
        11:10       R/W      ht_dest            0x0
        31:12       R        reserved           0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t ht_base_addr: 10;
                uint32_t ht_dest: 2;
                const uint32_t reserved_0: 20;
            } b;
        } JPU_HUFF_ADDR_TypeDef;


    /* 0x88
        31:0     R/W      hft_data          0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t hft_data: 32;
            } b;
        } JPU_HUFF_DATA_TypeDef;

    /* 0x90
        0           R/W      mode               0x0
        1           R/W      auto_ctl           0x0
        5:2         R        reserved           0x0
        7:6         R/W      qmat_dest          0x0
        31:8        R        reserved           0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t mode: 1;
                uint32_t auto_ctl: 1;
                const uint32_t reserved_0: 4;
                uint32_t qmat_dest: 2;
                const uint32_t reserved_1: 24;
            } b;
        } JPU_QMAT_CTRL_TypeDef;

    /* 0x94
        5:0      R/W      qmat_base_addr         0x0
        7:6      R/W      qmat_dest              0x0
        31:8     R        reserved               0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t qmat_base_addr: 6;
                uint32_t qmat_dest: 2;
                const uint32_t reserved_0: 24;
            } b;
        } JPU_QMAT_ADDR_TypeDef;

    /* 0x98
        19:0     R/W      qmat_data                  0x0
        31:20    R        reserved                   0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t qmat_data: 20;
                const uint32_t reserved_0: 12;
            } b;
        } JPU_QMAT_DATA_TypeDef;

    /* 0xA0
        0           R/W      mode               0x0
        1           R/W      auto_ctl           0x0
        5:2         R        reserved           0x0
        8:6         R/W      coef_dest          0x0
        31:9        R        reserved           0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t mode: 1;
                uint32_t auto_ctl: 1;
                const uint32_t reserved_0: 4;
                uint32_t coef_dest: 3;
                const uint32_t reserved_1: 23;
            } b;
        } JPU_COEF_CTRL_TypeDef;

    /* 0xA4
        5:0      R/W      coef_base_addr          0x0
        8:6      R/W      coef_dest               0x0
        31:9     R        reserved                0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t coef_base_addr: 6;
                uint32_t coef_dest: 3;
                const uint32_t reserved_0: 23;
            } b;
        } JPU_COEF_ADDR_TypeDef;

    /* 0xA8
        31:0    R/W        coef_data                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t coef_data: 32;
            } b;
        } JPU_COEF_DATA_TypeDef;

    /* 0xB0
        31:0    R/W        restart_interval                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t restart_interval: 32;
            } b;
        } JPU_RST_INTERVAL_TypeDef;

    /* 0xB4
        25:0     R/W      rst_indx                  0x0
        31:26    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t rst_indx: 26;
                const uint32_t reserved_0: 6;
            } b;
        } JPU_RST_INDEX_TypeDef;

    /* 0xB8
        25:0     R/W      rst_cnt                   0x0
        31:26    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t rst_cnt: 26;
                const uint32_t reserved_0: 6;
            } b;
        } JPU_RST_COUNT_TypeDef;

    /* 0xC0
        0       R/W      jpg_done                  0x0
        1       R/W      jpg_err                   0x0
        2       R/W      bbc_int                   0x0
        3       R/W      jpg_of                    0x0
        4       R/W      pb0                       0x0
        5       R/W      pb1                       0x0
        6       R/W      pb2                       0x0
        7       R/W      pb3                       0x0
        8       R/W      jpg_stop                  0x0
        31:9    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t jpg_done: 1;
                uint32_t jpg_err: 1;
                uint32_t bbc_int: 1;
                uint32_t jpg_of: 1;
                uint32_t pb0: 1;
                uint32_t pb1: 1;
                uint32_t pb2: 1;
                uint32_t pb3: 1;
                uint32_t jpg_stop: 1;
                const uint32_t reserved_0: 23;
            } b;
        } JPU_INTR_MASK_TypeDef;

    /* 0xC8
        31:0    R        cycle                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t cycle;
            } b;
        } JPU_CYCLE_INFO_TypeDef;

    /* 0xF0
        11:0     R/W      dc_predict_cmp0           0x0
        31:12    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t dc_predict_cmp0: 12;
                const uint32_t reserved_0: 20;
            } b;
        } JPU_DPCM_DIFF_Y_TypeDef;

    /* 0xF4
        11:0     R/W      dc_predict_cmp1           0x0
        31:12    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t dc_predict_cmp1: 16;
                const uint32_t reserved_0: 16;
            } b;
        } JPU_DPCM_DIFF_CB_TypeDef;

    /* 0xF8
        11:0     R/W      dc_predict_cmp2           0x0
        31:12    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t dc_predict_cmp2: 16;
                const uint32_t reserved_0: 16;
            } b;
        } JPU_DPCM_DIFF_CR_TypeDef;

    /* 0xFC
        31:0    R        version                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t version;
            } b;
        } JPU_VERSION_INFO_TypeDef;

    /* 0x100
        0       R        reserved                   0x0
        1       R/W      init_gbu_ff_dec            0x0
        2       R/W      init_gbu_dec               0x0
        3       R/W      set_gbu_flush_enc          0x0
        31:4    R        reserved                   0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t reserved_0: 1;
                uint32_t init_gbu_ff_dec: 1;
                uint32_t init_gbu_dec: 1;
                uint32_t set_gbu_flush_enc: 1;
                const uint32_t reserved_1: 28;
            } b;
        } JPU_GBU_CTRL_TypeDef;

    /* 0x104
        0       R      busy                      0x0
        31:1    R      reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t busy: 1;
                const uint32_t reserved_0: 31;
            } b;
        } JPU_GBU_PBIT_BUSY_TypeDef;

    /* 0x110
        31:0     R/W      bit_ptr          0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t bit_ptr;
            } b;
        } JPU_GBU_BT_PTR_TypeDef;

    /* 0x114
        7:0     R/W      wd_ptr                    0x0
        31:8    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t wd_ptr: 8;
                const uint32_t reserved_0: 24;
            } b;
        } JPU_GBU_WD_PTR_TypeDef;

    /* 0x118
        31:0    R/W        total_bit_cnt_l                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t total_bit_cnt_l: 32;
            } b;
        } JPU_GBU_TT_CNT_L_TypeDef;

    /* 0x11C
        31:0    R/W        total_bit_cnt_h                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t total_bit_cnt_h: 32;
            } b;
        } JPU_GBU_TT_CNT_H_TypeDef;

    /* 0x120
        7:0     W        data_8b                   0x0
        31:8    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t data_8b: 8;
                const uint32_t reserved_0: 24;
            } b;
        } JPU_GBU_PBIT_08_TypeDef;

    /* 0x124
        15:0     W        data_16b                  0x0
        31:16    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t data_16b: 16;
                const uint32_t reserved_0: 16;
            } b;
        } JPU_GBU_PBIT_16_TypeDef;

    /* 0x128
        23:0     W        data_24b                  0x0
        31:24    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t data_24b: 24;
                const uint32_t reserved_0: 8;
            } b;
        } JPU_GBU_PBIT_24_TypeDef;

    /* 0x12C
        31:0     W        data_32b                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t data_32b: 32;
            } b;
        } JPU_GBU_PBIT_32_TypeDef;

    /* 0x140
        7:0      R/W      start_ptr                 0x0
        31:8     R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t start_ptr: 8;
                const uint32_t reserved_0: 24;
            } b;
        } JPU_GBU_BBSR_TypeDef;

    /* 0x144
        7:0      R/W      end_ptr                   0x0
        31:8     R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t end_ptr: 8;
                const uint32_t reserved_0: 24;
            } b;
        } JPU_GBU_BBER_TypeDef;

    /* 0x148
        7:0      R/W      int_ptr                   0x0
        31:8     R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t int_ptr: 8;
                const uint32_t reserved_0: 24;
            } b;
        } JPU_GBU_BBIR_TypeDef;

    /* 0x14C
        7:0      R/W      halt_ptr                  0x0
        31:8     R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t halt_ptr: 8;
                const uint32_t reserved_0: 24;
            } b;
        } JPU_GBU_BBHR_TypeDef;

    /* 0x158
        15:0     R/W      total_bit_cnt             0x0
        31:16    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t total_bit_cnt: 16;
                const uint32_t reserved_0: 16;
            } b;
        } JPU_GBU_BCNT_TypeDef;

    /* 0x160
        6:0      R/W      cur_rd_ptr                0x0
        31:7     R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t cur_rd_ptr: 7;
                const uint32_t reserved_0: 25;
            } b;
        } JPU_GBU_FF_RPTR_TypeDef;

    /* 0x164
        6:0      R/W      cur_wr_ptr                0x0
        31:7     R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t cur_wr_ptr: 7;
                const uint32_t reserved_0: 25;
            } b;
        } JPU_GBU_FF_WPTR_TypeDef;

    /* 0x208
        31:0     R/W      int_addr          0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t int_addr: 32;
            } b;
        } JPU_BBC_END_ADDR_TypeDef;

    /* 0x20C
        31:0     R/W      wr_ptr          0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t wr_ptr: 32;
            } b;
        } JPU_BBC_WR_PTR_TypeDef;

    /* 0x210
        31:0     R/W      rd_ptr          0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t rd_ptr: 32;
            } b;
        } JPU_BBC_RD_PTR_TypeDef;

    /* 0x214
        31:0     R/W      ext_start_addr          0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t ext_start_addr: 32;
            } b;
        } JPU_BBC_EXT_ADDR_TypeDef;

    /* 0x218
        6:0     R/W      int_start_addr            0x0
        31:7    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t int_start_addr: 7;
                const uint32_t reserved_0: 25;
            } b;
        } JPU_BBC_INT_ADDR_TypeDef;

    /* 0x21C
        7:0     R/W      transfer_sz_4B            0x0
        31:8    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t transfer_sz_4B: 8;
                const uint32_t reserved_0: 24;
            } b;
        } JPU_BBC_DATA_CNT_TypeDef;

    /* 0x220
        0        R/W      ld_sv_cmd                 0x0
        2:1      R/W      endian                    0x0
        31:3     R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t ld_sv_cmd: 1;
                uint32_t endian: 2;
                const uint32_t reserved_0: 29;
            } b;
        } JPU_BBC_COMMAND_TypeDef;

    /* 0x224
        0        R        busy                      0x0
        31:1     R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t busy: 1;
                const uint32_t reserved_0: 31;
            } b;
        } JPU_BBC_BUSY_TypeDef;

    /* 0x228
        0       R/W      auto_run                  0x0
        2:1     R/W      endian                    0x0
        31:3    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t auto_run: 1;
                uint32_t endian: 2;
                const uint32_t reserved_0: 29;
            } b;
        } JPU_BBC_CTRL_TypeDef;

    /* 0x22C
        23:0     R/W      cur_pos_pg                0x0
        31:24    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t cur_pos_pg: 24;
                const uint32_t reserved_0: 8;
            } b;
        } JPU_BBC_CUR_POS_TypeDef;

    /* 0x230
        31:0     R/W      base_addr          0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t base_addr;
            } b;
        } JPU_BBC_BAS_ADDR_TypeDef;

    /* 0x234
        23:0     R/W      strm_cnt              0x0
        30:24    R        reserved              0x0
        31       R/W      eof                   0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t strm_cnt: 24;
                const uint32_t reserved_0: 7;
                uint32_t eof: 1;
            } b;
        } JPU_BBC_STRM_CTRL_TypeDef;

    /* 0x238
        0       R/W      flush_end                 0x0
        31:1    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t flush_end: 1;
                const uint32_t reserved_0: 31;
            } b;
        } JPU_BBC_FLUSH_CMD_TypeDef;

    /*============================================================================*
    *                 JPU Wrapper Registers and Field Descriptions
    *============================================================================*/
    /* 0x00
        1:0       R/W      rgb_format                        0x0
        7:2       R        reserved                          0x0
        15:8      R/W      alpha                             0x0
        31:16     R        reserved                          0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t rgb_format: 2;
                const uint32_t reserved_0: 6;
                uint32_t alpha: 8;
                const uint32_t reserved_1: 16;
            } b;
        } JPU_WRAP_RGB_PF_TypeDef;

    /* 0x04
        0       R/W      axi_wr_mstr                        0x0
        31:1    R        reserved                           0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t axi_wr_mstr: 1;
                const uint32_t reserved_0: 31;
            } b;
        } JPU_WRAP_CFG_TypeDef;

    /* 0x08
        0       R        done                                     0x0
        31:4    R        reserved                                 0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t done: 1;
                const uint32_t reserved_0: 31;
            } b;
        } JPU_WRAP_DONE_TypeDef;

    /* 0x0C
        0       R/W      finish_int_en                        0x0
        31:1    R        reserved                             0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t finish_int_en: 1;
                const uint32_t reserved_0: 31;
            } b;
        } JPU_WRAP_INT_EN_TypeDef;

    /* 0x10
        0       R/W      finish_int_msk                           0x0
        31:1    R        reserved                                 0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t finish_int_msk: 1;
                const uint32_t reserved_0: 31;
            } b;
        } JPU_WRAP_INT_MSK_TypeDef;

    /* 0x14
        0       R        finish_int_raw_status                    0x0
        31:1    R        reserved                                 0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t finish_int_raw_status: 1;
                const uint32_t reserved_0: 31;
            } b;
        } JPU_WRAP_INT_RAW_STATUS_TypeDef;

    /* 0x18
        0       R        finish_int_status                        0x0
        31:1    R        reserved                                 0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t finish_int_status: 1;
                const uint32_t reserved_0: 31;
            } b;
        } JPU_WRAP_INT_STATUS_TypeDef;

    /* 0x1C
        0       W1C      int_clr                                  0x0
        31:1    R        reserved                                 0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t int_clr: 1;
                const uint32_t reserved_0: 31;
            } b;
        } JPU_WRAP_INT_CLR_TypeDef;


#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* RTL_JPU_REG_H */
