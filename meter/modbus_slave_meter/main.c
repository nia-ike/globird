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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>  //stat
#include <time.h>
#include <stdbool.h>
#include <signal.h>

#include "db_cfg.h"
#include "db_device.h"
#include "db_msgevent_api.h"
#include "db_msr_msgevent_api.h"
#include "db_struct.h"

#include "mb_cfg.h"
#include "mb_com_baseclass.h"
#include "mb_process.h"
#include "mb_protocol.h"
#include "mb_dlt645_07_com.h"
#include "mb_lora_cfg.h"
#include "dev_log.h"


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static void usage(char *_name)
{
	fprintf(stderr,"\nUsage: %s [options] \n",_name);

    fprintf(stderr,"  -m 0/1/2/3/4/5/6/7    : set mode. 0-serial 1-tcpclient 2-stdin.stdout 3-tcpserver.4-dlt645-07 5-pilot lora 6-lora transfer 7-modbusRTU over tcpserver default 0\n");
    fprintf(stderr,"  -b 9600             : set baudrate. work only mode=0 \n");
    fprintf(stderr,"  -s pmac.com.cn      : set server url(IP). or /dev/xxx, for example: /dev/sx127x0 \n");
    fprintf(stderr,"  -p 502              : set port. work only mode=1 \n");
    fprintf(stderr,"  -k 120              : set tcp keepalive inv. work only mode=1 \n");
    fprintf(stderr,"  -r register content : set tcp register packet. work only mode=1 \n");
    fprintf(stderr,"  -c cfg file         : set cfg file \n");
    fprintf(stderr,"  -n sn               : set sn. action only in lora mode[-m 5] \n");
    fprintf(stderr,"  -N N                : set max message channels (default: 2, must match db_process -c N)\n");
    fprintf(stderr,"  -f lora channel no. : set lora channel 0-47. action only in lora mode[-m 5][-m 6] \n");
    fprintf(stderr,"  -C 1/2/3/4          : set lora cr. use for lora mode 1 \n");
    fprintf(stderr,"  -S 6/7/8/9/10/11/12 : set lora spreading factor. use for lora mode 1 \n");
    fprintf(stderr,"  -B 0-9              : set lora bandwidth. use for lora mode 1. 7-125K \n");
    fprintf(stderr,"  -F 470000000-510000000 : set lora freq. use for lora mode 1 \n");
    fprintf(stderr,"  -l 0/1 .            : set tx rx led come under with Rs485 or LORA. 0-485  1-lora \n");
    fprintf(stderr,"  -d 0/1              : debug. printf tx,rx.  0-dump 485  1-dump lora \n");
	fprintf(stderr,"  -h                  : Show this help message\n");
}


void record_signo(int signo)
{
	
}

/*********************************************************************************************************
** Function name:       mb_msg_process_rs485
** Descriptions:        modbus 识别DLT645 和 modbusRTU
** input parameters:    pRecvBuff 接收到的数据
**                      recvLen   接收到的数据长度
** output parameters:   None
** Returned value:      
*********************************************************************************************************/
static int mb_msg_process_rs485(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen)
{
    int ret = -1;

    ret = mb_msg_process_Dlt645_07(pCtx, &pCtx->recvBuff[0], pCtx->recvLen);

    if(0 == ret)
    {
        ret = mb_msg_process_rtu(pCtx, &pCtx->recvBuff[0], pCtx->recvLen);
    }

    return ret;
}


/*********************************************************************************************************
** Function name:       
** Descriptions:   由于EGO700 SN号历史原因是11位数字（比如20112001000）超出u32.  格式为 2位数字年， 2位数字月， 2位数字日， 5位数字序号
				   现在压缩序号只要0-9999， 因此第5位固定为0. 最后压缩为 2011201000
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static uint32_t Sn_str2u32(char *psn)
{
	char sn[11];

	if(strstr(psn, "TSM") == NULL)
		return 0;

	psn = psn + (sizeof("TSM") - 1); // -1 . "TSM" have a '0'

	// drop the 7th char
	strncpy(sn, psn, 6);		// get the first 6 char
	strncpy(sn+6, psn+7, 4);    // get the last  4 char
	sn[10] = '\0';
	
	return strtoul(sn, 0, 10);
}


/*************************************************************************/
/** Function Name:                                                     **/
/** Purpose:                                                           **/
/** Params:            None.                                           **/
/** Return:            None.                                           **/
/** Notice:            None.                                           **/
/*************************************************************************/
void *Vmem(void *p)
{
    static void *vmem = NULL;

    if(p != NULL)
        vmem = p;

    return vmem;
}

/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int main(int argc, char **argv)
{
    void    *vmem;
    void    *debugMsg;
    struct mb_comm_ctx_t mb_comm_ctx;
    int     opt, ret;
    int     baud = 0;
    int     paritybit = 0;
    int     databit = 0;
    int     stopbit = 0;
    uint8_t     mode = 0; // 0-serial  1-tcpclient
    uint16_t    port = 502;
    uint32_t    keepintvl = 120;
    char        url[64] = "/dev/ttyS1";
    char        cfg[64] = "/userdata/config/comm/universally/modbustcp_slave_client.json";
    char        regPacket[24] = {0};
    struct mb_optarg_t  optarg_mgr = {
        .sn = 0,
        .lora_pktmode = 0xff,
        .signal_chnl = 0xff,
        .center_frequency = 0xffffffff,
        .coding_rate = 0xff,
        .spreading_factor = 0xff,
        .band_width = 0xff,
        .preamble_size = 0xffff,
        .LowDataRateOptimize = 0xff,
    };
    
    uint8_t max_msg_channels = 2;  // 默认2个通道，需与 db_process 保持一致
    
    mb_comm_ctx.running = true;
    mb_comm_ctx.led_blink = MB_CFG_LED_NONE;
    mb_comm_ctx.dump = false;
    if(argc >= 2)
    {
        while((opt=getopt(argc, argv, "hm:b:p:k:s:r:c:n:N:f:l:d:C:S:B:F:")) != -1)
        {
            switch(opt)
            {
                case 'b':
                    baud = atoi(optarg);
                break;
                case 'm':
                    mode = atoi(optarg);
                break;
                case 'p':
                    port = atoi(optarg);
                break;
                case 'k':
                    keepintvl = atoi(optarg);
                break;
                case 's':
                    strncpy(url, optarg, sizeof(url));
                break;
                case 'c':
                    strncpy(cfg, optarg, sizeof(cfg));
                break;
                case 'r':
                    strncpy(regPacket, optarg, sizeof(regPacket));
                break;
                case 'n':
                    mb_comm_ctx.sn = Sn_str2u32(optarg);
                break;
                case 'N':
                    max_msg_channels = (uint8_t)atoi(optarg);
                    if(max_msg_channels < 1 || max_msg_channels > 32) {
                        fprintf(stderr, "Error: max_channels must be between 1 and 32\n");
                        exit(1);
                    }
                    printf("Set max message channels to: %d\n", max_msg_channels);
                break;
                case 'l':
                    mb_comm_ctx.led_blink = (atoi(optarg) == 0) ? 0 : 1;
                break;
                case 'd':
                    mb_comm_ctx.dump = atoi(optarg);
                break;
                case 'f':
                {
                    int chnl;

                    chnl = atoi(optarg);
                    if(chnl > 47 || chnl < 0)
                    {
                        printf("-f only support 0-47");
                        exit(0);
                    }
                    optarg_mgr.signal_chnl = chnl;
                }
                break;
                 case 'C':
                {
                    int coding_rate;

                    coding_rate = atoi(optarg);
                    if(coding_rate > 4 || coding_rate < 1)
                    {
                        printf("-c only support 1-4");
                        exit(0);
                    }
                    optarg_mgr.coding_rate = coding_rate;
                }
                break;
                case 'S':
                {
                    int spreading_factor;

                    spreading_factor = atoi(optarg);
                    if(spreading_factor > 12 || spreading_factor < 6)
                    {
                        printf("-c only support 6-12");
                        exit(0);
                    }
                    optarg_mgr.spreading_factor = spreading_factor;
                }
                break;
                case 'B':
                {
                    int band_width;

                    band_width = atoi(optarg);
                    if(band_width > 9)
                    {
                        printf("-b only support 0-9");
                        exit(0);
                    }
                    optarg_mgr.band_width = band_width;
                }
                break;
                case 'F':
                    optarg_mgr.center_frequency = strtoul(optarg, 0, 0);
                break;
                case 'h':
                    usage(argv[0]);
                    exit(0);
                break;

            }
        }
    }

    /*
        database attach:
            it just get the vmem pointer from /dev/vmem  (mmap)
    */
    vmem = rt_database_attach();
    if(vmem == NULL)
    {
        printf("please insert /dev/vmem.ko first\n");
        return -1;
    }
    Vmem(vmem);
    /*
        prepare the modbus register map table and some other configure
    */
    debugMsg = log_open();
    if(NULL == debugMsg)
    {
        printf("debug log open fail!\r\n");
        return -1;
    }

//    mb_config_init(vmem, cfg);
    DLT645_07_config_init(vmem);
    /*
        if not -b baud, will use the db_process setting.
    */
    if(baud == 0)
        baud = ((struct db_database_type *)vmem)->db_measurebase[0].db_cfg.user.base.BaudRate.value;

    databit = ((struct db_database_type *)vmem)->db_measurebase[0].db_cfg.user.base.CharSize.value;
    if(strcmp(url, "/dev/ttyS1") == 0)
        paritybit = ((struct db_database_type *)vmem)->db_measurebase[0].db_cfg.user.base.ParityBit.value;
    stopbit = ((struct db_database_type *)vmem)->db_measurebase[0].db_cfg.user.base.StopBit.value;

        // 为了多态表读取同一批寄存器号，检查到地址1=地址2时，地址2在地址1的基础上+1
    if(((struct db_database_type *)vmem)->db_measurebase[0].db_cfg.user.base.Addr.value == ((struct db_database_type *)vmem)->db_measurebase[1].db_cfg.user.base.Addr.value)
    {
        ((struct db_database_type *)vmem)->db_measurebase[1].db_cfg.user.base.Addr.value = ((struct db_database_type *)vmem)->db_measurebase[0].db_cfg.user.base.Addr.value + 1;
    }
    
    mb_comm_ctx.mode = mode;

    switch(mode)
    {
        case 0:
            mb_comm_ctx.fd = -1;
            mb_comm_ctx.fd_listen = -1;
            mb_comm_ctx.fd_max = -1;
            mb_comm_ctx.dev = url;
            mb_comm_ctx.baud = baud;
            mb_comm_ctx.databit = databit;
            mb_comm_ctx.paritybit = paritybit;
            mb_comm_ctx.stopbit = stopbit;
            mb_comm_ctx.pvmem = vmem;
            mb_comm_ctx.pcombase = mb_com_serial_ops_get();
            mb_comm_ctx.msg_protocol_process = mb_msg_process_rs485;
        break;
        case 1:
            mb_comm_ctx.fd = -1;
            mb_comm_ctx.fd_listen = -1;
            mb_comm_ctx.fd_max = -1;
            mb_comm_ctx.dev = url;
            mb_comm_ctx.port = port;
            mb_comm_ctx.keepidle = keepintvl;
            mb_comm_ctx.keepintvl = keepintvl;
            mb_comm_ctx.keepcnt = 2;
            mb_comm_ctx.pvmem = vmem;
            mb_comm_ctx.pcombase = mb_com_tcpclient_ops_get();
            mb_comm_ctx.msg_protocol_process = mb_msg_process_tcp;
            snprintf(mb_comm_ctx.regPacket, sizeof(mb_comm_ctx.regPacket), "%s", regPacket);
        break;
        case 2:
            /*
                stdin stdout, raw mode. all the signal include ctrl+c ctrl+a...z, is data. so wen need set up signal
                process must exit by 'q'
            */
            signal(SIGHUP, record_signo);
            signal(SIGINT, record_signo);
            signal(SIGTERM, record_signo);
            signal(SIGPIPE, record_signo);

            mb_comm_ctx.fd = -1;
            mb_comm_ctx.fd_listen = -1;
            mb_comm_ctx.fd_max = -1;
            mb_comm_ctx.dev = url;
            mb_comm_ctx.baud = baud;
            mb_comm_ctx.databit = databit;
            mb_comm_ctx.paritybit = paritybit;
            mb_comm_ctx.stopbit = stopbit;
            mb_comm_ctx.pvmem = vmem;
            mb_comm_ctx.pcombase = mb_com_stdinout_ops_get();
            mb_comm_ctx.msg_protocol_process = mb_msg_process_rs485;
        break;
        case 3:
        /*
            TCP SERVER mode. working with modbusTCP protocol
        */
            mb_comm_ctx.fd = -1;
            mb_comm_ctx.fd_listen = -1;
            mb_comm_ctx.fd_max = -1;
            mb_comm_ctx.dev = url;
            mb_comm_ctx.port = port;
            mb_comm_ctx.keepidle = keepintvl;
            mb_comm_ctx.keepintvl = keepintvl;
            mb_comm_ctx.keepcnt = 2;
            mb_comm_ctx.pvmem = vmem;
            mb_comm_ctx.pcombase = mb_com_tcpserver_ops_get();
            mb_comm_ctx.msg_protocol_process = mb_msg_process_tcp;
        break;
        case 4:
            mb_comm_ctx.fd = -1;
            mb_comm_ctx.fd_listen = -1;
            mb_comm_ctx.fd_max = -1;
            mb_comm_ctx.dev = url;
            mb_comm_ctx.baud = baud;
            mb_comm_ctx.databit = databit;
            mb_comm_ctx.paritybit = paritybit;
            mb_comm_ctx.stopbit = stopbit;
            mb_comm_ctx.pvmem = vmem;
            mb_comm_ctx.pcombase = mb_com_serial_ops_get();
            mb_comm_ctx.msg_protocol_process = mb_msg_process_rs485;
        break;
        case 5:
            ret = config_lora_load(&optarg_mgr, CFG_FILE_ROOT_CHANNEL);
            if(ret < 0)
                config_lora_default(&optarg_mgr);

            mb_comm_ctx.fd = -1;
            mb_comm_ctx.fd_listen = -1;
            mb_comm_ctx.fd_max = -1;
            mb_comm_ctx.dev = url;
            mb_comm_ctx.baud = baud;
            mb_comm_ctx.databit = databit;
            mb_comm_ctx.paritybit = paritybit;
            mb_comm_ctx.stopbit = stopbit;
            mb_comm_ctx.pvmem = vmem;
            mb_comm_ctx.pcombase = mb_com_lora_ops_get();
            mb_comm_ctx.msg_protocol_process = mb_msg_process_rs485;
            mb_comm_ctx.lora_pktmode = SAMPLING_CFG_LORA_PILOT;

            mb_comm_ctx.signal_chnl         = optarg_mgr.signal_chnl;
            mb_comm_ctx.spreading_factor    = optarg_mgr.spreading_factor;
            mb_comm_ctx.coding_rate         = optarg_mgr.coding_rate;
            mb_comm_ctx.band_width          = optarg_mgr.band_width;          /* 0-9   7.8K - 500K*/
            mb_comm_ctx.center_frequency    = optarg_mgr.center_frequency;
            mb_comm_ctx.preamble_size       = optarg_mgr.preamble_size;
            mb_comm_ctx.LowDataRateOptimize = optarg_mgr.LowDataRateOptimize;
                
        break;
        case 6:
            ret = config_lora_load(&optarg_mgr, CFG_FILE_ROOT_CHANNEL);
            if(ret < 0)
                config_lora_default(&optarg_mgr);

            mb_comm_ctx.fd = -1;
            mb_comm_ctx.fd_listen = -1;
            mb_comm_ctx.fd_max = -1;
            mb_comm_ctx.dev = url;
            mb_comm_ctx.baud = baud;
            mb_comm_ctx.databit = databit;
            mb_comm_ctx.paritybit = paritybit;
            mb_comm_ctx.stopbit = stopbit;
            mb_comm_ctx.pvmem = vmem;
            mb_comm_ctx.pcombase = mb_com_sx127x_ops_get();
            mb_comm_ctx.msg_protocol_process = mb_msg_process_rs485;
            mb_comm_ctx.lora_pktmode = SAMPLING_CFG_LORA_TRAN;

            mb_comm_ctx.signal_chnl         = optarg_mgr.signal_chnl;
            mb_comm_ctx.spreading_factor    = optarg_mgr.spreading_factor;
            mb_comm_ctx.coding_rate         = optarg_mgr.coding_rate;
            mb_comm_ctx.band_width          = optarg_mgr.band_width;          /* 0-9   7.8K - 500K*/
            mb_comm_ctx.center_frequency    = optarg_mgr.center_frequency;
            mb_comm_ctx.preamble_size       = optarg_mgr.preamble_size;
            mb_comm_ctx.LowDataRateOptimize = optarg_mgr.LowDataRateOptimize;
        break;
        case 7:
        /*
            TCP SERVER mode. working with modbusTCP protocol
        */
            mb_comm_ctx.fd = -1;
            mb_comm_ctx.fd_listen = -1;
            mb_comm_ctx.fd_max = -1;
            mb_comm_ctx.dev = url;
            mb_comm_ctx.port = port;
            mb_comm_ctx.keepidle = keepintvl;
            mb_comm_ctx.keepintvl = keepintvl;
            mb_comm_ctx.keepcnt = 2;
            mb_comm_ctx.pvmem = vmem;
            mb_comm_ctx.pcombase = mb_com_tcpserver_ops_get();
            mb_comm_ctx.msg_protocol_process = mb_msg_process_rs485;
        break;
        default:
            usage(argv[0]);
            exit(0);
        return -1;

    }




    mb_config_init(vmem, cfg, &mb_comm_ctx);

    mb_comm_ctx.pcombase->init(&mb_comm_ctx);
    /*
        event message. modbus may set some command to other process
    */
    db_msg_event_api_init();
    
    /* 配置消息通道数（必须在初始化之前调用） */

    ret = db_msr_msg_event_api_config(max_msg_channels);
    if(ret < 0)
    {
        printf("Failed to configure message channels\n");
        return -1;
    }

    /* 初始化所有通道的消息队列（动态支持1-32通道）*/
    for(uint8_t i = 0; i < max_msg_channels; i++)
    {
        if(db_msr_msg_event_api_init(i) < 0)
        {
            printf("modbus_slave: failed to init channel %d\n", i);
            return -1;
        }
    }


    /* modbus server */
    mb_server_init(&mb_comm_ctx);

    /* modbus process */
    while(mb_comm_ctx.running)
    {
        mb_thread_proc(&mb_comm_ctx);
    }

    db_msg_event_api_destory();
    db_msr_msg_event_api_destory();
    mb_server_destory(&mb_comm_ctx);
    rt_database_deattach();

    return 0;
}

