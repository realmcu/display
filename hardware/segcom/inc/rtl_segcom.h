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
typedef enum
{
    BIAS_STATIC = 0x0,
    BIAS_1_2    = 0x1,
    BIAS_1_3    = 0x2,
    BIAS_RSVD   = 0x3,
} SEGCOM_BIAS;

typedef enum
{
    DUTY_STATIC = 0x0,
    DUTY_1_2,
    DUTY_1_3,
    DUTY_1_4,
} SEGCOM_DUTY;

typedef enum
{
    BLINK_DISABLE = 0x0,
    BLINK_ON_SEG0_COM0,
    BLINK_ON_SEG0_COM_ALL,
    BLINK_ON_ALL_SEG_COM,
} SEGCOM_BLINK_MODE;

typedef enum
{
    DEAD_TIME_NONE = 0x0,
    DEAD_TIME_1_PERIOD,
    DEAD_TIME_2_PERIOD,
    DEAD_TIME_3_PERIOD,
    DEAD_TIME_4_PERIOD,
    DEAD_TIME_5_PERIOD,
    DEAD_TIME_6_PERIOD,
    DEAD_TIME_7_PERIOD,
} SEGCOM_DEAD_TIME;

typedef enum
{
    CLK_DIV_16 = 0x0,
    CLK_DIV_17,
    CLK_DIV_18,
    CLK_DIV_19,
    CLK_DIV_20,
    CLK_DIV_21,
    CLK_DIV_22,
    CLK_DIV_23,
    CLK_DIV_24,
    CLK_DIV_25,
    CLK_DIV_26,
    CLK_DIV_27,
    CLK_DIV_28,
    CLK_DIV_29,
    CLK_DIV_30,
    CLK_DIV_31 = 0xF,
} SEGCOM_CLK_DIV;

typedef enum
{
    CLK_PRESCALE_1 = 0x0,
    CLK_PRESCALE_2,
    CLK_PRESCALE_4,
    CLK_PRESCALE_8,
    CLK_PRESCALE_16,
    CLK_PRESCALE_32,
    CLK_PRESCALE_64 = 0x06,
} SEGCOM_CLK_PRESCALER;

typedef enum
{
    BLINK_FREQ_8 = 0x0,
    BLINK_FREQ_16,
    BLINK_FREQ_32,
    BLINK_FREQ_64,
    BLINK_FREQ_128,
    BLINK_FREQ_256,
    BLINK_FREQ_512,
    BLINK_FREQ_1024,
} SEGCOM_BLINK_FREQ_SEL;

typedef enum
{
    SEGCOM_LOW_DRIVING = 0x0,
    SEGCOM_HIGH_DRIVING,
} SEGCOM_VOLTAGE_SWITCH;

typedef enum
{
    SEGCOM_AUTO_MODE = 0x0,
    SEGCOM_MANUAL_MODE,
} SEGCOM_CONTROL_MODE;

typedef enum
{
    HIGH_DURATION_ALL = 0x0,
    HIGH_DURATION_1,
    HIGH_DURATION_2,
    HIGH_DURATION_3,
    HIGH_DURATION_4,
    HIGH_DURATION_5,
    HIGH_DURATION_6,
    HIGH_DURATION_7,
} SEGCOM_HIGH_DURATION;

typedef struct
{
    SEGCOM_BIAS SEG_bias;
    SEGCOM_DUTY SEG_duty;
    SEGCOM_BLINK_MODE SEG_blink_mode;
    SEGCOM_BLINK_FREQ_SEL SEG_blink_freq;
    SEGCOM_DEAD_TIME SEG_dead_time;
    SEGCOM_CLK_DIV SEG_clk_divider;
    SEGCOM_CLK_PRESCALER SEG_clk_prescaler;
    SEGCOM_HIGH_DURATION SEG_high_driv_duration;
    SEGCOM_CONTROL_MODE SEG_driv_ctl_mode;
    SEGCOM_VOLTAGE_SWITCH SEG_set_manual_mode;
} SEGCOM_InitTypeDef;

typedef enum
{
    SEGCOM_DR_0,
    SEGCOM_DR_1,
    SEGCOM_DR_2,
    SEGCOM_DR_3,
} SEGCOM_SEG_CHANNEL;

typedef enum
{
    SEGCOM_STATUS_LCD_ENABLE = 0x0,
    SEGCOM_STATUS_START_OF_FRAME = 0x1,
    SEGCOM_STATUS_UPDATE_DISP_REQ = 0x2,
    SEGCOM_STATUS_UPDATE_DISP_DONE = 0x3,
    SEGCOM_STATUS_FRAME_CTL_REG_SYNC = 0x5,
} SEGCOM_STATUS;

typedef enum
{
    SEGCOM_UPDATE_DISPLAY_DONE_INT,
    SEGCOM_START_OF_FRAME_INT,
} SEGCOM_INTERRUPT;

typedef enum
{
    SEGCOM_CLEAR_START_OF_FRAME,
    SEGCOM_CLEAR_UPDATE_DISPLAY_DONE,
} SEGCOM_CLEAR_STATUS;

typedef union SEGCOM_com_pin_cfg
{
    uint32_t d32;
    struct
    {
        uint32_t com0_p_0_2:    1;
        uint32_t com1_p_0_1:    1;
        uint32_t com2_p_0_0:    1;
        uint32_t com3_p_3_2:    1;
        const uint32_t rsvd:    28;
        } pin;
    } SEGCOM_com_pin_cfg_Typedef;

    typedef union SEGCOM_seg_pin_cfg
        {
            uint32_t d32;
            struct
            {
                uint32_t seg0_p_0_7:        1;
                uint32_t seg1_p_0_6:        1;
                uint32_t seg2_p_0_5:        1;
                uint32_t seg3_p_1_2:        1;
                uint32_t seg4_p_0_4:        1;
                uint32_t seg5_p_1_5:        1;
                uint32_t seg6_p_1_6:        1;
                uint32_t seg7_p_1_7:        1;
                uint32_t seg8_p_3_6:        1;
                uint32_t seg9_p_3_5:        1;
                uint32_t seg10_p_3_4:       1;
                uint32_t seg11_p_3_3:       1;
                uint32_t seg12_p_5_5:       1;
                uint32_t seg13_p_3_7:       1;
                uint32_t seg14_p_4_0:       1;
                uint32_t seg15_p_4_1:       1;
                uint32_t seg16_p_4_2:       1;
                uint32_t seg17_p_4_3:       1;
                uint32_t seg18_p_9_0:       1;
                uint32_t seg19_p_9_1:       1;
                uint32_t seg20_p_9_2:       1;
                uint32_t seg21_p_4_7:       1;
                uint32_t seg22_p_4_6:       1;
                uint32_t seg23_p_4_5:       1;
                uint32_t seg24_p_4_4:       1;
                uint32_t seg25_p_1_4:       1;
                uint32_t seg26_p_1_3:       1;
                uint32_t seg27_p_5_0:       1;
                uint32_t seg28_p_5_1:       1;
                uint32_t seg29_p_5_2:       1;
                uint32_t seg30_p_5_3:       1;
                uint32_t seg31_p_5_4:       1;
            } pin;
        } SEGCOM_seg_pin_cfg_Typedef;

    /** End of SEGCOM_Exported_Constants
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
     * rtl_segcom.h
     *
     * \brief  Enable or disable SEGCOM clock.
     *
     * \param[in] state: New state of SEGCOM clock.
     *            This parameter can be one of the following values:
     *            \arg ENABLE: Enable SEGCOM clock.
     *            \arg DISABLE: Disable SEGCOM clock.
     *
     * \return None.
     *
     * <b>Example usage</b>
     * \code{.c}
     *
     * void driver_segcom_init(void)
     * {
     *     SEGCOM_clk_config(ENABLE);
     * }
     * \endcode
     */
    void SEGCOM_clk_config(FunctionalState state);

    /**
     * rtl_segcom.h
     *
     * \brief  Enable or disable SEGCOM.
     *
     * \param[in] NewState: New state of SEGCOM.
     *            This parameter can be one of the following values:
     *            \arg ENABLE: Enable SEGCOM.
     *            \arg DISABLE: Disable SEGCOM.
     *
     * \return None.
     *
     * <b>Example usage</b>
     * \code{.c}
     *
     * void driver_segcom_init(void)
     * {
     *     SEGCOM_CMD(ENABLE);
     * }
     * \endcode
     */
    void SEGCOM_CMD(FunctionalState state);

    /**
     * rtl_segcom.h
     *
     * \brief  Fill configuration structure with default value.
     *
     * \param[in] cfg: Pointer to configuration structure.
     *
     * \return None.
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
    void SEGCOM_StructInit(SEGCOM_InitTypeDef *cfg);

    /**
     * rtl_segcom.h
     *
     * \brief  Configure specified interrupt.
     *
     * \param[in] interrupt: Specified interrupt type.
     *            This parameter can be one of the following values:
     *            \arg SEGCOM_UPDATE_DISPLAY_DONE_INT: Update display done interrupt.
     *            \arg SEGCOM_START_OF_FRAME_INT: Start of frame interrupt.
     * \param[in] FunctionalState: Specified interrupt type.
     *            \arg ENABLE: Enable interrupt.
     *            \arg DISABLE: Disable interrupt.
     * \return None.
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
    void SEGCOM_IntConfig(SEGCOM_INTERRUPT interrupt, FunctionalState state);

    /**
     * rtl_segcom.h
     *
     * \brief  Get SEGCOM status.
     *
     * \param[in] flag: Specified status type.
     *            This parameter can be one of the following values:
     *            \arg SEGCOM_STATUS_LCD_ENABLE: LCD enable status.
     *            \arg SEGCOM_STATUS_START_OF_FRAME: Start of frame.
     *            \arg SEGCOM_STATUS_UPDATE_DISP_REQ: Update display request.
     *            \arg SEGCOM_STATUS_UPDATE_DISP_DONE: Update display done.
     *            \arg SEGCOM_STATUS_FRAME_CTL_REG_SYNC: Frame control synchronize.
     * \return None.
     *
     * <b>Example usage</b>
     * \code{.c}
     *
     * void driver_segcom_init(void)
     * {
     *     uint32_t status = SEGCOM_GetStatus(SEGCOM_STATUS_LCD_ENABLE);
     *     if(status)
     *     {
     *         //Do something
     *     }
     * }
     * \endcode
     */
    uint32_t SEGCOM_GetStatus(SEGCOM_STATUS flag);

    /**
     * rtl_segcom.h
     *
     * \brief  Clear SEGCOM status.
     *
     * \param[in] status: Specified status type.
     *            This parameter can be one of the following values:
     *            \arg SEGCOM_CLEAR_START_OF_FRAME: Clear start of frame status.
     *            \arg SEGCOM_CLEAR_UPDATE_DISPLAY_DONE: Clear update display done status.
     * \return None.
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
    void SEGCOM_ClearStatus(SEGCOM_CLEAR_STATUS status);

    /**
     * rtl_segcom.h
     *
     * \brief  Send update display request.
     *
     * \param None.
     *
     * \return None.
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
     * rtl_segcom.h
     *
     * \brief  Write value to segment channel.
     *
     * \param[in] channel: Specified channel.
     *            This parameter can be one of the following values:
     *            \arg SEGCOM_DR_0: SEGCOM data register 0.
     *            \arg SEGCOM_DR_1: SEGCOM data register 1.
     *            \arg SEGCOM_DR_2: SEGCOM data register 2.
     *            \arg SEGCOM_DR_3: SEGCOM data register 3.
     * \param[in] value: Value to be displayed
     * \return None.
     *
     * <b>Example usage</b>
     * \code{.c}
     *
     * void driver_segcom_init(void)
     * {
     *     SEGCOM_Write(SEGCOM_DR_0, 0xF0F0F0F0);
     * }
     * \endcode
     */
    void SEGCOM_Write(SEGCOM_SEG_CHANNEL channel, uint32_t value);

    /**
     * rtl_segcom.h
     *
     * \brief  Use the configuration from input parameter to initilize SEGCOM.
     *
     * \param[in] cfg: Pointer to configuration structure.
     *
     * \return None.
     *
     * <b>Example usage</b>
     * \code{.c}
     *
     * void driver_lcdc_init(void)
     * {
     *     SEGCOM_InitTypeDef init_struct;
     *     SEGCOM_StructInit(&init_struct);
     *     SEGCOM_init(&init_struct);
     * }
     * \endcode
     */
    void SEGCOM_init(SEGCOM_InitTypeDef *cfg);

    /**
     * rtl_segcom.h
     *
     * \brief  Send update clock request.
     *
     * \param None.
     *
     * \return None.
     *
     * <b>Example usage</b>
     * \code{.c}
     *
     * void driver_segcom_init(void)
     * {
     *     SEGCOM_UpdateClk();
     * }
     * \endcode
     */
    void SEGCOM_UpdateClk(void);

    /**
     * rtl_segcom.h
     *
     * \brief  Determine which pins to be used as SEGs and COMs.
     *
     * \param[in] com_cfg: Determine COM pins
     * \param[in] seg_cfg: Determine SEG pins
     * \return None.
     *
     * <b>Example usage</b>
     * \code{.c}
     *
     * void driver_segcom_init(void)
     * {
     *     SEGCOM_com_pin_cfg_Typedef coms;
     *     memset(coms, 0, sizeof(SEGCOM_com_pin_cfg_Typedef);
     *     SEGCOM_seg_pin_cfg_Typedef segs;
     *     memset(segs, 0, sizeof(SEGCOM_seg_pin_cfg_Typedef);
     *     coms.pin.com0_p_0_2 = 1;
     *     segs.pin.seg17_p_4_3 = 1;
     *     SEGCOM_Pin_Init(coms, segs);
     * }
     * \endcode
     */
    void SEGCOM_Pin_Init(SEGCOM_com_pin_cfg_Typedef com_cfg, SEGCOM_seg_pin_cfg_Typedef seg_cfg);

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
