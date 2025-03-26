/**
*********************************************************************************************************
*               Copyright(c) 2023, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* \file     rtl_segcom.h
* \brief    This file provides all the SEG_COM controller firmware functions.
* \details
* \author   astor zhang
* \date     2022-07-29
* \version  v0.1
*********************************************************************************************************
*/

/*============================================================================*
 *               Define to prevent recursive inclusion
 *============================================================================*/
#ifndef RTL_SEGCOM_H
#define RTL_SEGCOM_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include "utils/rtl_utils.h"
#if defined (CONFIG_SOC_SERIES_RTL87X2G)
#include "segcom/src/rtl87x2g/rtl_segcom_def.h"
#endif

/** \defgroup SEGCOM      SEGCOM
  * \brief
  * \{
  */
/*============================================================================*
 *                         Constants
 *============================================================================*/
/** \defgroup SEGCOM_Exported_Constants SEGCOM Exported Constants
  * \brief
  * \{
  */

/**
 * \defgroup    SEGCOM_BIAS SEGCOM Bias
 * \{
 * \ingroup     SEGCOM_Exported_Constants
 */
typedef enum
{
    BIAS_STATIC = 0x0,    /*!< No bias voltage. */
    BIAS_1_2    = 0x1,    /*!< A bias of 1/2 means that the voltage of the common electrode is at the midpoint of the drive voltage.*/
    BIAS_1_3    = 0x2,    /*!< A bias of 1/3 means that the voltage of the common electrode is 1/3 and 2/3 of the drive voltage.*/
    BIAS_RSVD   = 0x3,    /*!< Reserved. */
} SEGCOM_BIAS;
/** End of SEGCOM_BIAS
  * \}
  */

/**
 * \defgroup    SEGCOM_DUTY SEGCOM Duty Cycle
 * \{
 * \ingroup     SEGCOM_Exported_Constants
 */
typedef enum
{
    DUTY_STATIC = 0x0,    /*!< No duty cycle, for only 1 COM. */
    DUTY_1_2,             /*!< A duty cycle of 1/2 supports 2 COMs. */
    DUTY_1_3,             /*!< A duty cycle of 1/3 supports 3 COMs. */
    DUTY_1_4,             /*!< A duty cycle of 1/4 supports 4 COMs. */
} SEGCOM_DUTY;
/** End of SEGCOM_DUTY
  * \}
  */

/**
 * \defgroup    SEGCOM_BLINK_MODE SEGCOM Blink Mode
 * \{
 * \ingroup     SEGCOM_Exported_Constants
 */
typedef enum
{
    BLINK_DISABLE = 0x0,      /*!< Blink mode disabled. */
    BLINK_ON_SEG0_COM0,       /*!< Blink on COM0 together with SEGMENT0. */
    BLINK_ON_SEG0_COM_ALL,    /*!< Blink on all COMs together with SEGMENT0. */
    BLINK_ON_ALL_SEG_COM,     /*!< Blink on all COMs and all SEGMENTs. */
} SEGCOM_BLINK_MODE;
/** End of SEGCOM_BLINK_MODE
  * \}
  */

/**
 * \defgroup    SEGCOM_DEAD_TIME SEGCOM Dead Time
 * \{
 * \ingroup     SEGCOM_Exported_Constants
 */
typedef enum
{
    DEAD_TIME_NONE = 0x0,     /*!< No dead time. */
    DEAD_TIME_1_PERIOD,       /*!< Dead time of 1 phase period. */
    DEAD_TIME_2_PERIOD,       /*!< Dead time of 2 phase periods. */
    DEAD_TIME_3_PERIOD,       /*!< Dead time of 3 phase periods. */
    DEAD_TIME_4_PERIOD,       /*!< Dead time of 4 phase periods. */
    DEAD_TIME_5_PERIOD,       /*!< Dead time of 5 phase periods. */
    DEAD_TIME_6_PERIOD,       /*!< Dead time of 6 phase periods. */
    DEAD_TIME_7_PERIOD,       /*!< Dead time of 7 phase periods. */
} SEGCOM_DEAD_TIME;
/** End of SEGCOM_DEAD_TIME
  * \}
  */

/**
 * \defgroup    SEGCOM_CLK_PRESCALER SEGCOM Clock Prescaler
 * \{
 * \ingroup     SEGCOM_Exported_Constants
 */
typedef enum
{
    CLK_PRESCALE_1 = 0x0,   /*!< Clock pre-scaler factor is 1. */
    CLK_PRESCALE_2,          /*!< Clock pre-scaler factor is 2. */
    CLK_PRESCALE_4,          /*!< Clock pre-scaler factor is 4. */
    CLK_PRESCALE_8,          /*!< Clock pre-scaler factor is 8. */
    CLK_PRESCALE_16,         /*!< Clock pre-scaler factor is 16. */
    CLK_PRESCALE_32,         /*!< Clock pre-scaler factor is 32. */
    CLK_PRESCALE_64 = 0x06,  /*!< Clock pre-scaler factor is 64. */
} SEGCOM_CLK_PRESCALER;
/** End of SEGCOM_CLK_PRESCALER
  * \}
  */

/**
 * \defgroup    SEGCOM_CLK_DIV SEGCOM Clock Divider
 * \{
 * \ingroup     SEGCOM_Exported_Constants
 */
typedef enum
{
    CLK_DIV_16 = 0x0,    /*!< Clock divider = 16. */
    CLK_DIV_17,           /*!< Clock divider = 17. */
    CLK_DIV_18,           /*!< Clock divider = 18. */
    CLK_DIV_19,           /*!< Clock divider = 19. */
    CLK_DIV_20,           /*!< Clock divider = 20. */
    CLK_DIV_21,           /*!< Clock divider = 21. */
    CLK_DIV_22,           /*!< Clock divider = 22. */
    CLK_DIV_23,           /*!< Clock divider = 23. */
    CLK_DIV_24,           /*!< Clock divider = 24. */
    CLK_DIV_25,           /*!< Clock divider = 25. */
    CLK_DIV_26,           /*!< Clock divider = 26. */
    CLK_DIV_27,           /*!< Clock divider = 27. */
    CLK_DIV_28,           /*!< Clock divider = 28. */
    CLK_DIV_29,           /*!< Clock divider = 29. */
    CLK_DIV_30,           /*!< Clock divider = 30. */
    CLK_DIV_31 = 0xF,     /*!< Clock divider = 31. */
} SEGCOM_CLK_DIV;
/** End of SEGCOM_CLK_DIV
  * \}
  */

/**
 * \defgroup    SEGCOM_BLINK_FREQ_SEL SEGCOM Blink Frequency Divider
 * \{
 * \ingroup     SEGCOM_Exported_Constants
 */
typedef enum
{
    BLINK_FREQ_DIV_8 = 0x0,  /*!< Blink frequency divider is 8. */
    BLINK_FREQ_DIV_16,       /*!< Blink frequency divider is 16. */
    BLINK_FREQ_DIV_32,       /*!< Blink frequency divider is 32. */
    BLINK_FREQ_DIV_64,       /*!< Blink frequency divider is 64. */
    BLINK_FREQ_DIV_128,      /*!< Blink frequency divider is 128. */
    BLINK_FREQ_DIV_256,      /*!< Blink frequency divider is 256. */
    BLINK_FREQ_DIV_512,      /*!< Blink frequency divider is 512. */
    BLINK_FREQ_DIV_1024,     /*!< Blink frequency divider is 1024. */
} SEGCOM_BLINK_FREQ_SEL;
/** End of SEGCOM_BLINK_FREQ_SEL
  * \}
  */

/**
 * \defgroup    SEGCOM_VOLTATE_SWITCH SEGCOM High Driving Voltage Switch
 * \{
 * \ingroup     SEGCOM_Exported_Constants
 */
typedef enum
{
    SEGCOM_LOW_DRIVING = 0x0,    /*!< Manual mode set driving voltage low level. */
    SEGCOM_HIGH_DRIVING,         /*!< Manual mode set driving voltage high level. */
} SEGCOM_VOLTAGE_SWITCH;
/** End of SEGCOM_VOLTAGE_SWITCH
  * \}
  */

/**
 * \defgroup    SEGCOM_CONTROL_MODE SEGCOM High Driving Control Mode
 * \{
 * \ingroup     SEGCOM_Exported_Constants
 */
typedef enum
{
    SEGCOM_AUTO_MODE = 0x0,     /*!< High driving control auto mode. */
    SEGCOM_MANUAL_MODE,         /*!< High driving control manual mode. */
} SEGCOM_CONTROL_MODE;
/** End of SEGCOM_CONTROL_MODE
  * \}
  */

/**
 * \defgroup    SEGCOM_HIGH_DURATION SEGCOM High Driving Duration
 * \{
 * \ingroup     SEGCOM_Exported_Constants
 */
typedef enum
{
    HIGH_DURATION_0 = 0x0,      /*!< No high driving. */
    HIGH_DURATION_1,            /*!< High driving duration for 1 clock pulse width. */
    HIGH_DURATION_2,            /*!< High driving duration for 2 clock pulse width. */
    HIGH_DURATION_3,    /*!< High driving duration for 3 clock pulse width. */
    HIGH_DURATION_4,    /*!< High driving duration for 4 clock pulse width. */
    HIGH_DURATION_5,    /*!< High driving duration for 5 clock pulse width. */
    HIGH_DURATION_6,    /*!< High driving duration for 6 clock pulse width. */
    HIGH_DURATION_7,    /*!< High driving duration for 7 clock pulse width. */
} SEGCOM_HIGH_DURATION;
/** End of SEGCOM_HIGH_DURATION
  * \}
  */

/**
 * \defgroup    SEGCOM_COM_NUM SEGCOM COM Number
 * \{
 * \ingroup     SEGCOM_Exported_Constants
 */
typedef enum
{
    SEGCOM_RAM_COM0,          /*!< SEGMENT data with COM0, each bit corresponding to one pixel of LCD panel. */
    SEGCOM_RAM_COM1,          /*!< SEGMENT data with COM1, each bit corresponding to one pixel of LCD panel. */
    SEGCOM_RAM_COM2,          /*!< SEGMENT data with COM2, each bit corresponding to one pixel of LCD panel. */
    SEGCOM_RAM_COM3,          /*!< SEGMENT data with COM3, each bit corresponding to one pixel of LCD panel. */
} SEGCOM_COM_NUM;
/** End of SEGCOM_COM_NUM
  * \}
  */

/**
 * \defgroup    SEGCOM_STATUS SEGCOM Event Status
 * \{
 * \ingroup     SEGCOM_Exported_Constants
 */
typedef enum
{
    SEGCOM_STATUS_LCD_ENABLE = 0x0,                 /*!< Status of LCD controller. */
    SEGCOM_STATUS_START_OF_FRAME = 0x1,            /*!< Start of frame event occured and corresponding interrupt will be raised if enabled. */
    SEGCOM_STATUS_UPDATE_DISP_REQ = 0x2,            /*!< Update display request status. When the display is disabled, the update is performed for all display region.
                                                        When the display is enabled, the update is performed only for locations for which commons are active (depending on DUTY).
                                                        For example if DUTY = 1/2, only the COM0 and COM1 will be updated. */
    SEGCOM_STATUS_UPDATE_DISP_DONE = 0x3,           /*!< Update display done event occured and corresponding interrupt will be raised if enabled. */
    SEGCOM_STATUS_FRAME_CTL_REG_SYNC = 0x5,        /*!< LCD Frame Control Register Synchronization flag. This bit is set by hardware each time the clock domain is changed.
                                                        It is cleared by hardware when writing change to clock domain and set after synchronization finishes. */
} SEGCOM_STATUS;
/** End of SEGCOM_STATUS
  * \}
  */

/**
 * \defgroup    SEGCOM_INTERRUPT SEGCOM Interrupts
 * \{
 * \ingroup     SEGCOM_Exported_Constants
 */
typedef enum
{
    SEGCOM_UPDATE_DISPLAY_DONE_INT,         /*!< Update display done interrupt. */
    SEGCOM_START_OF_FRAME_INT,              /*!< Start of frame interrupt. */
} SEGCOM_INTERRUPT;
/** End of SEGCOM_INTERRUPT
  * \}
  */

/**
 * \defgroup    SEGCOM_CLEAR_STATUS SEGCOM Clear Status
 * \{
 * \ingroup     SEGCOM_Exported_Constants
 */
typedef enum
{
    SEGCOM_CLEAR_START_OF_FRAME,            /*!< Clear start of frame event status. */
    SEGCOM_CLEAR_UPDATE_DISPLAY_DONE,    /*!< Clear update display done event status. */
} SEGCOM_CLEAR_STATUS;
/** End of SEGCOM_CLEAR_STATUS
  * \}
  */

/** End of SEGCOM_Exported_Constants
  * \}
  */

/*============================================================================*
 *                                Types
 *============================================================================*/
/** \defgroup SEGCOM_Exported_Types SEGCOM Exported Types
  * \brief
  * \{
  */
/**
 * \defgroup    SEGCOM_INIT_STRUCT SEGCOM Initialization Structure
 * \{
 * \ingroup     SEGCOM_Exported_Constants
 */
typedef struct
{
    SEGCOM_BIAS SEG_bias;   /*!< Bias of LCD segment display, can be a value of \ref SEGCOM_BIAS. */
    SEGCOM_DUTY SEG_duty;   /*!< Duty cycle of each COM signal, can be a value of \ref SEGCOM_DUTY. */
    SEGCOM_BLINK_MODE SEG_blink_mode;  /*!< Blink mode, can be a value of \ref SEGCOM_BLINK_MODE . */
    SEGCOM_CLK_PRESCALER
    SEG_clk_prescaler; /*!< Clock prescaler, Fclk_ps = display clock / prescaler, can be a value of \ref SEGCOM_CLK_PRESCALER. */
    SEGCOM_CLK_DIV
    SEG_clk_divider;   /*!< Clock divider, Fclk_div = Fclk_ps / divider, can be a value of \ref SEGCOM_CLK_DIV. */
    SEGCOM_BLINK_FREQ_SEL
    SEG_blink_freq_div; /*!< Blink frequency divider, Fblink = Fclk_div / blink divider, can be a value of \ref SEGCOM_BLINK_FREQ_SEL. */
    SEGCOM_DEAD_TIME SEG_dead_time;  /*!< Length of the dead time between frames.
                                          During the dead time the COM and SEG voltage levels are held at 0 V to reduce the contrast.
                                          This member can be a value of \ref SEGCOM_DEAD_TIME . */
    SEGCOM_HIGH_DURATION
    SEG_high_driv_duration;  /*!< Use low impedence to achieve fast settling, can be a value of \ref SEGCOM_HIGH_DURATION . */
    SEGCOM_CONTROL_MODE
    SEG_driv_ctl_mode;  /*!< High driving control mode, can be a value of \ref SEGCOM_CONTROL_MODE. */
} SEGCOM_InitTypeDef;
/** End of SEGCOM_INIT_STRUCT
* \}
*/

/**
 * \defgroup    SEGCOM_COM_PIN_CFG SEGCOM Pin Configuration of COMs
 * \{
 * \ingroup     SEGCOM_Exported_Constants
 */
typedef struct SEGCOM_com_pin_cfg
{
    uint32_t com0_p_0_2:    1;        /*!< Enable P0_2 as COM0. */
    uint32_t com1_p_0_1:    1;        /*!< Enable P0_1 as COM1. */
    uint32_t com2_p_0_0:    1;        /*!< Enable P0_0 as COM2. */
    uint32_t com3_p_3_2:    1;        /*!< Enable P3_2 as COM3. */
    const uint32_t rsvd:    28;       /*!< Reserved. */
    } SEGCOM_COM_Pin_CFG_Typedef;
    /** End of SEGCOM_COM_PIN_CFG
      * \}
      */

    /**
     * \defgroup    SEGCOM_SEG_PIN_CFG SEGCOM Pin Configuration of SEGs
     * \{
     * \ingroup     SEGCOM_Exported_Constants
     */
    typedef struct SEGCOM_seg_pin_cfg
{
    uint32_t seg0_p_0_7:        1;        /*!< Enable P0_7 as SEG0. */
    uint32_t seg1_p_0_6:        1;        /*!< Enable P0_6 as SEG1. */
    uint32_t seg2_p_0_5:        1;        /*!< Enable P0_5 as SEG2. */
    uint32_t seg3_p_1_2:        1;        /*!< Enable P1_2 as SEG3. */
    uint32_t seg4_p_0_4:        1;        /*!< Enable P0_4 as SEG4. */
    uint32_t seg5_p_1_5:        1;        /*!< Enable P1_5 as SEG5. */
    uint32_t seg6_p_1_6:        1;        /*!< Enable P1_6 as SEG6. */
    uint32_t seg7_p_1_7:        1;        /*!< Enable P1_7 as SEG7. */
    uint32_t seg8_p_3_6:        1;        /*!< Enable P3_6 as SEG8. */
    uint32_t seg9_p_3_5:        1;        /*!< Enable P3_5 as SEG9. */
    uint32_t seg10_p_3_4:       1;        /*!< Enable P3_4 as SEG10. */
    uint32_t seg11_p_3_3:       1;        /*!< Enable P3_3 as SEG11. */
    uint32_t seg12_p_5_5:       1;        /*!< Enable P5_5 as SEG12. */
    uint32_t seg13_p_3_7:       1;        /*!< Enable P3_7 as SEG13. */
    uint32_t seg14_p_4_0:       1;        /*!< Enable P4_0 as SEG14. */
    uint32_t seg15_p_4_1:       1;        /*!< Enable P4_1 as SEG15. */
    uint32_t seg16_p_4_2:       1;        /*!< Enable P4_2 as SEG16. */
    uint32_t seg17_p_4_3:       1;        /*!< Enable P4_3 as SEG17. */
    uint32_t seg18_p_9_0:       1;        /*!< Enable P9_0 as SEG18. */
    uint32_t seg19_p_9_1:       1;        /*!< Enable P9_1 as SEG19. */
    uint32_t seg20_p_9_2:       1;        /*!< Enable P9_2 as SEG20. */
    uint32_t seg21_p_4_7:       1;        /*!< Enable P4_7 as SEG21. */
    uint32_t seg22_p_4_6:       1;        /*!< Enable P4_6 as SEG22. */
    uint32_t seg23_p_4_5:       1;        /*!< Enable P4_5 as SEG23. */
    uint32_t seg24_p_4_4:       1;        /*!< Enable P4_4 as SEG24. */
    uint32_t seg25_p_1_4:       1;        /*!< Enable P1_4 as SEG25. */
    uint32_t seg26_p_1_3:       1;        /*!< Enable P1_3 as SEG26. */
    uint32_t seg27_p_5_0:       1;        /*!< Enable P5_0 as SEG27. */
    uint32_t seg28_p_5_1:       1;        /*!< Enable P5_1 as SEG28. */
    uint32_t seg29_p_5_2:       1;        /*!< Enable P5_2 as SEG29. */
    uint32_t seg30_p_5_3:       1;        /*!< Enable P5_3 as SEG30. */
    uint32_t seg31_p_5_4:       1;        /*!< Enable P5_4 as SEG31. */
} SEGCOM_SEG_Pin_CFG_Typedef;
/** End of SEGCOM_SEG_PIN_CFG
  * \}
  */

/** End of SEGCOM_Exported_Types
  * \}
  */


/*============================================================================*
    *                         Functions
    *============================================================================*/
/** \defgroup SEGCOM_Exported_Functions SEGCOM Exported Functions
     * \brief
     * \{
     */

/**
 * \brief  Enable or disable SEGCOM clock.
 *
 * \param[in]  NewState: New state of SEGCOM clock.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable SEGCOM clock.
 *            - DISABLE: Disable SEGCOM clock.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_segcom_init(void)
 * {
 *     SEGCOM_CLK_Config(ENABLE);
 * }
 * \endcode
 */
void SEGCOM_CLK_Config(FunctionalState NewState);

/**
 * \brief  Enable or disable SEGCOM.
 *
 * \param[in] NewState: New state of SEGCOM.
 *            This parameter can be one of the following values:
 *            - ENABLE: Enable SEGCOM.
 *            - DISABLE: Disable SEGCOM.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_segcom_init(void)
 * {
 *     SEGCOM_Cmd(ENABLE);
 * }
 * \endcode
 */
void SEGCOM_Cmd(FunctionalState NewState);

/**
 * \brief  Fill configuration structure with default value.
 *
 * \param[in] SEGCOM_InitStruct: Pointer to configuration structure to be initialized.
 *
 * | Parameter              | Default Value           |
 * |------------------------|-------------------------|
 * | SEG_bias               | \ref BIAS_STATIC        |
 * | SEG_duty               | \ref DUTY_STATIC        |
 * | SEG_blink_mode         | \ref BLINK_DISABLE      |
 * | SEG_blink_freq_div     | \ref BLINK_FREQ_DIV_8   |
 * | SEG_dead_time          | \ref DEAD_TIME_NONE     |
 * | SEG_driv_ctl_mode      | \ref SEGCOM_AUTO_MODE   |
 * | SEG_high_driv_duration | \ref HIGH_DURATION_0    |
 * | SEG_clk_divider        | \ref CLK_DIV_16         |
 * | SEG_clk_prescaler      | \ref CLK_PRESCALE_1     |
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     SEGCOM_InitTypeDef init_struct;
 *     SEGCOM_StructInit(&init_struct);
 * }
 * \endcode
 */
void SEGCOM_StructInit(SEGCOM_InitTypeDef *SEGCOM_InitStruct);

/**
 * \brief  Configure specified interrupt.
 *
 * \param[in] interrupt: Specified interrupt type.
 *            This parameter can be one of the following values:
 *            - SEGCOM_UPDATE_DISPLAY_DONE_INT: Update display done interrupt.
 *            - SEGCOM_START_OF_FRAME_INT: Start of frame interrupt.
 * \param[in] NewState: Specified interrupt type.
 *            - ENABLE: Enable interrupt.
 *            - DISABLE: Disable interrupt.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_segcom_init(void)
 * {
 *     SEGCOM_IntConfig(SEGCOM_UPDATE_DISPLAY_DONE_INT, ENABLE);
 * }
 * \endcode
 */
void SEGCOM_IntConfig(SEGCOM_INTERRUPT Interrupt, FunctionalState NewState);

/**
 * \brief  Get SEGCOM status.
 *
 * \param[in] Flag: Specified status type.
 *            This parameter can be one of the following values:
 *            - SEGCOM_STATUS_LCD_ENABLE: LCD enable status.
 *            - SEGCOM_STATUS_START_OF_FRAME: Start of frame.
 *            - SEGCOM_STATUS_UPDATE_DISP_REQ: Update display request.
 *            - SEGCOM_STATUS_UPDATE_DISP_DONE: Update display done.
 *            - SEGCOM_STATUS_FRAME_CTL_REG_SYNC: Frame control synchronize.
 * \return Current event status
 * \retval SET    Event has occured.
 * \retval RESET  No such event occured.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_segcom_init(void)
 * {
 *     uint32_t status = SEGCOM_GetStatus(SEGCOM_STATUS_LCD_ENABLE);
 *     if(status)
 *     {
 *         //User operation
 *     }
 * }
 * \endcode
 */
ITStatus SEGCOM_GetStatus(SEGCOM_STATUS Flag);

/**
 * \brief  Clear SEGCOM status.
 *
 * \param[in] Status: Specified status type.
 *            This parameter can be one of the following values:
 *            - SEGCOM_CLEAR_START_OF_FRAME: Clear start of frame status.
 *            - SEGCOM_CLEAR_UPDATE_DISPLAY_DONE: Clear update display done status.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_segcom_init(void)
 * {
 *     SEGCOM_ClearStatus(SEGCOM_CLEAR_START_OF_FRAME);
 * }
 * \endcode
 */
void SEGCOM_ClearStatus(SEGCOM_CLEAR_STATUS Status);

/**
 * \brief  Write value to specified COM.
 *
 * \param[in] Channel: Specified COM number.
 *            This parameter can be one of the following values:
 *            - SEGCOM_RAM_COM0: SEGCOM data register for COM0.
 *            - SEGCOM_RAM_COM1: SEGCOM data register for COM1.
 *            - SEGCOM_RAM_COM2: SEGCOM data register for COM2.
 *            - SEGCOM_RAM_COM3: SEGCOM data register for COM3.
 * \param[in] Value: Value to be written.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_segcom_init(void)
 * {
 *     SEGCOM_Write(SEGCOM_RAM_COM0, 0xF0F0F0F0);
 * }
 * \endcode
 */
void SEGCOM_Write(SEGCOM_COM_NUM Channel, uint32_t Value);

/**
 * \brief  Send update display request.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_segcom_init(void)
 * {
 *     SEGCOM_UpdateDisplayRequest();
 * }
 * \endcode
 */
void SEGCOM_UpdateDisplayRequest(void);

/**
 * \brief  Use the configuration from input parameter to initilize SEGCOM.
 *
 * \param[in] SEGCOM_InitStruct: Pointer to configuration structure.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_lcdc_init(void)
 * {
 *     SEGCOM_InitTypeDef init_struct;
 *     SEGCOM_StructInit(&init_struct);
 *     SEGCOM_Init(&init_struct);
 * }
 * \endcode
 */
void SEGCOM_Init(SEGCOM_InitTypeDef *SEGCOM_InitStruct);

/**
 * \brief  Send update clock request.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_segcom_init(void)
 * {
 *     SEGCOM_Update_Clk();
 * }
 * \endcode
 */
void SEGCOM_Update_Clk(void);

/**
 * \brief  Determine which pins to be used as SEGs and COMs.
 *
 * \param[in] com_cfg: Determine COM pins configuration.
 * \param[in] seg_cfg: Determine SEG pins configuration.
 *
 * <b>Example usage</b>
 * \code{.c}
 *
 * void driver_segcom_init(void)
 * {
 *     SEGCOM_COM_Pin_CFG_Typedef coms = {0};
 *     SEGCOM_SEG_Pin_CFG_Typedef segs = {0};
 *
 *     coms.com0_p_0_2 = 1;
 *     segs.seg17_p_4_3 = 1;
 *
 *     SEGCOM_Pin_Init(coms, segs);
 * }
 * \endcode
 */
void SEGCOM_Pin_Init(SEGCOM_COM_Pin_CFG_Typedef com_cfg, SEGCOM_SEG_Pin_CFG_Typedef seg_cfg);

/** End of SEGCOM_Exported_Functions
     * \}
     */

/** End of SEGCOM
     * \}
     */

#ifdef __cplusplus
}
#endif

#endif /* RTL_SEGCOM_H */

/******************* (C) COPYRIGHT 2023 Realtek Semiconductor Corporation *****END OF FILE****/
