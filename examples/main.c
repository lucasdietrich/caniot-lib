#include <stdio.h>
#include <stdint.h>
#include <memory.h>

#include <caniot.h>

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

	char buf[8];
	uint8_t len = sizeof(buf);
	memset(buf, 0x00, sizeof(buf));

	tempsens.dev.api->telemetry(&tempsens.dev, 0, buf, &len);

	for (int i = 0; i < len; i++) {
		printf("%hhx ", buf[i]);
	}
	printf("\n");
	
	return 0;
}