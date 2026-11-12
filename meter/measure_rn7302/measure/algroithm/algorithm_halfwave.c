/**
 * @file        algorithm_halfwave.c
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
#include <stddef.h>
#include "rn7302_read.h"
#include "rn7302_phasecheck.h"
#include "algorithm_halfwave.h"
#include "algorithm_angle.h"
#include "algorithm_voltage.h"
#include "algorithm_current.h"
#include "algorithm_power.h"
#include "algorithm_powerfactor.h"
#include "algorithm_frequency.h"
#include "algorithm_reverse.h"

/** 半波跨拍环缓冲（与产品 abort 清零约定：后续可改为对外状态） */
static struct msr_halfwave_volt_state s_st_v;
static struct msr_halfwave_curr_state s_st_i;

/**
 * @brief   半波相序关闭路径：用芯片半波寄存器算 P/Q/S
 */
static int32_t halfwave_phasecheck_off(struct msr_data_realtime *out,
                                       const struct msr_config *cfg,
                                       const struct msr_input_fullfund *in,
                                       struct msr_halfwave_volt_state *st_v,
                                       struct msr_halfwave_curr_state *st_i,
                                       uint32_t hz_hp)
{
    if ((0 != algorithm_voltageangle     (out, cfg, in))
    ||  (0 != algorithm_halfwave_voltage (out, cfg, in, st_v, hz_hp))
    ||  (0 != algorithm_halfwave_current (out, cfg, in, st_i, hz_hp))
    ||  (0 != algorithm_currentangle     (out, cfg, in))
    ||  (0 != algorithm_power_fullfund   (out, cfg, in))
    ||  (0 != algorithm_frequency        (out, cfg, in))
    ||  (0 != algorithm_halfwave_apparent(out))
    ||  (0 != algorithm_powerfactor      (out))
    ||  (0 != algorithm_reverse          (out, cfg))) {
        return -1;
    }

    return 0;
}

/**
 * @brief   半波相序开启路径：按 phase_adjust 映射电流后合成功率
 *
 * 半波有效值寄存器不跟 ICHSEL，先按物理通道校准，再映射到逻辑相。
 */
static int32_t halfwave_phasecheck_on(struct msr_data_realtime *out,
                                      const struct msr_config *cfg,
                                      const struct msr_input_fullfund *in,
                                      struct msr_halfwave_volt_state *st_v,
                                      struct msr_halfwave_curr_state *st_i,
                                      uint32_t hz_hp)
{
    uint32_t i_phys[3];
    int32_t  ret;

    ret = algorithm_voltageangle(out, cfg, in);
    if (ret != 0) {
        return ret;
    }

    ret = algorithm_halfwave_current(out, cfg, in, st_i, hz_hp);
    if (ret != 0) {
        return ret;
    }

    /* 物理通道 → 逻辑相（半波有效值不跟 ICHSEL） */
    i_phys[0] = out->Ia;
    i_phys[1] = out->Ib;
    i_phys[2] = out->Ic;
    out->Ia = i_phys[rn7302_phasecheck_trans2idx(cfg->user.phase_adjust[0])];
    out->Ib = i_phys[rn7302_phasecheck_trans2idx(cfg->user.phase_adjust[1])];
    out->Ic = i_phys[rn7302_phasecheck_trans2idx(cfg->user.phase_adjust[2])];
    out->Iavg = (out->Ia + out->Ib + out->Ic) / 3U;

    ret = algorithm_frequency(out, cfg, in);
    if (ret != 0 && in->UFreq != 0) {
        return ret;
    }

    ret = algorithm_currentangle(out, cfg, in);
    if (ret != 0) {
        return ret;
    }

    ret = algorithm_halfwave_voltage(out, cfg, in, st_v, hz_hp);
    if (ret != 0) {
        return ret;
    }

    ret = algorithm_halfwave_power(out, cfg);
    if (ret != 0) {
        return ret;
    }

    return algorithm_powerfactor(out);
}

/**
 * @brief   半波一拍计算
 *
 * 先读芯片半波寄存器到 in->half，再编排算子；先算 hz_hp，为 0 则整拍失败。
 * 结果写入 out->realtime_half。
 *
 * @param[in,out] out  测量数据块（写 realtime_half）
 * @param[in]     cfg  接线、PT/CT、校表、phase_adjust 等
 * @param[in,out] in   输入聚合（读芯片写入 half）
 *
 * @return 0 成功；参数非法、hz_hp==0 或任一步失败返回 -1
 */
int32_t algorithm_halfwave(struct msr_data *out,
                           const struct msr_config *cfg,
                           struct msr_input *in)
{
    struct msr_data_realtime *rt;
    const struct msr_input_fullfund *half;
    uint32_t hz_hp;
    const uint8_t phasecheck_on = 0; /* 产品 phasecheck flag 接入前默认关闭 */

    if (out == NULL || cfg == NULL || in == NULL) {
        return -1;
    }

    if (0 != get_rn7302_register_msr_half(in)) {
        return -1;
    }

    rt   = &out->realtime_half;
    half = &in->half;

    hz_hp = algorithm_frequency_hw(half);
    if (hz_hp == 0U) {
        return -1;
    }

    if (phasecheck_on) {
        return halfwave_phasecheck_on(rt, cfg, half, &s_st_v, &s_st_i, hz_hp);
    }
    return halfwave_phasecheck_off(rt, cfg, half, &s_st_v, &s_st_i, hz_hp);
}
