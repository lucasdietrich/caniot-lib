/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "header.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <caniot/caniot.h>
#include <caniot/caniot_private.h>
#include <caniot/controller.h>
#include <caniot/device.h>
#include <unistd.h>

caniot_frame_t qtelemetry = {
	.id =
		{
			.type	  = CANIOT_FRAME_TYPE_TELEMETRY,
			.endpoint = CANIOT_ENDPOINT_BOARD_CONTROL,
			.query	  = CANIOT_QUERY,
		},
	.len = 0U,
};

caniot_frame_t qwrite_attr = {
	.id =
		{
			.type  = CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE,
			.query = CANIOT_QUERY,
		},
	.len = 6U,
	.buf = {0x60, 0x20, 'G', 'B', 'E', 'N'},
};

caniot_frame_t qcommand = {
	.id =
		{
			.type	  = CANIOT_FRAME_TYPE_COMMAND,
			.endpoint = CANIOT_ENDPOINT_APP,
			.query	  = CANIOT_QUERY,
		},
	.len = 1U,
	.buf = {0xFFu},
};

caniot_frame_t fake_telem_resp = {
	.id =
		{
			.type	  = CANIOT_FRAME_TYPE_TELEMETRY,
			.endpoint = CANIOT_ENDPOINT_BOARD_CONTROL,
			.query	  = CANIOT_RESPONSE,
			.cls	  = CANIOT_DEVICE_CLASS1,
			.sid	  = CANIOT_DEVICE_SID0,
		},
	.len = 0U,
	.buf = {0xFFu},
};

struct timed_frame {
	uint64_t time; /* ms */

	uint8_t ctrlid;
	caniot_did_t did;
	uint32_t timeout; /* ms */
	struct caniot_frame *frame;
};

struct timed_frame timed_frames[] = {
	{100u,
	 0U,
	 CANIOT_DID(CANIOT_DEVICE_CLASS1, CANIOT_DEVICE_SID0),
	 1000u,
	 &qtelemetry},
	{100u, 0U, CANIOT_DID_BROADCAST, 1000u, &qtelemetry},
};

int main(void)
{
	uint32_t counter = 0;

	init_controllers();
	init_devices();

	caniot_frame_t frame;
	caniot_clear_frame(&frame);

	// ctrl_Q(0U, CANIOT_DID_BROADCAST, &qtelemetry, 400u);

	uint64_t last_time = 0u;

	while (1) {
		/* Get current time */
		uint32_t sec;
		uint16_t ms;
		vtime_get(&sec, &ms);
		const uint64_t now = (uint64_t)sec * 1000U + ms;
		char chr;
		ssize_t ret;

		/* Send schedulded frames */
		for (struct timed_frame *tf = timed_frames;
		     tf < timed_frames + ARRAY_SIZE(timed_frames);
		     tf++) {

			if (now >= tf->time) {
				ctrl_Q(tf->ctrlid, tf->did, tf->frame, tf->timeout);
				tf->time = (uint64_t)-1;
			}
		}

		/* Check whether there is a new character with select */
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds);
		struct timeval timeout = {.tv_sec = 0, .tv_usec = 0};
		ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
		if (ret > 0 && read(STDIN_FILENO, &chr, 1u) > 0) {
			switch (chr) {
			case 'd':
				controllers_discovery_start();
				break;
			case 's':
				controllers_discovery_stop();
				break;
			default:
				break;
			}
		}

		/* Compute time delta */
		if (last_time == 0u) {
			last_time = now;
		}
		const uint64_t delta = now - last_time;
		last_time	     = now;

		/* Process a single frame */
		ret = can_recv(&frame);
		if (ret == 0U) {
			// caniot_show_frame(&frame);
			caniot_explain_frame(&frame);
			printf("\n");

			if (caniot_controller_is_target(&frame)) {
				controllers_process(&frame, delta);
			} else {
				devices_process(&frame);
			}
		} else if (ret == -CANIOT_EAGAIN) {
			controllers_process(NULL, delta);
			sleep(1);
		} else {
			printf("Error on can_recv\n");
			exit(EXIT_FAILURE);
		}

		counter++;

		vtime_inc_const();
	}

	return 0;
}