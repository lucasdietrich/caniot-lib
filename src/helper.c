#include "helper.h"

#include <stdio.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static const char *const id_type_str[] = {
	"Command",
	"Telemetry",
	"Write-attribute",
	"Read-attribute"
};

static const char *const id_query_str[] = {
	"Query",
	"Response"
};

static const char *const id_class_str[] = {
	"C0",
	"C1",
	"C2",
	"C3",
	"C4",
	"C5",
	"C6",
	"C7",
};

static const char *const id_dev_str[] = {
	"D0",
	"D1",
	"D2",
	"D3",
	"D4",
	"D5",
	"D6",
	"D7",
};

static const char *const id_endpoint_str[] = {
	"Default-endpoint",
	"Endpoint-1",
	"Endpoint-2",
	"ALL-ENDPOINTS",
};

static const char *get_id_type_str(uint8_t type)
{
	if (type < sizeof(id_type_str) / sizeof(id_type_str[0])) {
		return id_type_str[type];
	}
	return "<UNKNOWN>";
}

static const char *get_id_query_str(uint8_t query)
{
	if (query < sizeof(id_query_str) / sizeof(id_query_str[0])) {
		return id_query_str[query];
	}
	return "<UNKNOWN>";
}

static const char *get_id_class_str(uint8_t class)
{
	if (class < sizeof(id_class_str) / sizeof(id_class_str[0])) {
		return id_class_str[class];
	}
	return "<UNKNOWN>";
}

static const char *get_id_dev_str(uint8_t dev)
{
	if (dev < sizeof(id_dev_str) / sizeof(id_dev_str[0])) {
		return id_dev_str[dev];
	}
	return "<UNKNOWN>";
}

static const char *get_id_endpoint_str(uint8_t endpoint)
{
	if (endpoint < sizeof(id_endpoint_str) / sizeof(id_endpoint_str[0])) {
		return id_endpoint_str[endpoint];
	}
	return "<UNKNOWN>";
}

void caniot_show_frame(struct caniot_frame *frame)
{
	printf(F("%x [ %x %x %x %x %x %x %x %x] len = %d\n"),
	       frame->id.raw, frame->buf[0], frame->buf[1],
	       frame->buf[2], frame->buf[3], frame->buf[4], frame->buf[5],
	       frame->buf[6], frame->buf[7], frame->len);
}

void caniot_explain_id(union caniot_id id)
{
	if (caniot_is_error(id)) {
		printf(F("Error frame "));
		return;
	} else {
		printf(F("%s %s"),
		       get_id_type_str(id.type),
		       get_id_query_str(id.query));
	}

	if (caniot_is_broadcast(CANIOT_DEVICE(id.cls, id.sid))) {
		printf(F(" to BROADCAST : %s /"),
		       get_id_endpoint_str(id.endpoint));
	} else {
		printf(F(" to device %s %s : %s / "),
		       get_id_class_str(id.cls),
		       get_id_dev_str(id.sid),
		       get_id_endpoint_str(id.endpoint));
	}
}

void caniot_explain_frame(struct caniot_frame *frame)
{
	caniot_explain_id(frame->id);

	if (caniot_is_error(frame->id)) {
		printf(F(": %4x \n"), (uint32_t) -frame->err);
		return;
	}

	if ((frame->id.type == telemetry) || (frame->id.type == command)) {
		for (int i = 0; i < frame->len; i++) {
			printf(F("%02hhx "), frame->buf[i]);
		}
	} else {
		printf(F("LEN = %d, key = %x val = %4x"), frame->len,
		      frame->attr.key, frame->attr.val);
	}

	printf(F("\n"));
}

void caniot_build_query_telemtry(struct caniot_frame *frame,
				 union deviceid did,
				 uint8_t endpoint)
{
	struct caniot_frame tmp = {
		.id = {
			.cls = did.cls,
			.sid = did.sid,
			.type = telemetry,
			.query = query,
			.endpoint = endpoint
		},
		.buf = {0, 0, 0, 0, 0, 0, 0, 0},
		.len = 0
	};

	memcpy(frame, &tmp, sizeof(struct caniot_frame));
}

void caniot_build_query_command(struct caniot_frame *frame,
				union deviceid did,
				uint8_t endpoint,
				const uint8_t *buf,
				uint8_t size)
{
	struct caniot_frame tmp = {
		.id = {
			.cls = did.cls,
			.sid = did.sid,
			.type = command,
			.query = query,
			.endpoint = endpoint
		},
		.len = size
	};

	memcpy(frame, &tmp, sizeof(struct caniot_frame));

	memcpy(frame->buf, buf, MIN(size, 8));
}