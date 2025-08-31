/*
 * Copyright (c) 2025 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define _GNU_SOURCE
#include <stddef.h>
#include <stdint.h>

#include <caniot/caniot.h>

// --------------------------------------------------------------------------------------
// Driver API table exposure
static const struct caniot_drivers_api g_dummy_api = {
	.entropy  = NULL,
	.get_time = NULL,
	.set_time = NULL,
	.send	  = NULL,
	.recv	  = NULL,
};

// Exported pointer named exactly as requested.
const struct caniot_drivers_api *dummy_driver_api_ptr = &g_dummy_api;
