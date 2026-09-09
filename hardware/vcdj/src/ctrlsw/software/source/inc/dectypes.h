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

#ifndef DECTYPES_H
#define DECTYPES_H

#include "basetype.h"
#include "dwl.h"
#include "decapicommon.h"
#include "decsei.h"

/** \brief The common z-score normalization algorithm.
 *  \ingroup common_group */
#define ZSCORE 0
/** \brief The z-score normalization with a scaling factor.
 *  \ingroup common_group */
#define VSI_ZSCORE 1

/** \brief The output data format using normalized 32-bit floating-point precision.
 *  \ingroup common_group */
#define NORM_OUT_FP32 0
/** \brief The output data format using normalized 16-bit floating-point precision.
 *  \ingroup common_group */
#define NORM_OUT_FP16 1
/** \brief The output data format using normalized 16-bit signed integer precision.
 *  \ingroup common_group */
#define NORM_OUT_INT16 2
/** \brief The output data format using normalized 8-bit signed integer precision.
 *  \ingroup common_group */
#define NORM_OUT_INT8 3
/** \brief The output data format using normalized 8-bit unsined integer precision.
 *  \ingroup common_group */
#define NORM_OUT_UINT8 4

#define HIST_LOCAL 0
#define HIST_GLOBAL 1
#define HIST_HSV 2

/** Defines decoder types.
 *  \ingroup common_group */
enum DecCodec {
  /** \brief MPEG-4 decoder. */
  DEC_MPEG4,
  /** \brief MPEG-2 decoder. */
  DEC_MPEG2,
  /** \brief VP6 decoder. */
  DEC_VP6,
  /** \brief VP8 decoder. */
  DEC_VP8,
  /** \brief VP9 decoder. */
  DEC_VP9,
  /** \brief HEVC (H.265) decoder. */
  DEC_HEVC,
  /** \brief H.264 (AVC) decoder. */
  DEC_H264,
  /** \brief AVS decoder. */
  DEC_AVS,
  /** \brief AVS2 decoder. */
  DEC_AVS2,
  /** \brief AV1 decoder. */
  DEC_AV1,
  /** \brief JPEG decoder. */
  DEC_JPEG,
  /** \brief VC1 decoder. */
  DEC_VC1,
  /** \brief RV decoder. */
  DEC_RV,
  /** \brief VVC decoder. */
  DEC_VVC,
  /** \brief AVS3 decoder */
  DEC_AVS3,
  /** \brief PNG decoder */
  DEC_PNG,
  /** \brief Decoder that serves as a standalone post-processing core. */
  DEC_PPDEC,
  /** \brief The maximum number of supported codec standards.*/
  DEC_FMT_MAX
};

/** Defines the return values for the decoder API functions.
 *  \ingroup common_group */
enum DecRet {
  /** \brief The API call is successful. */
  DEC_OK = 0, /**<\hideinitializer */
  /** \brief Processing of the stream is finished. */
  DEC_STRM_PROCESSED = 1, /**<\hideinitializer */
  /** \brief The picture is ready for output. */
  DEC_PIC_RDY = 2, /**<\hideinitializer */
  /** \brief The picture is decoded. */
  DEC_PIC_DECODED = 3, /**<\hideinitializer */
  /** \brief New stream headers are decoded. */
  DEC_HDRS_RDY = 4, /**<\hideinitializer */
  /** \brief (For MPEG-4 only) New stream headers are decoded for data-partitioned
   *  streams. */
  DEC_DP_HDRS_RDY = 5, /**<\hideinitializer */
  /** \brief Advanced coding tools such as ASO and FMO are detected in the stream. */
  DEC_ADVANCED_TOOLS = 6, /**<\hideinitializer */
  /** \brief A slice is decoded. */
  DEC_SLICE_RDY = 7, /**<\hideinitializer */
  /** \brief Output pictures must be retrieved before decoding continues. */
  DEC_PENDING_FLUSH = 8, /**<\hideinitializer */
  /** \brief Non-reference pictures are skipped. */
  DEC_NONREF_PIC_SKIPPED = 9, /**<\hideinitializer */
  /** \brief The decoder detects the end-of-stream state. */
  DEC_END_OF_STREAM = 10,         /**<\hideinitializer */
  /** \brief The end of the video sequence is reached. */
  DEC_END_OF_SEQ = 11,         /**<\hideinitializer */
  /** \brief The decoder is waiting for external buffer allocation. */
  DEC_WAITING_FOR_BUFFER = 12,    /**<\hideinitializer */
  /** \brief (For PJPEG only) The scan is processed. */
  DEC_SCAN_PROCESSED = 13,     /**<\hideinitializer */
  /** \brief The decoder is aborted. */
  DEC_ABORTED = 14,              /**<\hideinitializer */
  /** \brief All pictures have been flushed. */
  DEC_FLUSHED = 15,              /**<\hideinitializer */
  /** \brief The input buffer has been consumed, but the decoding is not completed. */
  DEC_BUF_EMPTY = 16,              /**<\hideinitializer */
  /** \brief The resolution is changed in the multi-resolution video. */
  DEC_RESOLUTION_CHANGED = 17,    /**<\hideinitializer */
  /** \brief The end marker of the video object sequence is dedected. */
  DEC_VOS_END = 18,    /**<\hideinitializer */
  /** \brief The VPS, PPS, or SPS is parsed. */
  DEC_PARAM_SET_PARSED = 19,
  /** \brief The SEI is parsed. */
  DEC_SEI_PARSED = 20,
  /** \brief Decoding of P- and B-pictures is skipped. */
  DEC_PB_PIC_SKIPPED = 21,
  /** \brief The output picture is discarded. */
  DEC_DISCARD_INTERNAL = 22,
  /** Slice decoding is interrupted due to system suspend or system power off. */
  DEC_SLICE_DECODING_SUSPEND = 23,   /**<\hideinitializer */

  /** \brief Invalid parameters are used. */
  DEC_PARAM_ERROR = -1,          /**<\hideinitializer */
  /** \brief An unrecoverable error occurs in decoding. */
  DEC_STRM_ERROR = -2,           /**<\hideinitializer */
  /** \brief The decoder is not initialized. */
  DEC_NOT_INITIALIZED = -3,      /**<\hideinitializer */
  /** \brief Memory allocation fails. */
  DEC_MEMFAIL = -4,              /**<\hideinitializer */
  /** \brief Initialization fails. */
  DEC_INITFAIL = -5,             /**<\hideinitializer */
  /** \brief Video sequence information is unavailable because stream headers are not
   *  decoded. */
  DEC_HDRS_NOT_RDY = -6,         /**<\hideinitializer */
  /** \brief The video sequence frame size or tools are not supported. */
  DEC_STREAM_NOT_SUPPORTED = -8, /**<\hideinitializer */
  /** \brief The decoder rejects an externally allocated buffer because the buffer has exceeded implementation limits. */
  DEC_EXT_BUFFER_REJECTED = -9,    /**<\hideinitializer */
  /** \brief Invalid post-processing parameters are used. */
  DEC_INFOPARAM_ERROR = -10,    /**<\hideinitializer */
  /** \brief An error occurs. */
  DEC_ERROR = -11,              /**<\hideinitializer */
  /** \brief Unsupported. */
  DEC_UNSUPPORTED = -12,        /**<\hideinitializer */
  /** \brief The stream length is invalid. */
  DEC_INVALID_STREAM_LENGTH = -13, /**<\hideinitializer */
  /** \brief The input buffer size is invalid. */
  DEC_INVALID_INPUT_BUFFER_SIZE = -14, /**<\hideinitializer */
  /** \brief Input buffer size is insufficient. Please allocate a larger input buffer. */
  DEC_INCREASE_INPUT_BUFFER = -15, /**<\hideinitializer */
  /** \brief The slice mode is not supported for this operation type. */
  DEC_SLICE_MODE_UNSUPPORTED = -16, /**<\hideinitializer */
  /** \brief The metadata is in improper format. */
  DEC_METADATA_FAIL = -17,     /**<\hideinitializer */
  /** \brief The current picture has no reference pictures, and decoding is skipped. */
  DEC_NO_REFERENCE = -18,
  /** \brief An error occurs during the parsing of VPS, SPS or PPS. */
  DEC_PARAM_SET_ERROR = -19,
  /** \brief An error occurs when SEI is being extracted or parsed. */
  DEC_SEI_ERROR = -20,
  /** \brief An error occurs when the slice header is being parsed. */
  DEC_SLICE_HDR_ERROR = -21,
  /** \brief An error occurs when the reference picture for the current picture is being set. */
  DEC_REF_PICS_ERROR = -22,
  /** \brief An error occurs when NAL is being extracted or parsed. */
  DEC_NALUNIT_ERROR = -23,
  /** \brief An error occurs during the checking of AU boundaries. */
  DEC_AU_BOUNDARY_ERROR = -24,
  /** \brief An error occurs in syntax elements of the stream. */
  DEC_STREAM_ERROR_DEDECTED = -25,  /**<\hideinitializer */
  /** \brief The decoder has no available buffer. */
  DEC_NO_DECODING_BUFFER = -99,  /**<\hideinitializer */
  /** \brief The driver cannot reserve decoder hardware. */
  DEC_HW_RESERVED = -254,        /**<\hideinitializer */
  /** \brief The decoder hardware times out. */
  DEC_HW_TIMEOUT = -255,         /**<\hideinitializer */
  /** \brief The decoder hardware receives the error status from the system bus. */
  DEC_HW_BUS_ERROR = -256,       /**<\hideinitializer */
  /** \brief An unrecoverable system error occurs in the decoder hardware. */
  DEC_SYSTEM_ERROR = -257,       /**<\hideinitializer */
  /** \brief An error occurs in the decoder wrapper layer (DWL). */
  DEC_DWL_ERROR = -258,          /**<\hideinitializer */
  /** \brief A fatal system error occurs in the hardware. */
  DEC_FATAL_SYSTEM_ERROR = -259,       /**<\hideinitializer */
  /** \brief An external timeout occurs. */
  DEC_HW_EXT_TIMEOUT = -260,         /**<\hideinitializer */
  /** \brief The evaluation limit is exceeded. */
  DEC_EVALUATION_LIMIT_EXCEEDED = -999, /**<\hideinitializer */
  /** \brief The video codec standard is not supported. */
  DEC_FORMAT_NOT_SUPPORTED = -1000 /**<\hideinitializer */
    /* TODO(vmr): Prune what is not needed from these. */
};

/** (For AVS and MPEG-2 only) Defines display aspect ratios (DARs).
 *  \ingroup common_group */
enum DecDARFormat{
  /** \brief DAR 1 : 1. */
  DEC_1_1 = 0x01, /**<\hideinitializer */
  /** \brief DAR 4 : 3. */
  DEC_4_3 = 0x02, /**<\hideinitializer */
  /** \brief DAR 16 : 9. */
  DEC_16_9 = 0x03, /**<\hideinitializer */
  /** \brief DAR 2.21 : 1 */
  DEC_2_21_1 = 0x04 /**<\hideinitializer */
};

/** (For VP8 and MPEG-4 only) Defines input sequence formats.
 *  \ingroup common_group */
enum DecInputFormat {
  /** \brief The VP7 format. */
  DEC_INPUT_VP7 = 0x01, /**<\hideinitializer */
  /** \brief The VP8 format. */
  DEC_INPUT_VP8, /**<\hideinitializer */
  /** \brief The WebP format. */
  DEC_INPUT_WEBP, /**<\hideinitializer */
  /** \brief The MPEG-4 format. */
  DEC_INPUT_MPEG4, /**<\hideinitializer */
  /** \brief The Sorenson format. */
  DEC_INPUT_SORENSON, /**<\hideinitializer */
  /** \brief A custom format. */
  DEC_INPUT_CUSTOM_1 /**<\hideinitializer */
};

/** Defines frame skipping modes for non-reference frames.
 *  \ingroup common_group */
enum DecSkipFrameMode {
  /** \brief (Default) Decodes all frames and does not output reconstructed data of
   *  non-reference frames.
   *  \n This value is available only for H.264 (AVC) of the High 10 profile and for
   *  HEVC (H.265). */
  DEC_SKIP_NON_REF_RECON = 0, /**<\hideinitializer */
  /** \brief Does not decode non-reference frames. */
  DEC_SKIP_NON_REF = 1, /**<\hideinitializer */
  /** \brief Does not skip non-reference frames during decoding. */
  DEC_SKIP_NONE /**<\hideinitializer */
};

/** Defines YUV sample ranges for decoded pictures.
 *
 *  <b>NOTE:</b> <tt>DEC_VIDEO_RANGE_NORMAL</tt> is equivalent to <tt>DEC_VIDEO_STUDIO_SWING</tt>.
 *  <tt>DEC_VIDEO_RANGE_FULL</tt> is equivalent to <tt>DEC_VIDEO_FULL_SWING</tt>.
 *  \ingroup common_group */
enum DecVideoRange {
  /** \brief Limited ranges, where Y samples are in range [16, 235]. */
  DEC_VIDEO_RANGE_NORMAL = 0x0, /**<\hideinitializer */
  /** \brief Full ranges, where Y samples are in range [0, 255]. */
  DEC_VIDEO_RANGE_FULL = 0x1, /**<\hideinitializer */
  /** \brief Limited ranges, where Y samples are in range [16, 235]. */
  DEC_VIDEO_STUDIO_SWING = 0x0, /**<\hideinitializer */
  /** \brief Full ranges, where Y samples are in range [0, 255]. */
  DEC_VIDEO_FULL_SWING = 0x1 /**<\hideinitializer */
};

/** Defines color spaces for decoded pictures.
 *  \ingroup common_group */
enum DecColorSpace {
  /** \brief The YUV color space. */
  DEC_YCbCr_BT601 = 0x0, /**<\hideinitializer */
  /** \brief A custom color space. */
  DEC_CUSTOM = 0x1, /**<\hideinitializer */
  /** \brief The RGB color space. */
  DEC_RGB = 0x7 /**<\hideinitializer */
};

/** Defines buffer types.
 *  \ingroup common_group */
enum DecBufferType {
  /** \brief Reference frame buffer, which can be either a reference frame compression (RFC) table buffer,
   *  or a direct motion vector (DMV) buffer. */
  REFERENCE_BUFFER = 0, /**<\hideinitializer */
  /** \brief Downscaled output buffer. */
  DOWNSCALE_OUT_BUFFER, /**<\hideinitializer */
  BUFFER_TYPE_NUM /**<\hideinitializer */
};

/** (For MPEG-4 only) Defines user data types.
 *  \ingroup common_group */
enum DecUserDataType {
  /** \brief The VOS type. */
  DEC_USER_DATA_VOS = 0, /**<\hideinitializer */
  /** \brief The VISO type. */
  DEC_USER_DATA_VISO, /**<\hideinitializer */
  /** \brief The VOL type. */
  DEC_USER_DATA_VOL, /**<\hideinitializer */
  /** \brief The GOV type. */
  DEC_USER_DATA_GOV /**<\hideinitializer */
};

/** \brief Contains some of the \c tb_cfg.dec_params parameters.
 *  \ingroup common_group */
struct DecParams {
  /** \brief The hardware burst length. It is suggested that you use the defult value.*/
  u32 bus_burst_length;
  /** \brief Whether to disable decoder clocks during idle periods for disabled codecs. */
  u32 clk_gate_decoder;
  /** \brief The memory access wait states for reference buffer operation during non-sequential clock domains. */
  u32 non_seq_clk;
  /** \brief The memory access wait states for reference buffer operation during sequential clock domains. */
  u32 seq_clk;
  /** \brief The APF threshold value. */
  i32 apf_threshold_value;
  /** \brief The maximum number of outstanding AXI write requests the decoder can issue. */
  u32 axi_wr_outstand;
  /** \brief The maximum number of outstanding AXI read requests the decoder can issue.*/
  u32 axi_rd_outstand;
};

/** \brief Contains some of the \c tb_cfg.tb_params parameters for random errors.
 *  \ingroup common_group */
struct ErrorParams {
  /** \brief The random seed. */
  u32 seed;
  /** \brief Whether to truncate the stream by randomized stream length. */
  u8 truncate_stream;
  /** \brief Whether to control bitstream truncation. */
  char truncate_stream_odds[24];
  /** \brief Whether to randomly swap bits in the bit stream. */
  u8 swap_bits_in_stream;
  /** \brief Whether to swap the odd bits. */
  char swap_bit_odds[24];
  /** \brief Whether to randomly discard packets. */
  u8 lose_packets;
  /** \brief Whether to discard odd-positioned bits. */
  char packet_loss_odds[24];
  /** \brief Whether to enable the random error function.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 random_error_enabled;
};

/** \brief (For MPEG-4 only) Contains user data configurations.
 *  \ingroup common_group */
struct DecUserConf {
  /** \brief The type of buffer allocated. */
  enum DecUserDataType user_data_type;
  /** \brief A pointer to the VOS data.
   *  \n This field is valid only if \c DecUserConf.user_data_type is set to
   *  <tt> \ref DEC_USER_DATA_VOS</tt>. */
  u8  *p_user_data_vos;
  /** \brief The maximum length of the VOS data.
   *  \n This field is valid only if \c DecUserConf.user_data_type is set to
   *  <tt> \ref DEC_USER_DATA_VOS</tt>. */
  u32  user_data_vosmax_len;
  /** \brief A pointer to the VISO data.
   *  \n This field is valid only if \c DecUserConf.user_data_type is set to
   *  <tt> \ref DEC_USER_DATA_VISO</tt>. */
  u8  *p_user_data_viso;
  /** \brief The maximum length of the VISO data.
   *  \n This field is valid only if \c DecUserConf.user_data_type is set to
   *  <tt> \ref DEC_USER_DATA_VISO</tt>. */
  u32  user_data_visomax_len;
  /** \brief The maximum length of the VOL data.
   *  \n This field is valid only if \c DecUserConf.user_data_type is set to
   *  <tt> \ref DEC_USER_DATA_VOL</tt>. */
  u8  *p_user_data_vol;
  /** \brief The maximum length of the VOL data.
   *  \n This field is valid only if \c DecUserConf.user_data_type is set to
   *  <tt> \ref DEC_USER_DATA_VOL</tt>. */
  u32  user_data_volmax_len;
  /** \brief The maximum length of the GOV data.
   *  \n This field is valid only if \c DecUserConf.user_data_type is set to
   *  <tt> \ref DEC_USER_DATA_VISO</tt>. */
  u8  *p_user_data_gov;
  /** \brief The maximum length of the GOV data.
   *  \n This field is valid only if \c DecUserConf.user_data_type is set to
   *  <tt> \ref DEC_USER_DATA_GOV</tt>. */
  u32  user_data_govmax_len;
};

/** \brief Defines a cropping region for output.
 *  \ingroup common_group */
struct DecCropParams {
  /** \brief The left offset in samples. */
  u32 crop_left_offset;
  /** \brief The width of the cropping region in samples. */
  u32 crop_out_width;
  /** \brief The top offset in samples. */
  u32 crop_top_offset;
  /** \brief The height of the cropping region in samples. */
  u32 crop_out_height;
};

/** \brief (For VC-1 only) Contains the metadata of a stream.
 *  \ingroup common_group */
struct DecMetaData {
  /** \brief The maximum width supported by the decoder for a video sequence.
   *  \n Valid values: Even integers in range [2, 8192].
   *  \n Unit: Pixels. */
  u32     max_coded_width;
  /** \brief The maximum height supported by the decoder for a video sequence.
   *  \n Valid values: Even integers in range [2, 8192].
   *  \n Unit: Pixels. */
  u32     max_coded_height;
  /** \brief Whether to enable variable-size transform for the sequence.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32     vs_transform;
  /** \brief Whether to enable overlap smoothing for the sequence.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32     overlap;
  /** \brief Whether the stream contains syncronization markers.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Does not contain.
   *  \n - <tt>1</tt>: Contains. */
  u32     sync_marker;
  /** \brief The quantizer type used for the sequence.
   *  \n Valid range: [0, 3]. */
  u32     quantizer;
  /** \brief Whether the INTERPFRM flag exists in the picture headers.
   *  \n The flag indicates that the frame should be interpolated during display.
   *  \n - <tt>0</tt>: Does not exist.
   *  \n - <tt>1</tt>: Exists. */
  u32     frame_interp;
  /** \brief The maximum number of consecutive B-frames in the sequence.
   *  \n Valid range: [0, 7].*/
  u32     max_bframes;
  /** \brief Whether to enable the rounding of color difference motion vectors.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32     fast_uv_mc;
  /** \brief Whether to enable extended motion vectors for the sequence.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32     extended_mv;
  /** \brief Whether frames can be coded at smaller resolutions than the specified frame
   *  resolution.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Cannot be coded at smaller resolutions.
   *  \n - <tt>1</tt>: Can be coded at smaller resolutions. */
  u32     multi_res;
  /** \brief Whether range reduction is used in the sequence.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Not used.
   *  \n - <tt>1</tt>: Used. */
  u32     range_red;
  /** \brief Whether the quantization step may vary within a frame.
   *  \n Valid range: [0, 2]. */
  u32     dquant;
  /** \brief Whether to enable loop filtering for the sequence.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32     loop_filter;
  /** \brief The profile of the input video bitstream. */
  enum VC1Profile profile;
};

/** \brief (For AVS/MPEG-2/MPEG-4 only) The time information.
 *  \ingroup common_group */
struct DecTime {
  /** \brief The hours information. */
  u32 hours;
  /** \brief The minutes information. */
  u32 minutes;
  /** \brief The seconds information. */
  u32 seconds;
  /** \brief The pictures information. */
  u32 pictures;
  /** \brief (For MPEG-4 only) The VOP time increment. */
  u32 time_incr;
  /** \brief (For MPEG-4 only) The VOP time increment resolution. */
  u32 time_res;
};

#pragma pack(push, 1)
struct SEI_header {
  /* TODO(JZQ) : need fix, it has to be a compact struct */
  /** \brief The SEI type */
  u8 type;
  /** \brief The SEI data size */
  u16 size;
  /** \brief The SEI status */
  u8 status;
};
#pragma pack()

/** \brief Contains captured SEI stream data.
 *  \ingroup common_group */
struct SEI_buffer {
  /** \brief The 256-bit mask that indicates the concerned SEI. */
  u8 bitmask[32];
  /** \brief A pointer to the SEI buffer. */
  u8 *buffer;
  /** \brief The size of the buffer. */
  u32 total_size;
  /** \brief The size of the available data in the SEI buffer. */
  u32 available_size;
};

/** \brief (For RV only) Contains the slice information.
 *  \ingroup common_group */
struct DecSliceInfo {
  /** \brief The offset of each slice in the data buffer, including the start point 0 and the end
   *  point <tt>DecInput.data_len</tt>. */
  u32 offset;
  /** \brief Whether slice information is available.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Unavailable.
   *  \n - <tt>1</tt>: Available. */
  u32 is_valid;
};

/** \brief Contains the information about the input to the decoder.
 *  \ingroup common_group */
struct DecInputParameters {
  /** \brief The ID of the decoded picture. */
  u64 pic_id;
  /** \brief A pointer to the user data. \n This field is a parameter of <tt>DecMCStreamConsumed()</tt> when multi-core decoding is enabled. */
  void* p_user_data;
  /** \brief The virtual address of the input stream. */
  u8* stream;
  /** \brief The length of the input stream. */
  u32 strm_len;
  /** \brief The bus address of the input stream. */
  addr_t stream_bus_address;
  /** \brief The frame skipping mode. */
  enum DecSkipFrameMode skip_frame;
  /** \brief The input stream buffer. */
  struct DWLLinearMem stream_buffer;
  /** \brief (For HEVC and H.264 only) A pointer to the SEI buffer. */
  struct SEI_buffer *sei_buffer;
  /** \brief The base address of the slice text buffer. */
  struct DecSliceTextInfo slice_text;
  /* low_latency mode used */
  /** \brief Whether to enable low-latency decoding.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 low_latency;
  /** \brief Whether the picture is decoded in low-latency mode.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Not decoded in low-latency mode.
   *  \n - <tt>1</tt>: Decoded in low-latency mode. */
  u32 pic_decoded;
  /** \brief Whether the decoder exits the data sending thread.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Not exit.
   *  \n - <tt>1</tt>: Exit. */
  u32 exit_send_thread;
  /** \brief The data length of each frame in low-latency mode. */
  u32 frame_len;
  /** \brief The virtual address of the input stream. */
  u8* strm_vir_start_addr;

  /* MPEG4 specific */
  /** \brief (For MPEG-4 only) Whether to enable deblocking for post-processed pictures.
   *  \n This field has no effect on the decoding process. It is only valid if the decoder is
   *  pipelined with post-processing channels. */
  u32 enable_deblock;

  /* VP8 specific */
  /** \brief (For VP8 only) The height of each VP8 WebP slice. */
  u32 slice_height;
  /** \brief (For VP8 only) The address of the user-allocated buffer to which luma data is output.
   *  \n This field is used in conjunction with external buffer allocation. */
  u32 *p_pic_buffer_y;
  /** \brief (For VP8 only) The bus address of the luma output buffer. */
  addr_t pic_buffer_bus_address_y;
  /** \brief (For VP8 only) The address of the user-allocated buffer to which chroma data is
   *  output.
   *  \n This field is used in conjunction with external buffer allocation. */
  u32 *p_pic_buffer_c;
  /** \brief (For VP8 only) The bus address of the chroma output buffer. */
  addr_t pic_buffer_bus_address_c;

  /* JPEG specific */
  /** \brief (For JPEG only) The decoded picture type.
   *  \n Valid values:
   *  \n - <tt>JPEGDEC_IMAGE</tt>: full picture.
   *  \n - <tt>JPEGDEC_THUMBNAIL</tt>: thumbnail picture. */
  u32 dec_image_type;
  /** \brief (For JPEG only) The number of MCU rows to be decoded in slice mode. */
  u32 slice_mb_set;
  /** \brief (For JPEG only) The number of restart intervals in the input stream. */
  u32 ri_count;
  /** \brief (For JPEG only) The offset of the beginning of each restart interval. */
  u32 *ri_array;
  /** \brief (For JPEG only) The input stream buffer size in input stream buffering mode. */
  u32 buffer_size;
  /** \brief (For JPEG only) The address for storing decoded Y component.
   *  \n Make sure that this field is set to a valid, pre-allocated buffer address if you expect data to be output
   *  to your own memory buffers. */
  struct DWLLinearMem picture_buffer_y;
  /** \brief (For JPEG only) The address for storing decoded UV or U component.
   *  \n Make sure that this field is set to a valid, pre-allocated buffer address if you expect data to be output
   *  to your own memory buffers. */
  struct DWLLinearMem picture_buffer_cb_cr;
  /** \brief (For JPEG only) The address for storing decoded V component.
   *  \n Make sure that this field is set to a valid, pre-allocated buffer address if you expect data to be output
   *  to your own memory buffers. */
  struct DWLLinearMem picture_buffer_cr;

  /* RV specific */
  /** \brief (For RV only) The timestamp of the current picture, which is parsed from RV frame
   *  headers.
   *  \n The timestamp of a B-frame should be adjusted according to the timestamp of its forward
   *  reference frame. */
  u32 timestamp;
  /** \brief (For RV only) The number of valid entries in the \c DecInputParameters.slice_info
   *  array. */
  u32 slice_info_num;
  /** \brief (For RV only) A pointer to the slice information. */
  struct DecSliceInfo *slice_info;
  /** \brief \- Bits[6:0] specify whether a post-procesing channel is disabled to save bandwidth when the power optimization feature (POF) is supported. Bits 0 to 6 correspond to post-processing channels 0 to 6.
    \n Value 0 of a bit indicates the corresponding post-processing channel is enabled.
    \n Value 1 of a bit indicates the corresponding post-processing channel is disabled.
    \n For example, <tt>0x0000010</tt> indicates only post-processing channel 1 is disabled and other post-processing channels are enabled.
    \n Bit 7 indicates the priority of a command buffer task for the VCMD.
    \n This bit is valid only when the VCMD is used for H.264, H.265, or JPEG decoding.
    \n - <tt>0</tt>: Normal.
    \n - <tt>1</tt>: High.
    \n - Bits[31:8]: Reserved. */
  u32 dec_ctrl;
  /** \brief The batch size.
   *   \n This field is used to dynamically adjust the batch size before decoding.
   *   \n This field is currently vaild only for the H.264 or H.265 format.
   *   \n Valid values: [2, <tt>max_batch_size</tt>], where <tt>max_batch_size</tt> is the value of \c DecInitConfig.batch_size. */
  u32 batch_size;
  /** \brief A pointer to user data for each picture. \n Each value of this field corresponds to a value of \c DecInputParameters.pic_id, and the value of this field is the same as the value of \c DecPictures.pic_user_data. */
  void *pic_user_data;
  /** \brief Film grain parameters from external file */
  struct PpFilmGrainParams *fg_params;
};

/** \brief The prototype of the callback function for stream consumption. This callback function
 *  is called by the decoder to notify the application that data in a stream buffer has been
 *  consumed and can be reused.
 *  \n <tt>stream</tt>: The base address of the input stream buffer specified in VCDecDecode().
 *  \n <tt>p_user_data</tt>: An application-provided pointer to user data. This is
 *  specified during decoder initialization.
 *  \ingroup common_group */
typedef void DecMCStreamConsumed(void *stream, void *p_user_data);

/** \brief Contains multi-core configurations for decoder initialization.
 *  \ingroup common_group */
struct DecMCConfig {
  /** \brief Whether multiple cores are used.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Single core.
   *  \n - <tt>1</tt>: Multiple cores. */
  u32 mc_enable;
  /** \brief The callback function for stream consumption. */
  DecMCStreamConsumed *stream_consumed_callback;
};

/** \brief Contains decoder initialization configurations.
 *  \ingroup common_group */
struct DecInitConfig {
  /** \brief The DWL instance. */
  const void* dwl_inst;
  /** \brief The decoder type. */
  enum DecCodec codec;
  /** \brief Whether to output decoded pictures in decoding order.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Do not output decoded pictures in decoding order.
   *  \n - Non-zero integers: Output decoded pictures in decoding order. */
  u32 disable_picture_reordering;
  /** \brief The error handling mode. */
  enum DecErrorHandling error_handling;
  /** \brief The error ratio used to determine whether to output an erroneous frame.
   *  \n This field is valid only if \c DecInitConfig.error_handling is set to
   *  <tt> \ref DEC_EC_FRAME_TOLERANT_ERROR</tt>.
   *  \n Valid range: [0, 100]. */
  u32 error_ratio;
  /** \brief Whether to enable reference frame compression (RFC).
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 use_video_compressor;
  /** \brief A flag to indicate that the input stream contains at least two frames. */
  u32 multi_frame_input_flag;
  /** \brief The decoder working mode. */
  enum DecDecoderMode decoder_mode;
  /** \brief The minimum difference between the minimum buffer number and the allocated buffer
   * number.
   * \n Once the specified difference is met, the decoder forcibly returns \ref <tt>HDRS_RDY</tt> even if buffer number or buffer size is
   * sufficient for the new sequence. */
  u32 guard_size;
  /** \brief Whether to use existing output frame buffers when the buffer number and size meet
   *  the requirement of the coming sequence.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Reallocate buffers and use the new buffers.
   *  \n - <tt>1</tt>: Use existing buffers. */
  u32 use_adaptive_buffers;
  /** \brief Whether to enable ASO and FMO stream decoding.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 support_asofmo_stream;
  /** \brief The number of frame buffers for the decoder. */
  u32 num_frame_buffers;
  /** \brief (Obsolete) Whether to enable RLC mode for H.264 and MPEG-4 only.
   *  \n When RLC mode is enabled, the software performs bitstream entropy
   *  decoding and saves the intermediate data in the external RLC buffer. The hardware then reads data
   *  from the buffer and performs subsequent decoding.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 rlc_mode;
  /** \brief The multi-core configurations. */
  struct DecMCConfig mc_cfg;
  /** \brief The frame skipping mode. */
  enum DecSkipFrameMode skip_frame;
  /** \brief Whether to dump auxiliary information.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable auxiliary information dump.
   *  \n - <tt>1</tt>: Enable QP dump.
   *  \n - <tt>2</tt>: Enable MV dump.
   *  \n - <tt>3</tt>: Enable both QP dump and MV dump. */
  u32 auxinfo;
  /** \brief (For VP8 and MPEG-4 only) The input sequence format. */
  enum DecInputFormat dec_format;
  /* h264 specific */
  /** \brief (For H.264 only) Whether the input stream is an MVC stream.
   *  \n Valid values:
   *  \n - <tt>0</tt>: The input stream is a non-MVC stream.
   *  \n - <tt>1</tt>: The input stream is an MVC stream. */
  u32 mvc;
  /* rv specific */
  /** \brief (For RV only) The length of each frame code, in bits.
   *  \n The frame code length indicates the number of bits in the stream used for the
   *  \c frame_size_code syntax, which will be discarded by the hardware. */
  u32 frame_code_length;
  /** \brief (For RV only) An array of dimensions of possible resampled image sizes.
   *  \n For each frame, the first four bytes indicate the width, while the subsequent four bytes indicate the height. */
  u32 *frame_sizes;
  /** \brief (For RV only) The RV version.
   *  \n Valid values:
   *  \n - <tt>0</tt>: RV8.
   *  \n - <tt>1</tt>: RV9 or RV10. */
  u32 rv_version;
  /** \brief (For RV only) The maximum frame width for coded pictures in the stream.
   *  \n Unit: Pixels. */
  u32 max_frame_width;
  /** \brief (For RV only) The maximum frame heigth for coded pictures in the stream.
   *  \n Unit: Pixels. */
  u32 max_frame_height;
  /* vc1 specific */
  /** \brief (For VC-1 only) The metadata of the stream. */
  struct DecMetaData meta_data;
  /* av1 specific */
  /** \brief (For AV1 only) Whether the stream to be decoded conforms to <em>Annex B: Length
   *  delimited bitstream format</em> defined in the AV1 specifications.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Do not conform.
   *  \n - <tt>1</tt>: Conform. */
  u32 annexb;
  /** \brief (For AV1 only) Whether the input stream is packetized in OBUs. Each OBU has a header,
   *  which provides information for identifying the contained data (payload).
   *  \n Valid values:
   *  \n - <tt>0</tt>: The input stream is not packetized in OBUs.
   *  \n - <tt>1</tt>: The input stream is packetized in OBUs. */
  u32 plainobu;
  /** \brief Whether to enable film grain after post-processsing. This field is valid only if \c APPLY_FILM_GRIAN is defined.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable */
  u32 apply_film_grain;
  /** \brief (For AV1 only) The polling period with multi-core decoding.
   *  \n For more information, see the description of <tt>sw_dec_mc_polltime</tt> in <tt>Hantro VC9x00D
   *  Series Accessible Registers</tt>. */
  u32 multicore_poll_period;
  /** \brief (For AV1 only) Whether tiles to be decoded are processed in column-first order. If column-first order is used,  all
   *  tiles are decoded from top to bottom in the first column, and then from top to bottom in the second column,
   *  and so on.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Tiles are not decoded in column-first order.
   *  \n - <tt>1</tt>: Tiles are decoded in column-first order. */
  u32 tile_transpose;
  /** \brief (For AV1 only) The maximum IDs of operating points for identifying frames to be decoded.
   *  \n Bits 0 to 3 indicate the maximum temporal layer ID, which ranges from 0 to 7.
   *  \n Bits 4 to 7 indicate the maximum spatial layer ID, which ranges from 0 to 3.
   *  \n For example, value <tt>0x24</tt> indicates that the decoding core decodes only frames whose temporal layer ID is less than or equal to 4 and spatial layer ID is less than or equal to 2.
   *  \n If this data field is not specified, frames of all layers are decoded.
   *  \n Default value: <tt>0x37</tt>. */
  u32 oppoints;
  /** \brief Whether checksum validation is enabled. Checksum validation is only supported when VCMD is used.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 crc_enabled;
  /** \brief \- Bits[15:0] control which core is used for decoding.
   * \n  <tt>0x00000000</tt>: SDK adaptively chooses a core.
   * \n  <tt>0x00000001</tt>: Core 0 is used.
   * \n  <tt>0x00000002</tt>: Core 1 is used.
   * \n  <tt>0x00000003</tt>: Core 0 and core 1 are used.
   * \n - Bit[16] controls whether to output IDR immediately.
   * \n <tt>1</tt>: Output immediately.
   * \n <tt>0</tt>: Do not output immediately.
   * \n - Bit[17] controls whether to enable dynamic buffer allocation for post-processing (PP) output buffers and decoded picture buffers (DPBs).
   * \n <tt>1</tt>: Enable.
   * \n <tt>0</tt>: Disable.
   * \n - Bit[18] controls whether to discard repeated frames.
   * \n <tt>1</tt>: Discard.
   * \n <tt>0</tt>: Do not discard.
   * \n - Bits[31:19]: Reserved. */
  u32 misc_ctrl;

  /** \brief The batch size.
   *   \n If the batch size is larger than 1, decoding starts only after the specified batch size is reached.
   *   \n This field is currently vaild only for the H.264 or H.265 format.
   *   \n Valid values: [0, <tt>MAX_BATCH_SIZE</tt>]. */
  u32 batch_size;
};

/** \brief Contains decoder configurations.
 *  \ingroup common_group */
struct DecConfig {
  /** \brief The post-processing configurations of each output channel. */
  PpUnitConfig ppu_cfg[DEC_MAX_OUT_COUNT];
  /** \brief The delogo filter configurations. */
  DelogoConfig delogo_params[2];
  /** \brief The stride alignment.
   *  \n Vallid values: <tt> \ref DEC_ALIGN_1B</tt>, <tt> \ref DEC_ALIGN_8B</tt>,
   *  <tt> \ref DEC_ALIGN_16B</tt>, and <tt> \ref DEC_ALIGN_512B</tt>. */
  DecPicAlignment align;
  /** \brief (For HEVC/H.264/AV1 only) The error concealment.*/
  u32 hw_conceal;
  /** \brief (For HEVC/H.264) Whether to disable the slice mode.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Enable.
   *  \n - <tt>1</tt>: Disable. */
  u32 disable_slice;
  /** \brief (For HEVC/H.264/VVC only) The maximum number of temporal layers that the decoder
   *  can decode. */
  i8 max_temporal_layer;

  /* jpeg specific */
  /** \brief (For JPEG only) The decoded picture type.
   *  \n Valid values:
   *  \n - <tt>JPEGDEC_IMAGE</tt>: Full picture.
   *  \n - <tt>JPEGDEC_THUMBNAIL</tt>: Thumbnail picture. */
  u32 dec_image_type;
  /** \brief (For JPEG only) The chroma sampling modes.
   *  \n Supported modes: 420, 422, etc. */
  u32 chroma_format;
  /* standalone pp specific */
  /** \brief (For standalone post-processing only) The format of the input. */
  u32 in_format;
  /** \brief (For standalone post-processing only) The luma stride of the input pictures. */
  u32 in_stride;
  /** \brief (For standalone post-processing only) The chroma stride of the input pictures. */
  u32 in_stride_ch;
  /** \brief (For standalone post-processing only) The height of the input pictures.
   *  \n Unit: Pixels. */
  u32 in_height;
  /** \brief (For standalone post-processing only) The width of the input pictures.
   *  \n Unit: Pixels. */
  u32 in_width;
  /** \brief (For standalone post-processing only) The stride of the RFC compressed input luma data. */
  u32 in_lu_stride;
  /** \brief (For standalone post-processing only) The stride of the RFC compressed input chroma data. */
  u32 in_ch_stride;
  /** \brief (For standalone post-processing only) The stride of the RFC table for the luma component. */
  u32 in_lut_stride;
  /** \brief (For standalone post-processing only) The stride of the RFC table for the chroma component. */
  u32 in_cht_stride;
  /** \brief (For standalone post-processing only) The bit depth of the RFC compressed input luma data. */
  u32 in_luma_bitdepth;
  /** \brief (For standalone post-processing only) The bit depth of the RFC compressed input chroma data. */
  u32 in_chroma_bitdepth;
  /** \brief (For standalone post-processing only) Whether the input is RFC compressed.
   *  \n Valid values:
   *  \n - <tt>0</tt>: The input is not RFC compressed.
   *  \n - <tt>1</tt>: The input is RFC compressed. */
  u8 in_rfc;
  /** \brief (For standalone post-processing only) The input tiled mode of DEC400. */
  u8 in_tiled_mode;
  /** \brief (For standalone post-processing only) Whether the input is DEC400 compressed.
   *  \n Valid values:
   *  \n - <tt>0</tt>: The input is not DEC400 compressed.
   *  \n - <tt>1</tt>: The input is DEC400 compressed. */
  u8 in_dec400;
  /** \brief (For standalone post-processing only) The alignment of the DEC400 compressed input.
   *  \n Valid values: <tt> \ref DEC_ALIGN_32B</tt> and <tt> \ref DEC_ALIGN_64B</tt>. */
  u8 in_dec400_a;
  /** \brief (For standalone post-processing only) The input chroma sampling mode relative to luma sampling.
   *  \n Valid values:
   *  \n - <tt>0</tt>: YUV400 (monochrome).
   *  \n - <tt>1</tt>: YUV420.
   *  \n - <tt>2</tt>: YUV422. */
  u8 in_chroma_format_idc;
  /** \brief (For standalone post-processing only) The post-processing input buffer. */
  struct DWLLinearMem pp_in_buffer;
  /** \brief (For standalone post-processing only) The post-processing output buffer. */
  struct DWLLinearMem pp_out_buffer;
  /* for color remapping (3dlut) */
  /** \brief (For standalone post-processing only) The 3D LUT buffer. */
  struct DWLLinearMem table_3dlut_buffer;
  /** \brief \- Bits[15:0] control which core is used for decoding.
   * \n <tt>0x00000000</tt>: SDK adaptively chooses a core or a special core is used.
   * \n <tt>0x00000001</tt>: Core 0 is used.
   * \n <tt>0x0000002</tt>: Core 1 is used.
   * \n <tt>0x00000003</tt>: Core 0 and core 1 are used.
   * \n - Bit[16] controls whether to provide different addresses for different post-processing channels.
   * \n <tt>0</tt>: Do not provide different addresses.
   * \n <tt>1</tt>: Provide different addresses.
   * \n - Bits[31:17]: Reserved.*/
  u32 misc_ctrl;
  /** \brief Whether checksum validation is enabled. Checksum validation is only supported when VCMD is used.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 crc_enabled;
  /** \brief Whether to enable odd-to-even resolution conversion for JPEG decoding output.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u8 jpeg_adjust_even;
};

/* IFD0 main image information */
struct MainImageInfo {
  char ImageDescription[400];
  char Artist[100];
  char CameraMake[100];
  char CameraModel[100];
  u32 Orientation;
  u32 XResolution;
  u32 YResolution;
  // u32 ImageWidth;
  // u32 ImageHeight;
  char ResolutionUnit[10];
  char Software[100];
  char DateTime[20];
  u32 YCbCrPositioning;
  u32 ExifOffset;
  u32 Ifd1_Offset;
};

/* Exif SubIFD information */
struct ExifSubIfdInfo {
  Fraction ExposureTime; // 1/190s
  float FNumber;
  char ExposureProgram[32];
  u32 ISOSpeedRatings;
  char ExifVersion[32];
  char DateTimeOriginal[32];
  char DateTimeDigitized[32];
  float ExposureBiasValue;
  float MaxApertureValue;
  char MeteringMode[32];
  char Lightsource[100];
  char Flash[100];
  float FocalLength;
  char ColorSpace[32];
  u32 ExifImageWidth; /* main image width */
  u32 ExifImageHeight; /* main image height */
};

/* IFD1 main image information */
struct ThumbnailImageInfo {
  u32 ImageWidth;
  u32 ImageHeight;
  char Compression[16];
  u32 PhotometricInterpretation;
  u32 XResolution;
  u32 YResolution;
  char ResolutionUnit[10];
  u32 JpegIFOffset;
  u32 JpegIFByteCount;
  float YCbCrCoefficients;
  u32 YCbCrPositioning;
  // u32 BitsPerSample;
  // u32 StripOffsets;
  // u32 SamplesPerPixel;
  // u32 RowsPerStrip;
  // u32 StripByteConunts;
  // u32 PlanarConfiguration;
  // u32 YCbCrSubSampling;
  // u32 ReferenceBlackWhite;
};

/* Exif information */
struct ExifInfo {
  struct MainImageInfo ifd0_info;
  struct ExifSubIfdInfo exif_subifd_info;
  struct ThumbnailImageInfo ifd1_info;
};

/** \brief Contains information of a video sequence.
 *  \ingroup common_group */
struct DecSequenceInfo {
  /** \brief The VP codec version defined in the input stream. \n  This is a digital value that serves as a syntax element at the bitstream level. */
  u32 vp_version;
  /** \brief The VP codec profile defined in the input stream. \n This is a digital value that serves as a syntax element at the bitstream level. */
  u32 vp_profile;
  /** \brief The codec profile defined in the input stream. \n This is a semantic interpretation derived from syntax values. */
  u8  *profile;
  /** \brief The codec level defined in the input stream. \n This is a semantic interpretation derived from syntax values. */
  u8  *level;
  /** \brief The decoded picture width.
   *  \n Unit: Pixels. */
  u32 pic_width;
  /** \brief The decoded picture height.
   *  \n Unit: Pixels. */
  u32 pic_height;
  /** \brief The scaled width of the displayed picture.
   *  \n Unit: Pixels. */
  u32 scaled_width;
  /** \brief The scaled height of the displayed picture.
   *  \n Unit: Pixels. */
  u32 scaled_height;
  /** \brief The decoded thumbnail width.
   *  \n Unit: Pixels. */
  u32 pic_width_thumb;
  /** \brief The decoded thumbnail height.
   *  \n Unit: Pixels. */
  u32 pic_height_thumb;
  /** \brief The scaled width of the displayed thumbnail.
   *  \n Unit: Pixels. */
  u32 scaled_width_thumb;
  /** \brief The scaled height of the displayed thumbnail.
   *  \n Unit: Pixels. */
  u32 scaled_height_thumb;
  /** \brief The width that is used to calculate the sample aspect ratio. */
  u32 sar_width;
  /** \brief The height that is used to calculate the sample aspect ratio. */
  u32 sar_height;
  /** \brief Whether to disable the comformance window.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Enable.
   *  \n - <tt>1</tt>: Disable. */
  u32 dis_comformance_window;
  /** \brief The cropping configurations. */
  struct DecCropParams crop_params;
  /** \brief The YUV sample range of the decoded pictures. */
  enum DecVideoRange video_range;
  /** \brief The matrix coefficients for RGB-to-YUV conversion. */
  u32 matrix_coefficients;
  /** \brief Whether the sequence is monochrome.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Not monochrome.
   *  \n - <tt>1</tt>: Monochrome. */
  u32 is_mono_chrome;
  /** \brief Whether the sequence is interlaced.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Not interlaced.
   *  \n - <tt>1</tt>: Interlaced. */
  u32 is_interlaced;
  /** \brief The maximum number of reference frames. */
  u32 num_of_ref_frames;
  /** \brief The chroma sampling mode relative to luma sampling.
   *  \n Valid values:
   *  \n - <tt>0</tt>: YUV400 (monochrome).
   *  \n - <tt>1</tt>: YUV420.
   *  \n - <tt>2</tt>: YUV422. */
  u32 chroma_format_idc;
  /** \brief The bit depth of the stored luma picture. */
  u32 bit_depth_luma;
  /** \brief The bit depth of the stored chroma picture. */
  u32 bit_depth_chroma;
  /** \brief (For AVS2 only) The bit depth of the output picture. */
  u32 out_bit_depth;
  /** \brief (For PNG only) The color type of the output picture. */
  u32 color_type;
  /** \brief (For PNG only) The number of channels of the output picture. */
  u32 channels;
  /** \brief The content storage mode in DPB. */
  enum DecDpbMode dpb_mode;
  /** \brief The format of the output picture. */
  enum DecPictureFormat output_format;
  /** \brief The format of the output thumbnail. */
  enum DecPictureFormat output_format_thumb;
  /** \brief (For JPEG/PNG) The picture coding type.
   *  \n Valid values: <tt>JPEG_BASELINE</tt>, <tt>JPEG_PROGRESSIVE</tt>, and
   *  <tt>JPEG_NONINTERLEAVED</tt>. */
  u32 coding_mode;
  /** \brief (For JPEG only) The thumbnail coding type.
   *  \n Valid values: <tt>JPEG_BASELINE</tt>, <tt>JPEG_PROGRESSIVE</tt>,
   *  <tt>JPEG_NONINTERLEAVED</tt> and <tt>PNG_BASELINE<tt>.*/
  enum JpegCodingMode coding_mode_thumb;
  /** \brief Whether thumbnail is available and supported.
   *  \n Valid values:
   *  \n <tt>0</tt>: No thumbnail exists.
   *  \n <tt>1</tt>: A thumbnail exists.
   *  \n <tt>2</tt>: Thumbnails are not supported. */
  u32 thumbnail_type;
  /* AV1 specific */
  /** \brief The maximum output width of AV1 decoding. */
  u32 av1_max_dec_width;
  /** \brief The maximum output height of AV1 decoding. */
  u32 av1_max_dec_height;
  /* H264 specific */
  /** \brief (For H.264 only) Whether to enable post-processing channels.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 pp_enabled;
  /** \brief (For H.264 only) The baseline profile. */
  u32 h264_base_mode;
  /* VC1 specific */
  /** \brief (For VC1 only) The frame rate numerator of the current sequence. */
  u32 frame_rate_numerator;
  /** \brief (For VC1 only) The frame rate denominator of the current sequence. */
  u32 frame_rate_denominator;
  /** \brief (For VC1 only) The output buffer release mode. */
  u32 buf_release_flag;
  /* JPEG specific */
  /** \brief (For JPEG only) The input parameters. */
  struct DecInputParameters pic_input_info;
  /** \brief (For JPEG only) The maximum decoding width. */
  u32 img_max_dec_width;
  /** \brief (For JPEG only) The maximum decoding height. */
  u32 img_max_dec_height;
  /* AVS specific */
  /** \brief (For AVS only) The profile ID. */
  u32 profile_id;
  /** \brief (For AVS only) The level ID. */
  u32 level_id;
  /* MPEG2 MPEG4 AVS specific */
  /** \brief (For MPEG2/MPEG4 only) The profile and level indication of the current sequence. */
  u32 profile_and_level_indication;
  /** \brief (For MPEG2/MPEG4 only) The display aspect ratio of the current sequence. */
  u32 display_aspect_ratio;
  /** \brief (For MPEG2/MPEG4 only) The stream format of the current sequence. */
  u32 stream_format;
  /* HDR related syntax */
  /** \brief The standardized video format. */
  u32 video_format;
  /** \brief The chromaticity coordinates of the source primaries */
  u32 colour_primaries;
  /** \brief The transfer specifications. */
  u32 transfer_characteristics;
  /** \brief Whether \c colour_primaries, \c transfer_characteristics, and \c matrix_coefficients are present.
   * \n Valid values:
   * \n - <tt>0</tt>: Present.
   * \n - <tt>1</tt>: Not present.*/
  u32 colour_description_present_flag;
  /** \brief (For H.265 only) Whether to fill in the \c transfer_characteristics and \c preferred_transfer_characteristics fields.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Fill in both fields.
   *  \n - <tt>1</tt>: Fill in the \c transfer_characteristics field only.
   *  \n - <tt>2</tt>: Fill in the \c preferred_transfer_characteristics field only. */
  u32 trc_status;
  /** \brief (For H.265 only) The <tt>transfer_characteristic</tt> syntax of SEI. */
  u32 preferred_transfer_characteristics;
  /** \brief The number of picture buffers when the decoder and post-processing core operate in multi-buffer mode */
  u32 multi_buff_pp_size;
  /* MPEG4 specific */
  /** \brief (For MPEG-4 only) The length of VOS data. */
  u32 user_data_voslen;
  /** \brief (For MPEG-4 only) The length of VISO data. */
  u32 user_data_visolen;
  /** \brief (For MPEG-4 only) The length of VOL data. */
  u32 user_data_vollen;
  /** \brief (For MPEG-4 only) The length of GOV data. */
  u32 user_data_govlen;
  /** \brief (For MPEG-4 only) Whether GMC is supported. */
  u32 gmc_support;
  /* RV specific */
  /** \brief (For RV only) The output picture width. */
  u32 out_pic_coded_width;
  /** \brief (For RV only) The output picture height. */
  u32 out_pic_coded_height;
  /** \brief (For RV only) The output status of the current picture. */
  u32 out_pic_stat;
  /* time scale info */
  /** \brief Whether \c num_units_in_tick and \c time_scale are present.
   * \n Valid values:
   * \n - <tt>0</tt>: Not present.
   * \n - <tt>1</tt>: Present.*/
  u32 timing_info_present_flag;
  /** \brief The number of time units of a clock operating at the frequency specified by \c time_scale. */
  u32 num_units_in_tick;
  /** \brief The number of time units per second. */
  u32 time_scale;
  /** \brief Whether the thumbnail needs to be decoded.
   * \n Valid values:
   * \n - <tt>0</tt>: No decoding is required.
   * \n - <tt>1</tt>: Decoding is required. */
  u32 thumbnail_done;
  /** \brief The related EXIF infomation. */
  struct ExifInfo exif_info;
#ifdef EXTRACT_THUMBNAIL_JPG
  FILE *out_thumbnail_file;
  u8* thumbnail_base;
  u32 thumbnail_len;
#endif
  /** \brief (For AV1 only) The maximum width of the picture. */
  u32 av1_max_width;
  /** \brief (For AV1 only) The maximum height of the picture. */
  u32 av1_max_height;
  /** \brief Whether the HDR static metadata exists.
   * \n Valid values:
   * \n - <tt>0</tt>: The metadata does not exist.
   * \n - <tt>1</tt>: The metadata exists. */
  u32 sei_static_metadata_present;
  /** \brief Green of luma primaries. */
  u32 lu_primaries[2];
  /** \brief Blue of Cb primaries. */
  u32 cb_primaries[2];
  /** \brief Red of Cr primaries. */
  u32 cr_primaries[2];
  /** \brief The white point location. */
  u32 white_point[2];
  /** \brief The maximum value of display luminance. */
  u32 max_display_mastering_luminance;
  /** \brief The minimum value of display luminance. */
  u32 min_display_mastering_luminance;
  /** \brief The maximum content light level. */
  u16 max_content_light_level;
  /** \brief The maximum picture average light level. */
  u16 max_pic_average_light_level;
};

/** \brief Contains information of each picture.
 *  \ingroup common_group */
struct DecPictureInfo {
  /** \brief The picture coding type. */
  enum DecPicCodingType pic_coding_type;
  /** \brief The picture coding type of the bottom field. */
  enum DecPicCodingType pic_coding_type_field;
  /** \brief Whether the picture is corrupted.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Not corrupted.
   *  \n - <tt>1</tt>: Corrupted. */
  u32 is_corrupted;
  /** \brief The color format of the picture. */
  enum DecPictureFormat format;
  /** \brief (For AVS/MPEG-2/MPEG-4 only) The time information. */
  struct DecTime time_code;
  /** \brief The avarage decoding time per MB, in cycles. */
  u32 cycles_per_mb;
#ifdef FPGA_PERF_AND_BW
  /** \brief The bit rate per picture (4k60fps). */
  u32 bitrate_4k60fps;
  /** \brief The read bandwidth in luma frame size. */
  u32 bwrd_in_fs;
  /** \brief The write bandwidth in luma frame size. */
  u32 bwwr_in_fs;
#endif
  /** \brief The ID of the picture to be decoded. The value of this field must be the same as the value of \c DecInputParameters.pic_id. */
  u64 pic_id;
  /** \brief The ID of the picture to be decoded. This field determines which input frame corresponds to the current output frame. */
  u64 decode_id;
  /** \brief The picture order count of the picture. */
  i32 poc;
  /** \brief Whether the frame is an intra frame.
   *  \n Valid values:
   *  \n - <tt>0</tt>: The frame is not an intra frame.
   *  \n - <tt>1</tt>: The frame is an intra frame. */
  u32 is_intra_frame;
  /** \brief Whether the frame is an intra frame for the bottom field.
   *  \n Valid values:
   *  \n - <tt>0</tt>: The frame is not an intra frame.
   *  \n - <tt>1</tt>: The frame is an intra frame. */
  u32 is_intra_frame_field;
  /** \brief Whether the frame is a golden reference frame.
   *  \n Valid values:
   *  \n - <tt>0</tt>: The frame is not a golden reference frame.
   *  \n - <tt>1</tt>: The frame is a golden reference frame. */
  u32 is_golden_frame;
  /** \brief Whether the frame is a field frame.
   *  \n Valid values:
   *  \n - <tt>0</tt>: The frame is not a field frame.
   *  \n - <tt>1</tt>: The frame is a field frame. */
  u32 field_picture;
  /** \brief Whether the frame is a top frame.
   *  \n Valid values:
   *  \n - <tt>0</tt>: The frame is not a top frame.
   *  \n - <tt>1</tt>: The frame is a top frame. */
  u32 top_field;
  /** \brief Whether the frame is a first frame.
   *  \n Valid values:
   *  \n - <tt>0</tt>: The frame is not a first frame.
   *  \n - <tt>1</tt>: the frame is a first frame. */
  u32 first_field;
  /** \brief Whether to repeat the first field of a frame.
   * \n Valid values:
   * \n - <tt>0</tt>: Do not repeat.
   * \n - <tt>1</tt>: Repeat the first field. */
  u32 repeat_first_field;
  /** \brief Whether to process only a single field of a frame.
   * \n Valid values:
   * \n - <tt>0</tt>: Process not only the single field.
   * \n - <tt>1</tt>: Process only a single field. */
  u32 single_field;
  /** \brief Whether to output the other field.
   * \n Valid values:
   * \n - <tt>0</tt>: Do not output the other field.
   * \n - <tt>1</tt>: Output the other field. */
  u32 output_other_field;
  /** \brief The count of repeat frames. */
  u32 repeat_frame_count;
  /** \brief The number of concealed macroblocks in the frame. */
  u32 nbr_of_err_mbs;
  /** \brief The number of luma pixel rows in the WebP output frame buffer.
   *  \n The field value is \c 0 if the buffer contains the entire picture. */
  u32 num_slice_rows;
  /** \brief Whether the frame contains the last slice.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Does not contain.
   *  \n - <tt>1</tt>: Contains. */
  u32 last_slice;
  /** \brief The pixel row stride for luma. */
  u32 luma_stride;
  /** \brief The pixel row stride for chroma. */
  u32 chroma_stride;
  /* H264 specific */
  /** \brief (For H.264 only) The ID of the view to which the output picture belongs. */
  u32 view_id;
  /** \brief The pixel sum for luma data. */
  u32 luma_sum;
};

/** \brief Contains information about decoded pictures of each output channel.
 *  \ingroup common_group */
struct DecPicture {
  /** \brief The video sequence coding parameters. */
  struct DecSequenceInfo sequence_info;
  /** \brief The buffer to store the Y component. */
  struct DWLLinearMem luma;
  /** \brief The buffer to store the U component. */
  struct DWLLinearMem chroma;
  /** \brief The buffer to store the V component. */
  struct DWLLinearMem chroma_cr;
  /** \brief The DMV buffer. */
  struct DWLLinearMem dmv;
  /** \brief The QP buffer. */
  struct DWLLinearMem qp;
  /** \brief The buffer to store the luma tile status for DEC or RFC. */
  struct DWLLinearMem luma_table;
  /** \brief The buffer to store the chroma tile status for DEC or RFC. */
  struct DWLLinearMem chroma_table;
  /** \brief The information of each picture. */
  struct DecPictureInfo picture_info;
  /** \brief The SEI parameters. */
  struct DecSEIParameters sei_param;
  /** \brief The ID of the enabled cropping region. */
  u32 crop_flag;
  /** \brief The picture width. */
  u32 pic_width;
  /** \brief The picture height. */
  u32 pic_height;
  /** \brief The picture stride for luma. */
  u32 pic_stride;
  /** \brief The picture stride for chroma. */
  u32 pic_stride_ch;
  /** \brief The chroma format for decoded output. */
  u32 chroma_format;
  /** \brief Whether to enable post-processing.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u32 pp_enabled;
  /** \brief Whether the picture is a GDR picture.
   *  \n Valid values:
   *  \n - <tt>0</tt>: The picture is not a GDR picture.
   *  \n - <tt>1</tt>: The picture is a GDR picture. */
  u32 is_gdr_frame;
  /** \brief The checksum of the output data. */
  u32 *crc;

	/*if sequence_info.is_interlaced==1*/
  /** \brief The field order for interlaced coding.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Bottom field first.
   *  \n - <tt>1</tt>: Top field first. */
  u32 top_field_first;
  /** \brief The number of fields in the picture.
   *  \n Valid range: [0, 2] */
  u32 fields_in_picture;
  /** \brief Whether to enable normalization.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u8 norm_enable;
  /** \brief The normalized output format. */
  u8 norm_fmat;
  /** \brief Whether to enable the histogram statistics feature.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u8 hist_e;
  /** \brief Whether to enable DEC400 compression conformance test.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u8 dec400_conformance_test_en;
  /** \brief Whether to enable FBC tile mode.
   *  \n Valid values:
   *  \n - <tt>0</tt>: Disable.
   *  \n - <tt>1</tt>: Enable. */
  u8 pp_fbc_tile_enable;
};

/** \brief Contains decoded pictures for each decoder output channel.
 *  \ingroup common_group */
struct DecPictures {
  /** \brief The internal post-processing slot that outputs data. \n This field is only vaild when post-processing is enabled. \n
  * The application does not modify the value. The value is returned by <tt>VCDecPictureConsumed()</tt>. */
  u32 pp_index;
  /** \brief The user data of the picture. The value of this field must be the same as the value of \c DecInputParameters.pic_user_data. */
  void *pic_user_data;
  /** \brief The information about decoded pictures of each output channel. */
  struct DecPicture pictures[DEC_MAX_OUT_COUNT];
};

/** \brief Contains the information of reference frame buffers.
 *  \ingroup common_group */
struct DecBufferInfo {
  /** \brief The extra buffer size. */
  u64 next_buf_size;
  /** \brief The number of extra buffers. */
  u32 buf_num;
  /** \brief The initialized buffer. */
  struct DWLDecBuffers buf_to_free;
#ifdef ASIC_TRACE_SUPPORT
  /** \brief Indicates if the buffer is a frame buffer */
  u32 is_frame_buffer;
#endif
  /** \brief The buffer size for each plane required by an output channel. */
  u64 output_buf_size[DEC_MAX_OUT_COUNT][MAX_OUTPUT_PLANE];
  /** \brief The stride for the luma (Y) plane of each output buffer. */
  u32 ystride[DEC_MAX_OUT_COUNT];
  /** \brief The stride for the chroma (Cb/Cr) planes of each output buffer. */
  u32 cstride[DEC_MAX_OUT_COUNT];
};

/** \brief Contains the output information of the video decoder.
 *  \ingroup common_group */
struct DecOutput {
  /** \brief A pointer to the position where decoding ends in the stream. */
  u8* strm_curr_pos;
  /** \brief The bus address of the location where decoding ends. */
  addr_t strm_curr_bus_address;
  /** \brief The number of bytes left undecoded. */
  u32 data_left;
  /** \brief A pointer to the start of the stream buffer. */
  u8* strm_buff;
  /** \brief The bus address of the start of the stream buffer. */
  addr_t strm_buff_bus_address;
  /** \brief The size of the stream buffer. */
  u32 buff_size;
  /** \brief A pointer to the SEI buffer. */
  struct SEI_buffer *sei_buffer;
  /* h264 specific */
  /** \brief (For H264 only) Whether the bitstream buffer pointer is flushed back to the beginning of the buffer.
  *  \n Valid values:
  *  \n - <tt>0</tt>: Do not flush back to the beginning of the buffer.
  *  \n - <tt>1</tt>: Flush back to the beginning of the buffer. */
  u32 is_back_to_buffer_begin;

  /* VP8 specific */
  /** \brief (For VP8 only) The height of each VP8 WebP slice. */
  u32 slice_height;
  /* VP9 specific */
  /** \brief (For VP9 only) The address for the current stream in low-latency mode. */
  addr_t llstrm_curr_address;
  /* JPEG specific */
  /** \brief (For JPEG only) Contains decoded pictures for each decoder output channel. */
  struct DecPictures pic;
  /** \brief Film grain parameters from external file */
  struct PpFilmGrainParams *fg_params;
};

enum PfcTileMode{
  TILED_16X16_COMP = 1,
  TILED_32X8_COMP = 2
};

#define IS_EXTERNAL_BUFFER(config, type) (((config) >> (type)) & 1)

#endif /* DECTYPES_H */
