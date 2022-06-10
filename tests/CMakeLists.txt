add_executable(test)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
target_sources(test PUBLIC ${SOURCES})

target_include_directories(test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

target_link_libraries(test caniotlib)
