/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtl_dci_int.h"
#include "rtl_dma.h"
#include "hal_dma.h"
#include "rtl_rcc.h"
#include "rtl_nvic.h"
#include "vector_table.h"
#include "os_sync.h"

/*============================================================================*
 *                         HAL placeholders
 *============================================================================*/
/*
 * TODO(HAL) 1/3 - DMA handshake index. Blocks capture on real silicon.
 *
 * The hardware request line does exist - spec section 2.3 says a DMA request is
 * raised every time a complete 32-bit word has been assembled - but the index is
 * an SoC-level mux value that the IP spec does not carry.
 *
 * Re-checked against SDK 4cf8219f7 (2026-09-16) / driver 27c144b0 (2026-09-15):
 * the AP-core table in rtl_dma_def.h is still 0..48, ending GPIO_GDMA_TIMER0_7(46),
 * IR_TX(47), IR_RX(48), with no DCI / DCMI / DVP / camera / CSI entry and no gap
 * to occupy - 49 upwards is simply absent. Its reference sheet is
 * BB3_Dma1Ports_20250518.xlsx, i.e. the port list predates the display island
 * landing in the tree at all. Ask the SoC team for DMA_HANDSHAKE_DCI_RX and for a
 * Dma1Ports revision newer than 2025-05-18.
 *
 * 0xFF is deliberately invalid so that a reg dump shows SRC_PER=0xFF and the
 * placeholder cannot be mistaken for a working configuration.
 */
#define DCI_DMA_HANDSHAKE_RX        (0xFFU)

uint8_t rtl_dci_dma_handshake_rx(void)
{
    return DCI_DMA_HANDSHAKE_RX;
}

/*
 * TODO(HAL) 2/3 - DCI clock enable. Blocks everything, including the register
 * self test.
 *
 * Re-checked against SDK 4cf8219f7 (2026-09-16):
 *   - rtl_rcc_def.h's AP-core RCCClock_TypeDef still has no display-island entry
 *     at all - no DCI, IDU, PPE, LCDC, MIPI, JPEG, PNG or GPU.
 *   - DISPLAY_PCC_BASE (0x40647000) still appears in address_map.h and NOWHERE
 *     else in the tree: no register table, no user.
 *   - ap/pcc_reg.h describes PCC0 at 0x40702000, whose registers are all
 *     PCC0_DMY_n placeholders, and mentions no display domain.
 *
 * The wider finding is that this is not a DCI-specific gap. Every display-island
 * base - JPEG_DECODER_CFG(0x40640000), MIPI(0x40641000), DCI(0x40643000),
 * PNG(0x40644000), IDU(0x40650000) - has zero users in the SDK, while the vector
 * table already carries the whole island's interrupts (JPEG 179, PNG 180,
 * MIPI 181, DCI 182, RXI300_DISPLAY 189). So the island exists in silicon and in
 * the IRQ map, but its clock/power HAL has not landed for any of its peripherals.
 * Whoever enables the first one unblocks the rest.
 *
 * Until then DCI_RegTest() reports DCI_ERROR_NO_RESPONSE, which is the correct and
 * self-explanatory symptom rather than a silent failure.
 *
 * Note RCC_PeriphClockCmd(p, c, s) is not the API to use here: on this SoC
 * rtl_rcc_def.h defines it as RCC_ClockCmd(p, s), so the single-argument form
 * below is the native one. The sibling modules (rtl_idu.c, rtl_jpu.c) call the
 * three-argument form only because they are shared with older parts.
 */
void rtl_dci_clock_enable(void)
{
#ifdef DCI_CLOCK
    RCC_ClockCmd(DCI_CLOCK, ENABLE);
#else
    /* Intentionally empty: see TODO(HAL) 2/3 above. Once the display PCC is known,
     * this becomes either RCC_ClockCmd(DCI_CLOCK, ENABLE) if DCI joins the main
     * clock enum, or a direct write to the display PCC register. */
#endif
}

/*
 * TODO(HAL) 3/3 - DCI pin configuration: D[9:0], PIXCLK, HSYNC, VSYNC.
 *
 * Re-checked against SDK 4cf8219f7 (2026-09-16): rtl_pinmux_def.h and
 * pinmux/inc/rtl87x3k/pin_def.h still carry no DCI / DCMI / DVP / CAM / PIXCLK /
 * HSYNC / VSYNC entry, and no display-class pin function of any kind - pin_def.h's
 * function numbering runs out at SPIC0_SIO3(72).
 *
 * Two possibilities, and which one holds decides whether this function stays empty:
 *   - DCI uses dedicated pads and needs no pinmux at all (13 signals is a lot of
 *     pins to route through a general mux, so this is plausible), or
 *   - the function table simply has not landed yet, consistent with the rest of the
 *     display island being absent - see TODO(HAL) 2/3.
 *
 * Ask for the BBPro4 pin list before assuming the first.
 */
void rtl_dci_pinmux_init(void)
{
    /* Intentionally empty: see TODO(HAL) 3/3 above. */
}

/*============================================================================*
 *                         Critical section
 *============================================================================*/
uint32_t rtl_dci_lock(void)
{
    return os_lock();
}

void rtl_dci_unlock(uint32_t s)
{
    os_unlock(s);
}

/*============================================================================*
 *                         DMA channel properties
 *============================================================================*/
/*
 * Channel FIFO depth. Unlike the older parts, this SoC does not fix the depth per
 * channel number - hal_dma_channel_alloc() is asked for a depth instead - so the
 * value is recorded at allocation time rather than derived from a hard-coded table.
 *
 * Kept as a lookup so that a caller who pins a channel number (rather than passing
 * DCI_DMA_CH_ANY) still gets a sane answer.
 */
static uint8_t dci_dma_depth[CHIP_DMA_CHANNEL_NUM];

/*
 * The pixel stream must not stall, so ask for the deepest FIFO. A shallower one is
 * accepted rather than failing outright: it still works, only with less headroom
 * against bus contention, and the reg dump makes the resulting DEST_MSIZE visible.
 */
bool rtl_dci_dma_channel_alloc(uint8_t *channel_num, void (*isr)(void))
{
    static const struct
    {
        T_HAL_DMA_CHANNEL_TYPE type;
        uint8_t                depth;
    } wanted[] =
    {
        {HAL_DMA_FIFO_DEPTH_64, 64},
        {HAL_DMA_FIFO_DEPTH_32, 32},
        {HAL_DMA_FIFO_DEPTH_16, 16},
        {HAL_DMA_DEFAULT,        8},
    };
    uint8_t ch;
    uint8_t i;

    if (channel_num == NULL)
    {
        return false;
    }

    for (i = 0; i < sizeof(wanted) / sizeof(wanted[0]); i++)
    {
        ch = *channel_num;
        if (hal_dma_channel_alloc(&ch, (void *)isr, wanted[i].type) && ch < CHIP_DMA_CHANNEL_NUM)
        {
            dci_dma_depth[ch] = wanted[i].depth;
            *channel_num      = ch;
            return true;
        }
    }
    return false;
}

void rtl_dci_dma_channel_free(uint8_t channel_num)
{
    if (channel_num < CHIP_DMA_CHANNEL_NUM)
    {
        dci_dma_depth[channel_num] = 0;
        hal_dma_channel_free(channel_num);
    }
}

uint8_t rtl_dci_get_dma_depth(uint8_t channel_num)
{
    if (channel_num >= CHIP_DMA_CHANNEL_NUM)
    {
        return 0;
    }
    /* Not allocated through us: assume the smallest FIFO the HAL can hand out, so
     * DEST_MSIZE stays conservative. */
    return (dci_dma_depth[channel_num] != 0) ? dci_dma_depth[channel_num] : 8;
}

/* DEST_MSIZE selection lives in rtl_dci_dma.c next to the CTL_LOW composition that
 * consumes it. */

/* DMA_CLOCK is DMAC0_CLOCK on this part; DMAC1 has its own DMA1_CLOCK. Only DMAC0
 * is used here because hal_dma_channel_alloc() allocates from it. */
void rtl_dci_dma_clock_enable(void)
{
    RCC_ClockCmd(DMA_CLOCK, ENABLE);
}

/*============================================================================*
 *                         Interrupt vectors
 *============================================================================*/
/*
 * All five DCI sources share one vector; DCI_Handler() demultiplexes them from MIS.
 * The DMA channel vector is registered by hal_dma_channel_alloc() itself, which is
 * why there is no counterpart hook for it here.
 */
void rtl_dci_irq_init(void (*isr)(void))
{
    NVIC_InitTypeDef nvic_init;

    ram_vector_table_update(DCI_IRQn, (IRQ_Fun)isr);

    nvic_init.NVIC_IRQChannel         = DCI_IRQn;
    nvic_init.NVIC_IRQChannelPriority = 3;
    nvic_init.NVIC_IRQChannelCmd      = ENABLE;
    NVIC_Init(&nvic_init);
}

/* Logging is a macro (DCI_LOG in rtl_dci_int.h); nothing to implement here. */
