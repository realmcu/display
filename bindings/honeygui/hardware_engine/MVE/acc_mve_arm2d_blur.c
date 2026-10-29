#if defined(__ARM_FEATURE_MVE) && __ARM_FEATURE_MVE
/*
 * This work is based on the Arm-2D Library, Copyright (C) 2024 Arm Limited or its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOCAL_ARM2D_USE_MVE
#endif

#ifdef LOCAL_ARM2D_USE_MVE

#include "arm_mve.h"
#include "guidef.h"
#include "gui_api.h"
#include "gui_post_process.h"
#include "gui_matrix.h"



#ifndef MVE_ARM2D_BLUR_DOWNSCALE_FACTOR
#define MVE_ARM2D_BLUR_DOWNSCALE_FACTOR 2U
#endif

#if MVE_ARM2D_BLUR_DOWNSCALE_FACTOR < 1 || MVE_ARM2D_BLUR_DOWNSCALE_FACTOR > 16
#error "MVE_ARM2D_BLUR_DOWNSCALE_FACTOR must be in the range 1..16"
#endif

#define ROUND_UP_8(x) (((x) + 7) & ~7)

typedef struct
{
    int16_t iX;
    int16_t iY;
} arm2d_local_location_t;

typedef struct
{
    int16_t iWidth;
    int16_t iHeight;
} arm2d_local_size;

typedef struct arm2d_local_region_t
{
    arm2d_local_location_t tLocation;
    arm2d_local_size tSize;
} arm2d_local_region_t;

typedef struct arm2d_local_scratch_mem_t
{
    union
    {
        struct
        {
            uint32_t u24SizeInByte      : 24;                                       //!< the memory size in Byte
uint32_t u2ItemSize         :
            3;                                        //!< the size of the data item
            uint32_t u2Align            : 3;                                        //!< the alignment
uint32_t u2Type             :
            2;                                        //!< The memory type define in enum arm_2d_mem_type_t
        };
        uint32_t Value;                                                             //!< Memory Information
    } tInfo;

    uintptr_t pBuffer;
    uint16_t hwTargetWidth;
    uint16_t hwTargetHeight;
    uint8_t chStatusBytesPerPixel;
    uintptr_t pScaledBuffer;
    uint16_t hwScaledStride;
    uint16_t hwScaledRows;
    uint8_t chBytesPerPixel;
    size_t scaledBufferSize;
    bool bScaledBufferAllocated;
} arm2d_local_scratch_mem_t;

typedef struct arm2d_local_filter_iir_blur_descriptor_t
{
    union
    {
        uint8_t chBlurMode;
        struct
        {
            uint8_t bForwardHorizontal  : 1;
            uint8_t bForwardVertical    : 1;
            uint8_t bReverseHorizontal  : 1;
            uint8_t bReverseVertical    : 1;
        };
    };

    uint8_t chBlurDegree;
    arm2d_local_scratch_mem_t tScratchMemory;

} arm2d_local_filter_iir_blur_descriptor_t;

typedef struct arm2d_color_cccn888_t
{
    uint16_t hwB;
    uint16_t hwG;
    uint16_t hwR;
} arm2d_color_cccn888_t;

typedef arm2d_color_cccn888_t arm2d_color_rgb565_t;

static void *arm2d_local_allocate_scratch_memory(uint32_t wSize,
                                                 uint_fast8_t nAlign)
{
    GUI_UNUSED(nAlign);

    /* ensure nAlign is 2^n */
    GUI_ASSERT((((~nAlign) + 1) & nAlign) == nAlign);

    void *pBuff = gui_malloc(wSize);
    GUI_ASSERT(0 == ((uintptr_t)pBuff & (nAlign - 1)));

    return pBuff;
}

static void arm2d_local_free_scratch_memory(void *pBuff)
{
    gui_free(pBuff);
}


static arm2d_local_scratch_mem_t *arm2d_local_scratch_memory_new(arm2d_local_scratch_mem_t
                                                                 *ptMemory,
                                                                 uint16_t hwItemSize,
                                                                 uint16_t hwItemCount,
                                                                 uint16_t hwAlignment)
{
    size_t tSize = (size_t)hwItemSize * (size_t)hwItemCount;
    do
    {
        if (NULL == ptMemory)
        {
            GUI_ASSERT(false);
            break;
        }
        else if (0 == tSize)
        {
            break;
        }

        tSize = (tSize + 3) & ~3;

        ptMemory->pBuffer
            = (uintptr_t)arm2d_local_allocate_scratch_memory(tSize + 4,
                                                             hwAlignment);

        ptMemory->tInfo.u24SizeInByte = tSize;
        ptMemory->tInfo.u2Align = hwAlignment;
        ptMemory->tInfo.u2ItemSize = hwItemSize;

        if (NULL != (void *)(ptMemory->pBuffer))
        {
            /* add canary */
            *(volatile uint32_t *)((uintptr_t)(ptMemory->pBuffer) + tSize) = 0xCAFE0ACE;
        }

        return ptMemory;
    }
    while (0);

    return NULL;
}

static arm2d_local_scratch_mem_t *arm2d_local_scratch_memory_free(arm2d_local_scratch_mem_t
                                                                  *ptMemory)
{
    do
    {
        if (NULL == ptMemory)
        {
            break;
        }
        if (NULL == (void *)(ptMemory->pBuffer))
        {
            break;
        }

        size_t tSize = ptMemory->tInfo.u24SizeInByte;

        /* check canary */
        if (*(volatile uint32_t *)((uintptr_t)(ptMemory->pBuffer) + tSize) != 0xCAFE0ACE)
        {
            GUI_ASSERT(false);
        }

        arm2d_local_free_scratch_memory((void *)(ptMemory->pBuffer));
        memset(ptMemory, 0, sizeof(arm2d_local_scratch_mem_t));

    }
    while (0);

    return ptMemory;
}

static inline void arm2d_local_rgb565_unpack_single_vec(uint16x8_t in,
                                                        uint16x8_t *R, uint16x8_t *G, uint16x8_t *B)
{
    in = vbrsrq_n_u16(in, 16);

    uint16x8_t vecMaskB = vdupq_n_u16(0xF800);
    uint16x8_t vecMaskG = vdupq_n_u16(0x003F << 5);

    uint16x8_t tB = (uint16x8_t)vshrq((int8x16_t)(in & vecMaskB), 3);
    uint16x8_t tR = (uint16x8_t)vshrq((int8x16_t)(in << 11), 3);
    uint16x8_t tG = (uint16x8_t)vshrq((int8x16_t)((in & vecMaskG) << 5), 2);

    *B = vbrsrq_n_u16(tB, 16);
    *R = vbrsrq_n_u16(tR, 16);
    *G = vbrsrq_n_u16(tG, 16);
}

static inline uint16x8_t arm2d_local_rgb565_pack_single_vec(uint16x8_t R, uint16x8_t G,
                                                            uint16x8_t B)
{
    uint16x8_t      vecMaskRpck = vdupq_n_u16(0x00f8);
    uint16x8_t      vecMaskGpck = vdupq_n_u16(0x00fc);

    uint16x8_t vOut = vorrq(vshrq(B, 3),
                            vmulq_n_u16(vandq(G, vecMaskGpck), 8));

    vOut = vorrq(vOut,
                 vmulq_n_u16(vandq(R, vecMaskRpck), 256));

    return vOut;
}

static inline bool arm2d_local_reverse_h_allowed(
    const arm2d_local_region_t *ptValid,
    const arm2d_local_region_t *ptTarget)
{
    return (ptValid->tLocation.iX <= ptTarget->tLocation.iX)
           && ((ptValid->tLocation.iX + ptValid->tSize.iWidth)
               >= (ptTarget->tLocation.iX + ptTarget->tSize.iWidth));
}

static inline bool arm2d_local_reverse_v_allowed(
    const arm2d_local_region_t *ptValid,
    const arm2d_local_region_t *ptTarget)
{
    return (ptValid->tLocation.iY <= ptTarget->tLocation.iY)
           && ((ptValid->tLocation.iY + ptValid->tSize.iHeight)
               >= (ptTarget->tLocation.iY + ptTarget->tSize.iHeight));
}

void arm2d_local_rgb565_filter_iir_blur_mve(
    uint16_t *__restrict phwTarget,
    int16_t iTargetStride,
    arm2d_local_region_t *__restrict ptValidRegionOnVirtualScreen,
    arm2d_local_region_t *ptTargetRegionOnVirtualScreen,
    uint8_t chBlurDegree,
    arm2d_local_filter_iir_blur_descriptor_t *ptThis)
{
    arm2d_local_scratch_mem_t *ptScratchMemory = &ptThis->tScratchMemory;
    int_fast16_t    iWidth = ptValidRegionOnVirtualScreen->tSize.iWidth;
    int_fast16_t    iHeight = ptValidRegionOnVirtualScreen->tSize.iHeight;

    if (0 == chBlurDegree)
    {
        return ;
    }

    int32_t         iY, iX;
    /* pre-scaled ratio to take into account doubling + high-part extraction of vqdmulhq */
    int16_t        hwRatio = (256 - chBlurDegree) << 7;

    arm2d_color_rgb565_t *ptStatusH = NULL;
    arm2d_color_rgb565_t *ptStatusV = NULL;

    int16_t       *pAccBase = NULL;
    int16x8_t      vaccR, vaccG, vaccB;

    if (NULL != (void *)(ptScratchMemory->pBuffer))
    {
        ptStatusH = (arm2d_color_rgb565_t *) ptScratchMemory->pBuffer;
        ptStatusV = ptStatusH + ROUND_UP_8(ptTargetRegionOnVirtualScreen->tSize.iWidth);
    }

    /* calculate the offset between the target region and the valid region */
    arm2d_local_location_t tOffset =
    {
        .iX = ptValidRegionOnVirtualScreen->tLocation.iX - ptTargetRegionOnVirtualScreen->tLocation.iX,
        .iY = ptValidRegionOnVirtualScreen->tLocation.iY - ptTargetRegionOnVirtualScreen->tLocation.iY,
    };

    const bool bAllowReverseH = arm2d_local_reverse_h_allowed(
                                    ptValidRegionOnVirtualScreen,
                                    ptTargetRegionOnVirtualScreen);
    const bool bAllowReverseV = arm2d_local_reverse_v_allowed(
                                    ptValidRegionOnVirtualScreen,
                                    ptTargetRegionOnVirtualScreen);

    /* left to right, top to down process */
    if (ptThis->bForwardHorizontal)
    {
        uint16_t *phwPixel = phwTarget;
        if (NULL != ptStatusV)
        {
            /* rows direct path */
            ptStatusV += tOffset.iY;
        }

        uint16x8_t      vstride = vidupq_n_u16(0, 1);
        vstride = vstride * iTargetStride;

        for (iY = 0; iY < iHeight / 8; iY++)
        {
            uint16x8_t      voffs = vstride;

            if (NULL != ptStatusV && tOffset.iX > 0)
            {
                /* recover the previous accumulators */
                pAccBase = (int16_t *) ptStatusV;
                vaccR = vld1q(pAccBase);
                vaccG = vld1q(pAccBase + 8);
                vaccB = vld1q(pAccBase + 16);
            }
            else
            {
                uint16x8_t       vacc = vldrhq_gather_shifted_offset_u16(phwPixel, voffs);
                arm2d_local_rgb565_unpack_single_vec(vacc, (uint16x8_t *)&vaccR, (uint16x8_t *)&vaccG,
                                                     (uint16x8_t *)&vaccB);
            }

            int32_t iXStart = (NULL != ptStatusV && tOffset.iX > 0) ? 0 : 1;
            voffs += (uint16_t)iXStart;
            for (iX = iXStart; iX < iWidth; iX++)
            {
                uint16x8_t      in = vldrhq_gather_shifted_offset_u16(phwPixel, voffs);
                uint16x8_t      vR, vG, vB;

                arm2d_local_rgb565_unpack_single_vec(in, &vR, &vG, &vB);

                int16x8_t       vdiffR = vsubq_s16((int16x8_t)vR, vaccR);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)vG, vaccG);
                int16x8_t       vdiffB = vsubq_s16((int16x8_t)vB, vaccB);

                vaccR += vqdmulhq(vdiffR, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccB += vqdmulhq(vdiffB, hwRatio);

                vstrhq_scatter_shifted_offset_u16(phwPixel, voffs,
                                                  arm2d_local_rgb565_pack_single_vec((uint16x8_t)vaccR, (uint16x8_t)vaccG,
                                                                                     (uint16x8_t)vaccB));
                voffs += 1;
            }

            if (NULL != ptStatusV)
            {
                /* save the last pixels */
                pAccBase = (int16_t *) ptStatusV;
                vst1q(pAccBase, vaccR);
                vst1q(pAccBase + 8, vaccG);
                vst1q(pAccBase + 16, vaccB);
                ptStatusV += 8;
            }

            phwPixel += (iTargetStride * 8);
        }

        if (iHeight & 7)
        {
            uint16x8_t      voffs = vstride;
            mve_pred16_t    tailPred = vctp16q(iHeight & 7);

            if (NULL != ptStatusV && tOffset.iX > 0)
            {
                /* recover the previous values */
                pAccBase = (int16_t *) ptStatusV;
                vaccR = vld1q(pAccBase);
                vaccG = vld1q(pAccBase + 8);
                vaccB = vld1q(pAccBase + 16);

            }
            else
            {
                uint16x8_t       vacc = vldrhq_gather_shifted_offset_u16(phwPixel, voffs);

                arm2d_local_rgb565_unpack_single_vec(vacc, (uint16x8_t *)&vaccR, (uint16x8_t *)&vaccG,
                                                     (uint16x8_t *)&vaccB);
            }


            int32_t iXStart = (NULL != ptStatusV && tOffset.iX > 0) ? 0 : 1;
            voffs += (uint16_t)iXStart;
            for (iX = iXStart; iX < iWidth; iX++)
            {
                uint16x8_t      in = vldrhq_gather_shifted_offset_u16(phwPixel, voffs);
                uint16x8_t      vR, vG, vB;

                arm2d_local_rgb565_unpack_single_vec(in, &vR, &vG, &vB);

                int16x8_t       vdiffR = vsubq_s16((int16x8_t)vR, vaccR);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)vG, vaccG);
                int16x8_t       vdiffB = vsubq_s16((int16x8_t)vB, vaccB);

                vaccR += vqdmulhq(vdiffR, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccB += vqdmulhq(vdiffB, hwRatio);

                vstrhq_scatter_shifted_offset_p_u16(phwPixel, voffs,
                                                    arm2d_local_rgb565_pack_single_vec((uint16x8_t)vaccR,
                                                                                       (uint16x8_t)vaccG, (uint16x8_t)vaccB), tailPred);
                voffs += 1;

            }
            if (NULL != ptStatusV)
            {
                /* save the last pixels */
                pAccBase = (int16_t *) ptStatusV;
                vst1q(pAccBase, vaccR);
                vst1q(pAccBase + 8, vaccG);
                vst1q(pAccBase + 16, vaccB);
            }
        }
    }

    /* rows reverse path (right to left) - seeds fresh from the right edge */
    if (ptThis->bReverseHorizontal && bAllowReverseH)
    {
        uint16_t *phwPixel = phwTarget;

        uint16x8_t      vstride = vidupq_n_u16(0, 1);
        vstride = vstride * iTargetStride;

        for (iY = 0; iY < iHeight / 8; iY++)
        {
            uint16x8_t      voffs = vstride + (uint16_t)(iWidth - 1);

            uint16x8_t      vacc = vldrhq_gather_shifted_offset_u16(phwPixel, voffs);
            arm2d_local_rgb565_unpack_single_vec(vacc, (uint16x8_t *)&vaccR, (uint16x8_t *)&vaccG,
                                                 (uint16x8_t *)&vaccB);

            voffs -= 1;
            for (iX = 1; iX < iWidth; iX++)
            {
                uint16x8_t      in = vldrhq_gather_shifted_offset_u16(phwPixel, voffs);
                uint16x8_t      vR, vG, vB;

                arm2d_local_rgb565_unpack_single_vec(in, &vR, &vG, &vB);

                int16x8_t       vdiffR = vsubq_s16((int16x8_t)vR, vaccR);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)vG, vaccG);
                int16x8_t       vdiffB = vsubq_s16((int16x8_t)vB, vaccB);

                vaccR += vqdmulhq(vdiffR, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccB += vqdmulhq(vdiffB, hwRatio);

                vstrhq_scatter_shifted_offset_u16(phwPixel, voffs,
                                                  arm2d_local_rgb565_pack_single_vec((uint16x8_t)vaccR, (uint16x8_t)vaccG,
                                                                                     (uint16x8_t)vaccB));
                voffs -= 1;
            }

            phwPixel += (iTargetStride * 8);
        }

        if (iHeight & 7)
        {
            mve_pred16_t    tailPred = vctp16q(iHeight & 7);
            uint16x8_t      voffs = vstride + (uint16_t)(iWidth - 1);

            uint16x8_t      vacc = vldrhq_gather_shifted_offset_u16(phwPixel, voffs);
            arm2d_local_rgb565_unpack_single_vec(vacc, (uint16x8_t *)&vaccR, (uint16x8_t *)&vaccG,
                                                 (uint16x8_t *)&vaccB);

            voffs -= 1;
            for (iX = 1; iX < iWidth; iX++)
            {
                uint16x8_t      in = vldrhq_gather_shifted_offset_u16(phwPixel, voffs);
                uint16x8_t      vR, vG, vB;

                arm2d_local_rgb565_unpack_single_vec(in, &vR, &vG, &vB);

                int16x8_t       vdiffR = vsubq_s16((int16x8_t)vR, vaccR);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)vG, vaccG);
                int16x8_t       vdiffB = vsubq_s16((int16x8_t)vB, vaccB);

                vaccR += vqdmulhq(vdiffR, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccB += vqdmulhq(vdiffB, hwRatio);

                vstrhq_scatter_shifted_offset_p_u16(phwPixel, voffs,
                                                    arm2d_local_rgb565_pack_single_vec((uint16x8_t)vaccR,
                                                                                       (uint16x8_t)vaccG, (uint16x8_t)vaccB), tailPred);
                voffs -= 1;
            }
        }
    }

    /* top to down, left to right */
    if (ptThis->bForwardVertical)
    {
        uint16_t *phwPixel = phwTarget;

        if (NULL != ptStatusH)
        {
            ptStatusH += tOffset.iX;
        }

        /* columns direct path */
        for (iX = 0; iX < iWidth / 8; iX++)
        {
            uint16_t       *phwChannel = phwPixel;

            if (NULL != ptStatusH && tOffset.iY > 0)
            {

                /* recover the previous values */
                pAccBase = (int16_t *) ptStatusH;
                vaccR = vld1q(pAccBase);
                vaccG = vld1q(pAccBase + 8);
                vaccB = vld1q(pAccBase + 16);
            }
            else
            {
                uint16x8_t       vacc = vldrhq_u16(phwChannel);
                arm2d_local_rgb565_unpack_single_vec(vacc, (uint16x8_t *)&vaccR, (uint16x8_t *)&vaccG,
                                                     (uint16x8_t *)&vaccB);
            }

            int32_t iYStart = (NULL != ptStatusH && tOffset.iY > 0) ? 0 : 1;
            phwChannel += iYStart * iTargetStride;
            for (iY = iYStart; iY < iHeight; iY++)
            {

                uint16x8_t      in = vldrhq_u16(phwChannel);
                uint16x8_t      vR, vG, vB;

                arm2d_local_rgb565_unpack_single_vec(in, &vR, &vG, &vB);

                int16x8_t       vdiffR = vsubq_s16((int16x8_t)vR, vaccR);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)vG, vaccG);
                int16x8_t       vdiffB = vsubq_s16((int16x8_t)vB, vaccB);

                vaccR += vqdmulhq(vdiffR, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccB += vqdmulhq(vdiffB, hwRatio);

                vstrhq_u16(phwChannel, arm2d_local_rgb565_pack_single_vec((uint16x8_t)vaccR, (uint16x8_t)vaccG,
                                                                          (uint16x8_t)vaccB));
                phwChannel += iTargetStride;
            }

            phwPixel += 8;

            if (NULL != ptStatusH)
            {
                /* save the last pixels */
                pAccBase = (int16_t *) ptStatusH;
                vst1q(pAccBase, vaccR);
                vst1q(pAccBase + 8, vaccG);
                vst1q(pAccBase + 16, vaccB);
                ptStatusH += 8;
            }
        }


        if (iWidth & 7)
        {
            mve_pred16_t    tailPred = vctp16q(iWidth & 7);
            uint16_t       *phwChannel = phwPixel;

            if (NULL != ptStatusH && tOffset.iY > 0)
            {
                /* recover the previous values */
                pAccBase = (int16_t *) ptStatusH;
                vaccR = vld1q(pAccBase);
                vaccG = vld1q(pAccBase + 8);
                vaccB = vld1q(pAccBase + 16);
            }
            else
            {
                uint16x8_t       vacc = vldrhq_u16(phwChannel);
                arm2d_local_rgb565_unpack_single_vec(vacc, (uint16x8_t *)&vaccR, (uint16x8_t *)&vaccG,
                                                     (uint16x8_t *)&vaccB);
            }

            int32_t iYStart = (NULL != ptStatusH && tOffset.iY > 0) ? 0 : 1;
            phwChannel += iYStart * iTargetStride;
            for (iY = iYStart; iY < iHeight; iY++)
            {
                uint16x8_t      in = vldrhq_u16(phwChannel);
                uint16x8_t      vR, vG, vB;

                arm2d_local_rgb565_unpack_single_vec(in, &vR, &vG, &vB);

                int16x8_t       vdiffR = vsubq_s16((int16x8_t)vR, vaccR);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)vG, vaccG);
                int16x8_t       vdiffB = vsubq_s16((int16x8_t)vB, vaccB);

                vaccR += vqdmulhq(vdiffR, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccB += vqdmulhq(vdiffB, hwRatio);

                vstrhq_p_u16(phwChannel, arm2d_local_rgb565_pack_single_vec((uint16x8_t)vaccR, (uint16x8_t)vaccG,
                                                                            (uint16x8_t)vaccB), tailPred);
                phwChannel += iTargetStride;
            }
            if (NULL != ptStatusH)
            {
                /* save the last pixels */
                pAccBase = (int16_t *) ptStatusH;
                vst1q(pAccBase, vaccR);
                vst1q(pAccBase + 8, vaccG);
                vst1q(pAccBase + 16, vaccB);
            }
        }
    }

    /* columns reverse path (bottom to top) - seeds fresh from the bottom edge */
    if (ptThis->bReverseVertical && bAllowReverseV)
    {
        uint16_t *phwPixel = phwTarget;

        for (iX = 0; iX < iWidth / 8; iX++)
        {
            uint16_t       *phwChannel = phwPixel + (iHeight - 1) * iTargetStride;

            uint16x8_t      vacc = vldrhq_u16(phwChannel);
            arm2d_local_rgb565_unpack_single_vec(vacc, (uint16x8_t *)&vaccR, (uint16x8_t *)&vaccG,
                                                 (uint16x8_t *)&vaccB);

            for (iY = 0; iY < iHeight; iY++)
            {
                uint16x8_t      in = vldrhq_u16(phwChannel);
                uint16x8_t      vR, vG, vB;

                arm2d_local_rgb565_unpack_single_vec(in, &vR, &vG, &vB);

                int16x8_t       vdiffR = vsubq_s16((int16x8_t)vR, vaccR);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)vG, vaccG);
                int16x8_t       vdiffB = vsubq_s16((int16x8_t)vB, vaccB);

                vaccR += vqdmulhq(vdiffR, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccB += vqdmulhq(vdiffB, hwRatio);

                vstrhq_u16(phwChannel, arm2d_local_rgb565_pack_single_vec((uint16x8_t)vaccR, (uint16x8_t)vaccG,
                                                                          (uint16x8_t)vaccB));
                phwChannel -= iTargetStride;
            }

            phwPixel += 8;
        }

        if (iWidth & 7)
        {
            mve_pred16_t    tailPred = vctp16q(iWidth & 7);
            uint16_t       *phwChannel = phwPixel + (iHeight - 1) * iTargetStride;

            uint16x8_t      vacc = vldrhq_u16(phwChannel);
            arm2d_local_rgb565_unpack_single_vec(vacc, (uint16x8_t *)&vaccR, (uint16x8_t *)&vaccG,
                                                 (uint16x8_t *)&vaccB);

            for (iY = 0; iY < iHeight; iY++)
            {
                uint16x8_t      in = vldrhq_u16(phwChannel);
                uint16x8_t      vR, vG, vB;

                arm2d_local_rgb565_unpack_single_vec(in, &vR, &vG, &vB);

                int16x8_t       vdiffR = vsubq_s16((int16x8_t)vR, vaccR);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)vG, vaccG);
                int16x8_t       vdiffB = vsubq_s16((int16x8_t)vB, vaccB);

                vaccR += vqdmulhq(vdiffR, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccB += vqdmulhq(vdiffB, hwRatio);

                vstrhq_p_u16(phwChannel, arm2d_local_rgb565_pack_single_vec((uint16x8_t)vaccR, (uint16x8_t)vaccG,
                                                                            (uint16x8_t)vaccB), tailPred);
                phwChannel -= iTargetStride;
            }
        }
    }
}

void arm2d_local_argb8888_filter_iir_blur_mve(
    uint32_t *__restrict pwTarget,
    int16_t iTargetStride,
    arm2d_local_region_t *__restrict ptValidRegionOnVirtualScreen,
    arm2d_local_region_t *ptTargetRegionOnVirtualScreen,
    uint8_t chBlurDegree,
    arm2d_local_filter_iir_blur_descriptor_t *ptThis)
{
    arm2d_local_scratch_mem_t *ptScratchMemory = &ptThis->tScratchMemory;
    int_fast16_t    iWidth = ptValidRegionOnVirtualScreen->tSize.iWidth;
    int_fast16_t    iHeight = ptValidRegionOnVirtualScreen->tSize.iHeight;

    if (0 == chBlurDegree)
    {
        return ;
    }

    int32_t         iY, iX;
    /* pre-scaled ratio to take into account doubling + high-part extraction of vqdmulhq */
    int16_t         hwRatio = (256 - chBlurDegree) << 7;
    arm2d_color_cccn888_t       *ptStatusH = NULL;
    arm2d_color_cccn888_t       *ptStatusV = NULL;
    int16_t        *pAccBase = NULL;
    int16x8_t       vaccB, vaccG, vaccR;

    if (NULL != (void *)(ptScratchMemory->pBuffer))
    {
        ptStatusH = (arm2d_color_cccn888_t *) ptScratchMemory->pBuffer;
        ptStatusV = ptStatusH + ROUND_UP_8(ptTargetRegionOnVirtualScreen->tSize.iWidth);
    }

    /* calculate the offset between the target region and the valid region */
    arm2d_local_location_t tOffset =
    {
        .iX = ptValidRegionOnVirtualScreen->tLocation.iX - ptTargetRegionOnVirtualScreen->tLocation.iX,
        .iY = ptValidRegionOnVirtualScreen->tLocation.iY - ptTargetRegionOnVirtualScreen->tLocation.iY,
    };

    const bool bAllowReverseH = arm2d_local_reverse_h_allowed(
                                    ptValidRegionOnVirtualScreen,
                                    ptTargetRegionOnVirtualScreen);
    const bool bAllowReverseV = arm2d_local_reverse_v_allowed(
                                    ptValidRegionOnVirtualScreen,
                                    ptTargetRegionOnVirtualScreen);

    if (ptThis->bForwardHorizontal)
    {
        uint32_t *pwPixel = pwTarget;

        if (NULL != ptStatusV)
        {
            /* rows direct path */
            ptStatusV += tOffset.iY;
        }
        uint16x8_t vstride = vidupq_n_u16(0, 4);
        vstride = vstride * iTargetStride;

        for (iY = 0; iY < iHeight / 8; iY++)
        {
            uint8_t        *pchPixelB = (uint8_t *) pwPixel;
            uint8_t        *pchPixelG = pchPixelB + 1;
            uint8_t        *pchPixelR = pchPixelB + 2;

            if (NULL != ptStatusV && tOffset.iX > 0)
            {
                /* recover the previous accumulators */
                pAccBase = (int16_t *) ptStatusV;
                vaccB = vld1q(pAccBase);
                vaccG = vld1q(pAccBase + 8);
                vaccR = vld1q(pAccBase + 16);
            }
            else
            {
                vaccB = (int16x8_t)vldrbq_gather_offset_u16(pchPixelB, vstride);
                vaccG = (int16x8_t)vldrbq_gather_offset_u16(pchPixelG, vstride);
                vaccR = (int16x8_t)vldrbq_gather_offset_u16(pchPixelR, vstride);
            }


            for (iX = 0; iX < iWidth; iX++)
            {

                uint16x8_t      inB = vldrbq_gather_offset_u16(pchPixelB, vstride);
                uint16x8_t      inG = vldrbq_gather_offset_u16(pchPixelG, vstride);
                uint16x8_t      inR = vldrbq_gather_offset_u16(pchPixelR, vstride);

                int16x8_t       vdiffB = vsubq_s16((int16x8_t)inB, vaccB);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)inG, vaccG);
                int16x8_t       vdiffR = vsubq_s16((int16x8_t)inR, vaccR);

                vaccB += vqdmulhq(vdiffB, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccR += vqdmulhq(vdiffR, hwRatio);

                vstrbq_scatter_offset_u16(pchPixelB, vstride, (uint16x8_t)vaccB);
                vstrbq_scatter_offset_u16(pchPixelG, vstride, (uint16x8_t)vaccG);
                vstrbq_scatter_offset_u16(pchPixelR, vstride, (uint16x8_t)vaccR);

                pchPixelB += 4;
                pchPixelG += 4;
                pchPixelR += 4;
            }

            if (NULL != ptStatusV)
            {
                pAccBase = (int16_t *) ptStatusV;
                vst1q(pAccBase, vaccB);
                vst1q(pAccBase + 8, vaccG);
                vst1q(pAccBase + 16, vaccR);
                ptStatusV += 8;
            }

            pwPixel += (iTargetStride * 8);
        }

        if (iHeight & 7)
        {

            mve_pred16_t    tailPred = vctp16q(iHeight & 7);

            uint8_t        *pchPixelB = (uint8_t *) pwPixel;
            uint8_t        *pchPixelG = pchPixelB + 1;
            uint8_t        *pchPixelR = pchPixelB + 2;

            if (NULL != ptStatusV && tOffset.iX > 0)
            {
                /* recover the previous accumulators */
                pAccBase = (int16_t *) ptStatusV;
                vaccB = vld1q(pAccBase);
                vaccG = vld1q(pAccBase + 8);
                vaccR = vld1q(pAccBase + 16);
            }
            else
            {
                vaccB = (int16x8_t)vldrbq_gather_offset_u16(pchPixelB, vstride);
                vaccG = (int16x8_t)vldrbq_gather_offset_u16(pchPixelG, vstride);
                vaccR = (int16x8_t)vldrbq_gather_offset_u16(pchPixelR, vstride);
            }

            for (iX = 0; iX < iWidth; iX++)
            {

                uint16x8_t      inB = vldrbq_gather_offset_u16(pchPixelB, vstride);
                uint16x8_t      inG = vldrbq_gather_offset_u16(pchPixelG, vstride);
                uint16x8_t      inR = vldrbq_gather_offset_u16(pchPixelR, vstride);

                int16x8_t       vdiffB = vsubq_s16((int16x8_t)inB, vaccB);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)inG, vaccG);
                int16x8_t       vdiffR = vsubq_s16((int16x8_t)inR, vaccR);

                vaccB += vqdmulhq(vdiffB, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccR += vqdmulhq(vdiffR, hwRatio);


                vstrbq_scatter_offset_p_u16(pchPixelB, vstride, (uint16x8_t)vaccB, tailPred);
                vstrbq_scatter_offset_p_u16(pchPixelG, vstride, (uint16x8_t)vaccG, tailPred);
                vstrbq_scatter_offset_p_u16(pchPixelR, vstride, (uint16x8_t)vaccR, tailPred);

                pchPixelB += 4;
                pchPixelG += 4;
                pchPixelR += 4;
            }

            if (NULL != ptStatusV)
            {
                pAccBase = (int16_t *) ptStatusV;
                vst1q(pAccBase, vaccB);
                vst1q(pAccBase + 8, vaccG);
                vst1q(pAccBase + 16, vaccR);
            }
        }
    }

    /* Reverse horizontal pass (right to left) - seeds fresh from the right edge */
    if (ptThis->bReverseHorizontal && bAllowReverseH)
    {
        uint32_t *pwPixel = pwTarget;

        uint16x8_t vstride = vidupq_n_u16(0, 4);
        vstride = vstride * iTargetStride;

        for (iY = 0; iY < iHeight / 8; iY++)
        {
            uint8_t        *pchPixelB = (uint8_t *) pwPixel + (iWidth - 1) * 4;
            uint8_t        *pchPixelG = pchPixelB + 1;
            uint8_t        *pchPixelR = pchPixelB + 2;

            vaccB = (int16x8_t)vldrbq_gather_offset_u16(pchPixelB, vstride);
            vaccG = (int16x8_t)vldrbq_gather_offset_u16(pchPixelG, vstride);
            vaccR = (int16x8_t)vldrbq_gather_offset_u16(pchPixelR, vstride);

            for (iX = 0; iX < iWidth; iX++)
            {
                uint16x8_t      inB = vldrbq_gather_offset_u16(pchPixelB, vstride);
                uint16x8_t      inG = vldrbq_gather_offset_u16(pchPixelG, vstride);
                uint16x8_t      inR = vldrbq_gather_offset_u16(pchPixelR, vstride);

                int16x8_t       vdiffB = vsubq_s16((int16x8_t)inB, vaccB);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)inG, vaccG);
                int16x8_t       vdiffR = vsubq_s16((int16x8_t)inR, vaccR);

                vaccB += vqdmulhq(vdiffB, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccR += vqdmulhq(vdiffR, hwRatio);

                vstrbq_scatter_offset_u16(pchPixelB, vstride, (uint16x8_t)vaccB);
                vstrbq_scatter_offset_u16(pchPixelG, vstride, (uint16x8_t)vaccG);
                vstrbq_scatter_offset_u16(pchPixelR, vstride, (uint16x8_t)vaccR);

                pchPixelB -= 4;
                pchPixelG -= 4;
                pchPixelR -= 4;
            }

            pwPixel += (iTargetStride * 8);
        }

        if (iHeight & 7)
        {
            mve_pred16_t    tailPred = vctp16q(iHeight & 7);

            uint8_t        *pchPixelB = (uint8_t *) pwPixel + (iWidth - 1) * 4;
            uint8_t        *pchPixelG = pchPixelB + 1;
            uint8_t        *pchPixelR = pchPixelB + 2;

            vaccB = (int16x8_t)vldrbq_gather_offset_u16(pchPixelB, vstride);
            vaccG = (int16x8_t)vldrbq_gather_offset_u16(pchPixelG, vstride);
            vaccR = (int16x8_t)vldrbq_gather_offset_u16(pchPixelR, vstride);

            for (iX = 0; iX < iWidth; iX++)
            {
                uint16x8_t      inB = vldrbq_gather_offset_u16(pchPixelB, vstride);
                uint16x8_t      inG = vldrbq_gather_offset_u16(pchPixelG, vstride);
                uint16x8_t      inR = vldrbq_gather_offset_u16(pchPixelR, vstride);

                int16x8_t       vdiffB = vsubq_s16((int16x8_t)inB, vaccB);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)inG, vaccG);
                int16x8_t       vdiffR = vsubq_s16((int16x8_t)inR, vaccR);

                vaccB += vqdmulhq(vdiffB, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccR += vqdmulhq(vdiffR, hwRatio);

                vstrbq_scatter_offset_p_u16(pchPixelB, vstride, (uint16x8_t)vaccB, tailPred);
                vstrbq_scatter_offset_p_u16(pchPixelG, vstride, (uint16x8_t)vaccG, tailPred);
                vstrbq_scatter_offset_p_u16(pchPixelR, vstride, (uint16x8_t)vaccR, tailPred);

                pchPixelB -= 4;
                pchPixelG -= 4;
                pchPixelR -= 4;
            }
        }
    }

    if (ptThis->bForwardVertical)
    {
        uint32_t *pwPixel = pwTarget;

        if (NULL != ptStatusH)
        {
            ptStatusH += tOffset.iX;
        }

        uint16x8_t vstride = vidupq_n_u16(0, 4);

        /* columns direct path */
        for (iX = 0; iX < iWidth / 8; iX++)
        {

            uint8_t        *pchPixelB = (uint8_t *) pwPixel;
            uint8_t        *pchPixelG = pchPixelB + 1;
            uint8_t        *pchPixelR = pchPixelB + 2;

            if (NULL != ptStatusH && tOffset.iY > 0)
            {
                /* recover the previous accumulators */
                pAccBase = (int16_t *) ptStatusH;
                vaccB = vld1q(pAccBase);
                vaccG = vld1q(pAccBase + 8);
                vaccR = vld1q(pAccBase + 16);
            }
            else
            {
                vaccB = (int16x8_t)vldrbq_gather_offset_u16(pchPixelB, vstride);
                vaccG = (int16x8_t)vldrbq_gather_offset_u16(pchPixelG, vstride);
                vaccR = (int16x8_t)vldrbq_gather_offset_u16(pchPixelR, vstride);
            }

            for (iY = 0; iY < iHeight; iY++)
            {

                uint16x8_t      inB = vldrbq_gather_offset_u16(pchPixelB, vstride);
                uint16x8_t      inG = vldrbq_gather_offset_u16(pchPixelG, vstride);
                uint16x8_t      inR = vldrbq_gather_offset_u16(pchPixelR, vstride);

                int16x8_t       vdiffB = vsubq_s16((int16x8_t)inB, vaccB);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)inG, vaccG);
                int16x8_t       vdiffR = vsubq_s16((int16x8_t)inR, vaccR);

                vaccB += vqdmulhq(vdiffB, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccR += vqdmulhq(vdiffR, hwRatio);


                vstrbq_scatter_offset_u16(pchPixelB, vstride, (uint16x8_t)vaccB);
                vstrbq_scatter_offset_u16(pchPixelG, vstride, (uint16x8_t)vaccG);
                vstrbq_scatter_offset_u16(pchPixelR, vstride, (uint16x8_t)vaccR);

                pchPixelB += 4 * iTargetStride;
                pchPixelG += 4 * iTargetStride;
                pchPixelR += 4 * iTargetStride;

            }

            pwPixel += 8;

            if (NULL != ptStatusH)
            {
                pAccBase = (int16_t *) ptStatusH;
                vst1q(pAccBase, vaccB);
                vst1q(pAccBase + 8, vaccG);
                vst1q(pAccBase + 16, vaccR);

                ptStatusH += 8;
            }
        }

        if (iWidth & 7)
        {
            mve_pred16_t    tailPred = vctp16q(iWidth & 7);
            uint8_t        *pchPixelB = (uint8_t *) pwPixel;
            uint8_t        *pchPixelG = pchPixelB + 1;
            uint8_t        *pchPixelR = pchPixelB + 2;

            if (NULL != ptStatusH && tOffset.iY > 0)
            {
                /* recover the previous accumulators */
                pAccBase = (int16_t *) ptStatusH;
                vaccB = vld1q(pAccBase);
                vaccG = vld1q(pAccBase + 8);
                vaccR = vld1q(pAccBase + 16);
            }
            else
            {
                vaccB = (int16x8_t)vldrbq_gather_offset_u16(pchPixelB, vstride);
                vaccG = (int16x8_t)vldrbq_gather_offset_u16(pchPixelG, vstride);
                vaccR = (int16x8_t)vldrbq_gather_offset_u16(pchPixelR, vstride);
            }

            for (iY = 0; iY < iHeight; iY++)
            {
                uint16x8_t      inB = vldrbq_gather_offset_u16(pchPixelB, vstride);
                uint16x8_t      inG = vldrbq_gather_offset_u16(pchPixelG, vstride);
                uint16x8_t      inR = vldrbq_gather_offset_u16(pchPixelR, vstride);

                int16x8_t       vdiffB = vsubq_s16((int16x8_t)inB, vaccB);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)inG, vaccG);
                int16x8_t       vdiffR = vsubq_s16((int16x8_t)inR, vaccR);

                vaccB += vqdmulhq(vdiffB, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccR += vqdmulhq(vdiffR, hwRatio);


                vstrbq_scatter_offset_p_u16(pchPixelB, vstride, (uint16x8_t)vaccB, tailPred);
                vstrbq_scatter_offset_p_u16(pchPixelG, vstride, (uint16x8_t)vaccG, tailPred);
                vstrbq_scatter_offset_p_u16(pchPixelR, vstride, (uint16x8_t)vaccR, tailPred);

                pchPixelB += 4 * iTargetStride;
                pchPixelG += 4 * iTargetStride;
                pchPixelR += 4 * iTargetStride;
            }

            if (NULL != ptStatusH)
            {
                pAccBase = (int16_t *) ptStatusH;
                vst1q(pAccBase, vaccB);
                vst1q(pAccBase + 8, vaccG);
                vst1q(pAccBase + 16, vaccR);

            }
        }
    }

    /* Reverse vertical pass (bottom to top) - seeds fresh from the bottom edge */
    if (ptThis->bReverseVertical && bAllowReverseV)
    {
        uint32_t *pwPixel = pwTarget;

        uint16x8_t vstride = vidupq_n_u16(0, 4);

        for (iX = 0; iX < iWidth / 8; iX++)
        {
            uint8_t        *pchPixelB = (uint8_t *) pwPixel + (iHeight - 1) * iTargetStride * 4;
            uint8_t        *pchPixelG = pchPixelB + 1;
            uint8_t        *pchPixelR = pchPixelB + 2;

            vaccB = (int16x8_t)vldrbq_gather_offset_u16(pchPixelB, vstride);
            vaccG = (int16x8_t)vldrbq_gather_offset_u16(pchPixelG, vstride);
            vaccR = (int16x8_t)vldrbq_gather_offset_u16(pchPixelR, vstride);

            for (iY = 0; iY < iHeight; iY++)
            {
                uint16x8_t      inB = vldrbq_gather_offset_u16(pchPixelB, vstride);
                uint16x8_t      inG = vldrbq_gather_offset_u16(pchPixelG, vstride);
                uint16x8_t      inR = vldrbq_gather_offset_u16(pchPixelR, vstride);

                int16x8_t       vdiffB = vsubq_s16((int16x8_t)inB, vaccB);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)inG, vaccG);
                int16x8_t       vdiffR = vsubq_s16((int16x8_t)inR, vaccR);

                vaccB += vqdmulhq(vdiffB, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccR += vqdmulhq(vdiffR, hwRatio);

                vstrbq_scatter_offset_u16(pchPixelB, vstride, (uint16x8_t)vaccB);
                vstrbq_scatter_offset_u16(pchPixelG, vstride, (uint16x8_t)vaccG);
                vstrbq_scatter_offset_u16(pchPixelR, vstride, (uint16x8_t)vaccR);

                pchPixelB -= 4 * iTargetStride;
                pchPixelG -= 4 * iTargetStride;
                pchPixelR -= 4 * iTargetStride;
            }

            pwPixel += 8;
        }

        if (iWidth & 7)
        {
            mve_pred16_t    tailPred = vctp16q(iWidth & 7);
            uint8_t        *pchPixelB = (uint8_t *) pwPixel + (iHeight - 1) * iTargetStride * 4;
            uint8_t        *pchPixelG = pchPixelB + 1;
            uint8_t        *pchPixelR = pchPixelB + 2;

            vaccB = (int16x8_t)vldrbq_gather_offset_u16(pchPixelB, vstride);
            vaccG = (int16x8_t)vldrbq_gather_offset_u16(pchPixelG, vstride);
            vaccR = (int16x8_t)vldrbq_gather_offset_u16(pchPixelR, vstride);

            for (iY = 0; iY < iHeight; iY++)
            {
                uint16x8_t      inB = vldrbq_gather_offset_u16(pchPixelB, vstride);
                uint16x8_t      inG = vldrbq_gather_offset_u16(pchPixelG, vstride);
                uint16x8_t      inR = vldrbq_gather_offset_u16(pchPixelR, vstride);

                int16x8_t       vdiffB = vsubq_s16((int16x8_t)inB, vaccB);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)inG, vaccG);
                int16x8_t       vdiffR = vsubq_s16((int16x8_t)inR, vaccR);

                vaccB += vqdmulhq(vdiffB, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccR += vqdmulhq(vdiffR, hwRatio);

                vstrbq_scatter_offset_p_u16(pchPixelB, vstride, (uint16x8_t)vaccB, tailPred);
                vstrbq_scatter_offset_p_u16(pchPixelG, vstride, (uint16x8_t)vaccG, tailPred);
                vstrbq_scatter_offset_p_u16(pchPixelR, vstride, (uint16x8_t)vaccR, tailPred);

                pchPixelB -= 4 * iTargetStride;
                pchPixelG -= 4 * iTargetStride;
                pchPixelR -= 4 * iTargetStride;
            }
        }
    }
}

void arm2d_local_rgb888_filter_iir_blur_mve(
    uint32_t *__restrict pwTarget,
    int16_t iTargetStride,
    arm2d_local_region_t *__restrict ptValidRegionOnVirtualScreen,
    arm2d_local_region_t *ptTargetRegionOnVirtualScreen,
    uint8_t chBlurDegree,
    arm2d_local_filter_iir_blur_descriptor_t *ptThis)
{
    arm2d_local_scratch_mem_t *ptScratchMemory = &ptThis->tScratchMemory;
    int_fast16_t    iWidth = ptValidRegionOnVirtualScreen->tSize.iWidth;
    int_fast16_t    iHeight = ptValidRegionOnVirtualScreen->tSize.iHeight;

    if (0 == chBlurDegree)
    {
        return ;
    }

    int32_t         iY, iX;
    /* pre-scaled ratio to take into account doubling + high-part extraction of vqdmulhq */
    int16_t         hwRatio = (256 - chBlurDegree) << 7;
    arm2d_color_cccn888_t       *ptStatusH = NULL;
    arm2d_color_cccn888_t       *ptStatusV = NULL;
    int16_t        *pAccBase = NULL;
    int16x8_t       vaccB, vaccG, vaccR;

    if (NULL != (void *)(ptScratchMemory->pBuffer))
    {
        ptStatusH = (arm2d_color_cccn888_t *) ptScratchMemory->pBuffer;
        ptStatusV = ptStatusH + ROUND_UP_8(ptTargetRegionOnVirtualScreen->tSize.iWidth);
    }

    /* calculate the offset between the target region and the valid region */
    arm2d_local_location_t tOffset =
    {
        .iX = ptValidRegionOnVirtualScreen->tLocation.iX - ptTargetRegionOnVirtualScreen->tLocation.iX,
        .iY = ptValidRegionOnVirtualScreen->tLocation.iY - ptTargetRegionOnVirtualScreen->tLocation.iY,
    };

    const bool bAllowReverseH = arm2d_local_reverse_h_allowed(
                                    ptValidRegionOnVirtualScreen,
                                    ptTargetRegionOnVirtualScreen);
    const bool bAllowReverseV = arm2d_local_reverse_v_allowed(
                                    ptValidRegionOnVirtualScreen,
                                    ptTargetRegionOnVirtualScreen);

    if (ptThis->bForwardHorizontal)
    {
        uint8_t *pwPixel = (uint8_t *)pwTarget;

        if (NULL != ptStatusV)
        {
            /* rows direct path */
            ptStatusV += tOffset.iY;
        }
        uint16x8_t step = vidupq_n_u16(0, 1);
        uint16x8_t vstride = vmulq_n_u16(step, 3);
        vstride = vstride * iTargetStride;

        for (iY = 0; iY < iHeight / 8; iY++)
        {
            uint8_t        *pchPixelB = (uint8_t *) pwPixel;
            uint8_t        *pchPixelG = pchPixelB + 1;
            uint8_t        *pchPixelR = pchPixelB + 2;

            if (NULL != ptStatusV && tOffset.iX > 0)
            {
                /* recover the previous accumulators */
                pAccBase = (int16_t *) ptStatusV;
                vaccB = vld1q(pAccBase);
                vaccG = vld1q(pAccBase + 8);
                vaccR = vld1q(pAccBase + 16);
            }
            else
            {
                vaccB = (int16x8_t)vldrbq_gather_offset_u16(pchPixelB, vstride);
                vaccG = (int16x8_t)vldrbq_gather_offset_u16(pchPixelG, vstride);
                vaccR = (int16x8_t)vldrbq_gather_offset_u16(pchPixelR, vstride);
            }


            for (iX = 0; iX < iWidth; iX++)
            {

                uint16x8_t      inB = vldrbq_gather_offset_u16(pchPixelB, vstride);
                uint16x8_t      inG = vldrbq_gather_offset_u16(pchPixelG, vstride);
                uint16x8_t      inR = vldrbq_gather_offset_u16(pchPixelR, vstride);

                int16x8_t       vdiffB = vsubq_s16((int16x8_t)inB, vaccB);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)inG, vaccG);
                int16x8_t       vdiffR = vsubq_s16((int16x8_t)inR, vaccR);

                vaccB += vqdmulhq(vdiffB, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccR += vqdmulhq(vdiffR, hwRatio);

                vstrbq_scatter_offset_u16(pchPixelB, vstride, (uint16x8_t)vaccB);
                vstrbq_scatter_offset_u16(pchPixelG, vstride, (uint16x8_t)vaccG);
                vstrbq_scatter_offset_u16(pchPixelR, vstride, (uint16x8_t)vaccR);

                pchPixelB += 3;
                pchPixelG += 3;
                pchPixelR += 3;
            }

            if (NULL != ptStatusV)
            {
                pAccBase = (int16_t *) ptStatusV;
                vst1q(pAccBase, vaccB);
                vst1q(pAccBase + 8, vaccG);
                vst1q(pAccBase + 16, vaccR);
                ptStatusV += 8;
            }

            pwPixel += (iTargetStride * 24);
        }

        if (iHeight & 7)
        {

            mve_pred16_t    tailPred = vctp16q(iHeight & 7);

            uint8_t        *pchPixelB = (uint8_t *) pwPixel;
            uint8_t        *pchPixelG = pchPixelB + 1;
            uint8_t        *pchPixelR = pchPixelB + 2;

            if (NULL != ptStatusV && tOffset.iX > 0)
            {
                /* recover the previous accumulators */
                pAccBase = (int16_t *) ptStatusV;
                vaccB = vld1q(pAccBase);
                vaccG = vld1q(pAccBase + 8);
                vaccR = vld1q(pAccBase + 16);
            }
            else
            {
                vaccB = (int16x8_t)vldrbq_gather_offset_u16(pchPixelB, vstride);
                vaccG = (int16x8_t)vldrbq_gather_offset_u16(pchPixelG, vstride);
                vaccR = (int16x8_t)vldrbq_gather_offset_u16(pchPixelR, vstride);
            }

            for (iX = 0; iX < iWidth; iX++)
            {

                uint16x8_t      inB = vldrbq_gather_offset_u16(pchPixelB, vstride);
                uint16x8_t      inG = vldrbq_gather_offset_u16(pchPixelG, vstride);
                uint16x8_t      inR = vldrbq_gather_offset_u16(pchPixelR, vstride);

                int16x8_t       vdiffB = vsubq_s16((int16x8_t)inB, vaccB);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)inG, vaccG);
                int16x8_t       vdiffR = vsubq_s16((int16x8_t)inR, vaccR);

                vaccB += vqdmulhq(vdiffB, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccR += vqdmulhq(vdiffR, hwRatio);


                vstrbq_scatter_offset_p_u16(pchPixelB, vstride, (uint16x8_t)vaccB, tailPred);
                vstrbq_scatter_offset_p_u16(pchPixelG, vstride, (uint16x8_t)vaccG, tailPred);
                vstrbq_scatter_offset_p_u16(pchPixelR, vstride, (uint16x8_t)vaccR, tailPred);

                pchPixelB += 3;
                pchPixelG += 3;
                pchPixelR += 3;
            }

            if (NULL != ptStatusV)
            {
                pAccBase = (int16_t *) ptStatusV;
                vst1q(pAccBase, vaccB);
                vst1q(pAccBase + 8, vaccG);
                vst1q(pAccBase + 16, vaccR);
            }
        }
    }

    /* Reverse horizontal pass (right to left) - seeds fresh from the right edge */
    if (ptThis->bReverseHorizontal && bAllowReverseH)
    {
        uint8_t *pwPixel = (uint8_t *)pwTarget;

        uint16x8_t step = vidupq_n_u16(0, 1);
        uint16x8_t vstride = vmulq_n_u16(step, 3);
        vstride = vstride * iTargetStride;

        for (iY = 0; iY < iHeight / 8; iY++)
        {
            uint8_t        *pchPixelB = (uint8_t *) pwPixel + (iWidth - 1) * 3;
            uint8_t        *pchPixelG = pchPixelB + 1;
            uint8_t        *pchPixelR = pchPixelB + 2;

            vaccB = (int16x8_t)vldrbq_gather_offset_u16(pchPixelB, vstride);
            vaccG = (int16x8_t)vldrbq_gather_offset_u16(pchPixelG, vstride);
            vaccR = (int16x8_t)vldrbq_gather_offset_u16(pchPixelR, vstride);

            for (iX = 0; iX < iWidth; iX++)
            {
                uint16x8_t      inB = vldrbq_gather_offset_u16(pchPixelB, vstride);
                uint16x8_t      inG = vldrbq_gather_offset_u16(pchPixelG, vstride);
                uint16x8_t      inR = vldrbq_gather_offset_u16(pchPixelR, vstride);

                int16x8_t       vdiffB = vsubq_s16((int16x8_t)inB, vaccB);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)inG, vaccG);
                int16x8_t       vdiffR = vsubq_s16((int16x8_t)inR, vaccR);

                vaccB += vqdmulhq(vdiffB, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccR += vqdmulhq(vdiffR, hwRatio);

                vstrbq_scatter_offset_u16(pchPixelB, vstride, (uint16x8_t)vaccB);
                vstrbq_scatter_offset_u16(pchPixelG, vstride, (uint16x8_t)vaccG);
                vstrbq_scatter_offset_u16(pchPixelR, vstride, (uint16x8_t)vaccR);

                pchPixelB -= 3;
                pchPixelG -= 3;
                pchPixelR -= 3;
            }

            pwPixel += (iTargetStride * 24);
        }

        if (iHeight & 7)
        {
            mve_pred16_t    tailPred = vctp16q(iHeight & 7);

            uint8_t        *pchPixelB = (uint8_t *) pwPixel + (iWidth - 1) * 3;
            uint8_t        *pchPixelG = pchPixelB + 1;
            uint8_t        *pchPixelR = pchPixelB + 2;

            vaccB = (int16x8_t)vldrbq_gather_offset_u16(pchPixelB, vstride);
            vaccG = (int16x8_t)vldrbq_gather_offset_u16(pchPixelG, vstride);
            vaccR = (int16x8_t)vldrbq_gather_offset_u16(pchPixelR, vstride);

            for (iX = 0; iX < iWidth; iX++)
            {
                uint16x8_t      inB = vldrbq_gather_offset_u16(pchPixelB, vstride);
                uint16x8_t      inG = vldrbq_gather_offset_u16(pchPixelG, vstride);
                uint16x8_t      inR = vldrbq_gather_offset_u16(pchPixelR, vstride);

                int16x8_t       vdiffB = vsubq_s16((int16x8_t)inB, vaccB);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)inG, vaccG);
                int16x8_t       vdiffR = vsubq_s16((int16x8_t)inR, vaccR);

                vaccB += vqdmulhq(vdiffB, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccR += vqdmulhq(vdiffR, hwRatio);

                vstrbq_scatter_offset_p_u16(pchPixelB, vstride, (uint16x8_t)vaccB, tailPred);
                vstrbq_scatter_offset_p_u16(pchPixelG, vstride, (uint16x8_t)vaccG, tailPred);
                vstrbq_scatter_offset_p_u16(pchPixelR, vstride, (uint16x8_t)vaccR, tailPred);

                pchPixelB -= 3;
                pchPixelG -= 3;
                pchPixelR -= 3;
            }
        }
    }

    if (ptThis->bForwardVertical)
    {
        uint8_t *pwPixel = (uint8_t *)pwTarget;

        if (NULL != ptStatusH)
        {
            ptStatusH += tOffset.iX;
        }

        uint16x8_t step = vidupq_n_u16(0, 1);
        uint16x8_t vstride = vmulq_n_u16(step, 3);

        /* columns direct path */
        for (iX = 0; iX < iWidth / 8; iX++)
        {

            uint8_t        *pchPixelB = (uint8_t *) pwPixel;
            uint8_t        *pchPixelG = pchPixelB + 1;
            uint8_t        *pchPixelR = pchPixelB + 2;

            if (NULL != ptStatusH && tOffset.iY > 0)
            {
                /* recover the previous accumulators */
                pAccBase = (int16_t *) ptStatusH;
                vaccB = vld1q(pAccBase);
                vaccG = vld1q(pAccBase + 8);
                vaccR = vld1q(pAccBase + 16);
            }
            else
            {
                vaccB = (int16x8_t)vldrbq_gather_offset_u16(pchPixelB, vstride);
                vaccG = (int16x8_t)vldrbq_gather_offset_u16(pchPixelG, vstride);
                vaccR = (int16x8_t)vldrbq_gather_offset_u16(pchPixelR, vstride);
            }

            for (iY = 0; iY < iHeight; iY++)
            {

                uint16x8_t      inB = vldrbq_gather_offset_u16(pchPixelB, vstride);
                uint16x8_t      inG = vldrbq_gather_offset_u16(pchPixelG, vstride);
                uint16x8_t      inR = vldrbq_gather_offset_u16(pchPixelR, vstride);

                int16x8_t       vdiffB = vsubq_s16((int16x8_t)inB, vaccB);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)inG, vaccG);
                int16x8_t       vdiffR = vsubq_s16((int16x8_t)inR, vaccR);

                vaccB += vqdmulhq(vdiffB, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccR += vqdmulhq(vdiffR, hwRatio);


                vstrbq_scatter_offset_u16(pchPixelB, vstride, (uint16x8_t)vaccB);
                vstrbq_scatter_offset_u16(pchPixelG, vstride, (uint16x8_t)vaccG);
                vstrbq_scatter_offset_u16(pchPixelR, vstride, (uint16x8_t)vaccR);

                pchPixelB += 3 * iTargetStride;
                pchPixelG += 3 * iTargetStride;
                pchPixelR += 3 * iTargetStride;

            }

            pwPixel += 24;

            if (NULL != ptStatusH)
            {
                pAccBase = (int16_t *) ptStatusH;
                vst1q(pAccBase, vaccB);
                vst1q(pAccBase + 8, vaccG);
                vst1q(pAccBase + 16, vaccR);

                ptStatusH += 8;
            }
        }

        if (iWidth & 7)
        {
            mve_pred16_t    tailPred = vctp16q(iWidth & 7);
            uint8_t        *pchPixelB = (uint8_t *) pwPixel;
            uint8_t        *pchPixelG = pchPixelB + 1;
            uint8_t        *pchPixelR = pchPixelB + 2;

            if (NULL != ptStatusH && tOffset.iY > 0)
            {
                /* recover the previous accumulators */
                pAccBase = (int16_t *) ptStatusH;
                vaccB = vld1q(pAccBase);
                vaccG = vld1q(pAccBase + 8);
                vaccR = vld1q(pAccBase + 16);
            }
            else
            {
                vaccB = (int16x8_t)vldrbq_gather_offset_u16(pchPixelB, vstride);
                vaccG = (int16x8_t)vldrbq_gather_offset_u16(pchPixelG, vstride);
                vaccR = (int16x8_t)vldrbq_gather_offset_u16(pchPixelR, vstride);
            }

            for (iY = 0; iY < iHeight; iY++)
            {
                uint16x8_t      inB = vldrbq_gather_offset_u16(pchPixelB, vstride);
                uint16x8_t      inG = vldrbq_gather_offset_u16(pchPixelG, vstride);
                uint16x8_t      inR = vldrbq_gather_offset_u16(pchPixelR, vstride);

                int16x8_t       vdiffB = vsubq_s16((int16x8_t)inB, vaccB);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)inG, vaccG);
                int16x8_t       vdiffR = vsubq_s16((int16x8_t)inR, vaccR);

                vaccB += vqdmulhq(vdiffB, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccR += vqdmulhq(vdiffR, hwRatio);


                vstrbq_scatter_offset_p_u16(pchPixelB, vstride, (uint16x8_t)vaccB, tailPred);
                vstrbq_scatter_offset_p_u16(pchPixelG, vstride, (uint16x8_t)vaccG, tailPred);
                vstrbq_scatter_offset_p_u16(pchPixelR, vstride, (uint16x8_t)vaccR, tailPred);

                pchPixelB += 3 * iTargetStride;
                pchPixelG += 3 * iTargetStride;
                pchPixelR += 3 * iTargetStride;
            }

            if (NULL != ptStatusH)
            {
                pAccBase = (int16_t *) ptStatusH;
                vst1q(pAccBase, vaccB);
                vst1q(pAccBase + 8, vaccG);
                vst1q(pAccBase + 16, vaccR);

            }
        }
    }

    /* Reverse vertical pass (bottom to top) - seeds fresh from the bottom edge */
    if (ptThis->bReverseVertical && bAllowReverseV)
    {
        uint8_t *pwPixel = (uint8_t *)pwTarget;

        uint16x8_t step = vidupq_n_u16(0, 1);
        uint16x8_t vstride = vmulq_n_u16(step, 3);

        for (iX = 0; iX < iWidth / 8; iX++)
        {
            uint8_t        *pchPixelB = (uint8_t *) pwPixel + (iHeight - 1) * iTargetStride * 3;
            uint8_t        *pchPixelG = pchPixelB + 1;
            uint8_t        *pchPixelR = pchPixelB + 2;

            vaccB = (int16x8_t)vldrbq_gather_offset_u16(pchPixelB, vstride);
            vaccG = (int16x8_t)vldrbq_gather_offset_u16(pchPixelG, vstride);
            vaccR = (int16x8_t)vldrbq_gather_offset_u16(pchPixelR, vstride);

            for (iY = 0; iY < iHeight; iY++)
            {
                uint16x8_t      inB = vldrbq_gather_offset_u16(pchPixelB, vstride);
                uint16x8_t      inG = vldrbq_gather_offset_u16(pchPixelG, vstride);
                uint16x8_t      inR = vldrbq_gather_offset_u16(pchPixelR, vstride);

                int16x8_t       vdiffB = vsubq_s16((int16x8_t)inB, vaccB);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)inG, vaccG);
                int16x8_t       vdiffR = vsubq_s16((int16x8_t)inR, vaccR);

                vaccB += vqdmulhq(vdiffB, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccR += vqdmulhq(vdiffR, hwRatio);

                vstrbq_scatter_offset_u16(pchPixelB, vstride, (uint16x8_t)vaccB);
                vstrbq_scatter_offset_u16(pchPixelG, vstride, (uint16x8_t)vaccG);
                vstrbq_scatter_offset_u16(pchPixelR, vstride, (uint16x8_t)vaccR);

                pchPixelB -= 3 * iTargetStride;
                pchPixelG -= 3 * iTargetStride;
                pchPixelR -= 3 * iTargetStride;
            }

            pwPixel += 24;
        }

        if (iWidth & 7)
        {
            mve_pred16_t    tailPred = vctp16q(iWidth & 7);
            uint8_t        *pchPixelB = (uint8_t *) pwPixel + (iHeight - 1) * iTargetStride * 3;
            uint8_t        *pchPixelG = pchPixelB + 1;
            uint8_t        *pchPixelR = pchPixelB + 2;

            vaccB = (int16x8_t)vldrbq_gather_offset_u16(pchPixelB, vstride);
            vaccG = (int16x8_t)vldrbq_gather_offset_u16(pchPixelG, vstride);
            vaccR = (int16x8_t)vldrbq_gather_offset_u16(pchPixelR, vstride);

            for (iY = 0; iY < iHeight; iY++)
            {
                uint16x8_t      inB = vldrbq_gather_offset_u16(pchPixelB, vstride);
                uint16x8_t      inG = vldrbq_gather_offset_u16(pchPixelG, vstride);
                uint16x8_t      inR = vldrbq_gather_offset_u16(pchPixelR, vstride);

                int16x8_t       vdiffB = vsubq_s16((int16x8_t)inB, vaccB);
                int16x8_t       vdiffG = vsubq_s16((int16x8_t)inG, vaccG);
                int16x8_t       vdiffR = vsubq_s16((int16x8_t)inR, vaccR);

                vaccB += vqdmulhq(vdiffB, hwRatio);
                vaccG += vqdmulhq(vdiffG, hwRatio);
                vaccR += vqdmulhq(vdiffR, hwRatio);

                vstrbq_scatter_offset_p_u16(pchPixelB, vstride, (uint16x8_t)vaccB, tailPred);
                vstrbq_scatter_offset_p_u16(pchPixelG, vstride, (uint16x8_t)vaccG, tailPred);
                vstrbq_scatter_offset_p_u16(pchPixelR, vstride, (uint16x8_t)vaccR, tailPred);

                pchPixelB -= 3 * iTargetStride;
                pchPixelG -= 3 * iTargetStride;
                pchPixelR -= 3 * iTargetStride;
            }
        }
    }
}


static void mve_arm2d_run_blur(uint8_t *buffer, int16_t stride,
                               arm2d_local_region_t *valid, arm2d_local_region_t *target,
                               uint16_t bit_depth, uint8_t blur_degree,
                               arm2d_local_scratch_mem_t *scratch_mem)
{
    arm2d_local_filter_iir_blur_descriptor_t dsc = {0};
    dsc.tScratchMemory.tInfo = scratch_mem->tInfo;
    dsc.tScratchMemory.pBuffer = scratch_mem->pBuffer;
    dsc.bForwardHorizontal = 1;
    dsc.bForwardVertical = 1;
    dsc.bReverseHorizontal = 1;
    dsc.bReverseVertical = 0;

    if (bit_depth == 16)
    {
        arm2d_local_rgb565_filter_iir_blur_mve((uint16_t *)buffer, stride, valid, target,
                                               blur_degree, &dsc);
    }
    else if (bit_depth == 32)
    {
        arm2d_local_argb8888_filter_iir_blur_mve((uint32_t *)buffer, stride, valid, target,
                                                 blur_degree, &dsc);
    }
    else if (bit_depth == 24)
    {
        arm2d_local_rgb888_filter_iir_blur_mve((uint32_t *)buffer, stride, valid, target,
                                               blur_degree, &dsc);
    }
}

#if MVE_ARM2D_BLUR_DOWNSCALE_FACTOR > 1
static void mve_arm2d_nearest_scale(uint8_t *target_buffer,
                                    const uint8_t *source_buffer,
                                    uint16_t source_width,
                                    uint16_t source_height,
                                    uint16_t source_stride,
                                    uint16_t target_stride,
                                    uint8_t factor,
                                    uint16_t target_width,
                                    uint16_t target_height,
                                    uint8_t bytes_per_pixel,
                                    bool scale_up)
{
    struct acc_engine *acc = gui_get_acc();
    if ((acc != NULL) && (acc->blur != NULL) && (acc->blur->scale != NULL)
        && acc->blur->scale(target_buffer, source_buffer, source_width, source_height,
                            source_stride, target_stride, factor, target_width,
                            target_height, bytes_per_pixel, scale_up))
    {
        return;
    }

    uint8_t copy_size = (scale_up && (bytes_per_pixel == 4U)) ? 3U : bytes_per_pixel;

    for (uint32_t y = 0; y < target_height; y++)
    {
        uint32_t source_y = (!scale_up) ? y * factor : y / factor;
        if (source_y >= source_height)
        {
            source_y = source_height - 1U;
        }
        for (uint32_t x = 0; x < target_width; x++)
        {
            uint32_t source_x = (!scale_up) ? x * factor : x / factor;
            if (source_x >= source_width)
            {
                source_x = source_width - 1U;
            }
            uint8_t *destination = target_buffer
                                   + ((size_t)y * target_stride + x) * bytes_per_pixel;
            const uint8_t *source = source_buffer
                                    + ((size_t)source_y * source_stride + source_x) * bytes_per_pixel;
            memcpy(destination, source, copy_size);
        }
    }
}

#endif

static uint8_t mve_arm2d_bytes_per_pixel(uint16_t bit_depth)
{
    if ((bit_depth == 16) || (bit_depth == 24) || (bit_depth == 32))
    {
        return bit_depth / 8;
    }
    return 0;
}

static bool mve_arm2d_scratch_ensure(arm2d_local_scratch_mem_t *scratch_mem,
                                     uint16_t target_width, uint16_t target_height,
                                     uint8_t bytes_per_pixel)
{
    if ((scratch_mem->pBuffer != 0U)
        && (scratch_mem->hwTargetWidth >= target_width)
        && (scratch_mem->hwTargetHeight >= target_height)
        && (scratch_mem->chStatusBytesPerPixel == bytes_per_pixel))
    {
        return true;
    }

    uintptr_t scaled_buffer = scratch_mem->pScaledBuffer;
    uint16_t scaled_stride = scratch_mem->hwScaledStride;
    uint16_t scaled_rows = scratch_mem->hwScaledRows;
    uint8_t scaled_bytes_per_pixel = scratch_mem->chBytesPerPixel;
    size_t scaled_buffer_size = scratch_mem->scaledBufferSize;
    bool scaled_buffer_allocated = scratch_mem->bScaledBufferAllocated;

    if (scratch_mem->pBuffer != 0U)
    {
        arm2d_local_scratch_memory_free(scratch_mem);
    }
    else
    {
        scratch_mem->tInfo.Value = 0U;
    }
    scratch_mem->pScaledBuffer = scaled_buffer;
    scratch_mem->hwScaledStride = scaled_stride;
    scratch_mem->hwScaledRows = scaled_rows;
    scratch_mem->chBytesPerPixel = scaled_bytes_per_pixel;
    scratch_mem->scaledBufferSize = scaled_buffer_size;
    scratch_mem->bScaledBufferAllocated = scaled_buffer_allocated;
    uint32_t status_count = (uint32_t)target_width + target_height + 14U;
    if (NULL == arm2d_local_scratch_memory_new(
            scratch_mem, sizeof(arm2d_color_rgb565_t), status_count,
            __alignof__(arm2d_color_rgb565_t)))
    {
        return false;
    }
    scratch_mem->hwTargetWidth = target_width;
    scratch_mem->hwTargetHeight = target_height;
    scratch_mem->chStatusBytesPerPixel = bytes_per_pixel;
    return true;
}

#if MVE_ARM2D_BLUR_DOWNSCALE_FACTOR > 1
static bool mve_arm2d_scaled_buffer_ensure(arm2d_local_scratch_mem_t *scratch_mem,
                                           uint16_t scaled_width, uint16_t scaled_height,
                                           uint8_t bytes_per_pixel)
{
    uint16_t stride = ROUND_UP_8((uint32_t)scaled_width + 7U);
    uint16_t rows = ROUND_UP_8((uint32_t)scaled_height + 7U);
    size_t required_size = (size_t)stride * rows * bytes_per_pixel;

    if ((scratch_mem->pScaledBuffer != 0U)
        && (scratch_mem->scaledBufferSize >= required_size)
        && (scratch_mem->hwScaledStride >= stride)
        && (scratch_mem->hwScaledRows >= rows)
        && (scratch_mem->chBytesPerPixel == bytes_per_pixel))
    {
        return true;
    }

    if (scratch_mem->bScaledBufferAllocated)
    {
        gui_free((void *)scratch_mem->pScaledBuffer);
    }
    scratch_mem->pScaledBuffer = 0U;
    scratch_mem->bScaledBufferAllocated = false;

    /* use the cache memory/size provided by the acc engine (gui_get_acc),
     * fall back to heap allocation when it does not fit or is unavailable */
    struct acc_engine *acc = gui_get_acc();
    uint8_t *acc_cache = NULL;
    size_t acc_cache_size = 0U;
    if ((acc != NULL) && (acc->cache_mem != NULL) && (acc->cache_size > 0U))
    {
        acc_cache = acc->cache_mem;
        acc_cache_size = (size_t)acc->cache_size;
    }

    if ((acc_cache != NULL) && (required_size <= acc_cache_size))
    {
        scratch_mem->pScaledBuffer = (uintptr_t)acc_cache;
    }
    else
    {
        scratch_mem->pScaledBuffer = (uintptr_t)gui_malloc(required_size);
        scratch_mem->bScaledBufferAllocated = scratch_mem->pScaledBuffer != 0U;
    }
    if (scratch_mem->pScaledBuffer == 0U)
    {
        return false;
    }

    scratch_mem->hwScaledStride = stride;
    scratch_mem->hwScaledRows = rows;
    scratch_mem->chBytesPerPixel = bytes_per_pixel;
    scratch_mem->scaledBufferSize = required_size;
    return true;
}

static bool mve_arm2d_scaled_blur(uint8_t *buffer, uint16_t buffer_stride,
                                  uint16_t bit_depth, const gui_rect_t *buffer_rect,
                                  const gui_rect_t *valid_rect, const gui_rect_t *target_rect,
                                  uint8_t blur_degree, arm2d_local_scratch_mem_t *scratch_mem)
{
    uint8_t bytes_per_pixel = mve_arm2d_bytes_per_pixel(bit_depth);
    uint16_t target_width = target_rect->x2 - target_rect->x1 + 1;
    uint16_t target_height = target_rect->y2 - target_rect->y1 + 1;
    uint32_t factor = MVE_ARM2D_BLUR_DOWNSCALE_FACTOR;
    uint16_t scaled_target_width = ((uint32_t)target_width + factor - 1U) / factor;
    uint16_t scaled_target_height = ((uint32_t)target_height + factor - 1U) / factor;
    if ((bytes_per_pixel == 0U) || (scratch_mem == NULL))
    {
        return false;
    }

    bool full_target = (valid_rect->x1 == target_rect->x1)
                       && (valid_rect->x2 == target_rect->x2)
                       && (valid_rect->y1 == target_rect->y1)
                       && (valid_rect->y2 == target_rect->y2);
    bool full_width_y_slice = (valid_rect->x1 == target_rect->x1)
                              && (valid_rect->x2 == target_rect->x2)
                              && (valid_rect->y1 >= target_rect->y1)
                              && (valid_rect->y2 <= target_rect->y2);
    if (!full_target && !full_width_y_slice)
    {
        return false;
    }
    if (!mve_arm2d_scratch_ensure(scratch_mem, scaled_target_width,
                                  scaled_target_height, bytes_per_pixel))
    {
        return false;
    }

    uint16_t source_width = valid_rect->x2 - valid_rect->x1 + 1;
    uint16_t source_height = valid_rect->y2 - valid_rect->y1 + 1;
    uint32_t scaled_y = ((uint32_t)(valid_rect->y1 - target_rect->y1) + factor - 1U) / factor;
    uint32_t scaled_y_end = ((uint32_t)(valid_rect->y2 - target_rect->y1 + 1)
                             + factor - 1U) / factor;
    uint16_t scaled_height = (uint16_t)(scaled_y_end - scaled_y);
    if ((scaled_height == 0U) || (scaled_y + scaled_height > scaled_target_height)
        || !mve_arm2d_scaled_buffer_ensure(scratch_mem, scaled_target_width + 1U,
                                           scaled_height + 1U, bytes_per_pixel))
    {
        return false;
    }

    uint8_t *source_start = buffer
                            + (((size_t)(valid_rect->y1 - buffer_rect->y1) * buffer_stride)
                               + (valid_rect->x1 - buffer_rect->x1)) * bytes_per_pixel;
    uint8_t *scaled_start = (uint8_t *)scratch_mem->pScaledBuffer;
    mve_arm2d_nearest_scale(scaled_start, source_start,
                            source_width, source_height, buffer_stride,
                            scratch_mem->hwScaledStride, factor,
                            scaled_target_width, scaled_height, bytes_per_pixel, false);

    arm2d_local_region_t scaled_valid =
    {
        .tLocation = {.iX = 0, .iY = (int16_t)scaled_y},
        .tSize = {.iWidth = scaled_target_width, .iHeight = scaled_height},
    };
    arm2d_local_region_t scaled_target =
    {
        .tLocation = {.iX = 0, .iY = 0},
        .tSize = {.iWidth = scaled_target_width, .iHeight = scaled_target_height},
    };
    mve_arm2d_run_blur(scaled_start, scratch_mem->hwScaledStride,
                       &scaled_valid, &scaled_target, bit_depth, blur_degree, scratch_mem);
    for (uint32_t y = 0; y < scaled_height; y++)
    {
        uint8_t *row = scaled_start
                       + (size_t)y * scratch_mem->hwScaledStride * bytes_per_pixel;
        memcpy(row + (size_t)scaled_target_width * bytes_per_pixel,
               row + (size_t)(scaled_target_width - 1U) * bytes_per_pixel,
               bytes_per_pixel);
    }
    /* Copy the bottom edge after padding columns to include the guard corner. */
    memcpy(scaled_start + (size_t)scaled_height * scratch_mem->hwScaledStride * bytes_per_pixel,
           scaled_start + (size_t)(scaled_height - 1U) * scratch_mem->hwScaledStride
           * bytes_per_pixel,
           (size_t)scratch_mem->hwScaledStride * bytes_per_pixel);

    mve_arm2d_nearest_scale(source_start, scaled_start,
                            scaled_target_width + 1U, scaled_height + 1U,
                            scratch_mem->hwScaledStride, buffer_stride, factor,
                            source_width, source_height, bytes_per_pixel, true);
    return true;
}
#endif

void mve_arm_2d_blur(uint8_t *buffer, uint16_t buffer_stride, uint16_t bit_depth,
                     const gui_rect_t *buffer_rect, const gui_rect_t *valid_rect,
                     const gui_rect_t *target_rect, uint8_t blur_degree, void *cache_mem)
{
    if ((buffer == NULL) || (buffer_rect == NULL) || (valid_rect == NULL)
        || (target_rect == NULL) || (buffer_stride == 0U)
        || (buffer_rect->x2 < buffer_rect->x1) || (buffer_rect->y2 < buffer_rect->y1)
        || (valid_rect->x2 < valid_rect->x1) || (valid_rect->y2 < valid_rect->y1)
        || (target_rect->x2 < target_rect->x1) || (target_rect->y2 < target_rect->y1)
        || (buffer_stride < (uint16_t)(buffer_rect->x2 - buffer_rect->x1 + 1))
        || (valid_rect->x1 < buffer_rect->x1) || (valid_rect->y1 < buffer_rect->y1)
        || (valid_rect->x2 > buffer_rect->x2) || (valid_rect->y2 > buffer_rect->y2)
        || (valid_rect->x1 < target_rect->x1) || (valid_rect->y1 < target_rect->y1)
        || (valid_rect->x2 > target_rect->x2) || (valid_rect->y2 > target_rect->y2))
    {
        return;
    }

#if MVE_ARM2D_BLUR_DOWNSCALE_FACTOR > 1
    if ((cache_mem != NULL)
        && mve_arm2d_scaled_blur(buffer, buffer_stride, bit_depth, buffer_rect,
                                 valid_rect, target_rect, blur_degree,
                                 (arm2d_local_scratch_mem_t *)cache_mem))
    {
        return;
    }
#endif

    uint8_t bytes_per_pixel = mve_arm2d_bytes_per_pixel(bit_depth);
    if (bytes_per_pixel == 0U)
    {
        return;
    }
    uint8_t *valid_start = buffer
                           + (((size_t)(valid_rect->y1 - buffer_rect->y1) * buffer_stride)
                              + (valid_rect->x1 - buffer_rect->x1)) * bytes_per_pixel;
    arm2d_local_region_t valid =
    {
        .tLocation = {.iX = valid_rect->x1, .iY = valid_rect->y1},
        .tSize =
        {
            .iWidth = valid_rect->x2 - valid_rect->x1 + 1,
            .iHeight = valid_rect->y2 - valid_rect->y1 + 1,
        },
    };
    arm2d_local_region_t target =
    {
        .tLocation = {.iX = target_rect->x1, .iY = target_rect->y1},
        .tSize =
        {
            .iWidth = target_rect->x2 - target_rect->x1 + 1,
            .iHeight = target_rect->y2 - target_rect->y1 + 1,
        },
    };
    arm2d_local_scratch_mem_t local_scratch_mem = {0};
    arm2d_local_scratch_mem_t *mem_for_blur = (arm2d_local_scratch_mem_t *)cache_mem;
    if (mem_for_blur == NULL)
    {
        mem_for_blur = &local_scratch_mem;
    }
    else if (!mve_arm2d_scratch_ensure(
                 mem_for_blur, target.tSize.iWidth, target.tSize.iHeight,
                 bytes_per_pixel))
    {
        return;
    }
    mve_arm2d_run_blur(valid_start, buffer_stride, &valid, &target, bit_depth,
                       blur_degree, mem_for_blur);
}

void mve_arm_2d_create(void **mem)
{
    if ((mem == NULL) || (*mem != NULL))
    {
        return;
    }
    arm2d_local_scratch_mem_t *scratch_mem = gui_malloc(sizeof(arm2d_local_scratch_mem_t));
    if (scratch_mem != NULL)
    {
        memset(scratch_mem, 0, sizeof(*scratch_mem));
    }
    *mem = (void *)scratch_mem;
}

void mve_arm_2d_depose(void **mem)
{
    if (*mem != NULL)
    {
        arm2d_local_scratch_mem_t *scratch_mem = (arm2d_local_scratch_mem_t *)*mem;
#if MVE_ARM2D_BLUR_DOWNSCALE_FACTOR > 1
        if (scratch_mem->bScaledBufferAllocated)
        {
            gui_free((void *)scratch_mem->pScaledBuffer);
        }
#endif
        if (scratch_mem->pBuffer != 0U)
        {
            arm2d_local_scratch_memory_free(scratch_mem);
        }
    }
    gui_free(*mem);
    *mem = NULL;
}

gui_blur_ops_t mve_arm2d_blur_ops =
{
    .prepare = mve_arm_2d_create,
    .process = mve_arm_2d_blur,
    .release = mve_arm_2d_depose,
};

#endif
