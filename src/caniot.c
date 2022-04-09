#include <caniot/caniot.h>
#include <caniot/archutils.h>

static const char cls_str[][2] ROM = {
	"C0",
	"C1",
	"C2",
	"C3",
	"C4",
	"C5",
	"C6",
	"C7",
};

static const char sid_str[][2] ROM = {
	"D0",
	"D1",
	"D2",
	"D3",
	"D4",
	"D5",
	"D6",
	"D7",
};

static const char *get_unknown(void)
{
	return F("Unkown");
}

static const char *get_type_str(uint8_t type)
{
	switch (type) {
	case command:
		return F("Command");
	case telemetry:
		return F("Telemetry");
	case write_attribute:
		return F("Write-attribute");
	case read_attribute:
		return F("Read-attribute");
	default:
		return get_unknown();
	}
}

static const char *get_query_str(uint8_t q)
{
	switch (q) {
	case query:
		return F("Query");
	case response:
		return F("Response");
	default:
		return get_unknown();
	}
}

static const char *get_class_str(uint8_t class)
{
	if (class >= ARRAY_SIZE(cls_str)) {
		class = ARRAY_SIZE(cls_str) - 1;
	}

	return cls_str[class];
}

static void cpy_class_str(uint8_t class, char buf[2])
{
	memcpy_P(buf, get_class_str(class), 2);
}

static const char *get_sid_str(uint8_t sid)
{
	if (sid >= ARRAY_SIZE(sid_str)) {
		sid = ARRAY_SIZE(sid_str) - 1;
	}

	return sid_str[sid];
}

static void cpy_sid_str(uint8_t sid, char buf[2])
{
	memcpy_P(buf, get_sid_str(sid), 2);
}

static const char *get_endpoint_str(uint8_t endpoint)
{
	switch (endpoint) {
	case endpoint_app:
		return F("ep-0");
	case endpoint_1:
		return F("ep-1");
	case endpoint_2:
		return F("ep-2");
	case endpoint_board_control:
		return F("ep-c");
	default:
		return get_unknown();
	}
}

void caniot_show_deviceid(union deviceid did)
{
	if (caniot_valid_deviceid(did)) {
		if (CANIOT_DEVICE_IS_BROADCAST(did)) {
			printf(F("BROADCAST"));
		} else {
			char cls[3], sid[3];
			cpy_class_str(did.cls, cls);
			cpy_sid_str(did.sid, sid);

			cls[2] = '\0';
			sid[2] = '\0';

			printf(F("[%hhd] 0x%02x (cls=%s sid=%s)"),
			       did.val, did.val, cls, sid);
		}
	} else {
		printf(F("invalid did"));
	}
}

void caniot_show_frame(const struct caniot_frame *frame)
{
	printf(F("%x [ %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx ] len = %d"),
	       frame->id.raw, (uint8_t)frame->buf[0], (uint8_t)frame->buf[1],
	       (uint8_t)frame->buf[2], (uint8_t)frame->buf[3], (uint8_t)frame->buf[4],
	       (uint8_t)frame->buf[5], (uint8_t)frame->buf[6], (uint8_t)frame->buf[7],
	       (uint8_t)frame->len);
}

void caniot_explain_id(union caniot_id id)
{
	printf(F("[ %x ] "), id.raw);
	if (caniot_is_error_frame(id)) {
		printf(F("Error frame "));
		return;
	} else {
		prinf("%s %s ", get_type_str(id.type), get_query_str(id.query));
	}

	caniot_show_deviceid(CANIOT_DEVICE(id.cls, id.sid));

	printf(F(" : %s / "), get_endpoint_str(id.endpoint));
}

void caniot_explain_frame(const struct caniot_frame *frame)
{
	caniot_explain_id(frame->id);

	if (caniot_is_error_frame(frame->id)) {
		printf(F(": -%04x \n"), (uint32_t) -frame->err);
		return;
	}

	if ((frame->id.type == telemetry) || (frame->id.type == command)) {
		for (int i = 0; i < frame->len; i++) {
			printf(F("%02hhx "), (uint8_t) frame->buf[i]);
		}
	} else {
		printf(F("LEN = %d, key = %02x val = %04lx"), frame->len,
		       frame->attr.key, frame->attr.val);
	}
}

void caniot_build_query_telemetry(struct caniot_frame *frame,
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

bool caniot_validate_drivers_api(struct caniot_drivers_api *api)
{
	return api->entropy && api->get_time && 
		api->send && api->recv;
}

bool caniot_is_error(int cterr)
{
	return (-cterr >= CANIOT_ERROR_BASE && -cterr <= (CANIOT_ERROR_BASE + 0xFF));
}

void caniot_show_error(int cterr)
{
	if (cterr == 0) {
		return;
	}
	
	if (caniot_is_error(cterr) == false) {
		printf(F("Error -%04x (%d)\n"), -cterr, cterr);
	} else {
		// TODO show error name foreach error

		printf(F("CANIOT -%04x\n"), -cterr);
	}
}