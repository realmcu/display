/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <draw_img.h>
#include <stdio.h>
#include <stdint.h>
#include <gui_matrix.h>
#include <rtl_ppe.h>
#include <rtl_idu.h>
#include <rtl_idu_int.h>
#include <rtl876x_rcc.h>
#include <rtl876x_gdma.h>
#include <dma_channel.h>
#include "rtl876x_gpio.h"
#include "os_mem.h"
#include "rtl876x_pinmux.h"
#include "math.h"
#include "fmc_api_ext.h"
#include "os_sync.h"

#define F_APP_GUI_USE_PSRAM 1
#ifndef PSRAM_GUI_HEAP_ADDR
#define PSRAM_GUI_HEAP_ADDR 0x4000000
#endif


extern void sw_acc_blit(draw_img_t *image, struct gui_dispdev *dc, struct gui_rect *rect);

#define PPE_BARE_COPY_ACCURATE      1
#define PPE_ACC_MIN_OPA             3
#define PPE_TESS_LENGTH             92

#include "section.h"
#define CACHE_BUF_SIZE           (42 * 1024)
SHM_DATA_SECTION static uint8_t cache_buf1[CACHE_BUF_SIZE];
static uint8_t *cache_buf2;
static uint8_t *cache_buf = cache_buf1;
static uint32_t last_cache_size = 0;
static uint8_t high_speed_dma_channel_num = 0xFF;
static uint8_t low_speed_dma_channel_num = 0xFF;

static void change_cache_buf(uint32_t cache_size)
{
    if (cache_buf == cache_buf1)
    {
        cache_buf2 = cache_buf1 + (CACHE_BUF_SIZE - cache_size - 4);
        cache_buf = cache_buf2;
    }
    else if (cache_buf == cache_buf2)
    {
        cache_buf = cache_buf1;
    }
#if F_APP_GUI_USE_PSRAM
    else if ((uint32_t)cache_buf > PSRAM_GUI_HEAP_ADDR)
    {
        gui_lower_free(cache_buf);
        cache_buf = cache_buf1;
    }
#endif
    else
    {
        cache_buf = cache_buf1;
    }
}
static void restore_cache_buf(void)
{
#if F_APP_GUI_USE_PSRAM
    if ((uint32_t)cache_buf > PSRAM_GUI_HEAP_ADDR)
    {
        gui_lower_free(cache_buf);
    }
#endif
    cache_buf = cache_buf1;
}

#if F_APP_GUI_USE_PSRAM
static void cache_on_psram(uint32_t size)
{
    if ((uint32_t)cache_buf > PSRAM_GUI_HEAP_ADDR)
    {
        gui_lower_free(cache_buf);
    }
    cache_buf = gui_lower_malloc(size);
}
#endif

void hw_dma_copy(uint32_t length, uint32_t height, uint32_t src_stride,
                 uint32_t dst_stride, uint8_t *src, uint8_t *dst)
{
    bool use_LLI = true;
    if ((length == src_stride && length == dst_stride) || height == 1)
    {
        use_LLI = false;
    }
    uint32_t dma_height = height;
    uint32_t buffer_size = 0;
    uint32_t total_size_in_byte = length * height;
    uint32_t total_size = 0;
    uint8_t m_size = 0, data_size = 0;
    uint8_t dma_depth = rtl_idu_get_dma_depth(high_speed_dma_channel_num);
    if (length % 4 == 0)
    {
        data_size = GDMA_DataSize_Word;
        if (dma_depth == 4)
        {
            m_size = GDMA_Msize_4;
        }
        else if (dma_depth == 8)
        {
            m_size = GDMA_Msize_8;
        }
        else if (dma_depth == 16)
        {
            m_size = GDMA_Msize_16;
        }
        else
        {
            return;
        }
        buffer_size = length / 4;
        total_size = 0;
        if (!use_LLI)
        {
            total_size = total_size_in_byte / 4;
            if (total_size > 65535)
            {
                use_LLI = true;
                dma_height = total_size / 65535;
                if (total_size % 65535)
                {
                    dma_height += 1;
                }
                buffer_size = 65535;
            }
            else
            {
                buffer_size = total_size;
            }
        }
    }
    else if (length % 2 == 0)
    {
        if (dma_depth == 4)
        {
            m_size = GDMA_Msize_8;
        }
        else if (dma_depth == 8)
        {
            m_size = GDMA_Msize_16;
        }
        else if (dma_depth == 16)
        {
            m_size = GDMA_Msize_32;
        }
        else
        {
            return;
        }
        data_size = GDMA_DataSize_HalfWord;
        buffer_size = length / 2;
        total_size = 0;
        if (!use_LLI)
        {
            total_size = total_size_in_byte / 2;
            if (total_size > 65535)
            {
                use_LLI = true;
                dma_height = total_size / 65535;
                if (total_size % 65535)
                {
                    dma_height += 1;
                }
                buffer_size = 65535;
            }
            else
            {
                buffer_size = total_size;
            }
        }
    }
    else
    {
        data_size = GDMA_DataSize_Byte;
        if (dma_depth == 4)
        {
            m_size = GDMA_Msize_16;
        }
        else if (dma_depth == 8)
        {
            m_size = GDMA_Msize_32;
        }
        else if (dma_depth == 16)
        {
            m_size = GDMA_Msize_64;
        }
        else
        {
            return;
        }
        buffer_size = length;
        total_size = 0;
        if (!use_LLI)
        {
            total_size = total_size_in_byte;
            if (total_size > 65535)
            {
                use_LLI = true;
                dma_height = total_size / 65535;
                if (total_size % 65535)
                {
                    dma_height += 1;
                }
                buffer_size = 65535;
            }
            else
            {
                buffer_size = total_size;
            }
        }
    }
    GDMA_LLIDef *GDMA_LLIStruct;
    if (use_LLI)
    {
        GDMA_LLIStruct = (GDMA_LLIDef *)gui_malloc(dma_height * sizeof(GDMA_LLIDef));
        if (GDMA_LLIStruct == NULL)
        {
            assert_param(GDMA_LLIStruct != NULL);
        }
        else
        {
            memset(GDMA_LLIStruct, 0, dma_height * sizeof(GDMA_LLIDef));
        }
    }
    uint32_t start_address = (uint32_t)src;
    uint32_t dest_address = (uint32_t)dst;
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    GDMA_ChannelTypeDef *dma_channel = rtl_idu_get_dma_channel_int(high_speed_dma_channel_num);
    GDMA_InitTypeDef RX_GDMA_InitStruct;
    /*--------------GDMA init-----------------------------*/
    GDMA_StructInit(&RX_GDMA_InitStruct);
    RX_GDMA_InitStruct.GDMA_ChannelNum          = high_speed_dma_channel_num;
    RX_GDMA_InitStruct.GDMA_BufferSize          = buffer_size;
    RX_GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToMemory;
    RX_GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    RX_GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Inc;
    RX_GDMA_InitStruct.GDMA_SourceMsize         =
        m_size;                         // 8 msize for source msize
    RX_GDMA_InitStruct.GDMA_DestinationMsize    =
        m_size;                         // 8 msize for destiantion msize
    RX_GDMA_InitStruct.GDMA_DestinationDataSize =
        data_size;                   // 32 bit width for destination transaction
    RX_GDMA_InitStruct.GDMA_SourceDataSize      =
        data_size;                   // 32 bit width for source transaction
    RX_GDMA_InitStruct.GDMA_SourceAddr          = (uint32_t)start_address;
    RX_GDMA_InitStruct.GDMA_DestinationAddr     = (uint32_t)dest_address;

    if (use_LLI)
    {
        RX_GDMA_InitStruct.GDMA_Multi_Block_Mode = LLI_TRANSFER;
        RX_GDMA_InitStruct.GDMA_Multi_Block_En = 1;
        RX_GDMA_InitStruct.GDMA_Multi_Block_Struct = (uint32_t)GDMA_LLIStruct;
    }

    GDMA_Init(dma_channel, &RX_GDMA_InitStruct);
    if (use_LLI && GDMA_LLIStruct != NULL)
    {
        for (int i = 0; i < dma_height; i++)
        {
            if (i == dma_height - 1)
            {
                GDMA_LLIStruct[i].SAR = start_address + src_stride * i;
                GDMA_LLIStruct[i].DAR = (uint32_t)dest_address + dst_stride * i;
                GDMA_LLIStruct[i].LLP = 0;
                /* configure low 32 bit of CTL register */
                GDMA_LLIStruct[i].CTL_LOW = (BIT(0)
                                             | (RX_GDMA_InitStruct.GDMA_DestinationDataSize << 1)
                                             | (data_size << 4)
                                             | (RX_GDMA_InitStruct.GDMA_DestinationInc << 7)
                                             | (RX_GDMA_InitStruct.GDMA_SourceInc << 9)
                                             | (RX_GDMA_InitStruct.GDMA_DestinationMsize << 11)
                                             | (RX_GDMA_InitStruct.GDMA_SourceMsize << 14)
                                             | (RX_GDMA_InitStruct.GDMA_DIR << 20));
                /* configure high 32 bit of CTL register */
                if (total_size == 0)
                {
                    GDMA_LLIStruct[i].CTL_HIGH = buffer_size;
                }
                else
                {
                    GDMA_LLIStruct[i].CTL_HIGH = total_size - i * buffer_size;
                }
            }
            else
            {
                GDMA_LLIStruct[i].SAR = start_address + src_stride * i;
                GDMA_LLIStruct[i].DAR = (uint32_t)dest_address + dst_stride * i;
                GDMA_LLIStruct[i].LLP = (uint32_t)&GDMA_LLIStruct[i + 1];
                /* configure low 32 bit of CTL register */
                GDMA_LLIStruct[i].CTL_LOW = rtl_idu_get_dma_ctl_low_int(dma_channel);
                /* configure high 32 bit of CTL register */
                GDMA_LLIStruct[i].CTL_HIGH = buffer_size;
            }
        }
    }
    GDMA_INTConfig(high_speed_dma_channel_num, GDMA_INT_Transfer, ENABLE);
    GDMA_Cmd(high_speed_dma_channel_num, ENABLE);
    while (GDMA_GetTransferINTStatus(high_speed_dma_channel_num) != SET);
    GDMA_ClearINTPendingBit(high_speed_dma_channel_num, GDMA_INT_Transfer);
    if (use_LLI && GDMA_LLIStruct != NULL)
    {
        gui_free(GDMA_LLIStruct);
    }
}

bool hw_idu_decode(ppe_rect_t *rect, uint8_t *input, uint32_t buffer_stride, uint8_t *buffer)
{
    IDU_file_header *header = (IDU_file_header *)input;
    IDU_decode_range range;
    range.start_column = rect->x;
    range.end_column = rect->x + rect->w - 1;
    range.start_line = rect->y;
    range.end_line = rect->y + rect->h - 1;
    range.target_stride = buffer_stride;
    IDU_DMA_config dma_cfg = {0};
    dma_cfg.output_buf = (uint32_t *)buffer;
    dma_cfg.RX_DMA_channel_num = low_speed_dma_channel_num;
    dma_cfg.TX_DMA_channel_num = high_speed_dma_channel_num;
    dma_cfg.TX_FIFO_INT_threshold = 8;
    dma_cfg.RX_FIFO_INT_threshold = 8;
    IDU_ERROR err = IDU_Decode((uint8_t *)header, &range, &dma_cfg);
    if (err != IDU_SUCCESS)
    {
        return false;
    }
    else
    {
        return true;
    }
}

static bool memcpy_by_dma(ppe_rect_t *p_rect, ppe_buffer_t *source)
{
    uint8_t pixel_size = PPE_Get_Pixel_Size(source->format);
    if (p_rect->w * p_rect->h * pixel_size > CACHE_BUF_SIZE)
    {
        return false;
    }
    uint32_t length = p_rect->w * pixel_size;

    uint32_t src_stride = source->stride * pixel_size;
    uint32_t height = p_rect->h;
    uint32_t start_address = source->address + (p_rect->x + p_rect->y * source->stride) * pixel_size;
    hw_dma_copy(length, height, src_stride, length, (uint8_t *)start_address, cache_buf);
    source->address = (uint32_t)cache_buf;
    return true;
}


static bool memcpy_by_idu(ppe_rect_t *p_rect, ppe_buffer_t *source)
{
    uint8_t pixel_size = PPE_Get_Pixel_Size(source->format);
#if !F_APP_GUI_USE_PSRAM
    if (p_rect->w * p_rect->h * pixel_size > CACHE_BUF_SIZE)
    {
        return false;
    }
#endif
    uint32_t dst_stride = p_rect->w * pixel_size;
    if (hw_idu_decode(p_rect, (uint8_t *)source->address, dst_stride, cache_buf))
    {
        source->address = (uint32_t)cache_buf;
        return true;
    }
    else
    {
        return false;
    }
}

void bare_blit_by_dma(ppe_buffer_t *target, ppe_buffer_t *source, ppe_rect_t *src_rect,
                      ppe_rect_t *dst_trans)
{
    uint8_t pixel_size = PPE_Get_Pixel_Size(target->format);
    uint32_t dst_start_address = target->address + (dst_trans->x + dst_trans->y * target->stride) *
                                 pixel_size;
    uint32_t src_start_address = source->address + (src_rect->x + src_rect->y * source->stride) *
                                 pixel_size;
    uint32_t length = src_rect->w * pixel_size;
    uint32_t src_stride = source->stride * pixel_size;
    uint32_t dst_stride = target->stride * pixel_size;
    uint32_t height = src_rect->h;
    hw_dma_copy(length, height, src_stride, dst_stride, (uint8_t *)src_start_address,
                (uint8_t *)dst_start_address);
}

static float get_x(float *line, float y)
{
    float x;
    if (line[0] == 0)
    {
        x = 0;
    }
    else if (line[1] == 0)
    {
        x = -line[2];
    }
    else
    {
        x = (-line[2] - line[1] * y) / line[0];
    }
    return x;
}

void acc_get_intersect_area(draw_img_t *image, ppe_rect_t *new_rect, ppe_rect_t *dst,
                            struct gui_dispdev *dc)
{
    memcpy(new_rect, dst, sizeof(ppe_rect_t));
    float left = dst->x * 1.0f, right = (dst->x + dst->w - 1) * 1.0f;
    float top = dst->y * 1.0f;
    float bottom = (dst->y + dst->h - 1) * 1.0f;
    float x_min = dc->fb_width, x_max = -1;
    float *lines = (float *)image->acc_user;
    for (int i = 0; i < 4; i++)
    {
        float x = get_x(&lines[i * 3], top);
        if (x < x_min)
        {
            x_min = x;
        }
        if (x > x_max)
        {
            x_max = x;
        }
    }
    for (int i = 0; i < 4; i++)
    {
        float x = get_x(&lines[i * 3], bottom);
        if (x < x_min)
        {
            x_min = x;
        }
        if (x > x_max)
        {
            x_max = x;
        }
    }
    if (x_min > left)
    {
        new_rect->x = floor(x_min);
    }
    if (x_max < right)
    {
        new_rect->w = ceil(x_max - new_rect->x) + 1;
    }
    else
    {
        new_rect->w = right - new_rect->x + 1;
    }
}

bool acc_get_src_rect_area(ppe_rect_t *src_rect, gui_rect_t *rect)
{
    int32_t src_hor_end = src_rect->x + src_rect->w - 1;
    int32_t src_ver_end = src_rect->y + src_rect->h - 1;
    if (src_rect->x > rect->x2 || src_rect->y > rect->y2 || src_hor_end < rect->x1 ||
        src_ver_end < rect->y1)
    {
        return false;
    }
    if (src_rect->x < rect->x1)
    {
        src_rect->x = rect->x1;
    }
    if (src_rect->y < rect->y1)
    {
        src_rect->y = rect->y1;
    }
    if (src_hor_end > rect->x2)
    {
        src_hor_end = rect->x2;
    }
    if (src_ver_end > rect->y2)
    {
        src_ver_end = rect->y2;
    }
    int new_w = src_hor_end - src_rect->x + 1;
    int new_h = src_ver_end - src_rect->y + 1;
    if (new_w <= 0 || new_h <= 0)
    {
        return false;
    }
    else
    {
        src_rect->w = new_w;
        src_rect->h = new_h;
        return true;
    }
}

void hw_acc_blit(draw_img_t *image, struct gui_dispdev *dc, struct gui_rect *rect)
{
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

    ppe_buffer_t target, source;
    memset(&target, 0, sizeof(ppe_buffer_t));
    memset(&source, 0, sizeof(ppe_buffer_t));
    PPE_BLEND_MODE mode = PPE_BYPASS_MODE;

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

    source.address = (uint32_t)image->data + sizeof(struct gui_rgb_data_head);
    source.opacity = image->opacity_value;
    source.width = image->img_w;
    source.height = image->img_h;
    source.stride = image->img_w;
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

    bool shape_transform = true;
    if (image->blend_mode == IMG_RECT)
    {
        mode = PPE_CONST_MASK_MODE;
    }
    else if ((image->matrix.m[2][2] == 1 && image->matrix.m[0][1] == 0 && \
              image->matrix.m[1][0] == 0 && image->matrix.m[2][0] == 0 && \
              image->matrix.m[2][1] == 0) || image->blend_mode == IMG_COVER_MODE)
    {
        shape_transform = false;
        if ((image->matrix.m[0][0] == 1 && image->matrix.m[1][1] == 1) || image->blend_mode == IMG_RECT)
        {
            if ((image->blend_mode == IMG_BYPASS_MODE && source.format == target.format) ||
                image->blend_mode == IMG_RECT || image->blend_mode == IMG_COVER_MODE)
            {

                if ((x_max < dc->section.x1) || (y_max < dc->section.y1)
                    || (x_min >= dc->section.x1 + dc->fb_width) || (y_min >= dc->section.y1 + dc->fb_height))
                {
                    return;
                }
                ppe_rect_t dst_rect = {.x = x_min - dc->section.x1, .y = y_min - dc->section.y1, .w = dc->fb_width, .h = dc->fb_height};
                if (dst_rect.x < 0)
                {
                    dst_rect.x = 0;
                }
                if (dst_rect.y < 0)
                {
                    dst_rect.y = 0;
                }
                if (x_max >= dc->section.x1 + dc->fb_width)
                {
                    dst_rect.w = dc->fb_width - dst_rect.x;
                }
                else
                {
                    dst_rect.w = x_max - dc->section.x1 - dst_rect.x + 1;
                }
                if (y_max >= dc->section.y1 + dc->fb_height)
                {
                    dst_rect.h = dc->fb_height - dst_rect.y;
                }
                else
                {
                    dst_rect.h = y_max - dc->section.y1 - dst_rect.y + 1;
                }
                dst_rect.x += dc->section.x1;
                dst_rect.y += dc->section.y1;
                ppe_rect_t src_rect;
                ppe_matrix_t inv_matrix;
                if (image->blend_mode == IMG_BYPASS_MODE || image->blend_mode == IMG_COVER_MODE)
                {
                    memcpy(&inv_matrix, &image->inverse, sizeof(float) * 9);
                    source.address = (uint32_t)image->data + sizeof(struct gui_rgb_data_head);
                    source.width = image->img_w;
                    source.height = image->img_h;
                    source.stride = image->img_w;
                    bool ret = ppe_get_area(&src_rect, &dst_rect, &inv_matrix, &source);
#if !PPE_BARE_COPY_ACCURATE
                    if (src_rect.w % 2)
                    {
                        src_rect.w -= 1;
                        if (src_rect.w == 0)
                        {
                            return;
                        }
                    }
#endif
                    if (!ret)
                    {
                        //DBG_DIRECT("MAT err! addr %x, section %x", image->data, dc->section_count);
                        return;
                    }
                    if (rect != NULL)
                    {
                        int16_t x0 = src_rect.x;
                        int16_t y0 = src_rect.y;
                        if (!acc_get_src_rect_area(&src_rect, rect))
                        {
                            return;
                        }
                        dst_rect.x += (src_rect.x - x0);
                        dst_rect.y += (src_rect.y - y0);
                        dst_rect.w = src_rect.w;
                        dst_rect.h = src_rect.h;
                    }
                }
                dst_rect.x -= dc->section.x1;
                dst_rect.y -= dc->section.y1;
                if (image->blend_mode == IMG_BYPASS_MODE || image->blend_mode == IMG_COVER_MODE)
                {
                    if (src_rect.w % 2 == 0 && head->compress)
                    {
                        uint8_t pixel_size = PPE_Get_Pixel_Size(target.format);
                        uint32_t dst_stride = target.stride * pixel_size;
                        uint8_t *dest_address = (uint8_t *)(target.address + (dst_rect.y * target.stride + dst_rect.x) *
                                                            pixel_size);
                        hw_idu_decode(&src_rect, (uint8_t *)source.address, dst_stride, dest_address);
                        return;
                    }
                    else if (!head->compress)
                    {
                        uint32_t s;
                        s = os_lock();
                        bare_blit_by_dma(&target, &source, &src_rect, &dst_rect);
                        os_unlock(s);
                        return;
                    }
                    else
                    {
                        mode = PPE_BYPASS_MODE;
                    }
                }
                else if (image->blend_mode == IMG_RECT)
                {
                    gui_rect_file_head_t *rect_header = (gui_rect_file_head_t *)image->data;
                    gui_color_t color;
                    color.color.argb_full = rect_header->color.color.argb_full;
                    color.color.rgba.a = rect_header->color.color.rgba.a * (image->opacity_value * 1.0f / 255);

                    uint8_t tmp = color.color.rgba.b;
                    color.color.rgba.b = color.color.rgba.r;
                    color.color.rgba.r = tmp;
                    PPE_Mask(&target, color.color.argb_full, &dst_rect);
                    return;
                }
            }
            else
            {
                mode = PPE_SRC_OVER_MODE;
            }
        }
        else if (image->blend_mode == IMG_BYPASS_MODE)
        {
            mode = PPE_BYPASS_MODE;
        }
        else if (image->blend_mode == IMG_RECT)
        {
            mode = PPE_CONST_MASK_MODE;
        }
        else if (image->blend_mode == IMG_FILTER_BLACK)
        {
            mode = PPE_SRC_OVER_MODE;
        }
        else
        {
            if (source.format >= PPE_BGR888 && source.format <= PPE_RGB565 && image->opacity_value == 0xFF)
            {
                mode = PPE_BYPASS_MODE;
            }
            else
            {
                mode = PPE_SRC_OVER_MODE;
            }
        }
    }
    else if (image->blend_mode == IMG_COVER_MODE)
    {
        mode = PPE_BYPASS_MODE;
    }
    else if (image->blend_mode == IMG_SRC_MODE || image->blend_mode == IMG_SRC_OVER_MODE ||
             image->blend_mode == IMG_FILTER_BLACK)
    {
        mode = PPE_SRC_OVER_MODE;
    }

    if (image->blend_mode == IMG_FILTER_BLACK)
    {
        source.color_key_enable = PPE_COLOR_KEY_INSIDE;
        source.color_key_min = 0;
        source.color_key_max = 0x010101;
    }
    else if (image->blend_mode == IMG_BYPASS_MODE)
    {
        source.color_key_enable = PPE_COLOR_KEY_DISABLE;
    }
    else if (image->blend_mode == IMG_SRC_MODE || image->blend_mode == IMG_SRC_OVER_MODE)
    {
        source.color_key_enable = PPE_COLOR_KEY_DISABLE;
    }
    else if (image->blend_mode == IMG_RECT)
    {
        source.color_key_enable = PPE_COLOR_KEY_DISABLE;
    }
    else if (image->blend_mode == IMG_COVER_MODE)
    {
        source.color_key_enable = PPE_COLOR_KEY_DISABLE;
    }
    else
    {
        return;
    }
    ppe_rect_t constraint, old_rect;
    int32_t tessalation_len = PPE_TESS_LENGTH;
    uint32_t block_num = dc->fb_width / tessalation_len;
    if (dc->fb_width % tessalation_len)
    {
        block_num++;
    }
    if (image->blend_mode == IMG_RECT)
    {
        tessalation_len = dc->fb_width;
        block_num = 1;
    }

    if (shape_transform && image->acc_user != NULL)
    {
        ppe_rect_t section_rect = {.x = x_min - dc->section.x1, .y = y_min - dc->section.y1, .w = dc->fb_width, .h = dc->fb_height};
        if (section_rect.x < 0)
        {
            section_rect.x = 0;
        }
        if (section_rect.y < 0)
        {
            section_rect.y = 0;
        }
        if (x_max >= dc->section.x1 + dc->fb_width)
        {
            section_rect.w = dc->section.x1 + dc->fb_width - section_rect.x;
        }
        else
        {
            section_rect.w = x_max - dc->section.x1 - section_rect.x + 1;
        }
        if (y_max >= dc->section.y1 + dc->fb_height)
        {
            section_rect.h = dc->fb_height - section_rect.y;
        }
        else
        {
            section_rect.h = y_max - dc->section.y1 - section_rect.y + 1;
        }
        section_rect.y += dc->section.y1;
        acc_get_intersect_area(image, &constraint, &section_rect, dc);
    }
    else
    {
        constraint = (ppe_rect_t) {.x = x_min - dc->section.x1, .y = y_min - dc->section.y1, .w = dc->fb_width, .h = dc->fb_height};
        if (constraint.x < 0)
        {
            constraint.x = 0;
        }
        if (constraint.y < 0)
        {
            constraint.y = 0;
        }
        if (x_max >= dc->section.x1 + dc->fb_width)
        {
            constraint.w = dc->section.x1 + dc->fb_width - constraint.x;
        }
        else
        {
            constraint.w = x_max - dc->section.x1 - constraint.x + 1;
        }
        if (y_max >= dc->section.y1 + dc->fb_height)
        {
            constraint.h = dc->fb_height - constraint.y;
        }
        else
        {
            constraint.h = y_max - dc->section.y1 - constraint.y + 1;
        }
        constraint.y += dc->section.y1;
    }
    bool ret = false;
    if ((image->matrix.m[2][0] == 0 && image->matrix.m[2][1] == 0))
    {
        ret = ppe_get_area(&old_rect, &constraint, (ppe_matrix_t *)&image->inverse, &source);
        if (ret)
        {
            if (old_rect.w * old_rect.h * PPE_Get_Pixel_Size(source.format) < CACHE_BUF_SIZE)
            {
                block_num = 1;
                tessalation_len = dc->fb_width;
            }
        }
        else
        {
            return;
        }
    }
    for (int i = 0; i < block_num; i++)
    {
        int32_t section_x1 = tessalation_len * i;
        int32_t section_y1 = dc->section.y1;
        if (tessalation_len * (i + 1) > dc->fb_width)
        {
            tessalation_len = dc->fb_width % tessalation_len;
        }
        if ((constraint.x + constraint.w <= section_x1) || (constraint.y + constraint.h <= section_y1)
            || (constraint.x >= section_x1 + tessalation_len) || (constraint.y >= section_y1 + dc->fb_height))
        {
            continue;
        }
        source.win_x_min = 0;
        source.win_x_max = target.width - 1;
        source.win_y_min = 0;
        source.win_y_max = target.height - 1;
        source.address = (uint32_t)image->data + sizeof(struct gui_rgb_data_head);
        source.width = image->img_w;
        source.height = image->img_h;
        source.stride = image->img_w;
        float x_ref = 0, y_ref = 0;
        ppe_rect_t ppe_rect = {0};
        memcpy(&ppe_rect, &constraint, sizeof(ppe_rect_t));
        if (block_num != 1)
        {
            if (ppe_rect.x - section_x1 < 0)
            {
                ppe_rect.x = section_x1;
            }
            if (constraint.x + constraint.w > section_x1 + tessalation_len)
            {
                ppe_rect.w = section_x1 + tessalation_len - ppe_rect.x;
            }
            else
            {
                ppe_rect.w = constraint.x + constraint.w - ppe_rect.x;
            }
        }
        ppe_matrix_t inverse;
        memcpy(&inverse, &image->inverse, sizeof(float) * 9);
        ppe_matrix_t pre_trans;
        if (mode != PPE_CONST_MASK_MODE)
        {
            if (block_num != 1)
            {
                ret = ppe_get_area(&old_rect, &ppe_rect, &inverse, &source);
                if (!ret)
                {
                    if (i == block_num - 1)
                    {
                        return;
                    }
                    else
                    {
                        continue;
                    }
                }
            }
            if (rect != NULL)
            {
                bool res = acc_get_src_rect_area(&old_rect, rect);
                if (!res)
                {
                    if (i == block_num - 1)
                    {
                        return;
                    }
                    else
                    {
                        continue;
                    }
                }
            }
            uint32_t new_cache_size = old_rect.w * old_rect.h * PPE_Get_Pixel_Size(source.format) + 4;
            if (new_cache_size < (CACHE_BUF_SIZE - last_cache_size - 4))
            {
                change_cache_buf(new_cache_size);
                last_cache_size = new_cache_size;
            }
            else if (new_cache_size < CACHE_BUF_SIZE)
            {
                PPE_Finish();
                restore_cache_buf();
                last_cache_size = new_cache_size;
            }
#if F_APP_GUI_USE_PSRAM
            else
            {
                if (head->compress)
                {
                    PPE_Finish();
                    cache_on_psram(new_cache_size);
                    last_cache_size = 0;
                }
                else
                {
                    restore_cache_buf();
                }
            }
#else
            else
            {
                if (head->compress)
                {
                    GUI_ASSERT(new_cache_size < CACHE_BUF_SIZE);
                    return;
                }
            }
#endif

            if (head->compress)
            {
                ret = memcpy_by_idu(&old_rect, &source);
            }
            else
            {
                uint32_t s = os_lock();
                ret = memcpy_by_dma(&old_rect, &source);
                os_unlock(s);
            }
            ppe_rect.y -= section_y1;
            if (ret)
            {
                ppe_get_identity(&pre_trans);
                pre_trans.m[0][2] = old_rect.x * -1.0f;
                pre_trans.m[1][2] = old_rect.y * -1.0f;
                ppe_mat_multiply(&pre_trans, &inverse);
                y_ref = section_y1;
                ppe_translate(0, y_ref, &pre_trans);
                memcpy(&inverse, &pre_trans, sizeof(float) * 9);
                source.width = old_rect.w;
                source.height = old_rect.h;
                source.stride = old_rect.w;
            }
            else
            {
                last_cache_size = 0;
                restore_cache_buf();
                if (head->compress)
                {
                    return;
                }
                if (dc->type == DC_RAMLESS)
                {
                    x_ref = 0;
                    y_ref = section_y1;
                }
                ppe_translate(x_ref, y_ref, &inverse);
            }
        }
        else
        {
            if (rect != NULL)
            {
                ppe_get_identity(&pre_trans);
                pre_trans.m[0][2] = rect->x1 * -1.0f;
                pre_trans.m[1][2] = rect->y1 * -1.0f;
                ppe_mat_multiply(&pre_trans, &inverse);
                source.width = rect->x2 - rect->x1 + 1;
                source.height = rect->y2 - rect->y1 + 1;
            }
            ppe_translate(0, dc->section.y1, &inverse);
            gui_rect_file_head_t *rect_header = (gui_rect_file_head_t *)image->data;
            gui_color_t color = {.color.argb_full = rect_header->color.color.argb_full};

            uint8_t tmp = color.color.rgba.b;
            color.color.rgba.b = color.color.rgba.r;
            color.color.rgba.r = tmp;

            color.color.rgba.a = rect_header->color.color.rgba.a * (image->opacity_value * 1.0f / 255);
            source.const_color = color.color.argb_full;
            ppe_rect.y -= dc->section.y1;
        }
        source.high_quality = image->high_quality;
        if ((image->matrix.m[0][0] == 1 && image->matrix.m[1][1] == 1 && \
             image->matrix.m[2][2] == 1 && image->matrix.m[0][1] == 0 && \
             image->matrix.m[1][0] == 0 && image->matrix.m[2][0] == 0 && \
             image->matrix.m[2][1] == 0))
        {
            source.high_quality = false;
        }
        PPE_Finish();
        PPE_err err = PPE_Blit_Inverse(&target, &source, &inverse, &ppe_rect, mode);
        if (err != PPE_SUCCESS)
        {
            sw_acc_blit(image, dc, rect);
        }
    }
    PPE_Finish();
}

bool hw_acc_idu_decode(uint8_t *image, gui_rect_t *rect, uint8_t *output)
{
    if (image == NULL || rect == NULL || output == NULL)
    {
        GUI_ASSERT(image != NULL && rect != NULL && output != NULL);
        return false;
    }
    struct gui_rgb_data_head output_header;
    struct gui_rgb_data_head *head = (struct gui_rgb_data_head *)image;
    memset(&output_header, 0, sizeof(struct gui_rgb_data_head));
    const IDU_file_header *header = (IDU_file_header *)((uint32_t)image + sizeof(
                                                            struct gui_rgb_data_head));
    uint8_t pixel_size = 0;
    switch (head->type)
    {
    case RGB565:
        output_header.type = RGB565;
        pixel_size = 2;
        break;
    case RGB888:
        output_header.type = RGB888;
        pixel_size = 3;
        break;
    case ARGB8888:
        output_header.type = ARGB8888;
        pixel_size = 4;
        break;
    case ARGB8565:
        output_header.type = ARGB8565;
        pixel_size = 3;
        break;
    default:
        return false;
    }
    RCC_PeriphClockCmd(APBPeriph_IDU, APBPeriph_IDU_CLOCK, ENABLE);
    ppe_rect_t decode_rect = {.x = rect->x1, .y = rect->y1, .w = rect->x2 - rect->x1 + 1, .h = rect->y2 - rect->y1 + 1};
    uint32_t buffer_stride = decode_rect.w * pixel_size;
    if (hw_idu_decode(&decode_rect, (uint8_t *)header, buffer_stride, output + 8))
    {
        memcpy(output, &output_header, sizeof(struct gui_rgb_data_head));
        return true;
    }
    else
    {
        return false;
    }
}

void hw_acc_prepare_cb(draw_img_t *img, gui_rect_t *rect)
{
    if (img->acc_user != NULL)
    {
        gui_free(img->acc_user);
        img->acc_user = NULL;
    }

    if (img->matrix.m[2][2] != 1 || img->matrix.m[0][1] != 0 || \
        img->matrix.m[1][0] != 0 || img->matrix.m[2][0] != 0 || \
        img->matrix.m[2][1] != 0)
    {
        gui_point3f_t pox = {0.0f, 0.0f, 1.0f};
        float point[4][2];
        float *line = gui_malloc(12 * sizeof(float));
        float x1 = 0;
        float y1 = 0;
        float x2 = 0;
        float y2 = 0;

        if (rect == NULL)
        {
            x1 = 0;
            y1 = 0;
            x2 = img->img_w - 1;
            y2 = img->img_h - 1;
        }
        else
        {
            x1 = _UI_MAX(0, rect->x1);
            y1 = _UI_MAX(0, rect->y1);
            x2 = _UI_MIN(img->img_w - 1, rect->x2);
            y2 = _UI_MIN(img->img_h - 1, rect->y2);
        }

        pox.x = x1;
        pox.y = y1;
        pox.z = 1.0f;
        matrix_multiply_point(&img->matrix, &pox);
        point[0][0] = pox.x;
        point[0][1] = pox.y;

        pox.x = x2;
        pox.y = y1;
        pox.z = 1.0f;
        matrix_multiply_point(&img->matrix, &pox);
        point[1][0] = pox.x;
        point[1][1] = pox.y;

        pox.x = x2;
        pox.y = y2;
        pox.z = 1.0f;
        matrix_multiply_point(&img->matrix, &pox);
        point[2][0] = pox.x;
        point[2][1] = pox.y;

        pox.x = x1;
        pox.y = y2;
        pox.z = 1.0f;
        matrix_multiply_point(&img->matrix, &pox);

        point[3][0] = pox.x;
        point[3][1] = pox.y;

        if (point[0][0] == point[1][0])
        {
            line[0] = 1;
            line[1] = 0;
            line[2] = -point[0][0];
        }
        else if (point[0][1] == point[1][1])
        {
            line[0] = 0;
            line[1] = 1;
            line[2] = -point[0][1];
        }
        else
        {
            line[1] = -1;
            line[0] = (point[1][1] - point[0][1]) / (point[1][0] - point[0][0]);
            line[2] = point[1][1] - line[0] * point[1][0];
        }

        if (point[0][0] == point[3][0])
        {
            line[3] = 1;
            line[4] = 0;
            line[5] = -point[0][0];
        }
        else if (point[0][1] == point[3][1])
        {
            line[3] = 0;
            line[4] = 1;
            line[5] = -point[0][1];
        }
        else
        {
            line[4] = -1;
            line[3] = (point[3][1] - point[0][1]) / (point[3][0] - point[0][0]);
            line[5] = point[3][1] - line[3] * point[3][0];
        }

        if (point[2][0] == point[1][0])
        {
            line[6] = 1;
            line[7] = 0;
            line[8] = -point[2][0];
        }
        else if (point[2][1] == point[1][1])
        {
            line[6] = 0;
            line[7] = 1;
            line[8] = -point[2][1];
        }
        else
        {
            line[7] = -1;
            line[6] = (point[1][1] - point[2][1]) / (point[1][0] - point[2][0]);
            line[8] = point[1][1] - line[6] * point[1][0];
        }

        if (point[2][0] == point[3][0])
        {
            line[9] = 1;
            line[10] = 0;
            line[11] = -point[2][0];
        }
        else if (point[2][1] == point[3][1])
        {
            line[9] = 0;
            line[10] = 1;
            line[11] = -point[2][1];
        }
        else
        {
            line[10] = -1;
            line[9] = (point[3][1] - point[2][1]) / (point[3][0] - point[2][0]);
            line[11] = point[3][1] - line[9] * point[3][0];
        }
        img->acc_user = line;
    }
}

void hw_acc_end_cb(draw_img_t *image)
{
    if (image->acc_user != NULL)
    {
        gui_free(image->acc_user);
        image->acc_user = NULL;
    }
    return;
}

void hw_acc_init(void)
{
    PPE_CLK_ENABLE(ENABLE);
    GDMA_channel_request(&high_speed_dma_channel_num, NULL, true);
    GDMA_channel_request(&low_speed_dma_channel_num, NULL, false);

    extern void (* draw_img_acc_prepare_cb)(struct draw_img * image, gui_rect_t *rect);
    extern void (* draw_img_acc_end_cb)(struct draw_img * image);
    draw_img_acc_prepare_cb = hw_acc_prepare_cb;
    draw_img_acc_end_cb = hw_acc_end_cb;
}


