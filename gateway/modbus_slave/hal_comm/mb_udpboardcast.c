#define _GNU_SOURCE
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
#include <net/if.h>

#include <linux/serial.h>
#include <sys/ioctl.h>

#include "mb_com_baseclass.h"




/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_udpboardcast_comset(struct mb_comm_ctx_t *pCtx)
{


    return 0;
}




/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void mb_udpboardcast_close(struct mb_comm_ctx_t *pCtx)
{
    if(pCtx->fd  < 0)
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
static int mb_udpboardcast_connect(struct mb_comm_ctx_t *pCtx)
{
#define ETH_BOARDCAST_DEV "eth0"
    struct ifreq ifr;

    // 1、创建UDP套接字socket
    pCtx->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(pCtx->fd < 0)
        return -1;

    // 2、准备服务端ip和端口
    struct sockaddr_in servaddr;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons (pCtx->port);

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ETH_BOARDCAST_DEV, IFNAMSIZ-1);
    
    // 获取网卡地址
    if (ioctl(pCtx->fd, SIOCGIFADDR, &ifr) < 0) {
        close(pCtx->fd);
        pCtx->fd = -1;
        return -1;
    }
    servaddr.sin_addr.s_addr = INADDR_ANY;

    // 3、绑定 bind
	if (bind(pCtx->fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        close(pCtx->fd);
        pCtx->fd = -1;
        return -1;
    }

    int so_broadcast = 1;
	if(setsockopt(pCtx->fd, SOL_SOCKET, SO_BROADCAST, &so_broadcast,sizeof(so_broadcast)) < 0)
	{
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
static void mb_udpboardcast_disconnect(struct mb_comm_ctx_t *pCtx)
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
static int mb_udpboardcast_read(struct mb_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes, int timeout)
{
    int rc;
    fd_set rd;
    struct timeval tv ;
    int32_t iRead = 0;
    

	if (-1 == pCtx->fd)
		return -1 ;

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
        bzero(&pCtx->cliaddr, sizeof(pCtx->cliaddr));
		socklen_t addrLen = sizeof(pCtx->cliaddr);

        iRead = recvfrom(pCtx->fd, pbuf, bytes, 0, (struct sockaddr*)&pCtx->cliaddr, &addrLen);

        if(iRead <= 0)
        {
            close(pCtx->fd);
            pCtx->fd = -1;
            return iRead;
        }
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
static int mb_udpboardcast_write(struct mb_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes)
{
#define ETH_BOARDCAST_DEV "eth0"

    int rc;
    int nwrite = -1;
    fd_set wr;
    struct timeval tv ;
    struct ifreq ifr;

    if(-1 == pCtx->fd)
        return -1;

    FD_ZERO(&wr);
    FD_SET(pCtx->fd, &wr);

    tv.tv_sec   = 5;
    tv.tv_usec  = 0;
    rc = select(pCtx->fd+1, NULL, &wr, NULL, &tv);
    if(rc > 0)
    {
        // 指定网口才能进行广播
        // j解决在有4G网卡时，默认路由问题，导致广播的数据没有在eth0出去导致搜索设备搜不到
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, ETH_BOARDCAST_DEV, IFNAMSIZ-1);
        
        // 绑定网卡发送
        if (setsockopt(pCtx->fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
            close(pCtx->fd);
            pCtx->fd = -1;
            return -1;
        }

        socklen_t addrLen = sizeof(pCtx->cliaddr);
        pCtx->cliaddr.sin_addr.s_addr = 0xffffffff;

        nwrite = sendto(pCtx->fd, pbuf, bytes, 0, (struct sockaddr*)&pCtx->cliaddr, addrLen);
        
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

    MB_PACKET_DUMP(pCtx, "TX:", pbuf, bytes);
    return nwrite;
}

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_udpboardcast_Flush(struct mb_comm_ctx_t *pCtx)
{
    if(-1 == pCtx->fd)
        return -1;

    return 0;
}

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_udpboardcast_setoption(struct mb_comm_ctx_t *pCtx, int cmd)
{
    return 0;
}


/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_udpboardcast_init(struct mb_comm_ctx_t *pCtx)
{
    return 0;
}


static struct mb_com_baseclass_ops_t udpbrardcast_ops = {
    .init = mb_udpboardcast_init,
    .connect = mb_udpboardcast_connect,
    .disconnect = mb_udpboardcast_disconnect,
    .read = mb_udpboardcast_read,
    .write = mb_udpboardcast_write,
};

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
struct mb_com_baseclass_ops_t *mb_com_udpboardcast_ops_get(void)
{
    return &udpbrardcast_ops;
}
