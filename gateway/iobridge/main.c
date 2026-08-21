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



#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>  //stat
#include <time.h>
#include <stdbool.h>
#include <signal.h> //

#include "hal_comm/hal_com_baseclass.h"
#include "dev_log.h"
#include "iobridge.h"



/*
    接口结构体，用于形成协议或者物理接口的表格和对应接口
*/
struct iface_table
{
    char        name[32];
    uint8_t     mode;
    void        *fops;
};


static const struct iface_table m_iface[] = {
    { "serial",        HAL_COM_MODE_SERIAL,     hal_com_serial_ops_get},
    { "tcpclient",     HAL_COM_MODE_TCP_CLIENT, hal_com_tcpclient_ops_get},
    { "tcpserver",     HAL_COM_MODE_TCP_SERVER, hal_com_tcpserver_ops_get},
};


static struct iobridge_ctx_t iobridge_ctx = {
    .northCommCtx = {
        .mode = HAL_COM_MODE_TCP_SERVER,
        .local_port = 7001,
        .remote_port = 0,
        .keepidle = 30,
        .keepintvl = 30,
        .keepcnt = 3,
        .dev = "192.168.1.216",
        .baud = 9600,
        .databit = 8,
        .stopbit = 1,
        .paritybit = 'N',
        .dump = 0,
        .fd = -1,
        .fd_listen = -1,
        .fd_max = -1,
    },
    .southCommCtx = {
        .mode = HAL_COM_MODE_SERIAL,
        .local_port = 0,
        .remote_port = 0,
        .keepidle = 30,
        .keepintvl = 30,
        .keepcnt = 3,
        .dev = "/dev/ttyO1",
        .baud = 9600,
        .databit = 8,
        .stopbit = 1,
        .paritybit = 'N',
        .dump = 0,
        .fd = -1,
        .fd_listen = -1,
        .fd_max = -1,
    },
};

/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static struct hal_com_baseclass_ops_t *find_fops_by_mode(uint8_t mode)
{
    uint8_t i;
    hal_com_ops_get pfunc;

    for(i = 0; i < sizeof(m_iface)/sizeof(m_iface[0]); i++)
    {
        if(mode == m_iface[i].mode)
        {
            pfunc = m_iface[i].fops;
            return pfunc();
        }
    }
    return NULL;
}


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int find_mode_by_iface(char *name)
{
    uint8_t i;

    for(i = 0; i < sizeof(m_iface)/sizeof(m_iface[0]); i++)
    {
        if(strcmp(name, m_iface[i].name) == 0)
        {
            return m_iface[i].mode;
        }
    }
    return -1;
}

/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int serial_split(char *str, uint32_t *baud, uint8_t *databit, uint8_t *paritybit, uint8_t *stopbit)
{
    /*
        9600,8N1
    */
    char *p = str;
    char *q = str;
    int len = 0;

    // 检查参数是否为空
    if(str == NULL)
        return -1;
    // 检查字符串长度是否为0
    len = strlen(str);
    if(len == 0)
        return -1;

    // 检查是否包含逗号
    q = strchr(p, ',');
    if(q == NULL)
        return -1;

    *q = '\0';
    // 获取波特率
    *baud = atoi(p);

    // 获取校验位
    q = q + 2;
    *paritybit = *q;

    *q = '\0';
    *databit = atoi(q-1);
    *stopbit = atoi(q+1);

    // 检查数据位
    if(!((*databit == 8) || (*databit == 7)))
        return -1;
    // 检查校验位
    if(!((*paritybit == 'N') || (*paritybit == 'E') || (*paritybit == 'O')))
        return -1;
    // 检查停止位
    if(!((*stopbit == 1) || (*stopbit == 2)))
        return -1;

    return 0;
}

/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static void usage(void)
{
	fprintf(stderr,"\niobridge_service [options] \n");

    fprintf(stderr,"\nexample:  iobridge -c 1 -m tcpserver -s 192.168.1.216 -p 7001 -S /dev/ttyO1 -B 9600,8N1 -n TSGXXXXXX   \n");

    fprintf(stderr,"  -m mode             : dir North: serial/tcpclient/tcpserver... \n");
    fprintf(stderr,"  -s dev              : dir North: set dev, for example: 192.168.1.216 , /dev/ttyO1 \n");
    fprintf(stderr,"  -p port             : dir North: set port. nouse for serial.   local for tcpserver, remote for tcpclient \n");
    fprintf(stderr,"  -b serial param     : dir North: set only for serial, for example: 9600,8N1  \n");
    fprintf(stderr,"  -k register enable  : dir North: 'none' - disable register. 'sn' - send SN register. 'ascii' - send txt register   only work for tcpclient \n");
    fprintf(stderr,"  -r register content : dir North: set tcp register packet. work only mode=tcpclient \n");
    
    fprintf(stderr,"  -M mode             : dir South: serial/tcpclient/tcpserver... \n");
    fprintf(stderr,"  -S dev              : dir South: set dev, for example: 192.168.1.216 , /dev/ttyO1 \n");
    fprintf(stderr,"  -P port             : dir South: set port. nouse for serial.   local for tcpserver, remote for tcpclient \n");
    fprintf(stderr,"  -B serial param     : dir South: set only for serial, for example: 8n1  \n");
    fprintf(stderr,"  -K register enable  : dir South: 'none' - disable register. 'sn' - send SN register. 'ascii' - send txt register   only work for tcpclient \n");
    fprintf(stderr,"  -R register content : dir South: set tcp register packet. work only mode=tcpclient \n");

    fprintf(stderr,"  -n sn               : set sn.  \n");
    fprintf(stderr,"  -c chnl no          : set chnl number \n");
    fprintf(stderr,"  -g                  : do nothing.  \n");
    fprintf(stderr,"  -d 0/1              : debug. printf tx,rx.  0-dump 485  1-dump lora \n");
	fprintf(stderr,"  -h                  : Show this help message\n");
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
int main(int argc, char *argv[])
{
    int  ret;
    int  opt;
    void *debugMsg;
    char sn[32];
    struct hal_comm_ctx_t *pnorthCommCtx = &iobridge_ctx.northCommCtx;
    struct hal_comm_ctx_t *psouthCommCtx = &iobridge_ctx.southCommCtx;

    if(argc < 8)
    {
        usage();
        return -1;
    }
    
    while((opt = getopt(argc, argv, "m:s:p:b:k:r:M:S:P:B:K:R:n:c:g:d:h")) != -1)
    {
        printf("opt = %c, optarg = %s\n", opt, optarg);
        switch(opt)
        {
            case 'c':
                // 外部使用base1， 内部实际使用base0. 通道号（北向通道号）
                pnorthCommCtx->channel_no = atoi(optarg) - 1;
                psouthCommCtx->channel_no = pnorthCommCtx->channel_no;
                printf("chnl no = %d\n", pnorthCommCtx->channel_no);
            break;
            case 'n':
                snprintf(sn, sizeof(sn), "%s", optarg);
            break;
            case 'd':
                iobridge_ctx.northCommCtx.dump = atoi(optarg);
                iobridge_ctx.southCommCtx.dump = iobridge_ctx.northCommCtx.dump;
            break;
            case 'h':
                usage();
                return -1;

            case 'm':
                 pnorthCommCtx->mode = find_mode_by_iface(optarg);
                 if(pnorthCommCtx->mode < 0)
                 {
                    printf("find mode by iface %s failed\n", optarg);
                    usage();
                    return -1;
                 }
            break;
            case 's':
                snprintf(pnorthCommCtx->dev, sizeof(pnorthCommCtx->dev), "%s", optarg);
                if(strstr(pnorthCommCtx->dev, "/dev/ttyO") != NULL)
                {
                    pnorthCommCtx->channel_no = LOG_CHNL_FW(atoi(pnorthCommCtx->dev + strlen("/dev/ttyO")));
                }
                
            break;
            case 'p':
                pnorthCommCtx->local_port = atoi(optarg);
                pnorthCommCtx->remote_port = pnorthCommCtx->local_port;
            break;
            case 'b':
                ret = serial_split(optarg, &pnorthCommCtx->baud, &pnorthCommCtx->databit, &pnorthCommCtx->paritybit, &pnorthCommCtx->stopbit);
                if(ret < 0)
                {
                    printf("serial split %s failed\n", optarg);
                    usage();
                    return -1;
                }
            break;
            case 'k':
                if(strcmp(optarg, "none"))
                    pnorthCommCtx->regEnable = 0;
                else if(strcmp(optarg, "sn"))
                    pnorthCommCtx->regEnable = 1;
                else if(strcmp(optarg, "ascii"))
                    pnorthCommCtx->regEnable = 2;
                else 
                {
                    printf("reg enable %s failed\n", optarg);
                    usage();
                    return -1;
                }
            break;
            case 'r':   
                snprintf((char *)pnorthCommCtx->regPacket, sizeof(pnorthCommCtx->regPacket), "%s", optarg);
            break;

            case 'M':
                 psouthCommCtx->mode = find_mode_by_iface(optarg);
                 if(psouthCommCtx->mode < 0)
                 {
                    printf("find mode by iface %s failed\n", optarg);
                    usage();
                    return -1;
                 }
            break;
            case 'S':
                snprintf(psouthCommCtx->dev, sizeof(psouthCommCtx->dev), "%s", optarg); 
                if(strstr(psouthCommCtx->dev, "/dev/ttyO") != NULL)
                {
                    psouthCommCtx->channel_no = atoi(psouthCommCtx->dev + strlen("/dev/ttyO"));
                }
            break;
            case 'P':
                psouthCommCtx->local_port = atoi(optarg);
                psouthCommCtx->remote_port = psouthCommCtx->local_port;
            break;
            case 'B':
                ret = serial_split(optarg, &psouthCommCtx->baud, &psouthCommCtx->databit, &psouthCommCtx->paritybit, &psouthCommCtx->stopbit);
                if(ret < 0)
                {
                    printf("serial split %s failed\n", optarg);
                    usage();
                    return -1;
                }
            break;
            case 'K':
                if(strcmp(optarg, "none"))
                    psouthCommCtx->regEnable = 0;
                else if(strcmp(optarg, "sn"))
                    psouthCommCtx->regEnable = 1;
                else if(strcmp(optarg, "ascii"))
                    psouthCommCtx->regEnable = 2;
                else 
                {
                    printf("reg enable %s failed\n", optarg);
                    usage();
                    return -1;
                }
            break;
            case 'R':
                snprintf((char *)psouthCommCtx->regPacket, sizeof(psouthCommCtx->regPacket), "%s", optarg);
            break;
        }
    }

    // 注册帧内容为SN
    if(pnorthCommCtx->regEnable == 1)
        snprintf((char *)pnorthCommCtx->regPacket, sizeof(pnorthCommCtx->regPacket), "%s", sn);
    
    // 注册帧内容为SN
    if(psouthCommCtx->regEnable == 1)
        snprintf((char *)psouthCommCtx->regPacket, sizeof(psouthCommCtx->regPacket), "%s", sn);

    if(iobridge_ctx.northCommCtx.dump)
    {
        printf("north param: \n"
               "   mode = %d, dev = %s, local_port = %d, remote_port = %d, baud = %d, databit = %d, paritybit = %d, stopbit = %d, regEnable = %d, regPacket = %s\n",
            pnorthCommCtx->mode, pnorthCommCtx->dev, pnorthCommCtx->local_port, pnorthCommCtx->remote_port, pnorthCommCtx->baud, pnorthCommCtx->databit, pnorthCommCtx->paritybit, pnorthCommCtx->stopbit, pnorthCommCtx->regEnable, pnorthCommCtx->regPacket);
        printf("south param: \n"
               "   mode = %d, dev = %s, local_port = %d, remote_port = %d, baud = %d, databit = %d, paritybit = %d, stopbit = %d, regEnable = %d, regPacket = %s\n",
            psouthCommCtx->mode, psouthCommCtx->dev, psouthCommCtx->local_port, psouthCommCtx->remote_port, psouthCommCtx->baud, psouthCommCtx->databit, psouthCommCtx->paritybit, psouthCommCtx->stopbit, psouthCommCtx->regEnable, psouthCommCtx->regPacket);
    }

    debugMsg = log_open(NULL);
    if(NULL == debugMsg)
    {
        printf("please insert /dev/log.ko first\n");
        return -1;
    }

    // 查找协议操作函数
    pnorthCommCtx->pcombase = find_fops_by_mode(pnorthCommCtx->mode);
    if(!pnorthCommCtx->pcombase)
    {
        printf("find fops by north mode %d failed\n", pnorthCommCtx->mode);
        usage();
        return -1;
    }

    // 查找协议操作函数
    psouthCommCtx->pcombase = find_fops_by_mode(psouthCommCtx->mode);
    if(!psouthCommCtx->pcombase)
    {
        printf("find fops by south mode %d failed\n", psouthCommCtx->mode);
        usage();
        return -1;
    }

    // 初始化iobridge
    iobridge_init(&iobridge_ctx);

    while(1)
    {
        sleep(100);
    }

    log_close();
    return 0;
}