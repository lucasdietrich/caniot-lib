#include "caniot.h"

bool caniot_validate_drivers_api(struct caniot_drivers_api *api)
{
	return api->entropy && api->get_time && 
		api->send && api->recv;
}