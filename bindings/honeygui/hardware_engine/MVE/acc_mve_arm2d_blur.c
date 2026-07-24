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

    uint16x8_t      vOut = vorrq(vshrq(B, 3),
                                 vmulq(vandq(G, vecMaskGpck), 8));

    vOut = vorrq(vOut, vmulq(vandq(R, vecMaskRpck), 256));

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

void mve_arm_2d_blur(struct gui_dispdev *dc, gui_rect_t *rect, uint8_t blur_degree, void *cache_mem)
{
    gui_rect_t blur_rect = {0};
    gui_rect_t target_rect = {.x1 = 0, .x2 = dc->screen_width - 1, .y1 = 0, .y2 = dc->screen_height - 1};
    if (!rect_intersect(&blur_rect, rect, &dc->section))
    {
        return;
    }
    if (!rect_intersect(&target_rect, &target_rect, rect))
    {
        return;
    }
    uint16_t *buffer = (uint16_t *)(dc->frame_buf + \
                                    ((blur_rect.y1 - dc->section.y1) * dc->fb_width + blur_rect.x1) * dc->bit_depth / 8);
    arm2d_local_region_t valid, target;
    valid.tLocation.iX = blur_rect.x1;
    valid.tLocation.iY = blur_rect.y1;
    valid.tSize.iWidth = blur_rect.x2 - blur_rect.x1 + 1;
    valid.tSize.iHeight = blur_rect.y2 - blur_rect.y1 + 1;
    target.tLocation.iX = target_rect.x1;
    target.tLocation.iY = target_rect.y1;
    target.tSize.iWidth = target_rect.x2 - target_rect.x1 + 1;
    target.tSize.iHeight = target_rect.y2 - target_rect.y1 + 1;
    arm2d_local_scratch_mem_t local_scratch_mem = {0};
    arm2d_local_scratch_mem_t *mem_for_blur = (arm2d_local_scratch_mem_t *)cache_mem;
    if (cache_mem == NULL)
    {
        mem_for_blur = &local_scratch_mem;
    }
    arm2d_local_filter_iir_blur_descriptor_t dsc = {0};
    dsc.tScratchMemory = *mem_for_blur;
    dsc.bForwardHorizontal = 1;
    dsc.bForwardVertical = 1;
    dsc.bReverseHorizontal = 1;
    dsc.bReverseVertical = 0;
    if (dc->bit_depth == 16)
    {
        arm2d_local_rgb565_filter_iir_blur_mve((uint16_t *)buffer, dc->fb_width, &valid, &target,
                                               blur_degree,
                                               &dsc);
    }
    else if (dc->bit_depth == 32)
    {
        arm2d_local_argb8888_filter_iir_blur_mve((uint32_t *)buffer, dc->fb_width, &valid, &target,
                                                 blur_degree,
                                                 &dsc);
    }
    else if (dc->bit_depth == 24)
    {
        arm2d_local_rgb888_filter_iir_blur_mve((uint32_t *)buffer, dc->fb_width, &valid, &target,
                                               blur_degree,
                                               &dsc);
    }
}

void mve_arm_2d_create(gui_rect_t *rect, void **mem)
{
    if (*mem != NULL)
    {
        return;
    }
    uint16_t w = rect->x2 - rect->x1 + 1;
    uint16_t h = rect->y2 - rect->y1 + 1;
    arm2d_local_scratch_mem_t *scratch_mem = gui_malloc(sizeof(arm2d_local_scratch_mem_t));

    if (scratch_mem != NULL)
    {
        if (NULL ==  arm2d_local_scratch_memory_new(
                scratch_mem,
                sizeof(arm2d_color_rgb565_t),
                (w + h
                 + 14
                ),
                __alignof__(arm2d_color_rgb565_t)))
        {
            memset(scratch_mem, 0, sizeof(arm2d_local_scratch_mem_t));
        }
    }
    *mem = (void *)scratch_mem;
}

void mve_arm_2d_depose(void **mem)
{
    if (*mem != NULL)
    {
        arm2d_local_scratch_memory_free((arm2d_local_scratch_mem_t *)*mem);
    }
    gui_free(*mem);
    *mem = NULL;
}

void mve_arm2d_blur_init(void)
{
    blur_depose = mve_arm_2d_depose;
    blur_prepare = mve_arm_2d_create;
}
#endif
