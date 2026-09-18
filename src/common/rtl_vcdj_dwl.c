/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *                        Header Files
 *============================================================================*/
/*
 * EVERYTHING THE VENDOR CONTROL SOFTWARE CALLS INTO.
 *
 * Three contracts live here, all pointing the same way (vendor calls us):
 *
 *   1. DWL (Decoder Wrapper Layer)  memory, registers, core scheduling,
 *                                   hardware identification, VCMD stubs
 *   2. dwl_os                       the 13 Mutex/Event/Semaphore functions
 *                                   `dwlthread.h` declares under NON_PTHREAD_H
 *   3. OSAL                         osal_usleep() and JpegDecTrace()
 *
 * Contracts 2 and 3 used to sit in separate files so that a Zephyr build could
 * keep `dwlthread.h` and `<zephyr/kernel.h>` in different translation units:
 * `dwlthread.h` redefines `atomic_t` and `EINVAL`, which collide with Zephyr.
 * On this platform they are merged because they never touch an OS header - the
 * OS primitives are reached through the `vcdj_os_*()` contract in
 * rtl_vcdj_internal.h, which is plain `<stdint.h>`.
 *
 * Rule that still applies: NO OS header may be included here, directly or
 * indirectly. Platform services come exclusively from rtl_vcdj_internal.h.
 */
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "rtl_vcdj_internal.h"

#include "basetype.h"
#include "dwl.h"
#include "dwlthread.h"
#include "sw_util.h"
#include "vpufeature.h"

/*============================================================================*
 *                        Local Definitions
 *============================================================================*/
/** ASIC ID register: swreg 0. */
#define VCDJ_REG_ASIC_ID_OFFSET         0x00U

/**
 * Hardware build ID register: swreg 309.
 *
 * Same index the vendor kernel driver reads
 * (`software/linux/dwl/dwl_linux_hw.c`: `dev->hw_build_id[i] = *(reg_mem_vir + 309)`).
 */
#define VCDJ_REG_HW_BUILD_ID_OFFSET     (309U * 4U)

/** Maximum number of cores this first version tracks. */
#define VCDJ_MAX_CORES                  1

/*
 * pthread_cond_wait() blocks forever. With NON_PTHREAD_H the vendor code also
 * turns pthread_create() into a no-op, so nothing else is running to signal the
 * condition variable, and a genuine forever-wait would simply hang the caller.
 *
 * Every vendor call site re-checks its predicate in a `while` loop, so returning
 * from the wait early is safe: it degrades into a slow poll instead of a deadlock.
 * That is strictly preferable to hanging, and it keeps the "no unbounded wait
 * anywhere in this driver" property.
 */
#ifndef VCDJ_COND_WAIT_TIMEOUT_MS
#define VCDJ_COND_WAIT_TIMEOUT_MS   1000U
#endif

/** Vendor convention (pthread): 0 means success, non-zero means failure. */
#define VCDJ_OS_OK      0
#define VCDJ_OS_FAIL    (-1)

/*============================================================================*
 *                        Local Types
 *============================================================================*/
struct vcdj_dwl_inst
{
    u32 in_use;
    enum DWLClientType client_type;
    void *priv;
    u32 n_cores;
    i32 reserved_core;
    DWLIRQCallbackFn *irq_callback[VCDJ_MAX_CORES];
    void *irq_arg[VCDJ_MAX_CORES];
};

/*============================================================================*
 *                        Local Variables
 *============================================================================*/
/*
 * One static instance. This SoC has a single decoder core and limited RAM
 * headroom, so a dynamically grown instance table would buy nothing.
 */
static struct vcdj_dwl_inst vcdj_dwl_instance;

/*============================================================================*
 *                        Local Functions
 *============================================================================*/
static struct vcdj_dwl_inst *vcdj_dwl_of(const void *instance)
{
    struct vcdj_dwl_inst *inst = (struct vcdj_dwl_inst *)instance;

    if ((inst == NULL) || (inst->in_use == 0U))
    {
        return NULL;
    }

    return inst;
}

/*============================================================================*
 *                        DWL: instance life cycle
 *============================================================================*/
const void *DWLInit(struct DWLInitParam *param)
{
    struct vcdj_dwl_inst *inst = &vcdj_dwl_instance;

    if (param == NULL)
    {
        return NULL;
    }

    if (inst->in_use != 0U)
    {
        /* Single instance only in this version; the multi-instance decision is
         * still open. */
        return NULL;
    }

    if (vcdj_core_init() != 0)
    {
        return NULL;
    }

    memset(inst, 0, sizeof(*inst));
    inst->in_use = 1U;
    inst->client_type = param->client_type;
    inst->priv = param->priv;
    inst->n_cores = VCDJ_MAX_CORES;
    inst->reserved_core = -1;

    return (const void *)inst;
}

enum DWLRet DWLRelease(const void *instance)
{
    struct vcdj_dwl_inst *inst = vcdj_dwl_of(instance);

    if (inst == NULL)
    {
        return DWL_ERROR;
    }

    if (inst->reserved_core >= 0)
    {
        vcdj_core_release(inst->reserved_core);
        inst->reserved_core = -1;
    }

    memset(inst, 0, sizeof(*inst));
    vcdj_core_deinit();

    return DWL_OK;
}

/*============================================================================*
 *                        DWL: plain heap helpers
 *============================================================================*/
void *DWLmalloc(size_t n)
{
    return vcdj_mem_alloc(n);
}

void *DWLcalloc(size_t n, size_t s)
{
    size_t total = n * s;
    void *p;

    if ((n != 0U) && ((total / n) != s))
    {
        return NULL;    /* multiplication overflow */
    }

    p = vcdj_mem_alloc(total);
    if (p != NULL)
    {
        memset(p, 0, total);
    }

    return p;
}

void DWLfree(void *p)
{
    vcdj_mem_free(p);
}

void *DWLmemcpy(void *d, const void *s, size_t n)
{
    return memcpy(d, s, n);
}

void *DWLmemset(void *d, i32 c, size_t n)
{
    return memset(d, (int)c, n);
}

/*============================================================================*
 *                        DWL: linear (DMA capable) memory
 *============================================================================*/
enum DWLRet DWLMallocLinear(const void *instance, u64 size, struct DWLLinearMem *info)
{
    void *cpu_addr = NULL;
    uintptr_t bus_addr = 0U;
    size_t alloc_size;

    (void)instance;

    if ((info == NULL) || (size == 0U))
    {
        return DWL_ERROR;
    }

    /* size is u64 in the vendor API but this SoC is a 32-bit target. */
    if (size > (u64)(size_t) - 1)
    {
        return DWL_ERROR;
    }

    alloc_size = (size_t)size;

    if (vcdj_core_dma_alloc(alloc_size, VCDJ_DMA_ALIGN, &cpu_addr, &bus_addr) != 0)
    {
        return DWL_ERROR;
    }

    info->virtual_address = (u32 *)cpu_addr;
    info->bus_address = (addr_t)bus_addr;
    info->alloc_virtual_addr = (u32 *)cpu_addr;
    info->alloc_bus_addr = (addr_t)bus_addr;
    info->size = size;
    info->logical_size = size;
    info->id = 0U;
    info->priv = NULL;

    return DWL_OK;
}

void DWLFreeLinear(const void *instance, struct DWLLinearMem *info)
{
    (void)instance;

    if ((info == NULL) || (info->virtual_address == NULL))
    {
        return;
    }

    vcdj_core_dma_free((void *)info->virtual_address,
                       (uintptr_t)info->bus_address,
                       (size_t)info->size);

    info->virtual_address = NULL;
    info->bus_address = 0U;
    info->alloc_virtual_addr = NULL;
    info->alloc_bus_addr = 0U;
    info->size = 0U;
    info->logical_size = 0U;
}

enum DWLRet DWLMallocRefFrm(const void *instance, u64 size, struct DWLLinearMem *info)
{
    return DWLMallocLinear(instance, size, info);
}

void DWLFreeRefFrm(const void *instance, struct DWLLinearMem *info)
{
    DWLFreeLinear(instance, info);
}

void *DWLLinearMemcpy(const void *instance, addr_t device_bus_addr, const void *s, size_t n)
{
    void *dst;

    (void)instance;

    /*
     * TODO (hardware-contract): a real bus-to-CPU translation is needed once the address
     * map is known. Until then the platform layer hands out an identity mapping, so
     * the bus address can be dereferenced directly on this 32-bit SoC.
     */
    dst = (void *)(uintptr_t)device_bus_addr;

    if ((dst == NULL) || (s == NULL) || (n == 0U))
    {
        return dst;
    }

    (void)memcpy(dst, s, n);
    vcdj_core_cache_clean(dst, n);

    return dst;
}

void *DWLLinearMemset(const void *instance, struct DWLLinearMem *mem, i32 offset, i32 c, size_t n)
{
    u8 *dst;

    (void)instance;

    if ((mem == NULL) || (mem->virtual_address == NULL) || (n == 0U))
    {
        return NULL;
    }

    if (((u64)offset + (u64)n) > mem->size)
    {
        return NULL;
    }

    dst = (u8 *)mem->virtual_address + offset;
    (void)memset(dst, (int)c, n);
    vcdj_core_cache_clean(dst, n);

    return dst;
}

/*============================================================================*
 *                        DWL: register access
 *============================================================================*/
void DWLWriteReg(const void *instance, i32 core_id, u32 offset, u32 value)
{
    if (vcdj_dwl_of(instance) == NULL)
    {
        return;
    }

    vcdj_core_write_reg(core_id, offset, value);
}

u32 DWLReadReg(const void *instance, i32 core_id, u32 offset)
{
    if (vcdj_dwl_of(instance) == NULL)
    {
        return 0U;
    }

    return vcdj_core_read_reg(core_id, offset);
}

void DWLWriteRegs(const void *instance, i32 core_id, u32 offset, u32 *from,
                  u32 num_regs, u32 print_flag)
{
    u32 i;

    (void)print_flag;

    if ((vcdj_dwl_of(instance) == NULL) || (from == NULL))
    {
        return;
    }

    for (i = 0U; i < num_regs; i++)
    {
        vcdj_core_write_reg(core_id, offset + (i * 4U), from[i]);
    }
}

void DWLReadRegs(const void *instance, i32 core_id, u32 offset, u32 *dest, u32 num)
{
    u32 i;

    if ((vcdj_dwl_of(instance) == NULL) || (dest == NULL))
    {
        return;
    }

    for (i = 0U; i < num; i++)
    {
        dest[i] = vcdj_core_read_reg(core_id, offset + (i * 4U));
    }
}

void DWLWriteRegToHw(const void *instance, i32 core_id, u32 offset, u32 value)
{
    DWLWriteReg(instance, core_id, offset, value);
}

u32 DWLReadRegFromHw(const void *instance, i32 core_id, u32 offset)
{
    return DWLReadReg(instance, core_id, offset);
}

/*============================================================================*
 *                        DWL: core scheduling and completion
 *============================================================================*/
enum DWLRet DWLReserveHw(const void *instance, struct DWLReqInfo *info, i32 *core_id)
{
    struct vcdj_dwl_inst *inst = vcdj_dwl_of(instance);
    int reserved = -1;

    (void)info;

    if ((inst == NULL) || (core_id == NULL))
    {
        return DWL_ERROR;
    }

    if (vcdj_core_reserve(&reserved, VCDJ_TIMEOUT_FOREVER) != 0)
    {
        return DWL_ERROR;
    }

    inst->reserved_core = reserved;
    *core_id = (i32)reserved;

    return DWL_OK;
}

void DWLEnableHw(const void *instance, i32 core_id, u32 offset, u32 value)
{
    if (vcdj_dwl_of(instance) == NULL)
    {
        return;
    }

    vcdj_core_enable(core_id, offset, value);
}

void DWLDisableHw(const void *instance, i32 core_id, u32 offset, u32 value)
{
    if (vcdj_dwl_of(instance) == NULL)
    {
        return;
    }

    vcdj_core_disable(core_id, offset, value);
}

enum DWLRet DWLWaitHwReady(const void *instance, i32 core_id, u32 timeout)
{
    int ret;

    if (vcdj_dwl_of(instance) == NULL)
    {
        return DWL_HW_WAIT_ERROR;
    }

    /*
     * The vendor call sites pass (u32)DEC_X170_TIMEOUT_LENGTH, whose default is
     * -1 == 0xFFFFFFFF, i.e. "wait forever". vcdj_core_wait_done() clamps that
     * to the platform bound on purpose: an unbounded poll like the one in the
     * existing Realtek JPU driver would hang the caller when the core is absent
     * or wedged.
     */
    ret = vcdj_core_wait_done(core_id, timeout);

    switch (ret)
    {
    case VCDJ_CORE_WAIT_OK:
        return DWL_HW_WAIT_OK;
    case VCDJ_CORE_WAIT_TIMEOUT:
        return DWL_HW_WAIT_TIMEOUT;
    default:
        return DWL_HW_WAIT_ERROR;
    }
}

void DWLWaitOwnerDone(const void *instance, const void *owner)
{
    /* Decoding is fully synchronous in this version, so by the time the caller
     * gets here every job of `owner` has already completed. */
    (void)instance;
    (void)owner;
}

enum DWLRet DWLReleaseHw(const void *instance, i32 core_id)
{
    struct vcdj_dwl_inst *inst = vcdj_dwl_of(instance);

    if (inst == NULL)
    {
        return DWL_ERROR;
    }

    vcdj_core_release(core_id);
    inst->reserved_core = -1;

    return DWL_OK;
}

void DWLSetIRQCallback(const void *instance, i32 core_id,
                       DWLIRQCallbackFn *callback_fn, void *arg)
{
    struct vcdj_dwl_inst *inst = vcdj_dwl_of(instance);

    if ((inst == NULL) || (core_id < 0) || (core_id >= VCDJ_MAX_CORES))
    {
        return;
    }

    inst->irq_callback[core_id] = callback_fn;
    inst->irq_arg[core_id] = arg;
}

/*============================================================================*
 *                        DWL: hardware identification
 *============================================================================*/
u32 DWLReadAsicID(const void *instance, enum DWLClientType client_type)
{
    (void)client_type;

    if (vcdj_dwl_of(instance) == NULL)
    {
        return 0U;
    }

    return vcdj_core_read_reg(0, VCDJ_REG_ASIC_ID_OFFSET);
}

u32 DWLReadCoreHwBuildID(const void *instance, u32 core_id)
{
    u32 id;

    if (vcdj_dwl_of(instance) == NULL)
    {
        return 0U;
    }

    id = vcdj_core_read_reg((i32)core_id, VCDJ_REG_HW_BUILD_ID_OFFSET);

    if ((id == 0U) || (id == 0xFFFFFFFFU))
    {
        /*
         * No core is mapped: VCDJ_SOC_REG_BASE (see
         * src/device/<soc>/rtl_vcdj_soc_cfg.h) is still a placeholder because the
         * hardware contract has not been delivered. Report the build ID this
         * Control Software drop was released for so the software layers can still
         * be exercised.
         *
         * This is a declared build-time assumption, not a hardware reading. It
         * must be removed as soon as the real register base is known, and the
         * M2 smoke test must then assert that the register really returns
         * VCDJ_EXPECTED_HW_BUILD_ID.
         */
        id = VCDJ_EXPECTED_HW_BUILD_ID;
    }

    return id;
}

const void *DWLGetHwFeaturesByID(const void *instance, u32 core_id)
{
    const struct DecHwFeatures *hw_feature = NULL;

    GetReleaseHwFeaturesByID(DWLReadCoreHwBuildID(instance, core_id), &hw_feature);

    return (const void *)hw_feature;
}

const void *DWLGetHwFeaturesByClientType(const void *instance,
                                         enum DWLClientType client_type,
                                         u32 *core_mask)
{
    (void)client_type;

    if (core_mask != NULL)
    {
        *core_mask = 1U;    /* single core */
    }

    return DWLGetHwFeaturesByID(instance, 0U);
}

u32 DWLReadAsicCoreCount(const void *instance)
{
    struct vcdj_dwl_inst *inst = vcdj_dwl_of(instance);

    if (inst == NULL)
    {
        return 0U;
    }

    return inst->n_cores;
}

/*============================================================================*
 *                        Checksum buffer queue
 *============================================================================*/
/*
 * SwGetCrcMem() / SwGetCrcLast() / SwSetCrcUnused() / SwClearCrcFlag() /
 * SwFreeCrcMem() are DECLARED in software/source/common/sw_util.h, which is part
 * of the compiled subset, but IMPLEMENTED in the DWL layer
 * (software/linux/dwl/dwl_pc.c, dwl_linux_hw.c). They are therefore part of the
 * porting surface even though their names do not start with DWL, and
 * jpegdecapi.c / pnghwd_asic.c reference them unconditionally.
 *
 * Behaviour mirrors the vendor reference implementation: a singly linked list of
 * CrcData nodes with `used` and `last` flags, grown on demand.
 */
static void vcdj_crc_set_used(CrcData *crc)
{
    if (crc != NULL)
    {
        crc->used = 1U;
    }
}

static void vcdj_crc_set_last(CrcData **crc, CrcData **cur)
{
    CrcData *tmp = *crc;

    while (tmp != NULL)
    {
        tmp->last = 0U;
        tmp = tmp->next;
    }

    (*cur)->last = 1U;
}

void SwSetCrcUnused(CrcData *crc)
{
    if (crc != NULL)
    {
        crc->used = 0U;
    }
}

void SwGetCrcMem(CrcData **crc, CrcData **cur)
{
    if ((crc == NULL) || (cur == NULL))
    {
        return;
    }

    *cur = *crc;

    if (*cur == NULL)
    {
        *cur = *crc = (CrcData *)DWLcalloc(sizeof(CrcData), 1U);
    }
    else
    {
        while (*cur != NULL)
        {
            if ((*cur)->used == 0U)
            {
                break;
            }
            *cur = (*cur)->next;
        }

        if (*cur == NULL)
        {
            *cur = (CrcData *)DWLcalloc(sizeof(CrcData), 1U);
            if ((*cur != NULL) && ((*crc)->next != NULL))
            {
                (*cur)->next = (*crc)->next;
            }
            (*crc)->next = *cur;
        }
    }

    if (*cur != NULL)
    {
        vcdj_crc_set_used(*cur);
        vcdj_crc_set_last(crc, cur);
    }
}

void SwGetCrcLast(CrcData **crc, CrcData **cur)
{
    CrcData *tmp;

    if ((crc == NULL) || (cur == NULL))
    {
        return;
    }

    tmp = *crc;

    while (tmp != NULL)
    {
        if (tmp->last != 0U)
        {
            *cur = tmp;
            break;
        }
        tmp = tmp->next;
    }
}

void SwClearCrcFlag(CrcData **crc)
{
    CrcData *tmp;

    if (crc == NULL)
    {
        return;
    }

    tmp = *crc;

    while (tmp != NULL)
    {
        tmp->used = 0U;
        tmp->last = 0U;
        tmp = tmp->next;
    }
}

void SwFreeCrcMem(CrcData **crc)
{
    CrcData *tmp;

    if (crc == NULL)
    {
        return;
    }

    tmp = *crc;

    while (tmp != NULL)
    {
        *crc = tmp->next;
        DWLfree(tmp);
        tmp = *crc;
    }
}

/*============================================================================*
 *                        DWL: VCMD - not supported in this version
 *============================================================================*/
/*
 * The command buffer (VCMD) path is deliberately left unimplemented: the first
 * version targets a single core in direct register mode. Every entry point below
 * reports "not available" so the vendor code takes its non-VCMD branch, which is
 * what DWLVcmdIsUsed() == 0 already tells it to do.
 */
u32 DWLVcmdIsUsed(const void *instance)
{
    (void)instance;
    return 0U;
}

u32 DWLReadVcmdHwID(const void *instance)
{
    (void)instance;
    return 0U;
}

u32 DWLCheckVcmdM2M(const void *instance)
{
    (void)instance;
    return 0U;
}

enum DWLRet DWLReserveCmdBuf(const void *instance, struct DWLReqInfo *info, u32 *cmd_buf_id)
{
    (void)instance;
    (void)info;
    (void)cmd_buf_id;
    return DWL_ERROR;
}

enum DWLRet DWLFlushRegister(const void *instance, u32 cmd_buf_id, u32 *dec_regs,
                             u32 *mc_fresh_regs, u32 mc_buf_id)
{
    (void)instance;
    (void)cmd_buf_id;
    (void)dec_regs;
    (void)mc_fresh_regs;
    (void)mc_buf_id;
    return DWL_ERROR;
}

enum DWLRet DWLPrepareCmds(const void *instance, u32 cmd_buf_id)
{
    (void)instance;
    (void)cmd_buf_id;
    return DWL_ERROR;
}

enum DWLRet DWLEnableCmdBuf(const void *instance, u32 cmd_buf_id)
{
    (void)instance;
    (void)cmd_buf_id;
    return DWL_ERROR;
}

enum DWLRet DWLWaitCmdBufReady(const void *instance, u16 cmd_buf_id)
{
    (void)instance;
    (void)cmd_buf_id;
    return DWL_ERROR;
}

enum DWLRet DWLRefreshRegister(const void *instance, u32 cmd_buf_id, u32 *dec_regs)
{
    (void)instance;
    (void)cmd_buf_id;
    (void)dec_regs;
    return DWL_ERROR;
}

enum DWLRet DWLReleaseCmdBuf(const void *instance, u32 cmd_buf_id)
{
    (void)instance;
    (void)cmd_buf_id;
    return DWL_ERROR;
}

enum DWLRet DWLWaitCmdbufsDone(const void *instance, const void *owner)
{
    (void)instance;
    (void)owner;
    return DWL_OK;
}

u32 DWLGetCmdBufExeStatus(const void *instance, u16 cmd_buf_id)
{
    (void)instance;
    (void)cmd_buf_id;
    return 0U;
}

enum DWLRet DWLParseCmdBufExeStatus(const void *instance, u32 exe_status)
{
    (void)instance;
    (void)exe_status;
    return DWL_ERROR;
}

void DWLCmdM2MSendData(const void *instance, struct CDF_INFO *head_data,
                       struct CDF_INFO *tail_data, u32 cmdbuf_id)
{
    (void)instance;
    (void)head_data;
    (void)tail_data;
    (void)cmdbuf_id;
}

void DWLCmdMsetSendData(const void *instance, struct MSET_INFO *before_data,
                        struct MSET_INFO *after_data, u32 cmdbuf_id)
{
    (void)instance;
    (void)before_data;
    (void)after_data;
    (void)cmdbuf_id;
}

void DWLCmdM2MPSendData(const void *instance, struct M2MP_INFO *before_data,
                        struct M2MP_INFO *after_data, u32 cmdbuf_id)
{
    (void)instance;
    (void)before_data;
    (void)after_data;
    (void)cmdbuf_id;
}

enum DWLRet DWLRefreshCrcResult(const void *instance, CrcData **crc, u32 cmdbuf_id)
{
    (void)instance;
    (void)cmdbuf_id;

    if (crc != NULL)
    {
        *crc = NULL;
    }

    return DWL_ERROR;
}

enum DWLRet DWLMCRefreshCrcResult(const void *instance, CrcData **crc, u32 cmdbuf_id)
{
    return DWLRefreshCrcResult(instance, crc, cmdbuf_id);
}

enum DWLRet DWLVcmdMCRefreshStatusRegs(const void *instance, u32 *dec_regs, u32 cmdbuf_id)
{
    (void)instance;
    (void)dec_regs;
    (void)cmdbuf_id;
    return DWL_ERROR;
}

enum DWLRet DWLCmdPushSliceRegs(const void *instance, u32 cmd_buf_id, u32 *regs)
{
    (void)instance;
    (void)cmd_buf_id;
    (void)regs;
    return DWL_ERROR;
}

void DWLAbortCmdbuf(const void *instance, u16 cmd_buf_id)
{
    (void)instance;
    (void)cmd_buf_id;
}

void DWLDropCmdbufs(const void *instance, const void *owner)
{
    (void)instance;
    (void)owner;
}

void DWLSetCmdBufIntCtrl(const void *instance, u16 cmd_buf_id, u64 interrupt_ctrl)
{
    (void)instance;
    (void)cmd_buf_id;
    (void)interrupt_ctrl;
}

i32 DWLGetVcmdMCVirtualCoreId(const void *instance, u32 cmdbuf_id)
{
    (void)instance;
    (void)cmdbuf_id;
    return -1;
}

/*============================================================================*
 *                        dwl_os: Mutex
 *============================================================================*/
/*
 * The 13 `dwl_os` functions that `dwlthread.h` declares when NON_PTHREAD_H is
 * defined. That header redirects all 139 `pthread_*` and 41 `sem_*` uses in the
 * Control Software onto these functions, so no vendor source file has to be
 * touched.
 *
 * The actual OS objects live in src/device/<soc>/rtl_vcdj_port.c and are reached
 * through the plain-<stdint.h> `vcdj_os_*()` contract.
 */
int MutexInit(PMutex m, PAttribute a)
{
    vcdj_os_handle_t handle;

    (void)a;

    if (m == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    if (vcdj_os_mutex_create(&handle) != 0)
    {
        m->locked = (dwl_bool)VCDJ_OS_HANDLE_INVALID;
        return VCDJ_OS_FAIL;
    }

    /* `locked` is the only storage the vendor Mutex offers; it carries the
     * bridge handle, not a boolean. Nothing in the compiled vendor subset reads
     * the field directly. */
    m->locked = (dwl_bool)handle;
    m->attrib.a = 0;

    return VCDJ_OS_OK;
}

int MutexDestroy(PMutex m)
{
    if (m == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    if (vcdj_os_mutex_destroy((vcdj_os_handle_t)m->locked) != 0)
    {
        return VCDJ_OS_FAIL;
    }

    m->locked = (dwl_bool)VCDJ_OS_HANDLE_INVALID;

    return VCDJ_OS_OK;
}

int MutexLock(PMutex m)
{
    if (m == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    return (vcdj_os_mutex_lock((vcdj_os_handle_t)m->locked, VCDJ_OS_WAIT_FOREVER) == 0)
           ? VCDJ_OS_OK : VCDJ_OS_FAIL;
}

int MutexUnlock(PMutex m)
{
    if (m == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    return (vcdj_os_mutex_unlock((vcdj_os_handle_t)m->locked) == 0)
           ? VCDJ_OS_OK : VCDJ_OS_FAIL;
}

/*============================================================================*
 *                        dwl_os: Event (condition variable)
 *============================================================================*/
int EventInit(PEvent e, PAttribute a)
{
    vcdj_os_handle_t handle;

    (void)a;

    if (e == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    if (vcdj_os_cond_create(&handle) != 0)
    {
        e->singaled = (dwl_bool)VCDJ_OS_HANDLE_INVALID;
        return VCDJ_OS_FAIL;
    }

    e->singaled = (dwl_bool)handle;
    e->attrib.a = 0;

    return VCDJ_OS_OK;
}

int EventDestroy(PEvent e)
{
    if (e == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    if (vcdj_os_cond_destroy((vcdj_os_handle_t)e->singaled) != 0)
    {
        return VCDJ_OS_FAIL;
    }

    e->singaled = (dwl_bool)VCDJ_OS_HANDLE_INVALID;

    return VCDJ_OS_OK;
}

int EventWait(PEvent e, PMutex m)
{
    if ((e == NULL) || (m == NULL))
    {
        return VCDJ_OS_FAIL;
    }

    return (vcdj_os_cond_wait((vcdj_os_handle_t)e->singaled,
                              (vcdj_os_handle_t)m->locked,
                              VCDJ_COND_WAIT_TIMEOUT_MS) == 0)
           ? VCDJ_OS_OK : VCDJ_OS_FAIL;
}

int EventSignal(PEvent e)
{
    if (e == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    return (vcdj_os_cond_signal((vcdj_os_handle_t)e->singaled) == 0)
           ? VCDJ_OS_OK : VCDJ_OS_FAIL;
}

/*============================================================================*
 *                        dwl_os: Semaphore
 *============================================================================*/
int SemaphoreInit(PSemaphore s, u32 value)
{
    vcdj_os_handle_t handle;

    if (s == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    if (vcdj_os_sem_create(&handle, value) != 0)
    {
        s->val = (i32)VCDJ_OS_HANDLE_INVALID;
        return VCDJ_OS_FAIL;
    }

    s->val = (i32)handle;

    return VCDJ_OS_OK;
}

int SemaphoreDestroy(PSemaphore s)
{
    if (s == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    if (vcdj_os_sem_destroy((vcdj_os_handle_t)s->val) != 0)
    {
        return VCDJ_OS_FAIL;
    }

    s->val = (i32)VCDJ_OS_HANDLE_INVALID;

    return VCDJ_OS_OK;
}

int SemaphoreWait(PSemaphore s)
{
    if (s == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    /* Bounded for the same reason as EventWait(): the vendor call sites re-check
     * their predicate, so an expiry becomes a retry rather than a hang. */
    return (vcdj_os_sem_wait((vcdj_os_handle_t)s->val, VCDJ_COND_WAIT_TIMEOUT_MS) == 0)
           ? VCDJ_OS_OK : VCDJ_OS_FAIL;
}

int SemaphorePost(PSemaphore s)
{
    if (s == NULL)
    {
        return VCDJ_OS_FAIL;
    }

    return (vcdj_os_sem_post((vcdj_os_handle_t)s->val) == 0)
           ? VCDJ_OS_OK : VCDJ_OS_FAIL;
}

int SemaphoreGetValue(PSemaphore s)
{
    int count;

    if (s == NULL)
    {
        return 0;
    }

    count = vcdj_os_sem_count((vcdj_os_handle_t)s->val);

    return (count < 0) ? 0 : count;
}

/*============================================================================*
 *                        OSAL shims
 *============================================================================*/
int osal_usleep(unsigned long usec)
{
#if ULONG_MAX > UINT32_MAX
    if (usec > UINT32_MAX)
    {
        usec = UINT32_MAX;
    }
#endif

    vcdj_os_delay_us((uint32_t)usec);

    return 0;
}

/**
 * The vendor JPEG API declares this and states it "must be implemented by the
 * application using the decoder API". It is only reached when the vendor trace
 * build options are enabled, which they are not here, so it stays empty and is
 * dropped by --gc-sections. It exists so the contract is satisfied.
 */
void JpegDecTrace(const char *string)
{
    (void)string;
}
