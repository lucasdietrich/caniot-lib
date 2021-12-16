#include "std.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "helper.h"

#if __AVR__
#error "This example is not for AVR"
#endif

void z_entropy(uint8_t *buf, size_t len)
{
	for (uint8_t *p = buf; p < buf + len; p++)
	{
		*p = rand();
	}
}

void z_get_time(uint32_t *sec, uint32_t *usec)
{
	struct timespec spec;

	clock_gettime(CLOCK_REALTIME, &spec);

	*sec = spec.tv_sec;
	*usec = spec.tv_nsec / 1000;
}

bool z_pending_telemetry(void)
{
	return false;
}

int z_send(struct caniot_frame *frame, uint32_t delay)
{
	printf(F("TX delay=%d"), delay);
	caniot_show_frame(frame);

	return 0;
}

int z_recv(struct caniot_frame *frame)
{
	static int i = 0;
	i++;

	uint32_t v = 0xAAA;

	memcpy(&frame->id, &v, sizeof(frame->id));
	frame->len = 8;
	memcpy(frame->buf, "12345678", 8);

	caniot_show_frame(frame);

	return 0;
}
int z_set_filter(struct caniot_filter *filter)
{
	return 0;
}

int z_set_mask(struct caniot_filter *filter)
{
	return 0;
}

void z_rom_read(void *p, void *d, uint8_t size)
{
	memcpy(d, p, size);
}

void z_persistent_read(void *p, void *d, uint8_t size)
{
	memcpy(d, p, size);
}

void z_persistent_write(void *p, void *s, uint8_t size)
{
	memcpy(p, s, size);
}