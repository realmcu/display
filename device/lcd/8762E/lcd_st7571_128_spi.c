/*
 * Copyright(c) 2025, Realtek Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "board.h"
#include "app_section.h"
#include "rtl876x_spi.h"
#include "trace.h"
#include "lcd_st7571_128_spi.h"
#include "platform_utils.h"
#include "rtl876x_rcc.h"
#include "rtl876x_tim.h"
#include "rtl876x_gpio.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_gdma.h"
#include "rtl876x_nvic.h"
#include "flash_device.h"

DATA_RAM_FUNCTION
void st7571_write_cmd(uint8_t command)
{
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_BUSY));
    /* switch to CS0 signal */
    SPI_SetCSNumber(LCD_SPI_BUS, 0);
    GPIO_ResetBits(LCD_SPI_DC_PIN);
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_TFE) == false);
    SPI_SendData(LCD_SPI_BUS, command);

    /* Waiting for SPI data transfer to end */
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_BUSY));
    GPIO_SetBits(LCD_SPI_DC_PIN);
}

DATA_RAM_FUNCTION
void st7571_write_data(uint8_t data)
{
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_TFE) == false);
    SPI_SendData(LCD_SPI_BUS, data);

    /* Waiting for SPI data transfer to end */
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_BUSY)); //Howie todo, remove later
}

void lcd_pad_init(void)
{
    Pinmux_Config(LCD_SPI_CLK, LCD_SPI_FUNC_CLK);
    Pinmux_Config(LCD_SPI_MOSI, LCD_SPI_FUNC_MOSI);
    Pinmux_Config(LCD_SPI_CS, LCD_SPI_FUNC_CS);

    Pinmux_Config(LCD_SPI_DC, DWGPIO);

    Pad_Config(LCD_SPI_CLK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_DC, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);

}
void lcd_enter_dlps(void)
{
    Pad_Config(LCD_SPI_CLK, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(LCD_SPI_MOSI, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pad_Config(LCD_SPI_CS, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(LCD_SPI_DC, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
}
void lcd_exit_dlps(void)
{
    Pad_Config(LCD_SPI_CLK, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    Pad_Config(LCD_SPI_MOSI, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE,
               PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_CS, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(LCD_SPI_DC, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
}


/**
  * @brief  writband lcd device init set IO config here
  * @param  None
  * @retval None
  */
void lcd_device_init(void)
{
    lcd_pad_init();

    /* Open timer clock */
    RCC_PeriphClockCmd(APBPeriph_TIMER, APBPeriph_TIMER_CLOCK, ENABLE);
    TIM_TimeBaseInitTypeDef TIM_InitStruct;
    TIM_StructInit(&TIM_InitStruct);
    TIM_InitStruct.TIM_PWM_En = PWM_ENABLE;
    /* Set period */
    TIM_InitStruct.TIM_Period = 10 - 1 ;
    /* Set PWM high count and low count */
    TIM_InitStruct.TIM_PWM_High_Count = 900 - 1 ;
    TIM_InitStruct.TIM_PWM_Low_Count = 100 - 1 ;
    /* Set timer mode User_define */
    TIM_InitStruct.TIM_Mode = TIM_Mode_UserDefine;
    /* Set timer clock source divider 40, timer pclk = 40M/4 */
    TIM_InitStruct.TIM_SOURCE_DIV = TIM_CLOCK_DIVIDER_40;
    TIM_TimeBaseInit(BL_PWM_TIM, &TIM_InitStruct);
    TIM_Cmd(BL_PWM_TIM, DISABLE);

    RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, ENABLE);
    SPI_DeInit(LCD_SPI_BUS);
    RCC_PeriphClockCmd(LCD_SPI_APBPeriph, LCD_SPI_APBClock, ENABLE);
    SPI_InitTypeDef  SPI_InitStructure;

    SPI_StructInit(&SPI_InitStructure);
    SPI_InitStructure.SPI_Direction   = SPI_Direction_FullDuplex;
    SPI_InitStructure.SPI_Mode        = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize    = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL        = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA        = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler  = 4;
    SPI_InitStructure.SPI_FrameFormat = SPI_Frame_Motorola;
    SPI_InitStructure.SPI_NDF         = 0;
    SPI_InitStructure.SPI_TxWaterlevel = 30;
    SPI_InitStructure.SPI_TxDmaEn = ENABLE;

    SPI_Init(LCD_SPI_BUS, &SPI_InitStructure);
    SPI_Cmd(LCD_SPI_BUS, ENABLE);

    RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_ControlMode = GPIO_SOFTWARE_MODE;
    GPIO_InitStruct.GPIO_Pin  = GPIO_GetPin(LCD_SPI_DC);
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_ITCmd = DISABLE;
    GPIO_Init(&GPIO_InitStruct);
    GPIO_SetBits(GPIO_GetPin(LCD_SPI_BL));
}

void rtk_lcd_hal_transfer_done(void)
{
    while (GDMA_GetTransferINTStatus(LCD_DMA_CHANNEL_NUM) != SET);
    GDMA_ClearINTPendingBit(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer);
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_TFE) == false);
}

static void lcd_address(uint8_t page, uint8_t column)
{
    // DBG_DIRECT("lcd_address page : %d, column %d", page, column);
    st7571_write_cmd(0xB0 + page);
    st7571_write_cmd(((column >> 4) & 0x0f) + 0x10);
    st7571_write_cmd(column & 0x0f);
}

// Function to clear a specific page and fill it with the given color
static void fill_page(uint8_t page, uint8_t color)
{
    if (page >= ST7571_PAGE_COUNT)
    {
        return; // Return if the page address is invalid
    }

    color &= 0x03; // Ensure the color is within the 0-3 range

    uint8_t high_byte = 0, low_byte = 0;

    // Determine high_byte and low_byte based on the color value
    switch (color)
    {
    case 0: // White
        high_byte = 0b00000000;
        low_byte = 0b00000000;
        break;
    case 1: // Light Gray
        high_byte = 0b00000000;
        low_byte = 0b11111111;
        break;
    case 2: // Dark Gray
        high_byte = 0b11111111;
        low_byte = 0b00000000;
        break;
    case 3: // Black
        high_byte = 0b11111111;
        low_byte = 0b11111111;
        break;
    default:
        return; // This should not happen due to bitwise operation
    }

    // Set the address of the page to be cleared
    lcd_address(page, 0);

    // Loop through the columns to write the color data
    for (uint8_t i = 0; i < ST7571_LCD_WIDTH; i++)
    {
        st7571_write_data(high_byte); // Write high byte
        st7571_write_data(low_byte); // Write low byte
    }
}

// Function to clear the entire screen and fill it with the given color
static void fill_screen(uint8_t color)
{
    for (uint8_t j = 0; j < ST7571_PAGE_COUNT; j++)
    {
        fill_page(j, color);
    }
}

// Function to clear a specific rectangle and fill it with the given color
static void clear_rect(uint8_t start_page, uint8_t start_column, uint8_t width, uint8_t height,
                       uint8_t color)
{
    // Ensure start_page and start_column are within valid ranges
    if (start_page >= ST7571_PAGE_COUNT || start_column >= ST7571_LCD_WIDTH)
    {
        return;
    }

    // Ensure the end positions are within valid ranges
    uint8_t end_page = start_page + height;
    uint8_t end_column = start_column + width;

    if (end_page > ST7571_PAGE_COUNT) { end_page = ST7571_PAGE_COUNT; }
    if (end_column > ST7571_LCD_WIDTH) { end_column = ST7571_LCD_WIDTH; }

    color &= 0x03; // Ensure the color is within the 0-3 range

    uint8_t high_byte = 0, low_byte = 0;

    // Determine high_byte and low_byte based on the color value
    switch (color)
    {
    case 0: // White
        high_byte = 0b00000000;
        low_byte = 0b00000000;
        break;
    case 1: // Light Gray
        high_byte = 0b00000000;
        low_byte = 0b11111111;
        break;
    case 2: // Dark Gray
        high_byte = 0b11111111;
        low_byte = 0b00000000;
        break;
    case 3: // Black
        high_byte = 0b11111111;
        low_byte = 0b11111111;
        break;
    default:
        return; // This should not happen due to bitwise operation
    }

    // Loop through each page and column within the specified rectangle and write the color data
    for (uint8_t page = start_page; page < end_page; page++)
    {
        lcd_address(page, start_column); // Set the start address for the current page
        for (uint8_t col = start_column; col < end_column; col++)
        {
            st7571_write_data(high_byte); // Write high byte
            st7571_write_data(low_byte); // Write low byte
        }
    }
}

void rtl_gui_lcd_clear(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd,
                       uint32_t color)
{
#if 1
    // Convert pixel coordinates to page and column coordinates
    uint8_t start_page = yStart / ST7571_PAGE_HEIGHT;
    uint8_t end_page = (yEnd + ST7571_PAGE_HEIGHT - 1) / ST7571_PAGE_HEIGHT;
    uint8_t start_column = xStart;
    uint8_t end_column = xEnd;

    // Calculate width and height in terms of pages and columns
    uint8_t width = end_column - start_column;
    uint8_t height = end_page - start_page;

    // Call clear_rect with converted coordinates and appropriate color
    clear_rect(start_page, start_column, width, height, (uint8_t)color);
#else
    static uint32_t color_buf = 0;
    color_buf = (color >> 8) | (color << 8);
    color_buf = color_buf | color_buf << 16;

    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = LCD_DMA_CHANNEL_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStruct);

    /* Initialize GDMA peripheral */
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum          = LCD_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_4;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_4;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_SourceInc_Fix;
    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_SPI0_TX;

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    rtk_lcd_hal_set_window(xStart, yStart, xEnd, yEnd);
    for (uint16_t i = 0; i < TOTAL_SECTION_COUNT - 1; i++)
    {
        lcd_dma_single_block_start(NULL, (uint32_t)&color_buf,
                                   ST7571_LCD_WIDTH * ST7571_SEC_HEIGHT * INPUT_PIXEL_BYTES);
        rtk_lcd_hal_transfer_done();
    }
    uint32_t last_len = 0;
    if (ST7571_LCD_HEIGHT % ST7571_SEC_HEIGHT == 0)
    {
        last_len = ST7571_SEC_HEIGHT * ST7571_LCD_WIDTH * INPUT_PIXEL_BYTES;
    }
    else
    {
        last_len = (ST7571_LCD_HEIGHT % ST7571_SEC_HEIGHT) * ST7571_LCD_WIDTH * INPUT_PIXEL_BYTES;
    }
    lcd_dma_single_block_start(NULL, (uint32_t)&color_buf, last_len);
    lcd_wait_lcd_control_transfer(ST7571_LCD_WIDTH * ST7571_LCD_HEIGHT * INPUT_PIXEL_BYTES);
#endif
}

void rtk_lcd_hal_set_window(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd)
{
    lcd_address(yStart / 8, xStart);
}

void rtk_lcd_hal_start_transfer(uint8_t *buf, uint32_t len) //todo luke
{
    DBG_DIRECT("rtk_lcd_hal_start_transfer buf : %p, len %d", buf, len);
    lcd_address(0, 0);
#if 1
    for (int j = 0; j < len; j++)
    {
        st7571_write_data(*buf++);
    }
#else
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_BUSY));
    for (int j = 0; j < len; j++)
    {
        while (!(LCD_SPI_BUS->SR & BIT(1)));
        LCD_SPI_BUS->DR[0] = *buf++;
    }
    while (SPI_GetFlagState(LCD_SPI_BUS, SPI_FLAG_BUSY));
#endif
}

uint32_t rtk_lcd_hal_get_width(void)
{
    return ST7571_LCD_WIDTH;
}
uint32_t rtk_lcd_hal_get_height(void)
{
    return ST7571_LCD_HEIGHT;
}
uint32_t rtk_lcd_hal_get_pixel_bits(void)
{
    return ST7571_DRV_PIXEL_BITS;
}

void rtk_lcd_hal_update_framebuffer(uint8_t *buf, uint32_t len)
{
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = LCD_DMA_CHANNEL_IRQ;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&NVIC_InitStruct);
    /* Initialize GDMA peripheral */
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum          = LCD_DMA_CHANNEL_NUM;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_BufferSize          = len;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_Byte;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_4;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_4;
    GDMA_InitStruct.GDMA_SourceAddr          = (uint32_t)buf;
    GDMA_InitStruct.GDMA_DestinationAddr     = (uint32_t)(LCD_SPI_BUS->DR);
    GDMA_InitStruct.GDMA_DestHandshake       = LCD_SPI_DMA_TX_HANDSHAKE;

    GDMA_Init(LCD_DMA_CHANNEL_INDEX, &GDMA_InitStruct);
    GDMA_INTConfig(LCD_DMA_CHANNEL_NUM, GDMA_INT_Transfer, ENABLE);

    GDMA_Cmd(LCD_DMA_CHANNEL_NUM, ENABLE);
    SPI_GDMACmd(LCD_SPI_BUS, SPI_GDMAReq_Tx, ENABLE);
}

uint32_t rtk_lcd_hal_power_on(void)
{
    st7571_write_cmd(0x2F);
    st7571_write_cmd(0xAF);
    return 0;
}

uint32_t rtk_lcd_hal_power_off(void)
{
    st7571_write_cmd(0x28);
    st7571_write_cmd(0xAE);
    platform_delay_ms(150);
    return 0;
}

void lcd_set_reset(bool reset)
{
    if (reset)
    {
        Pad_Config(LCD_SPI_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_DOWN, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }
    else
    {
        Pad_Config(LCD_SPI_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
}

static void lcd_st7571_init(void)
{
    st7571_write_cmd(0xAE);// Display OFF
    st7571_write_cmd(0x38);// MODE SET
    st7571_write_cmd(0xB8);// FR = 1011 => 85Hz   BE[1:0]=1,0 => booster efficiency Level-3

    st7571_write_cmd(0xA1);// ADC select, ADC=1 => reverse direction
    st7571_write_cmd(0xC8);// SHL select, SHL=1 => reverse direction
    st7571_write_cmd(0x44);// Set initial COM0 register
    st7571_write_cmd(0x00);
    st7571_write_cmd(0x40);// Set initial display line register
    st7571_write_cmd(0x00);

    st7571_write_cmd(0xAB);// OSC ON
    st7571_write_cmd(0x67);// DC-DC step up, 8 times boosting circuit
    st7571_write_cmd(0x25);// Select regulator register(1+(Ra+Rb))
    st7571_write_cmd(0x81);// Set Reference Voltage
    st7571_write_cmd(0x15);// EV=35 => Vop =10.556V
    st7571_write_cmd(0x54);// Set LCD Bias=1/9 V0
    st7571_write_cmd(0xF3);// Release Bias Power Save Mode
    st7571_write_cmd(0x04);

    st7571_write_cmd(0x7B);    // command set 3
#if ST7571_PIXEL_BITS == 2
    st7571_write_cmd(0x10);    // 2bit mode
#endif
#if ST7571_PIXEL_BITS == 1
    st7571_write_cmd(0x11);    // 1bit mode
#endif
    st7571_write_cmd(0x00);    // exit command set 3

    st7571_write_cmd(0xA6);    // 00 white
    // st7571_write_cmd(0xA7);    // 00 black

    st7571_write_cmd(0xC8);    //Set COM Scan Direction   C0 / C8
    st7571_write_cmd(0xA0);    //Set SEG Scan Direction   A0 / A1

    st7571_write_cmd(0x93);//Set FRC and PWM mode (4FRC & 15PWM)

    st7571_write_cmd(0x2C);//Power Control, VC: ON VR: OFF VF: OFF
    platform_delay_ms(100);
    st7571_write_cmd(0x2E);//Power Control, VC: ON VR: ON VF: OFF
    platform_delay_ms(100);
    st7571_write_cmd(0x2F);//Power Control, VC: ON VR: ON VF: ON
    platform_delay_ms(100);
    // st7571_write_cmd(0xAF);//Display ON
    // platform_delay_ms(10);
}

void rtk_lcd_hal_init(void)
{
    lcd_device_init();
    lcd_set_reset(true);
    platform_delay_ms(50);
    lcd_set_reset(false);
    platform_delay_ms(50);
    lcd_st7571_init();

    fill_screen(0x0);
    st7571_write_cmd(0xAF); //Display ON

    // for (int i = 0; i < 16; i++)
    //     fill_page(i, i / 4);

    DBG_DIRECT("rtk_lcd_hal_init DONE");
}



