#ifndef RTL_LCDC_DMA_REG_H
#define RTL_LCDC_DMA_REG_H

#include <stdint.h>
#include <stdbool.h>
#include "rtl876x.h"

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

/*============================================================================*
 *                         GDMA Registers Memory Map
 *============================================================================*/
#define LCDC_DMA_REGISTER_DEFINE 1
typedef struct
{
    __IO uint32_t LCDC_DMA_SARx;                /*!< 0x00 */
    __I  uint32_t LCDC_DMA_CURR_SARx;           /*!< 0x04 */
    __IO uint32_t LCDC_DMA_DARx;                /*!< 0x08 */
    __I  uint32_t LCDC_DMA_CURR_DARx;           /*!< 0x0C */
    __IO uint32_t LCDC_DMA_LLPx;                /*!< 0x10 */
    uint32_t LCDC_DMA_RSVD;                     /*!< 0x14 */
    __IO uint32_t LCDC_DMA_CTL_LOWx;            /*!< 0x18 */
    __IO uint32_t LCDC_DMA_CTL_HIGHx;           /*!< 0x1C */
    uint32_t LCDC_DMA_RSVD1[8];                 /*!< 0x20 ~ 0x3C */
    __IO uint32_t LCDC_DMA_CFG_LOWx;            /*!< 0x40 */
    __IO uint32_t LCDC_DMA_CFG_HIGHx;           /*!< 0x44 */
    __IO uint32_t LCDC_DMA_SGR_LOW;             /*!< 0x48 */
    __IO uint32_t LCDC_DMA_SGR_HIGH;            /*!< 0x4C */
    __IO uint32_t LCDC_DMA_DSR_LOW;             /*!< 0x50 */
    __IO uint32_t LCDC_DMA_DSR_HIGH;            /*!< 0x54 */
} LCDC_DMA_ChannelTypeDef;

typedef struct
{
    uint32_t LCDC_DMA_RSVD[10];                 /*!< 0x2C0 - 0x2E4 */

    __I uint32_t LCDC_DMA_StatusTfr;            /*!< 0x2E8 */
    uint32_t LCDC_DMA_RSVD1;
    __I uint32_t LCDC_DMA_StatusBlock;          /*!< 0x2F0 */
    uint32_t LCDC_DMA_RSVD2;
    __I uint32_t LCDC_DMA_StatusSrcTran;        /*!< 0x2F8 */
    uint32_t LCDC_DMA_RSVD3;
    __I uint32_t LCDC_DMA_StatusDstTran;        /*!< 0x300 */
    uint32_t LCDC_DMA_RSVD4;
    __I uint32_t LCDC_DMA_StatuErr;             /*!< 0x308 */
    __I uint32_t LCDC_DMA_StatuErrNonSecure;

    __IO uint32_t LCDC_DMA_MaskTfr;             /*!< 0x310 */
    uint32_t LCDC_DMA_RSVD6;
    __IO uint32_t LCDC_DMA_MaskBlock;           /*!< 0x318 */
    uint32_t LCDC_DMA_RSVD7;
    __IO uint32_t LCDC_DMA_MaskSrcTran;         /*!< 0x320 */
    uint32_t LCDC_DMA_RSVD8;
    __IO uint32_t LCDC_DMA_MaskDstTran;         /*!< 0x328 */
    uint32_t LCDC_DMA_RSVD9;
    __IO uint32_t LCDC_DMA_MaskErr;             /*!< 0x330 */
    __IO uint32_t LCDC_DMA_MaskErrNonSecure;

    __O uint32_t LCDC_DMA_ClearTfr;             /*!< 0x338 */
    uint32_t LCDC_DMA_RSVD11;
    __O uint32_t LCDC_DMA_ClearBlock;           /*!< 0x340 */
    uint32_t LCDC_DMA_RSVD12;
    __O uint32_t LCDC_DMA_ClearSrcTran;         /*!< 0x348 */
    uint32_t LCDC_DMA_RSVD13;
    __O uint32_t LCDC_DMA_ClearDstTran;         /*!< 0x350 */
    uint32_t LCDC_DMA_RSVD14;
    __O uint32_t LCDC_DMA_ClearErr;             /*!< 0x358 */
    __O uint32_t LCDC_DMA_ClearErrNonSecure;

    uint32_t LCDC_DMA_RSVD16[14];               /*!< 0x360 ~ 0x394 */

    __IO uint32_t LCDC_DMA_DmaCfgReg;           /*!< 0x398 */
    uint32_t LCDC_DMA_RSVD17;
    __IO uint32_t LCDC_DMA_ChEnReg;             /*!< 0x3A0 */
    uint32_t LCDC_DMA_RSVD18;

    uint32_t LCDC_DMA_RSVD19[4];                /*!< 0x3A8 ~ 0x3B4 */
    __IO uint32_t LCDC_DMA_DmaOsNum;            /*!< 0x3B8 */
    uint32_t LCDC_DMA_RSVD20;
} LCDC_DMA_TypeDef;



/*============================================================================*
 *                         GDMA Registers and Field Descriptions
 *============================================================================*/
/* 0x00
   31:0    R/W    SAR                 undefined
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t SAR: 32;
    } b;
} LCDC_DMA_SARx_TypeDef;



/* 0x04
   31:0    R      CURR_SAR            0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t CURR_SAR: 32;
    } b;
} LCDC_DMA_CURR_SARx_TypeDef;



/* 0x08
   31:0    R/W    DAR                 undefined
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t DAR: 32;
    } b;
} LCDC_DMA_DARx_TypeDef;



/* 0x0C
   31:0    R      CURR_DAR            0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t CURR_DAR: 32;
    } b;
} LCDC_DMA_CURR_DARx_TypeDef;



/* 0x10
   1:0     R/W    reserved13          0x0
   31:2    R/W    LOC                 0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reserved_0: 2;
        __IO uint32_t LOC: 30;
    } b;
} LCDC_DMA_LLPx_TypeDef;


/* 0x18
   0       R/W    INT_EN              0x1
   3:1     R/W    DST_TR_WIDTH        0x0
   6:4     R/W    SRC_TR_WIDTH        0x0
   8:7     R/W    DINC                0x0
   10:9    R/W    SINC                0x0
   13:11   R/W    DEST_MSIZE          0x1
   16:14   R/W    SRC_MSIZE           0x1
   17      R/W    SRC_GATHER_EN       0x0
   18      R/W    DST_SCATTER_EN      0x0
   19      R/W    reserved20          0x0
   22:20   R/W    TT_FC               0x0
   26:23   R      reserved18          0x0
   27      R/W    LLP_DST_EN          0x0
   28      R/W    LLP_SRC_EN          0x0
   31:29   R      reserved15          0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t INT_EN: 1;
        __IO uint32_t DST_TR_WIDTH: 3;
        __IO uint32_t SRC_TR_WIDTH: 3;
        __IO uint32_t DINC: 2;
        __IO uint32_t SINC: 2;
        __IO uint32_t DEST_MSIZE: 3;
        __IO uint32_t SRC_MSIZE: 3;
        __IO uint32_t SRC_GATHER_EN: 1;
        __IO uint32_t DST_SCATTER_EN: 1;
        __IO uint32_t reserved_2: 1;
        __IO uint32_t TT_FC: 3;
        __I uint32_t reserved_1: 4;
        __IO uint32_t LLP_DST_EN: 1;
        __IO uint32_t LLP_SRC_EN: 1;
        __I uint32_t reserved_0: 3;
    } b;
} LCDC_DMA_CTL_LOWx_TypeDef;



/* 0x1C
   31:0    R      TRANS_DATA_CNT      0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t TRANS_DATA_CNT: 32;
    } b;
} LCDC_DMA_CTL_HIGHx_R_TypeDef;



/* 0x1C
   15:0    W      BLOCK_TS            0x2
   31:16   W      reserved31          0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __O uint32_t BLOCK_TS: 16;
        __O uint32_t reserved_0: 16;
    } b;
} LCDC_DMA_CTL_HIGHx_W_TypeDef;


/* 0x40
   0       R      INACTIVE            0x1
   1       R      SRC_PCTL_OVER       0x1
   2       R      DST_PCTL_OVER       0x1
   3       R      reserved45          0x0
   7:4     R/W    CH_PRIOR            0x0
   8       R/W    CH_SUSP             0x0
   9       R      FIFO_EMPTY          0x1
   10      R/W    HS_SEL_DST          0x1
   11      R/W    HS_SEL_SRC          0x1
   17:12   R      reserved39          0x0
   18      R/W    DST_HS_POL          0x0
   19      R/W    SRC_HS_POL          0x0
   29:20   R/W    reserved36          0x0
   30      R/W    RELOAD_SRC          0x0
   31      R/W    RELOAD_DST          0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t INACTIVE: 1;
        __I uint32_t SRC_PCTL_OVER: 1;
        __I uint32_t DST_PCTL_OVER: 1;
        __I uint32_t reserved_2: 1;
        __IO uint32_t CH_PRIOR: 4;
        __IO uint32_t CH_SUSP: 1;
        __I uint32_t FIFO_EMPTY: 1;
        __IO uint32_t HS_SEL_DST: 1;
        __IO uint32_t HS_SEL_SRC: 1;
        __I uint32_t reserved_1: 6;
        __IO uint32_t DST_HS_POL: 1;
        __IO uint32_t SRC_HS_POL: 1;
        __IO uint32_t reserved_0: 10;
        __IO uint32_t RELOAD_SRC: 1;
        __IO uint32_t RELOAD_DST: 1;
    } b;
} LCDC_DMA_CFG_LOWx_TypeDef;



/* 0x44
   0       R/W    reserved63          0x0
   1       R/W    reserved62          0x0
   2       R      reserved61          0x0
   3       R/W    PROTCTL             0x1
   6:4     R      reserved59          0x0
   10:7    R/W    SRC_PER             0x0
   14:11   R/W    DEST_PER            0x0
   15      R/W    ExtendedSRC_PER1    0x0
   16      R/W    ExtendedDEST_PER1   0x0
   17      R/W    ExtendedSRC_PER2    0x0
   18      R/W    ExtendedDEST_PER2   0x0
   19      R      ExtendedSRC_PER3    0x0
   20      R      ExtendedDEST_PER3   0x0
   31:21   R      reserved50          0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reserved_4: 1;
        __IO uint32_t reserved_3: 1;
        __I uint32_t reserved_2: 1;
        __IO uint32_t PROTCTL: 1;
        __I uint32_t reserved_1: 3;
        __IO uint32_t SRC_PER: 4;
        __IO uint32_t DEST_PER: 4;
        __IO uint32_t ExtendedSRC_PER1: 1;
        __IO uint32_t ExtendedDEST_PER1: 1;
        __IO uint32_t ExtendedSRC_PER2: 1;
        __IO uint32_t ExtendedDEST_PER2: 1;
        __IO uint32_t ExtendedSRC_PER3: 1;
        __IO uint32_t ExtendedDEST_PER3: 1;
        __I uint32_t reserved_0: 11;
    } b;
} LCDC_DMA_CFG_HIGHx_TypeDef;



/* 0x48
   19:0    R/W    SGI                 0x0
   31:20   R/W    SGC                 0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t SGI: 20;
        __IO uint32_t SGC: 12;
    } b;
} LCDC_DMA_SGR_LOW_TypeDef;


/* 0x4C
   15:0    R/W    SGSN                0x0
   31:16   R      reserved68          0x2
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t SGSN: 16;
        __I uint32_t reserved_0: 16;
    } b;
} LCDC_DMA_SGR_HIGH_TypeDef;




/* 0x50
   19:0    R/W    DSI                 0x0
   31:20   R      DSC                 0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t DSI: 20;
        __IO uint32_t DSC: 12;
    } b;
} LCDC_DMA_DSR_LOW_TypeDef;


/* 0x54
   15:0    R/W    DSSN                0x0
   31:16   R      reserved74          0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t DSSN: 16;
        __I uint32_t reserved_0: 16;
    } b;
} LCDC_DMA_DSR_HIGH_TypeDef;



/* 0x2E8
   31:0    R      STATUS              0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t STATUS: 32;
    } b;
} LCDC_DMA_StatusTfr_TypeDef;



/* 0x2F0
   31:0    R      STATUS              0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t STATUS: 32;
    } b;
} LCDC_DMA_StatusBlock_TypeDef;



/* 0x2F8
   31:0    R      STATUS              0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t STATUS: 32;
    } b;
} LCDC_DMA_StatusSrcTran_TypeDef;



/* 0x300
   31:0    R      STATUS              0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t STATUS: 32;
    } b;
} LCDC_DMA_StatusDstTran_TypeDef;



/* 0x308
   31:0    R      STATUS              0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t STATUS: 32;
    } b;
} LCDC_DMA_StatusErr_TypeDef;



/* 0x310
   7:0     R/W    INT_MASK_L          0x0
   15:8    W      INT_MASK_WE_L       0x0
   23:16   R/W    INT_MASK_H          0x0
   31:24   W      INT_MASK_WE_H       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t INT_MASK_L: 8;
        __O uint32_t INT_MASK_WE_L: 8;
        __IO uint32_t INT_MASK_H: 8;
        __O uint32_t INT_MASK_WE_H: 8;
    } b;
} LCDC_DMA_MaskTfr_TypeDef;



/* 0x318
   7:0     R/W    INT_MASK_L          0x0
   15:8    W      INT_MASK_WE_L       0x0
   23:16   R/W    INT_MASK_H          0x0
   31:24   W      INT_MASK_WE_H       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t INT_MASK_L: 8;
        __O uint32_t INT_MASK_WE_L: 8;
        __IO uint32_t INT_MASK_H: 8;
        __O uint32_t INT_MASK_WE_H: 8;
    } b;
} LCDC_DMA_MaskBlock_TypeDef;



/* 0x320
   7:0     R/W    INT_MASK_L          0x0
   15:8    W      INT_MASK_WE_L       0x0
   23:16   R/W    INT_MASK_H          0x0
   31:24   W      INT_MASK_WE_H       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t INT_MASK_L: 8;
        __O uint32_t INT_MASK_WE_L: 8;
        __IO uint32_t INT_MASK_H: 8;
        __O uint32_t INT_MASK_WE_H: 8;
    } b;
} LCDC_DMA_MaskSrcTran_TypeDef;



/* 0x328
   7:0     R/W    INT_MASK_L          0x0
   15:8    W      INT_MASK_WE_L       0x0
   23:16   R/W    INT_MASK_H          0x0
   31:24   W      INT_MASK_WE_H       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t INT_MASK_L: 8;
        __O uint32_t INT_MASK_WE_L: 8;
        __IO uint32_t INT_MASK_H: 8;
        __O uint32_t INT_MASK_WE_H: 8;
    } b;
} LCDC_DMA_MaskDstTran_TypeDef;



/* 0x330
   7:0     R/W    INT_MASK_L          0x0
   15:8    W      INT_MASK_WE_L       0x0
   23:16   R/W    INT_MASK_H          0x0
   31:24   W      INT_MASK_WE_H       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t INT_MASK_L: 8;
        __O uint32_t INT_MASK_WE_L: 8;
        __IO uint32_t INT_MASK_H: 8;
        __O uint32_t INT_MASK_WE_H: 8;
    } b;
} LCDC_DMA_MaskErr_TypeDef;



/* 0x338
   31:0    W      CLEAR               0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __O uint32_t CLEAR: 32;
    } b;
} LCDC_DMA_ClearTfr_TypeDef;



/* 0x340
   31:0    W      CLEAR               0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __O uint32_t CLEAR: 32;
    } b;
} LCDC_DMA_ClearBlock_TypeDef;



/* 0x348
   31:0    W      CLEAR               0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __O uint32_t CLEAR: 32;
    } b;
} LCDC_DMA_ClearSrcTran_TypeDef;



/* 0x350
   31:0    W      CLEAR               0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __O uint32_t CLEAR: 32;
    } b;
} LCDC_DMA_ClearDstTran_TypeDef;



/* 0x358
   31:0    W      CLEAR               0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __O uint32_t CLEAR: 32;
    } b;
} LCDC_DMA_ClearErr_TypeDef;



/* 0x398
   0       R/W    DMAC_EN             0x0
   31:1    R      reserved130         0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t DMAC_EN: 1;
        __I uint32_t reserved_0: 31;
    } b;
} LCDC_DMA_DmaCfgReg_TypeDef;



/* 0x3A0
   7:0     R/W    CH_EN_L             0x0
   15:8    W      CH_EN_WE_L          0x0
   23:16   R/W    CH_EN_H             0x0
   31:24   W      CH_EN_WE_H          0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t CH_EN_L: 8;
        __O uint32_t CH_EN_WE_L: 8;
        __IO uint32_t CH_EN_H: 8;
        __O uint32_t CH_EN_WE_H: 8;
    } b;
} LCDC_DMA_ChEnReg_TypeDef;



/* 0x3B8
   7:0     R/W    OSR                 0x0
   15:8    R/W    OSW                 0x0
   31:16   R      Reserved            0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t OSR: 8;
        __IO uint32_t OSW: 8;
        __I uint32_t reserved_0: 16;
    } b;
} LCDC_DMA_DmaOsNum_TypeDef;


#ifdef  __cplusplus
}
#endif /* __cplusplus */
#endif /* RTL_LCDC_DMA_REG_H */
