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

#ifndef __OPC_UA_BASECLASS_H__
#define __OPC_UA_BASECLASS_H__

#include <stdint.h>
#include <pthread.h>




/********************************************************/
/*
   为了节省内存。当前SPM90的寄存器数量较少，因此定义缓冲区长度稍短。
   在仪表寄存器寄存器较长时，请调整该长度。最大260.
*/
#define MB_COM_RXBUFF_SIZE 1280  // 本仪表支持的最大接收报文长度
#define MB_COM_TXBUFF_SIZE 1280  // 本仪表支持的最大发送报文长度
#define MB_COM_PACKET_MIN_LEN 7 // 接收到的MODBUS 报文最小长度

#define MB_COM_BROADCAST_ADDR 0xff // 广播地址
#define MB_COM_ADMIN_ADDR 0        // 超级地址

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


struct opc_ua_comm_ctx_t
{
    int      fd;    // support serial fd, and socket
    int      fd_listen; // tcp accept
    int      fd_max; // use for poll n section
    char     *dev;  // support "/dev/ttySx"  "url" "ip"
    uint32_t sn;
    int      dump;  // 0 - no dump 1 -dump
    uint8_t  mode;

    int      running;
    /* serial */
    uint32_t baud;
    uint8_t  databit;
    uint8_t  paritybit;
    uint8_t  stopbit;
    uint8_t  rs485;
    /* tcp */
    uint16_t port;
    int keepidle;       //
    int keepintvl;      //
    int keepcnt;        //


    /* 
        commucation obj: serial, tcp client, and so on 
        only 1 layer.  application will write and read by this communication phy object directly
    */
    struct opc_ua_com_baseclass_ops_t *pcombase;


    /* protocol. modbusRTU, modbusTCP */
    int (*msg_protocol_process)(struct opc_ua_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen);

    /* 
        实时数据表
     */
    struct db_spl_realtime_data_tbl_t *pvmem;


    pthread_t   process_thread_id;

};


#endif
