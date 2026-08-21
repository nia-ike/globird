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

#ifndef __HAL_COM_BASECLASS_H__
#define __HAL_COM_BASECLASS_H__

#include <stdint.h>
#include <pthread.h>
#include "list.h"


/********************************************************/
/*
   
*/
#define HAL_COM_RXBUFF_SIZE 2048  // 本仪表支持的最大接收报文长度
#define HAL_COM_TXBUFF_SIZE 2048  // 本仪表支持的最大发送报文长度



enum {
    HAL_COM_MODE_SERIAL,
    HAL_COM_MODE_TCP_CLIENT,
    HAL_COM_MODE_TCP_SERVER,
    HAL_COM_MODE_UDP_CLIENT,
    HAL_COM_MODE_UDP_SERVER,
};


/***********************END of SET*****************************************/

#define MB_PACKET_DUMP(pCtx, dir, pbuf, bytes) do{ \
    if(pCtx->dump == 1){ \
        printf("%s: ", dir); \
        for(int loop=0; loop<bytes; loop++) { \
            printf("%02X ", pbuf[loop]); \
        } \
        printf("\n"); \
    } \
}while(0);





enum {
    HAL_COM_SET_SERIAL_TERMIOS,
};


struct tcpserver_fdlist 
{
    /* 当前tcp 链接的fd */
    int fd;

    struct slist_node list; 
};


struct hal_comm_ctx_t
{
    int      fd;    // support serial fd, and socket
    int      fd_listen; // tcp accept
    int      fd_max; // use for poll n section
    char     dev[32];  // support "/dev/ttySx"  "url" "ip"
    int      mode; // 0 - client 1 - server
    int      dump;  // 0 - dump485 1 -lora  other not dump now

    /* serial */
    uint32_t baud;
    uint8_t  databit;
    uint8_t  paritybit;
    uint8_t  stopbit;
    /* tcp */
    uint16_t local_port;
    uint16_t remote_port;

    int keepidle;       //
    int keepintvl;      //
    int keepcnt;        //

    /*  */
    uint8_t channel_no;

    /* commucation obj: serial, tcp client, and so on */
    struct hal_com_baseclass_ops_t *pcombase;


    pthread_t   process_thread_id;


    uint8_t rxbuff[HAL_COM_RXBUFF_SIZE];

    /*注册帧*/
    uint8_t  regEnable;

    uint8_t  regPacket[32]; //注册帧内容

    struct tcpserver_fdlist  fdlist_head;
    struct tcpserver_fdlist *pfdlist_pool;
};





struct hal_com_baseclass_ops_t
{
    int  (*init)(struct hal_comm_ctx_t *pCtx);
    int  (*connect)(struct hal_comm_ctx_t *pCtx);
    void (*disconnect)(struct hal_comm_ctx_t *pCtx);

    int  (*read)(struct hal_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes, int timeout);
    int  (*write)(struct hal_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes);

    int (*flush)(struct hal_comm_ctx_t *pCtx);
};


typedef struct hal_com_baseclass_ops_t *(*hal_com_ops_get)(void);


struct hal_com_baseclass_ops_t *hal_com_serial_ops_get(void);
struct hal_com_baseclass_ops_t *hal_com_tcpclient_ops_get(void);
struct hal_com_baseclass_ops_t *hal_com_tcpserver_ops_get(void);
#endif
