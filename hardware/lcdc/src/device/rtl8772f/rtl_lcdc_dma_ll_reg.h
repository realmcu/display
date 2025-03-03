#ifndef RTL_DMALL_REG_H
#define RTL_DMALL_REG_H

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
    __IO uint32_t  GRP1_SAR;                             //0X00
    __IO uint32_t  GRP1_DAR;                             //0X04
    __IO uint32_t  GRP1_LLP;                             //0X08
    __IO uint32_t  GRP1_CTL0;                            //0X0C
    __IO uint32_t  GRP1_CTL1;                            //0X10
    __IO uint32_t  RSVD1;                                //0X14
    __IO uint32_t  RSVD2;                                //0X18
    __IO uint32_t  RSVD3;                                //0X1C
    __IO uint32_t  GRP1_SAR_OFFSET;                      //0X20
    __IO uint32_t  GRP1_DAR_OFFSET;                      //0X24
    __IO uint32_t  GRP2_SAR_OFFSET;                      //0X28
    __IO uint32_t  GRP2_DAR_OFFSET;                      //0X2C
    __IO uint32_t  GRP2_SAR;                             //0X30
    __IO uint32_t  GRP2_DAR;                             //0X34
    __IO uint32_t  GRP2_LLP;                             //0X38
    __IO uint32_t  GRP2_CTL0;                            //0X3C
    __IO uint32_t  GRP2_CTL1;                            //0X40
    __IO uint32_t  DMA_CFG;                              //0X44
    __IO uint32_t  RSVD4;                                //0X48
    __IO uint32_t  RSVD5;                                //0X4C
    __IO uint32_t  GRP1_SAR_DMA;                         //0X50
    __IO uint32_t  GRP1_DAR_DMA;                         //0X54
    __IO uint32_t  GRP1_LLP_DMA;                         //0X58
    __IO uint32_t  GRP1_CTL0_DMA;                        //0X5C
    __IO uint32_t  GRP1_CTL1_DMA;                        //0X60
    __IO uint32_t  RSVD6;                                //0X64
    __IO uint32_t  RSVD7;                                //0X68
    __IO uint32_t  RSVD8;                                //0X6C
    __IO uint32_t  GRP2_SAR_DMA;                         //0X70
    __IO uint32_t  GRP2_DAR_DMA;                         //0X74
    __IO uint32_t  GRP2_LLP_DMA;                         //0X78
    __IO uint32_t  GRP2_CTL0_DMA;                        //0X7C
    __IO uint32_t  GRP2_CTL1_DMA;                        //0X80
    __IO uint32_t  RSVD9;                                //0X84
    __IO uint32_t  RSVD10;                               //0X88
    __IO uint32_t  GRP1_SAR_FOR_INFINITE_MODE;           //0X8C
    __IO uint32_t  GRP2_SAR_FOR_INFINITE_MODE;           //0X90
} LCDC_DMA_LinkList_TypeDef;

/* 0x00
    31:0    R/W1P  reg_dma_g1_sar                  0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g1_sar: 32;
    } b;
} LCDC_DMALL_GRP1_SAR_TypeDef;



/* 0x04
    31:0    R/W1P  reg_dma_g1_dar                  0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g1_dar: 32;
    } b;
} LCDC_DMALL_GRP1_DAR_TypeDef;



/* 0x08
    31:0    R/W1P  reg_dma_g1_llp                  0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g1_llp: 32;
    } b;
} LCDC_DMALL_GRP1_LLP_TypeDef;



/* 0x0C
    31:0    R/W1P  reg_dma_g1_ctl0                 0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g1_ctl0: 32;
    } b;
} LCDC_DMALL_GRP1_CTL0_TypeDef;



/* 0x10
    31:0    R/W1P  reg_dma_g1_ctrl1                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g1_ctrl1: 32;
    } b;
} LCDC_DMALL_GRP1_CTL1_TypeDef;



/* 0x14
    31:0    R      reserved                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_0: 32;
    } b;
} LCDC_DMALL_RSVD1_TypeDef;



/* 0x18
    31:0    R      reserved                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_0: 32;
    } b;
} LCDC_DMALL_RSVD2_TypeDef;



/* 0x1C
    31:0    R      reserved                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_0: 32;
    } b;
} LCDC_DMALL_RSVD3_TypeDef;



/* 0x20
    31:0    R/W    reg_dma_g1_sar_offset           0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g1_sar_offset: 32;
    } b;
} LCDC_DMALL_GRP1_SAR_OFFSET_TypeDef;



/* 0x24
    31:0    R/W    reg_dma_g1_dar_offset           0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g1_dar_offset: 32;
    } b;
} LCDC_DMALL_GRP1_DAR_OFFSET_TypeDef;



/* 0x28
    31:0    R/W    reg_dma_g2_sar_offset           0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g2_sar_offset: 32;
    } b;
} LCDC_DMALL_GRP2_SAR_OFFSET_TypeDef;



/* 0x2C
    31:0    R/W    reg_dma_g2_dar_offset           0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g2_dar_offset: 32;
    } b;
} LCDC_DMALL_GRP2_DAR_OFFSET_TypeDef;



/* 0x30
    31:0    R/W1P  reg_dma_g2_sar                  0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g2_sar: 32;
    } b;
} LCDC_DMALL_GRP2_SAR_TypeDef;



/* 0x34
    31:0    R/W1P  reg_dma_g2_dar                  0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g2_dar: 32;
    } b;
} LCDC_DMALL_GRP2_DAR_TypeDef;



/* 0x38
    31:0    R/W1P  reg_dma_g2_llp                  0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g2_llp: 32;
    } b;
} LCDC_DMALL_GRP2_LLP_TypeDef;



/* 0x3C
    31:0    R/W1P  reg_dma_g2_ctl0                 0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g2_ctl0: 32;
    } b;
} LCDC_DMALL_GRP2_CTL0_TypeDef;



/* 0x40
    31:0    R/W1P  reg_dma_g2_ctl1                 0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g2_ctl1: 32;
    } b;
} LCDC_DMALL_GRP2_CTL1_TypeDef;



/* 0x44
    29:0    R      reserved                        0x0
    30      R/W    reg_dma_g2_wp                   0x0
    31      R/W    reg_dma_g1_wp                   0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_0: 30;
        __IO uint32_t reg_dma_g2_wp: 1;
        __IO uint32_t reg_dma_g1_wp: 1;
    } b;
} LCDC_DMALL_DMA_CFG_TypeDef;



/* 0x48
    31:0    R      reserved                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_0: 32;
    } b;
} LCDC_DMALL_RSVD4_TypeDef;



/* 0x4C
    31:0    R      reserved                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_0: 32;
    } b;
} LCDC_DMALL_RSVD5_TypeDef;



/* 0x50
    31:0    R/W1P  reg_dma_g1_sar                  0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g1_sar: 32;
    } b;
} LCDC_DMALL_GRP1_SAR_DMA_TypeDef;



/* 0x54
    31:0    R/W1P  reg_dma_g1_dar                  0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g1_dar: 32;
    } b;
} LCDC_DMALL_GRP1_DAR_DMA_TypeDef;



/* 0x58
    31:0    R/W1P  reg_dma_g1_llp                  0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g1_llp: 32;
    } b;
} LCDC_DMALL_GRP1_LLP_DMA_TypeDef;



/* 0x5C
    31:0    R/W1P  reg_dma_g1_ctl0                 0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g1_ctl0: 32;
    } b;
} LCDC_DMALL_GRP1_CTL0_DMA_TypeDef;



/* 0x60
    31:0    R/W1P  reg_dma_g1_ctrl1                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g1_ctrl1: 32;
    } b;
} LCDC_DMALL_GRP1_CTL1_DMA_TypeDef;



/* 0x64
    31:0    R      reserved                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_0: 32;
    } b;
} LCDC_DMALL_RSVD6_TypeDef;



/* 0x68
    31:0    R      reserved                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_0: 32;
    } b;
} LCDC_DMALL_RSVD7_TypeDef;



/* 0x6C
    31:0    R      reserved                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_0: 32;
    } b;
} LCDC_DMALL_RSVD8_TypeDef;



/* 0x70
    31:0    R/W1P  reg_dma_g2_sar                  0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g2_sar: 32;
    } b;
} LCDC_DMALL_GRP2_SAR_DMA_TypeDef;



/* 0x74
    31:0    R/W1P  reg_dma_g2_dar                  0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g2_dar: 32;
    } b;
} LCDC_DMALL_GRP2_DAR_DMA_TypeDef;



/* 0x78
    31:0    R/W1P  reg_dma_g2_llp                  0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g2_llp: 32;
    } b;
} LCDC_DMALL_GRP2_LLP_DMA_TypeDef;



/* 0x7C
    31:0    R/W1P  reg_dma_g2_ctl0                 0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g2_ctl0: 32;
    } b;
} LCDC_DMALL_GRP2_CTL0_DMA_TypeDef;



/* 0x80
    31:0    R/W1P  reg_dma_g2_ctl1                 0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g2_ctl1: 32;
    } b;
} LCDC_DMALL_GRP2_CTL1_DMA_TypeDef;



/* 0x84
    31:0    R      reserved                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_0: 32;
    } b;
} LCDC_DMALL_RSVD9_TypeDef;



/* 0x88
    31:0    R      reserved                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_0: 32;
    } b;
} LCDC_DMALL_RSVD10_TypeDef;



/* 0x8C
    31:0    R/W    reg_dma_g1_sar_infinite_mode    0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g1_sar_infinite_mode: 32;
    } b;
} LCDC_DMALL_GRP1_SAR_FOR_INFINITE_MODE_TypeDef;



/* 0x90
    31:0    R/W    reg_dma_g2_sar_infinite_mode    0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_dma_g2_sar_infinite_mode: 32;
    } b;
} LCDC_DMALL_GRP2_SAR_FOR_INFINITE_MODE_TypeDef;


#ifdef  __cplusplus
}
#endif /* __cplusplus */
#endif /* RTL_DMALL_REG_H */
