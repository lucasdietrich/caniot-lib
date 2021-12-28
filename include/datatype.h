#ifndef _CANIOT_DATATYPE_H
#define _CANIOT_DATATYPE_H

#include "caniot.h"

#include <stdint.h>

/* Data types */

// MASK 0b0000001111111111000000111111111100000011111111111111111111111111
struct caniot_CRTHP {
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

#define CANIOT_INTERPRET_CRTHP(buf) \
	CANIOT_INTERPRET(buf, caniot_CRTHP)

int caniot_dt_endpoints_counts(uint8_t class);

bool caniot_dt_valid_endpoint(uint8_t class, uint8_t endpoint);

/* Class 0 */

#endif /* _CANIOT_DATATYPE_H */