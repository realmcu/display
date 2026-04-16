#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: Apache-2.0
#
cmake_minimum_required(VERSION 3.10)


# Define a variable for the library name
if(CONFIG_REALTEK_DISPLAY_ENABLE_JPU AND (CONFIG_SOC_SERIES_RTL8773G OR CONFIG_SOC_SERIES_RTL87X3G)) #set to 1 to enable or variable to for depnedency

    file(GLOB SOURCES "*.c" "*.cpp")
    file(GLOB HEADERS "*.h" "*.hpp")

    target_compile_definitions(${HW_DISPLAY_TARGET_NAME} PRIVATE T_OS_MEM_TYPE=RAM_TYPE)
    target_compile_definitions(${HW_DISPLAY_TARGET_NAME} PRIVATE OS_MEM_TYPE_DATA=RAM_TYPE_DATA_ON)

    # Determine if we should create a library
    if((NOT HEADERS) AND (NOT SOURCES))
        message(STATUS "No source or header files found in ${CMAKE_CURRENT_SOURCE_DIR}.")
        target_include_directories(${HW_DISPLAY_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    if(HEADERS AND (NOT SOURCES))
        target_include_directories(${HW_DISPLAY_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    if(SOURCES AND (NOT HEADERS))
        target_sources(${HW_DISPLAY_TARGET_NAME} PRIVATE ${SOURCES})
    endif()

    if(SOURCES AND HEADERS)
        target_sources(${HW_DISPLAY_TARGET_NAME} PRIVATE ${SOURCES})
        target_include_directories(${HW_DISPLAY_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
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
