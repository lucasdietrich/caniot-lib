#ifndef _CANIOT_H
#define _CANIOT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <caniot/errors.h>

#ifdef CONFIG_CANIOT_DRIVERS_API
#define CANIOT_DRIVERS_API CONFIG_CANIOT_DRIVERS_API
#else
#define CANIOT_DRIVERS_API 0
#endif /* CONFIG_CANIOT_DRIVERS_API */


#define CANIOT_VERSION1	1
#define CANIOT_VERSION2 2
#define CANIOT_VERSION 	CANIOT_VERSION2

#define CANIOT_MAX_PENDING_QUERIES	2
#define CANIOT_ID(t, q, c, d, e) ((t & 0x3U) | ((q & 0x1U) << 2U) | ((c & 0x7U) << 3U) | ((d & 0x7U) << 6U) | ((e & 0x3U) << 9U))


#define CANIOT_CLASS_BROADCAST (0x7U)
#define CANIOT_SUBID_BROADCAST (0x7U)

#define CANIOT_DID_MAX_VALUE CANIOT_DID_BROADCAST
#define CANIOT_DID_MIN_VALUE (0x00U)

#define CANIOT_DID(class_id, sub_id)	((caniot_did_t) ((class_id) & 0x7U) | (((sub_id) & 0x7U) << 3U))
#define CANIOT_DID_FROM_RAW(raw) ((raw) & CANIOT_DID_BROADCAST)
#define CANIOT_DID_CLS(did) ((caniot_device_class_t) ((did) & 0x7U))
#define CANIOT_DID_SID(did) ((caniot_device_subid_t) (((did) >> 3U) & 0x7U))

#define CANIOT_DID_BROADCAST CANIOT_DID(CANIOT_CLASS_BROADCAST, 0x7U)

#define CANIOT_DID_EQ(did1, did2) (((did1) & CANIOT_DID_MAX_VALUE) == ((did2) & CANIOT_DID_MAX_VALUE))

#define CANIOT_DEVICE_IS_BROADCAST(did) CANIOT_DID_EQ(did, CANIOT_DID_BROADCAST)

/* milliseconds */
#define CANIOT_TELEMETRY_DELAY_MIN_DEFAULT	0
#define CANIOT_TELEMETRY_DELAY_MAX_DEFAULT	100
#define CANIOT_TELEMETRY_DELAY_DEFAULT		5

/* seconds */
#define CANIOT_TELEMETRY_PERIOD_DEFAULT		60

#define CANIOT_TELEMETRY_ENDPOINT_DEFAULT	CANIOT_ENDPOINT_BOARD_CONTROL

#define CANIOT_TIMEZONE_DEFAULT			3600
#define CANIOT_LOCATION_REGION_DEFAULT		{'E', 'U'}
#define CANIOT_LOCATION_COUNTRY_DEFAULT		{'F', 'R'}

#define CANIOT_ID_GET_TYPE(id) ((caniot_frame_type_t) (id & 0x3))
#define CANIOT_ID_GET_QUERY(id) ((caniot_frame_dir_t) ((id >> 2) & 0x1))
#define CANIOT_ID_GET_CLASS(id) ((caniot_device_class_t) ((id >> 3) & 0x7))
#define CANIOT_ID_GET_SUBID(id) ((caniot_device_subid_t) ((id >> 6) & 0x7))
#define CANIOT_ID_GET_ENDPOINT(id) ((caniot_endpoint_t) ((id >> 9) & 0x3))

#define CANIOT_ADDR_LEN sizeof("0x3f")


typedef enum
{
	CANIOT_DEVICE_CLASS0 = 0,
	CANIOT_DEVICE_CLASS1,
	CANIOT_DEVICE_CLASS2,
	CANIOT_DEVICE_CLASS3,
	CANIOT_DEVICE_CLASS4,
	CANIOT_DEVICE_CLASS5,
	CANIOT_DEVICE_CLASS6,
	CANIOT_DEVICE_CLASS7,
} caniot_device_class_t;

typedef enum
{
	CANIOT_DEVICE_SID0 = 0,
	CANIOT_DEVICE_SID1,
	CANIOT_DEVICE_SID2,
	CANIOT_DEVICE_SID3,
	CANIOT_DEVICE_SID4,
	CANIOT_DEVICE_SID5,
	CANIOT_DEVICE_SID6,
	CANIOT_DEVICE_SID7,
} caniot_device_subid_t;

typedef enum
{
	CANIOT_FRAME_TYPE_COMMAND = 0,
	CANIOT_FRAME_TYPE_TELEMETRY = 1,
	CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE = 2,
	CANIOT_FRAME_TYPE_READ_ATTRIBUTE = 3,
} caniot_frame_type_t;

typedef enum
{
	CANIOT_QUERY = 0,
	CANIOT_RESPONSE = 1,
} caniot_frame_dir_t;

typedef enum {
	CANIOT_ENDPOINT_APP = 0,
	CANIOT_ENDPOINT_1 = 1,
	CANIOT_ENDPOINT_2 = 2,
	CANIOT_ENDPOINT_BOARD_CONTROL = 3,
} caniot_endpoint_t;

typedef uint8_t caniot_did_t;

/* https://stackoverflow.com/questions/7957363/effects-of-attribute-packed-on-nested-array-of-structures */
typedef struct {
	caniot_frame_type_t type : 2U;
	caniot_frame_dir_t query : 1U;
	caniot_device_class_t cls : 3U;
	caniot_device_subid_t sid : 3U;
	caniot_endpoint_t endpoint : 2U;
} caniot_id_t;

struct caniot_attribute
{
	union {
		uint16_t key;
		struct {
			uint16_t section : 4;
			uint16_t attribute : 8;
			uint16_t part : 4;
		};
	};
	union {
		uint32_t u32;
		uint16_t u16;
		uint8_t u8;
		uint32_t val;
	};
};

struct caniot_frame {
	caniot_id_t id;
	union {
		char buf[8];
		struct caniot_attribute attr;
		int32_t err;
	};
	uint8_t len;
};

typedef struct caniot_frame caniot_frame_t;

typedef int (*caniot_query_callback_t)(caniot_did_t did,
				       const struct caniot_frame *resp);

struct caniot_drivers_api {
	/* util */
	void (*entropy)(uint8_t *buf, size_t len);
	void (*get_time)(uint32_t *sec, uint16_t *ms);
	void (*set_time)(uint32_t sec);
	/**
	 * @brief Send a CANIOT frame
	 *
	 * Return 0 on success, any other value on error.
	 */
	int (*send)(const struct caniot_frame *frame, uint32_t delay_ms);

	/**
	 * @brief Receive a CANIOT frame.
	 *
	 * Return 0 on success, -EAGAIN if no frame is available.
	 */
	int (*recv)(struct caniot_frame *frame);
};

// Return if deviceid is broadcast
static inline bool caniot_is_broadcast(caniot_did_t id)
{
	return CANIOT_DEVICE_IS_BROADCAST(id);
}

bool caniot_device_is_target(caniot_did_t did,
			     const struct caniot_frame *frame);

bool caniot_controller_is_target(const struct caniot_frame *frame);

static inline void caniot_clear_frame(struct caniot_frame *frame)
{
	memset(frame, 0x00U, sizeof(struct caniot_frame));
}

static inline void caniot_copy_frame(struct caniot_frame *dst,
				     const struct caniot_frame *src)
{
	memcpy(dst, src, sizeof(struct caniot_frame));
}

static inline bool caniot_is_error_frame(caniot_id_t id)
{
	return (id.query == CANIOT_RESPONSE) &&
		(id.type == CANIOT_FRAME_TYPE_COMMAND);
}

static inline bool is_telemetry_response(struct caniot_frame *frame)
{
	return (frame->id.query == CANIOT_RESPONSE) &&
		(frame->id.type == CANIOT_FRAME_TYPE_TELEMETRY);
}

// Check if drivers api is valid
bool caniot_validate_drivers_api(struct caniot_drivers_api *api);

void caniot_show_deviceid(caniot_did_t did);

void caniot_show_frame(const struct caniot_frame *frame);

void caniot_explain_id(caniot_id_t id);

void caniot_explain_frame(const struct caniot_frame *frame);

int caniot_explain_id_str(caniot_id_t id, char *buf, size_t len);

int caniot_explain_frame_str(const struct caniot_frame *frame, char *buf, size_t len);

void caniot_build_query_telemetry(struct caniot_frame *frame,
				  caniot_did_t did,
				  uint8_t endpoint);

void caniot_build_query_command(struct caniot_frame *frame,
				caniot_did_t did,
				uint8_t endpoint,
				const uint8_t *buf,
				uint8_t size);

bool caniot_is_error(int cterr);

void caniot_show_error(int cterr);

int caniot_encode_deviceid(caniot_did_t did, uint8_t *buf, size_t len);

bool caniot_deviceid_equal(caniot_did_t a, caniot_did_t b);

/**
 * @brief Compare CANIOT device addresses
 *
 * @param a First CANIOT device address to compare
 * @param b Second CANIOT device address to compare
 * @return int negative value if @a a < @a b, 0 if @a a == @a b, else positive
 */
int caniot_deviceid_cmp(caniot_did_t a, caniot_did_t b);

/**
 * @brief Convert CAN id to caniot ID format
 *
 * @param id
 * @return uint16_t
 */
static inline uint16_t caniot_id_to_canid(caniot_id_t id)
{
	return CANIOT_ID(id.type, id.query, id.cls, id.sid, id.endpoint);
}

/**
 * @brief Convert CANIOT id to caniot ID format
 *
 * Note: Architecture agnostic.
 *
 * @param canid
 * @return caniot_id_t
 */
static inline caniot_id_t caniot_canid_to_id(uint16_t canid)
{
	caniot_id_t id = {
		.type = CANIOT_ID_GET_TYPE(canid),
		.query = CANIOT_ID_GET_QUERY(canid),
		.cls = CANIOT_ID_GET_CLASS(canid),
		.sid = CANIOT_ID_GET_SUBID(canid),
		.endpoint = CANIOT_ID_GET_ENDPOINT(canid),
	};

	return id;
}

void caniot_test(void);

#endif