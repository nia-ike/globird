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

#ifndef __IEC104_SLAVE_ASUD_PACKETANDSEND_H__
#define __IEC104_SLAVE_ASUD_PACKETANDSEND_H__

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "cs104_slave.h"

#include "hal_thread.h"
#include "hal_time.h"


#if 0
void iec104_slave_ASDUSend(CS101_AppLayerParameters alParams,IMasterConnection connection,CS104_Slave slave,ST_FW_NODE_ATTR* pNodeAttr,CS101_CauseOfTransmission cot);
void iec104_slave_measuredValueShortASDUSend(CS101_AppLayerParameters alParams,IMasterConnection connection,CS104_Slave slave,struct cs104_node_data* pNode,int nodeNum,CS101_CauseOfTransmission cot);
void iec104_slave_CP56Time2a_measuredValueShortASDUSend(CS101_AppLayerParameters alParams,IMasterConnection connection,CS104_Slave slave,struct cs104_node_data* pNode,int nodeNum,uint64_t timestamp,CS101_CauseOfTransmission cot);
void iec104_slave_singlePointASDUSend(CS101_AppLayerParameters alParams,IMasterConnection connection,CS104_Slave slave,struct cs104_node_data* pNode,int nodeNum,CS101_CauseOfTransmission cot);
void iec104_slave_CP56Time2a_singlePointASDUSend(CS101_AppLayerParameters alParams,IMasterConnection connection,CS104_Slave slave,struct cs104_node_data* pNode,int nodeNum,uint64_t timestamp,CS101_CauseOfTransmission cot);
#endif
#endif /*IEC104_ASUD_PACKETANDSEND_H*/