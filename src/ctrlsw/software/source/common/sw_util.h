/*------------------------------------------------------------------------------
--       Copyright (c) 2015, VeriSilicon Inc. All rights reserved             --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
-- * Redistributions of source code must retain the above copyright notice,   --
--       this list of conditions and the following disclaimer.                --
-- * Redistributions in binary form must reproduce the above copyright        --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
-- * Neither the names of the copyright holder nor the names of its           --
--       contributors may be used to endorse or promote products derived from --
--       this software without specific prior written permission.             --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

#ifndef SW_UTIL_H_
#define SW_UTIL_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "basetype.h"
#include "dwl.h"
#include "sw_debug.h"
#include "dec_log.h"
#include "ppu.h"
#include "dectypes.h"
#include "low_latency.h"

#define HANTRO_OK 0
#define HANTRO_NOK 1

#ifndef HANTRO_FALSE
#define HANTRO_FALSE (0U)
#endif

#ifndef HANTRO_TRUE
#define HANTRO_TRUE (1U)
#endif

#ifndef UNUSED
#  define UNUSED(x) (void)(x)
#endif

#define MEMORY_ALLOCATION_ERROR  0xFFFF
#define MEMORY_ALLOCATION_ERROR2 0xFFFFFFFF
#define PARAM_SET_ERROR 0xFFF0

#define SYNC_WORD_SIZE_TILE   (64)
#define SYNC_WORD_SIZE_FRAME  (32)
#define SYNC_WORD_SIZE_FIELD  (16)

/* value to be returned by GetBits if stream buffer is empty */
#define END_OF_STREAM 0xFFFFFFFEU

#define OUT_IDR (3U)
#define EXT_DEC_RECON_NUM       1
#define EXT_OUT_NUM             1
#define EXT_DPB_NUM             2
#define EXT_DV_MC_DPB_NUM       10

/* macro to get smaller of two values */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/* macro to get greater of two values */
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/* macro to get absolute value */
#define ABS(a) (((a) < 0) ? -(a) : (a))

/* macro to clip a value z, so that x <= z =< y */
#define CLIP3(x, y, z) (((z) < (x)) ? (x) : (((z) > (y)) ? (y) : (z)))

/* macro to clip a value z, so that 0 <= z =< 255 */
#define CLIP1(z) (((z) < 0) ? 0 : (((z) > 255) ? 255 : (z)))

/* macro to allocate memory */
#define ALLOCATE(ptr, count, type)           \
  {                                          \
    if (ptr == NULL)                         \
      ptr = DWLmalloc((count) * sizeof(type)); \
  }

/* macro to free allocated memory */
#define FREE(ptr)      \
  {                    \
    if (ptr != NULL) { \
      DWLfree(ptr);    \
      ptr = NULL;      \
    }                  \
  }

/* macro to report syntax error */
#define SW_CHECK_SYNTAX_ELEMENT(rv, elem)                    \
  if (rv != HANTRO_OK) {                                     \
    fprintf(stderr, "error in syntax element '%s'\n", elem); \
    return rv;                                               \
  }

#define SW_CHECK_END_OF_STREAM(rv, elem)                             \
  if (rv == END_OF_STREAM) {                                         \
    fprintf(stderr, "end of stream in syntax element '%s'\n", elem); \
    return rv;                                                       \
  }

/* round to next multiple of n */
#define NEXT_MULTIPLE(value, n) (((value) + (n) - 1) & ~((n) - 1))
#define NEXT_MULTIPLE_1(value, n) (((value) + (n) - 1) / (n) * (n))
#define ALIGN(a) (1 << (a))

#define ROUNDUPX(val, mul) (((val) + ((mul)-1)) & (~((mul)-1)))

#if INTPTR_MAX == INT64_MAX
#ifdef VDK_STRICT_TEST
#define REG_WRITE_ADDR(reg, addr) \
  do {                            \
    if ((addr) != 0) { \
      reg##_msb = (u32)((u64)(addr) >> 32) ^ ENCRYP_CODE;   \
    } else { \
      reg##_msb = 0;   \
    } \
    reg = addr;                   \
  } while (0)
#else
#define REG_WRITE_ADDR(reg, addr) \
  do {                            \
    reg##_msb = (u32)((u64)(addr) >> 32);     \
    reg = addr;                   \
  } while (0)
#endif
#else
#define REG_WRITE_ADDR(reg, addr) \
  do {                            \
    reg = addr;                   \
  } while (0)
#endif

#define SET_REG_VALUE(regs, id, start_bits, bit_width, value) do {\
  u32 *p = (u32 *)(regs); \
  u32 tmp = p[id]; \
  tmp &= ~(((1 << (bit_width)) - 1) << (start_bits)); \
  tmp |= (value) << (start_bits); \
  p[id] = tmp; \
} while(0)

/* RFC table overflow */
/* All the CBS size except the last group may exceeds 1<<16 bytes, which may result in
   offset of the last CBS group overflows the 16-bit range. */
#ifdef RFC_WORD_ALIGN
#define RFC_MAY_OVERFLOW(w,bd) (1)
#else
#define RFC_MAY_OVERFLOW(w,bd) (NEXT_MULTIPLE((w)-128, 128)*bd >= (1<<16))
#endif

#define MAJOR_VERSION(hw_id) (((hw_id) & 0xF000) >> 12)
#define MINOR_VERSION(hw_id) (((hw_id) & 0x0FF0) >> 4)

/* legacy release based on product number */
#define IS_G1_LEGACY(hw_id) (((hw_id) >> 16) == 0x6731)
#define IS_G2_LEGACY(hw_id) (((hw_id) >> 16) == 0x6732)
#define IS_LEGACY(hw_id) ((((hw_id) >> 16) == 0x6731) || (((hw_id) >> 16) == 0x6732))

struct SwParames {
  const void *dwl;
  const PpUnitIntConfig *ppu_cfg;
  u32 use_separated_pp;
  u32 init_value;
};

struct PreResInfo {
  u32 pic_max_width;
  u32 pic_max_height;
  u32 bit_depth;
  u32 min_buf_num;
};

/* Removed from dwl.h to here.*/
typedef addr_t DWLMemAddr;
#define DWL_DEVMEM_INIT                    (DWLMemAddr)(uintptr_t)NULL
#define DWL_DEVMEM_VAILD(mem)              ((mem).bus_address != DWL_DEVMEM_INIT)
#define DWL_DEVMEM_COMPARE(mem, addr)      ((mem).bus_address == addr)
#define DWL_DEVMEM_IN_RANGE(mem, addr)     ((mem).bus_address <= addr && (mem).bus_address + (mem).size > addr)
#define DWL_DEVMEM_COMPARE_V2(mem1, mem2)  ((mem1).bus_address == (mem2).bus_address)
#define DWL_GET_DEVMEM_ADDR(mem)           ((mem).bus_address)
#define DWL_DEVMEM_IS_EQUAL(mem1, mem2)    ((mem1) == (mem2))

/** \brief convert the dec_ctrl of DecInputParameters to pp out ctrl */
#define PP_OUT_CTRL(a) ((a) & 0x3F)
#define PP_CHANNEL_OUT_ENABLE(a, channel_id) !(((a) >> channel_id) & 0x1)

#define CORE_MASK(a)                              ((a) & 0xFFFF)
#define NON_CORE_MASK(a)                          ((((a) >> 16 ) & 0xFFFF) << 16)
#define SET_CLIENT_TYPE(core_mask, client_type)   ((core_mask) |= ((client_type) << 16))
#define SET_SECURE_MODE(core_mask, secure_mode)   ((core_mask) |= ((secure_mode) << 31))


#define S_MISC_CTRL(a,b,bit_mask)    do { \
                                       u32 tmp = a; \
                                       a = (b << bit_mask); \
                                       a |= (tmp & (~0x1)); \
                                     } while(0)

/* bit control for misc_ctrl of struct DecInitConfig */
#define IDR_OUT_IMD_BIT_MASK  (16)
#define ADD_BUFF_DYN_BIT_MASK (17)
#define PIC_DISCARD_BIT_MASK  (18)
#define G_IDR_OUT_IMD(a)      (((a) >> IDR_OUT_IMD_BIT_MASK) & 0x1)
#define G_ADD_BUF_DYN(a)      (((a) >> ADD_BUFF_DYN_BIT_MASK) & 0x1)
#define G_PIC_DISCARD(a)      (((a) >> PIC_DISCARD_BIT_MASK) & 0x1)

/* bit contrl for misc_ctrl of struct DecConfig */
#define SEPERATED_PP_MASK     (16)
#define G_SEPERATED_PP(a)     (((a) >> SEPERATED_PP_MASK) & 0x1)

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

struct VcmdDataMvInfo {
  struct CDF_INFO *before_dec;/* data mv info before decoding  */
  struct CDF_INFO *after_dec;/* data mv info after decoding */
};

typedef struct VcmdMsetInfo {
  struct MSET_INFO *before_dec;
  struct MSET_INFO *after_dec;
} VcmdMsetInfo;

typedef struct VcmdM2MPInfo {
  struct M2MP_INFO before_dec[MAX_VCMD_M2MP_NUM];
  struct M2MP_INFO after_dec[MAX_VCMD_M2MP_NUM];
} VcmdM2MPInfo;

typedef struct CommonCfg {
  u8 idr_out_imd; /* 264/265 */
  u8 add_buf_dyn; /* g2 */

  u8 picture_discard;
  u8 use_separated_pp; /* all */
} CommonCfg;

/**
 * \struct struct DecDroppedPicInfo
 * \brief A structure containing output pciture dropped info
 * \ingroup common_group
 */
struct DecDroppedPicInfo {
  u64 pic_id;
};

/**
 * \brief Internal debug to dump reg and buf
 * \ingroup common_group
 */
#ifndef IN_STAGE
#define IN_STAGE 0
#else
#if IN_STAGE != 0
#error("IN_STAGE not 0")
#endif
#endif
#ifndef OUT_STAGE
#define OUT_STAGE 1
#else
#if OUT_STAGE != 1
#error("OUT_STAGE not 1")
#endif
#endif

#ifdef VDK_STRICT_TEST
#define ENCRYP_CODE (0xbeef)
#else
#define ENCRYP_CODE (0)
#endif

#define DUMP_REG(mode) (mode==1 || mode == 3)
#define DUMP_BUF(mode) (mode==2 || mode == 3)
/** \brief Max buffer num allocated by DWLCommonMallocLinear */
#define BUF_SLOTS (64)
struct Reg_Info {
  u32 reg_lsb; /** \brief reg lsb */
  u32 reg_msb; /** \brief reg msb */
  u32 buf_len; /** \brief the buffer size */
  u32 io_mode;
};
void SwSaveBuf(struct DWLLinearMem *info, struct DWLLinearMem **buffer_infos, u32 *slots);
void SwRecycleBuf(struct DWLLinearMem *info, struct DWLLinearMem *buffer_infos, u32 slots);
u32 SwGetBufByAddr(struct DWLLinearMem *buffer_infos, u32 slots, addr_t addr);
addr_t GetRegsBufAddr(u32 *regs, u32 reg_lsb_idx, u32 reg_msb_idx, int flag);
void SwLogRegInfo(const struct DecHwFeatures *hw_feature, u32 *regs, int stage, struct Reg_Info *reg_info, u32 pic_id);
void SwLogBufInfo(u8 *buf, u32 buf_len, u32 reg_lsb_idx, u32 reg_msb_idx, int stage, u32 pic_id);

#define DEC_DYNAMIC_PIC_LIMIT 10
#define CONSUMED_IN_BIT_MASK  (28)
#define S_CONSUMED_IN(a)      (a |= (1 << CONSUMED_IN_BIT_MASK))
#define C_CONSUMED_IN(a)      (a &= 0xEFFFFFFF)
#define G_CONSUMED_IN(a)      ((a >> CONSUMED_IN_BIT_MASK) & 1)

/** \brief convert the dec_ctrl of DecInputParameters to cmd priority */
#define SW_CMD_PRIORITY_BIT     7
#define SW_S_CMD_PRIORITY(a, b) (a |= (b << SW_CMD_PRIORITY_BIT))
#define SW_G_CMD_PRIORITY(a)    (((a) >> SW_CMD_PRIORITY_BIT) & 1)

static inline int FloorLog2(u32 x) {
  if (x == 0)
  {
    // note: ceilLog2() expects -1 as return value
    return -1;
  }
#ifdef __GNUC__
  return 31 - __builtin_clz(x);
#else
#ifdef _MSC_VER
  unsigned long r = 0;
  _BitScanReverse(&r, x);
  return r;
#else
  int result = 0;
  if (x & 0xffff0000)
  {
    x >>= 16;
    result += 16;
  }
  if (x & 0xff00)
  {
    x >>= 8;
    result += 8;
  }
  if (x & 0xf0)
  {
    x >>= 4;
    result += 4;
  }
  if (x & 0xc)
  {
    x >>= 2;
    result += 2;
  }
  if (x & 0x2)
  {
    x >>= 1;
    result += 1;
  }
  return result;
#endif
#endif
}

static inline int CeilLog2(u32 x) {
  return (x==0) ? -1 : FloorLog2(x - 1) + 1;
}
u32 SwGetCores(u32 core_mask);
#ifdef STACK_STAT
void StackStatInit();
void StackConsumption(const char *);
#else
#define StackStatInit()
#define StackConsumption(str)
#endif

#define UNIQUE_ID(id) (u16)((rand() & 0xFFFF) + (id))

enum FEATURE {
  VSI_HEVC,
  VSI_HEVC_MAIN10,
  VSI_HEVC_422_INTRA,
  VSI_VP9,
  VSI_VP9_PROFILE2,
  VSI_H264,
  VSI_H264_ADV,
  VSI_H264_HIGH10,
  VSI_H264_422_INTRA,
  VSI_MPEG4,
  VSI_CUSTOM_MPEG4,
  VSI_MPEG2,
  VSI_SORENSON_SPARK,
  VSI_VC1,
  VSI_JPEG,
  VSI_RV,
  VSI_VP8,
  VSI_WEBP,
  VSI_VP7,
  VSI_AVS,
  VSI_AVS_PLUS,
  VSI_AVS2,
  VSI_AVS2_MAIN10,
  VSI_AV1,
  VSI_VVC,
  VSI_AVS3,
  VSI_RFC,
  VSI_QP_DUMP,
  VSI_LOW_LATENCY,
  VSI_VP9_HW_PROB,
  VSI_PARTIAL_DECODING,
  VSI_ADDR64_SUPPORT,
};
#define LEGACY_CLEINT_TYPE(client_type) (client_type == DWL_CLIENT_TYPE_AVS_DEC)   || \
                                        (client_type == DWL_CLIENT_TYPE_JPEG_DEC)  || \
                                        (client_type == DWL_CLIENT_TYPE_MPEG2_DEC) || \
                                        (client_type == DWL_CLIENT_TYPE_MPEG4_DEC) || \
                                        (client_type == DWL_CLIENT_TYPE_RV_DEC)    || \
                                        (client_type == DWL_CLIENT_TYPE_VC1_DEC)   || \
                                        (client_type == DWL_CLIENT_TYPE_AVS_DEC)   || \
                                        (client_type == DWL_CLIENT_TYPE_VP6_DEC)   || \
                                        (client_type == DWL_CLIENT_TYPE_VP8_DEC)

#define RFC_CLIENT_TYPE(client_type)    (client_type == DWL_CLIENT_TYPE_VP9_DEC)  || \
                                        (client_type == DWL_CLIENT_TYPE_HEVC_DEC) || \
                                        (client_type == DWL_CLIENT_TYPE_H264_DEC) || \
                                        (client_type == DWL_CLIENT_TYPE_AVS2_DEC) || \
                                        (client_type == DWL_CLIENT_TYPE_AV1_DEC)  || \
                                        (client_type == DWL_CLIENT_TYPE_VVC_DEC)  || \
                                        (client_type == DWL_CLIENT_TYPE_AVS3_DEC)

u32 SwGetCores(u32 core_mask);
u32 SwGetCoreMaskByFeature(const void *dwl_inst, enum FEATURE feature);
const struct DecHwFeatures *SwGetHwFeature(const void *dwl_inst, u32 core_mask,
                                           enum DWLClientType client_type);
void SwSetHwFeatures(const void *dwl_inst, u32 core_mask, const struct DecHwFeatures *hw_features[]);
u32 SwAdjustCoreMaskByWxH(const void *dwl_inst, u32 width, u32 height, u32 align_m,
                          enum DWLClientType client_type, u32 *core_mask,
                          const struct DecHwFeatures **cur_hw_feature);

void SwCommonInfoInit(const void *dwl, u32 misc_ctrl, enum DWLClientType client_type, u32 dec_mode,
                      u32 *core_mask, u32 *secure_mode, const struct DecHwFeatures *hw_features[],
                      const struct DecHwFeatures **cur_hw_feature);
enum DecRet SwSetCommonInfo(const void *dwl, u32 misc_ctrl, enum DWLClientType client_type, u32 *core_mask,
      const struct DecHwFeatures *hw_features[], const struct DecHwFeatures **cur_hw_feature);

void ClearSwIRQ(u32 *regs);
void SwAbortSliceDec(const void *dwl, const void *dec_inst, u32 *regs,
                     u32 vcmd_used, u32 core_id);
void SwAbortDec(const void *dwl, struct DWLReqInfo *info, u32 vcmd_used,
                  u32 n_cores_available, void *cmd_info);
u32 SwCheckSliceStatus(const void *dwl, u32 vcmd_used, u32 id);
u32 SwCheckClientType(enum DWLClientType client_type);
/**
 * Get free/new mem for crc.
 * \param [in]     crc                           Pointer to a crc queue.
 * \param [in]     cur                           Pointer to use crcdata.
 * \return         void                          None
 */
void SwGetCrcMem(CrcData **crc, CrcData **cur);

/**
 * Get the last used checksum buffer.
 * \param [in]     crc                           Pointer to a crc queue.
 * \param [in]     cur                           Pointer to use crcdata.
 * \return         void                          None
 */
void SwGetCrcLast(CrcData **crc, CrcData **cur);

/**
 * Set the crc mem as unused.
 * \param [in]     crc                           Pointer to a crc struct.
 * \return         void                          None
 */
void SwSetCrcUnused(CrcData *crc);

/**
 * Clear all crc flag.
 * \param [in]     crc                           Pointer to a crc queue.
 * \return         void                          None
 */
void SwClearCrcFlag(CrcData **crc);

/**
 * Free all crc mem.
 * \param [in]     crc                           Pointer to a crc queue.
 * \return         void                          None
 */
void SwFreeCrcMem(CrcData **crc);

/**
 * Read one byte from a given pointer.
 * \ingroup common_group
 * \param [in]     p                         Pointer to the memory block to read.
 * \return         u8                        One byte data from the given address.
 */
u8 SwReadByte(const u8 *p);

/**
 * Read one byte from the memory block pointed to by p. \n
 * This function is just an interface to access private or protected buffer; it should be realized by customer.
 * \ingroup common_group
 * \param [in]     p                         Pointer to a virtual address in the private buffer.
 * \return         u8                        One byte data from the given address.
 */
u8 SwPrivateAreaReadByte(const u8 *p);

/**
 * \brief  Write one byte data to the memory block pointed to by p. \n
 * This function is just an interface to access private or protected buffer; it should be realized by customer.
 * \ingroup common_group
 * \param [in]     p                         Pointer to the memory block to write.
 * \param [in]     data                      One byte of data to write.
 * \return         void                      None
 */
void SwPrivateAreaWriteByte(u8 *p, u8 data);

/**
 *  \brief Copies the values of n bytes from the location pointed to by s directly to the memory block pointed to by d. \n
 * This function is just an interface to access private or protected buffer; it should be realized by customer.
 * \ingroup common_group
 * \param [in]     d                         Pointer to the destination array where the content is to be copied.
 * \param [in]     s                         Pointer to the source of data to be copied.
 * \param [in]     n                         Number of bytes to copy.
 * \return         void *                    A pointer to the destination array, d, is returned.
 */
void * SwPrivateAreaMemcpy(void *d,  const void *s,  u32 n);

/**
 * Sets the first n bytes of the block of memory pointed to by d to the specified value c. \n
 * This function is just an interface to access private or protected buffer; it should be realized by customer.
 * \ingroup common_group
 * \param [in]     p                         Pointer to the block of memory to fill.
 * \param [in]     c                         Value to be set.
 * \param [in]     n                         number of bytes to be set to the value c.
 * \return         void *                    A pointer to the block of memory to fill, d, is returned.
 */
void * SwPrivateAreaMemset(void *p,  i32 c, u32 n);

/* check buffer whether NULL */
u32 SwCheckMemValid(const struct DWLDecBuffers *buffer, const void *ppu_cfg, u32 use_separated_pp);

/* check buffer whether aligned and the size valid */
u32 SwCheckBufferValid(u64 next_buf_size, const  struct DWLLinearMem *info, u64 *total_size);

/* find the matched addr from buffers, return the index of buffers */
u32 SwFindBufferIdByAddr(const struct DWLDecBuffers *buffers, u32 buffer_num,
                         const struct DWLLinearMem *addr);

/* whether exist such addr in buffers, currently only used for vp6 */
u32 SwIsMatchBufferByAddr(const struct DWLDecBuffers *buffers, const struct DWLLinearMem *addr);

/* use replace_buffer to replace the out_buf */
void SwReplacePpOutBuf(u32 ext_flag, const struct DWLDecBuffers *replace_buffer, struct DWLDecBuffers *out_buf);

/* set current pp channel output addr when nextpicture internal invoked */
void SwSetPpOutAddr(const struct SwParames *sw_parames, u32 ppu_id, const struct DWLLinearMem *cur_pp_data,
                    struct DWLDecBuffers *temp_out_addr, struct DecOutputPictureAddr *out_pic_addr);
void SwConvertOutAddr(const struct DWLDecBuffers *src_out0, u32 ppu_id, u32 use_separated_pp,
                      const struct DecOutputPictureAddr *out_pic_addr, struct DWLDecBuffers *buffers);
void SwSetNoPPOutFormat(u32 is_compressor, const u32 *tiled_mode, u32 bit_depth, u32 chroma_format_idc,
                        enum DecPictureFormat *output_format);
void SwSetG1NoPPOutFormat(const u32 *tiled_mode, u32 bit_depth, u32 chroma_format_idc,
                          enum DecPictureFormat *output_format);
u32 SwSetNoPPStride(u32 is_compressor, u32 chroma_format_idc, u32 bit_depth,
                    u32 align, u32 pic_width_y, u32 pic_width_c,
                    u32 *pic_stride, u32 *pic_stride_ch);
u32 SwSetG1NoPPStride(u32 chroma_format_idc, u32 bit_depth,
                      u32 align, u32 pic_width_y, u32 pic_width_c,
                      u32 *pic_stride, u32 *pic_stride_ch);
u32 SwIsMatchBufferByPPData(const struct DecOutputPictureAddr *out_pic_addr,
                            const struct DWLDecBuffers *pp_data, u32 use_separated_pp);

/* check current channel has multi plane output */
u32 SwGetChannelIsMultiPlane(const struct DWLDecBuffers *out_addr, u32 ppu_id);
#ifdef ENABLE_FPGA_VERIFICATION
/* only used to pp enable */
void SwMemsetBuffer(const struct SwParames *sw_parames, struct DWLDecBuffers *buffers);
#endif

void SwSetCommonCfg(u32 misc_ctrl, CommonCfg *common_cfg);
void SwUpdateCommonCfg(u32 misc_ctrl, CommonCfg *common_cfg);
enum DecRet SwSetLowLatencyInfo(const void *dwl, enum DWLClientType client_type,
                                struct LLStrmInfo *llstrminfo, u32 *regs);
void SwUpdateLowLatencyInfo(const void *dwl, const struct DecHwFeatures *hw_feature, addr_t strm_bus_address, u32 strm_len,
                            enum DWLClientType client_type, struct LLStrmInfo *llstrminfo, u32 *regs);
void SwReleaseLowLatencyInfo(const void *dwl, struct LLStrmInfo *llstrminfo);

void SwSetDecIRQAttr(u32 *regs, u32 vcmd_version_flag);
void SwSetCacheShaperReg(u32 *regs, u32 sim_mc);

#ifdef __cplusplus
}
#endif
#endif /* #ifdef SW_UTIL_H_ */
