/**
 * @file        algorithm_power.h
 * @brief       功率计算（有功 / 无功 / 视在）
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   功率计算
 */
#ifndef ALGORITHM_POWER_H
#define ALGORITHM_POWER_H

#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include "msr_config.h"
#include "msr_data.h"
#include "msr_input.h"

/**
 * @brief   全波/基波有功 + 无功（读芯片 PA/QA…）
 *
 * 不写视在。依赖 out 中已有 Va/Ia 等：V 或 I 为 0 时清对应相 P/Q。
 *
 * @param[out] out  Pa/Pb/Pc/Ptot、Qa/Qb/Qc/Qtot
 * @param[in]  cfg  接线、PT、CT、校表
 * @param[in]  in   电参快照
 *
 * @return 0 成功；参数非法或接线非法返回 -1
 */
int32_t algorithm_power_fullfund(struct msr_data_realtime *out,
                           const struct msr_config *cfg,
                           const struct msr_input_fullfund *in);

/**
 * @brief   全波/基波视在（读芯片 SA…）
 *
 * V/I 死区清零后写 Stot，并做 PowerJudge（S 与 P 一致性）。
 *
 * @param[out] out  Sa/Sb/Sc/Stot
 * @param[in]  cfg  接线、PT、CT、校表
 * @param[in]  in   电参快照
 *
 * @return 0 成功；参数非法或接线非法返回 -1
 */
int32_t algorithm_apparent_fullfund(struct msr_data_realtime *out,
                              const struct msr_config *cfg,
                              const struct msr_input_fullfund *in);

/**
 * @brief   半波功率合成（相序开启：V·I·角 → P/Q/S）
 *
 * P = V×I×cos(Δθ)/10000；Q = V×I×sin(Δθ)/10000；S = sqrt(P²+Q²)。
 * 依赖 out 中已有相电压、相电流、相角。
 *
 * @param[in,out] out  读写 realtime
 * @param[in]     cfg  接线模式
 *
 * @return 0 成功；参数非法或接线非法返回 -1
 */
int32_t algorithm_halfwave_power(struct msr_data_realtime *out,
                           const struct msr_config *cfg);

/**
 * @brief   半波视在：由已有 P/Q 算 S = sqrt(P²+Q²)
 *
 * P/Q 须已由芯片路径或其它算子写出。
 *
 * @param[in,out] out  读写 Sa/Sb/Sc/Stot
 *
 * @return 0 成功；out==NULL 返回 -1
 */
int32_t algorithm_halfwave_apparent(struct msr_data_realtime *out);

#endif
