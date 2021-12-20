#include "controller.h"

#define pendq caniot_pendq
#define pqt caniot_pqt

#define CONTAINER_OF(ptr, type, field) ((type *)(((char *)(ptr)) - offsetof(type, field)))

// Get deviceid from frame
static inline union deviceid get_deviceid(const struct caniot_frame *frame)
{
	return CANIOT_DEVICE(frame->id.cls, frame->id.dev);
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
	frame->id.dev = did.dev;
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

// Send query to device and prepare response callback
static int caniot_controller_query(struct caniot_controller *controller,
				   union deviceid did,
				   struct caniot_frame *frame,
				   caniot_query_callback_t cb,
				   int32_t timeout)
{
	int ret;
	struct caniot_device_entry *device = get_device_entry(controller, did);

	/* get the device */
	if (device == NULL) {
		return -CANIOT_EDEVICE;
	}

	finalize_query_frame(frame, did);

	/* send the query frame */
	const uint32_t deffered = 0U;
	ret = controller->driv->send(frame, deffered);
	if (ret < 0) {
		goto error;
	}

	/* register the pending query */
	ret = pendq_register(controller, device, cb, timeout);
	if (ret != 0) {
		goto error;
	}

error:
	device->flags.pending = 0U;
	return ret;
}

int caniot_request_telemetry(struct caniot_controller *ctrl,
			     union deviceid did,
			     uint8_t ep,
			     caniot_query_callback_t cb,
			     int32_t timeout)
{
	struct caniot_frame frame = {
		.id.endpoint = ep,
		.id.type = telemetry,
		.len = 0,
	};

	return caniot_controller_query(ctrl, did, &frame, cb, timeout);
}

int caniot_send_command(struct caniot_controller *ctrl,
			union deviceid did,
			struct caniot_command *cmd,
			caniot_query_callback_t cb,
			int32_t timeout)
{
	struct caniot_frame frame = {
		.id.type = command,
		.id.endpoint = cmd->ep,
		.len = cmd->len,
	};

	memcpy(frame.buf, cmd->buf, cmd->len);

	return caniot_controller_query(ctrl, did, &frame, cb, timeout);
}

int caniot_read_attribute(struct caniot_controller *ctrl,
			  union deviceid did,
			  uint16_t key,
			  caniot_query_callback_t cb,
			  int32_t timeout)
{
	struct caniot_frame frame = {
		.id.type = read_attribute,
		.len = 2u,
		.attr.key = key
	};

	return caniot_controller_query(ctrl, did, &frame, cb, timeout);
}

int caniot_write_attribute(struct caniot_controller *ctrl,
			   union deviceid did,
			   uint16_t key,
			   uint32_t value,
			   caniot_query_callback_t cb,
			   int32_t timeout)
{
	struct caniot_frame frame = {
		.id.type = write_attribute,
		.len = 6u,
		.attr.key = key,
		.attr.val = value
	};

	return caniot_controller_query(ctrl, did, &frame, cb, timeout);
}

int caniot_discover(struct caniot_controller *ctrl,
		    caniot_query_callback_t cb,
		    int32_t timeout)
{
	struct caniot_frame frame = {
		.id.type = telemetry,
	};

	return caniot_controller_query(ctrl, CANIOT_DEVICE_BROADCAST,
				       &frame, cb, timeout);
}

int caniot_controller_handle_rx_frame(struct caniot_controller *ctrl,
				      struct caniot_frame *frame)
{
	// Assert that frame is not NULL and is a response
	if (!frame || frame->id.query != response) {
		return -CANIOT_EINVAL;
	}

	// Get device entry from frame device id
	union deviceid did = get_deviceid(frame);
	struct caniot_device_entry *entry = get_device_entry(ctrl, did);

	// If device entry is not found return error
	if (!entry) {
		return -CANIOT_EDEVICE;
	}

	// If a query is pending and the frame is the response for it
	// Call callback and clear pending query
	struct pendq *pq = get_pending_query(ctrl, entry);
	if (pq != NULL) {
		pq->callback(pq->did, frame);
		pendq_remove(ctrl, pq);
		pendq_free(ctrl, pq);
	}

	// Update last seen timestamp for entry
	ctrl->driv->get_time(&entry->last_seen, NULL);

	return 0;
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
			goto error;
		}
	}

	uint32_t sec, usec, diff, ms;
	ctrl->driv->get_time(&sec, &usec);
	ms = sec*1000U + usec/1000U; 
	diff = ctrl->last_process_ms - ms;
	ctrl->last_process_ms = ms;

	/* update timeouts */
	pendq_shift(ctrl, diff);

	/* call callbacks for expired queries */
	struct pendq *pq;
	while ((pq = pendq_get_expired(ctrl)) != NULL) {
		pq->callback(pq->did, NULL);
		pendq_remove(ctrl, pq);
		pendq_free(ctrl, pq);
	}

error:
	return ret;
}