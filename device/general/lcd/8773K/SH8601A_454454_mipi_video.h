/**
*********************************************************************************************************
*     Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.

*
*     SPDX-License-Identifier: Apache-2.0
**********************************************************************************************************
* @file
* @brief
* @details
* @author   howie wang
* @date     2022-02-16
* @version  v0.1
*********************************************************************************************************
*/
#ifndef __SH8601A_454454_MIPI_VIDEO_H
#define __SH8601A_454454_MIPI_VIDEO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
/** @addtogroup BSP
  * @{
  */

/** @addtogroup Components
  * @{
  */

/** @addtogroup otm8009a
  * @{
  */

/** @addtogroup OTM8009A_Exported_Variables
  * @{
  */

#if defined ( __GNUC__ ) || (defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)) /* GNU and ARM Compiler 6 compilers */
#ifndef __weak
#define __weak __attribute__((weak))
#endif /* __weak */
#endif /* __GNUC__ || (__ARMCC_VERSION && (__ARMCC_VERSION >= 6010050)) */

#define  SH8601A_LCD_WIDTH    (454U)
#define  SH8601A_LCD_HEIGHT   (454U)
//#define  SH8601A_LCD_WIDTH    (480)
//#define  SH8601A_LCD_HEIGHT   (480)
/*
 * Host-side video timing. The module specification confirms 454 x 454 and
 * one MIPI lane, but leaves the video porch values to the host integration.
 */
#define  SH8601A_LCD_HSYNC    (2U)
#define  SH8601A_LCD_HBP      (16U)
#define  SH8601A_LCD_HFP      (20U)
#define  SH8601A_LCD_VSYNC    (2U)
#define  SH8601A_LCD_VBP      (8U)
#define  SH8601A_LCD_VFP      (8U)


///*size from command mode*/
//#define  SH8601A_LCD_WIDTH    (454U)
//#define  SH8601A_LCD_HEIGHT   (454U)

//#define  SH8601A_LCD_HSYNC    (5U)
//#define  SH8601A_LCD_HBP      (15U)
//#define  SH8601A_LCD_HFP      (16U)

//#define  SH8601A_LCD_VSYNC    (2U)
//#define  SH8601A_LCD_VBP      (34U)
//#define  SH8601A_LCD_VFP      (34U)


#define INPUT_PIXEL_BYTES                   4
#define OUTPUT_PIXEL_BYTES                  3


void rtk_lcd_hal_init(void);

/**
 * \brief  Switch the DSI host from Command mode to Standard Video mode.
 *
 * \note   The panel initialization sequence is sent while the host is still in
 *         Command mode (MODE_CFG.cmd_video_mode = 1), because in Video mode the
 *         host only drains the generic command FIFO during the blanking periods
 *         of an already-running pixel stream. Call this only after the panel
 *         sequence has completed; \ref rtk_lcd_hal_init already invokes it, so
 *         a separate call is normally not needed.
 */
void sh8601a_video_mode_init(void);

/**
 * \brief  Fill the whole active area with one color and keep refreshing it.
 *
 * \param  color: Pixel value in the configured LCDC input format
 *                (ARGB8888 by default, e.g. 0xFF00FF00 for green).
 *
 * \note   Uses a fixed DMA source address, so the single word feeds every pixel
 *         of the frame. Infinite mode is re-armed from LCDC_IRQHandler, so the
 *         frame keeps being sent until an update/scroll call replaces it.
 */
void lcd_sh8601a_mipi_video_fill_screen(uint32_t color);

void lcd_sh8601a_mipi_video_init_framebuffer(uint8_t *buf, uint32_t len);
void lcd_sh8601a_mipi_video_update_framebuffer(uint8_t *buf, uint32_t len);

/**
 * \brief  Start a continuous vertical (up/down) scroll of an image.
 *
 * \param  up:   Pointer (uint32_t) to the memory holding the upper part image.
 * \param  down: Pointer (uint32_t) to the memory holding the lower part image.
 * \param  w:    Image width in pixels.
 * \param  h:    Image height in pixels.
 * \param  offset: Initial scroll row offset (0..h). 0 shows only "up", h shows
 *               only "down", between shows the two half-frames split at that row.
 * \param  bytes_per_pixel: Bytes per pixel of the source buffers.
 *
 * \note   Both source buffers must be 4-byte aligned. The offset is advanced by
 *         one row on every finished frame inside LCDC_IRQHandler (continuous
 *         scroll keeps running; after the last row it cycles back to 0).
 *         To stop a scroll, set the offset back to 0 / disable it, or call again
 *         with offset == 0.
 *
 * \code{.c}
 *     rtk_lcd_hal_update_fb_use_up_and_down((uint32_t)up_buf, (uint32_t)down_buf,
 *                                            SH8601A_LCD_WIDTH, SH8601A_LCD_HEIGHT,
 *                                            0, 4);   // start scrolling down
 * \endcode
 */
void rtk_lcd_hal_update_fb_use_up_and_down(uint32_t up,
                                           uint32_t down,
                                           uint32_t w,
                                           uint32_t h,
                                           uint32_t offset,
                                           uint32_t bytes_per_pixel);

/**
 * \brief  Start a continuous horizontal (left/right) scroll of an image.
 *
 * \param  left:  Pointer (uint32_t) to the memory holding the left part image.
 * \param  right: Pointer (uint32_t) to the memory holding the right part image.
 * \param  w:     Image width in pixels.
 * \param  h:     Image height in pixels.
 * \param  offset: Initial scroll column offset (0..w). 0 shows only "left", w
 *               shows only "right", between shows the two half-frames split at
 *               that column (each row is cut into two segments).
 * \param  bytes_per_pixel: Bytes per pixel of the source buffers.
 *
 * \note   Both source buffers must be 4-byte aligned. The offset is advanced by
 *         one column on every finished frame inside LCDC_IRQHandler; after the
 *         last column it cycles back to 0.
 *
 * \code{.c}
 *     rtk_lcd_hal_update_fb_use_left_and_right((uint32_t)left_buf, (uint32_t)right_buf,
 *                                              SH8601A_LCD_WIDTH, SH8601A_LCD_HEIGHT,
 *                                              0, 4);  // start scrolling right
 * \endcode
 */
void rtk_lcd_hal_update_fb_use_left_and_right(uint32_t left,
                                              uint32_t right,
                                              uint32_t w,
                                              uint32_t h,
                                              uint32_t offset,
                                              uint32_t bytes_per_pixel);



#ifdef __cplusplus
}
#endif

#endif /* __SH8601A_454454_MIPI_VIDEO_H */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
