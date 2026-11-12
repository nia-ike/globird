/**
 * @file        algorithm_frequency.h
 * @brief       频率计算
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   频率计算
 */
#ifndef ALGORITHM_FREQUENCY_H
#define ALGORITHM_FREQUENCY_H

#include <stdint.h>
#include "msr_config.h"
#include "msr_data.h"
#include "msr_input.h"

/**
 * @brief   频率（全波 / 基波 / 半波共用）
 *
 * Freq_hp = FOSC×8×10000/UFreq+1；越界钳到额定；无电压时置 0；
 * Freq = Freq_hp/100（×100 刻度）。
 *
 * @param[out] out  写出 Freq_hp、Freq（依赖 out 中已有电压做无压判断）
 * @param[in]  cfg  接线模式
 * @param[in]  in   含 UFreq 的电参快照
 *
 * @return 0 成功；参数非法或 UFreq==0 返回 -1（并清零频率）
 */
int32_t algorithm_frequency(struct msr_data_realtime *out,
                      const struct msr_config *cfg,
                      const struct msr_input_fullfund *in);

/**
 * @brief   仅计算频率×10000（半波 V/I 滤波用，不写 out）
 *
 * @param[in] in  含 UFreq；NULL 或 UFreq==0 时返回 0
 *
 * @return Hz_hp（×10000）
 */
uint32_t algorithm_frequency_hw(const struct msr_input_fullfund *in);

#endif
