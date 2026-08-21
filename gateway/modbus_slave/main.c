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
#include <signal.h>

#include "db_device.h"
#include "db_struct.h"

#include "mb_cfg.h"
#include "mb_com_baseclass.h"
#include "mb_process.h"
#include "mb_protocol.h"

#include "dev_log.h"

#include "db_node_fops.h"


/*
    接口结构体，用于形成协议或者物理接口的表格和对应接口
*/
struct iface_table
{
    char        name[32];
    void       *fops;
};




static const struct iface_table m_iface[] = {
    { "serial",         mb_com_serial_ops_get},
    { "tcp client",     mb_com_tcpclient_ops_get},
    { "stdinout",       mb_com_stdinout_ops_get},
    { "tcp server",     mb_com_tcpserver_ops_get},
    { "udpboardcast",   mb_com_udpboardcast_ops_get},
};

static const struct iface_table m_protocol[] = {
    { "modbusRTU",      mb_msg_process_rtu},
    { "modbusTCP",      mb_msg_process_tcp}
};



/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int log_abort_cb(void *arg, int signum)
{
    return mb_protocol_tcp_is_log_frame(arg, signum);
}


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

//    fprintf(stderr,"  -m 0/1/2/3//7    : set mode. 0-serial 1-tcpclient 2-stdin.stdout 3-tcpserver. 7-modbusRTU over tcpserver 8-modbusRTU over UDP boardcast default 0\n");
    fprintf(stderr,"  -i serial           : set phy interface. serial/tcpclient/stdinout/tcpserver/udpboardcast... \n");
    fprintf(stderr,"  -e modbusTCP        : set encode protocol. modbusTCP/modbusRTU/... \n");
    fprintf(stderr,"  -b 9600             : set baudrate. work only mode=0 \n");
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


void record_signo(int signo)
{
	
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
static struct mb_com_baseclass_ops_t *get_pcomm_fops(char *name, uint8_t *mode)
{
    int i;
    mb_com_ops_get pfunc;
    
    for(i=0; i<sizeof(m_iface)/sizeof(struct iface_table); i++)
    {
        if(!strcmp(m_iface[i].name, name))
        {
            pfunc = m_iface[i].fops;
            *mode = i;
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
static void *get_protocol_fops(char *name)
{
    int i;
    
    for(i=0; i<sizeof(m_protocol)/sizeof(struct iface_table); i++)
    {
        if(!strcmp(m_protocol[i].name, name))
            return m_protocol[i].fops;
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
int main(int argc, char **argv)
{
    static struct config_chnl_working_attr mb_cfg_chnl_working_attr;
    
    void    *debugMsg;
    struct db_spl_realtime_data_tbl_t *vmem;
    int     opt;
    int     ret, chnl;
    int     baud = 0;
    int     paritybit = 0;
    int     databit = 0;
    int     stopbit = 0;

    char        iface[32] = {0};
    char        protocol[32] = {0};
    int     port = 502;
    uint32_t    keepintvl = 120;
    char        url[64] = {0};
    char        cfg_file[64] = "/userdata/config/sample/channels/chnl_1.json";
    char        regPacket[24] = {0};
    
    mb_cfg_chnl_working_attr.mb_comm_ctx.running = true;
    mb_cfg_chnl_working_attr.mb_comm_ctx.dump = false;
    mb_cfg_chnl_working_attr.debug = false;
    mb_cfg_chnl_working_attr.mb_comm_ctx.mode = 0;

    if(argc >= 2)
    {
        while((opt=getopt(argc, argv, "hb:p:k:s:r:f:n:d:i:e:gc:")) != -1)
        {
            switch(opt)
            {
                case 'c':
                    if(strcmp("server_modbusTCP", optarg) == 0)
                    {
                        // 主通道，强制chnl = 1
                        chnl = 1;
                    }
                    else 
                    {
                        chnl = atoi(optarg);
                    }
                    
                break;
                case 'b':
                    baud = atoi(optarg);
                break;
                case 'i':
                    snprintf(iface, sizeof(iface), "%s", optarg);
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
                    mb_cfg_chnl_working_attr.mb_comm_ctx.sn = Sn_str2u32(optarg);
                    snprintf(mb_cfg_chnl_working_attr.sn, sizeof(mb_cfg_chnl_working_attr.sn), "%s", optarg);
                break;
                case 'd':
                    mb_cfg_chnl_working_attr.mb_comm_ctx.dump = atoi(optarg);
                    mb_cfg_chnl_working_attr.debug = mb_cfg_chnl_working_attr.mb_comm_ctx.dump;
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

    /*
        prepare the modbus register map table and some other configure
        转发主通道，log需要处理log 递归问题，
        因为502端口为主数据通道，调试使用该通道获取log， 而log本身也是从主通道获取的
        如果不限制，就会导致：
        上位机读取log， 读的报文也会形成log， 这个log报文也会形成log， 这样递归下去。所以需要回调中填入判断为读log
        的功能码，就不写log。
    */
    if(chnl == 1)
        debugMsg = log_open(log_abort_cb);
    else 
        debugMsg = log_open(NULL);

    if(NULL == debugMsg)
    {
        printf("debug log open fail!\r\n");
        rt_database_deattach();
        return -1;
    }

    DataMgmtFops.init(vmem);

    mb_cfg_project_load(&mb_cfg_chnl_working_attr, "/userdata/config/project/project.json");
    
    if(strcmp(iface, "udpboardcast"))
    {
        ret = mb_config_init(&mb_cfg_chnl_working_attr, cfg_file);
        if(ret < 0)
        {
            printf("cfg file parse fail\n");
            log_close();
            rt_database_deattach();
            return -1;
        }
    }
    
    
    /* 命令行无设置时，采用配置文件值 */
    if(iface[0] == 0){
        snprintf(iface, sizeof(iface), "%s", mb_cfg_chnl_working_attr.link);
    }

    if(url[0] == 0)
    {
        sscanf(mb_cfg_chnl_working_attr.dev, "%[^:]:%d", (char*)&url, (int *)&port);
    }

    if(protocol[0] == 0)
        snprintf(protocol, sizeof(protocol), "%s", mb_cfg_chnl_working_attr.protocol);

    if(baud == 0)
        baud = mb_cfg_chnl_working_attr.serial_attr.baud ;

    mb_cfg_chnl_working_attr.mb_comm_ctx.fd = -1;
    mb_cfg_chnl_working_attr.mb_comm_ctx.fd_listen = -1;
    mb_cfg_chnl_working_attr.mb_comm_ctx.fd_max = -1;
    mb_cfg_chnl_working_attr.mb_comm_ctx.dev = url;
    mb_cfg_chnl_working_attr.mb_comm_ctx.port = port;
    mb_cfg_chnl_working_attr.mb_comm_ctx.keepidle = keepintvl;
    mb_cfg_chnl_working_attr.mb_comm_ctx.keepintvl = keepintvl;
    mb_cfg_chnl_working_attr.mb_comm_ctx.keepcnt = 2;
    mb_cfg_chnl_working_attr.mb_comm_ctx.baud = baud;
    mb_cfg_chnl_working_attr.mb_comm_ctx.databit = databit;
    mb_cfg_chnl_working_attr.mb_comm_ctx.paritybit = paritybit;
    mb_cfg_chnl_working_attr.mb_comm_ctx.stopbit = stopbit;
    mb_cfg_chnl_working_attr.mb_comm_ctx.pvmem = vmem;
    mb_cfg_chnl_working_attr.mb_comm_ctx.channel_no = chnl;
    mb_cfg_chnl_working_attr.mb_comm_ctx.dev_no = 1;
    mb_cfg_chnl_working_attr.mb_comm_ctx.rs485_addr = mb_cfg_chnl_working_attr.rs485_addr;
    mb_cfg_chnl_working_attr.mb_comm_ctx.pcombase = get_pcomm_fops(iface, &mb_cfg_chnl_working_attr.mb_comm_ctx.mode);
    mb_cfg_chnl_working_attr.mb_comm_ctx.msg_protocol_process = get_protocol_fops(protocol);
    snprintf((char*)mb_cfg_chnl_working_attr.mb_comm_ctx.regPacket, sizeof(mb_cfg_chnl_working_attr.mb_comm_ctx.regPacket), "%s", regPacket);
    

    if(!strcmp(iface, "tcp client"))
        mb_cfg_chnl_working_attr.mb_comm_ctx.mode = 1;

    if(!strcmp(iface, "stdinout"))
    {
        /*
            stdin stdout, raw mode. all the signal include ctrl+c ctrl+a...z, is data. so wen need set up signal
            process must exit by 'q'
        */
        signal(SIGHUP, record_signo);
        signal(SIGINT, record_signo);
        signal(SIGTERM, record_signo);
        signal(SIGPIPE, record_signo);
    }

    mb_cfg_chnl_working_attr.mb_comm_ctx.pcombase->init(&mb_cfg_chnl_working_attr.mb_comm_ctx);

    /* modbus server */
    mb_server_init(&mb_cfg_chnl_working_attr.mb_comm_ctx);

    while(mb_cfg_chnl_working_attr.mb_comm_ctx.running)
    {
        mb_thread_proc(&mb_cfg_chnl_working_attr.mb_comm_ctx);
    }

    mb_server_destory(&mb_cfg_chnl_working_attr.mb_comm_ctx);
    log_close();
    rt_database_deattach();

    return 0;
}

