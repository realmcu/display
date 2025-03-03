#ifndef RTL_DBIC_REG_H
#define RTL_DBIC_REG_H

#include <stdint.h>
#include <stdbool.h>
#include "rtl876x.h"

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ================================================================================ */
/* ============= Refence: DC.xlsx =============== */
/* ================================================================================ */
typedef struct
{
    __IO uint32_t  CTRLR0;                  //0X00
    __IO uint32_t  RX_NDF;                  //0X04
    __IO uint32_t  SSIENR;                  //0X08
    __IO uint32_t  RESERVED[1];             //0X0C
    __IO uint32_t  SER;                     //0X10
    __IO uint32_t  BAUDR;                   //0X14
    __IO uint32_t  TXFTLR;                  //0X18
    __IO uint32_t  RXFTLR;                  //0X1C
    __IO uint32_t  TXFLR;                   //0X20
    __IO uint32_t  RXFLR;                   //0X24
    __IO uint32_t  SR;                      //0X28
    __IO uint32_t  IMR;                     //0X2C
    __IO uint32_t  ISR;                     //0X30
    __IO uint32_t  RISR;                    //0X34
    __IO uint32_t  TXOICR;                  //0X38
    __IO uint32_t  RXOICR;                  //0X3C
    __IO uint32_t  RXUICR;                  //0X40
    __IO uint32_t  MSTICR;                  //0X44
    __IO uint32_t  ICR;                     //0X48
    __IO uint32_t  DMACR;                   //0X4C
    __IO uint32_t  DMATDLR;                 //0X50
    __IO uint32_t  DMARDLR;                 //0X54
    __IO uint32_t  IDR;                     //0X58
    __IO uint32_t  SPIC_VERSION;            //0X5C
    union
    {
        __IO uint8_t  byte;
        __IO uint16_t half;
        __IO uint32_t word;
    } DR[16];                               //0x60-0x9C
    __IO uint32_t  DM_DR[16];               //0XA0-0xDC
    __IO uint32_t  READ_FAST_SINGLE;        //0XE0
    __IO uint32_t  READ_DUAL_DATA;          //0XE4
    __IO uint32_t  READ_DUAL_ADDR_DATA;     //0XE8
    __IO uint32_t  READ_QUAD_DATA;          //0XEC
    __IO uint32_t  READ_QUAD_ADDR_DATA;     //0XF0
    __IO uint32_t  WRITE_SINGLE;            //0XF4
    __IO uint32_t  WRITE_DUAL_DATA;         //0XF8
    __IO uint32_t  WRITE_DUAL_ADDR_DATA;    //0XFC
    __IO uint32_t  WRITE_QUAD_DATA;         //0X100
    __IO uint32_t  WRITE_QUAD_ADDR_DATA;    //0X104
    __IO uint32_t  WRITE_ENABLE;            //0X108
    __IO uint32_t  READ_STATUS;             //0X10C
    __IO uint32_t  CTRLR2;                  //0X110
    __IO uint32_t  FBAUDR;                  //0X114
    __IO uint32_t  USER_LENGTH;             //0X118
    __IO uint32_t  AUTO_LENGTH;             //0X11C
    __IO uint32_t  VALID_CMD;               //0X120
    __IO uint32_t  FLASH_SIZE;              //0X124
    __IO uint32_t  FLUSH_FIFO;              //0X128
    __IO uint32_t  DUM_BYTE;                //0X12C
    __IO uint32_t  TX_NDF;                  //0X130
    __IO uint32_t  DEVICE_INFO;             //0X134
    __IO uint32_t  TPR0;                    //0X138
    __IO uint32_t  AUTO_LENGTH2;            //0X13C
    __IO uint32_t  TPR1;                    //0X140
    __IO uint32_t  RESERVED1[15];            //0x144-0x17C
    __IO uint32_t  ST_DR[16];               //0X180-0x1BC
    __IO uint32_t  STFLR;                   //0X1C0
    __IO uint32_t  RESERVED2[3];            //0x1C4-0x1CC
    __IO uint32_t  PAGE_READ;               //0X1D0
} LCDC_DBIC_TypeDef;

/* 0x00
    4:0     R/W    sipol                   0x1F
    5       R/W    sipol_en                0x0
    6       R/W    scph                    0x0
    7       R/W    scpol                   0x0
    9:8     R/W    tmod                    0x0
    10      R/W    gclk_dis                0x0
    12:11   R      reserved2               0x0
    15:13   R/W    ddr_en                  0x0
    17:16   R/W    addr_ch                 0x0
    19:18   R/W    data_ch                 0x0
    21:20   R/W    cmd_ch                  0x0
    22      R/W    fast_rd                 0x0
    27:23   R/W    ck_mtimes               0x2
    29:28   R      reserved1               0x0
    30      R/W    uar                     0x0
    31      R/W    user_mode               0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t sipol: 5;
        __IO uint32_t sipol_en: 1;
        __IO uint32_t scph: 1;
        __IO uint32_t scpol: 1;
        __IO uint32_t tmod: 2;
        __IO uint32_t gclk_dis: 1;
        __I uint32_t reserved_1: 2;
        __IO uint32_t ddr_en: 3;
        __IO uint32_t addr_ch: 2;
        __IO uint32_t data_ch: 2;
        __IO uint32_t cmd_ch: 2;
        __IO uint32_t fast_rd: 1;
        __IO uint32_t ck_mtimes: 5;
        __I uint32_t reserved_0: 2;
        __IO uint32_t uar: 1;
        __IO uint32_t user_mode: 1;
    } b;
} DBIC_CTRLR0_TypeDef;



/* 0x04
    23:0    R/W    rx_ndf                  0x0
    31:24   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t rx_ndf: 24;
        __I uint32_t reserved_0: 8;
    } b;
} DBIC_RX_NDF_TypeDef;



/* 0x08
    0       R/W    spic_en                 0x0
    1       R/W    atck_cmd                0x0
    3:2     R      reserved2               0x0
    4       W      pgm_rst_test_en         0x0
    31:5    R      reserved1               0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t spic_en: 1;
        __IO uint32_t atck_cmd: 1;
        __I uint32_t reserved_1: 2;
        __O uint32_t pgm_rst_test_en: 1;
        __I uint32_t reserved_0: 27;
    } b;
} DBIC_SSIENR_TypeDef;



/* 0x10
    0       R/W    ser                     0x1
    31:1    R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t ser: 1;
        __I uint32_t reserved_0: 31;
    } b;
} DBIC_SER_TypeDef;



/* 0x14
    11:0    R/W    sckdv                   0x6
    31:12   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t sckdv: 12;
        __I uint32_t reserved_0: 20;
    } b;
} DBIC_BAUDR_TypeDef;



/* 0x18
    5: 0    R/W    tft                     0x0
    31:6    R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t tft: 6;
        __I uint32_t reserved_0: 26;
    } b;
} DBIC_TXFTLR_TypeDef;



/* 0x1C
    2: 0    R/W    rft                     0x7
    31:3    R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t rft: 3;
        __I uint32_t reserved_0: 29;
    } b;
} DBIC_RXFTLR_TypeDef;



/* 0x20
    6: 0    R      txflr                   0x0
    31: 7   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t txflr: 7;
        __I uint32_t reserved_0: 25;
    } b;
} DBIC_TXFLR_TypeDef;



/* 0x24
    3: 0    R      rxflr                   0x0
    31: 4   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t rxflr: 4;
        __I uint32_t reserved_0: 28;
    } b;
} DBIC_RXFLR_TypeDef;



/* 0x28
    0       R      busy                    0x0
    1       R      tfnf                    0x1
    2       R      tfe                     0x1
    3       R      rfne                    0x0
    4       R      rff                     0x0
    5       R      txe                     0x0
    6       R      dcol                    0x0
    7       R      boot_fin                0x0
    8       R/W    atwr_rdsr_n             0x0
    31:9    R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t busy: 1;
        __I uint32_t tfnf: 1;
        __I uint32_t tfe: 1;
        __I uint32_t rfne: 1;
        __I uint32_t rff: 1;
        __I uint32_t txe: 1;
        __I uint32_t dcol: 1;
        __I uint32_t boot_fin: 1;
        __IO uint32_t atwr_rdsr_n: 1;
        __I uint32_t reserved_0: 23;
    } b;
} DBIC_SR_TypeDef;



/* 0x2C
    0       R/W    txeim                   0x0
    1       R/W    txoim                   0x1
    2       R/W    rxuim                   0x1
    3       R/W    rxoim                   0x1
    4       R/W    rxfim                   0x0
    5       R/W    fseim                   0x1
    6       R/W    wbeim                   0x1
    7       R/W    byeim                   0x1
    8       R/W    aceim                   0x1
    9       R/W    useim                   0x0
    10      R/W    tfsim                   0x0
    11      R/W    acsim                   0x0
    12      R/W    dreim                   0x0
    13      R/W    stuim                   0x0
    14      R/W    stoim                   0x0
    15      R/W    stfim                   0x0
    16      R/W    nweim                   0x0
    31:17   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t txeim: 1;
        __IO uint32_t txoim: 1;
        __IO uint32_t rxuim: 1;
        __IO uint32_t rxoim: 1;
        __IO uint32_t rxfim: 1;
        __IO uint32_t fseim: 1;
        __IO uint32_t wbeim: 1;
        __IO uint32_t byeim: 1;
        __IO uint32_t aceim: 1;
        __IO uint32_t useim: 1;
        __IO uint32_t tfsim: 1;
        __IO uint32_t acsim: 1;
        __IO uint32_t dreim: 1;
        __IO uint32_t stuim: 1;
        __IO uint32_t stoim: 1;
        __IO uint32_t stfim: 1;
        __IO uint32_t nweim: 1;
        __I uint32_t reserved_0: 15;
    } b;
} DBIC_IMR_TypeDef;



/* 0x30
    0       R      txeis                   0x0
    1       R      txois                   0x0
    2       R      rxuis                   0x0
    3       R      rxois                   0x0
    4       R      rxfis                   0x0
    5       R      fseis                   0x0
    6       R      wbeis                   0x0
    7       R      byeis                   0x0
    8       R      aceis                   0x0
    9       R      useis                   0x0
    10      R      tfsis                   0x0
    11      R      acsis                   0x0
    12      R      dreis                   0x0
    13      R      stuis                   0x0
    14      R      stois                   0x0
    15      R      stfis                   0x0
    16      R      nweis                   0x0
    31:17   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t txeis: 1;
        __I uint32_t txois: 1;
        __I uint32_t rxuis: 1;
        __I uint32_t rxois: 1;
        __I uint32_t rxfis: 1;
        __I uint32_t fseis: 1;
        __I uint32_t wbeis: 1;
        __I uint32_t byeis: 1;
        __I uint32_t aceis: 1;
        __I uint32_t useis: 1;
        __I uint32_t tfsis: 1;
        __I uint32_t acsis: 1;
        __I uint32_t dreis: 1;
        __I uint32_t stuis: 1;
        __I uint32_t stois: 1;
        __I uint32_t stfis: 1;
        __I uint32_t nweis: 1;
        __I uint32_t reserved_0: 15;
    } b;
} DBIC_ISR_TypeDef;



/* 0x34
    0       R      txeir                   0x0
    1       R      txoir                   0x0
    2       R      rxuir                   0x0
    3       R      rxoir                   0x0
    4       R      rxfir                   0x0
    5       R      fseir                   0x0
    6       R      wbeir                   0x0
    7       R      byeir                   0x0
    8       R      aceir                   0x0
    9       R      useir                   0x0
    10      R      tfsir                   0x0
    11      R      acsir                   0x0
    12      R      dreir                   0x0
    13      R      stuir                   0x0
    14      R      stoir                   0x0
    15      R      stfir                   0x0
    16      R      nweir                   0x0
    31:17   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t txeir: 1;
        __I uint32_t txoir: 1;
        __I uint32_t rxuir: 1;
        __I uint32_t rxoir: 1;
        __I uint32_t rxfir: 1;
        __I uint32_t fseir: 1;
        __I uint32_t wbeir: 1;
        __I uint32_t byeir: 1;
        __I uint32_t aceir: 1;
        __I uint32_t useir: 1;
        __I uint32_t tfsir: 1;
        __I uint32_t acsir: 1;
        __I uint32_t dreir: 1;
        __I uint32_t stuir: 1;
        __I uint32_t stoir: 1;
        __I uint32_t stfir: 1;
        __I uint32_t nweir: 1;
        __I uint32_t reserved_0: 15;
    } b;
} DBIC_RISR_TypeDef;



/* 0x38
    0       R/W    txoicr                  0x0
    31:1    R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t txoicr: 1;
        __I uint32_t reserved_0: 31;
    } b;
} DBIC_TXOICR_TypeDef;



/* 0x3C
    0       R/W    rxoicr                  0x0
    31:1    R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t rxoicr: 1;
        __I uint32_t reserved_0: 31;
    } b;
} DBIC_RXOICR_TypeDef;



/* 0x40
    0       R/W    rxuicr                  0x0
    31:1    R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t rxuicr: 1;
        __I uint32_t reserved_0: 31;
    } b;
} DBIC_RXUICR_TypeDef;



/* 0x44
    0       R/W    msticr                  0x0
    31:1    R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t msticr: 1;
        __I uint32_t reserved_0: 31;
    } b;
} DBIC_MSTICR_TypeDef;



/* 0x48
    0       R/W    icr                     0x0
    31:1    R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t icr: 1;
        __I uint32_t reserved_0: 31;
    } b;
} DBIC_ICR_TypeDef;



/* 0x4C
    0       R/W    rx_dmac_en              0x0
    1       R/W    tx_dmac_en              0x0
    31:2    R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t rx_dmac_en: 1;
        __IO uint32_t tx_dmac_en: 1;
        __I uint32_t reserved_0: 30;
    } b;
} DBIC_DMACR_TypeDef;



/* 0x50
    5: 0    R/W    dmatdl                  0x0
    31: 6   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t dmatdl: 6;
        __I uint32_t reserved_0: 26;
    } b;
} DBIC_DMATDLR_TypeDef;



/* 0x54
    2: 0    R/W    dmardl                  0x0
    31: 3   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t dmardl: 3;
        __I uint32_t reserved_0: 29;
    } b;
} DBIC_DMARDLR_TypeDef;



/* 0x58
    31:0    R      idcode                  0x303FF01
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t idcode: 32;
    } b;
} DBIC_IDR_TypeDef;



/* 0x5C
    31:0    R      spic_version            0x30530
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t spic_version: 32;
    } b;
} DBIC_SPIC_VERSION_TypeDef;



/* 0x60
    31:0    R/W    dr                      0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t dr: 32;
    } b;
} DBIC_DR_TypeDef;



/* 0xA0
    0       W      data_mask_dr            0x0
    1       W      data_en_dr              0x0
    31: 2   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __O uint32_t data_mask_dr: 1;
        __O uint32_t data_en_dr: 1;
        __I uint32_t reserved_0: 30;
    } b;
} DBIC_DM_DR_TypeDef;




/* 0xE0
    15: 0   R/W    frd_cmd                 0x0B0B
    31: 16  R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t frd_cmd: 16;
        __I uint32_t reserved_0: 16;
    } b;
} DBIC_READ_FAST_SINGLE_TypeDef;



/* 0xE4
    7: 0    R/W    rd_dual_o_cmd           0x3B
    31: 8   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t rd_dual_o_cmd: 8;
        __I uint32_t reserved_0: 24;
    } b;
} DBIC_READ_DUAL_DATA_TypeDef;



/* 0xE8
    7: 0    R/W    rd_dual_io_cmd          0xBB
    31: 8   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t rd_dual_io_cmd: 8;
        __I uint32_t reserved_0: 24;
    } b;
} DBIC_READ_DUAL_ADDR_DATA_TypeDef;



/* 0xEC
    7: 0    R/W    rd_quad_o_cmd           0x6B
    31: 8   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t rd_quad_o_cmd: 8;
        __I uint32_t reserved_0: 24;
    } b;
} DBIC_READ_QUAD_DATA_TypeDef;



/* 0xF0
    7: 0    R/W    rd_quad_io_cmd          0xEB
    15: 8   R      exit_prm_dum_len        0x0
    23: 16  R/W    prm_val                 0xA5
    31: 24  R      exit_prm_cmd            0xFF
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t rd_quad_io_cmd: 8;
        __I uint32_t exit_prm_dum_len: 8;
        __IO uint32_t prm_val: 8;
        __I uint32_t exit_prm_cmd: 8;
    } b;
} DBIC_READ_QUAD_ADDR_DATA_TypeDef;



/* 0xF4
    15: 0   R/W    wr_cmd                  0x0202
    31: 16  R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t wr_cmd: 16;
        __I uint32_t reserved_0: 16;
    } b;
} DBIC_WRITE_SINGLE_TypeDef;



/* 0xF8
    7: 0    R/W    wr_dual_i_cmd           0xA2
    31: 8   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t wr_dual_i_cmd: 8;
        __I uint32_t reserved_0: 24;
    } b;
} DBIC_WRITE_DUAL_DATA_TypeDef;



/* 0xFC
    7: 0    R/W    wr_dual_ii_cmd          0x0
    31: 8   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t wr_dual_ii_cmd: 8;
        __I uint32_t reserved_0: 24;
    } b;
} DBIC_WRITE_DUAL_ADDR_DATA_TypeDef;



/* 0x100
    7: 0    R/W    wr_quad_i_cmd           0x32
    31: 8   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t wr_quad_i_cmd: 8;
        __I uint32_t reserved_0: 24;
    } b;
} DBIC_WRITE_QUAD_DATA_TypeDef;



/* 0x104
    7: 0    R/W    wr_quad_ii_cmd          0x38
    31: 8   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t wr_quad_ii_cmd: 8;
        __I uint32_t reserved_0: 24;
    } b;
} DBIC_WRITE_QUAD_ADDR_DATA_TypeDef;



/* 0x108
    7: 0    R/W    wr_en_cmd_1st_byte      0x06
    15: 8   R/W    wr_en_cmd_2nd_byte      0x06
    31: 16  R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t wr_en_cmd_1st_byte: 8;
        __IO uint32_t wr_en_cmd_2nd_byte: 8;
        __I uint32_t reserved_0: 16;
    } b;
} DBIC_WRITE_ENABLE_TypeDef;



/* 0x10C
    7: 0    R/W    rd_st_cmd_1st_byte      0x05
    15: 8   R/W    rd_st_cmd_2nd_byte      0x05
    23: 16  R/W    rd_st_cmd_3rd_byte      0x0
    24      R      reserved                0x0
    25      R/W    st_cmd_ddr_en           0x0
    27:26   R/W    st_cmd_ch               0x0
    29:28   R/W    st_cmd_len              0x1
    30      R/W    interval_odd            0x0
    31      R/W    interval_en             0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t rd_st_cmd_1st_byte: 8;
        __IO uint32_t rd_st_cmd_2nd_byte: 8;
        __IO uint32_t rd_st_cmd_3rd_byte: 8;
        __I uint32_t reserved_0: 1;
        __IO uint32_t st_cmd_ddr_en: 1;
        __IO uint32_t st_cmd_ch: 2;
        __IO uint32_t st_cmd_len: 2;
        __IO uint32_t interval_odd: 1;
        __IO uint32_t interval_en: 1;
    } b;
} DBIC_READ_STATUS_TypeDef;



/* 0x110
    0       R/W    so_dnum                 0x1
    1       R/W    wpn_set                 0x0
    2       R/W    wpn_dnum                0x0
    3       R/W    dr_fixed                0x0
    7: 4    R/W    tx_fifo_entry           0x6
    11: 8   R/W    rx_fifo_entry           0x3
    12      R/W    dm_act                  0x1
    13      R/W    full_wr                 0x1
    14      R/W    dis_dm_ca               0x0
    15      R/W    dis_wrap_align          0x0
    31: 16  R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t so_dnum: 1;
        __IO uint32_t wpn_set: 1;
        __IO uint32_t wpn_dnum: 1;
        __IO uint32_t dr_fixed: 1;
        __IO uint32_t tx_fifo_entry: 4;
        __IO uint32_t rx_fifo_entry: 4;
        __IO uint32_t dm_act: 1;
        __IO uint32_t full_wr: 1;
        __IO uint32_t dis_dm_ca: 1;
        __IO uint32_t dis_wrap_align: 1;
        __I uint32_t reserved_0: 16;
    } b;
} DBIC_CTRLR2_TypeDef;



/* 0x114
    11: 0   R/W    fsckdv                  0x01
    31: 12  R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t fsckdv: 12;
        __I uint32_t reserved_0: 20;
    } b;
} DBIC_FBAUDR_TypeDef;



/* 0x118
    11: 0   R/W    user_rd_dummy_length    0x0
    13: 12  R/W    user_cmd_lenght         0x0
    15: 14  R      reserved2               0x0
    19: 16  R/W    user_addr_length        0x3
    31: 20  R      reserved1               0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t user_rd_dummy_length: 12;
        __IO uint32_t user_cmd_length: 2;
        __I uint32_t reserved_1: 2;
        __IO uint32_t user_addr_length: 4;
        __I uint32_t reserved_0: 12;
    } b;
} DBIC_USER_LENGTH_TypeDef;



/* 0x11C
    11: 0   R/W    rd_dummy_length         0x0
    15: 12  R/W    in_physical_cyc         0x0
    19: 16  R/W    auto_addr_length        0x3
    27: 20  R/W    rdsr_dummy_length       0x0
    31: 28  R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t rd_dummy_length: 12;
        __IO uint32_t in_physical_cyc: 4;
        __IO uint32_t auto_addr_length: 4;
        __IO uint32_t rdsr_dummy_length: 8;
        __I uint32_t reserved_0: 4;
    } b;
} DBIC_AUTO_LENGTH_TypeDef;



/* 0x120
    0       R/W    frd_single              0x0
    1       R/W    rd_dual_i               0x0
    2       R/W    rd_dual_io              0x0
    3       R/W    rd_quad_o               0x0
    4       R/W    rd_quad_io              0x0
    5       R/W    wr_dual_i               0x0
    6       R/W    wr_dual_ii              0x0
    7       R/W    wr_quad_i               0x0
    8       R/W    wr_quad_ii              0x0
    9       R/W    rm_rdsr                 0x0
    10      R/W    rm_wen                  0x0
    11      R/W    prm_en                  0x0
    12      R/W    ctrlr0_ch               0x0
    13      R/W    dum_en                  0x0
    14      R/W    seq_rd_en               0x1
    15      R/W    seq_wr_en               0x0
    31: 16  R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t frd_single: 1;
        __IO uint32_t rd_dual_i: 1;
        __IO uint32_t rd_dual_io: 1;
        __IO uint32_t rd_quad_o: 1;
        __IO uint32_t rd_quad_io: 1;
        __IO uint32_t wr_dual_i: 1;
        __IO uint32_t wr_dual_ii: 1;
        __IO uint32_t wr_quad_i: 1;
        __IO uint32_t wr_quad_ii: 1;
        __IO uint32_t rm_rdsr: 1;
        __IO uint32_t rm_wen: 1;
        __IO uint32_t prm_en: 1;
        __IO uint32_t ctrlr0_ch: 1;
        __IO uint32_t dum_en: 1;
        __IO uint32_t seq_rd_en: 1;
        __IO uint32_t seq_wr_en: 1;
        __I uint32_t reserved_0: 16;
    } b;
} DBIC_VALID_CMD_TypeDef;



/* 0x124
    3: 0    R/W    flash_size              0xD
    31: 4   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t flash_size: 4;
        __I uint32_t reserved_0: 28;
    } b;
} DBIC_FLASH_SIZE_TypeDef;



/* 0x128
    0       W      flush_all               0x0
    1       W      flush_dr_fifo           0x0
    2       W      flush_st_fifo           0x0
    31: 3   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __O uint32_t flush_all: 1;
        __O uint32_t flush_dr_fifo: 1;
        __O uint32_t flush_st_fifo: 1;
        __I uint32_t reserved_0: 29;
    } b;
} DBIC_FLUSH_FIFO_TypeDef;



/* 0x12C
    7: 0    R/W    dum_byte_val            0x00
    31: 8   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t dum_byte_val: 8;
        __I uint32_t reserved_0: 24;
    } b;
} DBIC_DUM_BYTE_TypeDef;



/* 0x130
    23: 0   R/W    tx_ndf                  0x00
    31: 24  R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t tx_ndf: 24;
        __I uint32_t reserved_0: 8;
    } b;
} DBIC_TX_NDF_TypeDef;



/* 0x134
    3: 0    R/W    page_size               0x08
    4       R/W    wr_page_en              0x1
    5       R/W    rd_page_en              0x0
    6       R/W    atom_size               0x0
    7       R      reserved2               0x0
    8       R/W    nor_flash               0x1
    9       R/W    nand_flash              0x0
    10      R/W    psram                   0x0
    11      R/W    jedec_p2cmf             0x0
    12      R/W    data_unit_2b            0x0
    31: 13  R      reserved1               0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t page_size: 4;
        __IO uint32_t wr_page_en: 1;
        __IO uint32_t rd_page_en: 1;
        __IO uint32_t atom_size: 1;
        __I uint32_t reserved_1: 1;
        __IO uint32_t nor_flash: 1;
        __IO uint32_t nand_flash: 1;
        __IO uint32_t psram: 1;
        __IO uint32_t jedec_p2cmf: 1;
        __IO uint32_t data_unit_2b: 1;
        __I uint32_t reserved_0: 19;
    } b;
} DBIC_DEVICE_INFO_TypeDef;



/* 0x138
    5: 0    R/W    cs_h_rd_dum_len         0x30
    11: 6   R/W    cs_h_wr_dum_len         0x30
    15: 12  R/W    cs_active_hold          0x00
    23: 16  R/W    cs_seq_timeout          0x10
    31: 24  R/W    cs_tcem                 0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t cs_h_rd_dum_len: 6;
        __IO uint32_t cs_h_wr_dum_len: 6;
        __IO uint32_t cs_active_hold: 4;
        __IO uint32_t cs_seq_timeout: 8;
        __IO uint32_t cs_tcem: 8;
    } b;
} DBIC_TPR0_TypeDef;



/* 0x13C
    11: 0   R/W    wr_dummy_length         0x0
    31: 12  R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t wr_dummy_length: 12;
        __I uint32_t reserved_0: 20;
    } b;
} DBIC_AUTO_LENGTH2_TypeDef;



/* 0x140
    3:0     R/W    cr_active_setup         0x1
    7:4     R      reserved2               0x0
    15:8    R/W    cr_idle_window          0x10
    23:16   R/W    cr_tpwr                 0x40
    31:24   R      reserved1               0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t cr_active_setup: 4;
        __I uint32_t reserved_1: 4;
        __IO uint32_t cr_idle_window: 8;
        __IO uint32_t cr_tpwr: 8;
        __I uint32_t reserved_0: 8;
    } b;
} DBIC_TPR1_TypeDef;



/* 0x180
    7:0     R      st_dr                   0x0
    31: 8   R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t st_dr: 8;
        __I uint32_t reserved_0: 24;
    } b;
} DBIC_ST_DR_TypeDef;



/* 0x1C0
    2: 0    R     stflr                  0x0
    31: 3   R      reserved               0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t stflr: 3;
        __I uint32_t reserved: 29;
    } b;
} DBIC_STFLR_TypeDef;



/* 0x1D0
    7: 0    R/W    page_rd_cmd             0x13
    15: 8   R      reserved2               0x0
    17: 16  R/W    page_rd_ch              0x0
    19: 18  R/W    page_rd_addr_len        0x3
    31: 20  R      reserved1               0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t page_rd_cmd: 8;
        __I uint32_t reserved_1: 8;
        __IO uint32_t page_rd_ch: 2;
        __IO uint32_t page_rd_addr_len: 2;
        __I uint32_t reserved_0: 12;
    } b;
} DBIC_PAGE_READ_TypeDef;


#ifdef  __cplusplus
}
#endif /* __cplusplus */
#endif /* RTL_DBIC_REG_H */
