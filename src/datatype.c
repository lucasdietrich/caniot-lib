#include <caniot/datatype.h>
#include <caniot/archutils.h>

#include <stdbool.h>

static inline bool is_valid_class(uint8_t cls)
{
	return cls <= 0x7u;
}

int caniot_dt_endpoints_count(uint8_t cls)
{
	switch (cls) {
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

bool caniot_dt_valid_endpoint(uint8_t cls, uint8_t endpoint)
{
	int ret;

	ret = caniot_dt_endpoints_count(cls);
	if (ret < 0)
		return false;
	
	return endpoint < ret;
}

uint16_t caniot_dt_T16_to_T10(int16_t T16)
{
	T16 /= 10;
	T16 = MAX(MIN(T16, 720), -280);

	const uint16_t T10 = T16 + 280;

	return T10;
}

int16_t caniot_dt_T10_to_T16(uint16_t T)
{
	return ((int16_t)T * 10) - 2800;
}