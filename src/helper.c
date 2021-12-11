#include "helper.h"

#include <stdio.h>

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
	printf("%x [ %x %x %x %x %x %x %x %x] len = %d\n",
	       frame->id.raw, frame->buf[0], frame->buf[1],
	       frame->buf[2], frame->buf[3], frame->buf[4], frame->buf[5],
	       frame->buf[6], frame->buf[7], frame->len);
}

void caniot_explain_id(union caniot_id id)
{
	if (caniot_is_error(id)) {
		printf("Error");
		return;
	} else {
		printf("%s %s",
		       get_id_type_str(id.type),
		       get_id_query_str(id.query));
	}

	printf(" to device %s %s : %s / ",
	       get_id_class_str(id.cls),
	       get_id_dev_str(id.dev),
	       get_id_endpoint_str(id.endpoint));
}

void caniot_explain_frame(struct caniot_frame *frame)
{
	caniot_explain_id(frame->id);

	if ((frame->id.type == telemetry) || (frame->id.type == command)) {
		for (int i = 0; i < frame->len; i++) {
			printf("%02hhx ", frame->buf[i]);
		}
	} else {
		printf("LEN = %d, key = %x val = %x", frame->len,
		      frame->attr.key, frame->attr.val);
	}

	printf("\n");
}

void caniot_build_query_telemtry(struct caniot_frame *frame, union deviceid did)
{
	struct caniot_frame tmp = {
		.id = {
			.cls = did.cls,
			.dev = did.dev,
			.type = telemetry,
			.query = query,
			.endpoint = endpoint_default
		},
		.buf = {0, 0, 0, 0, 0, 0, 0, 0},
		.len = 0
	};

	memcpy(frame, &tmp, sizeof(struct caniot_frame));
}