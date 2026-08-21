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
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>  //stat
#include <time.h>
#include <stdbool.h>
#include <signal.h> //

#include "hal_comm/hal_com_baseclass.h"
#include "iobridge.h"
#include "dev_log.h"




/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static void *iobridge_north2south_thread(void *arg)
{
#define NORTH2SOUTN_READ_SERIAL_TIMEOUT (300 * 1000)  // 5 minutes
#define NORTH2SOUTN_READ_OTHER_TIMEOUT (0x7fffffff)  // 其他模式下的超时时间

    int ret;
    struct iobridge_ctx_t *piobridge = (struct iobridge_ctx_t *)arg;
    struct hal_comm_ctx_t *pNorthCommCtx = &piobridge->northCommCtx;
    struct hal_comm_ctx_t *pSouthCommCtx = &piobridge->southCommCtx;
    uint32_t timeout = NORTH2SOUTN_READ_OTHER_TIMEOUT;
    uint16_t window = 0;;
    const uint16_t window_tbl[] = {1, 4, 8, 16, 32, 64, 128};

    // 串口需要超时做容错处理，因此等待超时比较短，
    // TCP 超时在检测到断开时会read立刻返回，因此超时时间设定特别大 
    if(pNorthCommCtx->mode == HAL_COM_MODE_SERIAL)
    {
        timeout = NORTH2SOUTN_READ_SERIAL_TIMEOUT;
    }

    // 
    pNorthCommCtx->pcombase->init(pNorthCommCtx);
    pNorthCommCtx->pcombase->connect(pNorthCommCtx);

    while(1)
    {
        ret = pNorthCommCtx->pcombase->read(pNorthCommCtx, pNorthCommCtx->rxbuff, sizeof(pNorthCommCtx->rxbuff), timeout);
        if(ret > 0)
        {
            pSouthCommCtx->pcombase->write(pSouthCommCtx, pNorthCommCtx->rxbuff, ret);

            // 收到数据后，重置超时时间
            window = 0;
        }
        else 
        {
            switch(pNorthCommCtx->mode)
            {
                case HAL_COM_MODE_SERIAL:
                    // 该桥接端点为串口，需要处理异常重连情况
                    // 如果北向连接在，而南向又超时没有接收到数据，则断开南向串口连接，并等待1秒后重新连接南向（用于南向异常的恢复）
                    // 如果北向连接不在，则不进行任何操作
                    if(pSouthCommCtx->fd > 0)
                    {
                        pNorthCommCtx->pcombase->disconnect(pNorthCommCtx);
                        sleep(1);
                        pNorthCommCtx->pcombase->connect(pNorthCommCtx);
                    }
                    break;
                case HAL_COM_MODE_TCP_SERVER:
                    // 该桥接端点为tcp server模式，需要无需处理，只管等待新链接即可
                    break;
                case HAL_COM_MODE_TCP_CLIENT:
                    // 该桥接端点为tcp client模式，需要处理异常重连情况
                    // 考虑到可能4G的流量问题， tcpclient 重连使用窗口
                    pNorthCommCtx->pcombase->disconnect(pNorthCommCtx);
                    sleep(window_tbl[window]);
                    ret = pNorthCommCtx->pcombase->connect(pNorthCommCtx);
                    if(ret < 0)
                    {
                        // 超时时间递增, 达到最大后，保持最大值
                        window++;
                        if(window >= sizeof(window_tbl) / sizeof(window_tbl[0]))
                        {
                            window = sizeof(window_tbl) / sizeof(window_tbl[0]) - 1;
                        }
                    }
                    else 
                    {
                        // 连接成功，重置超时时间
                        window = 0;
                    }
                    
                    break;
                case HAL_COM_MODE_UDP_SERVER:
                    // UDP 无需管理重连
                    break;
                case HAL_COM_MODE_UDP_CLIENT:
                    // UDP 无需管理重连
                    break;
                default:
                    break;
            }
        }
    }
    pNorthCommCtx->pcombase->disconnect(pNorthCommCtx);

    return NULL;
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static void *iobridge_south2north_thread(void *arg)
{
#define SOUTH2NORTH_READ_SERIAL_TIMEOUT (300 * 1000)  // 5 minutes
#define SOUTH2NORTH_READ_OTHER_TIMEOUT (0x7fffffff)  //

    int ret;
    struct iobridge_ctx_t *piobridge = (struct iobridge_ctx_t *)arg;
    struct hal_comm_ctx_t *pSouthCommCtx = &piobridge->southCommCtx;
    struct hal_comm_ctx_t *pNorthCommCtx = &piobridge->northCommCtx;
    uint16_t window = 0;;
    uint32_t timeout = SOUTH2NORTH_READ_OTHER_TIMEOUT;
    const uint16_t window_tbl[] = {1, 4, 8, 16, 32, 64, 128};

    // 串口需要超时做容错处理，因此等待超时比较短，
    // TCP 超时在检测到断开时会read立刻返回，因此超时时间设定特别大 
    if(pSouthCommCtx->mode == HAL_COM_MODE_SERIAL)
    {
        timeout = SOUTH2NORTH_READ_SERIAL_TIMEOUT;
    }

    // 
    pSouthCommCtx->pcombase->init(pSouthCommCtx);
    pSouthCommCtx->pcombase->connect(pSouthCommCtx);

    while(1)
    {
        if(pSouthCommCtx->mode == HAL_COM_MODE_SERIAL)
        {
            timeout = SOUTH2NORTH_READ_SERIAL_TIMEOUT;
        }

        ret = pSouthCommCtx->pcombase->read(pSouthCommCtx, pSouthCommCtx->rxbuff, sizeof(pSouthCommCtx->rxbuff), timeout);
        if(ret > 0)
        {
            pNorthCommCtx->pcombase->write(pNorthCommCtx, pSouthCommCtx->rxbuff, ret);
             // 收到数据后，重置超时时间
             window = 0;
        }
        else 
        {
            switch(pSouthCommCtx->mode)
            {
                case HAL_COM_MODE_SERIAL:
                    // 该桥接端点为串口，需要处理异常重连情况
                    // 如果北向连接在，而南向又超时没有接收到数据，则断开南向串口连接，并等待1秒后重新连接南向（用于南向异常的恢复）
                    // 如果北向连接不在，则不进行任何操作
                    if(pNorthCommCtx->fd > 0)
                    {
                        pSouthCommCtx->pcombase->disconnect(pSouthCommCtx);
                        sleep(1);
                        pSouthCommCtx->pcombase->connect(pSouthCommCtx);
                    }
                    break;
                case HAL_COM_MODE_TCP_SERVER:
                    // 该桥接端点为tcp server模式，需要无需处理，只管等待新链接即可
                    break;
                case HAL_COM_MODE_TCP_CLIENT:
                    // 该桥接端点为tcp client模式，需要处理异常重连情况
                    // 考虑到可能4G的流量问题， tcpclient 重连使用窗口
                    pSouthCommCtx->pcombase->disconnect(pSouthCommCtx);
                    sleep(window_tbl[window]);
                    ret = pSouthCommCtx->pcombase->connect(pSouthCommCtx);
                    if(ret < 0)
                    {
                        // 超时时间递增, 达到最大后，保持最大值
                        window++;
                        if(window >= sizeof(window_tbl) / sizeof(window_tbl[0]))
                        {
                            window = sizeof(window_tbl) / sizeof(window_tbl[0]) - 1;
                        }
                    }
                    else 
                    {
                        // 连接成功，重置超时时间
                        window = 0;
                    }
                    break;
                case HAL_COM_MODE_UDP_SERVER:
                    // UDP 无需管理重连
                    break;
                case HAL_COM_MODE_UDP_CLIENT:
                    // UDP 无需管理重连
                    break;
                default:
                    break;
            }
            
        }
    }
    pSouthCommCtx->pcombase->disconnect(pSouthCommCtx);

    return NULL;
}





/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
int iobridge_init(struct iobridge_ctx_t *piobridge)
{
    pthread_t thread_id_north2south;
    pthread_t thread_id_south2north;

    pthread_create(&thread_id_north2south, NULL, iobridge_north2south_thread, (void *)piobridge);
    pthread_create(&thread_id_south2north, NULL, iobridge_south2north_thread, (void *)piobridge);

    return 0;
}

