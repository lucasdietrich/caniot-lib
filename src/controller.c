#include <caniot/controller.h>
#include <caniot/archutils.h>

#define pendq caniot_pendq
#define pqt caniot_pqt

// Get deviceid from frame
static inline caniot_did_t get_deviceid(const struct caniot_frame *frame)
{
	return CANIOT_DID(frame->id.cls, frame->id.sid);
}

static bool is_query_pending_for(struct caniot_controller *ctrl,
				 caniot_did_t did)
{
	return ctrl->pendingq.pending_devices_bf & (1LLU << did);
}

static void mark_query_pending_for(struct caniot_controller *ctrl,
				   caniot_did_t did,
				   bool status)
{
	if (status == true) {
		ctrl->pendingq.pending_devices_bf |= (1LLU << did);
	} else {
		ctrl->pendingq.pending_devices_bf &= ~(1LLU << did);
	}
}

// Finalize frame with device id
static void finalize_query_frame(struct caniot_frame *frame, caniot_did_t did)
{
	frame->id.query = CANIOT_QUERY;

	frame->id.cls = CANIOT_DID_CLS(did);
	frame->id.sid = CANIOT_DID_SID(did);
}

static void _pendq_queue(struct pqt **root, struct pqt *item)
{
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
	if (pq == NULL)
		return;

	/* last item doesn't have a "next" item */
	pq->tie.next = NULL;
	pq->tie.timeout = timeout;

	_pendq_queue(&ctrl->pendingq.timeout_queue, &pq->tie);
}

static void pendq_shift(struct caniot_controller *ctrl, uint32_t time_passed)
{
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

static struct pendq *pendq_get_expired(struct pqt **root)
{
	if ((*root != NULL) && ((*root)->delay == 0)) {
		struct pqt *item = *root;
		*root = (*root)->next;

		return CONTAINER_OF(item, struct pendq, tie);
	}
	return NULL;
}

static void pendq_remove(struct caniot_controller *ctrl, struct pendq *pq)
{
	struct pqt **prev_next_p = &ctrl->pendingq.timeout_queue;
	while (*prev_next_p != NULL) {
		struct pqt *p_current = *prev_next_p;
		if (p_current == &pq->tie) {
			*prev_next_p = p_current->next;
			if (p_current->next != NULL) {
				p_current->next->delay
					+= pq->tie.delay;
			}
			pq->tie.next = NULL;
			break;
		}
		prev_next_p = &(p_current->next);
	}
}

static void pendq_init_queue(struct caniot_controller *ctrl)
{
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
	struct pendq *p = ctrl->pendingq.free_list;
	if (p != NULL) {
		ctrl->pendingq.free_list = p->next;
	}
	return p;
}

static void pendq_free(struct caniot_controller *ctrl, struct pendq *p)
{
	if (p != NULL) {
		p->next = ctrl->pendingq.free_list;
		ctrl->pendingq.free_list = p;
	}
}

static struct pendq *pendq_find(struct caniot_controller *ctrl,
				caniot_did_t did)
{
	struct pqt *tie;
	for (tie = ctrl->pendingq.timeout_queue; tie != NULL; tie = tie->next) {
		struct pendq *const pq = CONTAINER_OF(tie, struct pendq, tie);
		if (CANIOT_DID_EQ(pq->did, did)) {
			return pq;
		}
	}
	return NULL;
}

static struct pendq *peek_pending_query(struct caniot_controller *ctrl,
					caniot_did_t did)
{
	struct pendq *pq = NULL;
	if (is_query_pending_for(ctrl, did)) {
		pq = pendq_find(ctrl, did);
	}
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

static void user_event(struct caniot_controller *ctrl,
		       const caniot_controller_event_t *ev,
		       struct pendq *pq)
{
	ASSERT(ctrl != NULL);
	ASSERT(ev != NULL);
	ASSERT(ctrl->event_cb != NULL);

	bool unregister = false;
	
	/* already mark as terminated, so we can query the same device 
	 * immediately in the following controller callback
	 */
	if (ev->terminated == CANIOT_CONTROLLER_QUERY_TERMINATED) {
		unregister = true;
		mark_query_pending_for(ctrl, ev->did, false);
	}

	/* if callback returns false, the query should be aborted if not terminated */
	if ((ctrl->event_cb(ev, ctrl->user_data) == false) && !unregister) {
		unregister = true;
		mark_query_pending_for(ctrl, ev->did, false);
	}

	/* unregister the pending query */
	if (unregister && (pq != NULL)) {
		pendq_remove(ctrl, pq);
		pendq_free(ctrl, pq);
	}
}

static void orphan_resp_event(struct caniot_controller *ctrl,
			      const struct caniot_frame *response)
{
	ASSERT(ctrl != NULL);
	ASSERT(response != NULL);

	const caniot_controller_event_t ev = {
		.controller = ctrl,
		.context = CANIOT_CONTROLLER_EVENT_CONTEXT_ORPHAN,
		.status = CANIOT_CONTROLLER_EVENT_STATUS_OK,

		.did = CANIOT_DID(response->id.cls, response->id.sid),

		.terminated = 1U, /* meaningless in this context */
		.handle = 0U, /* meaningless in this context */

		.response = response,
	};

	user_event(ctrl, &ev, NULL);
}

static void ignored_query_event(struct caniot_controller *ctrl,
				struct pendq *pq)
{
	ASSERT(pq != NULL);

	const caniot_controller_event_t ev = {
		.controller = ctrl,
		.context = CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY,
		.status = CANIOT_CONTROLLER_EVENT_STATUS_IGNORED,

		.did = pq->did,

		.terminated = 1U,
		.handle = pq->handle,

		.response = NULL,
	};

	user_event(ctrl, &ev, pq);
}

static void expired_query_event(struct caniot_controller *ctrl,
				struct pendq *pq)
{
	ASSERT(pq != NULL);

	const caniot_controller_event_t ev = {
		.controller = ctrl,
		.context = CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY,
		.status = CANIOT_CONTROLLER_EVENT_STATUS_TIMEOUT,
		
		.did = pq->did,

		.terminated = 1U,
		.handle = pq->handle,

		.response = NULL,
	};

	user_event(ctrl, &ev, pq);
}

static void resp_query_event(struct caniot_controller *ctrl,
			     const struct caniot_frame *response,
			     struct pendq *pq)
{
	ASSERT(pq != NULL);

	const bool is_broadcast = caniot_is_broadcast(pq->did);

	const caniot_controller_event_t ev = {
		.controller = ctrl,
		.context = CANIOT_CONTROLLER_EVENT_CONTEXT_QUERY,
		.status = caniot_is_error_frame(response->id) ?
				CANIOT_CONTROLLER_EVENT_STATUS_ERROR :
				CANIOT_CONTROLLER_EVENT_STATUS_OK,

		.did = CANIOT_DID(response->id.cls, response->id.sid),

		.terminated = is_broadcast == false,
		.handle = pq->handle,

		.response = response,
	};

	user_event(ctrl, &ev, pq);
}

static void pendq_call_expired(struct caniot_controller *ctrl)
{
	struct pendq *pq;
	struct pqt **const q = &ctrl->pendingq.timeout_queue;

	while ((pq = pendq_get_expired(q)) != NULL) {
		expired_query_event(ctrl, pq);
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

static struct pendq *pendq_prepare(struct caniot_controller *ctrl,
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

	struct pendq *pq = pendq_prepare(ctrl, did, frame->id.type, timeout);
	if (pq == NULL) {
		ret = -CANIOT_EPQALLOC;
		goto exit;
	}

	/* finalize and send the query frame */
	finalize_query_frame(frame, did);

	/* send frame */
	ret = ctrl->driv->send(frame, 0U);
	if (ret < 0) {
		goto exit;
	}

	/* add query to pending query */
	pendq_queue(ctrl, pq, timeout);

	/* tells that a query is pending for the device */
	mark_query_pending_for(ctrl, did, true);

	ret = 0;

exit:
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

int caniot_controller_handle_rx_frame(struct caniot_controller *ctrl,
				      const struct caniot_frame *response)
{
	// Assert that frame is not NULL and is a response
	if (!response || response->id.query != CANIOT_RESPONSE) {
		return -CANIOT_EMLFRM;
	}

	const caniot_did_t did = CANIOT_DID(response->id.cls, response->id.sid);

	// If a query is pending and the frame is the response for it
	// Call callback and clear pending query
	struct pendq *pq = peek_pending_query(ctrl, did);
	if (pq == NULL) {
		orphan_resp_event(ctrl, response);
	} else if ((true == caniot_type_is_valid_response_of(response->id.type,
							     pq->query_type)) ||
		   (true == caniot_is_error_frame(response->id))) {
		resp_query_event(ctrl, response, pq);
	} else {
		ignored_query_event(ctrl, pq);
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