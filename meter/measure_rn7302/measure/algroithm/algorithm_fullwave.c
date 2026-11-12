/**
 * @file        algorithm_fullwave.c
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
#include <stddef.h>
#include <stdio.h>
#include "rn7302_read.h"
#include "algorithm_fullwave.h"
#include "algorithm_angle.h"
#include "algorithm_voltage.h"
#include "algorithm_current.h"
#include "algorithm_power.h"
#include "algorithm_powerfactor.h"
#include "algorithm_frequency.h"
#include "algorithm_energy.h"
#include "algorithm_reverse.h"

/**
 * @brief   全波一拍计算
 *
 * 先读芯片全波寄存器到 in->full / in->energy，再按序计算：
 * 相角 → 电压 → 电流 → P/Q → 频率 → 视在 → 功因 → 反向 → 电能
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
                     struct msr_input *in)
{
    struct msr_data_realtime        *rt;
    struct msr_energy_realtime      *eng;
    const struct msr_input_fullfund *full;

    if (out == NULL || cfg == NULL || in == NULL) {
        return -1;
    }

    rt   = &out->realtime_full;
    eng  = &out->energy_full.realtime;
    full = &in->full;

    if ((0 != get_rn7302_register_msr_full(in))
    ||  (0 != algorithm_voltageangle      (rt, cfg, full))
    ||  (0 != algorithm_currentangle      (rt, cfg, full))
    ||  (0 != algorithm_voltage_fullfund  (rt, cfg, full))
    ||  (0 != algorithm_current_fullfund  (rt, cfg, full))
    ||  (0 != algorithm_power_fullfund    (rt, cfg, full))
    ||  (0 != algorithm_frequency         (rt, cfg, full))
    ||  (0 != algorithm_apparent_fullfund (rt, cfg, full))
    ||  (0 != algorithm_powerfactor       (rt))
    ||  (0 != algorithm_reverse           (rt, cfg))) {
        printf("FUNC = %s, line = %d\n", __func__, __LINE__);
        return -1;
    }
    
    if ((0 != get_rn7302_register_eng_full(in))
    ||  (0 != algorithm_energy_remain(eng, cfg, &in->full.energy, rt))
    ||  (0 != algorithm_energy_realtime_hp(eng, cfg))
    ||  (0 != algorithm_energy_realtime(eng, cfg))
    ||  (0 != algorithm_energy_multirate(&out->energy_full.multirate, &eng->net, out))
    ||  (0 != algorithm_energy_fourquadrant(&out->energy_full.fourquadrant, &eng->net, rt))) {
        printf("FUNC = %s, line = %d\n", __func__, __LINE__);
        return -1;
    }

    return 0;
}
