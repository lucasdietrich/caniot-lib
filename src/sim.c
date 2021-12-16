#include "sim.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#define device struct caniot_device

static bool frame_match(struct caniot_device *dev, struct caniot_frame *frame)
{
	return (dev->identification->did.cls == frame->id.cls) &&
		(dev->identification->did.dev == frame->id.dev);
}

int process_rx_frame(struct caniot_device *dev_list[],
		     uint8_t dev_count,
		     struct caniot_frame *frame)
{
	struct caniot_frame resp;
	memset(&resp, 0x00, sizeof(struct caniot_frame));

	if (frame->id.query != query) {
		return -EINVAL;
	}

	for (device **dev = dev_list; dev < &dev_list[dev_count]; dev++) {
		if (frame_match(*dev, frame)) {
			return caniot_device_handle_rx_frame(*dev, frame, &resp);
		}
	}

	return 0;
}