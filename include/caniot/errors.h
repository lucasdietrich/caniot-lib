/*
 * Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CANIOT_ERRORS_H
#define _CANIOT_ERRORS_H

#include <stdbool.h>
#include <stdint.h>

#define CANIOT_ERROR_BASE	 0x3A00U
#define CANIOT_ERROR_MAX	 0x3AFFU
#define CANIOT_ERROR_DEVICE_MASK 0x0080U

typedef enum {
	CANIOT_OK     = 0x0000,
	CANIOT_EINVAL = CANIOT_ERROR_BASE, /* Invalid argument */
	CANIOT_ENPROC,			   /*  UNPROCESSABLE */
	CANIOT_ECMD,			   /*  COMMAND */
	CANIOT_EKEY,			   /*  KEY (read/write-attribute) */
	CANIOT_ETIMEOUT,		   /*  TIMEOUT */
	CANIOT_EAGAIN,			   /*  BUSY / EAGAIN */
	CANIOT_EFMT,			   /*  FORMAT */
	CANIOT_EHANDLERC,		   /*  UNDEFINED COMMAND HANDLER  */
	CANIOT_EHANDLERT,		   /*  UNDEFINED TELEMETRY HANDLER */
	CANIOT_ETELEMETRY,		   /*  TELEMETRY */

	CANIOT_EEP,    /*  ENDPOINT */
	CANIOT_ECMDEP, /*  ILLEGAL COMMAND, BROADCAST TO ALL ENDPOINTS */

	CANIOT_ENOINIT, /*  NOT INITIALIZED */
	CANIOT_EDRIVER, /*  DRIVER */
	CANIOT_EAPI,	/*  API */

	CANIOT_EKEYSECTION, /* Unknown attributes section */
	CANIOT_EKEYATTR,    /* Unknown attribute */
	CANIOT_EKEYPART,    /* Unknown attribute part */
	CANIOT_ENOATTR,	    /* No attribute */

	CANIOT_EREADONLY,

	CANIOT_ENULL,
	CANIOT_ENULLDRV,
	CANIOT_ENULLAPI,
	CANIOT_ENULLID,
	CANIOT_ENULLDEV,
	CANIOT_ENULLCFG,
	CANIOT_ENULLCTRL,
	CANIOT_ENULLCTRLCB,

	CANIOT_EROATTR,	   /*  READ-ONLY ATTRIBUTE */
	CANIOT_EREADATTR,  /*  QUERY READ ATTR */
	CANIOT_EWRITEATTR, /*  QUERY WRITE ATTR */

	CANIOT_EENOCB,	  /*  no event handler */
	CANIOT_EECB,	  /*  ECCB  */
	CANIOT_EPQALLOC,  /*  PENDING QUERY ALLOCATION  */
	CANIOT_ENOPQ,	  /*  NO PENDQING QUERY  */
	CANIOT_ENOHANDLE, /*  NO HANDLER */

	CANIOT_EDEVICE, /*  DEVICE */
	CANIOT_EFRAME,	/*  FRAME, not a valid caniot frame*/
	CANIOT_EMLFRM,	/*  MALFORMED FRAME */

	CANIOT_ECLASS, /*  INVALID CLASS */
	CANIOT_ECFG,   /*  INVALID CONFIGURATION */

	CANIOT_EHYST, /* Invalid hysteresis structure */

	CANIOT_ENOTSUP, /*  NOT SUPPORTED */
	CANIOT_ENIMPL,	/*  NOT IMPLEMENTED */
} caniot_error_t;

/* STATIC_ASSERT(CANIOT_ENIMPL < 0x80) */

#define CANIOT_EBUSY CANIOT_EAGAIN

/**
 * @brief Get caniot error from original error code
 *
 * Fowarded tells if the error is actually a device error which was forwarded by the
 * controller
 *
 * @param err
 * @param forwarded
 * @return caniot_error_t
 */
caniot_error_t caniot_interpret_error(int err, bool *forwarded);

static inline int caniot_forward_device_error(int cterr)
{
	return -((-cterr) | CANIOT_ERROR_DEVICE_MASK);
}

#endif /* _CANIOT_ERRORS_H */