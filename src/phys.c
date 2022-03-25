#include <stddef.h>

#include "phys.h"
#include "caniot_errors.h"

// void caniot_show_temperature(uint8_t t)
// {
// 	printf("Temperature: %d.%1u\n", (t - 56) / 2, (t - 56) % 2);
// }

// void caniot_show_Temperature(uint16_t T)
// {
// 	printf("Temperature: %d.%1u\n", (T - 280) / 10, (T - 280) % 10);
// }

int caniot_phys_hysteresis_init(caniot_phys_hysteresis_t *hysteresis,
				int low,
				int high)
{
	if (hysteresis == NULL) {
		return -CANIOT_EINVAL;
	}

	if (low > high) {
		return -CANIOT_PHYS_HYST_INVALID;
	}

	hysteresis->state = CANIOT_PHYS_HYSTERESIS_UNDEF;
	hysteresis->low = low;
	hysteresis->high = high;

	return 0;
}

caniot_phys_hysteresis_state_t caniot_phys_hysteresis_update(
	caniot_phys_hysteresis_t *hyst,
	int val)
{
	if (val <= hyst->low) {
		hyst->state = CANIOT_PHYS_HYSTERESIS_LOW;
	} else if (val >= hyst->high) {
		hyst->state = CANIOT_PHYS_HYSTERESIS_HIGH;
	}

	return hyst->state;
}