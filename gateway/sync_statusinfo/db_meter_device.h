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

#ifndef __DB_METER_DEVICE_H__
#define __DB_METER_DEVICE_H__



void *rt_meter_database_attach(void);
void rt_meter_database_deattach(void);
void *rt_meter_db_getVmem(void);



#endif
