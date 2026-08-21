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


#include "iec104_slave_clocksync_handle.h"

/*********************************************************************************************************
** Function name:
** Descriptions: 从主站获取时钟，并且设置到本地
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void printCP56Time2a(CP56Time2a time)
{
    printf("%02i:%02i:%02i %02i/%02i/%04i", CP56Time2a_getHour(time),
                             CP56Time2a_getMinute(time),
                             CP56Time2a_getSecond(time),
                             CP56Time2a_getDayOfMonth(time),
                             CP56Time2a_getMonth(time),
                             CP56Time2a_getYear(time) + 2000);
}

/*********************************************************************************************************
** Function name:
** Descriptions: 从主站获取时钟，并且设置到本地
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
bool iec104_slave_clockSyncHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime)
{
    printf("Process time sync command with time "); printCP56Time2a(newTime); printf("\n");

    uint64_t newSystemTimeInMs = CP56Time2a_toMsTimestamp(newTime);

    printf("newSystemTimeInMs:%lld\n", newSystemTimeInMs);

    if(Hal_setTimeInMs(newSystemTimeInMs) == false){
               printf("clock sync failed!  cannot use the client time  to set local time!\n");
      }

    /* Set time for ACT_CON message */
    CP56Time2a_setFromMsTimestamp(newTime, Hal_getTimeInMs());

    /* update system time here ，可把时钟存入RTC中*/
    system("hwclock -w -u");

    return true;
}

