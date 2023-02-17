/**
 * @file canbus.c
 * @author Dietrich Lucas (ld.adecy@gmail.com)
 * @brief Canbus emulation
 * @version 0.1
 * @date 2022-06-09
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <caniot/caniot.h>
#include <malloc.h>
#include <memory.h>

struct item;

struct item {
	struct item *next;
	struct caniot_frame frame;
};

/**
 * @brief doubly linked queue
 * Note: Keeping track of the head and the tail allows
 * to queue and dequeue with O(1) complexity
 */
struct {
	struct item *head;
	struct item *tail;
} queue = {
	.head = NULL,
	.tail = NULL,
};

/**
 * @brief Send a can message on the emulated CAN bus
 *
 * Frame pointed by the pointer can be deallocated after the function terminates.
 *
 * @param frame
 * @param delay_ms ignored, frame is always without delay
 * @return int 0 on success
 */
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

	// printf("\tcan_send(%p, %u) = -0x%X\n", &frame, delay_ms, -ret);

	return ret;
}

/**
 * @brief Receive a CAN message from the emulated CAN bus
 *
 * @param frame Should point to a valid memory space
 * @return int 0 on success
 */
int can_recv(struct caniot_frame *frame)
{
	int ret = -CANIOT_EINVAL;

	if (frame != NULL) {
		// dequeue if not null
		if (queue.head != NULL) {
			struct item *item = queue.head;
			queue.head	  = queue.head->next;

			if (queue.head == NULL) {
				queue.tail = NULL;
			}

			memcpy(frame, &item->frame, sizeof(struct caniot_frame));

			ret = 0;
		} else {
			ret = -CANIOT_EAGAIN;
		}
	}

	// printf("\tcan_recv(%p, 0) = -0x%X\n", &frame, -ret);

	return ret;
}