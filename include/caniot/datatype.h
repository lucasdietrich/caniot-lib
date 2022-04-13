#ifndef _CANIOT_DATATYPE_H
#define _CANIOT_DATATYPE_H

#include "caniot.h"

#include <stdint.h>

/* Data types */

typedef enum
{
	CANIOT_STATE_OFF = 0,
	CANIOT_STATE_ON
} caniot_state_t;

typedef enum 
{
	CANIOT_SS_CMD_RESET = 0,
	CANIOT_SS_CMD_SET
} caniot_onestate_cmd_t;

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

typedef enum {
	CANIOT_PHYS_HYSTERESIS_UNDEF = 0,
	CANIOT_PHYS_HYSTERESIS_LOW,
	CANIOT_PHYS_HYSTERESIS_HIGH,
} caniot_phys_hysteresis_state_t;

/**
 * @brief 
 * 
 * Note: Is compatible with caniot_twostate_cmd_t or caniot_light_cmd_t for example
 */
typedef enum {
	CANIOT_XPS_NONE = 0,
	CANIOT_XPS_SET_ON,
	CANIOT_XPS_SET_OFF,
	CANIOT_XPS_TOGGLE,
	CANIOT_XPS_RESET,
	CANIOT_XPS_PULSE_ON,
	CANIOT_XPS_PULSE_OFF,
	CANIOT_XPS_PULSE_CANCEL,
} caniot_complex_digital_cmd_t;

#pragma pack(push, 1)
struct caniot_board_control_telemetry
{
	union {
		struct {
			uint8_t rl1 : 1;
			uint8_t rl2 : 1;
			uint8_t oc1 : 1;
			uint8_t oc2 : 1;
			uint8_t in1 : 1;
			uint8_t in2 : 1;
			uint8_t in3 : 1;
			uint8_t in4 : 1;
		};
		uint8_t dio;
	};

	uint8_t poc1 : 1;
	uint8_t poc2 : 1;
	uint8_t prl1 : 1;
	uint8_t prl2 : 1;

	uint8_t _unused : 4;

	uint16_t int_temperature : 10;
	uint16_t ext_temperature : 10;
};

#pragma pack(push, 1)
struct caniot_board_control_command
{
	caniot_complex_digital_cmd_t coc1 : 3;
	caniot_complex_digital_cmd_t coc2 : 3;
	caniot_complex_digital_cmd_t crl1 : 3;
	caniot_complex_digital_cmd_t crl2 : 3;

	uint8_t _unused : 4;
	
	uint8_t _unused9[5];

	/* in the case of the AVR, proper software reset should use the watchdog :
	* https://www.avrfreaks.net/comment/178013#comment-178013
	*/

	/* Default reset function (Recommended). Can be linked to Watchdog reset if
	* using the watchdog is recommanded for a proper MCU reset.
	*/
	caniot_onestate_cmd_t reset : 1;

	/* Software reset by calling reset vector */
	caniot_onestate_cmd_t software_reset : 1;

	/* Reset by forcing the watchdog to timeout */
	caniot_onestate_cmd_t watchdog_reset : 1;

	/* Enable/disable the watchdog */
	caniot_twostate_cmd_t watchdog : 2;

	uint8_t _unused10 : 3;
};

#pragma pack(push, 1)
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
#define AS_BOARD_CONTROL_CMD(buf) CANIOT_INTERPRET(buf, caniot_board_control_command)
#define AS_BOARD_CONTROL_TELEMETRY(buf) CANIOT_INTERPRET(buf, caniot_board_control_telemetry)

int caniot_dt_endpoints_count(uint8_t cls);

bool caniot_dt_valid_endpoint(uint8_t cls, uint8_t endpoint);

/* conversion functions */

uint16_t caniot_dt_T16_to_T10(int16_t T16);

int16_t caniot_dt_T10_to_T16(uint16_t T);


/* constants */
#define CANIOT_DT_T16_INVALID ((int16_t) INT16_MAX)
#define CANIOT_DT_T10_INVALID ((uint16_t) 0x3FFU)
#define CANIOT_DT_T8_INVALID ((uint8_t) 0xFFU)

#define CANIOT_DT_VALID_T16_TEMP(temp) ((temp) != CANIOT_DT_T16_INVALID)
#define CANIOT_DT_VALID_T10_TEMP(temp) ((temp) != CANIOT_DT_T10_INVALID)


#endif /* _CANIOT_DATATYPE_H */