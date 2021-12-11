#ifndef _CANIOT_HELPER_H_
#define _CANIOT_HELPER_H_

#include "caniot.h"

void caniot_show_frame(struct caniot_frame *frame);

void caniot_explain_id(union caniot_id id);

void caniot_explain_frame(struct caniot_frame *frame);

void caniot_build_query_telemtry(struct caniot_frame *frame, union deviceid did);


#endif /* _CANIOT_HELPER_H_ */