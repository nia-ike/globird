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
#include <errno.h>
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

#include "hal_com_baseclass.h"
#include "dev_log.h"


#define TCPSERVER_LINK_NUM              16




/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int hal_tcpserver_comset(struct hal_comm_ctx_t *pCtx)
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
static __inline__ struct tcpserver_fdlist *hal_tcpserver_fdlist_head(struct hal_comm_ctx_t *pCtx)
{
    return &pCtx->fdlist_head;
}

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static __inline__ struct tcpserver_fdlist *hal_tcpserver_fdlist_pool(struct hal_comm_ctx_t *pCtx)
{
    return pCtx->pfdlist_pool;
}


/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static struct tcpserver_fdlist *hal_tcpserver_fdlist_getidle(struct hal_comm_ctx_t *pCtx)
{
    int i;
    struct tcpserver_fdlist *pfd_pool;

    pfd_pool = hal_tcpserver_fdlist_pool(pCtx);
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
static int hal_tcpserver_fdlist_push(struct hal_comm_ctx_t *pCtx, int fd)
{
    struct tcpserver_fdlist *pfd_head, *pfd_idle;

    pfd_head = hal_tcpserver_fdlist_head(pCtx);
    pfd_idle = hal_tcpserver_fdlist_getidle(pCtx);
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
static int hal_tcpserver_fdlist_fdset(struct hal_comm_ctx_t *pCtx, fd_set *rd)
{
    struct tcpserver_fdlist *pfd_head, *pfd;

    pfd_head = hal_tcpserver_fdlist_head(pCtx);
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
static int hal_tcpserver_fdlist_closeall(struct hal_comm_ctx_t *pCtx)
{
    struct tcpserver_fdlist *pfd_head, *pfd;

    pfd_head = hal_tcpserver_fdlist_head(pCtx);

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
static int hal_tcpserver_fdlist_removeall(struct hal_comm_ctx_t *pCtx)
{
    struct tcpserver_fdlist *pfd_head, *pfd;

    pfd_head = hal_tcpserver_fdlist_head(pCtx);

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
static int hal_tcpserver_fdlist_remove(struct hal_comm_ctx_t *pCtx, int fd)
{
    struct tcpserver_fdlist *pfd_head, *pfd;

    pfd_head = hal_tcpserver_fdlist_head(pCtx);

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
static int hal_tcpserver_fdlist_remove_oldest(struct hal_comm_ctx_t *pCtx)
{
    int fd;
    struct tcpserver_fdlist *pfd_head, *pfd;

    pfd_head = hal_tcpserver_fdlist_head(pCtx);

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
static int hal_tcpserver_connect(struct hal_comm_ctx_t *pCtx)
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
    addr.sin_port = htons(pCtx->local_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    /* 绑定 */
    rc = bind(pCtx->fd, (const struct sockaddr *)&addr, sizeof(addr));
    /* 监听，backlog 加大以吸收重连风暴，避免队列瞬间被打满后丢弃 SYN */
    listen(pCtx->fd, 64);

    log_write_string(pCtx->channel_no, 0, LOG_LEVEL_INFO, "tcpserver connect success\n");

    return 0;
}


/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void hal_tcpserver_disconnect(struct hal_comm_ctx_t *pCtx)
{
    uint32_t fd_active_cnt = 0;
    struct tcpserver_fdlist *pfd_head, *pfd;

    /* 遍历所有连接（已在read函数中对应关闭），如果所有连接都关闭，才能关闭socket */
    pfd_head = hal_tcpserver_fdlist_head(pCtx);
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

        log_write_string(pCtx->channel_no, 0, LOG_LEVEL_INFO, "tcpserver disconnect\n");
    }
}


/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int hal_tcpserver_fdmax_update(struct hal_comm_ctx_t *pCtx)
{
    int fd_max = -1;
    struct tcpserver_fdlist *pfd_head, *pfd;

    /* 遍历所有accept连接（已在read函数中对应关闭），如果所有连接都关闭，才能关闭socket */
    pfd_head = hal_tcpserver_fdlist_head(pCtx);
    if(!slist_isempty(&pfd_head->list))
    {
        slist_for_each_entry(pfd, &pfd_head->list, list)
        {
            fd_max = (fd_max < pfd->fd) ? pfd->fd : fd_max;
        }
    }    

    /* socket fd 加入max */
    fd_max = (fd_max < pCtx->fd) ? pCtx->fd : fd_max;
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
static int hal_tcpserver_read(struct hal_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes, int timeout)
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
    hal_tcpserver_fdlist_fdset(pCtx, &rd);

    rc = select(pCtx->fd_max+1, &rd, NULL, NULL, &tv);
    if(rc < 0)
    {
        /* 关闭所有连接 */
        hal_tcpserver_fdlist_closeall(pCtx);
        /* 清空连接链表 */
        hal_tcpserver_fdlist_removeall(pCtx);
        close(pCtx->fd);
        pCtx->fd_listen = -1;
        pCtx->fd = -1;
        pCtx->fd_max = -1;
        return -1;
    }
    else if(rc > 0)
    {
        /* 一次把监听队列取空。新连接没有应用数据，不能把返回值当成要转发的字节 */
        if(FD_ISSET(pCtx->fd, &rd))
        {
            for(;;)
            {
                int32_t option = 1;
                int     flags;

                client_addr_len = sizeof(client_sockaddr);
                fd_listen = accept(pCtx->fd, (struct sockaddr *)&client_sockaddr, &client_addr_len);
                if(fd_listen < 0)
                {
                    if((errno == EINTR) || (errno == ECONNABORTED))
                        continue;
                    break;
                }

                /* 业务连接设为非阻塞，单条连接的收发不能堵死本线程 */
                flags = fcntl(fd_listen, F_GETFL, 0);
                if(flags >= 0)
                    fcntl(fd_listen, F_SETFL, flags | O_NONBLOCK);

                /* 远端异常断开时内核可探测并回收失效连接，避免占满 TCPSERVER_LINK_NUM */
                setsockopt(fd_listen, SOL_SOCKET, SO_KEEPALIVE, (void *)&option, sizeof(option));
                setsockopt(fd_listen, IPPROTO_TCP, TCP_KEEPIDLE, (void *)&pCtx->keepidle, sizeof(pCtx->keepidle));
                setsockopt(fd_listen, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&pCtx->keepintvl, sizeof(pCtx->keepintvl));
                setsockopt(fd_listen, IPPROTO_TCP, TCP_KEEPCNT, (void *)&pCtx->keepcnt, sizeof(pCtx->keepcnt));

                /* 插入新连接 */
                rc = hal_tcpserver_fdlist_push(pCtx, fd_listen);
                if(rc < 0)
                {
                    /* 满链接，需要断掉最旧连接 */
                    rc = hal_tcpserver_fdlist_remove_oldest(pCtx);
                    if(rc != -1)
                        close(rc);

                    /* 插入新连接 */
                    rc = hal_tcpserver_fdlist_push(pCtx, fd_listen);
                }

                pCtx->fd_listen = fd_listen;
                hal_tcpserver_fdmax_update(pCtx);
            }
        }

        /* accept 与收包在同一轮内都要处理，两者不能互斥 */
        {
            struct tcpserver_fdlist *pfd_head, *pfd;
            /*
                遍历找到第一个有报文链接，读取该链接后退出函数。
                不在遍历时读取所有链接，是因为本程序架构是 读写模块分离接口提供给外面调用。因此这里只管读取， 不涉及协议解析和写。

                但由于tcpip协议栈会缓存链接数据，因此在下个循环read时，会读第2个链接报文数据，因此不会导致数据丢失
            */
            pfd_head = hal_tcpserver_fdlist_head(pCtx);

            slist_for_each_entry(pfd, &pfd_head->list, list)
            {
                if(pfd->fd != -1)
                {
                    if(FD_ISSET(pfd->fd, &rd))
                    {
                        iRead = recv(pfd->fd, pbuf, bytes, 0);
                        if(iRead < 0)
                        {
                            if((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
                                continue;

                            close(pfd->fd);
                            hal_tcpserver_fdlist_remove(pCtx, pfd->fd);
                            hal_tcpserver_fdmax_update(pCtx);
                            pfd->fd = -1;
                            /* 返回 0：调用方对 TCP Server 不会拆监听，也不会把空缓冲转发出去 */
                            return 0;
                        }
                        else if(iRead == 0)
                        {
                            close(pfd->fd);
                            hal_tcpserver_fdlist_remove(pCtx, pfd->fd);
                            hal_tcpserver_fdmax_update(pCtx);
                            pfd->fd = -1;
                            return 0;
                        }

                        /* 保存 当前处理的链接。用于本次问答处理（write）的socket */
                        pCtx->fd_listen = pfd->fd;
                        log_write(pCtx->channel_no, 0, LOG_RX, pbuf, iRead);
                        MB_PACKET_DUMP(pCtx, "tcps-RX:", pbuf, iRead);
                        return iRead;
                    }
                }
            }
        }
    }

    return iRead;
}

#if 0
/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int hal_tcpserver_write(struct hal_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes)
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
            hal_tcpserver_fdlist_remove(pCtx, pCtx->fd_listen);

            hal_tcpserver_fdmax_update(pCtx);
            pCtx->fd_listen = -1;
            return -1;
        }
    }
    else
    {
        return -1;
    }
    MB_PACKET_DUMP(pCtx, "tcps-TX:", pbuf, bytes);
    return nwrite;
}

#else 
/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int hal_tcpserver_send_all(int fd, uint8_t *pbuf, uint32_t bytes)
{
    uint32_t total_write = 0;

    while(total_write < bytes)
    {
        int rc;
        int nwrite;
        fd_set wr;
        struct timeval tv;

        FD_ZERO(&wr);
        FD_SET(fd, &wr);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        rc = select(fd + 1, NULL, &wr, NULL, &tv);
        if(rc <= 0)
            return -1;

        nwrite = send(fd, pbuf + total_write, bytes - total_write, 0);
        if(nwrite < 0)
        {
            if((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
                continue;
            return -1;
        }
        if(nwrite == 0)
            return -1;

        total_write += (uint32_t)nwrite;
    }

    return (int)total_write;
}

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int hal_tcpserver_write(struct hal_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes)
{
    int sent_any = 0;
    struct tcpserver_fdlist *pfd_head, *pfd;

    if(-1 == pCtx->fd)
        return -1;

    pfd_head = hal_tcpserver_fdlist_head(pCtx);

    /*
        往所有连接发数据。连接已是非阻塞，必须发完；一条慢连接失败只关掉它自己。
    */
    slist_for_each_entry(pfd, &pfd_head->list, list)
    {
        int fd;

        if(pfd->fd == -1)
            continue;

        fd = pfd->fd;
        if(hal_tcpserver_send_all(fd, pbuf, bytes) < 0)
        {
            close(fd);
            hal_tcpserver_fdlist_remove(pCtx, fd);
            hal_tcpserver_fdmax_update(pCtx);
            pfd->fd = -1;
            if(pCtx->fd_listen == fd)
                pCtx->fd_listen = -1;
        }
        else
        {
            sent_any = 1;
        }
    }

    log_write(pCtx->channel_no, 0, LOG_TX, pbuf, bytes);
    MB_PACKET_DUMP(pCtx, "tcps-TX:", pbuf, bytes);
    return sent_any ? (int)bytes : -1;
}

#endif

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int hal_tcpserver_Flush(struct hal_comm_ctx_t *pCtx)
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
static int hal_tcpserver_setoption(struct hal_comm_ctx_t *pCtx, int cmd)
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
static int hal_tcpserver_init(struct hal_comm_ctx_t *pCtx)
{
    // tcp 连接链表头
    pCtx->fdlist_head.fd = -1;
    pCtx->fdlist_head.list.next = NULL;

    // tcp 连接链表池。这里设定最大数量
    pCtx->pfdlist_pool = malloc(sizeof(struct tcpserver_fdlist) * TCPSERVER_LINK_NUM);
    if(NULL == pCtx->pfdlist_pool)
    {
        printf("malloc tcpserver_fdlist_pool failed\n");
        return -1;
    }

    for(int i=0; i<TCPSERVER_LINK_NUM; i++)
    {
        pCtx->pfdlist_pool[i].fd = -1;
        pCtx->pfdlist_pool[i].list.next = NULL;
    }

    return 0;
}


static struct hal_com_baseclass_ops_t tcpserver_ops = {
    .init = hal_tcpserver_init,
    .connect = hal_tcpserver_connect,
    .disconnect = hal_tcpserver_disconnect,
    .read = hal_tcpserver_read,
    .write = hal_tcpserver_write,
};

/*********************************************************************************************************
** Function name:
** Descriptions:
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
struct hal_com_baseclass_ops_t *hal_com_tcpserver_ops_get(void)
{
    return &tcpserver_ops;
}
