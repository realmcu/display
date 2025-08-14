/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* \file     rtl_lcdc_dmaii_def.h
* \brief    LCDC DMAII related definitions for RTL8762G
* \details
* \author
* \date     2023-11-15
* \version  v1.1
* *********************************************************************************************************
*/

#ifndef RTL_DMALL_DEF_H
#define RTL_DMALL_DEF_H

#include "utils/rtl_utils.h"
#include "address_map.h"

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

/*============================================================================*
 *                         DMAII Registers Memory Map
 *============================================================================*/
typedef struct
{
    __IO uint32_t  GRP1_SAR;                             /*!< 0x00 */
    __IO uint32_t  GRP1_DAR;                             /*!< 0x04 */
    __IO uint32_t  GRP1_LLP;                             /*!< 0x08 */
    __IO uint32_t  GRP1_CTL0;                            /*!< 0x0C */
    __IO uint32_t  GRP1_CTL1;                            /*!< 0x10 */
    __IO uint32_t  RSVD1;                                /*!< 0x14 */
    __IO uint32_t  RSVD2;                                /*!< 0x18 */
    __IO uint32_t  RSVD3;                                /*!< 0x1C */
    __IO uint32_t  GRP1_SAR_OFFSET;                      /*!< 0x20 */
    __IO uint32_t  GRP1_DAR_OFFSET;                      /*!< 0x24 */
    __IO uint32_t  GRP2_SAR_OFFSET;                      /*!< 0x28 */
    __IO uint32_t  GRP2_DAR_OFFSET;                      /*!< 0x2C */
    __IO uint32_t  GRP2_SAR;                             /*!< 0x30 */
    __IO uint32_t  GRP2_DAR;                             /*!< 0x34 */
    __IO uint32_t  GRP2_LLP;                             /*!< 0x38 */
    __IO uint32_t  GRP2_CTL0;                            /*!< 0x3C */
    __IO uint32_t  GRP2_CTL1;                            /*!< 0x40 */
    __IO uint32_t  DMA_CFG;                              /*!< 0x44 */
    __IO uint32_t  RSVD4;                                /*!< 0x48 */
    __IO uint32_t  RSVD5;                                /*!< 0x4C */
    __IO uint32_t  GRP1_SAR_DMA;                         /*!< 0x50 */
    __IO uint32_t  GRP1_DAR_DMA;                         /*!< 0x54 */
    __IO uint32_t  GRP1_LLP_DMA;                         /*!< 0x58 */
    __IO uint32_t  GRP1_CTL0_DMA;                        /*!< 0x5C */
    __IO uint32_t  GRP1_CTL1_DMA;                        /*!< 0x60 */
    __IO uint32_t  RSVD6;                                /*!< 0x64 */
    __IO uint32_t  RSVD7;                                /*!< 0x68 */
    __IO uint32_t  RSVD8;                                /*!< 0x6C */
    __IO uint32_t  GRP2_SAR_DMA;                         /*!< 0x70 */
    __IO uint32_t  GRP2_DAR_DMA;                         /*!< 0x74 */
    __IO uint32_t  GRP2_LLP_DMA;                         /*!< 0x78 */
    __IO uint32_t  GRP2_CTL0_DMA;                        /*!< 0x7C */
    __IO uint32_t  GRP2_CTL1_DMA;                        /*!< 0x80 */
    __IO uint32_t  RSVD9;                                /*!< 0x84 */
    __IO uint32_t  RSVD10;                               /*!< 0x88 */
    __IO uint32_t  GRP1_SAR_FOR_INFINITE_MODE;           /*!< 0x8C */
    __IO uint32_t  GRP2_SAR_FOR_INFINITE_MODE;           /*!< 0x90 */
} LCDC_DMA_LinkList_TypeDef;


/*============================================================================*
 *                         DMAII Declaration
 *============================================================================*/

/*============================================================================*
 *                         DMAII Registers and Field Descriptions
 *============================================================================*/
/* 0x00
    31:0    R/W1P  reg_dma_g1_sar                  0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        uint32_t reg_dma_g1_sar: 32;
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
        uint32_t reg_dma_g1_dar: 32;
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
        uint32_t reg_dma_g1_llp: 32;
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
        uint32_t reg_dma_g1_ctl0: 32;
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
        uint32_t reg_dma_g1_ctrl1: 32;
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
        const uint32_t reserved_0: 32;
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
        const uint32_t reserved_0: 32;
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
        const uint32_t reserved_0: 32;
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
        uint32_t reg_dma_g1_sar_offset: 32;
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
        uint32_t reg_dma_g1_dar_offset: 32;
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
        uint32_t reg_dma_g2_sar_offset: 32;
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
        uint32_t reg_dma_g2_dar_offset: 32;
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
        uint32_t reg_dma_g2_sar: 32;
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
        uint32_t reg_dma_g2_dar: 32;
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
        uint32_t reg_dma_g2_llp: 32;
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
        uint32_t reg_dma_g2_ctl0: 32;
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
        uint32_t reg_dma_g2_ctl1: 32;
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
        const uint32_t reserved_0: 30;
        uint32_t reg_dma_g2_wp: 1;
        uint32_t reg_dma_g1_wp: 1;
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
        const uint32_t reserved_0: 32;
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
        const uint32_t reserved_0: 32;
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
        uint32_t reg_dma_g1_sar: 32;
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
        uint32_t reg_dma_g1_dar: 32;
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
        uint32_t reg_dma_g1_llp: 32;
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
        uint32_t reg_dma_g1_ctl0: 32;
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
        uint32_t reg_dma_g1_ctrl1: 32;
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
        const uint32_t reserved_0: 32;
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
        const uint32_t reserved_0: 32;
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
        const uint32_t reserved_0: 32;
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
        uint32_t reg_dma_g2_sar: 32;
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
        uint32_t reg_dma_g2_dar: 32;
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
        uint32_t reg_dma_g2_llp: 32;
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
        uint32_t reg_dma_g2_ctl0: 32;
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
        uint32_t reg_dma_g2_ctl1: 32;
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
        const uint32_t reserved_0: 32;
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
        const uint32_t reserved_0: 32;
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
        uint32_t reg_dma_g1_sar_infinite_mode: 32;
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
        uint32_t reg_dma_g2_sar_infinite_mode: 32;
    } b;
} LCDC_DMALL_GRP2_SAR_FOR_INFINITE_MODE_TypeDef;


#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* RTL_DMALL_DEF_H */
