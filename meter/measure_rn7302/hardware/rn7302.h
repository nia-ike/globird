/**
 * @file        rn7302.h
 * @brief       RN7302 相位检查
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-28  yansh    1.0.0   RN7302 相位检查
 */
#ifndef RN7302_H
#define RN7302_H

#include <stdint.h>
#include "msr_config.h"

enum {
    CURRENT_RATED_C5    = 5,            // 内置5A
    CURRENT_RATED_K100  = 100,          // 外置100A
    CURRENT_RATED_K5    = (0x8000 | 5), // 外置5A
};


/**
 * @brief       RN7302寄存器值检查
 * @param[in]    cfg    配置
 * @return      0 成功；-1 失败
 */
int32_t rn7302_status_check(struct msr_config *cfg);

/**
 * @brief       RN7302硬件初始化
 * @param[in]    cfg    配置
 * @return      0 成功；-1 失败
 */
void rn7302_init(struct msr_config *cfg);

#endif