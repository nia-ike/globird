/*
* Copyright (c) 2013,珠海一构物联有限公司
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

#include <linux/serial.h>
#include <sys/ioctl.h>

#include "com_baseclass.h"



/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int hal_tcpclient_init(struct hal_comm_ctx_t *pCtx)
{
    char                    *p;

    p = strstr(pCtx->dev, ":");
    if(!p)
    {
        printf("ip:port format error:%s\n", pCtx->dev);
        return -1;
    }

    /* 提取 tcp PORT */
    pCtx->port = atoi(p+1);
    /* pCtx->dev 只要IP部分 */
    *p = '\0';

    return 0;
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
//static int hal_tcpclient_comset(struct hal_comm_ctx_t *pCtx)
//{
//    return 0;
//}



/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int hal_tcpclient_connect(struct hal_comm_ctx_t *pCtx)
{
    int                     rc;
    int32_t                 option;
    struct sockaddr_in      addr;

    fd_set                  readset;
    fd_set                  writeset;
    struct timeval          tv;

    pCtx->fd = socket(PF_INET, SOCK_STREAM, 0);
    if(pCtx->fd < 0)
        return -1;

    /* set the tco no delay flag */
    /* SOL_TCP = IPPROTO_TCP */
    option = 1;
    rc = setsockopt(pCtx->fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&option, sizeof(int));
    if (rc == -1) {
        close(pCtx->fd);
        pCtx->fd = -1;
        return -1;
    }

    /* set block    0-block   1-noblock */
    option = fcntl(pCtx->fd, F_GETFL, 0);
    fcntl(pCtx->fd, F_SETFL, option | O_NONBLOCK);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(pCtx->port);
    addr.sin_addr.s_addr = inet_addr(pCtx->dev);

    rc = connect(pCtx->fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

    FD_ZERO(&writeset);
    FD_SET(pCtx->fd, &writeset);
    FD_ZERO(&readset);
    FD_SET(pCtx->fd, &readset);

    tv.tv_sec   = 5;
    tv.tv_usec  = 0;

    rc = select(pCtx->fd+1, &readset, &writeset, 0, &tv);
    if(rc > 0)
    {
        if(FD_ISSET(pCtx->fd, &readset))
        {
            close(pCtx->fd);
            pCtx->fd = -1;
            return -1;
        }
        else
        {
            /* open keepalive */
            option = 1;
            setsockopt(pCtx->fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&option, sizeof (option));
            setsockopt(pCtx->fd, IPPROTO_TCP, TCP_KEEPIDLE, (void *)&pCtx->keepidle, sizeof(pCtx->keepidle));
            setsockopt(pCtx->fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&pCtx->keepintvl, sizeof(pCtx->keepintvl));
            setsockopt(pCtx->fd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&pCtx->keepcnt, sizeof(pCtx->keepcnt));
        }
    }

    else {
        close(pCtx->fd);
        pCtx->fd = -1;
        return -1;
    }

    return 0;
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void hal_tcpclient_disconnect(struct hal_comm_ctx_t *pCtx)
{
    if(pCtx->fd < 0)
        return;

    close(pCtx->fd);
    pCtx->fd = -1;
}



/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int hal_tcpclient_read(struct hal_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes, int timeout)
{
     int rc;
    fd_set rd;
    struct timeval tv ;
    int32_t iRead = 0;

	if (-1 == pCtx->fd)
    {
        usleep(timeout * 1000);
        return -1 ;
    }

    /* Packet */
    // 等待时间
    tv.tv_sec   = timeout / 1000;
    tv.tv_usec  = timeout % 1000 * 1000;

    FD_ZERO(&rd);
    FD_SET(pCtx->fd, &rd);

    rc = select(pCtx->fd+1, &rd, NULL, NULL, &tv);
    if(rc < 0)
    {
        close(pCtx->fd);
        pCtx->fd = -1;
        return -1;
    }
    else if(rc > 0)
    {
        iRead = recv(pCtx->fd, pbuf, bytes, 0);
        if(iRead <= 0)
        {
            close(pCtx->fd);
            pCtx->fd = -1;
            return iRead;
        }
    }

    if(iRead > 0)
    {
        MB_PACKET_DUMP(pCtx, "RX:", pbuf, iRead);
    }
    
    return iRead;
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int hal_tcpclient_write(struct hal_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes)
{
    int rc;
    int nwrite = -1;
    fd_set wr;
    struct timeval tv ;

    if(-1 == pCtx->fd)
        return -1;

    FD_ZERO(&wr);
    FD_SET(pCtx->fd, &wr);

    tv.tv_sec   = 5;
    tv.tv_usec  = 0;
    rc = select(pCtx->fd+1, NULL, &wr, NULL, &tv);
    if(rc > 0)
    {
        MB_PACKET_DUMP(pCtx, "TX:", pbuf, bytes);
        nwrite = send(pCtx->fd, pbuf, bytes, 0);
        if(nwrite <= 0)
        {
            close(pCtx->fd);
            pCtx->fd = -1;
            return -1;
        }
    }
    else
    {
        return -1;
    }

    return nwrite;
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int hal_tcpclient_Flush(struct hal_comm_ctx_t *pCtx)
{
    return 0;
}




static struct hal_com_baseclass_ops_t tcpclient_ops = {
    .init = hal_tcpclient_init,
    .connect = hal_tcpclient_connect,
    .disconnect = hal_tcpclient_disconnect,
    .read = hal_tcpclient_read,
    .write = hal_tcpclient_write,
    .flush = hal_tcpclient_Flush
};

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
struct hal_com_baseclass_ops_t *hal_com_tcpclient_ops_get(void)
{
    return &tcpclient_ops;
}