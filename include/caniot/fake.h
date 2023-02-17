/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CANIOT_FAKE_H_
#define _CANIOT_FAKE_H_

#include <caniot/device.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t caniot_fake_get_temp(struct caniot_device *dev);

#ifdef __cplusplus
}
#endif

#endif /* _CANIOT_FAKE_H_ */