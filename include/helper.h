#ifndef _CANIOT_HELPER_H_
#define _CANIOT_HELPER_H_

#include "caniot.h"

void caniot_show_frame(struct caniot_frame *frame);

void caniot_explain_id(union caniot_id id);

void caniot_explain_frame(struct caniot_frame *frame);

void caniot_build_query_telemtry(struct caniot_frame *frame,
				 union deviceid did,
				 uint8_t endpoint);

void caniot_build_query_command(struct caniot_frame *frame,
				union deviceid did,
				uint8_t endpoint,
				const uint8_t *buf,
				uint8_t size);

#endif /* _CANIOT_HELPER_H_ */