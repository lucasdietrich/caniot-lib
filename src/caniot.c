#include "caniot.h"

#include <errno.h>
#include <stdio.h>
#include <memory.h>

static void prepare_response(struct caniot_device *dev,
			     struct caniot_frame *resp,
			     uint8_t type)
{
	memset(resp, 0x00, sizeof(struct caniot_frame));

	/* id */
	resp->id.type = type;
	resp->id.query = response;
	resp->id.cls = dev->identification.nodeid.cls;
	resp->id.dev = dev->identification.nodeid.dev;
	resp->id.endpoint = 0;
}

static void prepare_error(struct caniot_device *dev,
			  struct caniot_frame *resp,
			  int error)
{
	prepare_response(dev, resp, command);

	resp->err = (int32_t) error;
}

int caniot_telemetry_resp(struct caniot_device *dev,
			  struct caniot_frame *resp,
			  uint8_t ep)
{
	int ret;

	prepare_response(dev, resp, telemetry);

	/* buffer */
	ret = dev->api->telemetry(dev, ep, resp->buf, &resp->len);
	if (ret == 0) {
		resp->id.endpoint = ep;
	}

	return ret;
}

int caniot_attribute_resp(struct caniot_device *dev,
			  struct caniot_frame *resp,
			  uint16_t key)
{
	int ret;

	prepare_response(dev, resp, read_attribute);

	/* buffer */
	ret = dev->api->read_attribute(dev, key, &resp->attr.val);
	if (ret == 0) {
		resp->attr.key = key;
		resp->len = 6u;
	}

	return ret;
}

int process_dev_rx_frame(struct caniot_device *dev,
			 struct caniot_frame *req,
			 struct caniot_frame *resp)
{
	int ret;

	/* TODO check or not ? */
	if (req->id.query != query) {
		return -EINVAL;
	}

	switch (req->id.type) {
	case command:
		ret = dev->api->command(dev, req->id.endpoint,
					req->buf, req->len);
		if (ret != 0) {
			goto error;
		}

	case telemetry:
		ret = caniot_telemetry_resp(dev, resp, req->id.endpoint);
		break;

	case write_attribute:
		ret = dev->api->write_attribute(dev, req->attr.key,
						req->attr.val);
		if (ret != 0) {
			goto error;
		}
		
	case read_attribute:
		ret = caniot_attribute_resp(dev, resp, req->attr.key);
		break;
	}

	return 0;

error:
	prepare_error(dev, resp, ret);
	return ret;
}