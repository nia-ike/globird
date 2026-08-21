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
#include <pthread.h>
#include <errno.h>

#include <linux/serial.h>
#include <sys/ioctl.h>

#include "mb_com_baseclass.h"
#include "sx127x.h"
#include "crc16.h"



/*
    本lora接口是基于 sx127x驱动接口封装成派诺lora协议的驱动

    sx127x 接口是 物理层接口，应用数据直接透传到射频
    本lora 接口把sx127x 封装PMAC121接口（兼容PMAC121），


    write 流程：  应用层数据write -> pilot lora封装 write -> sx127x物理 write

    read  流程：  sx127x 物理 read -> pilot lora 解包 read -> 应用层数据 read
                            ^           ^
                            |           |
                            --------------------使用管道通信进行报文传递
*/





#define PI_LORA_DATA            0x01
#define PI_LORA_GETLOCAL_PARAM  0x02
#define PI_LORA_SCAN            0x03
#define PI_LORA_SETLOCAL_PARAM  0x04
#define PI_LORA_PING            0x05
#define PI_LORA_PAUSE           0x07


#define MB_ALIGN_PT_16BIT_READ(p)       (*((uint8_t *)(p)) << 8 | *((uint8_t *)(p)+1)) 
#define MB_ALIGN_PT_16BIT_WRITE(p,v)    do {(*((uint8_t*)p)) = v >> 8 & 0xff; (*((uint8_t*)p+1)) = v & 0xff;}while(0)

#define SWAP_16BIT(x)                   (uint16_t)(((x) >> 8)  | ((x) << 8 ))
#define SWAP_32BIT(x)                   (uint32_t)(((x) >> 16) | ((x) << 16))


/*
    本通讯接口基于mb_hal_sx1278.c

    本接口封装多一层 派诺lora协议（兼容PMAC121）

*/
#pragma pack(push,1)

struct lora_protocol_devinfo_t
{
    union un_info
    {
        struct info_t
        {
            uint8_t reserved:1;
            uint8_t sub_network:1;
            uint8_t terminal_mode:1;
            uint8_t work_mode:1;
            uint8_t channel_no:4;
        }split;

        uint8_t mix;
    }uni;
};


struct lora_protocol_head_t
{
    uint8_t                         sof;
    uint8_t                         seq;
    uint32_t                        sn_src;
    uint32_t                        sn_dest;

    struct lora_protocol_devinfo_t  devinfo;
    uint16_t                        reserved;
    uint8_t                         func_code;
    uint8_t                         length;     /* data len */

};


struct lora_protocol_scan_03h_t
{
    uint32_t                        baud;
    uint8_t                         databit;
    uint8_t                         paritybit;
    uint8_t                         stopbit;

    uint8_t                         channel_no;
    uint8_t                         work_mode;
    uint8_t                         terminal_model;
    uint8_t                         sub_network;

    uint8_t                         switch_addr; // compatible with pmac121
    uint8_t                         switch_status; // compatible 

    uint16_t                         timeout;

    uint32_t                         rssi;
};


struct lora_protocol_ping_05h_t
{
    uint8_t                         content;
};

struct lora_protocol_pause_07h_t
{
    uint8_t                         timeout;
};


#pragma pack(pop)


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_lora_frame_is_valid(uint8_t *pbuf, uint16_t nbytes)
{
    uint16_t CalCrc, FrameCrc;
    struct lora_protocol_head_t *pLoRaHead;

    pLoRaHead = (struct lora_protocol_head_t *)pbuf;

    /* header is error? */
    if(pLoRaHead->sof != 0x7e)
        return -1;
    /* frame length is incollect? */
    if((pLoRaHead->length + sizeof(struct lora_protocol_head_t) + sizeof(CalCrc)) != nbytes)
        return -1;
    /* frame crc is right? */
    CalCrc = crc16(pbuf, pLoRaHead->length + sizeof(struct lora_protocol_head_t));
    CalCrc = SWAP_16BIT(CalCrc);
    FrameCrc = MB_ALIGN_PT_16BIT_READ(pbuf+pLoRaHead->length + sizeof(struct lora_protocol_head_t));
    if(CalCrc != FrameCrc)
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
static int mb_lora_frame_data_01H(struct mb_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t nbytes)
{
    uint8_t                     pending = 0;
    struct lora_protocol_head_t *pLoRaHead;

    pLoRaHead = (struct lora_protocol_head_t *)pbuf;
    return write(pCtx->fd_lora_pipe[1], pbuf+sizeof(struct lora_protocol_head_t)+sizeof(pending), pLoRaHead->length - sizeof(pending));

}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_lora_frame_scan_03H(struct mb_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t nbytes)
{
    int                          i;
    uint16_t                     crc = 0;
    uint16_t                     delay;
    uint8_t                      buf[64];
    struct lora_protocol_head_t  *pLoraSendHal;
    struct lora_protocol_scan_03h_t *pScan03H;

    pLoraSendHal = (struct lora_protocol_head_t *)buf;
    pScan03H = (struct lora_protocol_scan_03h_t *)(buf + sizeof(struct lora_protocol_head_t));

    pLoraSendHal->sof = 0x7e;
    pLoraSendHal->seq = pCtx->seq++;
    pLoraSendHal->sn_src = pCtx->sn;
    pLoraSendHal->sn_dest = 0xffffffff;
    pLoraSendHal->devinfo.uni.split.channel_no = 0;
    pLoraSendHal->devinfo.uni.split.work_mode = 0;
    pLoraSendHal->devinfo.uni.split.terminal_mode = 0;
    pLoraSendHal->devinfo.uni.split.sub_network = 0;
    pLoraSendHal->reserved = 0;
    pLoraSendHal->func_code = PI_LORA_SCAN | 0x80;
    pLoraSendHal->length = sizeof(struct lora_protocol_scan_03h_t);   

    pScan03H->baud = pCtx->baud;
    pScan03H->databit = pCtx->databit;
    pScan03H->paritybit = pCtx->paritybit;
    pScan03H->stopbit = pCtx->stopbit;
    pScan03H->channel_no = pCtx->signal_chnl;
    pScan03H->work_mode = 0;
    pScan03H->terminal_model = 0;
    pScan03H->sub_network = 0;
    pScan03H->switch_addr = 148; // compatible 
    pScan03H->switch_status = 0; // compatibl
    pScan03H->timeout = 2000;
    pScan03H->rssi = 0;

    crc = crc16(buf, sizeof(struct lora_protocol_head_t) + sizeof(struct lora_protocol_scan_03h_t));
    crc = SWAP_16BIT(crc);
    MB_ALIGN_PT_16BIT_WRITE(buf+sizeof(struct lora_protocol_head_t)+ sizeof(struct lora_protocol_scan_03h_t), crc);

    /*
        信道扫描是广播，需要随机延时，防信道冲突
    */
    delay = rand() % 500;
    usleep(delay * 1000);

    return pCtx->pcomChildbase->write(pCtx, buf, sizeof(struct lora_protocol_head_t)+ sizeof(struct lora_protocol_scan_03h_t) + sizeof(crc));

}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_lora_frame_ping_05H(struct mb_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t nbytes)
{
    uint16_t                     crc = 0;
    uint8_t                      buf[64];
    struct lora_protocol_head_t *pLoraSendHal;
    struct lora_protocol_ping_05h_t *pPing05H;

    pLoraSendHal = (struct lora_protocol_head_t *)buf;
    pPing05H = (struct lora_protocol_ping_05h_t *)(buf+sizeof(struct lora_protocol_head_t));

    pLoraSendHal->sof = 0x7e;
    pLoraSendHal->seq = pCtx->seq++;
    pLoraSendHal->sn_src = pCtx->sn;
    pLoraSendHal->sn_dest = 0xffffffff;
    pLoraSendHal->devinfo.uni.split.channel_no = 0;
    pLoraSendHal->devinfo.uni.split.work_mode = 0;
    pLoraSendHal->devinfo.uni.split.terminal_mode = 0;
    pLoraSendHal->devinfo.uni.split.sub_network = 0;
    pLoraSendHal->reserved = 0;
    pLoraSendHal->func_code = PI_LORA_PING | 0x80;
    pLoraSendHal->length = 1;   
    pPing05H->content = 0x55; // magic number. 

    crc = crc16(buf, sizeof(struct lora_protocol_head_t) + sizeof(struct lora_protocol_ping_05h_t));
    crc = SWAP_16BIT(crc);
    MB_ALIGN_PT_16BIT_WRITE(buf+sizeof(struct lora_protocol_head_t)+ sizeof(struct lora_protocol_ping_05h_t), crc);


    return pCtx->pcomChildbase->write(pCtx, buf, sizeof(struct lora_protocol_head_t)+ sizeof(struct lora_protocol_ping_05h_t) + sizeof(crc));

}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_lora_frame_ping_07H(struct mb_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t nbytes)
{
    uint16_t                     crc = 0;
    uint8_t                      buf[64];
    struct lora_protocol_head_t *pLoraSendHal;
    struct lora_protocol_pause_07h_t *pPause07H, *pRecvPause07H;

    pLoraSendHal = (struct lora_protocol_head_t *)buf;
    pPause07H = (struct lora_protocol_pause_07h_t *)(buf+sizeof(struct lora_protocol_head_t));
    pRecvPause07H = (struct lora_protocol_pause_07h_t *)(pbuf+sizeof(struct lora_protocol_head_t));

    pLoraSendHal->sof = 0x7e;
    pLoraSendHal->seq = pCtx->seq++;
    pLoraSendHal->sn_src = pCtx->sn;
    pLoraSendHal->sn_dest = 0xffffffff;
    pLoraSendHal->devinfo.uni.split.channel_no = 0;
    pLoraSendHal->devinfo.uni.split.work_mode = 0;
    pLoraSendHal->devinfo.uni.split.terminal_mode = 0;
    pLoraSendHal->devinfo.uni.split.sub_network = 0;
    pLoraSendHal->reserved = 0;
    pLoraSendHal->func_code = PI_LORA_PAUSE | 0x80;
    pLoraSendHal->length = 2;   
    pPause07H->timeout = pRecvPause07H->timeout;

    if(pRecvPause07H->timeout != 0)
        pCtx->time_pause = time(NULL);
    else 
        pCtx->time_pause = 0;

    crc = crc16(buf, sizeof(struct lora_protocol_head_t) + sizeof(struct lora_protocol_ping_05h_t));
    crc = SWAP_16BIT(crc);
    MB_ALIGN_PT_16BIT_WRITE(buf+sizeof(struct lora_protocol_head_t)+ sizeof(struct lora_protocol_ping_05h_t), crc);


    return pCtx->pcomChildbase->write(pCtx, buf, sizeof(struct lora_protocol_head_t)+ sizeof(struct lora_protocol_ping_05h_t) + sizeof(crc));

}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_lora_frame_process(struct mb_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t nbytes)
{
    int ret = -1;
    struct lora_protocol_head_t *pLoRaHead;

    pLoRaHead = (struct lora_protocol_head_t *)pbuf;

    /* check whether the frame is good */
    if(mb_lora_frame_is_valid(pbuf, nbytes) < 0)
        return -1;
        
    switch(pLoRaHead->func_code)
    {
        case PI_LORA_DATA           :    
            ret = mb_lora_frame_data_01H(pCtx, pbuf, nbytes);
        break; 
        case PI_LORA_GETLOCAL_PARAM :

        break;
        case PI_LORA_SCAN          :
            ret = mb_lora_frame_scan_03H(pCtx, pbuf, nbytes);
        break;
        case PI_LORA_SETLOCAL_PARAM:

        break;
        case PI_LORA_PING          :
            ret = mb_lora_frame_ping_05H(pCtx, pbuf, nbytes);
        break;
        case PI_LORA_PAUSE         :
            ret = mb_lora_frame_ping_07H(pCtx, pbuf, nbytes);
        break;

        default:
        return -1;
    }
    
    return ret;
}






/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void *mb_lora_process_thread(void* parameter)
{
    int         nread;
    uint8_t     recvbuf[256];
    struct mb_comm_ctx_t *pCtx = (struct mb_comm_ctx_t *)parameter;
    
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while(1)
    {
        
        nread = pCtx->pcomChildbase->read(pCtx, recvbuf, sizeof(recvbuf), 2000);
        if(nread > 0)
        {
            mb_lora_frame_process(pCtx, recvbuf, nread);
        }
        else
        {
            usleep(50000);
        }
    }
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_lora_init(struct mb_comm_ctx_t *pCtx)
{
    pthread_attr_t 	attr;

    /*
        Lora HAL, base on sx127x hal driver
    
    */
    pCtx->pcomChildbase = mb_com_sx127x_ops_get();

    /* 
        do not stoping poll
     */
    pCtx->time_pause = 0;

    /*
        start up the lora 
    */
//    pCtx->pcomChildbase->connect(pCtx);

    if( pipe(pCtx->fd_lora_pipe) < 0 ){// 创建无名管道

		return -1;
	}

    /*
		create the channel poll thread
	*/
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (!pthread_create(&pCtx->process_thread_id, &attr, mb_lora_process_thread, pCtx) == 0)
	{
		pCtx->process_thread_id = -1;
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
static int mb_lora_comset(struct mb_comm_ctx_t *pCtx)
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
static int mb_lora_connect(struct mb_comm_ctx_t *pCtx)
{
    int ret;
    ret = pCtx->pcomChildbase->connect(pCtx);
    return ret;
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void mb_lora_disconnect(struct mb_comm_ctx_t *pCtx)
{
    return pCtx->pcomChildbase->disconnect(pCtx);
}



/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_lora_read(struct mb_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes, int timeout)
{
    int     nread;
    fd_set  rd;
    struct  timeval tv ;
    
    // 等待时间
    tv.tv_sec   = timeout / 1000;
    tv.tv_usec  = timeout % 1000 * 1000;

    FD_ZERO(&rd);
    FD_SET(pCtx->fd_lora_pipe[0], &rd);

    if(select(pCtx->fd_lora_pipe[0]+1, &rd, NULL, NULL, &tv) <= 0)
    {
        return -1;
    }
    else
    {
        nread = read(pCtx->fd_lora_pipe[0], pbuf, bytes);
        return nread;
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
static int mb_lora_write(struct mb_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes)
{
    uint8_t buf[256];
    uint8_t pending;
    uint16_t crc;
    time_t   now;
    struct lora_protocol_head_t *pLoraHal;

    /*
        only support stop poling for 300s
    */
    if(pCtx->time_pause != 0)
    {
        now = time(NULL);
        if(now - pCtx->time_pause < 300)
            return 0;
    }

    pCtx->time_pause = 0;

    if((bytes + sizeof(pending) + sizeof(struct lora_protocol_head_t) + sizeof(crc)) > sizeof(buf))
        bytes = sizeof(buf) - sizeof(struct lora_protocol_head_t) - sizeof(crc) - sizeof(pending);


    pLoraHal = (struct lora_protocol_head_t *)buf;
    pLoraHal->sof = 0x7e;
    pLoraHal->seq = pCtx->seq++;
    pLoraHal->sn_src = pCtx->sn;
    pLoraHal->sn_dest = 0xffffffff;
    pLoraHal->devinfo.uni.split.channel_no = 0;
    pLoraHal->devinfo.uni.split.work_mode = 0;
    pLoraHal->devinfo.uni.split.terminal_mode = 0;
    pLoraHal->devinfo.uni.split.sub_network = 0;
    pLoraHal->reserved = 0;
    pLoraHal->func_code = PI_LORA_DATA;
    pLoraHal->length = bytes + sizeof(pending);   

    /* pending flag. read protocol document 4.1 */
    pending = 0;
    buf[sizeof(struct lora_protocol_head_t) + sizeof(pending)] = pending;

    memcpy(buf+sizeof(struct lora_protocol_head_t) + sizeof(pending), pbuf, bytes);

    crc = crc16(buf, sizeof(struct lora_protocol_head_t)+bytes + sizeof(pending));
    crc = SWAP_16BIT(crc);
    MB_ALIGN_PT_16BIT_WRITE(buf+sizeof(struct lora_protocol_head_t)+bytes + sizeof(pending), crc);

    return pCtx->pcomChildbase->write(pCtx, buf, sizeof(struct lora_protocol_head_t) + bytes + sizeof(crc) + sizeof(pending));
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_lora_Flush(struct mb_comm_ctx_t *pCtx)
{
    return 0;
}




static struct mb_com_baseclass_ops_t lora_ops = {
    .init = mb_lora_init,
    .connect = mb_lora_connect,
    .disconnect = mb_lora_disconnect,
    .read = mb_lora_read,
    .write = mb_lora_write,
    .flush = mb_lora_Flush
};

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
struct mb_com_baseclass_ops_t *mb_com_lora_ops_get(void)
{
    return &lora_ops;
}