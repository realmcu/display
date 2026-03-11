# Copyright (c) 2024 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)


# Define a variable for the library name
if(1) #set to 1 to enable or variable to for depnedency

	set(SOURCES "") # Initialize SOURCES
	
    file(GLOB HEADERS "*.h" "*.hpp")
	if(CONFIG_REALTEK_LCD_SH8601Z_410_502_QSPI)
		list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/lcd_sh8601z_410_502_qspi.c")
	endif()

	if(CONFIG_REALTEK_SH8601Z_454454_QSPI)
		list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/SH8601Z_454454_qspi.c")
	endif()
    if(CONFIG_REALTEK_LCD_ST7265_800_480_RGB)
		list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/lcd_st7265_800480_rgb.c")
	endif()
    if(CONFIG_REALTEK_LCD_EK9716_800_480_RGB)
		list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/EK9716_800480_rgb.c")
	endif()
    if(CONFIG_REALTEK_LCD_ST77916_360_360_8080)
		list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/st77916_360_360_dbib_8080.c")
	endif()
    if(CONFIG_REALTEK_LCD_ST7789_170_320_SPI)
		list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/st7789_170_320_lcdc_spi.c")
	endif()
    if(CONFIG_REALTEK_LCD_SH8601Z_466_466_QSPI)
        list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/lcd_sh8601z_466_466_qspi.c")
    endif()

    # Determine if we should create a library
    if((NOT HEADERS) AND (NOT SOURCES))
        message(STATUS "No source or header files found in ${CMAKE_CURRENT_SOURCE_DIR}.")
		target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    if(HEADERS AND (NOT SOURCES))
        target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    if(SOURCES AND (NOT HEADERS))
        target_sources(${LIBRARY_NAME} PRIVATE ${SOURCES})
    endif()

    if(SOURCES AND HEADERS)
        target_sources(${LIBRARY_NAME} PRIVATE ${SOURCES})
        target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    # Process subdirectories regardless
    file(GLOB SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

    foreach(subdir ${SUBDIRS})
        if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
            if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/CMakeLists.txt)
                add_subdirectory(${subdir})
            else()
                message(STATUS "Skipping directory ${subdir} as it does not contain a CMakeLists.txt file.")
            endif()
        endif()
    endforeach()
endif()
