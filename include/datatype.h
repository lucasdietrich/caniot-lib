#ifndef _CANIOT_DATATYPE_H
#define _CANIOT_DATATYPE_H

#include "caniot.h"

#include <stdint.h>

/* Data types */

/**
 * @brief Commands for controlling a two state output:
 * 
 * Note: TS stands for "TwoState"
 */
typedef enum
{
	CANIOT_TS_CMD_NONE = 0,
	CANIOT_TS_CMD_ON,
	CANIOT_TS_CMD_OFF,
	CANIOT_TS_CMD_TOGGLE,
} caniot_twostate_cmd_t;

typedef enum
{
	CANIOT_LIGHT_CMD_NONE = 0,
	CANIOT_LIGHT_CMD_ON,
	CANIOT_LIGHT_CMD_OFF,
	CANIOT_LIGHT_CMD_TOGGLE,
} caniot_light_cmd_t;

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
			uint8_t r1 : 1;
			uint8_t r2 : 1;
			uint8_t r3 : 1;
			uint8_t r4 : 1;
			uint8_t r5 : 1;
			uint8_t r6 : 1;
			uint8_t r7 : 1;
			uint8_t r8 : 1;
		};
		uint8_t relays;
		union {
			struct {
				caniot_light_cmd_t lights1 : 2;
				caniot_light_cmd_t lights2 : 2;
				caniot_light_cmd_t lights3 : 2;
				caniot_light_cmd_t lights4 : 2;
			};
			struct {
				caniot_light_cmd_t cmd1 : 2;
				caniot_light_cmd_t cmd2 : 2;
				caniot_light_cmd_t cmd3 : 2;
				caniot_light_cmd_t cmd4 : 2;
			};
		};
	};
	struct {
		uint16_t int_temperature : 10;
		uint16_t humidity : 10;
		uint16_t pressure : 10;
		uint16_t ext_temperature : 10;
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