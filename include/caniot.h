#ifndef _CANIOT_H
#define _CANIOT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "caniot_errors.h"
#include "caniot_common.h"

#define MEMBER_SIZEOF(s, member)	(sizeof(((s *)0)->member))
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#define CONTAINER_OF(ptr, type, field) ((type *)(((char *)(ptr)) - offsetof(type, field)))
#define MIN(a, b) (a < b ? a : b)

union deviceid {
	struct {
		uint8_t cls : 3;
		uint8_t dev : 3;
	};
	uint8_t val;
};

enum { command = 0, telemetry = 1, write_attribute = 3, read_attribute = 2 };
enum { query = 0, response = 1 };
enum { endpoint_default = 0, endpoint_1 = 1, endpoint_2 = 2, endpoint_broadcast = 3 };

enum { device_broadcast = 0b111111 };

struct caniot_command
{
	uint8_t ep;
	uint8_t buf[8];
	uint8_t len;
};

struct caniot_telemetry
{
	uint8_t ep;
	uint8_t buf[8];
	uint8_t len;
};

/* https://stackoverflow.com/questions/7957363/effects-of-attribute-packed-on-nested-array-of-structures */
struct caniot_id {
        uint16_t type : 2;
        uint16_t query : 1;
        uint16_t cls: 3;
        uint16_t dev: 3;
        uint16_t endpoint : 2;
};

struct caniot_attribute
{
	union {
		uint16_t key;
		struct {
			uint16_t section : 4;
			uint16_t attribute : 8;
			uint16_t part : 4;
		};
	};
	union {
		uint32_t u32;
		uint16_t u16;
		uint8_t u8;
		uint32_t val;
	};
};

struct caniot_frame {
        struct caniot_id id;
        union {
		char buf[8];
                struct caniot_attribute attr;
		int32_t err;
        };
        uint8_t len;
};

struct caniot_filter
{
	uint32_t filter;
	uint32_t mask;
	uint8_t ext;
};

struct caniot_drivers_api {
	/* arch R/W */
	void (*rom_read)(void *p, void *d, uint8_t size);
	void (*persistent_read)(void *p, void *d, uint8_t size);
	void (*persistent_write)(void *p, void *s, uint8_t size);

	/* util */
	void (*entropy)(uint8_t *buf, size_t len);

	/* event (RTOS API) */
	int (*schedule)(void *event, int32_t delay, void (*callback)(void *event)); /* -1 is forever */
	int (*unschedule)(void *event);

	/* CAN */
	int (*send)(struct caniot_frame *frame, uint32_t delay); /* TX queue */
	int (*recv)(struct caniot_frame *frame); /* RX queue */
	int (*set_filter) (struct caniot_filter *filter);
	int (*set_mask) (struct caniot_filter *filter);

	/* device specific */
	bool (*pending_telemetry)(void);
};

// Return if deviceid is valid
static inline bool caniot_valid_deviceid(union deviceid id)
{
	return id.val <= device_broadcast;
}

// Return if deviceid is broadcast
static inline bool caniot_is_broadcast(union deviceid id)
{
	return id.val == device_broadcast;
}

#endif