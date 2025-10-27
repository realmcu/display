
#include <stdio.h>
#include <stdint.h>
#include "rtl_ppe.h"
#include <gui_matrix.h>
#include "math.h"
#include "def_file.h"
#include "draw_img.h"

#define PPE_ACC_MIN_OPA 0

void hw_acc_clear(uint32_t color, struct gui_dispdev *dc)
{
    ppe_buffer_t target;
    memset(&target, 0, sizeof(ppe_buffer_t));
    target.address = (uint32_t)dc->frame_buf;
    target.width = dc->fb_width;
    target.height = dc->fb_height;
    target.stride = dc->fb_width;
    target.format = PPE_RGB565;
    target.const_color = 0;
    target.win_x_min = 0;
    target.win_x_max = target.width - 1;
    target.win_y_min = 0;
    target.win_y_max = target.height - 1;
    ppe_rect_t rect1 = {.x = 0, .y = 0, .w = dc->fb_width, .h = dc->fb_height};
    PPE_Mask(&target, color, &rect1);
}

extern void sw_acc_blit(draw_img_t *image, struct gui_dispdev *dc, gui_rect_t *rect);
extern uint32_t rtos_time_get_current_system_time_us(void);
void hw_acc_blit(draw_img_t *image, struct gui_dispdev *dc, struct gui_rect *rect)
{
    if (image->inverse.m[0][1] != 0 || image->inverse.m[1][0] != 0)
    {
        sw_acc_blit(image, dc, rect);
        return;
    }
#if 0
    (void)rect;
    (void)image;
    ppe_buffer_t target;
    memset(&target, 0, sizeof(ppe_buffer_t));
    target.address = (uint32_t)dc->frame_buf;
    target.width = dc->fb_width;
    target.height = dc->fb_height;
    target.stride = dc->fb_width;
    target.format = PPE_RGB565;
    target.const_color = 0;
    target.win_x_min = 0;
    target.win_x_max = target.width - 1;
    target.win_y_min = 0;
    target.win_y_max = target.height - 1;
    ppe_rect_t rect1 = {.x = 0, .y = 0, .w = dc->fb_width, .h = dc->fb_height};

    uint32_t time1 = rtos_time_get_current_system_time_us();
    PPE_Mask(&target, 0xFF00FF00, &rect1);
    PPE_Finish();
    uint32_t time2 = rtos_time_get_current_system_time_us();
    gui_log("PPE mask %d us", time2 - time1);
#else
    // uint32_t time1 = rtos_time_get_current_system_time_us();
    int32_t x_max = (image->img_target_w + image->img_target_x - 1);
    int32_t y_max = (image->img_target_h + image->img_target_y - 1);
    int32_t x_min = image->img_target_x;
    int32_t y_min = image->img_target_y;
    if (dc->section.y2 < y_min || dc->section.y1 > y_max || dc->section.x2 < x_min ||
        dc->section.x1 > x_max)
    {
        return;
    }
    if (image->opacity_value <= PPE_ACC_MIN_OPA)
    {
        return;
    }
    //gui_log("draw x %d -> %d, y %d -> %d\n", x_min, x_max, y_min, y_max);
    ppe_buffer_t target, source;
    memset(&target, 0, sizeof(ppe_buffer_t));
    memset(&source, 0, sizeof(ppe_buffer_t));

    ppe_matrix_t inverse;
    memcpy(&inverse, &image->inverse, sizeof(float) * 9);
    PPE_BLEND_MODE mode = PPE_SRC_OVER_MODE;

    switch (dc->bit_depth)
    {
    case 16:
        target.format = PPE_RGB565;
        break;
    case 24:
        target.format = PPE_RGB888;
        break;
    case 32:
        target.format = PPE_ARGB8888;
        break;
    default:
        return;
    }
    target.address = (uint32_t)dc->frame_buf;
    target.width = dc->fb_width;
    target.height = dc->fb_height;
    target.stride = dc->fb_width;
    target.const_color = 0;
    target.win_x_min = 0;
    target.win_x_max = target.width - 1;
    target.win_y_min = 0;
    target.win_y_max = target.height - 1;

    source.opacity = image->opacity_value;
    source.stride = image->img_w;
    source.const_color = 0xFFFFFFFF;
    struct gui_rgb_data_head *head = image->data;
    switch (head->type)
    {
    case RGB565:
        source.format = PPE_RGB565;
        break;
    case RGB888:
        source.format = PPE_RGB888;
        break;
    case ARGB8888:
        source.format = PPE_ARGB8888;
        break;
    case ARGB8565:
        source.format = PPE_ARGB8565;
        break;
    case XRGB8888:
        source.format = PPE_XRGB8888;
        break;
    default:
        return;
    }
    source.win_x_min = 0;
    source.win_x_max = target.width - 1;
    source.win_y_min = 0;
    source.win_y_max = target.height - 1;
    if (rect != NULL)
    {
        source.address = (uint32_t)image->data + sizeof(struct gui_rgb_data_head) +
                         (rect->y1 * source.stride + rect->x1) * PPE_Get_Pixel_Size(source.format);
        source.width = rect->x2 - rect->x1 + 1;
        source.height = rect->y2 - rect->y1 + 1;
        ppe_matrix_t pre_trans;
        ppe_get_identity(&pre_trans);
        pre_trans.m[0][2] = rect->x1 * -1.0f;
        pre_trans.m[1][2] = rect->y1 * -1.0f;
        ppe_mat_multiply(&pre_trans, &inverse);
        memcpy(&inverse, &pre_trans, sizeof(float) * 9);
    }
    else
    {
        source.address = (uint32_t)image->data + sizeof(struct gui_rgb_data_head);
        source.width = image->img_w;
        source.height = image->img_h;
    }
    int32_t x1 = x_min - dc->section.x1;
    int32_t y1 = y_min - dc->section.y1;
    int32_t x2 = dc->fb_width - 1;
    int32_t y2 = dc->fb_height - 1;
    if (x1 < 0)
    {
        x1 = 0;
    }
    if (y1 < 0)
    {
        y1 = 0;
    }
    if (x_max >= dc->section.x1 + dc->fb_width)
    {
        x2 = dc->section.x1 + dc->fb_width - 1;
    }
    else
    {
        x2 = x_max - dc->section.x1;
    }
    if (y_max >= dc->section.y1 + dc->fb_height)
    {
        y2 = dc->fb_height - 1;
    }
    else
    {
        y2 = y_max - dc->section.y1;
    }
    ppe_rect_t constraint = (ppe_rect_t) {.x = x1, .y = y1, .w = x2 - x1 + 1, .h = y2 - y1 + 1};

    ppe_translate(0, dc->section.y1, &inverse);
    source.high_quality = true;
    if (image->blend_mode == IMG_FILTER_BLACK)
    {
        source.color_key_enable = PPE_COLOR_KEY_INSIDE;
        source.color_key_min = 0;
        source.color_key_max = 0;
    }
    if (!ppe_matrix_is_complex(&inverse))
    {
        source.high_quality = false;
        if ((source.format == PPE_RGB565 || source.format == PPE_RGB888) \
            && image->blend_mode != IMG_FILTER_BLACK)
        {
            mode = PPE_BYPASS_MODE;
        }
    }
    PPE_Finish();
    PPE_err err = PPE_Blit_Inverse(&target, &source, &inverse, &constraint,
                                   mode);
    // PPE_Finish();
    if (err != PPE_SUCCESS)
    {
        return;
    }
    if (!gui_get_acc()->enable_async)
    {
        PPE_Finish();
    }
    // uint32_t time2 = rtos_time_get_current_system_time_us();
    // gui_log("PPE draw %x use %d.%d ms\n", image->data, (time2 - time1) / 1000, (time2 - time1) % 1000);
    return;
#endif
}

void hw_acc_init(void)
{
    //PPE clock init
}
