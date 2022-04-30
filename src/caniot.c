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

static const char *get_type_str(caniot_frame_type_t type)
{
	switch (type) {
	case CANIOT_FRAME_TYPE_COMMAND:
		return F("Command");
	case CANIOT_FRAME_TYPE_TELEMETRY:
		return F("Telemetry");
	case CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE:
		return F("Write-attr");
	case CANIOT_FRAME_TYPE_READ_ATTRIBUTE:
		return F("Read-attr");
	default:
		return get_unknown();
	}
}

static const char *get_query_str(caniot_frame_dir_t q)
{
	switch (q) {
	case CANIOT_QUERY:
		return F("Query");
	case CANIOT_RESPONSE:
		return F("Response");
	default:
		return get_unknown();
	}
}

static const char *get_class_str(caniot_device_class_t class)
{
	if (class >= ARRAY_SIZE(cls_str)) {
		class = ARRAY_SIZE(cls_str) - 1;
	}

	return cls_str[class];
}

static void cpy_class_str(caniot_device_class_t class, char buf[2])
{
	memcpy_P(buf, get_class_str(class), 2);
}

static const char *get_sid_str(caniot_device_subid_t sid)
{
	if (sid >= ARRAY_SIZE(sid_str)) {
		sid = ARRAY_SIZE(sid_str) - 1;
	}

	return sid_str[sid];
}

static void cpy_sid_str(caniot_device_subid_t sid, char buf[2])
{
	memcpy_P(buf, get_sid_str(sid), 2);
}

static const char *get_endpoint_str(caniot_endpoint_t endpoint)
{
	switch (endpoint) {
	case CANIOT_ENDPOINT_APP:
		return F("ep-0");
	case CANIOT_ENDPOINT_1:
		return F("ep-1");
	case CANIOT_ENDPOINT_2:
		return F("ep-2");
	case CANIOT_ENDPOINT_BOARD_CONTROL:
		return F("ep-c");
	default:
		return get_unknown();
	}
}

void caniot_show_deviceid(union deviceid did)
{
	if (caniot_valid_deviceid(did)) {
		if (CANIOT_DEVICE_IS_BROADCAST(did)) {
			CANIOT_INF(F("BROADCAST"));
		} else {
			char cls[3], sid[3];
			cpy_class_str(did.cls, cls);
			cpy_sid_str(did.sid, sid);

			cls[2] = '\0';
			sid[2] = '\0';

			CANIOT_INF(F("[%hhd] 0x%02x (cls=%s sid=%s)"),
			       did.val, did.val, cls, sid);
		}
	} else {
		CANIOT_INF(F("invalid did"));
	}
}

void caniot_show_frame(const struct caniot_frame *frame)
{
	CANIOT_DBG(F("%x [ %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx ] len = %d"),
	       frame->id.raw, (uint8_t)frame->buf[0], (uint8_t)frame->buf[1],
	       (uint8_t)frame->buf[2], (uint8_t)frame->buf[3], (uint8_t)frame->buf[4],
	       (uint8_t)frame->buf[5], (uint8_t)frame->buf[6], (uint8_t)frame->buf[7],
	       (uint8_t)frame->len);
}

void caniot_explain_id(caniot_id_t id)
{
	CANIOT_INF(F("[ %x ] "), id.raw);
	if (caniot_is_error_frame(id) == true) {
		CANIOT_INF(F("Error frame "));
		return;
	} else {
#if defined(__AVR__)
		CANIOT_INF(F("%d %d "), id.type, id.query);
#else
		CANIOT_INF("%s %s ", get_type_str(id.type), get_query_str(id.query));
#endif
	}

	caniot_show_deviceid(CANIOT_DEVICE(id.cls, id.sid));

#if defined(__AVR__)
		CANIOT_INF(F(" : %d / "), id.endpoint);
#else
		CANIOT_INF(" : %s / ", get_endpoint_str(id.endpoint));
#endif
}

void caniot_explain_frame(const struct caniot_frame *frame)
{
	caniot_explain_id(frame->id);

	if (caniot_is_error_frame(frame->id)) {
		CANIOT_INF(F(": -%04x \n"), (uint32_t) -frame->err);
		return;
	}

	if ((frame->id.type == CANIOT_FRAME_TYPE_TELEMETRY) ||
	    (frame->id.type == CANIOT_FRAME_TYPE_COMMAND)) {
		for (int i = 0; i < frame->len; i++) {
			CANIOT_INF(F("%02hhx "), (uint8_t) frame->buf[i]);
		}
	} else {
		CANIOT_INF(F("LEN = %d, key = %02x val = %04lx"), frame->len,
		       frame->attr.key, frame->attr.val);
	}
}

#if !defined(__AVR__)

int caniot_explain_id_str(caniot_id_t id, char *buf, size_t len)
{
	int ret;
	size_t total = 0U;

	ret = snprintf(buf, len, "[ %x ] ", caniot_id_to_canid(id));
	if (ret < 0) {
		return ret;
	}

	total += ret;
	buf += ret;
	len -= ret;

	if (caniot_is_error_frame(id) == true) {
		ret = snprintf(buf, len, "Error frame ");
	} else {
		ret = snprintf(buf, len, "%s %s ",
			       get_type_str(id.type), get_query_str(id.query));
	}

	if (ret < 0) {
		return ret;
	}

	total += ret;
	// buf += ret;
	// len -= ret;

	return total;
}

int caniot_explain_frame_str(const struct caniot_frame *frame, char *buf, size_t len)
{
	int ret;
	size_t total = 0U;

	ret = caniot_explain_id_str(frame->id, buf, len);
	if (ret < 0) {
		return ret;
	}

	total += ret;
	buf += ret;
	len -= ret;

	if (caniot_is_error_frame(frame->id)) {
		ret = snprintf(buf, len, ": -%04x", (uint32_t)-frame->err);
		total += ret;
		buf += ret;
		len -= ret;
	} else if ((frame->id.type == CANIOT_FRAME_TYPE_TELEMETRY) ||
		   (frame->id.type == CANIOT_FRAME_TYPE_COMMAND)) {
		ret = snprintf(buf, len, "ep : %s", 
			       get_endpoint_str(frame->id.endpoint));
		total += ret;
		buf += ret;
		len -= ret;

		for (int i = 0; i < frame->len; i++) {
			ret = snprintf(buf, len, " %02hhx", (uint8_t)frame->buf[i]);
			if (ret < 0) {
				return ret;
			}
			total += ret;
			buf += ret;
			len -= ret;
		}
	} else {
		ret = snprintf(buf, len, "LEN = %d, key = %02x val = %04x",
			       frame->len, frame->attr.key, frame->attr.val);
		if (ret < 0) {
			return ret;
		}
		total += ret;
		buf += ret;
		len -= ret;
	}

	if (len > 0) {
		buf[0] = '\0';
		total++;
	} else {
		return -1;
	}

	return total;
}
#endif

void caniot_build_query_telemetry(struct caniot_frame *frame,
				 union deviceid did,
				 uint8_t endpoint)
{
	struct caniot_frame tmp = {
		.id = {
			.cls = did.cls,
			.sid = did.sid,
			.type = CANIOT_FRAME_TYPE_TELEMETRY,
			.query = CANIOT_QUERY,
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
			.type = CANIOT_FRAME_TYPE_COMMAND,
			.query = CANIOT_QUERY,
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
		CANIOT_DBG(F("Error -%04x (%d)\n"), -cterr, cterr);
	} else {
		// TODO show error name foreach error

		CANIOT_DBG(F("CANIOT -%04x\n"), -cterr);
	}
}

int caniot_encode_deviceid(union deviceid did, uint8_t *buf, size_t len)
{
	return snprintf((char *)buf, len, F("0x%02hhx"), did.val);
}

int caniot_deviceid_cmp(union deviceid a, union deviceid b)
{
	return a.val - b.val;
}

bool caniot_deviceid_equal(union deviceid a, union deviceid b)
{
	return caniot_deviceid_cmp(a, b) == 0;
}