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

#define CANIOT_SHUTTER_CMD_NONE	       0xFFu
#define CANIOT_SHUTTER_CMD_OPENNES(_o) (_o)
#define CANIOT_SHUTTER_CMD_OPEN	       (100u)
#define CANIOT_SHUTTER_CMD_CLOSE       (0u)

#define CANIOT_BLC_SYS_RESET_MASK	    0x1u
#define CANIOT_BLC_SYS_SOFT_RESET_MASK	    0x2u
#define CANIOT_BLC_SYS_WATCHDOG_RESET_MASK  0x4u
#define CANIOT_BLC_SYS_WATCHDOG_MASK	    0x18u
#define CANIOT_BLC_SYS_WATCHDOG_ENABLE_MASK 0x10u

struct caniot_blc_sys_command {
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
};

#define CANIOT_BLC0_TELEMETRY_BUF_LEN 7
#define CANIOT_BLC0_COMMAND_BUF_LEN   2

#define CANIOT_BLC1_TELEMETRY_BUF_LEN 8
#define CANIOT_BLC1_COMMAND_BUF_LEN   7

struct caniot_blc0_telemetry {
	uint8_t dio;
	uint8_t pdio : 4;
	uint16_t int_temperature : 10;
	uint16_t ext_temperature : 10;
	uint16_t ext_temperature2 : 10;
	uint16_t ext_temperature3 : 10;
};

/* Board level control (blc) command */
struct caniot_blc0_command {
	uint16_t coc1 : 3u;
	uint16_t coc2 : 3u;
	uint16_t crl1 : 3u;
	uint16_t crl2 : 3u;
};

struct caniot_blc1_telemetry {
	uint8_t pcpd;
	uint8_t eio;
	uint8_t pb0 : 1;
	uint8_t pe0 : 1;
	uint8_t pe1 : 1;
	uint32_t int_temperature : 10;
	uint32_t ext_temperature : 10;
	uint32_t ext_temperature2 : 10;
	uint32_t ext_temperature3 : 10;
};

/* TODO remove bitfields*/
struct caniot_blc1_command {
	uint64_t cpc0 : 3u;
	uint64_t cpc1 : 3u;
	uint64_t cpc2 : 3u;
	uint64_t cpc3 : 3u;
	uint64_t cpd0 : 3u;
	uint64_t cpd1 : 3u;
	uint64_t cpd2 : 3u;
	uint64_t cpd3 : 3u;
	uint64_t ceio0 : 3u;
	uint64_t ceio1 : 3u;
	uint64_t ceio2 : 3u;
	uint64_t ceio3 : 3u;
	uint64_t ceio4 : 3u;
	uint64_t ceio5 : 3u;
	uint64_t ceio6 : 3u;
	uint64_t ceio7 : 3u;
	uint64_t cpb0 : 3u;
	uint64_t cpe0 : 3u;
	uint64_t cpe1 : 2u;
};

/* Same for command and telemetry */
struct caniot_heating_control {
	caniot_heating_status_t heater1_cmd : 4u;
	caniot_heating_status_t heater2_cmd : 4u;
	caniot_heating_status_t heater3_cmd : 4u;
	caniot_heating_status_t heater4_cmd : 4u;
	uint8_t power_status : 1u; /* Tells whether power is detected or not, telemetry
				      only */
};

struct caniot_shutters_control {
	uint8_t shutters_openness[4u];
};

void caniot_blc0_command_init(struct caniot_blc0_command *cmd);
void caniot_blc1_command_init(struct caniot_blc1_command *cmd);
void caniot_caniot_blc_sys_command_init(struct caniot_blc_sys_command *cmd);

uint8_t caniot_caniot_blc_sys_command_to_byte(const struct caniot_blc_sys_command *cmd);
void caniot_caniot_blc_sys_command_from_byte(struct caniot_blc_sys_command *cmd,
					     uint8_t byte);

int caniot_blc0_telemetry_ser(const struct caniot_blc0_telemetry *t,
			      uint8_t *buf,
			      size_t len);
int caniot_blc0_telemetry_get(struct caniot_blc0_telemetry *t, uint8_t *buf, size_t len);

int caniot_blc0_command_ser(const struct caniot_blc0_command *t,
			    uint8_t *buf,
			    size_t len);
int caniot_blc0_command_get(struct caniot_blc0_command *t, uint8_t *buf, size_t len);

int caniot_blc1_cmd_set_xps(uint8_t *buf,
			    size_t len,
			    uint8_t n,
			    caniot_complex_digital_cmd_t xps);

caniot_complex_digital_cmd_t
caniot_blc1_cmd_parse_xps(uint8_t *buf, size_t len, uint8_t n);

int caniot_blc1_telemetry_ser(const struct caniot_blc1_telemetry *t,
			      uint8_t *buf,
			      size_t len);
int caniot_blc1_telemetry_get(struct caniot_blc1_telemetry *t, uint8_t *buf, size_t len);

int caniot_blc1_command_ser(const struct caniot_blc1_command *t,
			    uint8_t *buf,
			    size_t len);
int caniot_blc1_command_get(struct caniot_blc1_command *t, uint8_t *buf, size_t len);

/* conversion functions */


uint16_t caniot_dt_T16_to_T10(int16_t T16);


int16_t caniot_dt_T10_to_T16(uint16_t T);

/* constants */
#define CANIOT_DT_T16_INVALID ((int16_t)INT16_MAX)
#define CANIOT_DT_T10_INVALID ((uint16_t)0x3FFU)
#define CANIOT_DT_T8_INVALID  ((uint8_t)0xFFU)

#define CANIOT_DT_VALID_T16_TEMP(temp) ((temp) != CANIOT_DT_T16_INVALID)
#define CANIOT_DT_VALID_T10_TEMP(temp) ((temp) != CANIOT_DT_T10_INVALID)

#ifdef __cplusplus
}
#endif

#endif /* _CANIOT_DATATYPE_H */