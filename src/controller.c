#include "controller.h"

// Get deviceid from frame
static inline union deviceid get_deviceid(const struct caniot_frame *frame)
{
	return CANIOT_DEVICE(frame->id.cls, frame->id.dev);
}

// Clear caniot telemetry entry
static inline void telemetry_entry_clear(struct caniot_telemetry_entry *entry)
{
	memset(entry, 0x00, sizeof(*entry));
}

static bool has_config(struct caniot_controller *controller)
{
	return controller->cfg;
}

static bool telemetry_db_enabled(struct caniot_controller *controller)
{
	return controller->telemetry_db &&
		(!has_config(controller) || controller->cfg->store_telemetry);
}

// Build telemetry entry from frame
static int caniot_build_telemetry_entry(struct caniot_telemetry_entry *entry,
					struct caniot_frame *frame,
					uint32_t timestamp)
{
	telemetry_entry_clear(entry);

	entry->did = get_deviceid(frame);
	entry->timestamp = timestamp;
	entry->ep = frame->id.endpoint;
	memcpy(entry->buf, frame->buf, frame->len);

	return 0;
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

// Tells if a query is pending for device entry
static int is_query_pending(struct caniot_device_entry *entry)
{
	return entry->query.callback != NULL;
}

// Initialize controller structure
int caniot_controller_init(struct caniot_controller *ctrl)
{
	// initialize devices array
	memset(ctrl->devices, 0, sizeof(ctrl->devices));

	return ctrl->telemetry_db->init();
}

int caniot_controller_deinit(struct caniot_controller *ctrl)
{
	return ctrl->telemetry_db->deinit();
}

// Event handler
static void query_timeout_cb(void *event)
{
	struct caniot_device_entry *entry = (struct caniot_device_entry *) event;
	
	if (entry && is_query_pending(entry)) {
		entry->query.callback(get_device_id(entry->query.controller, 
						    entry),
				      NULL);
		entry->query.callback = NULL;
	}
}

// Send query to device and prepare response callback
static int caniot_controller_query(struct caniot_controller *controller,
				   union deviceid did,
				   struct caniot_frame *frame,
				   caniot_query_callback_t cb,
				   int32_t delay)
{
	int ret;
	struct caniot_device_entry *entry;

	entry = get_device_entry(controller, did);
	if (!entry) {
		return -CANIOT_EDEVICE;
	}

	// if cb is set and delay is not 0 and query is
	// already pending return error CANIOT_EAGAIN
	if (cb && delay && is_query_pending(entry)) {
		return -CANIOT_EAGAIN;
	}

	finalize_query_frame(frame, did);

	ret = controller->driv->send(frame, delay);
	// return error on failure
	if (ret < 0) {
		return ret;
	}

	// cb is set and delay is not 0 -> set pending callback and schedule timer 
	// for delay if different from -1
	if (cb && delay) {
		entry->query.callback = cb;
		if (delay != -1) {
			ret = controller->driv->schedule(entry, delay,
							 query_timeout_cb);
			if (ret < 0) {
				return ret;
			}
		}
	}

	return 0;
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
	int ret;

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
	if (is_query_pending(entry)) {
		entry->query.callback(did, frame);
		entry->query.callback = NULL;
	}

	// Update last seen timestamp for entry
	ctrl->driv->get_time(&entry->last_seen, NULL);

	// If frame is a telemetry frame, update telemetry database
	if ((frame->id.type == telemetry) && telemetry_db_enabled(ctrl)) {
		struct caniot_telemetry_entry tentry;
		caniot_build_telemetry_entry(&tentry, frame, entry->last_seen);
		ret = ctrl->telemetry_db->append(&tentry);

		// Return if error
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}