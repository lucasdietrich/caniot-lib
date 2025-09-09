#
# Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.8)

project(caniot
        VERSION 1.0.0
        DESCRIPTION "CANIOT lib"
        LANGUAGES C)

set(CONFIG_CANIOT_SAMPLES OFF CACHE BOOL "Enable CANIOT samples")
set(CONFIG_CANIOT_TESTS OFF CACHE BOOL "Enable CANIOT tests")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

add_library(caniot ${CANIOT_SOURCES})

set_target_properties(caniot PROPERTIES
  VERSION ${caniot_VERSION}
  SOVERSION ${caniot_VERSION_MAJOR}
)

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
target_compile_definitions(caniot PUBLIC CONFIG_CANIOT_DRIVER_LINUX=1)

target_include_directories(caniot PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

# foreach directory in "samples" include CMakeLists.txt
if (CONFIG_CANIOT_SAMPLES)
    add_subdirectory(samples)
endif()

if (CONFIG_CANIOT_TESTS)
    add_subdirectory(tests)
endif()

install(TARGETS caniot
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)


configure_file(libcaniot.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/libcaniot.pc
  @ONLY
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/libcaniot.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)