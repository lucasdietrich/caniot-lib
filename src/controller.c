#include <caniot/controller.h>
#include <caniot/archutils.h>

#define pendq caniot_pendq
#define pqt caniot_pqt

#define __DBG(fmt, ...) CANIOT_DBG("-- " fmt, ## __VA_ARGS__)

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
			struct pendq *pq, uint32_t timeout)
{
	ASSERT(ctrl != NULL);

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

	__DBG("pendq_shift(%u)\n", time_passed);
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

	struct pendq *retpq = NULL;

	struct pqt *tie;
	for (tie = ctrl->pendingq.timeout_queue; tie != NULL; tie = tie->next) {
		struct pendq *const pq = CONTAINER_OF(tie, struct pendq, tie);
		if (pq->handle == handle) {
			retpq = pq;
			break;
		}
	}

	__DBG("pendq_get_by_handle(%u) -> %p\n", handle, retpq);

	return retpq;
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
			   const struct caniot_drivers_api *driv,
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

	if (driv == NULL) {
		ret = -CANIOT_EDRIVER;
		goto exit;
	}

	ctrl->event_cb = cb;
	ctrl->user_data = user_data;

	ctrl->driv = driv;

	pendq_init_queue(ctrl);

exit:
	return ret;
}

int caniot_controller_deinit(struct caniot_controller *ctrl)
{
	(void)ctrl;

	return -CANIOT_ENIMPL;
}

static bool user_event(struct caniot_controller *ctrl,
		       const caniot_controller_event_t *ev)
{
	ASSERT(ctrl != NULL);
	ASSERT(ev != NULL);
	ASSERT(ctrl->event_cb != NULL);

	__DBG("user_event(%p)\n", ev);

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
		};

		mark_query_pending_for(ctrl, pq->did, false);
		pendq_free(ctrl, pq);

		user_event(ctrl, &ev);
	}
}

static uint32_t process_get_diff_ms(struct caniot_controller *ctrl)
{
	const uint32_t last_sec = ctrl->last_process.sec;
	const uint16_t last_ms = ctrl->last_process.ms;

	ctrl->driv->get_time(&ctrl->last_process.sec,
			     &ctrl->last_process.ms);

	return (ctrl->last_process.sec - last_sec) * 1000 +
		ctrl->last_process.ms - last_ms;
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
		pq->handle = INDEX_OF(pq, ctrl->pendingq.pool, struct pendq);
		pq->query_type = query_type;
	}

	return pq;
}

static struct pendq *pendq_cancel(struct caniot_controller *ctrl,
				  struct pendq *pq)
{
	ASSERT(ctrl != NULL);
	ASSERT(pq != NULL);
}


// Send query to device and prepare response callback
int caniot_controller_query(struct caniot_controller *ctrl,
			    caniot_did_t did,
			    struct caniot_frame *frame,
			    uint32_t timeout)
{
	int ret;

	/* validate arguments */
	if (ctrl == NULL || frame == NULL) {
		ret = -CANIOT_EINVAL;
		goto exit;
	}

	/* invalid timeout value */
	if (timeout <= 0) {
		return -CANIOT_EINVAL;
	}

	if (caniot_deviceid_valid(did) == false) {
		ret = -CANIOT_EDEVICE;
		goto exit;
	}

	/* another query is already pending for the device */
	if (is_query_pending_for(ctrl, did) == true) {
		ret = -CANIOT_EAGAIN;
		goto exit;
	}

	struct pendq *pq = pendq_alloc_and_prepare(ctrl, did, frame->id.type, timeout);
	if (pq == NULL) {
		ret = -CANIOT_EPQALLOC;
		goto exit;
	}

	/* finalize and send the query frame */
	finalize_query_frame(frame, did);

	/* send frame */
	ret = ctrl->driv->send(frame, 0U);
	if (ret < 0) {
		/* deallocate immediately*/
		pendq_free(ctrl, pq);

		goto exit;
	}

	/* add query to pending query */
	pendq_queue(ctrl, pq, timeout);

	/* tells that a query is pending for the device */
	mark_query_pending_for(ctrl, did, true);

	ret = 0;

exit:
	__DBG("caniot_controller_query(%u, %p, %u) -> %d\n", did, frame, timeout, ret);

	return ret;
}

int caniot_controller_cancel(struct caniot_controller *ctrl,
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
	__DBG("caniot_controller_cancel(%u, %u) -> %d\n", handle, suppress, ret);

	return ret;
}

static inline int prepare_request_telemetry(struct caniot_frame *frame,
					    caniot_endpoint_t ep)
{
	// Can we request telemetry broadcast ? would say no
	if (ep > CANIOT_ENDPOINT_BOARD_CONTROL) {
		return -CANIOT_EEP;
	}

	frame->id.endpoint = ep;
	frame->id.type = CANIOT_FRAME_TYPE_TELEMETRY;
	frame->len = 0;

	return 0;
}

int caniot_request_telemetry(struct caniot_controller *ctrl,
			     caniot_did_t did,
			     caniot_endpoint_t ep,
			     uint32_t timeout)
{
	int ret;
	struct caniot_frame frame;

	ret = prepare_request_telemetry(&frame, ep);
	if (ret < 0) {
		return ret;
	}

	return caniot_controller_query(ctrl, did, &frame, timeout);
}

static inline int prepare_command(struct caniot_frame *frame,
				  caniot_endpoint_t ep,
				  uint8_t *buf,
				  uint8_t len)
{
	if (buf == NULL) {
		return -CANIOT_EINVAL;
	}

	len = len > sizeof(frame->buf) ? sizeof(frame->buf) : len;

	frame->id.endpoint = ep;
	frame->id.type = CANIOT_FRAME_TYPE_COMMAND;
	frame->len = len;
	memcpy(frame->buf, buf, len);

	return 0;
}

int caniot_command(struct caniot_controller *ctrl,
		   caniot_did_t did,
		   caniot_endpoint_t ep,
		   uint8_t *buf,
		   uint8_t len,
		   uint32_t timeout)
{
	int ret;
	struct caniot_frame frame;

	ret = prepare_command(&frame, ep, buf, len);
	if (ret < 0) {
		return ret;
	}

	return caniot_controller_query(ctrl, did, &frame, timeout);
}

static inline int prepare_read_attribute(struct caniot_frame *frame,
					 uint16_t key)
{
	frame->id.type = CANIOT_FRAME_TYPE_READ_ATTRIBUTE;
	frame->len = 2u;
	frame->attr.key = key;

	return 0;
}

int caniot_read_attribute(struct caniot_controller *ctrl,
			  caniot_did_t did,
			  uint16_t key,
			  uint32_t timeout)
{
	int ret;
	struct caniot_frame frame;

	ret = prepare_read_attribute(&frame, key);
	if (ret < 0) {
		return ret;
	}

	return caniot_controller_query(ctrl, did, &frame, timeout);
}

static inline int prepare_write_attribute(struct caniot_frame *frame,
					  uint16_t key,
					  uint32_t value)
{
	frame->id.type = CANIOT_FRAME_TYPE_WRITE_ATTRIBUTE;
	frame->len = 6u;
	frame->attr.key = key;
	frame->attr.val = value;

	return 0;
}

int caniot_write_attribute(struct caniot_controller *ctrl,
			   caniot_did_t did,
			   uint16_t key,
			   uint32_t value,
			   uint32_t timeout)
{
	int ret;
	struct caniot_frame frame;

	ret = prepare_write_attribute(&frame, key, value);
	if (ret < 0) {
		return ret;
	}

	return caniot_controller_query(ctrl, did, &frame, timeout);
}

int caniot_discover(struct caniot_controller *ctrl,
		    uint32_t timeout)
{
	struct caniot_frame frame;

	prepare_request_telemetry(&frame, CANIOT_ENDPOINT_APP);

	return caniot_controller_query(ctrl, CANIOT_DID_BROADCAST,
				       &frame, timeout);
}

static int caniot_controller_handle_rx_frame(struct caniot_controller *ctrl,
					     const struct caniot_frame *frame)
{
	// Assert that frame is not NULL and is a response
	if (!frame || frame->id.query != CANIOT_RESPONSE) {
		return -CANIOT_EMLFRM;
	}

	const caniot_did_t did = CANIOT_DID(frame->id.cls, frame->id.sid);

	// If a query is pending and the frame is the response for it
	// Call callback and clear pending query
	struct pendq *pq = peek_pending_query(ctrl, did);
	if (pq == NULL) {
		/* not in a context of a query */
		orphan_resp_event(ctrl, frame);
	} else if ((true == caniot_type_is_response_of(frame->id.type, 
						       pq->query_type, NULL))) {
		/* is response for pending query */
		resp_query_event(ctrl, frame, pq);
	} else {
		/* is not response for pending query */
		orphan_resp_event(ctrl, frame);
	}

	return 0;
}

int caniot_controller_process_frame(struct caniot_controller *ctrl,
				    const struct caniot_frame *frame)
{
	if (frame != NULL) {
		caniot_controller_handle_rx_frame(ctrl, frame);
	}

	/* update timeouts */
	pendq_shift(ctrl, process_get_diff_ms(ctrl));

	/* call callbacks for expired queries */
	pendq_call_expired(ctrl);

	return 0U;
}

int caniot_controller_process(struct caniot_controller *ctrl)
{
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