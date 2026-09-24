/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Deliberately NOT including hal_dma.h or os_sync.h.
 *
 * The target is a bare-metal project. Both of those would drag in code it does not
 * otherwise build:
 *   - hal_dma.c (the channel allocator) is not in the project's file list, and it
 *     itself calls os_lock/os_unlock;
 *   - os_sync.c pulls in osif + os_mem + os_timer + sys_list + os_pm, an OS port, for
 *     what is two instructions on Cortex-M.
 * So the critical section and the channel allocator are implemented here instead, on
 * top of CMSIS and rtl_dma.c - and rtl_dma.c is already compiled by the project.
 * Nothing new has to be added to the build for DCI.
 */
#include "rtl_dci_int.h"
#include "rtl_dma.h"
#include "rtl_rcc.h"
#include "rtl_nvic.h"
#include "vector_table.h"

/*============================================================================*
 *                         SoC wiring
 *============================================================================*/
/*
 * DMA handshake index - RESOLVED (was TODO(HAL) 1/3).
 *
 * The request line is spec section 2.3: one DMA request per complete 32-bit word.
 * The index is an SoC-level mux value the IP spec does not carry, and it comes from
 * BBPRO4_DmaPorts_20260915_v0.xlsx, whose history says
 *
 *     20260914_v0  Add handshakes for AP FMC0~FMC3 and AP DCI, SCP DCI, XP DCI
 *
 * i.e. DCI handshakes landed 2026-09-14, which is why the SDK snapshot checked
 * earlier (AP table ending at IR_RX(48), reference sheet BB3_Dma1Ports_20250518)
 * did not have them yet.
 *
 * WHY 48 AND NOT THE 49 IN THAT SHEET. The sheet numbers one column for "AP
 * DMA0/1" jointly and lists DCI at 49, immediately after IR_RX(48). But AP DMA0
 * was dropped from the silicon, so DCI runs on AP DMA1, and SRC_PER is a
 * PER-CONTROLLER port number. On DMA1 the DCI port is 48. Per the SoC team:
 * "AP DMA0 被拿掉, 請用 AP DMA1 HS[48]".
 *
 * The SDK corroborates that DMA0 is gone on this core, independently of that note:
 *   - the AP RCCClock_TypeDef has DMAC1_CLOCK and no DMAC0_CLOCK
 *     (soc/driver/rcc/src/device/rtl87x3k/rtl_rcc_def.h), and defines DMA_CLOCK as
 *     NULL for AP while SCP defines it as DMAC0_CLOCK;
 *   - DMA_GetDMAChannelx() maps AP channels 16..23 onto DMA1_Channel0..7
 *     (soc/driver/dma/src/device/rtl87x3k/rtl_dma_int.c), with DMA1_CH_NUM0 = 16.
 *
 * So a DCI capture is only possible on a DMA1 channel - see
 * rtl_dci_dma_channel_alloc() below, which enforces that.
 */
#define DCI_DMA_HANDSHAKE_RX        (48U)

uint8_t rtl_dci_dma_handshake_rx(void)
{
    return DCI_DMA_HANDSHAKE_RX;
}

/*
 * DCI clock enable - NOT NEEDED FOR NOW (was TODO(HAL) 2/3, no longer blocking).
 *
 * Per the SoC team the DCI clock is enabled by default, so this is deliberately a
 * no-op rather than an unimplemented stub: there is nothing to call. The register
 * block is reachable as-is, which is why DCI_RegTest() and a capture-less
 * configuration run are both possible today.
 *
 * Kept as a function rather than deleted because the display island does have its
 * own power/clock domain and a future part may well gate it. What such an
 * implementation would need, if that day comes:
 *   - the AP RCCClock_TypeDef carries no display-island entry (no DCI, IDU, PPE,
 *     LCDC, MIPI, JPEG, PNG or GPU), so DCI would not simply join that enum;
 *   - the island has its own DISPLAY_AON(0x40645000) / DISPLAY_PLATFORM(0x40646000)
 *     / DISPLAY_PCC(0x40647000) / DISPLAY_RAP(0x4064a000) block, and
 *     DISPLAY_PCC_BASE appears in address_map.h with zero users anywhere in the
 *     tree - no register table has landed for it;
 *   - so the enable would most likely be a direct write to the display PCC, not an
 *     RCC_ClockCmd() call.
 *
 * Note RCC_PeriphClockCmd(p, c, s) would not be the API either: on this SoC
 * rtl_rcc_def.h defines it as RCC_ClockCmd(p, s). The sibling modules (rtl_idu.c,
 * rtl_jpu.c) call the three-argument form only because they are shared with older
 * parts.
 */
void rtl_dci_clock_enable(void)
{
    /* Intentionally empty - the DCI clock is on by default on this part. */

    /* Left commented rather than compiled out: if a part ever does gate DCI, this
     * is the line to restore, and #ifdef DCI_CLOCK would silently do nothing on a
     * build where the macro is simply absent - which is exactly how a missing clock
     * enable turns into an unexplained DCI_ERROR_NO_RESPONSE.
     *
     * RCC_ClockCmd(DCI_CLOCK, ENABLE);
     */
}

/*
 * DCI pin configuration - NOTHING TO DO ON THE FPGA PLATFORM.
 *
 * Empty because that is correct here, not because it is unfinished. On the VU13P FPGA
 * board the DCI signals are dedicated top-level RTL ports pinned by the .xdc, so they
 * reach the peripheral directly and never pass through the SoC pin mux.
 *
 * Evidence, from BBpro4_VU13P_FPGA_Display_FMC1DQ16_Pin_Assignment_20260915_v0.xlsx
 * (sheet "新版J4"; history 20260911_v1 "add DCMI signals"):
 *   - the Pin Name column IS the RTL port name (sheet "NOTE", row 20), and the DCI
 *     signals appear there as dcmi_d[13:0], dcmi_pixclk, dcmi_hsync, dcmi_vsync.
 *     Note the pin table routes 14 data lines while the IP only has ten: the spec is
 *     explicit ("DCI 拥有 10 条数据线 D[9:0]", EDM is EDM[0] alone, and unused inputs
 *     "必须将外部未使用的输入引脚接地"). So d[13:10] are spare board routing, not
 *     usable DCI data - the 8/10-bit enumeration in this driver is correct;
 *   - those names form their own class. The same sheet's other pins are either
 *     P_GPIOx_y (39 of them - SoC GPIOs, the ones a pin mux would apply to) or
 *     FPGA_IO_PIN_AP/BT/WIFI[n] (42 - reassignable FPGA IO). None of the dcmi_* names
 *     is a GPIO pin;
 *   - dcmi_pixclk (FPGA BB9, J4 G5) is on a clock-capable pin, marked *P_*_GC_*, which
 *     is what an externally driven pixel clock needs.
 * Consistent with the SDK: pin_def.h has no DCI/DCMI/DVP function number at all
 * (numbering ends at SPIC0_SIO3(72)), which is expected if these are dedicated pads.
 *
 * NOTE for ASIC bring-up: whether silicon also uses dedicated pads is NOT established
 * by the above - the FPGA board proves nothing about the ASIC's pad ring. 17 signals is
 * a lot to route through a general mux, so dedicated pads stay the likely answer, but
 * confirm against the BBPro4 pin list before trusting this function to stay empty
 * there.
 *
 * Board-level facts worth knowing when the capture does not work (they are wiring, not
 * driver configuration):
 *   - the DCI pins reuse the MIPI D-PHY debug and LCDC signals on J4, so a bitfile
 *     carrying DCMI excludes MIPI/LCDC on that connector;
 *   - the camera's I2C is FPGA_IO_PIN_AP[80]/[81] (J4 G29/G30), deliberately placed
 *     next to the DCI pins for sensor init.
 */
void rtl_dci_pinmux_init(void)
{
    /* Dedicated RTL ports on this platform - no mux to program. */
}

/*============================================================================*
 *                         Critical section
 *============================================================================*/
/*
 * PRIMASK save/disable/restore rather than os_lock(): this is a bare-metal target, and
 * os_lock() lives in an OS port the project does not build (see the note on the includes
 * at the top of this file).
 *
 * Saving and restoring PRIMASK rather than unconditionally re-enabling is what makes
 * this safe to nest and safe to call from an ISR - the driver does both. __disable_irq()
 * has the compiler barrier CMSIS gives it, so the register writes inside the section
 * cannot be hoisted out.
 */
uint32_t rtl_dci_lock(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    return primask;
}

void rtl_dci_unlock(uint32_t s)
{
    __set_PRIMASK(s);
}

/*============================================================================*
 *                         DMA channel properties
 *============================================================================*/
/*
 * Self-contained channel allocator, replacing hal_dma_channel_alloc().
 *
 * Why not the HAL one: it lives in soc/driver/dma/src/hal/hal_dma.c, which the
 * bare-metal project does not build, and which itself needs os_lock/os_unlock from an
 * OS port. Everything used below comes from rtl_dma.c, which the project already
 * compiles, plus CMSIS. Adding DCI therefore needs no new files in the build.
 *
 * WHICH CHANNELS. DCI must run on an AP DMA1 channel, which on this core is 16..23
 * (DMA1_CH_NUM0 is 16, and DMA_GetDMAChannelx() maps 16..23 onto DMA1_Channel0..7).
 * AP DMA0 was dropped from the silicon, so a DMA0 channel would be programmed into
 * registers nothing drives and the capture would simply never start. Scanning only
 * 16..23 is also why this is simpler than the HAL version, which always scans from 0
 * and would have to be coaxed upwards.
 *
 * HOW "FREE" IS DECIDED. DMA_GetChannelStatus(ch) reads CHENREG, i.e. whether the
 * channel is currently enabled. That is weaker than the HAL's allocation bitmask: it
 * cannot see a channel that some other module has reserved but not yet started. In a
 * bare-metal test image where DCI is the only DMA client on DMA1 that is sufficient, and
 * it is checked under a critical section so two callers cannot pick the same one. An
 * application that shares DMA1 should pass an explicit channel instead of AUTO.
 */
#define DCI_DMA1_CH_FIRST       (16U)   /* DMA1_CH_NUM0 on the AP core */

/*
 * Channel FIFO depth. This SoC does not fix the depth per channel number, and without
 * the HAL there is nobody to ask, so a conservative 8 is assumed for every channel:
 * DEST_MSIZE is derived from it, and being too small only costs burst efficiency, while
 * being too large would overrun the FIFO. The reg dump shows the resulting DEST_MSIZE.
 */
#define DCI_DMA_FIFO_DEPTH_ASSUMED  (8U)

bool rtl_dci_dma_channel_alloc(uint8_t *channel_num, void (*isr)(void))
{
    uint32_t s;
    uint8_t  ch;
    bool     got = false;

    if (channel_num == NULL)
    {
        return false;
    }

    /* An explicit channel outside DMA1 is a configuration error, not something to
     * silently correct: the caller asked for a channel that cannot carry DCI. */
    if (*channel_num != DCI_DMA_CH_AUTO &&
        (*channel_num < DCI_DMA1_CH_FIRST || *channel_num >= CHIP_DMA_CHANNEL_NUM))
    {
        return false;
    }

    s = rtl_dci_lock();

    if (*channel_num != DCI_DMA_CH_AUTO)
    {
        got = (DMA_GetChannelStatus(*channel_num) == RESET);
        ch  = *channel_num;
    }
    else
    {
        for (ch = DCI_DMA1_CH_FIRST; ch < CHIP_DMA_CHANNEL_NUM; ch++)
        {
            if (DMA_GetChannelStatus(ch) == RESET)
            {
                got = true;
                break;
            }
        }
    }

    if (got)
    {
        /* Claim it by disabling it explicitly, so a half-configured channel left behind
         * by an earlier run cannot start mid-setup. */
        DMA_Cmd(ch, DISABLE);
        *channel_num = ch;
    }

    rtl_dci_unlock(s);

    /* Outside the critical section: updating the vector table is not something to do
     * with interrupts masked, and the channel is already claimed by this point. */
    if (got && isr != NULL)
    {
        IRQn_Type        irq = DMA_GetDMAIRQx(ch);
        NVIC_InitTypeDef nvic_init;

        ram_vector_table_update(irq, (IRQ_Fun)isr);

        /*
         * And enable it. hal_dma_channel_alloc() only ever did the vector table update,
         * leaving the NVIC line to the caller - so on the HAL path this was missing and
         * dci_dma_isr() would never have run. Same priority as the DCI vector: the two
         * ISRs touch the same driver state, and equal priority means neither can
         * pre-empt the other.
         */
        nvic_init.NVIC_IRQChannel         = irq;
        nvic_init.NVIC_IRQChannelPriority = 3;
        nvic_init.NVIC_IRQChannelCmd      = ENABLE;
        NVIC_Init(&nvic_init);
    }

    return got;
}

void rtl_dci_dma_channel_free(uint8_t channel_num)
{
    if (channel_num < CHIP_DMA_CHANNEL_NUM)
    {
        /* Disabling is the whole of releasing here: there is no allocation bitmask to
         * clear, since this allocator infers "free" from the channel being disabled. */
        DMA_Cmd(channel_num, DISABLE);
    }
}

uint8_t rtl_dci_get_dma_depth(uint8_t channel_num)
{
    if (channel_num >= CHIP_DMA_CHANNEL_NUM)
    {
        return 0;
    }
    return DCI_DMA_FIFO_DEPTH_ASSUMED;
}

/* DEST_MSIZE selection lives in rtl_dci_dma.c next to the CTL_LOW composition that
 * consumes it. */

/*
 * DMA1, not DMA_CLOCK. On the AP core rtl_rcc_def.h defines DMA_CLOCK as NULL - only
 * the SCP core maps it to DMAC0_CLOCK - so the obvious RCC_ClockCmd(DMA_CLOCK, ENABLE)
 * compiles, runs and enables nothing. AP DMA0 was dropped from the silicon and DCI
 * runs on DMA1 (see the handshake note at the top of this file), so DMAC1_CLOCK is
 * both the correct clock and the only one that exists here.
 */
void rtl_dci_dma_clock_enable(void)
{
    RCC_ClockCmd(DMA1_CLOCK, ENABLE);
}

/*============================================================================*
 *                         Interrupt vectors
 *============================================================================*/
/*
 * All five DCI sources share one vector; DCI_Handler() demultiplexes them from MIS.
 * The DMA channel's own vector is hooked and enabled by rtl_dci_dma_channel_alloc(),
 * which is why there is no counterpart hook for it here.
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
