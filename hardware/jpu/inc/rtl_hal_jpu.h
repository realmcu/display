/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_HAL_JPU_H
#define RTL_HAL_JPU_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/

#include "rtl_jpu.h"

/** \defgroup JPU       JPU
  * \brief    JPEG Processing Unit (JPU) driver for hardware-accelerated JPEG image decode and encode
  * \{
  */
/*============================================================================*
 *                         JPU Declaration
 *============================================================================*/
/** \defgroup JPU_Exported_Constants JPU Exported Constants
  * \brief    Constants and enumerations used by the JPU module
  * \{
  */

#define MAX_JPG_PIC_WIDTH   (32768)
#define MAX_JPG_PIC_HEIGHT  (32768)

#define DEFAULT_JPG_BUFF_SZ  (40 * 1024)

#define JPU_BUFF_NUM_MAX  (64)

/** \defgroup JPU_Declaration JPU Declaration
  * \{
  * \ingroup  JPU_Exported_Constants
  */

/** End of JPU_Declaration
  * \}
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/
/** \defgroup JPU_ALGORITHM Supported algorithms in JPU
  * \{
  * \ingroup  JPU_Exported_Constants
  */

typedef enum
{
    // TODO: specify error type
    JPU_SUCCESS = 0x0,
    JPU_ERR_FAILURE,
    JPU_ERR_INVALID_PARAM,
    JPU_ERR_HEAP_NOT_AVAILABLE,
    JPU_ERR_MALLOC_FAIL,
    JPU_ERR_DECODE,
    JPU_ERR_ENCODE,

    JPU_ERR_BBC_INT,

    JPU_ERR_BIT_EMPTY,
    JPU_ERR_EOS,
    JPU_ERR_INVALID_HANDLE,
    JPU_ERR_INVALID_COMMAND,
    JPU_ERR_ROTATOR_OUTPUT_NOT_SET,
    JPU_ERR_ROTATOR_STRIDE_NOT_SET,
    JPU_ERR_FRAME_NOT_COMPLETE,
    JPU_ERR_INVALID_FRAME_BUFFER,
    JPU_ERR_INSUFFICIENT_FRAME_BUFFERS,
    JPU_ERR_INVALID_STRIDE,
    JPU_ERR_WRONG_CALL_SEQUENCE,
    JPU_ERR_CALLED_BEFORE,
    JPU_ERR_NOT_INITIALIZED,
    JPU_ERR_NOT_SUPPORTED_FORMAT,

} JPU_ERROR;

/** End of JPU_ERROR
  * \}
  */

typedef enum
{
    JPU_STATUS_DONE       = 0x001,
    JPU_STATUS_ERROR      = 0x002,
    JPU_STATUS_BBC_INT    = 0x004,  /* JPEG buffer size is too small  */
    JPU_STATUS_OVERFLOW   = 0x008,
    JPU_STATUS_PB0        = 0x010,
    JPU_STATUS_PB1        = 0x020,
    JPU_STATUS_PB2        = 0x040,
    JPU_STATUS_PB3        = 0x080,
    JPU_STATUS_STOP       = 0x100,
} JPU_STATUS;


enum
{
    JPU_Marker          = 0xFF,
    JPU_FF_Marker       = 0x00,

    JPU_SOI_Marker      = 0xFFD8,           // Start of image
    JPU_EOI_Marker      = 0xFFD9,           // End of image

    JPU_JFIF_CODE       = 0xFFE0,           // Application  JFIF(JPEG File Interchange Format): jpeg info
    JPU_EXIF_CODE       = 0xFFE1,           // Exif(Exchangeable Image File Format): camera info

    JPU_DRI_Marker      = 0xFFDD,           // Define restart interval
    JPU_RST_Marker      = 0xD,              // 0xD0 ~0xD7

    JPU_DQT_Marker      = 0xFFDB,           // Define quantization table(s)
    JPU_DHT_Marker      = 0xFFC4,           // Define Huffman table(s)

    JPU_SOF_Marker      = 0xFFC0,           // Start of frame : Baseline DCT
    JPU_SOF1_Marker     = 0xFFC1,           // Nondifferential Huffman-coding frames :Extended sequential DCT - Not supported
    JPU_SOF2_Marker     = 0xFFC2,           // Nondifferential Huffman-coding frames :Progressive DCT - Not supported
    JPU_SOF3_Marker     = 0xFFC3,           // Nondifferential Huffman-coding frames :Lossless (sequential) - Not supported
    JPU_SOF5_Marker     = 0xFFC5,           // Defferential Huffman-coding frames : Differential sequential DCT - Not supported
    JPU_SOF6_Marker     = 0xFFC6,           // Defferential Huffman-coding frames : Differential progressive DCT - Not supported
    JPU_SOF7_Marker     = 0xFFC7,           // Defferential Huffman-coding frames : Differential lossless - Not supported
    JPU_SOF9_Marker     = 0xFFC9,           // Nondifferential arithmetic-coding frames : Extended sequential DCT - Not supported
    JPU_SOF10_Marker    = 0xFFCA,           // Nondifferential arithmetic-coding frames : Progressive DCT - Not supported
    JPU_SOF11_Marker    = 0xFFCB,           // Nondifferential arithmetic-coding frames : Lossless (sequential) - Not supported
    JPU_SOF13_Marker     = 0xFFCD,           // Defferential arithmetic-coding frames : Differential sequential DCT - Not supported
    JPU_SOF14_Marker     = 0xFFCE,           // Defferential arithmetic-coding frames : Differential progressive DCT - Not supported
    JPU_SOF15_Marker     = 0xFFCF,           // Defferential arithmetic-coding frames: Differential lossless - Not supported

    JPU_SOS_Marker      = 0xFFDA,           // Start of scan
};



typedef enum
{
    ROT_ANGLE_0  = 0,
    ROT_ANGLE_90 = 90,
    ROT_ANGLE_180 = 180,
    ROT_ANGLE_270 = 270
} JPU_ROT_ANGLE;

typedef enum
{
    MIRDIR_NONE = 0,
    MIRDIR_VER,
    MIRDIR_HOR,
    MIRDIR_HOR_VER
} JPU_MIR_DIRECTION;

typedef enum
{
    ENC_HEADER_MODE_NORMAL,
    ENC_HEADER_MODE_SOS_ONLY
} JpgEncHeaderMode;

/** End of JPU_Exported_Constants
  * \}
  */

/*============================================================================*
 *                             Types
 *============================================================================*/
/** \defgroup JPU_Exported_Types JPU Exported Types
  * \brief    Data structures and enumerations used by the JPU module
  * \{
  */

typedef struct
{
    // image
    uint8_t *data;            /**< JPEG image address to be decoded, should aligned to 8*/
    uint32_t size;            /**< JPEG image data size to be decoded*/

    //packed
    uint32_t frameFormat;     /**< Output YUV format, see `JPU_PACKED_FORMAT`*/

    //ROI
    uint32_t roiEnable;       /**< Enable ROI decode*/
    uint32_t roiWidth;        /**< ROI area: pixel unit, real decode area depends on JPEG subsample rate*/
    uint32_t roiHeight;
    uint32_t roiOffsetX;
    uint32_t roiOffsetY;

    //wrapper
    uint32_t useWrapper;      /**< Enable Wrapper: Convert RGB to YUV format after decoding*/
    uint32_t rgbType;         /**< Output RGB format: see `JPU_RGB_TYPE`*/
    uint8_t opacity;          /**< Global opacity when output ARGB: 0 ~ 255*/

    /**Function when output YUV format only*/
    // down scale
    int iHorScaleMode;
    int iVerScaleMode;

    // rotate
    int rotAngle;
    int mirDir;

} JPU_DEC_PARAM;



typedef struct
{
    // image
    uint8_t *data;
    uint32_t size;

    uint32_t useWrapper;
    uint32_t rgbType;
    uint8_t opacity;

    int StreamEndian;
    int FrameEndian;
    int chromaInterleave;

    // down scale
    int iHorScaleMode;
    int iVerScaleMode;

    // rotate
    uint8_t useRot;
    uint32_t rotAngle;
    uint32_t mirDir;

    //ROI
    int roiEnable;
    int roiWidth;
    int roiHeight;
    int roiOffsetX;
    int roiOffsetY;

    //packed
    int packedFormat;

    // partial
    int usePartialMode;
    int partialBufNum;
    int partialHeight;
} JPU_DEC_PARAM_INT;  // internal



typedef struct
{
    uint8_t *buffer;    // buffer start pointer
    int index;          // byte used
    int size;           // total bytes
} jpu_getbit_context_t;

typedef struct
{
    uint32_t streamEndian;
    uint32_t frameEndian;
    uint32_t chromaInterleave;

    uint32_t format;            // decode from header
    uint32_t picWidth;
    uint32_t picHeight;
    uint32_t alignedWidth;
    uint32_t alignedHeight;
    uint32_t headerSize;
    uint32_t ecsPtr;
    uint32_t pagePtr;
    uint32_t wordPtr;
    uint32_t bitPtr;
    uint32_t rstIntval;

    uint32_t userHuffTab;
    uint32_t huffDcIdx;
    uint32_t huffAcIdx;
    uint32_t Qidx;

    uint8_t huffVal[4][162];
    uint8_t huffBits[4][256];
    uint8_t cInfoTab[4][6];   // Component 0 compID, 1 hSampFact, 2 vSampFact,
    // 3 Quantization table ID, 4 dcHufTblIdx, 5 acHufTblIdx
    uint8_t qMatTab[4][64];   // Quantization table

    uint32_t huffMin[4][16];
    uint32_t huffMax[4][16];
    uint8_t huffPtr[4][16];


    uint32_t busReqNum;
    uint32_t compNum;
    uint32_t mcuBlockNum;
    uint32_t compInfo[3];
    uint32_t mcuWidth;
    uint32_t mcuHeight;
    jpu_getbit_context_t gbc;   // bit buffer

} JPU_DEC_INFO;



// Encoder
typedef struct
{
    // image
    uint8_t *data;        /**< Data to be encoded, should aligned to 8*/
    uint32_t size;        /**< Data size to be encoded*/

    // size
    int picWidth;         /**< Input image width*/
    int picHeight;        /**< Input image height*/

    // format
    int jpgFormat;        /**< Output JPEG image sample rate, see `JPU_JPG_FORMAT`*/
    int frameFormat;      /**< YUV format to be encoded, see `JPU_PACKED_FORMAT`*/
    uint8_t quality;      /**< JPEG image quality: 1 ~ 100*/

    // wrapper
    int useWrapper;       /**< Enable Wrapper: Convert RGB to YUV format before encoding*/
    int rgbType;          /**< Input RGB format: see `JPU_RGB_TYPE`*/

    // buffer
    uint32_t jpg_buff_sz; /**< jpeg image buffer size, if set 0, use default `DEFAULT_JPG_BUFF_SZ`*/

    // rotate
    // int useRot;
    int rotAngle;
    int mirDir;

} JPU_ENC_PARAM;



typedef struct
{
    // image
    uint8_t *data;
    uint32_t size;

    // size
    int picWidth;
    int picHeight;

    // format
    int jpgFormat;        // jpg
    int frameFormat;      // frame
    uint32_t quality;

    // wrapper
    int useWrapper;
    int rgbType;

    int StreamEndian;
    int FrameEndian;
    int chromaInterleave;
    int bEnStuffByte;

    // buffer
    uint32_t buff_sz;

    // rotate
    uint8_t useRot;
    uint32_t rotAngle;
    uint32_t mirDir;

    // partial
    int usePartialMode;
    int partialBufNum;
    int partialHeight;
} JPU_ENC_PARAM_INT;  // internal



typedef struct
{
    uint32_t bufY;
    uint32_t bufCb;
    uint32_t bufCr;

    int alignedWidth;
    int alignedHeight;

    int rstIntval;
    int busReqNum;
    int mcuBlockNum;
    int compNum;
    int compInfo[3];

    int disableAPPMarker;
    int stuffByteEnable;

    uint8_t huffVal[4][162];
    uint8_t huffBits[4][256];
    uint8_t qMatTab[4][64];

    // partial
    int usePartial;
    int partiallineNum;
    int partialBufNum;
} JPU_ENC_INFO;


typedef struct
{
    uint32_t state;          /**< JPU Status, see `JPU_STATUS`*/
    uint32_t cycle;
    uint32_t picWidth;       /**< Input Picture size*/
    uint32_t picHeight;
    uint32_t alignedWidth;   /**< Output Picture size*/
    uint32_t alignedHeight;

    uint8_t *buff_raw;       /**< Dec: RGB/YUV buffer, Enc: JPEG buffer*/
    uint8_t *buff;
    uint32_t buff_size;

    uint16_t err_mcu_x;      /**< Error codec mcu location*/
    uint16_t err_mcu_y;

    uint32_t jpg_format;     /**< JPEG image sample rate, see `JPU_JPG_FORMAT`*/


} JPU_OUTPUT_INFO;



/** End of JPU_Exported_Types
  * \}
  */
/*============================================================================*
  *                         Functions
  *============================================================================*/
/** \defgroup JPU_Exported_Functions JPU Exported Functions
  * \brief
  * \{
  */

/**
 * @brief      Initializes the memory allocation functions used internally by the JPU driver.
 *
 * @param[in]  jmalloc    Pointer to a memory allocation function with the same signature as malloc (i.e., void *jmalloc(size_t size)).
 *                        This function will be used internally for dynamic memory allocation within the JPU driver.
 * @param[in]  jfree      Pointer to a memory free function with the same signature as free (i.e., void jfree(void *ptr)).
 *                        This function will be used internally to release memory allocated by jmalloc.
 *
 * @note       This function must be called before using any JPU encoding or decoding APIs that require dynamic memory allocation.
 *
 * @example
 *     // Example using standard C driver malloc and free:
 *     hal_jpu_mem_init(malloc, free);
 */
void hal_jpu_mem_init(void *(*jmalloc)(size_t), void (*jfree)(void *));

/**
 * @brief      Frees a buffer that was allocated by the JPU driver.
 *
 * @param[in]  ptr    Pointer to the memory buffer to be freed. This buffer should have been previously allocated by the JPU
 *                    driver (e.g., via an encode or decode operation that returns an output pointer).
 *
 * @note       This function is intended to release memory allocated by JPU.
 *
 * @warning    Passing a pointer not allocated by the JPU driver results in undefined behavior.
 *
 * @example
 *     uint8_t *jpeg_data = NULL;
 *     // ... jpeg_data receives memory from a JPU encode/decode API
 *     hal_jpu_free_cache(jpeg_data); // Safe way to clean up JPU-allocated output
 *     jpeg_data = NULL;
 */
void hal_jpu_free_cache(void *ptr);

/**
 * @brief      Removes a buffer pointer from the JPU internal allocation or tracking record without freeing memory.
 *
 * @param[in]  ptr    Pointer to the buffer that should be removed from JPU's internal record.
 *                    This buffer should have been previously registered or tracked by the JPU driver.
 *
 * @note       This function does not free or deallocate the memory associated with `ptr`.
 *             It simply removes `ptr` from the JPU's internal record. You should call this
 *             function when the frame buffer will no longer be managed by the JPU driver,
 *             but the buffer itself is still needed elsewhere in your application.
 *             Typically used when the application takes over ownership of the frame buffer.
 *
 * @warning    Only use this function for pointers that have been tracked by the JPU.
 *             Passing an unknown or already cleaned pointer may cause undefined behavior.
 *
 * @example
 *     void *fb = NULL;
 *     // ... fb gets allocated by or registered with JPU ...
 *     // When you want to remove fb from JPU's management but not free it:
 *     hal_jpu_fb_clean(fb);
 *     // Now fb is not tracked by JPU anymore, but app can still use or free it as needed.
 */
void hal_jpu_clean_buffer(void *ptr);


/**
 * @brief      Retrieves the original raw address of a memory buffer as allocated by malloc,
 *             prior to any address alignment or offset adjustment for JPU usage.
 *
 * @param[in]  ptr    The aligned buffer pointer currently in use (as returned or managed by JPU).
 *
 * @return     The raw, unaligned memory address originally returned by malloc when the buffer was allocated.
 *             If `ptr` is invalid or not recognized, returns NULL.
 *
 * @details
 *             - In many systems, memory buffers for hardware accelerators (such as JPU) need to be aligned
 *               to specific boundaries. Thus, extra memory is often allocated, and the aligned pointer exposed to
 *               user/JPU APIs is at an offset from the true malloc base address.
 *             - This function allows you to retrieve the unaligned (original) malloc pointer associated with
 *               the given buffer. This is often necessary for proper memory deallocation (e.g., with free())
 *               or for advanced memory management tasks.
 *
 * @usage
 *             - Use the returned raw pointer to free memory. **Do not free the aligned pointer directly.**
 *             - **If the raw buffer has been freed externally, you must call `void hal_jpu_clean_buffer(ptr)` to
 *               remove JPU's internal record for this buffer**, otherwise it may cause resource leaks or undefined behavior.
 *
 * @note
 *             - The returned pointer should be used when freeing the buffer, not the aligned pointer.
 *             - Make sure the input `ptr` is a valid aligned buffer obtained from JPU allocation routines.
 *
 * @example
 *    void *aligned_jpu_buf;
 *    // Use aligned_jpu_buf with JPU...
 *    // On cleanup:
 *    void *raw_buf = hal_jpu_get_raw_buffer(aligned_jpu_buf);
 *    free(raw_buf);                                  // 1. Free raw buffer
 *    hal_jpu_clean_buffer(aligned_jpu_buf);          // 2. Notify JPU to clean internal record!
 */
void *hal_jpu_get_raw_buffer(void *ptr);


/**
 * @brief      Configures the size of a custom header to be reserved before each decoded frame buffer.
 *
 * @param[in]  size    The size in bytes to reserve for the custom header before each decoded frame buffer.
 *                     If set to 0, no header will be reserved (default behavior).
 *
 * @details
 *             - By default, no memory is reserved for a header (header size is 0).
 *             - Calling this function sets or overrides the header size. The new value persists and will be applied
 *               to all subsequently allocated decode frame buffers.
 *             - The user is responsible for filling and managing the header data; the JPU driver will only reserve
 *               the space and not initialize or use it.
 *             - This function can be called multiple times to change the header size as needed. Changes only affect
 *               buffers allocated after the call; already allocated buffers retain their previously defined layout.
 *
 * @note
 *             Make sure to call this function before any frame buffer allocations for the change to take effect.
 *             Changing the header size does not retroactively affect buffers that have already been allocated.
 *
 * @example
 *     // No header is reserved by default.
 *
 *     // Reserve a 8-byte header area before each frame buffer
 *     hal_jpu_add_dec_header(8);
 *
 *     // Later, change the header size to 12 bytes for new allocations
 *     hal_jpu_add_dec_header(12);
 *
 *     // Reset to no header space
 *     hal_jpu_add_dec_header(0);
 */
void hal_jpu_add_dec_header(uint32_t size);


/**
 * @brief      Decodes a JPEG image using the hardware JPEG decoder (JPU).
 *
 * @param[in]  dec_param    Pointer to the decoding parameter structure containing JPEG bitstream and decoding settings.
 * @param[out] frame_buff   Pointer to a pointer that will be set to a buffer allocated internally by the function,
 *                          containing the decoded raw image data (such as YUV, RGB, etc. format depending on settings).
 *                          The caller is responsible for releasing this buffer when it is no longer needed.
 * @param[out] frame_size   Pointer to a variable that will receive the number of bytes written to the output buffer.
 * @param[out] dec_w        Pointer to a variable that will receive the width (in pixels) of the decoded image.
 * @param[out] dec_h        Pointer to a variable that will receive the height (in pixels) of the decoded image.
 *
 * @return     JPU_ERROR code indicating success or failure of the decoding operation.
 *
 * @note       The output buffer is dynamically allocated within this function.
 *             The caller must free the buffer after use to prevent memory leaks.
 *             All output parameters are valid only if the function returns a success code.
 *
 * @example
 *      JPU_DEC_PARAM dec_param;
 *      uint8_t *fb_data = NULL;
 *      uint32_t fb_size = 0, w = 0, h = 0;
 *      JPU_ERROR err;
 *
 *      memset(&dec_param, 0, sizeof(JPU_DEC_PARAM));
 *      dec_param.data = img_data;
 *      dec_param.size = f_sz;
 *      dec_param.frameFormat = PACKED_FORMAT_422_YUYV;
 *      dec_param.useWrapper = 1;
 *      dec_param.rgbType = JPU_RGB565;
 *
 *      hal_jpu_mem_init(lv_malloc, lv_free);
 *      err = hal_jpu_decode(&dec_param, &fb_data, &fb_size, &w, &h);
 *      if (err != JPU_SUCCESS)
 *      {
 *          DBG_DIRECT("decode jpeg file failed, err: %d", err);
 *          while(1);
 *      }
 *
 *
 *      // After usage, remember to free the buffer
 *      hal_jpu_free_cache(fb_data);
*/
JPU_ERROR hal_jpu_decode(JPU_DEC_PARAM *dec_param, uint8_t **frame_buff, uint32_t *frame_size,
                         uint32_t *dec_w, uint32_t *dec_h);


/**
 * @brief      Encodes a frame using the hardware JPEG encoder (JPU).
 * @param[in]  enc_param    Pointer to the encoding parameter structure containing source image and encoding settings.
 * @param[out] jpg_buff     Pointer to a pointer that will be set to a buffer allocated internally by the function,
 *                          containing the encoded JPEG bitstream.
 *                          The caller is responsible for releasing this buffer when it is no longer needed.
 * @param[out] jpg_size     Pointer to a variable that will receive the number of bytes written to the output buffer.
 * @param[out] enc_w        Pointer to a variable that will receive the width (in pixels) of the encoded image.
 * @param[out] enc_h        Pointer to a variable that will receive the height (in pixels) of the encoded image.
 *
 * @return     JPU_ERROR code indicating success or failure of the encoding operation.
 *
 * @note       The output buffer is dynamically allocated within this function. The caller must free the buffer after use
 *             to prevent memory leaks. All output parameters are valid only if the function returns a success code.
 * @example
 *      JPU_ENC_PARAM enc_param;
 *      uint8_t *jpg_data = NULL;
 *      uint32_t jpg_size = 0;
 *      uint32_t w = 0;
 *      uint32_t h = 0;
 *      JPU_ERROR err;
 *
 *      memset((void *)&enc_param, 0, sizeof(JPU_ENC_PARAM));
 *      enc_param.data = (uint8_t *)fb;
 *      enc_param.size = (MY_DISP_HOR_RES * MY_DISP_VER_RES * LV_COLOR_DEPTH / 8);
 *      enc_param.picWidth = MY_DISP_HOR_RES ;
 *      enc_param.picHeight = MY_DISP_VER_RES;
 *      enc_param.jpgFormat = JPU_FORMAT_422;
 *      enc_param.frameFormat = PACKED_FORMAT_422_YUYV;
 *      enc_param.quality = 50;
 *      enc_param.useWrapper = 1;
 *      enc_param.rgbType = JPU_RGB565;
 *      enc_param.jpg_buff_sz = 30 * 1024;
 *
 *      hal_jpu_mem_init(lv_malloc, lv_free);
 *      err = hal_jpu_encode(&enc_param, &jpg_data, &jpg_size, &w, &h);
 *      if (err != JPU_SUCCESS)
 *      {
 *          DBG_DIRECT("enc jpeg file failed, err: %d", err);
 *          while(1);
 *      }
 *
 *
 *      // After usage, remember to free the buffer
 *      hal_jpu_free_cache(jpeg_data);
*/
JPU_ERROR hal_jpu_encode(JPU_ENC_PARAM *enc_param, uint8_t **jpg_buff, uint32_t *jpg_size,
                         uint32_t *enc_w, uint32_t *enc_h);


/**
 * @brief      Retrieves the current output information from the JPU, including decoding or encoding status,
 *             frame properties, and other state information.
 *
 * @return     A pointer to a `JPU_OUTPUT_INFO` structure containing up-to-date information about the most recent
 *             JPU operation.
 *
*/
JPU_OUTPUT_INFO *hal_jpu_get_info(void);
/** End of JPU_Exported_Functions
  * \}
  */

/** End of JPU
  * \}
  */

#ifdef __cplusplus
}
#endif

#endif /* RTL_HAL_JPU_H */
