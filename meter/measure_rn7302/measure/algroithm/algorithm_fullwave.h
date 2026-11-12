/**
 * @file        algorithm_fullwave.h
 * @brief       全波计算流程调度
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   全波调度
 *  2026-10-29  yansh    1.0.1   改为 msr_data / msr_input 聚合入口
 */
#ifndef ALGORITHM_FULLWAVE_H
#define ALGORITHM_FULLWAVE_H

#include <stdint.h>
#include "msr_config.h"
#include "msr_data.h"
#include "msr_input.h"

/**
 * @brief   全波一拍计算
 *
 * 先读芯片全波寄存器到 in->full / in->energy，再按序计算：
 * 相角 → 电压 → 电流 → P/Q → 频率 → 视在 → 功因 → 电能
 * （remain / realtime_hp / realtime）。
 * 结果写入 out->realtime_full 与 out->energy_full.realtime。
 *
 * @param[in,out] out  测量数据块（写 realtime_full / energy_full）
 * @param[in]     cfg  接线、PT/CT、校表、死区、脉冲常数等
 * @param[in,out] in   输入聚合（读芯片写入 full/energy，再供算子使用）
 *
 * @return 0 成功；任一步失败返回 -1
 */
int32_t algorithm_fullwave(struct msr_data *out,
                     const struct msr_config *cfg,
                     struct msr_input *in);

#endif
