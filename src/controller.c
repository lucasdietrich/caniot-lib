#include <caniot/controller.h>
#include <caniot/archutils.h>

#define pendq caniot_pendq
#define pqt caniot_pqt

#define __DBG(fmt, ...) CANIOT_DBG("-- " fmt, ## __VA_ARGS__)

#define INVALID_HANDLE ((uint8_t) 0x00U)

static bool is_query_pending_for(struct caniot_controller *ctrl,
				 caniot_did_t did)
{
	ASSERT(ctrl != NULL);

	const bool result = ctrl->pendingq.pending_devices_bf & (1LLU << did);

	__DBG("is_query_pending_for(%u) -> %u\n", did, (uint32_t)result);

	return result;
}

static void mark_query_pending_for(struct caniot_controller *ctrl,
				   caniot_did_t did,
				   bool status)
{
	ASSERT(ctrl != NULL);

	__DBG("mark_query_pending_for(%u, %u)\n", did, status);

	if (status == true) {
		ctrl->pendingq.pending_devices_bf |= (1LLU << did);
	} else {
		ctrl->pendingq.pending_devices_bf &= ~(1LLU << did);
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

static void pendq_queue(struct caniot_controller *ctrl,
			struct pendq *pq, 
			uint32_t timeout)
{
	ASSERT(ctrl != NULL);
	ASSERT(timeout != CANIOT_TIMEOUT_FOREVER);

	if (pq == NULL)
		return;

	/* last item doesn't have a "next" item */
	pq->tie.next = NULL;
	pq->tie.timeout = timeout;

	_pendq_queue(&ctrl->pendingq.timeout_queue, &pq->tie);

	__DBG("pendq_queue(%p, %u)\n", pq, timeout);
}

static void pendq_shift(struct caniot_controller *ctrl, uint32_t time_passed)
{
	ASSERT(ctrl != NULL);

	__DBG("pendq_shift(%u)\n", time_passed);

	struct pqt **prev_next_p = &ctrl->pendingq.timeout_queue;
	while (*prev_next_p != NULL) {
		struct pqt *p_current = *prev_next_p;

		if (p_current->delay <= time_passed) {
			if (p_current->delay != 0) {
				time_passed -= p_current->delay;
				p_current->delay = 0;
			}
			prev_next_p = &(p_current->next);
		} else {
			p_current->delay -= time_passed;
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
		*root = (*root)->next;

		pq = CONTAINER_OF(item, struct pendq, tie);
	}

	__DBG("pendq_pop_expired() -> %p\n", pq);
	
	return pq;
}

static void pendq_remove(struct caniot_controller *ctrl, struct pendq *pq)
{
	ASSERT(ctrl != NULL);
	ASSERT(pq != NULL);

	struct pqt **prev_next_p = &ctrl->pendingq.timeout_queue;
	while (*prev_next_p != NULL) {
		struct pqt *p_current = *prev_next_p;
		if (p_current == &pq->tie) {
			__DBG("pendq_remove(%p) -> removed\n", pq);

			*prev_next_p = p_current->next;
			if (p_current->next != NULL) {
				p_current->next->delay
					+= pq->tie.delay;
			}
			pq->tie.next = NULL;
			return;
		}
		prev_next_p = &(p_current->next);
	}

	__DBG("pendq_remove(%p) -> not found\n", pq);
}

static void pendq_init_queue(struct caniot_controller *ctrl)
{
	ASSERT(ctrl != NULL);

	/* init free list */
	for (struct pendq *cur = ctrl->pendingq.pool;
	     cur < ctrl->pendingq.pool + (CANIOT_MAX_PENDING_QUERIES - 1U); cur++) {
		cur->next = cur + 1;
		cur->handle = INVALID_HANDLE;
	}
	ctrl->pendingq.pool[CANIOT_MAX_PENDING_QUERIES - 1U].next = NULL;

	ctrl->pendingq.free_list = ctrl->pendingq.pool;
	ctrl->pendingq.timeout_queue = NULL;
}

static struct pendq *pendq_alloc(struct caniot_controller *ctrl)
{
	ASSERT(ctrl != NULL);

	struct pendq *p = ctrl->pendingq.free_list;
	if (p != NULL) {
		__DBG("pendq_alloc() -> %p\n", p);

		ctrl->pendingq.free_list = p->next;
	} else {
		__DBG("pendq_alloc() -> NULL\n");
	}
	return p;
}

static void pendq_free(struct caniot_controller *ctrl, struct pendq *p)
{
	ASSERT(ctrl != NULL);

	if (p != NULL) {
		__DBG("pendq_free(%p)\n", p);

		p->next = ctrl->pendingq.free_list;
		p->handle = INVALID_HANDLE;
		ctrl->pendingq.free_list = p;
	} else {
		__DBG("pendq_free(NULL)\n");
	}
}

static struct pendq *pendq_get_by_did(struct caniot_controller *ctrl,
				      caniot_did_t did)
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

	__DBG("pendq_get_by_did(%u) -> %p\n", did, retpq);

	return retpq;
}

static struct pendq *pendq_get_by_handle(struct caniot_controller *ctrl,
					 uint8_t handle)
{
	ASSERT(ctrl);
	
	struct pendq *pq = NULL;

	if ((handle != INVALID_HANDLE) && (handle <= CANIOT_MAX_PENDING_QUERIES)) {
		struct pendq *const tmp = &ctrl->pendingq.pool[handle - 1U];

		if (tmp->handle == handle) {
			pq = tmp;
		}
	}

	__DBG("pendq_get_by_handle(%u) -> %p\n", handle, pq);

	return pq;
}

int caniot_controller_handle_set_user_data(struct caniot_controller *ctrl,
					   uint8_t handle,
					   void *user_data)
{
	int ret = -CANIOT_EINVAL;
	struct pendq *pq = pendq_get_by_handle(ctrl, handle);

	if (pq != NULL) {
		pq->user_data = user_data;
		ret = 0;
	}

	return ret;
}

static struct pendq *peek_pending_query(struct caniot_controller *ctrl,
					caniot_did_t did)
{
	ASSERT(ctrl != NULL);

	struct pendq *pq = NULL;
	if (is_query_pending_for(ctrl, did)) {
		pq = pendq_get_by_did(ctrl, did);
	}

	__DBG("peek_pending_query(%u) -> %p\n", did, pq);

	return pq;
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

	ctrl->event_cb = cb;
	ctrl->user_data = user_data;

	pendq_init_queue(ctrl);

exit:
	return ret;
}

int caniot_controller_driv_init(struct caniot_controller *ctrl,
				const struct caniot_drivers_api *driv,
				caniot_controller_event_cb_t cb,
				void *user_data)
{
	int ret = caniot_controller_init(ctrl, cb, user_data);
	if (ret < 0) {
		goto exit;
	}

#if CANIOT_CTRL_DRIVERS_API
	if (driv == NULL) {
		ret = -CANIOT_EDRIVER;
		goto exit;
	}
	ctrl->driv = driv;
#endif

exit:
	return ret;
}

int caniot_controller_deinit(struct caniot_controller *ctrl)
{
	ASSERT(ctrl);

	return -CANIOT_ENIMPL;
}

uint32_t caniot_controller_next_timeout(const struct caniot_controller *ctrl)
{
	ASSERT(ctrl);

	uint32_t next_timeout = (uint32_t)-1;

	struct pqt *next = ctrl->pendingq.timeout_queue;

	if (next != NULL) {
		next_timeout = next->timeout;
	}

	return next_timeout;
}

static bool user_event(struct caniot_controller *ctrl,
		       const caniot_controller_event_t *ev)
{
	ASSERT(ctrl != NULL);
	ASSERT(ev != NULL);
	ASSERT(ctrl->event_cb != NULL);

	__DBG("user_event(%p) -> x=%u s=%u t=%u did=%u h=%u resp=%p\n", 
	      ev, ev->context, ev->status, ev->terminated, 
	      ev->did, ev->handle, ev->response);

	return ctrl->event_cb(ev, ctrl->user_data);
}

static void orphan_resp_event(struct caniot_controller *ctrl,
			      const struct caniot_frame *response)
{
	ASSERT(ctrl != NULL);
	ASSERT(response != NULL);

	const caniot_did_t did = CANIOT_DID(response->id.cls, response->id.sid);

	const caniot_controller_event_t ev = {
		.controller = ctrl,
		.context = CANIOT_CONTROLLER_EVENT_CONTEXT_ORPHAN,
		.status = CANIOT_CONTROLLER_EVENT_STATUS_OK,

		.did = did,

		.terminated = 1U, /* meaningless in this context */
		.handle = 0U, /* meaningless in this context */

		.response = response,

		.user_data = NULL
	};

	user_event(ctrl, &ev);
}

static void resp_query_event(struct caniot_controller *ctrl,
			     const struct caniot_frame *response,
			     struct pendq *pq)
{
	ASSERT(pq != NULL);

	const bool terminated = caniot_is_broadcast(pq->did) == false;

	const caniot_controller_event_t ev = {
		.controller = ctrl,
		.context = CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY,
		.status = caniot_is_error_frame(response->id) ?
				CANIOT_CONTROLLER_EVENT_STATUS_ERROR :
				CANIOT_CONTROLLER_EVENT_STATUS_OK,

		.did = CANIOT_DID(response->id.cls, response->id.sid),

		.terminated = terminated,
		.handle = pq->handle,

		.response = response,
		.user_data = pq->user_data,
	};

	if (terminated) {
		mark_query_pending_for(ctrl, pq->did, false);
		pendq_remove(ctrl, pq);
		pendq_free(ctrl, pq);
	}	

	if ((user_event(ctrl, &ev) == false) && 
	    (terminated == false)) { /* only unregister if not previously terminated */
		mark_query_pending_for(ctrl, pq->did, false);
		pendq_remove(ctrl, pq);
		pendq_free(ctrl, pq);
	}
}

static void cancelled_query_event(struct caniot_controller *ctrl,
				  struct pendq *pq)
{
	ASSERT(pq != NULL);

	const caniot_controller_event_t ev = {
		.controller = ctrl,
		.context = CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY,
		.status = CANIOT_CONTROLLER_EVENT_STATUS_CANCELLED,

		.did = pq->did,

		.terminated = 1U,
		.handle = pq->handle,

		.response = NULL,
		.user_data = pq->user_data
	};

	mark_query_pending_for(ctrl, pq->did, false);
	pendq_remove(ctrl, pq);
	pendq_free(ctrl, pq);

	user_event(ctrl, &ev);
}

static void pendq_call_expired(struct caniot_controller *ctrl)
{
	ASSERT(ctrl != NULL);

	struct pendq *pq;
	struct pqt **const q = &ctrl->pendingq.timeout_queue;

	while ((pq = pendq_pop_expired(q)) != NULL) {
		const caniot_controller_event_t ev = {
			.controller = ctrl,
			.context = CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY,
			.status = CANIOT_CONTROLLER_EVENT_STATUS_TIMEOUT,

			.did = pq->did,

			.terminated = 1U,
			.handle = pq->handle,

			.response = NULL,
			.user_data = pq->user_data
		};

		mark_query_pending_for(ctrl, pq->did, false);
		pendq_free(ctrl, pq);

		user_event(ctrl, &ev);
	}
}

static struct pendq *pendq_alloc_and_prepare(struct caniot_controller *ctrl,
					     caniot_did_t did,
					     caniot_frame_type_t query_type,
					     uint32_t timeout)
{
	/* allocate */
	struct pendq *pq = pendq_alloc(ctrl);

	if (pq != NULL) {
		/* prepare data */
		pq->did = did;
		pq->handle = 1U + INDEX_OF(pq, ctrl->pendingq.pool, struct pendq);
		pq->query_type = query_type;
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
	if (ctrl == NULL || frame == NULL) {
		ret = -CANIOT_EINVAL;
		goto exit;
	}

	const bool alloc_context = timeout != 0U;
	struct pendq *pq = NULL;

	/* if timeout is defined, we need to allocate a context */
	if (alloc_context == true) {
		if (caniot_deviceid_valid(did) == false) {
			ret = -CANIOT_EDEVICE;
			goto exit;
		}

		/* another query is already pending for the device */
		if (is_query_pending_for(ctrl, did) == true) {
			ret = -CANIOT_EAGAIN;
			goto exit;
		}

		pq = pendq_alloc_and_prepare(ctrl, did, frame->id.type, timeout);
		if (pq == NULL) {
			ret = -CANIOT_EPQALLOC;
			goto exit;
		}
	}

	/* finalize and send the query frame */
	finalize_query_frame(frame, did);

#if CANIOT_CTRL_DRIVERS_API
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
	__DBG("query(%u, %p, %u, %u) -> %d\n",
	      did, frame, timeout, (uint32_t) driv_send, ret);

	return ret;
}

int caniot_controller_query_register(struct caniot_controller *ctrl,
				     caniot_did_t did,
				     struct caniot_frame *frame,
				     uint32_t timeout)
{
	int ret = query(ctrl, did, frame, timeout, false);

	__DBG("caniot_controller_query_register(%u, %p, %u) -> %d\n",
	      did, frame, timeout, ret);

	return ret;
}

bool caniot_controller_query_pending(struct caniot_controller *ctrl,
				     uint8_t handle)
{
	ASSERT(ctrl != NULL);

	struct pendq *const pq = pendq_get_by_handle(ctrl, handle);

	__DBG("caniot_controller_query_pending(%u) -> %u\n", handle, pq != NULL); 

	return pq != NULL;
}

int caniot_controller_cancel_query(struct caniot_controller *ctrl,
				   uint8_t handle,
				   bool suppress)
{
	int ret;

	/* validate arguments */
	if (ctrl == NULL) {
		ret = -CANIOT_EINVAL;
		goto exit;
	}

	struct pendq *pq = pendq_get_by_handle(ctrl, handle);
	if (pq == NULL) {
		ret = -CANIOT_ENOHANDLE;
		goto exit;
	}

	/* execute user callback if not suppressed */
	if (suppress == false) {
		cancelled_query_event(ctrl, pq);
	}

	ret = 0;
exit:
	__DBG("caniot_controller_cancel_query(%u, %u) -> %d\n", handle, suppress, ret);

	return ret;
}

static void caniot_controller_handle_rx_frame(struct caniot_controller *ctrl,
					     const struct caniot_frame *frame)
{
	ASSERT(ctrl);
	ASSERT(frame);
	ASSERT(caniot_controller_is_target(frame));

	const caniot_did_t did = CANIOT_DID(frame->id.cls, frame->id.sid);

	// If a query is pending and the frame is the response for it
	// Call callback and clear pending query
	struct pendq *pq = peek_pending_query(ctrl, did);
	if (pq != NULL) {
		bool is_error = false;
		const bool is_valid_response = caniot_type_is_response_of(
			frame->id.type, pq->query_type, &is_error
		);

		if (is_valid_response || is_error) {
			/* is response for pending query */
			resp_query_event(ctrl, frame, pq);
		} else {
			/* is not response for pending query */
			orphan_resp_event(ctrl, frame);
		}
	} else {
		/* not in a context of a query */
		orphan_resp_event(ctrl, frame);
	}
}

int caniot_controller_process_single(struct caniot_controller *ctrl,
				     uint32_t time_passed,
				     const struct caniot_frame *frame)
{
	ASSERT(ctrl);

	if (frame != NULL) {
		caniot_controller_handle_rx_frame(ctrl, frame);
	}

	/* update timeouts */
	pendq_shift(ctrl, time_passed);

	/* call callbacks for expired queries */
	pendq_call_expired(ctrl);

	__DBG("caniot_controller_process_single(%u, %p) -> 0\n", 
	      time_passed, frame);

	return 0U;
}

/*___________________________________________________________________________*/

#if CANIOT_CTRL_DRIVERS_API

int caniot_controller_send(struct caniot_controller *ctrl,
			   caniot_did_t did,
			   struct caniot_frame *frame)
{
	const int ret = caniot_controller_query(ctrl, did, frame, 0U);

	/* As timeout is 0U, no context is created and the frame is
	 * sent immediately. */
	ASSERT(ret <= 0);

	return ret;
}

int caniot_controller_query(struct caniot_controller *ctrl,
			    caniot_did_t did,
			    struct caniot_frame *frame,
			    uint32_t timeout)
{
	int ret = query(ctrl, did, frame, timeout, true);

	__DBG("caniot_controller_query(%u, %p, %u) -> %d\n",
	      did, frame, timeout, ret);

	return ret;
}

static uint32_t process_get_diff_ms(struct caniot_controller *ctrl)
{
	ASSERT(ctrl != NULL);
	ASSERT(ctrl->driv != NULL);
	ASSERT(ctrl->driv->get_time != NULL);

	const uint32_t last_sec = ctrl->last_process.sec;
	const uint16_t last_ms = ctrl->last_process.ms;

	ctrl->driv->get_time(&ctrl->last_process.sec,
			     &ctrl->last_process.ms);

	return (ctrl->last_process.sec - last_sec) * 1000 +
		ctrl->last_process.ms - last_ms;
}

int caniot_controller_process(struct caniot_controller *ctrl)
{
	ASSERT(ctrl != NULL);
	ASSERT(ctrl->driv != NULL);
	ASSERT(ctrl->driv->recv != NULL);
	
	int ret;
	struct caniot_frame frame;

	while (true) {
		ret = ctrl->driv->recv(&frame);
		if (ret == 0) {
			caniot_controller_handle_rx_frame(ctrl, &frame);
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

/*___________________________________________________________________________*/

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