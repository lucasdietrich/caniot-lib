#ifndef _CANIOT_SIM_H
#define _CANIOT_SIM_H

#include "device.h"

int process_rx_frame(struct caniot_device *dev_list[],
		     uint8_t dev_count,
		     struct caniot_frame *frame);

#endif