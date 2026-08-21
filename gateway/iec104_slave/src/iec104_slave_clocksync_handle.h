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


#ifndef __IEC104_SLAVE_CLOCKSYNC_HANDLE_H__
#define __IEC104_SLAVE_CLOCKSYNC_HANDLE_H__

#include <stdio.h>
#include "cs104_slave.h"
#include "hal_thread.h"
#include "hal_time.h"


bool iec104_slave_clockSyncHandler(void *parameter, IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime);


#endif /* CS104_SERVER_CLOCKSYNC_HANDLE_H*/

