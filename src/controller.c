#include "controller.h"

#define pendq caniot_pendq
#define pqt caniot_pqt

#define CONTAINER_OF(ptr, type, field) ((type *)(((char *)(ptr)) - offsetof(type, field)))

// Get deviceid from frame
static inline union deviceid get_deviceid(const struct caniot_frame *frame)
{
	return CANIOT_DEVICE(frame->id.cls, frame->id.sid);
}

static bool is_query_pending(struct caniot_device_entry *device)
{
	return device->flags.pending == 1U;
}

// Finalize frame with device id
static void finalize_query_frame(struct caniot_frame *frame, union deviceid did)
{
	frame->id.query = query;

	frame->id.cls = did.cls;
	frame->id.sid = did.sid;
}

// Return device entry corresponding to device id or last entry if broadcast
static struct caniot_device_entry *get_device_entry(struct caniot_controller *ctrl,
						    union deviceid did)
{
	if (caniot_valid_deviceid(did)) {
		return ctrl->devices + did.val;
	}

	return NULL;
}

// Get device id from device entry
static inline union deviceid get_device_id(struct caniot_controller *ctrl,
					   struct caniot_device_entry *entry)
{
	return (union deviceid) { .val = entry - ctrl->devices };
}

static inline bool device_is_broadcast(struct caniot_controller *ctrl,
				       struct caniot_device_entry *entry)
{
	return CANIOT_DEVICE_IS_BROADCAST(get_device_id(ctrl, entry));
}

static void _pendq_queue(struct caniot_controller *ctrl, struct pqt *item)
{
	struct pqt **prev_next_p = &ctrl->pendingq.timeout_queue;
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

	_pendq_queue(ctrl, &pq->tie);
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

static struct pendq *pendq_get_expired(struct caniot_controller *ctrl)
{
	struct pqt *item;
	struct pendq *pq = NULL;
	struct pqt **root = &ctrl->pendingq.timeout_queue;

	if ((*root != NULL) && ((*root)->delay == 0)) {
		item = *root;
		*root = (*root)->next;
	}

	if (item != NULL) {
		pq = CONTAINER_OF(item, struct pendq, tie);
	}

	return pq;
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
	     cur < ctrl->pendingq.pool + (CANIOT_MAX_PENDING_QUERIES - 1); cur++) {
		cur->next = cur + 1;
	}
	ctrl->pendingq.pool[CANIOT_MAX_PENDING_QUERIES - 1].next = NULL;

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

static struct pendq *pendq_find(struct caniot_controller *ctrl, union deviceid did)
{
	struct pqt *tie;
	for (tie = ctrl->pendingq.timeout_queue; tie != NULL; tie = tie->next) {
		struct pendq *const pq = CONTAINER_OF(tie, struct pendq, tie);
		if (pq->did.val == did.val) {
			return pq;
		}
	}
	return NULL;
}

static struct pendq *get_pending_query(struct caniot_controller *ctrl,
				       struct caniot_device_entry *entry)
{
	struct pendq *pq = NULL;
	if (is_query_pending(entry)) {
		pq = pendq_find(ctrl, get_device_id(ctrl, entry));
	}
	return pq;
}

// Initialize controller structure
int caniot_controller_init(struct caniot_controller *ctrl)
{
	// initialize devices array
	memset(ctrl->devices, 0, sizeof(ctrl->devices));

	pendq_init_queue(ctrl);

	return 0;
}

int caniot_controller_deinit(struct caniot_controller *ctrl)
{
	return 0;
}

static int pendq_register(struct caniot_controller *controller,
			  struct caniot_device_entry *device,
			  caniot_query_callback_t cb,
			  int32_t timeout)
{
	/* nothing to do */
	if (cb == NULL) {
		return 0;
	}

	/* invalid timeout value */
	if (timeout <= 0) {
		return -CANIOT_EINVAL;
	}

	/* another query is already pending */
	if (is_query_pending(device)) {
		return -CANIOT_EAGAIN;
	}

	struct pendq *pq = pendq_alloc(controller);
	if (pq == NULL) {
		return -CANIOT_EPQALLOC;
	}

	/* prepare data */
	pq->callback = cb;
	pq->did = get_device_id(controller, device);

	/* add query to pending query */
	pendq_queue(controller, pq, timeout);

	/* tells that a query is pending */
	device->flags.pending = 1U;

	return 0;
}

static int pendq_call_and_unregister(struct caniot_controller *controller,
				     struct pendq *pq,
				     struct caniot_frame *frame)
{
	int ret = -CANIOT_ENOPQ;

	if (pq != NULL) {
		/* void should we discard the callback ? */
		ret = pq->callback(pq->did, frame);
		pendq_remove(controller, pq);
		pendq_free(controller, pq);
	}

	return ret;
}

static int pendq_call_expired(struct caniot_controller *controller)
{
	int ret;
	struct pendq *pq;

	for (pq = pendq_get_expired(controller); pq != NULL;
	     pq = pendq_get_expired(controller)) {
		ret = pendq_call_and_unregister(controller, pq, NULL);
	}

	return ret;
}

static uint32_t get_diff_ms(struct caniot_controller *controller)
{
	uint32_t sec;
	uint16_t ms;
	controller->driv->get_time(&sec, &ms);

	uint16_t diff_ms = ms - controller->last_process.ms;
	diff_ms += (sec - controller->last_process.sec) * 1000;

	return diff_ms;
}

static void pendq_process(struct caniot_controller *ctrl)
{
	/* update timeouts */
	pendq_shift(ctrl, get_diff_ms(ctrl));

	/* call callbacks for expired queries */
	pendq_call_expired(ctrl);
}

// Send query to device and prepare response callback
int caniot_controller_query(struct caniot_controller *controller,
			    union deviceid did,
			    struct caniot_frame *frame,
			    caniot_query_callback_t cb,
			    int32_t timeout)
{
	int ret;
	struct caniot_device_entry *device;

	/* validate arguments */
	if (controller == NULL || frame == NULL) {
		return -CANIOT_EINVAL;
	}

	device = get_device_entry(controller, did);
	if (device == NULL) {
		return -CANIOT_EDEVICE;
	}

	ret = pendq_register(controller, device, cb, timeout);
	if (ret < 0) {
		goto error;
	}

	/* finalize and send the query frame */
	finalize_query_frame(frame, did);
	const uint32_t deffered = 0U;
	ret = controller->driv->send(frame, deffered);
	if (ret < 0) {
		goto error;
	}

	return 0;

error:
	device->flags.pending = 0U;
	return ret;
}

static inline int prepare_request_telemetry(struct caniot_frame *frame,
					    uint8_t ep)
{
	if (ep > endpoint_broadcast) {
		return -CANIOT_EINVAL;
	}

	frame->id.endpoint = ep;
	frame->id.type = telemetry;
	frame->len = 0;

	return 0;
}

int caniot_request_telemetry(struct caniot_controller *ctrl,
			     union deviceid did,
			     uint8_t ep,
			     caniot_query_callback_t cb,
			     int32_t timeout)
{
	int ret;
	struct caniot_frame frame;

	ret = prepare_request_telemetry(&frame, ep);
	if (ret < 0) {
		return ret;
	}

	return caniot_controller_query(ctrl, did, &frame, cb, timeout);
}

static inline int prepare_command(struct caniot_frame *frame,
				  uint8_t ep,
				  uint8_t *buf,
				  uint8_t len)
{
	if (buf == NULL) {
		return -CANIOT_EINVAL;
	}

	len = len > sizeof(frame->buf) ? sizeof(frame->buf) : len;

	frame->id.endpoint = ep;
	frame->id.type = command;
	frame->len = len;
	memcpy(frame->buf, buf, len);

	return 0;
}

int caniot_command(struct caniot_controller *ctrl,
		   union deviceid did,
		   uint8_t ep,
		   uint8_t *buf,
		   uint8_t len,
		   caniot_query_callback_t cb,
		   int32_t timeout)
{
	int ret;
	struct caniot_frame frame;

	ret = prepare_command(&frame, ep, buf, len);
	if (ret < 0) {
		return ret;
	}

	return caniot_controller_query(ctrl, did, &frame, cb, timeout);
}

static inline int prepare_read_attribute(struct caniot_frame *frame,
					 uint16_t key)
{
	frame->id.type = read_attribute;
	frame->len = 2u;
	frame->attr.key = key;

	return 0;
}

int caniot_read_attribute(struct caniot_controller *ctrl,
			  union deviceid did,
			  uint16_t key,
			  caniot_query_callback_t cb,
			  int32_t timeout)
{
	int ret;
	struct caniot_frame frame;

	ret = prepare_read_attribute(&frame, key);
	if (ret < 0) {
		return ret;
	}

	return caniot_controller_query(ctrl, did, &frame, cb, timeout);
}

static inline int prepare_write_attribute(struct caniot_frame *frame,
					  uint16_t key,
					  uint32_t value)
{
	frame->id.type = write_attribute;
	frame->len = 6u;
	frame->attr.key = key;
	frame->attr.val = value;

	return 0;
}

int caniot_write_attribute(struct caniot_controller *ctrl,
			   union deviceid did,
			   uint16_t key,
			   uint32_t value,
			   caniot_query_callback_t cb,
			   int32_t timeout)
{
	int ret;
	struct caniot_frame frame;

	ret = prepare_write_attribute(&frame, key, value);
	if (ret < 0) {
		return ret;
	}

	return caniot_controller_query(ctrl, did, &frame, cb, timeout);
}

int caniot_discover(struct caniot_controller *ctrl,
		    caniot_query_callback_t cb,
		    int32_t timeout)
{
	struct caniot_frame frame;

	prepare_request_telemetry(&frame, endpoint_default);

	return caniot_controller_query(ctrl, CANIOT_DEVICE_BROADCAST,
				       &frame, cb, timeout);
}

int caniot_controller_handle_rx_frame(struct caniot_controller *ctrl,
				      struct caniot_frame *frame)
{
	// Assert that frame is not NULL and is a response
	if (!frame || frame->id.query != response) {
		return -CANIOT_EMLFRM;
	}

	// Get device entry from frame device id
	struct caniot_device_entry *device =
		get_device_entry(ctrl, get_deviceid(frame));
	if (!device) {
		return -CANIOT_EDEVICE;
	}

	// If a query is pending and the frame is the response for it
	// Call callback and clear pending query
	pendq_call_and_unregister(ctrl,
				  get_pending_query(ctrl, device),
				  frame);


	// Update last seen timestamp for entry
	ctrl->driv->get_time(&device->last_seen, NULL);

	return 0;
}

static int process_incoming_frames(struct caniot_controller *ctrl)
{
	int ret;
	struct caniot_frame frame;

	while (true) {
		ret = ctrl->driv->recv(&frame);
		if (ret == 0) {
			ret = caniot_controller_handle_rx_frame(ctrl, &frame);
			printf(F("Processing frame returned: -%04x\n"), -ret);
		} else if (ret == -CANIOT_EAGAIN) {
			break;
		} else {
			return ret;
		}
	}

	return 0;
}

int caniot_controller_process(struct caniot_controller *ctrl)
{
	int ret = process_incoming_frames(ctrl);

	if (ret == 0) {
		pendq_process(ctrl);
	}

	return ret;
}

bool caniot_controller_is_target(struct caniot_frame *frame)
{
	return frame->id.query == response;
}