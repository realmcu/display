#ifndef RTL_PPE_INT_H
#define RTL_PPE_INT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rtl_ppe.h"
#include "rtl_ppe_def.h"

#define PPE_FUNCTION_ALPHA_BLEND                        ((uint32_t)0x0)
#define PPE_FUNCTION_SCALE                              ((uint32_t)0x1)
#define IS_PPE_FUNCTION(func)                           (((func) == PPE_FUNCTION_ALPHA_BLEND) || ((func) == PPE_FUNCTION_SCALE))

typedef struct
{
    uint32_t LAYER_ADDR;
    uint32_t LAYER_POS;
    uint32_t LAYER_WINSIZE;
    uint32_t LAYER_CONST_PIX;
} PPE_LLI_LAYER;

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

#ifdef __cplusplus
}
#endif

#endif /* RTL_PPE_INT_H */