/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RTL_DCI_REG_H
#define RTL_DCI_REG_H

#ifdef __arm__
#include "address_map.h"
#include "rtl876x.h"
#endif


#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

/*============================================================================*
 *                         DCI Defines
 *============================================================================*/
/* Per address_map.h of RTL87X3K (BBPro4): DCI_BASE 0x40643000, DCI_TOP 0x40643fff.
 * DCI sits inside the display island, next to MIPI (0x40641000), SPI_JDI
 * (0x40642000) and the PNG decoder (0x40644000).
 *
 * NOTE: the DCMI register table quotes 0x40017000. That value belongs to a
 * different SoC's map and must not be used here - on this part it lands in the
 * low peripheral block. The address map is authoritative. */
#define DCI_REG_BASE                  0x40643000UL


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
 *                         DCI Registers Memory Map
 *============================================================================*/
typedef struct
{
    __IO uint32_t CR;                                           /*< 0x00 */
    __I  uint32_t SR;                                           /*< 0x04 */
    __I  uint32_t RIS;                                          /*< 0x08 */
    __IO uint32_t IER;                                          /*< 0x0C */
    __I  uint32_t MIS;                                          /*< 0x10 */
    __IO uint32_t ICR;                                          /*< 0x14 */
    __IO uint32_t ESCR;                                         /*< 0x18 */
    __IO uint32_t ESUR;                                         /*< 0x1C */
    __IO uint32_t CWSTRT;                                       /*< 0x20 */
    __IO uint32_t CWSIZE;                                       /*< 0x24 */
    __I  uint32_t DR;                                           /*< 0x28 */
} DCI_TypeDef;

/*============================================================================*
 *                         DCI Declaration
 *============================================================================*/

/*============================================================================*
 *                         DCI Private Types
 *============================================================================*/

/*============================================================================*
 *                         DCI Registers and Field Descriptions
 *============================================================================*/
/* 0x00
    0        R/WAC    capture                   0x0
    1        R/W      cm                        0x0
    2        R/W      crop                      0x0
    3        R/W      jpeg                      0x0
    4        R/W      ess                       0x0
    5        R/W      pckpol                    0x0
    6        R/W      hspol                     0x0
    7        R/W      vspol                     0x0
    9:8      R/W      fcrc                      0x0
    11:10    R/W      edm                       0x0
    13:12    R        reserved                  0x0
    14       R/W      enable                    0x0
    15       R        reserved                  0x0
    17:16    R/W      bsm                       0x0
    18       R/W      oebs                      0x0
    19       R/W      lsm                       0x0
    20       R/W      oels                      0x0
    31:21    R        reserved                  0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        uint32_t capture: 1;
        uint32_t cm: 1;
        uint32_t crop: 1;
        uint32_t jpeg: 1;
        uint32_t ess: 1;
        uint32_t pckpol: 1;
        uint32_t hspol: 1;
        uint32_t vspol: 1;
        uint32_t fcrc: 2;
        uint32_t edm: 2;
        const uint32_t reserved_2: 2;
            uint32_t enable: 1;
            const uint32_t reserved_1: 1;
            uint32_t bsm: 2;
            uint32_t oebs: 1;
            uint32_t lsm: 1;
            uint32_t oels: 1;
            const uint32_t reserved_0: 11;
        } b;
    } DCI_CR_TypeDef;

    /* 0x04
        0        R        hsync                     0x0
        1        R        vsync                     0x0
        2        R        fne                       0x0
        31:3     R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t hsync: 1;
                const uint32_t vsync: 1;
                const uint32_t fne: 1;
                const uint32_t reserved_0: 29;
            } b;
        } DCI_SR_TypeDef;

    /* 0x08
        0        R        frame_ris                 0x0
        1        R        ovr_ris                   0x0
        2        R        err_ris                   0x0
        3        R        vsync_ris                 0x0
        4        R        line_ris                  0x0
        31:5     R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t frame_ris: 1;
                const uint32_t ovr_ris: 1;
                const uint32_t err_ris: 1;
                const uint32_t vsync_ris: 1;
                const uint32_t line_ris: 1;
                const uint32_t reserved_0: 27;
            } b;
        } DCI_RIS_TypeDef;

    /* 0x0C
        0        R/W      frame_ie                  0x0
        1        R/W      ovr_ie                    0x0
        2        R/W      err_ie                    0x0
        3        R/W      vsync_ie                  0x0
        4        R/W      line_ie                   0x0
        31:5     R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t frame_ie: 1;
                uint32_t ovr_ie: 1;
                uint32_t err_ie: 1;
                uint32_t vsync_ie: 1;
                uint32_t line_ie: 1;
                const uint32_t reserved_0: 27;
            } b;
        } DCI_IER_TypeDef;

    /* 0x10
        0        R        frame_mis                 0x0
        1        R        ovr_mis                   0x0
        2        R        err_mis                   0x0
        3        R        vsync_mis                 0x0
        4        R        line_mis                  0x0
        31:5     R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t frame_mis: 1;
                const uint32_t ovr_mis: 1;
                const uint32_t err_mis: 1;
                const uint32_t vsync_mis: 1;
                const uint32_t line_mis: 1;
                const uint32_t reserved_0: 27;
            } b;
        } DCI_MIS_TypeDef;

    /* 0x14
        0        R/W1S    frame_isc                 0x0
        1        R/W1S    ovr_isc                   0x0
        2        R/W1S    err_isc                   0x0
        3        R/W1S    vsync_isc                 0x0
        4        R/W1S    line_isc                  0x0
        31:5     R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t frame_isc: 1;
                uint32_t ovr_isc: 1;
                uint32_t err_isc: 1;
                uint32_t vsync_isc: 1;
                uint32_t line_isc: 1;
                const uint32_t reserved_0: 27;
            } b;
        } DCI_ICR_TypeDef;

    /* 0x18
        7:0      R/W      fsc                       0x0
        15:8     R/W      lsc                       0x0
        23:16    R/W      lec                       0x0
        31:24    R/W      fec                       0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t fsc: 8;
                uint32_t lsc: 8;
                uint32_t lec: 8;
                uint32_t fec: 8;
            } b;
        } DCI_ESCR_TypeDef;

    /* 0x1C
        7:0      R/W      fsu                       0x0
        15:8     R/W      lsu                       0x0
        23:16    R/W      leu                       0x0
        31:24    R/W      feu                       0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t fsu: 8;
                uint32_t lsu: 8;
                uint32_t leu: 8;
                uint32_t feu: 8;
            } b;
        } DCI_ESUR_TypeDef;

    /* 0x20
        13:0     R/W      hoffcnt                   0x0
        15:14    R        reserved                  0x0
        28:16    R/W      vst                       0x0
        31:29    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t hoffcnt: 14;
                const uint32_t reserved_1: 2;
                uint32_t vst: 13;
                const uint32_t reserved_0: 3;
            } b;
        } DCI_CWSTRT_TypeDef;

    /* 0x24
        13:0     R/W      capcnt                    0x0
        15:14    R        reserved                  0x0
        29:16    R/W      vline                     0x0
        31:30    R        reserved                  0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                uint32_t capcnt: 14;
                const uint32_t reserved_1: 2;
                uint32_t vline: 14;
                const uint32_t reserved_0: 2;
            } b;
        } DCI_CWSIZE_TypeDef;

    /* 0x28
        7:0      R        byte0                     0x0
        15:8     R        byte1                     0x0
        23:16    R        byte2                     0x0
        31:24    R        byte3                     0x0
    */
    typedef union
        {
            uint32_t d32;
            uint8_t d8[4];
            struct
            {
                const uint32_t byte0: 8;
                const uint32_t byte1: 8;
                const uint32_t byte2: 8;
                const uint32_t byte3: 8;
            } b;
        } DCI_DR_TypeDef;


#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* RTL_DCI_REG_H */
