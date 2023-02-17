/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <caniot/caniot_private.h>
#include <caniot/datatype.h>

_Static_assert(sizeof(struct caniot_blc_command) == 8u, "Invalid size");

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
	if (ret < 0) return false;

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

void caniot_blc_command_init(struct caniot_blc_command *cmd)
{
	ASSERT(cmd != NULL);

	memset(cmd, 0, sizeof(*cmd));
}

void caniot_blc0_command_init(struct caniot_blc0_command *cmd)
{
	ASSERT(cmd != NULL);

	memset(cmd, 0x00U, sizeof(struct caniot_blc0_command));
}

void caniot_blc1_command_init(struct caniot_blc1_command *cmd)
{
	ASSERT(cmd != NULL);

	memset(cmd, 0x00U, sizeof(struct caniot_blc1_command));
}

void caniot_blc_sys_req_reboot(struct caniot_blc_sys_command *sysc)
{
	ASSERT(sysc != NULL);

	sysc->reset = 1u;
}

void caniot_blc_sys_req_factory_reset(struct caniot_blc_sys_command *sysc)
{
	ASSERT(sysc != NULL);

	sysc->config_reset = 1u;
}