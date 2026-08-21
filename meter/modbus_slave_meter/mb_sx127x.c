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

#include <linux/serial.h>
#include <sys/ioctl.h>

#include "mb_com_baseclass.h"
#include "sx127x.h"

#define LED_RX          "/sys/class/leds/rx/shot"
#define LED_TX          "/sys/class/leds/tx/shot"

/*
    PMAC121 分16主网，每个主网支持2个子网。共48个频段。 但实际现场PMAC121对外常用主网，不用子网。
    EGO700使用将48通道全用于主网 ，不再使用子网概念

    为了兼容PMAC121，0-15 信道 兼容PMAC121
    其余32个PMAC121子网信道，全部用于EGO700的 16-47信道

    即 PMAC121每隔 3x830000 为1个主信道。  其中分成3份， 第0份是主信道频率， 第1份为子信道0频率，第2份为子信道1频率


*/
#define SIGNAL_CHANNEL_BASE     470030000       // 信道起始频率。 不从470000000开始是因为避开常用频率，降低与其他lora产品频率冲突概率
#define SIGNAL_CHANNEL_DIV      830000          // 每个子信道频率的步进



const uint32_t signal_channel[48] = {

    // 0-15 compatible with pmac121 main network
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 0 ,   /* 0  */     
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 1 ,   /* 1  */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 2 ,   /* 2  */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 3 ,   /* 3  */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 4 ,   /* 4  */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 5 ,   /* 5  */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 6 ,   /* 6  */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 7 ,   /* 7  */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 8 ,   /* 8  */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 9 ,   /* 9  */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 10,   /* 10  */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 11,   /* 11  */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 12,   /* 12  */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 13,   /* 13  */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 14,   /* 14  */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 15,   /* 15  */

    // 16-31 use pmac121 sub0 network
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 0  + SIGNAL_CHANNEL_DIV ,   /* 0 + 16 */     
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 1  + SIGNAL_CHANNEL_DIV ,   /* 1 + 16*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 2  + SIGNAL_CHANNEL_DIV ,   /* 2 + 16*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 3  + SIGNAL_CHANNEL_DIV ,   /* 3 + 16*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 4  + SIGNAL_CHANNEL_DIV ,   /* 4 + 16*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 5  + SIGNAL_CHANNEL_DIV ,   /* 5 + 16*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 6  + SIGNAL_CHANNEL_DIV ,   /* 6 + 16*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 7  + SIGNAL_CHANNEL_DIV ,   /* 7 + 16*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 8  + SIGNAL_CHANNEL_DIV ,   /* 8 + 16*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 9  + SIGNAL_CHANNEL_DIV ,   /* 9 + 16*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 10 + SIGNAL_CHANNEL_DIV ,   /* 10+ 16 */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 11 + SIGNAL_CHANNEL_DIV ,   /* 11+ 16 */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 12 + SIGNAL_CHANNEL_DIV ,   /* 12+ 16 */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 13 + SIGNAL_CHANNEL_DIV ,   /* 13+ 16 */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 14 + SIGNAL_CHANNEL_DIV ,   /* 14+ 16 */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 15 + SIGNAL_CHANNEL_DIV ,   /* 15+ 16 */

    // 32-47 use pmac121 sub1 network
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 0  + SIGNAL_CHANNEL_DIV*2 ,   /* 0 +32*/     
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 1  + SIGNAL_CHANNEL_DIV*2 ,   /* 1 +32*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 2  + SIGNAL_CHANNEL_DIV*2 ,   /* 2 +32*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 3  + SIGNAL_CHANNEL_DIV*2 ,   /* 3 +32*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 4  + SIGNAL_CHANNEL_DIV*2 ,   /* 4 +32*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 5  + SIGNAL_CHANNEL_DIV*2 ,   /* 5 +32*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 6  + SIGNAL_CHANNEL_DIV*2 ,   /* 6 +32*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 7  + SIGNAL_CHANNEL_DIV*2 ,   /* 7 +32*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 8  + SIGNAL_CHANNEL_DIV*2 ,   /* 8 +32*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 9  + SIGNAL_CHANNEL_DIV*2 ,   /* 9 +32*/
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 10 + SIGNAL_CHANNEL_DIV*2 ,   /* 10+32 */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 11 + SIGNAL_CHANNEL_DIV*2 ,   /* 11+32 */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 12 + SIGNAL_CHANNEL_DIV*2 ,   /* 12+32 */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 13 + SIGNAL_CHANNEL_DIV*2 ,   /* 13+32 */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 14 + SIGNAL_CHANNEL_DIV*2 ,   /* 14+32 */
    SIGNAL_CHANNEL_BASE + SIGNAL_CHANNEL_DIV * 3 * 15 + SIGNAL_CHANNEL_DIV*2 ,   /* 15+32 */

};



/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void mb_sx127x_led_rx(struct mb_comm_ctx_t *pCtx)
{
    int fd;

    if(pCtx->led_blink == MB_CFG_LED_LORA)
    {
        fd = open(LED_RX, O_WRONLY);
        if(fd < 0)
            return;
        write(fd, "1\n", 2);
        close(fd);
    }
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void mb_sx127x_led_tx(struct mb_comm_ctx_t *pCtx)
{
    int fd;

    if(pCtx->led_blink == MB_CFG_LED_LORA)
    {
        fd = open(LED_TX, O_WRONLY);
        if(fd < 0)
            return;
        write(fd, "1\n", 2);
        close(fd);
    }
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_sx127x_init(struct mb_comm_ctx_t *pCtx)
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
static int mb_sx127x_comset(struct mb_comm_ctx_t *pCtx)
{

    if (ioctl(pCtx->fd, SX127X_IOCTL_CMD_SETPAOUTPUT, SX127X_PA_PABOOST) != 0) {
		printf("failed to set pa output\n");
		return -1;
	}

	if (ioctl(pCtx->fd, SX127X_IOCTL_CMD_SETOUTPUTPOWER, 20) != 0) {
		printf("failed to set power\n");
		return -1;
	}


	if (ioctl(pCtx->fd, SX127X_IOCTL_CMD_SETMODULATION, SX127X_MODULATION_LORA) != 0) {
		printf("failed to set modulation\n");
		return -1;
	}

    /*  */
    if(pCtx->signal_chnl >= 48)
    {
        pCtx->signal_chnl = 0;
    }

    
    /*
        LORA 支持2种模式： pilot lora协议（兼容PMAC121） 和透传 LORA 无协议（用于对接第三方客户）
        pilot lora协议： 更详尽的调试功能，防止冲突，参数设定简单
        lora 透传   ： 无协议， 所以lora参数由用户配置
    */
    if(pCtx->lora_pktmode == SAMPLING_CFG_LORA_PILOT)
    {
        if (ioctl(pCtx->fd, SX127X_IOCTL_CMD_SETCARRIERFREQUENCY, signal_channel[pCtx->signal_chnl]) != 0) {
            printf("failed to set carrier frequency\n");
            return -1;
        }

        if (ioctl(pCtx->fd, SX127X_IOCTL_CMD_SETBANDWIDTH, 7) != 0) {
            printf("failed to set band_width\n");
            return -1;
        }

        if (ioctl(pCtx->fd, SX127X_IOCTL_CMD_SET_CODINGRATE, 3) != 0) {
            printf("failed to set coding_rate\n");
            return -1;
        }

        if (ioctl(pCtx->fd, SX127X_IOCTL_CMD_SETSF, 9) != 0) {
            printf("failed to set spreading factor\n");
            return -1;
        }

        if (ioctl(pCtx->fd, SX127X_IOCTL_CMD_SET_LOWDATARATE, 1) != 0) {
            printf("failed to LowDataRateOptimize\n");
            return -1;
        }
    }
    else 
    {
        if (ioctl(pCtx->fd, SX127X_IOCTL_CMD_SETCARRIERFREQUENCY, pCtx->center_frequency) != 0) {
            printf("failed to set carrier frequency\n");
            return -1;
        }

        if(pCtx->band_width > 9)
            pCtx->band_width = 7;
        if (ioctl(pCtx->fd, SX127X_IOCTL_CMD_SETBANDWIDTH, pCtx->band_width) != 0) {
            printf("failed to set band_width\n");
            return -1;
        }

        if((pCtx->coding_rate > 4) || (pCtx->coding_rate < 1))
            pCtx->coding_rate = 3;

        if (ioctl(pCtx->fd, SX127X_IOCTL_CMD_SET_CODINGRATE, pCtx->coding_rate) != 0) {
            printf("failed to set coding_rate\n");
            return -1;
        }

        if (ioctl(pCtx->fd, SX127X_IOCTL_CMD_SETSF, pCtx->spreading_factor) != 0) {
            printf("failed to set spreading factor\n");
            return -1;
        }

        if (ioctl(pCtx->fd, SX127X_IOCTL_CMD_SET_LOWDATARATE, pCtx->LowDataRateOptimize) != 0) {
            printf("failed to LowDataRateOptimize\n");
            return -1;
        }

        if (ioctl(pCtx->fd, SX127X_IOCTL_CMD_SET_PREAMBLE_SIZE, pCtx->preamble_size) != 0) {
            printf("failed to preamble_size\n");
            return -1;
        }
    }

	if (ioctl(pCtx->fd, SX127X_IOCTL_CMD_SETOPMODE, SX127X_OPMODE_RXCONTINUOS) != 0) {
		printf("failed to set opmode\n");
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
static int mb_sx127x_connect(struct mb_comm_ctx_t *pCtx)
{
    int ret;

    if(!pCtx->dev)
    {
        printf("sx1278 dev is NULL\n");
        return -1;
    }

    if(pCtx->fd < 0)
    {
		// Try to open the sx127x port.
        pCtx->fd = open(pCtx->dev, O_RDWR);
		if (pCtx->fd != -1)
        {
            /* initial uart */
            ret = mb_sx127x_comset(pCtx);
            if(ret < 0)
                return -1;

            return 0;
        }
        else
        {
            printf("can not open /dev/sx127x0\n");
            exit(-1);
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
static void mb_sx127x_disconnect(struct mb_comm_ctx_t *pCtx)
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
static int mb_sx127x_read(struct mb_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes, int timeout)
{
    int n;

    if(-1 == pCtx->fd)
        return -1;

    timeout = 600000; // LoRa 异常超时强制使用10分钟。
    ioctl(pCtx->fd, SX127X_IOCTL_CMD_SET_RXTIMEOUT, timeout);

    n = read(pCtx->fd, pbuf, bytes);
    if(n > 0){
        mb_sx127x_led_rx(pCtx);
        MB_PACKET_DUMP(pCtx, "RX:", pbuf, n);
    }
    return n;
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_sx127x_write(struct mb_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes)
{
    if(-1 == pCtx->fd)
        return -1;
        
    mb_sx127x_led_tx(pCtx);

    MB_PACKET_DUMP(pCtx, "TX:", pbuf, bytes);
    return write(pCtx->fd, pbuf, bytes);
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_sx127x_Flush(struct mb_comm_ctx_t *pCtx)
{
    return 0;
}




static struct mb_com_baseclass_ops_t sx127x_ops = {
    .init = mb_sx127x_init,
    .connect = mb_sx127x_connect,
    .disconnect = mb_sx127x_disconnect,
    .read = mb_sx127x_read,
    .write = mb_sx127x_write,
    .flush = mb_sx127x_Flush
};

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
struct mb_com_baseclass_ops_t *mb_com_sx127x_ops_get(void)
{
    return &sx127x_ops;
}