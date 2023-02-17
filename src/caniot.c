/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <caniot/caniot.h>
#include <caniot/caniot_private.h>
#include <caniot/classes/class0.h>
#include <caniot/classes/class1.h>
#include <caniot/datatype.h>

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

static void cpy_str(char *dst, const char *flash_str, size_t len)
{
	const size_t copy_len = MIN(strlen_P(flash_str), len - 1);
	strncpy_P(dst, flash_str, copy_len);
	dst[copy_len] = '\0';
}

static inline void cpy_class_str(caniot_device_class_t class, char *buf, size_t len)
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

		CANIOT_INF(
			F("[%hhd] 0x%02x (cls=%s sid=%s)"), did, did, cls_str, sid_str);
#else
		CANIOT_INF(F("[%hhd] 0x%02x (cls=%s sid=%s)"),
			   did,
			   did,
			   get_class_str(CANIOT_DID_CLS(did)),
			   get_sid_str(CANIOT_DID_SID(did)));
#endif
	}
}

void caniot_show_frame(const struct caniot_frame *frame)
{
	CANIOT_INF(F("%x [ %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx ] len "
		     "= %d"),
		   caniot_id_to_canid(frame->id),
		   (uint8_t)frame->buf[0],
		   (uint8_t)frame->buf[1],
		   (uint8_t)frame->buf[2],
		   (uint8_t)frame->buf[3],
		   (uint8_t)frame->buf[4],
		   (uint8_t)frame->buf[5],
		   (uint8_t)frame->buf[6],
		   (uint8_t)frame->buf[7],
		   (uint8_t)frame->len);
}

void caniot_explain_id(caniot_id_t id)
{
	CANIOT_INF(F("[ 0x%x ] "), caniot_id_to_canid(id));
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
		CANIOT_INF(F(": -%04x"), (uint32_t)-frame->err);
		return;
	}

	if ((frame->id.type == CANIOT_FRAME_TYPE_TELEMETRY) ||
	    (frame->id.type == CANIOT_FRAME_TYPE_COMMAND)) {
		for (int i = 0; i < frame->len; i++) {
			CANIOT_INF(F("%02hhx "), (uint8_t)frame->buf[i]);
		}
	} else {
		CANIOT_INF(F("LEN = %d, key = %02x val = %04x"),
			   frame->len,
			   frame->attr.key,
			   frame->attr.val);
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
		ret = snprintf(buf,
			       len,
			       "%s %s ",
			       get_type_str(id.type),
			       get_query_str(id.query));
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
		ret = snprintf(buf, len, "ep : %s", get_endpoint_str(frame->id.endpoint));
		if (ret > (int)len || ret < 0) {
			return ret;
		}
		total += ret;
		buf += ret;
		len -= ret;

		for (int i = 0; i < frame->len; i++) {
			ret = snprintf(buf, len, " %02hhx", (uint8_t)frame->buf[i]);
			if (ret > (int)len || ret < 0) {
				return ret;
			}
			total += ret;
			buf += ret;
			len -= ret;
		}
	} else {
		ret = snprintf(buf,
			       len,
			       "LEN = %d, key = %02x val = %04x",
			       frame->len,
			       frame->attr.key,
			       frame->attr.val);
		if (ret > (int)len || ret < 0) {
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

/*____________________________________________________________________________*/

caniot_did_t caniot_frame_get_did(struct caniot_frame *frame)
{
	ASSERT(frame != NULL);

	return CANIOT_DID(frame->id.cls, frame->id.sid);
}

void caniot_frame_set_did(struct caniot_frame *frame, caniot_did_t did)
{
	ASSERT(frame != NULL);
	ASSERT(caniot_deviceid_valid(did) == true);

	frame->id.cls = CANIOT_DID_CLS(did);
	frame->id.sid = CANIOT_DID_SID(did);
}

int caniot_build_query_telemetry(struct caniot_frame *frame, uint8_t endpoint)
{
	ASSERT(frame);

	int ret = -CANIOT_EINVAL;
	if (caniot_endpoint_valid(endpoint) == true) {
		frame->id.type	   = CANIOT_FRAME_TYPE_TELEMETRY;
		frame->id.query	   = CANIOT_QUERY;
		frame->id.endpoint = endpoint;
		frame->len	   = 0U;
		ret		   = 0;
	}
	return ret;
}

int caniot_build_query_command(struct caniot_frame *frame,
			       uint8_t endpoint,
			       const uint8_t *buf,
			       uint8_t size)
{
	ASSERT(frame);
	ASSERT(buf);

	int ret = -CANIOT_EINVAL;
	if (caniot_endpoint_valid(endpoint) == true) {
		frame->id.type	   = CANIOT_FRAME_TYPE_COMMAND;
		frame->id.query	   = CANIOT_QUERY;
		frame->id.endpoint = endpoint;
		frame->len	   = MIN(size, sizeof(frame->buf));
		memcpy(frame->buf, buf, frame->len);
		ret = 0;
	}
	return ret;
}

int caniot_build_query_read_attribute(struct caniot_frame *frame, uint16_t key)
{
	ASSERT(frame);

	frame->id.type	= CANIOT_FRAME_TYPE_READ_ATTRIBUTE;
	frame->id.query = CANIOT_QUERY;
	frame->len	= 2u;
	frame->attr.key = key;

	return 0;
}

int caniot_build_query_write_attribute(struct caniot_frame *frame,
				       uint16_t key,
				       uint32_t value)
{
	ASSERT(frame);

	frame->id.type	= CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE;
	frame->len	= 6u;
	frame->attr.key = key;
	frame->attr.val = value;

	return 0;
}

/*____________________________________________________________________________*/

bool caniot_validate_drivers_api(struct caniot_drivers_api *api)
{
	return api->entropy && api->get_time && api->send && api->recv;
}

bool caniot_is_error(int cterr)
{
	return (cterr < 0) && ((uint32_t)cterr < CANIOT_ERROR_MAX) &&
	       ((uint32_t)cterr > CANIOT_ERROR_BASE);
}

bool caniot_device_is_target(caniot_did_t did, const struct caniot_frame *frame)
{
	return (frame->id.query == CANIOT_QUERY) &&
	       (((frame->id.cls == CANIOT_DID_CLS(did)) &&
		 (frame->id.sid == CANIOT_DID_SID(did))) ||
		((frame->id.cls == CANIOT_CLASS_BROADCAST) &&
		 (frame->id.sid == CANIOT_SUBID_BROADCAST)));
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

#if !defined(__AVR__)

int caniot_encode_deviceid(caniot_did_t did, uint8_t *buf, size_t len)
{
	return snprintf((char *)buf, len, F("0x%02hhx"), did);
}

#endif

int caniot_deviceid_cmp(caniot_did_t a, caniot_did_t b)
{
	return a - b;
}

bool caniot_deviceid_equal(caniot_did_t a, caniot_did_t b)
{
	return caniot_deviceid_cmp(a, b) == 0;
}

bool caniot_deviceid_valid(caniot_did_t did)
{
	return (did <= CANIOT_DID_MAX_VALUE);
}

bool caniot_deviceid_match(caniot_did_t dev, caniot_did_t pkt)
{
	return caniot_is_broadcast(pkt) || caniot_deviceid_equal(dev, pkt);
}

bool caniot_endpoint_valid(caniot_endpoint_t endpoint)
{
	return (endpoint <= CANIOT_ENDPOINT_BOARD_CONTROL);
}

uint16_t caniot_id_to_canid(caniot_id_t id)
{
	return CANIOT_ID(id.type, id.query, id.cls, id.sid, id.endpoint);
}

caniot_id_t caniot_canid_to_id(uint16_t canid)
{
	caniot_id_t id = {
		.type	  = CANIOT_ID_GET_TYPE(canid),
		.query	  = CANIOT_ID_GET_QUERY(canid),
		.cls	  = CANIOT_ID_GET_CLASS(canid),
		.sid	  = CANIOT_ID_GET_SUBID(canid),
		.endpoint = CANIOT_ID_GET_ENDPOINT(canid),
	};

	return id;
}

bool caniot_is_error_frame(caniot_id_t id)
{
	return id.query == CANIOT_RESPONSE &&
	       (id.type == CANIOT_FRAME_TYPE_COMMAND ||
		id.type == CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE);
}

bool is_telemetry_response(struct caniot_frame *frame)
{
	return (frame->id.query == CANIOT_RESPONSE) &&
	       (frame->id.type == CANIOT_FRAME_TYPE_TELEMETRY);
}

bool caniot_type_is_valid_response_of(caniot_frame_type_t resp, caniot_frame_type_t query)
{
	switch (query) {
	case CANIOT_FRAME_TYPE_COMMAND:
	case CANIOT_FRAME_TYPE_TELEMETRY:
		return resp == CANIOT_FRAME_TYPE_TELEMETRY;
		break;

	case CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE:
	case CANIOT_FRAME_TYPE_READ_ATTRIBUTE:
		return resp == CANIOT_FRAME_TYPE_READ_ATTRIBUTE;
		break;
	default:
		return false;
	}
}

caniot_query_response_type_t caniot_type_what_response_of(caniot_frame_type_t resp,
							  caniot_frame_type_t query)
{
	/* matrix representing how the current
	 * response type matches the query type
	 */

	/*    Query  (row) | Response (columns) | Result
	 * ---------------------------------
	 *    	  | C | T | W | R |
	 *  	C | 1 | 0 | 3 | 2 |
	 * 	T | 1 | 0 | 3 | 2 |
	 * 	W | 3 | 2 | 1 | 0 |
	 * 	R | 3 | 2 | 1 | 0 |
	 * ---------------------------------
	 *
	 * 	0: CANIOT_IS_RESPONSE
	 * 	1: CANIOT_IS_ERROR
	 * 	2: CANIOT_IS_OTHER_RESPONSE
	 * 	3: CANIOT_IS_OTHER_ERROR
	 */

	const bool is_error = (resp == CANIOT_FRAME_TYPE_COMMAND) ||
			      (resp == CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE);

	const bool is_response_of = ((resp ^ query) & 2) == 0;

	return (caniot_query_response_type_t)((is_error ? 1 : 0) |
					      (is_response_of ? 0 : 2));
}

bool caniot_type_is_response_of(caniot_frame_type_t resp,
				caniot_frame_type_t query,
				bool *iserror)
{
	/* VARIANT 2 */

	bool match		    = false;
	caniot_frame_type_t errtype = CANIOT_FRAME_TYPE_COMMAND;

	switch (query) {
	case CANIOT_FRAME_TYPE_COMMAND:
	case CANIOT_FRAME_TYPE_TELEMETRY:
		errtype = CANIOT_FRAME_TYPE_COMMAND;
		match	= resp == CANIOT_FRAME_TYPE_TELEMETRY;
		break;

	case CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE:
	case CANIOT_FRAME_TYPE_READ_ATTRIBUTE:
		errtype = CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE;
		match	= resp == CANIOT_FRAME_TYPE_READ_ATTRIBUTE;
		break;
	default:
		break;
	}

	if (iserror != NULL) {
		*iserror = errtype == resp;
	}

	return match;
}

caniot_frame_type_t caniot_resp_error_for(caniot_frame_type_t query)
{
	switch (query) {
	case CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE:
	case CANIOT_FRAME_TYPE_READ_ATTRIBUTE:
		return CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE;

	case CANIOT_FRAME_TYPE_COMMAND:
	case CANIOT_FRAME_TYPE_TELEMETRY:
	default:
		return CANIOT_FRAME_TYPE_COMMAND;
	}
}

void caniot_test(void)
{
	printf("caniot test\n");
}

caniot_error_t caniot_interpret_error(int err, bool *forwarded)
{
	bool zforwarded = false;

	if (caniot_is_error(err)) {
		err = -err - CANIOT_ERROR_BASE;

		if ((err & CANIOT_ERROR_DEVICE_MASK) != 0) {
			err &= ~CANIOT_ERROR_DEVICE_MASK;
			zforwarded = true;
		}
	}

	if (forwarded != NULL) {
		*forwarded = zforwarded;
	}

	return (caniot_error_t)(err);
}

int caniot_cmd_blc1_init(struct caniot_blc1_command *cmd)
{
	return caniot_cmd_blc1_clear(cmd);
}

int caniot_cmd_blc1_set_xps(struct caniot_blc1_command *cmd,
			    uint8_t n,
			    caniot_complex_digital_cmd_t xps)
{
#if CONFIG_CANIOT_CHECKS_ENABLED
	if (cmd == NULL) {
		return -CANIOT_EINVAL;
	}

	if (n >= CANIOT_CLASS1_IO_COUNT) {
		return -CANIOT_EINVAL;
	}
#endif

	const uint8_t msb_index	   = n * 3u;
	const uint8_t msb_offset   = msb_index & 0x7u;
	const uint8_t msb_rem_size = 8u - msb_offset;
	const uint8_t byte_n	   = msb_index >> 3u;

	cmd->data[byte_n] |= ((xps & 0x7u) << msb_offset) & 0xffu;

	if ((msb_rem_size < 3u) && ((byte_n + 1u) <= 6u)) {
		cmd->data[byte_n + 1u] |= ((xps & 0x7u) >> msb_rem_size);
	}

	return 0;
}

int caniot_cmd_blc1_clear(struct caniot_blc1_command *cmd)
{
#if CONFIG_CANIOT_CHECKS_ENABLED
	if (cmd == NULL) {
		return -CANIOT_EINVAL;
	}
#endif

	memset(cmd, 0, sizeof(struct caniot_blc1_command));

	return 0;
}

caniot_complex_digital_cmd_t caniot_cmd_blc1_parse_xps(struct caniot_blc1_command *cmd,
						       uint8_t n)
{
#if CONFIG_CANIOT_CHECKS_ENABLED
	if (cmd == NULL) {
		return -EINVAL;
	}

	if (n >= CANIOT_CLASS1_IO_COUNT) {
		return -EINVAL;
	}
#endif
	caniot_complex_digital_cmd_t xps = CANIOT_XPS_NONE;

	const uint8_t msb_index	   = n * 3u;
	const uint8_t msb_offset   = msb_index & 0x7u;
	const uint8_t msb_rem_size = 8u - msb_offset;
	const uint8_t byte_n	   = msb_index >> 3u;

	xps = (cmd->data[byte_n] >> msb_offset) & 0x7u;

	if ((msb_rem_size < 3u) && ((byte_n + 1u) <= 6u)) {
		xps |= (cmd->data[byte_n + 1u] << msb_rem_size) & 0x7u;
	}

	return xps;
}