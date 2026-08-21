/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_loadrun.h
*  文件标识：
*  描    述：负载运行
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

#ifndef _ALGORITHM_LOADRUN_H_
#define _ALGORITHM_LOADRUN_H_

#include <stdint.h>
#include "db_struct.h"

enum
{
    OBJ_NULL = 0,
    OBJ_VOLTAGE ,   //电压
    OBJ_CURRENT ,   //电流
    OBJ_POWER   ,   //总有功功率
};

int32_t rms_algorithm_loadrun(struct db_database_measure_base_type *pMeasureBase, struct db_statistical_loadrun_type *out, uint32_t pt, uint32_t ct);
int32_t LoadrunInit(struct db_database_measure_base_type *pMeasureBase);

#endif
