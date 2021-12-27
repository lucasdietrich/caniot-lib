#include "device.h"

#include <errno.h>
#include <string.h>
#include <class.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#define ROM	PROGMEM
#else
#define ROM	
#endif

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

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

enum section {
	section_identification = 0,
	section_system = 1,
	section_config = 2
};

enum section_option
{
	READONLY = 1 << 0,
	VOLATILE = 1 << 1,
	PERSISTENT = 1 << 2
};

enum attr_option
{
	READABLE = 1 << 0,
	WRITABLE = 1 << 1,
};

static void attr_option_adjust(enum attr_option *attr_opt,
			       enum section_option sec_opt)
{
	if (sec_opt & READONLY) {
		*attr_opt &= ~WRITABLE;
	}
}

struct attr_ref
{
	enum attr_option option;
	enum section_option section_option;
	uint8_t section;
	uint8_t offset;
	uint8_t read_size;
};

struct attribute
{
	uint8_t offset;
	uint8_t size;
	uint8_t option;
	char name[32];
};

struct attr_section
{
	enum section_option option;
	char name[16];
	const struct attribute *array;
	uint8_t array_size;
};

#define MEMBER_SIZEOF(s, member)	(sizeof(((s *)0)->member))

#define ATTRIBUTE(s, rw, name, param) 	\
    {                                           \
        (uint8_t) offsetof(s, param),       	\
            (uint8_t)MEMBER_SIZEOF(s, param),   \
            (uint8_t)rw,   			\
            name                                \
    }

#define SECTION(options, name, array) \
    {                                 \
        options,                      \
            name,                     \
            array,                    \
            ARRAY_SIZE(array)         \
    }

static const struct attribute identification_attr[] ROM = {
	ATTRIBUTE(struct caniot_identification, READABLE, "nodeid", did),
	ATTRIBUTE(struct caniot_identification, READABLE, "version", version),
	ATTRIBUTE(struct caniot_identification, READABLE, "name", name),
};

static const struct attribute system_attr[] ROM = {
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "uptime", uptime),
	ATTRIBUTE(struct caniot_system, WRITABLE, "abstime", abstime),
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "calculated_abstime", calculated_abstime),
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "uptime_shift", uptime_shift),
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "last_telemetry", last_telemetry),
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "received.total", received.total),
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "received.read_attribute", received.read_attribute),
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "received.write_attribute", received.write_attribute),
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "received.command", received.command),
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "received.request_telemetry", received.request_telemetry),
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "received.processed", received.processed),
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "received.query_failed", received.query_failed),
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "sent.total", sent.total),
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "sent.telemetry", sent.telemetry),
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "sent.events", events.total),
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "last_query_error", last_query_error),
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "last_telemetry_error", last_telemetry_error),
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "last_event_error", last_event_error), 
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "battery", battery),
};

static const struct attribute config_attr[] ROM = {
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "telemetry.period", telemetry.period),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "telemetry.delay", telemetry.delay),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "telemetry.delay_min", telemetry.delay_min),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "telemetry.delay_max", telemetry.delay_max),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "telemetry.flags", flags),
};

static const struct attr_section attr_sections[] ROM = {
	SECTION(READONLY, "identification", identification_attr),
	SECTION(VOLATILE, "system", system_attr),
	SECTION(PERSISTENT, "configuration", config_attr),
};

static inline void arch_rom_cpy_byte(uint8_t *d, const uint8_t *p)
{
#ifdef __AVR__
	*d = pgm_read_byte(p);
#else
	*d = *p;
#endif
}

static inline void arch_rom_cpy_word(uint16_t *d, const uint16_t *p)
{
#ifdef __AVR__
	*d = pgm_read_word(p);
#else
	*d = *p;
#endif
}

static inline void arch_rom_cpy_dword(uint32_t *d, const uint32_t *p)
{
#ifdef __AVR__
	*d = pgm_read_dword(p);
#else
	*d = *p;
#endif
}

static inline void arch_rom_cpy_ptr(void **d, const void **p)
{
#ifdef __AVR__
	*d = pgm_read_ptr(p);
#else
	*d = *p;
#endif
}

static inline void arch_rom_cpy_mem(void *d, const void *p, uint16_t size)
{
#ifdef __AVR__
	memcpy_P(d, p, size);
#else
	memcpy(d, p, size);
#endif
}

static inline void arch_rom_cpy(void *d, const void *p, uint16_t size)
{
	switch (size) {
	case 1u:
		return arch_rom_cpy_byte(d, p);
	case 2u:
		return arch_rom_cpy_word(d, p);
	case 4u:
		return arch_rom_cpy_dword(d, p);
	default:
		return arch_rom_cpy_mem(d, p, size);
	}
}

static inline uint8_t attr_get_section_size(const struct attr_section *section)
{
	uint8_t size;
	arch_rom_cpy_byte(&size, &section->array_size);
	return size;
}

static inline const struct attribute *attr_get_section_array(
	const struct attr_section *section)
{
	const struct attribute *array;
	arch_rom_cpy_ptr((void **)&array, (const void **)&section->array);
	return array;
}

static inline enum section_option attr_get_section_option(const struct attr_section *section)
{
	uint8_t option;
	arch_rom_cpy_byte(&option, (const uint8_t *)&section->option);
	return option;
}

static inline uint8_t attr_get_size(const struct attribute *attr)
{
	uint8_t size;
	arch_rom_cpy_byte(&size, &attr->size);
	return size;
}

static inline uint8_t attr_get_offset(const struct attribute *attr)
{
	uint8_t offset;
	arch_rom_cpy_byte(&offset, &attr->offset);
	return offset;
}

static inline enum attr_option attr_get_option(const struct attribute *attr)
{
	uint8_t option;
	arch_rom_cpy_byte(&option, &attr->option);
	return option;
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
		return -CANIOT_EKEYSECTION; /* TODO return accurate error CANIOT_EKEYSECTION */
	}

	const struct attribute *attr = attr_get(key, section);
	if (!attr) {
		return -CANIOT_EKEYATTR; /* TODO return accurate error CANIOT_EKEYATTR */
	}

	uint8_t attr_size = attr_get_size(attr);
	if (ATTR_KEY_OFFSET(key) >= attr_size) {
		return -CANIOT_EKEYPART; /* TODO return accurate error CANIOT_EKEYPART */
	}

	ref->section = ATTR_KEY_SECTION(key);
	ref->read_size = attr_size > 4u ? 4u : attr_size;
	ref->offset = ATTR_KEY_OFFSET(key) + attr_get_offset(attr);
	ref->option = attr_get_option(attr);
	ref->section_option = attr_get_section_option(section);

	/* adjust attribute options in function of uppermost section options */
	attr_option_adjust(&ref->option, ref->section_option);

	return 0;
}

static void read_identificate_attr(struct caniot_device *dev,
				   const struct attr_ref *ref,
				   struct caniot_attribute *attr)
{
	arch_rom_cpy((void *)dev->identification + ref->offset,
		     &attr->val, ref->read_size);
}

static void read_rom_identification(struct caniot_identification *d,
				    const struct caniot_identification *p)
{
	arch_rom_cpy_mem(d, p, sizeof(struct caniot_identification));
}

void caniot_print_device_identification(const struct caniot_device *dev)
{
	struct caniot_identification id;

	read_rom_identification(&id, dev->identification);

	printf(F("name    = %s\ncls/dev = %d/%d\nversion = %hhx\n\n"),
	       id.name, id.did.cls, id.did.sid, id.version);
}

/*
static inline uint16_t get_identification_version(struct caniot_device *dev)
{
	uint16_t version;
	arch_rom_cpy_word(&dev->identification->version, &version);
	return version;
}
*/

static inline void read_identification_nodeid(struct caniot_device *dev,
					      union deviceid *did)
{
	arch_rom_cpy_byte(&did->val, (const uint8_t *)&dev->identification->did);
}

union deviceid caniot_device_get_id(struct caniot_device *dev)
{
	union deviceid did;
	read_identification_nodeid(dev, &did);
	return did;
}

static int prepare_config_read(struct caniot_device *dev)
{
	/* local configuration in RAM should be updated */
	if (dev->api->config.on_read != NULL) {
		return dev->api->config.on_read(dev, dev->config);
	}

	return 0;
}

static int read_config_attr(struct caniot_device *dev,
			    const struct attr_ref *ref,
			    struct caniot_attribute *attr)
{
	/* local configuration in RAM should be updated */
	int ret = prepare_config_read(dev);
	if (ret != 0) {
		return ret;
	}

	memcpy(&attr->val, (void *)&dev->config + ref->offset, ref->read_size);

	return 0;
}


static int attribute_read(struct caniot_device *dev,
			  const struct attr_ref *ref,
			  struct caniot_attribute *attr)
{
	int ret = 0;

	switch (ref->section) {
	case section_identification:
	{
		read_identificate_attr(dev, ref, attr);
		break;
	}

	case section_system:
	{
		memcpy(&attr->val, (void *)&dev->system + ref->offset,
		       ref->read_size);
		break;
	}

	case section_config:
	{
		ret = read_config_attr(dev, ref, attr);
		break;
	}

	default:
		ret = -EINVAL;
	}

	return ret;
}


// static bool is_error_response(struct caniot_frame *frame) {
// 	return frame->id.query == response && frame->id.type == command;
// }

static void prepare_response(struct caniot_device *dev,
			     struct caniot_frame *resp,
			     uint8_t type)
{
	union deviceid did;

	caniot_clear_frame(resp);

	/* read device class and id from ROM */
	read_identification_nodeid(dev, &did);

	/* id */
	resp->id.type = type;
	resp->id.query = response;

	resp->id.cls = did.cls;
	resp->id.sid = did.sid;
	resp->id.endpoint = 0;
}

static void prepare_error(struct caniot_device *dev,
			  struct caniot_frame *resp,
			  int error)
{
	prepare_response(dev, resp, command);

	resp->err = (int32_t)error;
	resp->len = 4;
}

static int handle_read_attribute(struct caniot_device *dev,
				 struct caniot_frame *resp,
				 uint16_t key)
{
	int ret;
	struct attr_ref ref;

	CANIOT_DBG(F("Executing read attribute key = 0x%x\n"), key);

	ret = attr_resolve(key, &ref);
	if (ret != 0 && ret != -EINVAL) {
		goto exit;
	}

	prepare_response(dev, resp, read_attribute);

	if (ret == -EINVAL) { /* if custom attribute */
		if (dev->api->custom_attr.read != NULL) {
			ret = dev->api->custom_attr.read(dev, key, &resp->attr.val);
		} else {
			ret = -CANIOT_ENOTSUP; /* not supported attribute */
		}
	} else {
		/* if standart attribute */
		ret = attribute_read(dev, &ref, &resp->attr);
	}

	/* finalize response */
	if (ret == 0) {
		resp->len = 6u;
		resp->attr.key = key;
	}

exit:
	return ret;
}

static int handle_write_attribute(struct caniot_device *dev,
				  struct caniot_frame *resp,
				  uint16_t key)
{
	CANIOT_DBG(F("Executing write attribute key = 0x%x\n"), key);
	return -1;
}

static int command_resp(struct caniot_device *dev,
			struct caniot_frame *req,
			uint8_t ep)
{
	if (req->id.endpoint == endpoint_broadcast) {
		return -CANIOT_ECMDEP;
	}

	if (dev->api->command_handler == NULL) {
		return -CANIOT_EHANDLERC;
	}

	CANIOT_DBG(F("Executing command handler (0x%x) for endpoint %d\n"),
		   dev->api->command_handler, ep);

	return dev->api->command_handler(dev, ep, req->buf, req->len);
}

static int telemetry_resp(struct caniot_device *dev,
			  struct caniot_frame *resp,
			  uint8_t ep)
{
	int ret;

	dev->flags.request_telemetry = 0;

	if (dev->api->telemetry == NULL) {
		return -CANIOT_EHANDLERT;
	}

	prepare_response(dev, resp, telemetry);

	CANIOT_DBG(F("Executing telemetry handler (0x%x) for endpoint %d\n"),
		   dev->api->telemetry, ep);

	/* buffer */
	ret = dev->api->telemetry(dev, ep, resp->buf, &resp->len);
	if (ret == 0) {
		resp->id.endpoint = ep;
	}

	return ret;
}

int caniot_device_handle_rx_frame(struct caniot_device *dev,
				  struct caniot_frame *req,
				  struct caniot_frame *resp)
{
	int ret;

	/* no response in this case */
	if (req->id.query != query) {
		ret = -EINVAL;
		goto exit;
	}

	switch (req->id.type) {
	case command:
	{
		ret = command_resp(dev, resp, req->id.endpoint);
		if (ret != 0) {
			goto exit;
		}
	}
	case telemetry:
	{
		ret = telemetry_resp(dev, resp, req->id.endpoint);
		break;
	}

	case write_attribute:
	{
		ret = handle_write_attribute(dev, resp, req->attr.key);
		if (ret != 0) {
			goto exit;
		}
	}
	case read_attribute:
		ret = handle_read_attribute(dev, resp, req->attr.key);
		break;
	}

exit:
	if (ret != 0) {
		prepare_error(dev, resp, ret);
	}
	return ret;
}

bool caniot_device_is_target(union deviceid did,
			     struct caniot_frame *frame)
{
	return (frame->id.query == query) && (frame->id.cls == did.cls) &&
		(frame->id.sid == did.sid || frame->id.sid == CANIOT_CLASS_BROADCAST);
}

int caniot_device_verify(struct caniot_device *dev)
{
	return -CANIOT_ENIMPL;
}

/*___________________________________________________________________________*/

#if CANIOT_DRIVERS_API
uint32_t caniot_device_telemetry_remaining(struct caniot_device *dev)
{
	if (prepare_config_read(dev) == 0) {
		uint32_t now;
		dev->driv->get_time(&now, NULL);
		
		uint32_t diff = now - dev->system.last_telemetry;
		if (dev->config->telemetry.period <= diff) {
			return 0;
		} else {
			return (dev->config->telemetry.period - diff) * 1000;
		}
	}

	/* default 1 second */
	return 1000u;
}

static uint32_t get_response_delay(struct caniot_device *dev,
				   struct caniot_frame *frame)
{
	uint32_t delay_ms = 0;

	if (is_telemetry_response(frame)) {
		int ret = prepare_config_read(dev);
		if (ret == 0) {
			if (dev->config->flags.telemetry_delay_rdm == 1u) {
				dev->driv->entropy((uint8_t *)&delay_ms, sizeof(delay_ms));

				const uint32_t amplitude = dev->config->telemetry.delay_max
					- dev->config->telemetry.delay_min;

				delay_ms = dev->config->telemetry.delay_min + (delay_ms % amplitude);
			} else {
				delay_ms = dev->config->telemetry.delay;
			}
		} else {
			delay_ms = CANIOT_TELEMETRY_DELAY_DEFAULT;
		}
	}

	return delay_ms;
}

static inline bool telemetry_requested(struct caniot_device *dev)
{
	return dev->flags.request_telemetry == 1u;
}

int caniot_device_process(struct caniot_device *dev)
{
	int ret;
	struct caniot_frame req, resp;

	/* get current time */
	uint32_t now;
	dev->driv->get_time(&now, NULL);

	/* check if we need to send telemetry */
	prepare_config_read(dev);
	if (now - dev->system.last_telemetry >= dev->config->telemetry.period) {
		dev->flags.request_telemetry = 1;

		CANIOT_DBG(F("Requesting telemetry\n"));
	}

	/* received any incoming frame */
	caniot_clear_frame(&req);
	ret = dev->driv->recv(&req);

	/* if we received a frame */
	if (ret == 0) {
		/* handle received frame */
		ret = caniot_device_handle_rx_frame(dev, &req, &resp);
	/* if we didn't received a frame and telemetry is requested */
	} else if (telemetry_requested(dev)) {
		/* prepare telemetry response */
		ret = telemetry_resp(dev, &resp, dev->config->flags.telemetry_endpoint);
	} else {
		/* next call would block */
		return -CANIOT_EAGAIN;
	}

	if (ret != 0) {
		prepare_config_read(dev);

		/* we "error frame" are not enabled */
		if (dev->config->flags.error_response == 0u) {
			goto exit;
		}
	}

	/* send response or error frame if configured */
	ret = dev->driv->send(&resp, get_response_delay(dev, &resp));
	if (ret == 0 && is_telemetry_response(&resp)) {
		dev->system.last_telemetry = now;
	}

exit:
	return ret;
}

#endif /* CANIOT_DRIVERS_API */