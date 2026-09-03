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

#ifndef __DB_MSG_PROCESS_H__
#define __DB_MSG_PROCESS_H__

#include <stdio.h>
#include <string.h> 
#include <sys/types.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/stat.h>  //stat
#include <time.h>
#include <linux/input.h>
#include <pthread.h>

#include "db_cfg.h"
#include "db_device.h"
#include "db_struct.h"
#include "db_measure.h"




/*
    numbers of timer
*/
#define DB_TIMER_NUM_MAX				(4)

/* 
    "power off dectet" is working KEYBOARD mode 
 */
#define DB_KEY_DEV         				"/dev/input/event0"
#define DB_KEY_EVENT_TYPE                  (1)
#define DB_KEY_EVENT_CODE_PWROFF           (11)
#define DB_KEY_EVENT_CODE_DOWN             (40)
#define DB_KEY_EVENT_CODE_ENTER            (13)
#define DB_KEY_EVENT_PRESSED               (1)
#define DB_KEY_EVENT_RELEASE               (0)


struct db_msg_timer_event_t
{
	uint8_t		idx;
};


int db_msg_event_init(void);
int db_msg_settimer(uint8_t id, uint32_t timeout);
int db_msg_destorytimer(uint8_t id);
void db_msg_event_destory(void);


#endif
