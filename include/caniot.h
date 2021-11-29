#ifndef _CANIOT_H
#define _CANIOT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MEMBER_SIZEOF(s, member)	(sizeof(((s *)0)->member))
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#define CONTAINER_OF(ptr, type, field) ((type *)(((char *)(ptr)) - offsetof(type, field)))
#define MIN(a, b) (a < b ? a : b)

// custom errors codes

struct deviceid
{
	uint8_t cls : 3;
	uint8_t dev : 3;
};

enum { command = 0, telemetry = 1, write_attribute = 3, read_attribute = 2 };
enum { query = 0, response = 1 };

/* https://stackoverflow.com/questions/7957363/effects-of-attribute-packed-on-nested-array-of-structures */
struct caniot_id {
        uint16_t type : 2;
        uint16_t query : 1;
        uint16_t cls: 3;
        uint16_t dev: 3;
        uint16_t endpoint : 2;
};


struct caniot_attribute
{
        uint16_t key;
        uint32_t val;
};

struct caniot_frame {
        struct caniot_id id;
        union {
		char buf[8];
                struct caniot_attribute attr;
		int32_t err;
        };
        uint8_t len;
};

struct caniot_filter
{
	uint32_t filter;
	uint32_t mask;
	uint8_t ext;
};

struct caniot_identification
{
	union {
		struct deviceid node;
		uint8_t nodeid;
	};
        uint16_t version;
        char name[32];
};

struct caniot_system
{
        uint32_t uptime;
        uint32_t abstime;
        uint32_t calculated_abstime;
        uint32_t uptime_shift;
        uint32_t last_telemetry;
        struct {
                uint32_t total;
                uint32_t read_attribute;
                uint32_t write_attribute;
                uint32_t command;
                uint32_t request_telemetry;
                uint32_t processed;
                uint32_t query_failed;
        } received;
        struct {
                uint32_t total;
                uint32_t telemetry;
        } sent;
        struct {
                uint32_t total;

        } events;
        int32_t last_query_error;
        int32_t last_telemetry_error;
        int32_t last_event_error;
        int32_t battery;
};

struct caniot_config
{
        uint32_t telemetry_period;
        uint32_t telemetry_rdm_delay;
        uint32_t telemetry_min;
	
	struct {
		uint8_t error_response: 1;
	};
};

struct caniot_scheduled
{
        uint8_t days;
        uint8_t time;
        uint32_t command;
};

struct caniot_drivers_api {
	int (*send)(struct caniot_frame *frame, uint32_t delay); /* TX queue */
	int (*recv)(struct caniot_frame *frame); /* RX queue */

	int (*set_filter) (struct caniot_filter *filter);
};

struct caniot_device
{
        struct caniot_identification identification;
        struct caniot_system system;
        struct caniot_config config;
        struct caniot_scheduled scheduled[32];
        
        const struct caniot_api *api;
	const struct caniot_drivers_api *driv;
};

struct caniot_api
{
	/* called when time is updated with new time */
        int (*update_time)(struct caniot_device *dev, uint32_t ts);

	/* called when configuration is updated */
        int (*update_config)(struct caniot_device *dev);

        int (*scheduled_handler)(struct caniot_device *dev, struct caniot_scheduled *sch);

        int (*read_attribute)(struct caniot_device *dev, uint16_t key, uint32_t *val);
        int (*write_attribute)(struct caniot_device *dev, uint16_t key, uint32_t val);

	/* Handle command */
        int (*command_handler)(struct caniot_device *dev, uint8_t ep, char *buf, uint8_t len);

	/* Build telemetry */
        int (*telemetry)(struct caniot_device *dev, uint8_t ep, char *buf, uint8_t *len);

	bool (*pending_telemetry)(struct caniot_device *dev);

	void (*entropy)(uint8_t *buf, size_t len);
};

int caniot_process_rx_frame(struct caniot_device *dev,
			    struct caniot_frame *req,
			    struct caniot_frame *resp);

int caniot_process(struct caniot_device *dev);

#endif