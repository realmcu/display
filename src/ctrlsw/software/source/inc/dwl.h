/*
  * Copyright 2014-2025 Verisilicon Inc.
  *
  * The software contained herein may be directly re-distributed or may be adapted for redistribution
  * by SoC vendors who have licensed the related VeriSilicon/Hantro core IP, with the understanding
  * that the re-distribution is limited to the users who have purchased the SoC product which contains
  * the related VeriSilicon/Hantro IP. Otherwise, the information contained herein is not to be used by
  * or disclosed to the third parties without the express written permission of an officer of VeriSilicon Holdings Co., Ltd.
  *
  * The above copyright notice and this permission notice (including the
  * next paragraph) shall be included in all copies or substantial
  * portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
  * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
  * IN NO EVENT SHALL VERISILICON AND/OR ITS SUPPLIERS BE LIABLE FOR ANY
  * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __DWL_H__
#define __DWL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "basetype.h"
#include "decapicommon.h"
#include "dwlthread.h"

#ifdef EXT_BUFS_NUM
#define MAX_PIC_BUFFERS EXT_BUFS_NUM
#else
#define MAX_PIC_BUFFERS 34
#endif

#define MAX_DEC_DEV_NODES 1
#define MAX_MEM_DEV_NODES 1
#if defined(SUPPORT_MEM_SYNC) && defined(USE_DMA_DRIVER)
#define MAX_DMA_DEV_NODES 1
#endif

/**
 * Defines the data transfer directions for DWL.
 * \ingroup common_group
 */
enum DWLMemSyncDirection {
  HOST_TO_DEVICE = 0, /**< \brief Copies data from host memory to device memory. */
  DEVICE_TO_HOST = 1  /**< \brief Copies data from device memory to host memory. */
};

/**
 * \defgroup return_codes Return Codes
 * @{
 *
 */
/**
 * Defines return codes of DWL API functions.
 * \ingroup return_codes
 */
enum DWLRet {
  DWL_OK = 0,  /**< \brief The hardware is working properly. */
  DWL_ERROR = -1,  /**< \brief An error occurs when the hardware is working. */
  DWL_CACHE_FATAL_RECOVERY = 2, /**< \brief (Deprecated) An error has occurred and then
recovered when the DWL cache is working. */
  DWL_CACHE_FATAL_UNRECOVERY = 3, /**< \brief (Deprecated) An error has occurred but does not recover when the DWL cache is working. */
  DWL_CACHE_OK, /**< \brief (Deprecated) The DWL cache is working properly. */
  DWL_SLICE_DECODING_SUSPEND, /**< \brief The hardware suspends during slice decoding.*/
  DWL_SLICE_DECODING_ABORTED, /**< \brief The hardware is aborted when slice decoding times out. */
  DWL_HW_WAIT_OK = DWL_OK,  /**< \brief The function waits for the hardware to complete processing and the hardware works properly as
expected. */
  DWL_HW_WAIT_ERROR = DWL_ERROR,  /**< \brief An error occurs when the function waits for the hardware to complete processing, which may be caused by an error in the Linux kernel API. */
  DWL_HW_WAIT_TIMEOUT = 1, /**< \brief A timeout occurs when the function waits for the hardware to complete processing. */
  DWL_MEM_SYNC_NOT_SUPPORT = 2 /**< \brief The data transfer between the host and device is disabled.*/
};

/** @} */

/**
 * Defines DWL client types. Each client type corresponds to a stream type.
 * \ingroup common_group
 */
enum DWLClientType {
  DWL_CLIENT_TYPE_H264_DEC = 1 , /**< \brief H.264 client.*/
  DWL_CLIENT_TYPE_MPEG4_DEC = 2, /**< \brief MPEG-4 client.*/
  DWL_CLIENT_TYPE_JPEG_DEC = 3, /**< \brief JPEG client.*/
  DWL_CLIENT_TYPE_PP = 4, /**< \brief (Reserved) PP client.*/
  DWL_CLIENT_TYPE_VC1_DEC = 5, /**< \brief VC1 client.*/
  DWL_CLIENT_TYPE_MPEG2_DEC = 6, /**< \brief MPEG-2 client.*/
  DWL_CLIENT_TYPE_VP6_DEC = 7, /**< \brief VP6 client.*/
  DWL_CLIENT_TYPE_AVS_DEC = 8, /**< \brief AVS client.*/
  DWL_CLIENT_TYPE_RV_DEC = 9, /**< \brief RV client.*/
  DWL_CLIENT_TYPE_VP8_DEC = 10, /**< \brief VP8 client.*/
  DWL_CLIENT_TYPE_VP9_DEC = 11, /**< \brief VP9 client.*/
  DWL_CLIENT_TYPE_HEVC_DEC =12, /**< \brief HEVC client.*/
  DWL_CLIENT_TYPE_ST_PP = 14, /**< \brief Standalone PP client.*/
  DWL_CLIENT_TYPE_H264_MAIN10 = 15, /**< \brief H.264 Main Profile 10 client.*/
  DWL_CLIENT_TYPE_AVS2_DEC = 16, /**< \brief AVS2 client.*/
  DWL_CLIENT_TYPE_AV1_DEC = 17, /**< \brief AV1 client.*/
  DWL_CLIENT_TYPE_VVC_DEC =18 , /**< \brief VVC client.*/
  DWL_CLIENT_TYPE_AVS3_DEC =19 , /**< \brief AVS3 client.*/
  DWL_CLIENT_TYPE_PNG_DEC =20 , /**< \brief PNG client. */
  DWL_CLIENT_TYPE_MAX = MAX_CLIENT_TYPE /**< \brief The maximum number of client types. */
};

/** \brief The page size and alignment for linear memory chunks. */
#define LINMEM_ALIGN 4096
/** \brief The nearest aligned linear memory address before the input address x. */
#define NEXT_ALIGNED(x) \
  ((((size_t)(x)) + LINMEM_ALIGN - 1) / LINMEM_ALIGN * LINMEM_ALIGN)

/** \brief The macro that signals a maximum of 20 M2M buffers.
 *  \ingroup common_group */
#define MAX_VCMD_M2M_NUM 20

/** \brief The macro that signals a maximum of 20 MSET buffers.
  * \ingroup common_group */
#define MAX_VCMD_M2MP_NUM  20

/** \brief The macro that signals a maximum of 2 M2MP buffers.
  * \ingroup common_group */
#define MAX_VCMD_MSET_NUM  2

/**
 * \brief Contains the information about the M2M command buffer.
 * \ingroup common_group
 */
struct CDF_INFO {
  /** \brief The size of the M2M command buffer. */
  u32 cdf_size;
  /** \brief The source address of the M2M command buffer.*/
  addr_t src_cdf_addr;
  /** \brief The destination address of the M2M command buffer. */
  addr_t dst_prob_addr;
};

/**
 * \brief Contains the information about MSET commands.
 * \ingroup common_group
 */
struct MSET_INFO {
  /** \brief The data length of MSET commands. */
  u32 mset_length;
  /** \brief The data size of MSET commands. */
  u32 mset_size;
  /** \brief The value of MSET commands. */
  u32 mset_value;
  /** \brief The data destination address of MSET commands. */
  addr_t mset_dst_addr;
};

/**
 * \brief Contains the information about M2MP commands.
 * \ingroup common_group
 */
struct M2MP_INFO {
  /** \brief The data width of M2MP commands. */
  u32 m2mp_width;
  /** \brief The data height of M2MP commands. */
  u32 m2mp_height;
  /** \brief The size of skipped data when data is copied from the source address of M2MP commands. */
  u32 m2mp_srcskip;
  /** \brief The data size of M2MP commands. */
  u32 m2mp_size;
  /** \brief The mode of M2MP commands. */
  u32 m2mp_mode;
  /** \brief The data source address of M2MP commands. */
  addr_t m2mp_src_addr;
  /** \brief The data destination address of M2MP commands. */
  addr_t m2mp_dst_addr;
  /** \brief The CRC result of M2MP commands. */
  u32 m2mp_crc_res;
};

/**
 * \brief The memory buffer used only by the CPU. It is a non-secure CMA memory buffer.
 * \ingroup common_group
*/
#define DWL_MEM_TYPE_CPU                 0x0000U
/**
 * \brief The memory buffer that can be read by the video processing unit (VPU) and written by the CAAM.
 * \ingroup common_group
*/
#define DWL_MEM_TYPE_SLICE               0x0001U
/**
 * \brief The memory buffer that can be read and written by the VPU, and can be read by the render.
 * \ingroup common_group
*/
#define DWL_MEM_TYPE_DPB                 0x0002U
/**
 * \brief The memory buffer that can be read by the VPU and written by the CPU. It is a non-secure memory buffer.
 * \ingroup common_group
*/
#define DWL_MEM_TYPE_VPU_WORKING         0x0003U
/**
 * \brief The memory buffer that can be read and written by the CPU and can be written by the VPU. It is used only for the
VP9 counter context table.
 * \ingroup common_group
*/
#define DWL_MEM_TYPE_VPU_WORKING_SPECIAL 0x0004U
/**
 * \brief The memory buffer that can be read and written only by the VPU.
 * \ingroup common_group
*/
#define DWL_MEM_TYPE_VPU_ONLY            0x0005U
/**
 * \brief The memory buffer that can be read and written by both the VPU and CPU.
 * \ingroup common_group
*/
#define DWL_MEM_TYPE_VPU_CPU             0x0006U
/**
 * \brief The memory buffer that can only be used by the host or device.
 * \ingroup common_group
*/
#define DWL_MEM_TYPE_SYNC_DEVICE_ONLY     0x0100U
/**
 * \brief The memory buffer that requires data transfer from the host to device.
 * \ingroup common_group
*/
#define DWL_MEM_TYPE_SYNC_HOST_TO_DEVICE  0x0200U
/**
 * \brief The memory buffer that requires data transfer from the device to host.
 * \ingroup common_group
*/
#define DWL_MEM_TYPE_SYNC_DEVICE_TO_HOST  0x0300U
/**
 * \brief The memory buffer that requires bidirectional data transfer between the host and device.
 * \ingroup common_group
*/
#define DWL_MEM_TYPE_SYNC_HOST_AND_DEVICE 0x0400U

/** \brief bit 22:16 is used to indicate the usage hint of the buffer */
#define DWL_MEM_USAGE_HINT_MASK  0x7F0000U
/** \brief The least significant bit (LSB) of memory usage hint bits in memory attribute words. */
#define DWL_MEM_USAGE_HINT_SHIFT 16
/** \brief bit 23 is used to indicate the mem attribute(META/CORE) */
#define DWL_MEM_ATTR_SHIFT 23
/** \brief The security attribute of a buffer.*/
enum MEM_ATTR {
  META, /**< \brief The buffer does not have sensitive data. */
  CORE  /**< \brief The buffer has sensitive data. */
};

/** \brief Non-specified attributes of memory. These attributes are regarded as META. */
#define ATTR_NOT_DEF META


#define DWL_HINT_LIST(DEF) \
  DEF(DWL_MEM_USAGE_UNDEFINED, ATTR_NOT_DEF) /* invalid*/ \
  DEF(DWL_MEM_USAGE_IN_STRM, CORE) /* R0 */ \
  DEF(DWL_MEM_USAGE_IN_STRM_SLICE, CORE) /* R3 */ \
  DEF(DWL_MEM_USAGE_IN_AVS2ALF_TABLE, META) /* R8 */ \
  DEF(DWL_MEM_USAGE_IN_AVS2WQM_TABLE, META) /* R10 */ \
  DEF(DWL_MEM_USAGE_IN_MBCTRL, META) /* R3 */ \
  DEF(DWL_MEM_USAGE_IN_MBRLC, CORE) /* R0 */ \
  DEF(DWL_MEM_USAGE_IN_MBDC_COEFF, META) /* R3 */ \
  DEF(DWL_MEM_USAGE_IN_MV, META) /* R3 */ \
  DEF(DWL_MEM_USAGE_IN_INTRA_PRED, META) /* R3 */ \
  DEF(DWL_MEM_USAGE_IN_RESIDUAL, CORE) /* R0 */ \
  DEF(DWL_MEM_USAGE_IN_CABAC_INIT, META) /* R3, R8, R10 */ \
  DEF(DWL_MEM_USAGE_IN_QTABLE, META) /* R3 */ \
  DEF(DWL_MEM_USAGE_IN_PROBTABLE, META) /* R8 */ \
  DEF(DWL_MEM_USAGE_IN_SEGMENT_MAP, META) /* R7, W8 */ \
  DEF(DWL_MEM_USAGE_IN_PPULANCZOS_TABLE, META) /* R(18,21,24,27) */ \
  DEF(DWL_MEM_USAGE_IN_AV1TILEINFO, META) /* R3 */ \
  DEF(DWL_MEM_USAGE_IN_AV1GLOBAL_MODEL, META) /* R8 */ \
  DEF(DWL_MEM_USAGE_IN_GRAIN, META) /* R2 */ \
  DEF(DWL_MEM_USAGE_IN_QUANT_MAT, META) /* R3 */ \
  DEF(DWL_MEM_USAGE_OUT_SURFACE, CORE) /* W0, W1*/ \
  DEF(DWL_MEM_USAGE_OUT_BITPLANE_CTRL, CORE) /* R0 */ \
  DEF(DWL_MEM_USAGE_OUT_RVRM_NCNB, ATTR_NOT_DEF) /* dev */ \
  DEF(DWL_MEM_USAGE_OUT_VP8DONWSCALE, ATTR_NOT_DEF) /* dev */\
  DEF(DWL_MEM_USAGE_OUT_REFERENCE, CORE) /* W0, W1, W10, W11*/ \
  DEF(DWL_MEM_USAGE_OUT_EXTRA_B, ATTR_NOT_DEF) /* dev */ \
  DEF(DWL_MEM_USAGE_OUT_PP, CORE) /* W(0, 1, 3, 4, 12, 13, (18-21)) */ \
  DEF(DWL_MEM_USAGE_OUT_CTXCOUNT, META) /* W30 */ \
  DEF(DWL_MEM_USAGE_TMP_AV1MULTICORE_SYNC, META) /* R13, W7 */ \
  DEF(DWL_MEM_USAGE_TMP_AV1AFBC_TILE, ATTR_NOT_DEF) /* dev */\
  DEF(DWL_MEM_USAGE_TMP_AV1FILTER, CORE) /* R/W5, W6, R/W14, R/W15, R/W16, R/W17, R(19,20,22,23,25,26,28,29), W(22-29) */\
  DEF(DWL_MEM_USAGE_TMP_DIRMV, META) /* R4, R13, W2, W7*/ \
  DEF(DWL_MEM_USAGE_TMP_PPUTILE_EDGE, CORE) /* R/W5, R/W6, R/W16, R/W17, R(19,20,22,23,25,26,28,29), W(22-29)*/ \
  DEF(DWL_MEM_USAGE_TMP_FILTER_CONTROL, META) /*R/W9*/ \
  DEF(DWL_MEM_USAGE_TMP_COEEFT, META) /* R4, W2 */ \
  DEF(DWL_MEM_USAGE_TMP_RPRWORK, ATTR_NOT_DEF) /* dev */\
  DEF(DWL_MEM_USAGE_TMP_PP, CORE) /* W3, W4, W12, W13, W(18-21)*/ \
  DEF(DWL_MEM_USAGE_TMP_EXTRA, ATTR_NOT_DEF) /* test */\
  DEF(DWL_MEM_USAGE_TMP_EXTERNAL, ATTR_NOT_DEF) /* test */\
  DEF(DWL_MEM_USAGE_TMP_EXTERNAL_FRM, ATTR_NOT_DEF) /* test */\
  DEF(DWL_MEM_USAGE_TMP_PROBTABLE_OUT, META) /* R10 */ \
  DEF(DWL_MEM_USAGE_TMP_MISC_LINEAR, META) /* R3, R10 */ \
  DEF(DWL_MEM_USAGE_TMP_VP9MULTICORE_SYNC,META) /* R13, W7*/ \
  DEF(DWL_MEM_USAGE_TMP_FAKERFC_TABLE, META) /* R11 */\
  DEF(DWL_MEM_USAGE_IN_STRM_STATUS, META) /* R11 */\
  DEF(DWL_MEM_USAGE_IN_AV1CDFS, ATTR_NOT_DEF) /* dev */\
  DEF(DWL_MEM_USAGE_IN_AV1CDFS_NDVC, ATTR_NOT_DEF) /* dev */\
  DEF(DWL_MEM_USAGE_IN_AV1CDFS_LAST, ATTR_NOT_DEF) /* dev */\
  DEF(DWL_MEM_USAGE_IN_VVC_SLICE_SCAN, META) \

#define DEF_ENUM(enum, attr) enum,

/** \brief Specifies the usage of a buffer. */
enum DWLMemUsageHint {
  DWL_HINT_LIST(DEF_ENUM)
};

#define DEF_ATTR(enum, attr) enum##_Attr=attr,
/** Specifies the buffer attributes when the security mode is enabled. */
enum DWLMemAttrHint {
  DWL_HINT_LIST(DEF_ATTR)
};

#define STR(str) #str

#define USAGE_ATTR(usage) usage##_Attr

/** \brief Sets the memory usage hint and security flag in memory attribute words.
 *
 * \param [inout] mem_type Input: The memory attribute words and other attributes. <br>
 *    Output: The input data added with usage hint and security flag.
 * \param [in] usage The memory usage hint.
 * \param [in] is_secure The security status of the memory.
 */
#define SET_MEM_USAGE(mem_type, usage, is_secure) do { \
      (mem_type) |= (((usage)<<DWL_MEM_USAGE_HINT_SHIFT)| \
      ((USAGE_ATTR(usage)&(is_secure))<<DWL_MEM_ATTR_SHIFT)); \
      LOG_MEM_SECURE((1), STR(usage), USAGE_ATTR(usage)); \
  } while(0)

#define DWLGetMemHint(mem_type) (((mem_type)&DWL_MEM_USAGE_HINT_MASK)>>DWL_MEM_USAGE_HINT_SHIFT)
/** \brief The macro that signals whether the memory is secure by parsing the memory attribute flag. \n
 * Valid values: \n
 * <tt>1</tt>: This memory is secure because it is only accessible to the device. \n
 * <tt>0</tt>: This memory is non-secure because it is accessible to both the host and device.
 *  \ingroup common_group */
#define DWLGetMemAttribute(mem_type) ((mem_type>>DWL_MEM_ATTR_SHIFT)&0x1)

/**
 * \brief Contains the information about a linear memory buffer.
 */
struct DWLLinearMem {
  u32 *virtual_address; /**< \brief A pointer to the aligned virtual address of the linear memory buffer.*/
  addr_t bus_address; /**< \brief The bus address of the aligned linear memory buffer. \n
   The start address of the linear memory buffer must be 128 bits aligned. */
  u32 *alloc_virtual_addr; /**< \brief The virtual address of the memory buffer provided for the CPU to access. */
  addr_t alloc_bus_addr;    /**< \brief The bus address of the memory buffer provided for the video decoding IP to access. */
  u64 size;         /**< \brief The physical size in bytes of the linear memory buffer. \n
  The value must be rounded up to a multiple of the Linux page size. */
  u64 logical_size; /**< \brief The requested size in bytes of the linear memory buffer. */
  u32 mem_type; /**< \brief The owner and attributes of the memory buffer.
                 * \n Bit [23] indicates whether the buffer is secure. \n
                 Valid values: \n
                 <tt>CORE</tt>: This memory buffer is secure because it is only accessible to the device. \n
                 <tt>META</tt>: This memory buffer is non-secure because it is accessible to both the host and device.
                 * \n Bits [16:22] indicate the usage hints of the memory buffer.
                 * \n Bits [15:8] indicate  which IP is allowed to access the memory.
                 * \n Bits [7:0] indicate the attributes of the memory buffer.*/
  unsigned long id;      /**< \brief The ID of the memory buffer. */
  void *priv; /**< \brief The context. */
};

#ifdef SUPPORT_MEM_SYNC
/**
 * \brief Contains input information to the host.
 *  \ingroup common_group
 */
struct HostInInfo {
  /** \brief The I/O virtual address (IOVA) of the host memory allocated by the memory driver when MMU is enabled.*/
  addr_t iova;
  /** \brief The bus address of the device memory allocated by the memory driver when MMU is disabled. */
  addr_t ba;
};

/**
 * \brief Defines output information from the host.
 *  \ingroup common_group
 */
struct HostOutInfo {
  /** \brief The virtual address of the device memory output from the host. */
  u32 *va;
};

/** Gets output information from the host. \n
 * This is a callback function.
 * \ingroup common_group
 * \param [in] priv      A pointor to the context.
 * \param [in] host_in_info   A pointer to the input information to the host.
 * \return The output information from the host.
 */
typedef struct HostOutInfo DWLMemSyncGetHostOutInfo(void *priv, struct HostInInfo *host_in_info);
#endif

/**
 * \brief For internal debugging use.
 * \ingroup common_group
 */
#define IN_STAGE (0)
/**
 * \brief For internal debugging use.
 * \ingroup common_group
 */
#define OUT_STAGE (1)

struct DWL_Buf_Info {
  u32 reg_msb; /**< \brief The most significant bit (MSB) of the address register in the buffer. */
  u32 reg_lsb; /**< \brief The least significant bit (LSB) of the address register in the buffer. */
  u32 buf_len; /**< \brief The buffer size. */
};

/**
 * \brief Contains the initialization parameters of the DWL instance.
 * \ingroup common_group
 */
struct DWLInitParam {
  enum DWLClientType client_type; /**< \brief The client type.*/
  char *dec_dev;  /**< \brief The name of the video decoding IP. */
  char *mem_dev;  /**< \brief The name of the <tt>memalloc</tt> driver. */
#ifdef USE_DMA_DRIVER
  char *dma_dev;  /**< \brief The DMA to select. */
#endif
  void *priv; /**<\brief The context.*/
/* Sets the AXI latency for FPGA tests. */
#ifdef SUPPORT_RANDOM_LATENCY
  u32 axi_lg_r;  /**< \brief Sets the base latency for AXI AR channel. */
  u32 axi_lg_w;  /**< \brief Sets the base latency for AXI AW channel. */
#endif
#ifdef SUPPORT_MEM_SYNC
  /** \brief The callback function to get device memory information from an application.*/
  DWLMemSyncGetHostOutInfo *mem_sync_callback;
  u32 mem_sync_enable; /**<\brief Enables the environment where the host and device cannot access each other's memory. \n Memory synchronization is required in this environment.*/
  u32 mem_sync_test; /**<\brief Enables the test on the memory synchronization flow.*/
#endif

  /** \brief Sets the dump mode.
   * \n The dump function is an internal debugging method for a single stream.
   * \n Valid values:
   * \n <tt>1</tt>: Dump register configurations.
   * \n <tt>2</tt>: Dump buffer information.
   * \n <tt>3</tt>: Dump buffer information and register configurations. */
  u8 dump_mode;
};

/**
 * \brief Contains the information about output buffers.
 * \ingroup common_group
 */
struct DWLDecBuffers {
  /** \brief The information about data in a linear memory buffer that serves as an output buffer. \n
   * <tt>DEC_MAX_OUT_COUNT</tt> indicates the maximum number of channels from which the data is output. \n
   * <tt>DEC_MAX_OUTPUT_PLANE</tt> indicates the maximum number of planes in data of each output channel. \n
  */
  struct DWLLinearMem dec_buffer[DEC_MAX_OUT_COUNT][MAX_OUTPUT_PLANE];
};

#define DWL_SECURE_MODE_BIT         31
#define DWL_CMD_PRIORITY_BIT        30
#define DWL_S_CMD_PRIORITY(a, b)    (a |= (b << DWL_CMD_PRIORITY_BIT))
#define DWL_G_CMD_PRIORITY(a)       ((a >> DWL_CMD_PRIORITY_BIT) & 1)
/**
 * \brief Contains the information that is used to determine the resources to be reserved.
 * \ingroup common_group
 */
struct DWLReqInfo {
  /** \brief The mask that is used to select a core. \n
   * Bits [0, 15] indicate the masks of cores 0 to 15. \n
   * Bits [16, 26] indicate the client types. \n
   * Bits [27, 29] are reserved. \n
   * Bit [30] indicates the priority of a command buffer task for the VCMD. \n
   * Valid values: \n
   * <tt>1</tt>: High. <tt>0</tt>: Normal. \n
   * Bit [31] indicates whether to enable DRM. \n
   * Valid values: \n
   * <tt>1</tt>: Enable DRM. <tt>0</tt>: Disable DRM. */
  u32 core_mask;
  /** \brief The picture width. */
  u32 width;
  /** \brief The picture height.*/
  u32 height;
  /** \brief The decoder instance. */
  void *owner;
};

/**
 * \brief Contains the information about the low-latency mode.
 * \ingroup common_group
 */
struct strmInfo {
  u32 low_latency; /**< \brief Whether the stream runs in low-latecny mode. */
  u32 send_len; /**< \brief The length of the data perpared for hardware. */
  addr_t strm_bus_addr; /**< \brief The physical address of the input stream buffer, which is provided by the host for the decoder node. */
  addr_t strm_bus_start_addr; /**< \brief The start physical address of the input stream buffer, which is provided by the host for the decoder node. */
  u8* strm_vir_addr; /**< \brief The virtual address of the input stream buffer, whihch is provided for the host.*/
  u8* strm_vir_start_addr; /**< \brief The start virtual address of the input stream buffer, which is provided for the host. */
  u32 last_flag; /**< \brief The flag for the last data of the current stream. */
  u8* strm_buff; /**< \brief The virtual address of the input stream buffer. */
  addr_t buff_size; /**< \brief The size of the input stream buffer. */
  addr_t strm_buff_bus_address; /**< \brief The bus address of the input stream buffer. */
  u32 is_back_to_buffer_begin;/**< \brief (For H.264) Whether the bits are flushed to the start of the buffer. This field is set to HIGH when the bits are flushed. */
};

/**
 * \brief Contains the CRC information.
 * \ingroup common_group
 */
typedef struct CrcData_ {
  struct CrcData_ *next; /**< \brief A poniter to the next CrcData. */
  u32 crc[MAX_VCMD_M2MP_NUM]; /**< \brief CRC data. */
  u8 used; /**< \brief The flag for used CrcData. */
  u8 last; /**< \brief The flag for the last used CrcData. */
} CrcData;

#ifdef FPGA_PERF_AND_BW
struct DWLPerfInfo{
  u64 bitrate;
  u64 write_bw;
  u64 read_bw;
  u32 cycles;
  u32 pic_num;
#ifdef DUMP_STRM_COMP_RATIO
  u64 strm_len;
  u64 yuv_size;
#endif
};
#endif


/**
 * Initializes a DWL instance.
 * \ingroup common_group
 * \param [in]     param                         A pointer to the initialization parameters.
 * \return         A pointer to the new DWL instance.
 */
const void *DWLInit(struct DWLInitParam *param);

/**
 * Releases a DWL instance.
 * \ingroup common_group
 * \param [in]     instance                     A pointer to the DWL instance.
 * \return         <tt> \ref DWL_OK</tt> or <tt> \ref DWL_ERROR</tt>
 */
enum DWLRet DWLRelease(const void *instance);


/**
 * Allocates memory in <tt>n</tt> bytes.
 * \ingroup common_group
 * \param [in]     n                         The number of bytes to be allocated.
 * \return         If the function succeeds, a pointer to the memory is returned. \n
 *                 If the function fails, a NULL pointer is returned.
 */
void *DWLmalloc(size_t n);

/**
 * Releases memory allocated by <tt>DWLmalloc()</tt> or <tt>DWLcalloc()</tt>.
 * \ingroup common_group
 * \param [in]     p                         A pointer to the memory to be released.
 * \return         None
 */
void DWLfree(void *p);

/**
 * Allocates memory for an array of <tt>n</tt> elements with each element in <tt>s</tt> bytes. \n
 * The allocated memory is cleared.
 * \ingroup common_group
 * \param [in]     n                         The number of elements.
 * \param [in]     s                         The size of each element.
 * \return         If the function succeeds, a pointer to the allocated memory is returned. \n
 *                 If the function fails, a NULL pointer is returned.
 */
void *DWLcalloc(size_t n, size_t s);

/**
 * Copies <tt>n</tt> bytes from memory area <tt>s</tt> to memory area <tt>d</tt>.
 * \ingroup common_group
 * \param [in]     d                         A pointer to the destination memory area.
 * \param [in]     s                         A pointer to the source memory area.
 * \param [in]     n                         The number of bytes to copy.
 * \return         A pointer to the destination, <tt>d</tt>, is returned.
 */
void *DWLmemcpy(void *d, const void *s, size_t n);

/**
 * Fills memory area with a value.
 * \ingroup common_group
 * \param [in]     d                         A pointer to the start address of the memory.
 * \param [in]     c                         The value used to fill the memory.
 * \param [in]     n                         The number of bytes to be filled with the value <tt>c</tt>.
 * \return         A pointer to the filled memory is returned.
 */
void *DWLmemset(void *d, i32 c, size_t n);

/**
 * Allocates memory as a reference frame buffer for a DWL instance. \n
 * The memory must be contiguous and linear and reside in the physical memory.
 * \ingroup common_group
 * \param [in]     instance                  A pointer to the DWL instance.
 * \param [in]     size                      The size in bytes of the memory to be allocated.
 * \param [in, out]     info                 A pointer to the allocated memory.
 * \return         <tt> \ref DWL_OK</tt> or <tt> \ref DWL_ERROR</tt>
 */
enum DWLRet DWLMallocRefFrm(const void *instance, u64 size, struct DWLLinearMem *info);

/**
 * Releases a reference frame buffer allocated by <tt>DWLMallocRefFrm()</tt>.
 * \ingroup common_group
 * \param [in]     instance                  A pointer to the DWL instance.
 * \param [in]     info                      A pointer to the buffer to be released.
 * \return         None
 */
void DWLFreeRefFrm(const void *instance, struct DWLLinearMem *info);

/**
 * Allocates memory as a software/hardware shared buffer for a DWL instance. \n
 * The memory must be contiguous and linear and reside in the physical memory.
 * \ingroup common_group
 * \param [in]     instance                  A pointer to the DWL instance.
 * \param [in]     size                      The size in bytes of the memory to be allocated.
 * \param [in, out]     info                 A pointer to the allocated memory.
 * \return         <tt> \ref DWL_OK</tt> or <tt> \ref DWL_ERROR</tt>
 */
enum DWLRet DWLMallocLinear(const void *instance, u64 size, struct DWLLinearMem *info);

/**
 * Releases a software/hardware shared buffer allocated by <tt>DWLMallocLinear()</tt>.
 * \ingroup common_group
 * \param [in]     instance                  A pointer to the DWL instance.
 * \param [in]     info                      A pointer to the buffer to be released.
 * \return         None
 */
void DWLFreeLinear(const void *instance, struct DWLLinearMem *info);

/**
 * Copies <tt>n</tt> bytes from host buffer <tt>s</tt> to a linear device memory for a DWL instance. \n
 * You can customize the implementation of this function.
 * \ingroup common_group
 * \param [in]     instance        A pointer to the DWL instance.
 * \param[in]      device_bus_addr The address of the destination device memory.
 * \param [in]     s               A pointer to the source host buffer.
 * \param [in]     n               The number of bytes to copy.
 * \return         None
 */
void *DWLLinearMemcpy(const void *instance, addr_t device_bus_addr, const void *s, size_t n);

/**
 * Fills a physical linear buffer with a character for a DWL instance. \n
 * You can customize the implementation of this function.
 * \ingroup common_group
 * \param [in]     instance   A pointer to the DWL instance.
 * \param[in]      mem        A pointer to the physical linear buffer to be filled.
 * \param[in]      offset     The offset related to the start address of the physical linear buffer, in bytes. \n
 *                            Alignment is implemented in this API function.
 * \param [in]     c          The character to fill.
 * \param [in]     n          The number of characters to fill.
 * \return         None
 */
void *DWLLinearMemset(const void *instance, struct DWLLinearMem *mem, i32 offset, i32 c, size_t n);

#ifdef SUPPORT_MEM_SYNC
/**
 * \brief Contains the address information of the buffer for verification of memory synchronization.
 * \ingroup common_group
 * \n <b>NOTE</b>: This structure can be overwritten.
 */
struct sync_mem {
  /** \brief The aligned virtual address of the buffer from the host view. */
  u32 *dev_va;
  /** \brief The aligned physical address of the buffer from the host view. */
  addr_t dev_pa_alloc;
  /** \brief The original virtual address of the buffer in device from the host view. */
  u32 *dev_va_alloc;
};

/**
 * Transfers data between the host and device for a DWL instance. \n
 * The host memory is an SDK internal buffer. \n
 * This function is available only if memory synchronization is supported.
 * \ingroup common_group
 * \param [in]     inst     A pointer to the DWL instance.
 * \param [in]     mem      A pointer to the address of the host memory.
 * \param [in]     offset   The offset related to the hardware register base address, in bytes.
 * \param [in]     length   The number of bytes to be synchronized.
 * \param [in]     dir      The data transfer direction.
 * \return         <tt> \ref DWL_OK</tt> or <tt> \ref DWL_ERROR</tt>
 */
i32 DWLSyncMemData(const void *inst, const struct DWLLinearMem *mem, i32 offset, u32 length,
                    enum DWLMemSyncDirection dir);

/**
 * Transfers data between the host and device for a DWL instance. \n
 * The host memory is an SDK internal buffer. \n
 * This function is available only if memory synchronization is supported.
 * \ingroup common_group
 * \param [in]     inst              A pointer to the DWL instance.
 * \param [in]     device_bus_addr   The address of the device memory.
 * \param [in]     host_virtual_addr A pointer to the address of the host memory.
 * \param [in]     length            The length of data to be transferred, in bytes.
 * \param [in]     dir               The data transfer direction.
 * \return         <tt> \ref DWL_OK</tt> or <tt> \ref DWL_ERROR</tt>
 */
i32 DWLSyncMemData2(const void *inst, addr_t device_bus_addr, void *host_virtual_addr,
                     u32 length, enum DWLMemSyncDirection dir);
/**
 * Transfers data between the host and device for a DWL instance. \n
 * The host memory is an external buffer allocated by an application. \n
 * This function is available only if memory synchronization is supported.
 * \ingroup common_group
 * \param [in]     inst                             A pointer to the DWL instance.
 * \param [in]     device_bus_addr                  The address of the device memory.
 * \param [in]     host_virtual_addr                A pointer to the address of the host memory.
 * \param [in]     length                           The length of data to be transferred, in bytes.
 * \param [in]     dir                              The data transfer direction.
 * \return  <tt> \ref DWL_OK</tt> or <tt> \ref DWL_ERROR</tt>
 */
i32 DWLSyncMemDataExt(const void *inst, addr_t device_bus_addr, void *host_virtual_addr,
                       u32 length, enum DWLMemSyncDirection dir);
/**
 * Allocates memory as a buffer to the host for a DWL instance. \n
 * This function is available only if memory synchronization is supported.
 * \ingroup common_group
 * \param [in]    inst          A pointer to the DWL instance.
 * \param [in]    alignment     The alignment for allocation.
 * \param [in]    size          The size in bytes of the memory to be allocated.
 * \param [out]   buff          A pointer to the allocated memory.
 * \return        <tt> \ref DWL_OK</tt>
 */
i32 DWLSyncMemAllocHostBuffer(const void *inst, u32 size, u32 alignment,
                              struct DWLLinearMem *buff);

/**
 * Releases a memory buffer from the host. \n
 * This function is available only if memory synchronization is supported.
 * \ingroup common_group
 * \param [in]   inst     A pointer to the DWL instance.
 * \param [in]   buff     A pointer to the memory buffer to be released.
 * \return       <tt> \ref DWL_OK</tt>
 */
i32 DWLSyncMemFreeHostBuffer(const void *inst, struct DWLLinearMem *buff);

#else
#define DWLSyncMemData(inst, mem, offset, length, dir)
#define DWLSyncMemData2(inst, ba, va, length, dir)
#define DWLSyncMemDataExt(inst, ba, va, length, dir)
#define DWLSyncMemAllocHostBuffer(inst, size, alignment, buff) (DWL_MEM_SYNC_NOT_SUPPORT)
#define DWLSyncMemFreeHostBuffer(inst, buff) (DWL_MEM_SYNC_NOT_SUPPORT)
#endif

/**
 * Writes a 32-bit value to a specified hardware register for a DWL instance. \n
 * \ingroup common_group
 * \param [in]     instance                  A pointer to the DWL instance.
 * \param [in]     core_id                   The core ID of the hardware resource.
 * \param [in]     offset                    The offset related to the hardware register base address, in bytes.
 * \param [in]     value                     The value to write to the hardware register.
 * \return         None
 */
void DWLWriteReg(const void *instance, i32 core_id, u32 offset, u32 value);

/**
 * Writes a 32-bit value to a specified number of hardware registers for a DWL instance. \n
 * All registers are written at once at the beginning of frame decoding.
 * \ingroup common_group
 * \param [in]     instance                  A pointer to the DWL instance.
 * \param [in]     core_id                   The core ID of the hardware resource.
 * \param [in]     offset                    The offset related to the hardware register base address, in bytes.
 * \param [in]     from                      The source register address.
 * \param [in]     num_regs                  The number of registers.
 * \param [in]     print_flag                Whether to print the register information.
 * \return         None
 */
void DWLWriteRegs(const void *instance, i32 core_id, u32 offset, u32* from, u32 num_regs, u32 print_flag);

/**
 * Reads and returns a 32-bit value from a specified hardware register for a DWL instance. \n
 * The status register is read after every macroblock is decoded by software.
 * \ingroup common_group
 * \param [in]     instance                  A pointer to the DWL instance.
 * \param [in]     core_id                   The core ID of the hardware resource.
 * \param [in]     offset                    The offset related to the hardware register base address, in bytes.
 * \return         The value of the specified hardware register.
 */
u32 DWLReadReg(const void *instance, i32 core_id, u32 offset);

/**
  * Reads 32-bit values from a specified number of hardware registers for a DWL instance. \n
  * The status registers are read every time a macroblock is decoded by software.
  * \ingroup common_group
  * \param [in]     instance                  A pointer to the DWL instance.
  * \param [in]     core_id                   The core ID of the hardware resource.
  * \param [in]     offset                    The offset related to the hardware register base address, in bytes.
  * \param [in]     dest                      A pointer to the destination address for the register values.
  * \param [in]     num                       The number of registers whose values are read.
  * \return         None
  */
void DWLReadRegs(const void *instance, i32 core_id, u32 offset, u32* dest, u32 num);

/**
 * Writes a 32-bit value to a specified hardware register for a DWL instance. \n
 * This register is written during frame decoding.
 * \ingroup common_group
 * \param [in]     instance                  A pointer to the DWL instance.
 * \param [in]     core_id                   The core ID of the hardware resource.
 * \param [in]     offset                    The offset related to the hardware register base address, in bytes.
 * \param [in]     value                     The value to write to the hardware register.
 * \return         None
 */
void DWLWriteRegToHw(const void *instance, i32 core_id, u32 offset, u32 value);

/**
 * Reads a 32-bit value from a specified hardware register for a DWL instance when hardware is running.
 * \ingroup common_group
 * \param [in]     instance                  A pointer to the DWL instance.
 * \param [in]     core_id                   The core ID of the hardware resource.
 * \param [in]     offset                    The offset related to the hardware register base address, in bytes.
 * \return         The value of the specified hardware register.
 */
u32 DWLReadRegFromHw(const void *instance, i32 core_id, u32 offset);

/**
 * Reserves a hardware resource for the current decoding job. \n
 * This function is part of the hardware sharing mechanism for multi-instance support. \n
 * The function hangs until the calling client is granted exclusive access to the required hardware resource.
 * \ingroup common_group
 * \param [in]     instance                   A pointer to the DWL instance.
 * \param [in]     core_id                    A pointer to the core ID.
 * \param [in]     info                       A pointer to the information about the command buffer to be reserved.
 * \return         <tt> \ref DWL_OK</tt> or <tt> \ref DWL_ERROR</tt>
 */
enum DWLRet DWLReserveHw(const void *instance, struct DWLReqInfo *info, i32 *core_id);

/**
 * Enables the hardware to start a decoding job.
 * \ingroup common_group
 * \param [in]     instance                  A pointer to the DWL instance.
 * \param [in]     core_id                   The core ID of the hardware resource.
 * \param [in]     offset                    The offset related to the hardware register base address, in bytes.
 * \param [in]     value                     The value to be written into the register for enabling the hardware.
 * \return         None
 */
void DWLEnableHw(const void *instance, i32 core_id, u32 offset, u32 value);

/**
 * Stops decoding and disables the hardware.
 * \ingroup common_group
 * \param [in]     instance                  A pointer to the DWL instance.
 * \param [in]     core_id                   The core ID of the hardware resource.
 * \param [in]     offset                    The offset related to the hardware register base address, in bytes.
 * \param [in]     value                     The value to be written into the register for disabling the hardware.
 * \return         None
 */
void DWLDisableHw(const void *instance, i32 core_id, u32 offset, u32 value);

/**
 * Synchronizes the software with the hardware.
 * \ingroup common_group
 * \param [in]     instance                  A pointer to the DWL instance.
 * \param [in]     core_id                   The core ID of the hardware resource.
 * \param [in]     timeout                   The timeout period in milliseconds. \n
 *                                           Valid values: <tt>0</tt>: Do not wait. <tt>-1</tt>: Infinitely wait.
 * \return         <tt> \ref DWL_HW_WAIT_OK</tt>, <tt> \ref DWL_HW_WAIT_ERROR</tt>, or <tt> \ref DWL_HW_WAIT_TIMEOUT</tt>
 */
enum DWLRet DWLWaitHwReady(const void *instance, i32 core_id, u32 timeout);

/**
 * Waits for the specified decoder instance to complete all the decoding jobs in non-VCMD mode.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     owner                         A pointer to the decoder instance.
 * \return         None
 */
void DWLWaitOwnerDone(const void *instance, const void *owner);

/**
 * Releases hardware resources of a decoder core. \n
 * This function should be called after the hardware has finished decoding frames.
 * \ingroup common_group
 * \param [in]     instance                  A pointer to the DWL instance.
 * \param [in]     core_id                   The core ID of the hardware resources to be released.
 * \return         <tt> \ref DWL_OK</tt>
 */
enum DWLRet DWLReleaseHw(const void *instance, i32 core_id);

/**
 * Reserves a command buffer for the current decoding job.
 * \ingroup common_group
 * \param [in]     instance                       A pointer to the DWL instance.
 * \param [in]     info                           A pointer to the required information.
 * \param [in, out]    cmd_buf_id                 A pointer to the command buffer ID.
 * \return         <tt> \ref DWL_OK</tt> or <tt> \ref DWL_ERROR</tt>
 */
enum DWLRet DWLReserveCmdBuf(const void *instance, struct DWLReqInfo *info, u32 *cmd_buf_id);

/**
 * Flushes values of decoder registers to mirror registers of a DWL instance.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     cmd_buf_id                    The ID of the command buffer that stores register addresses and contents in the multi-core buffer.
 * \param [in]     dec_regs                      A pointer to the decoder registers, which are mirror registers in the DWL.
 * \param [in]     mc_fresh_regs                 A pointer to multi-core refresh registers. \n The registers transfer intermediate data of multi-core decoding.
 * \param [in]     mc_buf_id                     The ID of the multi-core buffer that stores intermediate data of multi-core decoding.
 * \return         <tt> \ref DWL_OK</tt> or <tt> \ref DWL_ERROR</tt>
 */
enum DWLRet DWLFlushRegister(const void *instance, u32 cmd_buf_id, u32 *dec_regs, u32 *mc_fresh_regs, u32 mc_buf_id);

/**
 * Fills commands into the command buffer of a specific ID.
 * \ingroup common_group
 * \param [in]     instance                       A pointer to the DWL instance.
 * \param [in]     cmd_buf_id                     The ID of the command buffer that stores commands.
 * \return         <tt> \ref DWL_OK</tt>
 */
enum DWLRet DWLPrepareCmds(const void *instance, u32 cmd_buf_id);

/**
 * Enables a command buffer and puts the buffer into a VCMD buffer queue.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     cmd_buf_id                    The ID of the command buffer.
 * \return         <tt> \ref DWL_OK</tt> or <tt> \ref DWL_ERROR</tt>
 */
enum DWLRet DWLEnableCmdBuf(const void *instance, u32 cmd_buf_id);

/**
 * Waits for the specified command buffer to become ready.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     cmd_buf_id                    The ID of the command buffer.
 * \return         <tt> \ref DWL_OK</tt>: The command buffer is ready and the wait ends. \n
 * <tt> \ref DWL_ERROR</tt>: An error occurs in the wait period. \n
 * <tt> \ref DWL_HW_WAIT_ERROR</tt>: An error occurs in the waiting period and the error is caused by the Linux kernel API. \n
 * <tt> \ref DWL_SLICE_DECODING_SUSPEND</tt>: The command buffer processing hangs and the wait persists. \n
 * <tt> \ref DWL_SLICE_DECODING_ABORTED</tt>: The command buffer processing is aborted and the wait ends.
 */
enum DWLRet DWLWaitCmdBufReady(const void *instance, u16 cmd_buf_id);

/**
 * Refreshes registers in VCMD mode. \n
 * This function is for internal reference only.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     cmd_buf_id                    The ID of the command buffer.
 * \param [in]     dec_regs                      A pointer to decoder registers.
 * \return         <tt> \ref DWL_OK</tt> or <tt> \ref DWL_ERROR</tt>
 */
enum DWLRet DWLRefreshRegister(const void *instance, u32 cmd_buf_id, u32 *dec_regs);

/**
 * Releases a command buffer for a DWL instance.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     cmd_buf_id                    The ID of the command buffer.
 * \return         <tt> \ref DWL_OK</tt> or <tt> \ref DWL_ERROR</tt>
 */
enum DWLRet DWLReleaseCmdBuf(const void *instance, u32 cmd_buf_id);

/**
 * Waits for all remaining command buffers of a decoder instance to be processed for a DWL instance. \n
 * The wait persists until all command buffers are processed.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     owner                         A pointer to the decoder instance.
 * \return        <tt> \ref DWL_OK</tt>
 */
enum DWLRet DWLWaitCmdbufsDone(const void *instance, const void *owner);

/**
 * Queries the number of ASIC cores with the VCMD enabled.
 * \ingroup common_group
 * \param [in]     instance                    A pointer to the DWL instance.
 * \return         The number of ASIC cores with the VCMD enabled.
 */
u32 DWLVcmdIsUsed(const void *instance);

/**
 * Gets the hardware version of the VCMD.
 * \ingroup common_group
 * \param [in]     instance       A pointer to the DWL instance.
 * \return         The hardware version of the VCMD.
 */
u32 DWLReadVcmdHwID(const void *instance);

/**
 * Gets the execution status of a command buffer. \n
 * This API is for internal use.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     cmd_buf_id                    The ID of the command buffer.
 * \return         The execution status of the command buffer is returned to the kernel layer. \n
 * For details, see <tt>hantrovcmd.h</tt>.
 */
u32 DWLGetCmdBufExeStatus(const void *instance, u16 cmd_buf_id);

/**
 * Parses the execution status of a command buffer into a job status maintained by DWL in VCMD mode. \n
 * This function must be called after the <tt>DWLGetCmdBufExeStatus()</tt> function.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     exe_status                    The execution status of a command buffer in the kernel layer.
 * \return         <tt> \ref DWL_OK</tt>: The execution status is parsed into a job status. \n
 *  <tt> \ref DWL_SLICE_DECODING_SUSPEND</tt>: The parse operation hangs. \n
 *  <tt> \ref DWL_SLICE_DECODING_ABORTED</tt>: The parse operation is aborted.
 */
enum DWLRet DWLParseCmdBufExeStatus(const void *instance, u32 exe_status);

/**
 * Configures an M2M command.
 * \ingroup common_group
 * \param [in]     instance                    A pointer to the DWL instance.
 * \param [in]     head_data                   The information about the head data.
 * \param [in]     tail_data                   The information about the tail data.
 * \param [in]     cmdbuf_id                   The ID of the command buffer.
 * \return         None
 */
void DWLCmdM2MSendData(const void *instance, struct CDF_INFO *head_data, struct CDF_INFO *tail_data, u32 cmdbuf_id);

/**
 * Checks whether M2M is supported in VCMD mode.
 * \ingroup common_group
 * \param [in]     instance                    A pointer to the DWL instance.
 * \return         <tt>1</tt>: M2M is supported. \n
 *                 <tt>0</tt>: M2M is not supported.
 */
u32 DWLCheckVcmdM2M(const void *instance);

/**
 * Configures an MSET command.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     before_data                   The information about the head data.
 * \param [in]     after_data                    The information about the tail data.
 * \param [in]     cmdbuf_id                     The ID of the command buffer.
 * \return         None
 */
void DWLCmdMsetSendData(const void *instance, struct MSET_INFO *before_data, struct MSET_INFO *after_data, u32 cmdbuf_id);

/**
 * Configures an M2MP command.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     before_data                   The information about the head data.
 * \param [in]     after_data                    The information about the tail data.
 * \param [in]     cmdbuf_id                     The ID of the command buffer.
 * \return         None
 */
void DWLCmdM2MPSendData(const void *instance, struct M2MP_INFO *before_data, struct M2MP_INFO *after_data, u32 cmdbuf_id);

/**
 * Refreshes the checksum result from the status buffer.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     crc                           A pointer to the checksum result.
 * \param [in]     cmdbuf_id                     The ID of the command buffer.
 * \return         <tt> \ref DWL_OK</tt> or <tt> \ref DWL_ERROR</tt>
 */
enum DWLRet DWLRefreshCrcResult(const void *instance, CrcData **crc, u32 cmdbuf_id);

/**
 * Configures the registers required for slice decoding when the VCMD is enabled.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     cmd_buf_id                    The ID of the command buffer.
 * \param [in]     regs                          A pointer to the required registers.
 * \return         <tt> \ref DWL_OK</tt> or <tt> \ref DWL_ERROR</tt>
 */
enum DWLRet DWLCmdPushSliceRegs(const void *instance, u32 cmd_buf_id, u32 *regs);

/**
 * Aborts the decoder corresponding to the specified command buffer when the slice decoding job is waiting for more slices.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     cmd_buf_id                    The ID of the command buffer.
 * \return         None
 */
void DWLAbortCmdbuf(const void *instance, u16 cmd_buf_id);

/**
 * Drops decoding jobs and releases command buffers of a decoder owner.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     owner                         A pointer to the owner, which is a decoder instance.
 * \return         None
 */
void DWLDropCmdbufs(const void *instance, const void *owner);

/**
 * Selects the interrupt mode for generating JMP commands to be stored in the command buffer of a specific ID.
 * \ingroup common_group
 * \param [in]     instance                       A pointer to the DWL instance.
 * \param [in]     cmd_buf_id                     The ID of the command buffer that stores JMP commands.
 * \param [in]     interrupt_ctrl                 The bit mask that determines the interrupt mode, which can be adaptive or manual. \n
 *                                                For details about this parameter, see <tt>dwl_linux.h</tt>.
 * \return         None
 */
void DWLSetCmdBufIntCtrl(const void *instance, u16 cmd_buf_id, u64 interrupt_ctrl);

/** \brief The prototype of the callback function for multi-core decoding. This callback function
 *  is called by the listener thread to notify the completion of a job.
 *  \n <tt>arg</tt>: A pointer to the decoder instance.
 *  \n <tt>core_id</tt>: The job ID, which is the core ID in non-VCMD mode and the command buffer ID in VCMD mode.
 *  \ingroup common_group */
typedef void DWLIRQCallbackFn(void *arg, i32 core_id);

/**
 * Sets the IRQ callback function when multi-core decoding is enabled.
 * \ingroup common_group
 * \param [in]     instance                  A pointer to the DWL instance.
 * \param [in]     core_id                   The core ID of the hardware resource.
 * \param [in]     callback_fn               A pointer to the callback function.
 * \param [in]     arg                       The argument set for the callback function.
 * \return         None
 */
void DWLSetIRQCallback(const void *instance, i32 core_id,
                       DWLIRQCallbackFn *callback_fn, void *arg);

/**
 * Gets the ID of the multi-core buffer of the VCMD.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     cmdbuf_id                     The ID of the command buffer corrsponding to the multi-core buffer.
 * \return         The ID of the multi-core buffer.
 */
i32 DWLGetVcmdMCVirtualCoreId(const void *instance, u32 cmdbuf_id);

/**
 * Refreshes the checksum result from the status buffer when multi-core decoding is enabled.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     crc                           A pointer to the checksum result.
 * \param [in]     cmdbuf_id                     The ID of the command buffer.
 * \return         <tt> \ref DWL_OK</tt> or <tt> \ref DWL_ERROR</tt>
 */
enum DWLRet DWLMCRefreshCrcResult(const void *instance, CrcData **crc, u32 cmdbuf_id);

/**
 * Refreshes the status registers when the return value of <tt>DWLReadAsicCoreCount()</tt> is more than 2.
 * \ingroup common_group
 * \param [in]     instance                      A pointer to the DWL instance.
 * \param [in]     dec_regs                      A pointer to decoder registers.
 * \param [in]     cmdbuf_id                     The ID of the command buffer.
 * \return         <tt> \ref DWL_OK</tt> or <tt> \ref DWL_ERROR</tt>
 */
enum DWLRet DWLVcmdMCRefreshStatusRegs(const void *instance, u32 *dec_regs, u32 cmdbuf_id);

/**
 * Reads the hardware ID of the first core that supports the specified client type within a DWL instance.
 * \ingroup common_group
 * \param [in]     instance       A pointer to the DWL instance.
 * \param [in]     client_type    The client type.
 * \return         The hardware ID of the first core that supports the client type.
 */
u32 DWLReadAsicID(const void *instance, enum DWLClientType client_type);

/**
 * Reads the build ID of the specified core.
 * \ingroup common_group
 * \param [in]     instance       A pointer to the DWL instance.
 * \param [in]     core_id        The core ID.
 * \return         The build ID.
 */
u32 DWLReadCoreHwBuildID(const void *instance, u32 core_id);

/**
 * Gets the hardware feature list of a core by the specified core ID.
 * \ingroup common_group
 * \param [in]     instance            A pointer to the DWL instance.
 * \param [in]     core_id             The core ID.
 * \return         The hardware feature list.
 */
const void *DWLGetHwFeaturesByID(const void *instance, u32 core_id);

/**
 * Gets the hardware feature list of the first core that supports the specified client type.
 * \ingroup common_group
 * \param [in]     instance            A pointer to the DWL instance.
 * \param [in]     client_type         The client type.
 * \param [in]     core_mask           A pointer to the core mask, which specifies a scope of cores. \n
 *                                     Bit <tt>n</tt> corresponds to Core <tt>n</tt> (<tt>n</tt> = 0 to 31).
 * \return         The hardware feature list.
 */
const void *DWLGetHwFeaturesByClientType(const void *instance, enum DWLClientType client_type, u32 *core_mask);

/**
 * Gets the number of ASIC cores, including the video decoding core and sub-IPs.
 * \ingroup common_group
 * \param [in]     instance            A pointer to the DWL instance.
 * \return         The number of ASIC cores.
 */
u32 DWLReadAsicCoreCount(const void *instance);

#ifdef FPGA_PERF_AND_BW
u32 DWLReadBw(const void *instance, u32 core_id, u32 num);
void DWLPerfInfoCollect(const void *instance, i32 core_id, struct DWLPerfInfo *perf_info);
#endif

#ifdef ASIC_TRACE_SUPPORT
void DWLSetSimMc(const void *instance);
/* For hw simulation purpose, HW need use DWLMallocRefFrm to generate trace files */
#define DWLMALLOC_LINEAR_MEM2 DWLMallocRefFrm
#define DWLFREE_LINEAR_MEM2 DWLFreeRefFrm
#else
#define DWLMALLOC_LINEAR_MEM2 DWLMallocLinear
#define DWLFREE_LINEAR_MEM2 DWLFreeLinear
#endif

#ifdef MODEL_SIMULATION
void DWLSetCoreCount(u32 core_count);
#endif
#ifdef __cplusplus
}
#endif

#endif /* __DWL_H__ */
