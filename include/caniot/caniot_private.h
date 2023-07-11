/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CANIOT_PRIVATE_H_
#define _CANIOT_PRIVATE_H_

#include <caniot/caniot.h>

/**
 * @brief Helper for printing strings :
 *
 * printf(F("Hello %d\n"), 42);
 *
 * Equivalent to :
 * - AVR : printf_P(PSTR("Hello %d\n"), 42);
 * - ARM/x86/any : printf("Hello %d\n", 42);
 */

#if defined(__AVR__)
#include <stdio.h>

#include <avr/pgmspace.h>
#define printf	 printf_P
#define snprintf snprintf_P
#define F(x)	 PSTR(x)
#define memcpy_P memcpy_P
#define ROM		 PROGMEM
#define Z_ASSERT(x)

#if CONFIG_CANIOT_ATTRIBUTE_NAME
#error "Attribute name is not supported on AVR architectures"
#endif

#elif defined(__ZEPHYR__)

#include <stdio.h>

#include <zephyr/kernel.h>
// #	define snprintf snprintf
#define strlen_P  strlen
#define strncpy_P strncpy
#define F(x)	  (x)
#define memcpy_P  memcpy
#define ROM
#define Z_ASSERT(x) __ASSERT(x, STRINGIFY(xSTRINGIFY()))

#else /* stdlib */

#include <stdio.h>
#define snprintf  snprintf
#define strlen_P  strlen
#define strncpy_P strncpy
#define F(x)	  x
#define memcpy_P  memcpy
#define ROM

#include <stdbool.h>
extern void __assert(bool statement);
#define Z_ASSERT(x) __assert(x)
#endif

#if CONFIG_CANIOT_ASSERT
#define ASSERT(x) Z_ASSERT(x)
#else
#define ASSERT(x)
#endif

/* 0 : NO DEBUG
 * 1 : ERR
 * 2 : WRN
 * 3 : INF
 * 4 : DBG
 */
#if defined(CONFIG_CANIOT_LOG_LEVEL)
#define CANIOT_LOG_LEVEL CONFIG_CANIOT_LOG_LEVEL
#else
#define CANIOT_LOG_LEVEL 1
#endif

#if CANIOT_LOG_LEVEL >= 4
#define CANIOT_DBG(...) printf(__VA_ARGS__)
#else
#define CANIOT_DBG(...)
#endif /* CANIOT_LOG_LEVEL >= 4 */

#if CANIOT_LOG_LEVEL >= 3
#define CANIOT_INF(...) printf(__VA_ARGS__)
#else
#define CANIOT_INF(...)
#endif /* CANIOT_LOG_LEVEL >= 3 */

#if CANIOT_LOG_LEVEL >= 2
#define CANIOT_WRN(...) printf(__VA_ARGS__)
#else
#define CANIOT_WRN(...)
#endif /* CANIOT_LOG_LEVEL >= 2 */

#if CANIOT_LOG_LEVEL >= 1
#define CANIOT_ERR(...) printf(__VA_ARGS__)
#else
#define CANIOT_ERR(...)
#endif /* CANIOT_LOG_LEVEL >= 1 */

#if !defined(__ZEPHYR__)
#define MIN(a, b)					   ((a) < (b) ? (a) : (b))
#define MAX(a, b)					   ((a) > (b) ? (a) : (b))
#define ARRAY_SIZE(a)				   (sizeof(a) / sizeof((a)[0]))
#define CONTAINER_OF(ptr, type, field) ((type *)(((char *)(ptr)) - offsetof(type, field)))
#endif

#define INDEX_OF(obj, base, _struct) ((_struct *)(obj) - (_struct *)(base))

#endif /* _CANIOT_PRIVATE_H_ */