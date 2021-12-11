#include "tempsens.h"

#include <stdio.h>
#include <stdlib.h>

#include "std.h"

static int update_time(struct caniot_device *dev, uint32_t ts) {
	
	dev->system.abstime = ts;

	return 0;
}

static int tmp_command(struct caniot_device *dev, uint8_t ep, char *buf, uint8_t len)
{
	if (ep != endpoint_default) {
		return -CANIOT_EEP;
	}
	
	TEMPDEV_OF(dev)->trigger_temp = ((uint32_t *)buf)[1];

	return 0;
}

static int tmp_telemetry(struct caniot_device *dev, uint8_t ep, char *buf, uint8_t *len)
{
	*len = 8;

	((uint32_t *)buf)[1] = TEMPDEV_OF(dev)->trigger_temp;
	((uint32_t *)buf)[0] = TEMPDEV_OF(dev)->measured_temp;

	return 0;
}

static const struct caniot_api tempsens_api = {
	.update_time = update_time,
	.command_handler = tmp_command,
	.telemetry = tmp_telemetry,

	.custom_attr = {
		.read = NULL,
		.write = NULL,
	},
	.config = {
		.on_read = NULL,
		.written = NULL,
	},
};

const struct caniot_identification id = {
			.name = "TempSens1",
			.did = {
				.cls = 2,
				.dev = 1
			},
			.version = 1
};

const struct caniot_drivers_api drivers = {
	.entropy = z_entropy,
	.get_time = z_get_time,
	.pending_telemetry = z_pending_telemetry,
	.recv = z_recv,
	.send = z_send,
	.set_filter = z_set_filter,
	.set_mask = z_set_mask,
	.unschedule = NULL,
	.schedule = NULL,
	.rom_read = z_rom_read,
	.persistent_read = z_persistent_read,
	.persistent_write = z_persistent_write,
};

struct caniot_config config = {
	.error_response = 1,
	.telemetry_min = 0,
	.telemetry_rdm_delay = 1000,
	.telemetry_period = 1000*10,
};

struct tempdev tempsens = {
	.trigger_temp = 0xDDAAAA,
	.measured_temp = 0x1234,
	.dev = {
		.identification = &id,
		.api = &tempsens_api,
		.driv = &drivers,
		.config = &config
	}
};


