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

project(caniot VERSION 1.0.0 DESCRIPTION "CANIOT lib")

option(BUILD_SHARED_LIBS "Build shared libraries (.so) instead of static ones (.a)" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug)

# add debug symbols
set(CMAKE_C_FLAGS_DEBUG "-Wall -Wextra -pedantic -g")
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -pedantic -g")

set(CMAKE_C_FLAGS_RELEASE "-Wall -g -O2")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -g -O2")

message(STATUS "CMAKE_CXX_FLAGS_DEBUG : ${CMAKE_CXX_FLAGS_DEBUG}")

set(CANIOT_SOURCES "src/caniot.c"
                   "src/classes.c"
                   "src/controller.c"
                   "src/datatype.c"
                   "src/device.c"
                   "src/fake.c"
                   "src/phys.c"
                   "src/drivers/linux.c"
                   "src/drivers/dummy.c"
    )
file(GLOB CANIOT_HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/**.h")


add_library(caniot ${CANIOT_SOURCES})

# # library config
target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_CHECKS=1)
target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_DRIVERS_API=1)
target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_DEVICE_HANDLE_BLC_SYS_CMD=1)
target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_CTRL_DRIVERS_API=1)
target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_DEBUG=1)
target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_DEVICE_FILTER_FRAME=1)
target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_DEVICE_STARTUP_ATTRIBUTES=1)
target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_MAX_PENDING_QUERIES=4) # 32 for production
target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_ATTRIBUTE_NAME=1)
target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_CONTROLLER_DISCOVERY=1)
target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_ASSERT=1)
target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_QUERY_ID=1)
target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_BUILD_INFOS=1)
target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_LOG_LEVEL=4) # 4 for debug
target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_POSIX=1)

# # SIM config
# target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_DEVICE_DRIVERS_API=0)
# target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_CTRL_DRIVERS_API=1)
# target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_LOG_LEVEL=4)
# target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_ASSERT=1)
# target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_MAX_PENDING_QUERIES=4)
# target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_ATTRIBUTE_NAME=1)

target_include_directories(caniot PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

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

install(TARGETS caniot
        EXPORT caniot-targets
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)