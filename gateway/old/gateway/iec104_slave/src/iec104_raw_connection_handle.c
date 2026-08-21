
/*
*  Copyright (c) 2019,广州碳索智控科技有限公司
*  All rights reserved.
* 
*  文件名称：
*  文件标识：
*  描    述：
* 
*  当前版本：V1.0
*  作    者：chenzy
*  运行环境：
*  操作系统: linux
*  硬件平台：
*  固件库  ：
*  修改记录 
*  作者       时间         版本      修改描述
*  chenzy          V1.00      
*/

#include "dev_log.h"
#include "iec104_cfg.h"
#include "db_node_fops.h"
#include "iec104_raw_connection_handle.h"

/*********************************************************************************************************
** Function name:
** Descriptions: Callback handler to log sent or received messages (optional)
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void iec104_slave_clientRawMessageHandler (void* parameter, uint8_t* msg, int msgSize, bool sent)
{
    struct config_chnl_working_attr *pChnl = (struct config_chnl_working_attr *)parameter;

    if(pChnl->debug)
	{
		if (sent)
			printf("SEND cnnl%d: ", pChnl->id);
		else
			printf("RCVD chnl%d: ", pChnl->id);

		int i;
		for (i = 0; i < msgSize; i++) {
			printf("%02x ", msg[i]);
		}

    	printf("\n");
	}

    if (sent) {
        log_write(LOG_CHNL_FW(pChnl->id), 1, LOG_TX, msg, msgSize);
    } else {
        log_write(LOG_CHNL_FW(pChnl->id), 1, LOG_RX, msg, msgSize);
    }
}


/*********************************************************************************************************
** Function name:
** Descriptions: Callback handler to log sent or received messages (optional)
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void iec104_slave_serverRawMessageHandler(void* parameter, IMasterConnection conneciton, uint8_t* msg, int msgSize, bool sent)
{
    struct config_chnl_working_attr *pChnl = (struct config_chnl_working_attr *)parameter;

    if(pChnl->debug)
	{
		if (sent)
			printf("SEND cnnl%d: ", pChnl->id);
		else
			printf("RCVD chnl%d: ", pChnl->id);

		int i;
		for (i = 0; i < msgSize; i++) {
			printf("%02x ", msg[i]);
		}

    	printf("\n");
	}

    if (sent) {
        log_write(LOG_CHNL_FW(pChnl->id), 1, LOG_TX, msg, msgSize);
    } else {
        log_write(LOG_CHNL_FW(pChnl->id), 1, LOG_RX, msg, msgSize);
    }
}
/*********************************************************************************************************
** Function name:
** Descriptions: Connection event handler,for client
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void iec104_slave_connectionHandler (void* parameter, CS104_Connection connection, CS104_ConnectionEvent event)
{
    switch (event) {
    case CS104_CONNECTION_OPENED:
        printf("Connection established\n");
        break;
    case CS104_CONNECTION_CLOSED:
        printf("Connection closed\n");
        break;
    case CS104_CONNECTION_STARTDT_CON_RECEIVED:
        printf("Received STARTDT_CON\n");
        break;
    case CS104_CONNECTION_STOPDT_CON_RECEIVED:
        printf("Received STOPDT_CON\n");
        break;
    }
}
/*********************************************************************************************************
** Function name:
** Descriptions: Connection event handler,for server
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void iec104_slave_connectionEventHandler(void* parameter, IMasterConnection con, CS104_PeerConnectionEvent event)
{
    struct config_chnl_working_attr *pChnl = (struct config_chnl_working_attr *)parameter;
    struct iec104_comm_ctx_t *pcomm = &pChnl->iec104_comm_ctx;

    if (event == CS104_CON_EVENT_CONNECTION_OPENED) {
        pcomm->cs104_connected = true;
        printf("Connection opened (%p)\n", con);
    }
    else if (event == CS104_CON_EVENT_CONNECTION_CLOSED) {
        pcomm->cs104_connected = false;
        printf("Connection closed (%p)\n", con);
    }
    else if (event == CS104_CON_EVENT_ACTIVATED) {
        printf("Connection activated (%p)\n", con);
    }
    else if (event == CS104_CON_EVENT_DEACTIVATED) {
        printf("Connection deactivated (%p)\n", con);
    }
}

/*********************************************************************************************************
** Function name:
** Descriptions: 
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
bool iec104_slave_connectionRequestHandler(void* parameter, const char* ipAddress)
{
    printf("New connection request from %s\n", ipAddress);

#if 0
    if (strcmp(ipAddress, "127.0.0.1") == 0) {//可以设置客户端白名单
        printf("Accept connection\n");
        return true;
    }
    else {
        printf("Deny connection\n");//设置黑名单
        return false;
    }
#else
    return true;
#endif
}

