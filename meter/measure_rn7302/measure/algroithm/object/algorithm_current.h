/**
 * @file        algorithm_current.h
 * @brief       电流计算
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   电流计算
 */
#ifndef ALGORITHM_CURRENT_H
#define ALGORITHM_CURRENT_H

#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include "msr_config.h"
#include "msr_data.h"
#include "msr_input.h"

/**
 * @brief   全波/基波电流（相电流 + 中性线 In）
 *
 * type_i0==MSR_TYPE_I0_CH4 时 In 按第四路电流（×3/2）换算，否则按漏电。
 *
 * @param[out] out  Ia/Ib/Ic/Iavg/In
 * @param[in]  cfg  接线、CT、校表、死区、type_i0
 * @param[in]  in   电参快照（IA/IB/IC/IN）
 *
 * @return 0 成功；参数非法或接线非法返回 -1
 */
int32_t algorithm_current_fullfund(struct msr_data_realtime *out,
                             const struct msr_config *cfg,
                             const struct msr_input_fullfund *in);

/**
 * @brief   半波电流单入口（按 cfg->wiremode 分支）
 *
 * 环缓冲与写槽由 st 持有；hz_hp 为频率×10000。
 * - P1W2：只出 Ia
 * - P3W3 / P3W4：Ia/Ib/Ic
 * 半波不算 In（与 Globird 一致）。
 *
 * @param[out] out    Ia/Ib/Ic/Iavg
 * @param[in]  cfg    接线、CT、校表、死区
 * @param[in]  in     半波电参快照
 * @param[in,out] st  跨拍原始值环与 wr
 * @param[in]  hz_hp  频率（×10000）
 *
 * @return 0 成功；非 0 失败
 */
int32_t algorithm_halfwave_current(struct msr_data_realtime *out,
                             const struct msr_config *cfg,
                             const struct msr_input_fullfund *in,
                             struct msr_halfwave_curr_state *st,
                             uint32_t hz_hp);

#endif
