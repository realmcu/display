/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SoC-specific hooks for the DCI driver.
 *
 * Everything that depends on the SoC HAL lives behind this interface so that
 * rtl_common/rtl_dci.c stays portable and free of OS and trace dependencies.
 *
 * Target: RTL87X3K (BBPro4), application processor core.
 */

#ifndef RTL_DCI_INT_H
#define RTL_DCI_INT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rtl_dci.h"
#include "rtl_dci_dma.h"

/*============================================================================*
 *                         Memory placement
 *============================================================================*/
/*
 * The descriptor ring is written by the CPU and read by the DMA. If it sits in
 * cacheable write-back memory the DMA can fetch a stale descriptor and write to
 * the wrong address, so the driver has to make it visible itself. Framebuffers
 * are a different matter: those belong to the caller and the driver does not
 * check or maintain them.
 */
#ifndef DCI_NONCACHE_SECTION
/* TODO(HAL): point at a real non-cacheable section.
 *
 * Re-checked against SDK 4cf8219f7 (2026-09-16): kernel/linker/inc/section.h still
 * offers placement sections only - RAM_DATA_SECTION, BUFFER_RAM_DATA_SECTION,
 * SHM_DATA_SECTION, RAM_NON_RETENTION_SECTION, the overlay set - none carrying a
 * cacheability attribute, and no DTCM/ITCM section macro. fmc_platform.h's
 * FMC_MAINx_NON_CACHE_ADDR() still covers flash (SPIC) only.
 *
 * Not a blocker: the ring is written only on Init / Reconfig / SetBuffers, never
 * per frame, so a single clean after writing is equally acceptable. Placing it in
 * DTCM also works - TCM does not go through the cache. Whichever the SoC HAL ends
 * up offering, it is confined to this one macro. */
#define DCI_NONCACHE_SECTION
#endif

/*============================================================================*
 *                         Critical section
 *============================================================================*/
/*
 * Guards buf_lock_mask, ready_head/ready_count, state and the rendezvous flags.
 *
 * MUST disable interrupts (or equivalent) - never a semaphore or mutex.
 * DCI_Capture_ReleaseBuffer() is explicitly allowed from the frame_ready callback,
 * i.e. from ISR context: taking a semaphore there would block or assert, and a
 * spinlock would deadlock when the ISR preempts a thread holding it.
 */
uint32_t rtl_dci_lock(void);
void     rtl_dci_unlock(uint32_t s);

/*============================================================================*
 *                         DMA integration
 *============================================================================*/
/*
 * Channel setup and programming live in rtl_dci_dma.{h,c}; what remains here is the
 * allocation lifecycle and the clocks.
 */

/**
 * \brief  Reserve a DMA channel deep enough for a pixel stream.
 *
 * \param[in,out] channel_num  Requested channel, or DCI_DMA_CH_ANY to let the HAL
 *                             pick one; updated with the channel actually given.
 * \param[in]     isr          Channel ISR, registered in the RAM vector table.
 *
 * \return  true on success.
 */
bool rtl_dci_dma_channel_alloc(uint8_t *channel_num, void (*isr)(void));

/** \brief  Release a channel taken by rtl_dci_dma_channel_alloc(). */
void rtl_dci_dma_channel_free(uint8_t channel_num);

/** \brief  Channel FIFO depth in items. 0 if the channel number is invalid. */
uint8_t rtl_dci_get_dma_depth(uint8_t channel_num);

/** \brief  Enable the DMA peripheral clock. */
void rtl_dci_dma_clock_enable(void);

/** \brief  Enable the DCI peripheral clock. Must succeed before any register access. */
void rtl_dci_clock_enable(void);

/** \brief  Configure the DCI pins: D[9:0], PIXCLK, HSYNC, VSYNC. */
void rtl_dci_pinmux_init(void);

/** \brief  Hook the DCI global ISR and enable its NVIC vector. */
void rtl_dci_irq_init(void (*isr)(void));

/** \brief  Let the HAL pick the DMA channel. */
#define DCI_DMA_CH_ANY              0xFFU

/*============================================================================*
 *                         Logging backend
 *============================================================================*/
/*
 * A macro rather than a function so that the variadic arguments expand at the call
 * site: DBG_DIRECT is itself a macro and cannot be handed a va_list. trace.h is
 * pulled in only for debug builds, so rtl_common stays trace-free in release.
 */
#if (DCI_DEBUG_REG_DUMP == 1)
#include "trace.h"
#ifndef DCI_LOG
#define DCI_LOG(...)        DBG_DIRECT(__VA_ARGS__)
#endif
#else
#define DCI_LOG(...)        do { } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* RTL_DCI_INT_H */
