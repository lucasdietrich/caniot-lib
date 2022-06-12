#include <errno.h>

#include <caniot/archutils.h>
#include <caniot/caniot.h>
#include <caniot/controller.h>

static uint32_t counter = 0;

#include "header.h"

struct caniot_controller controllers[CONTROLLERS_COUNT];

static int can_send_fail(const struct caniot_frame *frame, uint32_t delay_ms)
{
	return -1;
}

const struct caniot_drivers_api driv = {
	.entropy = NULL,
	.get_time = vtime_get, /* get_time */
	.recv = can_recv,
	.send = can_send, /* can_send, can_send_fail */
	.set_time = NULL
};

extern caniot_frame_t qtelemetry;

bool ctrl_event_cb(const caniot_controller_event_t *ev,
		   void *user_data)
{
	printf("[CTRL EV] did=%u, handle=%u ctx=%u status=%u term=%u resp=%p\n",
	       ev->did, ev->handle, ev->context, ev->status, ev->terminated,
	       ev->response);

	vtime_inc_const();

	if (ev->status == CANIOT_CONTROLLER_EVENT_STATUS_TIMEOUT) {
		printf("!!!!!! TIMEOUT !!!!! : %u\n", ev->did);
	}

	/*
	if (ev->status == CANIOT_CONTROLLER_EVENT_STATUS_OK) {
		if (counter++ > 10) {
			goto exit;
		}
		ctrl_Q(0U, CANIOT_DID(ev->response->id.cls, ev->response->id.sid),
		       &qtelemetry, 350U);
	}
	*/

exit:
	return true;
}

void init_controllers(void)
{
	struct caniot_controller *ctrl;

	for (ctrl = controllers; ctrl < controllers + ARRAY_SIZE(controllers); ctrl++) {
		ctrl->driv = &driv;
		caniot_controller_driv_init(ctrl, &driv, ctrl_event_cb, NULL);
	}
}

void controllers_process(const struct caniot_frame *frame)
{
	struct caniot_controller *ctrl;

	for (ctrl = controllers; ctrl < controllers + ARRAY_SIZE(controllers); ctrl++) {
		caniot_controller_process_single(ctrl, 100U, frame);
	}
}

int ctrl_Q(uint32_t ctrlid,
	   caniot_did_t did,
	   struct caniot_frame *frame,
	   uint32_t timeout)
{
	int ret = -EINVAL;

	if (ctrlid < CONTROLLERS_COUNT) {
		caniot_controller_query_send(&controllers[ctrlid], did, frame, timeout);

		ret = 0;
	}

	return ret;
}

int ctrl_C(uint32_t ctrlid, uint8_t handle, bool suppress)
{
	caniot_controller_cancel_query(&controllers[ctrlid], handle, suppress);
}