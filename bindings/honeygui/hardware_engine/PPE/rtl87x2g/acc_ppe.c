/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <gui_matrix.h>
#include <rtl_ppe.h>
#include <rtl_idu.h>
#include <rtl_idu_int.h>
#include <rtl_gdma.h>
#include <rtl_rcc.h>
#include "math.h"
#include "gui_api.h"
#include "string.h"
#include "guidef.h"
#include "draw_img.h"

#define USE_BARE_COPY_ACC       1
#define USE_FINE_BARE_COPY      1

#define _UI_MIN(x, y)           (((x)<(y))?(x):(y))
#define _UI_MAX(x, y)           (((x)>(y))?(x):(y))

static uint8_t high_speed_dma_channel_num = 0xFF;
static uint8_t low_speed_dma_channel_num = 0xFF;

extern void sw_acc_blit(draw_img_t *image, struct gui_dispdev *dc, gui_rect_t *rect);

static double acc_ppe_ceil(double _x)
{
#if defined (__GNUC__)
    return (_x + 1.0); //todo, only fix gcc compile issue
#else
    return ceil(_x);
#endif
}
bool hw_idu_decode(gui_rect_t *rect, uint8_t *input, uint32_t buffer_stride, uint8_t *buffer);

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
    uint32_t total_size = (total_size_in_byte % 4) ? (total_size_in_byte / 4 + 1) :
                          (total_size_in_byte / 4);
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
        if (!use_LLI)
        {
            if (buffer_size > 65535)
            {
                use_LLI = true;
                dma_height = buffer_size / 65535;
                if (buffer_size % 65535)
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



bool hw_idu_decode_to_image(draw_img_t *image, gui_rect_t *rect, uint8_t *output)
{
    if (image == NULL || rect == NULL || output == NULL)
    {
        GUI_ASSERT(image != NULL && rect != NULL && output != NULL);
        return false;
    }
    struct gui_rgb_data_head output_header;
    struct gui_rgb_data_head *head = image->data;
    memset(&output_header, 0, sizeof(struct gui_rgb_data_head));
    const IDU_file_header *header = (IDU_file_header *)((uint32_t)image->data + sizeof(
                                                            struct gui_rgb_data_head));
    output_header.type = head->type;
    output_header.w = rect->x2 - rect->x1 + 1;
    output_header.h = rect->y2 - rect->y1 + 1;
    uint8_t pixel_size = 0;
    if (head->type == RGB565)
    {
        pixel_size = 2;
    }
    else if (head->type == ARGB8565 || head->type == RGB888)
    {
        pixel_size = 3;
    }
    else if (head->type == ARGB8888 || head->type == XRGB8888)
    {
        pixel_size = 4;
    }
    uint32_t dst_stride = pixel_size * output_header.w;
    bool ret = hw_idu_decode(rect, (uint8_t *)header, dst_stride, (uint8_t *)(output + 8));

    if (!ret)
    {
        memcpy(output, &output_header, sizeof(gui_rgb_data_head_t));
        return true;
    }
    else
    {
        return false;
    }
}

bool hw_idu_decode(gui_rect_t *rect, uint8_t *input, uint32_t buffer_stride, uint8_t *buffer)
{
    IDU_file_header *header = (IDU_file_header *)input;
    IDU_decode_range range;
    range.start_column = rect->x1;
    range.end_column = rect->x2;
    range.start_line = rect->y1;
    range.end_line = rect->y2;
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


static void hw_ppe_blit(draw_img_t *image, struct gui_dispdev *dc, gui_rect_t *rect)
{
    ppe_rect_t buf_area = {.x1 = dc->section.x1, .x2 = dc->section.x2, \
                           .y1 = dc->section.y1, .y2 = dc->section.y2
                          };
    ppe_rect_t draw_area =
    {
        .x2 = (image->img_target_w + image->img_target_x - 1),
        .y2 = (image->img_target_h + image->img_target_y - 1),
        .x1 = image->img_target_x,
        .y1 = image->img_target_y
    };
    ppe_rect_t constraint_area;
    if (!ppe_rect_intersect(&constraint_area, &buf_area, &draw_area))
    {
        return;
    }

    struct gui_rgb_data_head *head = image->data;

    ppe_buffer_t source, target;
    memset(&source, 0, sizeof(ppe_buffer_t));
    memset(&target, 0, sizeof(ppe_buffer_t));

    if (dc->bit_depth == 16)
    {
        target.format = PPE_RGB565;
    }
    else if (dc->bit_depth == 32)
    {
        target.format = PPE_ARGB8888;
    }
    else if (dc->bit_depth == 24)
    {
        target.format = PPE_RGB888;
    }
    target.memory = (void *)dc->frame_buf;
    target.address = (uint32_t)dc->frame_buf;
    target.width  = dc->fb_width;
    target.stride = dc->fb_width;
    target.height = dc->fb_height;

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
    default:
        return;
    }
    source.width = image->img_w;
    source.stride = image->img_w;
    source.height = image->img_h;
    source.address = (uint32_t)image->data + sizeof(struct gui_rgb_data_head);
    source.memory = (void *)source.address;
    source.global_alpha_en = true;
    source.global_alpha = image->opacity_value;
    PPE_BLEND_MODE mode = PPE_BYPASS_MODE;
    if (image->blend_mode == IMG_FILTER_BLACK)
    {
        source.color_key_en = true;
        source.color_key_value = 0x00000000;
        mode = PPE_SRC_OVER_MODE;
    }
    else if (image->blend_mode == IMG_BYPASS_MODE)
    {
        mode = PPE_BYPASS_MODE;
    }
    else
    {
        if (source.format < PPE_BGR888 || source.format > PPE_RGB565 || \
            (source.global_alpha_en == DISABLE && source.global_alpha != 0xFF))
        {
            mode = PPE_SRC_OVER_MODE;
        }
        else
        {
            mode = PPE_BYPASS_MODE;
        }
    }
    if ((image->matrix.m[0][0] != 1 || image->matrix.m[1][1] != 1) && \
        (image->img_w != image->img_target_w && image->img_h != image->img_target_h))
    {
        float scale_x = image->matrix.m[0][0], scale_y = image->matrix.m[1][1];
        ppe_buffer_t scaled_img;
        ppe_translate_t trans;
        ppe_rect_t range;
        ppe_rect_t scale_rect;
        memset(&scaled_img, 0, sizeof(ppe_buffer_t));
        if (image->blend_mode == IMG_FILTER_BLACK)
        {
            scaled_img.color_key_en = true;
            scaled_img.color_key_value = 0x00000000;
            mode = PPE_SRC_OVER_MODE;
        }
        trans.x = constraint_area.x1 - buf_area.x1;
        trans.y = constraint_area.y1 - buf_area.y1;
        scale_rect.top = (constraint_area.y1 - draw_area.y1) / scale_y;
        scale_rect.bottom = ceil((constraint_area.y2 - draw_area.y1) / scale_y) + ceil(
                                1 / scale_y);
        if (scale_rect.bottom >= image->img_h)
        {
            scale_rect.bottom = image->img_h - 1;
        }
        scale_rect.left = (constraint_area.x1 - draw_area.x1) / scale_x;
        scale_rect.right = ceil((constraint_area.x2 - draw_area.x1) / scale_x) + ceil(
                               1 / scale_x);
        if (scale_rect.right >= image->img_w)
        {
            scale_rect.right = image->img_w - 1;
        }

        if (rect != NULL)
        {
            draw_area.x1 = image->img_target_x + rect->x1 * scale_x;
            draw_area.x2 = image->img_target_x + ceil(rect->x2 * scale_x);
            draw_area.y1 = image->img_target_y + rect->y1 * scale_y;
            draw_area.y2 = image->img_target_y + ceil(rect->y2 * scale_y);
            ppe_rect_t scope_area = {.x1 = rect->x1, .x2 = rect->x2, \
                                     .y1 = rect->y1, .y2 = rect->y2
                                    };
            if (!ppe_rect_intersect(&constraint_area, &buf_area, &draw_area))
            {
                return;
            }
        }
        scaled_img.width = (uint32_t)((scale_rect.right - scale_rect.left + 1) * scale_x);
        scaled_img.height = (uint32_t)((scale_rect.bottom - scale_rect.top + 1) * scale_y);
        scaled_img.stride = scaled_img.width;
        if (scaled_img.height < 1)
        {
            return;
        }
        scaled_img.global_alpha_en = true;
        scaled_img.global_alpha = image->opacity_value;
        if (head->compress)
        {
            source.height = scale_rect.bottom - scale_rect.top + 1;
            source.width = scale_rect.right - scale_rect.left + 1;
            source.stride = source.width;
            const IDU_file_header *header = (IDU_file_header *)((uint32_t)image->data + sizeof(
                                                                    struct gui_rgb_data_head));
            gui_rect_t info;
            info.x1 = scale_rect.left;
            info.x2 = scale_rect.right;
            info.y1 = scale_rect.top;
            info.y2 = scale_rect.bottom;
            uint32_t dst_stride = (info.x2 - info.x1 + 1) * ppe_get_format_data_len(source.format);
            scale_rect.left = 0;
            scale_rect.right = info.x2 - info.x1;
            scale_rect.top = 0;
            scale_rect.bottom = info.y2 - info.y1;
            source.memory = gui_malloc(source.height * source.width *
                                       ppe_get_format_data_len(source.format));
            source.address = (uint32_t)source.memory;
            bool ret = hw_idu_decode(&info, (uint8_t *)header, dst_stride, (uint8_t *)source.memory);
            if (!ret)
            {
                gui_free(source.memory);
                return;
            }
        }
        scaled_img.format = source.format;
        scaled_img.memory = gui_malloc(scaled_img.width * scaled_img.height * ppe_get_format_data_len(
                                           source.format));
        PPE_ERR err = PPE_Scale_Rect(&source, &scaled_img, scale_x, scale_y, &scale_rect);
        if (err == PPE_SUCCESS)
        {
            if (rect != NULL)
            {
                ppe_rect_t final = {.x1 = constraint_area.x1 - dc->section.x1,
                                    .x2 = constraint_area.x2 - dc->section.x1,
                                    .y1 = constraint_area.y1 - dc->section.y1,
                                    .y2 = constraint_area.y2 - dc->section.y1
                                   };
                err = PPE_Blend_Rect(&scaled_img, &target, &trans, &final, mode);
            }
            else
            {
                err = PPE_Blend(&scaled_img, &target, &trans, mode);
            }
        }
        if (head->compress)
        {
            gui_free(source.memory);
        }
        gui_free(scaled_img.memory);
    }
    else
    {
        ppe_translate_t trans = {.x = draw_area.x1 - dc->section.x1, .y = draw_area.y1 - dc->section.y1};
        if (head->compress)
        {
            if (rect != NULL)
            {
                ppe_rect_t scope_area = {.x1 = rect->x1 + image->img_target_x, \
                                         .x2 = rect->x2 + image->img_target_x, \
                                         .y1 = rect->y1 + image->img_target_y, \
                                         .y2 = rect->y2 + image->img_target_y
                                        };
                if (!ppe_rect_intersect(&constraint_area, &constraint_area, &scope_area))
                {
                    return;
                }
            }
            RCC_PeriphClockCmd(APBPeriph_IDU, APBPeriph_IDU_CLOCK, ENABLE);

            const IDU_file_header *header = (IDU_file_header *)((uint32_t)image->data + sizeof(
                                                                    struct gui_rgb_data_head));
            gui_rect_t info;
            info.x1 = constraint_area.x1 - draw_area.x1;
            info.x2 = constraint_area.x2 - draw_area.x1;
            info.y1 = constraint_area.y1 - draw_area.y1;
            info.y2 = constraint_area.y2 - draw_area.y1;
            uint32_t dst_stride = (info.x2 - info.x1 + 1) * ppe_get_format_data_len(source.format);
            source.memory = gui_malloc((info.y2 - info.y1 + 1) * (info.x2 - info.x1 + 1)
                                       * ppe_get_format_data_len(source.format));
            source.address = (uint32_t)source.memory;
            source.width = info.x2 - info.x1 + 1;
            source.height = info.y2 - info.y1 + 1;
            source.stride = source.width;
            trans.x = constraint_area.x1 - dc->section.x1;
            trans.y = constraint_area.y1 - dc->section.y1;
            bool ret = hw_idu_decode(&info, (uint8_t *)header, dst_stride, (uint8_t *)source.memory);
            if (!ret)
            {
                gui_free(source.memory);
                return;
            }
        }
        else
        {
            if (rect != NULL)
            {
                ppe_rect_t scope_area = {.x1 = rect->x1 + image->img_target_x, \
                                         .x2 = rect->x2 + image->img_target_x, \
                                         .y1 = rect->y1 + image->img_target_y, \
                                         .y2 = rect->y2 + image->img_target_y
                                        };
                if (!ppe_rect_intersect(&constraint_area, &constraint_area, &scope_area))
                {
                    return;
                }
                trans.x = constraint_area.x1 - dc->section.x1;
                trans.y = constraint_area.y1 - dc->section.y1;
                source.address = source.address + ((constraint_area.x1 - image->img_target_x) +
                                                   (constraint_area.y1 - image->img_target_y)
                                                   * image->img_w) * ppe_get_format_data_len(source.format);
                source.memory = (uint32_t *)source.address;
                source.width = constraint_area.x2 - constraint_area.x1 + 1;
                source.height = constraint_area.y2 - constraint_area.y1 + 1;
            }
        }
        PPE_ERR err = PPE_Blend(&source, &target, &trans, PPE_SRC_OVER_MODE);
        if (head->compress)
        {
            gui_free(source.memory);
        }
    }
}

#if USE_BARE_COPY_ACC
static void hw_bare_blit(draw_img_t *image, struct gui_dispdev *dc, gui_rect_t *rect)
{
    ppe_rect_t buf_area = {.x1 = dc->section.x1, .x2 = dc->section.x2, \
                           .y1 = dc->section.y1, .y2 = dc->section.y2
                          };
    ppe_rect_t draw_area =
    {
        .x2 = (image->img_target_w + image->img_target_x - 1),
        .y2 = (image->img_target_h + image->img_target_y - 1),
        .x1 = image->img_target_x,
        .y1 = image->img_target_y
    };
    ppe_rect_t constraint_area;
    if (!ppe_rect_intersect(&constraint_area, &buf_area, &draw_area))
    {
        return;
    }
    if (rect != NULL)
    {
        ppe_rect_t scope_area = {.x1 = rect->x1 + image->img_target_x, \
                                 .x2 = rect->x2 + image->img_target_x, \
                                 .y1 = rect->y1 + image->img_target_y, \
                                 .y2 = rect->y2 + image->img_target_y
                                };
        if (!ppe_rect_intersect(&constraint_area, &constraint_area, &scope_area))
        {
            return;
        }
    }

    struct gui_rgb_data_head *head = image->data;

    ppe_buffer_t source, target;
    memset(&source, 0, sizeof(ppe_buffer_t));
    memset(&target, 0, sizeof(ppe_buffer_t));

    if (dc->bit_depth == 16)
    {
        target.format = PPE_RGB565;
    }
    else if (dc->bit_depth == 32)
    {
        target.format = PPE_ARGB8888;
    }
    else if (dc->bit_depth == 24)
    {
        target.format = PPE_RGB888;
    }
    uint8_t pixel_size = ppe_get_format_data_len(target.format);
#if USE_FINE_BARE_COPY
    if (((constraint_area.x2 - constraint_area.x1) * pixel_size % 4) != 0 \
        && head->compress == true && image->matrix.m[0][0] == 1 && image->matrix.m[1][1] == 1)
    {
        hw_ppe_blit(image, dc, rect);
        return;
    }
#endif
    target.memory = (void *)dc->frame_buf;
    target.address = (uint32_t)dc->frame_buf;
    target.width  = dc->fb_width;
    target.height = dc->fb_height;
    target.stride = dc->fb_width;

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
    default:
        return;
    }
    source.width = image->img_w;
    source.height = image->img_h;
    source.stride = image->img_w;
    source.address = (uint32_t)image->data + sizeof(struct gui_rgb_data_head);
    source.memory = (void *)source.address;

    ppe_translate_t trans = {.x = draw_area.x1 - dc->section.x1, .y = draw_area.y1 - dc->section.y1};
    if (head->compress)
    {
        RCC_PeriphClockCmd(APBPeriph_IDU, APBPeriph_IDU_CLOCK, ENABLE);
        const IDU_file_header *header = (IDU_file_header *)((uint32_t)image->data + sizeof(
                                                                struct gui_rgb_data_head));
        gui_rect_t info;
        uint32_t length = (constraint_area.x2 - constraint_area.x1 + 1) * pixel_size;
        if (length % 4)
        {
            if (constraint_area.x2 - draw_area.x1 + 1 < image->img_w && constraint_area.x2 + 1 <= buf_area.x2)
            {
                constraint_area.x2 += 1;
                length += pixel_size;
            }
            else
            {
                constraint_area.x2 -= 1;
                length -= pixel_size;
            }
        }
        if (length == 0)
        {
            return;
        }
        info.x1 = constraint_area.x1 - draw_area.x1;
        info.x2 = constraint_area.x2 - draw_area.x1;
        info.y1 = constraint_area.y1 - draw_area.y1;
        info.y2 = constraint_area.y2 - draw_area.y1;
        uint32_t dst_stride = dc->fb_width * pixel_size;

        trans.x = constraint_area.x1 - dc->section.x1;
        trans.y = constraint_area.y1 - dc->section.y1;
        uint8_t *dest_address = (uint8_t *)(target.address + (trans.y * dc->fb_width + trans.x) *
                                            pixel_size);
        bool ret = hw_idu_decode(&info, (uint8_t *)header, dst_stride, (uint8_t *)dest_address);
        if (!ret)
        {
            sw_acc_blit(image, dc, rect);
            return;
        }
    }
    else
    {
        uint32_t basic_x = constraint_area.x1 - draw_area.x1;
        uint32_t basic_y = constraint_area.y1 - draw_area.y1;
        uint32_t target_x = constraint_area.x1 - dc->section.x1;
        uint32_t target_y = constraint_area.y1 - dc->section.y1;
        uint32_t length = (constraint_area.x2 - constraint_area.x1 + 1) * pixel_size;
        uint32_t src_stride = image->img_w * pixel_size;
        uint32_t dst_stride = dc->fb_width * pixel_size;
        uint32_t height = constraint_area.y2 - constraint_area.y1 + 1;
        uint32_t dst_start_address = target.address + (target_x + target_y * dc->fb_width) * pixel_size;
        uint32_t src_start_address = source.address + (basic_x + basic_y * image->img_w) * pixel_size;
        hw_dma_copy(length, height, src_stride, dst_stride, (uint8_t *)src_start_address,
                    (uint8_t *)dst_start_address);
    }
}
#endif

void hw_ppe_fill_rect(draw_img_t *image, struct gui_dispdev *dc, gui_rect_t *rect)
{
    ppe_rect_t buf_area = {.x1 = dc->section.x1, .x2 = dc->section.x2, \
                           .y1 = dc->section.y1, .y2 = dc->section.y2
                          };
    ppe_rect_t draw_area =
    {
        .x2 = (image->img_target_w + image->img_target_x - 1),
        .y2 = (image->img_target_h + image->img_target_y - 1),
        .x1 = image->img_target_x,
        .y1 = image->img_target_y
    };
    ppe_rect_t constraint_area;
    if (!ppe_rect_intersect(&constraint_area, &buf_area, &draw_area))
    {
        return;
    }
    constraint_area.x1 = constraint_area.x1 - buf_area.x1;
    constraint_area.x2 = constraint_area.x2 - buf_area.x1;
    constraint_area.y1 = constraint_area.y1 - buf_area.y1;
    constraint_area.y2 = constraint_area.y2 - buf_area.y1;
    ppe_buffer_t target;
    memset(&target, 0, sizeof(ppe_buffer_t));

    if (dc->bit_depth == 16)
    {
        target.format = PPE_RGB565;
    }
    else if (dc->bit_depth == 32)
    {
        target.format = PPE_ARGB8888;
    }
    else if (dc->bit_depth == 24)
    {
        target.format = PPE_RGB888;
    }
    target.memory = (void *)dc->frame_buf;
    target.address = (uint32_t)dc->frame_buf;
    target.width  = dc->fb_width;
    target.height = dc->fb_height;
    target.stride = dc->fb_width;
    gui_rect_file_head_t *head = (gui_rect_file_head_t *)image->data;
    uint32_t color = (head->color.color.rgba.a * image->opacity_value / 255 << 24) +
                     (head->color.color.rgba.b << 16)
                     + (head->color.color.rgba.b << 8) + (head->color.color.rgba.r);
    PPE_Clear_Rect(&target, &constraint_area, color);
}

void hw_acc_blit(draw_img_t *image, struct gui_dispdev *dc, gui_rect_t *rect)
{
    if ((image->matrix.m[0][1] != 0 || image->matrix.m[1][0] != 0 || \
         image->matrix.m[2][0] != 0 || image->matrix.m[2][1] != 0) || \
        image->matrix.m[2][2] != 1)
    {
        sw_acc_blit(image, dc, rect);
        return;
    }
    if (image->blend_mode == IMG_RECT)
    {
        hw_ppe_fill_rect(image, dc, rect);
        return;
    }
#if USE_BARE_COPY_ACC
    struct gui_rgb_data_head *head = image->data;
    if (image->blend_mode != IMG_FILTER_BLACK && \
        image->matrix.m[0][0] == 1 && image->matrix.m[1][1] == 1 && \
        ((dc->bit_depth == 16 && head->type == RGB565) || \
         ((dc->bit_depth == 24 && head->type == RGB888) && head->compress == 0) || \
         (dc->bit_depth == 32 && head->type == ARGB8888 && image->blend_mode == PPE_BYPASS_MODE)) && \
        image->opacity_value == 0xFF)
    {
        hw_bare_blit(image, dc, rect);
    }
    else
#endif
    {
        hw_ppe_blit(image, dc, rect);
    }

}

void hw_acc_init(uint8_t high_speed_dma, uint8_t low_speed_dma)
{
    RCC_PeriphClockCmd(APBPeriph_IDU, APBPeriph_IDU_CLOCK, ENABLE);
    RCC_PeriphClockCmd(APBPeriph_PPE, APBPeriph_PPE_CLOCK, ENABLE);

    high_speed_dma_channel_num = high_speed_dma;
    low_speed_dma_channel_num = low_speed_dma;
}
