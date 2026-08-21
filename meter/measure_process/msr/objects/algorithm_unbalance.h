
/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_unbalance.h
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
#ifndef _ALGORITHM_UNBALANCE_H__
#define _ALGORITHM_UNBALANCE_H__
#include <stdint.h>
#include <time.h>

#include "db_struct.h"


//int32_t UnbalanceVoltJudge(void *pargs, struct db_powerquality_unbalance_type *out, uint16_t debug);
//int32_t UnbalanceVoltCal(void *pargs, struct db_powerquality_unbalance_type *out, uint16_t debug);
//int32_t UnbalanceCurrentJudge(void *pargs, struct db_powerquality_unbalance_type *out, uint16_t debug);
//int32_t UnbalanceCurrentCal(void *pargs, struct db_powerquality_unbalance_type *out, uint16_t debug);
int32_t rms_algorithm_Unbalance(struct db_database_measure_base_type *pMeasureBase, struct db_powerquality_unbalance_type *out, uint32_t pt, uint32_t ct);


#endif

