/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* \file     rtl_hal_jpu.h
* \brief    This file provides all the JPU firmware functions.
* \details  This module implements the JPEG Processing Unit (JPU) driver,
*           supporting various JPEG image including RLE, FastLZ,
*           and YUV sampling with blur effects for efficient image processing.
* \author   roy xie
* \date     2026-03-24
* \version  v1.0
*********************************************************************************************************
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

#ifdef __WIN32
// #define JPU_MALLOC          malloc
// #define JPU_FREE            free
#else
// #define JPU_MALLOC          malloc
// #define JPU_FREE            free
#endif

#define DEFAULT_JPG_BUFF_SZ  (40 * 1024)

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
    JPU_ERR_BIT_EMPTY,
    JPU_ERR_EOS,
    JPU_ERR_INVALID_HANDLE,
    JPU_ERR_INVALID_PARAM,
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

    JPU_ERR_MALLOC_FAIL,
    JPU_ERR_DECODE,
    JPU_ERR_ENCODE,
} JPU_ERROR;

/** End of JPU_ERROR
  * \}
  */




enum
{
    JPU_Marker          = 0xFF,
    JPU_FF_Marker       = 0x00,

    JPU_SOI_Marker      = 0xFFD8,           // Start of image
    JPU_EOI_Marker      = 0xFFD9,           // End of image

    JPU_JFIF_CODE       = 0xFFE0,           // Application  JFIF(JPEG File Interchange Format): jpeg info
    JPU_EXIF_CODE       = 0xFFE1,                   // Exif(Exchangeable Image File Format): camera info

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
    uint8_t *data;
    uint32_t size;

    //packed
    uint32_t packedFormat;

    //ROI
    uint32_t roiEnable;
    uint32_t roiWidth;
    uint32_t roiHeight;
    uint32_t roiOffsetX;
    uint32_t roiOffsetY;

    //wrapper
    uint32_t useWrapper;
    uint32_t rgbType;
    uint8_t opacity;

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

    // buffer
    uint32_t jpg_buff_sz;

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
    // decoder
    uint32_t format;            // decode from header
    uint32_t picWidth;
    uint32_t picHeight;
    uint32_t alignedWidth;
    uint32_t alignedHeight;
    uint8_t *fb_raw;
    uint8_t *fb;
    uint32_t fb_size;

    // encoder


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

void hal_jpu_mem_init(void *(*jmalloc)(size_t), void (*jfree)(void *));

void hal_jpu_free_cache(void *ptr);

void hal_jpu_add_dec_header(uint32_t size);

void hal_jpu_fb_free(void);

JPU_ERROR hal_jpu_decode(JPU_DEC_PARAM *dec_param, uint8_t **output, uint32_t *output_size);


void hal_jpu_jpg_buf_free(void);

JPU_ERROR hal_jpu_encode(JPU_ENC_PARAM *enc_param, uint8_t **output, uint32_t *wr_size);

JPU_OUTPUT_INFO *hal_jpu_get_output(void);
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

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/