#ifndef _HEADER_H
#define _HEADER_H

#define CONTROLLERS_COUNT 1U
#define DEVICES_COUNT 8U

#include <caniot/caniot.h>

void init_controllers(void);
void init_devices(void);
void init_canbus(void);

void controllers_process(const struct caniot_frame *frame);
void devices_process(const struct caniot_frame *frame);

int can_send(const struct caniot_frame *frame, uint32_t delay_ms);
int can_recv(struct caniot_frame *frame);


#endif 