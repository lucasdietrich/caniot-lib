/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <caniot/caniot.h>
#include <caniot/caniot_private.h>
#include <caniot/controller.h>
#include <caniot/device.h>

int main(void)
{
	struct caniot_device_attribute attr;
	int ret = caniot_attr_get_by_key(&attr, 0x1020);
	printf("ret: %d name: %s\n", ret, attr.name);
}