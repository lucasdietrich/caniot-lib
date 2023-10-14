/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <caniot/caniot_private.h>
#include <caniot/classes/class0.h>
#include <caniot/classes/class1.h>
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

void caniot_caniot_blc_sys_command_init(struct caniot_blc_sys_command *cmd)
{
	ASSERT(cmd != NULL);

	memset(cmd, 0x00U, sizeof(struct caniot_blc_sys_command));
}

uint8_t caniot_caniot_blc_sys_command_to_byte(const struct caniot_blc_sys_command *bf)
{
	uint8_t byte = 0x00u;

	if (bf) {
		byte |= bf->reset << 0;
		byte |= bf->software_reset << 1;
		byte |= bf->watchdog_reset << 2;
		byte |= bf->watchdog << 3;
		byte |= bf->config_reset << 5;
	}

	return byte;
}

void caniot_caniot_blc_sys_command_from_byte(struct caniot_blc_sys_command *cmd,
											 uint8_t byte)
{
	ASSERT(cmd != NULL);

	cmd->reset			= (byte >> 0) & 0x01u;
	cmd->software_reset = (byte >> 1) & 0x01u;
	cmd->watchdog_reset = (byte >> 2) & 0x01u;
	cmd->watchdog		= (byte >> 3) & 0x03u;
	cmd->config_reset	= (byte >> 5) & 0x01u;
}

int caniot_blc0_telemetry_ser(const struct caniot_blc0_telemetry *t,
							  uint8_t *buf,
							  size_t len)
{
	if (!t || !buf || len < CANIOT_BLC0_TELEMETRY_BUF_LEN) return -CANIOT_EINVAL;

	buf[0] = t->dio;
	buf[1] = t->pdio;
	buf[2] = (t->int_temperature) & 0xffu;
	buf[3] = (t->int_temperature >> 8) & 0x03u;

	buf[3] |= (t->ext_temperature & 0x3fu) << 2;
	buf[4] = (t->ext_temperature >> 6) & 0xfu;

	buf[4] |= (t->ext_temperature2 & 0x0fu) << 4;
	buf[5] = (t->ext_temperature2 >> 4) & 0x3fu;

	buf[5] |= (t->ext_temperature3 & 0x03u) << 6;
	buf[6] = (t->ext_temperature3 >> 2) & 0xffu;

	return CANIOT_BLC0_TELEMETRY_BUF_LEN;
}

int caniot_blc0_telemetry_get(struct caniot_blc0_telemetry *t, uint8_t *buf, size_t len)
{
	if (!t || !buf || len < CANIOT_BLC0_TELEMETRY_BUF_LEN) return -CANIOT_EINVAL;

	t->dio				= buf[0];
	t->pdio				= buf[1] & 0x0Fu;
	t->int_temperature	= (buf[2]) | ((buf[3] & 0x03u) << 8);
	t->ext_temperature	= ((buf[3] >> 2) & 0x3Fu) | ((buf[4] & 0x0Fu) << 6);
	t->ext_temperature2 = ((buf[4] >> 4) & 0x0Fu) | ((buf[5] & 0x3Fu) << 4);
	t->ext_temperature3 = ((buf[5] >> 6) & 0x03u) | ((buf[6]) << 2);

	return 0;
}

int caniot_blc0_command_ser(const struct caniot_blc0_command *t, uint8_t *buf, size_t len)
{
	if (!t || !buf || len < CANIOT_BLC0_COMMAND_BUF_LEN) return -CANIOT_EINVAL;

	buf[0] = t->coc1 | (t->coc2 << 3) | ((t->crl1 & 0x03u) << 6);
	buf[1] = (t->crl1 >> 2) | (t->crl2 << 1);

	return CANIOT_BLC0_COMMAND_BUF_LEN;
}

int caniot_blc0_command_get(struct caniot_blc0_command *t, uint8_t *buf, size_t len)
{
	if (!t || !buf || len < CANIOT_BLC0_COMMAND_BUF_LEN) return -CANIOT_EINVAL;

	t->coc1 = buf[0] & 0x07u;
	t->coc2 = (buf[0] >> 3) & 0x07u;
	t->crl1 = ((buf[0] >> 6) & 0x01u) | ((buf[1] & 0x03u) << 1);
	t->crl2 = (buf[1] >> 2) & 0x07u;

	return 0;
}

static void z_blc1_cmd_set_xps(uint8_t *buf, uint8_t n, caniot_complex_digital_cmd_t xps)
{
	const uint8_t msb_index	   = n * 3u;
	const uint8_t msb_offset   = msb_index & 0x7u;
	const uint8_t msb_rem_size = 8u - msb_offset;
	const uint8_t byte_n	   = msb_index >> 3u;

	buf[byte_n] |= ((xps & 0x7u) << msb_offset) & 0xffu;

	if ((msb_rem_size < 3u) && ((byte_n + 1u) <= 6u)) {
		buf[byte_n + 1u] |= ((xps & 0x7u) >> msb_rem_size);
	}
}

static caniot_complex_digital_cmd_t z_blc1_cmd_parse_xps(uint8_t *buf, uint8_t n)
{
	caniot_complex_digital_cmd_t xps = CANIOT_XPS_NONE;

	const uint8_t msb_index	   = n * 3u;
	const uint8_t msb_offset   = msb_index & 0x7u;
	const uint8_t msb_rem_size = 8u - msb_offset;
	const uint8_t byte_n	   = msb_index >> 3u;

	xps = (buf[byte_n] >> msb_offset) & 0x7u;

	if ((msb_rem_size < 3u) && ((byte_n + 1u) <= 6u)) {
		xps |= (buf[byte_n + 1u] << msb_rem_size) & 0x7u;
	}

	return xps;
}

int caniot_blc1_cmd_set_xps(uint8_t *buf,
							size_t len,
							uint8_t n,
							caniot_complex_digital_cmd_t xps)
{
	if (!buf || n >= CANIOT_CLASS1_IO_COUNT || len >= CANIOT_BLC1_COMMAND_BUF_LEN)
		return -CANIOT_EINVAL;

	z_blc1_cmd_set_xps(buf, n, xps);

	return 0;
}

caniot_complex_digital_cmd_t
caniot_blc1_cmd_parse_xps(uint8_t *buf, size_t len, uint8_t n)
{
	if (!buf || n >= CANIOT_CLASS1_IO_COUNT || len >= CANIOT_BLC1_COMMAND_BUF_LEN)
		return -CANIOT_EINVAL;

	return z_blc1_cmd_parse_xps(buf, n);
}

int caniot_blc1_telemetry_ser(const struct caniot_blc1_telemetry *t,
							  uint8_t *buf,
							  size_t len)
{
	if (!t || !buf || len < CANIOT_BLC1_TELEMETRY_BUF_LEN) return -CANIOT_EINVAL;

	buf[0] = t->pcpd;
	buf[1] = t->eio;
	buf[2] = t->pb0 | (t->pe0 << 1) | (t->pe1 << 2);

	buf[3] = t->int_temperature & 0xffu;
	buf[4] = (t->int_temperature >> 8) & 0x03u;

	buf[4] |= (t->ext_temperature & 0x3fu) << 2;
	buf[5] = (t->ext_temperature >> 6) & 0xfu;

	buf[5] |= (t->ext_temperature2 & 0x0fu) << 4;
	buf[6] = (t->ext_temperature2 >> 4) & 0x3fu;

	buf[6] |= (t->ext_temperature3 & 0x03u) << 6;
	buf[7] = (t->ext_temperature3 >> 2) & 0xffu;

	return CANIOT_BLC1_TELEMETRY_BUF_LEN;
}

int caniot_blc1_telemetry_get(struct caniot_blc1_telemetry *t, uint8_t *buf, size_t len)
{
	if (!t || !buf || len < CANIOT_BLC1_TELEMETRY_BUF_LEN) return -CANIOT_EINVAL;

	t->pcpd = buf[0];
	t->eio	= buf[1];
	t->pb0	= buf[2] & 0x01u;
	t->pe0	= (buf[2] >> 1) & 0x01u;
	t->pe1	= (buf[2] >> 2) & 0x01u;

	t->int_temperature	= (buf[3]) | ((buf[4] & 0x03u) << 8);
	t->ext_temperature	= ((buf[4] >> 2) & 0x3fu) | ((buf[5] & 0x0fu) << 6);
	t->ext_temperature2 = ((buf[5] >> 4) & 0x0fu) | ((buf[6] & 0x3fu) << 4);
	t->ext_temperature3 = ((buf[6] >> 6) & 0x03u) | ((buf[7]) << 2);

	return 0;
}

int caniot_blc1_command_ser(const struct caniot_blc1_command *t, uint8_t *buf, size_t len)
{
	if (!t || !buf || len < CANIOT_BLC1_COMMAND_BUF_LEN) return -CANIOT_EINVAL;

	z_blc1_cmd_set_xps(buf, PC0_IDX, t->cpc0);
	z_blc1_cmd_set_xps(buf, PC1_IDX, t->cpc1);
	z_blc1_cmd_set_xps(buf, PC2_IDX, t->cpc2);
	z_blc1_cmd_set_xps(buf, PC3_IDX, t->cpc3);
	z_blc1_cmd_set_xps(buf, PD4_IDX, t->cpd0);
	z_blc1_cmd_set_xps(buf, PD5_IDX, t->cpd1);
	z_blc1_cmd_set_xps(buf, PD6_IDX, t->cpd2);
	z_blc1_cmd_set_xps(buf, PD7_IDX, t->cpd3);
	z_blc1_cmd_set_xps(buf, EIO0_IDX, t->ceio0);
	z_blc1_cmd_set_xps(buf, EIO1_IDX, t->ceio1);
	z_blc1_cmd_set_xps(buf, EIO2_IDX, t->ceio2);
	z_blc1_cmd_set_xps(buf, EIO3_IDX, t->ceio3);
	z_blc1_cmd_set_xps(buf, EIO4_IDX, t->ceio4);
	z_blc1_cmd_set_xps(buf, EIO5_IDX, t->ceio5);
	z_blc1_cmd_set_xps(buf, EIO6_IDX, t->ceio6);
	z_blc1_cmd_set_xps(buf, EIO7_IDX, t->ceio7);
	z_blc1_cmd_set_xps(buf, PB0_IDX, t->cpb0);
	z_blc1_cmd_set_xps(buf, PE0_IDX, t->cpe0);
	z_blc1_cmd_set_xps(buf, PE1_IDX, t->cpe1);

	return CANIOT_BLC1_COMMAND_BUF_LEN;
}

int caniot_blc1_command_get(struct caniot_blc1_command *t, uint8_t *buf, size_t len)
{
	if (!t || !buf || len < CANIOT_BLC1_COMMAND_BUF_LEN) return -CANIOT_EINVAL;

	t->cpc0	 = z_blc1_cmd_parse_xps(buf, PC0_IDX);
	t->cpc1	 = z_blc1_cmd_parse_xps(buf, PC1_IDX);
	t->cpc2	 = z_blc1_cmd_parse_xps(buf, PC2_IDX);
	t->cpc3	 = z_blc1_cmd_parse_xps(buf, PC3_IDX);
	t->cpd0	 = z_blc1_cmd_parse_xps(buf, PD4_IDX);
	t->cpd1	 = z_blc1_cmd_parse_xps(buf, PD5_IDX);
	t->cpd2	 = z_blc1_cmd_parse_xps(buf, PD6_IDX);
	t->cpd3	 = z_blc1_cmd_parse_xps(buf, PD7_IDX);
	t->ceio0 = z_blc1_cmd_parse_xps(buf, EIO0_IDX);
	t->ceio1 = z_blc1_cmd_parse_xps(buf, EIO1_IDX);
	t->ceio2 = z_blc1_cmd_parse_xps(buf, EIO2_IDX);
	t->ceio3 = z_blc1_cmd_parse_xps(buf, EIO3_IDX);
	t->ceio4 = z_blc1_cmd_parse_xps(buf, EIO4_IDX);
	t->ceio5 = z_blc1_cmd_parse_xps(buf, EIO5_IDX);
	t->ceio6 = z_blc1_cmd_parse_xps(buf, EIO6_IDX);
	t->ceio7 = z_blc1_cmd_parse_xps(buf, EIO7_IDX);
	t->cpb0	 = z_blc1_cmd_parse_xps(buf, PB0_IDX);
	t->cpe0	 = z_blc1_cmd_parse_xps(buf, PE0_IDX);
	t->cpe1	 = z_blc1_cmd_parse_xps(buf, PE1_IDX);

	return 0;
}