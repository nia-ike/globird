/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
* 
*  文件名称：rt_database.c
*  文件标识：
*  描    述：SimpleGUI接口
* 
*  当前版本：V1.0
*  作    者：chenzy
*  运行环境：
*  操作系统: linux
*  硬件平台：f1c100s
*  固件库  ：
*  修改记录 
*  作者       时间         版本      修改描述
*  chenzy   2020       V1.00      
*/

#include <stdio.h>  
#include <string.h>
#include <sys/types.h>  
#include <sys/stat.h>  
#include <sys/ioctl.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <sys/mman.h>  
#include <linux/vt.h>  
#include <stdint.h> 
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h> 
#include <sys/stat.h>  //stat
#include <time.h>
#include <fcntl.h>   
#include <assert.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/ipc.h> 
#include <sys/msg.h>

#include "db_msgevent_api.h"

struct msgevent_api_t
{
    int msgid;
	uint32_t msg_done;
};

#define TRUE 1
#define FALSE 0

static struct msgevent_api_t msgevent_api_ctx;

/*************************************************************************/
/** Function Name:                                                         **/
/** Purpose:                                                               **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int db_msg_event_api_pend(struct db_msg_evt_general_t *p)
{
	int                 ret;
	struct db_msg_evt_t msg;     
    long int            msgtype = 0;// recieve first msg

	ret = msgrcv(msgevent_api_ctx.msgid, (void*)&msg, sizeof(struct db_msg_evt_t)-sizeof(long), msgtype, 0);
	if(ret > 0)
		memcpy(p, msg.mq_pool, sizeof(msg.mq_pool));
	else 
		sleep(1);

	return ret;
} 


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int db_msg_event_api_post(void *p, int len)
{
    int ret;
    struct db_msg_evt_t msg;
	
    len = (len > DB_EVENT_MSG_SIZE) ? DB_EVENT_MSG_SIZE : len;

	msg.msg_type = 1;
	memcpy(msg.mq_pool, p, len);
	ret = msgsnd(msgevent_api_ctx.msgid, (void*)&msg, sizeof(struct db_msg_evt_t)-sizeof(long), 0);

    return ret;
} 


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int db_msg_event_api_init(void)
{
    /* msg  */
	msgevent_api_ctx.msgid = msgget((key_t)DB_MSG_KEY, 0666 | IPC_CREAT);
	msgevent_api_ctx.msg_done = FALSE;
	if(msgevent_api_ctx.msgid == -1)
	{
		printf("msgget err:%s\n", strerror(errno));
		return -1;
	}
	return 0;
}

/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
void db_msg_event_api_destory(void)
{
    /* 销毁消息队列 */
    if(msgevent_api_ctx.msgid != -1)
    {
        if(msgctl(msgevent_api_ctx.msgid, IPC_RMID, NULL) == -1)
        {
            printf("db_msg_event_api_destory: msgctl failed: %s\n", strerror(errno));
        }
        else
        {
            printf("db_msg_event_api: message queue destroyed\n");
        }
        msgevent_api_ctx.msgid = -1;
    }
}



/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:           general command. 事件体无需参数，只需要命令                              **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int db_msg_event_cmd(int cmd)
{
    struct db_msg_evt_general_t msg;

    msg.type = cmd;
    return db_msg_event_api_post(&msg, sizeof(msg));
}


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:           general command. 事件体无需参数，只需要命令                              **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int db_msg_event_cmd_send(int cmd)
{
    struct db_msg_evt_general_t msg;
    int n;
    msg.type = cmd;
	msgevent_api_ctx.msg_done = FALSE;

    db_msg_event_api_post(&msg, sizeof(msg));

	n = 20;
	while(n--)
	{
		if(msgevent_api_ctx.msg_done == TRUE)
			return 0;
		usleep(100000);
	}

	return -1;
}

/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:           general command. 事件体无需参数，只需要命令                              **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int db_msg_done(void)
{
	msgevent_api_ctx.msg_done = FALSE;
	return 0;
}

