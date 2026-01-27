/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <rtl876x_rcc.h>
#include "os_mem.h"
#include "draw_img.h"

#include "trace.h"

#include "rtl_hal_jpu.h"


void *hw_jpeg_load(void *input, int len, int *w, int *h, int *channel)
{
    JPU_DEC_PARAM dec_param;
    uint8_t *output = NULL;
    uint32_t dec_size = 0, dec_w = 0, dec_h = 0;

    JPU_ERROR err;

    memset(&dec_param, 0, sizeof(JPU_DEC_PARAM));
    dec_param.data = input;
    dec_param.size = len;
    dec_param.frameFormat = PACKED_FORMAT_422_YUYV;
    dec_param.useWrapper = 1;
    // TODO: need more info from upper layer to determine output RGB type
    dec_param.rgbType = JPU_RGB565;

    // DBG_DIRECT("img 0x%x, sz %d", dec_param.data, dec_param.size);

    err = hal_jpu_decode(&dec_param, &output, &dec_size, &dec_w, &dec_h);
    if (err != JPU_SUCCESS)
    {
        DBG_DIRECT("img 0x%x, sz %d", dec_param.data, dec_param.size);
        DBG_DIRECT("decode jpeg failed, err: %d", err);
        return NULL;
    }

    gui_rgb_data_head_t *pheader = (gui_rgb_data_head_t *)output;
    memset(pheader, 0, sizeof(gui_rgb_data_head_t));
    pheader->type = RGB565;
    pheader->w = dec_w;
    pheader->h = dec_h;
    pheader->jpeg = true;


    *w = pheader->w;
    *h = pheader->h;
    *channel = 3;
    // DBG_DIRECT("size %d, %d", pheader->w, pheader->h);

    return (void *)output;
}

void hw_jpeg_free(void *ptr)
{
    hal_jpu_free_cache(ptr);
}

void hw_jpeg_init(void)
{
    JPU_Clk_Init();
    hal_jpu_mem_init(gui_malloc, gui_free);
    hal_jpu_add_dec_header(sizeof(gui_rgb_data_head_t));
}
