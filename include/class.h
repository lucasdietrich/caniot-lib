#ifndef _CANIOT_CLASS_H
#define _CANIOT_CLASS_H

#include "caniot.h"

#include <stdint.h>

/* Data types */

// MASK 0b0000001111111111000000111111111100000011111111111111111111111111
struct caniot_C0 {
	union {
		struct {
			uint8_t c0 : 1;
			uint8_t c1 : 1;
			uint8_t c2 : 1;
			uint8_t c3 : 1;
			uint8_t c4 : 1;
			uint8_t c5 : 1;
			uint8_t c6 : 1;
			uint8_t c7 : 1;
		};
		uint8_t contacts;
	};
	union {
		struct {
			uint8_t r0:1;
			uint8_t r1:1;
			uint8_t r2:1;
			uint8_t r3:1;
			uint8_t r4:1;
			uint8_t r5:1;
			uint8_t r6:1;
			uint8_t r7:1;
		};
		uint8_t relays;
	};
	struct {
		uint16_t temperature: 10;
	};
	struct {
		uint16_t humidity: 10;
	};
	struct {
		uint16_t pressure: 10;
	};
};

#define CANIOT_INTERPRET(buf, s) \
	((struct s *)buf)

#define CANIOT_INTERPRET_C0(buf) \
	CANIOT_INTERPRET(buf, caniot_C0)

int caniot_class_endpoints_count(uint8_t class);

bool caniot_class_valid_endpoint(uint8_t class, uint8_t endpoint);

/* Class 0 */

#endif /* _CANIOT_CLASS_H */