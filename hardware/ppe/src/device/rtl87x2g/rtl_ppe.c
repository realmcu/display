/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* \file     rtl_ppe.c
* \brief    This file provides all the Pixel Process Engine firmware functions.
* \details
* \author   astor zhang
* \date     2023-10-17
* \version  v1.0
*********************************************************************************************************
*/

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "rtl_ppe.h"
#include "rtl_ppe_int.h"
#include "rtl_rcc.h"
#include "stddef.h"
#include "string.h"

/*============================================================================*
 *                          Private Macros
 *============================================================================*/
#define MIN(x, y)           (((x)<(y))?(x):(y))
#define MAX(x, y)           (((x)>(y))?(x):(y))

/*============================================================================*
 *                           Public Functions
 *============================================================================*/
bool ppe_rect_intersect(ppe_rect_t *result_rect, ppe_rect_t *rect1, ppe_rect_t *rect2)
{
//     if(result_rect == NULL || rect1 == NULL || rect2 == NULL)
//     {
//         return false;
//     }
    result_rect->left = MAX(rect1->left, rect2->left);
    result_rect->top = MAX(rect1->top, rect2->top);
    result_rect->right = MIN(rect1->right, rect2->right);
    result_rect->bottom = MIN(rect1->bottom, rect2->bottom);
    if (result_rect->top > result_rect->bottom || result_rect->left > result_rect->right)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void PPE_Cmd(FunctionalState state)
{
    //PPE cannot be disabled by user
    if (state)
    {
        PPE_GLB_CTL_TypeDef ppe_reg_0x400 = {.d32 = PPE->GLB_CTL};
        ppe_reg_0x400.b.glb_en = state;
        PPE->GLB_CTL = ppe_reg_0x400.d32;
    }
}

void PPE_Suspend(void)
{
    PPE_GLB_CTL_TypeDef ppe_reg_0x400 = {.d32 = PPE->GLB_CTL};
    ppe_reg_0x400.b.susp = SET;
    PPE->GLB_CTL = ppe_reg_0x400.d32;
}

void PPE_Resume(void)
{
    PPE_GLB_CTL_TypeDef ppe_reg_0x400 = {.d32 = PPE->GLB_CTL};
    if (ppe_reg_0x400.b.all_inact)
    {
        ppe_reg_0x400.b.resume = SET;
        PPE->GLB_CTL = ppe_reg_0x400.d32;
    }
}

void PPE_Abort(void)
{
    PPE_GLB_CTL_TypeDef ppe_reg_0x400 = {.d32 = PPE->GLB_CTL};
    if (ppe_reg_0x400.b.all_inact)
    {
        ppe_reg_0x400.b.abort = SET;
        PPE->GLB_CTL = ppe_reg_0x400.d32;
    }
}

ITStatus PPE_ChannelInactiveCheck(void)
{
    assert_param(IS_PPE_CHN_INDEX(chn_id));
    ITStatus rv = RESET;
    PPE_GLB_CTL_TypeDef ppe_reg_0x400 = {.d32 = PPE->GLB_CTL};
    if (ppe_reg_0x400.b.chn_inact == 0x1FFFF)
    {
        rv = SET;
    }
    return rv;
}

void PPE_Secure(FunctionalState state)
{
    PPE_MISC_TypeDef ppe_reg_0x420 = {.d32 = PPE->MISC};
    if (state)
    {
        ppe_reg_0x420.b.secure = SET;
    }
    else
    {
        ppe_reg_0x420.b.secure = RESET;
    }
    PPE->MISC = ppe_reg_0x420.d32;
}

ITStatus PPE_GetINTStatus(uint32_t PPE_INT)
{
    //assert_param(IS_PPE_INT(PPE_INT));
    ITStatus bit_status = RESET;
    if (PPE->INTR_ST & PPE_INT)
    {
        bit_status = SET;
    }
    return bit_status;
}

ITStatus PPE_GetINTStatusRaw(uint32_t PPE_INT)
{
    //assert_param(IS_PPE_INT(PPE_INT));
    ITStatus bit_status = RESET;
    if (PPE->INTR_RAW & PPE_INT)
    {
        bit_status = SET;
    }
    return bit_status;
}

void PPE_ClearINTPendingBit(uint32_t PPE_INT)
{
    //assert_param(IS_PPE_INT(PPE_INT));
    PPE->INTR_CLR |= PPE_INT;
}

void PPE_MaskINTConfig(uint32_t PPE_INT_MSK, FunctionalState NewState)
{
    /* Check the parameters */
    //assert_param(IS_PPE_INT(PPE_INT_MSK));
    //assert_param(IS_FUNCTIONAL_STATE(NewState));
    if (NewState == ENABLE)
    {
        PPE->INTR_MASK |= PPE_INT_MSK;
    }
    else
    {
        PPE->INTR_MASK &= ~PPE_INT_MSK;
    }
}

void PPE_AutoReloadCmd(FunctionalState NewState)
{
    //assert_param(IS_FUNCTIONAL_STATE(NewState));
    PPE_RELOAD_CFG_TypeDef ppe_reg_0x408 = {.d32 = PPE->RELOAD_CFG};
    if (NewState)
    {
        ppe_reg_0x408.b.layer_reload_en = SET;
    }
    else
    {
        ppe_reg_0x408.b.layer_reload_en = RESET;
    }
    PPE->RELOAD_CFG = ppe_reg_0x408.d32;
}

void PPE_AutoClearCmd(FunctionalState NewState)
{
    //assert_param(IS_FUNCTIONAL_STATE(NewState));
    PPE_RELOAD_CFG_TypeDef ppe_reg_0x408 = {.d32 = PPE->RELOAD_CFG};
    if (NewState)
    {
        ppe_reg_0x408.b.auto_clr = SET;
    }
    else
    {
        ppe_reg_0x408.b.auto_clr = RESET;
    }
    PPE->RELOAD_CFG = ppe_reg_0x408.d32;
}

void PPE_SetValid(FunctionalState NewState)
{
    //assert_param(IS_FUNCTIONAL_STATE(NewState));
    PPE_RELOAD_CFG_TypeDef ppe_reg_0x408 = {.d32 = PPE->RELOAD_CFG};
    if (NewState)
    {
        ppe_reg_0x408.b.set_valid = SET;
    }
    else
    {
        ppe_reg_0x408.b.set_valid = RESET;
    }
    PPE->RELOAD_CFG = ppe_reg_0x408.d32;
}

void PPE_structInit(PPE_InitTypeDef *PPE_init_struct)
{
    //assert_param(PPE_init_struct != NULL);
    PPE_init_struct->function = PPE_FUNCTION_ALPHA_BLEND;
}

void PPE_InputLayer_StructInit(PPE_InputLayer_InitTypeDef *layer_init_struct)
{
    layer_init_struct->byte_swap = PPE_NO_SWAP;
    layer_init_struct->AXCACHE = 0;
    layer_init_struct->AXSIZE = 0;
    layer_init_struct->color_key_en = 0;
    layer_init_struct->const_ABGR8888_value = 0xFFFFFFFF;
    layer_init_struct->format = PPE_BGRA8888;
    layer_init_struct->handshake_en = DISABLE;
    layer_init_struct->height = 0;
    layer_init_struct->width = 0;
    layer_init_struct->start_x = 0;
    layer_init_struct->start_y = 0;
    layer_init_struct->polarity = PPE_POLARITY_HIGH;
}

void PPE_ResultLayer_StructInit(PPE_ResultLayer_InitTypeDef *layer_init_struct)
{
    layer_init_struct->byte_swap = PPE_NO_SWAP;
    layer_init_struct->AXCACHE = 0;
    layer_init_struct->AXSIZE = 0;
    layer_init_struct->format = PPE_BGRA8888;
    layer_init_struct->handshake_en = DISABLE;
    layer_init_struct->height = 0;
    layer_init_struct->width = 0;
    layer_init_struct->polarity = PPE_POLARITY_HIGH;
}

void PPE_Init(PPE_InitTypeDef *PPE_init_struct)
{
    //assert_param(PPE_init_struct != NULL);
    //assert_param(IS_PPE_FUNCTION(PPE_init_struct->function));
    PPE_FUNC_CFG_TypeDef ppe_reg_0x404 = {.d32 = PPE->FUNC_CFG};
    switch (PPE_init_struct->function)
    {
    case PPE_FUNCTION_ALPHA_BLEND:
        {
            //assert_param(IS_PPE_ALPHA_BLEND_LAYER_NUM(PPE_init_struct->blend_layer_num));
            ppe_reg_0x404.b.func_sel = PPE_FUNCTION_ALPHA_BLEND;
            ppe_reg_0x404.b.blend_lay = PPE_init_struct->blend_layer_num;
            break;
        }
    case PPE_FUNCTION_SCALE:
        {
            ppe_reg_0x404.b.func_sel = PPE_FUNCTION_SCALE;
            break;
        }
    }
    PPE->FUNC_CFG = ppe_reg_0x404.d32;
}

void PPE_InitInputLayer(uint8_t id, PPE_InputLayer_InitTypeDef *layer_init_struct)
{
    uint8_t cov_layer_id = id - 1;
    PPE_LAYER->INPUT_LAYER[cov_layer_id].LAYERx_ADDR_L = layer_init_struct->src_addr;
    PPE_LAYER->INPUT_LAYER[cov_layer_id].LAYERx_CONST_PIX = layer_init_struct->const_ABGR8888_value;
    PPE_LAYERX_POS_TypeDef ppe_reg_0x48 = {.d32 = PPE_LAYER->INPUT_LAYER[cov_layer_id].LAYERx_POS};
    ppe_reg_0x48.b.start_x = layer_init_struct->start_x;
    ppe_reg_0x48.b.start_y = layer_init_struct->start_y;
    PPE_LAYER->INPUT_LAYER[cov_layer_id].LAYERx_POS = ppe_reg_0x48.d32;

    PPE_LAYERX_WIN_SIZE_TypeDef ppe_reg_0x4c = {.d32 = PPE_LAYER->INPUT_LAYER[cov_layer_id].LAYERx_WIN_SIZE};
    ppe_reg_0x4c.b.width = layer_init_struct->width;
    ppe_reg_0x4c.b.height = layer_init_struct->height;
    PPE_LAYER->INPUT_LAYER[cov_layer_id].LAYERx_WIN_SIZE = ppe_reg_0x4c.d32;

    PPE_LAYERX_PIC_CFG_TypeDef ppe_reg_0x54 = {.d32 = PPE_LAYER->INPUT_LAYER[cov_layer_id].LAYERx_PIC_CFG};
    ppe_reg_0x54.b.pix_src = layer_init_struct->src;
    ppe_reg_0x54.b.format = layer_init_struct->format;
    ppe_reg_0x54.b.key_en = layer_init_struct->color_key_en;
    ppe_reg_0x54.b.line_len = layer_init_struct->line_len;
    PPE_LAYER->INPUT_LAYER[cov_layer_id].LAYERx_PIC_CFG = ppe_reg_0x54.d32;

    PPE_LAYER->INPUT_LAYER[cov_layer_id].LAYERx_KEY_COLOR = layer_init_struct->key_color_value;

    PPE_LAYERX_HS_CFG_TypeDef ppe_reg_0x60 = {.d32 = PPE_LAYER->INPUT_LAYER[cov_layer_id].LAYERx_HS_CFG};
    ppe_reg_0x60.b.hw_index = layer_init_struct->hw_index;
    ppe_reg_0x60.b.msize_log = layer_init_struct->handshake_msize;
    ppe_reg_0x60.b.hs_en = layer_init_struct->handshake_en;
    ppe_reg_0x60.b.polar = layer_init_struct->polarity;
    ppe_reg_0x60.b.hwhs = layer_init_struct->handshake_mode;
    PPE_LAYER->INPUT_LAYER[cov_layer_id].LAYERx_HS_CFG = ppe_reg_0x60.d32;

    PPE_LAYERX_BUS_CFG_TypeDef ppe_reg_0x5c = {.d32 = PPE_LAYER->INPUT_LAYER[cov_layer_id].LAYERx_BUS_CFG};
    ppe_reg_0x5c.b.axcache = layer_init_struct->AXCACHE;
    ppe_reg_0x5c.b.axsize = layer_init_struct->AXSIZE;
    ppe_reg_0x5c.b.incr = layer_init_struct->INCR;
    ppe_reg_0x5c.b.max_axlen_log = layer_init_struct->MAX_AXLEN_LOG;
    ppe_reg_0x5c.b.prior = layer_init_struct->PRIOR;
    PPE_LAYER->INPUT_LAYER[cov_layer_id].LAYERx_BUS_CFG = ppe_reg_0x5c.d32;

}

void PPE_InitResultLayer(PPE_ResultLayer_InitTypeDef *layer_init_struct)
{
    PPE_LAYER->RESULT_LAYER.LAYER0_ADDR_L = layer_init_struct->src_addr;
    PPE_LAYER0_WIN_SIZE_TypeDef ppe_reg_0x0c = {.d32 = PPE_LAYER->RESULT_LAYER.LAYER0_WIN_SIZE};
    ppe_reg_0x0c.b.width = layer_init_struct->width;
    ppe_reg_0x0c.b.height = layer_init_struct->height;
    PPE_LAYER->RESULT_LAYER.LAYER0_WIN_SIZE = ppe_reg_0x0c.d32;

    PPE_LAYER0_PIC_CFG_TypeDef ppe_reg_0x14 = {.d32 = PPE_LAYER->RESULT_LAYER.LAYER0_PIC_CFG};
    ppe_reg_0x14.b.format = layer_init_struct->format;
    ppe_reg_0x14.b.line_len = layer_init_struct->line_len;
    PPE_LAYER->RESULT_LAYER.LAYER0_PIC_CFG = ppe_reg_0x14.d32;

    PPE_LAYER0_HS_CFG_TypeDef ppe_reg_0x20 = {.d32 = PPE_LAYER->RESULT_LAYER.LAYER0_HS_CFG};
    ppe_reg_0x20.b.hw_index = layer_init_struct->hw_index;
    ppe_reg_0x20.b.msize_log = layer_init_struct->handshake_msize;
    ppe_reg_0x20.b.hs_en = layer_init_struct->handshake_en;
    ppe_reg_0x20.b.polar = layer_init_struct->polarity;
    ppe_reg_0x20.b.hw_hs = layer_init_struct->handshake_mode;
    PPE_LAYER->RESULT_LAYER.LAYER0_HS_CFG = ppe_reg_0x20.d32;

    PPE_LAYER0_BUS_CFG_TypeDef ppe_reg_0x1c = {.d32 = PPE_LAYER->RESULT_LAYER.LAYER0_BUS_CFG};
    ppe_reg_0x1c.b.axcache = layer_init_struct->AXCACHE;
    ppe_reg_0x1c.b.axsize = layer_init_struct->AXSIZE;
    ppe_reg_0x1c.b.incr = layer_init_struct->INCR;
    ppe_reg_0x1c.b.max_axlen_log = layer_init_struct->MAX_AXLEN_LOG;
    ppe_reg_0x1c.b.prior = layer_init_struct->PRIOR;
    ppe_reg_0x1c.b.byte_swap = layer_init_struct->byte_swap;
    PPE_LAYER->RESULT_LAYER.LAYER0_BUS_CFG = ppe_reg_0x1c.d32;
}

static void PPE_LLI_Init_result_LAYER(PPE_LLI_LAYER *LLI_layer, uint32_t input_layer_num)
{
    LLI_layer->LAYER_ADDR                   = PPE_LAYER->RESULT_LAYER.LAYER0_ADDR_L;
    LLI_layer->LAYER_POS                    = 0;
    LLI_layer->LAYER_WINSIZE                = PPE_LAYER->RESULT_LAYER.LAYER0_WIN_SIZE;
    LLI_layer->LAYER_CONST_PIX              = 0;
}

static void PPE_LLI_Init_Input_LAYER(PPE_LLI_LAYER *LLI_layer, uint32_t input_layer_num)
{
    LLI_layer->LAYER_ADDR                   = PPE_LAYER->INPUT_LAYER[input_layer_num - 1].LAYERx_ADDR_L;
    LLI_layer->LAYER_POS                    = PPE_LAYER->INPUT_LAYER[input_layer_num - 1].LAYERx_POS;
    LLI_layer->LAYER_WINSIZE                = PPE_LAYER->INPUT_LAYER[input_layer_num -
                                                                                     1].LAYERx_WIN_SIZE;
    LLI_layer->LAYER_CONST_PIX              = PPE_LAYER->INPUT_LAYER[input_layer_num -
                                                                                     1].LAYERx_CONST_PIX;
}

PPE_ERR PPE_Scale(ppe_buffer_t *image, ppe_buffer_t *buffer, float x_ratio, float y_ratio)
{
    if (buffer == NULL)
    {
        return PPE_ERROR_NULL_TARGET;
    }
    if (image == NULL)
    {
        return PPE_ERROR_NULL_SOURCE;
    }
    PPE_InputLayer_InitTypeDef PPE_Input_Layer;
    PPE_ResultLayer_InitTypeDef PPE_Result_Layer;
    PPE_InitTypeDef PPE_Init_User;

    PPE_InputLayer_StructInit(&PPE_Input_Layer);
    PPE_ResultLayer_StructInit(&PPE_Result_Layer);
    PPE_structInit(&PPE_Init_User);
    if ((image->address % 4) || (buffer->address % 4))
    {
        return PPE_ERROR_ADDR_NOT_ALIGNED;
    }
    RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);

    PPE_Input_Layer.src_addr                = (uint32_t)image->memory;
    PPE_Input_Layer.start_x                 = 0;
    PPE_Input_Layer.start_y                 = 0;
    PPE_Input_Layer.width                   = image->width;
    PPE_Input_Layer.height                  = image->height;
    PPE_Input_Layer.const_ABGR8888_value    = 0XFFFFFFFF;
    PPE_Input_Layer.format                  = image->format;
    PPE_Input_Layer.src                     = PPE_LAYER_SRC_FROM_DMA;
    PPE_Input_Layer.color_key_en            = DISABLE;
    PPE_Input_Layer.line_len                = image->stride;
    PPE_Input_Layer.key_color_value         = 0x0;
    PPE_Input_Layer.AXSIZE                  = 2; // 32bit bandwidth;
    PPE_Input_Layer.INCR                    = PPE_ARBURST_INCR;
    PPE_Input_Layer.AXCACHE                 = 1;
    PPE_Input_Layer.MAX_AXLEN_LOG           = PPE_MAX_AXLEN_127;
    PPE_Input_Layer.PRIOR                   = 0;
    PPE_Input_Layer.byte_swap               = PPE_NO_SWAP;
    PPE_Input_Layer.handshake_mode          = PPE_DMA_SW_HANDSHAKE;
    PPE_Input_Layer.polarity                = PPE_POLARITY_HIGH;
    PPE_Input_Layer.handshake_en            = DISABLE;
    PPE_Input_Layer.handshake_msize         = PPE_MSIZE_2;
    PPE_Input_Layer.hw_index                = 0;
    PPE_InitInputLayer(1, &PPE_Input_Layer);

    PPE->SCA_RATIO_X = (uint32_t)(65536 / x_ratio); // 65536/ratio_x,ratio = 2
    PPE->SCA_RATIO_Y = (uint32_t)(65536 / y_ratio);
    buffer->width   = (uint32_t)PPE_Input_Layer.width * x_ratio;
    buffer->height  = (uint32_t)PPE_Input_Layer.height * y_ratio;
    if (buffer->width == 0 || buffer->height == 0)
    {
        return PPE_SUCCESS_NOT_CHANGE;
    }
    PPE_Result_Layer.src_addr               = (uint32_t)buffer->memory;
    PPE_Result_Layer.width                  = buffer->width;
    PPE_Result_Layer.height                 = buffer->height;
    PPE_Result_Layer.format                 = buffer->format;
    PPE_Result_Layer.line_len               = PPE_Result_Layer.width;
    PPE_Result_Layer.AXSIZE                 = 2;    // 32bit bandwidth
    PPE_Result_Layer.INCR                   = PPE_ARBURST_INCR;
    PPE_Result_Layer.AXCACHE                = 1;
    PPE_Result_Layer.MAX_AXLEN_LOG          = PPE_MAX_AXLEN_127;
    PPE_Result_Layer.PRIOR                  = 0;
    PPE_Result_Layer.byte_swap              = PPE_NO_SWAP;
    PPE_Result_Layer.handshake_mode         = PPE_DMA_SW_HANDSHAKE;
    PPE_Result_Layer.polarity               = PPE_POLARITY_HIGH;
    PPE_Result_Layer.handshake_en           = DISABLE;
    PPE_Result_Layer.handshake_msize        = PPE_MSIZE_2;
    PPE_Result_Layer.hw_index               = 0;
    PPE_InitResultLayer(&PPE_Result_Layer);

    PPE_Init_User.function          =   PPE_FUNCTION_SCALE;
    PPE_Init(&PPE_Init_User);

    PPE_Secure(ENABLE);
    PPE_Cmd(ENABLE);
    while (PPE->GLB_CTL & BIT0);

    return PPE_SUCCESS;
}

uint8_t ppe_get_format_data_len(PPE_PIXEL_FORMAT format)
{
    if (((format >= PPE_ABGR8888) && (format <= PPE_RGBX8888))
        || ((format >= PPE_ABGR8666) && (format <= PPE_RGBX6668)))
    {
        return 4;
    }
    if (((format >= PPE_ABGR4444) && (format <= PPE_RGBX4444))
        || ((format >= PPE_ABGR1555) && (format <= PPE_RGBX5551))
        || ((format >= PPE_BGR565) && (format <= PPE_RGB565)))
    {
        return 2;
    }
    if (((format >= PPE_ABGR2222) && (format <= PPE_RGBX2222))
        || ((format >= PPE_A8) && (format <= PPE_X8)))
    {
        return 1;
    }
    if (((format >= PPE_ABGR8565) && (format <= PPE_RGBX5658))
        || ((format >= PPE_BGR888) && (format <= PPE_RGB888)))
    {
        return 3;
    }
    return 0;
}

PPE_ERR PPE_Scale_Rect(ppe_buffer_t *image, ppe_buffer_t *buffer, float x_ratio, float y_ratio,
                       ppe_rect_t *rect)
{
    if (buffer == NULL)
    {
        return PPE_ERROR_NULL_TARGET;
    }
    if (image == NULL)
    {
        return PPE_ERROR_NULL_SOURCE;
    }
    PPE_InputLayer_InitTypeDef PPE_Input_Layer;
    PPE_ResultLayer_InitTypeDef PPE_Result_Layer;
    PPE_InitTypeDef PPE_Init_User;

    PPE_InputLayer_StructInit(&PPE_Input_Layer);
    PPE_ResultLayer_StructInit(&PPE_Result_Layer);
    PPE_structInit(&PPE_Init_User);
    if ((image->address % 4) || (buffer->address % 4))
    {
        return PPE_ERROR_ADDR_NOT_ALIGNED;
    }
    if ((rect == NULL) || (rect->top > rect->bottom) || (rect->left > rect->right)
        || (rect->right >= image->width) || (rect->bottom >= image->height))
    {
        return PPE_ERROR_INVALID_PARAM;
    }
    uint8_t format_len = ppe_get_format_data_len(image->format);
    if (!format_len)
    {
        return PPE_ERROR_UNKNOWN_FORMAT;
    }

    RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);

    PPE_Input_Layer.src_addr                = (uint32_t)(image->address +
                                                         (rect->left + image->stride * rect->top) * format_len);
    PPE_Input_Layer.start_x                 = 0;
    PPE_Input_Layer.start_y                 = 0;
    PPE_Input_Layer.width                   = rect->right - rect->left + 1;
    PPE_Input_Layer.height                  = rect->bottom - rect->top + 1;
    PPE_Input_Layer.const_ABGR8888_value    = 0XFFFFFFFF;
    PPE_Input_Layer.format                  = image->format;
    PPE_Input_Layer.src                     = PPE_LAYER_SRC_FROM_DMA;
    PPE_Input_Layer.color_key_en            = DISABLE;
    PPE_Input_Layer.line_len                = image->stride;
    PPE_Input_Layer.key_color_value         = 0x000000;
    PPE_Input_Layer.AXSIZE                  = 2; // 32bit bandwidth;
    PPE_Input_Layer.INCR                    = PPE_ARBURST_INCR;
    PPE_Input_Layer.AXCACHE                 = 1;
    PPE_Input_Layer.MAX_AXLEN_LOG           = PPE_MAX_AXLEN_127;
    PPE_Input_Layer.PRIOR                   = 0;
    PPE_Input_Layer.byte_swap               = PPE_NO_SWAP;
    PPE_Input_Layer.handshake_mode          = PPE_DMA_SW_HANDSHAKE;
    PPE_Input_Layer.polarity                = PPE_POLARITY_HIGH;
    PPE_Input_Layer.handshake_en            = DISABLE;
    PPE_Input_Layer.handshake_msize         = PPE_MSIZE_2;
    PPE_Input_Layer.hw_index                = 0;
    PPE_InitInputLayer(1, &PPE_Input_Layer);
    PPE->SCA_RATIO_X = (uint32_t)(65536 / x_ratio); // 65536/ratio_x,ratio = 2
    PPE->SCA_RATIO_Y = (uint32_t)(65536 / y_ratio);
    buffer->width   = (uint32_t)PPE_Input_Layer.width * x_ratio;
    buffer->height  = (uint32_t)PPE_Input_Layer.height * y_ratio;
    buffer->stride  = buffer->width;
    if (buffer->width == 0 || buffer->height == 0)
    {
        return PPE_SUCCESS_NOT_CHANGE;
    }
    PPE_Result_Layer.src_addr               = (uint32_t)buffer->memory;
    PPE_Result_Layer.width                  = buffer->width;
    PPE_Result_Layer.height                 = buffer->height;
    PPE_Result_Layer.format                 = buffer->format;
    PPE_Result_Layer.line_len               = buffer->stride;
    PPE_Result_Layer.AXSIZE                 = 2;    // 32bit bandwidth
    PPE_Result_Layer.INCR                   = PPE_ARBURST_INCR;
    PPE_Result_Layer.AXCACHE                = 1;
    PPE_Result_Layer.MAX_AXLEN_LOG          = PPE_MAX_AXLEN_127;
    PPE_Result_Layer.PRIOR                  = 0;
    PPE_Result_Layer.byte_swap              = PPE_NO_SWAP;
    PPE_Result_Layer.handshake_mode         = PPE_DMA_SW_HANDSHAKE;
    PPE_Result_Layer.polarity               = PPE_POLARITY_HIGH;
    PPE_Result_Layer.handshake_en           = DISABLE;
    PPE_Result_Layer.handshake_msize        = PPE_MSIZE_2;
    PPE_Result_Layer.hw_index               = 0;
    PPE_InitResultLayer(&PPE_Result_Layer);

    PPE_Init_User.function          =   PPE_FUNCTION_SCALE;
    PPE_Init(&PPE_Init_User);

    PPE_Secure(ENABLE);
    PPE_Cmd(ENABLE);
    while (PPE->GLB_CTL & BIT0);

    return PPE_SUCCESS;
}

PPE_ERR PPE_Clear(ppe_buffer_t *buffer, uint32_t color)
{
    if (buffer == NULL)
    {
        return PPE_ERROR_NULL_TARGET;
    }
    /*declaration of input/result layer initialization struct*/
    PPE_InputLayer_InitTypeDef          PPE_Input_Layer1;
    PPE_ResultLayer_InitTypeDef         PPE_Result_Layer;
    PPE_InitTypeDef                     PPE_Init_User;

    PPE_InputLayer_StructInit(&PPE_Input_Layer1);
    PPE_ResultLayer_StructInit(&PPE_Result_Layer);
    PPE_structInit(&PPE_Init_User);

    RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);

    /*initial input layer1*/
    PPE_Input_Layer1.src_addr                       = (uint32_t)NULL;
    PPE_Input_Layer1.start_x                        = 0;
    PPE_Input_Layer1.start_y                        = 0;
    PPE_Input_Layer1.width                          = buffer->width;
    PPE_Input_Layer1.height                         = buffer->height;
    PPE_Input_Layer1.const_ABGR8888_value           = color;
    PPE_Input_Layer1.format                         = PPE_ABGR8888;
    PPE_Input_Layer1.src                            = PPE_LAYER_SRC_CONST;
    PPE_Input_Layer1.color_key_en                   = DISABLE;
    PPE_Input_Layer1.line_len                       = PPE_Input_Layer1.width;
    PPE_Input_Layer1.key_color_value                = 0;
    PPE_Input_Layer1.AXSIZE                         = 2;// 32bit bandwidth;
    PPE_Input_Layer1.INCR                           = PPE_ARBURST_FIX;
    PPE_Input_Layer1.AXCACHE                        = 1;
    PPE_Input_Layer1.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_127;
    PPE_Input_Layer1.PRIOR                          = 0;
    PPE_Input_Layer1.byte_swap                      = PPE_NO_SWAP;
    PPE_Input_Layer1.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
    PPE_Input_Layer1.polarity                       = PPE_POLARITY_HIGH;
    PPE_Input_Layer1.handshake_en                   = DISABLE;
    PPE_Input_Layer1.handshake_msize                = PPE_MSIZE_2;
    PPE_Input_Layer1.hw_index                       = 0;
    PPE_InitInputLayer(1, &PPE_Input_Layer1);

    /*initial result layer*/
    PPE_Result_Layer.src_addr                   = (uint32_t)buffer->address;
    PPE_Result_Layer.width                      = buffer->width;
    PPE_Result_Layer.height                     = buffer->height;
    PPE_Result_Layer.format                     = buffer->format;
    PPE_Result_Layer.line_len                   = PPE_Result_Layer.width;
    PPE_Result_Layer.AXSIZE                     = 2;    // 32bit bandwidth
    PPE_Result_Layer.INCR                       = PPE_ARBURST_INCR;
    PPE_Result_Layer.AXCACHE                    = 1;
    PPE_Result_Layer.MAX_AXLEN_LOG              = PPE_MAX_AXLEN_127;
    PPE_Result_Layer.PRIOR                      = 0;
    PPE_Result_Layer.byte_swap                  = PPE_NO_SWAP;
    PPE_Result_Layer.handshake_mode             = PPE_DMA_SW_HANDSHAKE;
    PPE_Result_Layer.polarity                   = PPE_POLARITY_HIGH;
    PPE_Result_Layer.handshake_en               = DISABLE;
    PPE_Result_Layer.handshake_msize            = PPE_MSIZE_2;
    PPE_Result_Layer.hw_index                   = 0;
    PPE_InitResultLayer(&PPE_Result_Layer);


    /*initial PPE global*/
    PPE_Init_User.function   = PPE_FUNCTION_ALPHA_BLEND;
    PPE_Init_User.blend_layer_num = 1;
    PPE_Init(&PPE_Init_User);

    PPE_Secure(ENABLE);  /*secure for all channel*/

    /*enable PPE*/
    PPE_Cmd(ENABLE);
    while (PPE->GLB_CTL & BIT0);

    return PPE_SUCCESS;
}


PPE_ERR PPE_Clear_Rect(ppe_buffer_t *buffer, ppe_rect_t *rect, uint32_t color)
{
    /*declaration of input/result layer initialization struct*/
    PPE_InputLayer_InitTypeDef      PPE_Input_Layer1;
    PPE_InputLayer_InitTypeDef      PPE_Input_Layer2;
    PPE_ResultLayer_InitTypeDef     PPE_Result_Layer;
    PPE_InitTypeDef                 PPE_Init_User;

    PPE_InputLayer_StructInit(&PPE_Input_Layer1);
    PPE_InputLayer_StructInit(&PPE_Input_Layer2);
    PPE_ResultLayer_StructInit(&PPE_Result_Layer);
    PPE_structInit(&PPE_Init_User);
    if (buffer == NULL)
    {
        return PPE_ERROR_NULL_TARGET;
    }
    if ((rect == NULL) || (rect->top > rect->bottom) || (rect->left > rect->right))
    {
        return PPE_ERROR_INVALID_PARAM;
    }
    uint8_t format_len = ppe_get_format_data_len(buffer->format);
    if (format_len == 0)
    {
        return PPE_ERROR_UNKNOWN_FORMAT;
    }

    ppe_rect_t buffer_rect = {.left = 0, .right = buffer->width - 1, .top = 0, .bottom = buffer->height - 1};
    ppe_rect_t transfer_rect;
    if (!ppe_rect_intersect(&transfer_rect, &buffer_rect, rect))
    {
        return PPE_SUCCESS;
    }

    /*initial input layer2*/
    RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);

    PPE_Input_Layer2.src_addr                = (uint32_t)NULL;
    PPE_Input_Layer2.width                          = transfer_rect.right - transfer_rect.left + 1;
    PPE_Input_Layer2.height                         = transfer_rect.bottom - transfer_rect.top + 1;
    if (buffer->global_alpha_en)
    {
        uint32_t mix_color = (((color >> 24) * buffer->global_alpha / 255) << 24) + (color & 0xFFFFFF);
        PPE_Input_Layer2.const_ABGR8888_value           =  mix_color;
    }
    else
    {
        PPE_Input_Layer2.const_ABGR8888_value           = color;
    }
    PPE_Input_Layer2.color_key_en                   = DISABLE;
    PPE_Input_Layer2.key_color_value                = 0;
    PPE_Input_Layer2.format                         = PPE_ARGB8888;
    PPE_Input_Layer2.src                            = PPE_LAYER_SRC_CONST;
    PPE_Input_Layer2.line_len                       = transfer_rect.right - transfer_rect.left + 1;
    PPE_Input_Layer2.AXSIZE                         = 2;// 32bit bandwidth;
    PPE_Input_Layer2.INCR                           = PPE_ARBURST_FIX;
    PPE_Input_Layer2.AXCACHE                        = 2;
    PPE_Input_Layer2.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_15;
    PPE_Input_Layer2.PRIOR                          = 0;
    PPE_Input_Layer2.byte_swap                      = PPE_NO_SWAP;
    PPE_Input_Layer2.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
    PPE_Input_Layer2.polarity                       = PPE_POLARITY_HIGH;
    PPE_Input_Layer2.handshake_en                   = DISABLE;
    PPE_Input_Layer2.handshake_msize                = PPE_MSIZE_2;
    PPE_Input_Layer2.hw_index                       = 0;
    if ((PPE_Input_Layer2.const_ABGR8888_value >> 24) < 0xFF)
    {
        /*initial input layer1*/
        PPE_InitInputLayer(2, &PPE_Input_Layer2);
        PPE_Input_Layer1.start_x                        = 0;
        PPE_Input_Layer1.start_y                        = 0;
        PPE_Input_Layer1.width                          = transfer_rect.right - transfer_rect.left + 1;
        PPE_Input_Layer1.height                         = transfer_rect.bottom - transfer_rect.top + 1;
        PPE_Input_Layer1.src_addr                       = (uint32_t)buffer->memory +
                                                          (transfer_rect.left + transfer_rect.top * buffer->width) * ppe_get_format_data_len(buffer->format);
        PPE_Input_Layer1.const_ABGR8888_value           = 0XFF000000;
        PPE_Input_Layer1.format                         = buffer->format;
        PPE_Input_Layer1.src                            = PPE_LAYER_SRC_FROM_DMA;
        PPE_Input_Layer1.color_key_en                   = DISABLE;
        PPE_Input_Layer1.line_len                       = buffer->width;
        PPE_Input_Layer1.key_color_value                = buffer->color_key_value;
        PPE_Input_Layer1.AXSIZE                         = 2;// 32bit bandwidth;
        PPE_Input_Layer1.INCR                           = PPE_ARBURST_INCR;
        PPE_Input_Layer1.AXCACHE                        = 1;
        PPE_Input_Layer1.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_15;
        PPE_Input_Layer1.PRIOR                          = 0;
        PPE_Input_Layer1.byte_swap                      = PPE_NO_SWAP;
        PPE_Input_Layer1.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
        PPE_Input_Layer1.polarity                       = PPE_POLARITY_HIGH;
        PPE_Input_Layer1.handshake_en                   = DISABLE;
        PPE_Input_Layer1.handshake_msize                = PPE_MSIZE_2;
        PPE_Input_Layer1.hw_index                       = 0;
        PPE_InitInputLayer(1, &PPE_Input_Layer1);
        PPE_Init_User.blend_layer_num                   = 2;
    }
    else
    {
        memcpy(&PPE_Input_Layer1, &PPE_Input_Layer2, sizeof(PPE_InputLayer_InitTypeDef));
        PPE_InitInputLayer(1, &PPE_Input_Layer1);
        PPE_Init_User.blend_layer_num                   = 1;
    }

    /*initial result layer*/
    PPE_Result_Layer.width                          = transfer_rect.right - transfer_rect.left + 1;
    PPE_Result_Layer.height                         = transfer_rect.bottom - transfer_rect.top + 1;
    PPE_Result_Layer.src_addr                       = (uint32_t)buffer->memory +
                                                      (transfer_rect.left + transfer_rect.top * buffer->width) * ppe_get_format_data_len(buffer->format);
    PPE_Result_Layer.format                         = buffer->format;
    PPE_Result_Layer.line_len                       = buffer->width;
    PPE_Result_Layer.AXSIZE                         = 2;// 32bit bandwidth
    PPE_Result_Layer.INCR                           = PPE_ARBURST_INCR;
    PPE_Result_Layer.AXCACHE                        = 1;
    PPE_Result_Layer.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_15;
    PPE_Result_Layer.PRIOR                          = 0;
    PPE_Result_Layer.byte_swap                      = PPE_NO_SWAP;
    PPE_Result_Layer.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
    PPE_Result_Layer.polarity                       = PPE_POLARITY_HIGH;
    PPE_Result_Layer.handshake_en                   = DISABLE;
    PPE_Result_Layer.handshake_msize                = PPE_MSIZE_2;
    PPE_Result_Layer.hw_index                       = 0;
    PPE_InitResultLayer(&PPE_Result_Layer);

    /*initial PPE global*/
    PPE_Init_User.function                          =   PPE_FUNCTION_ALPHA_BLEND;
    PPE_Init(&PPE_Init_User);


    PPE_Secure(ENABLE);  /*secure for all channel*/

    /*enable PPE*/
    PPE_Cmd(ENABLE);
    while (PPE->GLB_CTL & BIT0);
    return PPE_SUCCESS;
}

#define WINDOW      1
PPE_ERR PPE_Blend(ppe_buffer_t *image, ppe_buffer_t *buffer, ppe_translate_t *trans,
                  PPE_BLEND_MODE blend_mode)
{
    if (buffer == NULL)
    {
        return PPE_ERROR_NULL_TARGET;
    }
    if (image == NULL)
    {
        return PPE_ERROR_NULL_SOURCE;
    }
    if ((trans->x < -(image->width - 1)) || (trans->x >= (buffer->width - 1)) \
        || (trans->y < -(image->height - 1)) || (trans->y >= (buffer->height - 1)))
    {
        return PPE_ERROR_OUT_OF_RANGE;
    }
    uint8_t format_len = ppe_get_format_data_len(image->format);
    if (format_len == 0)
    {
        return PPE_ERROR_UNKNOWN_FORMAT;
    }

    RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);
    /*declaration of input/result layer initialization struct*/
    PPE_InputLayer_InitTypeDef      PPE_Input_Layer1;
    PPE_InputLayer_InitTypeDef      PPE_Input_Layer2;
    PPE_ResultLayer_InitTypeDef     PPE_Result_Layer;
    PPE_InitTypeDef                 PPE_Init_User;

    PPE_InputLayer_StructInit(&PPE_Input_Layer1);
    PPE_InputLayer_StructInit(&PPE_Input_Layer2);
    PPE_ResultLayer_StructInit(&PPE_Result_Layer);
    PPE_structInit(&PPE_Init_User);

    ppe_rect_t source_rect = {.left = 0, .right = image->width - 1, \
                              .top = 0, .bottom = image->height - 1
                             };
    /*initial input layer1*/
    ppe_rect_t target_rect = {.left = trans->x, .top = trans->y};
#if WINDOW
    PPE_Input_Layer2.start_x = 0;
    PPE_Input_Layer2.start_y = 0;
#else
    PPE_Input_Layer2.start_x = trans->x;
    PPE_Input_Layer2.start_y = trans->y;
#endif
    if (trans->x < 0)
    {
        source_rect.left = -trans->x;
        target_rect.left = 0;
#if !WINDOW
        PPE_Input_Layer2.start_x = 0;
#endif
    }
    if ((trans->x + image->width) > buffer->width)
    {
        source_rect.right = buffer->width - trans->x - 1;
    }
    if (trans->y < 0)
    {
        source_rect.top = -trans->y;
        target_rect.top = 0;
#if !WINDOW
        PPE_Input_Layer2.start_y = 0;
#endif
    }
    if ((trans->y + image->height) > buffer->height)
    {
        source_rect.bottom = buffer->height - trans->y - 1;
    }
#if WINDOW
    PPE_Input_Layer2.src_addr                       = (uint32_t)image->memory +
                                                      (source_rect.left + image->width * source_rect.top) * format_len;
    PPE_Input_Layer2.width                          = source_rect.right - source_rect.left + 1;
    PPE_Input_Layer2.height                         = source_rect.bottom - source_rect.top + 1;
#else
    PPE_Input_Layer2.src_addr                       = (uint32_t)image->memory;
    PPE_Input_Layer2.width                          = image->width;
    PPE_Input_Layer2.height                         = image->height;
#endif
    if (image->global_alpha_en)
    {
        PPE_Input_Layer2.const_ABGR8888_value           = image->global_alpha << 24;
    }
    else
    {
        PPE_Input_Layer2.const_ABGR8888_value           = 0xFFFFFFFF;
    }
    PPE_Input_Layer2.format                         = image->format;
    PPE_Input_Layer2.src                            = PPE_LAYER_SRC_FROM_DMA;
    if (image->color_key_en)
    {
        PPE_Input_Layer2.color_key_en                   = ENABLE;
        PPE_Input_Layer2.key_color_value                = image->color_key_value;
    }
    else
    {
        PPE_Input_Layer2.color_key_en                   = DISABLE;
        PPE_Input_Layer2.key_color_value                = 0;
    }
    PPE_Input_Layer2.line_len                       = image->stride;
    PPE_Input_Layer2.AXSIZE                         = 2;// 32bit bandwidth;
    PPE_Input_Layer2.INCR                           = PPE_ARBURST_INCR;
    PPE_Input_Layer2.AXCACHE                        = 1;
    PPE_Input_Layer2.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_127;
    PPE_Input_Layer2.PRIOR                          = 0;
    PPE_Input_Layer2.byte_swap                      = PPE_NO_SWAP;
    PPE_Input_Layer2.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
    PPE_Input_Layer2.polarity                       = PPE_POLARITY_HIGH;
    PPE_Input_Layer2.handshake_en                   = DISABLE;
    PPE_Input_Layer2.handshake_msize                = PPE_MSIZE_1024;
    PPE_Input_Layer2.hw_index                       = 0;
    PPE_InitInputLayer(2, &PPE_Input_Layer2);

    if (blend_mode == PPE_BYPASS_MODE)
    {
        /*initial input layer2*/
        PPE_Input_Layer1.start_x                        = 0;
        PPE_Input_Layer1.start_y                        = 0;
#if WINDOW
        PPE_Input_Layer1.src_addr                       = (uint32_t)NULL;
        PPE_Input_Layer1.width                          = source_rect.right - source_rect.left + 1;
        PPE_Input_Layer1.height                         = source_rect.bottom - source_rect.top + 1;
#else
        PPE_Input_Layer1.src_addr                       = (uint32_t)buffer->memory;
        PPE_Input_Layer1.width                          = buffer->width;
        PPE_Input_Layer1.height                         = buffer->height;
#endif
        PPE_Input_Layer1.const_ABGR8888_value           = 0;
        PPE_Input_Layer1.format                         = buffer->format;
        PPE_Input_Layer1.src                            = PPE_LAYER_SRC_CONST;
        PPE_Input_Layer1.color_key_en                   = DISABLE;
        PPE_Input_Layer1.line_len                       = buffer->width;
        PPE_Input_Layer1.color_key_en                   = DISABLE;
        PPE_Input_Layer1.key_color_value                = 0;
        PPE_Input_Layer1.AXSIZE                         = 2;// 32bit bandwidth;
        PPE_Input_Layer1.INCR                           = PPE_ARBURST_INCR;
        PPE_Input_Layer1.AXCACHE                        = 1;
        PPE_Input_Layer1.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_127;
        PPE_Input_Layer1.PRIOR                          = 0;
        PPE_Input_Layer1.byte_swap                      = PPE_NO_SWAP;
        PPE_Input_Layer1.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
        PPE_Input_Layer1.polarity                       = PPE_POLARITY_HIGH;
        PPE_Input_Layer1.handshake_en                   = DISABLE;
        PPE_Input_Layer1.handshake_msize                = PPE_MSIZE_1024;
        PPE_Input_Layer1.hw_index                       = 0;
        PPE_InitInputLayer(1, &PPE_Input_Layer1);
    }
    else
    {
        /*initial input layer2*/
        PPE_Input_Layer1.start_x                        = 0;
        PPE_Input_Layer1.start_y                        = 0;
#if WINDOW
        PPE_Input_Layer1.src_addr                       = (uint32_t)buffer->memory +
                                                          (target_rect.left + target_rect.top * buffer->width) * ppe_get_format_data_len(buffer->format);
        PPE_Input_Layer1.width                          = source_rect.right - source_rect.left + 1;
        PPE_Input_Layer1.height                         = source_rect.bottom - source_rect.top + 1;
#else
        PPE_Input_Layer1.src_addr                       = (uint32_t)buffer->memory;
        PPE_Input_Layer1.width                          = buffer->width;
        PPE_Input_Layer1.height                         = buffer->height;
#endif
        PPE_Input_Layer1.const_ABGR8888_value           = 0XFFAABBCC;
        PPE_Input_Layer1.format                         = buffer->format;
        PPE_Input_Layer1.src                            = PPE_LAYER_SRC_FROM_DMA;
        PPE_Input_Layer1.color_key_en                   = DISABLE;
        PPE_Input_Layer1.line_len                       = buffer->width;
        PPE_Input_Layer1.key_color_value                = buffer->color_key_value;
        PPE_Input_Layer1.AXSIZE                         = 2;// 32bit bandwidth;
        PPE_Input_Layer1.INCR                           = PPE_ARBURST_INCR;
        PPE_Input_Layer1.AXCACHE                        = 1;
        PPE_Input_Layer1.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_127;
        PPE_Input_Layer1.PRIOR                          = 0;
        PPE_Input_Layer1.byte_swap                      = PPE_NO_SWAP;
        PPE_Input_Layer1.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
        PPE_Input_Layer1.polarity                       = PPE_POLARITY_HIGH;
        PPE_Input_Layer1.handshake_en                   = DISABLE;
        PPE_Input_Layer1.handshake_msize                = PPE_MSIZE_1024;
        PPE_Input_Layer1.hw_index                       = 0;
        PPE_InitInputLayer(1, &PPE_Input_Layer1);
    }

    /*initial result layer*/
    PPE_Result_Layer.src_addr                       = (uint32_t)buffer->memory +
                                                      (target_rect.left + target_rect.top * buffer->width) * ppe_get_format_data_len(buffer->format);
    PPE_Result_Layer.width                          = source_rect.right - source_rect.left + 1;
    PPE_Result_Layer.height                         = source_rect.bottom - source_rect.top + 1;
    PPE_Result_Layer.format                         = buffer->format;
    PPE_Result_Layer.line_len                       = buffer->width;
    PPE_Result_Layer.AXSIZE                         = 2;// 32bit bandwidth
    PPE_Result_Layer.INCR                           = PPE_ARBURST_INCR;
    PPE_Result_Layer.AXCACHE                        = 1;
    PPE_Result_Layer.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_127;
    PPE_Result_Layer.PRIOR                          = 0;
    PPE_Result_Layer.byte_swap                      = PPE_NO_SWAP;
    PPE_Result_Layer.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
    PPE_Result_Layer.polarity                       = PPE_POLARITY_HIGH;
    PPE_Result_Layer.handshake_en                   = DISABLE;
    PPE_Result_Layer.handshake_msize                = PPE_MSIZE_1024;
    PPE_Result_Layer.hw_index                       = 0;
    PPE_InitResultLayer(&PPE_Result_Layer);

    /*initial PPE global*/
    PPE_Init_User.function                          = PPE_FUNCTION_ALPHA_BLEND;
    PPE_Init_User.blend_layer_num                   = 2;
    PPE_Init(&PPE_Init_User);
    PPE_Secure(ENABLE);  /*secure for all channel*/
    /*enable PPE*/
    PPE_Cmd(ENABLE);
    while (PPE->GLB_CTL & BIT0);
    return PPE_SUCCESS;
}

PPE_ERR PPE_Blend_Rect(ppe_buffer_t *image, ppe_buffer_t *buffer, ppe_translate_t *trans,
                       ppe_rect_t *rect, PPE_BLEND_MODE blend_mode)
{
    /*declaration of input/result layer initialization struct*/
    PPE_InputLayer_InitTypeDef      PPE_Input_Layer1;
    PPE_InputLayer_InitTypeDef      PPE_Input_Layer2;
    PPE_ResultLayer_InitTypeDef     PPE_Result_Layer;
    PPE_InitTypeDef                 PPE_Init_User;

    PPE_InputLayer_StructInit(&PPE_Input_Layer1);
    PPE_InputLayer_StructInit(&PPE_Input_Layer2);
    PPE_ResultLayer_StructInit(&PPE_Result_Layer);
    PPE_structInit(&PPE_Init_User);
    if (buffer == NULL)
    {
        return PPE_ERROR_NULL_TARGET;
    }
    if (image == NULL)
    {
        return PPE_ERROR_NULL_SOURCE;
    }
    if ((image->address % 4) || (buffer->address % 4))
    {
        return PPE_ERROR_ADDR_NOT_ALIGNED;
    }
    uint8_t format_len = ppe_get_format_data_len(image->format);
    if (format_len == 0)
    {
        return PPE_ERROR_UNKNOWN_FORMAT;
    }

    ppe_rect_t source_rect = {.left = trans->x, .top = trans->y, .right = trans->x + image->width - 1, .bottom = trans->y + image->height - 1};
    ppe_rect_t blend_area;
    if (!ppe_rect_intersect(&blend_area, &source_rect, rect))
    {
        return PPE_SUCCESS;
    }
    source_rect.left = blend_area.x1 - trans->x;
    source_rect.top = blend_area.y1 - trans->y;
    source_rect.right = blend_area.x2 - trans->x;
    source_rect.bottom = blend_area.y2 - trans->y;
    RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);

    /*initial input layer2*/
    PPE_Input_Layer2.start_x = 0;
    PPE_Input_Layer2.start_y = 0;
    PPE_Input_Layer2.src_addr                = (uint32_t)image->memory +
                                               (source_rect.left + image->stride * source_rect.top) * format_len;
    PPE_Input_Layer2.width                          = source_rect.right - source_rect.left + 1;
    PPE_Input_Layer2.height                         = source_rect.bottom - source_rect.top + 1;
    if (image->global_alpha_en)
    {
        PPE_Input_Layer2.const_ABGR8888_value           = image->global_alpha << 24;
    }
    else
    {
        PPE_Input_Layer2.const_ABGR8888_value           = 0xFFFFFFFF;
    }
    PPE_Input_Layer2.format                         = image->format;
    PPE_Input_Layer2.src                            = PPE_LAYER_SRC_FROM_DMA;
    if (image->color_key_en)
    {
        PPE_Input_Layer2.color_key_en                   = ENABLE;
        PPE_Input_Layer2.key_color_value                = image->color_key_value;
    }
    else
    {
        PPE_Input_Layer2.color_key_en                   = DISABLE;
        PPE_Input_Layer2.key_color_value                = 0;
    }
    PPE_Input_Layer2.line_len                       = image->stride;
    PPE_Input_Layer2.AXSIZE                         = 2;// 32bit bandwidth;
    PPE_Input_Layer2.INCR                           = PPE_ARBURST_INCR;
    PPE_Input_Layer2.AXCACHE                        = 1;
    PPE_Input_Layer2.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_127;
    PPE_Input_Layer2.PRIOR                          = 0;
    PPE_Input_Layer2.byte_swap                      = PPE_NO_SWAP;
    PPE_Input_Layer2.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
    PPE_Input_Layer2.polarity                       = PPE_POLARITY_HIGH;
    PPE_Input_Layer2.handshake_en                   = DISABLE;
    PPE_Input_Layer2.handshake_msize                = PPE_MSIZE_2;
    PPE_Input_Layer2.hw_index                       = 0;
    PPE_InitInputLayer(2, &PPE_Input_Layer2);


    if (blend_mode == PPE_BYPASS_MODE)
    {
        /*initial input layer1*/
        PPE_Input_Layer1.start_x                        = 0;
        PPE_Input_Layer1.start_y                        = 0;
        PPE_Input_Layer1.src_addr                       = (uint32_t)NULL;
        PPE_Input_Layer1.width                          = blend_area.right - blend_area.left + 1;
        PPE_Input_Layer1.height                         = blend_area.bottom - blend_area.top + 1;
        PPE_Input_Layer1.const_ABGR8888_value           = 0;
        PPE_Input_Layer1.format                         = buffer->format;
        PPE_Input_Layer1.src                            = PPE_LAYER_SRC_CONST;
        PPE_Input_Layer1.color_key_en                   = DISABLE;
        PPE_Input_Layer1.line_len                       = buffer->width;
        PPE_Input_Layer1.color_key_en                   = DISABLE;
        PPE_Input_Layer1.key_color_value                = 0;
        PPE_Input_Layer1.AXSIZE                         = 2;// 32bit bandwidth;
        PPE_Input_Layer1.INCR                           = PPE_ARBURST_INCR;
        PPE_Input_Layer1.AXCACHE                        = 1;
        PPE_Input_Layer1.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_127;
        PPE_Input_Layer1.PRIOR                          = 0;
        PPE_Input_Layer1.byte_swap                      = PPE_NO_SWAP;
        PPE_Input_Layer1.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
        PPE_Input_Layer1.polarity                       = PPE_POLARITY_HIGH;
        PPE_Input_Layer1.handshake_en                   = DISABLE;
        PPE_Input_Layer1.handshake_msize                = PPE_MSIZE_1024;
        PPE_Input_Layer1.hw_index                       = 0;
        PPE_InitInputLayer(1, &PPE_Input_Layer1);
    }
    /*initial input layer1*/
    else
    {
        PPE_Input_Layer1.src_addr                       = (uint32_t)buffer->memory +
                                                          (blend_area.left + blend_area.top * buffer->width) * ppe_get_format_data_len(buffer->format);
        PPE_Input_Layer1.start_x                        = 0;
        PPE_Input_Layer1.start_y                        = 0;
        PPE_Input_Layer1.width                          = blend_area.right - blend_area.left + 1;
        PPE_Input_Layer1.height                         = blend_area.bottom - blend_area.top + 1;
        PPE_Input_Layer1.const_ABGR8888_value           = 0XFFFFFFFF;
        PPE_Input_Layer1.format                         = buffer->format;
        PPE_Input_Layer1.src                            = PPE_LAYER_SRC_FROM_DMA;
        PPE_Input_Layer1.color_key_en                   = DISABLE;
        PPE_Input_Layer1.line_len                       = buffer->width;
        PPE_Input_Layer1.key_color_value                = buffer->color_key_value;
        PPE_Input_Layer1.AXSIZE                         = 2;// 32bit bandwidth;
        PPE_Input_Layer1.INCR                           = PPE_ARBURST_INCR;
        PPE_Input_Layer1.AXCACHE                        = 1;
        PPE_Input_Layer1.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_127;
        PPE_Input_Layer1.PRIOR                          = 0;
        PPE_Input_Layer1.byte_swap                      = PPE_NO_SWAP;
        PPE_Input_Layer1.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
        PPE_Input_Layer1.polarity                       = PPE_POLARITY_HIGH;
        PPE_Input_Layer1.handshake_en                   = DISABLE;
        PPE_Input_Layer1.handshake_msize                = PPE_MSIZE_2;
        PPE_Input_Layer1.hw_index                       = 0;
        PPE_InitInputLayer(1, &PPE_Input_Layer1);
    }

    /*initial result layer*/
    PPE_Result_Layer.src_addr                       = (uint32_t)buffer->memory +
                                                      (blend_area.left + blend_area.top * buffer->width) * ppe_get_format_data_len(buffer->format);
    PPE_Result_Layer.width                          = blend_area.right - blend_area.left + 1;
    PPE_Result_Layer.height                         = blend_area.bottom - blend_area.top + 1;
    PPE_Result_Layer.format                         = buffer->format;
    PPE_Result_Layer.line_len                       = buffer->width;
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

    /*initial PPE global*/
    PPE_Init_User.function                          =   PPE_FUNCTION_ALPHA_BLEND;
    PPE_Init_User.blend_layer_num                   = 2;
    PPE_Init(&PPE_Init_User);

    PPE_Secure(ENABLE);  /*secure for all channel*/

    /*enable PPE*/
    PPE_Cmd(ENABLE);
    while (PPE->GLB_CTL & BIT0);

    return PPE_SUCCESS;
}

PPE_ERR PPE_Blend_Multi(ppe_input_list_t list)
{
    /*declaration of input/result layer initialization struct*/
    if (list.output_layer == NULL)
    {
        return PPE_ERROR_NULL_TARGET;
    }

    RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);
    /*declaration of input/result layer initialization struct*/
    PPE_InputLayer_InitTypeDef      PPE_Input_Layer1;
    PPE_InputLayer_InitTypeDef      PPE_Input_Layer2;
    PPE_InputLayer_InitTypeDef      PPE_Input_Layer3;
    PPE_InputLayer_InitTypeDef      PPE_Input_Layer4;
    PPE_ResultLayer_InitTypeDef     PPE_Result_Layer;
    PPE_InitTypeDef                 PPE_Init_User;

    PPE_InputLayer_StructInit(&PPE_Input_Layer1);
    PPE_InputLayer_StructInit(&PPE_Input_Layer2);
    PPE_ResultLayer_StructInit(&PPE_Result_Layer);
    PPE_structInit(&PPE_Init_User);

    PPE_Init_User.blend_layer_num = 0;

    if (list.input_layer1 != NULL)
    {
        if ((list.input_layer1->trans.x >= -(list.input_layer1->buffer.width - 1)) && \
            (list.input_layer1->trans.x < (list.output_layer->buffer.width - 1)) && \
            (list.input_layer1->trans.y >= -(list.input_layer1->buffer.height - 1)) && \
            (list.input_layer1->trans.y < (list.output_layer->buffer.height - 1)))
        {
            ppe_rect_t source_rect;
            if (list.input_layer1->rect)
            {
                source_rect.left = list.input_layer1->rect->left;
                source_rect.right = list.input_layer1->rect->right;
                source_rect.top = list.input_layer1->rect->top;
                source_rect.bottom = list.input_layer1->rect->bottom;
            }
            else
            {
                source_rect.left = 0;
                source_rect.right = list.input_layer1->buffer.width - 1;
                source_rect.top = 0;
                source_rect.bottom = list.input_layer1->buffer.height - 1;
            }

            PPE_Input_Layer1.start_x = list.input_layer1->trans.x;
            PPE_Input_Layer1.start_y = list.input_layer1->trans.y;

            if (list.input_layer1->trans.x < 0)
            {
                source_rect.left = -list.input_layer1->trans.x;
                PPE_Input_Layer1.start_x = 0;
            }
            if ((list.input_layer1->trans.x + list.input_layer1->buffer.width) >
                list.output_layer->buffer.width)
            {
                source_rect.right = list.output_layer->buffer.width - list.input_layer1->trans.x - 1;
            }
            if (list.input_layer1->trans.y < 0)
            {
                source_rect.top = -list.input_layer1->trans.y;
                PPE_Input_Layer1.start_y = 0;
            }
            if ((list.input_layer1->trans.y + list.input_layer1->buffer.height) >
                list.output_layer->buffer.height)
            {
                source_rect.bottom = list.output_layer->buffer.height - list.input_layer1->trans.y - 1;
            }
            uint8_t format_len = ppe_get_format_data_len(list.input_layer1->buffer.format);
            if (format_len == 0)
            {
                return PPE_ERROR_UNKNOWN_FORMAT;
            }

            PPE_Input_Layer1.src_addr                       = (uint32_t)list.input_layer1->buffer.memory +
                                                              (source_rect.left + list.input_layer1->buffer.width * source_rect.top) * format_len;
            PPE_Input_Layer1.width                          = source_rect.right - source_rect.left + 1;
            PPE_Input_Layer1.height                         = source_rect.bottom - source_rect.top + 1;
            if (list.input_layer1->buffer.global_alpha_en)
            {
                PPE_Input_Layer1.const_ABGR8888_value           = list.input_layer1->buffer.global_alpha << 24;
            }
            else
            {
                PPE_Input_Layer1.const_ABGR8888_value           = 0xFFFFFFFF;
            }
            PPE_Input_Layer1.format                         = list.input_layer1->buffer.format;
            PPE_Input_Layer1.src                            = PPE_LAYER_SRC_FROM_DMA;
            if (list.input_layer1->buffer.color_key_en)
            {
                PPE_Input_Layer1.color_key_en                   = ENABLE;
                PPE_Input_Layer1.key_color_value                = list.input_layer1->buffer.color_key_value;
            }
            else
            {
                PPE_Input_Layer1.color_key_en                   = DISABLE;
                PPE_Input_Layer1.key_color_value                = 0;
            }
            PPE_Input_Layer1.line_len                       = list.input_layer1->buffer.width;
            PPE_Input_Layer1.AXSIZE                         = 2;// 32bit bandwidth;
            PPE_Input_Layer1.INCR                           = PPE_ARBURST_INCR;
            PPE_Input_Layer1.AXCACHE                        = 1;
            PPE_Input_Layer1.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_127;
            PPE_Input_Layer1.PRIOR                          = 0;
            PPE_Input_Layer1.byte_swap                      = PPE_NO_SWAP;
            PPE_Input_Layer1.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
            PPE_Input_Layer1.polarity                       = PPE_POLARITY_HIGH;
            PPE_Input_Layer1.handshake_en                   = DISABLE;
            PPE_Input_Layer1.handshake_msize                = PPE_MSIZE_1024;
            PPE_Input_Layer1.hw_index                       = 0;

            PPE_Init_User.blend_layer_num++;
            PPE_InitInputLayer(1, &PPE_Input_Layer1);
        }
    }

    if (list.input_layer2 != NULL)
    {
        if ((list.input_layer2->trans.x >= -(list.input_layer2->buffer.width - 1)) && \
            (list.input_layer2->trans.x < (list.output_layer->buffer.width - 1)) && \
            (list.input_layer2->trans.y >= -(list.input_layer2->buffer.height - 1)) && \
            (list.input_layer2->trans.y < (list.output_layer->buffer.height - 1)))
        {
            ppe_rect_t source_rect;
            if (list.input_layer2->rect)
            {
                source_rect.left = list.input_layer2->rect->left;
                source_rect.right = list.input_layer2->rect->right;
                source_rect.top = list.input_layer2->rect->top;
                source_rect.bottom = list.input_layer2->rect->bottom;
            }
            else
            {
                source_rect.left = 0;
                source_rect.right = list.input_layer2->buffer.width - 1;
                source_rect.top = 0;
                source_rect.bottom = list.input_layer2->buffer.height - 1;
            }

            PPE_Input_Layer2.start_x = list.input_layer2->trans.x;
            PPE_Input_Layer2.start_y = list.input_layer2->trans.y;

            if (list.input_layer2->trans.x < 0)
            {
                source_rect.left = -list.input_layer2->trans.x;
                PPE_Input_Layer2.start_x = 0;
            }
            if ((list.input_layer2->trans.x + list.input_layer2->buffer.width) >
                list.output_layer->buffer.width)
            {
                source_rect.right = list.output_layer->buffer.width - list.input_layer2->trans.x - 1;
            }
            if (list.input_layer2->trans.y < 0)
            {
                source_rect.top = -list.input_layer2->trans.y;
                PPE_Input_Layer2.start_y = 0;
            }
            if ((list.input_layer2->trans.y + list.input_layer2->buffer.height) >
                list.output_layer->buffer.height)
            {
                source_rect.bottom = list.output_layer->buffer.height - list.input_layer2->trans.y - 1;
            }
            uint8_t format_len = ppe_get_format_data_len(list.input_layer2->buffer.format);
            if (format_len == 0)
            {
                return PPE_ERROR_UNKNOWN_FORMAT;
            }

            PPE_Input_Layer2.src_addr                       = (uint32_t)list.input_layer2->buffer.memory +
                                                              (source_rect.left + list.input_layer2->buffer.width * source_rect.top) * format_len;
            PPE_Input_Layer2.width                          = source_rect.right - source_rect.left + 1;
            PPE_Input_Layer2.height                         = source_rect.bottom - source_rect.top + 1;
            if (list.input_layer2->buffer.global_alpha_en)
            {
                PPE_Input_Layer2.const_ABGR8888_value           = list.input_layer2->buffer.global_alpha << 24;
            }
            else
            {
                PPE_Input_Layer2.const_ABGR8888_value           = 0xFFFFFFFF;
            }
            PPE_Input_Layer2.format                         = list.input_layer2->buffer.format;
            PPE_Input_Layer2.src                            = PPE_LAYER_SRC_FROM_DMA;
            if (list.input_layer2->buffer.color_key_en)
            {
                PPE_Input_Layer2.color_key_en                   = ENABLE;
                PPE_Input_Layer2.key_color_value                = list.input_layer2->buffer.color_key_value;
            }
            else
            {
                PPE_Input_Layer2.color_key_en                   = DISABLE;
                PPE_Input_Layer2.key_color_value                = 0;
            }
            PPE_Input_Layer2.line_len                       = list.input_layer2->buffer.width;
            PPE_Input_Layer2.AXSIZE                         = 2;// 32bit bandwidth;
            PPE_Input_Layer2.INCR                           = PPE_ARBURST_INCR;
            PPE_Input_Layer2.AXCACHE                        = 1;
            PPE_Input_Layer2.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_127;
            PPE_Input_Layer2.PRIOR                          = 0;
            PPE_Input_Layer2.byte_swap                      = PPE_NO_SWAP;
            PPE_Input_Layer2.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
            PPE_Input_Layer2.polarity                       = PPE_POLARITY_HIGH;
            PPE_Input_Layer2.handshake_en                   = DISABLE;
            PPE_Input_Layer2.handshake_msize                = PPE_MSIZE_1024;
            PPE_Input_Layer2.hw_index                       = 0;

            PPE_Init_User.blend_layer_num++;
            PPE_InitInputLayer(2, &PPE_Input_Layer2);
        }
    }

    if (list.input_layer3 != NULL)
    {
        if ((list.input_layer3->trans.x >= -(list.input_layer3->buffer.width - 1)) && \
            (list.input_layer3->trans.x < (list.output_layer->buffer.width - 1)) && \
            (list.input_layer3->trans.y >= -(list.input_layer3->buffer.height - 1)) && \
            (list.input_layer3->trans.y < (list.output_layer->buffer.height - 1)))
        {
            ppe_rect_t source_rect;
            if (list.input_layer3->rect)
            {
                source_rect.left = list.input_layer3->rect->left;
                source_rect.right = list.input_layer3->rect->right;
                source_rect.top = list.input_layer3->rect->top;
                source_rect.bottom = list.input_layer3->rect->bottom;
            }
            else
            {
                source_rect.left = 0;
                source_rect.right = list.input_layer3->buffer.width - 1;
                source_rect.top = 0;
                source_rect.bottom = list.input_layer3->buffer.height - 1;
            }

            PPE_Input_Layer3.start_x = list.input_layer3->trans.x;
            PPE_Input_Layer3.start_y = list.input_layer3->trans.y;

            if (list.input_layer3->trans.x < 0)
            {
                source_rect.left = -list.input_layer3->trans.x;
                PPE_Input_Layer3.start_x = 0;
            }
            if ((list.input_layer3->trans.x + list.input_layer3->buffer.width) >
                list.output_layer->buffer.width)
            {
                source_rect.right = list.output_layer->buffer.width - list.input_layer3->trans.x - 1;
            }
            if (list.input_layer3->trans.y < 0)
            {
                source_rect.top = -list.input_layer3->trans.y;
                PPE_Input_Layer3.start_y = 0;
            }
            if ((list.input_layer3->trans.y + list.input_layer3->buffer.height) >
                list.output_layer->buffer.height)
            {
                source_rect.bottom = list.output_layer->buffer.height - list.input_layer3->trans.y - 1;
            }
            uint8_t format_len = ppe_get_format_data_len(list.input_layer3->buffer.format);
            if (format_len == 0)
            {
                return PPE_ERROR_UNKNOWN_FORMAT;
            }

            PPE_Input_Layer3.src_addr                       = (uint32_t)list.input_layer3->buffer.memory +
                                                              (source_rect.left + list.input_layer3->buffer.width * source_rect.top) * format_len;
            PPE_Input_Layer3.width                          = source_rect.right - source_rect.left + 1;
            PPE_Input_Layer3.height                         = source_rect.bottom - source_rect.top + 1;
            if (list.input_layer3->buffer.global_alpha_en)
            {
                PPE_Input_Layer3.const_ABGR8888_value           = list.input_layer3->buffer.global_alpha << 24;
            }
            else
            {
                PPE_Input_Layer3.const_ABGR8888_value           = 0xFFFFFFFF;
            }
            PPE_Input_Layer3.format                         = list.input_layer3->buffer.format;
            PPE_Input_Layer3.src                            = PPE_LAYER_SRC_FROM_DMA;
            if (list.input_layer3->buffer.color_key_en)
            {
                PPE_Input_Layer3.color_key_en                   = ENABLE;
                PPE_Input_Layer3.key_color_value                = list.input_layer3->buffer.color_key_value;
            }
            else
            {
                PPE_Input_Layer3.color_key_en                   = DISABLE;
                PPE_Input_Layer3.key_color_value                = 0;
            }
            PPE_Input_Layer3.line_len                       = list.input_layer3->buffer.width;
            PPE_Input_Layer3.AXSIZE                         = 2;// 32bit bandwidth;
            PPE_Input_Layer3.INCR                           = PPE_ARBURST_INCR;
            PPE_Input_Layer3.AXCACHE                        = 1;
            PPE_Input_Layer3.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_127;
            PPE_Input_Layer3.PRIOR                          = 0;
            PPE_Input_Layer3.byte_swap                      = PPE_NO_SWAP;
            PPE_Input_Layer3.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
            PPE_Input_Layer3.polarity                       = PPE_POLARITY_HIGH;
            PPE_Input_Layer3.handshake_en                   = DISABLE;
            PPE_Input_Layer3.handshake_msize                = PPE_MSIZE_1024;
            PPE_Input_Layer3.hw_index                       = 0;

            PPE_Init_User.blend_layer_num++;
            PPE_InitInputLayer(3, &PPE_Input_Layer3);
        }
    }

    if (list.input_layer4 != NULL)
    {
        if ((list.input_layer4->trans.x >= -(list.input_layer4->buffer.width - 1)) && \
            (list.input_layer4->trans.x < (list.output_layer->buffer.width - 1)) && \
            (list.input_layer4->trans.y >= -(list.input_layer4->buffer.height - 1)) && \
            (list.input_layer4->trans.y < (list.output_layer->buffer.height - 1)))
        {
            ppe_rect_t source_rect;
            if (list.input_layer4->rect)
            {
                source_rect.left = list.input_layer4->rect->left;
                source_rect.right = list.input_layer4->rect->right;
                source_rect.top = list.input_layer4->rect->top;
                source_rect.bottom = list.input_layer4->rect->bottom;
            }
            else
            {
                source_rect.left = 0;
                source_rect.right = list.input_layer4->buffer.width - 1;
                source_rect.top = 0;
                source_rect.bottom = list.input_layer4->buffer.height - 1;
            }

            PPE_Input_Layer4.start_x = list.input_layer4->trans.x;
            PPE_Input_Layer4.start_y = list.input_layer4->trans.y;

            if (list.input_layer4->trans.x < 0)
            {
                source_rect.left = -list.input_layer4->trans.x;
                PPE_Input_Layer4.start_x = 0;
            }
            if ((list.input_layer4->trans.x + list.input_layer4->buffer.width) >
                list.output_layer->buffer.width)
            {
                source_rect.right = list.output_layer->buffer.width - list.input_layer4->trans.x - 1;
            }
            if (list.input_layer4->trans.y < 0)
            {
                source_rect.top = -list.input_layer4->trans.y;
                PPE_Input_Layer4.start_y = 0;
            }
            if ((list.input_layer4->trans.y + list.input_layer4->buffer.height) >
                list.output_layer->buffer.height)
            {
                source_rect.bottom = list.output_layer->buffer.height - list.input_layer4->trans.y - 1;
            }
            uint8_t format_len = ppe_get_format_data_len(list.input_layer4->buffer.format);
            if (format_len == 0)
            {
                return PPE_ERROR_UNKNOWN_FORMAT;
            }

            PPE_Input_Layer4.src_addr                       = (uint32_t)list.input_layer4->buffer.memory +
                                                              (source_rect.left + list.input_layer4->buffer.width * source_rect.top) * format_len;
            PPE_Input_Layer4.width                          = source_rect.right - source_rect.left + 1;
            PPE_Input_Layer4.height                         = source_rect.bottom - source_rect.top + 1;
            if (list.input_layer4->buffer.global_alpha_en)
            {
                PPE_Input_Layer4.const_ABGR8888_value           = list.input_layer4->buffer.global_alpha << 24;
            }
            else
            {
                PPE_Input_Layer4.const_ABGR8888_value           = 0xFFFFFFFF;
            }
            PPE_Input_Layer4.format                         = list.input_layer4->buffer.format;
            PPE_Input_Layer4.src                            = PPE_LAYER_SRC_FROM_DMA;
            if (list.input_layer4->buffer.color_key_en)
            {
                PPE_Input_Layer4.color_key_en                   = ENABLE;
                PPE_Input_Layer4.key_color_value                = list.input_layer4->buffer.color_key_value;
            }
            else
            {
                PPE_Input_Layer4.color_key_en                   = DISABLE;
                PPE_Input_Layer4.key_color_value                = 0;
            }
            PPE_Input_Layer4.line_len                       = list.input_layer4->buffer.width;
            PPE_Input_Layer4.AXSIZE                         = 2;// 32bit bandwidth;
            PPE_Input_Layer4.INCR                           = PPE_ARBURST_INCR;
            PPE_Input_Layer4.AXCACHE                        = 1;
            PPE_Input_Layer4.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_127;
            PPE_Input_Layer4.PRIOR                          = 0;
            PPE_Input_Layer4.byte_swap                      = PPE_NO_SWAP;
            PPE_Input_Layer4.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
            PPE_Input_Layer4.polarity                       = PPE_POLARITY_HIGH;
            PPE_Input_Layer4.handshake_en                   = DISABLE;
            PPE_Input_Layer4.handshake_msize                = PPE_MSIZE_1024;
            PPE_Input_Layer4.hw_index                       = 0;

            PPE_Init_User.blend_layer_num++;
            PPE_InitInputLayer(4, &PPE_Input_Layer4);
        }
    }

    /*initial result layer*/
    PPE_Result_Layer.src_addr                       = (uint32_t)list.output_layer->buffer.memory;
    PPE_Result_Layer.width                          = list.output_layer->buffer.width;
    PPE_Result_Layer.height                         = list.output_layer->buffer.height;
    PPE_Result_Layer.format                         = list.output_layer->buffer.format;
    PPE_Result_Layer.line_len                       = list.output_layer->buffer.width;
    PPE_Result_Layer.AXSIZE                         = 2;// 32bit bandwidth
    PPE_Result_Layer.INCR                           = PPE_ARBURST_INCR;
    PPE_Result_Layer.AXCACHE                        = 1;
    PPE_Result_Layer.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_127;
    PPE_Result_Layer.PRIOR                          = 0;
    PPE_Result_Layer.byte_swap                      = PPE_NO_SWAP;
    PPE_Result_Layer.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
    PPE_Result_Layer.polarity                       = PPE_POLARITY_HIGH;
    PPE_Result_Layer.handshake_en                   = DISABLE;
    PPE_Result_Layer.handshake_msize                = PPE_MSIZE_1024;
    PPE_Result_Layer.hw_index                       = 0;
    PPE_InitResultLayer(&PPE_Result_Layer);

    /*initial PPE global*/
    PPE_Init_User.function                          =   PPE_FUNCTION_ALPHA_BLEND;
    PPE_Init(&PPE_Init_User);
    PPE_Secure(ENABLE);  /*secure for all channel*/
    /*enable PPE*/
    PPE_Cmd(ENABLE);
    while (PPE->GLB_CTL & BIT0);
    return PPE_SUCCESS;
}

PPE_ERR PPE_Mask(ppe_buffer_t *src, ppe_buffer_t *target, ppe_rect_t *rect, uint32_t color)
{
    if (target == NULL)
    {
        return PPE_ERROR_NULL_TARGET;
    }
    if (src == NULL)
    {
        return PPE_ERROR_NULL_SOURCE;
    }
    uint8_t format_len = ppe_get_format_data_len(src->format);
    if (format_len == 0)
    {
        return PPE_ERROR_UNKNOWN_FORMAT;
    }

    RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);
    /*declaration of input/result layer initialization struct*/
    PPE_InputLayer_InitTypeDef      PPE_Input_Layer1;
    PPE_InputLayer_InitTypeDef      PPE_Input_Layer2;
    PPE_ResultLayer_InitTypeDef     PPE_Result_Layer;
    PPE_InitTypeDef                 PPE_Init_User;

    PPE_InputLayer_StructInit(&PPE_Input_Layer1);
    PPE_InputLayer_StructInit(&PPE_Input_Layer2);
    PPE_ResultLayer_StructInit(&PPE_Result_Layer);
    PPE_structInit(&PPE_Init_User);

    /*initial input layer2*/
    PPE_Input_Layer2.start_x = 0;
    PPE_Input_Layer2.start_y = 0;
    PPE_Input_Layer2.src_addr                       = (uint32_t)NULL;
    PPE_Input_Layer2.width                          = rect->right - rect->left + 1;
    PPE_Input_Layer2.height                         = rect->bottom - rect->top + 1;
    PPE_Input_Layer2.const_ABGR8888_value           = color;
    PPE_Input_Layer2.format                         = PPE_ABGR8888;
    PPE_Input_Layer2.src                            = PPE_LAYER_SRC_CONST;
    PPE_Input_Layer2.line_len                       = src->stride;
    PPE_Input_Layer2.AXSIZE                         = 2;// 32bit bandwidth;
    PPE_Input_Layer2.INCR                           = PPE_ARBURST_INCR;
    PPE_Input_Layer2.AXCACHE                        = 1;
    PPE_Input_Layer2.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_127;
    PPE_Input_Layer2.PRIOR                          = 0;
    PPE_Input_Layer2.byte_swap                      = PPE_NO_SWAP;
    PPE_Input_Layer2.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
    PPE_Input_Layer2.polarity                       = PPE_POLARITY_HIGH;
    PPE_Input_Layer2.handshake_en                   = DISABLE;
    PPE_Input_Layer2.handshake_msize                = PPE_MSIZE_1024;
    PPE_Input_Layer2.hw_index                       = 0;
    PPE_InitInputLayer(2, &PPE_Input_Layer2);

    /*initial input layer1*/
    PPE_Input_Layer1.start_x                        = 0;
    PPE_Input_Layer1.start_y                        = 0;
    PPE_Input_Layer1.src_addr                       = (uint32_t)src->memory +
                                                      (rect->top * src->width + rect->left) * ppe_get_format_data_len(src->format);
    PPE_Input_Layer1.width                          = rect->right - rect->left + 1;
    PPE_Input_Layer1.height                         = rect->bottom - rect->top + 1;
    PPE_Input_Layer1.const_ABGR8888_value           = 0XFFAABBCC;
    PPE_Input_Layer1.format                         = src->format;
    PPE_Input_Layer1.src                            = PPE_LAYER_SRC_FROM_DMA;
    if (src->color_key_en)
    {
        PPE_Input_Layer1.color_key_en                   = ENABLE;
        PPE_Input_Layer1.key_color_value                = src->color_key_value;
    }
    else
    {
        PPE_Input_Layer1.color_key_en                   = DISABLE;
        PPE_Input_Layer1.key_color_value                = 0;
    }
    PPE_Input_Layer1.line_len                       = src->width;
    PPE_Input_Layer1.AXSIZE                         = 2;// 32bit bandwidth;
    PPE_Input_Layer1.INCR                           = PPE_ARBURST_INCR;
    PPE_Input_Layer1.AXCACHE                        = 1;
    PPE_Input_Layer1.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_127;
    PPE_Input_Layer1.PRIOR                          = 0;
    PPE_Input_Layer1.byte_swap                      = PPE_NO_SWAP;
    PPE_Input_Layer1.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
    PPE_Input_Layer1.polarity                       = PPE_POLARITY_HIGH;
    PPE_Input_Layer1.handshake_en                   = DISABLE;
    PPE_Input_Layer1.handshake_msize                = PPE_MSIZE_1024;
    PPE_Input_Layer1.hw_index                       = 0;
    PPE_InitInputLayer(1, &PPE_Input_Layer1);

    /*initial result layer*/
    PPE_Result_Layer.src_addr                       = (uint32_t)target->memory;
    PPE_Result_Layer.width                          = PPE_Input_Layer1.width;
    PPE_Result_Layer.height                         = PPE_Input_Layer1.height;
    PPE_Result_Layer.format                         = target->format;
    PPE_Result_Layer.line_len                       = PPE_Input_Layer1.width;
    PPE_Result_Layer.AXSIZE                         = 2;// 32bit bandwidth
    PPE_Result_Layer.INCR                           = PPE_ARBURST_INCR;
    PPE_Result_Layer.AXCACHE                        = 1;
    PPE_Result_Layer.MAX_AXLEN_LOG                  = PPE_MAX_AXLEN_127;
    PPE_Result_Layer.PRIOR                          = 0;
    PPE_Result_Layer.byte_swap                      = PPE_NO_SWAP;
    PPE_Result_Layer.handshake_mode                 = PPE_DMA_SW_HANDSHAKE;
    PPE_Result_Layer.polarity                       = PPE_POLARITY_HIGH;
    PPE_Result_Layer.handshake_en                   = DISABLE;
    PPE_Result_Layer.handshake_msize                = PPE_MSIZE_1024;
    PPE_Result_Layer.hw_index                       = 0;
    PPE_InitResultLayer(&PPE_Result_Layer);

    /*initial PPE global*/
    PPE_Init_User.function                          =   PPE_FUNCTION_ALPHA_BLEND;
    PPE_Init_User.blend_layer_num                   = 2;
    PPE_Init(&PPE_Init_User);
    PPE_Secure(ENABLE);  /*secure for all channel*/
    /*enable PPE*/
    PPE_Cmd(ENABLE);
    while (PPE->GLB_CTL & BIT0);
    return PPE_SUCCESS;
}

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/
