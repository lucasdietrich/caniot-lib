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

bool attr_handler(struct caniot_device_attribute *attr, void *user_data)
{
	printf("key: %04hx section: %u name: %s [%s%s%s]\n",
	       attr->key,
	       attr->section,
	       attr->name,
	       attr->read ? "R" : "-",
	       attr->write ? "W" : "-",
	       attr->persistent ? "P" : "-");

	return true;
}

int main(void)
{
	struct caniot_device_attribute attr;
	int ret = caniot_attr_get_by_key(&attr, 0x1020);
	printf("caniot_attr_get_by_key: ret: %d name: %s\n", ret, attr.name);

	ret = caniot_attr_iterate(attr_handler, NULL);
	printf("caniot_attr_iterate: ret: %d\n", ret);
}