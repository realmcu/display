/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_VCDJ_SOC_CFG_H
#define RTL_VCDJ_SOC_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *   CONFIRMED: REG_BASE, CORE_OFFSET, REG_WINDOW_SIZE - THE REST IS PLACEHOLDER
 *============================================================================*/
/**
 * SoC integration constants for the VeriSilicon VC9000DJ (DecoderJ) core on
 * RTL87x3K.
 *
 * THIS IS THE ONE FILE TO EDIT WHEN THE HARDWARE CONTRACT ARRIVES. Apart from
 * the three values above, nothing below is a real, confirmed hardware parameter:
 * the values are deliberately obviously invalid rather than plausible, so that
 * nobody mistakes them for measured data.
 *
 * DO NOT copy register, clock or reset values from hardware/jpu. The Realtek JPU
 * is a different IP with a different register map, a different clock gate and
 * different completion bits.
 *
 * Still required from the SoC / FPGA side before anything here can be trusted:
 *   - number of cores
 *   - IRQ number, trigger type/polarity, interrupt controller routing, how the
 *     status is cleared
 *   - who owns clock / reset / power domain and the power-up and reset ordering
 *   - AXI address width, CPU virtual/physical/bus address relationship, which RAM
 *     regions the core can reach at all
 *   - alignment and contiguity requirements for input bitstream, output frame and
 *     table memory
 *   - cache coherency attributes, and clean/invalidate granularity plus barriers
 *     on a non-coherent path
 *   - presence of IOMMU/MMU, DEC400, AFBC, VCMD, AXI_FE (all assumed absent)
 *   - data endianness and any FPGA bridge byte swap
 */

/**
 * \brief Subsystem MMIO base address of the DecoderJ core.
 *
 * Confirmed on the BBpro4 FPGA bench: reading swreg0 through this base returns
 * the ASIC ID 0x90211010, and swreg309 the build ID 0x80002070 whose low half
 * is Build ID 0x2070, the release the vendored control software targets.
 *
 * The region is 0x40644000-0x40644FFF, i.e. 4 KB.
 */
#define VCDJ_SOC_REG_BASE               0x40644000UL

/**
 * \brief Byte offset of the decoder core inside the subsystem register space.
 *
 * From the vendor FPGA reference configuration
 * (`software/linux/subsys_driver/subsys_cfg.h`, core type HW_VCDJ).
 */
#define VCDJ_SOC_CORE_OFFSET            0x800U

/**
 * \brief Size of one core's register window in bytes.
 *
 * 0x2070 decodes max_regs = 428, and the vendor flushes regs[0..max_regs], i.e.
 * 429 registers (`vpu_features_list.h`, `regdrv.c`). 0x800 + 0x6B4 fits the
 * 4 KB region.
 *
 * Do not use MAX_REG_COUNT (896): that window runs past the region, and touching
 * the tail wedges the bus until the FPGA bitfile is reloaded.
 */
#define VCDJ_SOC_REG_WINDOW_SIZE        (429U * 4U)

/** \brief Number of DecoderJ cores. TODO(hardware-contract): assumed single core. */
#define VCDJ_SOC_CORE_COUNT             1U

/**
 * \brief Interrupt line of the DecoderJ core.
 *
 * TODO(hardware-contract): unknown. -1 means "no IRQ wired"; the IRQ build
 * variant must refuse to initialise rather than connect to an arbitrary vector.
 */
#define VCDJ_SOC_IRQ_NUMBER             (-1)

/** \brief Interrupt priority. TODO(hardware-contract): unknown. */
#define VCDJ_SOC_IRQ_PRIORITY           0U

/**
 * \brief 1 when \ref VCDJ_SOC_REG_BASE is a real mapped address.
 *
 * Everything that touches the register window must check this and degrade to a
 * clean error, otherwise the first bring-up attempt faults instead of reporting
 * "hardware contract missing".
 */
#define VCDJ_SOC_REG_BASE_VALID         (VCDJ_SOC_REG_BASE != 0x00000000UL)

/** \brief 1 when a usable IRQ line is known. */
#define VCDJ_SOC_IRQ_VALID              (VCDJ_SOC_IRQ_NUMBER >= 0)

/**
 * \brief Clock / reset control.
 *
 * TODO(hardware-contract): unknown. The Realtek JPU uses
 * `RCC_PeriphClockCmd(APBPeriph_JPEG, APBPeriph_JPEG_CLOCK, ENABLE)`; that gate
 * belongs to a different IP and must NOT be reused here. Until the real clock and
 * reset owner is identified, the platform layer performs no clock or reset action
 * at all.
 */
#define VCDJ_SOC_HAS_CLOCK_CONTROL      0
#define VCDJ_SOC_HAS_RESET_CONTROL      0

/**
 * \brief 1 when the CPU and the decoder core see memory coherently.
 *
 * TODO(hardware-contract): unknown. Assumed non-coherent, which is the
 * conservative choice; the cache maintenance hooks are still barrier-only stubs
 * because the cache API and the required granularity have not been confirmed.
 */
#define VCDJ_SOC_DMA_COHERENT           0

#ifdef __cplusplus
}
#endif

#endif /* RTL_VCDJ_SOC_CFG_H */
