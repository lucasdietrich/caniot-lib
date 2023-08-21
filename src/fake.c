/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <caniot/datatype.h>
#include <caniot/fake.h>

#define SCALE(x, a, b, c, d) (c + ((x) - (a)) * ((d) - (c)) / ((b) - (a)))

uint16_t caniot_fake_get_temp(struct caniot_device *dev)
{
#if CONFIG_CANIOT_DEVICE_DRIVERS_API
	uint8_t buf;
	dev->driv->entropy(&buf, 1U);
#else
	(void)dev;
	static uint8_t buf = 0U;
	buf++;
#endif

	int16_t temp = SCALE(buf, 0U, 255U, -1000LU, 4500LU);

	return caniot_dt_T16_to_T10(temp);
}