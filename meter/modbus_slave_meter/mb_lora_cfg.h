/*
* Copyright (c) 2019,珠海科技股份有限公司
* All rights reserved.
*
* 文件名称：modbusconfig.h
* 文件标识：
* 描    述：
*
* 当前版本：V1.0
* 作    者：
* 更改说明：
* 完成日期：20190530
* 运行环境：
操作系统: 
硬件平台：
固件库  ：V3.1.2
* 修改记录
*  作者       时间         版本      修改描述
*  chenzy   2019.09.18      V1.01       1. 增加modbus设备描述符的接口获取
                                        2.
*/

#ifndef _MB_LORA_CFG_H_
#define _MB_LORA_CFG_H_

#include <stdint.h>
#include "mb_com_baseclass.h"

int config_lora_load(struct mb_optarg_t  *pLora, char *file);
void config_lora_default(struct mb_optarg_t  *pLora);

#endif
