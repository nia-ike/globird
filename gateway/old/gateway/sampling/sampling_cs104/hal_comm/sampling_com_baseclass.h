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

#ifndef __SAMPLING_COM_BASECLASS_H__
#define __SAMPLING_COM_BASECLASS_H__

#include <stdint.h>


/********************************************************/
/*
   
*/
#define SAMPLING_COM_RXBUFF_SIZE 1280  // 本仪表支持的最大接收报文长度
#define SAMPLING_COM_TXBUFF_SIZE 1024  // 本仪表支持的最大发送报文长度
#define SAMPLING_COM_PACKET_MIN_LEN 7 // 接收到的MODBUS 报文最小长度

#define SAMPLING_COM_BROADCAST_ADDR 0xff // 广播地址
#define SAMPLING_COM_ADMIN_ADDR 0        // 超级地址




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



enum 
{
    SAMPLING_CFG_LORA_PILOT,
    SAMPLING_CFG_LORA_TRAN,
};


enum {
    SAMPLING_COM_SET_SERIAL_TERMIOS,
};


struct sampling_comm_ctx_t
{
    int      fd;    // support serial fd, and socket
    int      fd_listen; // tcp accept
    int      fd_max; // use for poll n section
    char     *dev;  // support "/dev/ttySx"  "url" "ip"
    int      dump;  // 0 - dump485 1 -lora  other not dump now

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



    /*  */

    /* commucation obj: serial, tcp client, and so on */
    struct sampling_com_baseclass_ops_t *pcombase;

    /* shard ram */
    struct db_database_type       *pvmem;

    pthread_t   process_thread_id;

    uint8_t channel_no;
    uint8_t dev_no;

};



struct sampling_com_baseclass_ops_t
{
    int  (*init)(struct sampling_comm_ctx_t *pCtx);
    int  (*connect)(struct sampling_comm_ctx_t *pCtx);
    void (*disconnect)(struct sampling_comm_ctx_t *pCtx);

    int  (*read)(struct sampling_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes, int timeout);
    int  (*write)(struct sampling_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes);

    int (*flush)(struct sampling_comm_ctx_t *pCtx);
};


struct sampling_com_baseclass_ops_t *sampling_com_serial_ops_get(void);
struct sampling_com_baseclass_ops_t *sampling_com_sx127x_ops_get(void);
struct sampling_com_baseclass_ops_t *sampling_com_lora_ops_get(void);
struct sampling_com_baseclass_ops_t *sampling_com_tcpclient_ops_get(void);
#endif
