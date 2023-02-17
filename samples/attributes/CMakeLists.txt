add_executable(sample_attr)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
target_sources(sample_attr PUBLIC ${SOURCES})

target_include_directories(sample_attr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_link_libraries(sample_attr caniotlib)
