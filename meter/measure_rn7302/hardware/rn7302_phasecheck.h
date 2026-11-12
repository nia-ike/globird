/**
 * @file        rn7302_phasecheck.h
 * @brief       RN7302 相位检查
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-28  yansh    1.0.0   RN7302 相位检查
 */
#ifndef RN7302_PHASECHECK_H
#define RN7302_PHASECHECK_H

#include <stdint.h>
#include "msr_config.h"

enum {
    PHASECHECK_NOT_HAPPENED = 0,
    PHASECHECK_HAPPENED,
};

/**
 * @brief   PhaseAdjust 低 3 位 → 物理通道索引
 *
 * @param[in] phase_adj  低 3 位为 1/2/3 对应 A/B/C；非法回退为 A
 * @return 通道索引 0/1/2
 */
static inline uint16_t rn7302_phasecheck_trans2idx(uint16_t phase_adj)
{
    uint16_t ch = (uint16_t)(phase_adj & 0x07U);

    if ((ch < 1U) || (ch > 3U)) {
        ch = 1U;
    }
    return (uint16_t)(ch - 1U);
}

/**
 * @brief   RN7302相序重映射检查是否发生
 * @param[in] cfg    配置
 * @return  0:未发生;1:发生
 */
uint16_t rn7302_phasecheck_is_happened(struct msr_config *cfg);

/**
 * @brief   RN7302相位检查初始化
 * @param[in] cfg    配置
 * @return  void
 */
void rn7302_phasecheck_init(struct msr_config *cfg);

#endif