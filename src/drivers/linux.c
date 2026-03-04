/*
 * Copyright (c) 2025 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <caniot/caniot.h>

#if CONFIG_CANIOT_DRIVER_LINUX

#define _GNU_SOURCE
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <caniot/drivers/linux.h>
#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <unistd.h>

#define CNI_MIN(a, b) ((a) < (b) ? (a) : (b))

// --------------------------------------------------------------------------------------
// CANIOT <-> Linux CAN conversions (use current struct can_frame layout)
static void caniot2msg(struct can_frame *msg, const struct caniot_frame *frame)
{
	memset(msg, 0, sizeof(*msg));
	msg->can_id	  = (canid_t)caniot_id_to_canid(frame->id); // helper defines flags/mask
	uint8_t len	  = (uint8_t)CNI_MIN((uint8_t)frame->len, (uint8_t)CAN_MAX_DLEN);
	msg->len	  = len; // union member (preferred)
	msg->len8_dlc = 0;	 // classic CAN only
	memcpy(msg->data, frame->buf, len);
	// __pad and __res0 already zero due to memset()
}

static void msg2caniot(struct caniot_frame *frame, const struct can_frame *msg)
{
	frame->id	= caniot_canid_to_id(msg->can_id);
	uint8_t len = (uint8_t)CNI_MIN((uint8_t)msg->len, (uint8_t)CAN_MAX_DLEN);
	frame->len	= len;
	memcpy(frame->buf, msg->data, len);
}

// --------------------------------------------------------------------------------------
// Socket helper
static int open_bind_can_nonblock(const char *ifname)
{
	int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (s < 0) return -errno;

	int flags = fcntl(s, F_GETFL, 0);
	if (flags == -1 || fcntl(s, F_SETFL, flags | O_NONBLOCK) == -1) {
		int err = errno;
		close(s);
		return -err;
	}

	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
	if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
		int err = errno;
		close(s);
		return -err;
	}

	struct sockaddr_can addr = {
		.can_family	 = AF_CAN,
		.can_ifindex = ifr.ifr_ifindex,
	};
	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		int err = errno;
		close(s);
		return -err;
	}

	// Classic CAN only (no CAN FD frames)
	int canfd_on = 0;
	(void)setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd_on, sizeof(canfd_on));

	return s;
}

// --------------------------------------------------------------------------------------
// Init / Deinit
int caniot_linux_api_ctx_init(struct linux_api_context *ctx,
							  const char *iface,
							  uint32_t flags)
{
	if (!ctx) return -EINVAL;
	memset(ctx, 0, sizeof(*ctx));
	ctx->sock = -1;

	const char *ifn = (iface && *iface) ? iface : "can0";
	int s			= open_bind_can_nonblock(ifn);
	if (s < 0) return s;

	ctx->sock		 = s;
	ctx->flags		 = flags;
	ctx->is_nonblock = 1; // socket is opened as non-blocking

	snprintf(ctx->ifname, sizeof(ctx->ifname), "%s", ifn);
	return 0;
}

void caniot_linux_api_ctx_deinit(struct linux_api_context *ctx)
{
	if (!ctx) return;
	if (ctx->sock >= 0) {
		close(ctx->sock);
		ctx->sock = -1;
	}
	// leave ifname as-is
}

// --------------------------------------------------------------------------------------
// API function implementations (all take void *ctx from the driver table)
static void linux_entropy(void *vctx, uint8_t *buf, size_t len)
{
	(void)vctx;
	if (getrandom(buf, len, 0) < 0) {
		// if something goes wrong, just zero-fill
		memset(buf, 0, len);
	}
}

static void linux_get_time(void *vctx, uint32_t *sec, uint16_t *ms)
{
	(void)vctx;
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	if (sec) *sec = (uint32_t)ts.tv_sec;
	if (ms) *ms = (uint16_t)(ts.tv_nsec / 1000000L);
}

static void linux_set_time(void *vctx, uint32_t sec)
{
	(void)vctx;
	struct timespec ts = {.tv_sec = (time_t)sec, .tv_nsec = 0};
	(void)clock_settime(CLOCK_REALTIME, &ts); // ignore errors (API is void)
}

static int linux_send(void *vctx, const struct caniot_frame *frame, uint32_t delay_ms)
{
	(void)delay_ms;
	if (!vctx || !frame) return -EINVAL;

	struct linux_api_context *ctx = (struct linux_api_context *)vctx;
	if (ctx->sock < 0) return -ENODEV;

	if (ctx->flags & CANIOT_LINUX_DRIVER_FLAG_NOTX) {
		return -CANIOT_ENOTSUP; // driver is not allowed to send
	}

	struct can_frame msg;
	caniot2msg(&msg, frame);

	if (delay_ms > 0) {
		usleep(delay_ms * 1000);
	}

	ssize_t n = write(ctx->sock, &msg, sizeof(msg));
	if (n < 0) {
		return (errno == EAGAIN || errno == EWOULDBLOCK) ? -CANIOT_EAGAIN : -errno;
	}
	if ((size_t)n != sizeof(msg)) return -EIO;
	return 0;
}

// Helper to set socket blocking/non-blocking
static int set_sock_blocking(int sock, int blocking)
{
	int flags = fcntl(sock, F_GETFL, 0);
	if (flags == -1) return -errno;
	if (blocking) {
		if (flags & O_NONBLOCK) {
			if (fcntl(sock, F_SETFL, flags & ~O_NONBLOCK) == -1) {
				return -errno;
			}
		}
	} else {
		if (!(flags & O_NONBLOCK)) {
			if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
				return -errno;
			}
		}
	}
	return 0;
}

static int linux_recv(void *vctx, struct caniot_frame *frame, bool blocking)
{
	if (!vctx || !frame) return -EINVAL;

	struct linux_api_context *ctx = (struct linux_api_context *)vctx;
	if (ctx->sock < 0) return -ENODEV;

	if (ctx->flags & CANIOT_LINUX_DRIVER_FLAG_NORX) {
		return -CANIOT_ENOTSUP; // driver is not allowed to receive
	}

	struct can_frame msg;
	ssize_t n;

	if (blocking && ctx->is_nonblock == 1) {
		if (set_sock_blocking(ctx->sock, 1) < 0) return -errno;
		ctx->is_nonblock = 0;
	} else if (!blocking && ctx->is_nonblock == 0) {
		if (set_sock_blocking(ctx->sock, 0) < 0) return -errno;
		ctx->is_nonblock = 1;
	}
	n = read(ctx->sock, &msg, sizeof(msg));

	if (n < 0) {
		return (errno == EAGAIN || errno == EWOULDBLOCK) ? -CANIOT_EAGAIN : -errno;
	}
	if ((size_t)n != sizeof(msg)) return -EIO;

	msg2caniot(frame, &msg);
	return 0;
}

int linux_fd(void *vctx)
{
	if (!vctx) return -EINVAL;

	struct linux_api_context *ctx = (struct linux_api_context *)vctx;
	return ctx->sock;
}

// --------------------------------------------------------------------------------------
// Driver API table exposure
static const struct caniot_drivers_api g_linux_api = {
	.entropy  = linux_entropy,
	.get_time = linux_get_time,
	.set_time = linux_set_time,
	.send	  = linux_send,
	.recv	  = linux_recv,
	.get_fd	  = linux_fd,
};

// Exported pointer named exactly as requested.
const struct caniot_drivers_api *linux_driver_api_ptr = &g_linux_api;

#endif // CONFIG_CANIOT_DRIVER_LINUX