/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <caniot/caniot_private.h>
#include <caniot/datatype.h>

static inline bool is_valid_class(uint8_t cls)
{
	return cls <= 0x7u;
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

void caniot_caniot_blc_sys_command_init(struct caniot_blc_sys_command *cmd)
{
	ASSERT(cmd != NULL);

	memset(cmd, 0x00U, sizeof(struct caniot_blc_sys_command));
}

#define BLC_SYS_RESET_OFFSET		  0u
#define BLC_SYS_WATCHDOG_RESET_OFFSET 1u
#define BLC_SYS_SOFTWARE_RESET_OFFSET 2u
#define BLC_SYS_WATCHDOG_OFFSET		  3u
#define BLC_SYS_CONFIG_RESET_OFFSET	  5u
#define BLC_SYS_INHIBIT_OFFSET		  6u

uint8_t caniot_blc_sys_command_to_byte(const struct caniot_blc_sys_command *bf)
{
	uint8_t byte = 0x00u;

	if (bf) {
		byte |= bf->reset << BLC_SYS_RESET_OFFSET;

		/* Deprecated */
		byte |= bf->_software_reset << BLC_SYS_WATCHDOG_RESET_OFFSET;
		byte |= bf->_watchdog_reset << BLC_SYS_SOFTWARE_RESET_OFFSET;

		byte |= bf->watchdog << BLC_SYS_WATCHDOG_OFFSET;
		byte |= bf->config_reset << BLC_SYS_CONFIG_RESET_OFFSET;
		byte |= bf->inhibit << BLC_SYS_INHIBIT_OFFSET;
	}

	return byte;
}

void caniot_blc_sys_command_from_byte(struct caniot_blc_sys_command *cmd, uint8_t byte)
{
	ASSERT(cmd != NULL);

	cmd->reset = (byte >> BLC_SYS_RESET_OFFSET) & 0x01u;

	/* Deprecated */
	cmd->_software_reset = (byte >> BLC_SYS_WATCHDOG_RESET_OFFSET) & 0x01u;
	cmd->_watchdog_reset = (byte >> BLC_SYS_SOFTWARE_RESET_OFFSET) & 0x01u;

	cmd->watchdog	  = (byte >> BLC_SYS_WATCHDOG_OFFSET) & 0x03u;
	cmd->config_reset = (byte >> BLC_SYS_CONFIG_RESET_OFFSET) & 0x01u;
	cmd->inhibit	  = (byte >> BLC_SYS_INHIBIT_OFFSET) & 0x03u;
}