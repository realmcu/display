# Copyright (c) 2024 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)


# Define a variable for the library name
if(1) #set to 1 to enable or variable to for depnedency

	set(SOURCES "") # Initialize SOURCES

    file(GLOB HEADERS "*.h" "*.hpp")

    if(CONFIG_REALTEK_TOUCH_CHSC6417_ZEPHYR)
		list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/touch_CHSC6417_zephyr.c")
	endif()

    if(CONFIG_REALTEK_TOUCH_CHSC6413_ZEPHYR)
		list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/touch_CHSC6413_zephyr.c")
	endif()

    if(CONFIG_REALTEK_TOUCH_CST816D_ZEPHYR)
		list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/touch_cst816d_zephyr.c")
	endif()

    if(CONFIG_REALTEK_TOUCH_CST820_ZEPHYR)
		list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/touch_cst820_zephyr.c")
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

    # Always add include directory for this chip
    target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

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
