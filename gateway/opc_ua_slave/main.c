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
#include <pthread.h>  // Add this header for thread support

// ... existing code ...
#include <signal.h>

// Add open62541 header
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>

#include "db_node_fops.h"
#include "db_device.h"
#include "db_struct.h"

#include "opc_ua_cfg.h"
#include "opc_ua.h"



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
 
    fprintf(stderr,"  -p 502              : set port. work only mode=1 \n");
    fprintf(stderr,"  -f cfg file         : set cfg file \n");
    fprintf(stderr,"  -n sn               : set sn. action only in lora mode[-m 5] \n");
    fprintf(stderr,"  -c chnl no          : set chnl number \n");
    fprintf(stderr,"  -g                  : do nothing.  \n");
    fprintf(stderr,"  -d 0/1              : debug.  \n");
	fprintf(stderr,"  -h                  : Show this help message\n");
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
    static struct config_chnl_working_attr opc_ua_cfg_chnl_working_attr;

    struct db_spl_realtime_data_tbl_t *vmem;
    int     opt;
    int     ret;
    char    cfg_file[64] = "/userdata/config/sample/channels/chnl_2.json";
    
    opc_ua_cfg_chnl_working_attr.opc_ua_comm_ctx.port = 4840;

    if(argc >= 2)
    {
        while((opt=getopt(argc, argv, "hp:f:n:d:gc:")) != -1)
        {
            switch(opt)
            {
                case 'c':
                    // do nothing
                break;
                case 'p':
                    opc_ua_cfg_chnl_working_attr.opc_ua_comm_ctx.port = atoi(optarg);
                break;
                case 'f':
                    strncpy(cfg_file, optarg, sizeof(cfg_file)-1);
                break;
                case 'n':
                    
                break;
                case 'd':
                   opc_ua_cfg_chnl_working_attr.opc_ua_comm_ctx.dump = atoi(optarg);
                   opc_ua_cfg_chnl_working_attr.debug = opc_ua_cfg_chnl_working_attr.opc_ua_comm_ctx.dump;
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

    /* database fops init */
    DataMgmtFops.init(vmem);

    /* iec104 table remap load */
    ret = opc_ua_config_init(&opc_ua_cfg_chnl_working_attr, cfg_file);
    if(ret < 0)
    {
        printf("cfg file parse fail\n");
        rt_database_deattach();
        return -1;
    }

    // opc ua server initial
    ret = opc_ua_server_init(&opc_ua_cfg_chnl_working_attr);
    if(ret < 0)
    {
        printf("opc ua init fail\n");
        rt_database_deattach();
        return -1;
    }
    // Main server loop
    opc_ua_process(&opc_ua_cfg_chnl_working_attr);

    opc_ua_server_destory(&opc_ua_cfg_chnl_working_attr);

    rt_database_deattach();
    
    return 0;

}