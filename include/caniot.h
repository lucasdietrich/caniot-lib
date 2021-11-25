#ifndef _CANIOT_H
#define _CANIOT_H

#include <stddef.h>
#include <stdint.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#define CONTAINER_OF(ptr, type, field) ((type *)(((char *)(ptr)) - offsetof(type, field)))

void caniot_input(void);

// custom errors codes

struct deviceid
{
        uint8_t cls: 3;
        uint8_t dev: 3;
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

struct caniot_identification
{
        struct deviceid nodeid;
        uint16_t version;
        char name[32];
};

struct caniot_system
{
        uint32_t uptime;
        uint32_t abstime;
        // uint32_t calculated_abstime;
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
};

struct caniot_scheduled
{
        uint8_t days;
        uint8_t time;
        uint32_t command;
};

struct caniot_device
{
        struct caniot_identification identification;
        struct caniot_system system;
        struct caniot_config config;
        struct caniot_scheduled scheduled[32];
        
        const struct caniot_api *api;
};

struct caniot_api
{
        int (*update_time)(struct caniot_device *dev, uint32_t ts);
        int (*update_config)(struct caniot_device *dev, struct caniot_config *cfg);

        int (*scheduled_handler)(struct caniot_device *dev, struct caniot_scheduled *sch);

        int (*read_attribute)(struct caniot_device *dev, uint16_t key, uint32_t *val);
        int (*write_attribute)(struct caniot_device *dev, uint16_t key, uint32_t val);

	/* Handle command */
        int (*command)(struct caniot_device *dev, uint8_t ep, char *buf, uint8_t len);

	/* Build telemetry */
        int (*telemetry)(struct caniot_device *dev, uint8_t ep, char *buf, uint8_t *len);
};

int caniot_process_rx_frame(struct caniot_device *dev,
			    struct caniot_frame *req,
			    struct caniot_frame *resp);

int caniot_telemetry_resp(struct caniot_device *dev,
			  struct caniot_frame *resp,
			  uint8_t ep);

int caniot_attribute_resp(struct caniot_device *dev,
			  struct caniot_frame *resp,
			  uint16_t key);
			  
#endif