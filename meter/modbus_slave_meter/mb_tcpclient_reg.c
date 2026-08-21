/*
* Copyright (c) 2013,珠海派诺电子有限公司
* All rights reserved.
*
* 文件名称：
* 文件标识：
* 描    述：
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：
* 完成日期：20131108
*
* 修改记录
*  作者       时间         版本      修改描述
*  chenzy   2020.6.09   V1.00      创建
*/

#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <errno.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include "mb_com_baseclass.h"
#include "cJSON.h"
#include "cJSON_Utils.h"
/* dataInv min value  */
#define REG_INV_MIN_TIME  (30)
#define DB_CFG_COMM_TCPCLIENT_FILE     "/userdata/config/comm/universally/modbustcp_slave_client.json"
struct mb_tcpclient_connect_cfg
{
    int  checked;
    char ip[17];
    int  port;
    int  slaveid;
    int  regtype;
    int  dataInv;    
    char regcontent[10];
};
static pthread_t  mb_tcpclient_regThread;
static struct     mb_tcpclient_connect_cfg  mb_tcpclient_cfg_obj;	
static struct     mb_tcpclient_connect_cfg *mb_tcpclient_cfg = &mb_tcpclient_cfg_obj;	
/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static char *db_cfg_jsonfile_alloc(char *file, int *size)
{ 
    struct stat filestat;
 
    if(NULL == file)
        return NULL;
   
    /* Get file size */
    if(0 != stat(file, &filestat))
    {  
        printf("Json file[%s] not exist\n", file);
        return NULL;
    }
    *size = filestat.st_size;

    return malloc(filestat.st_size + 1);
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static void db_cfg_jsonfile_delete(char *p)
{
    if(p == NULL)
        return;
    
    free(p);
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static char *db_cfg_jsonfile_read(char *file, char *p, int size)
{
    int n;
    FILE *fp;
    
    fp = fopen(file, "rb");
    if(fp == NULL)
        return NULL;

    n = fread(p, 1, size, fp);
    if(n <= 0)
        goto __err_close;

    p[size] = '\0';
    fclose(fp);
    return p;

__err_close:
    fclose(fp);

    return NULL;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int mb_comm_load_tcpclient_load(char *file)
{
    char    *p;
    int     size;
    cJSON   *pJson, *pjsClass, *pjsGroup;
    /* prepare the buffer for json */
    p = db_cfg_jsonfile_alloc(file, &size);
    if(p == NULL)
        return -1;
   
    p = db_cfg_jsonfile_read(file, p, size);
    if(p == NULL)
        goto __err_free;
 
    pJson = cJSON_Parse(p);  
    if(!pJson)
        goto __err_free;
  
    pjsClass = cJSON_GetObjectItem(pJson, "Param"); 
    if(!pjsClass)
        goto __err_json_format;

    pjsGroup=cJSON_GetObjectItem(pjsClass, "Checked");
    if(!pjsGroup)
	{
		goto __err_json_format;
	}
	mb_tcpclient_cfg->checked = pjsGroup->valueint;

    pjsGroup=cJSON_GetObjectItem(pjsClass, "IP");
    if(!pjsGroup)
	{
		goto __err_json_format;
	}
    snprintf(mb_tcpclient_cfg->ip, sizeof(mb_tcpclient_cfg->ip), "%s", pjsGroup->valuestring);
	
    pjsGroup=cJSON_GetObjectItem(pjsClass, "Port");
    if(!pjsGroup)
	{
		goto __err_json_format;
	}
	mb_tcpclient_cfg->port = pjsGroup->valueint;

    pjsGroup=cJSON_GetObjectItem(pjsClass, "SlaveId");
    if(!pjsGroup)
	{
		goto __err_json_format;
	}
	mb_tcpclient_cfg->slaveid = pjsGroup->valueint;

    pjsGroup=cJSON_GetObjectItem(pjsClass, "RegType");
    if(!pjsGroup)
	{
		goto __err_json_format;
	}
	mb_tcpclient_cfg->regtype = pjsGroup->valueint;

    /* get adjust parameters from syscfg.json */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "RegContent");
    if(!pjsGroup)
	{
		goto __err_json_format;
	}
    snprintf(mb_tcpclient_cfg->regcontent, sizeof(mb_tcpclient_cfg->regcontent), "%s", pjsGroup->valuestring);

    /* get reg dataInv  */
    pjsGroup=cJSON_GetObjectItem(pjsClass, "DataInv");
    if(!pjsGroup)
	{
        cJSON_AddNumberToObject(pjsClass, "DataInv", 0);
        mb_tcpclient_cfg->dataInv = 0;
		goto __err_json_format;
	}
    /* dataInv min value  */
    #define REG_INV_MIN_TIME  (30)
    if(pjsGroup->valueint > REG_INV_MIN_TIME )
    {
        mb_tcpclient_cfg->dataInv = pjsGroup->valueint;
    }
    else
    {
        mb_tcpclient_cfg->dataInv = REG_INV_MIN_TIME;
    }

    cJSON_Delete(pJson);
    db_cfg_jsonfile_delete(p);

    return 0;

__err_json_format:
    cJSON_Delete(pJson);

__err_free:
    db_cfg_jsonfile_delete(p);

    return -1;
}



/*********************************************************************************************************
** Function name:       mb_send_register_Packet
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void mb_tcpclient_register_Packet(void *parameter)
{
    struct mb_comm_ctx_t *pCtx = (struct mb_comm_ctx_t*)parameter;

    int reglen = strlen(pCtx->regPacket);

    if(reglen > 0)
    {
        int rc = send(pCtx->fd, pCtx->regPacket, reglen, 0);
        if(rc != reglen)
        {
            close(pCtx->fd);
            pCtx->fd = -1;
        }
    }
}


/*********************************************************************************************************
** Function name:       mb_upload_register_entry
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void* mb_upload_register_entry(void *parameter)
{
    static  int counter = 0;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    /* Attempt to open non blocking the access dev */
    while (1)
    {
        counter++;
        if(mb_tcpclient_cfg->dataInv < REG_INV_MIN_TIME)
        {
            mb_tcpclient_cfg->dataInv = REG_INV_MIN_TIME;
        }
        if(counter >= mb_tcpclient_cfg->dataInv)
        {
            counter = 0;
            mb_tcpclient_register_Packet(parameter);
        }
        sleep(1);    
    }
    exit(0);
}


/*********************************************************************************************************
** Function name:       mb_tcpclient_thread
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void mb_tcpclient_thread(void *parameter)
{
    /* load tcpclient cfg*/
    if(-1 == mb_comm_load_tcpclient_load(DB_CFG_COMM_TCPCLIENT_FILE))
    {
        printf("tcpclient_load faild\n");
        return;
    }

    if(mb_tcpclient_cfg->dataInv > 0)
    {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (!pthread_create(&mb_tcpclient_regThread, &attr, mb_upload_register_entry, parameter) == 0)
        {
            mb_tcpclient_regThread = 0;
            printf("Create reg Timer Thread failed.\n");
            return;
        }    
    }
}
