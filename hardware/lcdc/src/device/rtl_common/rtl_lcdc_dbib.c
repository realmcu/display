/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* \file     rtl_dbib.c
* \brief    This file provides all the DBI firmware functions.
* \details
* \author   boris yue
* \date     2023-10-17
* \version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "rtl_lcdc_dbib.h"

/*============================================================================*
 *                           Public Functions
 *============================================================================*/
void DBIB_Init(LCDC_DBIBCfgTypeDef *DBIBCfg)
{
    /* Check the parameters */
    assert_param(IS_DBIB_CLOCK_DIV(DBIBCfg->DBIB_SPEED_SEL));
    assert_param(IS_DBIB_GUARD_TIME_CMD(DBIBCfg->DBIB_InitGuardTimeCmd));
    assert_param(IS_DBIB_GUARD_TIME(DBIBCfg->DBIB_InitGuardTime));
    assert_param(IS_DBIB_GUARD_TIME_CMD(DBIBCfg->DBIB_CmdGuardTimeCmd));
    assert_param(IS_DBIB_GUARD_TIME(DBIBCfg->DBIB_CmdGuardTime));
    assert_param(IS_DBIB_GUARD_TIME_CMD(DBIBCfg->DBIB_GuardTimeCmd));
    assert_param(IS_DBIB_GUARD_TIME(DBIBCfg->DBIB_GuardTime));
    assert_param(IS_DBIB_WR_DELAY_TIME(DBIBCfg->DBIB_WRDelay));

    /* Configure clock divider, bypass guard time*/
    DBIB_CTRL0_TypeDef dbib_reg_0x00 = {.d32 = DBIB->DBIB_CTRL0};
    dbib_reg_0x00.b.speed_sel = DBIBCfg->DBIB_SPEED_SEL;
    dbib_reg_0x00.b.init_guard_time = DBIBCfg->DBIB_InitGuardTimeCmd;
    dbib_reg_0x00.b.cmd_guard_time = DBIBCfg->DBIB_CmdGuardTime;
    dbib_reg_0x00.b.guard_time = DBIBCfg->DBIB_GuardTime;
    dbib_reg_0x00.b.bypass_guard_time = DBIBCfg->DBIB_CmdGuardTimeCmd;
    dbib_reg_0x00.b.bypass_cmd_guard_time = DBIBCfg->DBIB_CmdGuardTimeCmd;
    dbib_reg_0x00.b.bypass_init_guard_time = DBIBCfg->DBIB_InitGuardTimeCmd;
    DBIB->DBIB_CTRL0 = dbib_reg_0x00.d32;

    /* Configure WR Delay */
    DBIB_CFG_TypeDef dbib_reg_0x04 = {.d32 = DBIB->DBIB_CFG};
    dbib_reg_0x04.b.reg_wr_trig_sel = DBIBCfg->DBIB_WRDelay;
    DBIB->DBIB_CFG = dbib_reg_0x04.d32;
}

void DBIB_auto_write_set_window(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd,
                                uint16_t xOffset, uint16_t yOffset)
{

}

void DBIB_SendCmd(uint8_t cmd)
{
    /* Fill command */
    DBIB->DBIB_CMD_REG1 = cmd;

    /* Enable command control */
    /* Send command */
    DBIB_CFG_TypeDef dbib_reg_0x04 = {.d32 = DBIB->DBIB_CFG};
    dbib_reg_0x04.b.reg_d_cb_manual = 0;
    dbib_reg_0x04.b.reg_wr_trig_manual = 1;
    DBIB->DBIB_CFG = dbib_reg_0x04.d32;

    /* Check write status */
    do
    {
        dbib_reg_0x04.d32 = DBIB->DBIB_CFG;
    }
    while (dbib_reg_0x04.b.reg_wr_trig_manual);
}

void DBIB_SendData(uint8_t *pBuf, uint32_t len)
{
    while (len--)
    {
        /* Fill data */
        DBIB->DBIB_CMD_REG1 = *pBuf++;

        /* Send command */
        DBIB_CFG_TypeDef dbib_reg_0x04 = {.d32 = DBIB->DBIB_CFG};
        dbib_reg_0x04.b.reg_d_cb_manual = 1;
        dbib_reg_0x04.b.reg_wr_trig_manual = 1;
        DBIB->DBIB_CFG = dbib_reg_0x04.d32;

        /* Check write status */
        do
        {
            dbib_reg_0x04.d32 = DBIB->DBIB_CFG;
        }
        while (dbib_reg_0x04.b.reg_wr_trig_manual);
    }
}

void DBIB_ReceiveData(uint8_t *pBuf, uint32_t len)
{
    DBIB_RXDATA_TypeDef dbib_reg_0x08;
    while (len--)
    {
        /* Select read data control */
        DBIB_CFG_TypeDef dbib_reg_0x04 = {.d32 = DBIB->DBIB_CFG};
        dbib_reg_0x04.b.reg_d_cb_manual = 1;
        dbib_reg_0x04.b.reg_rd_trig_manual = 1;
        DBIB->DBIB_CFG = dbib_reg_0x04.d32;

        /* Check read status */
        do
        {
            dbib_reg_0x04.d32 = DBIB->DBIB_CFG;
        }
        while (dbib_reg_0x04.b.reg_rd_trig_manual);

        /* Read data */
        dbib_reg_0x08.d32 = DBIB->DBIB_RXDATA;
        if (dbib_reg_0x08.b.rdata_valid)
        {
            *pBuf++ = dbib_reg_0x08.b.rdata;
        }
        else
        {
            //APP_PRINT_INFO1("[LCDC] DBIB_ReceiveData invalid 0x%x", reg_value);
        }
    }
}

void DBIB_Write(uint8_t cmd, uint8_t *pBuf, uint32_t len)
{
    /* Pull CS down */
    DBIB_ResetCS();

    /* Send command */
    DBIB_SendCmd(cmd);

    /* Write data */
    DBIB_SendData(pBuf, len);

    /* Pull CS up */
    DBIB_SetCS();
}

void DBIB_BypassCmdByteCmd(FunctionalState NewState)
{
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    DBIB_CTRL0_TypeDef dbib_reg_0x00 = {.d32 = DBIB->DBIB_CTRL0};
    if (NewState == ENABLE)
    {
        dbib_reg_0x00.b.bypass_cmd = 1;
    }
    else
    {
        dbib_reg_0x00.b.bypass_cmd = 0;
    }
    DBIB->DBIB_CTRL0 = dbib_reg_0x00.d32;
}

void DBIB_ResetCS(void)
{
    DBIB_CFG_TypeDef dbib_reg_0x04 = {.d32 = DBIB->DBIB_CFG};
    dbib_reg_0x04.b.reg_cs_manual = 0;
    DBIB->DBIB_CFG = dbib_reg_0x04.d32;
}

void DBIB_SetCS(void)
{
    DBIB_CFG_TypeDef dbib_reg_0x04 = {.d32 = DBIB->DBIB_CFG};
    dbib_reg_0x04.b.reg_cs_manual = 1;
    DBIB->DBIB_CFG = dbib_reg_0x04.d32;
}

void DBIB_ResetDC(void)
{
    DBIB_CFG_TypeDef dbib_reg_0x04 = {.d32 = DBIB->DBIB_CFG};
    dbib_reg_0x04.b.reg_d_cb_manual = 0;
    DBIB->DBIB_CFG = dbib_reg_0x04.d32;
}

void DBIB_SetDC(void)
{
    DBIB_CFG_TypeDef dbib_reg_0x04 = {.d32 = DBIB->DBIB_CFG};
    dbib_reg_0x04.b.reg_d_cb_manual = 1;
    DBIB->DBIB_CFG = dbib_reg_0x04.d32;
}

/**
  * \brief  Send command and read data buffer in manual mode.
  * \param  cmd: command which to be sent.
  * \param  pBuf: buffer address to be sent.
  * \param  len:  data length.
  * \return None
  */
void DBIB_Read(uint8_t cmd, uint8_t *pBuf, uint32_t len)
{
    /* Pull CS down */
    DBIB_ResetCS();

    /* Send command */
    DBIB_SendCmd(cmd);

    /* Read data */
    DBIB_ReceiveData(pBuf, len);

    /* Pull CS up */
    DBIB_SetCS();
}

/**
  * \brief  Configure command sequences in auto mode.
  * \param  pCmdBuf: buffer address which store command sequence.
  * \param  len:  command length.
  * \return None
  */
FlagStatus LCDC_DBIB_SetCmdSequence(uint8_t *pCmdBuf, uint8_t len)
{
    uint32_t i = 0;

    /* Check parameters */
    if ((len > 12) || (len < 1))
    {
        return RESET;
    }

    /* Configure command number */
    DBIB_CFG_TypeDef dbib_reg_0x04 = {.d32 = DBIB->DBIB_CFG};
    dbib_reg_0x04.b.reg_cmd_num = len - 1;
    DBIB->DBIB_CFG = dbib_reg_0x04.d32;

    /* Fill command sequence */
    DBIB->DBIB_CMD_REG1 = 0;
    DBIB->DBIB_CMD_REG2 = 0;
    DBIB->DBIB_CMD_REG3 = 0;
    for (i = 0; i < len; i++)
    {
        if (i < 4)
        {
            DBIB->DBIB_CMD_REG1 |= (*pCmdBuf++) << (i * 8);
        }
        else if (i < 8)
        {
            DBIB->DBIB_CMD_REG2 |= (*pCmdBuf++) << ((i - 4) * 8);
        }
        else
        {
            DBIB->DBIB_CMD_REG3 |= (*pCmdBuf++) << ((i - 8) * 8);
        }
    }
    return SET;
}

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/
