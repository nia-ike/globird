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
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>  //stat
#include <time.h>
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <math.h>
#include "msr_process.h"
#include "db_struct.h"
#include "db_device.h"
#include "db_msr_msgevent_api.h"
#include "db_msgevent_api.h"
#include "Rn7302File.h"


/*************************************************************************/
/** Function Name:                                                      **/
/** Purpose:                                                            **/
/** Params:            None.                                            **/
/** Return:            None.                                            **/
/** Notice:            None.                                            **/
/*************************************************************************/
static void usage(char *_name)
{
	fprintf(stderr,"\nUsage: %s [options] \n",_name);
    fprintf(stderr,"  -d                  : debug. print message \n");
    fprintf(stderr,"  -t                  : test.  \n");
    fprintf(stderr,"  -c                  : choose ENERGY CHIP. 0-chip1(grid)  1-chip2(hybird) 2-chip3(pv)\n");
    fprintf(stderr,"  -r                  : refresh time  ms,for example:10.  \n");
    fprintf(stderr,"  -n N                : set max message channels (default: 2, must match db_process -c N)\n");
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
    int     ret;
    void    *pVmem;

    /* 
        realtime database attach.
        please insmod "vmem.ko" first. 
        all the proccess use the RealTime data and some other data if come from /dev/vmem (size :256KB)
    */
    int     opt;
    uint8_t max_msg_channels = 2;  // 默认2个通道，需与 db_process 保持一致
    
    struct  measure_ctx  ctx = {
        .test = 0,
        .debug = 0,
        .refresh = 10,
        .chip = 0,
        .fd_device = -1,
    };
    if(argc >= 2)
    {
        while((opt=getopt(argc, argv, "c:d:t:r:n:h")) != -1)
        {
            switch(opt)
            {
                case 'd':
                    ctx.debug = atoi(optarg);
                break;
                case 't':
                    ctx.test = atoi(optarg);
                break;
                case 'r':
                    ctx.refresh = atoi(optarg);;
                break;
                case 'c':
                    ctx.chip = atoi(optarg);
                break;
                case 'n':
                    max_msg_channels = (uint8_t)atoi(optarg);
                    if(max_msg_channels < 1 || max_msg_channels > 32) {
                        fprintf(stderr, "Error: max_channels must be between 1 and 32\n");
                        exit(1);
                    }
                    printf("Set max message channels to: %d\n", max_msg_channels);
                break;
                case 'h':
                {
                    usage(argv[0]);
                    exit(0);
                    break;
                }
                default:
                    break;
            }
        }
    }
    // 初始化数据库
    pVmem = rt_database_attach();
    if(pVmem == NULL)
        goto __database_err;
    ctx.pdb = pVmem;

    /* 
     * 重要：必须在 msr_init 之前配置消息通道！
     * 因为 msr_init 会创建 RMSTimerThread 线程，
     * 该线程会立即调用 db_msr_msg_event_cmd 发送消息
     */
    ret = db_msg_event_api_init();
    if(ret < 0)
        goto __db_msg_event_api_init_err;

    /* 配置消息通道数（必须在 msr_init 之前调用） */
    printf("Configuring message API with %d channels\n", max_msg_channels);
    ret = db_msr_msg_event_api_config(max_msg_channels);
    if(ret < 0)
    {
        printf("Failed to configure message channels\n");
        goto __db_msg_event_api_init_err;
    }

    ret = db_msr_msg_event_api_init(ctx.chip);
    if(ret < 0)
        goto __db_msr_msg_event_api_init_err;

    // 打开rn7302设备文件
    ret = MeterChipInit(ctx.chip);
    if(ret < 0)
        goto __msr_init_err;
    // 保存文件描述符
    ctx.fd_device = ret;
    /*
        msr processer. this is a loop 
    */
   ret = msr_init(&ctx);
    if(ret < 0)
        goto __msr_init_err;

    msr_thread_proc(&ctx);

    /*
        before exit, release all the resource.
    */
__database_err:
    rt_database_deattach();

__msr_init_err:
    MeterChipDestory();

__db_msg_event_api_init_err:
    db_msg_event_api_destory();

__db_msr_msg_event_api_init_err:
    db_msr_msg_event_api_destory();


    return 0;
}