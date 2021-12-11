#include <stdio.h>
#include <stdint.h>
#include <memory.h>

// #include <caniot.h>
#include "tempsens.h"

#include "helper.h"


int main(void)
{
	int ret;
	struct caniot_frame req, resp;

	caniot_build_query_telemtry(&req, tempsens.dev.identification->did);

	caniot_explain_frame(&req);

	ret = caniot_device_handle_rx_frame(&tempsens.dev, &req, &resp);

	caniot_explain_frame(&resp);

	printf("ret = %d\n", ret);
}