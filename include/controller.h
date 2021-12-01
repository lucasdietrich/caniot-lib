#ifndef _CANIOT_CONTROLLER_H
#define _CANIOT_CONTROLLER_H

#include "caniot.h"

typedef int (*caniot_query_callback_t)(union deviceid did,
				       struct caniot_frame *resp);

// intitalize controller structure
int caniot_controller_init(struct caniot_controller *controller);

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
			  struct caniot_attribute *attr,
			  caniot_query_callback_t cb,
			  int32_t timeout);

int caniot_write_attribute(struct caniot_controller *ctrl,
			   union deviceid did,
			   struct caniot_attribute *attr,
			   caniot_query_callback_t cb,
			   int32_t timeout);

int caniot_discover(struct caniot_controller *ctrl,
		    caniot_query_callback_t cb,
		    int32_t timeout);

#endif /* _CANIOT_CONTROLLER_H */