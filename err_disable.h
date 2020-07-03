#ifndef ERR_DISABLE_H
#define ERR_DISABLE_H

#include "../bcmutils/sk_define.h"

#define ERRDISABLE_DEBUG
#define ERR_DISABLE_SEM_PATH "/tmp/err_disable_sem"
#define ERR_DISABLE_SHM_PATH "/tmp/err_disable_shm"
#define  EVENT_NUMBER  9
#define PORT_MAX PNUM

#define EDR_DISABLE 1
#define EDR_NORMAL 0

#define ERR_PRO_TRRIGER 1
#define ERR_PRO_UNTRRIGER 0

#define ERR_CFG_ENABLE '1'
#define ERR_CFG_DISABLE '0'

#define STATE_DISCARD 0x20
#define STATE_BLOCK 0x40
#define STATE_FORWARD 0xA0


typedef enum
{
ERR_SRC_NULL,
ERR_SRC_AGGREGATION,
ERR_SRC_ARP,
ERR_SRC_BPDUGUARD,
ERR_SRC_LOOPBACK,
ERR_SRC_SECURITY,
ERR_SRC_SFP,
ERR_SRC_UDLD,
ERR_SRC_ROOTGUARD,
}EDR_ERR_SRC;
typedef struct
{
    INT8     cPortState;/*当前端口状态: errdisable/normal*/
    INT8     cErrSrc[EVENT_NUMBER];
    INT8     cPriRecSrc[EVENT_NUMBER];
    INT8     cRecoverSrc[EVENT_NUMBER];
    INT32    lRecoveryTimer; /* 自动恢复事件：0-86400 seconds*/
}ERR_DISABLE_PORT;

typedef struct
{
	INT8 cPortState;/* port error-disable state*/
	EDR_ERR_SRC enErrSrc;/* error source */
	//EDR_ERR_SRC enPriRecSrc;/* need to recover error source */
	EDR_ERR_SRC enRecSrc;/* error disable recover for which error */
	INT32 lRecTimer;/* recover time */
}EDR_PORT;
typedef struct
{
	INT32 iPort;
	EDR_ERR_SRC enErrSrc;
}SK_EDR;

#endif
