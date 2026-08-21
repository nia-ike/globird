/*
* Copyright (c) 
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

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#include "mb_cfg.h"
#include "mb_protocol.h"
#include "mb_process.h"

#include "db_struct.h"





/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
void mb_thread_proc(struct mb_comm_ctx_t *pCtx)
{
    static int re_connect_window = 0;
    static const uint16_t reconnect_table[] = {1, 5, 30, 120};
    /*
        通常5分钟内一定会有数据报文接收。这里设定1小时都没有报文接收，超时后重连串口或者socket
        容错用途
    */
    pCtx->recvLen = pCtx->pcombase->read(pCtx, &pCtx->recvBuff[0], MB_COM_RXBUFF_SIZE, 3600000);
    if ((pCtx->recvLen >= MB_COM_PACKET_MIN_LEN) && (pCtx->recvLen < MB_COM_RXBUFF_SIZE))
    {
        /*收到数据，重连时间窗设置为0*/
        re_connect_window = 0;

        if(pCtx->msg_protocol_process)
            pCtx->msg_protocol_process(pCtx, &pCtx->recvBuff[0], pCtx->recvLen);
    }
    if(pCtx->recvLen <= 0)
    {
        pCtx->pcombase->disconnect(pCtx);

        sleep(reconnect_table[re_connect_window]);

        /*
            re-connect fail. maybe network is issue. we must sleep x seconds
            1. when fail, re-connect at once!
            2. when fail 2 times, re-connect wait 1 second, and next 5, 25, 120 second
            3. when fail continue, keep 120s re-connect.
            4. if connect successful, restore 1
        */
        ++re_connect_window;

        /*
            非tcpclient模式，无需节省流量，应该异常或者超时后5s重连
        */
        if(pCtx->mode != 1)
            re_connect_window = 0;

        if(re_connect_window >= sizeof(reconnect_table)/sizeof(uint16_t))
            re_connect_window = sizeof(reconnect_table)/sizeof(uint16_t) - 1;

        pCtx->pcombase->connect(pCtx);
       /*
            忽略本注释：（还保留是因为将来可能还需要查询时作为辅助参考）
            connect() 后会出现真正连接成功、或者对端端口拒绝情况，此时connect依然返回成功0 （暂不知原因）
            因此为了防止在服务器异常情况下，流量过快耗尽。现在无论重新connect是否成功，都进入窗口延迟机制.

            加快重连，首次重connect()，不延时。
       */

    }

}

/*********************************************************************************************************
** Function name:       mb_server_init
** Descriptions:        modbus rtu 转发服务初始化
** input parameters:    None
** output parameters:   None
** Returned value:      0: success  other:fail
*********************************************************************************************************/
int mb_server_init(struct mb_comm_ctx_t *pCtx)
{
    int ret;
   /*  */
    ret = pCtx->pcombase->connect(pCtx);
    if(ret < 0)
    {
        printf("Can not connect to %s\n", pCtx->dev);
        return -1;
    }

    return 0;
}


/*********************************************************************************************************
** Function name:       mb_server_i
** Descriptions:
** input parameters:    None
** output parameters:   None
** Returned value:      0: success  other:fail
*********************************************************************************************************/
void mb_server_destory(struct mb_comm_ctx_t *pCtx)
{
    pCtx->pcombase->disconnect(pCtx);
    pCtx->fd = -1;
}
