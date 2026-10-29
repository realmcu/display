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
#define  SH8601A_LCD_HSYNC    (2U)
#define  SH8601A_LCD_HBP      (10U)
#define  SH8601A_LCD_HFP      (10U)
#define  SH8601A_LCD_VSYNC    (2U)
#define  SH8601A_LCD_VBP      (8U)
#define  SH8601A_LCD_VFP      (8U)


#define INPUT_PIXEL_BYTES                   3
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


void lcd_sh8601a_mipi_video_init_framebuffer(uint8_t *buf, uint32_t len);
void lcd_sh8601a_mipi_video_update_framebuffer(uint8_t *buf, uint32_t len);



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
