#include <caniot/caniot_private.h>
#include <caniot/classes.h>

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

int caniot_blc0_telemetry_ser(const struct caniot_blc0_telemetry *t,
			      uint8_t *buf,
			      uint8_t *len)
{
#if CONFIG_CANIOT_CHECKS
	if (!t || !buf || !len || *len < CANIOT_BLC0_TELEMETRY_BUF_LEN)
		return -CANIOT_EINVAL;
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
#endif

	t->dio		    = buf[0];
	t->pdio		    = buf[1] & 0x0Fu;
	t->int_temperature  = (buf[2]) | ((buf[3] & 0x03u) << 8);
	t->ext_temperature  = ((buf[3] >> 2) & 0x3Fu) | ((buf[4] & 0x0Fu) << 6);
	t->ext_temperature2 = ((buf[4] >> 4) & 0x0Fu) | ((buf[5] & 0x3Fu) << 4);
	t->ext_temperature3 = ((buf[5] >> 6) & 0x03u) | ((buf[6]) << 2);

	return 0;
}

int caniot_blc0_command_ser(const struct caniot_blc0_command *t,
			    uint8_t *buf,
			    uint8_t *len)
{
#if CONFIG_CANIOT_CHECKS
	if (!t || !buf || !len || *len < CANIOT_BLC0_COMMAND_BUF_LEN)
		return -CANIOT_EINVAL;
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
#endif

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

int caniot_blc1_cmd_set_xps(uint8_t *buf,
			    uint8_t len,
			    uint8_t n,
			    caniot_complex_digital_cmd_t xps)
{
#if CONFIG_CANIOT_CHECKS
	if (!buf || n >= CANIOT_CLASS1_IO_COUNT || len >= CANIOT_BLC1_COMMAND_BUF_LEN)
		return -CANIOT_EINVAL;
#endif

	z_blc1_cmd_set_xps(buf, n, xps);

	return 0;
}

caniot_complex_digital_cmd_t
caniot_blc1_cmd_parse_xps(const uint8_t *buf, uint8_t len, uint8_t n)
{
	if (!buf || n >= CANIOT_CLASS1_IO_COUNT || len >= CANIOT_BLC1_COMMAND_BUF_LEN)
		return -CANIOT_EINVAL;

	return z_blc1_cmd_parse_xps(buf, n);
}

int caniot_blc1_telemetry_ser(const struct caniot_blc1_telemetry *t,
			      uint8_t *buf,
			      uint8_t *len)
{
#if CONFIG_CANIOT_CHECKS
	if (!t || !buf || !len || *len < CANIOT_BLC1_TELEMETRY_BUF_LEN)
		return -CANIOT_EINVAL;
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
#endif

	t->pcpd = buf[0];
	t->eio	= buf[1];
	t->pb0	= buf[2] & 0x01u;
	t->pe0	= (buf[2] >> 1) & 0x01u;
	t->pe1	= (buf[2] >> 2) & 0x01u;

	t->int_temperature  = (buf[3]) | ((buf[4] & 0x03u) << 8);
	t->ext_temperature  = ((buf[4] >> 2) & 0x3fu) | ((buf[5] & 0x0fu) << 6);
	t->ext_temperature2 = ((buf[5] >> 4) & 0x0fu) | ((buf[6] & 0x3fu) << 4);
	t->ext_temperature3 = ((buf[6] >> 6) & 0x03u) | ((buf[7]) << 2);

	return 0;
}

int caniot_blc1_command_ser(const struct caniot_blc1_command *t,
			    uint8_t *buf,
			    uint8_t *len)
{
#if CONFIG_CANIOT_CHECKS
	if (!t || !buf || !len || *len < CANIOT_BLC1_COMMAND_BUF_LEN)
		return -CANIOT_EINVAL;
#endif

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
#endif

	for (uint8_t i = 0u; i < CANIOT_CLASS1_IO_COUNT; i++) {
		t->gpio_commands[i] = z_blc1_cmd_parse_xps(buf, i);
	}

	return 0;
}