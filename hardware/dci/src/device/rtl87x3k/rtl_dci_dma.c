/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DMA abstraction for the DCI driver, RTL87X3K (BBPro4) application processor.
 *
 * This is the only file in the driver that spells DMA_* identifiers. Everything the
 * capture logic needs is exposed through the neutral names in rtl_dci_dma.h.
 */

#include "rtl_dci_int.h"
#include "rtl_dma.h"

/*============================================================================*
 *                         Compile-time cross-checks
 *============================================================================*/
/* If either of these ever fires, the shim is out of step with the DMA driver and
 * the descriptor ring would be built wrong at runtime instead of failing here. */
_Static_assert(sizeof(DCI_DMA_LLIDef) == sizeof(DMA_LLIDef),
               "LLI layout diverged from the SoC DMA driver");

/*
 * DCI_MAX_BLOCK_TS drives the block-count derivation in rtl_common. If it ever
 * exceeds what CTLx_H can hold, every frame larger than one block would be
 * programmed with a truncated size and silently lose data - so pin it here, where
 * the register definition is in scope.
 */
_Static_assert(DCI_MAX_BLOCK_TS == DCI_DMA_MAX_BLOCK_TS,
               "public DCI_MAX_BLOCK_TS disagrees with the SoC DMA shim");
_Static_assert(DCI_DMA_MAX_BLOCK_TS <= 0xFFFFUL,
               "BLOCK_TS is 16 bit on this SoC (DMA_CTLx_H_W_TypeDef.block_ts)");

/*============================================================================*
 *                         Descriptor CTL_LOW composition
 *============================================================================*/
/*
 * Field positions of CTLx_L on this SoC:
 *   int_en[0] dst_tr_width[3:1] src_tr_width[6:4] dinc[8:7] sinc[10:9]
 *   dest_msize[13:11] src_msize[16:14] tt_fc[23:20] llp_dst_en[27] llp_src_en[28]
 *
 * Note tt_fc is 4 bits here (23:20) against 3 bits on rtl87x3g - the extra bit
 * carries the peripheral-flow-control directions. The value DCI uses
 * (PERIPHERAL_TO_MEMORY = 2) is unaffected, but the width matters to anyone masking
 * the field later.
 */
#define DCI_CTL_INT_EN              BIT0
#define DCI_CTL_DST_WIDTH_POS       1
#define DCI_CTL_SRC_WIDTH_POS       4
#define DCI_CTL_DINC_POS            7
#define DCI_CTL_SINC_POS            9
#define DCI_CTL_DEST_MSIZE_POS      11
#define DCI_CTL_SRC_MSIZE_POS       14
#define DCI_CTL_TT_FC_POS           20

/*
 * Only the destination side is free to burst: SRC_MSIZE is pinned to 1 because DCI
 * raises one request per word, but the destination is memory with no handshake, so
 * draining the channel FIFO in bursts is both allowed and worth more than any
 * block-count tuning. A short tail is completed with single transactions, so
 * BLOCK_TS need not be a multiple of this.
 *
 * This part has DMA_SUPPORT_MSIZE_BIGGER_THAN_FIFO_DEPTH == 0 on the AP core, so the
 * burst must not exceed the real FIFO depth - hence the depth-driven cap rather than
 * a fixed Msize.
 */
static DMAMSize_TypeDef dci_dest_msize(uint8_t channel_num)
{
    uint8_t depth = rtl_dci_get_dma_depth(channel_num);

    if (depth >= 32)
    {
        return DMA_MSIZE_16;
    }
    else if (depth >= 16)
    {
        return DMA_MSIZE_8;
    }
    else if (depth >= 8)
    {
        return DMA_MSIZE_4;
    }
    return DMA_MSIZE_1;
}

uint32_t rtl_dci_dma_ctl_low(uint8_t channel_num, bool lli)
{
    uint32_t v = DCI_CTL_INT_EN
                 | ((uint32_t)DMA_DATA_SIZE_WORD          << DCI_CTL_DST_WIDTH_POS)
                 | ((uint32_t)DMA_DATA_SIZE_WORD          << DCI_CTL_SRC_WIDTH_POS)
                 | ((uint32_t)DMA_DESTINATION_INC         << DCI_CTL_DINC_POS)
                 | ((uint32_t)DMA_SOURCE_FIX              << DCI_CTL_SINC_POS)
                 | ((uint32_t)dci_dest_msize(channel_num) << DCI_CTL_DEST_MSIZE_POS)
                 | ((uint32_t)DMA_MSIZE_1                 << DCI_CTL_SRC_MSIZE_POS)
                 | ((uint32_t)DMA_DIR_PERIPHERAL_TO_MEMORY << DCI_CTL_TT_FC_POS);

    /* Clearing both LLP select bits is what marks a descriptor as the last block of
     * its chain, which is how SNAPSHOT gets its LLP = 0 stop. */
    if (lli)
    {
        v |= LLI_WITH_CONTIGUOUS_SAR;
    }
    return v;
}

/*============================================================================*
 *                         Channel programming
 *============================================================================*/
void rtl_dci_dma_arm(uint8_t channel_num, uint32_t src, uint32_t dar,
                     uint32_t block_ts, const DCI_DMA_LLIDef *lli_head,
                     uint8_t priority)
{
    DMA_ChannelTypeDef *chan = DMA_GetDMAChannelx(channel_num);
    DMA_InitTypeDef     init;

    DMA_StructInit(&init);
    init.DMA_ChannelNum          = channel_num;
    init.DMA_Direction           = DMA_DIR_PERIPHERAL_TO_MEMORY;
    init.DMA_SourceInc           = DMA_SOURCE_FIX;
    init.DMA_DestinationInc      = DMA_DESTINATION_INC;
    init.DMA_SourceDataSize      = DMA_DATA_SIZE_WORD;
    init.DMA_DestinationDataSize = DMA_DATA_SIZE_WORD;
    init.DMA_SourceMsize         = DMA_MSIZE_1;
    init.DMA_DestinationMsize    = dci_dest_msize(channel_num);
    init.DMA_SourceAddr          = src;
    init.DMA_DestinationAddr     = dar;
    init.DMA_BufferSize          = block_ts;
    init.DMA_ChannelPriority     = priority;
    init.DMA_MultiBlockMode      = LLI_WITH_CONTIGUOUS_SAR;
    init.DMA_MultiBlockStruct    = (uint32_t)lli_head;
    init.DMA_MultiBlockEn        = 1;
    init.DMA_SourceHandshake     = rtl_dci_dma_handshake_rx();

    DMA_Init(chan, &init);
}

void rtl_dci_dma_cmd(uint8_t channel_num, bool enable)
{
    DMA_Cmd(channel_num, enable ? ENABLE : DISABLE);
}

/*
 * A closed ring never presents LLP = 0, so transfer-complete can never fire and
 * block-complete is the only completion signal. The LLP = 0 topologies are the
 * mirror image. Exactly one is unmasked so the ISR cannot be reached by the wrong
 * event (DESIGN.md 6.4).
 */
void rtl_dci_dma_int_select(uint8_t channel_num, bool block_not_transfer)
{
    DMA_INTConfig(channel_num, DMA_INT_TRANSFER | DMA_INT_BLOCK, DISABLE);
    DMA_INTConfig(channel_num,
                  block_not_transfer ? DMA_INT_BLOCK : DMA_INT_TRANSFER, ENABLE);
}

void rtl_dci_dma_int_clear_all(uint8_t channel_num)
{
    DMA_ClearAllTypeINT(channel_num);
}

/*
 * An ERROR interrupt has to be cleared here, and this is the only place that can.
 *
 * The channel raises TRANSFER, BLOCK and ERROR onto one NVIC line. rtl_dci.c's ISR only
 * ever asks about the first two, so an ERROR would leave its pending bit set: the ISR
 * returns, the NVIC re-enters immediately, and the CPU never gets back to the thread. The
 * symptom is a total hang with no further output - not an error report - which is why
 * this is swallowed at the point every ISR entry passes through rather than left to the
 * common driver (which must not be modified for a SoC quirk anyway).
 *
 * Counted so it is visible: the count is printed by the register dump, and a non-zero
 * value means the DMA rejected a descriptor or a transfer - usually a bad address or a
 * width/handshake mismatch, not a pixel problem.
 */
static volatile uint32_t dci_dma_err_count;

static void dci_dma_take_error(uint8_t channel_num)
{
    if (DMA_GetINTStatus(channel_num, DMA_INT_ERROR) == SET)
    {
        DMA_ClearINTPendingBit(channel_num, DMA_INT_ERROR);
        dci_dma_err_count++;
    }
}

uint32_t rtl_dci_dma_err_count(void)
{
    return dci_dma_err_count;
}

/*
 * DMA_GetINTStatus() is used rather than reading the global status registers
 * directly: this SoC splits them into _L / _H halves and the driver already knows
 * which half a channel lives in.
 */
bool rtl_dci_dma_int_take_block(uint8_t channel_num)
{
    /* First thing on every ISR entry - see dci_dma_take_error() above. */
    dci_dma_take_error(channel_num);

    if (DMA_GetINTStatus(channel_num, DMA_INT_BLOCK) != SET)
    {
        return false;
    }
    DMA_ClearINTPendingBit(channel_num, DMA_INT_BLOCK);
    return true;
}

bool rtl_dci_dma_int_take_transfer(uint8_t channel_num)
{
    /* Also here: the ISR reaches this without going through take_block() when the
     * BLOCK source is masked, and an uncleared ERROR hangs the part either way. */
    dci_dma_take_error(channel_num);

    if (DMA_GetINTStatus(channel_num, DMA_INT_TRANSFER) != SET)
    {
        return false;
    }
    DMA_ClearINTPendingBit(channel_num, DMA_INT_TRANSFER);
    return true;
}

/*
 * Reads CURR_DAR - where the DMA has written up to - by touching the register directly,
 * because this SDK tree has no accessor for it.
 *
 * CAREFUL, the two are easy to confuse and only one is correct here:
 *   DMA_DARx      (0x08) is the block's PROGRAMMED destination, and never moves;
 *   DMA_CURR_DARx (0x0C) is where the transfer has actually got to.
 * soc/driver/dma offers DMA_GetDstTransferAddress(), which returns DMA_DARx - using it
 * would make every delivered frame report its own start address as the end, i.e. a
 * length of zero. The sibling tree driver/dma does have DMA_GetCurrDstAddr() for
 * CURR_DAR, but the AP project builds against soc/driver, where it does not exist.
 *
 * Reading the field is what the register dump below already does, so this stays
 * consistent with the rest of the file.
 */
uint32_t rtl_dci_dma_curr_dar(uint8_t channel_num)
{
    DMA_ChannelTypeDef *chan = DMA_GetDMAChannelx(channel_num);

    return (chan != NULL) ? chan->DMA_CURR_DARx : 0U;
}

bool rtl_dci_dma_fifo_empty(uint8_t channel_num)
{
    return (DMA_GetFIFOStatus(DMA_GetDMAChannelx(channel_num)) == SET);
}

/*============================================================================*
 *                         Register dump support
 *============================================================================*/
#if (DCI_DEBUG_REG_DUMP == 1)

/*
 * CFGx_H handshake fields: src_per[10:7] and dest_per[14:11], each widened to 7 bits
 * by three extended bits scattered higher up the word - src at 15/17/19, dest at
 * 16/18/20. All of them have to be reassembled, not just the low nibble: the DCI
 * handshake is 48, whose low nibble is 0, so decoding four bits alone would print
 * src_per=0 and read as UART0_TX - a wrong value that looks entirely plausible.
 */
#define DCI_CFGH_SRC_PER(h)     ((((h) >> 7)  & 0x0FU)        | \
                                 ((((h) >> 15) & 1U) << 4)    | \
                                 ((((h) >> 17) & 1U) << 5)    | \
                                 ((((h) >> 19) & 1U) << 6))
#define DCI_CFGH_DEST_PER(h)    ((((h) >> 11) & 0x0FU)        | \
                                 ((((h) >> 16) & 1U) << 4)    | \
                                 ((((h) >> 18) & 1U) << 5)    | \
                                 ((((h) >> 20) & 1U) << 6))

void rtl_dci_dma_read_regs(uint8_t channel_num, DCI_DMA_regs_t *regs)
{
    DMA_ChannelTypeDef *chan = DMA_GetDMAChannelx(channel_num);
    DMA_TypeDef        *dmax;

    if (regs == NULL || chan == NULL)
    {
        return;
    }

    regs->sar      = chan->DMA_SARx;
    regs->curr_sar = chan->DMA_CURR_SARx;
    regs->dar      = chan->DMA_DARx;
    regs->curr_dar = chan->DMA_CURR_DARx;
    regs->llp      = chan->DMA_LLPx_L;
    regs->ctl_low  = chan->DMA_CTLx_L;
    regs->ctl_high = chan->DMA_CTLx_H;
    regs->cfg_low  = chan->DMA_CFGx_L;
    regs->cfg_high = chan->DMA_CFGx_H;

    regs->dest_msize = (regs->ctl_low >> DCI_CTL_DEST_MSIZE_POS) & 0x7U;
    regs->src_msize  = (regs->ctl_low >> DCI_CTL_SRC_MSIZE_POS) & 0x7U;
    regs->tt_fc      = (regs->ctl_low >> DCI_CTL_TT_FC_POS) & 0xFU;
    regs->llp_dst_en = ((regs->ctl_low & LLI_WITH_CONTIGUOUS_SAR) != 0);
    regs->llp_src_en = ((regs->ctl_low & LLI_WITH_CONTIGUOUS_DAR) != 0);

    regs->hs_sel_dst = ((regs->cfg_low & BIT(10)) != 0);
    regs->hs_sel_src = ((regs->cfg_low & BIT(11)) != 0);
    regs->fifo_empty = (DMA_GetFIFOStatus(chan) == SET);

    regs->src_per  = (uint8_t)DCI_CFGH_SRC_PER(regs->cfg_high);
    regs->dest_per = (uint8_t)DCI_CFGH_DEST_PER(regs->cfg_high);

    /* Channels 16..23 are DMA1 on the AP core (DMA1_CH_NUM0 = 16), and DCI only ever
     * runs there - AP DMA0 was dropped from the silicon. */
    dmax = (channel_num < 16) ? DMA0 : DMA1;
    regs->ch_en        = dmax->DMA_CHENREG_L;
    regs->status_tfr   = dmax->DMA_STATUSTFR_L;
    regs->status_block = dmax->DMA_STATUSBLOCK_L;
    regs->status_err   = dmax->DMA_STATUSERR_L;
}

#endif /* DCI_DEBUG_REG_DUMP */
