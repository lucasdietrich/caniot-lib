#ifndef _CANIOT_DATATYPE_H
#define _CANIOT_DATATYPE_H

#include "caniot.h"

#include <stdint.h>

/* Data types */

struct caniot_CRTHPT {
	union {
		struct {
			uint8_t c1 : 1;
			uint8_t c2 : 1;
			uint8_t c3 : 1;
			uint8_t c4 : 1;
			uint8_t c5 : 1;
			uint8_t c6 : 1;
			uint8_t c7 : 1;
			uint8_t c8 : 1;
		};
		uint8_t contacts;
	};
	union {
		struct {
			uint8_t r1:1;
			uint8_t r2:1;
			uint8_t r3:1;
			uint8_t r4:1;
			uint8_t r5:1;
			uint8_t r6:1;
			uint8_t r7:1;
			uint8_t r8:1;
		};
		uint8_t relays;
	};
	struct {
		uint16_t int_temperature: 10;
		uint16_t humidity: 10;
		uint16_t pressure: 10;
		uint16_t ext_temperature: 10;
	};
};

#define CANIOT_INTERPRET(buf, s) \
	((struct s *)buf)

#define CANIOT_INTERPRET_CRTHP(buf) \
	CANIOT_INTERPRET(buf, caniot_CRTHPT)

#define AS(buf, s) CANIOT_INTERPRET(buf, s)

#define AS_CRTHPT(buf) CANIOT_INTERPRET(buf, caniot_CRTHPT)

int caniot_dt_endpoints_count(uint8_t cls);

bool caniot_dt_valid_endpoint(uint8_t cls, uint8_t endpoint);

/* conversion functions */

uint16_t caniot_dt_T16_to_Temp(int16_t T16);

int16_t caniot_dt_Temp_to_T16(uint16_t T);


#endif /* _CANIOT_DATATYPE_H */