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

target_compile_definitions(caniotlib PRIVATE CONFIG_CANIOT_DRIVERS_API=0)
target_compile_definitions(caniotlib PRIVATE CONFIG_CANIOT_LOG_LEVEL=4)

target_include_directories(caniotlib PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

####################################################################

# foreach directory in "samples" include CMakeLists.txt
add_subdirectory(samples/sim)
add_subdirectory(tests)