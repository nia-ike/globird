
/*
* Copyright (c) 2019,珠海科技股份有限公司
* All rights reserved.
* 
* 文件名称：modbusscheduler.h
* 文件标识：modbus协议的调度
* 描    述：
* 
* 当前版本：V1.0
* 作    者：wangcq
* 更改说明：
* 完成日期：20190530
* 运行环境：
操作系统: RT-Thread
硬件平台：STM32F030
固件库  ：V3.1.2
* 修改记录 
*  作者       时间         版本      修改描述
*  chenzy   2019.09.20       V1.00      创建
*/

#ifndef _MB_PROCESS_H_
#define _MB_PROCESS_H_


#include "mb_com_baseclass.h"


/*********************************************************************************************************
** Function name:       modbusrtu_server_init
** Descriptions:        modbus rtu 转发服务初始化
** input parameters:    None
** output parameters:   None
** Returned value:      0: success other:fail
*********************************************************************************************************/
void mb_thread_proc(struct mb_comm_ctx_t *pCtx);
int mb_server_init(struct mb_comm_ctx_t *pCtx);
void mb_server_destory(struct mb_comm_ctx_t *pCtx);
void mb_tcpclient_thread(void *parameter);
#endif
