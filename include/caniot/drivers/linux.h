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

#ifndef CANIOT_DRIVERS_LINUX_H
#define CANIOT_DRIVERS_LINUX_H

#include <stdint.h>

#include <linux/can.h>
#include <net/if.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CANIOT_LINUX_DRIVER_FLAG_NOTX (1 << 0)
#define CANIOT_LINUX_DRIVER_FLAG_NORX (1 << 1)

/**
 * @brief Driver context for Linux SocketCAN.
 *
 * The caller must provide an instance of this struct
 * (on stack or statically allocated). It holds the socket
 * file descriptor and the bound interface name.
 */
struct linux_api_context {
	int sock;			   /**< Non-blocking PF_CAN/RAW socket, -1 if closed */
	char ifname[IFNAMSIZ]; /**< Bound interface name (NUL-terminated) */
	uint32_t flags;		   /**< Driver flags, e.g. CANIOT_LINUX_DRIVER_FLAG_NOTX */
	int is_nonblock; /**< 1 if socket is non-blocking, 0 if blocking, -1 if unknown */
};

/**
 * @brief Initialize a Linux CAN driver context.
 *
 * Opens a non-blocking SocketCAN socket on the given interface
 * and binds it.
 *
 * @param ctx   Pointer to caller-allocated context.
 * @param iface Interface name (e.g. "can0"). If NULL or empty, defaults to "can0".
 * @return 0 on success, negative errno on error.
 */
int caniot_linux_api_ctx_init(struct linux_api_context *ctx,
							  const char *iface,
							  uint32_t flags);

/**
 * @brief Deinitialize a Linux CAN driver context.
 *
 * Closes the socket if open. Safe to call multiple times.
 *
 * @param ctx Pointer to context to deinitialize.
 */
void caniot_linux_api_ctx_deinit(struct linux_api_context *ctx);

/**
 * @brief Pointer to the Linux driver API table.
 *
 * Use this together with a valid @ref linux_context instance:
 *
 * @code
 * linux_driver_api_ptr->send(&ctx, &frame, 0);
 * @endcode
 */
extern const struct caniot_drivers_api *linux_driver_api_ptr;

#ifdef __cplusplus
}
#endif

#endif /* CANIOT_DRIVERS_LINUX_H */
