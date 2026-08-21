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


#ifndef __OPC_UA_H__
#define __OPC_UA_H__

#include "opc_ua_cfg.h"

int opc_ua_server_init(struct config_chnl_working_attr *pCfgChlAttr);
int opc_ua_server_destory(struct config_chnl_working_attr *pCfgChlAttr);
void opc_ua_process(struct config_chnl_working_attr *pCfgChlAttr);


#endif
