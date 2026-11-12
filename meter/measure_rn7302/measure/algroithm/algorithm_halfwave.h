/**
 * @file        algorithm_halfwave.h
 * @brief       半波计算流程调度
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   半波调度
 *  2026-10-29  yansh    1.0.1   改为 msr_data / msr_input 聚合入口
 */
#ifndef ALGORITHM_HALFWAVE_H
#define ALGORITHM_HALFWAVE_H

#include <stdint.h>
#include "msr_config.h"
#include "msr_data.h"
#include "msr_input.h"

/**
 * @brief   半波一拍计算
 *
 * 先读芯片半波寄存器到 in->half，再编排算子；先算 hz_hp，为 0 则整拍失败。
 * 结果写入 out->realtime_half。电压/电流环缓冲由本模块内部维护。
 *
 * - phasecheck 关闭：电压角 → 半波 V/I → 电流角 → 芯片 P/Q → 频率
 *   → 芯片视在 → 功因 → CT 反向（phase_adjust bit15）
 * - phasecheck 开启：电压角 → 半波电流 → phase_adjust 重映射逻辑相
 *   → 频率 → 电流角 → 半波电压 → V·I·角合成 P/Q/S → 功因
 *
 * P3W4 半波线电压依赖 AngleU*，故电压角在半波电压之前。
 *
 * @param[in,out] out  测量数据块（写 realtime_half）
 * @param[in]     cfg  接线、PT/CT、校表、phase_adjust 等
 * @param[in,out] in   输入聚合（读芯片写入 half）
 *
 * @return 0 成功；参数非法、hz_hp==0 或任一步失败返回 -1
 */
int32_t algorithm_halfwave(struct msr_data *out,
                           const struct msr_config *cfg,
                           struct msr_input *in);

#endif
