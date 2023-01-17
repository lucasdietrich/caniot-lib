#include <stddef.h>

#include <caniot/errors.h>
#include <caniot/phys.h>

int caniot_phys_hysteresis_init(caniot_phys_hysteresis_t *hysteresis, int low, int high)
{
	if (hysteresis == NULL) {
		return -CANIOT_EINVAL;
	}

	if (low > high) {
		return -CANIOT_EHYST;
	}

	hysteresis->state = CANIOT_PHYS_HYSTERESIS_UNDEF;
	hysteresis->low	  = low;
	hysteresis->high  = high;

	return 0;
}

caniot_phys_hysteresis_state_t
caniot_phys_hysteresis_update(caniot_phys_hysteresis_t *hyst, int val)
{
	if (val <= hyst->low) {
		hyst->state = CANIOT_PHYS_HYSTERESIS_LOW;
	} else if (val >= hyst->high) {
		hyst->state = CANIOT_PHYS_HYSTERESIS_HIGH;
	}

	return hyst->state;
}