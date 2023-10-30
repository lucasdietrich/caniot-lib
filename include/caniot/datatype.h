/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CANIOT_DATATYPE_H
#define _CANIOT_DATATYPE_H

#include "caniot.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Data types */

typedef enum {
	CANIOT_STATE_OFF = 0,
	CANIOT_STATE_ON
} caniot_state_t;

typedef enum {
	CANIOT_SS_CMD_RESET = 0,
	CANIOT_SS_CMD_SET
} caniot_onestate_cmd_t;

/**
 * @brief Commands for controlling a two state output:
 *
 * Note: TS stands for "TwoState"
 */
typedef enum {
	CANIOT_TS_CMD_NONE = 0,
	CANIOT_TS_CMD_ON,
	CANIOT_TS_CMD_OFF,
	CANIOT_TS_CMD_TOGGLE,
} caniot_twostate_cmd_t;

typedef enum {
	CANIOT_TSP_CMD_NONE = 0,
	CANIOT_TSP_CMD_ON,
	CANIOT_TSP_CMD_OFF,
	CANIOT_TSP_CMD_PULSE,
} caniot_twostate_pulse_cmd_t;

typedef enum {
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
	CANIOT_HEATER_NONE			   = 0,
	CANIOT_HEATER_COMFORT		   = 1,
	CANIOT_HEATER_COMFORT_MIN_1	   = 2,
	CANIOT_HEATER_COMFORT_MIN_2	   = 3,
	CANIOT_HEATER_ENERGY_SAVING	   = 4,
	CANIOT_HEATER_FROST_PROTECTION = 5,
	CANIOT_HEATER_STOP			   = 6
	// 7 is reserved for future use
} caniot_heating_mode_t;

struct caniot_blc_sys_command {
	/* in the case of the AVR, proper software reset should use the watchdog :
	 * https://www.avrfreaks.net/comment/178013#comment-178013
	 */

	/* Default reset function (Recommended). Can be linked to Watchdog reset if
	 * using the watchdog is recommanded for a proper MCU reset.
	 */
	caniot_onestate_cmd_t reset : 1;

	/* Software reset by calling reset vector
	 * DEPRECATED, use global reset instead
	 */
	caniot_onestate_cmd_t _software_reset : 1;

	/* Reset by forcing the watchdog to timeout
	 * DEPRECATED, use global reset instead
	 */
	caniot_onestate_cmd_t _watchdog_reset : 1;

	/* Enable/disable the watchdog */
	caniot_twostate_cmd_t watchdog : 2;

	/* Reset the device configuration */
	caniot_onestate_cmd_t config_reset : 1;

	/* Set the device in safe configuration.

	 * This mode tupically stops all running actions (like heating, siren, ...)
	 * and set the device in a safe/inactive state.
	 *
	 * - CANIOT_TSP_CMD_ON: Inhibit all actions until CANIOT_TSP_CMD_OFF is
	 * received.
	 * - CANIOT_TSP_CMD_OFF: Resume normal operation.
	 * - CANIOT_TSP_CMD_PULSE: Inhibit all actions momentarily, the mode won't
	 *   last. The device will resume normal operation after receiving its
	 *   next event (command, external event, ...).
	 */
	caniot_twostate_pulse_cmd_t inhibit : 2;
};

/* Same for command and telemetry */
struct caniot_heating_control {
	caniot_heating_mode_t heater1_cmd : 4u;
	caniot_heating_mode_t heater2_cmd : 4u;
	caniot_heating_mode_t heater3_cmd : 4u;
	caniot_heating_mode_t heater4_cmd : 4u;
	uint8_t power_status : 1u; /* Tells whether power is detected or not, telemetry
					  only */
};

#define CANIOT_SHUTTER_CMD_NONE		   0xFFu
#define CANIOT_SHUTTER_CMD_OPENNES(_o) (_o)
#define CANIOT_SHUTTER_CMD_OPEN		   (100u)
#define CANIOT_SHUTTER_CMD_CLOSE	   (0u)

struct caniot_shutters_control {
	uint8_t shutters_openness[4u];
};

void caniot_caniot_blc_sys_command_init(struct caniot_blc_sys_command *cmd);

uint8_t caniot_blc_sys_command_to_byte(const struct caniot_blc_sys_command *cmd);
void caniot_blc_sys_command_from_byte(struct caniot_blc_sys_command *cmd, uint8_t byte);

/* conversion functions */
uint16_t caniot_dt_T16_to_T10(int16_t T16);

int16_t caniot_dt_T10_to_T16(uint16_t T);

/* constants */
#define CANIOT_DT_T16_MIN ((int16_t)-2800)
#define CANIOT_DT_T16_MAX ((int16_t)7200)

#define CANIOT_DT_T16_INVALID ((int16_t)INT16_MAX)
#define CANIOT_DT_T10_INVALID ((uint16_t)0x3FFU)
#define CANIOT_DT_T8_INVALID  ((uint8_t)0xFFU)

#define CANIOT_DT_VALID_T16_TEMP(temp) ((temp) != CANIOT_DT_T16_INVALID)
#define CANIOT_DT_VALID_T10_TEMP(temp) ((temp) != CANIOT_DT_T10_INVALID)

#ifdef __cplusplus
}
#endif

#endif /* _CANIOT_DATATYPE_H */