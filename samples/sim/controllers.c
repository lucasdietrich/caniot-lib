/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <caniot/caniot.h>
#include <caniot/caniot_private.h>
#include <caniot/controller.h>

// static uint32_t counter = 0;

#include "header.h"

struct caniot_controller controllers[CONTROLLERS_COUNT];

/*
static int can_send_fail(const struct caniot_frame *frame, uint32_t delay_ms)
{
	return -1;
}
*/

const struct caniot_drivers_api driv = {
	.entropy  = NULL,
	.get_time = vtime_get, /* get_time */
	.recv	  = can_recv,
	.send	  = can_send, /* can_send, can_send_fail */
	.set_time = NULL,
};

extern caniot_frame_t qtelemetry;

bool ctrl_event_cb(const caniot_controller_event_t *ev, void *user_data)
{
	(void)user_data;

	printf("[CTRL EV] did=%u handle=%u ctx=%u status=%u term=%u resp=%p\n",
	       ev->did,
	       ev->handle,
	       ev->context,
	       ev->status,
	       ev->terminated,
	       (void *)ev->response);

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
exit:
	*/
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

void controllers_process(const struct caniot_frame *frame, uint32_t time_passed)
{
	struct caniot_controller *ctrl;

	for (ctrl = controllers; ctrl < controllers + ARRAY_SIZE(controllers); ctrl++) {
		caniot_controller_rx_frame(ctrl, time_passed, frame);
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

int ctrl_C(uint32_t ctrlid, uint8_t handle, bool suppress)
{
	caniot_controller_query_cancel(&controllers[ctrlid], handle, suppress);

	return 0;
}

static struct caniot_discovery_params params;

bool discovery_cb(struct caniot_controller *ctrl,
		  caniot_did_t did,
		  const struct caniot_frame *frame,
		  void *user_data)
{
	(void)ctrl;
	(void)user_data;

	printf("Discovery callback: did=%u : ", did);
	caniot_show_frame(frame);
	printf("\n");

	return true;
}

void controllers_discovery_start(void)
{
	int ret;

	if (!caniot_controller_discovery_running(&controllers[0u])) {
		params.mode	     = CANIOT_DISCOVERY_MODE_ACTIVE;
		params.type	     = CANIOT_DISCOVERY_TYPE_TELEMETRY;
		params.data.endpoint = CANIOT_ENDPOINT_BOARD_CONTROL;

		params.timeout = 10000u;

		params.user_callback = discovery_cb;
		params.user_data     = NULL;

		ret = caniot_controller_discovery_start(
			&controllers[0u],
			(const struct caniot_discovery_params *)&params);

		printf("Discovery started ret: %d", ret);
	}
}

void controllers_discovery_stop(void)
{
	int ret;

	ret = caniot_controller_discovery_stop(&controllers[0u]);

	printf("Discovery stopped ret: %d\n", ret);
}