/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "rtl_lcdc_dbic.h"

/*============================================================================*
 *                           Public Functions
 *============================================================================*/
void DBIC_Init(LCDC_DBICCfgTypeDef *DBIC_InitStruct)
{
    LCDC_AXIMUXMode(LCDC_FW_MODE);

//    DBIC->SSIENR &= DBIC_EN_CLR;//disable
    /* set cs and baudrate */
    DBIC_CTRLR0_TypeDef dbic_reg_0x00 = {.d32 = DBIC->CTRLR0};
    dbic_reg_0x00.b.user_mode = DBIC_USER_MODE;
    dbic_reg_0x00.b.scpol = DBIC_InitStruct->SCPOL;
    dbic_reg_0x00.b.scph = DBIC_InitStruct->SCPH;
    DBIC->CTRLR0 = dbic_reg_0x00.d32;

    DBIC_Select();

    DBIC_ICR_TypeDef dbic_reg_0x48 = {.d32 = DBIC->ICR};
    dbic_reg_0x48.b.icr = 0;
    DBIC->ICR = dbic_reg_0x48.d32;

    DBIC_TXFTLR_TypeDef dbic_reg_0x18 = {.d32 = DBIC->TXFTLR};
    dbic_reg_0x18.b.tft = DBIC_InitStruct->DBIC_TxThr;
    DBIC->TXFTLR = dbic_reg_0x18.d32;

    DBIC_RXFTLR_TypeDef dbic_reg_0x1c = {.d32 = DBIC->RXFTLR};
    dbic_reg_0x1c.b.rft = DBIC_InitStruct->DBIC_RxThr;
    DBIC->RXFTLR = dbic_reg_0x1c.d32;

    DBIC_BAUDR_TypeDef dbic_reg_0x14 = {.d32 = DBIC->BAUDR};
    dbic_reg_0x14.b.sckdv = DBIC_InitStruct->DBIC_SPEED_SEL;
    DBIC->BAUDR = dbic_reg_0x14.d32;

#ifdef TARGET_RTL87x3EU
    DBIC_DCX_CTRL_t dcx_ctrl_reg = {.d32 = DBIC->DCX_CTRL};
    dcx_ctrl_reg.b.addr_head_en = DBICCfg->addr_head_en;
    dcx_ctrl_reg.b.cmd_head_en = DBICCfg->cmd_head_en;
    dcx_ctrl_reg.b.data_head_en = DBICCfg->data_head_en;
    dcx_ctrl_reg.b.addr_tail_en = DBICCfg->addr_tail_en;
    dcx_ctrl_reg.b.cmd_tail_en = DBICCfg->cmd_tail_en;
    dcx_ctrl_reg.b.data_tail_en = DBICCfg->data_tail_en;
    dcx_ctrl_reg.b.addr_ctrl_bit = DBICCfg->addr_ctrl_bit;
    if (DBICCfg->addr_ctrl_bit)
    {
        dcx_ctrl_reg.b.addr_ctrl = DBICCfg->addr_ctrl;
    }
    dcx_ctrl_reg.b.cmd_ctrl_bit = DBICCfg->cmd_ctrl_bit;
    if (DBICCfg->cmd_ctrl_bit)
    {
        dcx_ctrl_reg.b.cmd_ctrl = DBICCfg->cmd_ctrl;
    }
    dcx_ctrl_reg.b.data_ctrl_bit = DBICCfg->data_ctrl_bit;
    if (DBICCfg->data_ctrl_bit)
    {
        dcx_ctrl_reg.b.data_ctrl = DBICCfg->data_ctrl;
    }
    DBIC->DCX_CTRL = dcx_ctrl_reg.d32;

    DBIC_DCX_CMD_t dcx_cmd_reg = {.d32 = DBIC->DCX_CMD};
    dcx_cmd_reg.b.cmd_head_val = DBICCfg->cmd_head_value;
    dcx_cmd_reg.b.cmd_tail_val = DBICCfg->cmd_tail_value;
    DBIC->DCX_CMD = dcx_cmd_reg.d32;

    DBIC_DCX_ADDR_t dcx_addr_reg = {.d32 = DBIC->DCX_ADDR};
    dcx_addr_reg.b.addr_head_val = DBICCfg->addr_head_value;
    dcx_addr_reg.b.addr_tail_val = DBICCfg->addr_tail_value;
    DBIC->DCX_ADDR = dcx_addr_reg.d32;

    DBIC_DCX_DATA_t dcx_data_reg = {.d32 = DBIC->DCX_DATA};
    dcx_data_reg.b.data_head_val = DBICCfg->data_head_value;
    dcx_data_reg.b.data_tail_val = DBICCfg->data_tail_value;
    DBIC->DCX_DATA = dcx_data_reg.d32;

    DBIC->DMATDLR = 0x20;
#endif
}

void DBIC_SendBuf(uint8_t *buf, uint32_t len)
{
    DBIC_Select();

    DBIC_SSIENR_TypeDef dbic_reg_0x08 = {.d32 = DBIC->SSIENR};
    dbic_reg_0x08.b.spic_en = 0;
    DBIC->SSIENR = dbic_reg_0x08.d32;//disable DBIC

    DBIC_CTRLR0_TypeDef dbic_reg_0x00 = {.d32 = DBIC->CTRLR0};
    dbic_reg_0x00.b.cmd_ch = DBIC_CH_SINGLE;
    dbic_reg_0x00.b.data_ch = DBIC_CH_SINGLE;
    dbic_reg_0x00.b.addr_ch = DBIC_CH_SINGLE;
    dbic_reg_0x00.b.tmod = DBIC_TMODE_TX;
    DBIC->CTRLR0 = dbic_reg_0x00.d32;//tx mode

    DBIC_FLUSH_FIFO_TypeDef dbic_reg_0x128 = {.d32 = DBIC->FLUSH_FIFO};
    dbic_reg_0x128.b.flush_dr_fifo = 1;
    DBIC->FLUSH_FIFO = dbic_reg_0x128.d32;

//    DBIC_TX_NDF(4);
//    LCDC_SetTxPixelLen(2);
    DBIC_CmdLength(1);
    DBIC_AddrLength(3);

//    LCDC_AutoWriteCmd(ENABLE);
    for (uint32_t i = 0; i < 4; i++)
    {
        DBIC->DR[0].byte = buf[i];
    }
    dbic_reg_0x08.b.spic_en = 1;
    DBIC->SSIENR = dbic_reg_0x08.d32;//enable DBIC

    DBIC_SR_TypeDef dbic_reg_0x28;
    do
    {
        dbic_reg_0x28.d32 = DBIC->SR;
    }
    while (dbic_reg_0x28.b.busy);  // wait bus busy

    dbic_reg_0x08.b.spic_en = 0;
    DBIC->SSIENR = dbic_reg_0x08.d32;//disable DBIC
    //LCDC_AutoWriteCmd(ENABLE);
    //while (DBIC->SR & BIT0); // wait bus busy
//    while ((LCDC_HANDLER->OPERATE_CTR & LCDC_AUTO_WRITE_START_MSK) != RESET);
}

DBIC_ERR DBIC_ReceiveBuf(uint32_t cmd, uint32_t cmd_len, uint32_t addr, uint32_t addr_len,
                         uint16_t rd_dummy_len, uint8_t *read_buf, uint32_t read_len)
{
    assert_param(read_buf != NULL);
    assert_param(cmd_len <= 3);
    assert_param(addr_len <= 4);
    assert_param(rd_dummy_len <= 0x0FFF);
    assert_param((read_len > 0) && (read_len <= 0x00FFFFFF));
    if (read_buf == NULL)
    {
        return DBIC_ERR_NULL_BUFFER;
    }
    if ((cmd_len > 3) || (addr_len > 4) || (rd_dummy_len > 0x0FFF) ||
        (read_len == 0) || (read_len > 0x00FFFFFF))
    {
        return DBIC_ERR_INVALID_PARAMETER;
    }

    /* SDI registers are only accessible through APB in firmware mode. */
    uint32_t axi_mux_mode = LCDC_HANDLER->AXI_MUX_MODE;
    LCDC_AXIMUXMode(LCDC_FW_MODE);

    /* Save the configuration shared with the display write path. */
    uint32_t ssienr = DBIC->SSIENR;
    uint32_t ser = DBIC->SER;
    uint32_t ctrlr0 = DBIC->CTRLR0;
    uint32_t ctrlr2 = DBIC->CTRLR2;
    uint32_t user_length = DBIC->USER_LENGTH;
    uint32_t rx_ndf = DBIC->RX_NDF;
    uint32_t tx_ndf = DBIC->TX_NDF;
    uint32_t dmacr = DBIC->DMACR;

    DBIC_SSIENR_TypeDef dbic_reg_0x08 = {.d32 = ssienr};
    dbic_reg_0x08.b.spic_en = 0;
    dbic_reg_0x08.b.atck_cmd = 0;
    DBIC->SSIENR = dbic_reg_0x08.d32;
    DBIC_Select();

    /* Single-lane SDR receive in user mode for command, address and data phases. */
    DBIC_CTRLR0_TypeDef dbic_reg_0x00 = {.d32 = ctrlr0};
    dbic_reg_0x00.b.cmd_ch = DBIC_CH_SINGLE;
    dbic_reg_0x00.b.addr_ch = DBIC_CH_SINGLE;
    dbic_reg_0x00.b.data_ch = DBIC_CH_SINGLE;
    dbic_reg_0x00.b.ddr_en = 0;
    dbic_reg_0x00.b.tmod = DBIC_TMODE_RX;
    dbic_reg_0x00.b.user_mode = DBIC_USER_MODE;
    DBIC->CTRLR0 = dbic_reg_0x00.d32;

    /* Receive single-lane data on spi_sout[0] (SIO0). */
    DBIC_CTRLR2_TypeDef dbic_reg_0x110 = {.d32 = ctrlr2};
    dbic_reg_0x110.b.so_dnum = 0;
    DBIC->CTRLR2 = dbic_reg_0x110.d32;

    /* Use polling rather than DMA to drain the receive FIFO. */
    DBIC->DMACR = 0;

    DBIC_FLUSH_FIFO_TypeDef dbic_reg_0x128 = {.d32 = 0};
    dbic_reg_0x128.b.flush_dr_fifo = 1;
    DBIC->FLUSH_FIFO = dbic_reg_0x128.d32;

    DBIC_USER_LENGTH_TypeDef dbic_reg_0x118 = {.d32 = user_length};
    dbic_reg_0x118.b.user_cmd_length = cmd_len;
    dbic_reg_0x118.b.user_addr_length = addr_len;
    dbic_reg_0x118.b.user_rd_dummy_length = rd_dummy_len;
    DBIC->USER_LENGTH = dbic_reg_0x118.d32;

    DBIC_RX_NDF_TypeDef dbic_reg_0x04 = {.d32 = rx_ndf};
    dbic_reg_0x04.b.rx_ndf = read_len;
    DBIC->RX_NDF = dbic_reg_0x04.d32;
    DBIC_TX_NDF(0);

    for (uint32_t i = cmd_len; i > 0; i--)
    {
        DBIC->DR[0].byte = (uint8_t)(cmd >> ((i - 1) * 8));
    }
    for (uint32_t i = addr_len; i > 0; i--)
    {
        DBIC->DR[0].byte = (uint8_t)(addr >> ((i - 1) * 8));
    }

    dbic_reg_0x08.b.spic_en = 1;
    DBIC->SSIENR = dbic_reg_0x08.d32;

    DBIC_ERR status = DBIC_SUCCESS;
    DBIC_SR_TypeDef dbic_reg_0x28;
    uint32_t rd_num = 0;
    uint32_t idle_polls = 0;
    while (rd_num < read_len)
    {
        dbic_reg_0x28.d32 = DBIC->SR;
        if (dbic_reg_0x28.b.rfne && !dbic_reg_0x28.b.dcol)
        {
            read_buf[rd_num++] = DBIC->DR[0].byte;
            idle_polls = 0;
        }
        else if (++idle_polls >= DBIC_RX_TIMEOUT_POLLS)
        {
            status = DBIC_ERR_RX_TIMEOUT;
            break;
        }
    }

    if (status == DBIC_SUCCESS)
    {
        idle_polls = 0;
        dbic_reg_0x28.d32 = DBIC->SR;
        while (dbic_reg_0x28.b.busy)
        {
            if (++idle_polls >= DBIC_RX_TIMEOUT_POLLS)
            {
                status = DBIC_ERR_BUSY_TIMEOUT;
                break;
            }
            dbic_reg_0x28.d32 = DBIC->SR;
        }
    }

    dbic_reg_0x08.b.spic_en = 0;
    DBIC->SSIENR = dbic_reg_0x08.d32;
    if (status != DBIC_SUCCESS)
    {
        DBIC->FLUSH_FIFO = dbic_reg_0x128.d32;
    }

    /* Restore the transfer configuration. DBIC is left disabled. */
    dbic_reg_0x08.d32 = ssienr;
    dbic_reg_0x08.b.spic_en = 0;
    DBIC->SSIENR = dbic_reg_0x08.d32;
    DBIC->SER = ser;
    DBIC->CTRLR0 = ctrlr0;
    DBIC->CTRLR2 = ctrlr2;
    DBIC->USER_LENGTH = user_length;
    DBIC->RX_NDF = rx_ndf;
    DBIC->TX_NDF = tx_ndf;
    DBIC->DMACR = dmacr;
    LCDC_HANDLER->AXI_MUX_MODE = axi_mux_mode;

    return status;
}

void DBIC_Cmd(FunctionalState NewState)
{
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    DBIC_SSIENR_TypeDef dbic_reg_0x08 = {.d32 = DBIC->SSIENR};
    if (NewState == ENABLE)
    {
        dbic_reg_0x08.b.spic_en = 1;//enable
    }
    else
    {
        dbic_reg_0x08.b.spic_en = 0;
    }
    DBIC->SSIENR = dbic_reg_0x08.d32;
}

void DBIC_SwitchMode(uint32_t mode)
{
    assert_param(IS_DBIC_MODE(mode));
    DBIC_CTRLR0_TypeDef dbic_reg_0x00 = {.d32 = DBIC->CTRLR0};
    dbic_reg_0x00.b.user_mode = mode;
    DBIC->CTRLR0 = dbic_reg_0x00.d32;
}

void DBIC_SwitchDirect(uint32_t dir)
{
    assert_param(IS_DBIC_DIR(dir));
    DBIC_CTRLR0_TypeDef dbic_reg_0x00 = {.d32 = DBIC->CTRLR0};
    dbic_reg_0x00.b.tmod = dir;
    DBIC->CTRLR0 = dbic_reg_0x00.d32;//tx mode
}

void DBIC_CmdLength(uint32_t len)
{
    DBIC_USER_LENGTH_TypeDef dbic_reg_0x118 = {.d32 = DBIC->USER_LENGTH};
    dbic_reg_0x118.b.user_cmd_length = len;
    DBIC->USER_LENGTH = dbic_reg_0x118.d32;
}

void DBIC_AddrLength(uint32_t len)
{
    DBIC_USER_LENGTH_TypeDef dbic_reg_0x118 = {.d32 = DBIC->USER_LENGTH};
    dbic_reg_0x118.b.user_addr_length = len;
    DBIC->USER_LENGTH = dbic_reg_0x118.d32;
}

void DBIC_Select(void)
{
    DBIC_SER_TypeDef dbic_reg_0x10 = {.d32 = DBIC->SER};
    dbic_reg_0x10.b.ser = 1;
    DBIC->SER = dbic_reg_0x10.d32;
}

void DBIC_TX_NDF(uint32_t len)
{
    DBIC_TX_NDF_TypeDef dbic_reg_0x130 = {.d32 = DBIC->TX_NDF};
    dbic_reg_0x130.b.tx_ndf = len;
    DBIC->TX_NDF = dbic_reg_0x130.d32;
}

void DBIC_StructInit(LCDC_DBICCfgTypeDef *DBIC_InitStruct)
{
    /* Check the parameters */
    assert_param(DBIC_InitStruct != NULL);

    /*-------------- Reset DBIC init structure parameters values ------------------*/
    DBIC_InitStruct->DBIC_SPEED_SEL = 1;
    DBIC_InitStruct->DBIC_TxThr = 0;
    DBIC_InitStruct->DBIC_RxThr = 0;
    DBIC_InitStruct->SCPOL = DBIC_SCPOL_LOW;
    DBIC_InitStruct->SCPH = DBIC_SCPH_1Edge;
}
