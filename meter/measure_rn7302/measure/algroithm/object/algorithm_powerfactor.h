/**
 * @file        algorithm_powerfactor.h
 * @brief       功率因数
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   功率因数
 */
#ifndef ALGORITHM_POWERFACTOR_H
#define ALGORITHM_POWERFACTOR_H

#include <stdint.h>
#include "msr_config.h"
#include "msr_data.h"

/**
 * @brief   功率因数（全波 / 基波 / 半波共用）
 *
 * 使用 out 中已有 P、S：PF = P×1000/S；|P|>S 时钳到 ±1000；S==0 时 PF=0。
 * 调度须保证调用时 P、S 已按本拍策略写好（本拍视在或上一拍视在）。
 *
 * @param[in,out] out  读 Pa/Sa…，写出 PFa/PFb/PFc/PFtot
 *
 * @return 0 成功；out==NULL 返回 -1
 */
int32_t algorithm_powerfactor(struct msr_data_realtime *out);

#endif
