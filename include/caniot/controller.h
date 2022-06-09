#ifndef _CANIOT_CONTROLLER_H
#define _CANIOT_CONTROLLER_H

#include "caniot.h"

struct caniot_device_entry
{
	uint32_t last_seen;	/* timestamp this device was last seen */
	struct {
		uint8_t active : 1;	/* valid entry if set */
		uint8_t pending : 1;	/* query pending */
	} flags;
};

/* see https://github.com/lucasdietrich/AVRTOS/blob/master/src/avrtos/dstruct/tqueue.c */
struct caniot_pqt
{
	union {
		uint32_t timeout;
		uint32_t delay;
	}; /* in ms */
	struct caniot_pqt *next;
};

struct caniot_pendq
{
	caniot_did_t did;
	caniot_query_callback_t callback;

	union {
		struct caniot_pqt tie; /* for timeout queue */
		struct caniot_pendq *next; /* for memory allocation */
	};
};

struct caniot_controller {
	char name[32];
	uint32_t uid;

	struct caniot_device_entry devices[32];

	struct {
		struct caniot_pendq pool[CANIOT_MAX_PENDING_QUERIES];
		struct caniot_pendq *free_list;
		struct caniot_pqt *timeout_queue;
	} pendingq;

	struct {
		uint32_t sec;
		uint16_t ms;
	} last_process;

	const struct caniot_drivers_api *driv;
};

// intitalize controller structure
int caniot_controller_init(struct caniot_controller *controller);

int caniot_controller_deinit(struct caniot_controller *ctrl);

int caniot_controller_query(struct caniot_controller *controller,
			    caniot_did_t did,
			    struct caniot_frame *frame,
			    caniot_query_callback_t cb,
			    uint32_t timeout);

int caniot_request_telemetry(struct caniot_controller *ctrl,
			     caniot_did_t did,
			     uint8_t ep,
			     caniot_query_callback_t cb,
			     uint32_t timeout);

int caniot_command(struct caniot_controller *ctrl,
		   caniot_did_t did,
		   uint8_t ep,
		   uint8_t *buf,
		   uint8_t len,
		   caniot_query_callback_t cb,
		   uint32_t timeout);

int caniot_read_attribute(struct caniot_controller *ctrl,
			  caniot_did_t did,
			  uint16_t key,
			  caniot_query_callback_t cb,
			  uint32_t timeout);

int caniot_write_attribute(struct caniot_controller *ctrl,
			   caniot_did_t did,
			   uint16_t key,
			   uint32_t value,
			   caniot_query_callback_t cb,
			   uint32_t timeout);

int caniot_discover(struct caniot_controller *ctrl,
		    caniot_query_callback_t cb,
		    uint32_t timeout);

int caniot_controller_handle_rx_frame(struct caniot_controller *ctrl,
				      const struct caniot_frame *frame);

/**
 * @brief Check timeouts and receive incoming CANIOT message if any and handle it
 *
 * Note: Should be called on query timeout or when an incoming can message
 *
 * @param ctrl
 * @return int
 */
int caniot_controller_process(struct caniot_controller *ctrl);

// int caniot_debug_pendq(struct caniot_controller *ctrl);

#endif /* _CANIOT_CONTROLLER_H */