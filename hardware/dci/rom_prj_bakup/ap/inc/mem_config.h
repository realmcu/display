/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    mem_config.h
  * @brief   Memory Configuration
  * @date    2024.4.28
  * @version v1.0
  * *************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   * *************************************************************************************
  */

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef __MEM_CONFIG__
#define __MEM_CONFIG__

#ifdef __cplusplus
extern "C" {
#endif

#include "..\..\..\..\kernel\soc\rtl87x3j\address_map.h"


/** @defgroup MEM_CONFIG Memory Configure
  * @brief Memory configuration for user application
  * @{
  */
/** @defgroup MEM_CONFIG_Exported_Constant Mem Config Exported Constants
  * @{
  */

// Build Bank Configure
#define BUILD_BANK              0   //Bank0 as default

//Enable RAM Code
#define FEATURE_RAM_CODE        0

/** @brief encrypt app or not */
#define FEATURE_ENCRYPTION      0

//Use Cache RAM to run RAM Code
#define FEATURE_CACHE_RAM_CODE  0

/*============================================================================*
  *                            Memory layout
  *============================================================================*/
//#define EFLASH_ADDR            0x02000000
//#define EFLASH_SIZE            (720*1024)

//#define EFLASH_NVR_ADDR        0x02200000
//#define EFLASH_NVR_SIZE        (34*1024)

//#define EFLASH_NVR_CFG_AREA_ADDR    0x04B00000
//#define EFLASH_NVR_CFG_AREA_SIZE    (2*1024)

#define ITCM_ADDR        0x100000
#define ITCM_SIZE        (192*1024)
#define SPIC0_ADDR        SPIC0_MEM_BASE
#define SPIC0_SIZE        (32*1024*1024)
#define SPIC1_ADDR        SPIC1_MEM_BASE
#define SPIC1_SIZE        (32*1024*1024)
#define SPIC2_ADDR        SPIC2_MEM_BASE
#define SPIC2_SIZE        (32*1024*1024)
#define SPIC3_ADDR        SPIC3_MEM_BASE
#define SPIC3_SIZE        (256*1024*1024)

#define DATA_SRAM0_ADDR  SYS_RAM_BASE
#define DATA_SRAM0_SIZE  (128*1024)

#define DATA_RAM_ADDR    DTCM_ADDR
#define DATA_RAM_SIZE    DTCM_SIZE

#define DSP_SHM_ADDR     DSP_RAM_BASE
#define DSP_SHM_SIZE     (448 * 1024)

#define BUFFER_HEAP_ADDR   DSP_SHM_ADDR
#define BUFFER_HEAP_SIZE   DSP_SHM_SIZE

#define CODE_RAM_ADDR    ITCM_ADDR
#define CODE_RAM_SIZE    ITCM_SIZE

//#define BUFFER_RAM_ADDR       0x20100000
//#define BUFFER_RAM_SIZE       (48*1024) // for fpga test

/*============================================================================*
  *                            Eflash NVR layout
  *============================================================================*/
/* Fflash NVR:                  Bee5 size: 34K
example:
       1) OTP:                   1K
       3) OEM Header:            3K
       4) FTL                    16K
       5) User Data              14K
*/

#define OTP_TABLE_SIZE           (1*1024)
#define OEM_HEADER_SIZE          (3*1024)
#define FTL_SIZE                 (16*1024)

#define OTP_TABLE_ADDR           (EFLASH_NVR_ADDR)
#define OEM_HEADER_ADDR          OTP_TABLE_ADDR + OTP_TABLE_SIZE
#define FTL_ADDR                 OEM_HEADER_ADDR + OEM_HEADER_SIZE
#define USER_DATA_ADDR           FTL_ADDR + FTL_SIZE
#define USER_DATA_SIZE           EFLASH_NVR_ADDR + EFLASH_NVR_SIZE - USER_DATA_ADDR


/*============================================================================*
  *                                 Code RAM layout
  *============================================================================*/
/* Code RAM:                  bb3 size: 192k
example:
       1) main stack:              2K
       2) ram load:                170K
       2) ram test for rom:        20K
*/
#define CODE_RAM_MAIN_STACK_SIZE          (2*1024)
#define CODE_RAM_LOAD_SIZE                (170*1024)
#define CODE_RAM_TEXT_SIZE                (20*1024)

#define CODE_RAM_MAIN_STACK_START_ADDR       (CODE_RAM_ADDR)  // 0x100000
#define CODE_RAM_LOAD_ADDR                   (CODE_RAM_MAIN_STACK_START_ADDR + CODE_RAM_MAIN_STACK_SIZE)  // 0x100800
#define CODE_RAM_TEXT_ADDR                   (CODE_RAM_LOAD_ADDR + CODE_RAM_LOAD_SIZE)

/*============================================================================*
  *                                Data RAM layout
  *============================================================================*/
/* Data RAM:                  Bee5 size: 128K
example:
       1) ram vector table:            10K
       2) heap:                        118K
*/

#define DATA_RAM_VECTOR_TABLE_SIZE              (10*1024)

/* DCI test framebuffer pool, carved off the front of the heap.
 *
 * It cannot live in RW_SECTION: that region is capped at DATA_RAM_VECTOR_TABLE_SIZE
 * (10 KB) by the ScatterAssert in app_ram.sct / app_rom.sct, and one 160x120 RGB565
 * frame already needs 38432 B. 40 KB covers that single-buffered; 80 KB is needed for
 * double buffering at the same size. Set DCI_DEMO_POOL_KB to match.
 *
 * The demo places its pool in section .dci_pool (DCI_DEMO_SECTION in
 * dci_demo_config.h), and the scatter files give that section this region. */
#define DCI_POOL_SIZE                           (40*1024)
#define DCI_POOL_ADDR                           (DATA_RAM_VECTOR_TABLE_ADDR + DATA_RAM_VECTOR_TABLE_SIZE)

/* -----------------The following macros should not be modified!------------------------- */
#define DATA_RAM_VECTOR_TABLE_ADDR           (DATA_RAM_ADDR)  // 0x20000000
/* Heap starts after the DCI pool, so it shrinks by DCI_POOL_SIZE (118 KB -> 78 KB at
 * 40 KB). DATA_HEAP_SIZE is computed from DATA_HEAP_ADDR and follows automatically. */
#define DATA_HEAP_ADDR                       (DCI_POOL_ADDR + DCI_POOL_SIZE)   // 0x2000c800
#define DATA_HEAP_SIZE                       (DATA_RAM_ADDR + DATA_RAM_SIZE - DATA_HEAP_ADDR)

/** @} */ /* End of group MEM_CONFIG_Exported_Constents */

#ifdef __cplusplus
}
#endif

/** @} */ /* End of group MEM_CONFIG */

#endif

