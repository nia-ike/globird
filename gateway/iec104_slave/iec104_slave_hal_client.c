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

#include <stdio.h>  
#include <sys/types.h>  
#include <sys/stat.h>  
#include <sys/ioctl.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <sys/mman.h>  
#include <linux/vt.h>  
#include <stdint.h> 
#include <stdlib.h>

#include "iec104_slave_hal_client.h"

#include "iec104_slave_clocksync_handle.h"
#include "iec104_slave_interrogation_handle.h"
#include "iec104_slave_asdu_handle.h"
#include "iec104_raw_connection_handle.h"

/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int iec104_slave_hal_client_init(struct config_chnl_working_attr *pChnl)  
{
    struct iec104_comm_ctx_t *pComm = &pChnl->iec104_comm_ctx;

    /* create a new slave/server instance with default connection parameters and
     * default message queue size */
    pComm->slave = CS104_Slave_create(100, 100);

//    CS104_Client_Connection_set(pComm->slave, pComm->dev, pComm->port);

    CS104_Slave_setLocalAddress(pComm->slave, pComm->dev);
    CS104_Slave_setLocalPort(pComm->slave, pComm->port);

    /* Set mode to a single redundancy group
     * NOTE: library has to be compiled with CONFIG_CS104_SUPPORT_SERVER_MODE_SINGLE_REDUNDANCY_GROUP enabled (=1)
     */
    CS104_Slave_setServerMode(pComm->slave, CS104_MODE_CONNECTION_IS_REDUNDANCY_GROUP);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(pComm->slave);
    //APCI参数读取，server端一样设置APCI参数，但都在初始化协议的时候设置默认参数，之后不再更改.client端的APCI
    //参数要和server端的保持一致。因此，如对server端的APCI参数值不清楚，可进行读出操作
    //CS104_server_getACPIParameters(pCtx->slave);//读出参数，看参数是否是设置的值，或者读出默认设置的参数值

    //一些报文字节数设置:传输原因字节数、公共地址字节数、信息体地址字节数
    //默认值是2,2,3，一样是在初始化协议参数时设置，之后不再更改. server端设置的值，client端需和server端保持一致
    //如对server端的此3个报文字节参数值不清楚，可进行读出操作
    //CS104_server_getAppLayerParameters(pCtx->slave);

    //初始化回调（6种） 
    /* set the callback handler for the clock synchronization command */
    CS104_Slave_setClockSyncHandler(pComm->slave, iec104_slave_clockSyncHandler, pChnl);//收到时钟同步信息
    /* set the callback handler for the interrogation command */
    CS104_Slave_setInterrogationHandler(pComm->slave, iec104_slave_interrogationHandler, pChnl);//处理客户端的查询请求
    CS104_Slave_setCounterInterrogationHandler(pComm->slave, iec104_slave_counterInterrogationHandler, pChnl);//处理客户端的反查询请求

    /* set handler for other message types */
    CS104_Slave_setASDUHandler(pComm->slave, iec104_slave_asduHandler, pChnl);//除查询请求外，其他消息类型处理
    /* set handler to handle connection requests (optional) */
    CS104_Slave_setConnectionRequestHandler(pComm->slave, iec104_slave_connectionRequestHandler, pChnl);//设置黑白名单，限制对server的访问
    /* set handler to track connection events (optional) */
    CS104_Slave_setConnectionEventHandler(pComm->slave, iec104_slave_connectionEventHandler, pChnl);//
    /* 查看打印原始收发数据，十六进制格式*/
    /* uncomment to log messages */
    
    CS104_Slave_setRawMessageHandler(pComm->slave, iec104_slave_serverRawMessageHandler, pChnl);

    return 0;
}


