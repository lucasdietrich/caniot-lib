#include "caniot.h"

bool caniot_valid_drivers_api(struct caniot_drivers_api *api)
{
	return api->rom_read && api->persistent_read && api->persistent_write &&
		api->entropy && api->get_time && api->schedule && api->unschedule &&
		api->send && api->recv && api->set_filter && api->set_mask;
}