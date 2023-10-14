/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <caniot/caniot_private.h>
#include <caniot/controller.h>

#define pendq caniot_pendq
#define pqt	  caniot_pendq_time_handle

#define __DBG(fmt, ...) CANIOT_DBG("-- " fmt, ##__VA_ARGS__)

#define INVALID_HANDLE ((uint8_t)0x00U)

static void stop_discovery(struct caniot_controller *ctrl);

static bool is_query_pending_for(struct caniot_controller *ctrl, caniot_did_t did)
{
	ASSERT(ctrl != NULL);

	const bool result = ctrl->pendingq.pending_devices_bf & (1llu << did);

	__DBG("is_query_pending_for(did: %u) -> success: %u\n", did, (uint32_t)result);

	return result;
}

static void
mark_query_pending_for(struct caniot_controller *ctrl, caniot_did_t did, bool status)
{
	ASSERT(ctrl != NULL);

	__DBG("mark_query_pending_for(did: %u, status: %u)\n", did, status);

	if (status == true) {
		ctrl->pendingq.pending_devices_bf |= (1llu << did);
	} else {
		ctrl->pendingq.pending_devices_bf &= ~(1llu << did);
	}
}

// Finalize frame with device id
static void finalize_query_frame(struct caniot_frame *frame, caniot_did_t did)
{
	ASSERT(frame != NULL);

	frame->id.query = CANIOT_QUERY;

	frame->id.cls = CANIOT_DID_CLS(did);
	frame->id.sid = CANIOT_DID_SID(did);
}

static void _pendq_queue(struct pqt **root, struct pqt *item)
{
	ASSERT(root != NULL);
	ASSERT(item != NULL);

	struct pqt **prev_next_p = root;
	while (*prev_next_p != NULL) {
		struct pqt *p_current = *prev_next_p;

		if (p_current->delay <= item->delay) {
			item->delay -= p_current->delay;
			prev_next_p = &(p_current->next);
		} else {
			item->next = p_current;
			p_current->delay -= item->delay;
			break;
		}
	}
	*prev_next_p = item;
}

static void
pendq_queue(struct caniot_controller *ctrl, struct pendq *pq, uint32_t timeout)
{
	ASSERT(ctrl != NULL);
	ASSERT(pq != NULL);
	ASSERT(timeout != CANIOT_TIMEOUT_FOREVER);

	if (pq == NULL) return;

	/* last item doesn't have a "next" item */
	pq->tie.next	= NULL;
	pq->tie.timeout = timeout;

	_pendq_queue(&ctrl->pendingq.timeout_queue, &pq->tie);

	__DBG("pendq_queue(ps: %p, timeout: %u)\n", (void *)pq, timeout);
}

static void pendq_shift(struct caniot_controller *ctrl, uint32_t time_passed_ms)
{
	ASSERT(ctrl != NULL);

	__DBG("pendq_shift(time_passed_ms: %u)\n", time_passed_ms);

	if (time_passed_ms == 0u) return;

	struct pqt **prev_next_p = &ctrl->pendingq.timeout_queue;
	while (*prev_next_p != NULL) {
		struct pqt *p_current = *prev_next_p;

		if (p_current->delay <= time_passed_ms) {
			if (p_current->delay != 0) {
				time_passed_ms -= p_current->delay;
				p_current->delay = 0;
			}
			prev_next_p = &(p_current->next);
		} else {
			p_current->delay -= time_passed_ms;
			break;
		}
	}
}

static struct pendq *pendq_pop_expired(struct pqt **root)
{
	ASSERT(root != NULL);

	struct pendq *pq = NULL;

	if ((*root != NULL) && ((*root)->delay == 0)) {
		struct pqt *item = *root;
		*root			 = (*root)->next;

		pq = CONTAINER_OF(item, struct pendq, tie);
	}

	__DBG("pendq_pop_expired() -> %p\n", (void *)pq);

	return pq;
}

static struct pendq *pendq_pop(struct pqt **root)
{
	ASSERT(root != NULL);

	struct pendq *pq = NULL;

	if (*root != NULL) {
		struct pqt *item = *root;
		*root			 = (*root)->next;

		pq = CONTAINER_OF(item, struct pendq, tie);
	}

	__DBG("pendq_pop() -> %p\n", (void *)pq);

	return pq;
}

static void pendq_tqueue_remove(struct caniot_controller *ctrl, struct pendq *pq)
{
	ASSERT(ctrl != NULL);
	ASSERT(pq != NULL);

	struct pqt **prev_next_p = &ctrl->pendingq.timeout_queue;
	while (*prev_next_p != NULL) {
		struct pqt *p_current = *prev_next_p;
		if (p_current == &pq->tie) {
			__DBG("pendq_tqueue_remove(pq: %p) -> removed\n", (void *)pq);

			*prev_next_p = p_current->next;
			if (p_current->next != NULL) {
				p_current->next->delay += pq->tie.delay;
			}
			pq->tie.next = NULL;
			return;
		}
		prev_next_p = &(p_current->next);
	}

	__DBG("pendq_tqueue_remove(pq: %p) -> not found\n", (void *)pq);
}

static struct pendq *pendq_alloc(struct caniot_controller *ctrl)
{
	ASSERT(ctrl != NULL);

	struct pendq *p = ctrl->pendingq.free_list;
	if (p != NULL) {
		__DBG("pendq_alloc() -> pq: %p\n", (void *)p);

		ctrl->pendingq.free_list = p->next;
	} else {
		__DBG("pendq_alloc() -> NULL\n");
	}
	return p;
}

static void pendq_free(struct caniot_controller *ctrl, struct pendq *pq)
{
	ASSERT(ctrl != NULL);

	if (pq != NULL) {
		__DBG("pendq_free(pq: %p)\n", (void *)pq);

		pq->next				 = ctrl->pendingq.free_list;
		pq->handle				 = INVALID_HANDLE;
		ctrl->pendingq.free_list = pq;
	} else {
		__DBG("pendq_free(NULL)\n");
	}
}

static void pendq_init_queue(struct caniot_controller *ctrl)
{
	ASSERT(ctrl != NULL);

	/* init free list */
	ctrl->pendingq.free_list = NULL;
	struct pendq *cur		 = ctrl->pendingq.pool;
	while (cur < ctrl->pendingq.pool + CONFIG_CANIOT_MAX_PENDING_QUERIES) {
		pendq_free(ctrl, cur++);
	}

	/* init timeout queue */
	ctrl->pendingq.timeout_queue = NULL;
}

static struct pendq *pendq_get_by_did(struct caniot_controller *ctrl, caniot_did_t did)
{
	ASSERT(ctrl != NULL);

	struct pendq *retpq = NULL;

	struct pqt *tie;
	for (tie = ctrl->pendingq.timeout_queue; tie != NULL; tie = tie->next) {
		struct pendq *const pq = CONTAINER_OF(tie, struct pendq, tie);
		if (CANIOT_DID_EQ(pq->did, did)) {
			retpq = pq;
			break;
		}
	}

	__DBG("pendq_get_by_did(did: %u) -> pq: %p\n", did, (void *)retpq);

	return retpq;
}

static struct pendq *pendq_get_by_handle(struct caniot_controller *ctrl, uint8_t handle)
{
	ASSERT(ctrl);

	struct pendq *pq = NULL;

	if ((handle != INVALID_HANDLE) && (handle <= CONFIG_CANIOT_MAX_PENDING_QUERIES)) {
		struct pendq *const tmp = &ctrl->pendingq.pool[handle - 1U];

		/* if handle is not exactly the same as the index,
		 * the pq is not allocated */
		if (tmp->handle == handle) {
			pq = tmp;
		}
	}

	__DBG("pendq_get_by_handle(handle: %u) -> pq: %p\n", handle, (void *)pq);

	return pq;
}

static bool pendq_is_broadcast(struct pendq *pq)
{
	ASSERT(pq != NULL);

	return caniot_is_broadcast(pq->did);
}

int caniot_controller_query_user_data_set(struct caniot_controller *ctrl,
										  uint8_t handle,
										  void *user_data)
{
#if CONFIG_CANIOT_CHECKS
	if (!ctrl) return -CANIOT_EINVAL;
#endif

	int ret			 = -CANIOT_EINVAL;
	struct pendq *pq = pendq_get_by_handle(ctrl, handle);

	if (pq != NULL) {
		pq->user_data = user_data;
		ret			  = 0;
	}

	return ret;
}

void *caniot_controller_query_user_data_get(struct caniot_controller *ctrl,
											uint8_t handle)
{
#if CONFIG_CANIOT_CHECKS
	if (!ctrl) return NULL;
#endif

	void *user_data	 = NULL;
	struct pendq *pq = pendq_get_by_handle(ctrl, handle);

	if (pq != NULL) {
		user_data = pq->user_data;
	}

	return user_data;
}

static struct pendq *peek_pending_query(struct caniot_controller *ctrl, caniot_did_t did)
{
	ASSERT(ctrl != NULL);

	struct pendq *pq = NULL;

	if (is_query_pending_for(ctrl, did)) pq = pendq_get_by_did(ctrl, did);

	__DBG("peek_pending_query(did: %u) -> pq: %p\n", did, (void *)pq);

	return pq;
}

/**
 * @brief Remove a pending query from the list of tracked queries.
 *
 * @param ctrl Controller instance
 * @param pq Pending query to remove
 */
static void pendq_remove(struct caniot_controller *ctrl, struct pendq *pq)
{
	ASSERT(ctrl != NULL);
	ASSERT(pq != NULL);

	mark_query_pending_for(ctrl, pq->did, false);
	pendq_tqueue_remove(ctrl, pq);
	pendq_free(ctrl, pq);
}

// Initialize ctrl structure
int caniot_controller_init(struct caniot_controller *ctrl,
						   caniot_controller_event_cb_t cb,
						   void *user_data)
{
	int ret = 0;

	if (ctrl == NULL) {
		ret = -CANIOT_ENULLCTRL;
		goto exit;
	}

	if (cb == NULL) {
		ret = -CANIOT_ENULLCTRLCB;
		goto exit;
	}

	memset(ctrl, 0, sizeof(struct caniot_controller));

	ctrl->event_cb	= cb;
	ctrl->user_data = user_data;

	pendq_init_queue(ctrl);

exit:
	return ret;
}

#if CONFIG_CANIOT_CTRL_DRIVERS_API
int caniot_controller_driv_init(struct caniot_controller *ctrl,
								const struct caniot_drivers_api *driv,
								caniot_controller_event_cb_t cb,
								void *user_data)
{
	int ret = caniot_controller_init(ctrl, cb, user_data);
	if (ret < 0) {
		goto exit;
	}

	if (driv == NULL) {
		ret = -CANIOT_EDRIVER;
		goto exit;
	}
	ctrl->driv = driv;

exit:
	return ret;
}
#else
int caniot_controller_driv_init(struct caniot_controller *ctrl,
								const struct caniot_drivers_api *driv,
								caniot_controller_event_cb_t cb,
								void *user_data)
{
	(void)ctrl;
	(void)driv;
	(void)cb;
	(void)user_data;

	return -CANIOT_ENOTSUP;
}
#endif

uint32_t caniot_controller_next_timeout(const struct caniot_controller *ctrl)
{
#if CONFIG_CANIOT_CHECKS
	if (!ctrl) return 0xFFFFFFFFu;
#endif

	uint32_t next_timeout = (uint32_t)-1;

	struct pqt *next = ctrl->pendingq.timeout_queue;

	if (next != NULL) {
		next_timeout = next->timeout;
	}

	return next_timeout;
}

static bool call_user_callback(struct caniot_controller *ctrl,
							   const caniot_controller_event_t *ev)
{
	ASSERT(ctrl != NULL);
	ASSERT(ev != NULL);
	ASSERT(ctrl->event_cb != NULL);

	__DBG("call_user_callback(ev: %p) -> did: %u handle: %u ctx: %u status: %u term: "
		  "%u "
		  "resp: %p\n",
		  (void *)ev,
		  ev->did,
		  ev->handle,
		  ev->context,
		  ev->status,
		  ev->terminated,
		  (void *)ev->response);

	return ctrl->event_cb(ev, ctrl->user_data);
}

static void orphan_resp_event(struct caniot_controller *ctrl,
							  const struct caniot_frame *response)
{
	ASSERT(ctrl != NULL);
	ASSERT(response != NULL);

	const caniot_did_t did = CANIOT_DID(response->id.cls, response->id.sid);
	const bool is_error	   = caniot_is_error_frame(response->id);

	const caniot_controller_event_t ev = {
		.controller = ctrl,
		.context	= CANIOT_CONTROLLER_EVENT_CONTEXT_ORPHAN,
		.status		= is_error ? CANIOT_CONTROLLER_EVENT_STATUS_ERROR
							   : CANIOT_CONTROLLER_EVENT_STATUS_OK,

		.did = did,

		.terminated = 1U, /* meaningless in this context */
		.handle		= 0U, /* meaningless in this context */

		.response = response,

		.user_data = NULL,
	};

	call_user_callback(ctrl, &ev);
}

static bool pendq_is_discovery(struct caniot_controller *ctrl, struct pendq *pq)
{
	ASSERT(ctrl != NULL);
	ASSERT(pq != NULL);

#if CONFIG_CANIOT_CONTROLLER_DISCOVERY
	return pendq_is_broadcast(pq) && ctrl->discovery.pending;
#else
	return false;
#endif
}

static void
cancelled_query_event(struct caniot_controller *ctrl, struct pendq *pq, bool suppress)
{
	ASSERT(pq != NULL);

	const caniot_controller_event_t ev = {
		.controller = ctrl,
		.context	= CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY,
		.status		= CANIOT_CONTROLLER_EVENT_STATUS_CANCELLED,

		.did = pq->did,

		.terminated = 1U,
		.handle		= pq->handle,

		.response  = NULL,
		.user_data = pq->user_data,
	};

	pendq_remove(ctrl, pq);

#if CONFIG_CANIOT_CONTROLLER_DISCOVERY
	if (pendq_is_discovery(ctrl, pq)) stop_discovery(ctrl);
#endif

	if (!suppress) call_user_callback(ctrl, &ev);
}

static void pendq_call_expired(struct caniot_controller *ctrl)
{
	ASSERT(ctrl != NULL);

	struct pendq *pq;
	struct pqt **const q = &ctrl->pendingq.timeout_queue;

	while ((pq = pendq_pop_expired(q)) != NULL) {
		const caniot_controller_event_t ev = {
			.controller = ctrl,
			.context	= CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY,
			.status		= CANIOT_CONTROLLER_EVENT_STATUS_TIMEOUT,

			.did = pq->did,

			.terminated = 1U,
			.handle		= pq->handle,

			.response  = NULL,
			.user_data = pq->user_data,
		};

		mark_query_pending_for(ctrl, pq->did, false);
		pendq_free(ctrl, pq);

#if CONFIG_CANIOT_CONTROLLER_DISCOVERY
		if (pendq_is_discovery(ctrl, pq)) stop_discovery(ctrl);
#endif

		call_user_callback(ctrl, &ev);
	}
}

static struct pendq *pendq_alloc_and_prepare(struct caniot_controller *ctrl,
											 caniot_did_t did,
											 struct caniot_frame *frame)
{
	/* allocate */
	struct pendq *pq = pendq_alloc(ctrl);

	if (pq != NULL) {
		/* prepare data */
		pq->did		   = did;
		pq->handle	   = 1U + INDEX_OF(pq, ctrl->pendingq.pool, struct pendq);
		pq->query_type = frame->id.type;
		pq->notified   = 0llu;

#if CONFIG_CANIOT_QUERY_ID
		pq->query_id = 0u;
#endif

		switch (pq->query_type) {
		case CANIOT_FRAME_TYPE_COMMAND:
		case CANIOT_FRAME_TYPE_TELEMETRY:
			pq->req_endpoint = frame->id.endpoint;
			break;
		case CANIOT_FRAME_TYPE_READ_ATTRIBUTE:
		case CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE:
			pq->req_attr = frame->attr.key;
			break;
		default:
			break;
		}
	}

	return pq;
}

static int query(struct caniot_controller *ctrl,
				 caniot_did_t did,
				 struct caniot_frame *frame,
				 uint32_t timeout,
				 bool driv_send)
{
	int ret;

	/* validate arguments */
#if CONFIG_CANIOT_CHECKS
	if (!ctrl || !frame) return -CANIOT_EINVAL;
#endif

	const bool alloc_context = timeout != 0U;
	struct pendq *pq		 = NULL;

	/* if timeout is defined, we need to allocate a context */
	if (alloc_context == true) {
		if (caniot_deviceid_valid(did) == false) {
			ret = -CANIOT_EDEVICE;
			goto exit;
		}

		/* another query is already pending for the device */
		if (is_query_pending_for(ctrl, did) == true) {
			ret = -CANIOT_EBUSY;
			goto exit;
		}

		pq = pendq_alloc_and_prepare(ctrl, did, frame);
		if (pq == NULL) {
			ret = -CANIOT_EPQALLOC;
			goto exit;
		}
	}

	/* finalize and send the query frame */
	finalize_query_frame(frame, did);

#if CONFIG_CANIOT_CTRL_DRIVERS_API
	if (driv_send == true) {
		/* send frame */
		ret = ctrl->driv->send(frame, 0U);
		if (ret < 0) {
			goto exit;
		}
	}
#endif

	if (alloc_context == true) {
		if (timeout != CANIOT_TIMEOUT_FOREVER) {
			/* reference query for timeout */
			pendq_queue(ctrl, pq, timeout);
		}

		/* tells that a query is pending for the device */
		mark_query_pending_for(ctrl, did, true);

		ret = pq->handle;
	} else {
		ret = 0;
	}

exit:
	__DBG("query(did: %u, frame: %p, timeout: %u, driv: %u) -> ret (handle): %d\n",
		  did,
		  (void *)frame,
		  timeout,
		  (uint32_t)driv_send,
		  ret);

	return ret;
}

int caniot_controller_query_register(struct caniot_controller *ctrl,
									 caniot_did_t did,
									 struct caniot_frame *frame,
									 uint32_t timeout)
{
#if CONFIG_CANIOT_CHECKS
	if (!ctrl || !frame) return -CANIOT_EINVAL;
#endif

	int ret = query(ctrl, did, frame, timeout, false);

	__DBG("caniot_controller_query_register(did: %u, frame: %p, timeout: %u) -> ret: "
		  "%d\n",
		  did,
		  (void *)frame,
		  timeout,
		  ret);

	return ret;
}

bool caniot_controller_query_pending(struct caniot_controller *ctrl, uint8_t handle)
{
#if CONFIG_CANIOT_CHECKS
	if (!ctrl) return -CANIOT_EINVAL;
#endif

	struct pendq *const pq = pendq_get_by_handle(ctrl, handle);

	__DBG("caniot_controller_query_pending(handle: %u) -> pq: %u\n", handle, pq != NULL);

	return pq != NULL;
}

int caniot_controller_query_cancel(struct caniot_controller *ctrl,
								   uint8_t handle,
								   bool suppress)
{
	int ret;

#if CONFIG_CANIOT_CHECKS
	if (!ctrl) return -CANIOT_EINVAL;
#endif

	struct pendq *pq = pendq_get_by_handle(ctrl, handle);
	if (pq == NULL) {
		ret = -CANIOT_ENOHANDLE;
		goto exit;
	}

	/* execute user callback if not suppressed */
	if (suppress == false) {
		cancelled_query_event(ctrl, pq, suppress);
	}

	ret = 0;
exit:
	__DBG("caniot_controller_query_cancel(handle: %u, suppress: %u) -> ret: %d\n",
		  handle,
		  suppress,
		  ret);

	return ret;
}

static bool
is_response_to(const struct caniot_frame *frame, struct pendq *pq, bool *p_is_error)
{
	ASSERT((pq->did == CANIOT_DID(frame->id.cls, frame->id.sid)) ||
		   (pq->did == CANIOT_DID_BROADCAST));

	bool match					  = false;
	bool is_error				  = false;
	caniot_frame_type_t resp_type = frame->id.type;

	switch (pq->query_type) {
	case CANIOT_FRAME_TYPE_COMMAND:
	case CANIOT_FRAME_TYPE_TELEMETRY:
		if (frame->id.endpoint == pq->req_endpoint) {
			match = true;
		}
		if (frame->id.type == CANIOT_FRAME_TYPE_COMMAND) {
			is_error = true;
		}
		break;

	case CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE:
	case CANIOT_FRAME_TYPE_READ_ATTRIBUTE:
		if (resp_type == CANIOT_FRAME_TYPE_READ_ATTRIBUTE) {
			/* If it is a read attribute response, the key is stored in the
			 * attribute field */
			if (frame->attr.key == pq->req_attr) {
				match = true;
			}
		}
		if (frame->id.type == CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE) {
			is_error = true;
			/* If it is an error, the key which triggered it is
			 * stored in the error argument field */
			if ((frame->err.arg & 0xFFFFlu) == (uint32_t)pq->req_attr) {
				match = true;
			}
		}
		break;
	default:
		break;
	}

	if (p_is_error != NULL) {
		*p_is_error = is_error;
	}

	return match;
}

static void pendq_handle_device_resp(struct caniot_controller *ctrl,
									 struct pendq *pq,
									 const struct caniot_frame *response,
									 bool is_error)
{
	const caniot_controller_event_t ev = {
		.controller = ctrl,
		.context	= CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY,
		.status		= is_error ? CANIOT_CONTROLLER_EVENT_STATUS_ERROR
							   : CANIOT_CONTROLLER_EVENT_STATUS_OK,

		.did = CANIOT_DID(response->id.cls, response->id.sid),

		.terminated = true,
		.handle		= pq->handle,

		.response  = response,
		.user_data = pq->user_data,
	};

	/* Release context before callback call in case the use wants to
	 * perform operations on a pq which will no longer live
	 */
	pendq_remove(ctrl, pq);

	/* Ignore user callback return value in case of a response to a
	 * non-broadcast query because the pq context has already been
	 * released */
	(void)call_user_callback(ctrl, &ev);
}

static void pendq_handle_broadcast_resp(struct caniot_controller *ctrl,
										struct pendq *pq,
										const struct caniot_frame *response,
										bool is_error)
{
	ASSERT(caniot_is_broadcast(pq->did));

	const caniot_controller_event_t ev = {
		.controller = ctrl,
		.context	= CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY,
		.status		= is_error ? CANIOT_CONTROLLER_EVENT_STATUS_ERROR
							   : CANIOT_CONTROLLER_EVENT_STATUS_OK,

		.did = CANIOT_DID_BROADCAST,

		.terminated = false,
		.handle		= pq->handle,

		.response  = response,
		.user_data = pq->user_data,
	};

	/* Make sure not more than one broadcast response is received
	 * per device */
	if (pq->notified & (1U << ev.did)) {
		/* Already notified for this device, ignore ... */
		__DBG("broacast pq, device %u already notified\n", ev.did);
		return;
	} else {
		__DBG("broacast pq, device %u not notified yet\n", ev.did);
		pq->notified |= (1U << ev.did);
	}

	/* If discovery is enabled, call the discovery callback and
	 * terminate discovery if the callback returns false
	 */
	if (pendq_is_discovery(ctrl, pq) == true) {
#if CONFIG_CANIOT_CONTROLLER_DISCOVERY
		if (!is_error || ctrl->discovery.params.admit_errors == true) {
			bool continue_discovery = ctrl->discovery.params.user_callback(
				ctrl, ev.did, ev.response, ctrl->discovery.params.user_data);
			if (continue_discovery == false) {
				stop_discovery(ctrl);
			}
		}

		/* If a discovery is running, call the user callback,
		 * but consider the response as an orphan response, as
		 * a discovery initiated the query */
		orphan_resp_event(ctrl, response);
#endif
	} else {
		/* call pq user callback and release pq context if the callback
		 * returns false */
		bool release_pq = call_user_callback(ctrl, &ev) == false;

		if (release_pq) {
			pendq_remove(ctrl, pq);
		}
	}
}

/**
 * @brief Pass frame to pending query
 *
 * @param ctrl
 * @param pq
 * @param frame
 * @return true If frame can actually be handled by the pending query
 * @return false Otherwise
 */
static bool pendq_handle_frame(struct caniot_controller *ctrl,
							   struct pendq *pq,
							   const struct caniot_frame *response)
{
	ASSERT(pq != NULL);
	ASSERT(response != NULL);

	bool is_error;

	if (!is_response_to(response, pq, &is_error)) {
		return false;
	} else if (pendq_is_broadcast(pq)) {
		pendq_handle_broadcast_resp(ctrl, pq, response, is_error);
	} else {
		pendq_handle_device_resp(ctrl, pq, response, is_error);
	}

	return true;
}

static int caniot_controller_handle_rx_frame(struct caniot_controller *ctrl,
											 const struct caniot_frame *frame)
{
#if CONFIG_CANIOT_CHECKS
	if (!ctrl || !frame) return -CANIOT_EINVAL;
	if (!caniot_controller_is_target(frame)) return -CANIOT_EUNEXPECTED;
#endif

	struct pendq *pq;
	bool orphan			   = true;
	const caniot_did_t did = CANIOT_DID(frame->id.cls, frame->id.sid);

	/* If a query is pending and the frame is the response for it
	 * Call callback and clear pending query */

	/* Try pass frame to query pending for this DID */
	pq = peek_pending_query(ctrl, did);
	if (pq != NULL) {
		orphan &= !pendq_handle_frame(ctrl, pq, frame);
	}

	/* Try pass frame to query pending for broadcast */
	pq = peek_pending_query(ctrl, CANIOT_DID_BROADCAST);
	if (pq != NULL) {
		orphan &= !pendq_handle_frame(ctrl, pq, frame);
	}

	/* If frame is not a response to any pending query, call orphan callback */
	if (orphan) {
		orphan_resp_event(ctrl, frame);
	}

	return 0;
}

int caniot_controller_rx_frame(struct caniot_controller *ctrl,
							   uint32_t time_passed_ms,
							   const struct caniot_frame *frame)
{
#if CONFIG_CANIOT_CHECKS
	if (!ctrl) return -CANIOT_EINVAL;
#endif

	if (frame != NULL) {
		int ret;
		if ((ret = caniot_controller_handle_rx_frame(ctrl, frame)) < 0) {
			return ret;
		}
	}

	/* update timeouts */
	pendq_shift(ctrl, time_passed_ms);

	/* call callbacks for expired queries */
	pendq_call_expired(ctrl);

	__DBG("caniot_controller_rx_frame(time_passed_ms: %u, frame: %p) -> ret: 0\n",
		  time_passed_ms,
		  (void *)frame);

	return 0U;
}

int caniot_controller_deinit(struct caniot_controller *ctrl)
{
#if CONFIG_CANIOT_CHECKS
	if (!ctrl) return -CANIOT_EINVAL;
#endif

	/* Iterate over all pending queries and cancel them */
	struct pendq *pq;

	while ((pq = pendq_pop(&ctrl->pendingq.timeout_queue)) != NULL) {
		cancelled_query_event(ctrl, pq, true);
	}

	return 0;
}

/*____________________________________________________________________________*/

#if CONFIG_CANIOT_CTRL_DRIVERS_API

int caniot_controller_send(struct caniot_controller *ctrl,
						   caniot_did_t did,
						   struct caniot_frame *frame)
{
	/* As timeout is 0U, no context is created and the frame is
	 * sent immediately. Without expecting a response. */
	const int ret = caniot_controller_query(ctrl, did, frame, 0u);

	ASSERT(ret <= 0);

	return ret;
}

int caniot_controller_query(struct caniot_controller *ctrl,
							caniot_did_t did,
							struct caniot_frame *frame,
							uint32_t timeout)
{
	int ret = query(ctrl, did, frame, timeout, true);

	__DBG("caniot_controller_query(did: %u, frame: %p, timeout: %u) -> ret (handle): "
		  "%d\n",
		  did,
		  (void *)frame,
		  timeout,
		  ret);

	return ret;
}

static uint32_t process_get_diff_ms(struct caniot_controller *ctrl)
{
	ASSERT(ctrl != NULL);
	ASSERT(ctrl->driv != NULL);
	ASSERT(ctrl->driv->get_time != NULL);

	const uint32_t last_sec = ctrl->last_process.sec;
	const uint16_t last_ms	= ctrl->last_process.ms;

	ctrl->driv->get_time(&ctrl->last_process.sec, &ctrl->last_process.ms);

	return (ctrl->last_process.sec - last_sec) * 1000 + ctrl->last_process.ms - last_ms;
}

int caniot_controller_process(struct caniot_controller *ctrl)
{
#if CONFIG_CANIOT_CHECKS
	if (!ctrl) return -CANIOT_EINVAL;
#endif

	ASSERT(ctrl->driv != NULL);
	ASSERT(ctrl->driv->recv != NULL);

	int ret;
	struct caniot_frame frame;

	while (true) {
		ret = ctrl->driv->recv(&frame);
		if (ret == 0) {
			if ((ret = caniot_controller_handle_rx_frame(ctrl, &frame)) < 0) {
				return ret;
			}
		} else if (ret == -CANIOT_EAGAIN) {
			break;
		} else {
			return ret;
		}
	}

	/* update timeouts */
	pendq_shift(ctrl, process_get_diff_ms(ctrl));

	/* call callbacks for expired queries */
	pendq_call_expired(ctrl);

	return 0;
}

#endif

#if CONFIG_CANIOT_CONTROLLER_DISCOVERY

static void stop_discovery(struct caniot_controller *ctrl)
{
	ASSERT(ctrl != NULL);

	ctrl->discovery.pending = 0u;
}

int caniot_controller_discovery_start(struct caniot_controller *ctrl,
									  const struct caniot_discovery_params *params)
{
	int ret;
	struct caniot_frame frame;

#if CONFIG_CANIOT_CHECKS
	if (!ctrl || !params || !params->user_callback || !params->timeout)
		return -CANIOT_EINVAL;
#endif

	if (ctrl->discovery.pending) return -CANIOT_EBUSY;

	/* Copy discovery params to controller */
	memcpy(&ctrl->discovery.params, params, sizeof(struct caniot_discovery_params));

	switch (params->mode) {

	/* In active mode, we send a query frame and register a pendq */
	case CANIOT_DISCOVERY_MODE_ACTIVE: {
		if (params->type == CANIOT_DISCOVERY_TYPE_TELEMETRY) {
			ret = caniot_build_query_telemetry(&frame, params->data.endpoint);
		} else if (params->type == CANIOT_DISCOVERY_TYPE_ATTRIBUTE) {
			ret = caniot_build_query_read_attribute(&frame, params->data.attr_key);
		} else {
			ret = -CANIOT_ENOTSUP;
		}

		if (ret) return ret;

		ret =
			caniot_controller_query(ctrl, CANIOT_DID_BROADCAST, &frame, params->timeout);
		if (ret >= 0) {
			ctrl->discovery.handle = ret;
		} else {
			stop_discovery(ctrl);
			return ret;
		}
		break;
	}

	/* In passive mode, we just listen for incoming frames */
	case CANIOT_DISCOVERY_MODE_PASSIVE: {

		/* Passive mode does not support attribute discovery */
		if (params->type == CANIOT_DISCOVERY_TYPE_ATTRIBUTE) return -CANIOT_ENOTSUP;

		ctrl->discovery.handle = 0u; /* No pendq */
		ret					   = 0;
		break;
	}

	default:
		return -CANIOT_ENOTSUP;
	}

	ctrl->discovery.pending = 1u;

	return ret;
}

int caniot_controller_discovery_stop(struct caniot_controller *ctrl)
{
#if CONFIG_CANIOT_CHECKS
	if (!ctrl) return -CANIOT_EINVAL;
#endif

	if (caniot_controller_discovery_running(ctrl)) {
		caniot_controller_query_cancel(ctrl, ctrl->discovery.handle, false);
	}

	return 0u;
}

bool caniot_controller_discovery_running(struct caniot_controller *ctrl)
{
	if (!ctrl) return false;

	return ctrl->discovery.pending == 1u;
}

#endif /* CONFIG_CANIOT_CONTROLLER_DISCOVERY */

/*____________________________________________________________________________*/

int caniot_controller_dbg_free_pendq(struct caniot_controller *ctrl)
{
	ASSERT(ctrl);

	uint32_t count = 0U;
	struct caniot_pendq *pq;

	for (pq = ctrl->pendingq.free_list; pq != NULL; pq = pq->next) {
		count++;
	}

	return count;
}

const char *caniot_controller_event_context_to_str(caniot_controller_event_context_t ctx)
{
	switch (ctx) {
	case CANIOT_CONTROLLER_EVENT_CONTEXT_ORPHAN:
		return "orphan";
	case CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY:
		return "query";
	default:
		return "<unknown>";
	}
}

const char *caniot_controller_event_status_to_str(caniot_controller_event_status_t status)
{
	switch (status) {
	case CANIOT_CONTROLLER_EVENT_STATUS_OK:
		return "ok";
	case CANIOT_CONTROLLER_EVENT_STATUS_ERROR:
		return "error";
	case CANIOT_CONTROLLER_EVENT_STATUS_TIMEOUT:
		return "timeout";
	case CANIOT_CONTROLLER_EVENT_STATUS_CANCELLED:
		return "cancelled";
	default:
		return "<unknown>";
	}
}

bool caniot_controller_dbg_event_cb_stub(const caniot_controller_event_t *ev,
										 void *user_data)
{
	CANIOT_INF("cb stub ev: %p user: %p did: %u handle: %u ctx: %s (%u) status: %s (%u) "
			   "response: %p terminated: %u (ev pq: user: %p)\n",
			   (void *)ev,
			   user_data,
			   ev->did,
			   ev->handle,
			   caniot_controller_event_context_to_str(ev->context),
			   ev->context,
			   caniot_controller_event_status_to_str(ev->status),
			   ev->status,
			   (void *)ev->response,
			   ev->terminated,
			   ev->user_data);

	return true;
}