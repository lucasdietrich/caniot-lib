#ifndef _CANIOT_CONTROLLER_H
#define _CANIOT_CONTROLLER_H

#include "caniot.h"

struct caniot_telemetry_entry {
	union deviceid did;
	uint32_t timestamp;
	uint8_t ep;
	uint8_t buf[8];
};

struct caniot_device_entry
{
	uint32_t last_seen;	/* timestamp this device was last seen */

	/* move to a memslab */
	struct {
		struct caniot_controller *controller;
		caniot_query_callback_t callback; /* query callback if not NULL */
	} query;

	struct {
		uint8_t active : 1;	/* valid entry if set */
	} flags;
};

struct caniot_telemetry_database_api
{
	int (*init)(void);
	int (*deinit)(void);

	int (*append)(struct caniot_telemetry_entry *entry);

	int (*get_first)(union deviceid did);
	int (*get_next)(union deviceid did,
			struct caniot_telemetry_entry *entry);
	int (*get_last)(union deviceid did);
	int (*count)(union deviceid did);
};

struct caniot_controller_config
{
	uint16_t store_telemetry;	/* store telemetry data in database */
};

struct caniot_controller {
	char name[32];
	uint32_t uid;

	struct caniot_device_entry devices[CANIOT_MAX_DEVICES];
	const struct caniot_telemetry_database_api *telemetry_db;
	const struct caniot_drivers_api *driv;
	const struct caniot_controller_config *cfg;
};

// intitalize controller structure
int caniot_controller_init(struct caniot_controller *controller);

int caniot_controller_deinit(struct caniot_controller *ctrl);

int caniot_send_command(struct caniot_controller *ctrl,
			union deviceid did,
			struct caniot_command *cmd,
			caniot_query_callback_t cb,
			int32_t timeout);

int caniot_request_telemetry(struct caniot_controller *ctrl,
			     union deviceid did,
			     uint8_t ep,
			     caniot_query_callback_t cb,
			     int32_t timeout);

int caniot_read_attribute(struct caniot_controller *ctrl,
			  union deviceid did,
			  uint16_t key,
			  caniot_query_callback_t cb,
			  int32_t timeout);

int caniot_write_attribute(struct caniot_controller *ctrl,
			   union deviceid did,
			   uint16_t key,
			   uint32_t value,
			   caniot_query_callback_t cb,
			   int32_t timeout);

int caniot_discover(struct caniot_controller *ctrl,
		    caniot_query_callback_t cb,
		    int32_t timeout);

int caniot_controller_handle_rx_frame(struct caniot_controller *ctrl,
				      struct caniot_frame *frame);

#endif /* _CANIOT_CONTROLLER_H */