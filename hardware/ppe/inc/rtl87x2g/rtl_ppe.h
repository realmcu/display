/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* \file     rtl_ppe.h
* \brief    This file provides all the Pixel Process Engine firmware functions.
* \details
* \author   astor zhang
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
#include "rtl_ppe_def.h"

/*============================================================================*
 *                          Private Macros
 *============================================================================*/

/** \defgroup PPE        PPE
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
typedef struct
{
    uint32_t LAYER_ADDR;
    uint32_t LAYER_POS;
    uint32_t LAYER_WINSIZE;
    uint32_t LAYER_CONST_PIX;
} PPE_LLI_LAYER;

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
    PPE_LAYER_SRC_CONST,
    PPE_LAYER_SRC_FROM_DMA,
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
    PPE_POLARITY_LOW,
    PPE_POLARITY_HIGH,
} PPE_ACTIVE_POLARITY;

typedef enum
{
    PPE_ARBURST_FIX,
    PPE_ARBURST_INCR,
} PPE_ARBURST;

typedef enum
{
    PPE_NO_SWAP,
    PPE_8BIT_SWAP,
    PPE_16BIT_SWAP,
    PPE_8_16BIT_SWAP,
    PPE_32BIT_SWAP,
    PPE_32_8BIT_SWAP,
    PPE_32_16BIT_SWAP,
    PPE_32_16_8BIT_SWAP,
} PPE_BYTE_SWAP;

typedef struct
{
    uint32_t function;
    uint32_t blend_layer_num;
} PPE_InitTypeDef;

typedef struct
{
    uint32_t src_addr;
    uint32_t start_x;
    uint32_t start_y;
    uint32_t width;
    uint32_t height;
    uint32_t const_ABGR8888_value;
    uint32_t key_color_value;
    uint32_t line_len;
    PPE_PIXEL_FORMAT format;
    PPE_PIXEL_SOURCE src;
    FunctionalState color_key_en;
    PPE_BYTE_SWAP byte_swap;
    PPE_DMA_HANDSHAKE handshake_mode;
    PPE_ACTIVE_POLARITY polarity;
    FunctionalState handshake_en;
    PPE_MAX_AXLEN MAX_AXLEN_LOG;
    uint32_t AXSIZE;
    PPE_ARBURST INCR;
    uint32_t AXCACHE;
    uint32_t PRIOR;
    PPE_MSIZE_LOG handshake_msize;
    uint32_t hw_index;
} PPE_InputLayer_InitTypeDef;

typedef struct
{
    uint32_t src_addr;
    uint32_t width;
    uint32_t height;
    uint32_t line_len;
    PPE_PIXEL_FORMAT format;
    PPE_BYTE_SWAP byte_swap;
    PPE_DMA_HANDSHAKE handshake_mode;
    PPE_ACTIVE_POLARITY polarity;
    FunctionalState handshake_en;
    PPE_MAX_AXLEN MAX_AXLEN_LOG;
    uint32_t AXSIZE;
    PPE_ARBURST INCR;
    uint32_t AXCACHE;
    uint32_t PRIOR;
    PPE_MSIZE_LOG handshake_msize;
    uint32_t hw_index;
} PPE_ResultLayer_InitTypeDef;

typedef enum
{
    PPE_SUCCESS,
    PPE_SUCCESS_NOT_CHANGE,
    PPE_ERROR_ADDR_NOT_ALIGNED,
    PPE_ERROR_INVALID_PARAM,
    PPE_ERROR_UNKNOWN_FORMAT,
    PPE_ERROR_NULL_SOURCE,
    PPE_ERROR_NULL_TARGET,
    PPE_ERROR_OUT_OF_RANGE,
} PPE_ERR;

typedef enum
{
    PPE_BYPASS_MODE,    //D = S;
    PPE_SRC_OVER_MODE,  //D = (1 - a) * D + S * a;
} PPE_BLEND_MODE;

typedef struct
{
    union
    {
        int32_t left;
        int32_t x1;
    };
    union
    {
        int32_t top;
        int32_t y1;
    };
    union
    {
        int32_t right;
        int32_t x2;
    };
    union
    {
        int32_t bottom;
        int32_t y2;
    };
} ppe_rect_t;

typedef struct
{
    int32_t x;
    int32_t y;
} ppe_translate_t;

typedef struct
{
    uint32_t *memory;
    uint32_t address;
    PPE_PIXEL_FORMAT format;
    bool color_key_en;
    bool global_alpha_en;
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    uint32_t color_key_value;
    uint8_t global_alpha;
} ppe_buffer_t;


typedef struct
{
    ppe_buffer_t buffer;
    ppe_rect_t *rect;
    ppe_translate_t trans;
} ppe_layer_t;

typedef struct
{
    ppe_layer_t *input_layer1;
    ppe_layer_t *input_layer2;
    ppe_layer_t *input_layer3;
    ppe_layer_t *input_layer4;
    ppe_layer_t *output_layer;
} ppe_input_list_t;

/**
 * \defgroup    PPE_Interrupt PPE Interrupt
 * \{
 * \ingroup     PPE_Exported_Constants
 */
#define PPE_ALL_OVER_INT                                ((uint32_t)0x1 << PPE_INT_ALL_OVER_POS)
#define PPE_FR_OVER_INT                                 ((uint32_t)0x1 << PPE_INT_FR_OVER_POS)
#define PPE_LOAD_OVER_INT                               ((uint32_t)0x1 << PPE_INT_LOAD_OVER_POS)
#define PPE_LINE_WL_INT                                 ((uint32_t)0x1 << PPE_INT_LINE_WL_POS)
#define PPE_SUSP_INAC_INT                               ((uint32_t)0x1 << PPE_INT_SUSP_INAC_POS)
#define PPE_SECURE_ERR_INT                              ((uint32_t)0x1 << PPE_INT_SECURE_ERR_POS)
#define PPE_SET_ERR_INT                                 ((uint32_t)0x1 << PPE_INT_SET_ERR_POS)
#define PPE_BUS1_ERR_INT                                ((uint32_t)0x1 << PPE_INT_CHN_BUS_ERR_POS)
#define PPE_BUS2_ERR_INT                                ((uint32_t)0x2 << PPE_INT_CHN_BUS_ERR_POS)
#define PPE_BUS3_ERR_INT                                ((uint32_t)0x4 << PPE_INT_CHN_BUS_ERR_POS)
#define PPE_BUS4_ERR_INT                                ((uint32_t)0x8 << PPE_INT_CHN_BUS_ERR_POS)
#define PPE_BUS5_ERR_INT                                ((uint32_t)0x10 << PPE_INT_CHN_BUS_ERR_POS)
#define PPE_BUS6_ERR_INT                                ((uint32_t)0x20 << PPE_INT_CHN_BUS_ERR_POS)
#define PPE_BUS7_ERR_INT                                ((uint32_t)0x40 << PPE_INT_CHN_BUS_ERR_POS)
#define PPE_BUS8_ERR_INT                                ((uint32_t)0x80 << PPE_INT_CHN_BUS_ERR_POS)
#define PPE_BUS9_ERR_INT                                ((uint32_t)0x100 << PPE_INT_CHN_BUS_ERR_POS)
#define PPE_BUS10_ERR_INT                               ((uint32_t)0x200 << PPE_INT_CHN_BUS_ERR_POS)
#define PPE_BUS11_ERR_INT                               ((uint32_t)0x400 << PPE_INT_CHN_BUS_ERR_POS)
#define PPE_BUS12_ERR_INT                               ((uint32_t)0x800 << PPE_INT_CHN_BUS_ERR_POS)
#define PPE_BUS13_ERR_INT                               ((uint32_t)0x1000 << PPE_INT_CHN_BUS_ERR_POS)
#define PPE_BUS14_ERR_INT                               ((uint32_t)0x2000 << PPE_INT_CHN_BUS_ERR_POS)
#define PPE_BUS15_ERR_INT                               ((uint32_t)0x4000 << PPE_INT_CHN_BUS_ERR_POS)
#define PPE_BUS16_ERR_INT                               ((uint32_t)0x8000 << PPE_INT_CHN_BUS_ERR_POS)
#define PPE_BUS17_ERR_INT                               ((uint32_t)0x10000 << PPE_INT_CHN_BUS_ERR_POS)

#define IS_PPE_INT(interrupt)                           ((interrupt) == PPE_ALL_OVER_INT) || (interrupt) == PPE_INT_FR_OVER_POS) ||\
    (interrupt) == PPE_INT_LOAD_OVER_POS) || (interrupt) == PPE_INT_LINE_WL_POS) ||\
    (interrupt) == PPE_INT_SUSP_INAC_POS) || (interrupt) == PPE_INT_SECURE_ERR_POS) ||\
    (interrupt) == PPE_INT_SET_ERR_POS) || \
    ((interrupt) & PPE_INT_CHN_BUS_ERR_CLR) | PPE_INT_CHN_BUS_ERR_MSK))

/** End of PPE_Interrupt
  * \}
  */

/**
 * \cond        Private
 * \{
 */
#define IS_PPE_CHN_INDEX(chn)                               (((chn) < 17) && ((chn) >= 0))

#define PPE_FUNCTION_ALPHA_BLEND                        ((uint32_t)0x0)
#define PPE_FUNCTION_SCALE                              ((uint32_t)0x1)
#define IS_PPE_FUNCTION(func)                           (((func) == PPE_FUNCTION_ALPHA_BLEND) || ((func) == PPE_FUNCTION_SCALE))

#define IS_PPE_ALPHA_BLEND_LAYER_NUM(layer_num)         ((layer_num) < 16)
#define IS_PPE_SCALE_LINE_NUM(line_num)                 ((line_num) < 16)
#define IS_PPE_SCALE_COLUMN_NUM(colunm_num)             ((colunm_num) < 16)

/**
  * \}
  * \endcond
  */

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

/**
 * \brief  Enable PPE
 * \param[in] state             enable or disable PPE peripheral.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        PPE_Cmd(ENABLE);
    }
 * \endcode
*/
void PPE_Cmd(FunctionalState state);

/**
 * \brief  Suspend PPE
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        PPE_Suspend();
    }
 * \endcode
*/
void PPE_Suspend(void);

/**
 * \brief  Resume PPE, must envoke after all channels are inactive
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        PPE_Suspend();
    }
 * \endcode
*/
void PPE_Resume(void);

/**
 * \brief  Abort current operation, must envoke after all channels are inactive
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        PPE_Suspend();
    }
 * \endcode
*/
void PPE_Abort(void);

/**
 * \brief  Check channel status
 * \param[in] chn_id       channel index.
 * \return channel state
 * \retval ENABLE       channel is active.
 * \retval DISABLE      channel is inactive.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ITStatus res = PPE_ChannelInactiveCheck(0);
    }
 * \endcode
*/
ITStatus PPE_ChannelInactiveCheck(uint8_t chn_id);

/**
 * \brief  Enable/Disable PPE secure mode
 * \param[in] state          enable or disable PPE secure mode.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        PPE_Secure(ENABLE);
    }
 * \endcode
*/
void PPE_Secure(FunctionalState state);

/**
 * \brief  Check interrupt status
 * \param[in] PPE_INT       PPE interrupt.
 * \return interrupt status
 * \retval ENABLE       interrupt triggerred.
 * \retval DISABLE      interrupt not triggerred.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ITStatus res = PPE_GetINTStatus(PPE_SUSP_INAC_INT);
    }
 * \endcode
*/
ITStatus PPE_GetINTStatus(uint32_t PPE_INT);

/**
 * \brief  Check interrupt status before mask
 * \param[in] PPE_INT       PPE interrupt.
 * \return interrupt status
 * \retval ENABLE       interrupt triggerred.
 * \retval DISABLE      interrupt not triggerred.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ITStatus res = PPE_GetINTStatusRaw(PPE_SUSP_INAC_INT);
    }
 * \endcode
*/
ITStatus PPE_GetINTStatusRaw(uint32_t PPE_INT);

/**
 * \brief  Check interrupt pending bit
 * \param[in] PPE_INT       PPE interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        PPE_ClearINTPendingBit(PPE_SUSP_INAC_INT);
    }
 * \endcode
*/
void PPE_ClearINTPendingBit(uint32_t PPE_INT);

/**
 * \brief  Mask certain interrupt
 * \param[in] PPE_INT_MSK       PPE interrupt.
 * \param[in] NewState          mask/unmask interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        PPE_MaskINTConfig(PPE_SUSP_INAC_INT, ENABLE);
    }
 * \endcode
*/
void PPE_MaskINTConfig(uint32_t PPE_INT_MSK, FunctionalState NewState);

/**
 * \brief  Fill PPE_InitTypeDef with default value
 * \param[in] PPE_init_struct     pointer to structure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        PPE_InitTypeDef init_struct;
        PPE_structInit(&init_struct);
    }
 * \endcode
*/
void PPE_structInit(PPE_InitTypeDef *PPE_init_struct);

/**
 * \brief  Fill PPE_InputLayer_InitTypeDef with default value
 * \param[in] layer_init_struct     pointer to structure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        PPE_InputLayer_InitTypeDef init_struct;
        PPE_InputLayer_StructInit(&init_struct);
    }
 * \endcode
*/
void PPE_InputLayer_StructInit(PPE_InputLayer_InitTypeDef *layer_init_struct);

/**
 * \brief  Fill PPE_ResultLayer_InitTypeDef with default value
 * \param[in] layer_init_struct     pointer to structure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        PPE_ResultLayer_InitTypeDef init_struct;
        PPE_ResultLayer_StructInit(&init_struct);
    }
 * \endcode
*/
void PPE_ResultLayer_StructInit(PPE_ResultLayer_InitTypeDef *layer_init_struct);

/**
 * \brief  Initialize PPE with parameters in input structure
 * \param[in] PPE_init_struct     pointer to structure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        PPE_InitTypeDef PPE_Init_User;
        PPE_structInit(&PPE_Init_User);
        PPE_Init_User.function           = PPE_FUNCTION_ALPHA_BLEND;
        PPE_Init_User.blend_layer_num    = 2;
        PPE_Init(&PPE_Init_User);
    }
 * \endcode
*/
void PPE_Init(PPE_InitTypeDef *PPE_init_struct);

/**
 * \brief  Initialize input layer with parameters in input structure
 * \param[in] id                  index of input layer.
 * \param[in] layer_init_struct   pointer to structure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        PPE_InputLayer_InitTypeDef      PPE_Input_Layer1;
        PPE_InputLayer_StructInit(&PPE_Input_Layer1);
        PPE_Input_Layer1.src_addr                       = (uint32_t)buffer->memory;
        PPE_Input_Layer1.start_x                        = 0;
        PPE_Input_Layer1.start_y                        = 0;
        PPE_Input_Layer1.width                          = buffer->width;
        PPE_Input_Layer1.height                         = buffer->height;
        PPE_InitInputLayer(1, &PPE_Input_Layer1);
    }
 * \endcode
*/
void PPE_InitInputLayer(uint8_t id, PPE_InputLayer_InitTypeDef *layer_init_struct);

/**
 * \brief  Initialize result layer with parameters in input structure
 * \param[in] layer_init_struct     pointer to structure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        PPE_ResultLayer_InitTypeDef      PPE_Result_Layer;
        PPE_InputLayer_StructInit(&PPE_Result_Layer);
        PPE_Result_Layer.src_addr                       = (uint32_t)buffer->memory;
        PPE_Result_Layer.width                          = buffer->width;
        PPE_Result_Layer.height                         = buffer->height;
        PPE_Result_Layer.format                         = buffer->format;
        PPE_Result_Layer.line_len                       = PPE_Result_Layer.width;
        PPE_Result_Layer.AXSIZE                         = 2;// 32bit bandwidth
        PPE_Result_Layer.INCR                           = PPE_ARBURST_INCR;
        PPE_Result_Layer.AXCACHE                        = 1;
        PPE_Result_Layer.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_127;
        PPE_Result_Layer.PRIOR                          = 0;
        PPE_Result_Layer.byte_swap                      = PPE_NO_SWAP;
        PPE_Result_Layer.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
        PPE_Result_Layer.polarity                       = PPE_POLARITY_HIGH;
        PPE_Result_Layer.handshake_en                   = DISABLE;
        PPE_Result_Layer.handshake_msize                = PPE_MSIZE_2;
        PPE_Result_Layer.hw_index                       = 0;
        PPE_InitResultLayer(&PPE_Result_Layer);
    }
 * \endcode
*/
void PPE_InitResultLayer(PPE_ResultLayer_InitTypeDef *layer_init_struct);

/**
 * \brief  Scale the input image and save in output image buffer
 * \param[in] image         input image.
 * \param[in] buffer        output image buffer.
 * \param[in] x_ratio       scale ration on x-axis.
 * \param[in] y_ratio       scale ration on y-axis.
 * \return operation result
 * \retval PPE_SUCCESS  Operation success.
 * \retval others       Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);
        ppe_buffer_t image, buffer;
        image.width = 25;
        image.height = 25;
        image.format = PPE_RGB888;
        image.memory = (uint32_t*)SOURCE_PIC_1;
        image.address = (uint32_t)image.memory;

        buffer.format = PPE_RGB565;
        buffer.memory = (uint32_t*)PIC_OUTPUT_LAYER;
        buffer.address = (uint32_t)PIC_OUTPUT_LAYER;
        memset(PIC_OUTPUT_LAYER, 0, sizeof(PIC_OUTPUT_LAYER));
        PPE_ERR err = PPE_Scale(&image, &buffer, 3, 0.8);
    }
 * \endcode
 */
PPE_ERR PPE_Scale(ppe_buffer_t *image, ppe_buffer_t *buffer, float x_ratio, float y_ratio);

/**
 * \brief  Scale a part of input image and save in output image buffer
 * \param[in] image         input image.
 * \param[in] buffer        output image buffer.
 * \param[in] x_ratio       scale ration on x-axis.
 * \param[in] y_ratio       scale ration on y-axis.
 * \param[in] rect          the boundary of the part to be scaled.
 * \return operation result
 * \retval PPE_SUCCESS  Operation success.
 * \retval others       Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);
        ppe_buffer_t image, buffer;
        image.width = 25;
        image.height = 25;
        image.format = PPE_RGB888;
        image.memory = (uint32_t*)SOURCE_PIC_1;
        image.address = (uint32_t)image.memory;

        buffer.format = PPE_RGB565;
        buffer.memory = (uint32_t*)PIC_OUTPUT_LAYER;
        buffer.address = (uint32_t)PIC_OUTPUT_LAYER;
        memset(PIC_OUTPUT_LAYER, 0, sizeof(PIC_OUTPUT_LAYER));
        ppe_rect_t rect = {6, 4, 20, 24};
        PPE_ERR err = PPE_Scale_Rect(&image, &buffer, 3, 3, &rect);
    }
 * \endcode
 */
PPE_ERR PPE_Scale_Rect(ppe_buffer_t *image, ppe_buffer_t *buffer, float x_ratio, float y_ratio,
                       ppe_rect_t *rect);

/**
 * \brief  Scale a image and cover the destionation area on target.
 * \param[in] image         input image.
 * \param[in] buffer        output image buffer.
 * \param[in] trans         translate information of scaled image.
 * \param[in] x_ratio       scale ration on x-axis.
 * \param[in] y_ratio       scale ration on y-axis.
 * \param[in] rect          the boundary of the part to be scaled.
 * \return operation result
 * \retval PPE_SUCCESS  Operation success.
 * \retval others       Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);
        ppe_buffer_t image, buffer;
        image.width = 25;
        image.height = 25;
        image.format = PPE_RGB888;
        image.memory = (uint32_t*)SOURCE_PIC_1;
        image.address = (uint32_t)image.memory;

        buffer.format = PPE_RGB565;
        buffer.memory = (uint32_t*)PIC_OUTPUT_LAYER;
        buffer.address = (uint32_t)PIC_OUTPUT_LAYER;
        memset(PIC_OUTPUT_LAYER, 0, sizeof(PIC_OUTPUT_LAYER));
        ppe_rect_t rect = {6, 4, 20, 24};
        ppe_translate_t trans = {.x = 10, .y = 5};
        PPE_ERR err = PPE_Scale_Rect_Cover(&image, &buffer, &trans, 3, 3, &rect);
    }
 * \endcode
 */
PPE_ERR PPE_Scale_Rect_Cover(ppe_buffer_t *image, ppe_buffer_t *buffer, ppe_translate_t *trans,
                             float x_ratio, float y_ratio,
                             ppe_rect_t *rect);

/**
 * \brief  Clear the image buffer with certain color in ABGR8888 format
 * \param[in] buffer        input image buffer.
 * \param[in] color         specified color in ABGR8888 format
 * \return operation result
 * \retval PPE_SUCCESS  Operation success.
 * \retval others       Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);
        ppe_buffer_t buffer;

        buffer.format = PPE_RGB565;
        buffer.memory = (uint32_t*)PIC_OUTPUT_LAYER;
        buffer.address = (uint32_t)PIC_OUTPUT_LAYER;
        buffer.width = 64;
        buffer.height = 64;

        PPE_ERR PPE_Clear(&buffer, 0xFFFF00FF);
    }
 * \endcode
 */
PPE_ERR PPE_Clear(ppe_buffer_t *buffer, uint32_t color);

/**
 * \brief  Clear a part of buffer with certain color in ABGR8888 format
 * \param[in] buffer        input image buffer.
 * \param[in] rect          specified range
 * \param[in] color         specified color in ABGR8888 format
 * \return operation result
 * \retval PPE_SUCCESS  Operation success.
 * \retval others       Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);
        ppe_buffer_t buffer;

        buffer.format = PPE_RGB565;
        buffer.memory = (uint32_t*)PIC_OUTPUT_LAYER;
        buffer.address = (uint32_t)PIC_OUTPUT_LAYER;
        buffer.width = 64;
        buffer.height = 64;

        ppe_rect_t rect = {6, 4, 20, 24};
        PPE_ERR PPE_Clear(&buffer, &rect, 0xFFFF00FF);
    }
 * \endcode
 */
PPE_ERR PPE_Clear_Rect(ppe_buffer_t *buffer, ppe_rect_t *rect, uint32_t color);

/**
 * \brief  blend source image to target buffer
 * \param[in] image         source image.
 * \param[in] buffer        target image buffer.
 * \param[in] trans         translate information of source image.
 * \return operation result
 * \retval PPE_SUCCESS  Operation success.
 * \retval others       Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);
        ppe_buffer_t image, buffer;
        image.width = 25;
        image.height = 25;
        image.format = PPE_RGB888;
        image.memory = (uint32_t*)SOURCE_PIC_1;
        image.address = (uint32_t)image.memory;

        buffer.format = PPE_RGB565;
        buffer.memory = (uint32_t*)PIC_OUTPUT_LAYER;
        buffer.address = (uint32_t)PIC_OUTPUT_LAYER;
        buffer.width = 64;
        buffer.height = 64;
        memset(PIC_OUTPUT_LAYER, 0, sizeof(PIC_OUTPUT_LAYER));

        ppe_translate_t trans = {20, 20};
        PPE_blend(&image, &buffer, &trans, PPE_BYPASS_MODE);
    }
 * \endcode
 */
PPE_ERR PPE_blend(ppe_buffer_t *image, ppe_buffer_t *buffer, ppe_translate_t *trans,
                  PPE_BLEND_MODE blend_mode);

/**
 * \brief  blend a part of source image to target buffer
 * \param[in] image         source image.
 * \param[in] buffer        target image buffer.
 * \param[in] trans         translate information of source image.
 * \param[in] rect          the boundary of the part to be scaled.
 * \return operation result
 * \retval PPE_SUCCESS  Operation success.
 * \retval others       Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);
        ppe_buffer_t image, buffer;
        image.width = 25;
        image.height = 25;
        image.format = PPE_RGB888;
        image.memory = (uint32_t*)SOURCE_PIC_1;
        image.address = (uint32_t)image.memory;

        buffer.format = PPE_RGB565;
        buffer.memory = (uint32_t*)PIC_OUTPUT_LAYER;
        buffer.address = (uint32_t)PIC_OUTPUT_LAYER;
        buffer.width = 64;
        buffer.height = 64;
        memset(PIC_OUTPUT_LAYER, 0, sizeof(PIC_OUTPUT_LAYER));

        ppe_translate_t trans = {20, 20};
        ppe_rect_t rect = {6, 4, 20, 24};
        PPE_blend_rect(&image, &buffer, &trans, &rect, PPE_BYPASS_MODE);
    }
 * \endcode
 */
PPE_ERR PPE_blend_rect(ppe_buffer_t *image, ppe_buffer_t *buffer, ppe_translate_t *trans,
                       ppe_rect_t *rect, PPE_BLEND_MODE blend_mode);


/**
 * \brief  blend multiple images to target buffer
 * \param[in] image1        source image, must not be NULL.
 * \param[in] image2        another source image.
 * \param[in] image3        another source image.
 * \param[in] buffer        target image buffer.
 * \param[in] trans         translate information of source image.
 * \return operation result
 * \retval PPE_SUCCESS  Operation success.
 * \retval others       Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ppe_input_list_t list;
        memset(&list, 0, sizeof(list));
        ppe_layer_t layer1, layer2, layer3, layer4, layer0;
        memset(&layer0, 0, sizeof(layer0));
        memset(&layer1, 0, sizeof(layer1));
        memset(&layer2, 0, sizeof(layer2));
        layer0.buffer.address = address0;
        layer0.buffer.color_key_en = DISABLE;
        layer0.buffer.global_alpha_en = DISABLE;
        layer0.buffer.memory = (uint32_t*)address0;
        layer0.buffer.width = 480;
        layer0.buffer.height = 480;
        layer0.buffer.format = PPE_RGB888;
        list.output_layer = &layer0;

        layer1.buffer.address = address1;
        layer1.buffer.color_key_en = DISABLE;
        layer1.buffer.global_alpha_en = DISABLE;
        layer1.buffer.memory = (uint32_t*)address1;
        layer1.buffer.width = 200;
        layer1.buffer.height = 200;
        layer1.buffer.format = PPE_ARGB8888;
        list.input_layer1 = &layer1;
        layer1.trans.x = 139;
        layer1.trans.y = 89;

        layer2.buffer.address = address2;
        layer2.buffer.color_key_en = DISABLE;
        layer2.buffer.global_alpha_en = DISABLE;
        layer2.buffer.memory = (uint32_t*)address2;
        layer2.buffer.width = 200;
        layer2.buffer.height = 200;
        layer2.buffer.format = PPE_ARGB8888;
        list.input_layer2 = &layer2;
        layer2.trans.x = 89;
        layer2.trans.y = 189;

        PPE_ERR err = PPE_blend_multi(list);
    }
 * \endcode
 */
PPE_ERR PPE_blend_multi(ppe_input_list_t list);

/**
 * \brief  Get pixel size of certain format
 * \param[in] format        pixel format of PPE
 * \return pixel size
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        uint8_t pixel_size = ppe_get_format_data_len(PPE_RGB565);
    }
 * \endcode
 */
uint8_t ppe_get_format_data_len(PPE_PIXEL_FORMAT format);

/**
 * \brief  Get intersected area
 * \param[in] result_rect       porinter to rect that stores result
 * \param[in] rect1             porinter to one rectangle area
 * \param[in] rect2             porinter to another rectangle area
 * \return operation result
 * \retval true         rectangle areas have intersection.
 * \retval false        rectangle areas have no intersection.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        if (ppe_rect_intersect(&result, &rect1, &rect2))
        {
            //User operation
        }
        else
        {
            return;
        }
    }
 * \endcode
 */
bool ppe_rect_intersect(ppe_rect_t *result_rect, ppe_rect_t *rect1, ppe_rect_t *rect2);
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