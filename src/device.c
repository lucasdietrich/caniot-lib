/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <caniot/caniot_private.h>
#include <caniot/device.h>

#define ATTR_IDENTIFICATION 0
#define ATTR_SYSTEM	    1
#define ATTR_CONFIG	    2

#define ATTR_KEY_SECTION(key) ((uint8_t)(((uint16_t)key) >> 12))
#define ATTR_KEY_ATTR(key)    ((uint8_t)((((uint16_t)key) >> 4) & 0xFF))
#define ATTR_KEY_PART(key)    ((uint8_t)(((uint16_t)key) & 0xF))

/* A part is is 4B */
#define ATTR_KEY_OFFSET(key) (ATTR_KEY_PART(key) << 2)

#define ATTR_KEY(section, attr, part)                                                    \
	((section & 0xF) << 12 | (attr & 0xFF) << 4 | (part & 0xF))

typedef uint16_t attr_key_t;

enum section {
	section_identification = 0,
	section_system	       = 1,
	section_config	       = 2
};

enum section_option {
	READONLY   = 1 << 0,
	VOLATILE   = 1 << 1,
	PERSISTENT = 1 << 2
};

enum attr_option {
	DISABLED = 0,
	READABLE = 1 << 0,
	WRITABLE = 1 << 1,
};

static void attr_option_adjust(enum attr_option *attr_opt, enum section_option sec_opt)
{
	if (sec_opt & READONLY) {
		*attr_opt &= ~WRITABLE;
	}
}

struct attr_ref {
	enum attr_option option;
	enum section_option section_option;
	uint8_t section;
	uint8_t offset;
	uint8_t size;
};

struct attribute {
	uint8_t offset;
	uint8_t size; /* TODO merge fields size and option together */
	uint8_t option;
	// char name[32];
};

struct attr_section {
	enum section_option option;
	// char name[16];
	const struct attribute *array;
	uint8_t array_size;
};

#define MEMBER_SIZEOF(s, member) (sizeof(((s *)0)->member))

#define ATTRIBUTE(s, rw, name, param)                                                    \
	{                                                                                \
		(uint8_t) offsetof(s, param), (uint8_t)MEMBER_SIZEOF(s, param),          \
			(uint8_t)rw,                                                     \
	}

#define SECTION(options, name, array)                                                    \
	{                                                                                \
		options, array, ARRAY_SIZE(array)                                        \
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
	ATTRIBUTE(struct caniot_system,
		  READABLE,
		  "received.read_attribute",
		  received.read_attribute),
	ATTRIBUTE(struct caniot_system,
		  READABLE,
		  "received.write_attribute",
		  received.write_attribute),
	ATTRIBUTE(struct caniot_system, READABLE, "received.command", received.command),
	ATTRIBUTE(struct caniot_system,
		  READABLE,
		  "received.request_telemetry",
		  received.request_telemetry),
	ATTRIBUTE(struct caniot_system, DISABLED, "received.ignored", received.ignored),
	ATTRIBUTE(struct caniot_system, DISABLED, "", received._unused3),
	ATTRIBUTE(struct caniot_system, READABLE, "sent.total", sent.total),
	ATTRIBUTE(struct caniot_system, READABLE, "sent.telemetry", sent.telemetry),
	ATTRIBUTE(struct caniot_system, DISABLED, "", _unused4),
	ATTRIBUTE(
		struct caniot_system, READABLE, "last_command_error", last_command_error),
	ATTRIBUTE(struct caniot_system,
		  READABLE,
		  "last_telemetry_error",
		  last_telemetry_error),
	ATTRIBUTE(struct caniot_system, DISABLED, "", _unused5),
	ATTRIBUTE(struct caniot_system, READABLE, "battery", battery),
};

static const struct attribute config_attr[] ROM = {
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "telemetry.period",
		  telemetry.period), /* ms */
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "telemetry.delay",
		  telemetry.delay), /* ms */
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "telemetry.delay_min",
		  telemetry.delay_min), /* ms */
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "telemetry.delay_max",
		  telemetry.delay_max), /* ms */
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "flags", flags),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "timezone", timezone),
	ATTRIBUTE(struct caniot_config, READABLE | WRITABLE, "location", location),

	/* Class 0 */
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls0_gpio.pulse_duration.oc1",
		  cls0_gpio.pulse_durations[0u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls0_gpio.pulse_duration.oc2",
		  cls0_gpio.pulse_durations[1u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls0_gpio.pulse_duration.rl1",
		  cls0_gpio.pulse_durations[2u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls0_gpio.pulse_duration.rl2",
		  cls0_gpio.pulse_durations[3u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls0_gpio.outputs_default",
		  cls0_gpio.outputs_default),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls0_gpio.mask.telemetry_on_change",
		  cls0_gpio.telemetry_on_change),

	/* Class 1 */
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pc0",
		  cls1_gpio.pulse_durations[0u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pc1",
		  cls1_gpio.pulse_durations[1u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pc2",
		  cls1_gpio.pulse_durations[2u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pc3",
		  cls1_gpio.pulse_durations[3u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pd0",
		  cls1_gpio.pulse_durations[4u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pd1",
		  cls1_gpio.pulse_durations[5u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pd2",
		  cls1_gpio.pulse_durations[6u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pd3",
		  cls1_gpio.pulse_durations[7u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pei0",
		  cls1_gpio.pulse_durations[8u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pei1",
		  cls1_gpio.pulse_durations[9u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pei2",
		  cls1_gpio.pulse_durations[10u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pei3",
		  cls1_gpio.pulse_durations[11u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pei4",
		  cls1_gpio.pulse_durations[12u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pei5",
		  cls1_gpio.pulse_durations[13u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pei6",
		  cls1_gpio.pulse_durations[14u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pei7",
		  cls1_gpio.pulse_durations[15u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pb0",
		  cls1_gpio.pulse_durations[16u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pe0",
		  cls1_gpio.pulse_durations[17u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration.pe1",
		  cls1_gpio.pulse_durations[18u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.pulse_duration._reserved",
		  cls1_gpio.pulse_durations[19u]),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.directions",
		  cls1_gpio.directions),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.outputs_default",
		  cls1_gpio.outputs_default),
	ATTRIBUTE(struct caniot_config,
		  READABLE | WRITABLE,
		  "cls1_gpio.mask.telemetry_on_change",
		  cls1_gpio.telemetry_on_change),
};

static const struct attr_section attr_sections[] ROM = {
	SECTION(READONLY, "identification", identification_attr),
	SECTION(VOLATILE, "system", system_attr),
	SECTION(PERSISTENT, "configuration", config_attr),
	/* TODO create more sections for class0, class1, etc ... */
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
	*d = (void *)*p;
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
		arch_rom_cpy_byte(d, p);
		break;
	case 2u:
		arch_rom_cpy_word(d, p);
		break;
	case 4u:
		arch_rom_cpy_dword(d, p);
		break;
	default:
		arch_rom_cpy_mem(d, p, size);
		break;
	}
}

static inline uint8_t attr_get_section_size(const struct attr_section *section)
{
	uint8_t size;
	arch_rom_cpy_byte(&size, &section->array_size);
	return size;
}

static inline const struct attribute *
attr_get_section_array(const struct attr_section *section)
{
	const struct attribute *array;
	arch_rom_cpy_ptr((void **)&array, (const void **)&section->array);
	return array;
}

static inline enum section_option
attr_get_section_option(const struct attr_section *section)
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

static const struct attr_section *attr_get_section(attr_key_t key)
{
	uint8_t index = ATTR_KEY_SECTION(key);
	if (index < ARRAY_SIZE(attr_sections)) {
		return &attr_sections[index];
	}
	return NULL;
}

static const struct attribute *attr_get(attr_key_t key,
					const struct attr_section *section)
{
	uint8_t index = ATTR_KEY_ATTR(key);
	if (index < attr_get_section_size(section)) {
		return &attr_get_section_array(section)[index];
	}
	return NULL;
}

static int attr_resolve(attr_key_t key, struct attr_ref *ref)
{
	const struct attr_section *section;
	const struct attribute *attr;
	uint8_t attr_size;

	section = attr_get_section(key);
	if (!section) {
		return -CANIOT_EKEYSECTION;
	}

	attr = attr_get(key, section);
	if (!attr) {
		return -CANIOT_EKEYATTR;
	}

	attr_size = attr_get_size(attr);
	if (ATTR_KEY_OFFSET(key) >= attr_size) {
		return -CANIOT_EKEYPART;
	}

	ref->section	    = ATTR_KEY_SECTION(key);
	ref->size	    = MIN(attr_size, 4u);
	ref->offset	    = ATTR_KEY_OFFSET(key) + attr_get_offset(attr);
	ref->option	    = attr_get_option(attr);
	ref->section_option = attr_get_section_option(section);

	/* adjust attribute options in function of uppermost section options */
	attr_option_adjust(&ref->option, ref->section_option);

	return 0;
}

static void read_identificate_attr(struct caniot_device *dev,
				   const struct attr_ref *ref,
				   struct caniot_attribute *attr)
{
	arch_rom_cpy(&attr->val, (uint8_t *)dev->identification + ref->offset, ref->size);
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

	CANIOT_INF(F("name    = %s\ncls/dev = %d/%d\nversion = %hhx\n\n"),
		   id.name,
		   CANIOT_DID_CLS(id.did),
		   CANIOT_DID_SID(id.did),
		   id.version);
}

int caniot_device_system_reset(struct caniot_device *dev)
{
	if (!dev) return -CANIOT_EINVAL;

	memset(&dev->system, 0, sizeof(struct caniot_system));

	return 0;
}

static inline void read_identification_nodeid(struct caniot_device *dev,
					      caniot_did_t *did)
{
	arch_rom_cpy_byte(did, (const uint8_t *)&dev->identification->did);
}

caniot_did_t caniot_device_get_id(struct caniot_device *dev)
{
	caniot_did_t did;
	read_identification_nodeid(dev, &did);
	return did;
}

uint16_t caniot_device_get_filter(caniot_did_t did)
{
	const caniot_id_t filter = {
		.query = CANIOT_QUERY,
		.sid   = CANIOT_DID_SID(did),
		.cls   = CANIOT_DID_CLS(did),
	};

	return caniot_id_to_canid(filter);
}

uint16_t caniot_device_get_filter_broadcast(caniot_did_t did)
{
	(void)did;

	const caniot_id_t filter = {
		.query = CANIOT_QUERY,
		.sid   = CANIOT_DID_SID(CANIOT_DID_BROADCAST),
		.cls   = CANIOT_DID_CLS(CANIOT_DID_BROADCAST),
	};

	return caniot_id_to_canid(filter);
}

static int prepare_config_read(struct caniot_device *dev)
{
	ASSERT(dev != NULL);

	/* local configuration in RAM should be updated */
	if (dev->api->config.on_read != NULL) {
		return dev->api->config.on_read(dev, dev->config);
	}

	return 0;
}

static int config_written(struct caniot_device *dev)
{
	ASSERT(dev != NULL);

	int ret = 0;

	/* local configuration in RAM should be updated */
	if (dev->api->config.on_write != NULL) {
#if CONFIG_CANIOT_DRIVERS_API
		/* we update the last telemetry time which could
		 * have changed if the timezone changed
		 */
		uint32_t prev_sec, new_sec;
		uint16_t prev_msec, new_msec;
		dev->driv->get_time(&prev_sec, &prev_msec);
#endif /* CONFIG_CANIOT_DRIVERS_API */

		/* call application callback to apply the new configuration */
		ret = dev->api->config.on_write(dev, dev->config);

#if CONFIG_CANIOT_DRIVERS_API
		/* do adjustement if needed */
		dev->driv->get_time(&new_sec, &new_msec);

		const int32_t diff_sec	= new_sec - prev_sec;
		const int32_t diff_msec = diff_sec * 1000u + new_msec - prev_msec;

		/* adjust last_telemetry time,
		 * in order to not trigger it on time update
		 */
		dev->system.last_telemetry += diff_msec;
		dev->system.start_time += diff_sec;
#endif /* CONFIG_CANIOT_DRIVERS_API */
	}

	return ret;
}

static int read_config_attr(struct caniot_device *dev,
			    const struct attr_ref *ref,
			    struct caniot_attribute *attr)
{
	ASSERT(dev != NULL);
	ASSERT(ref != NULL);
	ASSERT(attr != NULL);

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
	ASSERT(dev != NULL);
	ASSERT(ref != NULL);
	ASSERT(attr != NULL);

	int ret = 0;

	/* print debug attr_ref */
	CANIOT_DBG(F("attr_ref: section = %hhu, offset = %hhu, option = %hhu\n"),
		   ref->section,
		   ref->offset,
		   ref->option);

	switch (ref->section) {
	case section_identification: {
		read_identificate_attr(dev, ref, attr);
		break;
	}

	case section_system: {
		memcpy(&attr->val, (uint8_t *)&dev->system + ref->offset, ref->size);
		break;
	}

	case section_config: {
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
			     caniot_frame_type_t resp_type)
{
	ASSERT(dev != NULL);
	ASSERT(resp != NULL);

	caniot_did_t did;

	caniot_clear_frame(resp);

	/* read device class and id from ROM */
	read_identification_nodeid(dev, &did);

	/* id */
	resp->id.type  = resp_type;
	resp->id.query = CANIOT_RESPONSE;

	resp->id.cls = CANIOT_DID_CLS(did);
	resp->id.sid = CANIOT_DID_SID(did);
}

static void prepare_error(struct caniot_device *dev,
			  struct caniot_frame *resp,
			  caniot_frame_type_t query_type,
			  int error)
{
	ASSERT(resp != NULL);

	/* if the error occured during a command or query telemetry,
	 * then the error frame is a RESPONSE/COMMAND

	 * otherwise (if it's an attribute error), error frame is RESPONSE/WRITE_ATTR
	 */
	prepare_response(dev, resp, caniot_resp_error_for(query_type));

	resp->err = (int32_t)error;
	resp->len = 4U;
}

static int handle_read_attribute(struct caniot_device *dev,
				 struct caniot_frame *resp,
				 const struct caniot_attribute *attr)
{
	ASSERT(dev != NULL);
	ASSERT(resp != NULL);
	ASSERT(attr != NULL);

	int ret;
	struct attr_ref ref;

	CANIOT_DBG(F("Executing read attribute key = 0x%x\n"), attr->key);

	ret = attr_resolve(attr->key, &ref);

	prepare_response(dev, resp, CANIOT_FRAME_TYPE_READ_ATTRIBUTE);

	if (ret == 0) {
		/* if standard attribute */
		ret = attribute_read(dev, &ref, &resp->attr);
	} else { /* if custom attribute */
		if (dev->api->custom_attr.read != NULL) {
			/* temp variable to avoid `-Waddress-of-packed-member` warning */
			uint32_t tval = (uint32_t)-1;
			ret	      = dev->api->custom_attr.read(dev, attr->key, &tval);
			if (ret == 0) {
				resp->attr.val = tval;
			}
		} else {
			ret = -CANIOT_ENOATTR; /* unsupported custom attribute */
		}
	}

	/* finalize response */
	if (ret == 0) {
		resp->len      = 6u;
		resp->attr.key = attr->key;
	}

	return ret;
}

static int write_system_attr(struct caniot_device *dev,
			     const struct attr_ref *ref,
			     const struct caniot_attribute *attr)
{
	ASSERT(dev != NULL);
	ASSERT(ref != NULL);
	ASSERT(attr != NULL);

#if CONFIG_CANIOT_DRIVERS_API
	if (attr->key == 0x1010U) { /* time */
		uint32_t prev_sec;
		uint16_t prev_msec;
		dev->driv->get_time(&prev_sec, &prev_msec);
		dev->driv->set_time(attr->u32);

		/* adjust last_telemetry time,
		 * in order to not trigger it on time update
		 */
		dev->system.last_telemetry += attr->u32 - prev_sec * 1000u - prev_msec;
		dev->system.start_time += attr->u32 - prev_sec;

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

	memcpy((uint8_t *)&dev->system + ref->offset, &attr->val, ref->size);

	return 0;
}

static int attribute_write(struct caniot_device *dev,
			   const struct attr_ref *ref,
			   const struct caniot_attribute *attr)
{
	ASSERT(dev != NULL);
	ASSERT(ref != NULL);
	ASSERT(attr != NULL);

	if ((ref->option & WRITABLE) == 0U) {
		return -CANIOT_EROATTR;
	}

	int ret;

	/* print debug attr_ref */
	CANIOT_DBG(F("attr_ref: section = %hhu, offset = %hhu, option = %hhu\n"),
		   ref->section,
		   ref->offset,
		   ref->option);

	switch (ref->section) {
	case section_system: {
		ret = write_system_attr(dev, ref, attr);
		break;
	}
	case section_config: {
		ret = write_config_attr(dev, ref, attr);
		break;
	}

	default:
		ret = -CANIOT_EWRITEATTR;
	}

	return ret;
}

static int handle_write_attribute(struct caniot_device *dev,
				  const struct caniot_frame *req,
				  const struct caniot_attribute *attr)
{
	ASSERT(dev != NULL);
	ASSERT(req != NULL);
	ASSERT(attr != NULL);

	int ret;
	struct attr_ref ref;

	ret = attr_resolve(attr->key, &ref);

	if (ret == 0) {
		/* if standart attribute */
		ret = attribute_write(dev, &ref, &req->attr);
	} else { /* if custom attribute */
		if (dev->api->custom_attr.read != NULL) {
			ret = dev->api->custom_attr.write(dev, attr->key, req->attr.val);
		} else {
			ret = -CANIOT_ENOATTR; /* unsupported custom attribute */
		}
	}

	return ret;
}

static int handle_command_req(struct caniot_device *dev, const struct caniot_frame *req)
{
	ASSERT(dev != NULL);
	ASSERT(req != NULL);

	int ret;
	const caniot_endpoint_t ep = req->id.endpoint;

	CANIOT_DBG(F("Executing command handler (0x%p) for endpoint %d\n"),
		   (void *)&dev->api->command_handler,
		   ep);

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
				caniot_endpoint_t ep)
{
	ASSERT(dev != NULL);
	ASSERT(resp != NULL);

	int ret;

	/* TODO check endpoint relative to class*/

	if (dev->api->telemetry_handler == NULL) {
		return -CANIOT_EHANDLERT;
	}

	prepare_response(dev, resp, CANIOT_FRAME_TYPE_TELEMETRY);

	CANIOT_DBG(F("Executing telemetry handler (0x%p) for endpoint %d\n"),
		   (void *)&dev->api->telemetry_handler,
		   ep);

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
				  const struct caniot_frame *req,
				  struct caniot_frame *resp)
{
	ASSERT(dev != NULL);
	ASSERT(req != NULL);
	ASSERT(resp != NULL);

	int ret;

	/* no response in this case */
	if (req->id.query != CANIOT_QUERY) {
		ret = -EINVAL;
		goto exit;
	}

	dev->system.received.total++;

	switch (req->id.type) {
	case CANIOT_FRAME_TYPE_COMMAND: {
		dev->system.received.command++;
		ret = handle_command_req(dev, req);
		if (ret == 0) {
			ret = build_telemetry_resp(dev, resp, req->id.endpoint);
		}
		break;
	}
	case CANIOT_FRAME_TYPE_TELEMETRY: {
		dev->system.received.request_telemetry++;
		ret = build_telemetry_resp(dev, resp, req->id.endpoint);
		break;
	}

	case CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE: {
		dev->system.received.write_attribute++;
		ret = handle_write_attribute(dev, req, &req->attr);
		if (ret == 0) {
			ret = handle_read_attribute(dev, resp, &req->attr);
		}
		break;
	}
	case CANIOT_FRAME_TYPE_READ_ATTRIBUTE:
		dev->system.received.read_attribute++;
		ret = handle_read_attribute(dev, resp, &req->attr);
		break;
	}

exit:
	if (ret != 0) {
		prepare_error(dev, resp, req->id.type, ret);
	}
	return ret;
}

int caniot_device_verify(struct caniot_device *dev)
{
	(void)dev;

	return -CANIOT_ENIMPL;
}

bool caniot_device_time_synced(struct caniot_device *dev)
{
	ASSERT(dev != NULL);

	return dev->system.uptime_synced != 0;
}

/*____________________________________________________________________________*/

#if CONFIG_CANIOT_DRIVERS_API
uint32_t caniot_device_telemetry_remaining(struct caniot_device *dev)
{
	ASSERT(dev != NULL);

	if (prepare_config_read(dev) == 0) {
		uint32_t sec;
		uint16_t msec;
		dev->driv->get_time(&sec, &msec);

		const uint32_t now = sec * 1000 + msec;

		CANIOT_DBG(F("now=%u ms last_telemetry = %u ms\n"),
			   now,
			   dev->system.last_telemetry);

		const uint32_t diff = now - dev->system.last_telemetry;
		if (dev->config->telemetry.period <= diff) {
			return 0;
		} else {
			return dev->config->telemetry.period - diff;
		}
	}

	/* default 1 second */
	return 1000u;
}

static uint32_t get_response_delay(struct caniot_device *dev, bool random)
{
	ASSERT(dev != NULL);

	uint32_t delay_ms = 0U;

	/* delay only on broadcast command */
	if (random == true) {
		ASSERT(dev->driv->entropy != NULL);

		/* define default parameters */
		uint16_t delay_min = CANIOT_TELEMETRY_DELAY_MIN_DEFAULT;
		uint16_t delay_max = CANIOT_TELEMETRY_DELAY_MAX_DEFAULT;

		uint16_t rdm;
		dev->driv->entropy((uint8_t *)&rdm, sizeof(rdm));

		/* get parameters from local configuration if possible */
		int ret = prepare_config_read(dev);
		if (ret == 0) {
			delay_min = dev->config->telemetry.delay_min;
			delay_max = dev->config->telemetry.delay_max;
		}

		/* evaluate amplitude */
		uint32_t amplitude = CANIOT_TELEMETRY_DELAY_MAX_DEFAULT;
		if (delay_max > delay_min) {
			amplitude = delay_max - delay_min;
		}

		delay_ms = delay_min + (rdm % amplitude);
	}

	return delay_ms;
}

static inline bool telemetry_requested(struct caniot_device *dev)
{
	ASSERT(dev != NULL);

	return dev->flags.request_telemetry == 1u;
}

int caniot_device_process(struct caniot_device *dev)
{
	ASSERT(dev != NULL);

	int ret;
	struct caniot_frame req, resp;

	/* get current time */
	uint16_t msec;
	dev->driv->get_time(&dev->system.time, &msec);
	dev->system.uptime = dev->system.time - dev->system.start_time;

	/* check if we need to send telemetry (calculated in seconds) */
	prepare_config_read(dev);
	const uint32_t now	   = dev->system.time * 1000 + msec;
	const uint32_t ellapsed_ms = now - dev->system.last_telemetry;
	if (ellapsed_ms >= dev->config->telemetry.period) {
		dev->flags.request_telemetry = 1;

		CANIOT_DBG(F("Requesting telemetry\n"));
	}

	/* received any incoming frame */
	caniot_clear_frame(&req);
	ret = dev->driv->recv(&req);

	/* response delay is not random by default */
	bool random_delay = false;

	/* if we received a frame */
	if (ret == 0) {
#if CONFIG_CANIOT_DEBUG
		if (!caniot_device_is_target(caniot_device_get_id(dev), &req)) {
			dev->system.received.ignored++;
			CANIOT_ERR(F("Unexpected frame id received: %u\n"));
		}
#endif

		/* handle received frame */
		ret = caniot_device_handle_rx_frame(dev, &req, &resp);

		/* broadcast request requires a randomly delayed response */
		if (caniot_is_broadcast(caniot_frame_get_did(&req)) == true) {
			random_delay = true;
		}
		/* if we didn't received a frame but telemetry is requested */
	} else if (telemetry_requested(dev)) {
		/* prepare telemetry response */
		ret = build_telemetry_resp(
			dev, &resp, dev->config->flags.telemetry_endpoint);
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
	ret = dev->driv->send(&resp, get_response_delay(dev, random_delay));
	if (ret == 0) {
		dev->system.sent.total++;

		if (is_telemetry_response(&resp) == true) {
			dev->system.last_telemetry   = now;
			dev->flags.request_telemetry = 0;
		}
	}

exit:
	return ret;
}

void caniot_app_init(struct caniot_device *dev)
{
	ASSERT(dev != NULL);
	ASSERT(dev->driv != NULL);
	ASSERT(dev->driv->get_time != NULL);

	memset(&dev->system, 0x00U, sizeof(dev->system));

	dev->driv->get_time(&dev->system.start_time, NULL);
}

#endif /* CONFIG_CANIOT_DRIVERS_API */