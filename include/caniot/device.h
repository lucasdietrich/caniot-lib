#ifndef _CANIOT_DEVICE_H
#define _CANIOT_DEVICE_H

#include <caniot/caniot.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct caniot_identification
{
	caniot_did_t did;
	uint16_t version;
	char name[32];
	uint32_t magic_number;
} __attribute__((packed));

struct caniot_system
{
	uint32_t uptime_synced;
	uint32_t time;
	uint32_t uptime;
	uint32_t start_time;
	uint32_t last_telemetry;
	struct {
		uint32_t total;
		uint32_t read_attribute;
		uint32_t write_attribute;
		uint32_t command;
		uint32_t request_telemetry;
		uint32_t _unused2;
		uint32_t _unused3;
	} received;
	struct {
		uint32_t total;
		uint32_t telemetry;
	} sent;
	uint32_t _unused4;
	int16_t last_command_error;
	int16_t last_telemetry_error;
	int16_t _unused5;
	uint8_t battery;
} __attribute__((packed));

struct caniot_class0_config
{
	/* Duration in seconds of the pulse for OC1, OC2, RL1, RL2
	 * respectively. */
	uint32_t pulse_durations[4u];

	/* Output default values for OC1, OC2, RL1, RL2 respectively. */
	uint32_t outputs_default;

	/* The mask gpio to be used for notifications. */
	uint32_t telemetry_on_change;
} __attribute__((packed));

struct caniot_class1_config
{
	/* Duration in seconds of the pulse for all outputs. */
	uint32_t pulse_durations[20u]; /* Last memory space is not used */

	/* Directions */
	uint32_t directions; /* 0 = input, 1 = output */

	/* Output default values for OC1, OC2, RL1, RL2 respectively. */
	uint32_t outputs_default;

	/* The mask gpio to be used for notifications. */
	uint32_t telemetry_on_change;
} __attribute__((packed));

struct caniot_config
{
	struct {
		uint32_t period; /* period in seconds */
		union {
			uint16_t delay_min; /* minimum in milliseconds */
			uint16_t delay;     /* delay in milliseconds */
		};
		uint16_t delay_max; /* maximum in milliseconds */
	} telemetry;

	struct {
		uint8_t error_response : 1;
		uint8_t telemetry_delay_rdm : 1;
		uint8_t telemetry_endpoint : 2;
	} flags;

	int32_t timezone;

	struct {
		char region[2];
		char country[2];
	} location;

	/* TODO Use different structure to represent different classes */
	union {
		struct caniot_class0_config cls0_gpio;
		struct caniot_class1_config cls1_gpio;
	};
	
} __attribute__((packed));

struct caniot_device
{
	const struct caniot_identification *identification;
	struct caniot_system system;
	struct caniot_config *config;

	const struct caniot_api *api;

#if CANIOT_DRIVERS_API
	const struct caniot_drivers_api *driv;
#endif

	struct {
		uint8_t request_telemetry : 1;
		uint8_t initialized: 1;
	} flags;
};

typedef int (caniot_telemetry_handler_t)(struct caniot_device *dev,
					 caniot_endpoint_t ep,
					 char *buf,
					 uint8_t *len);

typedef int (caniot_command_handler_t)(struct caniot_device *dev,
				       caniot_endpoint_t ep,
				       const char *buf,
				       uint8_t len);

struct caniot_api
{
	struct {
		/* called before configuration will be read */
		int (*on_read)(struct caniot_device *dev,
			       struct caniot_config *config);

		/* called after configuration is updated */
		int (*on_write)(struct caniot_device *dev,
				struct caniot_config *config);
	} config;

	struct {
		int (*read)(struct caniot_device *dev,
			    uint16_t key,
			    uint32_t *val);
		int (*write)(struct caniot_device *dev,
			     uint16_t key,
			     uint32_t val);
	} custom_attr;

	/* Handle command */
	caniot_command_handler_t *command_handler;

	/* Build telemetry */
	caniot_telemetry_handler_t *telemetry_handler;
};

void caniot_print_device_identification(const struct caniot_device *dev);

int caniot_device_system_reset(struct caniot_device *dev);

int caniot_device_handle_rx_frame(struct caniot_device *dev,
				  const struct caniot_frame *req,
				  struct caniot_frame *resp);

caniot_did_t caniot_device_get_id(struct caniot_device *dev);

uint32_t caniot_device_telemetry_remaining(struct caniot_device *dev);

static inline uint16_t caniot_device_get_mask(void)
{
	return 0x1fc; // 0b00111111100U;
}

uint16_t caniot_device_get_filter(caniot_did_t did);

uint16_t caniot_device_get_filter_broadcast(caniot_did_t did);

/**
 * @brief static-inline version of caniot_device_get_filter
 * 
 * @param did 
 * @return uint16_t 
 */
static inline uint16_t _si_caniot_device_get_filter(caniot_did_t did)
{
	return CANIOT_ID(0U, CANIOT_QUERY, CANIOT_DID_CLS(did), CANIOT_DID_SID(did), 0U);
}

/**
 * @brief static-inline version of caniot_device_get_filter_broadcast
 * 
 * @param did 
 * @return uint16_t 
 */
static inline uint16_t _si_caniot_device_get_filter_broadcast(caniot_did_t did)
{
	return CANIOT_ID(0U, CANIOT_QUERY, CANIOT_DID_CLS(CANIOT_DID_BROADCAST), 
			 CANIOT_DID_SID(CANIOT_DID_BROADCAST), 0U);
}

/*____________________________________________________________________________*/

void caniot_app_init(struct caniot_device *dev);

/**
 * @brief Receive incoming CANIOT message if any and handle it
 *
 * @param dev
 * @return int
 */
int caniot_device_process(struct caniot_device *dev);

int caniot_device_scales_rdmdelay(struct caniot_device *dev,
				  uint32_t *rdmdelay);

bool caniot_device_time_synced(struct caniot_device *dev);

/*____________________________________________________________________________*/

/**
 * @brief Verify if device is properly defined
 *
 * @param dev
 * @return int
 */
int caniot_device_verify(struct caniot_device *dev);

/*____________________________________________________________________________*/

#define CANIOT_CONFIG_DEFAULT_INIT() \
{ \
	.telemetry = { \
		.period = CANIOT_TELEMETRY_PERIOD_DEFAULT,  \
		.delay_min = CANIOT_TELEMETRY_DELAY_MIN_DEFAULT,  \
		.delay_max = CANIOT_TELEMETRY_DELAY_MAX_DEFAULT,  \
	},  \
	.flags = { \
		.error_response = 1u,  \
		.telemetry_delay_rdm = 1u,  \
		.telemetry_endpoint = CANIOT_TELEMETRY_ENDPOINT_DEFAULT  \
	},  \
	.timezone = CANIOT_TIMEZONE_DEFAULT,  \
	.location = { \
		.region = CANIOT_LOCATION_REGION_DEFAULT,  \
		.country = CANIOT_LOCATION_COUNTRY_DEFAULT,  \
	},  \
	.cls0_gpio = { \
		.pulse_durations = { \
			[0] = 0u,  \
			[1] = 0u,  \
			[2] = 0u,  \
			[3] = 0u,  \
		},  \
		.outputs_default = 0u, \
		.telemetry_on_change = 0xFFFFFFFFlu, \
	}\
}

#define CANIOT_API_FULL_INIT(cmd, tlm, cfgr, cfgw, attr, attw) \
{ \
	.config = { \
		.on_read = cfgr,  \
		.on_write = cfgw  \
	},  \
	.custom_attr = { \
		.read = attr,  \
		.write = attw  \
	},  \
	.command_handler = cmd,  \
	.telemetry_handler = tlm  \
}

#define CANIOT_API_STD_INIT(cmd, tlm, cfgr, cfgw) \
	CANIOT_API_FULL_INIT(cmd, tlm, cfgr, cfgw, NULL, NULL)

#define CANIOT_API_CFG_INIT(cmd, tlm, cfgr, cfgw) \
	CANIOT_API_STD_INIT(cmd, tlm, cfgr, cfgw)

#define CANIOT_API_MIN_INIT(cmd, tlm) \
	CANIOT_API_CFG_INIT(cmd, tlm, NULL, NULL)

#ifdef __cplusplus
}
#endif

#endif /* _CANIOT_DEVICE_H */