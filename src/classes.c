#include <stdio.h>
#include <string.h>

#include <caniot/caniot_private.h>
#include <caniot/classes.h>

int caniot_blc0_telemetry_ser(const struct caniot_blc0_telemetry *t,
							  uint8_t *buf,
							  uint8_t *len)
{
#if CONFIG_CANIOT_CHECKS
	if (!t || !buf || !len || *len < CANIOT_BLC0_TELEMETRY_BUF_LEN) return -CANIOT_EINVAL;
#endif

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

	*len = CANIOT_BLC0_TELEMETRY_BUF_LEN;

	return 0;
}

int caniot_blc0_telemetry_get(struct caniot_blc0_telemetry *t,
							  const uint8_t *buf,
							  uint8_t len)
{
#if CONFIG_CANIOT_CHECKS
	if (!t || !buf || len < CANIOT_BLC0_TELEMETRY_BUF_LEN) return -CANIOT_EINVAL;
#else
	(void)len;
#endif

	t->dio				= buf[0];
	t->pdio				= buf[1] & 0x0Fu;
	t->int_temperature	= (buf[2]) | ((buf[3] & 0x03u) << 8);
	t->ext_temperature	= ((buf[3] >> 2) & 0x3Fu) | ((buf[4] & 0x0Fu) << 6);
	t->ext_temperature2 = ((buf[4] >> 4) & 0x0Fu) | ((buf[5] & 0x3Fu) << 4);
	t->ext_temperature3 = ((buf[5] >> 6) & 0x03u) | ((buf[6]) << 2);


	return 0;
}

int caniot_blc0_telemetry_defaults(struct caniot_blc0_telemetry *t)
{
	if (!t) return -CANIOT_EINVAL;

	memset(t, 0x00U, sizeof(struct caniot_blc0_telemetry));

	t->dio				= 0x00u;
	t->pdio				= 0x00u;
	t->int_temperature	= CANIOT_DT_T10_INVALID;
	t->ext_temperature	= CANIOT_DT_T10_INVALID;
	t->ext_temperature2 = CANIOT_DT_T10_INVALID;
	t->ext_temperature3 = CANIOT_DT_T10_INVALID;

	return 0;
}

int caniot_blc0_telemetry_get_temperature(const struct caniot_blc0_telemetry *t,
										  caniot_temp_sens_t sensor,
										  uint16_t *temperature)
{
	if (!t || !temperature) return -CANIOT_EINVAL;

	switch (sensor) {
	case CANIOT_TEMP_INT:
		*temperature = t->int_temperature;
		break;
	case CANIOT_TEMP_EXT1:
		*temperature = t->ext_temperature;
		break;
	case CANIOT_TEMP_EXT2:
		*temperature = t->ext_temperature2;
		break;
	case CANIOT_TEMP_EXT3:
		*temperature = t->ext_temperature3;
		break;
	default:
		*temperature = CANIOT_DT_T10_INVALID;
		return -CANIOT_EINVAL;
		break;
	}

	if (*temperature == CANIOT_DT_T10_INVALID) return 1;

	return 0;
}

int caniot_blc0_telemetry_set_temperature(struct caniot_blc0_telemetry *t,
										  caniot_temp_sens_t sensor,
										  uint16_t temperature)
{
	if (!t) return -CANIOT_EINVAL;
	if ((temperature & CANIOT_DT_T10_MASK) == CANIOT_DT_T10_INVALID)
		return -CANIOT_EINVAL;

	switch (sensor) {
	case CANIOT_TEMP_INT:
		t->int_temperature = temperature;
		break;
	case CANIOT_TEMP_EXT1:
		t->ext_temperature = temperature;
		break;
	case CANIOT_TEMP_EXT2:
		t->ext_temperature2 = temperature;
		break;
	case CANIOT_TEMP_EXT3:
		t->ext_temperature3 = temperature;
		break;
	default:
		return -CANIOT_EINVAL;
		break;
	}

	return 0;
}

int caniot_blc0_telemetry_clear_temperature(struct caniot_blc0_telemetry *t,
											caniot_temp_sens_t sensor)
{
	if (!t) return -CANIOT_EINVAL;

	switch (sensor) {
	case CANIOT_TEMP_INT:
		t->int_temperature = CANIOT_DT_T10_INVALID;
		break;
	case CANIOT_TEMP_EXT1:
		t->ext_temperature = CANIOT_DT_T10_INVALID;
		break;
	case CANIOT_TEMP_EXT2:
		t->ext_temperature2 = CANIOT_DT_T10_INVALID;
		break;
	case CANIOT_TEMP_EXT3:
		t->ext_temperature3 = CANIOT_DT_T10_INVALID;
		break;
	default:
		return -CANIOT_EINVAL;
		break;
	}

	return 0;
}

int caniot_blc0_telemetry_get_io(const struct caniot_blc0_telemetry *t,
								 caniot_blc0_io_t io,
								 bool *state)
{
	if (!t || !state) return -CANIOT_EINVAL;

	switch (io) {
	case CANIOT_BLC0_OC1:
	case CANIOT_BLC0_OC2:
	case CANIOT_BLC0_RELAY1:
	case CANIOT_BLC0_RELAY2:
	case CANIOT_BLC0_IN1:
	case CANIOT_BLC0_IN2:
	case CANIOT_BLC0_IN3:
	case CANIOT_BLC0_IN4:
		*state = (t->dio & (1u << io)) ? true : false;
		break;
	case CANIOT_BLC0_OC1_PULSE_ACTIVE:
	case CANIOT_BLC0_OC2_PULSE_ACTIVE:
	case CANIOT_BLC0_RELAY1_PULSE_ACTIVE:
	case CANIOT_BLC0_RELAY2_PULSE_ACTIVE:
		*state = (t->pdio & (1u << (io - 8u))) ? true : false;
		break;
	default:
		*state = false;
		return -CANIOT_EINVAL;
		break;
	}

	return 0;
}

int caniot_blc0_telemetry_set_io(struct caniot_blc0_telemetry *t,
								 caniot_blc0_io_t io,
								 bool state)
{
	if (!t) return -CANIOT_EINVAL;

	switch (io) {
	case CANIOT_BLC0_OC1:
	case CANIOT_BLC0_OC2:
	case CANIOT_BLC0_RELAY1:
	case CANIOT_BLC0_RELAY2:
	case CANIOT_BLC0_IN1:
	case CANIOT_BLC0_IN2:
	case CANIOT_BLC0_IN3:
	case CANIOT_BLC0_IN4:
		if (state)
			t->dio |= (1u << io);
		else
			t->dio &= ~(1u << io);
		break;
	case CANIOT_BLC0_OC1_PULSE_ACTIVE:
	case CANIOT_BLC0_OC2_PULSE_ACTIVE:
	case CANIOT_BLC0_RELAY1_PULSE_ACTIVE:
	case CANIOT_BLC0_RELAY2_PULSE_ACTIVE:
		if (state)
			t->pdio |= (1u << (io - 8u));
		else
			t->pdio &= ~(1u << (io - 8u));
		break;
	default:
		return -CANIOT_EINVAL;
		break;
	}

	return 0;
}

int caniot_blc0_command_ser(const struct caniot_blc0_command *t,
							uint8_t *buf,
							uint8_t *len)
{
#if CONFIG_CANIOT_CHECKS
	if (!t || !buf || !len || *len < CANIOT_BLC0_COMMAND_BUF_LEN) return -CANIOT_EINVAL;
#else
	(void)len;
#endif

	buf[0] = t->coc1 | (t->coc2 << 3) | ((t->crl1 & 0x03u) << 6);
	buf[1] = (t->crl1 >> 2) | (t->crl2 << 1);

	*len = CANIOT_BLC0_COMMAND_BUF_LEN;

	return 0;
}

int caniot_blc0_command_get(struct caniot_blc0_command *t,
							const uint8_t *buf,
							uint8_t len)
{
#if CONFIG_CANIOT_CHECKS
	if (!t || !buf || len < CANIOT_BLC0_COMMAND_BUF_LEN) return -CANIOT_EINVAL;
#else
	(void)len;
#endif

	t->coc1 = buf[0] & 0x07u;
	t->coc2 = (buf[0] >> 3) & 0x07u;
	t->crl1 = ((buf[0] >> 6) & 0x03u) | ((buf[1] & 0x01u) << 2);
	t->crl2 = (buf[1] >> 1) & 0x07u;

	return 0;
}

int caniot_blc0_command_defaults(struct caniot_blc0_command *c)
{
	if (!c) return -CANIOT_EINVAL;

	memset(c, 0x00U, sizeof(struct caniot_blc0_command));

	c->coc1 = CANIOT_XPS_NONE;
	c->coc2 = CANIOT_XPS_NONE;
	c->crl1 = CANIOT_XPS_NONE;
	c->crl2 = CANIOT_XPS_NONE;

	return 0;
}

int caniot_blc0_command_set_xps(struct caniot_blc0_command *cmd,
								caniot_blc0_io_t io,
								caniot_complex_digital_cmd_t xps)
{
	if (!cmd) return -CANIOT_EINVAL;
	
	switch (io) {
	case CANIOT_BLC0_OC1:
		cmd->coc1 = xps;
		break;
	case CANIOT_BLC0_OC2:
		cmd->coc2 = xps;
		break;
	case CANIOT_BLC0_RELAY1:
		cmd->crl1 = xps;
		break;
	case CANIOT_BLC0_RELAY2:
		cmd->crl2 = xps;
		break;
	default:
		return -CANIOT_EINVAL;
		break;
	}

	return 0;
}

int caniot_blc0_command_get_xps(const struct caniot_blc0_command *cmd,
								caniot_blc0_io_t io,
								caniot_complex_digital_cmd_t *xps)
{
	if (!cmd || !xps) return -CANIOT_EINVAL;

	switch (io) {
	case CANIOT_BLC0_OC1:
		*xps = cmd->coc1;
		break;
	case CANIOT_BLC0_OC2:
		*xps = cmd->coc2;
		break;
	case CANIOT_BLC0_RELAY1:
		*xps = cmd->crl1;
		break;
	case CANIOT_BLC0_RELAY2:
		*xps = cmd->crl2;
		break;
	default:
		*xps = CANIOT_XPS_NONE;
		return -CANIOT_EINVAL;
		break;
	}

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

static caniot_complex_digital_cmd_t z_blc1_cmd_parse_xps(const uint8_t *buf, uint8_t n)
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

int caniot_blc1_cmd_buf_set_xps(caniot_complex_digital_cmd_t xps,
								uint8_t *buf,
								uint8_t len,
								uint8_t n)
{
#if CONFIG_CANIOT_CHECKS
	if (!buf || n >= CANIOT_CLASS1_IO_COUNT || len >= CANIOT_BLC1_COMMAND_BUF_LEN)
		return -CANIOT_EINVAL;
#else
	(void)len;
#endif

	z_blc1_cmd_set_xps(buf, n, xps);

	return 0;
}

int caniot_blc1_cmd_buf_parse_xps(caniot_complex_digital_cmd_t *xps,
								  const uint8_t *buf,
								  uint8_t len,
								  uint8_t n)
{
#if CONFIG_CANIOT_CHECKS
	if (!buf || !xps || n >= CANIOT_CLASS1_IO_COUNT ||
		len >= CANIOT_BLC1_COMMAND_BUF_LEN) {
		if (xps) *xps = CANIOT_XPS_NONE;
		return -CANIOT_EINVAL;
	}
#else
	(void)len;
#endif

	*xps = z_blc1_cmd_parse_xps(buf, n);

	return 0;
}

int caniot_blc1_telemetry_ser(const struct caniot_blc1_telemetry *t,
							  uint8_t *buf,
							  uint8_t *len)
{
#if CONFIG_CANIOT_CHECKS
	if (!t || !buf || !len || *len < CANIOT_BLC1_TELEMETRY_BUF_LEN) return -CANIOT_EINVAL;
#else
	(void)len;
#endif

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

	*len = CANIOT_BLC1_TELEMETRY_BUF_LEN;

	return 0;
}

int caniot_blc1_telemetry_get(struct caniot_blc1_telemetry *t,
							  const uint8_t *buf,
							  uint8_t len)
{
#if CONFIG_CANIOT_CHECKS
	if (!t || !buf || len < CANIOT_BLC1_TELEMETRY_BUF_LEN) return -CANIOT_EINVAL;
#else
	(void)len;
#endif

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

int caniot_blc1_telemetry_defaults(struct caniot_blc1_telemetry *t)
{
	if (!t) return -CANIOT_EINVAL;

	memset(t, 0x00U, sizeof(struct caniot_blc1_telemetry));

	t->pcpd				= 0x00u;
	t->eio				= 0x00u;
	t->pb0				= 0x00u;
	t->pe0				= 0x00u;
	t->pe1				= 0x00u;
	t->int_temperature	= CANIOT_DT_T10_INVALID;
	t->ext_temperature	= CANIOT_DT_T10_INVALID;
	t->ext_temperature2 = CANIOT_DT_T10_INVALID;
	t->ext_temperature3 = CANIOT_DT_T10_INVALID;

	return 0;
}

int caniot_blc1_telemetry_get_temperature(const struct caniot_blc1_telemetry *t,
										  caniot_temp_sens_t sensor,
										  uint16_t *temperature)
{
	if (!t || !temperature) return -CANIOT_EINVAL;

	switch (sensor) {
	case CANIOT_TEMP_INT:
		*temperature = t->int_temperature;
		break;
	case CANIOT_TEMP_EXT1:
		*temperature = t->ext_temperature;
		break;
	case CANIOT_TEMP_EXT2:	
		*temperature = t->ext_temperature2;
		break;
	case CANIOT_TEMP_EXT3:
		*temperature = t->ext_temperature3;
		break;
	default:
		*temperature = CANIOT_DT_T10_INVALID;
		return -CANIOT_EINVAL;
		break;
	}

	if (*temperature == CANIOT_DT_T10_INVALID) return 1;

	return 0;
}

int caniot_blc1_telemetry_set_temperature(struct caniot_blc1_telemetry *t,
										  caniot_temp_sens_t sensor,
										  uint16_t temperature)
{
	if (!t) return -CANIOT_EINVAL;
	if ((temperature & CANIOT_DT_T10_MASK) == CANIOT_DT_T10_INVALID)
		return -CANIOT_EINVAL;

	switch (sensor) {
	case CANIOT_TEMP_INT:
		t->int_temperature = temperature;
		break;
	case CANIOT_TEMP_EXT1:
		t->ext_temperature = temperature;
		break;
	case CANIOT_TEMP_EXT2:
		t->ext_temperature2 = temperature;
		break;
	case CANIOT_TEMP_EXT3:
		t->ext_temperature3 = temperature;
		break;
	default:
		return -CANIOT_EINVAL;
		break;
	}

	return 0;
}

int caniot_blc1_telemetry_clear_temperature(struct caniot_blc1_telemetry *t,
											caniot_temp_sens_t sensor)
{
	if (!t) return -CANIOT_EINVAL;

	switch (sensor) {
	case CANIOT_TEMP_INT:
		t->int_temperature = CANIOT_DT_T10_INVALID;
		break;
	case CANIOT_TEMP_EXT1:
		t->ext_temperature = CANIOT_DT_T10_INVALID;
		break;
	case CANIOT_TEMP_EXT2:
		t->ext_temperature2 = CANIOT_DT_T10_INVALID;
		break;
	case CANIOT_TEMP_EXT3:
		t->ext_temperature3 = CANIOT_DT_T10_INVALID;
		break;
	default:
		return -CANIOT_EINVAL;
		break;
	}

	return 0;
}

int caniot_blc1_telemetry_get_io(const struct caniot_blc1_telemetry *t,
								 caniot_blc1_io_t io,
								 bool *state)
{
	if (!t || !state) return -CANIOT_EINVAL;

	switch (io) {
	case CANIOT_BLC1_PC0:
	case CANIOT_BLC1_PC1:
	case CANIOT_BLC1_PC2:
	case CANIOT_BLC1_PC3:
	case CANIOT_BLC1_PD4:
	case CANIOT_BLC1_PD5:
	case CANIOT_BLC1_PD6:
	case CANIOT_BLC1_PD7:
		*state = (t->pcpd & (1u << io)) ? true : false;
		break;
	case CANIOT_BLC1_EIO0:
	case CANIOT_BLC1_EIO1:
	case CANIOT_BLC1_EIO2:
	case CANIOT_BLC1_EIO3:
	case CANIOT_BLC1_EIO4:
	case CANIOT_BLC1_EIO5:
	case CANIOT_BLC1_EIO6:
	case CANIOT_BLC1_EIO7:
		*state = (t->eio & (1u << (io - 8u))) ? true : false;
		break;
	case CANIOT_BLC1_PB0:
		*state = t->pb0 ? true : false;
		break;
	case CANIOT_BLC1_PE0:
		*state = t->pe0 ? true : false;
		break;
	case CANIOT_BLC1_PE1:
		*state = t->pe1 ? true : false;
		break;
	default:
		*state = false;
		return -CANIOT_EINVAL;
		break;
	}

	return 0;
}

int caniot_blc1_telemetry_set_io(struct caniot_blc1_telemetry *t,
								 caniot_blc1_io_t io,
								 bool state)
{
	if (!t) return -CANIOT_EINVAL;

	switch (io) {
	case CANIOT_BLC1_PC0:
	case CANIOT_BLC1_PC1:
	case CANIOT_BLC1_PC2:
	case CANIOT_BLC1_PC3:
	case CANIOT_BLC1_PD4:
	case CANIOT_BLC1_PD5:
	case CANIOT_BLC1_PD6:
	case CANIOT_BLC1_PD7:
		if (state)
			t->pcpd |= (1u << io);
		else
			t->pcpd &= ~(1u << io);
		break;
	case CANIOT_BLC1_EIO0:
	case CANIOT_BLC1_EIO1:
	case CANIOT_BLC1_EIO2:
	case CANIOT_BLC1_EIO3:
	case CANIOT_BLC1_EIO4:
	case CANIOT_BLC1_EIO5:
	case CANIOT_BLC1_EIO6:
	case CANIOT_BLC1_EIO7:
		if (state)
			t->eio |= (1u << (io - 8u));
		else
			t->eio &= ~(1u << (io - 8u));
		break;
	case CANIOT_BLC1_PB0:
		t->pb0 = state ? 1u : 0u;
		break;
	case CANIOT_BLC1_PE0:
		t->pe0 = state ? 1u : 0u;
		break;
	case CANIOT_BLC1_PE1:
		t->pe1 = state ? 1u : 0u;
		break;
	default:
		return -CANIOT_EINVAL;
		break;
	}

	return 0;
}

int caniot_blc1_command_ser(const struct caniot_blc1_command *t,
							uint8_t *buf,
							uint8_t *len)
{
#if CONFIG_CANIOT_CHECKS
	if (!t || !buf || !len || *len < CANIOT_BLC1_COMMAND_BUF_LEN) return -CANIOT_EINVAL;
#endif

	memset(buf, 0x00u, CANIOT_BLC1_COMMAND_BUF_LEN);

	for (uint8_t i = 0u; i < CANIOT_CLASS1_IO_COUNT; i++) {
		z_blc1_cmd_set_xps(buf, i, t->gpio_commands[i]);
	}

	*len = CANIOT_BLC1_COMMAND_BUF_LEN;

	return 0;
}

int caniot_blc1_command_get(struct caniot_blc1_command *t,
							const uint8_t *buf,
							uint8_t len)
{
#if CONFIG_CANIOT_CHECKS
	if (!t || !buf || len < CANIOT_BLC1_COMMAND_BUF_LEN) return -CANIOT_EINVAL;
#else
	(void)len;
#endif

	for (uint8_t i = 0u; i < CANIOT_CLASS1_IO_COUNT; i++) {
		t->gpio_commands[i] = z_blc1_cmd_parse_xps(buf, i);
	}

	return 0;
}

int caniot_blc1_command_defaults(struct caniot_blc1_command *c)
{
	if (!c) return -CANIOT_EINVAL;

	memset(c, 0x00U, sizeof(struct caniot_blc1_command));

	for (uint8_t i = 0u; i < CANIOT_CLASS1_IO_COUNT; i++) {
		c->gpio_commands[i] = CANIOT_XPS_NONE;
	}

	return 0;
}

int caniot_blc1_command_set_xps(struct caniot_blc1_command *cmd,
								caniot_blc1_io_t io,
								caniot_complex_digital_cmd_t xps)
{
	if (!cmd) return -CANIOT_EINVAL;
	if (io >= CANIOT_CLASS1_IO_COUNT) return -CANIOT_EINVAL;

	cmd->gpio_commands[io] = xps;

	return 0;
}

int caniot_blc1_command_get_xps(const struct caniot_blc1_command *cmd,
								caniot_blc1_io_t io,
								caniot_complex_digital_cmd_t *xps)
{
	if (!cmd || !xps) return -CANIOT_EINVAL;
	if (io >= CANIOT_CLASS1_IO_COUNT) {
		*xps = CANIOT_XPS_NONE;
		return -CANIOT_EINVAL;
	}

	*xps = cmd->gpio_commands[io];

	return 0;
}
