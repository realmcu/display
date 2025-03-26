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
    PPE_ABGR8666 = 0x32,    /*!< ABGR8666: A(bit 31:24) B(bit 23:18) G(bit 15:10) R(bit 8:2) */
    PPE_ARGB8666,           /*!< ARGB8666: A(bit 31:24) R(bit 23:18) G(bit 15:10) B(bit 8:2) */
    PPE_XBGR8666,           /*!< XBGR8666: X(bit 31:24) B(bit 23:18) G(bit 15:10) R(bit 8:2) */
    PPE_XRGB8666,           /*!< XRGB8666: X(bit 31:24) R(bit 23:18) G(bit 15:10) B(bit 8:2) */
    PPE_BGRA6668,           /*!< BGRA6668: B(bit 31:26) G(bit 23:18) R(bit 15:10) A(bit 7:0) */
    PPE_RGBA6668,           /*!< RGBA6668: R(bit 31:26) G(bit 23:18) B(bit 15:10) A(bit 7:0) */
    PPE_BGRX6668,           /*!< BGRX6668: B(bit 31:26) G(bit 23:18) R(bit 15:10) X(bit 7:0) */
    PPE_RGBX6668,           /*!< RGBX6668: R(bit 31:26) G(bit 23:18) B(bit 15:10) X(bit 7:0) */
    PPE_FORMAT_NOT_SUPPORT = 0xFF, /*!< Other color formats are not supported */
} PPE_PIXEL_FORMAT;
/** End of PPE_PIXEL_FORMAT
  * \}
  */

/** \defgroup PPE_ERR PPE Error Code
  * \{
  * \ingroup  PPE_Exported_Constants
  */
typedef enum
{
    PPE_SUCCESS,                    /*!< No error occurred. */
    PPE_SUCCESS_NOT_CHANGE,         /*!< No error occurred and PPE takes no operation. */
    PPE_ERROR_ADDR_NOT_ALIGNED,     /*!< Input buffer address is not aligned to 4 bytes. */
    PPE_ERROR_INVALID_PARAM,        /*!< Scissor or blending area is invalid. */
    PPE_ERROR_UNKNOWN_FORMAT,       /*!< Color format not supported. */
    PPE_ERROR_NULL_SOURCE,          /*!< Blending source is invalid. */
    PPE_ERROR_NULL_TARGET,          /*!< Blending target is invalid. */
    PPE_ERROR_OUT_OF_RANGE,         /*!< Image is out of blending area. */
} PPE_ERR;
/** End of PPE_ERR
  * \}
  */

/** \defgroup PPE_BLEND_MODE PPE Blend Mode
  * \{
  * \ingroup  PPE_Exported_Constants
  */
typedef enum
{
    PPE_BYPASS_MODE,    /*!< Bypass mode: D = S */
    PPE_SRC_OVER_MODE,  /*!< Source over mode: D = (1 - a) * D + S * a */
} PPE_BLEND_MODE;
/** End of PPE_BLEND_MODE
  * \}
  */

/** \defgroup PPE_INTERRUPT PPE Interrupt
  * \{
  * \ingroup  PPE_Exported_Constants
  */
#define PPE_ALL_OVER_INT             (BIT0)    /*!< Raised after all frames are completed. */
#define PPE_FR_OVER_INT              (BIT1)    /*!< Raised after every frame is completed. */
#define PPE_LOAD_OVER_INT            (BIT2)    /*!< Raised after layers registers load to internal DMA. */
#define PPE_LINE_OVER_INT            (BIT3)    /*!< Raised after the transaction of specified line has been completed. */
#define PPE_SUSP_INAC_INT            (BIT4)    /*!< Raised after the master port is inactive and PPE is suspended. */

#define IS_PPE_INT(interrupt)        (((interrupt) == PPE_ALL_OVER_INT) || \
                                      ((interrupt) == PPE_FR_OVER_INT) || \
                                      ((interrupt) == PPE_LOAD_OVER_INT) || \
                                      ((interrupt) == PPE_LINE_OVER_INT) || \
                                      ((interrupt) == PPE_SUSP_INAC_INT))
/** End of PPE_INTERRUPT
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

/** \defgroup PPE_RECT PPE Rectrangle
  * \{
  * \ingroup  PPE_Exported_Types
  */
typedef struct
{
    union
    {
        int32_t left;   /*!< Coordinate of left boarder, same with x1. */
        int32_t x1;     /*!< Coordinate of left boarder, same with left. */
    }; /*!< Union of left and x1. */
    union
    {
        int32_t top;   /*!< Coordinate of top boarder, same with y1. */
        int32_t y1;    /*!< Coordinate of top boarder, same with top. */
    }; /*!< Union of top and y1. */
    union
    {
        int32_t right; /*!< Coordinate of right boarder, same with x2. */
        int32_t x2;    /*!< Coordinate of right boarder, same with right. */
    }; /*!< Union of right and x2. */
    union
    {
        int32_t bottom; /*!< Coordinate of bottom boarder, same with y2. */
        int32_t y2;    /*!< Coordinate of bottom boarder, same with bottom. */
    }; /*!< Union of bottom and y2. */
} ppe_rect_t;
/** End of PPE_RECT
  * \}
  */

/** \defgroup PPE_TRANS PPE Position Translation
  * \{
  * \ingroup  PPE_Exported_Types
  */
typedef struct
{
    int32_t x;  /*!< Horizontal translation on blend target. */
    int32_t y;  /*!< Vertical translation on blend target. */
} ppe_translate_t;
/** End of PPE_TRANS
  * \}
  */

/** \defgroup PPE_BUFFER PPE Buffer Structure
  * \{
  * \ingroup  PPE_Exported_Types
  */
typedef struct
{
    uint32_t *memory;  /*!< Pointer to memory of blend buffer. */
    uint32_t address;  /*!< Unsigned integer of buffer address, has the same value with member 'memory'. */
    PPE_PIXEL_FORMAT format; /*!< Pixel format of buffer, can be a value of @ref PPE_PIXEL_FORMAT . */
    bool global_alpha_en;  /*!< Control additional opacity placed on image. */
    uint8_t global_alpha;  /*!< Value of additional opacity. */
    bool color_key_en; /*!< Control color key of image. */
    uint32_t color_key_value; /*!< Pixel value that equals to color key value will be filtered. */
    uint16_t width;    /*!< Width of image. */
    uint16_t height;   /*!< Height of image. */
    uint16_t stride;   /*!< Stride of image between 2 contiguous lines. */
} ppe_buffer_t;
/** End of PPE_BUFFER
  * \}
  */

/** \defgroup PPE_LAYER PPE Layer Configuration
  * \{
  * \ingroup  PPE_Exported_Types
  */
typedef struct
{
    ppe_buffer_t buffer;   /*!< Image buffer to be draw onto layer, described in @ref PPE_BUFFER. */
    ppe_rect_t *rect;      /*!< Draw area constraint, described in @ref PPE_RECT. */
    ppe_translate_t trans; /*!< Relative position, described in @ref PPE_TRANS . */
} ppe_layer_t;
/** End of PPE_LAYER
  * \}
  */

/** \defgroup PPE_LAYER_LIST PPE Layer Configuration List
  * \{
  * \ingroup  PPE_Exported_Types
  */
typedef struct
{
    ppe_layer_t *input_layer1;  /*!< Input layer 1 configuration. */
    ppe_layer_t *input_layer2;  /*!< Input layer 2 configuration. */
    ppe_layer_t *input_layer3;  /*!< Input layer 3 configuration. */
    ppe_layer_t *input_layer4;  /*!< Input layer 4 configuration. */
    ppe_layer_t *output_layer;  /*!< Output layer configuration. */
} ppe_input_list_t;
/** End of PPE_LAYER_LIST
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
 * \brief  Enable PPE.
 * \param[in] state: New state of the PPE peripheral.
 *            - ENABLE: Enable PPE function.
 *            - DISABLE: Disable PPE function.
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
 * \brief  Suspend PPE.
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
 * \brief  Resume PPE.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        PPE_Suspend();
        // some ohter operation
        PPE_Resume();
    }
 * \endcode
*/
void PPE_Resume(void);

/**
 * \brief  Abort current operation, must invoke after all channels are inactive.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        PPE_Suspend();
        while(PPE_ChannelInactiveCheck());
        PPE_Abort();
    }
 * \endcode
*/
void PPE_Abort(void);

/**
 * \brief  Check PPE channel status.
 * \return Channel state.
 * \retval SET    Channels are not all inactive.
 * \retval RESET  Channels are all inactive.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        ITStatus res = PPE_ChannelInactiveCheck();
    }
 * \endcode
*/
ITStatus PPE_ChannelInactiveCheck(void);

/**
 * \brief  Check interrupt status.
 * \param[in] PPE_INT: Specified PPE interrupt from \ref PPE_INTERRUPT
 * \return Interrupt status.
 * \retval SET    Interrupt has been triggered.
 * \retval RESET  Interrupt has not been triggered yet.
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
 * \brief  Check interrupt status before mask.
 * \param[in] PPE_INT: Specified PPE interrupt from \ref PPE_INTERRUPT
 * \return Interrupt status.
 * \retval SET    Interrupt has been triggered.
 * \retval RESET  Interrupt has not been triggered yet.
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
 * \brief  Check interrupt pending bit.
 * \param[in] PPE_INT: Specified PPE interrupt from \ref PPE_INTERRUPT
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
 * \brief  Mask specified interrupt.
 * \param[in] PPE_INT: Specified PPE interrupt from \ref PPE_INTERRUPT
 * \param[in] NewState: New state of the PPE interrupt mask
 *            - ENABLE: Mask specified interrupt.
 *            - DISABLE: Unmask specified interrupt.
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
 * \brief  Scale the input image and save the result in output image buffer.
 * \param[in] image: Pointer to input image buffer.
 * \param[in] buffer: Pointer to output image buffer.
 * \param[in] x_ratio: Scale ration on horizontal direction.
 * \param[in] y_ratio: Scale ration on vertical direction.
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
 * \brief  Scale a part of input image and save in output image buffer.
 * \param[in] image: Pointer to input image buffer.
 * \param[in] buffer: Pointer to output image buffer.
 * \param[in] x_ratio: Scale ration on horizontal direction.
 * \param[in] y_ratio: Scale ration on vertical direction.
 * \param[in] rect: Boundary inside the input image.
 * \return Operation result
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
 * \brief  Clear the image buffer with certain color in ABGR8888 format.
 * \param[in] buffer: Target buffer to be cleared.
 * \param[in] color: Specified color in ABGR8888 format \ref PPE_ABGR8888
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
 * \brief  Clear a part of buffer with certain color in ABGR8888 format.
 * \param[in] buffer: Target buffer to be cleared.
 * \param[in] rect: Area of buffer to be cleared.
 * \param[in] color: Specified color in ABGR8888 format \ref PPE_ABGR8888
 * \return Operation result.
 * \retval PPE_SUCCESS  Operation success.
 * \retval Others       Operation failure, cause refers to @ref PPE_ERR .
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
        PPE_ERR err = PPE_Clear_Rect(&buffer, &rect, 0xFFFF00FF);
    }
 * \endcode
 */
PPE_ERR PPE_Clear_Rect(ppe_buffer_t *buffer, ppe_rect_t *rect, uint32_t color);

/**
 * \brief  Blend source image onto target buffer.
 * \param[in] image: Source image buffer to be blended.
 * \param[in] buffer: Target image buffer to be blended onto.
 * \param[in] trans: Position information of source image.
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
        PPE_Blend(&image, &buffer, &trans, PPE_BYPASS_MODE);
    }
 * \endcode
 */
PPE_ERR PPE_Blend(ppe_buffer_t *image, ppe_buffer_t *buffer, ppe_translate_t *trans,
                  PPE_BLEND_MODE blend_mode);

/**
 * \brief  Blend source image to certain area target buffer.
 * \param[in] image: Source image buffer to be blended.
 * \param[in] buffer: Target image buffer to be bleneded onto.
 * \param[in] trans: Position information of source image.
 * \param[in] rect: Constraint blend area.
 * \param[in] blend_mode: Blend mode from @ref PPE_BLEND_MODE to be used.
 * \return Operation result.
 * \retval PPE_SUCCESS   Operation success.
 * \retval Others        Operation failure, cause refers to @ref PPE_ERR .
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
        PPE_Blend_Rect(&image, &buffer, &trans, &rect, PPE_BYPASS_MODE);
    }
 * \endcode
 */
PPE_ERR PPE_Blend_Rect(ppe_buffer_t *image, ppe_buffer_t *buffer, ppe_translate_t *trans,
                       ppe_rect_t *rect, PPE_BLEND_MODE blend_mode);


/**
 * \brief  Blend multiple input layers to output layer.
 * \param[in] list: List of layers to be blended.
 * \return Operation result.
 * \retval PPE_SUCCESS  Operation success.
 * \retval Others       Operation failure, cause refers to @ref PPE_ERR .
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

        PPE_ERR err = PPE_Blend_Multi(list);
    }
 * \endcode
 */
PPE_ERR PPE_Blend_Multi(ppe_input_list_t list);

/**
 * \brief  Get pixel size of specified format.
 * \param[in] format: Pixel format of PPE from @ref PPE_PIXEL_FORMAT .
 * \return Pixel size in bytes.
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
 * \brief  Get intersected area of 2 rectangle areas.
 * \param[in] result_rect: Pointer to rect that stores result
 * \param[in] rect1: Pointer to one rectangle area
 * \param[in] rect2: Pointer to another rectangle area
 * \return Operation result.
 * \retval true      rectangle areas have intersection.
 * \retval false     rectangle areas have no intersection.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        if (ppe_rect_intersect(&result, &rect1, &rect2))
        {
            //User operation
        }
    }
 * \endcode
 */
bool ppe_rect_intersect(ppe_rect_t *result_rect, ppe_rect_t *rect1, ppe_rect_t *rect2);


/**
 * \brief  Place mask on target area with specified color.
 * \param[in] src: Source buffer.
 * \param[in] target: Target buffer, can be same with src.
 * \param[in] rect: The boundary of the part to be masked.
 * \param[in] color: Specified color in ABGR8888 format. \ref PPE_ABGR8888
 * \return Operation result.
 * \retval PPE_SUCCESS   Operation success.
 * \retval Others        Operation failure, cause refers to @ref PPE_ERR .
 *
 * <b>Example usage</b>
 * \code{.c}
    void test_code(void){
        RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);
        ppe_buffer_t image, buffer;
        image.width = 25;
        image.height = 25;
        image.format = PPE_RGB888;
        image.memory = (uint32_t*)PSRAM_BUFFER;
        image.address = (uint32_t)image.memory;
        memcpy(&buffer, &image, sizeof(ppe_buffer_t));
        ppe_rect_t rect = {0, 0, 24, 24};
        uint32_t color = 0x80AABBCC;
        PPE_ERR err = PPE_Mask(&image, &buffer, &rect, color);
    }
 * \endcode
 */
PPE_ERR PPE_Mask(ppe_buffer_t *src, ppe_buffer_t *target, ppe_rect_t *rect, uint32_t color);


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