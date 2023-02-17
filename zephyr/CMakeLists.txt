#
# Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_CANIOT_LIB)

	zephyr_library()

	set(lib ${ZEPHYR_CURRENT_MODULE_DIR})

	zephyr_include_directories(${lib}/include)
	
	FILE(GLOB_RECURSE sources ${lib}/src/*.c*)
	zephyr_library_sources(${sources})

endif()