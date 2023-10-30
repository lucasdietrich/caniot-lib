/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "header.h"

#include <caniot/caniot.h>
#include <caniot/caniot_private.h>
#include <caniot/device.h>

struct caniot_device_id ids[DEVICES_COUNT];

struct caniot_device_config cfgs[DEVICES_COUNT];

const struct caniot_device_config default_cfg = CANIOT_CONFIG_DEFAULT_INIT();

struct caniot_device devices[DEVICES_COUNT];

static int cb_config_on_read(struct caniot_device *dev)
{
	printf("[DEV CB] cb_config_on_read dev=%p config=%p\n",
		   (void *)dev,
		   (void *)dev->config);

	vtime_inc_const();

	return 0U;
}

static int cb_config_on_write(struct caniot_device *dev)
{
	printf("[DEV CB] cb_config_on_write dev=%p config=%p\n",
		   (void *)dev,
		   (void *)dev->config);

	vtime_inc_const();

	return 0U;
}

static int cb_attr_read(struct caniot_device *dev, uint16_t key, uint32_t *val)
{
	*val = 0U;

	printf(
		"[DEV CB] cb_attr_read dev=%p key=%hu *val=%p\n", (void *)dev, key, (void *)val);

	vtime_inc_const();

	return 0U;
}

static int cb_attr_write(struct caniot_device *dev, uint16_t key, uint32_t val)
{
	printf("[DEV CB] cb_attr_write dev=%p key=%hu val=%u\n", (void *)dev, key, val);

	vtime_inc_const();

	return 0U;
}

static int cb_telemetry_handler(struct caniot_device *dev,
								caniot_endpoint_t ep,
								unsigned char *buf,
								uint8_t *len)
{
	*len = 0U;

	printf("[DEV CB] cb_telemetry_handler dev=%p ep=%hhu buf=%p [&len=%p len=%hhu]\n",
		   (void *)dev,
		   ep,
		   (void *)buf,
		   (void *)len,
		   *len);

	vtime_inc_const();

	return 0U;
}

static int cb_command_handler(struct caniot_device *dev,
							  caniot_endpoint_t ep,
							  const unsigned char *buf,
							  uint8_t len)
{
	printf("[DEV CB] cb_command_handler dev=%p ep=%hhu buf=%p [len = %hhu]\n",
		   (void *)dev,
		   ep,
		   (void *)buf,
		   len);

	vtime_inc_const();

	return 0U;
}

const struct caniot_device_api api = CANIOT_DEVICE_API_FULL_INIT(cb_command_handler,
																 cb_telemetry_handler,
																 cb_config_on_read,
																 cb_config_on_write,
																 cb_attr_read,
																 cb_attr_write);

void init_devices(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(devices); i++) {
		struct caniot_device *dev = &devices[i];

		ids[i].did			= i;
		ids[i].magic_number = 2 * i + 1U;
		ids[i].version		= 0;

		dev->identification = &ids[i];

		memset(&dev->system, 0x00, sizeof(struct caniot_device_system));

		dev->flags.request_telemetry_ep = CANIOT_ENDPOINT_APP;
		memcpy(&cfgs[i], &default_cfg, sizeof(struct caniot_device_config));
		dev->config = &cfgs[i];

		dev->api = &api;
	}
}

void devices_process(const struct caniot_frame *req)
{
	struct caniot_frame resp;

	caniot_clear_frame(&resp);

	for (size_t i = 0U; i < ARRAY_SIZE(devices); i++) {
		if (caniot_device_is_target(devices[i].identification->did, req) == true) {
			caniot_device_handle_rx_frame(&devices[i], req, &resp);
			// caniot_explain_frame(&resp);
			// printf("\n");

			can_send(&resp, 0U);
		}
	}
}