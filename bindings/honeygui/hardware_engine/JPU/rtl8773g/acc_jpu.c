/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <rtl876x_rcc.h>
#include <rtl876x_nvic.h>
#include "os_mem.h"
#include "os_sync.h"
#include "draw_img.h"
#include "vector_table.h"

#include "trace.h"

#include "rtl_hal_jpu.h"

#define JPU_WAIT_FOREVER 2000

static void *jpu_sync_sem;
static void *jpu_mutex;

static bool hw_jpeg_thread_sync_enabled(void)
{
    struct acc_engine *acc = gui_get_acc();
    return acc != NULL && acc->enable_thread_sync &&
           jpu_sync_sem != NULL && jpu_mutex != NULL;
}

static void hw_jpeg_handler(void)
{
    if (!JPU_Wrap_Get_Finish_Int_Status())
    {
        return;
    }

    JPU_Wrap_Clear_Finish_Int();
    NVIC_DisableIRQ(MJPEG_IRQn);
    if (jpu_sync_sem != NULL)
    {
        os_sem_give(jpu_sync_sem);
    }
}

void *hw_jpeg_load(void *input, int len, int *w, int *h, int *channel)
{
    JPU_DEC_PARAM dec_param;
    JPU_DEC_STATE_RECORD record = {0};
    uint8_t *output = NULL;
    uint32_t dec_size = 0, dec_w = 0, dec_h = 0;

    JPU_ERROR err;

    memset(&dec_param, 0, sizeof(JPU_DEC_PARAM));
    dec_param.data = input;
    dec_param.size = len;
    dec_param.frameFormat = PACKED_FORMAT_422_YUYV;
    dec_param.useWrapper = 1;
    dec_param.rgbType = JPU_RGB565;

    // DBG_DIRECT("img 0x%x, sz %d", dec_param.data, dec_param.size);

    bool mutex_locked = false;
    if (jpu_mutex != NULL)
    {
        if (!os_mutex_take(jpu_mutex, 1000))
        {
            DBG_DIRECT("JPEG mutex take failed");
            return NULL;
        }
        mutex_locked = true;
    }

    if (!hw_jpeg_thread_sync_enabled())
    {
        err = hal_jpu_decode(&dec_param, &record, &output, &dec_size, &dec_w, &dec_h);
    }
    else
    {
        err = hal_jpu_decode_start(&dec_param, &record);
        if (err == JPU_SUCCESS)
        {
            NVIC_EnableIRQ(MJPEG_IRQn);
            if (os_sem_take(jpu_sync_sem, JPU_WAIT_FOREVER))
            {
                err = hal_jpu_decode_finish(&record, &output, &dec_size, &dec_w, &dec_h);
            }
            else
            {
                NVIC_DisableIRQ(MJPEG_IRQn);
                err = JPU_ERR_FAILURE;
            }
        }
        else
        {
            NVIC_DisableIRQ(MJPEG_IRQn);
        }
    }

    if (mutex_locked)
    {
        os_mutex_give(jpu_mutex);
    }

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
    NVIC_InitTypeDef nvic_init = {0};

    if (jpu_sync_sem != NULL || jpu_mutex != NULL)
    {
        return;
    }

    JPU_Clk_Init();
    hal_jpu_mem_init(gui_malloc, gui_free);
    hal_jpu_add_dec_header(sizeof(gui_rgb_data_head_t));

    struct acc_engine *acc = gui_get_acc();
    if (acc == NULL || !acc->enable_thread_sync)
    {
        return;
    }

    if (!os_mutex_create(&jpu_mutex))
    {
        jpu_mutex = NULL;
        return;
    }
    if (!os_sem_create(&jpu_sync_sem, "jpu_sync_sem", 0, 1))
    {
        os_mutex_delete(jpu_mutex);
        jpu_mutex = NULL;
        jpu_sync_sem = NULL;
        return;
    }

    RamVectorTableUpdate(MJPEG_VECTORn, hw_jpeg_handler);
    nvic_init.NVIC_IRQChannel = MJPEG_IRQn;
    nvic_init.NVIC_IRQChannelPriority = 3;
    nvic_init.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&nvic_init);
    NVIC_ClearPendingIRQ(MJPEG_IRQn);
}

void hw_jpeg_deinit(void)
{
    NVIC_DisableIRQ(MJPEG_IRQn);
    JPU_Wrap_Clear_Finish_Int();
    NVIC_ClearPendingIRQ(MJPEG_IRQn);

    if (jpu_mutex != NULL)
    {
        os_mutex_delete(jpu_mutex);
        jpu_mutex = NULL;
    }
    if (jpu_sync_sem != NULL)
    {
        os_sem_delete(jpu_sync_sem);
        jpu_sync_sem = NULL;
    }

    JPU_Clk_Deinit();
}

gui_jpeg_ops_t hw_jpeg_ops =
{
    .init = hw_jpeg_init,
    .load = hw_jpeg_load,
    .release = hw_jpeg_free,
    .deinit = hw_jpeg_deinit,
};
