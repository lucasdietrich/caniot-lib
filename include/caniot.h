#ifndef _CANIOT_H
#define _CANIOT_H

#include <stdint.h>

void caniot_input(void);

#define ciot_dev struct caniot_device

enum { command = 0, telemetry = 1, attribute = 2, _unused = 3 };

struct id
{
        uint32_t type: 2;
        uint32_t device: 9;
        uint32_t _unused: 18;
};

enum { controller = 0, sensor = 1, actuator = 2, full_device = 3};

struct deviceid
{
        uint8_t type: 2;
        uint8_t lightning: 1;
        uint8_t heating: 1;
        uint8_t presence: 1;
        uint8_t contact: 1;
        uint8_t unique: 3;
};

struct can_msg
{
        uint32_t id;
        uint8_t rtr;
        uint8_t ext;

        uint8_t len;
        uint8_t buffer[8];
};

struct caniot_identification
{
        uint8_t nodeid;
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
        int (*update_time)(ciot_dev *dev, uint32_t ts);
        int (*update_config)(ciot_dev *dev, struct caniot_config *cfg);

        int (*scheduled_handler)(ciot_dev *dev, struct caniot_scheduled *sch);

        int (*read_attribute)(ciot_dev *dev, uint16_t key, uint32_t *val);
        int (*write_attribute)(ciot_dev *dev, uint16_t key, uint32_t *val);
        int (*command)(ciot_dev *dev, char *buf, uint8_t len);
        int (*telemetry)(ciot_dev *dev, char *buf, uint8_t *len);
};

#endif