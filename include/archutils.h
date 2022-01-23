#ifndef _CANIOT_ARCHUTILS_H_
#define _CANIOT_ARCHUTILS_H_

/**
 * @brief Helper for printing strings :
 * 
 * printf(F("Hello %d\n"), 42);
 * 
 * Equivalent to :
 * - AVR : printf_P(PSTR("Hello %d\n"), 42);
 * - ARM/x86/any : printf("Hello %d\n", 42);
 */
#include <stdio.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#define printf	printf_P
#define FLASH_STRING(x) PSTR(x)
#define memcpy_P memcpy_P
#define ROM	PROGMEM
#else
#define printf  printf
#define FLASH_STRING(x) (x)
#define memcpy_P memcpy
#define ROM
#endif
#define F(x) FLASH_STRING(x)


/* 0 : NO DEBUG
 * 1 : ERR
 * 2 : WRN
 * 3 : INF
 * 4 : DBG
 */
#ifdef CONFIG_CANIOT_LOG_LEVEL
#define CANIOT_LOG_LEVEL CONFIG_CANIOT_LOG_LEVEL
#else
#define CANIOT_LOG_LEVEL 4
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



#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define CONTAINER_OF(ptr, type, field) ((type *)(((char *)(ptr)) - offsetof(type, field)))

#endif /* _CANIOT_ARCHUTILS_H_ */