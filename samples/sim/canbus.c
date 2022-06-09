#include <caniot/caniot.h>

#include <malloc.h>
#include <memory.h>

struct item;

struct item {
	struct item *next;
	struct caniot_frame frame;
};

struct {
	struct item *head;
	struct item *tail;
} queue = {
	.head = NULL,
	.tail = NULL,
};

int can_send(const struct caniot_frame *frame, uint32_t delay_ms)
{
	int ret = -CANIOT_EINVAL;

	if (frame != NULL) {
		struct item *item = malloc(sizeof(struct item));

		// copy frame
		memcpy(&item->frame, frame, sizeof(struct caniot_frame));

		// queue can message
		if (queue.tail != NULL) {
			queue.tail->next = item;
		} else {
			queue.head = item;
		}
		queue.tail = item;
		item->next = NULL;

		ret = 0U;
	}

	return ret;
}

int can_recv(struct caniot_frame *frame)
{
	int ret = -CANIOT_EINVAL;

	if (frame != NULL) {
		// dequeue if not null
		if (queue.head != NULL) {
			struct item *item = queue.head;
			queue.head = queue.head->next;

			if (queue.head == NULL) {
				queue.tail = NULL;
			}

			memcpy(frame, &item->frame, sizeof(struct caniot_frame));
			
			ret = 0;
		} else {
			ret = -CANIOT_EAGAIN;
		}
	}

	return ret;
}