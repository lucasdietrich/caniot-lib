cmake_minimum_required(VERSION 3.2)

message(STATUS "$CMAKE_CURRENT_SOURCE_DIR : ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "$CMAKE_CURRENT_LIST_DIR : ${CMAKE_CURRENT_LIST_DIR}")
message(STATUS "$CMAKE_CURRENT_BINARY_DIR : ${CMAKE_CURRENT_BINARY_DIR}")

####################################################################

project(caniotlib VERSION 1.0.0 DESCRIPTION "CANIOT lib")

set(CMAKE_BUILD_TYPE Debug)

file(GLOB CANIOT_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/**.c")
file(GLOB CANIOT_HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/**.h")

add_library(caniotlib STATIC ${CANIOT_SOURCES} ${CANIOT_HEADERS})

target_include_directories(caniotlib PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")