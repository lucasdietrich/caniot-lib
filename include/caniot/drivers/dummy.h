/**
 * @file linux_driver.h
 * @brief CANIOT Linux backend driver using SocketCAN.
 *
 * This backend implements the caniot_drivers_api interface
 * for Linux using the standard SocketCAN API (PF_CAN / SOCK_RAW).
 *
 * Typical usage:
 * @code
 * #include "linux_driver.h"
 *
 * struct linux_context ctx;
 * if (caniot_linux_api_ctx_init(&ctx, "can0") == 0) {
 *     // use API with ctx
 *     struct caniot_frame tx = { .id = 0x123, .len = 2, .buf = {0xAA, 0x55} };
 *     linux_driver_api_ptr->send(&ctx, &tx, 0);
 *
 *     struct caniot_frame rx;
 *     if (linux_driver_api_ptr->recv(&ctx, &rx, true) == 0) {
 *         // handle rx
 *     }
 *
 *     caniot_linux_api_ctx_deinit(&ctx);
 * }
 * @endcode
 */
#ifndef CANIOT_DRIVERS_DUMMY_H
#define CANIOT_DRIVERS_DUMMY_H

#include <linux/can.h>
#include <net/if.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pointer to the Dummy driver API table.
 *
 * Use this together with a valid @ref linux_context instance:
 *
 * @code
 * dummy_driver_api_ptr->send(&ctx, &frame, 0);
 * @endcode
 */
extern const struct caniot_drivers_api *dummy_driver_api_ptr;

#ifdef __cplusplus
}
#endif

#endif // CANIOT_DRIVERS_DUMMY_H