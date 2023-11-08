/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <caniot/caniot_private.h>
#include <caniot/classes.h>
#include <caniot/controller.h>
#include <caniot/datatype.h>
#include <caniot/device.h>

#define SEED 0

#define TRUE  true
#define FALSE false

#define GB(_x, _pos)		 (((_x) >> (_pos)) & 0x1u)
#define GBZ(_x, _pos, _size) (((_x) >> (_pos)) & ((1u << (_size)) - 1u))
#define GB2(_x, _pos)		 (((_x) >> (_pos)) & 0x3u)
#define GB3(_x, _pos)		 (((_x) >> (_pos)) & 0x7u)
#define GB4(_x, _pos)		 (((_x) >> (_pos)) & 0xFu)

#define TEST_ASSERT(statement)                                                           \
	if (!(statement)) {                                                                  \
		printf("%s:%d: %s: Assertion `%s' failed.\n",                                    \
			   __FILE__,                                                                 \
			   __LINE__,                                                                 \
			   __func__,                                                                 \
			   #statement);                                                              \
		exit(EXIT_FAILURE);                                                              \
	}

#define CHECK(statement)                                                                 \
	if ((statement) == false) {                                                          \
		return false;                                                                    \
	}
#define CHECK_0(statement)                                                               \
	if ((statement) != 0) {                                                              \
		return false;                                                                    \
	}
#define CHECK_POSITIVE(statement)                                                        \
	if ((statement) < 0) {                                                               \
		return false;                                                                    \
	}
#define CHECK_STRICTLY_POSITIVE(statement)                                               \
	if ((statement) <= 0) {                                                              \
		return false;                                                                    \
	}

void __assert(bool statement)
{
	if (statement == false) {
		printf("Assertion failed\n");
		exit(EXIT_FAILURE);
	}
}

size_t rand_range(size_t min, size_t max)
{
	if (max > RAND_MAX) {
		exit(EXIT_FAILURE);
	}

	return min + (rand() % (max - min + 1U));
}

uint8_t r8(void)
{
	return (uint8_t)rand_range(0U, (1U << 8U) - 1U);
}

uint16_t r16(void)
{
	return (uint16_t)rand_range(0U, (1U << 16U) - 1U);
}

/*____________________________________________________________________________*/

// caniot_frame_type_t type = r8() & 0x3U;
// caniot_frame_dir_t dir = r8() & 0x1U;
// caniot_device_class_t cls = r8() & CANIOT_CLASS_BROADCAST;
// caniot_device_subid_t sid = r8() & CANIOT_SUBID_BROADCAST;
// caniot_endpoint_t ep = r8() & 0x3U;

/*____________________________________________________________________________*/

bool z_identity(void)
{
	return true;
}

bool z_macro__CANIOT_DID(void)
{
	const uint8_t cls = r8() & CANIOT_CLASS_BROADCAST;
	const uint8_t sid = r8() & CANIOT_SUBID_BROADCAST;

	const caniot_did_t did = CANIOT_DID(cls, sid);

	return (CANIOT_DID_CLS(did) == cls) && (CANIOT_DID_SID(did) == sid);
}

bool z_macro__CANIOT_DEVICE_IS_BROADCAST(void)
{
	const caniot_did_t did = CANIOT_DID(CANIOT_CLASS_BROADCAST, CANIOT_SUBID_BROADCAST);

	return CANIOT_DID_EQ(did, CANIOT_DID_BROADCAST);
}

static caniot_id_t gen_rdm_id(void)
{
	const caniot_id_t id = {
		.type = r8(), .query = r8(), .cls = r8(), .sid = r8(), .endpoint = r8()};

	return id;
}

bool z_macro__CANIOT_ID(void)
{
	const caniot_id_t id = gen_rdm_id();

	uint16_t canid = CANIOT_ID(id.type, id.query, id.cls, id.sid, id.endpoint);

	return (CANIOT_ID_GET_TYPE(canid) == id.type) &&
		   (CANIOT_ID_GET_QUERY(canid) == id.query) &&
		   (CANIOT_ID_GET_CLASS(canid) == id.cls) &&
		   (CANIOT_ID_GET_SUBID(canid) == id.sid) &&
		   (CANIOT_ID_GET_ENDPOINT(canid) == id.endpoint);
}

bool z_func__caniot_id_to_canid(void)
{
	const caniot_id_t id = gen_rdm_id();

	return caniot_id_to_canid(id) ==
		   CANIOT_ID(id.type, id.query, id.cls, id.sid, id.endpoint);
}

bool z_struct__caniot_id_t(void)
{
	const caniot_id_t id = {.type	  = CANIOT_FRAME_TYPE_READ_ATTRIBUTE,
							.query	  = CANIOT_RESPONSE,
							.cls	  = CANIOT_CLASS_BROADCAST,
							.sid	  = CANIOT_SUBID_BROADCAST,
							.endpoint = CANIOT_ENDPOINT_BOARD_CONTROL};

	uint16_t canid = caniot_id_to_canid(id);

	return canid & 0x3FU;
}

bool z_misc_id_conversion(void)
{
	const caniot_id_t id = gen_rdm_id();

	const uint16_t canid = caniot_id_to_canid(id);

	const caniot_id_t id2 = caniot_canid_to_id(canid);

	return (id.type == id2.type) && (id.query == id2.query) && (id.cls == id2.cls) &&
		   (id.sid == id2.sid) && (id.endpoint == id2.endpoint);
}

static caniot_did_t gen_rdm_did(bool including_broadcast)
{
	if (including_broadcast) {
		return CANIOT_DID_FROM_RAW(rand_range(0, CANIOT_DID_BROADCAST - 1));
	} else {
		return CANIOT_DID_FROM_RAW(rand_range(0, CANIOT_DID_BROADCAST - 1));
	}
}

bool z_func__caniot_device_is_target(void)
{
	const caniot_did_t did = gen_rdm_did(true);

	const struct caniot_frame fordev = {
		.id =
			{
				.cls   = CANIOT_DID_CLS(did),
				.sid   = CANIOT_DID_SID(did),
				.query = CANIOT_QUERY,
				.type  = CANIOT_FRAME_TYPE_READ_ATTRIBUTE,
			},
		.len = 0,
	};

	const struct caniot_frame notfordev = {
		.id =
			{
				.cls   = CANIOT_DID_CLS(did),
				.sid   = CANIOT_DID_SID(did),
				.query = CANIOT_RESPONSE,
				.type  = CANIOT_FRAME_TYPE_READ_ATTRIBUTE,
			},
		.len = 0,
	};

	return caniot_device_is_target(did, &fordev) &&
		   !caniot_device_is_target(did, &notfordev);
}

bool z_func__caniot_resp_error_for(void)
{
	bool all = true;

	all &=
		caniot_resp_error_for(CANIOT_FRAME_TYPE_TELEMETRY) == CANIOT_FRAME_TYPE_COMMAND;
	all &= caniot_resp_error_for(CANIOT_FRAME_TYPE_COMMAND) == CANIOT_FRAME_TYPE_COMMAND;
	all &= caniot_resp_error_for(CANIOT_FRAME_TYPE_READ_ATTRIBUTE) ==
		   CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE;
	all &= caniot_resp_error_for(CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE) ==
		   CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE;

	return all;
}

bool z_func__caniot_validate_drivers_api(void)
{
	struct caniot_drivers_api api = {
		.entropy = NULL, .get_time = NULL, .recv = NULL, .send = NULL, .set_time = NULL};

	return caniot_validate_drivers_api(&api) == false;
}

bool z_func__caniot_device_get_filter(void)
{
	const caniot_did_t did = gen_rdm_did(false);
	const uint16_t filter  = caniot_device_get_filter(did);

	return (CANIOT_ID_GET_CLASS(filter) == CANIOT_DID_CLS(did)) &&
		   (CANIOT_ID_GET_SUBID(filter) == CANIOT_DID_SID(did)) &&
		   (CANIOT_ID_GET_QUERY(filter) == CANIOT_QUERY);
}

bool z_func__caniot_device_get_mask(void)
{
	const uint16_t mask = caniot_device_get_mask();

	return CANIOT_ID(0, 0x1U, 0x7U, 0x7U, 0) == mask;
}

bool z_func__caniot_device_get_filter_broadcast(void)
{
	const uint16_t filter = caniot_device_get_filter_broadcast();

	printf("filter: %x, %d, %d, %d\n",
		   filter,
		   CANIOT_ID_GET_CLASS(filter),
		   CANIOT_ID_GET_SUBID(filter),
		   CANIOT_ID_GET_QUERY(filter));

	return (CANIOT_ID_GET_CLASS(filter) == CANIOT_CLASS_BROADCAST) &&
		   (CANIOT_ID_GET_SUBID(filter) == CANIOT_SUBID_BROADCAST) &&
		   (CANIOT_ID_GET_QUERY(filter) == CANIOT_QUERY);
}

bool z_func___si_caniot_device_get_filter(void)
{
	const caniot_did_t did = gen_rdm_did(false);

	return _si_caniot_device_get_filter(did) == caniot_device_get_filter(did);
}

bool z_func___si_caniot_device_get_filter_broadcast(void)
{
	return _si_caniot_device_get_filter_broadcast() ==
		   caniot_device_get_filter_broadcast();
}

/*____________________________________________________________________________*/

struct z_func_ctrl_test_ctx {
	struct caniot_controller ctrl;
	struct caniot_frame req;
	struct caniot_frame resp;
	const caniot_did_t did;
	uint8_t handle;
	bool success;
	bool terminated;

	struct {
		bool active; /* tells if desired sub structure is active */

		bool resp_set; /* tells should be set */
		caniot_controller_event_context_t context;
		caniot_controller_event_status_t status;
	} desired;
};

static bool z_func_ctrl_cb(const caniot_controller_event_t *ev, void *user_data)
{
	TEST_ASSERT(user_data != NULL);

	struct z_func_ctrl_test_ctx *x = user_data;

	/* Return prematurely if already terminated */
	if (x->terminated) return true;

	bool all = true;

	if (x->desired.active == true) {
		all &= ev->context == x->desired.context;
		all &= ev->status == x->desired.status;
		all &= (ev->response == NULL) ^ (x->desired.resp_set == true);
	}

	all &= ev->controller == &x->ctrl;
	all &= ev->terminated == 1U;
	if (ev->context == CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY)
		all &= ev->handle == x->handle;
	all &= CANIOT_DID_EQ(ev->did, x->did);

	x->success	  = all;
	x->terminated = true;

	return true;
}

/* Check timeout in controller callback */
bool z_func_ctrl1(void)
{
	struct z_func_ctrl_test_ctx x = {.did	  = gen_rdm_did(false),
									 .success = false,
									 .desired = {
										 .active  = true,
										 .context = CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY,
									 }};

	x.desired.status   = CANIOT_CONTROLLER_EVENT_STATUS_TIMEOUT;
	x.desired.resp_set = false;

	CHECK_0(caniot_controller_init(&x.ctrl, z_func_ctrl_cb, &x));
	caniot_build_query_telemetry(&x.req, CANIOT_ENDPOINT_BOARD_CONTROL);
	CHECK_STRICTLY_POSITIVE(
		x.handle = caniot_controller_query_register(&x.ctrl, x.did, &x.req, 1000U));
	CHECK(caniot_controller_query_pending(&x.ctrl, x.handle) == true);
	CHECK(caniot_controller_dbg_free_pendq(&x.ctrl) ==
		  CONFIG_CANIOT_MAX_PENDING_QUERIES - 1U);

	CHECK_0(caniot_controller_rx_frame(&x.ctrl, 1000U, NULL));
	CHECK(caniot_controller_query_pending(&x.ctrl, x.handle) == false);
	CHECK(x.ctrl.pendingq.pending_devices_bf == 0U);
	CHECK(x.ctrl.pendingq.timeout_queue == NULL);
	CHECK(caniot_controller_dbg_free_pendq(&x.ctrl) == CONFIG_CANIOT_MAX_PENDING_QUERIES);

	return x.success == true;
}

/* Check OK in controller callback */
bool z_func_ctrl2(void)
{
	struct z_func_ctrl_test_ctx x = {.did	  = gen_rdm_did(false),
									 .success = false,
									 .desired = {
										 .active  = true,
										 .context = CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY,
									 }};

	x.desired.status   = CANIOT_CONTROLLER_EVENT_STATUS_OK;
	x.desired.resp_set = true;

	CHECK_0(caniot_controller_init(&x.ctrl, z_func_ctrl_cb, &x));
	caniot_build_query_telemetry(&x.req, CANIOT_ENDPOINT_BOARD_CONTROL);
	CHECK_STRICTLY_POSITIVE(
		x.handle = caniot_controller_query_register(&x.ctrl, x.did, &x.req, 1000U));
	CHECK(caniot_controller_query_pending(&x.ctrl, x.handle) == true);
	CHECK(caniot_controller_dbg_free_pendq(&x.ctrl) ==
		  CONFIG_CANIOT_MAX_PENDING_QUERIES - 1U);

	memcpy(&x.resp, &x.req, sizeof(x.req));
	x.resp.id.query = CANIOT_RESPONSE;

	CHECK_0(caniot_controller_rx_frame(&x.ctrl, 1000U, &x.resp));
	CHECK(caniot_controller_query_pending(&x.ctrl, x.handle) == false);
	CHECK(x.ctrl.pendingq.pending_devices_bf == 0U);
	CHECK(x.ctrl.pendingq.timeout_queue == NULL);
	CHECK(caniot_controller_dbg_free_pendq(&x.ctrl) == CONFIG_CANIOT_MAX_PENDING_QUERIES);

	return x.success == true;
}

/* Check status error in controller callback */
bool z_func_ctrl3(void)
{
	struct z_func_ctrl_test_ctx x = {.did	  = gen_rdm_did(false),
									 .success = false,
									 .desired = {
										 .active  = true,
										 .context = CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY,
									 }};

	x.desired.status   = CANIOT_CONTROLLER_EVENT_STATUS_ERROR;
	x.desired.resp_set = true;

	CHECK_0(caniot_controller_init(&x.ctrl, z_func_ctrl_cb, &x));
	caniot_build_query_telemetry(&x.req, CANIOT_ENDPOINT_BOARD_CONTROL);
	CHECK_STRICTLY_POSITIVE(
		x.handle = caniot_controller_query_register(&x.ctrl, x.did, &x.req, 1000U));
	CHECK(caniot_controller_query_pending(&x.ctrl, x.handle) == true);
	CHECK(caniot_controller_dbg_free_pendq(&x.ctrl) ==
		  CONFIG_CANIOT_MAX_PENDING_QUERIES - 1U);

	memcpy(&x.resp, &x.req, sizeof(x.req));
	x.resp.id.query = CANIOT_RESPONSE;
	x.resp.id.type	= caniot_resp_error_for(x.req.id.type);
	int32_t err_code = -CANIOT_EHANDLERC;
	write_le32(x.resp.buf, *(uint32_t *)&err_code);
	write_le32(x.resp.buf + 4, 0x12345678U);

	CHECK_0(caniot_controller_rx_frame(&x.ctrl, 1000U, &x.resp));
	CHECK(caniot_controller_query_pending(&x.ctrl, x.handle) == false);
	CHECK(x.ctrl.pendingq.pending_devices_bf == 0U);
	CHECK(x.ctrl.pendingq.timeout_queue == NULL);
	CHECK(caniot_controller_dbg_free_pendq(&x.ctrl) == CONFIG_CANIOT_MAX_PENDING_QUERIES);

	return x.success == true;
}

/* Check status cancelled in controller callback */
bool z_func_ctrl4(void)
{
	struct z_func_ctrl_test_ctx x = {.did	  = gen_rdm_did(false),
									 .success = false,
									 .desired = {
										 .active  = true,
										 .context = CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY,
									 }};

	x.desired.status   = CANIOT_CONTROLLER_EVENT_STATUS_CANCELLED;
	x.desired.resp_set = false;

	CHECK_0(caniot_controller_init(&x.ctrl, z_func_ctrl_cb, &x));
	caniot_build_query_telemetry(&x.req, CANIOT_ENDPOINT_BOARD_CONTROL);
	CHECK_STRICTLY_POSITIVE(
		x.handle = caniot_controller_query_register(&x.ctrl, x.did, &x.req, 1000U));
	CHECK(caniot_controller_query_pending(&x.ctrl, x.handle) == true);
	CHECK(caniot_controller_dbg_free_pendq(&x.ctrl) ==
		  CONFIG_CANIOT_MAX_PENDING_QUERIES - 1U);

	CHECK_0(caniot_controller_query_cancel(&x.ctrl, x.handle, false));
	CHECK(caniot_controller_query_pending(&x.ctrl, x.handle) == false);
	CHECK_0(caniot_controller_rx_frame(&x.ctrl, 1000U, NULL));
	CHECK(x.ctrl.pendingq.pending_devices_bf == 0U);
	CHECK(x.ctrl.pendingq.timeout_queue == NULL);
	CHECK(caniot_controller_dbg_free_pendq(&x.ctrl) == CONFIG_CANIOT_MAX_PENDING_QUERIES);

	return x.success == true;
}

bool z_func_dev0(void)
{
	struct z_func_ctrl_test_ctx x = {
		.did	 = gen_rdm_did(false),
		.success = false,
		.desired = {
			.active	 = true,
			.context = CANIOT_CONTROLLER_EVENT_CONTEXT_ORPHAN,
		}};

	x.desired.status   = CANIOT_CONTROLLER_EVENT_STATUS_OK;
	x.desired.resp_set = true;

	CHECK_0(caniot_controller_init(&x.ctrl, z_func_ctrl_cb, &x));
	caniot_build_query_telemetry(&x.req, CANIOT_ENDPOINT_BOARD_CONTROL);
	CHECK_STRICTLY_POSITIVE(
		x.handle = caniot_controller_query_register(&x.ctrl, x.did, &x.req, 1000U));
	CHECK(caniot_controller_query_pending(&x.ctrl, x.handle) == true);
	CHECK(caniot_controller_dbg_free_pendq(&x.ctrl) ==
		  CONFIG_CANIOT_MAX_PENDING_QUERIES - 1U);

	memcpy(&x.resp, &x.req, sizeof(x.req));
	x.resp.id.query = CANIOT_RESPONSE;
	x.resp.id.endpoint =
		CANIOT_ENDPOINT_APP; /* Device ID doesn't match req, so timeout */

	CHECK_0(caniot_controller_rx_frame(&x.ctrl, 1000U, &x.resp));
	CHECK(caniot_controller_query_pending(&x.ctrl, x.handle) == false);
	CHECK(x.ctrl.pendingq.pending_devices_bf == 0U);
	CHECK(x.ctrl.pendingq.timeout_queue == NULL);
	CHECK(caniot_controller_dbg_free_pendq(&x.ctrl) == CONFIG_CANIOT_MAX_PENDING_QUERIES);

	return x.success == true;
}

bool z_func_datatype_blc_sys(void)
{
	bool success = true;

	for (uint32_t n = 0; n < 2 * 2 * 2 * 4 * 2 * 4; n++) {
		struct caniot_blc_sys_command parsed_cmd;
		const struct caniot_blc_sys_command cmd = {
			.reset			 = GB(n, 0),
			._software_reset = GB(n, 1),
			._watchdog_reset = GB(n, 2),
			.watchdog		 = GB2(n, 3),
			.config_reset	 = GB(n, 5),
			.inhibit		 = GB2(n, 6),
		};

		const uint8_t serialized_cmd = caniot_blc_sys_command_to_byte(&cmd);
		caniot_blc_sys_command_from_byte(&parsed_cmd, serialized_cmd);

		success &= parsed_cmd.reset == cmd.reset;
		success &= parsed_cmd._software_reset == cmd._software_reset;
		success &= parsed_cmd._watchdog_reset == cmd._watchdog_reset;
		success &= parsed_cmd.watchdog == cmd.watchdog;
		success &= parsed_cmd.config_reset == cmd.config_reset;
		success &= parsed_cmd.inhibit == cmd.inhibit;
	}

	return success;
}

bool z_func_datatype_tmperature(void)
{
	// Precision is 0.1°C
	const int16_t temperature16 =
		rand_range(CANIOT_DT_T16_MIN / 10, CANIOT_DT_T16_MAX / 10) * 10;
	const uint16_t temperature10  = caniot_dt_T16_to_T10(temperature16);
	const int16_t temperature16_2 = caniot_dt_T10_to_T16(temperature10);
	return temperature16 == temperature16_2;
}

struct test_case_blc0 {
	const uint8_t buf[9u];
	struct caniot_blc0_command cmd;
};

static struct test_case_blc0 tests_blc0[] = {{{0x00u, 0x00u}, {0, 0, 0, 0}},
											 {{7, 0}, {7, 0, 0, 0}},
											 {{7 << 3, 0}, {0, 7, 0, 0}},
											 {{3 << 6, 1}, {0, 0, 7, 0}},
											 {{0, 7 << 1}, {0, 0, 0, 7}}};

bool z_func_encode_caniot_blc0_command(void)
{
	bool success = true;

	for (size_t i = 0; i < ARRAY_SIZE(tests_blc0); i++) {
		struct test_case_blc0 *tc = &tests_blc0[i];

		uint8_t buf[2];
		uint8_t len = 2u;

		success &= caniot_blc0_command_ser(&tc->cmd, buf, &len) == 0;
		success &= len == 2u;
		success &= (tc->buf[0u] == buf[0u]) && (tc->buf[1u] == buf[1u]);

		if (!success) break;
	}

	return success;
}

bool z_func_decode_caniot_blc0_command(void)
{
	bool success = true;

	for (size_t i = 0; i < ARRAY_SIZE(tests_blc0); i++) {
		struct test_case_blc0 *tc = &tests_blc0[i];

		struct caniot_blc0_command cmd;
		caniot_blc0_command_init(&cmd);

		success &= caniot_blc0_command_get(&cmd, tc->buf, 2u) == 0;
		success &= cmd.coc1 == tc->cmd.coc1;
		success &= cmd.coc2 == tc->cmd.coc2;
		success &= cmd.crl1 == tc->cmd.crl1;
		success &= cmd.crl2 == tc->cmd.crl2;

		if (!success) break;
	}

	return success;
}
struct test_case_blc1 {
	const uint8_t buf[7u];
	struct caniot_blc1_command cmd;
};

static struct test_case_blc1 tests_blc1[] = {
	{{0}, {{0}}},
	{{[0] = 7}, {.gpio_commands = {[0] = 7}}},
	{{[0] = 7 << 3}, {.gpio_commands = {[1] = 7}}},
	{{[0] = 3 << 6, [1] = 1}, {.gpio_commands = {[2] = 7}}},
	{{[1] = 7 << 1}, {.gpio_commands = {[3] = 7}}},
	{{[1] = 7 << 4}, {.gpio_commands = {[4] = 7}}},
	{{[1] = 1 << 7, [2] = 3}, {.gpio_commands = {[5] = 7}}},
	{{[2] = 7 << 2}, {.gpio_commands = {[6] = 7}}},
	{{[2] = 7 << 5}, {.gpio_commands = {[7] = 7}}},
	{{[3] = 7}, {.gpio_commands = {[8] = 7}}},
	{{[3] = 7 << 3}, {.gpio_commands = {[9] = 7}}},
	{{[3] = 3 << 6, [4] = 1}, {.gpio_commands = {[10] = 7}}},
	{{[4] = 7 << 1}, {.gpio_commands = {[11] = 7}}},
	{{[4] = 7 << 4}, {.gpio_commands = {[12] = 7}}},
	{{[4] = 1 << 7, [5] = 3}, {.gpio_commands = {[13] = 7}}},
	{{[5] = 7 << 2}, {.gpio_commands = {[14] = 7}}},
	{{[5] = 7 << 5}, {.gpio_commands = {[15] = 7}}},
	{{[6] = 7}, {.gpio_commands = {[16] = 7}}},
	{{[6] = 7 << 3}, {.gpio_commands = {[17] = 7}}},
	{{[6] = 3 << 6}, {.gpio_commands = {[18] = 3}}},
};

bool z_func_encode_caniot_blc1_command(void)
{
	bool success = true;

	for (size_t i = 0; i < ARRAY_SIZE(tests_blc1); i++) {
		struct test_case_blc1 *tc = &tests_blc1[i];

		uint8_t buf[7];
		uint8_t len = 7u;

		success &= caniot_blc1_command_ser(&tc->cmd, buf, &len) == 0;
		success &= len == 7u;
		success &= (tc->buf[0u] == buf[0u]) && (tc->buf[1u] == buf[1u]) &&
				   (tc->buf[2u] == buf[2u]) && (tc->buf[3u] == buf[3u]) &&
				   (tc->buf[4u] == buf[4u]) && (tc->buf[5u] == buf[5u]) &&
				   (tc->buf[6u] == buf[6u]);

		if (!success) break;
	}

	return success;
}

bool z_func_decode_caniot_blc1_command(void)
{
	bool success = true;

	for (size_t i = 0; i < ARRAY_SIZE(tests_blc1); i++) {
		struct test_case_blc1 *tc = &tests_blc1[i];

		struct caniot_blc1_command cmd;
		caniot_blc1_command_init(&cmd);

		success &= caniot_blc1_command_get(&cmd, tc->buf, 7u) == 0;
		success &= cmd.gpio_commands[0] == tc->cmd.gpio_commands[0];
		success &= cmd.gpio_commands[1] == tc->cmd.gpio_commands[1];
		success &= cmd.gpio_commands[2] == tc->cmd.gpio_commands[2];
		success &= cmd.gpio_commands[3] == tc->cmd.gpio_commands[3];
		success &= cmd.gpio_commands[4] == tc->cmd.gpio_commands[4];
		success &= cmd.gpio_commands[5] == tc->cmd.gpio_commands[5];
		success &= cmd.gpio_commands[6] == tc->cmd.gpio_commands[6];
		success &= cmd.gpio_commands[7] == tc->cmd.gpio_commands[7];
		success &= cmd.gpio_commands[8] == tc->cmd.gpio_commands[8];
		success &= cmd.gpio_commands[9] == tc->cmd.gpio_commands[9];
		success &= cmd.gpio_commands[10] == tc->cmd.gpio_commands[10];
		success &= cmd.gpio_commands[11] == tc->cmd.gpio_commands[11];
		success &= cmd.gpio_commands[12] == tc->cmd.gpio_commands[12];
		success &= cmd.gpio_commands[13] == tc->cmd.gpio_commands[13];
		success &= cmd.gpio_commands[14] == tc->cmd.gpio_commands[14];
		success &= cmd.gpio_commands[15] == tc->cmd.gpio_commands[15];
		success &= cmd.gpio_commands[16] == tc->cmd.gpio_commands[16];
		success &= cmd.gpio_commands[17] == tc->cmd.gpio_commands[17];
		success &= cmd.gpio_commands[18] == tc->cmd.gpio_commands[18];

		if (!success) break;
	}

	return success;
}

/*____________________________________________________________________________*/

struct test {
	const char *name;
	bool (*test_handler)(void);

	size_t rerolls;
};

#define TEST(_handler, _rerolls)                                                         \
	{                                                                                    \
		.name = #_handler, .test_handler = _handler, .rerolls = _rerolls                 \
	}

const struct test tests[] = {
	// TEST(z_identity, 1U),
	TEST(z_macro__CANIOT_DID, 100U),
	TEST(z_macro__CANIOT_DEVICE_IS_BROADCAST, 1),
	TEST(z_macro__CANIOT_ID, 100U),
	TEST(z_func__caniot_id_to_canid, 100U),
	TEST(z_struct__caniot_id_t, 1U),
	TEST(z_misc_id_conversion, 100U),
	TEST(z_func__caniot_device_is_target, 100U),
	TEST(z_func__caniot_resp_error_for, 1U),
	TEST(z_func__caniot_validate_drivers_api, 1U),
	TEST(z_func__caniot_device_get_filter, 100U),
	TEST(z_func__caniot_device_get_mask, 1U),
	TEST(z_func__caniot_device_get_filter_broadcast, 1U),
	TEST(z_func___si_caniot_device_get_filter, 100U),
	TEST(z_func___si_caniot_device_get_filter_broadcast, 1u),
	TEST(z_func_ctrl1, 1U),
	TEST(z_func_ctrl2, 1U),
	TEST(z_func_ctrl3, 1U),
	TEST(z_func_ctrl4, 1U),
	TEST(z_func_dev0, 1U),
	TEST(z_func_datatype_blc_sys, 1U),
	TEST(z_func_datatype_tmperature, 100u),
	TEST(z_func_decode_caniot_blc0_command, 1u),
	TEST(z_func_encode_caniot_blc0_command, 1u),
	TEST(z_func_encode_caniot_blc1_command, 1u),
	TEST(z_func_decode_caniot_blc1_command, 1u),
};

int main(void)
{
	uint32_t tests_runned = 0u;
	uint32_t tests_failed = 0u;

	srand(SEED);

	for (size_t i = 0; i < ARRAY_SIZE(tests); i++) {
		tests_runned++;

		const struct test *tst = &tests[i];
		if (tst->test_handler == NULL) {
			printf("%lu:\tHANDLER IS NULL\n", i);
			continue;
		}

		size_t sucesses = 0U;
		size_t failures = 0U;

		for (size_t j = 0; j < tst->rerolls; j++) {
			if (tst->test_handler() == true) {
				sucesses++;
			} else {
				failures++;
			}
		}

		const bool success = failures == 0U;
		printf("%lu:\t%s %zu/%zu  \t(%.1f %%) -- %s\n",
			   i,
			   success ? "OK" : "NOK",
			   sucesses,
			   tst->rerolls,
			   (sucesses * 100.0) / tst->rerolls,
			   tst->name);

		if (!success) tests_failed++;
	}

	printf("\n========================================");
	printf("\nTests runned: %u failed: %u\n", tests_runned, tests_failed);
	printf("========================================\n");
}