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
	caniot_complex_digital_cmd_t coc1 : 3u;
	caniot_complex_digital_cmd_t coc2 : 3u;
	caniot_complex_digital_cmd_t crl1 : 3u;
	caniot_complex_digital_cmd_t crl2 : 3u;
};

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

struct caniot_blc1_command {
	caniot_complex_digital_cmd_t gpio_commands[CANIOT_CLASS1_IO_COUNT];
};

void caniot_blc0_command_init(struct caniot_blc0_command *cmd);
void caniot_blc1_command_init(struct caniot_blc1_command *cmd);

int caniot_blc0_telemetry_ser(const struct caniot_blc0_telemetry *t,
							  uint8_t *buf,
							  uint8_t *len);
int caniot_blc0_telemetry_get(struct caniot_blc0_telemetry *t,
							  const uint8_t *buf,
							  uint8_t len);

int caniot_blc0_command_ser(const struct caniot_blc0_command *t,
							uint8_t *buf,
							uint8_t *len);
int caniot_blc0_command_get(struct caniot_blc0_command *t,
							const uint8_t *buf,
							uint8_t len);

int caniot_blc1_cmd_set_xps(caniot_complex_digital_cmd_t xps,
							uint8_t *buf,
							uint8_t len,
							uint8_t n);

int caniot_blc1_cmd_parse_xps(caniot_complex_digital_cmd_t *xps,
							  const uint8_t *buf,
							  uint8_t len,
							  uint8_t n);

int caniot_blc1_telemetry_ser(const struct caniot_blc1_telemetry *t,
							  uint8_t *buf,
							  uint8_t *len);
int caniot_blc1_telemetry_get(struct caniot_blc1_telemetry *t,
							  const uint8_t *buf,
							  uint8_t len);

int caniot_blc1_command_ser(const struct caniot_blc1_command *t,
							uint8_t *buf,
							uint8_t *len);
int caniot_blc1_command_get(struct caniot_blc1_command *t,
							const uint8_t *buf,
							uint8_t len);

#endif /* _CANIOT_CLASSES_H_ */