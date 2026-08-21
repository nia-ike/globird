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
#include <assert.h>
#include <errno.h>
#include <sys/ipc.h> 
#include <sys/msg.h>

#include "db_msr_msgevent_api.h"

struct msgevent_api_t
{
    int msgid;
    uint8_t initialized;  // 标记该通道是否已初始化
};

/* 动态管理的消息事件上下文 */
static struct msgevent_api_t *msgevent_api_ctx = NULL;
static uint8_t max_channels = 2;  // 默认支持2个通道，可通过config函数修改
static uint8_t api_configured = 0; // 标记是否已配置

/*************************************************************************/
/** Function Name:     db_msr_msg_event_api_config                        **/
/** Purpose:           配置最大通道数，并分配内存                          **/
/** Params:            max_channels - 最大通道数                           **/
/** Return:            0: success, -1: error                              **/
/** Notice:            必须在任何init调用之前调用此函数                     **/
/*************************************************************************/
int db_msr_msg_event_api_config(uint8_t channels)
{
    if(channels == 0)
    {
        printf("db_msr_msg_event_api_config: invalid channels number (must > 0)\n");
        return -1;
    }
    if(api_configured)
    {
        printf("db_msr_msg_event_api_config: already configured, please call destory first\n");
        return -1;
    }

    /* 分配内存 */
    msgevent_api_ctx = (struct msgevent_api_t *)calloc(channels, sizeof(struct msgevent_api_t));
    if(msgevent_api_ctx == NULL)
    {
        printf("db_msr_msg_event_api_config: calloc failed\n");
        return -1;
    }

    max_channels = channels;
    api_configured = 1;
    
    printf("db_msr_msg_event_api_config: configured for %d channels\n", channels);
    return 0;
}

/*************************************************************************/
/** Function Name:     db_msr_msg_event_api_get_max_channels             **/
/** Purpose:           获取当前配置的最大通道数                            **/
/** Params:            None.                                              **/
/** Return:            max_channels                                       **/
/** Notice:            None.                                              **/
/*************************************************************************/
uint8_t db_msr_msg_event_api_get_max_channels(void)
{
    return max_channels;
}

/*************************************************************************/
/** Function Name:     _ensure_context_initialized                        **/
/** Purpose:           确保上下文已分配（懒加载机制）                       **/
/** Params:            None.                                              **/
/** Return:            0: success, -1: error                              **/
/** Notice:            内部使用，支持向后兼容（不调用config也能工作）        **/
/*************************************************************************/
static int _ensure_context_initialized(void)
{
    /* 已分配内存，直接返回 */
    if(msgevent_api_ctx != NULL)
        return 0;

    /* 如果已调用过 config 但内存为 NULL，说明出现异常 */
    if(api_configured)
    {
        printf("db_msr_msg_event_api: context lost after config (memory corrupted?)\n");
        return -1;
    }

    /* 懒加载：如果没有调用 config，使用默认值自动分配 */
    printf("db_msr_msg_event_api: auto-allocating for %d channels\n", max_channels);
    printf("  (Recommended: call db_msr_msg_event_api_config() explicitly before init)\n");
    
    msgevent_api_ctx = (struct msgevent_api_t *)calloc(max_channels, sizeof(struct msgevent_api_t));
    if(msgevent_api_ctx == NULL)
    {
        printf("db_msr_msg_event_api: calloc failed for %d channels\n", max_channels);
        return -1;
    }

    api_configured = 1;
    return 0;
}

/*************************************************************************/
/** Function Name:                                                         **/
/** Purpose:                                                               **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int db_msr_msg_event_api_pend(uint8_t chnlno, struct db_msr_msg_evt_general_t *p)
{
	int                 ret;
	struct db_msr_msg_evt_t msg;     
    long int            msgtype = 0;// recieve first msg

    /* 确保上下文已初始化 */
    if(_ensure_context_initialized() < 0)
        return -1;

    /* 边界检查 */
    if(chnlno >= max_channels)
    {
        printf("db_msr_msg_event_api_pend: invalid channel %d (max: %d)\n", chnlno, max_channels);
        return -1;
    }

    /* 检查通道是否已初始化 */
    if(!msgevent_api_ctx[chnlno].initialized)
    {
        printf("db_msr_msg_event_api_pend: channel %d not initialized\n", chnlno);
        return -1;
    }

	ret = msgrcv(msgevent_api_ctx[chnlno].msgid, (void*)&msg, sizeof(struct db_msr_msg_evt_t)-sizeof(long), msgtype, 0);
	if(ret > 0)
		memcpy(p, msg.mq_pool, sizeof(msg.mq_pool));
	else 
		sleep(1);

	return ret;
} 


/*************************************************************************/
/** Function Name:     db_msr_msg_event_api_post                          **/
/** Purpose:           发送消息到指定通道                                  **/
/** Params:            chnlno - 通道号, p - 消息指针, len - 消息长度       **/
/** Return:            0: success, -1: error                              **/
/** Notice:            None.                                               **/
/*************************************************************************/
int db_msr_msg_event_api_post(uint8_t chnlno, void *p, int len)
{
    int ret;
    struct db_msr_msg_evt_t msg;

    /* 确保上下文已初始化 */
    if(_ensure_context_initialized() < 0)
        return -1;

    /* 边界检查 */
    if(chnlno >= max_channels)
    {
        printf("db_msr_msg_event_api_post: invalid channel %d (max: %d)\n", chnlno, max_channels);
        return -1;
    }

    /* 检查通道是否已初始化 */
    if(!msgevent_api_ctx[chnlno].initialized)
    {
        printf("db_msr_msg_event_api_post: channel %d not initialized\n", chnlno);
        return -1;
    }
	
    len = (len > DB_MSR_EVENT_MSG_SIZE) ? DB_MSR_EVENT_MSG_SIZE : len;

	msg.msg_type = 1;
	memcpy(msg.mq_pool, p, len);
	ret = msgsnd(msgevent_api_ctx[chnlno].msgid, (void*)&msg, sizeof(struct db_msr_msg_evt_t)-sizeof(long), 0);

    return ret;
} 


/*************************************************************************/
/** Function Name:     db_msr_msg_event_api_init                          **/    
/** Purpose:           初始化指定通道的消息队列                             **/    
/** Params:            chnlno - 通道号                                     **/    
/** Return:            0: success, -1: error                              **/    
/** Notice:            如果通道号超出范围，返回错误                         **/    
/*************************************************************************/
int db_msr_msg_event_api_init(uint8_t chnlno)
{
    /* 确保上下文已初始化 */
    if(_ensure_context_initialized() < 0)
        return -1;

    /* 边界检查 */
    if(chnlno >= max_channels)
    {
        printf("db_msr_msg_event_api_init: invalid channel %d (max: %d)\n", chnlno, max_channels);
        printf("  Hint: call db_msr_msg_event_api_config(%d) before init\n", chnlno + 1);
        return -1;
    }

    /* 检查是否已经初始化 */
    if(msgevent_api_ctx[chnlno].initialized)
    {
        printf("db_msr_msg_event_api_init: channel %d already initialized\n", chnlno);
        return 0;  // 已初始化，返回成功
    }

    /* 创建消息队列 */
	msgevent_api_ctx[chnlno].msgid = msgget((key_t)(DB_MSR_MSG_KEY+chnlno), 0666 | IPC_CREAT);
	if(msgevent_api_ctx[chnlno].msgid == -1)
	{
		printf("db_msr_msg_event_api_init: msgget err:%s\n", strerror(errno));
		return -1;
	}

    msgevent_api_ctx[chnlno].initialized = 1;
    printf("db_msr_msg_event_api_init: channel %d initialized (msgid: %d)\n", chnlno, msgevent_api_ctx[chnlno].msgid);

	return 0;
}

/*************************************************************************/
/** Function Name:     db_msr_msg_event_api_destory                       **/
/** Purpose:           销毁所有消息队列，释放动态分配的资源                 **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            会删除所有System V消息队列并释放内存                 **/
/*************************************************************************/
void db_msr_msg_event_api_destory(void)
{
    int i;

    if(msgevent_api_ctx == NULL)
    {
        printf("db_msr_msg_event_api_destory: context not initialized\n");
        return;
    }

    /* 销毁所有已初始化的消息队列 */
    for(i = 0; i < max_channels; i++)
    {
        if(msgevent_api_ctx[i].initialized)
        {
            /* 删除System V消息队列 */
            if(msgctl(msgevent_api_ctx[i].msgid, IPC_RMID, NULL) == -1)
            {
                printf("db_msr_msg_event_api_destory: msgctl IPC_RMID failed for channel %d: %s\n", 
                       i, strerror(errno));
            }
            else
            {
                printf("db_msr_msg_event_api_destory: channel %d destroyed (msgid: %d)\n", 
                       i, msgevent_api_ctx[i].msgid);
            }
            msgevent_api_ctx[i].initialized = 0;
            msgevent_api_ctx[i].msgid = -1;
        }
    }

    /* 释放动态分配的内存 */
    free(msgevent_api_ctx);
    msgevent_api_ctx = NULL;
    api_configured = 0;
    
    printf("db_msr_msg_event_api_destory: all resources released\n");
}



/*************************************************************************/
/** Function Name:     db_msr_msg_event_cmd                               **/
/** Purpose:           发送通用命令消息（事件体无需参数，只需要命令）        **/
/** Params:            chnlno - 通道号, cmd - 命令类型                     **/
/** Return:            0: success, -1: error                              **/
/** Notice:            内部调用db_msr_msg_event_api_post，会自动检查边界    **/
/*************************************************************************/
int db_msr_msg_event_cmd(uint8_t chnlno, int cmd)
{
    struct db_msr_msg_evt_general_t msg;

    msg.type = cmd;
    return db_msr_msg_event_api_post(chnlno, &msg, sizeof(msg));
}

