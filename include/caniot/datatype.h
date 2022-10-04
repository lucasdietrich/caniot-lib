#ifndef _CANIOT_DATATYPE_H
#define _CANIOT_DATATYPE_H

#include "caniot.h"

#include "classes/class0.h"
#include "classes/class1.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

typedef enum {
	CANIOT_HEATER_NONE = 0,
	CANIOT_HEATER_CONFORT,
	CANIOT_HEATER_CONFORT_MIN_1,
	CANIOT_HEATER_CONFORT_MIN_2,
	CANIOT_HEATER_ENERGY_SAVING,
	CANIOT_HEATER_FROST_FREE,
	CANIOT_HEATER_OFF
} caniot_heating_status_t;

#define CANIOT_SHUTTER_CMD_NONE 0xFFu
#define CANIOT_SHUTTER_CMD_OPENNES(_o) (_o)
#define CANIOT_SHUTTER_CMD_OPEN (100u)
#define CANIOT_SHUTTER_CMD_CLOSE (0u)

#define CANIOT_BLT_SIZE sizeof(struct caniot_blc0_telemetry)

struct caniot_blc_sys_command
{
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

	/* Reset the device configuration */
	caniot_onestate_cmd_t config_reset : 1;

	uint8_t _unused10 : 2;
} __attribute__((packed));


/* is the same as board level telemetry (blt) */
struct caniot_blc0_telemetry
{
	uint8_t dio;
	uint8_t pdio : 4;
	uint8_t _unused : 4;
	uint16_t int_temperature : 10;
	uint16_t ext_temperature : 10;
	uint16_t ext_temperature2 : 10;
	uint16_t ext_temperature3 : 10;
} __attribute__((packed));

/* Board level control (blc) command */
struct caniot_blc0_command
{
	uint16_t coc1 : 3u;
	uint16_t coc2 : 3u;
	uint16_t crl1 : 3u;
	uint16_t crl2 : 3u;

	uint8_t _unused : 4u;

	uint8_t _unused9[5u];
} __attribute__((packed));

struct caniot_blc1_command
{
	uint8_t _unused[7u];
};

struct caniot_blc_command
{
	union {
		struct caniot_blc0_command blc0;
		struct caniot_blc1_command blc1;
		uint8_t _unused[7u];
	};

	struct caniot_blc_sys_command sys;
} __attribute__((packed));

/* Same for command and telemetry */
struct caniot_heating_control
{
	caniot_heating_status_t heater1_cmd: 4u;
	caniot_heating_status_t heater2_cmd: 4u;
	caniot_heating_status_t heater3_cmd: 4u;
	caniot_heating_status_t heater4_cmd: 4u;

	uint8_t shutters_openness[4u];
};

void caniot_blc0_command_init(struct caniot_blc0_command *cmd);

#define CANIOT_INTERPRET(buf, s) \
	((struct s *)buf)

#define AS(buf, s) CANIOT_INTERPRET(buf, s)

#define AS_BLC_COMMAND(buf) CANIOT_INTERPRET(buf, caniot_blc_command)
#define AS_BLC0_COMMAND(buf) CANIOT_INTERPRET(buf, caniot_blc0_command)
#define AS_BLC0_TELEMETRY(buf) CANIOT_INTERPRET(buf, caniot_blc0_telemetry)

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

#ifdef __cplusplus
}
#endif

#endif /* _CANIOT_DATATYPE_H */