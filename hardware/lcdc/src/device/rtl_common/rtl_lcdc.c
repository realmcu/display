/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* \file     rtl_lcdc.c
* \brief    This file provides all the LCDC HANDLER firmware functions.
* \details
* \author   howie wang
* \date     2023-10-17
* \version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "rtl_lcdc.h"
#include "rtl_lcdc_dbib.h"
#include "rtl_lcdc_edpi.h"

/*============================================================================*
 *                          Private Macros
 *============================================================================*/
#define LCDC_DMA_PROTCTL_POS          (3)
#define LCDC_DMA_PROTCTL_MSK          ((uint32_t)0x1 << LCDC_DMA_PROTCTL_POS)
#define LCDC_DMA_PROTCTL_CLR          (~LCDC_DMA_PROTCTL_MSK)

/*============================================================================*
 *                           Public Functions
 *============================================================================*/
typedef struct
{
    LCDC_DMA_LinkList_TypeDef *link;
    LCDC_Handler_TypeDef *handler;
    LCDC_DBIB_TypeDef *DBI_B;
    LCDC_EDPI_TypeDef *eDPI;
} LCDC_TypeDef;

LCDC_TypeDef LCDCdef = {LCDC_DMA_LINKLIST, LCDC_HANDLER, DBIB, EDPI};
LCDC_TypeDef *LCDC = &LCDCdef;

void LCDC_Clock_Cfg(FunctionalState state)
{
    if (state == ENABLE)
    {
#if defined (CONFIG_SOC_SERIES_RTL87X2G)
//TODO
#elif defined TARGET_RTL8773E
        uint32_t reg_value = *(uint32_t *)0x40000214;
        reg_value |= (BIT11 | BIT12);
        *(uint32_t *)0x40000214 = reg_value;
#endif
    }
    else if (state == DISABLE)
    {
#if defined (CONFIG_SOC_SERIES_RTL87X2G)
//TODO
#elif defined TARGET_RTL8773E
        uint32_t reg_value = *(uint32_t *)0x40000214;
        reg_value &= ~(BIT11 | BIT12);
        *(uint32_t *)0x40000214 = reg_value;
#endif
    }
}



void LCDC_Init(LCDC_InitTypeDef *cfg)
{
    /* Check the parameters */
    assert_param(IS_LCDC_IF_SEL(cfg->LCDC_Interface));
    assert_param(IS_LCDC_INPUT_FORMAT(cfg->LCDC_PixelInputFarmat));
    assert_param(IS_LCDC_OUTPUT_FORMAT(cfg->LCDC_PixelOutpuFarmat));
    assert_param(IS_LCDC_SWAP(cfg->LCDC_PixelBitSwap));
    assert_param(IS_LCDC_TE_CMD(cfg->LCDC_TeEn));
    assert_param(IS_LCDC_TE_SIGNAL_VALID(cfg->LCDC_TePolarity));

    /* Configure mask interrupt */
    LCDC_HANDLER_DMA_FIFO_IMR_TypeDef handler_reg_0x1c = {.d32 = LCDC_HANDLER->DMA_FIFO_IMR};
    handler_reg_0x1c.b.display_controller_waveform_finish_int_msk = 1;
    handler_reg_0x1c.b.rx_auto_done_int_msk = 1;
    handler_reg_0x1c.b.rx_fifo_overflow_int_msk = 1;
    handler_reg_0x1c.b.tear_trigger_int_msk = 1;
    handler_reg_0x1c.b.tx_auto_done_int_msk = 1;
    handler_reg_0x1c.b.tx_fifo_empty_int_msk = 1;
    handler_reg_0x1c.b.tx_fifo_overflow_int_msk = 1;
    handler_reg_0x1c.b.tx_fifo_threshold_int_msk = 1;

    LCDC_HANDLER_FT_IN_TypeDef handler_reg_0x04 = {.d32 = LCDC_HANDLER->FT_IN};
    handler_reg_0x04.b.input_format = cfg->LCDC_PixelInputFormat;
    LCDC_HANDLER->FT_IN = handler_reg_0x04.d32;

    LCDC_HANDLER_INTERFACE_SELECT_TypeDef handler_reg_0x00 = {.d32 = LCDC_HANDLER->INTERFACE_SELECT};
    handler_reg_0x00.b.interface_select = cfg->LCDC_Interface;
    if ((cfg->LCDC_Interface == LCDC_IF_DBIC))
    {
        LCDC_HANDLER_SPIC_SCLK_PHY_TypeDef handler_reg_0x60 = {.d32 = LCDC_HANDLER->SPIC_SCLK_PHY};
        handler_reg_0x60.b.spi_sclk_phase_shift = cfg->LCDC_PhaseShift;
        handler_reg_0x60.b.r_clko_dly_se = cfg->LCDC_DelayCell;
        LCDC_HANDLER->SPIC_SCLK_PHY = handler_reg_0x60.d32;
        if (cfg->LCDC_RamlessEn)
        {
            handler_reg_0x00.b.ramless_qspi_enable = 1;
        }
    }
    handler_reg_0x00.b.group_sel = cfg->LCDC_GroupSel;
    LCDC_HANDLER->INTERFACE_SELECT = handler_reg_0x00.d32;

    LCDC_HANDLER_FT_OUT_TypeDef handler_reg_0x08 = {.d32 = LCDC_HANDLER->FT_OUT};
    handler_reg_0x08.b.output_format = cfg->LCDC_PixelOutputFormat;
    LCDC_HANDLER->FT_OUT = handler_reg_0x08.d32;

    LCDC_HANDLER_BIT_SWAP_TypeDef handler_reg_0x0c = {.d32 = LCDC_HANDLER->BIT_SWAP};
    handler_reg_0x0c.b.bit_swap = cfg->LCDC_PixelBitSwap;
    LCDC_HANDLER->BIT_SWAP = handler_reg_0x0c.d32;

    LCDC_HANDLER_TEAR_CTR_TypeDef handler_reg_0x10 = {.d32 = LCDC_HANDLER->TEAR_CTR};
    handler_reg_0x10.b.tear_logic_enable = cfg->LCDC_TeEn;
    handler_reg_0x10.b.tear_polarity = cfg->LCDC_TePolarity;
    handler_reg_0x10.b.tear_input_mux = cfg->LCDC_TeInputMux;
    LCDC_HANDLER->TEAR_CTR = handler_reg_0x10.d32;

    LCDC_HANDLER_DMA_FIFO_CTRL_TypeDef handler_reg_0x18 = {.d32 = LCDC_HANDLER->DMA_FIFO_CTRL};
    handler_reg_0x18.b.dma_fifo_threshold = cfg->LCDC_DmaThreshold;
    LCDC_HANDLER->DMA_FIFO_CTRL = handler_reg_0x18.d32;

    LCDC_HANDLER_INFINITE_MODE_CTR_TypeDef handler_reg_0x64 = {.d32 = LCDC_HANDLER->INFINITE_MODE_CTR};
    if (cfg->LCDC_InfiniteModeEn == ENABLE)
    {
        handler_reg_0x64.b.infinite_mode_en = 1;
    }
    else
    {
        handler_reg_0x64.b.infinite_mode_en = 0;
    }
    LCDC_HANDLER->INFINITE_MODE_CTR = handler_reg_0x64.d32;
}

void LCDC_MaskINTConfig(uint32_t LCDC_INT_MSK, FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_LCDC_INT_MSK_CONFIG(LCDC_INT_MSK));
    assert_param(IS_FUNCTIONAL_STATE(NewState));

    if (NewState == ENABLE)
    {
        LCDC_HANDLER->DMA_FIFO_IMR |= LCDC_INT_MSK;
    }
    else
    {
        LCDC_HANDLER->DMA_FIFO_IMR &= ~LCDC_INT_MSK;
    }
}

ITStatus LCDC_GetINTStatus(uint32_t LCDC_INT)
{
    ITStatus bit_status = RESET;

    /* Check the parameters */
    assert_param(IS_LCDC_INT_STATUS(LCDC_INT));

    if (LCDC_HANDLER->DMA_FIFO_SR & LCDC_INT)
    {
        bit_status = SET;
    }

    return  bit_status;
}

void LCDC_ForceBurst(FunctionalState new_state)
{
    LCDC_HANDLER_DMA_FIFO_CTRL_TypeDef lcdc_reg_0x18 = {.d32 = LCDC_HANDLER->DMA_FIFO_CTRL};
    if (new_state == ENABLE)
    {
        lcdc_reg_0x18.b.dma_burst_request_only = 1;
    }
    else
    {
        lcdc_reg_0x18.b.dma_burst_request_only = 0;
    }
    LCDC_HANDLER->DMA_FIFO_CTRL = lcdc_reg_0x18.d32;
}

FlagStatus LCDC_GetFlagStatus(uint32_t LCDC_FLAG)
{
    FlagStatus bit_status = RESET;

    /* Check the parameters */
    assert_param(IS_LCDC_FLAG_STATUS(LCDC_FLAG));

    if (LCDC_HANDLER->DMA_FIFO_SR & LCDC_FLAG)
    {
        bit_status = SET;
    }

    return  bit_status;
}

void LCDC_StructInit(LCDC_InitTypeDef *LCDC_InitStruct)
{
    /*-------------- Reset LCDC init structure parameters values ------------------*/
    LCDC_InitStruct->LCDC_Interface = LCDC_IF_DPI;
    LCDC_InitStruct->LCDC_RamlessEn = DISABLE;
    LCDC_InitStruct->LCDC_GroupSel = 1;
    LCDC_InitStruct->LCDC_PixelInputFormat = LCDC_INPUT_RGB888;
    LCDC_InitStruct->LCDC_PixelOutputFormat = LCDC_OUTPUT_RGB888;
    LCDC_InitStruct->LCDC_PixelBitSwap = LCDC_SWAP_BYPASS;
    LCDC_InitStruct->LCDC_TeEn = DISABLE;
    LCDC_InitStruct->LCDC_TePolarity = LCDC_TE_EDGE_RISING;
    LCDC_InitStruct->LCDC_TeInputMux = LCDC_TE_LCD_INPUT;
    LCDC_InitStruct->LCDC_DmaThreshold = 8;
    LCDC_InitStruct->LCDC_InfiniteModeEn = DISABLE;
    LCDC_InitStruct->LCDC_PhaseShift = LCDC_SPI_PHASE_SHIFT_0_DEGREE;
    LCDC_InitStruct->LCDC_DelayCell = 0;
}

/**
  * \brief  Initializes the DMA Channelx according to the specified
  *         parameters in the DMA_InitStruct.
  * \param  DMA_Channelx: where x can be 0 to 7  to select the DMA Channel.
  * \param  DMA_InitStruct: pointer to a DMA_InitTypeDef structure that
  *         contains the configuration information for the specified DMA Channel.
  * \return None
  */
void LCDC_DMA_Init(LCDC_DMA_ChannelTypeDef *LCDC_DMA_Channelx,
                   LCDC_DMA_InitTypeDef *LCDC_DMA_InitStruct)
{
    uint32_t temp_bit = 0;

    /* Check the parameters */
    assert_param(IS_DMA_ALL_PERIPH(LCDC_DMA_Channelx));
    assert_param(IS_DMA_ChannelNum(LCDC_DMA_InitStruct->LCDC_DMA_ChannelNum));
    assert_param(IS_DMA_DIR(LCDC_DMA_InitStruct->LCDC_DMA_DIR));
    assert_param(IS_DMA_SourceInc(LCDC_DMA_InitStruct->LCDC_DMA_SourceInc));
    assert_param(IS_DMA_DestinationInc(LCDC_DMA_InitStruct->LCDC_DMA_DestinationInc));
    assert_param(IS_DMA_DATA_SIZE(LCDC_DMA_InitStruct->LCDC_DMA_SourceDataSize));
    assert_param(IS_DMA_DATA_SIZE(LCDC_DMA_InitStruct->LCDC_DMA_DestinationDataSize));

    /*------------------configure source and destination address of DMA---------*/
    /* program SARx register to set source address */
    LCDC_DMA_Channelx->LCDC_DMA_SARx = LCDC_DMA_InitStruct->LCDC_DMA_SourceAddr;
    /* program DARx register to set destination address */
    LCDC_DMA_Channelx->LCDC_DMA_DARx = LCDC_DMA_InitStruct->LCDC_DMA_DestinationAddr;

    /* Enable DMA in LCDC_DMA_DmaCfgReg*/
    LCDC_DMA_BASE->LCDC_DMA_DmaCfgReg = 0x01;

    /* read LCDC_DMA_ChEnReg to check channel is busy or not */
    if (LCDC_DMA_BASE->LCDC_DMA_ChEnReg & BIT(LCDC_DMA_InitStruct->LCDC_DMA_ChannelNum))
    {
        //channel is be used
        //error handle code
        //while (1);
    }

    /*--------------------------- DMA Configuration -----------------*/
    /* clear pending interrupts of corresponding DMA channel */
    temp_bit = BIT(LCDC_DMA_InitStruct->LCDC_DMA_ChannelNum);
    LCDC_DMA_BASE->LCDC_DMA_ClearTfr |= temp_bit;
    LCDC_DMA_BASE->LCDC_DMA_ClearBlock |= temp_bit;
    LCDC_DMA_BASE->LCDC_DMA_ClearDstTran |= temp_bit;
    LCDC_DMA_BASE->LCDC_DMA_ClearSrcTran |= temp_bit;
    LCDC_DMA_BASE->LCDC_DMA_ClearErr |= temp_bit;

    /* mask pending interrupts of corresponding DMA channel */
    temp_bit = BIT(LCDC_DMA_InitStruct->LCDC_DMA_ChannelNum + 8);
    LCDC_DMA_BASE->LCDC_DMA_MaskTfr = temp_bit;
    LCDC_DMA_BASE->LCDC_DMA_MaskBlock = temp_bit;
    LCDC_DMA_BASE->LCDC_DMA_MaskDstTran = temp_bit;
    LCDC_DMA_BASE->LCDC_DMA_MaskSrcTran = temp_bit;
    LCDC_DMA_BASE->LCDC_DMA_MaskErr = temp_bit;

    /*---------------------------- configure CTL register --------------------------------*/

    /* configure low 32 bit of CTL register */
    LCDC_DMA_CTL_LOWx_TypeDef gdma_0x18 = {.d32 = LCDC_DMA_Channelx->LCDC_DMA_CTL_LOWx};
    gdma_0x18.b.INT_EN = ENABLE;
    gdma_0x18.b.DST_TR_WIDTH = LCDC_DMA_InitStruct->LCDC_DMA_DestinationDataSize;
    gdma_0x18.b.SRC_TR_WIDTH = LCDC_DMA_InitStruct->LCDC_DMA_SourceDataSize;
    gdma_0x18.b.DINC = LCDC_DMA_InitStruct->LCDC_DMA_DestinationInc;
    gdma_0x18.b.SINC = LCDC_DMA_InitStruct->LCDC_DMA_SourceInc;
    gdma_0x18.b.DEST_MSIZE = LCDC_DMA_InitStruct->LCDC_DMA_DestinationMsize;
    gdma_0x18.b.SRC_MSIZE = LCDC_DMA_InitStruct->LCDC_DMA_SourceMsize;
    gdma_0x18.b.TT_FC = LCDC_DMA_InitStruct->LCDC_DMA_DIR;
    LCDC_DMA_Channelx->LCDC_DMA_CTL_LOWx = gdma_0x18.d32;

    /* configure high 32 bit of CTL register */
    LCDC_DMA_Channelx->LCDC_DMA_CTL_HIGHx = LCDC_DMA_InitStruct->LCDC_DMA_BufferSize;
    /*---------------------------- configure CFG register --------------------------------*/
    LCDC_DMA_Channelx->LCDC_DMA_CFG_LOWx = 0;

    LCDC_DMA_CFG_LOWx_TypeDef gdma_0x40 = {.d32 = LCDC_DMA_Channelx->LCDC_DMA_CFG_LOWx};
    /* Config multi-block mode */
    if (LCDC_DMA_InitStruct->LCDC_DMA_Multi_Block_En == 1 &&
        (LCDC_DMA_Channelx == LCDC_DMA_Channel0))
    {
        /* Clear automatic source/destination reload */
        gdma_0x40.b.RELOAD_SRC = 0x0;
        gdma_0x40.b.RELOAD_DST = 0x0;
        LCDC_DMA_Channelx->LCDC_DMA_CFG_LOWx = gdma_0x40.d32;
        /* Clear LLI for source/destination */
        gdma_0x18.b.LLP_DST_EN = 0x0;
        gdma_0x18.b.LLP_SRC_EN = 0x0;
        LCDC_DMA_Channelx->LCDC_DMA_CTL_LOWx = gdma_0x18.d32;

        if (LCDC_DMA_InitStruct->LCDC_DMA_Multi_Block_Mode & LCDC_DMA_LLP_SELECTED_BIT)
        {
            LCDC_DMA_Channelx->LCDC_DMA_LLPx = LCDC_DMA_InitStruct->LCDC_DMA_Multi_Block_Struct;
            gdma_0x18.b.LLP_DST_EN = (LCDC_DMA_InitStruct->LCDC_DMA_Multi_Block_Mode & BIT27) >> 27;
            gdma_0x18.b.LLP_SRC_EN = (LCDC_DMA_InitStruct->LCDC_DMA_Multi_Block_Mode & BIT28) >> 28;
            LCDC_DMA_Channelx->LCDC_DMA_CTL_LOWx = gdma_0x18.d32;
        }
        gdma_0x40.b.RELOAD_SRC = (LCDC_DMA_InitStruct->LCDC_DMA_Multi_Block_Mode & BIT30) >> 30;
        gdma_0x40.b.RELOAD_DST = (LCDC_DMA_InitStruct->LCDC_DMA_Multi_Block_Mode & BIT31) >> 31;
        LCDC_DMA_Channelx->LCDC_DMA_CFG_LOWx = gdma_0x40.d32;
    }

    /* ---------------- Set handshake ---------------- */
    /* configure peripheral parameters and configure source or destination hardware handshaking interface */

    /* Set DMA channel priority level */

    /* Enable FIFO mode and Flow control mode */
    LCDC_DMA_Channelx->LCDC_DMA_CFG_HIGHx &= ~0x03;
    LCDC_DMA_Channelx->LCDC_DMA_CFG_HIGHx |= 0x02;
#if defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
    LCDC_DMA_Channelx->LCDC_DMA_CFG_HIGHx &= LCDC_DMA_PROTCTL_CLR;// secure DMA
#else
    LCDC_DMA_Channelx->LCDC_DMA_CFG_HIGHx |= ~LCDC_DMA_PROTCTL_CLR;// non-secure DMA
#endif

    /* ---------------clear pending interrupts of corresponding DMA channel------------------ */
    temp_bit = BIT(LCDC_DMA_InitStruct->LCDC_DMA_ChannelNum);
    LCDC_DMA_BASE->LCDC_DMA_ClearTfr |= temp_bit;
    LCDC_DMA_BASE->LCDC_DMA_ClearBlock |= temp_bit;
    LCDC_DMA_BASE->LCDC_DMA_ClearDstTran |= temp_bit;
    LCDC_DMA_BASE->LCDC_DMA_ClearSrcTran |= temp_bit;
    LCDC_DMA_BASE->LCDC_DMA_ClearErr |= temp_bit;
}

/**
  * \brief  Fills each DMA_InitStruct member with its default value.
  * \param  DMA_InitStruct : pointer to a DMA_InitTypeDef structure which will
  *         be initialized.
  * \return None
  */
void LCDC_DMA_StructInit(LCDC_DMA_InitTypeDef *LCDC_DMA_InitStruct)
{
    /*-------------- Reset DMA init structure parameters values ------------------*/
    LCDC_DMA_InitStruct->LCDC_DMA_ChannelNum      = 0;
    LCDC_DMA_InitStruct->LCDC_DMA_DIR             =
        LCDC_DMA_DIR_PeripheralToMemory;    //flow control display handler
    LCDC_DMA_InitStruct->LCDC_DMA_BufferSize      = 0;
    LCDC_DMA_InitStruct->LCDC_DMA_SourceInc       = LCDC_DMA_SourceInc_Fix;
    LCDC_DMA_InitStruct->LCDC_DMA_DestinationInc  = LCDC_DMA_DestinationInc_Fix;

    LCDC_DMA_InitStruct->LCDC_DMA_SourceDataSize  = LCDC_DMA_DataSize_Byte;
    LCDC_DMA_InitStruct->LCDC_DMA_DestinationDataSize = LCDC_DMA_DataSize_Byte;
    LCDC_DMA_InitStruct->LCDC_DMA_SourceMsize     = LCDC_DMA_Msize_1;
    LCDC_DMA_InitStruct->LCDC_DMA_DestinationMsize  = LCDC_DMA_Msize_1;

    LCDC_DMA_InitStruct->LCDC_DMA_SourceAddr      = 0;
    LCDC_DMA_InitStruct->LCDC_DMA_DestinationAddr = 0;

    LCDC_DMA_InitStruct->LCDC_DMA_Multi_Block_Mode = LLI_TRANSFER;
    LCDC_DMA_InitStruct->LCDC_DMA_Multi_Block_En  = DISABLE;
    LCDC_DMA_InitStruct->LCDC_DMA_Multi_Block_Struct = 0;
}

/**
  * \brief  Enable or disable the specified DMA Channelx.
  * \param  DMA_Channelx: x can be 0 to 6 to select the DMA Channel.
  * \param  NewState: new state of the DMA Channelx.
  *   This parameter can be: ENABLE or DISABLE.
  * \return None
  */
void LCDC_DMAChannelCmd(uint8_t LCDC_DMA_ChannelNum, FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_DMA_ChannelNum(LCDC_DMA_ChannelNum));
    assert_param(IS_FUNCTIONAL_STATE(NewState));

    if (NewState != DISABLE)
    {
        /* Enable the selected DMAy Channelx */
        LCDC_DMA_BASE->LCDC_DMA_ChEnReg |= BIT(LCDC_DMA_ChannelNum) | BIT(LCDC_DMA_ChannelNum + 8);
    }
    else
    {
        /* Disable the selected DMAy Channelx */
        LCDC_DMA_BASE->LCDC_DMA_ChEnReg = BIT(LCDC_DMA_ChannelNum + 8);
    }
}

void LCDC_DMA_LinkList_Init(LCDC_DMALLI_InitTypeDef *LCDC_DMA_LLIConfig,
                            LCDC_DMA_InitTypeDef *LCDC_DMA_cfg)//offset mean up menu not display in current menu
{
    LCDC_DMA_InitTypeDef *LCDC_DMA_Init = (LCDC_DMA_InitTypeDef *)LCDC_DMA_cfg;
    /* Disable WP */
    LCDC_DMALL_DMA_CFG_TypeDef dmall_reg_0x44 = {.d32 = LCDC_DMA_LINKLIST->DMA_CFG};
    dmall_reg_0x44.b.reg_dma_g1_wp = 0;
    dmall_reg_0x44.b.reg_dma_g2_wp = 0;
    LCDC_DMA_LINKLIST->DMA_CFG = dmall_reg_0x44.d32;

    LCDC_DMA_LOAD_CNT_CLR();
    LCDC_DMA_LinkListCmd(ENABLE);


    LCDC_DMA_LINKLIST->GRP1_SAR = LCDC_DMA_LLIConfig->g1_source_addr;
    LCDC_DMA_LINKLIST->GRP1_SAR_OFFSET = LCDC_DMA_LLIConfig->g1_sar_offset;
    LCDC_DMA_LINKLIST->GRP1_DAR_OFFSET = 0;
    LCDC_DMA_LINKLIST->GRP1_LLP = LCDC_DMA_LINKLIST_REG_BASE + 0x70;

    LCDC_DMA_LINKLIST->GRP1_CTL0 = BIT(0)
                                   | (LCDC_DMA_Init->LCDC_DMA_DestinationDataSize << 1)
                                   | (LCDC_DMA_Init->LCDC_DMA_SourceDataSize << 4)
                                   | (LCDC_DMA_Init->LCDC_DMA_DestinationInc << 7)
                                   | (LCDC_DMA_Init->LCDC_DMA_SourceInc << 9)
                                   | (LCDC_DMA_Init->LCDC_DMA_DestinationMsize << 11)
                                   | (LCDC_DMA_Init->LCDC_DMA_SourceMsize << 14)
                                   | (LCDC_DMA_Init->LCDC_DMA_DIR << 20)
                                   | (LCDC_DMA_Init->LCDC_DMA_Multi_Block_Mode & LCDC_DMA_LLP_SELECTED_BIT);

    LCDC_DMA_LINKLIST->GRP2_SAR = LCDC_DMA_LLIConfig->g2_source_addr;
    LCDC_DMA_LINKLIST->GRP2_SAR_OFFSET = LCDC_DMA_LLIConfig->g2_sar_offset;
    LCDC_DMA_LINKLIST->GRP2_DAR_OFFSET = 0;
    LCDC_DMA_LINKLIST->GRP2_LLP = LCDC_DMA_LINKLIST_REG_BASE + 0x50;
    LCDC_DMA_LINKLIST->GRP2_CTL0 = BIT(0)
                                   | (LCDC_DMA_Init->LCDC_DMA_DestinationDataSize << 1)
                                   | (LCDC_DMA_Init->LCDC_DMA_SourceDataSize << 4)
                                   | (LCDC_DMA_Init->LCDC_DMA_DestinationInc << 7)
                                   | (LCDC_DMA_Init->LCDC_DMA_SourceInc << 9)
                                   | (LCDC_DMA_Init->LCDC_DMA_DestinationMsize << 11)
                                   | (LCDC_DMA_Init->LCDC_DMA_SourceMsize << 14)
                                   | (LCDC_DMA_Init->LCDC_DMA_DIR << 20)
                                   | (LCDC_DMA_Init->LCDC_DMA_Multi_Block_Mode & LCDC_DMA_LLP_SELECTED_BIT);

    /* Enable WP */
    dmall_reg_0x44.d32 = LCDC_DMA_LINKLIST->DMA_CFG;
    dmall_reg_0x44.b.reg_dma_g1_wp = 1;
    dmall_reg_0x44.b.reg_dma_g2_wp = 1;
    LCDC_DMA_LINKLIST->DMA_CFG = dmall_reg_0x44.d32;
}

void LCDC_DMA_Infinite_Buf_Update(uint8_t *G1_SAR_buf, uint8_t *G2_SAR_buf)
{
    LCDC_DMA_LINKLIST->GRP1_SAR_FOR_INFINITE_MODE = (uint32_t)G1_SAR_buf;
    LCDC_DMA_LINKLIST->GRP2_SAR_FOR_INFINITE_MODE = (uint32_t)G2_SAR_buf;
}

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/

