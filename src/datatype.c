#include "datatype.h"

#include <stdbool.h>

static inline bool is_valid_class(uint8_t class)
{
	return class <= 0x7u;
}

int caniot_dt_endpoints_counts(uint8_t class)
{
	switch (class) {
	case 0:
		return 1;
	case 1:
		return -CANIOT_ENIMPL;
	case 2:
		return -CANIOT_ENIMPL;
	case 3:
		return -CANIOT_ENIMPL;
	case 4:
		return -CANIOT_ENIMPL;
	case 5:
		return -CANIOT_ENIMPL;
	case 6:
		return -CANIOT_ENIMPL;
	case 7:
		return -CANIOT_ENIMPL;
	default:
		return -CANIOT_ECLASS;
	}
}

bool caniot_dt_valid_endpoint(uint8_t class, uint8_t endpoint)
{
	int ret;

	ret = caniot_class_endpoints_count(class);
	if (ret < 0)
		return false;
	
	return endpoint < ret;
}