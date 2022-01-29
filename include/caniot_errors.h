#ifndef _CANIOT_ERRORS_H
#define _CANIOT_ERRORS_H

/* TODO rework all error codes */

/* Tells if error i related to caniot lib */
#define CANIOT_IS_ERR(err)	   ((err == 0) || (err >= 0x3A00 && err <= 0x3AFF))

#define CANIOT_ERROR_BASE	   0x3A00

#define CANIOT_OK          0x0000         /* OK */
#define CANIOT_EINVAL	   0x3A00	  /* Invalid argument */
#define CANIOT_ENPROC      0x3A01         /* ERROR UNPROCESSABLE */
#define CANIOT_ECMD        0x3A02         /* ERROR COMMAND */
#define CANIOT_EKEY        0x3A03         /* ERROR KEY (read/write-attribute) */
#define CANIOT_ETIMEOUT    0x3A04         /* ERROR TIMEOUT */
#define CANIOT_EAGAIN      0x3A05         /* ERROR BUSY / EAGAIN */
#define CANIOT_EFMT        0x3A06         /* ERROR FORMAT */
#define CANIOT_EHANDLERC   0x3A07         /* ERROR UNDEFINED COMMAND HANDLER  */
#define CANIOT_EHANDLERT   0x3A08         /* ERROR UNDEFINED TELEMETRY HANDLER */
#define CANIOT_ETELEMETRY  0x3A09         /* ERROR TELEMETRY */
#define CANIOT_EHDLRCTRL   0x3A0A         /* ERROR UNDEFINED CONTROL COMMAND HANDLER  */

#define CANIOT_EEP	   0x3A17	  /* ERROR ENDPOINT */
#define CANIOT_ECMDEP	   0x3A18	  /* ERROR ILLEGAL COMMAND, BROADCAST TO ALL ENDPOINTS */	

#define CANIOT_ENOINIT     0x3A0A         /* ERROR NOT INITIALIZED */
#define CANIOT_EDRIVER     0x3A0B         /* ERROR DRIVER */
#define CANIOT_EDRIVER     0x3A0B         /* ERROR API */

#define CANIOT_EKEYSECTION 0x3A0C
#define CANIOT_EKEYATTR    0x3A0D
#define CANIOT_EKEYPART    0x3A0E

#define CANIOT_EREADONLY   0x3A0F

#define CANIOT_ENULL       0x3A10
#define CANIOT_ENULLDRV    0x3A11
#define CANIOT_ENULLAPI    0x3A12
#define CANIOT_ENULLID     0x3A13
#define CANIOT_ENULLDEV    0x3A14
#define CANIOT_ENULLCFG    0x3A15

#define CANIOT_EROATTR     0x3A20	  /* ERROR READ-ONLY ATTRIBUTE */
#define CANIOT_EREADATTR   0x3A21         /* ERROR QUERY READ ATTR */
#define CANIOT_EWRITEATTR  0x3A22         /* ERROR QUERY WRITE ATTR */

#define CANIOT_EENOCB      0x3A32         /* ERROR no event handler */
#define CANIOT_EECB        0x3A33         /* ERROR ECCB  */
#define CANIOT_EPQALLOC    0x3A34         /* ERROR PENDING QUERY ALLOCATION  */
#define CANIOT_ENOPQ       0x3A35         /* ERROR NO PENDQING QUERY  */

#define CANIOT_EDEVICE	   0x3A40	  /* ERROR DEVICE */
#define CANIOT_EFRAME	   0x3A41	  /* ERROR FRAME, not a valid caniot frame*/
#define CANIOT_EMLFRM	   0x3A41	  /* ERROR MALFORMED FRAME */

#define CANIOT_ECLASS      0x3A51	  /* ERROR INVALID CLASS */
#define CANIOT_ECFG        0x3A52	  /* ERROR INVALID CONFIGURATION */

#define CANIOT_EBUSY	   CANIOT_EAGAIN

#define CANIOT_ENOTSUP     0x3A60  		/* ERROR NOT SUPPORTED */
#define CANIOT_ENIMPL      CANIOT_ENOTSUP       /* ERROR NOT IMPLEMENTED */



#define CANIOT_ANY         0x3AFF         /* ANY ERROR + -1 */

#endif /* _CANIOT_ERRORS_H */