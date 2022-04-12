#include <caniot/device.h>
#include <caniot/archutils.h>

#include <errno.h>
#include <string.h>

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
	DISABLED = 0,
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
	uint8_t size;
};

struct attribute
{
	uint8_t offset;
	uint8_t size;
	uint8_t option;
	// char name[32];
};

struct attr_section
{
	enum section_option option;
	// char name[16];
	const struct attribute *array;
	uint8_t array_size;
};

#define MEMBER_SIZEOF(s, member)	(sizeof(((s *)0)->member))

#define ATTRIBUTE(s, rw, name, param) 	\
    {                                           \
        (uint8_t) offsetof(s, param),       	\
            (uint8_t)MEMBER_SIZEOF(s, param),   \
            (uint8_t)rw,   			\
    }

#define SECTION(options, name, array) \
    {                                 \
        options,                      \
            array,                    \
            ARRAY_SIZE(array)         \
    }

static const struct attribute identification_attr[] ROM = {
	ATTRIBUTE(struct caniot_identification, READABLE, "nodeid", did),
	ATTRIBUTE(struct caniot_identification, READABLE, "version", version),
	ATTRIBUTE(struct caniot_identification, READABLE, "name", name),
	ATTRIBUTE(struct caniot_identification, READABLE, "magic_number", magic_number),
};

static const struct attribute system_attr[] ROM = {
	ATTRIBUTE(struct caniot_system, READABLE, "uptime_synced", uptime_synced),
	ATTRIBUTE(struct caniot_system, READABLE | WRITABLE, "time", time),
	ATTRIBUTE(struct caniot_system, READABLE, "uptime", uptime),
	ATTRIBUTE(struct caniot_system, READABLE, "start_time", start_time),
	ATTRIBUTE(struct caniot_system, READABLE, "last_telemetry", last_telemetry),
	ATTRIBUTE(struct caniot_system, READABLE, "received.total", received.total),
	ATTRIBUTE(struct caniot_system, READABLE, "received.read_attribute", received.read_attribute),
	ATTRIBUTE(struct caniot_system, READABLE, "received.write_attribute", received.write_attribute),
	ATTRIBUTE(struct caniot_system, READABLE, "received.command", received.command),
	ATTRIBUTE(struct caniot_system, READABLE, "received.request_telemetry", received.request_telemetry),
	ATTRIBUTE(struct caniot_system, DISABLED, "", received._unused2),
	ATTRIBUTE(struct caniot_system, DISABLED, "", received._unused3),
	ATTRIBUTE(struct caniot_system, READABLE, "sent.total", sent.total),
	ATTRIBUTE(struct caniot_system, READABLE, "sent.telemetry", sent.telemetry),
	ATTRIBUTE(struct caniot_system, DISABLED, "", _unused4),
	ATTRIBUTE(struct caniot_system, READABLE, "last_command_error", last_command_error),
	ATTRIBUTE(struct caniot_system, READABLE, "last_telemetry_error", last_telemetry_error),
	ATTRIBUTE(struct caniot_system, DISABLED, "", _unused5), 
	ATTRIBUTE(struct caniot_system, READABLE, "battery", battery),
};

static const struct attribute config_attr[] ROM = {
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "telemetry.period", telemetry.period),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "telemetry.delay", telemetry.delay),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "telemetry.delay_min", telemetry.delay_min),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "telemetry.delay_max", telemetry.delay_max),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "flags", flags),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "timezone", timezone),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "location", location),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "custompcb.gpio.pulse_duration.oc1", 
		custompcb.gpio.pulse_duration.oc1),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "custompcb.gpio.pulse_duration.oc2", 
		custompcb.gpio.pulse_duration.oc2),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "custompcb.gpio.pulse_duration.rl1", 
		custompcb.gpio.pulse_duration.rl1),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "custompcb.gpio.pulse_duration.rl2", 
		custompcb.gpio.pulse_duration.rl2),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "custompcb.gpio.mask.outputs_default", 
		custompcb.gpio.mask.outputs_default.mask),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "custompcb.gpio.mask.telemetry_on_change",
		custompcb.gpio.mask.telemetry_on_change.mask),
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
	*d = (void*) *p;
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
		return -CANIOT_EKEYSECTION;
	}

	const struct attribute *attr = attr_get(key, section);
	if (!attr) {
		return -CANIOT_EKEYATTR;
	}

	uint8_t attr_size = attr_get_size(attr);
	if (ATTR_KEY_OFFSET(key) >= attr_size) {
		return -CANIOT_EKEYPART;
	}

	ref->section = ATTR_KEY_SECTION(key);
	ref->size = attr_size > 4u ? 4u : attr_size;
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
	arch_rom_cpy(&attr->val, (uint8_t *)dev->identification + ref->offset,
		     ref->size);
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

static int config_written(struct caniot_device *dev)
{
	int ret = 0;

	// useless 

	/* local configuration in RAM should be updated */
	if (dev->api->config.on_write != NULL) {
#if CANIOT_DRIVERS_API
		/* we update the last telemetry time which could
		 * have changed if the timezone changed
		 */
		uint32_t prev, new;
		dev->driv->get_time(&prev, NULL);
#endif /* CANIOT_DRIVERS_API */

		/* call application callback to apply the new configuration */
		ret = dev->api->config.on_write(dev, dev->config);
		
#if CANIOT_DRIVERS_API
		/* do adjustement if needed */	
		dev->driv->get_time(&new, NULL);
		if (new != prev) {
			const int32_t diff = new - prev;

			/* adjust last_telemetry time,
			 * in order to not trigger it on time update
			 */
			dev->system.last_telemetry += diff;
			dev->system.start_time += diff;
		}
#endif /* CANIOT_DRIVERS_API */
	}

	return ret;
}

static int read_config_attr(struct caniot_device *dev,
			    const struct attr_ref *ref,
			    struct caniot_attribute *attr)
{
	/* local configuration in RAM should be updated */
	int ret = prepare_config_read(dev);

	if (ret == 0) {
		memcpy(&attr->val, (uint8_t *)dev->config + ref->offset, ref->size);
	}

	return ret;
}

static int write_config_attr(struct caniot_device *dev,
			     const struct attr_ref *ref,
			     const struct caniot_attribute *attr)
{
	memcpy((uint8_t *)dev->config + ref->offset, &attr->val, ref->size);

	return config_written(dev);
}

static int attribute_read(struct caniot_device *dev,
			  const struct attr_ref *ref,
			  struct caniot_attribute *attr)
{
	int ret = 0;

	/* print debug attr_ref */
	CANIOT_DBG(F("attr_ref: section = %hhu, offset = %hhu, option = %hhu\n"),
	       ref->section, ref->offset, ref->option);

	switch (ref->section) {
	case section_identification:
	{
		read_identificate_attr(dev, ref, attr);
		break;
	}

	case section_system:
	{
		memcpy(&attr->val, (uint8_t *)&dev->system + ref->offset,
		       ref->size);
		break;
	}

	case section_config:
	{
		ret = read_config_attr(dev, ref, attr);
		break;
	}

	default:
		ret = -CANIOT_EREADATTR;
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
	resp->len = 4U;
}

static int handle_read_attribute(struct caniot_device *dev,
				 struct caniot_frame *resp,
				 struct caniot_attribute *attr)
{
	int ret;
	struct attr_ref ref;

	CANIOT_DBG(F("Executing read attribute key = 0x%x\n"), attr->key);

	ret = attr_resolve(attr->key, &ref);
	if (ret != 0 && ret != -EINVAL) {
		goto exit;
	}

	prepare_response(dev, resp, read_attribute);

	if (ret == 0) {
		/* if standart attribute */
		ret = attribute_read(dev, &ref, &resp->attr);
	} else if (ret == -EINVAL) { /* if custom attribute */
		if (dev->api->custom_attr.read != NULL) {
			ret = dev->api->custom_attr.read(dev, attr->key, &resp->attr.val);
		} else {
			ret = -CANIOT_ENOTSUP; /* not supported attribute */
		}
	}

	/* finalize response */
	if (ret == 0) {
		resp->len = 6u;
		resp->attr.key = attr->key;
	}

exit:
	return ret;
}

static int write_system_attr(struct caniot_device *dev,
			     const struct attr_ref *ref,
			     const struct caniot_attribute *attr)
{
#if CANIOT_DRIVERS_API
	if (attr->key == 0x1010U) {
		uint32_t prev;
		dev->driv->get_time(&prev, NULL);
		dev->driv->set_time(attr->u32);

		/* adjust last_telemetry time,
		 * in order to not trigger it on time update
		 */
		dev->system.last_telemetry += attr->u32 - prev;
		dev->system.start_time += attr->u32 - prev;

		/* sets the system time for the current loop,
		 * the response to reading an attribute will
		 * send the value acknowledgement.
		 */
		dev->system.time = attr->u32;
	
		/* last uptime when the UNIX time was set */
		dev->system.uptime_synced = attr->u32 - dev->system.start_time;

		return 0;
	}
#endif 

	memcpy((uint8_t *)&dev->system + ref->offset, &attr->val,
	       ref->size);

	return 0;
}

static int attribute_write(struct caniot_device *dev,
		       const struct attr_ref *ref,
		       const struct caniot_attribute *attr)
{
	if ((ref->option & WRITABLE) == 0U) {
		return -CANIOT_EROATTR;
	}

	int ret;

	/* print debug attr_ref */
	CANIOT_DBG(F("attr_ref: section = %hhu, offset = %hhu, option = %hhu\n"),
	       ref->section, ref->offset, ref->option);

	switch (ref->section) {
	case section_system:
	{
		ret = write_system_attr(dev, ref, attr);
		break;
	}
	case section_config:
	{
		ret = write_config_attr(dev, ref, attr);
		break;
	}

	default:
		ret = -CANIOT_EWRITEATTR;
	}

	return ret;
}

static int handle_write_attribute(struct caniot_device *dev,
				  struct caniot_frame *req,
				  struct caniot_attribute *attr)
{
	int ret;
	struct attr_ref ref;

	ret = attr_resolve(attr->key, &ref);
	if (ret != 0 && ret != -EINVAL) {
		goto exit;
	}

	if (ret == 0) {
		/* if standart attribute */
		ret = attribute_write(dev, &ref, &req->attr);
	} else if (ret == -EINVAL) { /* if custom attribute */
		if (dev->api->custom_attr.read != NULL) {
			ret = dev->api->custom_attr.write(dev, attr->key, req->attr.val);
		} else {
			ret = -CANIOT_ENOTSUP; /* not supported attribute */
		}
	}

exit:
	return ret;
}

static int handle_command_req(struct caniot_device *dev,
			      struct caniot_frame *req)
{
	int ret;
	const uint8_t ep = req->id.endpoint;

	CANIOT_DBG(F("Executing command handler (0x%x) for endpoint %d\n"),
		   dev->api->command_handler, ep);

	if (dev->api->command_handler != NULL) {
		ret = dev->api->command_handler(dev, ep, req->buf, req->len);
	} else {
		return -CANIOT_EHANDLERC;
	}

	dev->system.last_command_error = ret;

	return ret;
}

static int build_telemetry_resp(struct caniot_device *dev,
				struct caniot_frame *resp,
				uint8_t ep)
{
	int ret;

	/* TODO check endpoint relative to class*/

	if (dev->api->telemetry_handler == NULL) {
		return -CANIOT_EHANDLERT;
	}

	prepare_response(dev, resp, telemetry);

	CANIOT_DBG(F("Executing telemetry handler (0x%x) for endpoint %d\n"),
		   dev->api->telemetry_handler, ep);

	/* buffer */
	ret = dev->api->telemetry_handler(dev, ep, resp->buf, &resp->len);
	if (ret == 0) {
		/* set endpoint */
		resp->id.endpoint = ep;

		/* increment counter */
		dev->system.sent.telemetry++;
	}

	
	dev->system.last_telemetry_error = ret;

	/* TODO check and force response length */

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

	dev->system.received.total++;

	switch (req->id.type) {
	case command:
	{
		dev->system.received.command++;
		ret = handle_command_req(dev, req);
		if (ret == 0) {
			ret = build_telemetry_resp(dev, resp, req->id.endpoint);
		}
		break;
	}
	case telemetry:
	{
		dev->system.received.request_telemetry++;
		ret = build_telemetry_resp(dev, resp, req->id.endpoint);
		break;
	}

	case write_attribute:
	{
		dev->system.received.write_attribute++;
		ret = handle_write_attribute(dev, req, &req->attr);
		if (ret == 0) {
			ret = handle_read_attribute(dev, resp, &req->attr);
		}
		break;
	}
	case read_attribute:
		dev->system.received.read_attribute++;
		ret = handle_read_attribute(dev, resp, &req->attr);
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

bool caniot_device_time_synced(struct caniot_device *dev)
{
	return dev->system.uptime_synced != 0;
}

/*___________________________________________________________________________*/

#if CANIOT_DRIVERS_API
uint32_t caniot_device_telemetry_remaining(struct caniot_device *dev)
{
	if (prepare_config_read(dev) == 0) {
		uint32_t now;
		dev->driv->get_time(&now, NULL);

		CANIOT_DBG(F("now = %lu last_telemetry = %lu\n"), now, dev->system.last_telemetry);
		
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
				
				uint32_t amplitude = 100U;
				if (dev->config->telemetry.delay_max > dev->config->telemetry.delay_min) {
					amplitude = dev->config->telemetry.delay_max
						- dev->config->telemetry.delay_min;
				}

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
	dev->driv->get_time(&dev->system.time, NULL);
	dev->system.uptime = dev->system.time - dev->system.start_time;

	/* check if we need to send telemetry (calculated in seconds) */
	prepare_config_read(dev);
	if (dev->system.time - dev->system.last_telemetry >=
	    dev->config->telemetry.period) {
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
		ret = build_telemetry_resp(dev, &resp, dev->config->flags.telemetry_endpoint);
	} else {
		/* next call would block */
		return -CANIOT_EAGAIN;
	}

	if (ret != 0) {
		prepare_config_read(dev);

		/* if "error frame" are not enabled */
		if (dev->config->flags.error_response == 0u) {
			goto exit;
		}
	}

	/* send response or error frame if configured */
	ret = dev->driv->send(&resp, get_response_delay(dev, &resp));
	if (ret == 0) {
		dev->system.sent.total++;

		if (is_telemetry_response(&resp) == true) {
			dev->system.last_telemetry = dev->system.time;
			dev->flags.request_telemetry = 0;
		}
	}

exit:
	return ret;
}

void caniot_app_init(struct caniot_device *dev)
{
	memset(&dev->system, 0x00U, sizeof(dev->system));

	dev->driv->get_time(&dev->system.start_time, NULL);
}

#endif /* CANIOT_DRIVERS_API */