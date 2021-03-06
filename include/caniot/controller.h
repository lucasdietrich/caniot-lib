#ifndef _CANIOT_CONTROLLER_H
#define _CANIOT_CONTROLLER_H

#include "caniot.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CANIOT_TIMEOUT_FOREVER ((uint32_t) -1)

struct caniot_device_entry
{
	// uint32_t last_seen;	/* timestamp this device was last seen */
	struct {
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
	/**
	 * @brief Device the query is pending on.
	 */
	caniot_did_t did;

	/**
	 * @brief Handle identifying the query.
	 * (0 = invalid)
	 */
	uint8_t handle;

	caniot_frame_type_t query_type;
	union {
		struct caniot_pqt tie; /* for timeout queue */
		struct caniot_pendq *next; /* for memory allocation */
	};

	/* User context */
	void *user_data;
};

struct caniot_controller;

typedef enum {
	CANIOT_CONTROLLER_EVENT_CONTEXT_ORPHAN = 0U, 	/* pq not set */
	CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY, 		/* pq is set */
} caniot_controller_event_context_t;

typedef enum {
	CANIOT_CONTROLLER_EVENT_STATUS_OK = 0U, 	/* is not part of a query (response is set) */
	CANIOT_CONTROLLER_EVENT_STATUS_ERROR, 		/* error frame received, is part of a query (response is set) */
	CANIOT_CONTROLLER_EVENT_STATUS_TIMEOUT, 	/* a query timed out (response not set) */	
	CANIOT_CONTROLLER_EVENT_STATUS_CANCELLED,	/* a query was cancelled (response not set) */
} caniot_controller_event_status_t;

typedef enum {
	CANIOT_CONTROLLER_QUERY_PENDING = 0U,
	CANIOT_CONTROLLER_QUERY_TERMINATED = 1U
} caniot_controller_query_terminated_t;

typedef struct {
	struct caniot_controller *controller;

	caniot_controller_event_context_t context : 2U;
	caniot_controller_event_status_t status : 2U;

	/* terminated only valid if context is CANIOT_CONTROLLER_EVENT_CONTEXT_ORPHAN */
	caniot_controller_query_terminated_t terminated : 1U; 

	caniot_did_t did;

	uint8_t handle;

	const struct caniot_frame *response;

	void *user_data;
} caniot_controller_event_t;

typedef bool (*caniot_controller_event_cb_t)(const caniot_controller_event_t *ev,
					     void *user_data);

struct caniot_controller {
	struct {
		struct caniot_pendq pool[CANIOT_MAX_PENDING_QUERIES];
		struct caniot_pendq *free_list;
		struct caniot_pqt *timeout_queue;

		/**
		 * @brief Pending queries bitmap
		 */
		uint64_t pending_devices_bf;
	} pendingq;

	struct {
		uint32_t sec;
		uint16_t ms;
	} last_process;

	caniot_controller_event_cb_t event_cb;
	void *user_data;

#if CANIOT_CTRL_DRIVERS_API
	const struct caniot_drivers_api *driv;
#endif
};

typedef struct caniot_controller caniot_controller_t;

// intitalize controller structure
int caniot_controller_init(struct caniot_controller *ctrl,
			   caniot_controller_event_cb_t cb,
			   void *user_data);

int caniot_controller_driv_init(struct caniot_controller *ctrl,
				const struct caniot_drivers_api *driv,
				caniot_controller_event_cb_t cb,
				void *user_data);

int caniot_controller_deinit(struct caniot_controller *ctrl);

uint32_t caniot_controller_next_timeout(const struct caniot_controller *ctrl);
/**
 * @brief Build a query frame to be sent to a device
 * 
 * Note: That if the frame send fails, the query should be cancelled
 *  using function caniot_controller_cancel_query() with the returned handle
 * 
 * Note: 
 * 
 * @param controller 
 * @param did 
 * @param frame 
 * @param timeout 
 * 	- If timeout is 0 no context is allocated, 
 *  	- If timeout is CANIOT_TIMEOUT_FOREVER a context is allocated but 
 *  	  the query should be cancelled if it doesn't get a response or if did is BROADCAST
 * 	- otherwise a context is allocated and the query will 
 * 	  be automatically cancelled after timeout
 * @return int handle on success (> 0), negative value on error, 0 if no context allocated
 */
int caniot_controller_query_register(struct caniot_controller *ctrl,
				     caniot_did_t did,
				     struct caniot_frame *frame,
				     uint32_t timeout);

bool caniot_controller_query_pending(struct caniot_controller *ctrl,
				     uint8_t handle);


int caniot_controller_cancel_query(struct caniot_controller *ctrl,
				   uint8_t handle,
				   bool suppress);

int caniot_controller_process_single(struct caniot_controller *ctrl,
				     uint32_t time_passed,
				     const struct caniot_frame *frame);

/*____________________________________________________________________________*/

int caniot_controller_handle_set_user_data(struct caniot_controller *ctrl,
					   uint8_t handle,
					   void *user_data);

/*____________________________________________________________________________*/

/**
 * @brief Do a query which expects a response
 * 
 * @param ctrl 
 * @param did 
 * @param frame 
 * @param timeout 
 * @return int 
 */
int caniot_controller_query(struct caniot_controller *ctrl,
			    caniot_did_t did,
			    struct caniot_frame *frame,
			    uint32_t timeout);

/**
 * @brief Send a query frame only
 * 
 * @param ctrl 
 * @param did 
 * @param frame 
 * @return int 
 */
int caniot_controller_send(struct caniot_controller *ctrl,
			   caniot_did_t did,
			   struct caniot_frame *frame);

/**
 * 
 * @brief Check timeouts and receive incoming CANIOT message if any and handle it
 *
 * Note: Should be called on query timeout or when an incoming can message
 *
 * @param ctrl
 * @return int
 */
int caniot_controller_process(struct caniot_controller *ctrl);

/*____________________________________________________________________________*/

int caniot_controller_dbg_free_pendq(struct caniot_controller *ctrl);

#ifdef __cplusplus
}
#endif

#endif /* _CANIOT_CONTROLLER_H */
