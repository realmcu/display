/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_PPE_H
#define RTL_PPE_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "rtl_ppe_reg.h"



/*============================================================================*
 *                         PPE Declaration
 *============================================================================*/
#define PPE_REG_BASE                              0x40090000UL
#define PPE_Result_Layer_BASE                     (PPE_REG_BASE + 0x80)
#define PPE_Input_Layer1_BASE                     (PPE_REG_BASE + 2 * 0x80)
#define PPE_Input_Layer2_BASE                     (PPE_REG_BASE + 3 * 0x80)

#define PPE                                           ((PPE_Typedef *)PPE_REG_BASE)
#define PPE_ResultLayer                               ((PPE_ResultLayer_Typedef *)PPE_Result_Layer_BASE)
#define PPE_InputLayer1                               ((PPE_Input_Layer_Typedef *)PPE_Input_Layer1_BASE)
#define PPE_InputLayer2                               ((PPE_Input_Layer_Typedef *)PPE_Input_Layer2_BASE)

#define PPE_MAX_INPUTLAYER                          0x2
#define PPE_LINKLIST_GLB_REG_NUM                    3UL
#define PPE_LINKLIST_RESULT_REG_NUM                 4UL
#define PPE_LINKLIST_INPUT_REG_NUM                  17UL
#define PPE_LLI_MAX                                 (PPE_LINKLIST_GLB_REG_NUM + PPE_LINKLIST_RESULT_REG_NUM + PPE_LINKLIST_INPUT_REG_NUM * PPE_MAX_INPUTLAYER)

#define PPE_BYTE_SIZE                               8
/** \defgroup 8773G_PPE       PPE
  * \brief
  * \{
  */

/*============================================================================*
 *                         Constants
 *============================================================================*/
/** \defgroup PPE_Exported_Constants PPE Exported Constants
  * \brief
  * \{
  */



/**
 * \cond        private
 * \brief       Constants for internal use
 * \defgroup    PPE_INTERNAL_USE   PPE constants for internal use
 * \{
 */
typedef enum
{
    PPE_INPUT_1 = 0x1,
    PPE_INPUT_2 = 0x2,

} PPE_INPUT_LAYER_INDEX;

typedef enum
{
    PPE_AWBURST_FIXED     = 0X0,
    PPE_AWBURST_INC         = 0X1,
} PPE_AWBURST;

typedef enum
{
    PPE_LAYER_SRC_CONST             = 0X0,
    PPE_LAYER_SRC_FROM_DMA         = 0X1,
} PPE_PIXEL_SOURCE;
typedef enum
{
    PPE_ALL_OVER_INT = 0x0,
    PPE_FRAME_OVER_INT,
    PPE_LOAD_OVER_INT,
    PPE_LINE_OVER_INT,
    PPE_SUSPEND_IACTIVE_INT,
    PPE_SECURE_ERROR_INT,
    PPE_BUS_ERROR_INT = 0x7,
    PPE_DIV0_ERR_INT,
} PPE_INTERRUPT;


typedef enum
{
    PPE_DISABLE = 0x0,
    PPE_ENABLE     = 0x1,
    PPE_SUSPEND_ALL_INA = 0x2,
    PPE_SUSPEND = 0x3,
} PPE_RUN_STATE;
/**
 * \}
 * \endcond
 */

/** \defgroup PPE_COLOR_KEY_MODE PPE Color Key Mode
 * \{
 * \ingroup  PPE_Exported_Constants
 */
typedef enum
{
    PPE_COLOR_KEY_INSIDE    = 0X0,  /*!< Color key inside mode, minimum <= key_value <= maximum. */
    PPE_COLOR_KEY_OUTSIDE   = 0X1,   /*!< Color key outside mode, key_value < minimum or key_value > maximum. */
} PPE_COLOR_KEY_MODE;
/** End of PPE_COLOR_KEY_MODE
  * \}
  */

/** \defgroup PPE_SRC_INTERPOLATION PPE Interpolation Mode
 * \{
 * \ingroup  PPE_Exported_Constants
 */
typedef enum
{
    PPV2_SRC_NEAREST_NEIGHBOUR,   /*!< Nearest neighbour interpolation. */
    PPV2_SRC_BILINEAR,            /*!< Bilinear interpolation. */
} PPE_SRC_INTERPOLATION;
/** End of PPE_SRC_INTERPOLATION
  * \}
  */

/** \defgroup PPE_PIXEL_FORMAT PPE Pixel Format
 * \{
 * \ingroup  PPE_Exported_Constants
 */
typedef enum
{
    PPE_ABGR8888 = 0x0,     /*!< ABGR8888: A(bit 31:24) B(bit 23:16) G(bit 15:8) R(bit 7:0) */
    PPE_ARGB8888,           /*!< ARGB8888: A(bit 31:24) R(bit 23:16) G(bit 15:8) B(bit 7:0) */
    PPE_XBGR8888,           /*!< XBGR8888: X(bit 31:24) B(bit 23:16) G(bit 15:8) R(bit 7:0) */
    PPE_XRGB8888,           /*!< XRGB8888: X(bit 31:24) R(bit 23:16) G(bit 15:8) B(bit 7:0) */
    PPE_BGRA8888,           /*!< BGRA8888: B(bit 31:24) G(bit 23:16) R(bit 15:8) A(bit 7:0) */
    PPE_RGBA8888,           /*!< RGBA8888: R(bit 31:24) G(bit 23:16) B(bit 15:8) A(bit 7:0) */
    PPE_BGRX8888,           /*!< BGRX8888: B(bit 31:24) G(bit 23:16) R(bit 15:8) X(bit 7:0) */
    PPE_RGBX8888,           /*!< RGBX8888: R(bit 31:24) G(bit 23:16) B(bit 15:8) X(bit 7:0) */
    PPE_ABGR4444,           /*!< ABGR4444: A(bit 15:12) B(bit 11:8) G(bit 7:4) R(bit 3:0) */
    PPE_ARGB4444,           /*!< ARGB4444: A(bit 15:12) R(bit 11:8) G(bit 7:4) B(bit 3:0) */
    PPE_XBGR4444,           /*!< XBGR4444: X(bit 15:12) B(bit 11:8) G(bit 7:4) R(bit 3:0) */
    PPE_XRGB4444,           /*!< XRGB4444: X(bit 15:12) R(bit 11:8) G(bit 7:4) B(bit 3:0) */
    PPE_BGRA4444,           /*!< BGRA4444: B(bit 15:12) G(bit 11:8) R(bit 7:4) A(bit 3:0) */
    PPE_RGBA4444,           /*!< RGBA4444: R(bit 15:12) G(bit 11:8) B(bit 7:4) A(bit 3:0) */
    PPE_BGRX4444,           /*!< BGRX4444: B(bit 15:12) G(bit 11:8) R(bit 7:4) X(bit 3:0) */
    PPE_RGBX4444,           /*!< RGBX4444: R(bit 15:12) G(bit 11:8) B(bit 7:4) X(bit 3:0) */
    PPE_ABGR2222,           /*!< ABGR2222: A(bit 7:6) B(bit 5:4) G(bit 3:2) R(bit 1:0) */
    PPE_ARGB2222,           /*!< ARGB2222: A(bit 7:6) R(bit 5:4) G(bit 3:2) B(bit 1:0) */
    PPE_XBGR2222,           /*!< XBGR2222: X(bit 7:6) B(bit 5:4) G(bit 3:2) R(bit 1:0) */
    PPE_XRGB2222,           /*!< XRGB2222: X(bit 7:6) R(bit 5:4) G(bit 3:2) B(bit 1:0) */
    PPE_BGRA2222,           /*!< BGRA2222: B(bit 7:6) G(bit 5:4) R(bit 3:2) A(bit 1:0) */
    PPE_RGBA2222,           /*!< RGBA2222: R(bit 7:6) G(bit 5:4) B(bit 3:2) A(bit 1:0) */
    PPE_BGRX2222,           /*!< BGRX2222: B(bit 7:6) G(bit 5:4) R(bit 3:2) X(bit 1:0) */
    PPE_RGBX2222,           /*!< RGBX2222: R(bit 7:6) G(bit 5:4) B(bit 3:2) X(bit 1:0) */
    PPE_ABGR8565,           /*!< ABGR8565: A(bit 23:16) B(bit 15:11) G(bit 10:5) R(bit 4:0) */
    PPE_ARGB8565,           /*!< ARGB8565: A(bit 23:16) R(bit 15:11) G(bit 10:5) B(bit 4:0) */
    PPE_XBGR8565,           /*!< XBGR8565: X(bit 23:16) B(bit 15:11) G(bit 10:5) R(bit 4:0) */
    PPE_XRGB8565,           /*!< XRGB8565: X(bit 23:16) R(bit 15:11) G(bit 10:5) B(bit 4:0) */
    PPE_BGRA5658,           /*!< BGRA5658: B(bit 23:19) G(bit 18:13) R(bit 12:8) A(bit 7:0) */
    PPE_RGBA5658,           /*!< RGBA5658: R(bit 23:19) G(bit 18:13) B(bit 12:8) A(bit 7:0) */
    PPE_BGRX5658,           /*!< BGRX5658: B(bit 23:19) G(bit 18:13) R(bit 12:8) X(bit 7:0) */
    PPE_RGBX5658,           /*!< RGBX5658: R(bit 23:19) G(bit 18:13) B(bit 12:8) X(bit 7:0) */
    PPE_ABGR1555,           /*!< ABGR1555: A(bit 15) B(bit 14:10) G(bit 9:5) R(bit 4:0) */
    PPE_ARGB1555,           /*!< ARGB1555: A(bit 15) R(bit 14:10) G(bit 9:5) B(bit 4:0) */
    PPE_XBGR1555,           /*!< XBGR1555: X(bit 15) B(bit 14:10) G(bit 9:5) R(bit 4:0) */
    PPE_XRGB1555,           /*!< XRGB1555: X(bit 15) R(bit 14:10) G(bit 9:5) B(bit 4:0) */
    PPE_BGRA5551,           /*!< BGRA5551: B(bit 15:11) G(bit 10:6) R(bit 5:1) A(bit 0) */
    PPE_RGBA5551,           /*!< RGBA5551: R(bit 15:11) G(bit 10:6) B(bit 5:1) A(bit 0) */
    PPE_BGRX5551,           /*!< BGRX5551: B(bit 15:11) G(bit 10:6) R(bit 5:1) X(bit 0) */
    PPE_RGBX5551,           /*!< RGBX5551: R(bit 15:11) G(bit 10:6) B(bit 5:1) X(bit 0) */
    PPE_BGR888,             /*!< BGR888: B(bit 23:16) G(bit 15:8) R(bit 7:0) */
    PPE_RGB888,             /*!< RGB888: R(bit 23:16) G(bit 15:8) B(bit 7:0) */
    PPE_BGR565,             /*!< BGR565: B(bit 15:11) G(bit 10:5) R(bit 4:0) */
    PPE_RGB565,             /*!< RGB565: R(bit 15:11) G(bit 10:5) B(bit 4:0) */
    PPE_A8,                 /*!< A8: A(bit 7:0) */
    PPE_X8,                 /*!< X8: X(bit 7:0) */
    PPE_A4,                 /*!< A4: pixel1(bit7:4) pixel0(bit 3:0) */
    PPE_X4,                 /*!< X4: pixel1(bit7:4) pixel0(bit 3:0) */
    PPE_A2,                 /*!< A2: pixel3(bit7:6) pixel2(bit 5:4) pixel1(bit 3:2) pixel0(bit 1:0)*/
    PPE_X2,                 /*!< X2: pixel3(bit7:6) pixel2(bit 5:4) pixel1(bit 3:2) pixel0(bit 1:0)*/
    PPE_A1,                 /*!< A1: pixel3(bit 3) pixel2(bit 2) pixel1(bit 1) pixel0(bit 0)*/
    PPE_X1,                 /*!< X1: pixel3(bit 3) pixel2(bit 2) pixel1(bit 1) pixel0(bit 0)*/
    PPE_ABGR8666,           /*!< ABGR8666: A(bit 31:24) B(bit 23:18) G(bit 15:10) R(bit 8:2) */
    PPE_ARGB8666,           /*!< ARGB8666: A(bit 31:24) R(bit 23:18) G(bit 15:10) B(bit 8:2) */
    PPE_XBGR8666,           /*!< XBGR8666: X(bit 31:24) B(bit 23:18) G(bit 15:10) R(bit 8:2) */
    PPE_XRGB8666,           /*!< XRGB8666: X(bit 31:24) R(bit 23:18) G(bit 15:10) B(bit 8:2) */
    PPE_BGRA6668,           /*!< BGRA6668: B(bit 31:26) G(bit 23:18) R(bit 15:10) A(bit 7:0) */
    PPE_RGBA6668,           /*!< RGBA6668: R(bit 31:26) G(bit 23:18) B(bit 15:10) A(bit 7:0) */
    PPE_BGRX6668,           /*!< BGRX6668: B(bit 31:26) G(bit 23:18) R(bit 15:10) X(bit 7:0) */
    PPE_RGBX6668,           /*!< RGBX6668: R(bit 31:26) G(bit 23:18) B(bit 15:10) X(bit 7:0) */
    PPE_BGR565_S,           /*!< BGR565: G_high(bit 15:13) R(bit 12:8) B(7:3) G_low(2:0)*/
    PPE_RGB565_S,           /*!< RGB565: G_high(bit 15:13) B(bit 12:8) R(7:3) G_low(2:0)*/
    PPE_I8,                 /*!< I8: I(bit 7:0) */
    PPE_I4,                 /*!< I4: pixel1(bit 3:2) pixel0(bit 1:0) */
    PPE_I2,                 /*!< I2: pixel3(bit7:6) pixel2(bit 5:4) pixel1(bit 3:2) pixel0(bit 1:0)*/
    PPE_I1,                 /*!< I1: pixel3(bit 3) pixel2(bit 2) pixel1(bit 1) pixel0(bit 0) */
    PPE_FORMAT_NOT_SUPPORT = 0xFF    /*!< Format not support */
} PPE_PIXEL_FORMAT;
/** End of PPE_PIXEL_FORMAT
  * \}
  */

/** \defgroup PPE_BLEND_METHOD PPE Blend Method
 * \{
 * \ingroup  PPE_Exported_Constants
 */
typedef enum
{
    PPE_BLEND_SRC,            /*!< Result = S */
    PPE_BLEND_PREMULTIPLY,    /*!< Result = S * Sa + D * (1 - Sa)*/
    PPE_BLEND_SRC_OVER,       /*!< Result = S + D * (1 - Sa) */
    PPE_BLEND_DST_OVER,       /*!< Result = Da * S + D * (1 - Da) */
    PPE_BLEND_SRC_IN,         /*!< Result = Da * S */
    PPE_BLEND_DST_IN,         /*!< Result = Sa * D */
    PPE_BLEND_MULTIPLY,       /*!< Result = (1 - Da) * S + (1 - Sa) * D + S * D / 255 */
    PPE_BLEND_SCREEN,         /*!< Result = S + D + S * D / 255 */
    PPE_BLEND_ADD,            /*!< Result = S + D */
    PPE_BLEND_SUBSTRACT,      /*!< Result = (1 - Sa) * D */
    PPE_BLEND_BYPASS,         /*!< Result = Sa * S */
    PPE_BLEND_CONST_COLOR,    /*!< Replace input image with a constant. */
} PPE_BLEND_METHOD;
/** End of PPE_BLEND_METHOD
  * \}
  */

/** \defgroup PPE_ERR PPE Error Code
  * \{
  * \ingroup  PPE_Exported_Constants
  */
typedef enum
{
    PPE_SUCCESS = 0x0,           /*!< PPE configure and run successfully. */
    PPE_ERR_NULL_TARGET,         /*!< Target is NULL. */
    PPE_ERR_NULL_SOURCE,         /*!< Source is NULL. */
    PPE_ERR_INVALID_MATRIX,      /*!< Matrix is invalid. */
    PPE_ERR_INVALID_RANGE,       /*!< Blend region is invalid. */
    PPE_ERR_INVALID_PARAMETER,   /*!< Input parameter is invalid, such as buffer and color key. */
    PPE_ERR_TIMEOUT,             /*!< PPE engine did not complete within the busy-wait budget. */
} PPE_ERR;
/** End of PPE_ERR
  * \}
  */

/** End of PPE_Exported_Constants
  * \}
  */

/*============================================================================*
 *                         Structures
 *============================================================================*/
/** \defgroup PPE_Exported_Types PPE Exported Types
  * \brief
  * \{
  */

/** \defgroup PPE_COLOR_KEY_STATE PPE Color Key State
  * \{
  * \ingroup  PPE_Exported_Types
  */
typedef struct
{
    union
    {
        uint8_t key_enable;        /*!< General key status. */
        struct
        {
            __IO uint8_t r_en:  1;    /*!< Red channel key status. */
            __IO uint8_t g_en:  1;    /*!< Green channel key status. */
            __IO uint8_t b_en:  1;    /*!< Blue channel key status. */
            __IO uint8_t a_en:  1;    /*!< Alpha channel key status. */
            __IO uint8_t reserved: 4; /*!< Reserved. */
        } channel_en;
    }; /*!< Union of general key status and detail information. */
} PPE_color_key_state;
/** End of PPE_COLOR_KEY_STATE
  * \}
  */

/**
 * \cond        private
 * \brief       Structures for internal use
 * \defgroup    PPE_INTERNAL_STRUCTURE   PPE structures for internal use
 * \{
 */
typedef struct
{
    uint32_t LYR_ENABLE;
    uint32_t LL_CFG;
    uint32_t LLP;
} PPE_LLI_GLB;

typedef struct
{
    uint32_t LYR0_ADDR;
    uint32_t CANVAS_SIZE;
    uint32_t LYR0_PIC_CFG;
    uint32_t BACKGROUND;
} PPE_LLI_RESULT_LAYER;

typedef struct
{
    uint32_t LYRx_ADDR;
    uint32_t LYRx_PIC_SIZE;
    uint32_t LYRx_PIC_CFG;
    uint32_t LYRx_FIXED_COLOR;
    uint32_t LYRx_WIN_MIN;
    uint32_t LYRx_WIN_MAX;
    uint32_t LYRx_KEY_MIN;
    uint32_t LYRx_KEY_MAX;
    uint32_t LYRx_TRANS_MATRIX_E11;
    uint32_t LYRx_TRANS_MATRIX_E12;
    uint32_t LYRx_TRANS_MATRIX_E13;
    uint32_t LYRx_TRANS_MATRIX_E21;
    uint32_t LYRx_TRANS_MATRIX_E22;
    uint32_t LYRx_TRANS_MATRIX_E23;
    uint32_t LYRx_TRANS_MATRIX_E31;
    uint32_t LYRx_TRANS_MATRIX_E32;
    uint32_t LYRx_TRANS_MATRIX_E33;
} PPE_LLI_INPUT_LAYER;

typedef struct
{
    uint32_t                            Layer_Address;
    uint32_t                            Line_Length;
    uint16_t                            Layer_Window_Xmin;
    uint16_t                            Layer_Window_Xmax;
    uint16_t                            Layer_Window_Ymin;
    uint16_t                            Layer_Window_Ymax;
    uint16_t                            Block_Width;
    uint16_t                            Block_Height;
    PPE_PIXEL_FORMAT                    Color_Format;
    PPE_AWBURST                         LayerBus_Inc;
    uint8_t                             AXSize;
    uint8_t                             MAX_AXLEN_LOG;
} PPE_ResultLayer_Init_Typedef;

typedef struct
{
    uint32_t                            Layer_Address;
    uint32_t                            Pic_Height;
    uint32_t                            Pic_Width;
    uint32_t                            Line_Length;
    PPE_PIXEL_SOURCE                  Pixel_Source;
    PPE_PIXEL_FORMAT                  Pixel_Color_Format;
    PPE_SRC_INTERPOLATION             Source_Interpolation;
    PPE_BLEND_METHOD                  Blend_Method;
    uint32_t                            Const_Pixel;
    PPE_COLOR_KEY_MODE                  Color_Key_Mode;
    uint8_t                             Color_Key_MAX_R;
    uint8_t                             Color_Key_MAX_G;
    uint8_t                             Color_Key_MAX_B;
    uint8_t                             Color_Key_MIN_R;
    uint8_t                             Color_Key_MIN_G;
    uint8_t                             Color_Key_MIN_B;
    uint8_t                             Color_Key_Replace_A;
    uint8_t                             Color_Key_Replace_B;
    uint8_t                             Color_Key_Replace_G;
    uint8_t                             Color_Key_Replace_R;
    PPE_color_key_state                 Color_Key_Enable;
    PPE_AWBURST                         LayerBus_Inc;
    FunctionalState                     Cache_Enable;
    uint16_t                            Layer_Window_Xmin;
    uint16_t                            Layer_Window_Xmax;
    uint16_t                            Layer_Window_Ymin;
    uint16_t                            Layer_Window_Ymax;
    uint32_t                            Transfer_Matrix_E11;
    uint32_t                            Transfer_Matrix_E12;
    uint32_t                            Transfer_Matrix_E13;
    uint32_t                            Transfer_Matrix_E21;
    uint32_t                            Transfer_Matrix_E22;
    uint32_t                            Transfer_Matrix_E23;
    uint32_t                            Transfer_Matrix_E31;
    uint32_t                            Transfer_Matrix_E32;
    uint32_t                            Transfer_Matrix_E33;
    uint32_t                           *Index_Table;
} PPE_InputLayer_Init_Typedef;
/**
 * \}
 * \endcond
 */

/** \defgroup PPE_COLOR_KEY_RANGE PPE Color Key Range
  * \{
  * \ingroup  PPE_Exported_Types
  */
typedef struct
{
    uint8_t R_max;        /*!< Maximum value of red channel color key. */
    uint8_t R_min;        /*!< Minimum value of red channel color key. */
    uint8_t G_max;        /*!< Maximum value of green channel color key. */
    uint8_t G_min;        /*!< Minimum value of green channel color key. */
    uint8_t B_max;        /*!< Maximum value of blue channel color key. */
    uint8_t B_min;        /*!< Minimum value of blue channel color key. */
} PPE_color_key_range;
/** End of PPE_COLOR_KEY_RANGE
  * \}
  */

/** \defgroup PPE_COLOR_KEY_REPLACE PPE Color Key Replace
  * \{
  * \ingroup  PPE_Exported_Types
  */
typedef struct
{
    union
    {
        uint32_t key_replace;        /*!< General key replace value in ABGR8888 format. */
        struct
        {
            uint8_t r_replace;        /*!< Red channel replace value. */
            uint8_t g_replace;        /*!< Green channel replace value. */
            uint8_t b_replace;        /*!< Blue channel replace value. */
            uint8_t a_replace;        /*!< Alpha channel replace value. */
        } channel_replace;
    }; /*!< Union of general value to replace in ABGR8888 format and detail information of each channel. */
} PPE_color_key_replace;
/** End of PPE_COLOR_KEY_REPLACE
  * \}
  */

/** \defgroup PPE_COLOR_KEY_CONFIG PPE Color Key Configration
  * \{
  * \ingroup  PPE_Exported_Types
  */
typedef struct
{
    PPE_COLOR_KEY_MODE      key_mode;    /*!< Color key mode. */
    PPE_color_key_state     key_enable;  /*!< Color key enable state. */
    PPE_color_key_range     key_range;   /*!< Color key range. */
    PPE_color_key_replace   key_replace; /*!< Color key replace value. */
} PPE_color_key_config;
/** End of PPE_COLOR_KEY_REPLACE
  * \}
  */

/** \defgroup PPE_MATRIX PPE Matrix
  * \{
  * \ingroup  PPE_Exported_Types
  */
typedef struct
{
    float m[3][3];    /*! The 3x3 matrix in [row][column] order. */
} ppe_matrix_t;
/** End of PPE_MATRIX
  * \}
  */

/** \defgroup PPE_RECT PPE Rectangle
  * \{
  * \ingroup  PPE_Exported_Types
  */
typedef struct
{
    int32_t x1;     /*!< Minimum x coordinate. */
    int32_t y1;     /*!< Minimum y coordinate. */
    int32_t x2;     /*!< Maximum x coordinate. */
    int32_t y2;     /*!< Maximum y coordinate. */
} ppe_rect_t;
/** End of PPE_RECT
  * \}
  */

/** \defgroup PPE_POINT PPE Point
  * \{
  * \ingroup  PPE_Exported_Types
  */
typedef struct
{
    int x;        /*!< The x coordinate of the point. */
    int y;        /*!< The y coordinate of the point. */
}
ppe_point_t;
/** End of PPE_POINT
  * \}
  */

/** \defgroup PPE_POINTS PPE Points of Polygon
  * \{
  * \ingroup  PPE_Exported_Types
  */
typedef ppe_point_t ppe_point4_t[4]; /* Four 2D Point that form a polygon */
/** End of PPE_POINTS
  * \}
  */

/** \defgroup PPE_BUFFER PPE Layer Buffer
  * \{
  * \ingroup  PPE_Exported_Types
  */
typedef struct
{
    uint32_t address;     /* Address of buffer memory. */
    uint16_t width;       /* Width of buffer in pixel. */
    uint16_t height;      /* Height of buffer in pixel. */
    uint16_t stride;      /* Stride of buffer. */
    uint32_t const_color; /* Constant color for blending. */
    uint16_t win_x_min;   /* Horizontal minimum coordinate of buffer. */
    uint16_t win_x_max;   /* Horizontal Maximum coordinate of buffer. */
    uint16_t win_y_min;   /* Vertical minimun coordinate of buffer. */
    uint16_t win_y_max;   /* Vertical Maximum coordinateof buffer. */
    PPE_color_key_config color_key_config;   /* Color key configuration. */
    PPE_PIXEL_FORMAT format; /* Pixel format of buffer. */
    uint8_t opacity;       /* Opacity of buffer. */
    bool high_quality;    /* Anti-Aliasing with bilinear interpoaltion. */
} ppe_buffer_t;

/** End of PPE_BUFFER
  * \}
  */

/** End of PPE_Exported_Types
  * \}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** \defgroup PPE_Exported_Functions PPE Exported Functions
  * \brief
  * \{
  */

/**
 * \cond        private
 * \brief       Functions for internal use
 * \defgroup    PPE_INTERNAL_FUNCTION   PPE functions for internal use
 * \{
 */
void PPE_CLK_ENABLE(FunctionalState NewState);

void PPE_ResultLayer_Init(PPE_ResultLayer_Init_Typedef *PPE_ResultLyaer_Init_Struct);

void PPE_InputLayer_Init(PPE_INPUT_LAYER_INDEX intput_layer_index,
                         PPE_InputLayer_Init_Typedef *PPE_InputLayer_Init_Struct);

void PPE_ResultLayer_StructInit(PPE_ResultLayer_Init_Typedef *PPE_ResultLyaer_Init_Struct);

void PPE_InputLayer_StructInit(PPE_INPUT_LAYER_INDEX intput_layer_index,
                               PPE_InputLayer_Init_Typedef *PPE_InputLayer_Init_Struct);

void PPE_Cmd(FunctionalState NewState);

void PPE_InputLayer_enable(PPE_INPUT_LAYER_INDEX intput_layer_index, FunctionalState NewState);

FunctionalState PPE_Get_Interrupt_Status(PPE_INTERRUPT PPE_int);

FunctionalState PPE_Get_Raw_Interrupt_Status(PPE_INTERRUPT PPE_int);

void PPE_Clear_Interrupt(PPE_INTERRUPT PPE_int);

void PPE_Mask_Interrupt(PPE_INTERRUPT PPE_int, FunctionalState NewState);

void PPE_Mask_All_Interrupt(FunctionalState NewState);

void PPE_CLK_ENABLE_IN_DLPS(FunctionalState NewState);

PPE_ERR PPE_buffer_init(ppe_buffer_t *buffer);

PPE_ERR PPE_set_color_key(ppe_buffer_t *image, PPE_color_key_config *config);

PPE_ERR PPE_Blend_Handshake(ppe_buffer_t *dst, ppe_buffer_t *src, ppe_rect_t *rect);

void ppe_perspective(float px, float py, ppe_matrix_t *matrix);
/**
 * \}
 * \endcond
 */

/**
 * \brief  Blend source image onto target buffer with inverse matrix, this function usually has higher performance than @ref PPE_Blit.
 * \param[in] src: Source image buffer to be blended.
 * \param[in] dst: Target image buffer to be blended onto.
 * \param[in] inv: Inverse transformation matrix of source image.
 * \param[in] rect: Rectangle of blending area.
 * \param[in] blend_mode: Blend mode from @ref PPE_BLEND_MODE to be used.
 * \return Operation result.
 * \retval PPE_SUCCESS  Operation success.
 * \retval Others       Operation failure, cause refers to @ref PPE_ERR .
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);
        ppe_buffer_t image, buffer;
        image.width = 25;
        image.height = 25;
        image.stride = 32;
        image.format = PPE_RGB888;
        image.address = (uint32_t)address1;
        image.color_key_enable = PPE_COLOR_KEY_DISABLE;
        image.opacity = 255; //opaque
        image.high_quality = true; //anti-aliasing with bilinear interpolation
        image.win_x_min = 0;
        image.win_x_max = 25;
        image.win_y_min = 0;
        image.win_y_max = 25;

        buffer.format = PPE_RGB565;
        buffer.address = (uint32_t)address2;
        buffer.width = 64;
        buffer.height = 64;
        buffer.stride = 64;
        buffer.win_x_min = 0;
        buffer.win_x_max = 64;
        buffer.win_y_min = 0;
        buffer.win_y_max = 64;
        ppe_matrix_t matrix;
        ppe_get_identity(&matrix);
        ppe_rect_t src_rect = {.x = 0, .y = 0, .w = 25, .h = 25};
        ppe_rect_t dst_rect = {0}
        ppe_get_area(&dst_rect, &src_rect, &matrix);
        PPE_Blit_Inverse(&buffer, &image, &matrix, &dst_rect, PPE_SRC_OVER_MODE);
        PPE_Finish(); //wait for PPE to finish blending
    }
 * \endcode
 */
PPE_ERR PPE_Blit_Inverse(ppe_buffer_t *dst, ppe_buffer_t *src, uint8_t *output,
                         ppe_matrix_t *inverse,
                         ppe_rect_t *rect, PPE_BLEND_METHOD method);

/**
 * \brief  Wait for PPE to finish blending.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        //PPE initialization
        PPE_Finish();
    }
 * \endcode
*/
void PPE_Finish(void);

/**
 * \brief  Get pixel size of specified format.
 * \param[in] format: Pixel format of PPE from @ref PPE_PIXEL_FORMAT .
 * \return Pixel size in bits.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        uint8_t pixel_size_in_byte = PPE_Get_Pixel_Size(PPE_RGB565) / 8;
    }
 * \endcode
 */
uint8_t PPE_Get_Pixel_Size(PPE_PIXEL_FORMAT format);

/**
 * \brief  Initialize the input matrix as identity matrix.
 * \param[in] matrix: Pointer to matrix to be initialized.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ppe_matrix_t matrix;
        ppe_get_identity(&matrix);
    }
 * \endcode
 */
void ppe_get_identity(ppe_matrix_t *matrix);

/**
 * \brief  Do translation on input matrix.
 * \param[in] x: Horizontal coordinate to be translated.
 * \param[in] y: Vertical coordinate to be translated.
 * \param[in] matrix: Pointer to matrix to be transformed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ppe_matrix_t matrix;
        ppe_get_identity(&matrix);
        ppe_translate(10, 10, &matrix);
    }
 * \endcode
 */
/**
 * \brief  Do translation on input matrix.
 * \param[in] x: Horizontal coordinate to be translated.
 * \param[in] y: Vertical coordinate to be translated.
 * \param[in] matrix: Pointer to matrix to be transformed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ppe_matrix_t matrix;
        ppe_get_identity(&matrix);
        ppe_translate(10, 10, &matrix);
    }
 * \endcode
 */
void ppe_translate(float x, float y, ppe_matrix_t *matrix);

/**
 * \brief  Do scale transformation on input matrix.
 * \param[in] scale_x: Horizontal scale ratio.
 * \param[in] scale_y: Vertical scale ratio.
 * \param[in] matrix: Pointer to matrix to be transformed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ppe_matrix_t matrix;
        ppe_get_identity(&matrix);
        ppe_scale(0.5, 0.5, &matrix);
    }
 * \endcode
 */
void ppe_scale(float scale_x, float scale_y, ppe_matrix_t *matrix);

/**
 * \brief  Do rotation on input matrix.
 * \param[in] degrees: Angle to be rotated.
 * \param[in] matrix: Pointer to matrix to be rotated.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ppe_matrix_t matrix;
        ppe_get_identity(&matrix);
        ppe_rotate(45, &matrix);
    }
 * \endcode
 */
void ppe_rotate(float degrees, ppe_matrix_t *matrix);

/**
 * \brief  Do skew transformation on input matrix.
 * \param[in] skew_x: Horizontal skew angle.
 * \param[in] skew_y: Vertical skew angle.
 * \param[in] matrix: Pointer to matrix to be skewed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ppe_matrix_t matrix;
        ppe_get_identity(&matrix);
        ppe_skew(45, 45, &matrix);
    }
 * \endcode
 */
void ppe_skew(float skew_x, float skew_y, ppe_matrix_t *matrix);

/**
 * \brief  Do reflect transformation on input matrix.
 * \param[in] horizontal: Horizontal reflection.
 * \param[in] vertical: Vertical reflection.
 * \param[in] matrix: Pointer to matrix to be skewed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ppe_matrix_t matrix;
        ppe_get_identity(&matrix);
        ppe_reflect(true, false, &matrix); //reflect horizontally
    }
 * \endcode
 */
void ppe_reflect(bool horizontal, bool vertical, ppe_matrix_t *matrix);

/**
 * \brief  Get matrix from 4 corners of image and output region.
 * \param[in] src: Points of 4 corners of input image.
 * \param[in] dst: Points of 4 corners of output region.
 * \param[in] mat: Pointer to the matrix storing result.
 * \return Operation result.
 * \retval 0          Successfully find the matrix.
 * \retval -1         Input parameter is invalid.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ppe_matrix_t matrix;
        ppe_point4_t src = {{0,0},{100,0},{0,100},{100,100}};
        ppe_point4_t dst = {{-10,-10},{150,27},{-5,80},{130,99}};
        ppe_get_transform_matrix(src, dst, &matrix);
    }
 * \endcode
 */
int  ppe_get_transform_matrix(ppe_point4_t src, ppe_point4_t dst, ppe_matrix_t *mat);

/**
 * \brief  Inverse the input matrix.
 * \param[in] skew_x: Pointer to the input matrix.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ppe_matrix_t matrix;
        ppe_get_identity(&matrix);
        ppe_skew(45, 45, &matrix);
        ppe_matrix_inverse(&matrix);
    }
 * \endcode
 */
void ppe_matrix_inverse(ppe_matrix_t *matrix);

/**
 * \brief  Multiply 2 matrices.
 * \param[in] matrix: Pointer to one of the input matrix, this matrix stores result.
 * \param[in] mult: Pointer to the other input matrix.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ppe_matrix_t matrix, mult;
        ppe_get_identity(&matrix);
        ppe_skew(45, 45, &matrix);
        ppe_get_identity(&mult);
        ppe_translate(100, 100, &mult);
        ppe_mat_multiply(&matrix, &mult);
    }
 * \endcode
 */
void ppe_mat_multiply(ppe_matrix_t *matrix, ppe_matrix_t *mult);

/**
 * \brief  Find the output region generated by input region and input matrix.
 * \param[in] result_rect: Points of output region.
 * \param[in] source_rect: Points of input region.
 * \param[in] matrix: Pointer to the input matrix.
 * \param[in] buffer: Pointer to a PPE buffer that stores boundary information.
 * \return Operation result.
 * \retval true       Successfully find the output region.
 * \retval false      Input region or input matrix is not valid.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ppe_matrix_t matrix;
        ppe_point4_t src = {{0,0},{100,0},{0,100},{100,100}};
        ppe_matrix_identity(&matrix);
        ppe_skew(45, 45, &matrix);
        ppe_get_area(&result_rect, &src, &matrix, NULL);
    }
 * \endcode
 */
bool ppe_get_area(ppe_rect_t *result_rect, ppe_rect_t *source_rect, ppe_matrix_t *matrix,
                  ppe_buffer_t *buffer);

/**
 * \brief  Check if input matrix has shape transform.
 * \param[in] matrix: Pointer to the input matrix.
 * \return Operation result.
 * \retval true       Input matrix is transformed.
 * \retval false      Input matrix is not transformed.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        //generate a matrix
        if(!ppe_matrix_is_complex(&matrix)){
            //handle simple case
        }
    }
 * \endcode
 */
bool ppe_matrix_is_complex(ppe_matrix_t *mat);

/**
 * \brief  Mask the buffer with certain color in ABGR8888 format.
 * \param[in] dst: Target buffer to be cleared.
 * \param[in] color: Specified color in ABGR8888 format \ref PPE_ABGR8888
 * \param[in] rect: Rectangle region of masking area.
 * \return Operation result.
 * \retval PPE_SUCCESS   Operation success.
 * \retval Others        Operation failure, cause refers to @ref PPE_ERR .
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);
        ppe_buffer_t buffer;

        buffer.format = PPE_RGB565;
        buffer.address = (uint32_t)PIC_OUTPUT_LAYER;
        buffer.width = 64;
        buffer.height = 64;
        buffer.stride = 64;
        buffer.win_x_min = 0;
        buffer.win_x_max = 64;
        buffer.win_y_min = 0;
        buffer.win_y_max = 64;
        ppe_rect_t rect = {0, 0, 64, 64};
        PPE_Mask(&buffer, 0x80808080, &rect);
        PPE_Finish(); //wait for PPE to finish blending
    }
 * \endcode
 */
PPE_ERR PPE_Mask(ppe_buffer_t *dst, uint32_t color, ppe_rect_t *rect);

/**
 * \brief  Clear the buffer with certain color in ABGR8888 format, no blending is performed.
 * \param[in] dst: Target buffer to be cleared.
 * \param[in] color: Specified color in ABGR8888 format \ref PPE_ABGR8888 .
 * \param[in] rect: Rectangle region of clear area.
 * \return Operation result.
 * \retval PPE_SUCCESS   Operation success.
 * \retval Others        Operation failure, cause refers to @ref PPE_ERR .
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);
        ppe_buffer_t buffer;

        buffer.format = PPE_RGB565;
        buffer.address = (uint32_t)PIC_OUTPUT_LAYER;
        buffer.width = 64;
        buffer.height = 64;
        buffer.stride = 64;
        buffer.win_x_min = 0;
        buffer.win_x_max = 64;
        buffer.win_y_min = 0;
        buffer.win_y_max = 64;
        ppe_rect_t rect = {0, 0, 64, 64};
        PPE_Clear(&buffer, 0x0, &rect);
        PPE_Finish(); //wait for PPE to finish clearing
    }
 * \endcode
 */
PPE_ERR PPE_Clear(ppe_buffer_t *dst, uint32_t color, ppe_rect_t *rect);

/**
 * \brief  Mix alpha and other format together.
 * \param[in] alpha: Alpha buffer to be mixed.
 * \param[in] src: Ohter picture.
 * \param[in] output: Mix result.
 * \return Operation result.
 * \retval PPE_SUCCESS   Operation success.
 * \retval Others        Operation failure, cause refers to @ref PPE_ERR .
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ppe_buffer_t mix_a, mix_rgb, output;
        memset(&mix_a, 0, sizeof(ppe_buffer_t));
        memset(&mix_rgb, 0, sizeof(ppe_buffer_t));
        memset(&output, 0, sizeof(ppe_buffer_t));
        PPE_BLEND_METHOD method = PPE_BLEND_SRC_IN;

        mix_a.format = PPE_A8;
        mix_a.address = (uint32_t)a;
        mix_a.opacity = 0xFF;
        mix_a.width = w;
        mix_a.height = h;
        mix_a.stride = w;
        mix_a.const_color = 0xFF000000;
        mix_a.win_x_min = 0;
        mix_a.win_x_max = w - 1;
        mix_a.win_y_min = 0;
        mix_a.win_y_max = h - 1;

        mix_rgb.format = PPE_RGB565;
        mix_rgb.address = (uint32_t)rgb;
        mix_rgb.opacity = 0xFF;
        mix_rgb.width = w;
        mix_rgb.height = h;
        mix_rgb.stride = w;
        mix_rgb.const_color = 0xFFFFFFFF;
        mix_rgb.win_x_min = 0;
        mix_rgb.win_x_max = w - 1;
        mix_rgb.win_y_min = 0;
        mix_rgb.win_y_max = h - 1;

        output.format = PPE_ARGB8888;
        output.address = (uint32_t)argb;
        output.opacity = 0xFF;
        output.width = w;
        output.height = h;
        output.stride = w;
        output.const_color = 0xFFFFFFFF;
        output.win_x_min = 0;
        output.win_x_max = w - 1;
        output.win_y_min = 0;
        output.win_y_max = h - 1;

        PPE_Mix_Alpha(&mix_a, &mix_rgb, &output, false);
        PPE_Finish();
    }
 * \endcode
 */
PPE_ERR PPE_Mix_Alpha(ppe_buffer_t *alpha, ppe_buffer_t *src, ppe_buffer_t *output,
                      bool vertical);

/**
 * \brief  Get intersect area of 2 rectangles.
 * \param[in] result_rect: Pointer to the output area.
 * \param[in] prect_1: Pointer to first input rectangle.
 * \param[in] prect_1: Pointer to second input rectangle.
 * \return Intersect result.
 * \retval true          Intersect area returned.
 * \retval false         No intersect area between 2 input rectangles.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(ppe_rect_t *rect_1, ppe_rect_t *rect_2){
        ppe_rect_t result;
        if(!ppe_intersect_area(&result, rect_1, rect_2)
        {
            DBG_DIRECT("No intersect area, need no handle");
        }
    }
 * \endcode
 */
bool ppe_intersect_area(ppe_rect_t *result_rect, ppe_rect_t *prect_1, ppe_rect_t *prect_2);

/** End of PPE_Exported_Functions
  * \}
  */

/** End of PPE
  * \}
  */

#ifdef __cplusplus
}
#endif

#endif /* RTL_PPE_H */
