#include "caniot.h"

#include <errno.h>
#include <memory.h>

#ifdef __AVR__
#define RO_SECTION	PROGMEM
#else
#define RO_SECTION	
#endif

#define ATTR_IDENTIFICATION 0
#define ATTR_SYSTEM 1
#define ATTR_CONFIG 2

#define ATTR_KEY_SECTION(key) ((uint8_t)(((uint16_t)key) >> 12))
#define ATTR_KEY_ATTR(key) ((uint8_t)((((uint16_t)key) >> 4) & 0xFF))
#define ATTR_KEY_PART(key) ((uint8_t)(((uint16_t)key) & 0xF))

/* A part is is 4B */
#define ATTR_KEY_OFFSET(key) (ATTR_KEY_PART(key) << 2)

#define ATTR_KEY(section, attr, part) ((section & 0xF) << 12 | (attr & 0xFF) << 4 | (part & 0xF))

typedef uint16_t key_t;

enum attr_option
{
    WRITABLE = 0,
    RAM = 1 << 0,
    EEPROM = 1 << 1,
    PROGMEMORY = 1 << 2,
    READONLY = 1 << 3,
    PRIVATE = 1 << 4, // unimplemented
};

union attr_value
{
    uint32_t u32;
    uint16_t u16;
    uint8_t u8;
};

struct attr_ref
{
    uint8_t section;
    enum attr_option option;
    uint8_t offset;
    uint8_t read_size;
};

struct attribute
{
	uint8_t offset;
	uint8_t size;
	uint8_t readonly;
	char name[32];
};

struct attr_section
{
	uint8_t option;
	char name[16];
	const struct attribute *array;
	uint8_t array_size;
};


#define ATTRIBUTE(s, readonly, name, param) 	\
    {                                           \
        (uint8_t) offsetof(s, param),       	\
            (uint8_t)MEMBER_SIZEOF(s, param),   \
            (uint8_t)readonly ? READONLY : 0,   \
            name                                \
    }

#define SECTION(options, name, array) \
    {                                 \
        options,                      \
            name,                     \
            array,                    \
            ARRAY_SIZE(array)         \
    }

static const struct attribute identification_attr[] RO_SECTION = {
    ATTRIBUTE(struct caniot_identification, READONLY, "nodeid", nodeid),
    ATTRIBUTE(struct caniot_identification, READONLY, "version", version),
    ATTRIBUTE(struct caniot_identification, READONLY, "name", name),
};

static const struct attribute system_attr[] RO_SECTION = {
    ATTRIBUTE(struct caniot_system, READONLY, "uptime", uptime),
    ATTRIBUTE(struct caniot_system, WRITABLE, "abstime", abstime),
    ATTRIBUTE(struct caniot_system, READONLY, "calculated_abstime", calculated_abstime),
    ATTRIBUTE(struct caniot_system, READONLY, "uptime_shift", uptime_shift),
    ATTRIBUTE(struct caniot_system, READONLY, "last_telemetry", last_telemetry),
    ATTRIBUTE(struct caniot_system, READONLY, "received.total", received.total),
    ATTRIBUTE(struct caniot_system, READONLY, "received.read_attribute", received.read_attribute),
    ATTRIBUTE(struct caniot_system, READONLY, "received.write_attribute", received.write_attribute),
    ATTRIBUTE(struct caniot_system, READONLY, "received.command", received.command),
    ATTRIBUTE(struct caniot_system, READONLY, "received.request_telemetry", received.request_telemetry),
    ATTRIBUTE(struct caniot_system, READONLY, "received.processed", received.processed),
    ATTRIBUTE(struct caniot_system, READONLY, "received.query_failed", received.query_failed),
    ATTRIBUTE(struct caniot_system, READONLY, "sent.total", sent.total),
    ATTRIBUTE(struct caniot_system, READONLY, "sent.telemetry", sent.telemetry),
    ATTRIBUTE(struct caniot_system, READONLY, "sent.events", events.total),
    ATTRIBUTE(struct caniot_system, READONLY, "last_query_error", last_query_error),
    ATTRIBUTE(struct caniot_system, READONLY, "last_telemetry_error", last_telemetry_error),
    ATTRIBUTE(struct caniot_system, READONLY, "last_event_error", last_event_error),
    ATTRIBUTE(struct caniot_system, READONLY, "battery", battery),
};

static const struct attribute config_attr[] RO_SECTION = {
    ATTRIBUTE(struct caniot_config, WRITABLE, "telemetry_period", telemetry_period),
    ATTRIBUTE(struct caniot_config, WRITABLE, "telemetry_rdm_delay", telemetry_rdm_delay),
    ATTRIBUTE(struct caniot_config, WRITABLE, "telemetry_min", telemetry_min),
};

static const struct attr_section attr_sections[] RO_SECTION = {
    SECTION(RAM | PROGMEMORY | READONLY, "identification", identification_attr),
    SECTION(RAM, "system", system_attr),
    SECTION(EEPROM, "configuration", config_attr),
};

static inline uint8_t attr_get_section_size(const struct attr_section *section)
{
#ifdef __AVR__
	return pgm_read_byte(&section->array_size);
#else
	return section->array_size;
#endif
}

static inline const struct attribute *attr_get_section_array(
	const struct attr_section *section)
{
#ifdef __AVR__
	return pgm_read_word(&section->array);
#else
	return section->array;
#endif
}

static inline uint8_t attr_get_section_option(const struct attr_section *section) {
#ifdef __AVR__
	return pgm_read_byte(&section->option);
#else
	return section->option;
#endif
}

static inline uint8_t attr_get_size(const struct attribute *attr) {
#ifdef __AVR__
	return pgm_read_byte(&attr->size);
#else
	return attr->size;
#endif
}

static inline uint8_t attr_get_offset(const struct attribute *attr) {
#ifdef __AVR__
	return pgm_read_byte(&attr->offset);
#else
	return attr->offset;
#endif
}

static const struct attr_section *attr_get_section(key_t key)
{
	uint8_t index = ATTR_KEY_SECTION(key);
	if (index < ARRAY_SIZE(attr_sections)) {
		return &attr_sections[index];
	}
	return NULL;
}

static const struct attribute *attr_get(key_t key, const struct attr_section *section)
{
	uint8_t index = ATTR_KEY_ATTR(key);
	if (index < attr_get_section_size(section)) {
		return &attr_get_section_array(section)[index];
	}
	return NULL;
}

static int attr_resolve(key_t key, struct attr_ref *ref)
{
	const struct attr_section *section = attr_get_section(key);
	if (!section) {
		return -EINVAL; /* TODO return accurate error CANIOT_EKEYSECTION */
	}
	
	const struct attribute *attr = attr_get(key, section);
	if (!attr) {
		return -EINVAL; /* TODO return accurate error CANIOT_EKEYATTR */
	}

	uint8_t attr_size = attr_get_size(attr);
	if (ATTR_KEY_OFFSET(key) >= attr_size) {
		return -EINVAL; /* TODO return accurate error CANIOT_EKEYPART */
	}

	ref->section = ATTR_KEY_SECTION(key);
	ref->option = attr_get_section_option(section);
	ref->read_size = MIN(attr_size, 4u);
	ref->offset = ATTR_KEY_OFFSET(key) + attr_get_offset(attr);
	
	return 0;
}

static int attr_read(const struct attr_ref *ref, union attr_value value)
{
	return 0;
}

static int attr_write(const struct attr_ref *ref, union attr_value value)
{
	return 0;
}

static bool is_error_response(struct caniot_frame *frame) {
	return frame->id.query == response && frame->id.type == command;
}

static bool is_telemetry_response(struct caniot_frame *frame) {
	return frame->id.query == response && frame->id.type == telemetry;
}

static void clean_frame(struct caniot_frame *frame)
{
	memset(frame, 0x00, sizeof(struct caniot_frame));
}

static void prepare_response(struct caniot_device *dev,
			     struct caniot_frame *resp,
			     uint8_t type)
{
	clean_frame(resp);

	/* id */
	resp->id.type = type;
	resp->id.query = response;
	resp->id.cls = dev->identification.node.cls;
	resp->id.dev = dev->identification.node.dev;
	resp->id.endpoint = 0;
}

static void prepare_error(struct caniot_device *dev,
			  struct caniot_frame *resp,
			  int error)
{
	prepare_response(dev, resp, command);

	resp->err = (int32_t)error;
}

static int telemetry_resp(struct caniot_device *dev,
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

static int attribute_resp(struct caniot_device *dev,
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

static uint32_t get_random_u32_range(struct caniot_device *dev,
				     uint32_t a, uint32_t b)
{
	uint32_t rng;
	dev->api->entropy((uint8_t *)&rng, sizeof(rng));

	return a + rng % (b - a);
}

static uint32_t get_telemetry_delay(struct caniot_device *dev)
{
	if (!dev->config.telemetry_rdm_delay) {
		return dev->config.telemetry_min;
	}

	return get_random_u32_range(dev, dev->config.telemetry_min,
				    dev->config.telemetry_rdm_delay);
}

int process_dev_rx_frame(struct caniot_device *dev,
			 struct caniot_frame *req,
			 struct caniot_frame *resp)
{
	int ret;

	/* no response in this case */
	if (req->id.query != query) {
		ret = -EINVAL;
		goto error;
	}

	switch (req->id.type) {
	case command:
		ret = dev->api->command_handler(dev, req->id.endpoint,
					req->buf, req->len);
		if (ret != 0) {
		}

	case telemetry:
		ret = telemetry_resp(dev, resp, req->id.endpoint);
		break;

	case write_attribute:
		ret = dev->api->write_attribute(dev, req->attr.key,
						req->attr.val);
		if (ret != 0) {
			goto error;
		}

	case read_attribute:
		ret = attribute_resp(dev, resp, req->attr.key);
		break;
	}

	return 0;

error:
	prepare_error(dev, resp, ret);
	return ret;
}

int caniot_process(struct caniot_device *dev)
{
	int ret;
	struct caniot_frame req;
	struct caniot_frame resp;
	uint32_t delay = 0;

	clean_frame(&req);
	
	ret = dev->driv->recv(&req);
	if (ret) {
		goto exit; /* failed to receive */
	}

	ret = process_dev_rx_frame(dev, &req, &resp);

	/* If CANIOT error frames should be sent in case of error or not */
	if (ret && !dev->config.error_response) {
		goto exit;
	}

	if (is_telemetry_response(&resp)) {
		/* if there is a telemetry response which is not already sent
		 * we don't queue a new telemetry response
		 */
		if (dev->api->pending_telemetry(dev)) {
			goto exit;
		}

		delay = get_telemetry_delay(dev);
	}
	
	/* todo, how to delay response ? */
	ret = dev->driv->send(&resp, delay);
	if (ret) {
		goto exit; /* failed to send */
	}
exit:
	return ret;
}