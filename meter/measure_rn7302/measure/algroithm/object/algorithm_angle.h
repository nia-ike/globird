/**
 * @file        algorithm_angle.h
 * @brief       电压 / 电流相角
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   相角计算
 */
#ifndef ALGORITHM_ANGLE_H
#define ALGORITHM_ANGLE_H

#include <stdint.h>
#include "msr_config.h"
#include "msr_data.h"
#include "msr_input.h"

/**
 * @brief   电压相角（读 YUA/YUB/YUC，单位 0.01°）
 *
 * - P1W2：写 AngleUa，幅值 Va==0 时清零
 * - P3W3：清零 AngleU*（角型相角由线电压算子写出）
 * - P3W4：写三相角；对应相电压为 0 时该相角清零（可用上一拍 V）
 *
 * @param[out] out  AngleUa/Ub/Uc
 * @param[in]  cfg  接线模式
 * @param[in]  in   含 YUA/YUB/YUC
 *
 * @return 0 成功；参数非法或接线非法返回 -1
 */
int32_t algorithm_voltageangle(struct msr_data_realtime *out,
                         const struct msr_config *cfg,
                         const struct msr_input_fullfund *in);

/**
 * @brief   电流相角（读 YIA/YIB/YIC，单位 0.01°）
 *
 * 零值判断用 out->Ia/Ib/Ic（电流算子之前调用时为上一拍幅值）。
 * phase_adjust 非默认 1/2/3 或含 bit15 时，按物理通道重映射；
 * bit15 表示 CT 反向，相角再加 180°。
 *
 * @param[out] out  AngleIa/Ib/Ic
 * @param[in]  cfg  接线、phase_adjust
 * @param[in]  in   含 YIA/YIB/YIC
 *
 * @return 0 成功；参数非法或接线非法返回 -1
 */
int32_t algorithm_currentangle(struct msr_data_realtime *out,
                         const struct msr_config *cfg,
                         const struct msr_input_fullfund *in);

#endif
