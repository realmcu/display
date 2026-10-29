#ifndef _LCD_JD9165BA_1024600_RGB_H_
#define _LCD_JD9165BA_1024600_RGB_H_

#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JD9165BA_1024_600_LCD_WIDTH                1024
#define JD9165BA_1024_600_LCD_HEIGHT               600
#define JD9165BA_DRV_PIXEL_BITS                    16

typedef enum
{
    LCDC_TE_TYPE_NO_TE = 0x00,
    LCDC_TE_TYPE_HW_TE = 0x01,
    LCDC_TE_TYPE_SW_TE = 0x02,
} T_LCDC_TE_TYPE;

void rtk_lcd_hal_init(void);
void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t w, uint16_t h);
void rtk_lcd_hal_clear_screen(uint32_t ARGB_color);
void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len);
void rtk_lcd_hal_transfer_done(void);
uint32_t rtk_lcd_hal_get_width(void);
uint32_t rtk_lcd_hal_get_height(void);
uint32_t rtk_lcd_hal_get_pixel_bits(void);
bool rtk_lcd_hal_power_off(void);
bool rtk_lcd_hal_power_on(void);
void rtk_lcd_hal_lcd_enter_dlps(void);
void rtk_lcd_hal_set_TE_type(T_LCDC_TE_TYPE state);
T_LCDC_TE_TYPE rtk_lcd_hal_get_TE_type(void);
bool rtk_lcd_hal_dlps_check(void);
bool rtk_lcd_wake_up(void);
uint32_t rtk_lcd_hal_dlps_restore(void);
void rtk_lcd_dlps_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _LCD_JD9165BA_1024600_RGB_H_ */
