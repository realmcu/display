/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DMA abstraction for the DCI driver.
 *
 * The Realtek DMA driver has been through a rename: the older parts (rtl87x3g and
 * friends) expose GDMA_*, this one exposes DMA_*. Same DesignWare AHB DMAC, same
 * semantics, different identifiers - and a few genuinely different field widths.
 *
 * rtl_common/rtl_dci.c is written against the neutral names declared here and never
 * includes rtl_dma.h, so porting to another SoC family means providing a different
 * pair of rtl_dci_dma.h / rtl_dci_int.c and touching no business logic.
 */

#ifndef RTL_DCI_DMA_H
#define RTL_DCI_DMA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "rtl_dci.h"

/*============================================================================*
 *                         Hardware limits
 *============================================================================*/
/*
 * BLOCK_TS field width. 16 bit on this SoC family (CTLx_H bits 15:0) against 20 bit
 * on rtl87x3g - a 16x smaller block, which is why the block-count derivation in
 * DESIGN.md 6.3 has to be checked against this value rather than assumed.
 */
#define DCI_DMA_MAX_BLOCK_TS        65535UL

/*============================================================================*
 *                         Link list item
 *============================================================================*/
/*
 * Layout is identical across the family (SAR, DAR, LLP, CTL_LOW, CTL_HIGH, all
 * 32-bit, in that order), so this is declared locally rather than aliased. That
 * keeps the descriptor ring in rtl_common free of any SoC header.
 */
typedef struct
{
    volatile uint32_t SAR;
    volatile uint32_t DAR;
    volatile uint32_t LLP;
    volatile uint32_t CTL_LOW;
    volatile uint32_t CTL_HIGH;
} DCI_DMA_LLIDef;

/*============================================================================*
 *                         Handshake
 *============================================================================*/
/** \brief  Source handshake index for the DCI request line. See TODO(HAL) 1/3. */
uint8_t rtl_dci_dma_handshake_rx(void);

/*============================================================================*
 *                         Descriptor CTL_LOW composition
 *============================================================================*/
/**
 * \brief   Build the CTL_LOW word shared by every DCI descriptor.
 *
 * \param[in] channel_num  Channel the ring will run on; selects DEST_MSIZE.
 * \param[in] lli          true for a descriptor that chains on, false for one that
 *                         terminates the chain (LLP = 0).
 *
 * \note    Composed rather than read back from the channel so the ring can be built
 *          before the channel is programmed.
 */
uint32_t rtl_dci_dma_ctl_low(uint8_t channel_num, bool lli);

/*============================================================================*
 *                         Channel programming
 *============================================================================*/
/**
 * \brief   Load a channel for one descriptor chain and leave it disabled.
 *
 * Fixed by the DCI data path and therefore not parameters: peripheral-to-memory
 * direction, source fixed at &DCI->DR, destination incrementing, word width on both
 * sides, SRC_MSIZE = 1 (spec 2.3 raises one request per 32-bit word, so the source
 * cannot burst), multi-block mode LLI-with-contiguous-SAR, and the source handshake.
 *
 * \param[in] channel_num  Channel to program.
 * \param[in] src          Source address, always &DCI->DR.
 * \param[in] dar          Destination address of the chain's first block.
 * \param[in] block_ts     Transfer size of the first block, in words.
 * \param[in] lli_head     First descriptor of the chain.
 * \param[in] priority     Channel priority, 0 highest.
 */
void rtl_dci_dma_arm(uint8_t channel_num, uint32_t src, uint32_t dar,
                     uint32_t block_ts, const DCI_DMA_LLIDef *lli_head,
                     uint8_t priority);

/** \brief  Enable or disable a channel. */
void rtl_dci_dma_cmd(uint8_t channel_num, bool enable);

/** \brief  Unmask exactly one of the two completion interrupts, masking the other. */
void rtl_dci_dma_int_select(uint8_t channel_num, bool block_not_transfer);

/** \brief  Clear every pending interrupt of a channel. */
void rtl_dci_dma_int_clear_all(uint8_t channel_num);

/** \brief  Did the block-complete interrupt fire? Clears it when it did. */
bool rtl_dci_dma_int_take_block(uint8_t channel_num);

/** \brief  Did the transfer-complete interrupt fire? Clears it when it did. */
bool rtl_dci_dma_int_take_transfer(uint8_t channel_num);

/** \brief  Current destination address, i.e. how far the channel has written. */
uint32_t rtl_dci_dma_curr_dar(uint8_t channel_num);

/** \brief  True when the channel FIFO has drained to memory. */
bool rtl_dci_dma_fifo_empty(uint8_t channel_num);

#if (DCI_DEBUG_REG_DUMP == 1)
/** \brief  Collect and decode channel + global DMA registers. DCI_DMA_regs_t is
 *          declared in rtl_dci.h so the dump stays SoC-independent. */
void rtl_dci_dma_read_regs(uint8_t channel_num, DCI_DMA_regs_t *regs);
#endif

#ifdef __cplusplus
}
#endif

#endif /* RTL_DCI_DMA_H */
