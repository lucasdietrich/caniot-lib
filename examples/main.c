#include <stdio.h>
#include <stdint.h>
#include <string.h>

// #include <caniot.h>
#include "tempsens.h"

#include "helper.h"


int main(void)
{
	int ret;
	struct caniot_frame req, resp;

	const uint8_t command[8] = {11, 22, 33, 44, 55, 66, 77, 88};

	// caniot_build_query_telemetry(&req, tempsens.dev.identification->did, endpoint_app);
	caniot_build_query_telemetry(&req, CANIOT_DEVICE_BROADCAST, endpoint_app);
	caniot_explain_frame(&req);

	if (caniot_device_is_target(tempsens.dev.identification->did, &req)) {
		ret = caniot_device_handle_rx_frame(&tempsens.dev, &req, &resp);
		caniot_explain_frame(&resp);
		printf(F("ret = 0x%x\n"), -ret);
	} else {
		printf(F("not for me\n"));
	}


	caniot_build_query_command(&req, tempsens.dev.identification->did, endpoint_broadcast, command, 8);
	caniot_explain_frame(&req);

	if (caniot_device_is_target(tempsens.dev.identification->did, &req)) {
		ret = caniot_device_handle_rx_frame(&tempsens.dev, &req, &resp);
		caniot_explain_frame(&resp);
		printf(F("ret = 0x%x\n"), -ret);
	} else {
		printf(F("not for me\n"));
	}
}