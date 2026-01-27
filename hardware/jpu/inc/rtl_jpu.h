/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_JPU_H
#define RTL_JPU_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/

#include "rtl_jpu_def.h"
#include <stdbool.h>

#ifdef __arm__
#include "rtl876x.h"
#include "rtl876x_rcc.h"
#endif


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

/** \defgroup JPU_Declaration JPU Declaration
  * \{
  * \ingroup  JPU_Exported_Constants
  */

#define JPU                                ((JPU_TypeDef *)JPU_REG_BASE)
#define JPU_WRAP                           ((JPU_WRAPPER_TypeDef *)JPU_WRAPPER_REG_BASE)


#define DC_TABLE_INDEX0             0
#define AC_TABLE_INDEX0             1
#define DC_TABLE_INDEX1             2
#define AC_TABLE_INDEX1             3

#define Q_COMPONENT0                0
#define Q_COMPONENT1                0x40
#define Q_COMPONENT2                0x80

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
    JPU_FORMAT_420 = 0,
    JPU_FORMAT_422 = 1,
    JPU_FORMAT_224 = 2,
    JPU_FORMAT_444 = 3,
    JPU_FORMAT_400 = 4
} JPU_JPG_FORMAT;   // format of jpeg

typedef enum
{
    PACKED_FORMAT_NONE = 0x0,
    PACKED_FORMAT_422_YUYV = 0x4,
    PACKED_FORMAT_422_UYVY = 0x5,
    PACKED_FORMAT_422_YVYU = 0x6,
    PACKED_FORMAT_422_VYUY = 0x7,
    PACKED_FORMAT_444      = 0x8
} JPU_PACKED_FORMAT; // format of yuv/rgb

typedef enum
{
    JPU_BBC_64_BIT_LITTLE_ENDIAN  = 0x0,
    JPU_BBC_64_BIT_BIG_ENDIAN     = 0x1,
    JPU_BBC_32_BIT_LITTLE_ENDIAN  = 0x2,
    JPU_BBC_32_BIT_BIG_ENDIAN     = 0x3,
} JPU_BBC_ENDIAN;

typedef enum
{
    JPU_DPB_64_BIT_BIG_ENDIAN     = 0x0,
    JPU_DPB_64_BIT_LITTLE_ENDIAN  = 0x1,
    JPU_DPB_32_BIT_BIG_ENDIAN     = 0x2,
    JPU_DPB_32_BIT_LITTLE_ENDIAN  = 0x3,
} JPU_DPB_ENDIAN;

typedef enum
{
    JPU_SEPARATED_CHROMA  = 0,
    JPU_CBCR_INTERLEAVED,
    JPU_CRCB_INTERLEAVED
} JPU_CHROMA_INTERLEAVE;

typedef enum
{
    JPU_STUFFING_BYTE_FF = 0,
    JPU_STUFFING_BYTE_00
} JPU_STUFFING_BYTE;



typedef enum
{
    JPG_TBL_NORMAL,
    JPG_TBL_MERGE
} JPU_TableMode;

// scale down factor
typedef enum
{
    JPU_DOWN_SCALE_NONE    = 0,
    JPU_DOWN_SCALE_1_2     = 1,
    JPU_DOWN_SCALE_1_4     = 2,
    JPU_DOWN_SCALE_1_8     = 3
} JPU_DOWN_SCALE_FACTOR;

// wrapper output rgb
typedef enum
{
    JPU_ARGB8888   = 0,
    JPU_RGB888     = 1,
    JPU_RGB565     = 2
} JPU_RGB_TYPE;


/** End of JPU_Exported_Constants
  * \}
  */

/*============================================================================*
 *                             Types
 *============================================================================*/

typedef struct
{
    JPU_PIC_STATUS_TypeDef status;
    JPU_PIC_ERRMB_TypeDef dec_errmb;
    uint32_t cycle;
    uint32_t strmWrPtr;

} JPU_HW_INFO;


typedef struct
{
    // bit stream
    uint8_t *bs_buffer_start;
    uint8_t *bs_buffer_end;     // size: N * 1024
    uint8_t *bs_rd_ptr;
    uint8_t *bs_wr_ptr;
    uint8_t strm_end_flg;       // stream smaller than buffer

    // ctrl
    uint32_t ecsPtr;
    uint32_t pagePtr;
    uint32_t wordPtr;
    uint32_t bitPtr;


    // huff table
    uint32_t huffDcIdx;
    uint32_t huffAcIdx;
    uint32_t userHuffTab;
    uint32_t huffMin[4][16];
    uint32_t huffMax[4][16];
    uint8_t huffPtr[4][16];
    uint8_t huffVal[4][162];
    uint8_t huffBits[4][256];
    uint8_t cInfoTab[4][6];   // Component 0 compID, 1 hSampFact, 2 vSampFact,
    // 3 Quantization table ID, 4 dcHufTblIdx, 5 acHufTblIdx
    uint8_t qMatTab[4][64];   // Quantization table

    // image info
    uint32_t format;            // decode from header
    uint32_t alignedWidth;
    uint32_t alignedHeight;
    uint32_t stride;
    uint32_t rstIntval;

    uint32_t compNum;
    uint32_t mcuBlockNum;
    uint32_t compInfo[3];
    uint32_t busReqNum;

    uint32_t req_packed_format;   // request output format by user
    uint32_t streamEndian;
    uint32_t frameEndian;
    uint32_t chromaInterleave;

    // roi
    uint32_t roiEnable;
    uint32_t roiOffsetX;
    uint32_t roiOffsetY;
    uint32_t roiMcuWidth;
    uint32_t roiMcuHeight;
    uint32_t mcuWidth;
    uint32_t mcuHeight;


    // scaler
    uint32_t iHorScaleFactor;
    uint32_t iVerScaleFactor;

    // rotator, mirror
    uint32_t rotationEnable;
    uint32_t rotMir;
    uint32_t rotatorStride;

    // frame buffer
    uint32_t bufY;
    uint32_t bufCb;
    uint32_t bufCr;

    // wrapper
    uint8_t wrapper_en;
    uint8_t rgb_type;
    uint8_t opa;


    // partial
    uint8_t usePartial;
    uint32_t lineNum;
    uint32_t bufNum;

} JPU_DEC_CFG;


typedef struct
{
    // bit stream
    uint8_t *bs_buffer_start;
    uint8_t *bs_buffer_end;     // size: N * 1024
    uint8_t *bs_rd_ptr;
    uint8_t *bs_wr_ptr;
    // uint8_t strm_end_flg;       // stream smaller than buffer

    // rotator, mirror
    uint32_t rotMirEnable;
    uint32_t rotMirMode;
    uint32_t rotationAngle;

    //
    uint32_t frame_packed_format;   // input format by user
    uint32_t streamEndian;
    uint32_t frameEndian;
    uint32_t chromaInterleave;

    uint32_t format;
    uint32_t busReqNum;
    uint32_t picWidth;
    uint32_t picHeight;
    uint32_t alignedWidth;
    uint32_t alignedHeight;
    uint32_t stride;
    uint32_t mcuBlockNum;
    uint32_t compNum;
    uint32_t compInfo[3];
    uint32_t rstIntval;


    uint8_t *pHuffVal[4];
    uint8_t *pHuffBits[4];
    uint8_t *pCInfoTab[4];
    uint8_t *pQMatTab[4];
    uint32_t huffCode[4][256];
    uint32_t huffSize[4][256];
    int huffSizeTmp[256];
    int huffCodeTmp[256];

    uint32_t stuffByteEnable;



    // frame buffer
    uint32_t bufY;
    uint32_t bufCb;
    uint32_t bufCr;



    // JpgEncParamSet
    uint8_t *paraSet;
    uint8_t *pParaSet;
    uint32_t size;
    uint32_t headerMode;
    uint32_t quantMode;
    uint32_t huffMode;
    uint32_t disableAPPMarker;

    // wrapper
    uint8_t wrapper_en;
    uint8_t rgb_type;

    // partial
    uint8_t usePartial;
    uint32_t lineNum;
    uint32_t bufNum;
} JPU_ENC_CFG;

void JPU_Clk_Init(void);
void JPU_Deinit(void);
void JPU_Reset(void);
JPU_HW_INFO *JPU_Read_Output(void);

uint32_t JPU_Decode(JPU_DEC_CFG *config);
uint32_t JPU_Encode(JPU_ENC_CFG *config);

bool JPU_Busy(void);
void JPU_Dump(void);

/** End of JPU_Exported_Functions
  * \}
  */

/** End of JPU
  * \}
  */

#ifdef __cplusplus
}
#endif

#endif /* RTL_JPU_H */
