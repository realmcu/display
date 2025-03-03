#ifndef RTL_Handler_REG_H
#define RTL_Handler_REG_H

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
    __IO uint32_t  INTERFACE_SELECT;                                                             //0X00
    __IO uint32_t  FT_IN;                                                                        //0X04
    __IO uint32_t  FT_OUT;                                                                       //0X08
    __IO uint32_t  BIT_SWAP;                                                                     //0X0C
    __IO uint32_t  TEAR_CTR;                                                                     //0X10
    __IO uint32_t  OPERATE_CTR;                                                                  //0X14
    __IO uint32_t  DMA_FIFO_CTRL;                                                                //0X18
    __IO uint32_t  DMA_FIFO_IMR;                                                                 //0X1C
    __IO uint32_t  DMA_FIFO_SR;                                                                  //0X20
    __IO uint32_t  DMA_FIFO_ICR;                                                                 //0X24
    __IO uint32_t  TX_LEN;                                                                       //0X28
    __IO uint32_t  TX_CNT;                                                                       //0X2C
    __IO uint32_t  RSVD1;                                                                        //0X30
    __IO uint32_t  RX_LEN;                                                                       //0X34
    __IO uint32_t  RX_CNT;                                                                       //0X38
    __IO uint32_t  DMA_FIFO;                                                                     //0X3C
    __IO uint32_t  AXI_MUX_MODE;                                                                 //0X40
    __IO uint32_t  SPIC_CMD;                                                                     //0X44
    __IO uint32_t  SPIC_ADDR;                                                                    //0X48
    __IO uint32_t  DMA_MULTI_BLOCK_CTRL;                                                         //0X4C
    __IO uint32_t  DMA_MULTI_BLOCK_SIZE1;                                                        //0X50
    __IO uint32_t  DMA_MULTI_BLOCK_SIZE2;                                                        //0X54
    __IO uint32_t  DMA_FIFO_OFFSET;                                                              //0X58
    __IO uint32_t  DPHY_STOP_STATE_TIMEOUT;                                                      //0X5C
    __IO uint32_t  SPIC_SCLK_PHY;                                                                //0X60
    __IO uint32_t  INFINITE_MODE_CTR;                                                            //0X64
    __IO uint32_t  SPIC_RX_FIFO_READ_SIZE;                                                       //0X68
    __IO uint32_t  SPIC_TX_DUMMY;                                                                //0X6C
    __IO uint32_t  RSVD2;                                                                        //0X70
    __IO uint32_t  RSVD3;                                                                        //0X74
    __IO uint32_t  RSVD4;                                                                        //0X78
    __IO uint32_t  RSVD5;                                                                        //0X7C
} LCDC_Handler_TypeDef;

/* 0x00
    0       R/W    display_controller_enable                       0x0
    2:1     R/W    interface_select                                0x0
    3       R/W    reset_lcd_display_module                        0x1
    7:4     R      reserved3                                       0x0
    8       R/W    ramless_qspi_enable                             0x0
    15:9    R      reserved2                                       0x0
    17:16   R/W    group_sel                                       0x0
    31:18   R      reserved1                                       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t display_controller_enable: 1;
        __IO uint32_t interface_select: 2;
        __IO uint32_t reset_lcd_display_module: 1;
        __I uint32_t reserved_2: 4;
        __IO uint32_t ramless_qspi_enable: 1;
        __I uint32_t reserved_1: 7;
        __IO uint32_t group_sel: 2;
        __I uint32_t reserved_0: 14;
    } b;
} LCDC_HANDLER_INTERFACE_SELECT_TypeDef;



/* 0x04
    2:0     R/W    input_format                                    0x0
    31:3    R      reserved                                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t input_format: 3;
        __I uint32_t reserved_0: 29;
    } b;
} LCDC_HANDLER_FT_IN_TypeDef;



/* 0x08
    2:0     R/W    output_format                                   0x0
    31:3    R      reserved                                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t output_format: 3;
        __I uint32_t reserved_0: 29;
    } b;
} LCDC_HANDLER_FT_OUT_TypeDef;



/* 0x0C
    1:0     R/W    bit_swap                                        0x0
    31:2    R      reserved                                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t bit_swap: 2;
        __I uint32_t reserved_0: 30;
    } b;
} LCDC_HANDLER_BIT_SWAP_TypeDef;



/* 0x10
    0       R/W    tear_auto_turn_on_autowritestart                0x0
    1       R/W    tear_auto_turn_on_dma_en                        0x1
    2       R/W    tear_polarity                                   0x0
    3       R      tear                                            0x0
    4       R/W    tear_input_mux                                  0x0
    5       R/W    tear_logic_enable                               0x0
    15:6    R      reserved2                                       0x0
    27:16   R/W    t2w_delay                                       0x0
    30:28   R      reserved1                                       0x0
    31      R/W    bypass_t2w_delay                                0x1
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t tear_auto_turn_on_autowritestart: 1;
        __IO uint32_t tear_auto_turn_on_dma_en: 1;
        __IO uint32_t tear_polarity: 1;
        __I uint32_t tear: 1;
        __IO uint32_t tear_input_mux: 1;
        __IO uint32_t tear_logic_enable: 1;
        __I uint32_t reserved_1: 10;
        __IO uint32_t t2w_delay: 12;
        __I uint32_t reserved_0: 3;
        __IO uint32_t bypass_t2w_delay: 1;
    } b;
} LCDC_HANDLER_TEAR_CTR_TypeDef;



/* 0x14
    0       R/W    access_mode                                     0x1
    1       R/W    data_rw                                         0x0
    29:2    R      reserved                                        0x0
    30      R/W    auto_read_start                                 0x0
    31      R/W    auto_write_start                                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t access_mode: 1;
        __IO uint32_t data_rw: 1;
        __I uint32_t reserved_0: 28;
        __IO uint32_t auto_read_start: 1;
        __IO uint32_t auto_write_start: 1;
    } b;
} LCDC_HANDLER_OPERATE_CTR_TypeDef;



/* 0x18
    0       R/W    dma_enable                                      0x0
    7:1     R      reserved2                                       0x0
    8       R/W    dma_burst_request_only                          0x0
    15:9    R      reserved1                                       0x0
    31:16   R/W    dma_fifo_threshold                              0x8
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t dma_enable: 1;
        __I uint32_t reserved_1: 7;
        __IO uint32_t dma_burst_request_only: 1;
        __I uint32_t reserved_0: 7;
        __IO uint32_t dma_fifo_threshold: 16;
    } b;
} LCDC_HANDLER_DMA_FIFO_CTRL_TypeDef;



/* 0x1C
    4:0     R      reserved3                                       0x0
    5       R/W    tx_fifo_threshold_int_msk                       0x1
    6       R/W    tx_fifo_overflow_int_msk                        0x1
    7       R/W    tx_fifo_empty_int_msk                           0x1
    8       R/W    tx_auto_done_int_msk                            0x1
    9       R/W    rx_fifo_overflow_int_msk                        0x1
    10      R/W    rx_auto_done_int_msk                            0x1
    11      R/W    tear_trigger_int_msk                            0x1
    29:12   R      reserved2                                       0x0
    30      R/W    display_controller_waveform_finish_int_msk      0x0
    31      R      reserved1                                       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_2: 5;
        __IO uint32_t tx_fifo_threshold_int_msk: 1;
        __IO uint32_t tx_fifo_overflow_int_msk: 1;
        __IO uint32_t tx_fifo_empty_int_msk: 1;
        __IO uint32_t tx_auto_done_int_msk: 1;
        __IO uint32_t rx_fifo_overflow_int_msk: 1;
        __IO uint32_t rx_auto_done_int_msk: 1;
        __IO uint32_t tear_trigger_int_msk: 1;
        __I uint32_t reserved_1: 18;
        __IO uint32_t display_controller_waveform_finish_int_msk: 1;
        __I uint32_t reserved_0: 1;
    } b;
} LCDC_HANDLER_DMA_FIFO_IMR_TypeDef;



/* 0x20
    0       R      reserved4                                       0x0
    1       R      tx_fifo_threshold_int_flag                      0x0
    2       R      tx_fifo_overflow_int_flag                       0x0
    3       R      tx_fifo_empty_int_flag                          0x1
    4       R      tx_fifo_full_status                             0x0
    5       R      tx_fifo_empty_status                            0x1
    6       R      tx_auto_done_int                                0x0
    15:7    R      reserved3                                       0x0
    16      R      rx_fifo_overflow_int                            0x0
    17      R      rx_auto_done_int                                0x0
    18      R      rx_fifo_full_status                             0x0
    19      R      rx_fifo_empty_status                            0x1
    20      R      tear_trigger_int_status                         0x0
    29:21   R      reserved2                                       0x0
    30      R      display_controller_waveform_finish_int_status   0x0
    31      R      reserved1                                       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_3: 1;
        __I uint32_t tx_fifo_threshold_int_flag: 1;
        __I uint32_t tx_fifo_overflow_int_flag: 1;
        __I uint32_t tx_fifo_empty_int_flag: 1;
        __I uint32_t tx_fifo_full_status: 1;
        __I uint32_t tx_fifo_empty_status: 1;
        __I uint32_t tx_auto_done_int: 1;
        __I uint32_t reserved_2: 9;
        __I uint32_t rx_fifo_overflow_int: 1;
        __I uint32_t rx_auto_done_int: 1;
        __I uint32_t rx_fifo_full_status: 1;
        __I uint32_t rx_fifo_empty_status: 1;
        __I uint32_t tear_trigger_int_status: 1;
        __I uint32_t reserved_1: 9;
        __I uint32_t display_controller_waveform_finish_int_status: 1;
        __I uint32_t reserved_0: 1;
    } b;
} LCDC_HANDLER_DMA_FIFO_SR_TypeDef;



/* 0x24
    0       R/W1C  fifo_clr                                        0x0
    1       R      reserved3                                       0x0
    2       R/W1C  tx_output_cnt_clr                               0x0
    3       R/W1C  tx_fifo_threshold_int_clr                       0x0
    4       R/W1C  tx_fifo_overflow_int_clr                        0x0
    5       R/W1C  tx_fifo_empty_int_clr                           0x0
    6       R/W1C  tx_auto_done_int_clr                            0x0
    7       R/W1C  rx_output_cnt_clr                               0x0
    8       R/W1C  rx_fifo_overflow_int_clr                        0x0
    9       R/W1C  rx_auto_done_int_clr                            0x0
    10      R/W1C  tear_trigger_int_clr                            0x0
    29:11   R      reserved2                                       0x0
    30      R/W1C  display_controller_waveform_finish_int_clr      0x0
    31      R      reserved1                                       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t fifo_clr: 1;
        __I uint32_t reserved_2: 1;
        __IO uint32_t tx_output_cnt_clr: 1;
        __IO uint32_t tx_fifo_threshold_int_clr: 1;
        __IO uint32_t tx_fifo_overflow_int_clr: 1;
        __IO uint32_t tx_fifo_empty_int_clr: 1;
        __IO uint32_t tx_auto_done_int_clr: 1;
        __IO uint32_t rx_output_cnt_clr: 1;
        __IO uint32_t rx_fifo_overflow_int_clr: 1;
        __IO uint32_t rx_auto_done_int_clr: 1;
        __IO uint32_t tear_trigger_int_clr: 1;
        __I uint32_t reserved_1: 19;
        __IO uint32_t display_controller_waveform_finish_int_clr: 1;
        __I uint32_t reserved_0: 1;
    } b;
} LCDC_HANDLER_DMA_FIFO_ICR_TypeDef;



/* 0x28
    31:0    R/W    tx_output_pixel_num                             0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t tx_output_pixel_num: 32;
    } b;
} LCDC_HANDLER_TX_LEN_TypeDef;



/* 0x2C
    31:0    R      tx_output_pixel_cnt                             0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t tx_output_pixel_cnt: 32;
    } b;
} LCDC_HANDLER_TX_CNT_TypeDef;



/* 0x30
    31:0    R      reserved                                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_0: 32;
    } b;
} LCDC_HANDLER_RSVD1_TypeDef;



/* 0x34
    31:0    R/W    rx_read_byte_num                                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t rx_read_byte_num: 32;
    } b;
} LCDC_HANDLER_RX_LEN_TypeDef;



/* 0x38
    31:0    R      rx_read_cnt                                     0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t rx_read_cnt: 32;
    } b;
} LCDC_HANDLER_RX_CNT_TypeDef;



/* 0x3C
    31:0    R/W    fifo_data                                       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t fifo_data: 32;
    } b;
} LCDC_HANDLER_DMA_FIFO_TypeDef;



/* 0x40
    0       R/W    axi_mux_mode                                    0x0
    31:1    R      reserved                                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t axi_mux_mode: 1;
        __I uint32_t reserved_0: 31;
    } b;
} LCDC_HANDLER_AXI_MUX_MODE_TypeDef;



/* 0x44
    23:0    R/W    spic_cmd                                        0x0
    31:24   R      reserved                                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t spic_cmd: 24;
        __I uint32_t reserved_0: 8;
    } b;
} LCDC_HANDLER_SPIC_CMD_TypeDef;



/* 0x48
    31:0    R/W    spic_addr                                       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t spic_addr: 32;
    } b;
} LCDC_HANDLER_SPIC_ADDR_TypeDef;



/* 0x4C
    0       R/W    dma_multi_block_en                              0x0
    7:1     R      reserved3                                       0x0
    8       R/W    link_list_enable                                0x0
    15:9    R      reserved2                                       0x0
    26:16   R      dma_load_cnt                                    0x0
    30:27   R      reserved1                                       0x0
    31      W1C    reg_dma_load_cnt_clr                            0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t dma_multi_block_en: 1;
        __I uint32_t reserved_2: 7;
        __IO uint32_t link_list_enable: 1;
        __I uint32_t reserved_1: 7;
        __I uint32_t dma_load_cnt: 11;
        __I uint32_t reserved_0: 4;
        __IO uint32_t reg_dma_load_cnt_clr: 1;
    } b;
} LCDC_HANDLER_DMA_MULTI_BLOCK_CTRL_TypeDef;



/* 0x50
    31:0    R/W    block_size_group_1                              0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t block_size_group_1: 32;
    } b;
} LCDC_HANDLER_DMA_MULTI_BLOCK_SIZE1_TypeDef;



/* 0x54
    31:0    R/W    block_size_group_2                              0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t block_size_group_2: 32;
    } b;
} LCDC_HANDLER_DMA_MULTI_BLOCK_SIZE2_TypeDef;



/* 0x58
    15:0    R      tx_fifo_offset                                  0x0
    31:16   R      rx_fifo_offset                                  0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t tx_fifo_offset: 16;
        __I uint32_t rx_fifo_offset: 16;
    } b;
} LCDC_HANDLER_DMA_FIFO_OFFSET_TypeDef;



/* 0x5C
    31:0    R/W    dphy_stop_state_timeout                         0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t dphy_stop_state_timeout: 32;
    } b;
} LCDC_HANDLER_DPHY_STOP_STATE_TIMEOUT_TypeDef;



/* 0x60
    1:0     R/W    spi_sclk_phase_shift                            0x0
    7:2     R      reserved2                                       0x0
    15:8    R/W    r_clko_dly_se                                   0x0
    31:16   R      reserved1                                       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t spi_sclk_phase_shift: 2;
        __I uint32_t reserved_1: 6;
        __IO uint32_t r_clko_dly_se: 8;
        __I uint32_t reserved_0: 16;
    } b;
} LCDC_HANDLER_SPIC_SCLK_PHY_TypeDef;



/* 0x64
    11:0    R/W    infinite_f2f_dmaen2wen_delay                    0x0
    15:12   R      reserved2                                       0x0
    27:16   R/W    infinite_f2f_stop2dmaen_delay                   0x0
    30:28   R      reserved1                                       0x0
    31      R/W    infinite_mode_en                                0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t infinite_f2f_dmaen2wen_delay: 12;
        __I uint32_t reserved_1: 4;
        __IO uint32_t infinite_f2f_stop2dmaen_delay: 12;
        __I uint32_t reserved_0: 3;
        __IO uint32_t infinite_mode_en: 1;
    } b;
} LCDC_HANDLER_INFINITE_MODE_CTR_TypeDef;



/* 0x68
    1:0     R      spic_rx_fifo_read_size                          0x0
    31:2    R      reserved                                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t spic_rx_fifo_read_size: 2;
        __I uint32_t reserved_0: 30;
    } b;
} LCDC_HANDLER_SPIC_RX_FIFO_READ_SIZE_TypeDef;



/* 0x6C
    7:0     R/W    spic_dummy_byte                                 0x0
    15:8    R      reserved2                                       0x0
    23:16   R/W    spic_dummy_byte_num                             0x0
    31:24   R      reserved1                                       0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t spic_dummy_byte: 8;
        __I uint32_t reserved_1: 8;
        __IO uint32_t spic_dummy_byte_num: 8;
        __I uint32_t reserved_0: 8;
    } b;
} LCDC_HANDLER_SPIC_TX_DUMMY_TypeDef;



/* 0x70
    31:0    R      reserved                                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_0: 32;
    } b;
} LCDC_HANDLER_RSVD2_TypeDef;



/* 0x74
    31:0    R      reserved                                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_0: 32;
    } b;
} LCDC_HANDLER_RSVD3_TypeDef;



/* 0x78
    31:0    R      reserved                                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_0: 32;
    } b;
} LCDC_HANDLER_RSVD4_TypeDef;



/* 0x7C
    31:0    R      reserved                                        0x0
*/
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved_0: 32;
    } b;
} LCDC_HANDLER_RSVD5_TypeDef;


#ifdef  __cplusplus
}
#endif /* __cplusplus */
#endif /* RTL_Handler_REG_H */
