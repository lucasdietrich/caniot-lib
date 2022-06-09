#ifndef _CANIOT_PHYS_H_
#define _CANIOT_PHYS_H_

#include <stdint.h>

#include "datatype.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef  struct {
	caniot_phys_hysteresis_state_t state;
	int low;
	int high;
} caniot_phys_hysteresis_t;

int caniot_phys_hysteresis_init(caniot_phys_hysteresis_t *hysteresis,
				int low,
				int high);

/**
 * @brief 
 * 
 * @param hyst 
 * @param val 
 * @return true 
 * @return false 
 */
caniot_phys_hysteresis_state_t caniot_phys_hysteresis_update(
	caniot_phys_hysteresis_t *hyst, 
	int val);

void caniot_show_temperature(uint8_t t);

void caniot_show_Temperature(uint16_t T);

#ifdef __cplusplus
}
#endif

#endif