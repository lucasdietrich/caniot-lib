#ifndef _CANIOT_ERRORS_H
#define _CANIOT_ERRORS_H

/* Tells if error i related to caniot lib */
#define CANIOT_IS_ERR(err)	   ((err == 0) || (err >= 0xCA00 && err <= 0xCAFF))

#define CANIOT_OK          0x0000         /* OK */
#define CANIOT_EINVAL	   0xCA00	  /* Invalid argument */
#define CANIOT_ENPROC      0xCA01         /* ERROR UNPROCESSABLE */
#define CANIOT_ECMD        0xCA02         /* ERROR COMMAND */
#define CANIOT_EKEY        0xCA03         /* ERROR KEY (read/write-attribute) */
#define CANIOT_ETIMEOUT    0xCA04         /* ERROR TIMEOUT */
#define CANIOT_EAGAIN      0xCA05         /* ERROR BUSY / EAGAIN */
#define CANIOT_EFMT        0xCA06         /* ERROR FORMAT */
#define CANIOT_EHANDLERC   0xCA07         /* ERROR UNDEFINED COMMAND HANDLER  */
#define CANIOT_EHANDLERT   0xCA08         /* ERROR UNDEFINED TELEMETRY HANDLER */
#define CANIOT_ETELEMETRY  0xCA09         /* ERROR TELEMETRY */

#define CANIOT_EEP	   0xCA17	  /* ERROR ENDPOINT */
#define CANIOT_ECMDEP	   0xCA18	  /* ERROR ILLEGAL COMMAND, BROADCAST TO ALL ENDPOINTS */	

#define CANIOT_ENOINIT     0xCA0A         /* ERROR NOT INITIALIZED */
#define CANIOT_EDRIVER     0xCA0B         /* ERROR DRIVER */
#define CANIOT_EDRIVER     0xCA0B         /* ERROR API */

#define CANIOT_EKEYSECTION 0xCA0C
#define CANIOT_EKEYATTR    0xCA0D
#define CANIOT_EKEYPART    0xCA0E

#define CANIOT_EREADONLY   0xCA0F

#define CANIOT_ENULL       0xCA10
#define CANIOT_ENULLDRV    0xCA11
#define CANIOT_ENULLAPI    0xCA12
#define CANIOT_ENULLID     0xCA13
#define CANIOT_ENULLDEV    0xCA14
#define CANIOT_ENULLCFG    0xCA15

#define CANIOT_EREADATTR   0xCA21         /* ERROR QUERY READ ATTR */
#define CANIOT_EWRITEATTR  0xCA22         /* ERROR QUERY WRITE ATTR */

#define CANIOT_EENOCB      0xCA32         /* ERROR no event handler */
#define CANIOT_EECB        0xCA33         /* ERROR ECCB  */

#define CANIOT_EDEVICE	   0xCA40	  /* ERROR DEVICE */

#define CANIOT_EBUSY	   CANIOT_EAGAIN

#define CANIOT_ENIMPL      0xCA60         /* ERROR NOT IMPLEMENTED */


#define CANIOT_ANY         0xCAFF         /* ANY ERROR + -1 */

#endif /* _CANIOT_ERRORS_H */