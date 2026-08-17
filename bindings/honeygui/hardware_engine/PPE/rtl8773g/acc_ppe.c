/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <draw_img.h>
#include <stdio.h>
#include <stdint.h>
#include <gui_matrix.h>
#include "gui_post_process.h"
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
#include "section.h"
#include "trace.h"
#include "font_rendering_utils.h"
#include "def_file.h"

#define F_APP_GUI_USE_PSRAM 1
#define PSRAM_GUI_HEAP_ADDR SPIC1_MEM_BASE
void *sync_sem;

static void ppe_wait_finish_blocking(void);

static void *acc_ppe_malloc(size_t n)
{
    return gui_lower_malloc(n);
}

static void acc_ppe_free(void *rmem)
{
    gui_lower_free(rmem);
}

extern void sw_acc_blit(draw_img_t *image, struct gui_dispdev *dc, struct gui_rect *rect);

#define PPE_BARE_COPY_ACCURATE      1
#define PPE_ACC_MIN_OPA             3
#define PPE_TESS_LENGTH             92

#ifndef PPE_RASTER_USE_SW_BLEND
#define PPE_RASTER_USE_SW_BLEND     0
#endif

/* Default blur strength for IMG_A8_BLUR (matches gui_win / gui_view blur). */
#ifndef GUI_A8_BLUR_DEGREE
#define GUI_A8_BLUR_DEGREE 237
#endif

static uint8_t *cache_buf1;
static uint8_t *cache_buf2;
static uint8_t *cache_buf = NULL;
static int32_t last_cache_size = 0;
static uint8_t high_speed_dma_channel_num = 0xFF;
static uint8_t low_speed_dma_channel_num = 0xFF;
static int32_t CACHE_BUF_SIZE = 0;

typedef struct
{
    uint8_t *cache;
    uint32_t cache_size;
    uint32_t cache_offset;
} ppe_scratch_allocator_t;

static uint8_t *ppe_scratch_alloc(ppe_scratch_allocator_t *allocator, uint32_t size,
                                  bool *need_free)
{
    uint32_t aligned_offset = (allocator->cache_offset + 3U) & ~3U;
    if (allocator->cache != NULL && aligned_offset <= allocator->cache_size &&
        size <= allocator->cache_size - aligned_offset)
    {
        uint8_t *buffer = allocator->cache + aligned_offset;
        allocator->cache_offset = aligned_offset + size;
        *need_free = false;
        return buffer;
    }

    *need_free = true;
    uint8_t *buffer = acc_ppe_malloc(size);
    return buffer;
}

static void ppe_scratch_free(uint8_t *buffer, bool need_free)
{
    if (need_free && buffer != NULL)
    {
        acc_ppe_free(buffer);
    }
}

void hw_acc_blur_a8(struct gui_dispdev *dc, gui_rect_t *rect, uint8_t blur_degree, void *cache_mem,
                    const uint8_t *a8_base, int16_t a8_stride,
                    int16_t a8_origin_x, int16_t a8_origin_y, uint32_t recolor);
bool hw_acc_mix_a_rgb(uint8_t *a, uint16_t a_stride, uint8_t *rgb,
                      uint16_t rgb_stride, PPE_PIXEL_FORMAT rgb_format,
                      uint8_t *argb, uint16_t w, uint16_t h);
bool hw_acc_mix_a_rgb_vertical(uint8_t *a, uint8_t *rgb, uint16_t rgb_stride,
                               PPE_PIXEL_FORMAT rgb_format, uint8_t *argb,
                               uint16_t w, uint16_t h);

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
        acc_ppe_free(cache_buf);
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
        acc_ppe_free(cache_buf);
    }
#endif
    cache_buf = cache_buf1;
}

#if F_APP_GUI_USE_PSRAM
static void cache_on_psram(uint32_t size)
{
    if ((uint32_t)cache_buf > PSRAM_GUI_HEAP_ADDR)
    {
        acc_ppe_free(cache_buf);
    }
    cache_buf = acc_ppe_malloc(size);
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
    uint32_t total_size = total_size_in_byte / 4;
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
        else if (dma_depth == 32)
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
        else if (dma_depth == 32)
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
        else if (dma_depth == 32)
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
    GDMA_StructInit(&RX_GDMA_InitStruct);
    RX_GDMA_InitStruct.GDMA_ChannelNum          = high_speed_dma_channel_num;
    RX_GDMA_InitStruct.GDMA_BufferSize          = buffer_size;
    RX_GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToMemory;
    RX_GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    RX_GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Inc;
    RX_GDMA_InitStruct.GDMA_SourceMsize         = m_size;
    RX_GDMA_InitStruct.GDMA_DestinationMsize    = m_size;
    RX_GDMA_InitStruct.GDMA_DestinationDataSize = data_size;
    RX_GDMA_InitStruct.GDMA_SourceDataSize      = data_size;
    RX_GDMA_InitStruct.GDMA_SourceAddr          = start_address;
    RX_GDMA_InitStruct.GDMA_DestinationAddr     = dest_address;

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
                GDMA_LLIStruct[i].CTL_LOW = (BIT(0)
                                             | (RX_GDMA_InitStruct.GDMA_DestinationDataSize << 1)
                                             | (RX_GDMA_InitStruct.GDMA_SourceDataSize << 4)
                                             | (RX_GDMA_InitStruct.GDMA_DestinationInc << 7)
                                             | (RX_GDMA_InitStruct.GDMA_SourceInc << 9)
                                             | (RX_GDMA_InitStruct.GDMA_DestinationMsize << 11)
                                             | (RX_GDMA_InitStruct.GDMA_SourceMsize << 14)
                                             | (RX_GDMA_InitStruct.GDMA_DIR << 20));
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
                GDMA_LLIStruct[i].CTL_LOW = rtl_idu_get_dma_ctl_low_int(dma_channel);
                GDMA_LLIStruct[i].CTL_HIGH = buffer_size;
            }
        }
        SCB_CleanDCache_by_Addr(GDMA_LLIStruct, sizeof(GDMA_LLIDef) * dma_height);
    }
    NVIC_DisableIRQ(DMA_CH_IRQ(high_speed_dma_channel_num));
    NVIC_ClearPendingIRQ(DMA_CH_IRQ(high_speed_dma_channel_num));
    GDMA_ClearAllTypeINT(high_speed_dma_channel_num);
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
    if (gui_get_acc()->enable_thread_sync)
    {
        IDU_State_Record record = {0};
        IDU_Interrupt_Config int_cfg = {0};
        int_cfg.idu_decompress_finish_int = 1;
        int_cfg.idu_decompress_error_int = 1;
        IDU_ERROR err = IDU_Decode_With_Interrupt((uint8_t *)header, &range, &dma_cfg, int_cfg, &record);
        if (err != IDU_SUCCESS)
        {
            return false;
        }
        else
        {
            if (sync_sem && os_sem_take(sync_sem, 10000))
            {
                IDU_Clean_State(&record);
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    else
    {
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
}

static bool memcpy_by_dma(ppe_rect_t *p_rect, ppe_buffer_t *source)
{
    uint8_t pixel_size = PPE_Get_Pixel_Size(source->format) / 8;
    uint16_t width = p_rect->x2 - p_rect->x1 + 1;
    uint16_t height = p_rect->y2 - p_rect->y1 + 1;
    if (width * height * pixel_size > CACHE_BUF_SIZE)
    {
        return false;
    }
    uint32_t length = width * pixel_size;

    uint32_t src_stride = source->stride * pixel_size;
    uint32_t start_address = source->address + (p_rect->x1 + p_rect->y1 * source->stride) * pixel_size;
    hw_dma_copy(length, height, src_stride, length, (uint8_t *)start_address, cache_buf);
    source->address = (uint32_t)cache_buf;
    return true;
}


static bool memcpy_by_idu(ppe_rect_t *p_rect, ppe_buffer_t *source)
{
    uint8_t pixel_size = PPE_Get_Pixel_Size(source->format) / 8;
    uint16_t width = p_rect->x2 - p_rect->x1 + 1;
    uint16_t height = p_rect->y2 - p_rect->y1 + 1;
#if !F_APP_GUI_USE_PSRAM
    if (width * height * pixel_size > CACHE_BUF_SIZE)
    {
        return false;
    }
#endif
    uint32_t dst_stride = width * pixel_size;
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
    uint8_t pixel_size = PPE_Get_Pixel_Size(target->format) / 8;
    uint32_t dst_start_address = target->address + (dst_trans->x1 + dst_trans->y1 * target->stride) *
                                 pixel_size;
    uint32_t src_start_address = source->address + (src_rect->x1 + src_rect->y1 * source->stride) *
                                 pixel_size;
    uint16_t width = src_rect->x2 - src_rect->x1 + 1;
    uint16_t height = src_rect->y2 - src_rect->y1 + 1;
    uint32_t length = width * pixel_size;
    uint32_t src_stride = source->stride * pixel_size;
    uint32_t dst_stride = target->stride * pixel_size;
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
    float left = dst->x1 * 1.0f, right = dst->x2 * 1.0f;
    float top = dst->y1 * 1.0f;
    float bottom = dst->y2 * 1.0f;
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
        new_rect->x1 = floorf(x_min);
    }
    if (x_max < right)
    {
        new_rect->x2 = ceilf(x_max);
    }
    else
    {
        new_rect->x2 = right;
    }
}

bool acc_get_src_rect_area(ppe_rect_t *src_rect, gui_rect_t *rect)
{
    if (src_rect->x1 > rect->x2 || src_rect->y1 > rect->y2 || src_rect->x2 < rect->x1 ||
        src_rect->y2 < rect->y1)
    {
        return false;
    }
    if (src_rect->x1 < rect->x1)
    {
        src_rect->x1 = rect->x1;
    }
    if (src_rect->y1 < rect->y1)
    {
        src_rect->y1 = rect->y1;
    }
    if (src_rect->x2 > rect->x2)
    {
        src_rect->x2 = rect->x2;
    }
    if (src_rect->y2 > rect->y2)
    {
        src_rect->y2 = rect->y2;
    }
    int new_w = src_rect->x2 - src_rect->x1 + 1;
    int new_h = src_rect->y2 - src_rect->y1 + 1;
    if (new_w <= 0 || new_h <= 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

static bool init_target_from_dc(ppe_buffer_t *target, struct gui_dispdev *dc)
{
    memset(target, 0, sizeof(ppe_buffer_t));
    switch (dc->bit_depth)
    {
    case 16:
        target->format = PPE_RGB565;
        break;
    case 24:
        target->format = PPE_RGB888;
        break;
    case 32:
        target->format = PPE_ARGB8888;
        break;
    default:
        return false;
    }
    target->address = (uint32_t)dc->frame_buf;
    target->width = dc->fb_width;
    target->height = dc->fb_height;
    target->stride = dc->fb_width;
    target->const_color = 0xFFFFFFFF;
    target->win_x_min = 0;
    target->win_x_max = target->width - 1;
    target->win_y_min = 0;
    target->win_y_max = target->height - 1;
    return true;
}

static bool init_source_format(ppe_buffer_t *source, draw_img_t *image,
                               struct gui_rgb_data_head *head)
{
    switch (head->type)
    {
    case RGB565:
        source->format = PPE_RGB565;
        break;
    case RGB888:
        source->format = PPE_RGB888;
        break;
    case ARGB8888:
        source->format = PPE_ARGB8888;
        break;
    case ARGB8565:
        source->format = PPE_ARGB8565;
        break;
    case XRGB8888:
        source->format = PPE_XRGB8888;
        break;
    case ALPHAMASK:
    case A8:
        {
            source->format = PPE_A8;
            uint32_t c_a = (image->fg_color_set & 0xFF000000);
            uint32_t c_r = (image->fg_color_set & 0x00FF0000);
            uint32_t c_g = (image->fg_color_set & 0x0000FF00);
            uint32_t c_b = (image->fg_color_set & 0x000000FF);
            uint32_t ppe_abgr = (c_a | (c_r >> 16) | c_g | (c_b << 16));
            source->const_color = ppe_abgr;
        }
        break;
    case A4:
        {
            source->format = PPE_A4;
            uint32_t c_a = (image->fg_color_set & 0xFF000000);
            uint32_t c_r = (image->fg_color_set & 0x00FF0000);
            uint32_t c_g = (image->fg_color_set & 0x0000FF00);
            uint32_t c_b = (image->fg_color_set & 0x000000FF);
            uint32_t ppe_abgr = (c_a | (c_r >> 16) | c_g | (c_b << 16));
            source->const_color = ppe_abgr;
        }
        break;
    default:
        return false;
    }
    return true;
}

static void setup_filter_black_color_key(ppe_buffer_t *source)
{
    source->color_key_config.key_enable.key_enable = true;
    source->color_key_config.key_enable.channel_en.a_en = true;
    source->color_key_config.key_enable.channel_en.b_en = true;
    source->color_key_config.key_enable.channel_en.g_en = true;
    source->color_key_config.key_enable.channel_en.r_en = true;
    source->color_key_config.key_mode = PPE_COLOR_KEY_INSIDE;
    source->color_key_config.key_range.R_max = 0;
    source->color_key_config.key_range.R_min = 0;
    source->color_key_config.key_range.G_max = 0;
    source->color_key_config.key_range.G_min = 0;
    source->color_key_config.key_range.B_max = 0;
    source->color_key_config.key_range.B_min = 0;
    source->color_key_config.key_replace.key_replace = 0x00000000;
}

static void ppe_wait_finish_blocking(void)
{
    if (gui_get_acc()->enable_thread_sync && sync_sem != NULL)
    {
        if (!os_sem_take(sync_sem, 1000))
        {
            PPE_Finish();
        }
    }
    else
    {
        PPE_Finish();
    }
}

static void ppe_wait_finish(void)
{
    if (!gui_get_acc()->enable_async)
    {
        ppe_wait_finish_blocking();
    }
}

void hw_acc_blit_direct(draw_img_t *image, struct gui_dispdev *dc, struct gui_rect *rect)
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
    struct gui_rgb_data_head *head = image->data;
    ppe_buffer_t target, source;
    memset(&source, 0, sizeof(ppe_buffer_t));
    if (!init_target_from_dc(&target, dc)) { return; }
    PPE_BLEND_METHOD method = image->blend_mode == IMG_SRC_MODE ? PPE_BLEND_SRC :
                              PPE_BLEND_PREMULTIPLY;

    source.address = (uint32_t)image->data + sizeof(struct gui_rgb_data_head);
    source.width = image->img_w;
    source.height = image->img_h;
    source.stride = image->img_w;
    if (image->blend_mode == IMG_SRC_MODE)
    {
        source.const_color = image->opacity_value * 0x010101 + 0xFF000000;
        source.opacity = 0xFF;
        if (head->type == ARGB8565 || head->type == ARGB8888 || head->type == XRGB8888)
        {
            method = PPE_BLEND_PREMULTIPLY;
        }
    }
    else
    {
        source.const_color = 0xFFFFFFFF;
        source.opacity = image->opacity_value;
    }
    if (head->type == PALETTE)
    {
        gui_palette_file_t *phead = (gui_palette_file_t *)image->data;
        PPE->CLUT_INDEX = 0;
        for (int i = 0; i < 256; i++)
        {
            PPE->CLUT_CONT = (0xFF000000 | (phead->palette_data[i * 3] << 16) | (phead->palette_data[i * 3 + 1]
                                                                                 <<
                                                                                 8) | phead->palette_data[i * 3 + 2]);
        }
        source.address = (uint32_t)phead->palette_index;
        source.format = PPE_I8;
    }
    else if (head->type == I8)
    {
        PPE->CLUT_INDEX = 0;
        uint32_t *clut = (uint32_t *)((uint32_t)image->data + sizeof(gui_rgb_data_head_t));
        uint32_t clut_num = (*clut++ >> 16) + 1;
        for (int i = 0; i < clut_num; i++)
        {
            PPE->CLUT_CONT = *clut++;
        }
        source.address = (uint32_t)image->data + sizeof(gui_rgb_data_head_t) + (clut_num + 1) * 4;
        source.format = PPE_I8;
    }
    else if (!init_source_format(&source, image, head))
    {
        return;
    }
    source.win_x_min = 0;
    source.win_x_max = target.width - 1;
    source.win_y_min = 0;
    source.win_y_max = target.height - 1;
    if (rect != NULL)
    {
        source.address += (rect->x1 + rect->y1 * source.stride) * PPE_Get_Pixel_Size(
                              source.format) / PPE_BYTE_SIZE;
        source.width = rect->x2 - rect->x1 + 1;
        source.height = rect->y2 - rect->y1 + 1;
        ppe_buffer_t virtual_target;
        memset(&virtual_target, 0, sizeof(ppe_buffer_t));
        virtual_target.width = dc->screen_width;
        virtual_target.height = dc->screen_height;
        ppe_rect_t src_rect = {.x1 = rect->x1, .y1 = rect->y1,
                               .x2 = rect->x2, .y2 = rect->y2
                              };
        ppe_rect_t dst_rect;
        if (ppe_get_area(&dst_rect, &src_rect, (ppe_matrix_t *)&image->matrix, &virtual_target))
        {
            x_min = dst_rect.x1;
            y_min = dst_rect.y1;
            x_max = dst_rect.x2;
            y_max = dst_rect.y2;
        }
    }
    ppe_rect_t constraint = (ppe_rect_t) {.x1 = x_min - dc->section.x1, .y1 = y_min - dc->section.y1, .x2 = dc->fb_width - 1, .y2 = dc->fb_height - 1};
    if (constraint.x1 < 0)
    {
        constraint.x1 = 0;
    }
    if (constraint.y1 < 0)
    {
        constraint.y1 = 0;
    }
    if (x_max >= dc->section.x1 + dc->fb_width)
    {
        constraint.x2 = dc->section.x1 + dc->fb_width - 1;
    }
    else
    {
        constraint.x2 = x_max - dc->section.x1;
    }
    if (y_max >= dc->section.y1 + dc->fb_height)
    {
        constraint.y2 = dc->fb_height - 1;
    }
    else
    {
        constraint.y2 = y_max - dc->section.y1;
    }
    ppe_matrix_t inverse;
    memcpy(&inverse, &image->inverse, sizeof(float) * 9);
    if (rect != NULL)
    {
        ppe_matrix_t pre_trans;
        ppe_get_identity(&pre_trans);
        pre_trans.m[0][2] = rect->x1 * -1.0f;
        pre_trans.m[1][2] = rect->y1 * -1.0f;
        ppe_mat_multiply(&pre_trans, &inverse);
        memcpy(&inverse, &pre_trans, sizeof(float) * 9);
        ppe_translate(0, dc->section.y1, &pre_trans);
    }
    ppe_translate(0, dc->section.y1, &inverse);
    source.high_quality = true;
    if (image->blend_mode == IMG_FILTER_BLACK)
    {
        setup_filter_black_color_key(&source);
    }
    if (!ppe_matrix_is_complex(&inverse))
    {
        source.high_quality = false;
        if ((source.format == PPE_RGB565 || source.format == PPE_RGB888 || head->type == PALETTE) \
            && image->blend_mode != IMG_FILTER_BLACK)
        {
            method = PPE_BLEND_BYPASS;
        }
    }
    PPE_Finish();
    PPE_ERR err = PPE_Blit_Inverse(&target, &source, NULL, &inverse, &constraint,
                                   method);
    ppe_wait_finish();
    return;
}

void hw_acc_blit_rect(draw_img_t *image, struct gui_dispdev *dc, struct gui_rect *rect)
{
    ppe_rect_t draw_rect = {.x1 = image->img_target_x, .y1 = image->img_target_y,
                            .x2 = (image->img_target_w + image->img_target_x - 1),
                            .y2 = (image->img_target_h + image->img_target_y - 1)
                           };
    ppe_rect_t buf_rect = {.x1 = dc->section.x1, .x2 = dc->section.x2,
                           .y1 = dc->section.y1, .y2 = dc->section.y2
                          };
    ppe_rect_t constraint_rect;
    if (!ppe_intersect_area(&constraint_rect, &buf_rect, &draw_rect))
    {
        return;
    }
    if (image->opacity_value <= PPE_ACC_MIN_OPA)
    {
        return;
    }

    ppe_buffer_t target, source;
    memset(&source, 0, sizeof(ppe_buffer_t));
    if (!init_target_from_dc(&target, dc)) { return; }
    PPE_BLEND_METHOD method = PPE_BLEND_SRC;

    source.address = (uint32_t)image->data + sizeof(struct gui_rgb_data_head);
    source.format = PPE_ABGR8888;
    source.opacity = image->opacity_value;
    if (rect != NULL)
    {
        source.width = rect->x2 - rect->x1 + 1;
        source.height = rect->y2 - rect->y1 + 1;
        source.stride = source.width;
    }
    else
    {
        source.width = image->img_w;
        source.height = image->img_h;
        source.stride = image->img_w;
    }
    ppe_matrix_t inverse;
    memcpy(&inverse, &image->inverse, sizeof(ppe_matrix_t));
    if (rect != NULL)
    {
        ppe_matrix_t pre_trans;
        ppe_get_identity(&pre_trans);
        pre_trans.m[0][2] = rect->x1 * -1.0f;
        pre_trans.m[1][2] = rect->y1 * -1.0f;
        ppe_mat_multiply(&pre_trans, &inverse);
    }
    ppe_translate(0, dc->section.y1, &inverse);
    gui_rect_file_head_t *rect_header = (gui_rect_file_head_t *)image->data;
    gui_color_t color = {.color.argb_full = rect_header->color.color.argb_full};

    uint8_t tmp = color.color.rgba.b;
    color.color.rgba.b = color.color.rgba.r;
    color.color.rgba.r = tmp;

    color.color.rgba.a = rect_header->color.color.rgba.a * (image->opacity_value * 1.0f / 255);
    source.const_color = color.color.argb_full;
    source.opacity = 0xFF;
    source.high_quality = true;

    constraint_rect.x1 -= dc->section.x1;
    constraint_rect.y1 -= dc->section.y1;
    constraint_rect.x2 -= dc->section.x1;
    constraint_rect.y2 -= dc->section.y1;

    source.win_x_min = 0;
    source.win_x_max = target.width - 1;
    source.win_y_min = 0;
    source.win_y_max = target.height - 1;
    source.address = (uint32_t)image->data + sizeof(struct gui_rgb_data_head);

    PPE_Finish();
    PPE_ERR err = PPE_Blit_Inverse(&target, &source, NULL, &inverse, &constraint_rect,
                                   PPE_BLEND_CONST_COLOR);
    if (err != PPE_SUCCESS)
    {
        DBG_DIRECT("PPE err %d", err);
    }
    ppe_wait_finish();
}

void hw_acc_blit_simple(draw_img_t *image, struct gui_dispdev *dc, struct gui_rect *rect)
{
    ppe_rect_t draw_rect = {.x1 = image->img_target_x, .y1 = image->img_target_y,
                            .x2 = (image->img_target_w + image->img_target_x - 1),
                            .y2 = (image->img_target_h + image->img_target_y - 1)
                           };
    if (rect != NULL)
    {
        ppe_rect_t scope_rect = {.x1 = rect->x1 + image->img_target_x, .x2 = rect->x2 + image->img_target_x,
                                 .y1 = rect->y1 + image->img_target_y, .y2 = rect->y2 + image->img_target_y
                                };
        if (!ppe_intersect_area(&draw_rect, &draw_rect, &scope_rect))
        {
            return;
        }
    }
    ppe_rect_t buf_rect = {.x1 = dc->section.x1, .x2 = dc->section.x2,
                           .y1 = dc->section.y1, .y2 = dc->section.y2
                          };
    ppe_rect_t constraint_rect;
    if (!ppe_intersect_area(&constraint_rect, &buf_rect, &draw_rect))
    {
        return;
    }
    if (image->opacity_value <= PPE_ACC_MIN_OPA)
    {
        return;
    }

    ppe_buffer_t target, source;
    memset(&source, 0, sizeof(ppe_buffer_t));
    if (!init_target_from_dc(&target, dc)) { return; }
    PPE_BLEND_METHOD method = PPE_BLEND_SRC;

    source.address = (uint32_t)image->data + sizeof(struct gui_rgb_data_head);
    source.opacity = image->opacity_value;
    source.width = image->img_w;
    source.height = image->img_h;
    source.stride = image->img_w;
    source.const_color = 0xFFFFFFFF;
    struct gui_rgb_data_head *head = image->data;
    if (!init_source_format(&source, image, head)) { return; }
    ppe_rect_t source_rect = {.x1 = constraint_rect.x1 - image->img_target_x, .x2 = constraint_rect.x2 - image->img_target_x,
                              .y1 = constraint_rect.y1 - image->img_target_y, .y2 = constraint_rect.y2 - image->img_target_y
                             };
    constraint_rect.x1 -= dc->section.x1;
    constraint_rect.y1 -= dc->section.y1;
    constraint_rect.x2 -= dc->section.x1;
    constraint_rect.y2 -= dc->section.y1;
    if ((source.format == PPE_RGB565 || source.format == PPE_RGB888) && source.opacity == 0xFF
        && source.format == target.format && ((source_rect.x2 - source_rect.x1) % 2 != 0) &&
        image->blend_mode != IMG_FILTER_BLACK)
    {

        ppe_matrix_t inv_matrix;
        if (head->compress)
        {
            uint8_t pixel_size = PPE_Get_Pixel_Size(target.format) / 8;
            uint32_t dst_stride = target.stride * pixel_size;
            uint8_t *dest_address = (uint8_t *)(target.address + (constraint_rect.y1 * target.stride +
                                                                  constraint_rect.x1) *
                                                pixel_size);
            hw_idu_decode(&source_rect, (uint8_t *)source.address, dst_stride, dest_address);
        }
        else
        {
            bare_blit_by_dma(&target, &source, &source_rect, &draw_rect);
        }
        return;
    }
    else
    {
        method = image->blend_mode == IMG_SRC_MODE ? PPE_BLEND_SRC : PPE_BLEND_PREMULTIPLY;
    }
    if (image->blend_mode == IMG_FILTER_BLACK)
    {
        setup_filter_black_color_key(&source);
    }
    else
    {
        source.color_key_config.key_enable.key_enable = false;
    }

    source.win_x_min = 0;
    source.win_x_max = target.width - 1;
    source.win_y_min = 0;
    source.win_y_max = target.height - 1;
    source.address = (uint32_t)image->data + sizeof(struct gui_rgb_data_head);
    source.width = source_rect.x2 - source_rect.x1 + 1;
    source.height = source_rect.y2 - source_rect.y1 + 1;
    source.stride = source_rect.x2 - source_rect.x1 + 1;
    if (source.format == PPE_A4)
    {
        source_rect.x1 = 0;
        source_rect.x2 = image->img_w - 1;
    }
    uint32_t cache_size = (source_rect.x2 - source_rect.x1 + 1) *
                          (source_rect.y2 - source_rect.y1 + 1) *
                          PPE_Get_Pixel_Size(source.format) / 8;
    ppe_scratch_allocator_t scratch =
    {
        .cache = cache_buf1,
        .cache_size = CACHE_BUF_SIZE > 0 ? (uint32_t)CACHE_BUF_SIZE : 0,
        .cache_offset = 0,
    };
    bool need_clean_up = false;
    uint8_t *decode_buf = ppe_scratch_alloc(&scratch, cache_size, &need_clean_up);
    if (decode_buf == NULL)
    {
        return;
    }
    ppe_matrix_t inverse;
    memcpy(&inverse, &image->inverse, sizeof(float) * 9);
    ppe_matrix_t pre_trans;
    bool ret = false;
    float x_ref = dc->section.x1;
    float y_ref = dc->section.y1;
    if (head->compress)
    {
        uint32_t dst_stride =  source.width * PPE_Get_Pixel_Size(source.format) / 8;
        if (source.format == PPE_A4)
        {
            source_rect.x2 = image->img_w / 2 - 1;
        }
        ret = hw_idu_decode(&source_rect, (uint8_t *)source.address, dst_stride, decode_buf);
    }
    if (ret)
    {
        source.address = (uint32_t)decode_buf;
        ppe_get_identity(&pre_trans);
        pre_trans.m[0][2] = source_rect.x1 * -1.0f;
        pre_trans.m[1][2] = source_rect.y1 * -1.0f;
        ppe_mat_multiply(&pre_trans, &inverse);
        ppe_translate(x_ref, y_ref, &pre_trans);
        memcpy(&inverse, &pre_trans, sizeof(float) * 9);
    }
    else
    {
        ppe_scratch_free(decode_buf, need_clean_up);
        return;
    }
    source.high_quality = false;
    PPE_Finish();
    PPE_ERR err = PPE_Blit_Inverse(&target, &source, NULL, &inverse, &constraint_rect, method);
    if (err != PPE_SUCCESS)
    {
        DBG_DIRECT("PPE err %d", err);
    }
    ppe_wait_finish_blocking();
    ppe_scratch_free(decode_buf, need_clean_up);
}

void hw_acc_blit_cache(draw_img_t *image, struct gui_dispdev *dc, struct gui_rect *rect)
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
    struct gui_rgb_data_head *head = image->data;
    ppe_buffer_t target, source;
    memset(&source, 0, sizeof(ppe_buffer_t));
    if (!init_target_from_dc(&target, dc)) { return; }
    PPE_BLEND_METHOD method = PPE_BLEND_SRC;

    source.address = (uint32_t)image->data + sizeof(struct gui_rgb_data_head);
    source.width = image->img_w;
    source.height = image->img_h;
    source.stride = image->img_w;
    if (image->blend_mode == IMG_SRC_MODE)
    {
        source.const_color = image->opacity_value * 0x010101 + 0xFF000000;
        source.opacity = 0xFF;
        if (head->type == ARGB8565 || head->type == ARGB8888 || head->type == XRGB8888)
        {
            method = PPE_BLEND_PREMULTIPLY;
        }
    }
    else
    {
        source.const_color = 0xFFFFFFFF;
        source.opacity = image->opacity_value;
    }

    if (!init_source_format(&source, image, head)) { return; }


    bool shape_transform = true;
    if ((image->matrix.m[2][2] == 1 && image->matrix.m[0][1] == 0 && \
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
                ppe_rect_t dst_rect = {.x1 = x_min - dc->section.x1, .y1 = y_min - dc->section.y1, .x2 = dc->fb_width - 1, .y2 = dc->fb_height - 1};
                if (dst_rect.x1 < 0)
                {
                    dst_rect.x1 = 0;
                }
                if (dst_rect.y1 < 0)
                {
                    dst_rect.y1 = 0;
                }
                if (x_max > dc->section.x2)
                {
                    dst_rect.x2 = dc->fb_width - 1;
                }
                else
                {
                    dst_rect.x2 = x_max;
                }
                if (y_max >= dc->section.y1 + dc->fb_height)
                {
                    dst_rect.y2 = dc->fb_height - 1;
                }
                else
                {
                    dst_rect.y2 = y_max - dc->section.y1;
                }
                dst_rect.x2 += dc->section.x1;
                dst_rect.y2 += dc->section.y1;
                dst_rect.x1 += dc->section.x1;
                dst_rect.y1 += dc->section.y1;
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
                        return;
                    }
                    if (rect != NULL)
                    {
                        int16_t x0 = src_rect.x1;
                        int16_t y0 = src_rect.y1;
                        if (!acc_get_src_rect_area(&src_rect, rect))
                        {
                            return;
                        }
                        dst_rect.x1 += (src_rect.x1 - x0);
                        dst_rect.y1 += (src_rect.y1 - y0);
                        dst_rect.x2 += (src_rect.x1 - x0);
                        dst_rect.y2 += (src_rect.y1 - y0);
                    }
                }
                dst_rect.x1 -= dc->section.x1;
                dst_rect.y1 -= dc->section.y1;
                dst_rect.x2 -= dc->section.x1;
                dst_rect.y2 -= dc->section.y1;
                if (image->blend_mode == IMG_BYPASS_MODE || image->blend_mode == IMG_COVER_MODE)
                {
#if PPE_BARE_COPY_ACCURATE
                    if ((src_rect.x2 - src_rect.x1) % 2 != 0)
#endif
                    {
                        if (head->compress)
                        {
                            uint8_t pixel_size = PPE_Get_Pixel_Size(target.format) / 8;
                            uint32_t dst_stride = target.stride * pixel_size;
                            uint8_t *dest_address = (uint8_t *)(target.address + (dst_rect.y1 * target.stride + dst_rect.x1) *
                                                                pixel_size);
                            PPE_Finish();
                            hw_idu_decode(&src_rect, (uint8_t *)source.address, dst_stride, dest_address);
                        }
                        else
                        {
                            PPE_Finish();
                            bare_blit_by_dma(&target, &source, &src_rect, &dst_rect);
                        }
                        return;
                    }
#if PPE_BARE_COPY_ACCURATE
                    else
                    {
                        method = PPE_BLEND_SRC;
                    }
#endif
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
                    PPE_Finish();
                    PPE_Mask(&target, color.color.argb_full, &dst_rect);
                    ppe_wait_finish();
                    return;
                }
            }
            else
            {
                method = PPE_BLEND_PREMULTIPLY;
            }
        }
        else if (image->blend_mode == IMG_BYPASS_MODE)
        {
            method = PPE_BLEND_SRC;
        }
        else if (image->blend_mode == IMG_RECT)
        {
            method = PPE_BLEND_PREMULTIPLY;
        }
        else if (image->blend_mode == IMG_FILTER_BLACK)
        {
            method = PPE_BLEND_PREMULTIPLY;
        }
        else
        {
            if (source.format >= PPE_BGR888 && source.format <= PPE_RGB565 && image->opacity_value == 0xFF)
            {
                method = PPE_BLEND_SRC;
            }
            else
            {
                method = PPE_BLEND_PREMULTIPLY;
            }
        }
    }
    else if (image->blend_mode == IMG_COVER_MODE)
    {
        method = PPE_BLEND_SRC;
    }
    else if (image->blend_mode == IMG_SRC_OVER_MODE || image->blend_mode == IMG_FILTER_BLACK)
    {
        method = PPE_BLEND_PREMULTIPLY;
    }

    if (image->blend_mode == IMG_SRC_MODE)
    {
        if (head->type == ARGB8565 || head->type == ARGB8888 || head->type == XRGB8888)
        {
            method = PPE_BLEND_PREMULTIPLY;
        }
        else
        {
            method = PPE_BLEND_SRC;
        }
    }
    else if (image->blend_mode == IMG_RECT)
    {
        method = PPE_BLEND_CONST_COLOR;
    }

    if (image->blend_mode == IMG_FILTER_BLACK)
    {
        setup_filter_black_color_key(&source);
    }
    else
    {
        source.color_key_config.key_enable.key_enable = false;
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

    if (shape_transform && image->acc_user != NULL && image->blend_mode != IMG_RECT &&
        dc->type == DC_RAMLESS)
    {
        ppe_rect_t section_rect = {.x1 = x_min - dc->section.x1, .y1 = y_min - dc->section.y1, .x2 = dc->fb_width - 1, .y2 = dc->fb_height - 1};
        if (section_rect.x1 < 0)
        {
            section_rect.x1 = 0;
        }
        if (section_rect.y1 < 0)
        {
            section_rect.y1 = 0;
        }
        if (x_max >= dc->section.x1 + dc->fb_width)
        {
            section_rect.x2 = dc->section.x1 + dc->fb_width - 1;
        }
        else
        {
            section_rect.x2 = x_max - dc->section.x1;
        }
        if (y_max >= dc->section.y1 + dc->fb_height)
        {
            section_rect.y2 = dc->fb_height - 1;
        }
        else
        {
            section_rect.y2 = y_max - dc->section.y1;
        }
        section_rect.y1 += dc->section.y1;
        section_rect.y2 += dc->section.y1;
        acc_get_intersect_area(image, &constraint, &section_rect, dc);
    }
    else
    {
        constraint = (ppe_rect_t) {.x1 = x_min - dc->section.x1, .y1 = y_min - dc->section.y1, .x2 = dc->fb_width - 1, .y2 = dc->fb_height - 1};
        if (constraint.x1 < 0)
        {
            constraint.x1 = 0;
        }
        if (constraint.y1 < 0)
        {
            constraint.y1 = 0;
        }
        if (x_max >= dc->section.x1 + dc->fb_width)
        {
            constraint.x2 = dc->section.x1 + dc->fb_width - 1;
        }
        else
        {
            constraint.x2 = x_max - dc->section.x1;
        }
        if (y_max >= dc->section.y1 + dc->fb_height)
        {
            constraint.y2 = dc->fb_height - 1;
        }
        else
        {
            constraint.y2 = y_max - dc->section.y1;
        }
        constraint.y1 += dc->section.y1;
        constraint.y2 += dc->section.y1;
    }
    bool ret = false;
    if ((image->matrix.m[2][0] == 0 && image->matrix.m[2][1] == 0 && image->blend_mode != IMG_RECT) ||
        dc->type != DC_RAMLESS)
    {
        ret = ppe_get_area(&old_rect, &constraint, (ppe_matrix_t *)&image->inverse, &source);
        if (ret)
        {
            if (CACHE_BUF_SIZE)
            {
                if ((old_rect.x2 - old_rect.x1 + 1) * (old_rect.y2 - old_rect.y1 + 1) \
                    * PPE_Get_Pixel_Size(source.format) / 8 < CACHE_BUF_SIZE)
                {
                    block_num = 1;
                    tessalation_len = dc->fb_width;
                }
            }
            else
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
        if ((constraint.x2 < section_x1) || (constraint.y2 < section_y1)
            || (constraint.x1 >= section_x1 + tessalation_len) || (constraint.y1 >= section_y1 + dc->fb_height))
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
            if (ppe_rect.x1 < section_x1)
            {
                ppe_rect.x1 = section_x1;
            }
            if (constraint.x2 > section_x1 + tessalation_len)
            {
                ppe_rect.x2 = section_x1 + tessalation_len - 1;
            }
            else
            {
                ppe_rect.x2 = constraint.x2;
            }
        }
        ppe_matrix_t inverse;
        memcpy(&inverse, &image->inverse, sizeof(float) * 9);
        ppe_matrix_t pre_trans;
        if (image->blend_mode != IMG_RECT)
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
                if (old_rect.x1 > 0)
                {
                    old_rect.x1 -= 1;
                }
                if (old_rect.y1 > 0)
                {
                    old_rect.y1 -= 1;
                }
                if (old_rect.x2 < image->img_w - 1)
                {
                    old_rect.x2 += 1;
                }
                if (old_rect.y2 < image->img_h - 1)
                {
                    old_rect.y2 += 1;
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
            int new_cache_size = (old_rect.x2 - old_rect.x1 + 1) * (old_rect.y2 - old_rect.y1 + 1)\
                                 * PPE_Get_Pixel_Size(source.format) / 8 + 4;
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
                if (dc->type == DC_RAMLESS)
                {
                    ret = memcpy_by_dma(&old_rect, &source);
                }
                else
                {
                    ret = false;
                }
            }
            ppe_rect.y1 -= section_y1;
            ppe_rect.y2 -= section_y1;
            if (ret)
            {
                ppe_get_identity(&pre_trans);
                pre_trans.m[0][2] = old_rect.x1 * -1.0f;
                pre_trans.m[1][2] = old_rect.y1 * -1.0f;
                ppe_mat_multiply(&pre_trans, &inverse);
                y_ref = section_y1;
                ppe_translate(0, y_ref, &pre_trans);
                memcpy(&inverse, &pre_trans, sizeof(float) * 9);
                source.width = old_rect.x2 - old_rect.x1 + 1;
                source.height = old_rect.y2 - old_rect.y1 + 1;
                source.stride = old_rect.x2 - old_rect.x1 + 1;
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
            source.high_quality = image->high_quality;
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
            source.opacity = 0xFF;
            ppe_rect.y1 -= dc->section.y1;
            ppe_rect.y2 -= dc->section.y1;
        }
        source.high_quality = true;
        if ((image->matrix.m[0][0] == 1 && image->matrix.m[1][1] == 1 && \
             image->matrix.m[2][2] == 1 && image->matrix.m[0][1] == 0 && \
             image->matrix.m[1][0] == 0 && image->matrix.m[2][0] == 0 && \
             image->matrix.m[2][1] == 0))
        {
            source.high_quality = false;
        }
        PPE_Finish();
        PPE_ERR err = PPE_Blit_Inverse(&target, &source, NULL, &inverse, &ppe_rect, method);
        ppe_wait_finish();
    }
}

void *hw_acc_idu_decode(void *input)
{
    if (input == NULL)
    {
        GUI_ASSERT(input != NULL);
        return NULL;
    }
    uint8_t pixel_size = 0;
    uint8_t pixel_div = 1;
    const gui_rgb_data_head_t *head = (gui_rgb_data_head_t *)input;
    uint32_t clut_num = 0;
    uint32_t clut_offset = 0;
    switch (head->type)
    {
    case RGB565:
        pixel_size = 2;
        break;
    case RGB888:
        pixel_size = 3;
        break;
    case ARGB8888:
        pixel_size = 4;
        break;
    case ARGB8565:
        pixel_size = 3;
        break;
    case I8:
        pixel_size = 1;
        clut_num = (*(uint32_t *)((uint32_t)input + sizeof(gui_rgb_data_head_t)) >> 16) + 1;
        clut_offset = clut_num * 4 + sizeof(uint32_t);
        break;
    case A8:
    case A4:
    case A2:
    case A1:
        pixel_size = 1;
        break;
    default:
        return NULL;
    }

    const IDU_File_Header *idu_info = (IDU_File_Header *)((uint32_t)input + sizeof(
                                                              gui_rgb_data_head_t) + clut_offset);
    if (idu_info->raw_pic_width == 0 || idu_info->raw_pic_height == 0)
    {
        return NULL;
    }
    uint32_t image_size = idu_info->raw_pic_width
                          * idu_info->raw_pic_height * pixel_size;

    uint32_t alloc_size = sizeof(gui_rgb_data_head_t) + 4 + idu_info->raw_pic_width
                          * idu_info->raw_pic_height * pixel_size + clut_offset;
    uint8_t *decode_img_data = acc_ppe_malloc(alloc_size);
    if (decode_img_data == NULL)
    {
        return NULL;
    }
    gui_rgb_data_head_t *output_header = (gui_rgb_data_head_t *)decode_img_data;
    memcpy(output_header, head, sizeof(gui_rgb_data_head_t));
    output_header->compress = 0;
    output_header->idu = 1;
    if (clut_num)
    {
        memcpy(decode_img_data + sizeof(gui_rgb_data_head_t),
               (uint8_t *)input + sizeof(gui_rgb_data_head_t), clut_offset);
    }
    RCC_PeriphClockCmd(APBPeriph_IDU, APBPeriph_IDU_CLOCK, ENABLE);
    ppe_rect_t decode_rect = {.x1 = 0, .y1 = 0, .x2 = idu_info->raw_pic_width - 1, .y2 = idu_info->raw_pic_height - 1};
    uint32_t buffer_stride = idu_info->raw_pic_width * pixel_size;
    if (hw_idu_decode(&decode_rect, (uint8_t *)idu_info, buffer_stride,
                      decode_img_data + sizeof(gui_rgb_data_head_t) + clut_offset))
    {
        return decode_img_data;
    }
    else
    {
        acc_ppe_free(decode_img_data);
        return NULL;
    }
}

void hw_acc_idu_free(void *p)
{
    if (p)
    {
        if (((gui_rgb_data_head_t *)p)->idu)
        {
            acc_ppe_free(p);
        }
    }
}

void hw_blit_font(draw_font_t *font, font_glyph_t *glyph)
{
    if (glyph->data == NULL) { return; }
    uint32_t stride = glyph->stride * font->render_mode;
    if (stride % 8 != 0)
    {
        DBG_DIRECT("%s || %d", __FUNCTION__, __LINE__);
        return;
    }
    ppe_buffer_t target, source;
    memset(&target, 0, sizeof(ppe_buffer_t));
    memset(&source, 0, sizeof(ppe_buffer_t));
    PPE_BLEND_METHOD method = PPE_BLEND_PREMULTIPLY;
    switch (font->target_format)
    {
    case RGB565:
        target.format = PPE_RGB565;
        break;
    case RGB888:
        target.format = PPE_RGB888;
        break;
    case ARGB8888:
        target.format = PPE_ARGB8888;
        break;
    case ARGB8565:
        target.format = PPE_ARGB8565;
        break;
    default:
        DBG_DIRECT("%s || %d", __FUNCTION__, __LINE__);
        return;
    }
    uint8_t target_pixel_size = PPE_Get_Pixel_Size(target.format) / PPE_BYTE_SIZE;
    target.address = (uint32_t)font->target_buf;
    target.height = font->target_rect.y2 - font->target_rect.y1 + 1;
    target.width = font->target_rect.x2 - font->target_rect.x1 + 1;
    target.stride = font->target_buf_stride / target_pixel_size;
    target.const_color = 0xFFFFFFFF;

    switch (font->render_mode)
    {
    case 1:
        source.format = PPE_A1;
        break;
    case 2:
        source.format = PPE_A2;
        break;
    case 4:
        source.format = PPE_A4;
        break;
    case 8:
        source.format = PPE_A8;
        break;
    default:
        DBG_DIRECT("%s || %d", __FUNCTION__, __LINE__);
        return;
    }
    source.address = (uint32_t)glyph->data;
    source.opacity = 0xFF;
    source.width = glyph->width;
    source.height = glyph->height;
    source.stride = glyph->stride;
    uint8_t *font_data = (uint8_t *)glyph->data;
    gui_color_t font_color = {.color.rgba.a = font->color.color.rgba.a,
                              .color.rgba.r = font->color.color.rgba.b,
                              .color.rgba.g = font->color.color.rgba.g,
                              .color.rgba.b = font->color.color.rgba.r
                             };
    source.const_color = font_color.color.argb_full;

    ppe_rect_t constraint = {.x1 = font->clip_rect.x1 - font->target_rect.x1,
                             .y1 = font->clip_rect.y1 - font->target_rect.y1,
                             .x2 = font->clip_rect.x2 - font->target_rect.x1,
                             .y2 = font->clip_rect.y2 - font->target_rect.y1
                            };
    source.win_x_min = constraint.x1;
    source.win_x_max = constraint.x2;
    source.win_y_min = constraint.y1;
    source.win_y_max = constraint.y2;
    ppe_matrix_t inverse;
    ppe_get_identity(&inverse);
    inverse.m[0][2] = -glyph->pos_x;
    inverse.m[1][2] = -glyph->pos_y;
    ppe_translate(font->target_rect.x1, font->target_rect.y1, &inverse);
    PPE_Finish();
    PPE_ERR err = PPE_Blit_Inverse(&target, &source, NULL, &inverse, &constraint,
                                   PPE_BLEND_PREMULTIPLY);
    //if(!gui_get_acc()->enable_async)
    {
        PPE_Finish();
    }
    if (err != PPE_SUCCESS)
    {
        DBG_DIRECT("font err %d", err);
    }
}

void hw_acc_memset(uint8_t *addr, gui_color_t color, uint32_t len/*pixel count*/)
{
    ppe_buffer_t target;
    gui_dispdev_t *dc = gui_get_dc();
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
    target.address = (uint32_t)addr;
    target.width = len;
    target.height = 1;
    target.stride = len;
    target.const_color = 0;
    target.win_x_min = 0;
    target.win_x_max = target.width - 1;
    target.win_y_min = 0;
    target.win_y_max = target.height - 1;
    uint32_t abgr_color = color.color.rgba.a << 24 | color.color.rgba.b << 16 | color.color.rgba.g << 8
                          | color.color.rgba.r;
    PPE_Clear(&target, abgr_color, NULL);
    ppe_wait_finish();
}

void hw_acc_prepare_cb(draw_img_t *img, gui_rect_t *rect)
{
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

void hw_acc_blit_a8_blur(draw_img_t *image, struct gui_dispdev *dc, struct gui_rect *rect)
{
    gui_rect_t img_rect =
    {
        .x1 = image->img_target_x,
        .y1 = image->img_target_y,
        .x2 = image->img_target_x + (int16_t)image->img_target_w - 1,
        .y2 = image->img_target_y + (int16_t)image->img_target_h - 1,
    };
    if (image->acc_user == NULL)
    {
        blur_prepare(&img_rect, &image->acc_user);
    }

    uint32_t c_a = (image->fg_color_set & 0xFF000000);
    uint32_t c_r = (image->fg_color_set & 0x00FF0000);
    uint32_t c_g = (image->fg_color_set & 0x0000FF00);
    uint32_t c_b = (image->fg_color_set & 0x000000FF);
    uint32_t ppe_abgr = (c_a | (c_r >> 16) | c_g | (c_b << 16));

    const uint8_t *a8_base = (const uint8_t *)image->data + sizeof(gui_rgb_data_head_t);
    hw_acc_blur_a8(dc, &img_rect, GUI_A8_BLUR_DEGREE, image->acc_user,
                   a8_base, (int16_t)image->img_w,
                   image->img_target_x, image->img_target_y, ppe_abgr);

    /* Free scratch memory after the last section. */
    if (dc->section_count == dc->section_total - 1 && image->acc_user != NULL)
    {
        blur_depose(&image->acc_user);
    }
}

#if !PPE_RASTER_USE_SW_BLEND
static bool hw_acc_blit_raster_horizontal(draw_img_t *image, struct gui_dispdev *dc)
{
    struct gui_rgb_data_head *head = image->data;
    if (head == NULL || image->raster_prog <= 0.0f)
    {
        return true;
    }

    if (image->matrix.m[0][0] != 1.0f || image->matrix.m[1][1] != 1.0f ||
        image->matrix.m[0][1] != 0.0f || image->matrix.m[1][0] != 0.0f ||
        image->matrix.m[2][0] != 0.0f || image->matrix.m[2][1] != 0.0f ||
        image->matrix.m[2][2] != 1.0f)
    {
        return false;
    }

    int32_t x1 = _UI_MAX(image->img_target_x, dc->section.x1);
    int32_t y1 = _UI_MAX(image->img_target_y, dc->section.y1);
    int32_t x2 = _UI_MIN(image->img_target_x + image->img_target_w - 1, dc->section.x2);
    int32_t y2 = _UI_MIN(image->img_target_y + image->img_target_h - 1, dc->section.y2);
    x1 = _UI_MAX(x1, 0);
    y1 = _UI_MAX(y1, 0);
    x2 = _UI_MIN(x2, dc->screen_width - 1);
    y2 = _UI_MIN(y2, dc->screen_height - 1);

    int32_t src_x1 = x1 + (int32_t)image->inverse.m[0][2];
    int32_t src_y1 = y1 + (int32_t)image->inverse.m[1][2];
    int32_t src_x2 = x2 + (int32_t)image->inverse.m[0][2];
    int32_t src_y2 = y2 + (int32_t)image->inverse.m[1][2];
    if (src_x1 < 0)
    {
        x1 -= src_x1;
        src_x1 = 0;
    }
    if (src_y1 < 0)
    {
        y1 -= src_y1;
        src_y1 = 0;
    }
    if (src_x2 >= image->img_w)
    {
        x2 -= src_x2 - image->img_w + 1;
        src_x2 = image->img_w - 1;
    }
    if (src_y2 >= image->img_h)
    {
        y2 -= src_y2 - image->img_h + 1;
        src_y2 = image->img_h - 1;
    }
    if (x1 > x2 || y1 > y2)
    {
        return true;
    }

    if (image->raster_prog < 1.0f)
    {
        int32_t raster_length = (int32_t)(0.25f * dc->screen_width);
        if (raster_length < 1) { raster_length = 1; }
        int32_t raster_center = -raster_length / 2 +
                                (int32_t)(image->raster_prog * (dc->screen_width + raster_length));
        int32_t raster_end = raster_center + raster_length / 2;
        if (x1 > raster_end)
        {
            return true;
        }
        x2 = _UI_MIN(x2, raster_end);
        src_x2 = x2 + (int32_t)image->inverse.m[0][2];
    }

    uint16_t width = x2 - x1 + 1;
    uint16_t height = y2 - y1 + 1;
    ppe_buffer_t source;
    memset(&source, 0, sizeof(source));
    if (!init_source_format(&source, image, head))
    {
        return false;
    }

    uint8_t pixel_bits = PPE_Get_Pixel_Size(source.format);
    if (pixel_bits < 8 || (pixel_bits % 8) != 0)
    {
        return false;
    }
    uint8_t pixel_bytes = pixel_bits / 8;
    uint8_t *source_data = (uint8_t *)image->data + sizeof(gui_rgb_data_head_t);
    uint16_t source_stride = image->img_w;
    ppe_scratch_allocator_t scratch =
    {
        .cache = cache_buf1,
        .cache_size = CACHE_BUF_SIZE > 0 ? (uint32_t)CACHE_BUF_SIZE : 0,
        .cache_offset = 0,
    };
    uint8_t *decode_buf = NULL;
    bool decode_need_free = false;
    if (head->compress)
    {
        IDU_file_header *idu_header = (IDU_file_header *)source_data;
        uint8_t decode_pixel_bytes = IDU_Get_Pixel_Size(idu_header);
        if (decode_pixel_bytes == 0xFF || decode_pixel_bytes != pixel_bytes)
        {
            return true;
        }
        if (src_x1 < 0 || src_y1 < 0 || src_x2 < src_x1 || src_y2 < src_y1 ||
            src_x2 >= (int32_t)idu_header->raw_pic_width ||
            src_y2 >= (int32_t)idu_header->raw_pic_height)
        {
            return true;
        }
        uint32_t decode_size = (uint32_t)width * height * decode_pixel_bytes;
        decode_buf = ppe_scratch_alloc(&scratch, decode_size + 4U, &decode_need_free);
        if (decode_buf == NULL)
        {
            return true;
        }
        ppe_rect_t decode_rect = {.x1 = src_x1, .y1 = src_y1, .x2 = src_x2, .y2 = src_y2};
        if (!hw_idu_decode(&decode_rect, source_data, (uint32_t)width * pixel_bytes, decode_buf))
        {
            ppe_scratch_free(decode_buf, decode_need_free);
            return true;
        }
        source_data = decode_buf;
        source_stride = width;
    }
    else
    {
        source_data += ((uint32_t)src_y1 * image->img_w + src_x1) * pixel_bytes;
    }
    ppe_buffer_t target;
    if (!init_target_from_dc(&target, dc))
    {
        ppe_scratch_free(decode_buf, decode_need_free);
        return true;
    }
    uint32_t target_base_address = target.address;

    uint16_t direct_width = 0;
    if (source.format == target.format)
    {
        if (image->raster_prog >= 1.0f)
        {
            direct_width = width;
        }
        else
        {
            int32_t raster_length = (int32_t)(0.25f * dc->screen_width);
            if (raster_length < 1) { raster_length = 1; }
            int32_t raster_center = -raster_length / 2 +
                                    (int32_t)(image->raster_prog * (dc->screen_width + raster_length));
            int32_t raster_start = raster_center - raster_length / 2;
            if (raster_start >= x1)
            {
                direct_width = _UI_MIN(raster_start, x2) - x1 + 1;
            }
        }
    }

    if (direct_width > 0)
    {
        uint8_t target_bytes = PPE_Get_Pixel_Size(target.format) / 8;
        uint8_t *target_data = (uint8_t *)target_base_address +
                               ((uint32_t)(y1 - dc->section.y1) * target.stride +
                                x1 - dc->section.x1) * target_bytes;
        hw_dma_copy((uint32_t)direct_width * pixel_bytes, height,
                    (uint32_t)source_stride * pixel_bytes,
                    (uint32_t)target.stride * target_bytes,
                    source_data, target_data);

        source_data += (uint32_t)direct_width * pixel_bytes;
        x1 += direct_width;
        width -= direct_width;
        if (width == 0)
        {
            ppe_scratch_free(decode_buf, decode_need_free);
            return true;
        }
    }

    bool alpha_need_free = false;
    bool argb_need_free = false;
    uint8_t *alpha = ppe_scratch_alloc(&scratch, width, &alpha_need_free);
    uint8_t *argb = ppe_scratch_alloc(&scratch, (uint32_t)width * height * 4, &argb_need_free);
    if (alpha == NULL || argb == NULL)
    {
        ppe_scratch_free(argb, argb_need_free);
        ppe_scratch_free(alpha, alpha_need_free);
        ppe_scratch_free(decode_buf, decode_need_free);
        return true;
    }

    if (image->raster_prog >= 1.0f)
    {
        memset(alpha, 0xFF, width);
    }
    else
    {
        int32_t raster_length = (int32_t)(0.25f * dc->screen_width);
        if (raster_length < 1) { raster_length = 1; }
        int32_t raster_center = -raster_length / 2 +
                                (int32_t)(image->raster_prog * (dc->screen_width + raster_length));
        int32_t raster_start = raster_center - raster_length / 2;
        int32_t raster_end = raster_center + raster_length / 2;
        for (uint16_t i = 0; i < width; i++)
        {
            int32_t screen_x = x1 + i;
            if (screen_x <= raster_start)
            {
                alpha[i] = 0xFF;
            }
            else
            {
                alpha[i] = (uint8_t)(0xFF * (raster_end - screen_x) / raster_length);
            }
        }
    }

    bool mixed = hw_acc_mix_a_rgb(alpha, 0, source_data, source_stride, source.format,
                                  argb, width, height);
    if (mixed)
    {
        uint8_t target_bytes = PPE_Get_Pixel_Size(target.format) / 8;
        target.address = target_base_address;
        target.address += ((uint32_t)(y1 - dc->section.y1) * target.stride +
                           x1 - dc->section.x1) * target_bytes;
        target.width = width;
        target.height = height;
        target.win_x_max = width - 1;
        target.win_y_max = height - 1;

        ppe_buffer_t raster_source;
        memset(&raster_source, 0, sizeof(raster_source));
        raster_source.address = (uint32_t)argb;
        raster_source.width = width;
        raster_source.height = height;
        raster_source.stride = width;
        raster_source.format = PPE_ARGB8888;
        raster_source.opacity = 0xFF;
        raster_source.const_color = 0xFFFFFFFF;
        raster_source.win_x_max = width - 1;
        raster_source.win_y_max = height - 1;

        ppe_matrix_t inverse;
        ppe_get_identity(&inverse);
        PPE_ERR err = PPE_Blit_Inverse(&target, &raster_source, NULL, &inverse, NULL,
                                       PPE_BLEND_PREMULTIPLY);
        if (err != PPE_SUCCESS)
        {
            DBG_DIRECT("PPE raster err %d", err);
        }
        ppe_wait_finish_blocking();
    }

    ppe_scratch_free(argb, argb_need_free);
    ppe_scratch_free(alpha, alpha_need_free);
    ppe_scratch_free(decode_buf, decode_need_free);
    return true;
}

static bool hw_acc_blit_raster_vertical(draw_img_t *image, struct gui_dispdev *dc)
{
    struct gui_rgb_data_head *head = image->data;
    if (head == NULL || image->raster_prog <= 0.0f)
    {
        return true;
    }

    if (image->matrix.m[0][0] != 1.0f || image->matrix.m[1][1] != 1.0f ||
        image->matrix.m[0][1] != 0.0f || image->matrix.m[1][0] != 0.0f ||
        image->matrix.m[2][0] != 0.0f || image->matrix.m[2][1] != 0.0f ||
        image->matrix.m[2][2] != 1.0f)
    {
        return false;
    }

    int32_t x1 = _UI_MAX(image->img_target_x, dc->section.x1);
    int32_t y1 = _UI_MAX(image->img_target_y, dc->section.y1);
    int32_t x2 = _UI_MIN(image->img_target_x + image->img_target_w - 1, dc->section.x2);
    int32_t y2 = _UI_MIN(image->img_target_y + image->img_target_h - 1, dc->section.y2);
    x1 = _UI_MAX(x1, 0);
    y1 = _UI_MAX(y1, 0);
    x2 = _UI_MIN(x2, dc->screen_width - 1);
    y2 = _UI_MIN(y2, dc->screen_height - 1);

    int32_t src_x1 = x1 + (int32_t)image->inverse.m[0][2];
    int32_t src_y1 = y1 + (int32_t)image->inverse.m[1][2];
    int32_t src_x2 = x2 + (int32_t)image->inverse.m[0][2];
    int32_t src_y2 = y2 + (int32_t)image->inverse.m[1][2];
    if (src_x1 < 0) { x1 -= src_x1; src_x1 = 0; }
    if (src_y1 < 0) { y1 -= src_y1; src_y1 = 0; }
    if (src_x2 >= image->img_w) { x2 -= src_x2 - image->img_w + 1; src_x2 = image->img_w - 1; }
    if (src_y2 >= image->img_h) { y2 -= src_y2 - image->img_h + 1; src_y2 = image->img_h - 1; }
    if (x1 > x2 || y1 > y2)
    {
        return true;
    }

    int32_t raster_length = (int32_t)(0.25f * dc->screen_height);
    if (raster_length < 1) { raster_length = 1; }
    int32_t raster_center = -raster_length / 2 +
                            (int32_t)(image->raster_prog * (dc->screen_height + raster_length));
    int32_t raster_start = raster_center - raster_length / 2;
    int32_t raster_end = raster_center + raster_length / 2;
    if (image->raster_prog < 1.0f)
    {
        if (y1 > raster_end) { return true; }
        y2 = _UI_MIN(y2, raster_end);
        src_y2 = y2 + (int32_t)image->inverse.m[1][2];
    }

    uint16_t width = x2 - x1 + 1;
    uint16_t height = y2 - y1 + 1;
    ppe_buffer_t source, target;
    memset(&source, 0, sizeof(source));
    if (!init_source_format(&source, image, head) || !init_target_from_dc(&target, dc))
    {
        return false;
    }
    uint8_t pixel_bits = PPE_Get_Pixel_Size(source.format);
    if (pixel_bits < 8 || (pixel_bits % 8) != 0)
    {
        return false;
    }
    uint8_t pixel_bytes = pixel_bits / 8;
    uint8_t target_bytes = PPE_Get_Pixel_Size(target.format) / 8;
    uint8_t *source_data = (uint8_t *)image->data + sizeof(gui_rgb_data_head_t);
    uint16_t source_stride = image->img_w;
    ppe_scratch_allocator_t scratch =
    {
        .cache = cache_buf1,
        .cache_size = CACHE_BUF_SIZE > 0 ? (uint32_t)CACHE_BUF_SIZE : 0,
        .cache_offset = 0,
    };
    uint8_t *decode_buf = NULL;
    bool decode_need_free = false;
    if (head->compress)
    {
        IDU_file_header *idu_header = (IDU_file_header *)source_data;
        uint8_t decode_pixel_bytes = IDU_Get_Pixel_Size(idu_header);
        if (decode_pixel_bytes == 0xFF || decode_pixel_bytes != pixel_bytes)
        {
            return true;
        }
        if (src_x1 < 0 || src_y1 < 0 || src_x2 < src_x1 || src_y2 < src_y1 ||
            src_x2 >= (int32_t)idu_header->raw_pic_width ||
            src_y2 >= (int32_t)idu_header->raw_pic_height)
        {
            return true;
        }
        uint32_t decode_size = (uint32_t)width * height * decode_pixel_bytes;
        decode_buf = ppe_scratch_alloc(&scratch, decode_size + 4U, &decode_need_free);
        if (decode_buf == NULL) { return true; }
        ppe_rect_t decode_rect = {.x1 = src_x1, .y1 = src_y1, .x2 = src_x2, .y2 = src_y2};
        if (!hw_idu_decode(&decode_rect, source_data, (uint32_t)width * pixel_bytes, decode_buf))
        {
            ppe_scratch_free(decode_buf, decode_need_free);
            return true;
        }
        source_data = decode_buf;
        source_stride = width;
    }
    else
    {
        source_data += ((uint32_t)src_y1 * image->img_w + src_x1) * pixel_bytes;
    }
    uint32_t target_base_address = target.address;
    uint16_t direct_height = 0;
    if (source.format == target.format)
    {
        if (image->raster_prog >= 1.0f)
        {
            direct_height = height;
        }
        else if (raster_start >= y1)
        {
            direct_height = _UI_MIN(raster_start, y2) - y1 + 1;
        }
    }
    if (direct_height > 0)
    {
        uint8_t *target_data = (uint8_t *)target_base_address +
                               ((uint32_t)(y1 - dc->section.y1) * target.stride +
                                x1 - dc->section.x1) * target_bytes;
        hw_dma_copy((uint32_t)width * pixel_bytes, direct_height,
                    (uint32_t)source_stride * pixel_bytes,
                    (uint32_t)target.stride * target_bytes,
                    source_data, target_data);
        source_data += (uint32_t)direct_height * source_stride * pixel_bytes;
        y1 += direct_height;
        height -= direct_height;
        if (height == 0)
        {
            ppe_scratch_free(decode_buf, decode_need_free);
            return true;
        }
    }

    bool alpha_need_free = false;
    bool argb_need_free = false;
    uint8_t *alpha = ppe_scratch_alloc(&scratch, height, &alpha_need_free);
    uint8_t *argb = ppe_scratch_alloc(&scratch, (uint32_t)width * height * 4, &argb_need_free);
    if (alpha == NULL || argb == NULL)
    {
        ppe_scratch_free(argb, argb_need_free);
        ppe_scratch_free(alpha, alpha_need_free);
        ppe_scratch_free(decode_buf, decode_need_free);
        return true;
    }
    memset(alpha, 0xFF, height);
    for (uint16_t i = 0; i < height; i++)
    {
        if (image->raster_prog < 1.0f)
        {
            int32_t screen_y = y1 + i;
            if (screen_y > raster_start)
            {
                alpha[i] = (uint8_t)(0xFF * (raster_end - screen_y) / raster_length);
            }
        }
    }

    bool mixed = hw_acc_mix_a_rgb_vertical(alpha, source_data, source_stride, source.format,
                                           argb, width, height);
    if (mixed)
    {
        target.address = target_base_address +
                         ((uint32_t)(y1 - dc->section.y1) * target.stride +
                          x1 - dc->section.x1) * target_bytes;
        target.width = width;
        target.height = height;
        target.win_x_max = width - 1;
        target.win_y_max = height - 1;

        ppe_buffer_t raster_source;
        memset(&raster_source, 0, sizeof(raster_source));
        raster_source.address = (uint32_t)argb;
        raster_source.width = width;
        raster_source.height = height;
        raster_source.stride = width;
        raster_source.format = PPE_ARGB8888;
        raster_source.opacity = 0xFF;
        raster_source.const_color = 0xFFFFFFFF;
        raster_source.win_x_max = width - 1;
        raster_source.win_y_max = height - 1;

        ppe_matrix_t inverse;
        ppe_get_identity(&inverse);
        PPE_ERR err = PPE_Blit_Inverse(&target, &raster_source, NULL, &inverse, NULL,
                                       PPE_BLEND_PREMULTIPLY);
        if (err != PPE_SUCCESS) { DBG_DIRECT("PPE vertical raster err %d", err); }
        ppe_wait_finish_blocking();
    }

    ppe_scratch_free(argb, argb_need_free);
    ppe_scratch_free(alpha, alpha_need_free);
    ppe_scratch_free(decode_buf, decode_need_free);
    return true;
}
#endif

void hw_acc_blit(draw_img_t *image, struct gui_dispdev *dc, struct gui_rect *rect)
{
    if (dc->cache_need_clean)
    {
        SCB_CleanInvalidateDCache();
        dc->cache_need_clean = false;
    }
    struct gui_rgb_data_head *head = image->data;

    if (image->blend_mode == IMG_A8_BLUR && head != NULL &&
        (head->type == A8 || head->type == ALPHAMASK))
    {
        hw_acc_blit_a8_blur(image, dc, rect);
        return;
    }

    if (image->blend_mode == IMG_RASTER_HORIZONTAL ||
        image->blend_mode == IMG_RASTER_VERTICAL)
    {
#if PPE_RASTER_USE_SW_BLEND
        sw_acc_blit(image, dc, rect);
        return;
#else
        bool handled = image->blend_mode == IMG_RASTER_HORIZONTAL ?
                       hw_acc_blit_raster_horizontal(image, dc) :
                       hw_acc_blit_raster_vertical(image, dc);
        if (handled)
        {
            return;
        }
#endif
    }

    if (image->blend_mode != IMG_RECT && image->data == NULL)
    {
        return;
    }
    if (image->blend_mode == IMG_RECT)
    {
        hw_acc_blit_rect(image, dc, rect);
    }
    else if (!head->compress)
    {
        hw_acc_blit_direct(image, dc, rect);
    }
    else if (head->compress && !ppe_matrix_is_complex((ppe_matrix_t *)&image->matrix))
    {
        hw_acc_blit_simple(image, dc, rect);
    }
    else
    {
        hw_acc_blit_cache(image, dc, rect);
    }
}

static bool hw_acc_mix_a_rgb_internal(uint8_t *a, uint16_t a_width, uint16_t a_height,
                                      uint16_t a_stride, uint8_t *rgb, uint16_t rgb_stride,
                                      PPE_PIXEL_FORMAT rgb_format, uint8_t *argb,
                                      uint16_t w, uint16_t h, bool vertical)
{
    ppe_buffer_t mix_a, mix_rgb, output;
    memset(&mix_a, 0, sizeof(ppe_buffer_t));
    memset(&mix_rgb, 0, sizeof(ppe_buffer_t));
    memset(&output, 0, sizeof(ppe_buffer_t));

    mix_a.format = PPE_A8;
    mix_a.address = (uint32_t)a;
    mix_a.opacity = 0xFF;
    mix_a.width = a_width;
    mix_a.height = a_height;
    mix_a.stride = a_stride;
    mix_a.const_color = 0xFF000000;
    mix_a.win_x_min = 0;
    mix_a.win_x_max = w - 1;
    mix_a.win_y_min = 0;
    mix_a.win_y_max = h - 1;
    mix_rgb.format = rgb_format;
    mix_rgb.address = (uint32_t)rgb;
    mix_rgb.opacity = 0xFF;
    mix_rgb.width = w;
    mix_rgb.height = h;
    mix_rgb.stride = rgb_stride;
    mix_rgb.const_color = 0xFFFFFFFF;
    mix_rgb.win_x_min = 0;
    mix_rgb.win_x_max = w - 1;
    mix_rgb.win_y_min = 0;
    mix_rgb.win_y_max = h - 1;

    output.format = PPE_ARGB8888;
    output.address = (uint32_t)argb;
    output.opacity = 0xFF;
    output.width = w;
    output.height = h;
    output.stride = w;
    output.const_color = 0xFFFFFFFF;
    output.win_x_min = 0;
    output.win_x_max = w - 1;
    output.win_y_min = 0;
    output.win_y_max = h - 1;

    PPE_Finish();
    PPE_ERR err = PPE_Mix_Alpha(&mix_a, &mix_rgb, &output, vertical);
    if (err != PPE_SUCCESS)
    {
        DBG_DIRECT("PPE mix alpha err %d", err);
        return false;
    }
    ppe_wait_finish_blocking();
    return true;
}

bool hw_acc_mix_a_rgb(uint8_t *a, uint16_t a_stride, uint8_t *rgb,
                      uint16_t rgb_stride, PPE_PIXEL_FORMAT rgb_format,
                      uint8_t *argb, uint16_t w, uint16_t h)
{
    return hw_acc_mix_a_rgb_internal(a, w, h, a_stride, rgb, rgb_stride, rgb_format,
                                     argb, w, h, false);
}

bool hw_acc_mix_a_rgb_vertical(uint8_t *a, uint8_t *rgb, uint16_t rgb_stride,
                               PPE_PIXEL_FORMAT rgb_format, uint8_t *argb,
                               uint16_t w, uint16_t h)
{
    return hw_acc_mix_a_rgb_internal(a, w, h, 1, rgb, rgb_stride, rgb_format,
                                     argb, w, h, true);
}

void hw_acc_blur_a8(struct gui_dispdev *dc, gui_rect_t *rect, uint8_t blur_degree, void *cache_mem,
                    const uint8_t *a8_base, int16_t a8_stride,
                    int16_t a8_origin_x, int16_t a8_origin_y, uint32_t recolor)
{
    gui_rect_t blur_rect = {0};
    if (!rect_intersect(&blur_rect, rect, &dc->section))
    {
        return;
    }
    int16_t blur_w = blur_rect.x2 - blur_rect.x1 + 1;
    int16_t blur_h = blur_rect.y2 - blur_rect.y1 + 1;
    uint8_t  bpp   = dc->bit_depth / 8;

    uint8_t *blur_copy = (uint8_t *)gui_malloc((uint32_t)blur_w * (uint32_t)blur_h * bpp);
    if (blur_copy == NULL)
    {
        return;
    }

    uint8_t *buffer = dc->frame_buf +
                      ((blur_rect.y1 - dc->section.y1) * dc->fb_width +
                       (blur_rect.x1 - dc->section.x1)) * bpp;
    for (int16_t row = 0; row < blur_h; row++)
    {
        memcpy(blur_copy + (uint32_t)row * blur_w * bpp,
               buffer   + (uint32_t)row * dc->fb_width * bpp,
               (uint32_t)blur_w * bpp);
    }

    int16_t onscreen_top    = rect->y1 < 0 ? 0 : rect->y1;
    int16_t onscreen_bottom = rect->y2 > (int16_t)dc->screen_height - 1
                              ? (int16_t)dc->screen_height - 1 : rect->y2;
    int16_t target_h        = onscreen_bottom - onscreen_top + 1;

    gui_rect_t local_rect = {0, 0, blur_w - 1, target_h - 1};

    gui_dispdev_t fake_dc = {0};
    fake_dc.frame_buf     = blur_copy;
    fake_dc.fb_width      = (uint16_t)blur_w;
    fake_dc.screen_width  = (uint16_t)blur_w;
    fake_dc.screen_height = (uint16_t)target_h;
    fake_dc.bit_depth     = dc->bit_depth;
    fake_dc.section.x1    = 0;
    fake_dc.section.y1    = blur_rect.y1 - onscreen_top;
    fake_dc.section.x2    = blur_w - 1;
    fake_dc.section.y2    = blur_rect.y2 - onscreen_top;

    gui_get_acc()->blur(&fake_dc, &local_rect, blur_degree, cache_mem);

    uint8_t *argb_combine = (uint8_t *)gui_malloc((uint32_t)blur_w * (uint32_t)blur_h * 4 + 8);
    if (argb_combine == NULL)
    {
        gui_free(blur_copy);
        return;
    }
    const uint8_t *a8_row  = a8_base +
                             (blur_rect.y1 - a8_origin_y) * a8_stride +
                             (blur_rect.x1 - a8_origin_x);
    uint8_t       *p_out   = buffer;
    uint8_t       *p_blur  = blur_copy;
    PPE_PIXEL_FORMAT blur_format;
    switch (dc->bit_depth)
    {
    case 16:
        blur_format = PPE_RGB565;
        break;
    case 24:
        blur_format = PPE_RGB888;
        break;
    case 32:
        blur_format = PPE_ARGB8888;
        break;
    default:
        gui_free(argb_combine);
        gui_free(blur_copy);
        return;
    }
    hw_acc_mix_a_rgb((uint8_t *)a8_row, (uint16_t)a8_stride, p_blur, blur_w, blur_format,
                     argb_combine, blur_w, blur_h);

    ppe_matrix_t inverse;
    ppe_get_identity(&inverse);
    ppe_buffer_t target, source;
    memset(&source, 0, sizeof(ppe_buffer_t));
    memset(&target, 0, sizeof(ppe_buffer_t));
    target.width = blur_w;
    target.height = blur_h;
    target.stride = dc->fb_width;
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
        gui_free(argb_combine);
        gui_free(blur_copy);
        return;
    }
    target.opacity = 0xFF;
    target.const_color = 0xFFFFFFFF;
    target.address = (uint32_t)buffer;
    target.win_x_min = 0;
    target.win_x_max = blur_w - 1;
    target.win_y_min = 0;
    target.win_y_max = blur_h - 1;

    source.width = blur_w;
    source.height = blur_h;
    source.stride = blur_w;
    source.format = PPE_ARGB8888;
    source.address = (uint32_t)argb_combine;
    source.opacity = 0xFF;
    source.const_color = 0xFFFFFFFF;
    source.win_x_min = 0;
    source.win_x_max = target.width - 1;
    source.win_y_min = 0;
    source.win_y_max = target.height - 1;

    PPE_Blit_Inverse(&target, &source, NULL, &inverse, NULL, PPE_BLEND_PREMULTIPLY);
    PPE_Finish();
    gui_free(argb_combine);
    gui_free(blur_copy);

    if (recolor & 0xFF000000)
    {
        source.address = (uint32_t)a8_row;
        source.stride = a8_stride;
        source.const_color = recolor;
        source.format = PPE_A8;
        PPE_Blit_Inverse(&target, &source, NULL, &inverse, NULL, PPE_BLEND_PREMULTIPLY);
        PPE_Finish();
    }
}

static void ppe_handler(void)
{
    if (PPE_Get_Interrupt_Status(PPE_ALL_OVER_INT))
    {
        PPE_Mask_All_Interrupt(ENABLE);
        PPE_Clear_Interrupt(PPE_ALL_OVER_INT);
        os_sem_give(sync_sem);
    }
}

static void idu_handler(void)
{
    if (IDU_GetINTStatus(IDU_DECOMPRESS_FINISH_INT))
    {
        IDU_INTConfig(IDU_DECOMPRESS_FINISH_INT, DISABLE);
        IDU_MaskINTConfig(IDU_DECOMPRESS_FINISH_INT, ENABLE);
        IDU_ClearINTPendingBit(IDU_DECOMPRESS_FINISH_INT);
        os_sem_give(sync_sem);
    }
}

void hw_acc_init(void)
{
    cache_buf1 = gui_get_acc()->hw_acc_cache_mem;
    CACHE_BUF_SIZE = gui_get_acc()->hw_acc_cache_size;
    if (cache_buf1)
    {
        GUI_ASSERT(CACHE_BUF_SIZE != 0);
    }
    if (CACHE_BUF_SIZE)
    {
        GUI_ASSERT(cache_buf1 != NULL);
    }
    PPE_CLK_ENABLE(ENABLE);
    RCC_PeriphClockCmd(APBPeriph_IDU, APBPeriph_IDU_CLOCK, ENABLE);
    GDMA_channel_request(&high_speed_dma_channel_num, NULL, true);
    if (CACHE_BUF_SIZE < (5 * 1024))
    {
        GDMA_channel_request(&low_speed_dma_channel_num, NULL, true);
    }
    else
    {
        GDMA_channel_request(&low_speed_dma_channel_num, NULL, false);
    }
    if (gui_get_acc() != NULL && gui_get_acc()->enable_thread_sync)
    {
        os_sem_create(&sync_sem, "sync_sem", 0, 1);
        RamVectorTableUpdate(PPE_VECTORn, ppe_handler);
        PPE_Clear_Interrupt(PPE_ALL_OVER_INT);
        PPE_Mask_All_Interrupt(ENABLE);
        PPE_Mask_Interrupt(PPE_ALL_OVER_INT, DISABLE);
        NVIC_InitTypeDef PPE_NVIC_InitStruct;
        PPE_NVIC_InitStruct.NVIC_IRQChannel = PPE_IRQn;
        PPE_NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
        PPE_NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&PPE_NVIC_InitStruct);

        RamVectorTableUpdate(IDU_VECTORn, idu_handler);
        IDU_MaskINTConfig(IDU_LINE_DECOMPRESS_FINISH_INT, ENABLE);
        IDU_MaskINTConfig(IDU_RX_FIFO_THRESHOLD_INT, ENABLE);
        IDU_MaskINTConfig(IDU_RX_FIFO_OVERFLOW_INT, ENABLE);
        IDU_MaskINTConfig(IDU_TX_FIFO_THRESHOLD_INT, ENABLE);
        IDU_MaskINTConfig(IDU_TX_FIFO_UNDERFLOW_INT, ENABLE);
        IDU_MaskINTConfig(IDU_DECOMPRESS_ERROR_INT, ENABLE);
        IDU_MaskINTConfig(IDU_DECOMPRESS_FINISH_INT, ENABLE);
        IDU_INTConfig(IDU_DECOMPRESS_ERROR_INT, DISABLE);
        IDU_INTConfig(IDU_DECOMPRESS_FINISH_INT, DISABLE);
        IDU_INTConfig(IDU_LINE_DECOMPRESS_FINISH_INT, DISABLE);
        IDU_INTConfig(IDU_RX_FIFO_THRESHOLD_INT, DISABLE);
        IDU_INTConfig(IDU_RX_FIFO_OVERFLOW_INT, DISABLE);
        IDU_INTConfig(IDU_TX_FIFO_THRESHOLD_INT, DISABLE);
        IDU_INTConfig(IDU_TX_FIFO_UNDERFLOW_INT, DISABLE);
        IDU_ClearINTPendingBit(IDU_DECOMPRESS_FINISH_INT);
        IDU_ClearINTPendingBit(IDU_DECOMPRESS_ERROR_INT);
        NVIC_InitTypeDef IDU_NVIC_InitStruct;
        IDU_NVIC_InitStruct.NVIC_IRQChannel = IDU_IRQn;
        IDU_NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
        IDU_NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&IDU_NVIC_InitStruct);
    }
}
