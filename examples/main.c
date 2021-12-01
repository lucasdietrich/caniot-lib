#include <stdio.h>
#include <stdint.h>
#include <memory.h>

#include <device.h>

// #include <caniot.h>
#include "tempsens.h"

int main(void)
{
	struct caniot_id m = {
		.type = 3,
		.query = 1,
		.cls = 7,
		.dev = 7,
		.endpoint = 3
	};

	printf("sizeof(m) = %lu : %hu", sizeof(m), *((uint16_t*)&m));

	return 0;
}