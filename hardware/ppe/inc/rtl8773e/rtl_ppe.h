/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* \file     rtl_PPE.h
* \brief    This file provides all the PPE 2.0 firmware functions.
* \details
* \author   feihu wang
* \date     2023-10-17
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
 *                         PPE Registers Memory Map
 *============================================================================*/
typedef struct
{
    __IO uint32_t REG_GLB_STATUS;                       /* offset 0x00*/
    __IO uint32_t REG_LYR_ENABLE;                       /* offset 0x04*/
    __IO uint32_t REG_LD_CFG;                           /* offset 0x08*/
    __IO uint32_t REG_LL_CFG;                           /* offset 0x0C*/
    __IO uint32_t REG_LLP;                              /* offset 0x10*/
    __I  uint32_t REG_RESERVED1;                        /* offset 0x14*/
    __IO uint32_t REG_SECURE;                           /* offset 0x18*/
    __IO uint32_t REG_LINE_OVER_INDEX;                  /* offset 0x1C*/
    __I  uint32_t REG_RELEASE_DATE;                     /* offset 0x20*/
    __I  uint32_t REG_RTL_VER;                          /* offset 0x24*/
    __I  uint32_t REG_RESERVED2[6];                     /* offset 0x28-3C*/
    __I  uint32_t REG_INTR_STATUS;                      /* offset 0x40*/
    __I  uint32_t REG_INTR_RAW;                         /* offset 0x44*/
    __O  uint32_t REG_INTR_CLR;                         /* offset 0x48*/
    __IO uint32_t REG_INTR_MASK;                        /* offset 0x4C*/
    __I  uint32_t REG_BUS_ERR_DETAIL;                   /* offset 0x50*/
    __I  uint32_t REG_DIV0_ERR_DETAIL;                  /* offset 0x54*/
    __I  uint32_t REG_RESERVED3[2];                     /* offset 0x58-0x5C*/
} PPE_Typedef;

typedef struct
{
    __IO uint32_t REG_LYR0_ADDR;                /* offset 0x60*/
    __I  uint32_t REG_RESERVED1;                /* offset 0x64*/
    __IO uint32_t REG_CANVAS_SIZE;              /* offset 0x68*/
    __IO uint32_t REG_LYR0_PIC_CFG;             /* offset 0x6C*/
    __IO uint32_t REG_BACKGROUND;               /* offset 0x70*/
    __IO uint32_t REG_LYR0_BUS_CFG;             /* offset 0x74*/
    __IO uint32_t REG_LYR0_HS_CFG;              /* offset 0x78*/
    __I  uint32_t REG_RESERVED2;                /* offset 0x7C*/
} PPE_ResultLayer_Typedef;


typedef struct
{
    __IO uint32_t REG_LYRx_ADDR;                                /* offset 0x00 + X * 0x80*/
    __IO uint32_t REG_RESERVED1;                                /* offset 0x04 + X * 0x80*/
    __IO uint32_t REG_LYRx_PIC_SIZE;                            /* offset 0x08 + X * 0x80*/
    __IO uint32_t REG_LYRx_PIC_CFG;                             /* offset 0x0C + X * 0x80*/
    __IO uint32_t REG_LYRx_FIXED_COLOR;                         /* offset 0x10 + X * 0x80*/
    __IO uint32_t REG_LYRx_BUS_CFG;                             /* offset 0x14 + X * 0x80*/
    __IO uint32_t REG_LYRx_HS_CFG;                              /* offset 0x18 + X * 0x80*/
    __IO uint32_t REG_LYRx_WIN_MIN;                             /* offset 0x1C + X * 0x80*/
    __IO uint32_t REG_LYRx_WIN_MAX;                             /* offset 0x20 + X * 0x80*/
    __IO uint32_t REG_LYRx_KEY_MIN;                             /* offset 0x24 + X * 0x80*/
    __IO uint32_t REG_LYRx_KEY_MAX;                             /* offset 0x28 + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E11;                    /* offset 0x2C + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E12;                    /* offset 0x30 + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E13;                    /* offset 0x34 + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E21;                    /* offset 0x38 + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E22;                    /* offset 0x3C + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E23;                    /* offset 0x40 + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E31;                    /* offset 0x44 + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E32;                    /* offset 0x48 + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E33;                    /* offset 0x4C + X * 0x80*/
    __IO uint32_t REG_RESERVED2[12];                            /* offset (0x50-0x7C) + X * 0x80*/
} PPE_Input_Layer_Typedef;

/*============================================================================*
 *                         PPE Declaration
 *============================================================================*/
#define PPE_REG_BASE                              0x40005000UL
#define PPE_Result_Layer_BASE                     (PPE_REG_BASE + 0x60)
#define PPE_Input_Layer1_BASE                     (PPE_REG_BASE + 1 * 0x80)
#define PPE_Input_Layer2_BASE                     (PPE_REG_BASE + 2 * 0x80)
#define PPE_Input_Layer3_BASE                     (PPE_REG_BASE + 3 * 0x80)
#define PPE_Input_Layer4_BASE                     (PPE_REG_BASE + 4 * 0x80)

#define PPE                                           ((PPE_Typedef *)PPE_REG_BASE)
#define PPE_ResultLayer                               ((PPE_ResultLayer_Typedef *)PPE_Result_Layer_BASE)
#define PPE_InputLayer1                               ((PPE_Input_Layer_Typedef *)PPE_Input_Layer1_BASE)
#define PPE_InputLayer2                               ((PPE_Input_Layer_Typedef *)PPE_Input_Layer2_BASE)
#define PPE_InputLayer3                               ((PPE_Input_Layer_Typedef *)PPE_Input_Layer3_BASE)
#define PPE_InputLayer4                               ((PPE_Input_Layer_Typedef *)PPE_Input_Layer4_BASE)

#define PPE_MAX_INPUTLAYER                          0x4
#define PPE_LINKLIST_GLB_REG_NUM                    3UL
#define PPE_LINKLIST_RESULT_REG_NUM                 4UL
#define PPE_LINKLIST_INPUT_REG_NUM                  17UL
#define PPE_LLI_MAX                                 (PPE_LINKLIST_GLB_REG_NUM + PPE_LINKLIST_RESULT_REG_NUM + PPE_LINKLIST_INPUT_REG_NUM * PPE_MAX_INPUTLAYER)

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
    PPE_INPUT_3 = 0x3,
    PPE_INPUT_4 = 0x4,

} PPE_INPUT_LAYER_INDEX;

typedef enum
{
    PPE_NON_SECURE_MODE = 0X0,
    PPE_SECURE_MODE         = 0x1,
} PPE_SECURE;

typedef enum
{
    PPE_AWBURST_FIXED     = 0X0,
    PPE_AWBURST_INC         = 0X1,
} PPE_AWBURST;

typedef enum
{
    PPE_HW_HS_DISABLE     = 0X0,
    PPE_HW_HS_ENABLE         = 0X1,
} PPE_HW_HS;

typedef enum
{
    PPE_HW_HS_ACTIVE_HIGH     = 0X0,
    PPE_HW_HS_ACTIVE_LOW         = 0X1,
} PPE_HW_HS_POL;

typedef enum
{
    PPE_COLOR_KEY_DISABLE     = 0X0,
    PPE_COLOR_KEY_INSIDE         = 0X2,
    PPE_COLOR_KEY_OUTSIDE   = 0X3,
} PPE_COLOR_KEY_MODE;

typedef enum
{
    PPE_LAYER_SRC_CONST             = 0X0,
    PPE_LAYER_SRC_FROM_DMA         = 0X1,
} PPE_PIXEL_SOURCE;

typedef enum
{
    PPE_DMA_HW_HANDSHAKE,
    PPE_DMA_SW_HANDSHAKE,
} PPE_DMA_HANDSHAKE;

typedef enum
{
    PPE_MSIZE_1,
    PPE_MSIZE_2,
    PPE_MSIZE_4,
    PPE_MSIZE_8,
    PPE_MSIZE_16,
    PPE_MSIZE_32,
    PPE_MSIZE_64,
    PPE_MSIZE_128,
    PPE_MSIZE_256,
    PPE_MSIZE_512,
    PPE_MSIZE_1024,
} PPE_MSIZE_LOG;

typedef enum
{
    PPE_HS_IDU_Tx,
} PPE_HW_HANDSHAKE_INDEX;

typedef enum
{
    PPE_MAX_AXLEN_0,
    PPE_MAX_AXLEN_1,
    PPE_MAX_AXLEN_3,
    PPE_MAX_AXLEN_7,
    PPE_MAX_AXLEN_15,
    PPE_MAX_AXLEN_31,
    PPE_MAX_AXLEN_63,
    PPE_MAX_AXLEN_127,
} PPE_MAX_AXLEN;

typedef enum
{
    PPV2_READ_MATRIX_1X1,
    PPV2_READ_MATRIX_2X2,
} PPE_READ_MATRIX_SIZE;


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
    PPE_ABGR8666 = 0x32,
    PPE_ARGB8666,
    PPE_XBGR8666,
    PPE_XRGB8666,
    PPE_BGRA6668,
    PPE_RGBA6668,
    PPE_BGRX6668,
    PPE_RGBX6668,
    PPE_FORMAT_NOT_SUPPORT = 0xFF,
} PPE_PIXEL_FORMAT;

typedef enum
{
    PPE_BYPASS_MODE,
    PPE_TRANSPARENT_MODE,
    PPE_SRC_OVER_MODE,
    PPE_CONST_MASK_MODE,
} PPE_BLEND_MODE;

typedef enum
{
    PPE_SUCCESS = 0x0,
    PPE_ERR_NULL_TARGET,
    PPE_ERR_NULL_SOURCE,
    PPE_ERR_INVALID_MATRIX,
    PPE_ERR_INVALID_RANGE,
} PPE_err;

typedef struct
{
    FunctionalState         SetValid_AutoClear;
    uint32_t                LLP;
    PPE_SECURE            Secure_En;
} PPE_Init_Typedef;

typedef struct
{
    uint32_t                            Layer_Address;
    uint32_t                            Canvas_Height;
    uint32_t                            Canvas_Width;
    uint32_t                            Line_Length;
    PPE_PIXEL_FORMAT                  Color_Format;
    uint32_t                            BackGround;
    PPE_AWBURST                       LayerBus_Inc;
    PPE_MAX_AXLEN                     Max_Axlen;
    FunctionalState                     MultiFrame_Reload_En;
    FunctionalState                     MultiFrame_LLP_En;
    PPE_HW_HS                         Layer_HW_Handshake_En;
    PPE_HW_HANDSHAKE_INDEX            Layer_HW_Handshake_Index;
    PPE_HW_HS_POL                     Layer_HW_Handshake_Polarity;
    PPE_MSIZE_LOG                     Layer_HW_Handshake_MsizeLog;
} PPE_ResultLayer_Init_Typedef;

typedef struct
{
    uint32_t                            Layer_Address;
    uint32_t                            Pic_Height;
    uint32_t                            Pic_Width;
    uint32_t                            Line_Length;
    PPE_PIXEL_SOURCE                  Pixel_Source;
    PPE_PIXEL_FORMAT                  Pixel_Color_Format;
    PPE_READ_MATRIX_SIZE              Read_Matrix_Size;
    uint32_t                            Const_Pixel;
    PPE_COLOR_KEY_MODE                Color_Key_Mode;
    uint32_t                            Color_Key_Min;
    uint32_t                            Color_Key_Max;
    FunctionalState                     MultiFrame_Reload_En;
    FunctionalState                     MultiFrame_LLP_En;
    PPE_AWBURST                       LayerBus_Inc;
    PPE_MAX_AXLEN                     Max_Axlen;
    PPE_HW_HS                         Layer_HW_Handshake_En;
    PPE_HW_HANDSHAKE_INDEX            Layer_HW_Handshake_Index;
    PPE_HW_HS_POL                     Layer_HW_Handshake_Polarity;
    PPE_MSIZE_LOG                     Layer_HW_Handshake_MsizeLog;
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
    uint32_t color_key_max;
    uint32_t color_key_min;
    PPE_COLOR_KEY_MODE color_key_enable;
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

void PPE_Init(PPE_Init_Typedef *PPE_Init_Struct);

void PPE_ResultLayer_Init(PPE_ResultLayer_Init_Typedef *PPE_ResultLyaer_Init_Struct);

void PPE_InputLayer_Init(PPE_INPUT_LAYER_INDEX intput_layer_index,
                         PPE_InputLayer_Init_Typedef *PPE_InputLayer_Init_Struct);

void PPE_StructInit(PPE_Init_Typedef *PPE_Init_Struct);

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

PPE_err PPE_Blit(ppe_buffer_t *target, ppe_buffer_t *image, ppe_matrix_t *matrix,
                 PPE_BLEND_MODE mode);
PPE_err PPE_Blit_Inverse(ppe_buffer_t *dst, ppe_buffer_t *src, ppe_matrix_t *inverse,
                         ppe_rect_t *rect, PPE_BLEND_MODE mode);
PPE_err PPE_Blend_Handshake(ppe_buffer_t *dst, ppe_buffer_t *src, ppe_rect_t *rect);

void PPE_CLK_ENABLE(FunctionalState NewState);
void PPE_Finish(void);
uint8_t PPE_Get_Pixel_Size(PPE_PIXEL_FORMAT format);
void ppe_get_identity(ppe_matrix_t *matrix);
void ppe_translate(float x, float y, ppe_matrix_t *matrix);
void ppe_scale(float scale_x, float scale_y, ppe_matrix_t *matrix);
void ppe_rotate(float degrees, ppe_matrix_t *matrix);
void ppe_skew(float skew_x, float skew_y, ppe_matrix_t *matrix);
void ppe_perspective(float px, float py, ppe_matrix_t *matrix);
int  ppe_get_transform_matrix(ppe_point4_t src, ppe_point4_t dst, ppe_matrix_t *mat);
void ppe_matrix_inverse(ppe_matrix_t *matrix);
void ppe_mat_multiply(ppe_matrix_t *matrix, ppe_matrix_t *mult);
bool ppe_get_area(ppe_rect_t *result_rect, ppe_rect_t *source_rect, ppe_matrix_t *matrix,
                  ppe_buffer_t *buffer);
PPE_err PPE_Blend_Multi(ppe_buffer_t *dst, ppe_buffer_t *src_1,
                        ppe_buffer_t *src_2, ppe_buffer_t *src_3, PPE_BLEND_MODE mode);
PPE_err PPE_Mask(ppe_buffer_t *dst, uint32_t color, ppe_rect_t *rect);
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

