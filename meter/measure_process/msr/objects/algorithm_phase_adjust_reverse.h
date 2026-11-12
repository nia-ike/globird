/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_phase_adjust_reverse.h
*  描    述：PhaseAdjust bit15 极性反向——软件翻功率符号（电流/电压模式共用）
*
*  当前版本：V1.1
*/

#ifndef _ALGORITHM_PHASE_ADJUST_REVERSE_H_
#define _ALGORITHM_PHASE_ADJUST_REVERSE_H_

#include <stdint.h>
#include "db_struct.h"

/**
 * @brief : 按 PhaseAdjust1~3 的 bit15 翻各相 P/Q/PF 符号（P3W4）
 * @note  : 芯片 ICHSEL/UCHSEL 不接收 bit15；电流/电压模式均需本函数校正功率方向。
 *          半波 phasecheck_on（用含 +180° 的相角算功率）路径不要调用，避免双反。
 */
int32_t rms_algorithm_PhaseAdjustReverse(struct db_database_measure_base_type *pMeasureBase,
                                         struct db_basic_realtime_type *out,
                                         uint32_t pt,
                                         uint32_t ct);

#endif
