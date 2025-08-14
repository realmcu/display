/**
*********************************************************************************************************
*               Copyright(c) 2021, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     rtl_segcom_def.h
* @brief    This file provides SEGCOM registers.
* @details
* @author   astor zhang
* @date     2022-07-29
* @version  v0.1
*********************************************************************************************************
*/

#ifndef RTL_SEGCOM_DEF_H
#define RTL_SEGCOM_DEF_H

#include "utils/rtl_utils.h"
#include "address_map.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                         SEGCOM Declaration
 *============================================================================*/
/** \defgroup 87X2G_SEGCOM        SEGCOM
  * \brief
  * \{
  */

/** \defgroup SEGCOM_Declaration SEGCOM Declaration
  * \brief
  * \{
  */

#define SEGCOM                  ((SEGCOM_CR_TypeDef*)SEGCOM_CTL_REG_BASE)
typedef struct
{
    __IO uint32_t LCD_CR;
    __IO uint32_t LCD_FCR;
    __IO uint32_t LCD_SR;
    __I  uint32_t rsvd[2];
    __IO uint32_t LCD_RAM_COM0;
    __IO uint32_t LCD_RAM_COM1;
    __IO uint32_t LCD_RAM_COM2;
    __IO uint32_t LCD_RAM_COM3;
} SEGCOM_CR_TypeDef;
/** End of SEGCOM_Declaration
  * \}
  */

/*============================================================================*
 *                         SEGCOM Registers and Field Descriptions
 *============================================================================*/
/* Register: SEGCOM_LCD_CR -----------------------------------------------------*/
/* Description: LCD control register. Offset: 0x0000. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        uint32_t en_lcd:     1;
        uint32_t update:     1;
        uint32_t duty:       2;
        uint32_t bias:       2;
        const  uint32_t rsvd:       26;
    } b;
} SEGCOM_LCD_CR_TypeDef;

/* Register: SEGCOM_LCD_FCR -----------------------------------------------------*/
/* Description: LCD frame control register. Offset: 0x0004. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        uint32_t en_res_sel:     1;
        uint32_t res_l_en_set:   1;
        uint32_t pon:            3;
        uint32_t ps:             4;
        uint32_t div:            4;
        uint32_t blinkf:         3;
        uint32_t blink:          2;
        uint32_t dead:           3;
        uint32_t sofie:          1;
        uint32_t uddie:          1;
        const  uint32_t rsvd:           9;
    } b;
} SEGCOM_LCD_FCR_TypeDef;

/* Register: SEGCOM_LCD_SR -----------------------------------------------------*/
/* Description: LCD status register(Include  clear bit). Offset: 0x0008. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        const  uint32_t ens:      1;
        const  uint32_t sof:      1;
        uint32_t udr:      1;
        const  uint32_t udd:      1;
        const  uint32_t rsvd2:    1;
        const  uint32_t fcrsf:    1;
        uint32_t sofc:     1;
        uint32_t uddc:     1;
        const  uint32_t rsvd:     24;
    } b;
} SEGCOM_LCD_SR_TypeDef;

/* Register: SEGCOM_RAM_COM0 -----------------------------------------------------*/
/* Description: . Offset: 0x0014. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        uint32_t lcd_segcom_data:     32;
    } b;
} SEGCOM_RAM_COM0_TypeDef;

/* Register: SEGCOM_RAM_COM1 -----------------------------------------------------*/
/* Description: . Offset: 0x0018. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        uint32_t lcd_segcom_data:     32;
    } b;
} SEGCOM_RAM_COM1_TypeDef;

/* Register: SEGCOM_RAM_COM2 -----------------------------------------------------*/
/* Description: . Offset: 0x001c. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        uint32_t lcd_segcom_data:     32;
    } b;
} SEGCOM_RAM_COM2_TypeDef;

/* Register: SEGCOM_RAM_COM3 -----------------------------------------------------*/
/* Description: . Offset: 0x0020. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        uint32_t lcd_segcom_data:     32;
    } b;
} SEGCOM_RAM_COM3_TypeDef;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* RTL_SEGCOM_DEF_H */
