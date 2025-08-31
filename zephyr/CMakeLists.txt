#
# Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_CANIOT_LIB)

	zephyr_library()

	set(lib ${ZEPHYR_CURRENT_MODULE_DIR})

	zephyr_include_directories(${lib}/include)
	
	set(sources 	"${lib}/src/caniot.c"
					"${lib}/src/classes.c"
					"${lib}/src/controller.c"
					"${lib}/src/datatype.c"
					"${lib}/src/device.c"
					"${lib}/src/fake.c"
					"${lib}/src/phys.c"
		)

	FILE(GLOB_RECURSE sources ${lib}/src/*.c*)
	zephyr_library_sources(${sources})

endif()