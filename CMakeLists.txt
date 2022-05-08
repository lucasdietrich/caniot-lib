cmake_minimum_required(VERSION 3.2)

project(caniotlib VERSION 1.0.0 DESCRIPTION "CANIOT lib")

zephyr_library_named(caniotlib)

file(GLOB CANIOT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/**.c")
zephyr_library_sources(${CANIOT_SOURCES})

zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)