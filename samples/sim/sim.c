#include <stdio.h>

#include <caniot/caniot.h>
#include <caniot/controller.h>
#include <caniot/device.h>
#include <caniot/archutils.h>

#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include "header.h"

static void dispatch_can_frame(const struct caniot_frame *frame)
{
	if (caniot_controller_is_target(frame)) {
		controllers_process(frame);
	} else {
		devices_process(frame);
	}
}

caniot_frame_t qtelemetry = {
	.id = {
		.type = CANIOT_FRAME_TYPE_TELEMETRY,
		.endpoint = CANIOT_ENDPOINT_BOARD_CONTROL,
		.query = CANIOT_QUERY
	},
	.len = 0U,
};

caniot_frame_t qwrite_attr = {
	.id = {
		.type = CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE,
		.query = CANIOT_QUERY
	},
	.len = 6U,
	.buf = {
		0x60,
		0x20,
		'G',
		'B',
		'E',
		'N'
	}
};

caniot_frame_t qcommand = {
	.id = {
		.type = CANIOT_FRAME_TYPE_COMMAND,
		.endpoint = CANIOT_ENDPOINT_APP,
		.query = CANIOT_QUERY
	},
	.len = 1U,
	.buf = {
		0xFF
	}
};

struct timed_frame {
	uint64_t time; /* ms */

	uint8_t ctrlid;
	caniot_did_t did;
	uint32_t timeout; /* ms */
	struct caniot_frame *frame;
};

struct timed_frame timed_frames[] = {
	{ 500U, 0U, CANIOT_DID(CANIOT_DEVICE_CLASS1, CANIOT_DEVICE_SID0), 450U, &qtelemetry },
	{ 500U, 0U, CANIOT_DID(CANIOT_DEVICE_CLASS1, CANIOT_DEVICE_SID1), 450U, &qtelemetry },
	{ 500U, 0U, CANIOT_DID(CANIOT_DEVICE_CLASS1, CANIOT_DEVICE_SID2), 450U, &qtelemetry },
};

int main(void)
{
	int ret;
	
	init_controllers();
	init_devices();

	caniot_frame_t frame;
	caniot_clear_frame(&frame);

	ctrl_Q(0U, CANIOT_DID(CANIOT_DEVICE_CLASS1, CANIOT_DEVICE_SID0), &qtelemetry, 1000U);
	ctrl_Q(0U, CANIOT_DID(CANIOT_DEVICE_CLASS1, CANIOT_DEVICE_SID1), &qwrite_attr, 1000U);
	ctrl_Q(0U, CANIOT_DID(CANIOT_DEVICE_CLASS1, CANIOT_DEVICE_SID2), &qcommand, 750U);
	ctrl_Q(0U, CANIOT_DID(CANIOT_DEVICE_CLASS1, CANIOT_DEVICE_SID3), &qtelemetry, 650U);
	ctrl_Q(0U, CANIOT_DID(CANIOT_DEVICE_CLASS1, CANIOT_DEVICE_SID4), &qwrite_attr, 550U);



	while (1) {
		/* queue delayed frames */
		uint32_t sec;
		uint16_t ms;
		vtime_get(&sec, &ms);
		const uint64_t now = (uint64_t)sec * 1000U + ms;

		for (struct timed_frame *tf = timed_frames;
		     tf < timed_frames + ARRAY_SIZE(timed_frames); tf++) {
			
			if (tf->time >= now) {
				ctrl_Q(tf->ctrlid, tf->did, tf->frame, tf->timeout);
			}
		}

		ret = can_recv(&frame);
		if (ret == 0U) {
			// caniot_show_frame(&frame);
			caniot_explain_frame(&frame);
			printf("\n");
			dispatch_can_frame(&frame);
		} else if (ret == -CANIOT_EAGAIN) {
			sleep(1);
		} else {
			printf("Error on can_recv\n");
			exit(EXIT_FAILURE);
		}
	}

	return 0;
}