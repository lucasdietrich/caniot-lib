#ifndef _CANIOT_CLASSES_H_
#define _CANIOT_CLASSES_H_

#include "caniot.h"
#include "datatype.h"

#define CANIOT_CLASS0_IO_COUNT		  8u
#define CANIOT_BLC0_TELEMETRY_BUF_LEN 7
#define CANIOT_BLC0_COMMAND_BUF_LEN	  2

#define CANIOT_CLASS1_IO_COUNT		  19u
#define CANIOT_BLC1_TELEMETRY_BUF_LEN 8
#define CANIOT_BLC1_COMMAND_BUF_LEN	  7

typedef enum {
	CANIOT_TEMP_INT = 0,
	CANIOT_TEMP_EXT1,
	CANIOT_TEMP_EXT2,
	CANIOT_TEMP_EXT3,
} caniot_temp_sens_t;

typedef enum {
	CANIOT_BLC0_OC1					= 0,
	CANIOT_BLC0_OC2					= 1,
	CANIOT_BLC0_RELAY1				= 2,
	CANIOT_BLC0_RELAY2				= 3,
	CANIOT_BLC0_IN1					= 4,
	CANIOT_BLC0_IN2					= 5,
	CANIOT_BLC0_IN3					= 6,
	CANIOT_BLC0_IN4					= 7,
	CANIOT_BLC0_OC1_PULSE_ACTIVE	= 8,
	CANIOT_BLC0_OC2_PULSE_ACTIVE	= 9,
	CANIOT_BLC0_RELAY1_PULSE_ACTIVE = 10,
	CANIOT_BLC0_RELAY2_PULSE_ACTIVE = 11,
} caniot_blc0_io_t;

struct caniot_blc0_telemetry {
	uint8_t dio;	  // Digital Input/Output
	uint8_t pdio : 4; // Pulse active on digital output
	uint16_t int_temperature : 10;
	uint16_t ext_temperature : 10;
	uint16_t ext_temperature2 : 10;
	uint16_t ext_temperature3 : 10;
};

/** Set all fields to default values.
 *
 * @param t The telemetry structure to initialize.
 * @return 0 on success, negative error code on failure.
 */
int caniot_blc0_telemetry_defaults(struct caniot_blc0_telemetry *t);

/**
 * Get the temperature from the telemetry structure.
 *
 * @param t The telemetry structure.
 * @param sensor The sensor index (0-3).
 * @param temperature Pointer to store the temperature value.
 * @return 0 on valid temperature, 1 if temperature is invalid, negative error code on
 * failure.
 */
int caniot_blc0_telemetry_get_temperature(const struct caniot_blc0_telemetry *t,
										  caniot_temp_sens_t sensor,
										  uint16_t *temperature);

/**
 * Set the temperature in the telemetry structure.
 *
 * @param t The telemetry structure.
 * @param sensor The sensor index (0-3).
 * @param temperature The temperature value to set.
 * @return 0 on success, negative error code on failure.
 */
int caniot_blc0_telemetry_set_temperature(struct caniot_blc0_telemetry *t,
										  caniot_temp_sens_t sensor,
										  uint16_t temperature);

/**
 * Clear the temperature in the telemetry structure (set to invalid).
 *
 * @param t The telemetry structure.
 * @param sensor The sensor index (0-3).
 * @return 0 on success, negative error code on failure.
 */
int caniot_blc0_telemetry_clear_temperature(struct caniot_blc0_telemetry *t,
											caniot_temp_sens_t sensor);

/**
 * Get the state of a digital IO.
 *
 * @param t The telemetry structure.
 * @param io The IO index (0-11).
 * @param state Pointer to store the state (true for high, false for low).
 * @return 0 on success, negative error code on failure.
 */
int caniot_blc0_telemetry_get_io(const struct caniot_blc0_telemetry *t,
								 caniot_blc0_io_t io,
								 bool *state);

/**
 * Set the state of a digital IO.
 *
 * @param t The telemetry structure.
 * @param io The IO index (0-11).
 * @param state The state to set (true for high, false for low).
 * @return 0 on success, negative error code on failure.
 */
int caniot_blc0_telemetry_set_io(struct caniot_blc0_telemetry *t,
								 caniot_blc0_io_t io,
								 bool state);

/**
 * Serialize the telemetry structure.
 *
 * @param t The telemetry structure.
 * @param buf The buffer to serialize into.
 * @param len Pointer to the length of the buffer.
 * @return 0 on success, negative error code on failure.
 */
int caniot_blc0_telemetry_ser(const struct caniot_blc0_telemetry *t,
							  uint8_t *buf,
							  uint8_t *len);

/**
 * Deserialize the telemetry structure.
 *
 * @param t The telemetry structure to populate.
 * @param buf The buffer to deserialize from.
 * @param len The length of the buffer.
 * @return 0 on success, negative error code on failure.
 */
int caniot_blc0_telemetry_get(struct caniot_blc0_telemetry *t,
							  const uint8_t *buf,
							  uint8_t len);

/* Board level control (blc) command */
struct caniot_blc0_command {
	caniot_complex_digital_cmd_t coc1 : 3u;
	caniot_complex_digital_cmd_t coc2 : 3u;
	caniot_complex_digital_cmd_t crl1 : 3u;
	caniot_complex_digital_cmd_t crl2 : 3u;
};

/** Set all fields to default values.
 *
 * @param c The command structure to initialize.
 * @return 0 on success, negative error code on failure.
 */
int caniot_blc0_command_defaults(struct caniot_blc0_command *c);

/**
 * Set the extended pulse state for a specific IO.
 *
 * @param cmd The command structure.
 * @param io The IO index (0-3).
 * @param xps The extended pulse state to set.
 * @return 0 on success, negative error code on failure.
 */
int caniot_blc0_command_set_xps(struct caniot_blc0_command *cmd,
								caniot_blc0_io_t io,
								caniot_complex_digital_cmd_t xps);

/**
 * Get the extended pulse state for a specific IO.
 * @param cmd The command structure.
 * @param io The IO index (0-3).
 * @param xps Pointer to store the extended pulse state.
 * @return 0 on success, negative error code on failure.
 */
int caniot_blc0_command_get_xps(const struct caniot_blc0_command *cmd,
								caniot_blc0_io_t io,
								caniot_complex_digital_cmd_t *xps);

/**
 * Serialize the command structure.
 *
 * @param t The command structure.
 * @param buf The buffer to serialize into.
 * @param len Pointer to the length of the buffer.
 * @return 0 on success, negative error code on failure.
 */
int caniot_blc0_command_ser(const struct caniot_blc0_command *t,
							uint8_t *buf,
							uint8_t *len);

/**
 * Deserialize the command structure.
 * @param t The command structure to populate.
 * @param buf The buffer to deserialize from.
 * @param len The length of the buffer.
 * @return 0 on success, negative error code on failure.
 */
int caniot_blc0_command_get(struct caniot_blc0_command *t,
							const uint8_t *buf,
							uint8_t len);

struct caniot_blc1_telemetry {
	uint8_t pcpd;	 /* Represent first 1-8 IOs */
	uint8_t eio;	 /* Represent IOs 8-15 */
	uint8_t pb0 : 1; /* Represent IOs 17 */
	uint8_t pe0 : 1; /* Represent IOs 18 */
	uint8_t pe1 : 1; /* Represent IOs 19 */
	uint32_t int_temperature : 10;
	uint32_t ext_temperature : 10;
	uint32_t ext_temperature2 : 10;
	uint32_t ext_temperature3 : 10;
};

int caniot_blc1_telemetry_defaults(struct caniot_blc1_telemetry *t);

int caniot_blc1_telemetry_ser(const struct caniot_blc1_telemetry *t,
							  uint8_t *buf,
							  uint8_t *len);
int caniot_blc1_telemetry_get(struct caniot_blc1_telemetry *t,
							  const uint8_t *buf,
							  uint8_t len);

struct caniot_blc1_command {
	caniot_complex_digital_cmd_t gpio_commands[CANIOT_CLASS1_IO_COUNT];
};

int caniot_blc1_command_defaults(struct caniot_blc1_command *c);

int caniot_blc1_cmd_buf_set_xps(caniot_complex_digital_cmd_t xps,
								uint8_t *buf,
								uint8_t len,
								uint8_t n);

int caniot_blc1_cmd_buf_parse_xps(caniot_complex_digital_cmd_t *xps,
								  const uint8_t *buf,
								  uint8_t len,
								  uint8_t n);

int caniot_blc1_command_ser(const struct caniot_blc1_command *t,
							uint8_t *buf,
							uint8_t *len);
int caniot_blc1_command_get(struct caniot_blc1_command *t,
							const uint8_t *buf,
							uint8_t len);

#endif /* _CANIOT_CLASSES_H_ */