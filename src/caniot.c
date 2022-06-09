#include <caniot/caniot.h>
#include <caniot/archutils.h>

static const char cls_str[][3U] ROM = {
	"C0",
	"C1",
	"C2",
	"C3",
	"C4",
	"C5",
	"C6",
	"C7",
};

static const char sid_str[][3U] ROM = {
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

static const char *get_class_str(caniot_device_class_t class)
{
	if (class >= ARRAY_SIZE(cls_str)) {
		class = ARRAY_SIZE(cls_str) - 1;
	}

	return cls_str[class];
}

static const char *get_sid_str(caniot_device_subid_t sid)
{
	if (sid >= ARRAY_SIZE(sid_str)) {
		sid = ARRAY_SIZE(sid_str) - 1;
	}

	return sid_str[sid];
}

static void cpy_str(char *dst,
		    const char *flash_str,
		    size_t len)
{
	const size_t copy_len = MIN(strlen_P(flash_str), len - 1);
	strncpy_P(dst, flash_str, copy_len);
	dst[copy_len] = '\0';
}

static inline void cpy_class_str(caniot_device_class_t class,
				 char *buf,
				 size_t len)
{
	cpy_str(buf, get_class_str(class), len);
}

static inline void cpy_sid_str(caniot_device_subid_t sid, char *buf, size_t len)
{
	cpy_str(buf, get_sid_str(sid), len);
}

static inline void cpy_type_str(caniot_frame_type_t type, char *buf, size_t len)
{
	cpy_str(buf, get_type_str(type), len);
}

static inline void cpy_query_str(caniot_frame_dir_t q, char *buf, size_t len)
{
	cpy_str(buf, get_query_str(q), len);
}

static inline void cpy_endpoint_str(caniot_endpoint_t endpoint, char *buf, size_t len)
{
	cpy_str(buf, get_endpoint_str(endpoint), len);
}

void caniot_show_deviceid(caniot_did_t did)
{
	if (CANIOT_DEVICE_IS_BROADCAST(did)) {
		CANIOT_INF(F("BROADCAST"));
	} else {
#if defined(__AVR__)
		char cls_str[3], sid_str[3];
		cpy_class_str(CANIOT_DID_CLS(did), cls_str, sizeof(cls_str));
		cpy_sid_str(CANIOT_DID_SID(did), sid_str, sizeof(sid_str));

		CANIOT_INF(F("[%hhd] 0x%02x (cls=%s sid=%s)"),
			   did, did, cls_str, sid_str);
#else
		CANIOT_INF(F("[%hhd] 0x%02x (cls=%s sid=%s)"),
			   did, did, get_class_str(CANIOT_DID_CLS(did)),
			   get_sid_str(CANIOT_DID_SID(did)));
#endif
	}
}

void caniot_show_frame(const struct caniot_frame *frame)
{
	CANIOT_DBG(F("%x [ %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx ] len = %d"),
		   caniot_id_to_canid(frame->id), (uint8_t)frame->buf[0], (uint8_t)frame->buf[1],
		   (uint8_t)frame->buf[2], (uint8_t)frame->buf[3], (uint8_t)frame->buf[4],
		   (uint8_t)frame->buf[5], (uint8_t)frame->buf[6], (uint8_t)frame->buf[7],
		   (uint8_t)frame->len);
}

void caniot_explain_id(caniot_id_t id)
{
	CANIOT_INF(F("[ %x ] "), caniot_id_to_canid(id));
	if (caniot_is_error_frame(id) == true) {
		CANIOT_INF(F("Error frame "));
		return;
	} else {
#if defined(__AVR__)
		char type_str[11], query_str[9];
		cpy_type_str(id.type, type_str, sizeof(type_str));
		cpy_query_str(id.query, query_str, sizeof(query_str));
		CANIOT_INF(F("%s %s "), type_str, query_str);
#else
		CANIOT_INF("%s %s ", get_type_str(id.type), get_query_str(id.query));
#endif
	}

	caniot_show_deviceid(CANIOT_DID(id.cls, id.sid));

#if defined(__AVR__)
	char ep_str[5];
	cpy_endpoint_str(id.endpoint, ep_str, sizeof(ep_str));
	CANIOT_INF(F(" : %s / "), ep_str);
#else
	CANIOT_INF(" : %s / ", get_endpoint_str(id.endpoint));
#endif
}

void caniot_explain_frame(const struct caniot_frame *frame)
{
	caniot_explain_id(frame->id);

	if (caniot_is_error_frame(frame->id)) {
		CANIOT_INF(F(": -%04x \n"), (uint32_t)-frame->err);
		return;
	}

	if ((frame->id.type == CANIOT_FRAME_TYPE_TELEMETRY) ||
	    (frame->id.type == CANIOT_FRAME_TYPE_COMMAND)) {
		for (int i = 0; i < frame->len; i++) {
			CANIOT_INF(F("%02hhx "), (uint8_t)frame->buf[i]);
		}
	} else {
		CANIOT_INF(F("LEN = %d, key = %02x val = %04x"), frame->len,
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
	if (ret > (int)len || ret < 0) {
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
		if (ret > (int)len || ret < 0) {
			return ret;
		}
		total += ret;
		buf += ret;
		len -= ret;

		for (int i = 0; i < frame->len; i++) {
			ret = snprintf(buf, len, " %02hhx", (uint8_t)frame->buf[i]);
			if (ret > (int) len || ret < 0) {
				return ret;
			}
			total += ret;
			buf += ret;
			len -= ret;
		}
	} else {
		ret = snprintf(buf, len, "LEN = %d, key = %02x val = %04x",
			       frame->len, frame->attr.key, frame->attr.val);
		if (ret > (int) len || ret < 0) {
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
		return -CANIOT_EINVAL;
	}

	return total;
}
#endif

void caniot_build_query_telemetry(struct caniot_frame *frame,
				  caniot_did_t did,
				  uint8_t endpoint)
{
	struct caniot_frame tmp = {
		.id = {
			.cls = CANIOT_DID_CLS(did),
			.sid = CANIOT_DID_SID(did),
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
				caniot_did_t did,
				uint8_t endpoint,
				const uint8_t *buf,
				uint8_t size)
{
	struct caniot_frame tmp = {
		.id = {
			.cls = CANIOT_DID_CLS(did),
			.sid = CANIOT_DID_SID(did),
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

bool caniot_device_is_target(caniot_did_t did,
			     const struct caniot_frame *frame)
{
	return ((frame->id.query == CANIOT_QUERY) &&
		(frame->id.cls == CANIOT_DID_CLS(did)) &&
		(frame->id.sid == CANIOT_DID_SID(did))) ||
		((frame->id.cls == CANIOT_CLASS_BROADCAST) &&
		 (frame->id.sid == CANIOT_SUBID_BROADCAST));
}

bool caniot_controller_is_target(const struct caniot_frame *frame)
{
	return frame->id.query == CANIOT_RESPONSE;
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

int caniot_encode_deviceid(caniot_did_t did, uint8_t *buf, size_t len)
{
	return snprintf((char *)buf, len, F("0x%02hhx"), did);
}

int caniot_deviceid_cmp(caniot_did_t a, caniot_did_t b)
{
	return a - b;
}

bool caniot_deviceid_equal(caniot_did_t a, caniot_did_t b)
{
	return caniot_deviceid_cmp(a, b) == 0;
}

void caniot_test(void)
{
	printf("caniot test\n");
}