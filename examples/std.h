#ifndef _STD_H_
#define _STD_H_

#include <stdint.h>
#include <stddef.h>

#include <caniot.h>

void z_entropy(uint8_t *buf, size_t len);

void z_get_time(uint32_t *sec, uint32_t *usec);

int z_send(struct caniot_frame *frame, uint32_t delay);
int z_recv(struct caniot_frame *frame);
int z_set_filter(struct caniot_filter *filter);
int z_set_mask(struct caniot_filter *filter);

void z_rom_read(void *p, void *d, uint8_t size);
void z_persistent_read(void *p, void *d, uint8_t size);
void z_persistent_write(void *p, void *s, uint8_t size);

bool z_pending_telemetry(void);

#endif