#ifndef _HEADER_H
#define _HEADER_H

#define CONTROLLERS_COUNT 1U
#define DEVICES_COUNT 8U

#define VTIME_INC_CONST_VAL 100U

#include <caniot/caniot.h>

void init_controllers(void);
void init_devices(void);

void controllers_process(const struct caniot_frame *frame, uint32_t time_passed);
void devices_process(const struct caniot_frame *frame);

int ctrl_Q(uint32_t ctrlid,
	   caniot_did_t did,
	   struct caniot_frame *frame,
	   uint32_t timeout);
int ctrl_C(uint32_t ctrlid, uint8_t handle, bool suppress);

int can_send(const struct caniot_frame *frame, uint32_t delay_ms);
int can_recv(struct caniot_frame *frame);

void get_time(uint32_t *sec, uint16_t *ms);
void vtime_get(uint32_t *sec, uint16_t *ms);
void vtime_inc(uint32_t inc_ms);
static inline void vtime_inc_const(void) { vtime_inc(VTIME_INC_CONST_VAL); }


#endif 