#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

#include <time.h>

#include <caniot/device.h>
#include <caniot/controller.h>
#include <caniot/archutils.h>

#define SEED 0

#define TRUE true
#define FALSE false



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

/*___________________________________________________________________________*/

	// caniot_frame_type_t type = r8() & 0x3U;
	// caniot_frame_dir_t dir = r8() & 0x1U;
	// caniot_device_class_t cls = r8() & CANIOT_CLASS_BROADCAST;
	// caniot_device_subid_t sid = r8() & CANIOT_SUBID_BROADCAST;
	// caniot_endpoint_t ep = r8() & 0x3U;

/*___________________________________________________________________________*/

bool z_identity(void)
{
	return true;
}

bool z_macro__CANIOT_DID(void)
{
	const uint8_t cls = r8() & CANIOT_CLASS_BROADCAST;
	const uint8_t sid = r8() & CANIOT_SUBID_BROADCAST;

	const caniot_did_t did = CANIOT_DID(cls, sid);

	return (CANIOT_DID_CLS(did) == cls) &&
		(CANIOT_DID_SID(did) == sid);
}

bool z_macro__CANIOT_DEVICE_IS_BROADCAST(void)
{
	const caniot_did_t did = CANIOT_DID(CANIOT_CLASS_BROADCAST,
					    CANIOT_SUBID_BROADCAST);

	return CANIOT_DID_EQ(did, CANIOT_DID_BROADCAST);
}

static const caniot_id_t gen_rdm_id(void)
{
	const caniot_id_t id = {
		.type = r8(),
		.query = r8(),
		.cls = r8(),
		.sid = r8(),
		.endpoint = r8()
	};

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
	const caniot_id_t id = {
		.type = CANIOT_FRAME_TYPE_READ_ATTRIBUTE,
		.query = CANIOT_RESPONSE,
		.cls = CANIOT_CLASS_BROADCAST,
		.sid = CANIOT_SUBID_BROADCAST,
		.endpoint = CANIOT_ENDPOINT_BOARD_CONTROL
	};

	uint16_t canid = caniot_id_to_canid(id);

	return canid = 0x3FU;
}

bool z_misc_id_conversion(void)
{
	const caniot_id_t id = gen_rdm_id();

	const uint16_t canid = caniot_id_to_canid(id);

	const caniot_id_t id2 = caniot_canid_to_id(canid);

	return (id.type == id2.type) &&
		(id.query == id2.query) &&
		(id.cls == id2.cls) &&
		(id.sid == id2.sid) &&
		(id.endpoint == id2.endpoint);
}

static const caniot_did_t gen_rdm_did(void)
{
	return CANIOT_DID(r8() & CANIOT_CLASS_BROADCAST,
			  r8() & CANIOT_SUBID_BROADCAST);
}

bool z_func__caniot_device_is_target(void)
{
	const caniot_did_t did = gen_rdm_did();

	const struct caniot_frame fordev = {
		.id = {
			.cls = CANIOT_DID_CLS(did),
			.sid = CANIOT_DID_SID(did),
			.query = CANIOT_QUERY,
			.type = CANIOT_FRAME_TYPE_READ_ATTRIBUTE,
		},
		.len = 0,
	};

	const struct caniot_frame notfordev = {
		.id = {
			.cls = CANIOT_DID_CLS(did),
			.sid = CANIOT_DID_SID(did),
			.query = CANIOT_RESPONSE,
			.type = CANIOT_FRAME_TYPE_READ_ATTRIBUTE,
		},
		.len = 0,
	};

	return caniot_device_is_target(did, &fordev) &&
		!caniot_device_is_target(did, &notfordev);
}


bool z_func__caniot_type_is_valid_response_of(void)
{
	bool all = true;

	struct {
		caniot_frame_type_t resp;
		caniot_frame_type_t req;
		bool result;
	} tests[] = {
		{CANIOT_FRAME_TYPE_TELEMETRY, CANIOT_FRAME_TYPE_TELEMETRY, true},
		{CANIOT_FRAME_TYPE_TELEMETRY, CANIOT_FRAME_TYPE_COMMAND, true},
		{CANIOT_FRAME_TYPE_COMMAND, CANIOT_FRAME_TYPE_TELEMETRY, false},
		{CANIOT_FRAME_TYPE_COMMAND, CANIOT_FRAME_TYPE_COMMAND, false},

		{CANIOT_FRAME_TYPE_READ_ATTRIBUTE, CANIOT_FRAME_TYPE_READ_ATTRIBUTE, true},
		{CANIOT_FRAME_TYPE_READ_ATTRIBUTE, CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE, true},
		{CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE, CANIOT_FRAME_TYPE_READ_ATTRIBUTE, false},
		{CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE, CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE, false},

		{CANIOT_FRAME_TYPE_READ_ATTRIBUTE, CANIOT_FRAME_TYPE_TELEMETRY, false},
		{CANIOT_FRAME_TYPE_TELEMETRY, CANIOT_FRAME_TYPE_READ_ATTRIBUTE, false},

		{CANIOT_FRAME_TYPE_READ_ATTRIBUTE, CANIOT_FRAME_TYPE_COMMAND, false},
		{CANIOT_FRAME_TYPE_COMMAND, CANIOT_FRAME_TYPE_READ_ATTRIBUTE, false},

		{CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE, CANIOT_FRAME_TYPE_COMMAND, false},
		{CANIOT_FRAME_TYPE_COMMAND, CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE, false},

		{CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE, CANIOT_FRAME_TYPE_COMMAND, false},
		{CANIOT_FRAME_TYPE_COMMAND, CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE, false},
	};

	for (size_t i = 0; i < ARRAY_SIZE(tests); i++) {
		all &= caniot_type_is_valid_response_of(tests[i].resp,
							tests[i].req) == tests[i].result;

		if (!all) {
			break;
		}
	}

	return all;
}

bool z_func__caniot_type_is_response_of(void)
{
	bool all = true;

	struct {
		caniot_frame_type_t resp;
		caniot_frame_type_t req;
		bool result;
		bool iserr;
	} tests[] = {
		{CANIOT_FRAME_TYPE_TELEMETRY, CANIOT_FRAME_TYPE_TELEMETRY, true, false},
		{CANIOT_FRAME_TYPE_TELEMETRY, CANIOT_FRAME_TYPE_COMMAND, true, false},
		{CANIOT_FRAME_TYPE_COMMAND, CANIOT_FRAME_TYPE_TELEMETRY, false, true},
		{CANIOT_FRAME_TYPE_COMMAND, CANIOT_FRAME_TYPE_COMMAND, false, true},

		{CANIOT_FRAME_TYPE_READ_ATTRIBUTE, CANIOT_FRAME_TYPE_READ_ATTRIBUTE, true, false},
		{CANIOT_FRAME_TYPE_READ_ATTRIBUTE, CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE, true, false},
		{CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE, CANIOT_FRAME_TYPE_READ_ATTRIBUTE, false, true},
		{CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE, CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE, false, true},

		{CANIOT_FRAME_TYPE_READ_ATTRIBUTE, CANIOT_FRAME_TYPE_TELEMETRY, false, false},
		{CANIOT_FRAME_TYPE_TELEMETRY, CANIOT_FRAME_TYPE_READ_ATTRIBUTE, false, false},

		{CANIOT_FRAME_TYPE_READ_ATTRIBUTE, CANIOT_FRAME_TYPE_COMMAND, false, false},
		{CANIOT_FRAME_TYPE_COMMAND, CANIOT_FRAME_TYPE_READ_ATTRIBUTE, false, false},

		{CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE, CANIOT_FRAME_TYPE_COMMAND, false, false},
		{CANIOT_FRAME_TYPE_COMMAND, CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE, false, false},

		{CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE, CANIOT_FRAME_TYPE_COMMAND, false, false},
		{CANIOT_FRAME_TYPE_COMMAND, CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE, false, false},
	};

	bool iserr;

	for (size_t i = 0; i < ARRAY_SIZE(tests); i++) {
		all &= caniot_type_is_response_of(tests[i].resp,
						  tests[i].req,
						  &iserr) == tests[i].result;
		all &= iserr == tests[i].iserr;

		if (!all) {
			break;
		}
	}

	return all;
}

bool z_func__caniot_resp_error_for(void)
{
	bool all = true;

	all &= caniot_resp_error_for(CANIOT_FRAME_TYPE_TELEMETRY) == CANIOT_FRAME_TYPE_COMMAND;
	all &= caniot_resp_error_for(CANIOT_FRAME_TYPE_COMMAND) == CANIOT_FRAME_TYPE_COMMAND;
	all &= caniot_resp_error_for(CANIOT_FRAME_TYPE_READ_ATTRIBUTE) == CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE;
	all &= caniot_resp_error_for(CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE) == CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE;

	return all;
};

bool z_func__caniot_validate_drivers_api(void)
{
	struct caniot_drivers_api api = {
		.entropy = NULL,
		.get_time = NULL,
		.recv = NULL,
		.send = NULL,
		.set_time = NULL
	};

	return caniot_validate_drivers_api(&api) == false;
}

bool z_func__caniot_device_get_filter(void)
{
	const caniot_did_t did = gen_rdm_did();
	const uint16_t filter = caniot_device_get_filter(did);

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
	const caniot_did_t did = gen_rdm_did();
	
	const uint16_t filter = caniot_device_get_filter_broadcast(did);

	return (CANIOT_ID_GET_CLASS(filter) == CANIOT_CLASS_BROADCAST) &&
		(CANIOT_ID_GET_SUBID(filter) == CANIOT_SUBID_BROADCAST) &&
		(CANIOT_ID_GET_QUERY(filter) == CANIOT_QUERY);
}

bool z_func___si_caniot_device_get_filter(void)
{
	const caniot_did_t did = gen_rdm_did();

	return _si_caniot_device_get_filter(did) == 
		caniot_device_get_filter(did);
}

bool z_func___si_caniot_device_get_filter_broadcast(void)
{
	const caniot_did_t did = gen_rdm_did();

	return _si_caniot_device_get_filter_broadcast(did) == 
		caniot_device_get_filter_broadcast(did);
}

/*___________________________________________________________________________*/

struct test
{
	bool (*test_handler)(void);

	size_t rerolls;
};

#define TEST(_handler, _rerolls) \
	{ \
		.test_handler = _handler, \
		.rerolls = _rerolls \
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
	TEST(z_func__caniot_type_is_valid_response_of, 1U),
	TEST(z_func__caniot_type_is_response_of, 1U),
	TEST(z_func__caniot_resp_error_for, 1U),
	TEST(z_func__caniot_validate_drivers_api, 1U),
	TEST(z_func__caniot_device_get_filter, 100U),
	TEST(z_func__caniot_device_get_mask, 1U),
	TEST(z_func__caniot_device_get_filter_broadcast, 1U),
	TEST(z_func___si_caniot_device_get_filter, 100U),
	TEST(z_func___si_caniot_device_get_filter_broadcast, 100U),
};


int main(void)
{
	srand(SEED);

	for (size_t i = 0; i < ARRAY_SIZE(tests); i++) {
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
		printf("%lu:\t%s %zu/%zu  \t(%.1f %%)\n", i, success ? "OK" : "NOK",
		       sucesses, tst->rerolls, (sucesses * 100.0) / tst->rerolls);
	}
}