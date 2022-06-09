#include <caniot/archutils.h>
#include <caniot/caniot.h>
#include <caniot/device.h>

#include "header.h"

struct caniot_identification ids[DEVICES_COUNT];

struct caniot_config cfgs[DEVICES_COUNT];

const struct caniot_config default_cfg = CANIOT_CONFIG_DEFAULT_INIT();

struct caniot_device devices[DEVICES_COUNT];

void init_devices(void)
{

	for (size_t i = 0U; i < ARRAY_SIZE(devices); i++) {
		struct caniot_device *dev = &devices[i];

		ids[i].did.cls = CANIOT_DEVICE_CLASS1;
		ids[i].did.sid = i;
		ids[i].magic_number = 0x7777*i;
		ids[i].version = 0;

		dev->identification = &ids[i];

		memset(&dev->system, 0x00, sizeof(struct caniot_system));

		dev->flags.request_telemetry = 0U;
	}
}

void devices_process(const struct caniot_frame *frame)
{

}