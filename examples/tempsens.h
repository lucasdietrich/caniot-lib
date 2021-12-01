#ifndef _TEMPTSENS_H_
#define _TEMPTSENS_H_

#include <device.h>

#define TEMPDEV_OF(p_dev) CONTAINER_OF(p_dev, struct tempdev, dev)

struct tempdev {
	uint32_t trigger_temp;
	uint32_t measured_temp;
        
	struct caniot_device dev;
};

extern struct tempdev tempsens;

#endif