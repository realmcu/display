/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* \file     rtl_PPE.h
* \brief    This file provides all the PPE 2.0 firmware functions.
* \details
* \author   astor zhang
* \date     2024-7-15
* \version  v1.0
*********************************************************************************************************
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
/** \defgroup 87X2G_PPE       PPE
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
    PPE_COLOR_KEY_INSIDE         = 0X0,
    PPE_COLOR_KEY_OUTSIDE   = 0X1,
} PPE_COLOR_KEY_MODE;

typedef enum
{
    PPE_LAYER_SRC_CONST             = 0X0,
    PPE_LAYER_SRC_FROM_DMA         = 0X1,
} PPE_PIXEL_SOURCE;

typedef enum
{
    PPV2_SRC_NEAREST_NEIGHBOUR,
    PPV2_SRC_BILINEAR,
} PPE_SRC_INTERPOLATION;


typedef enum
{
    PPE_ABGR8888 = 0x0,
    PPE_ARGB8888,
    PPE_XBGR8888,
    PPE_XRGB8888,
    PPE_BGRA8888,
    PPE_RGBA8888,
    PPE_BGRX8888,
    PPE_RGBX8888,
    PPE_ABGR4444,
    PPE_ARGB4444,
    PPE_XBGR4444,
    PPE_XRGB4444,
    PPE_BGRA4444,
    PPE_RGBA4444,
    PPE_BGRX4444,
    PPE_RGBX4444,
    PPE_ABGR2222,
    PPE_ARGB2222,
    PPE_XBGR2222,
    PPE_XRGB2222,
    PPE_BGRA2222,
    PPE_RGBA2222,
    PPE_BGRX2222,
    PPE_RGBX2222,
    PPE_ABGR8565,
    PPE_ARGB8565,
    PPE_XBGR8565,
    PPE_XRGB8565,
    PPE_BGRA5658,
    PPE_RGBA5658,
    PPE_BGRX5658,
    PPE_RGBX5658,
    PPE_ABGR1555,
    PPE_ARGB1555,
    PPE_XBGR1555,
    PPE_XRGB1555,
    PPE_BGRA5551,
    PPE_RGBA5551,
    PPE_BGRX5551,
    PPE_RGBX5551,
    PPE_BGR888,
    PPE_RGB888,
    PPE_BGR565,
    PPE_RGB565,
    PPE_A8,
    PPE_X8,
    PPE_A4,
    PPE_X4,
    PPE_A2,
    PPE_X2,
    PPE_A1,
    PPE_X1,
    PPE_ABGR8666,
    PPE_ARGB8666,
    PPE_XBGR8666,
    PPE_XRGB8666,
    PPE_BGRA6668,
    PPE_RGBA6668,
    PPE_BGRX6668,
    PPE_RGBX6668,
    PPE_BGR565_S,
    PPE_RGB565_S,
    PPE_I8,
    PPE_I4,
    PPE_I2,
    PPE_I1
} PPE_PIXEL_FORMAT;

typedef enum
{
    PPE_BLEND_SRC,
    PPE_BLEND_PREMULTIPLY,
    PPE_BLEND_SRC_OVER,
    PPE_BLEND_DST_OVER,
    PPE_BLEND_SRC_IN,
    PPE_BLEND_DST_IN,
    PPE_BLEND_MULTIPLY,
    PPE_BLEND_SCREEN,
    PPE_BLEND_ADD,
    PPE_BLEND_SUBSTRACT,
    PPE_BLEND_BYPASS,
} PPE_BLEND_METHOD;

typedef enum
{
    PPE_SUCCESS = 0x0,
    PPE_ERR_NULL_TARGET,
    PPE_ERR_NULL_SOURCE,
    PPE_ERR_INVALID_MATRIX,
    PPE_ERR_INVALID_RANGE,
    PPE_ERR_INVALID_PARAMETER,
} PPE_ERR;

typedef struct
{
    union
    {
        uint8_t key_enable;
        struct
        {
            __IO uint8_t r_en:  1;
            __IO uint8_t g_en:  1;
            __IO uint8_t b_en:  1;
            __IO uint8_t a_en:  1;
            __IO uint8_t reserved: 4;
        } channel_en;
    };
} PPE_color_key_state;

typedef struct
{
    uint8_t R_max;
    uint8_t R_min;
    uint8_t G_max;
    uint8_t G_min;
    uint8_t B_max;
    uint8_t B_min;
} PPE_color_key_range;

typedef struct
{
    union
    {
        uint32_t key_replace;
        struct
        {
            uint8_t r_replace;
            uint8_t g_replace;
            uint8_t b_replace;
            uint8_t a_replace;
        } channel_replace;
    };
} PPE_color_key_replace;

typedef struct
{
    PPE_COLOR_KEY_MODE      key_mode;
    PPE_color_key_state     key_enable;
    PPE_color_key_range     key_range;
    PPE_color_key_replace   key_replace;
} PPE_color_key_config;

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
    float m[3][3];    /*! The 3x3 matrix itself, in [row][column] order. */
} ppe_matrix_t;

typedef struct
{
    int x;
    int y;
    uint32_t w;
    uint32_t h;
} ppe_rect_t;

typedef struct
{
    int x;
    int y;
}
ppe_point_t;

/* Four 2D Point that form a polygon */
typedef ppe_point_t ppe_point4_t[4];

typedef struct
{
    uint32_t address;
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    uint32_t const_color;
    uint16_t win_x_min;
    uint16_t win_x_max;
    uint16_t win_y_min;
    uint16_t win_y_max;
    PPE_color_key_config color_key_config;
    PPE_PIXEL_FORMAT format;
    uint8_t opacity;
    bool high_quality;
} ppe_buffer_t;

/** End of PPE_Exported_Constants
  * \}
  */

/*============================================================================*
 *                         Functions
 *============================================================================*/
/** \defgroup PPE_Exported_Functions PPE Exported Functions
  * \brief
  * \{
  */


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

PPE_ERR PPE_buffer_init(ppe_buffer_t *buffer);
PPE_ERR PPE_set_color_key(ppe_buffer_t *image, PPE_color_key_config *config);

PPE_ERR PPE_Blit_Inverse(ppe_buffer_t *dst, ppe_buffer_t *src, uint8_t *output,
                         ppe_matrix_t *inverse,
                         ppe_rect_t *rect, PPE_BLEND_METHOD method);
PPE_ERR PPE_Blend_Handshake(ppe_buffer_t *dst, ppe_buffer_t *src, ppe_rect_t *rect);

void PPE_CLK_ENABLE(FunctionalState NewState);
void PPE_Finish(void);
uint8_t PPE_Get_Pixel_Size(PPE_PIXEL_FORMAT format);
void ppe_get_identity(ppe_matrix_t *matrix);
void ppe_translate(float x, float y, ppe_matrix_t *matrix);
void ppe_scale(float scale_x, float scale_y, ppe_matrix_t *matrix);
void ppe_rotate(float degrees, ppe_matrix_t *matrix);
void ppe_perspective(float px, float py, ppe_matrix_t *matrix);
int  ppe_get_transform_matrix(ppe_point4_t src, ppe_point4_t dst, ppe_matrix_t *mat);
void ppe_matrix_inverse(ppe_matrix_t *matrix);
void ppe_mat_multiply(ppe_matrix_t *matrix, ppe_matrix_t *mult);
bool ppe_get_area(ppe_rect_t *result_rect, ppe_rect_t *source_rect, ppe_matrix_t *matrix,
                  ppe_buffer_t *buffer);
PPE_ERR PPE_Mask(ppe_buffer_t *dst, uint32_t color, ppe_rect_t *rect);
PPE_ERR PPE_Blit_Inverse_Simulate(ppe_buffer_t *dst, ppe_buffer_t *src, ppe_matrix_t *matrix,
                                  ppe_matrix_t *inverse,
                                  ppe_rect_t *rect, PPE_BLEND_METHOD method);
void PPE_Register_CLUT(uint32_t *clut, uint16_t size);
void PPE_CLK_ENABLE_IN_DLPS(FunctionalState NewState);
void PPE_test(void);
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

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/

