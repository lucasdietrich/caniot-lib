#ifndef _CANIOT_H
#define _CANIOT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "caniot_errors.h"
#include "caniot_common.h"

/**
 * @brief Helper for printing strings :
 * 
 * printf(F("Hello %d\n"), 42);
 * 
 * Equivalent to :
 * - AVR : printf_P(PSTR("Hello %d\n"), 42);
 * - ARM/x86/any : printf("Hello %d\n", 42);
 */
#ifdef __AVR__
#include <avr/pgmspace.h>
#define printf	printf_P
#define FLASH_STRING(x) PSTR(x)
#else
#define printf  printf
#define FLASH_STRING(x) (x)
#endif
#define F(x) FLASH_STRING(x)


#define CANIOT_ID(t, q, c, d, e) ((t) | (q << 2) | (c << 3) | (d << 6) | (e << 9))

#define CANIOT_CLASS_BROADCAST	0x7

#define CANIOT_DEVICE(class, sub_id)	((union deviceid) { .cls = class, .dev = sub_id })

#define CANIOT_DEVICE_BROADCAST CANIOT_DEVICE(0x7, CANIOT_CLASS_BROADCAST)

union deviceid {
	struct {
		uint8_t cls : 3;
		uint8_t dev : 3;
	};
	uint8_t val;
};

enum { command = 0, telemetry = 1, write_attribute = 2, read_attribute = 3 };
enum { query = 0, response = 1 };
enum { endpoint_default = 0, endpoint_1 = 1, endpoint_2 = 2, endpoint_broadcast = 3 };

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
union caniot_id {
	struct {
		uint16_t type : 2;
		uint16_t query : 1;
		uint16_t cls : 3;
		uint16_t dev : 3;
		uint16_t endpoint : 2;
	};
	uint16_t raw;
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
	union caniot_id id;
	
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

typedef int (*caniot_query_callback_t)(union deviceid did,
				       struct caniot_frame *resp);

struct caniot_drivers_api {
	/* arch R/W */
	void (*rom_read)(void *p, void *d, uint8_t size);
	void (*persistent_read)(void *p, void *d, uint8_t size);
	void (*persistent_write)(void *p, void *s, uint8_t size);

	/* util */
	void (*entropy)(uint8_t *buf, size_t len);
	void (*get_time)(uint32_t *sec, uint32_t *usec);

	/**
	 * @brief Send a CANIOT frame
	 * 
	 * Return 0 on success, any other value on error.
	 */
	int (*send)(struct caniot_frame *frame, uint32_t delay);

	/**
	 * @brief Receive a CANIOT frame.
	 * 
	 * Return 0 on success, -EAGAIN if no frame is available.
	 */
	int (*recv)(struct caniot_frame *frame);
	
	/* CAN configuration */
	int (*set_filter) (struct caniot_filter *filter);
	int (*set_mask) (struct caniot_filter *filter);

	/* Device specific */
	bool (*pending_telemetry)(void);
};

// Return if deviceid is valid
static inline bool caniot_valid_deviceid(union deviceid id)
{
	return id.val <= CANIOT_DEVICE_BROADCAST.val;
}

// Return if deviceid is broadcast
static inline bool caniot_is_class_broadcast(union deviceid id)
{
	return id.dev == CANIOT_CLASS_BROADCAST;
}

static inline void caniot_clear_frame(struct caniot_frame *frame)
{
	memset(frame, 0x00, sizeof(struct caniot_frame));
}

static inline bool caniot_is_error(union caniot_id id)
{
	return (id.query == response) && (id.type == command);
}

// Check if drivers api is valid
bool caniot_valid_drivers_api(struct caniot_drivers_api *api);

#endif