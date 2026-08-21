
/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_primary_to_secondary.h
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



#ifndef _ALGORITHM_PRIMARY_TO_SECONDARY_H_
#define _ALGORITHM_PRIMARY_TO_SECONDARY_H_

#include <stdint.h>
#include "db_struct.h"

int32_t rms_algorithm_Primary_to_secondary(struct db_database_measure_base_type *pMeasureBase, struct db_realtime_secondary_type *out, uint32_t pt, uint32_t ct);
int32_t rms_algorithm_fundwave_secondary(struct db_database_measure_base_type *pstVmem, struct db_realtime_secondary_type *out, uint32_t pt, uint32_t ct);
int32_t rms_algorithm_halfwave_Primary_to_secondary(struct db_database_measure_base_type *pstVmem, struct db_realtime_secondary_type *out, uint32_t pt, uint32_t ct);

#endif
