#include "controller.h"

struct caniot_telemetry_entry {
	union deviceid did;
	uint32_t timestamp;
	uint8_t buf[4][8];
};

struct caniot_device_entry
{
	uint32_t last_seen;	/* timestamp this device was last seen */

	caniot_query_callback_t callback; /* pending query if not NULL */

	struct {
		uint8_t active : 1;	/* valid entry if set */
	} flags;
};

struct caniot_telemetry_database_api
{
	int (*init)(void);
	int (*deinit)(void);

	int (*append)(struct caniot_telemetry_entry *entry);

	int (*get_first)(union deviceid did);
	int (*get_next)(union deviceid did,
			struct caniot_telemetry_entry *entry);
	int (*get_last)(union deviceid did);
	int (*count)(union deviceid did);
};

struct caniot_controller {
	char name[32];
	uint32_t uid;

	struct caniot_device_entry devices[CANIOT_MAX_DEVICES];
	struct caniot_telemetry_database_api *telemetry_db;
	struct caniot_drivers_api *driv;
};

static void clean_frame(struct caniot_frame *frame)
{
	memset(frame, 0x00, sizeof(struct caniot_frame));
}


// Finalize frame with device id
static void finalize_query_frame(struct caniot_frame *frame, union deviceid did)
{
	frame->id.query = query;
	
	frame->id.cls = did.cls;
	frame->id.dev = did.dev;
}

static void prepare_query_frame(struct caniot_frame *frame, union deviceid did)
{
	clean_frame(frame);

	finalize_query_frame(frame, did);
}

// Return device entry corresponding to device id or last entry if broadcast
static struct caniot_device_entry *get_device_entry(struct caniot_controller *ctrl,
						    union deviceid did)
{
	if (caniot_is_broadcast(did)) {
		return ctrl->devices + did.val;
	} else if (caniot_is_broadcast(did)) {
		return &ctrl->devices[CANIOT_MAX_DEVICES - 1];
	}

	return NULL;
}

// Tells if a query is pending for device entry
static int is_query_pending(struct caniot_device_entry *entry)
{
	return entry->callback != NULL;
}

static int caniot_controller_init(struct caniot_controller *ctrl)
{
	int i;

	for (i = 0; i < CANIOT_MAX_DEVICES; i++) {
		ctrl->devices[i].flags.active = 0;
	}

	return ctrl->telemetry_db->init();
}

static int caniot_controller_deinit(struct caniot_controller *ctrl)
{
	return ctrl->telemetry_db->deinit();
}

// Initialize controller structure
int caniot_controller_init(struct caniot_controller *controller)
{
	// initialize devices array
	memset(controller->devices, 0, sizeof(controller->devices));

	return 0;
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

	entry = get_device(controller, did);
	if (!entry) {
		return -CANIOT_EDEVICE;
	}

	// if cb is set and delay is not 0 and query is
	// already pending return error CANIOT_EAGAIN
	if (cb && delay && is_query_pending(entry)) {
		return -CANIOT_EAGAIN;
	}

	// cb is set and delay is not 0 -> set pending callback and send the frame
	if (cb && delay) {
		entry->callback = cb;
	}

	finalize_query_frame(&frame, did);

	ret = controller->driv->send(&frame, delay);

	// delete pending callback on failure and return error
	if (ret < 0) {
		entry->callback = NULL;
		return ret;
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
			  struct caniot_attribute *attr,
			  caniot_query_callback_t cb,
			  int32_t timeout)
{

}

int caniot_write_attribute(struct caniot_controller *ctrl,
			   union deviceid did,
			   struct caniot_attribute *attr,
			   caniot_query_callback_t cb,
			   int32_t timeout)
{

}

int caniot_discover(struct caniot_controller *ctrl,
		    caniot_query_callback_t cb,
		    int32_t timeout)
{

}