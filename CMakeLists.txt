#
# Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.2)

message(STATUS "$CMAKE_CURRENT_SOURCE_DIR : ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "$CMAKE_CURRENT_LIST_DIR : ${CMAKE_CURRENT_LIST_DIR}")
message(STATUS "$CMAKE_CURRENT_BINARY_DIR : ${CMAKE_CURRENT_BINARY_DIR}")

####################################################################

project(caniotlib VERSION 1.0.0 DESCRIPTION "CANIOT lib")

set(CMAKE_BUILD_TYPE Debug)

# add debug symbols
set(CMAKE_CXX_FLAGS "-Wall -g -O2")

file(GLOB CANIOT_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/**.c")
file(GLOB CANIOT_HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/**.h")

add_library(caniotlib STATIC ${CANIOT_SOURCES} ${CANIOT_HEADERS})

target_compile_definitions(caniotlib PUBLIC CONFIG_CANIOT_DRIVERS_API=0)
target_compile_definitions(caniotlib PUBLIC CONFIG_CANIOT_CTRL_DRIVERS_API=1)
target_compile_definitions(caniotlib PUBLIC CONFIG_CANIOT_LOG_LEVEL=4)
target_compile_definitions(caniotlib PUBLIC CONFIG_CANIOT_ASSERT=1)
target_compile_definitions(caniotlib PUBLIC CONFIG_CANIOT_MAX_PENDING_QUERIES=1)

target_include_directories(caniotlib PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

####################################################################

set(CONFIG_CANIOT_SAMPLES ON CACHE BOOL "Enable CANIOT samples")
set(CONFIG_CANIOT_TESTS ON CACHE BOOL "Enable CANIOT tests")

# foreach directory in "samples" include CMakeLists.txt
if (CONFIG_CANIOT_SAMPLES)
    add_subdirectory(samples)
endif()

if (CONFIG_CANIOT_TESTS)
    add_subdirectory(tests)
endif()