#include <caniot/archutils.h>
#include <caniot/caniot.h>
#include <caniot/controller.h>

#include "header.h"

struct caniot_controller controllers[CONTROLLERS_COUNT];

void init_controllers(void)
{
	struct caniot_controller *ctrl;

	for (ctrl = controllers; ctrl < controllers + ARRAY_SIZE(controllers); ctrl++) {
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