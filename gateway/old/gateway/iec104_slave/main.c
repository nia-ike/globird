/*
*  Copyright (c) 2019,广州碳索智控科技有限公司
*  All rights reserved.
* 
*  文件名称：
*  文件标识：
*  描    述：
* 
*  当前版本：V1.0
*  作    者：chenzy
*  运行环境：
*  操作系统: linux
*  硬件平台：
*  固件库  ：
*  修改记录 
*  作者       时间         版本      修改描述
*  chenzy          V1.00      
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

//接入lib60870库各种api及定义
#include "cs104_connection.h"
#include "cs104_slave.h"
#include "hal_time.h"
#include "hal_thread.h"

#include "iec104_slave_config.h"
#include "iec104_slave_hal_client.h"
#include "iec104_com_baseclass.h"
#include "iec104_cfg.h"

#include "db_node_fops.h"
#include "db_device.h"
#include "db_struct.h"

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

//    fprintf(stderr,"  -m 0/1/2/3//7    : set mode. 0-serial 1-tcpclient 2-stdin.stdout 3-tcpserver. 7-modbusRTU over tcpserver default 0\n");
    fprintf(stderr,"  -i serial           : set phy interface. serial/tcpclient/stdinout/tcpserver/... \n");
    fprintf(stderr,"  -e modbusTCP        : set encode protocol. modbusTCP/modbusRTU/... \n");
    fprintf(stderr,"  -m 1                : set mode.  mode=0 client  mode=1 server\n");
    fprintf(stderr,"  -s pmac.com.cn      : set server url(IP). or /dev/xxx, for example: /dev/sx127x0 \n");
    fprintf(stderr,"  -p 502              : set port. work only mode=1 \n");
    fprintf(stderr,"  -k 120              : set tcp keepalive inv. work only mode=1 \n");
    fprintf(stderr,"  -r register content : set tcp register packet. work only mode=1 \n");
    fprintf(stderr,"  -f cfg file         : set cfg file \n");
    fprintf(stderr,"  -n sn               : set sn. action only in lora mode[-m 5] \n");
    fprintf(stderr,"  -c chnl no          : set chnl number \n");
    fprintf(stderr,"  -g                  : do nothing.  \n");
    fprintf(stderr,"  -d 0/1              : debug. printf tx,rx.  0-dump 485  1-dump lora \n");
	fprintf(stderr,"  -h                  : Show this help message\n");
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

	if(strstr(psn, "TASO") == NULL)
		return 0;

	psn = psn + (sizeof("TASO") - 1); // -1 . "EGOM" have a '0'

	// drop the 7th char
	strncpy(sn, psn, 6);		// get the first 6 char
	strncpy(sn+6, psn+7, 4);    // get the last  4 char
	sn[10] = '\0';
	
	return strtoul(sn, 0, 10);
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
    static struct config_chnl_working_attr iec104_cfg_chnl_working_attr;

    void    *debugMsg;
    struct db_spl_realtime_data_tbl_t *vmem;
    int     opt;
    int     ret, chnl;
    int     mode = 1;
    char    protocol[32] = {0};
    int     port = 2404;
    uint32_t    keepintvl = 120;
    char        url[64] = "";
    char        cfg_file[64] = "/userdata/config/sample/channels/chnl_2.json";
    char        regPacket[24] = {0};
    
    iec104_cfg_chnl_working_attr.iec104_comm_ctx.running = true;
    iec104_cfg_chnl_working_attr.iec104_comm_ctx.dump = false;
    iec104_cfg_chnl_working_attr.debug = false;

    if(argc >= 2)
    {
        while((opt=getopt(argc, argv, "hp:m:k:s:r:f:n:d:i:e:gc:")) != -1)
        {
            switch(opt)
            {
                case 'c':
                    // do nothing
                    chnl = atoi(optarg);
                break;
                case 'm':
                    mode = atoi(optarg);
                break;
                case 'e':
                    snprintf(protocol, sizeof(protocol), "%s", optarg);
                break;
                case 'p':
                    port = atoi(optarg);
                break;
                case 'k':
                    keepintvl = atoi(optarg);
                break;
                case 's':
                    strncpy(url, optarg, sizeof(url)-1);
                break;
                case 'f':
                    strncpy(cfg_file, optarg, sizeof(cfg_file)-1);
                break;
                case 'r':
                    strncpy(regPacket, optarg, sizeof(regPacket)-1);
                break;
                case 'n':
                    iec104_cfg_chnl_working_attr.iec104_comm_ctx.sn = Sn_str2u32(optarg);
                break;
                case 'd':
                    iec104_cfg_chnl_working_attr.iec104_comm_ctx.dump = atoi(optarg);
                    iec104_cfg_chnl_working_attr.debug = iec104_cfg_chnl_working_attr.iec104_comm_ctx.dump;
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
    vmem = (struct db_spl_realtime_data_tbl_t *)rt_database_attach();
    if(vmem == NULL)
    {
        printf("please insert /dev/vmem.ko first\n");
        return -1;
    }

    debugMsg = log_open(NULL);
    if(NULL == debugMsg)
    {
        printf("debug log open fail!\r\n");
        rt_database_deattach();
        return -1;
    }

    /* database fops init */
    DataMgmtFops.init(vmem);
    /* iec104 table remap load */
    ret = iec104_config_init(&iec104_cfg_chnl_working_attr, cfg_file);
    if(ret < 0)
    {
        printf("cfg file parse fail\n");
        rt_database_deattach();
        return -1;
    }

    if(url[0] == 0)
    {
        sscanf(iec104_cfg_chnl_working_attr.dev, "%[^:]:%d", (char*)&url[0], (int *)&port);
    }

    if(protocol[0] == 0)
        snprintf(protocol, sizeof(protocol), "%s", iec104_cfg_chnl_working_attr.protocol);

    iec104_cfg_chnl_working_attr.iec104_comm_ctx.fd = -1;
    iec104_cfg_chnl_working_attr.iec104_comm_ctx.fd_listen = -1;
    iec104_cfg_chnl_working_attr.iec104_comm_ctx.fd_max = -1;
    iec104_cfg_chnl_working_attr.iec104_comm_ctx.dev = url;
    iec104_cfg_chnl_working_attr.iec104_comm_ctx.port = port;
    iec104_cfg_chnl_working_attr.iec104_comm_ctx.keepidle = keepintvl;
    iec104_cfg_chnl_working_attr.iec104_comm_ctx.keepintvl = keepintvl;
    iec104_cfg_chnl_working_attr.iec104_comm_ctx.keepcnt = 2;
    iec104_cfg_chnl_working_attr.iec104_comm_ctx.pvmem = vmem;
    iec104_cfg_chnl_working_attr.iec104_comm_ctx.cs104_connected = false;
    snprintf(iec104_cfg_chnl_working_attr.iec104_comm_ctx.regPacket, sizeof(iec104_cfg_chnl_working_attr.iec104_comm_ctx.regPacket), "%s", regPacket);
//    iec104_cfg_chnl_working_attr.iec104_comm_ctx.baud = baud;
//    iec104_cfg_chnl_working_attr.iec104_comm_ctx.databit = databit;
//    iec104_cfg_chnl_working_attr.iec104_comm_ctx.paritybit = paritybit;
//    iec104_cfg_chnl_working_attr.iec104_comm_ctx.stopbit = stopbit;
    // end of RUNNING setting

    // start iec104 setting 
    iec104_slave_hal_client_init(&iec104_cfg_chnl_working_attr);

    switch(mode)
    {
        // client mode 作为TCP Client主动发起TCP连接
        case 0:
            CS104_Client_Slave_start(iec104_cfg_chnl_working_attr.iec104_comm_ctx.slave);
            if(CS104_Slave_isRunning(iec104_cfg_chnl_working_attr.iec104_comm_ctx.slave))
                printf("Running In Client Mode. and keep deamon running\n");
            else 
                printf("First connect fail, will reconnect periods \n");
        break;
        // server mode 作为TCP 
        case 1:
            CS104_Slave_start(iec104_cfg_chnl_working_attr.iec104_comm_ctx.slave);
            if(CS104_Slave_isRunning(iec104_cfg_chnl_working_attr.iec104_comm_ctx.slave))
                printf("Running In server Mode. and keep deamon running\n");
            else 
                printf("First server connect fail, will reconnect periods \n");
        break;

    }
    
    while(CS104_Slave_isRunning(iec104_cfg_chnl_working_attr.iec104_comm_ctx.slave))
    {
extern bool iec104_slave_spontaneous_Handler(struct config_chnl_working_attr *pChnl, IMasterConnection connection);

        
        Thread_sleep(1000);
//       if(iec104_cfg_chnl_working_attr.iec104_comm_ctx.cs104_connected == true)
           iec104_slave_spontaneous_Handler(&iec104_cfg_chnl_working_attr, NULL);
    }

    log_close();
    rt_database_deattach();
    
    return 0;

}