
/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_extremum.h
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

#ifndef _ALGORITHM_EXTREMUM_H_
#define _ALGORITHM_EXTREMUM_H_

#include <stdint.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "db_struct.h"

struct rms_extreme_type
{
    uint8_t *realtime_val;
    uint8_t *extreme_val;
    uint8_t realtime_len;
    uint8_t extreme_len;
    uint32_t offset;
};


typedef enum {
    U16EXTREME = 0,
    S16EXTREME,
    U32EXTREME,
    S32EXTREME,
    U64EXTREME,
    S64EXTREME,
}enum_extreme_datatype;

struct extreme_offset_list_type
{
    uint16_t realtime_offset;
    uint16_t extreme_offset;
    enum_extreme_datatype extreme_datatype;
    int (*extreme_compare)(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset);
    int (*extreme_clear)(uint8_t *pstVmem, struct extreme_offset_list_type *p_extreme_offset);
};

int extreme_clear(uint8_t *pstVmem);
int32_t rms_algorithm_extremum(struct db_database_measure_base_type *pMeasureBase, struct db_statistical_extremum_type *out, uint32_t pt, uint32_t ct);

#endif
