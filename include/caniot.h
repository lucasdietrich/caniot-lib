#ifndef _CANIOT_H
#define _CANIOT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "caniot_errors.h"

#ifdef CONFIG_CANIOT_DRIVERS_API
#define CANIOT_DRIVERS_API CONFIG_CANIOT_DRIVERS_API
#else
#define CANIOT_DRIVERS_API 0
#endif /* CONFIG_CANIOT_DRIVERS_API */

/**
 * @brief Helper for printing strings :
 * 
 * printf(F("Hello %d\n"), 42);
 * 
 * Equivalent to :
 * - AVR : printf_P(PSTR("Hello %d\n"), 42);
 * - ARM/x86/any : printf("Hello %d\n", 42);
 */
#include <stdio.h>

/* 0 : NO DEBUG
 * 1 : ERR
 * 2 : WRN
 * 3 : INF
 * 4 : DBG
 */
#ifdef CONFIG_CANIOT_LOG_LEVEL
#define CANIOT_LOG_LEVEL CONFIG_CANIOT_LOG_LEVEL
#else
#define CANIOT_LOG_LEVEL 4
#endif

#if CANIOT_LOG_LEVEL >= 4
#define CANIOT_DBG(...) printf(__VA_ARGS__)
#else
#define CANIOT_DBG(...)
#endif /* CANIOT_LOG_LEVEL >= 4 */

#if CANIOT_LOG_LEVEL >= 3
#define CANIOT_INF(...) printf(__VA_ARGS__)
#else
#define CANIOT_INF(...)
#endif /* CANIOT_LOG_LEVEL >= 3 */

#if CANIOT_LOG_LEVEL >= 2
#define CANIOT_WRN(...) printf(__VA_ARGS__)
#else
#define CANIOT_WRN(...)
#endif /* CANIOT_LOG_LEVEL >= 2 */

#if CANIOT_LOG_LEVEL >= 1
#define CANIOT_ERR(...) printf(__VA_ARGS__)
#else
#define CANIOT_ERR(...)
#endif /* CANIOT_LOG_LEVEL >= 1 */


#define CANIOT_VERSION1	1
#define CANIOT_VERSION2 2
#define CANIOT_VERSION 	CANIOT_VERSION2

#define CANIOT_MAX_PENDING_QUERIES	2


#define CANIOT_ID(t, q, c, d, e) ((t) | (q << 2) | (c << 3) | (d << 6) | (e << 9))

#define CANIOT_CLASS_BROADCAST	(0x7)

#define CANIOT_DEVICE(class_id, sub_id)	((union deviceid) { { .cls = class_id, .sid = sub_id} })

#define CANIOT_DEVICE_BROADCAST CANIOT_DEVICE(0x7, 0x7)

#define CANIOT_DEVICE_EQUAL(did1, did2) ((did1).val == (did2).val)

#define CANIOT_DEVICE_IS_BROADCAST(did) CANIOT_DEVICE_EQUAL(did, CANIOT_DEVICE_BROADCAST)

/* milliseconds */
#define CANIOT_TELEMETRY_DELAY_MIN_DEFAULT	0
#define CANIOT_TELEMETRY_DELAY_MAX_DEFAULT	100
#define CANIOT_TELEMETRY_DELAY_DEFAULT		5

/* seconds */
#define CANIOT_TELEMETRY_PERIOD_DEFAULT		60

#define CANIOT_TELEMETRY_ENDPOINT_DEFAULT	0x00

union deviceid {
	struct {
		uint8_t cls : 3; /* device class */
		uint8_t sid : 3; /* device sub-id */
	};
	uint8_t val;
};

enum { command = 0, telemetry = 1, write_attribute = 2, read_attribute = 3 };
enum { query = 0, response = 1 };
enum { endpoint_default = 0, endpoint_1 = 1, endpoint_2 = 2, endpoint_broadcast = 3 };

struct caniot_data
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
		uint16_t sid : 3;
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

typedef int (*caniot_query_callback_t)(union deviceid did,
				       struct caniot_frame *resp);

struct caniot_drivers_api {
	/* util */
	void (*entropy)(uint8_t *buf, size_t len);
	void (*get_time)(uint32_t *sec, uint16_t *ms);

	/**
	 * @brief Send a CANIOT frame
	 * 
	 * Return 0 on success, any other value on error.
	 */
	int (*send)(const struct caniot_frame *frame, uint32_t delay_ms);

	/**
	 * @brief Receive a CANIOT frame.
	 * 
	 * Return 0 on success, -EAGAIN if no frame is available.
	 */
	int (*recv)(struct caniot_frame *frame);
};

// Return if deviceid is valid
static inline bool caniot_valid_deviceid(union deviceid id)
{
	return id.val <= CANIOT_DEVICE_BROADCAST.val;
}

// Return if deviceid is broadcast
static inline bool caniot_is_broadcast(union deviceid id)
{
	return CANIOT_DEVICE_IS_BROADCAST(id);
}

static inline void caniot_clear_frame(struct caniot_frame *frame)
{
	memset(frame, 0x00, sizeof(struct caniot_frame));
}

static inline bool caniot_is_error_frame(union caniot_id id)
{
	return (id.query == response) && (id.type == command);
}

static inline bool is_telemetry_response(struct caniot_frame *frame)
{
	return frame->id.query == response && frame->id.type == telemetry;
}

// Check if drivers api is valid
bool caniot_validate_drivers_api(struct caniot_drivers_api *api);

void caniot_show_deviceid(union deviceid did);

void caniot_show_frame(const struct caniot_frame *frame);

void caniot_explain_id(union caniot_id id);

void caniot_explain_frame(const struct caniot_frame *frame);

void caniot_build_query_telemetry(struct caniot_frame *frame,
				 union deviceid did,
				 uint8_t endpoint);

void caniot_build_query_command(struct caniot_frame *frame,
				union deviceid did,
				uint8_t endpoint,
				const uint8_t *buf,
				uint8_t size);

bool caniot_is_error(int cterr);

void caniot_show_error(int cterr);

#endif