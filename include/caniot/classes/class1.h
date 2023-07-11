/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DEV_CANIOT_CLASS1_H_
#define _DEV_CANIOT_CLASS1_H_

#include <stdint.h>

#include <caniot/caniot.h>
#include <caniot/datatype.h>
#include <caniot/device.h>

#define CANIOT_CLASS1_IO_COUNT 19u

#define PC0_IDX 0u
#define PC1_IDX 1u
#define PC2_IDX 2u
#define PC3_IDX 3u
#define PD4_IDX 4u
#define PD5_IDX 5u
#define PD6_IDX 6u
#define PD7_IDX 7u

#define EIO0_IDX 8u
#define EIO1_IDX 9u
#define EIO2_IDX 10u
#define EIO3_IDX 11u
#define EIO4_IDX 12u
#define EIO5_IDX 13u
#define EIO6_IDX 14u
#define EIO7_IDX 15u

#define PB0_IDX 16u
#define PE0_IDX 17u
#define PE1_IDX 18u

int caniot_cmd_blc1_init(struct caniot_blc1_command *cmd);

/**
 * @brief Set the XPS command for given pin
 *
 * @param cmd
 * @param n
 * @param xps
 * @return int
 */
int caniot_cmd_blc1_set_xps(struct caniot_blc1_command *cmd,
							uint8_t n,
							caniot_complex_digital_cmd_t xps);

int caniot_cmd_blc1_clear(struct caniot_blc1_command *cmd);

caniot_complex_digital_cmd_t caniot_cmd_blc1_parse_xps(struct caniot_blc1_command *cmd,
													   uint8_t n);

#endif /* _DEV_CANIOT_CLASS1_H_ */