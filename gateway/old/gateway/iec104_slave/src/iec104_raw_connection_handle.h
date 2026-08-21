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


#ifndef __IEC104_RAW_CONNECTION_HANDLE_H__
#define __IEC104_RAW_CONNECTION_HANDLE_H__

#include <stdint.h>
#include <stdio.h>
#include "cs104_connection.h"
#include "cs104_slave.h"

void iec104_slave_clientRawMessageHandler (void* parameter, uint8_t* msg, int msgSize, bool sent);
void iec104_slave_serverRawMessageHandler(void* parameter, IMasterConnection conneciton, uint8_t* msg, int msgSize, bool sent);
void iec104_slave_connectionHandler (void* parameter, CS104_Connection connection, CS104_ConnectionEvent event);
bool iec104_slave_connectionRequestHandler(void* parameter, const char* ipAddress);
void iec104_slave_connectionEventHandler(void* parameter, IMasterConnection con, CS104_PeerConnectionEvent event);

#endif /*__IEC104_RAW_CONNECTION_HANDLE_H__*/