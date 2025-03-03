#ifndef RTL_DBIB_REG_H
#define RTL_DBIB_REG_H

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
    __IO uint32_t  DBIB_CTRL0;           //0X00
    __IO uint32_t  DBIB_CFG;             //0X04
    __IO uint32_t  DBIB_RXDATA;          //0X08
    __IO uint32_t  DBIB_CMD_REG1;        //0X0C
    __IO uint32_t  DBIB_CMD_REG2;        //0X10
    __IO uint32_t  DBIB_CMD_REG3;        //0X14
} LCDC_DBIB_TypeDef;

/* 0x00
    3:0     R      reserved5               0x0
    9:4     R/W    speed_sel               0x0
    10      R      reserved4               0x0
    11      R/W    bypass_guard_time       0x1
    13:12   R      reserved3               0x0
    15:14   R/W    guard_time              0x0
    16      R/W    bypass_cmd_guard_time   0x1
    18:17   R/W    cmd_guard_time          0x0
    19      R/W    bypass_init_guard_time  0x1
    23:20   R/W    init_guard_time         0x0
    28:24   R      reserved2               0x0
    29      R/W    bypass_cmd              0x0
    31:30   R      reserved1               0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_4: 4;
        __IO uint32_t speed_sel: 6;
        __I uint32_t reserved_3: 1;
        __IO uint32_t bypass_guard_time: 1;
        __I uint32_t reserved_2: 2;
        __IO uint32_t guard_time: 2;
        __IO uint32_t bypass_cmd_guard_time: 1;
        __IO uint32_t cmd_guard_time: 2;
        __IO uint32_t bypass_init_guard_time: 1;
        __IO uint32_t init_guard_time: 4;
        __I uint32_t reserved_1: 5;
        __IO uint32_t bypass_cmd: 1;
        __I uint32_t reserved_0: 2;
    } b;
} DBIB_CTRL0_TypeDef;



/* 0x04
    3:0     R/W    reg_cmd_num             0x0
    7:4     R      reserved2               0x0
    8       R/W    reg_cs_manual           0x1
    9       R/W    reg_d_cb_manual         0x0
    10      R/W1C  reg_wr_trig_manual      0x0
    11      R/W1C  reg_rd_trig_manual      0x0
    29:12   R      reserved1               0x0
    31:30   R/W    reg_wr_trig_sel         0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_cmd_num: 4;
        __I uint32_t reserved_1: 4;
        __IO uint32_t reg_cs_manual: 1;
        __IO uint32_t reg_d_cb_manual: 1;
        __IO uint32_t reg_wr_trig_manual: 1;
        __IO uint32_t reg_rd_trig_manual: 1;
        __I uint32_t reserved_0: 18;
        __IO uint32_t reg_wr_trig_sel: 2;
    } b;
} DBIB_CFG_TypeDef;



/* 0x08
    7:0     R      rdata                   0x0
    8       R      rdata_valid             0x0
    31:9    R      reserved                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t rdata: 8;
        __I uint32_t rdata_valid: 1;
        __I uint32_t reserved_0: 23;
    } b;
} DBIB_RXDATA_TypeDef;



/* 0x0C
    7:0     R/W    reg_cmd_1               0x0
    15:8    R/W    reg_cmd_2               0x0
    23:16   R/W    reg_cmd_3               0x0
    31:24   R/W    reg_cmd_4               0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_cmd_1: 8;
        __IO uint32_t reg_cmd_2: 8;
        __IO uint32_t reg_cmd_3: 8;
        __IO uint32_t reg_cmd_4: 8;
    } b;
} DBIB_CMD_REG1_TypeDef;



/* 0x10
    7:0     R/W    reg_cmd_5               0x0
    15:8    R/W    reg_cmd_6               0x0
    23:16   R/W    reg_cmd_7               0x0
    31:24   R/W    reg_cmd_8               0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_cmd_5: 8;
        __IO uint32_t reg_cmd_6: 8;
        __IO uint32_t reg_cmd_7: 8;
        __IO uint32_t reg_cmd_8: 8;
    } b;
} DBIB_CMD_REG2_TypeDef;



/* 0x14
    7:0     R/W    reg_cmd_9               0x0
    15:8    R/W    reg_cmd_10              0x0
    23:16   R/W    reg_cmd_11              0x0
    31:24   R/W    reg_cmd_12              0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t reg_cmd_9: 8;
        __IO uint32_t reg_cmd_10: 8;
        __IO uint32_t reg_cmd_11: 8;
        __IO uint32_t reg_cmd_12: 8;
    } b;
} DBIB_CMD_REG3_TypeDef;


#ifdef  __cplusplus
}
#endif /* __cplusplus */
#endif /* RTL_DBIB_REG_H */
