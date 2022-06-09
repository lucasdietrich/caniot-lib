#include <caniot/archutils.h>
#include <caniot/caniot.h>
#include <caniot/controller.h>

#include <time.h>

#include "header.h"

struct caniot_controller controllers[CONTROLLERS_COUNT];

void get_time(uint32_t *sec, uint16_t *ms)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	printf("get_time %lu.%lu\n", ts.tv_sec, ts.tv_nsec);

	if (sec != NULL) {
		*sec = ts.tv_sec;
	}

	if (ms != NULL) {
		*ms = ts.tv_nsec / 1000000LLU;
	}
}

struct caniot_drivers_api driv = {
	.entropy = NULL,
	.get_time = get_time,
	.recv = can_recv,
	.send = can_send,
	.set_time = NULL
};

void init_controllers(void)
{
	struct caniot_controller *ctrl;

	for (ctrl = controllers; ctrl < controllers + ARRAY_SIZE(controllers); ctrl++) {
		ctrl->driv = &driv;
		caniot_controller_init(ctrl);
	}
}

void controllers_process(const struct caniot_frame *frame)
{
	struct caniot_controller *ctrl;

	for (ctrl = controllers; ctrl < controllers + ARRAY_SIZE(controllers); ctrl++) {
		caniot_controller_handle_rx_frame(ctrl, frame);
	}
}