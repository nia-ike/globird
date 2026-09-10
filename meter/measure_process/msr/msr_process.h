
/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：msr_process.c
*  文件标识：
*  描    述：
*
*  当前版本：V1.0
*  作    者：raoy
*  运行环境：
*  操作系统: linux
*  硬件平台：f1c100s
*  固件库  ：
*  修改记录
*  作者       时间         版本      修改描述
*  raoy      2021       V1.00
*/

#ifndef __MSR_PROCESS_H__
#define __MSR_PROCESS_H__

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <math.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include  "debug.h"


#define    STATE_POWER_OFF    (1 << 1) //����״̬


/* 毫秒级延时 */
#define MSR_PORT_DELAY(ms)    usleep(ms * 1000)



struct measure_ctx
{
    int                             test;
    int                             debug;
    int32_t                         refresh;
    int                             chip;

    int                             fd_device;
    /*
        总数据和参数集合(驱动形态的 共享内存)
    */
    struct db_database_type         *pdb;
};
//=======================================================================//
//= Public variable declaration.                                        =//
//=======================================================================//



//=======================================================================//
//= struct declaration.                                        =//
//=======================================================================//




//=======================================================================//
//= Function declare.                                                   =//
//=======================================================================//

/* ioctrl */
#define RN7302_IOC_MAGIC 'R'
enum
{
    RN7302_IOCTL_WAVEFORM_START = _IO(RN7302_IOC_MAGIC, 1),
    RN7302_IOCTL_SOFTWARE_RST = _IO(RN7302_IOC_MAGIC, 2),
    RN7302_IOCTL_HARDWARE_RST = _IO(RN7302_IOC_MAGIC, 3),
    RN8306_IOCTL_WAVEFORM_READ = _IO(RN7302_IOC_MAGIC, 4),
};


int32_t msr_init(struct measure_ctx *pctx);

void *msr_thread_proc(struct measure_ctx *pctx);

#endif // __MSR_PROCESS_H__
