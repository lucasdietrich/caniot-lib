add_executable(sim)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
target_sources(sim PUBLIC ${SOURCES})

target_include_directories(sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_link_libraries(sim caniotlib)
