#ifndef _CANIOT_CONTROLLER_H
#define _CANIOT_CONTROLLER_H

#include "caniot.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CANIOT_TIMEOUT_FOREVER ((uint32_t)-1)

struct caniot_pendq_time_handle {
	union {
		uint32_t timeout; /* Timeout if response is not yet received */
		uint32_t delay;	  /* Delay the query took when response is received */
	};			  /* in ms */

	/* next query in the time queue
	 * @see
	 * https://github.com/lucasdietrich/AVRTOS/blob/master/src/avrtos/dstruct/tqueue.c
	 */
	struct caniot_pendq_time_handle *next;
};

struct caniot_pendq {
	/**
	 * @brief Device the query is pending on.
	 */
	caniot_did_t did;

	/**
	 * @brief Handle identifying the query.
	 * (0 = invalid)
	 */
	uint8_t handle;

	/**
	 * @brief Query type, in order to identify the response.
	 *
	 * TODO: Use a unique identifier for the query that the device will forward
	 * in the response. Use of extended CAN ID enables this.
	 */
	caniot_frame_type_t query_type;
	union {
		struct caniot_pendq_time_handle tie; /* for timeout queue */
		struct caniot_pendq *next;	     /* for memory allocation */
	};

	/**
	 * @brief User context
	 *
	 * Set using caniot_controller_handle_user_data_set() function
	 */
	void *user_data;
};

struct caniot_controller;

typedef enum {
	/**
	 * @brief Frame received is not the response to a tracked query.
	 *
	 * "pq" NULL
	 */
	CANIOT_CONTROLLER_EVENT_CONTEXT_ORPHAN = 0U,

	/**
	 * @brief Frame received is the response to a tracked query.
	 *
	 * "pq" is set.
	 */
	CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY,
} caniot_controller_event_context_t;

typedef enum {
	/**
	 * @brief Valid response received
	 *
	 * "pq" can be NULL or set, "response" is VALID.
	 */
	CANIOT_CONTROLLER_EVENT_STATUS_OK = 0U,

	/**
	 * @brief Valid error frame received
	 *
	 * "pq" can be NULL or set, "response" is VALID.
	 */
	CANIOT_CONTROLLER_EVENT_STATUS_ERROR, /* error frame received, is part of a query
						 (response is set) */

	/**
	 * @brief A query timed out
	 *
	 * "pq" is set, reponse is NULL.
	 */
	CANIOT_CONTROLLER_EVENT_STATUS_TIMEOUT, /*  */

	/**
	 * @brief A query was cancelled
	 *
	 * "pq" is set, reponse is NULL.
	 */
	CANIOT_CONTROLLER_EVENT_STATUS_CANCELLED,
} caniot_controller_event_status_t;

typedef struct {
	/**
	 * @brief Controller that generated the event
	 */
	struct caniot_controller *controller;

	/**
	 * @brief Event context
	 */
	caniot_controller_event_context_t context : 2U;

	/**
	 * @brief Event status
	 */
	caniot_controller_event_status_t status : 2U;

	/**
	 * @brief Tells whether the query is terminated or not.
	 * Only valid in the context of a query (CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY)
	 */
	uint8_t terminated : 1U;

	/**
	 * @brief Device ID of the device that generated the event
	 */
	caniot_did_t did;

	/**
	 * @brief Handle to identify the query if context is:
	 * - CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY
	 */
	uint8_t handle;

	/**
	 * @brief Pointer to the response frame if status is:
	 * - CANIOT_CONTROLLER_EVENT_STATUS_OK
	 * - CANIOT_CONTROLLER_EVENT_STATUS_ERROR
	 */
	const struct caniot_frame *response;

	/**
	 * @brief User context
	 *
	 * Registered when initializing the controller with caniot_controller_init()
	 */
	void *user_data;
} caniot_controller_event_t;

/**
 * @brief Callback to handle controller events
 *
 * @param ev Event
 * @param user_data User data (passed when initializing the controller)
 */
typedef bool (*caniot_controller_event_cb_t)(const caniot_controller_event_t *ev,
					     void *user_data);

struct caniot_controller {
	struct {
		/* Pool of queries to be allocated */
		struct caniot_pendq pool[CONFIG_CANIOT_MAX_PENDING_QUERIES];

		/* Free list of unallocated blocks */
		struct caniot_pendq *free_list;

		/* Timeout queue */
		struct caniot_pendq_time_handle *timeout_queue;

		/* bitfield of pending devices */
		uint64_t pending_devices_bf;
	} pendingq;

	/* Reference when caniot_controller_process() was last called */
	struct {
		uint32_t sec;
		uint16_t ms;
	} last_process;

	/* Callback to handle controller events */
	caniot_controller_event_cb_t event_cb;

	/* User data for the event callback */
	void *user_data;

#if CONFIG_CANIOT_CTRL_DRIVERS_API
	/* Driver API in case the controller is initialized with
	 * caniot_controller_driv_init()
	 */
	const struct caniot_drivers_api *driv;
#endif
};

typedef struct caniot_controller caniot_controller_t;

/**
 * @brief Initialize a controller, register the event callback and user data
 *
 * @param ctrl
 * @param cb
 * @param user_data
 * @return int
 */
int caniot_controller_init(struct caniot_controller *ctrl,
			   caniot_controller_event_cb_t cb,
			   void *user_data);

/**
 * @brief Initialize a controller, provide functions API, cb and user data
 *
 * @param ctrl
 * @param driv Driver API to directly send/receive frames get current time, etc.
 * @param cb
 * @param user_data
 * @return int
 */
int caniot_controller_driv_init(struct caniot_controller *ctrl,
				const struct caniot_drivers_api *driv,
				caniot_controller_event_cb_t cb,
				void *user_data);

/**
 * @brief Deinitialize a controller
 *
 * @param ctrl
 * @return int
 */
int caniot_controller_deinit(struct caniot_controller *ctrl);

/**
 * @brief Get time in ms to next timeout
 *
 * @param ctrl
 * @return uint32_t
 */
uint32_t caniot_controller_next_timeout(const struct caniot_controller *ctrl);

/**
 * @brief Build a query frame to be sent to a device and register its context
 * for tracking.
 *
 * Note: The use should send the frame. (the function does not do it)
 *
 * Note: That if the frame send fails, the query should be cancelled
 *  using function caniot_controller_cancel_query() with the returned handle
 *
 * @param controller
 * @param did
 * @param frame
 * @param timeout
 * 	- If timeout is 0 no context is allocated,
 *  	- If timeout is -1 (CANIOT_TIMEOUT_FOREVER)  a context is allocated but
 *  	  the query should be cancelled if it doesn't get a response or if did is
 * BROADCAST
 * 	- otherwise a context is allocated and the query will
 * 	  be automatically cancelled after timeout
 * @return int handle on success (> 0), negative value on error, 0 if no context allocated
 */
int caniot_controller_query_register(struct caniot_controller *ctrl,
				     caniot_did_t did,
				     struct caniot_frame *frame,
				     uint32_t timeout);

bool caniot_controller_query_pending(struct caniot_controller *ctrl, uint8_t handle);

int caniot_controller_cancel_query(struct caniot_controller *ctrl,
				   uint8_t handle,
				   bool suppress);

int caniot_controller_process_single(struct caniot_controller *ctrl,
				     uint32_t time_passed,
				     const struct caniot_frame *frame);

/*____________________________________________________________________________*/

/**
 * @brief Set the user data for the given query handle
 *
 * @param ctrl
 * @param handle Handle of the query
 * @param user_data User data to set
 * @return int
 */
int caniot_controller_handle_user_data_set(struct caniot_controller *ctrl,
					   uint8_t handle,
					   void *user_data);

void *caniot_controller_handle_user_data_get(struct caniot_controller *ctrl,
					     uint8_t handle);

/*____________________________________________________________________________*/

/**
 * @brief Send a query to a device and return the handle if timeout is not 0
 *
 * Returned handle can be used to track the query status or cancel it.
 * It is necessarily positive. A value of 0 means that the query is not tracked.
 *
 * @param ctrl Controller
 * @param did ID of the device to query
 * @param frame Frame to send
 * @param timeout Timeout in ms, a value of 0 means no timeout.
 * @return int Handle of the query, 0 if not tracked, negative value on error
 */
int caniot_controller_query(struct caniot_controller *ctrl,
			    caniot_did_t did,
			    struct caniot_frame *frame,
			    uint32_t timeout);

/**
 * @brief Send a query without tracking it.
 *
 * Note: This function is equivalent to caniot_controller_query() with timeout = 0
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
