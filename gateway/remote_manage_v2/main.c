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

#include <stdio.h>
#include <string.h> 
#include <sys/types.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/stat.h>  //stat
#include <time.h>
#include <stdbool.h>

#include "db_device.h"
#include "rm_cfg_attr.h"
#include "db_node_fops.h"
#include "mqtt_fops.h"

#define RM_CFG_FILE "/userdata/config/project/remote_m.json"
#define RM_CFG_PROJECT_FILE "/userdata/config/project/project.json"


// shows how to use this program
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
	fprintf(stderr,"  -d                  : debug\n");
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
    void     *vmem;
    int      opt;
    char     dev[32];

    struct remote_m_ctx_t ctx = {
        .debug = false,
        .sn = "ECHO1022301010000",
        .version = "not set",
        .attr = {
            .checked = 1,
            .url = "taso.energyun.com.cn:1883",
            //.url = "14.115.88.120:1883",
            .user = "pilot",
            .passwd = "pilot@123456", 

            .ip = "taso.energyun.com.cn",
            //.ip = "14.115.88.120",
            .port = 1883,
        },
        .vpn_starttime = 0,
        .vpn_abort = false,
    };

    if(argc >= 2)
    {
        while((opt=getopt(argc, argv, "v:n:dh")) != -1)
        {
            switch(opt)
            {
                case 'd':
                    ctx.debug = 1;
                break;
                case 'n':
                    snprintf(ctx.sn, sizeof(ctx.sn) - 1, "%s", optarg);
                break;
                case 'v':
                    snprintf(ctx.version, sizeof(ctx.version) - 1, "%s", optarg);
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

    ctx.pDB = (struct db_spl_realtime_data_tbl_t *)vmem;

    /* database fops init */
    DataMgmtFops.init(vmem);

    // get cfg
    rm_cfg_load(&ctx, RM_CFG_FILE);
    if(ctx.debug)
    {
        printf("checked:        %05d\n", ctx.attr.checked);
        printf("url:            %s\n", ctx.attr.url);
        printf("ip:             %s\n", ctx.attr.ip);
        printf("port:           %d\n", ctx.attr.port);
        printf("user:           %s\n", ctx.attr.user);
        printf("passwd:         %s\n", ctx.attr.passwd);
    }

    rm_cfg_project_info_load(&ctx, RM_CFG_PROJECT_FILE);

//    ctx.hal_com.dump =  ctx.debug ;
//    snprintf(dev, sizeof(dev), "%s", "/dev/ttyNM_MQTT");
//    ctx.hal_com.dev = dev;
//    ctx.hal_com.baud = 115200;
//    ctx.hal_com.databit = 8;
//    ctx.hal_com.paritybit = 'N';
//    ctx.hal_com.stopbit = 1;
//    ctx.hal_com.pcombase = hal_com_serial_ops_get();

    ctx.hal_com.dump =  ctx.debug ;
    snprintf(dev, sizeof(dev), "%s", "127.0.0.1:502");
    ctx.hal_com.dev = dev;
    ctx.hal_com.port = 502;
    ctx.hal_com.baud = 115200;
    ctx.hal_com.databit = 8;
    ctx.hal_com.paritybit = 'N';
    ctx.hal_com.stopbit = 1;
    ctx.hal_com.pcombase = hal_com_tcpclient_ops_get();
    
    ctx.hal_com.pcombase->init(&ctx.hal_com);
    ctx.hal_com.pcombase->connect(&ctx.hal_com);

    // MQTT init    
    mqtt_init(&ctx);

    /* loop */
    mqtt_process(&ctx);


    mqtt_destroy(&ctx);
    rt_database_deattach();

    return 0;
}