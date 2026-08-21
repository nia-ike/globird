/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_halfwave_reactivepower.h
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

#ifndef _ALGORITHM_HALFWAVE_REACTIVEPOWER_H_
#define _ALGORITHM_HALFWAVE_REACTIVEPOWER_H_

#include <stdint.h>
#include "db_struct.h"

int32_t rms_algorithm_halfwave_reactivepower(struct db_database_measure_base_type *pMeasureBase, int64_t *out1, int64_t *out2, int64_t *out3, int64_t *out4, int64_t input1, int32_t input2, int64_t input3, int64_t input4, uint32_t pt, uint32_t ct, uint32_t time);



#endif
