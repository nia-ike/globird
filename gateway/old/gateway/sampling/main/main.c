/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：Interface.c
*  文件标识：
*  描    述：SimpleGUI接口
*
*  当前版本：V1.0
*  作    者：chenzy
*  运行环境：
*  操作系统: linux
*  硬件平台：f1c100s
*  固件库  ：
*  修改记录
*  作者       时间         版本      修改描述
*  chenzy   2020       V1.00
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
#include "db_struct.h"
#include "db_device.h"

#include "sampling_cfg.h"
#include "sampling_channelpoll.h"
#include "config_devices.h"
#include "db_node_fops.h"
#include "debug.h"
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
    fprintf(stderr,"\nexample: ./collect_process              => working with cfg file \n");
    fprintf(stderr,"\nexample: ./collect_process [option]     => working with cmd parameters \n");

    fprintf(stderr,"  -p 502              : set modbusTCP remote port(client mode) \n");
    fprintf(stderr,"  -n sn               : set sn.  \n");
    fprintf(stderr,"  -f cfg file path    : select the cfg file path \n");
    fprintf(stderr,"  -c channel no       : channel no \n");
    fprintf(stderr,"  -s                  : do nothing. just use for space \n");
    fprintf(stderr,"  -d                  : enable debug \n");
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
    int     opt;
    void    *debugMsg;
    struct db_spl_realtime_data_tbl_t *pVmem;
    struct channelpoll_mgr_t ChnlPollMgr;
    struct sampling_optarg_t    optarg_mgr = {
        .sn = 0,
        .tcp_port = 502,
        .dump = 0xff,
        .channl_no = 0,
        .cfg_file = "",
    };

    if(argc >= 2)
    {
        while((opt=getopt(argc, argv, "hp:n:df:c:s")) != -1)
        {
            switch(opt)
            {
                case 'p':
                    optarg_mgr.tcp_port = atoi(optarg);
                break;
                case 'n':
                    optarg_mgr.sn = atoi(optarg);
                break;
                case 'd':
                    optarg_mgr.dump = 1;
                break;
                case 'f':
                    snprintf(optarg_mgr.cfg_file, sizeof(optarg_mgr.cfg_file), "%s", optarg);
                break;
                case 'c':
                    optarg_mgr.channl_no = atoi(optarg);
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
    pVmem = rt_database_attach();
    if(pVmem == NULL)
    {
        printf("please insert /dev/vmem.ko first\n");
        goto __rt_database_err;

    }

    debugMsg = log_open(NULL);
    if(NULL == debugMsg)
    {
        printf("please insert /dev/log.ko first\n");
        goto __rt_database_err;
    }

    /*
        load the setting from json to RAM structure.
    */
    ChnlPollMgr.pChannelAttr = config_channel_init(optarg_mgr.cfg_file);
    if(!ChnlPollMgr.pChannelAttr)
    {
        printf("Channel Setting init fail!, just dead loop.\n");
        goto __rt_log_err;
    }

    /* 装载实时数据库 */
    ChnlPollMgr.pVmem = pVmem;
    
    /* 运行命令参数 */
    ChnlPollMgr.poptarg = &optarg_mgr;
    /*
        init sampling realtime databuffer module. 
        
        this depend on the Channel Setting .
        so this function MUST be called after config_channel_init();
    */    
    DataMgmtFops.init(pVmem);

    /* channel_start*/
    ChnnlPoll_channel_start((struct db_database_type *)pVmem, &ChnlPollMgr);

    /* loop for ever */
    ChnnlPoll_thread_entry(&ChnlPollMgr);

__rt_log_err:
    log_close();

__rt_database_err:
    rt_database_deattach();

    return 0;
}
