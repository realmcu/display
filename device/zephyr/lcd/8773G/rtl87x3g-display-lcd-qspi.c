#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <errno.h>

#ifdef CONFIG_REALTEK_SH8601Z_454454_QSPI
#include "SH8601Z_454454_qspi.h"
#endif
#ifdef CONFIG_REALTEK_LCD_SH8601Z_410_502_QSPI
#include "lcd_sh8601z_410_502_qspi.h"
#endif
#ifdef CONFIG_REALTEK_LCD_ST77916_360_360_QSPI
#include "lcd_st77916_360_360_qspi.h"
#endif
#ifdef CONFIG_REALTEK_LCD_ICNA3310_466_466_QSPI
#include "lcd_icna3310_466_466_qspi.h"
#endif
#ifdef CONFIG_REALTEK_LCD_ST7801N_466_466_QSPI
#include "lcd_st7801n_466_466_qspi.h"
#endif

#define DT_DRV_COMPAT realtek_rtl87x3g_display_lcd

struct lcd_display_config
{
    /** Base address of the LCD peripheral registers. */
    mm_reg_t base;

    /** Size of the LCD peripheral's memory-mapped register region. */
    size_t mem_size;

    /** Display width in pixels. */
    uint16_t width;

    /** Display height in pixels. */
    uint16_t height;

    /** Pixel format */
    enum display_pixel_format pixel_format;
};

struct lcd_display_data
{
    /** Framebuffer pointer, if applicable. */
    void *framebuffer;

    /** Display capabilities */
    struct display_capabilities capabilities;

    /** Initialization flag */
    bool initialized;
};
static int lcd_display_hw_init(const struct device *dev);
static void lcd_display_get_capabilities(const struct device *dev,
                                         struct display_capabilities *caps)
{
    const struct lcd_display_config *config = dev->config;
    struct lcd_display_data *data = dev->data;

    // Return actual capabilities from HAL
    caps->x_resolution = rtk_lcd_hal_get_width();
    caps->y_resolution = rtk_lcd_hal_get_height();

    // Convert HAL pixel bits to Zephyr pixel format
    uint8_t pixel_bits = rtk_lcd_hal_get_pixel_bits();
    switch (pixel_bits)
    {
    case 16:
        caps->current_pixel_format = PIXEL_FORMAT_RGB_565;
        caps->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
        break;
    case 24:
        caps->current_pixel_format = PIXEL_FORMAT_RGB_888;
        caps->supported_pixel_formats = PIXEL_FORMAT_RGB_888;
        break;
    case 32:
        caps->current_pixel_format = PIXEL_FORMAT_ARGB_8888;
        caps->supported_pixel_formats = PIXEL_FORMAT_ARGB_8888;
        break;
    default:
        caps->current_pixel_format = config->pixel_format;
        caps->supported_pixel_formats = config->pixel_format;
        break;
    }

    caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
    caps->screen_info = 0; // Adjust based on your display characteristics

}

static int lcd_display_blanking_on(const struct device *dev)
{
    struct lcd_display_data *data = dev->data;

    if (!data->initialized)
    {
        return 0;
    }

    printk("Display Blanking ON (Power Off)\n");
    bool ret = rtk_lcd_hal_power_off();
    // rtk_lcd_hal_lcd_enter_dlps();

    if (ret)
    {
        return 0;
    }
    else
    {
        return -EIO;
    }
}

static int lcd_display_blanking_off(const struct device *dev)
{
    struct lcd_display_data *data = dev->data;

    printk("Display Blanking OFF (Power On)\n");
    bool ret = rtk_lcd_hal_power_on();

    if (ret)
    {
        return 0;
    }
    else
    {
        return -EIO;
    }
}

static int lcd_display_write(const struct device *dev, const uint16_t x, const uint16_t y,
                             const struct display_buffer_descriptor *desc,
                             const void *buf)
{
    uint16_t w = desc->width;
    uint16_t h = desc->height;

    rtk_lcd_hal_set_window(x, y, w, h);
    rtk_lcd_hal_start_transfer((void *)buf, w * h);
    rtk_lcd_hal_transfer_done();

    return 0;
}

static void *lcd_display_get_framebuffer(const struct device *dev)
{
    // Return framebuffer if your display supports direct framebuffer access
    // return rtk_lcd_hal_get_framebuffer();
    return NULL; // Return NULL if not supported
}

static int lcd_display_set_brightness(const struct device *dev, uint8_t brightness)
{
    // Implement brightness control if supported by your hardware
    // Todo
    return -ENOSYS; // Return -ENOSYS if not supported
}

static int lcd_display_set_contrast(const struct device *dev, uint8_t contrast)
{
    return -ENOSYS;
}

static int lcd_display_set_pixel_format(const struct device *dev,
                                        const enum display_pixel_format pixel_format)
{
    return -ENOSYS;
}

static int lcd_display_set_orientation(const struct device *dev,
                                       const enum display_orientation orientation)
{
    return -ENOSYS;
}

static int lcd_display_init(const struct device *dev)
{
    const struct lcd_display_config *config = dev->config;
    struct lcd_display_data *data = dev->data;

    printk("Initializing Realtek LCD Display '%s'...\n", dev->name);
    printk("Display resolution: %dx%d\n", config->width, config->height);

    // Initialize data structure
    data->initialized = false;
    data->framebuffer = NULL;
    int ret = lcd_display_hw_init(dev);
    if (ret != 0)
    {
        printk("ERROR: LCD hardware initialization failed: %d\n", ret);
        return ret;
    }

    return 0;
}

static int lcd_display_hw_init(const struct device *dev)
{
    struct lcd_display_data *data = dev->data;

    if (data->initialized)
    {
        return 0;
    }

    printk("Initializing LCD hardware...\n");

    // wating for the system to be ready
    k_busy_wait(500); // delay for 0.5ms

    // LCD hardware initialization
    rtk_lcd_hal_init();

    // k_busy_wait(5000); // delay for 5ms

    data->initialized = true;
    printk("LCD hardware initialized successfully\n");
    return 0;
}

static const struct display_driver_api lcd_display_api =
{
    .blanking_on = lcd_display_blanking_on,
    .blanking_off = lcd_display_blanking_off,
    .write = lcd_display_write,
    .read = NULL, // Set to NULL if read is not supported
    .get_framebuffer = lcd_display_get_framebuffer,
    .set_brightness = lcd_display_set_brightness,
    .set_contrast = lcd_display_set_contrast,
    .get_capabilities = lcd_display_get_capabilities,
    .set_pixel_format = lcd_display_set_pixel_format,
    .set_orientation = lcd_display_set_orientation,
};

// Helper macro to convert DT pixel format string to enum
#define LCD_DISPLAY_INIT(index)                                             \
    static struct lcd_display_data lcd_display_data_##index;                \
    static const struct lcd_display_config lcd_display_cfg_##index = {      \
        .base = DT_INST_REG_ADDR(index),                                    \
                .mem_size = DT_INST_REG_SIZE(index),                                \
                            .width = DT_INST_PROP(index, width),                                \
                                     .height = DT_INST_PROP(index, height),                              \
                                               .pixel_format = PIXEL_FORMAT_RGB_565,                               \
    };                                                                      \
    DEVICE_DT_INST_DEFINE(index,                                            \
                          lcd_display_init,                                 \
                          NULL,                                             \
                          &lcd_display_data_##index,                        \
                          &lcd_display_cfg_##index,                         \
                          POST_KERNEL,                                      \
                          CONFIG_DISPLAY_INIT_PRIORITY,                     \
                          &lcd_display_api);

DT_INST_FOREACH_STATUS_OKAY(LCD_DISPLAY_INIT)