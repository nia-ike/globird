/**
 * @file        algorithm_voltage.h
 * @brief       电压计算
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   电压计算
 */
#ifndef ALGORITHM_VOLTAGE_H
#define ALGORITHM_VOLTAGE_H

#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include "msr_config.h"
#include "msr_data.h"
#include "msr_input.h"

/**
 * @brief   全波/基波电压（相电压 + 线电压）
 *
 * P3W4 线电压用 out 中已有 Angle*（调度须先调 alg_voltageangle）；
 * P3W3 由 UA/UC 与 YUC 三角合成线电压并写回 Angle*。
 *
 * @param[out] out  Va…/Vab… 及必要时 AngleU*
 * @param[in]  cfg  接线、PT、校表、死区
 * @param[in]  in   电参快照（UA/UB/UC、YUC 等）
 *
 * @return 0 成功；参数为空返回 -1
 */
int32_t algorithm_voltage_fullfund(struct msr_data_realtime *out,
                             const struct msr_config *cfg,
                             const struct msr_input_fullfund *in);

/**
 * @brief   半波电压单入口（相电压 + 线电压，按 cfg->wiremode 分支）
 *
 * 环缓冲与写槽由 st 持有；hz_hp 为频率×10000。
 * - P1W2：只出 Va；线电压清零
 * - P3W3：相电压清零；线电压由 UA/UC 半波有效值三角合成，并写回 Angle*
 * - P3W4：三相相电压；线电压用已有 Angle*，调度须先写相角：
 *     voltageangle → alg_halfwave_voltage → …
 *
 * @param[out] out    Va/Vb/Vc/VlnAvg 与 Vab/Vbc/Vca/VllAvg
 * @param[in]  cfg    接线、PT、校表、死区
 * @param[in]  in     半波电参快照（UA/UB/UC 为半波寄存器）
 * @param[in,out] st  跨拍原始值环与 wr
 * @param[in]  hz_hp  频率（×10000）
 *
 * @return 0 成功；非 0 失败（如 hz_hp==0、st==NULL、非法接线）
 */
int32_t algorithm_halfwave_voltage(struct msr_data_realtime *out,
                             const struct msr_config *cfg,
                             const struct msr_input_fullfund *in,
                             struct msr_halfwave_volt_state *st,
                             uint32_t hz_hp);

#endif
