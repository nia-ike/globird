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

#ifndef __MB_COM_BASECLASS_H__
#define __MB_COM_BASECLASS_H__

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


struct mb_optarg_t
{
    uint8_t  id;
    uint8_t  checked;
    uint32_t sn;
	uint32_t center_frequency;
	uint8_t	 lora_pktmode;
    uint8_t  signal_chnl;
    uint8_t  coding_rate;
    uint8_t  spreading_factor;
    uint8_t  band_width;
    uint8_t  LowDataRateOptimize;
    uint16_t preamble_size;
};






enum 
{
    MB_CFG_LED_RS485,
    MB_CFG_LED_LORA,
    MB_CFG_LED_NONE = 0xff,
};


enum 
{
    SAMPLING_CFG_LORA_PILOT,
    SAMPLING_CFG_LORA_TRAN,
};

enum {
    MB_COM_SET_SERIAL_TERMIOS,
};


struct mb_comm_ctx_t
{
    int      fd;    // support serial fd, and socket
    int      fd_listen; // tcp accept
    int      fd_max; // use for poll n section
    char     *dev;  // support "/dev/ttySx"  "url" "ip"
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
    /* lora */
    uint32_t time_pause;
    uint32_t sn;
    uint8_t  signal_chnl;
    uint8_t  spreading_factor;
    uint8_t  coding_rate;
    uint8_t  band_width;          /* 0-9   7.8K - 500K*/
    uint8_t  lora_pktmode;
    uint32_t center_frequency;
    uint8_t  LowDataRateOptimize;
    uint16_t preamble_size;

    /* led */
    uint8_t   led_blink;

    /* pilot lora frame */
    uint8_t  seq;
    int      fd_lora_pipe[2]; // use for lora unpack data.  lora thread read() -> user read()


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

    /*
        2 layer .
        sometime, some communication phy will base on another communication phy

        for example:
            mb_lora  will call mb_sx127x
        
    */
    struct mb_com_baseclass_ops_t *pcomChildbase;

    /* protocol. modbusRTU, modbusTCP */
    int (*msg_protocol_process)(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen);

    /* shard ram */
    struct db_database_type       *pvmem;
    struct db_spl_realtime_data_tbl_t *pCache;

    /**/
    uint32_t last_collect_num;
    
    pthread_t   process_thread_id;
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


struct mb_com_baseclass_ops_t *mb_com_serial_ops_get(void);
struct mb_com_baseclass_ops_t *mb_com_tcpclient_ops_get(void);
struct mb_com_baseclass_ops_t *mb_com_stdinout_ops_get(void);
struct mb_com_baseclass_ops_t *mb_com_tcpserver_ops_get(void);
struct mb_com_baseclass_ops_t *mb_com_sx127x_ops_get(void);
struct mb_com_baseclass_ops_t *mb_com_lora_ops_get(void);
#endif
