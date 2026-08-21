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

#include <linux/serial.h>
#include <sys/ioctl.h>
#include "list.h"

#include "mb_com_baseclass.h"



#define TCPSERVER_LINK_NUM              8

struct tcpserver_fdlist 
{
    /* 当前tcp 链接的fd */
    int fd;

    struct slist_node list; 
};


/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_tcpserver_comset(struct mb_comm_ctx_t *pCtx)
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
static __inline__ struct tcpserver_fdlist *mb_tcpserver_fdlist_head(void)
{
    static struct tcpserver_fdlist fd_head;

    return &fd_head;
}

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline__ struct tcpserver_fdlist *mb_tcpserver_fdlist_pool(void)
{
    static struct tcpserver_fdlist fd_list[TCPSERVER_LINK_NUM] = {
        {-1, NULL},
        {-1, NULL},
        {-1, NULL},
        {-1, NULL},
        {-1, NULL},
        {-1, NULL},
        {-1, NULL},
        {-1, NULL},
    };

    return &fd_list[0];
}


/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static struct tcpserver_fdlist *mb_tcpserver_fdlist_getidle(void)
{
    int i;
    struct tcpserver_fdlist *pfd_pool;

    pfd_pool = mb_tcpserver_fdlist_pool();
    for(i=0; i<TCPSERVER_LINK_NUM; i++)
    {
        if(pfd_pool[i].fd == -1)
            return &pfd_pool[i];
    }

    return NULL;
}



/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_tcpserver_fdlist_push(int fd)
{
    struct tcpserver_fdlist *pfd_head, *pfd_idle;

    pfd_head = mb_tcpserver_fdlist_head();
    pfd_idle = mb_tcpserver_fdlist_getidle();
    if(pfd_idle == NULL)
        return -1;

    pfd_idle->fd = fd;
    slist_append(&pfd_head->list, &pfd_idle->list);

    return 0;
}


/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_tcpserver_fdlist_fdset(fd_set *rd)
{
    struct tcpserver_fdlist *pfd_head, *pfd;

    pfd_head = mb_tcpserver_fdlist_head();

    slist_for_each_entry(pfd, &pfd_head->list, list)
    {
        if(pfd->fd != -1)
        {
            FD_SET(pfd->fd, rd);
        }
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
static int mb_tcpserver_fdlist_closeall(void)
{
    struct tcpserver_fdlist *pfd_head, *pfd;

    pfd_head = mb_tcpserver_fdlist_head();

    slist_for_each_entry(pfd, &pfd_head->list, list)
    {
        if(pfd->fd != -1)
        {
            close(pfd->fd);
        }
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
static int mb_tcpserver_fdlist_removeall(void)
{
    struct tcpserver_fdlist *pfd_head, *pfd;

    pfd_head = mb_tcpserver_fdlist_head();

    slist_for_each_entry(pfd, &pfd_head->list, list)
    {
        if(pfd->fd != -1)
        {
            pfd->fd = -1;
            slist_remove(&pfd_head->list, &pfd->list);  
            
        }
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
static int mb_tcpserver_fdlist_remove(int fd)
{
    struct tcpserver_fdlist *pfd_head, *pfd;

    pfd_head = mb_tcpserver_fdlist_head();

    slist_for_each_entry(pfd, &pfd_head->list, list)
    {
        if(pfd->fd == fd)
        {
            pfd->fd = -1;
            slist_remove(&pfd_head->list, &pfd->list);
            return 0;
        }
    }

    return -1;
}


/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_tcpserver_fdlist_remove_oldest(void)
{
    int fd;
    struct tcpserver_fdlist *pfd_head, *pfd;

    pfd_head = mb_tcpserver_fdlist_head();

    slist_for_each_entry(pfd, &pfd_head->list, list)
    {
        if(pfd->fd != -1)
        {
            fd = pfd->fd;
            pfd->fd = -1;
            slist_remove(&pfd_head->list, &pfd->list);
            return fd;
        }
    }

    return -1;
}


/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_tcpserver_connect(struct mb_comm_ctx_t *pCtx)
{
    int                     rc;
    int32_t                 option;
    struct sockaddr_in      addr;

    /* 主socket未关闭，此处不重新获取 */
    if(pCtx->fd != -1)
        return 0;

    pCtx->fd = socket(PF_INET, SOCK_STREAM, 0);
    if(pCtx->fd < 0)
        return -1;

    pCtx->fd_max = pCtx->fd;
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

        /* 允许地址复用() */
    option = 1;
    setsockopt(pCtx->fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(pCtx->port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    /* 绑定 */
    rc = bind(pCtx->fd, (const struct sockaddr *)&addr, sizeof(addr));
    /* 监听 */
    listen(pCtx->fd, 3);
   
    return 0;
}


/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void mb_tcpserver_disconnect(struct mb_comm_ctx_t *pCtx)
{
    uint32_t fd_active_cnt = 0;
    struct tcpserver_fdlist *pfd_head, *pfd;

    /* 遍历所有连接（已在read函数中对应关闭），如果所有连接都关闭，才能关闭socket */
    pfd_head = mb_tcpserver_fdlist_head();
    slist_for_each_entry(pfd, &pfd_head->list, list)
    {
        if(pfd->fd != -1)
        {
            fd_active_cnt++;
        }
    }
    /* 全部关闭 */
    if(fd_active_cnt == 0)
    {
        if(pCtx->fd != -1)
        {
            close(pCtx->fd);
            pCtx->fd = -1;
        }

        pCtx->fd_max = -1;
    }
}


/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_tcpserver_fdmax_update(struct mb_comm_ctx_t *pCtx)
{
    int fd_max = -1;
    struct tcpserver_fdlist *pfd_head, *pfd;

    /* 遍历所有accept连接（已在read函数中对应关闭），如果所有连接都关闭，才能关闭socket */
    pfd_head = mb_tcpserver_fdlist_head();
    if(!slist_isempty(&pfd_head->list))
    {
        slist_for_each_entry(pfd, &pfd_head->list, list)
        {
            fd_max = (fd_max < pfd->fd) ? pfd->fd : fd_max;
        }
    }    

    /* socket fd 加入max */
    fd_max = (fd_max < pCtx->fd) ? pfd->fd : fd_max;
    pCtx->fd_max = fd_max;

    return fd_max;
}


/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_tcpserver_read(struct mb_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes, int timeout)
{
    int rc;
    int fd_listen = -1;;
    fd_set rd;
    struct timeval tv ;
    int32_t iRead = 0;
    struct sockaddr_in client_sockaddr;                 /* 客户端socket 地址 */
    socklen_t client_addr_len = sizeof(client_sockaddr);

	if (-1 == pCtx->fd)
		return -1 ;

    /* Packet */
    // 等待时间
    tv.tv_sec   = timeout / 1000;
    tv.tv_usec  = timeout % 1000 * 1000;

    FD_ZERO(&rd);
    FD_SET(pCtx->fd, &rd);
    /* 监听所有连接 */
    mb_tcpserver_fdlist_fdset(&rd);
    rc = select(pCtx->fd_max+1, &rd, NULL, NULL, &tv);
    
    if(rc < 0)
    {
        /* 关闭所有连接 */
        mb_tcpserver_fdlist_closeall();
        /* 清空连接链表 */
        mb_tcpserver_fdlist_removeall();
        close(pCtx->fd);
        pCtx->fd_listen = -1;
        pCtx->fd = -1;
        pCtx->fd_max = -1;
        return -1;
    }
    else if(rc > 0)
    {
        // accept。 新连接请求
        if(FD_ISSET(pCtx->fd, &rd))
        {
            fd_listen = accept(pCtx->fd, (struct sockaddr *)&client_sockaddr, &client_addr_len);
            if(fd_listen >= 0)
            {
                
                /* 插入新连接 */
                rc = mb_tcpserver_fdlist_push(fd_listen);
                if(rc < 0)
                {
                    /* 满链接，需要断掉最旧连接 */
                    rc = mb_tcpserver_fdlist_remove_oldest();
                    if(rc != -1)
                        close(rc);
                    
                    /* 插入新连接 */
                    rc = mb_tcpserver_fdlist_push(fd_listen);
                }

                pCtx->fd_listen = fd_listen;
                mb_tcpserver_fdmax_update(pCtx);
                iRead = 1;
            }
        }
        else 
        {
            struct tcpserver_fdlist *pfd_head, *pfd;
            /*
                遍历找到第一个有报文链接，读取该链接后退出函数！！！！。
                不在遍历时读取所有链接，是因为本程序架构是 读写模块分离接口提供给外面调用。因此这里只管读取， 不涉及协议解析和写。

                但由于tcpip协议栈会缓存链接数据，因此在下个循环read时，会读第2个链接报文数据，因此不会导致数据丢失
            */
            pfd_head = mb_tcpserver_fdlist_head();
            slist_for_each_entry(pfd, &pfd_head->list, list)
            {
                if(pfd->fd != -1)
                {
                    if(FD_ISSET(pfd->fd, &rd))
                    {
                        
                        iRead = recv(pfd->fd, pbuf, bytes, 0);
                        if(iRead <= 0)
                        {
                            close(pfd->fd);
                            mb_tcpserver_fdlist_remove(pfd->fd);
                            mb_tcpserver_fdmax_update(pCtx);
                            pfd->fd = -1;
                            return -1;
                        }

                        /* 保存 当前处理的链接。用于本次问答处理（write）的socket */
                        pCtx->fd_listen = pfd->fd;
                        MB_PACKET_DUMP(pCtx, "RX:", pbuf, iRead);
                        return iRead;
                    }
                }
            }
        }
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
static int mb_tcpserver_write(struct mb_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes)
{
    int rc;
    int nwrite = -1;
    fd_set wr;
    struct timeval tv ;

    if(-1 == pCtx->fd)
        return -1;

    FD_ZERO(&wr);
    FD_SET(pCtx->fd_listen, &wr);

    tv.tv_sec   = 5;
    tv.tv_usec  = 0;
    rc = select(pCtx->fd_listen+1, NULL, &wr, NULL, &tv);
    if(rc > 0)
    {
        nwrite = send(pCtx->fd_listen, pbuf, bytes, 0);
        if(nwrite <= 0)
        {
            close(pCtx->fd_listen);
            mb_tcpserver_fdlist_remove(pCtx->fd_listen);
            mb_tcpserver_fdmax_update(pCtx);
            pCtx->fd_listen = -1;
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
static int mb_tcpserver_Flush(struct mb_comm_ctx_t *pCtx)
{
    if(-1 == pCtx->fd_listen)
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
static int mb_tcpserver_setoption(struct mb_comm_ctx_t *pCtx, int cmd)
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
static int mb_tcpserver_init(struct mb_comm_ctx_t *pCtx)
{
    return 0;
}


static struct mb_com_baseclass_ops_t tcpserver_ops = {
    .init = mb_tcpserver_init,
    .connect = mb_tcpserver_connect,
    .disconnect = mb_tcpserver_disconnect,
    .read = mb_tcpserver_read,
    .write = mb_tcpserver_write,
};

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
struct mb_com_baseclass_ops_t *mb_com_tcpserver_ops_get(void)
{
    return &tcpserver_ops;
}
