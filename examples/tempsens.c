#include "tempsens.h"

#include <stdio.h>

static int dev_update_time(struct caniot_device *dev, uint32_t ts) {
	
	dev->system.abstime = ts;

	return 0;
}

static int dev_command(struct caniot_device *dev, uint8_t ep, char *buf, uint8_t len)
{
	TEMPDEV_OF(dev)->trigger_temp = ((uint32_t *)buf)[1];

	return 0;
}

static int dev_telemetry(struct caniot_device *dev, uint8_t ep, char *buf, uint8_t *len)
{
	*len = 8;

	((uint32_t *)buf)[1] = TEMPDEV_OF(dev)->trigger_temp;
	((uint32_t *)buf)[0] = TEMPDEV_OF(dev)->measured_temp;

	return 0;
}

static const struct caniot_api tempsens_api = {
	.update_time = dev_update_time,
	.scheduled_handler = NULL,
	.update_config = NULL,
	.read_attribute = NULL,
	.write_attribute = NULL,
	.command_handler = dev_command,
	.telemetry = dev_telemetry
};

struct tempdev tempsens = {
	.trigger_temp = 0xDDAA,
	.measured_temp = 0x1234,
	.dev = {
		.identification = {
			.name = "TempSens1",
			.node = {
				.cls = 2,
				.dev = 1
			},
			.version = 1
		},
		.api = &tempsens_api
	}
};


