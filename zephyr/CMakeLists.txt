
if(CONFIG_CANIOT_LIB)

	# message("HELLO FROM CANIOT-LIB CMakeLists.txt")

	zephyr_library()

	set(CANIOT_LIB ${ZEPHYR_CURRENT_MODULE_DIR})

	# message(${ZEPHYR_CURRENT_MODULE_DIR})

	zephyr_include_directories(${CANIOT_LIB}/include)
	FILE(GLOB_RECURSE sources ${CANIOT_LIB}/src/*.c*)

	zephyr_library_sources(${sources})

endif()