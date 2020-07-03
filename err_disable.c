#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>

#include "../bcmutils/bcmutils.h"
#include "../cmdline/cmdlib.h"
#include "err_disable.h"
#include "../bcmutils/sk_define.h"
//#include "sk_define.h"

INT32 timerPipe[2];
INT8  acEdrDetect[EVENT_NUMBER+1];/* the config of error disable detection weather enable the event to cause err-disable*/
INT8  acEdrRecover[EVENT_NUMBER+1];/* the config of error disable recovery weather enable recover the event error*/
//INT8  acLastEdrRec[EVENT_NUMBER+1];
//INT8  acLastEdrDet[EVENT_NUMBER+1];
INT8  acLastEdrRec[PORT_MAX];
INT8  acEdrRec[PORT_MAX];
INT32 lTimerInterval;//err-disable recover time,30-86400 seconds
EDR_PORT port_edr_state[PORT_MAX];

INT64 edr_link;
INT64 edr_port_map = 0x00ULL;
struct sockaddr_un astEdrScokAddr[EVENT_NUMBER+1];
INT32 iEdrIpcSkfd;
#if 1
INT8 init_err_disable()
{
	int iPort;
	int skfd;

	if((skfd = open(DEVICE_FILE_NAME, 0)) < 0) 
	{
		return;
	}
    char *err_disable_cfg = cli_nvram_safe_get(CLI_ERR_DISABLE, (char*)&"err_disable_cfg");
    char *err_recover_cfg = cli_nvram_safe_get(CLI_ERR_RECOVER, (INT8*)&"err_recover_cfg");
    char *err_recover_time = nvram_safe_get((INT8*)&"err_recover_time");

    memcpy(acEdrDetect, err_disable_cfg, strlen(err_disable_cfg)+1);
    memcpy(acEdrRecover, err_recover_cfg, strlen(err_recover_cfg)+1);
    //memcpy(acLastEdrDet, err_disable_cfg, strlen(err_disable_cfg)+1);
    memcpy(acLastEdrRec, err_recover_cfg, strlen(err_recover_cfg)+1);
    lTimerInterval = atoi(err_recover_time);

    memset(port_edr_state, 0, sizeof(port_edr_state));

	init_sock_addr(&astEdrScokAddr[ERR_SRC_AGGREGATION], SOCK_PATH_LACP, 0);
	init_sock_addr(&astEdrScokAddr[ERR_SRC_BPDUGUARD], SOCK_PATH_RSTP, 0);
	init_sock_addr(&astEdrScokAddr[ERR_SRC_LOOPBACK], SOCK_PATH_LOOPBACK, 0);
	//printf("err_disable.c:init_err_disable:52--EdrDetect=%s, EdrRecover=%s\n", acEdrDetect, acEdrRecover);/*renshanming: for debug*/
	#if 1
	for (iPort = 1; iPort <= PNUM; iPort++) {
		edr_port_map |= (0x01ULL << phy[iPort]);
	}
	set_errdisable_cfp(skfd, edr_port_map);
	#endif
    free(err_disable_cfg);
    free(err_recover_cfg);
    free(err_recover_time);
	close(skfd);
    return 1;
}
void syslog_err_disable(INT32 iPort, INT32 iErrSrc)
{
	switch(iErrSrc)
	{
		case ERR_SRC_AGGREGATION:
			syslog(LOG_WARNING, "[ERR_DISABLE-4-DISABLE]:aggregation error detected on %s0/%d, putting %s0/%d in err-disable state\n",
				   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM),
				   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM));
			break;
		case ERR_SRC_ARP:
			syslog(LOG_WARNING, "[ERR_DISABLE-4-DISABLE]:arp-inspection error detected on %s0/%d, putting %s0/%d in err-disable state\n",
				   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM),
				   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM));
			break;
		case ERR_SRC_BPDUGUARD:
			syslog(LOG_WARNING, "[ERR_DISABLE-4-DISABLE]:bpdu-guard error detected on %s0/%d, putting %s0/%d in err-disable state\n",
				   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM),
				   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM));
			break;
		case ERR_SRC_LOOPBACK:
			syslog(LOG_WARNING, "[ERR_DISABLE-4-DISABLE]:loopback error detected on %s0/%d, putting %s0/%d in err-disable state\n",
				   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM),
				   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM));
			break;
		case ERR_SRC_SECURITY:
			syslog(LOG_WARNING, "[ERR_DISABLE-4-DISABLE]:802.1x-guard error detected on %s0/%d, putting %s0/%d in err-disable state\n",
				   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM),
				   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM));
			break;
		case ERR_SRC_SFP:
			syslog(LOG_WARNING, "[ERR_DISABLE-4-DISABLE]:sfp-config-mismatch detected on %s0/%d, putting %s0/%d in err-disable state\n",
				   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM),
				   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM));
			break;
		case ERR_SRC_UDLD:
			syslog(LOG_WARNING, "[ERR_DISABLE-4-DISABLE]:udld error detected on %s0/%d, putting %s0/%d in err-disable state\n",
				   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM),
				   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM));
			break;
		case ERR_SRC_ROOTGUARD:
			syslog(LOG_WARNING, "[ERR_DISABLE-4-DISABLE]:root-guard error detected on %s0/%d, putting %s0/%d in err-disable state\n",
				   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM),
				   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM));
			break;
		default:
			 break;
	}
}

void syslog_err_recover(INT32 iPort, INT32 iErrSrc)
{
	switch(iErrSrc)
    {
        case ERR_SRC_AGGREGATION:
            syslog(LOG_NOTICE, "[ERR_DISABLE-5-RECOVER]: Err-disable recover from aggregation error on port %s0/%d\n",
                   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM));
            break;
        case ERR_SRC_ARP:
            syslog(LOG_NOTICE, "[ERR_DISABLE-5-RECOVER]: Err-disable recover from arp-inspection error on port %s0/%d\n",
                   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM));
            break;
        case ERR_SRC_BPDUGUARD:
            syslog(LOG_NOTICE, "[ERR_DISABLE-5-RECOVER]: Err-disable recover from bpdu-guard error on port %s0/%d\n",
                   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM));
            break;
        case ERR_SRC_LOOPBACK:
            syslog(LOG_NOTICE, "[ERR_DISABLE-5-RECOVER]: Err-disable recover from loopback error on port %s0/%d\n",
                   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM));
            break;
        case ERR_SRC_SECURITY:
            syslog(LOG_NOTICE, "[ERR_DISABLE-5-RECOVER]: Err-disable recover from 802.1x-guard error on port %s0/%d\n",
                   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM));
            break;
        case ERR_SRC_SFP:
            syslog(LOG_NOTICE, "[ERR_DISABLE-5-RECOVER]: Err-disable recover from sfp-config-mismatch error on port %s0/%d\n",
                   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM));
            break;
        case ERR_SRC_UDLD:
            syslog(LOG_NOTICE, "[ERR_DISABLE-5-RECOVER]: Err-disable recover from udld error on port %s0/%d\n",
                   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM));
            break;
        case ERR_SRC_ROOTGUARD:
            syslog(LOG_NOTICE, "[ERR_DISABLE-5-RECOVER]: Err-disable recover from root-guard error on port %s0/%d\n",
                   (iPort<=FNUM)?"F":"G", (iPort<=FNUM)?iPort:(iPort-FNUM));
            break;
        default:
             break;
    }
}
INT32 edr_ipc_send(INT32 skfd, struct sockaddr_un *des_sock_addr, INT32 iPort, EDR_ERR_SRC enErrSrc)
{
	IPC_SK tx,rx;
	SK_EDR *pTx=(SK_EDR *)tx.acData;

	tx.stHead.enCmd = IPC_CMD_EDR_RECOVER;
	tx.stHead.cOpt = 0;
	tx.stHead.cBack = IPC_SK_NOBACK;
	pTx->iPort = iPort;
	pTx->enErrSrc = enErrSrc;

	//printf("%s.%s.%d--enCmd= %d, port=%d, ErrSrc=%d\n", __FILE__, __FUNCTION__, __LINE__,tx.stHead.enCmd, pTx->iPort, pTx->enErrSrc);
	/*send data to server*/
	if(ipc_send(skfd, &tx, des_sock_addr) == -1)
	{
		return -1;
	}
	//printf("%s.%s.%d\n", __FILE__, __FUNCTION__, __LINE__);
	//if(ipc_recv(skfd, &rx, des_sock_addr) == -1)
	//{
	//	return -1;
	//}
	//printf("%s.%s.%d\n", __FILE__, __FUNCTION__, __LINE__);
	//SK_EDR *pRx=(SK_EDR *)rx.acData;
	//printf("cmd=%d, opt=%d, back=%d, port=%d, errSrc=%d\n", rx.stHead.enCmd, rx.stHead.cOpt,rx.stHead.cBack, pRx->iPort, pRx->enErrSrc);
	return 0;
}
void port_recover(INT32 skfd, INT32 iPortNo , INT32 iErrSrc, INT32 iFlgLog)
{
    int iPort;
    iPort = iPortNo;
	
    char *rstp_enable = cli_nvram_safe_get(CLI_ERR_DISABLE, (char*)&"rstp_enable");

    if((*rstp_enable) == ERR_CFG_ENABLE)
    {
        stp_set_port_state(iPortNo, STATE_BLOCK);
    }
    else
    {
        stp_set_port_state(iPortNo, STATE_FORWARD);
    }
	free(rstp_enable);
	#if 0
    if((*rstp_enable) == ERR_CFG_ENABLE)
    {
		edr_port_map &= ~(0x01ULL << phy[iPort]);
    }
    else
    {
	    edr_port_map |= (0x01ULL << phy[iPort]);
    }
	
	#endif
	//printf("%s.%s.%d--port=%d\n", __FILE__, __FUNCTION__, __LINE__, iPort);
	//stp_set_port_state(iPortNo, STATE_FORWARD);
	//printf("recover before:edr_port_map=%08x %08x\n", (INT32)(edr_port_map>>32), (INT32)edr_port_map);
	edr_port_map &= ~(0x01ULL << phy[iPort]);
	set_errdisable_cfp(skfd, edr_port_map);
	//clear_mac_by_port(skfd, iPort);
	//printf("recover after:edr_port_map=%08x %08x\n", (INT32)(edr_port_map>>32), (INT32)edr_port_map);
	if(iFlgLog)
	{
		syslog_err_recover(iPort, iErrSrc);
	}
    
    
}
void edr_recover_port(INT32 iPort, INT32 iFlgLog)
{
	int iLoop;
	int iSkfd;

	if((iSkfd = open(DEVICE_FILE_NAME, 0)) < 0) 
		return;
	port_recover(iSkfd, iPort+1, port_edr_state[iPort].enErrSrc, iFlgLog);
	port_edr_state[iPort].cPortState = EDR_NORMAL;
	port_edr_state[iPort].enRecSrc = ERR_SRC_NULL;
	port_edr_state[iPort].lRecTimer = 0;
	acEdrRec[iPort] = ERR_PRO_UNTRRIGER;
	acLastEdrRec[iPort] = acEdrRec[iPort];
	for(iLoop = 0; iLoop < EVENT_NUMBER+1; iLoop++)
	{
		if(access(astEdrScokAddr[iLoop].sun_path, F_OK) == 0)
		{
			//printf("%s:%s:%d--sock_path=%s\n", __FILE__, __FUNCTION__, __LINE__, astEdrScokAddr[iLoop].sun_path);
			edr_ipc_send(iEdrIpcSkfd, &astEdrScokAddr[iLoop], iPort+1, port_edr_state[iPort].enErrSrc);
		}
	}
	port_edr_state[iPort].enErrSrc = ERR_SRC_NULL;
	close(iSkfd);
}
void edr_change_cfg()
{
	int iLoop;
	int iErrSrc;
    char *err_disable_cfg = cli_nvram_safe_get(CLI_ERR_DISABLE, (char*)&"err_disable_cfg");
    char *err_recover_cfg = cli_nvram_safe_get(CLI_ERR_RECOVER, (INT8*)&"err_recover_cfg");
    char *err_recover_time = nvram_safe_get((INT8*)&"err_recover_time");
	//char *port_enable = cli_nvram_safe_get(CLI_ALL_ONE, "port_enable");
    memcpy(acEdrDetect, err_disable_cfg, strlen(err_disable_cfg)+1);
    memcpy(acEdrRecover, err_recover_cfg, strlen(err_recover_cfg)+1);
	lTimerInterval = atoi(err_recover_time);

	for(iLoop = 0; iLoop < PNUM; iLoop++)
	{
		if(port_edr_state[iLoop].cPortState == EDR_DISABLE)
		{
			if(port_edr_state[iLoop].lRecTimer > lTimerInterval)
			{
				port_edr_state[iLoop].lRecTimer = lTimerInterval;
			}
			/*
			if(*(port_enable + iLoop) == ERR_CFG_DISABLE)
			{
				port_edr_state[iLoop].lRecTimer = 0;
				edr_recover_port(iLoop, 0);
			} 
			*/ 
			if(port_edr_state[iLoop].enRecSrc != ERR_SRC_NULL)
			{
				if(acEdrRecover[port_edr_state[iLoop].enRecSrc] == ERR_CFG_DISABLE) 
				{
					port_edr_state[iLoop].lRecTimer = 0;
					port_edr_state[iLoop].enRecSrc = ERR_SRC_NULL;
					acLastEdrRec[iLoop] = acEdrRec[iLoop] = ERR_PRO_UNTRRIGER;
				}
			}
			else
			{
				if(acEdrRecover[port_edr_state[iLoop].enErrSrc] == ERR_CFG_ENABLE)
				{
					port_edr_state[iLoop].lRecTimer = lTimerInterval;
					port_edr_state[iLoop].enRecSrc = port_edr_state[iLoop].enErrSrc;
					acLastEdrRec[iLoop] = acEdrRec[iLoop] = ERR_PRO_TRRIGER;
				}
			}
		}
	}
    free(err_disable_cfg);
    free(err_recover_cfg);
    free(err_recover_time);
}




void edr_recover(INT32 skfd)
{
    int iPort;
	
    //printf("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
    for(iPort = 0; iPort < PORT_MAX; iPort++)
    {
		//printf("%s:%d\n", __FUNCTION__, __LINE__);
		if(port_edr_state[iPort].cPortState == EDR_DISABLE)
		{
			if(acLastEdrRec[iPort] != acEdrRec[iPort])
			{
				if(acEdrRecover[port_edr_state[iPort].enErrSrc] == ERR_CFG_ENABLE)
				{
					port_edr_state[iPort].lRecTimer = lTimerInterval;
					port_edr_state[iPort].enRecSrc = port_edr_state[iPort].enErrSrc;
					acLastEdrRec[iPort] = acEdrRec[iPort];
				}
				
			}
			if(acEdrRecover[port_edr_state[iPort].enErrSrc] == ERR_CFG_ENABLE)
			{
				if(port_edr_state[iPort].lRecTimer > 0)
				{
					port_edr_state[iPort].lRecTimer--;
					if(port_edr_state[iPort].lRecTimer == 0)
					{
						edr_recover_port(iPort, 1);
						#if 0
						port_recover(cfp_skfd, iPort+1, port_edr_state[iPort].enErrSrc, 1);
						port_edr_state[iPort].cPortState = EDR_NORMAL;
						port_edr_state[iPort].enRecSrc = ERR_SRC_NULL;
						port_edr_state[iPort].lRecTimer = 0;
						acEdrRec[iPort] = ERR_PRO_UNTRRIGER;
						acLastEdrRec[iPort] = acEdrRec[iPort];
						for(iLoop = 0; iLoop < EVENT_NUMBER+1; iLoop++)
						{
							if(access(astEdrScokAddr[iLoop].sun_path, F_OK) == 0)
							{
								//printf("%s:%s:%d--sock_path=%s\n", __FILE__, __FUNCTION__, __LINE__, astEdrScokAddr[iLoop].sun_path);
								edr_ipc_send(skfd, &astEdrScokAddr[iLoop], iPort+1, port_edr_state[iPort].enErrSrc);
							}
						}
						#endif
					}
				}
			}
			
		}
    }
}

void edr_disable(INT32 iPortNo, INT32 iErrSrc)
{
    //static char link_status[PORT_MAX];
	int skfd;
	INT32 iPort = iPortNo-1;
	if((skfd = open(DEVICE_FILE_NAME, 0)) < 0) 
	{
		return;
	}
	//printf("port %d err %d state = %02x, enable detect = %02x\n", iPortNo, iErrSrc, (*(port_edr_state+iPortNo)).enErrSrc[iErrSrc], acEdrDetect[iErrSrc]);
	//printf("iPortNo=%d iErrSrc=%d\n", iPortNo, iErrSrc);
	//printf("disable before:edr_port_map=%08x %08x\n", (INT32)(edr_port_map>>32), (INT32)edr_port_map);
	if( acEdrDetect[iErrSrc] == ERR_CFG_ENABLE)
	{
		//printf("err_disable.c:edr_disable:359--\n");/*renshanming: for debug*/
		port_edr_state[iPort].enErrSrc = iErrSrc;
		port_edr_state[iPort].cPortState = EDR_DISABLE;
		acEdrRec[iPort] = ERR_PRO_TRRIGER;
		edr_port_map |= (0x01ULL << phy[iPortNo]);
		set_errdisable_cfp(skfd, edr_port_map);
		stp_set_port_state(iPortNo, STATE_DISCARD);/* disable learning mac address */
		clear_mac_by_port(skfd, iPortNo);
		syslog_err_disable(iPortNo, iErrSrc);
	}
	
	//printf("disable after:edr_port_map=%08x %08x\n", (INT32)(edr_port_map>>32), (INT32)edr_port_map);
	close(skfd);
}


#endif
void ring_tick(void)
{
	char *buffer = "ErrDisableTimer";

	/* Write to the pipe indicating that the timer has expired. */
	if (write (timerPipe[1], buffer, strlen (buffer)) < 0)
	{
		printf("Error: ErrDisable Pipe Timer write failed.\n");
	}
	return;
}

/* init */
void init_time(void)
{
    struct itimerval val;

    val.it_value.tv_sec = 1;
    val.it_value.tv_usec = 0;
    val.it_interval = val.it_value;
    setitimer(ITIMER_REAL, &val, NULL);
}
void edr_cfp_enable()
{
	int iPort;
	int skfd;
	static int first_signal = 0;
	if (0 == first_signal) {
		first_signal = 1;
		if((skfd = open(DEVICE_FILE_NAME, 0)) < 0) {
			return;
		}
		for (iPort = 1; iPort <= PNUM; iPort++) {
			edr_port_map &= (~(0x01ULL << phy[iPort]));
		}
		set_errdisable_cfp(skfd, edr_port_map);
		close(skfd);
	}
	
}
int main(int argc, char** argv)
{
    int retval, maxfd;
    struct sockaddr_un server_sock_addr, client_sock_addr;
	fd_set rfds, allset;
	
	IPC_SK rx, tx;
	unsigned char timerBuffer[20];
	int iLoop;
	#if 1
	/*creat socket for inter-process communication*/
    if(creat_sk_server(&iEdrIpcSkfd, &server_sock_addr, SOCK_PATH_EDR) == -1)
	{
		return -1;
	}
	//printf("%s:%s:%d--%s\n", __FILE__, __FUNCTION__, __LINE__, SOCK_PATH_SERVER);
	FD_ZERO(&rfds);/*clear file define*/
	/* Create a pipe for sending timer */
	if (pipe(timerPipe) < 0)
	{
		printf("errdisable unable to create network timer.\n");
		return -1;
	}
	
	/* Once the timer pipe has been created add it to the read set. */
	FD_SET(timerPipe[0], &rfds);
	if (maxfd < timerPipe[0])
		maxfd = timerPipe[0];

	FD_SET(iEdrIpcSkfd, &rfds);
	if (maxfd < iEdrIpcSkfd)
		maxfd = iEdrIpcSkfd;
	memcpy ((void *)&allset, (void *)&rfds, sizeof (fd_set));
	
	init_time();
	init_err_disable();
	signal(SIGALRM, (void*)ring_tick);/*error disable recover handler*/
	signal(SIGUSR1, (void*)edr_change_cfg);
	signal(SIGUSR2, (void*)edr_cfp_enable);
	while(1)
	{
		FD_ZERO(&rfds);/*clear file define*/
		memcpy ((void *)&rfds, (void *)&allset, sizeof (fd_set));
		retval = select(maxfd+1, &rfds, (fd_set *)NULL, (fd_set *)NULL, NULL);/*detect there are file can read */

		if(retval < 0)/*select failed*/
		{
			/* Error: Select failed. */
			if (errno == EINTR) 
			{
				//printf("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);/*shanming.ren 2012-1-13 16:38:01*/
				/* YES -- Check if this is because of our SIG_ALARM. */
				if (FD_ISSET (timerPipe[0], &rfds)) 
				{
					/* Timer expired. Read the pipe message. */
					if (read (timerPipe[0], &timerBuffer[0], sizeof(timerBuffer)) < 0) 
					{
						printf("Warning: Errdisable read on timer pipe failed.\n");
						break;
					} 
					else
					{
						/* Handle edr_disable recover Timer expiration. */
						edr_recover(iEdrIpcSkfd);
						//printf("error one second\n");
						continue;
					}
				} 
				else 
				{
					/* NO -- This is not because of our signal. */
					printf("Error: Errdisable exiting Application - No data on pipe.");
					break;
				}
			} 
			else 
			{
				/* Failure - Select failed. */
				printf("Error: Errdisable select failed Error Number:%d.",errno);
				break;
			}
		}
		else if(retval) /*select success*/
		{
			if (FD_ISSET (timerPipe[0], &rfds)) 
			{
				/* Timer expired. Read the pipe message. */
				if (read (timerPipe[0], &timerBuffer[0], sizeof(timerBuffer)) < 0) 
				{
					printf("Warning: Read on timer pipe failed.\n");
					break;
				} 
				else
				{
					/* Handle edr_disable recover Timer expiration. */
					edr_recover(iEdrIpcSkfd);
					//printf("one second\n");
					continue;
				}
			} 

			if ( FD_ISSET(iEdrIpcSkfd, &rfds) )/* whether socket receive data */
			{
				/*receive data*/
				//printf("%s.%s.%d\n", __FILE__, __FUNCTION__, __LINE__);
				if(ipc_recv(iEdrIpcSkfd, &rx, &client_sock_addr) == -1)
				{
					printf("errdisable:receive failed\n");
				}
				else/*receive successful*/
				{
					//printf("%s.%s.%d\n", __FILE__, __FUNCTION__, __LINE__);
					SK_EDR *pstRx = (SK_EDR*)rx.acData;
					SK_EDR *pstTx = (SK_EDR*)tx.acData;
					/*send data which received to client*/
					if(rx.stHead.enCmd == IPC_CMD_EDR_RECOVER)
					{
						//printf("%s.%s.%d\n", __FILE__, __FUNCTION__, __LINE__);
						//if(port_edr_state[pstRx->iPort-1].cPortState == EDR_DISABLE)
						{
							//printf("%s.%s.%d, port%d recover\n", __FILE__, __FUNCTION__, __LINE__, pstRx->iPort);
							edr_recover_port(pstRx->iPort-1, 0);
						}
					}
					else if(rx.stHead.enCmd == IPC_CMD_EDR_DISABLE)
					{
						//printf("%s.%s.%d\n", __FILE__, __FUNCTION__, __LINE__);
						edr_disable(pstRx->iPort, pstRx->enErrSrc);
						tx.stHead.enCmd = rx.stHead.enCmd;
						tx.stHead.cOpt = 1;
						tx.stHead.cBack = IPC_SK_NOBACK;
						pstTx->iPort = pstRx->iPort;
						pstTx->enErrSrc = pstRx->enErrSrc;
						memcpy(&astEdrScokAddr[pstRx->enErrSrc], &client_sock_addr, sizeof(struct sockaddr_un));

						//printf("%s.%s.%d\n", __FILE__, __FUNCTION__, __LINE__);
						for(iLoop = 0; iLoop < EVENT_NUMBER+1; iLoop++)
						{
							if(access(astEdrScokAddr[iLoop].sun_path, F_OK) == 0)
							{
								//printf("%s.%s.%d--path=%s\n", __FILE__, __FUNCTION__, __LINE__, astEdrScokAddr[iLoop].sun_path);
								if((rx.stHead.cBack == IPC_SK_NOBACK) && (iLoop == pstRx->enErrSrc))
								{
									continue;
								}
								if(ipc_send(iEdrIpcSkfd, &tx, &astEdrScokAddr[iLoop]) == -1)
								{
									printf("errdisable:send failed\n");
								}
							}
						}
						//printf("%s.%s.%d\n", __FILE__, __FUNCTION__, __LINE__);
						
					}
					#if 1
					else if(rx.stHead.enCmd == IPC_CMD_GET)
					{
						//printf("%s.%s.%d\n", __FILE__, __FUNCTION__, __LINE__);
						if(rx.stHead.cOpt == 0)
						{
							//printf("%s.%s.%d\n", __FILE__, __FUNCTION__, __LINE__);
							tx.stHead.enCmd = rx.stHead.enCmd;
							tx.stHead.cOpt = 1;
							tx.stHead.cBack = 0;
							memcpy(tx.acData, port_edr_state, sizeof(port_edr_state));
							if(ipc_send(iEdrIpcSkfd, &tx, &client_sock_addr) == -1)
							{
								printf("errdisable:send failed\n");
							}
						}
					}
					else if(rx.stHead.enCmd == IPC_CMD_SET)
					{
						int cfp_skfd;

						if((cfp_skfd = open(DEVICE_FILE_NAME, 0)) < 0) 
							continue;
						if(rx.stHead.cOpt == 0)
						{
							edr_port_map &= ~(0x01ULL << phy[1]);
						}
						else
						{
							edr_port_map |= (0x01ULL << phy[1]);
						}
						set_errdisable_cfp(cfp_skfd, edr_port_map);
						tx.stHead.enCmd = rx.stHead.enCmd;
						tx.stHead.cOpt = rx.stHead.cOpt;
						close(cfp_skfd);
					}
					#endif
					//printf("%s.%s.%d\n", __FILE__, __FUNCTION__, __LINE__);
					
				}
			}
		}
		else /*select timeout*/
		{
			
		}
	}
	#endif
	return 0;
}
