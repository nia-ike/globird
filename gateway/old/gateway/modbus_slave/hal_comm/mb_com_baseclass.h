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

#ifndef __MB_COM_BASECLASS_H__
#define __MB_COM_BASECLASS_H__

#include <stdint.h>
#include <pthread.h>
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

/********************************************************/
/*
   通讯收发缓冲区。
   需能容纳 0x46 文件下载一帧：MBAP/ack 头 + 最大 4096 字节负载。
   （纯 4096 不够：TCP 头约 40B，故设为 4352）
*/
#define MB_COM_RXBUFF_SIZE 4352  // 本仪表支持的最大接收报文长度
#define MB_COM_TXBUFF_SIZE 4352  // 本仪表支持的最大发送报文长度
#define MB_COM_PACKET_MIN_LEN 7 // 接收到的MODBUS 报文最小长度

#define MB_COM_BROADCAST_ADDR 0xff // 广播地址
#define MB_COM_ADMIN_ADDR 0        // 超级地址

#define CFG_FILE_ROOT_CHANNEL                            		"/userdata/config/collect/sampling_chnl_cfg.json"
/***********************END of SET*****************************************/


#define MB_PACKET_DUMP(pCtx, dev, pbuf, bytes) do{ \
    if(pCtx->dump){ \
        printf("%s: ", dev); \
        for(int loop=0; loop<bytes; loop++) { \
            printf("%02X ", pbuf[loop]); \
        } \
        printf("\n"); \
    } \
}while(0);






enum {
    MB_COM_SET_SERIAL_TERMIOS,
};


struct mb_comm_ctx_t
{
    int      fd;    // support serial fd, and socket
    int      fd_listen; // tcp accept
    int      fd_max; // use for poll n section
    char     *dev;  // support "/dev/ttySx"  "url" "ip"
    uint32_t sn;
    int      dump;  // 0 - no dump 1 -dump
    uint8_t  mode;
    uint8_t  channel_no;
    uint8_t  dev_no;

    int      running;
    /* serial */
    uint32_t baud;
    uint8_t  databit;
    uint8_t  paritybit;
    uint8_t  stopbit;
    uint8_t  rs485;
    uint8_t  rs485_addr;
    /* tcp */
    uint16_t port;
    int keepidle;       //
    int keepintvl;      //
    int keepcnt;        //

    /*注册帧*/
    uint8_t  regPacket[24]; //注册帧内容
    /*  */
    uint8_t recvBuff[MB_COM_RXBUFF_SIZE]; //读数据缓冲区；
    uint8_t sendBuff[MB_COM_TXBUFF_SIZE]; //发送数据缓冲区
    int recvLen;                     //接收到的数据长度
    int sendLen;                     //要发送的数据长度

    /* 
        commucation obj: serial, tcp client, and so on 
        only 1 layer.  application will write and read by this communication phy object directly
    */
    struct mb_com_baseclass_ops_t *pcombase;


    /* protocol. modbusRTU, modbusTCP */
    int (*msg_protocol_process)(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen);

    /* 
        实时数据表
     */
    struct db_spl_realtime_data_tbl_t *pvmem;


    pthread_t   process_thread_id;

    struct sockaddr_in cliaddr;
};



struct mb_com_baseclass_ops_t
{
    int  (*init)(struct mb_comm_ctx_t *pCtx);
    int  (*connect)(struct mb_comm_ctx_t *pCtx);
    void (*disconnect)(struct mb_comm_ctx_t *pCtx);

    int  (*read)(struct mb_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes, int timeout);
    int  (*write)(struct mb_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes);
    int (*flush)(struct mb_comm_ctx_t *pCtx);
};


typedef struct mb_com_baseclass_ops_t *(*mb_com_ops_get)(void);


struct mb_com_baseclass_ops_t *mb_com_serial_ops_get(void);
struct mb_com_baseclass_ops_t *mb_com_tcpclient_ops_get(void);
struct mb_com_baseclass_ops_t *mb_com_stdinout_ops_get(void);
struct mb_com_baseclass_ops_t *mb_com_tcpserver_ops_get(void);
struct mb_com_baseclass_ops_t *mb_com_udpboardcast_ops_get(void);
#endif
