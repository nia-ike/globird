/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_energy.h
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


#ifndef _ALGORITHM_ENERGY_REALTIME_HP_H_
#define _ALGORITHM_ENERGY_REALTIME_HP_H_

#include <stdint.h>
#include "db_struct.h"


//struct db_realtime_energy_type  m_ReMainlEnergy;//剩余原始电度

int32_t rms_algorithm_RealtimeEnergy_hp(void *pargs, struct db_realtime_energy_type *out,  void *pNet, uint32_t pt, uint32_t ct);



#endif
