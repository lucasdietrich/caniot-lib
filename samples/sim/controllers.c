#include <errno.h>

#include <caniot/archutils.h>
#include <caniot/caniot.h>
#include <caniot/controller.h>

static uint32_t counter = 0;

#include "header.h"

struct caniot_controller controllers[CONTROLLERS_COUNT];

const struct caniot_drivers_api driv = {
	.entropy = NULL,
	.get_time = vtime_get, /* get_time */
	.recv = can_recv,
	.send = can_send,
	.set_time = NULL
};

extern caniot_frame_t qtelemetry;

bool ctrl_event_cb(const caniot_controller_event_t *ev,
		   void *user_data)
{
	printf("[CTRL EV from %u] ctx=%u [h = %u] ctrl=%p, status=%u term=%u broadcast=%u resp=%p\n",
	       ev->did, ev->context, ev->handle, ev->controller, ev->status, ev->terminated,
	       ev->is_broadcast_query, ev->response);

	vtime_inc_const();


	if (ev->status == CANIOT_CONTROLLER_EVENT_STATUS_OK) {
		if (counter++ > 10) {
			goto exit;
		}
		ctrl_Q(0U, CANIOT_DID(ev->response->id.cls, ev->response->id.sid),
		       &qtelemetry, 350U);
	}

exit:
	return true;
}

void init_controllers(void)
{
	struct caniot_controller *ctrl;

	for (ctrl = controllers; ctrl < controllers + ARRAY_SIZE(controllers); ctrl++) {
		ctrl->driv = &driv;
		caniot_controller_init(ctrl, &driv, ctrl_event_cb, NULL);
	}
}

void controllers_process(const struct caniot_frame *frame)
{
	struct caniot_controller *ctrl;

	for (ctrl = controllers; ctrl < controllers + ARRAY_SIZE(controllers); ctrl++) {
		caniot_controller_process_frame(ctrl, frame);
	}
}

int ctrl_Q(uint32_t ctrlid,
	   caniot_did_t did,
	   struct caniot_frame *frame,
	   uint32_t timeout)
{
	int ret = -EINVAL;

	if (ctrlid < CONTROLLERS_COUNT) {
		caniot_controller_query(&controllers[ctrlid], did, frame, timeout);

		ret = 0;
	}

	return ret;
}