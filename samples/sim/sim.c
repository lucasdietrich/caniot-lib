#include <stdio.h>

#include <caniot/caniot.h>
#include <caniot/controller.h>
#include <caniot/device.h>

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

int main(void)
{
	int ret;

	caniot_test();
	
	init_controllers();
	init_devices();
	// init_canbus();

	caniot_frame_t frame = {
		.id = {
			.cls = CANIOT_DEVICE_CLASS1,
			.sid = CANIOT_DEVICE_SID1,
			.type = CANIOT_FRAME_TYPE_TELEMETRY,
			.endpoint = CANIOT_ENDPOINT_BOARD_CONTROL,
			.query = CANIOT_QUERY
		},
		.len = 0U,
	};

	ret = can_send(&frame, 0U);
	printf("can_send(%p, 0) = -0x%X\n", &frame, -ret);

	caniot_clear_frame(&frame);

	while (1) {
		ret = can_recv(&frame);
		printf("can_recv(%p, 0) = -0x%X\n", &frame, -ret);
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