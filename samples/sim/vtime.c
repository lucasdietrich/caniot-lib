/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static uint64_t time_ms = 0U;

void vtime_get(uint32_t *sec, uint16_t *ms)
{
	if (sec != NULL) {
		*sec = time_ms / 1000U;
	}

	if (ms != NULL) {
		*ms = time_ms % 1000U;
	}

	// printf("[ vtime_get %lu ms ]\n", time_ms);
}

void vtime_inc(uint32_t inc_ms)
{
	// printf("[ vtime_inc %lu + %u ms = %lu ms]\n",
	//        time_ms, inc_ms, time_ms + inc_ms);

	time_ms += inc_ms;
}