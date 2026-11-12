/**
 * @file        algorithm_current.c
 * @brief       电流计算
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   电流计算
 */
#include <stdlib.h>
#include "algorithm_current.h"


/**
 * @brief   死区约束：|value| < deadzone 则返回 0，否则原值
 *
 * @param[in] value     待约束值
 * @param[in] deadzone  死区阈值
 *
 * @return 约束后的值
 */
static int64_t curr_calc_range_constaraint(int64_t value, uint64_t deadzone)
{
    if (llabs(value) < (int64_t)deadzone) {
        return 0;
    }

    return value;
}

/**
 * @brief   全波/基波单相电流
 *
 * temp = emu × ct / MULTIPLE_I / Krms；dead = dz × ct / MULTIPLE_I。
 *
 * @param[in] emu_i           芯片电流寄存器
 * @param[in] rated_current   额定电流
 * @param[in] reg_ix          校表寄存器目标
 * @param[in] factor_current  精度系数
 * @param[in] deadzone_ix     死区
 * @param[in] ct              CT
 *
 * @return 一次侧电流有效值
 */
static uint32_t curr_fullfund_phase_single(int32_t emu_i,
                                           uint32_t reg_ix,
                                           uint32_t rated_current,
                                           uint32_t factor_current,
                                           uint32_t deadzone_ix,
                                           uint32_t ct)
{
    uint32_t krms = reg_ix / rated_current / factor_current;
    uint64_t temp;
    uint64_t dead;

    if (krms == 0) {
        krms = RN7302_Krmsi;
    }

    temp = (uint64_t)emu_i * ct / MULTIPLE_I / krms;
    dead = (uint64_t)deadzone_ix * ct / MULTIPLE_I;
    temp = (uint64_t)curr_calc_range_constaraint((int64_t)temp, dead);

    return (uint32_t)temp;
}

/**
 * @brief   全波/基波中性线 In
 *
 * type_i0==MSR_TYPE_I0_CH4：emu×3/2/Krms；否则 emu/Krms；再死区。
 *
 * @param[in] emu_in          芯片 IN 寄存器
 * @param[in] rated_current   额定电流
 * @param[in] reg_in          校表寄存器目标
 * @param[in] factor_current  精度系数
 * @param[in] deadzone_in     死区
 * @param[in] type_i0         MSR_TYPE_I0_*
 *
 * @return 一次侧中性线电流
 */
static uint32_t curr_fullfund_neutral_single(int32_t emu_in,
                                             uint32_t rated_current,
                                             uint32_t reg_in,
                                             uint32_t factor_current,
                                             uint32_t deadzone_in,
                                             uint16_t type_i0)
{
    uint32_t krms = rated_current / reg_in / factor_current;
    uint64_t temp;

    if (krms == 0) {
        krms = RN7302_Krmsi;
    }

    if (type_i0 == MSR_TYPE_I0_CH4) {
        temp = (uint64_t)emu_in * 3 / 2 / krms;
    } else {
        temp = (uint64_t)emu_in / krms;
    }

    temp = (uint64_t)curr_calc_range_constaraint((int64_t)temp, deadzone_in);
    return (uint32_t)temp;
}

/**
 * @brief   全波/基波相电流：按接线写 Ia/Ib/Ic/Iavg
 *
 * @param[out] out  Ia/Ib/Ic/Iavg
 * @param[in]  cfg  接线、CT、校表、死区
 * @param[in]  in   含 IA/IB/IC
 *
 * @return 0 成功；接线非法返回 -1
 */
static int32_t current_fullfund_phase(struct msr_data_realtime *out,
                                      const struct msr_config *cfg,
                                      const struct msr_input_fullfund *in)
{
    const struct cfg_adjust   *pAdjust = &cfg->sys.adjust;
    const struct cfg_deadzone *pDead   = &cfg->sys.deadzone;

    switch (cfg->user.wiremode) {
    case WIREMODE_P1W2:
        out->Ia = curr_fullfund_phase_single(in->IA, pAdjust->reg_ia, 
            pAdjust->rated_current, pAdjust->factor_current, pDead->ia, cfg->user.ct);
        out->Ib = 0;
        out->Ic = 0;
        out->Iavg = (out->Ia + out->Ib + out->Ic) / 3;
        break;
    case WIREMODE_P3W3:
    case WIREMODE_P3W4:
        out->Ia = curr_fullfund_phase_single(in->IA, pAdjust->reg_ia, 
            pAdjust->rated_current, pAdjust->factor_current, pDead->ia, cfg->user.ct);
        out->Ib = curr_fullfund_phase_single(in->IB, pAdjust->reg_ib, 
            pAdjust->rated_current, pAdjust->factor_current, pDead->ib, cfg->user.ct);
        out->Ic = curr_fullfund_phase_single(in->IC, pAdjust->reg_ic, 
            pAdjust->rated_current, pAdjust->factor_current, pDead->ic, cfg->user.ct);
        out->Iavg = (uint32_t)(((uint64_t)out->Ia + out->Ib + out->Ic) / 3);
        break;
    default:
        return -1;
    }

    return 0;
}

/**
 * @brief   写出 out->In（按 cfg->type_i0）
 *
 * @param[out] out  In
 * @param[in]  cfg  type_i0、校表、死区
 * @param[in]  in   含 IN
 *
 * @return 0 成功
 */
static int32_t current_fullfund_neutral(struct msr_data_realtime *out,
                                        const struct msr_config *cfg,
                                        const struct msr_input_fullfund *in)
{
    const struct cfg_adjust   *pAdjust = &cfg->sys.adjust;
    const struct cfg_deadzone *pDead   = &cfg->sys.deadzone;

    out->In = curr_fullfund_neutral_single(in->IN, pAdjust->rated_current,
        pAdjust->reg_in, pAdjust->factor_current, pDead->in, cfg->user.type_i0);
    return 0;
}

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
                             const struct msr_input_fullfund *in)
{
    int32_t ret;

    if (out == NULL || cfg == NULL || in == NULL) {
        return -1;
    }

    ret = current_fullfund_phase(out, cfg, in);
    if (ret != 0) {
        return ret;
    }

    return current_fullfund_neutral(out, cfg, in);
}

/**
 * @brief   由半波电流环两槽合成有效值（不写槽）
 *
 * @param[in] ring            两槽原始寄存器值
 * @param[in] kf              频率修正系数
 * @param[in] rated_current   额定电流
 * @param[in] reg_ix          校表寄存器目标
 * @param[in] factor_current  精度系数
 * @param[in] deadzone_ix     死区
 * @param[in] ct              CT
 *
 * @return 一次侧半波电流有效值
 */
static uint32_t curr_halfwave_rms_of_ring(const int32_t *ring,
                                         float kf,
                                         uint32_t reg_ix,
                                         uint32_t rated_current,
                                         uint32_t factor_current,
                                         uint32_t deadzone_ix,
                                         uint32_t ct)
{
    uint64_t temp = 0;
    uint64_t dead;
    uint32_t krms;
    uint32_t j;

    krms = reg_ix / rated_current / factor_current;
    if (krms == 0) {
        krms = RN7302_Krmsi;
    }

    for (j = 0; j < MSR_HALFWAVE_SLOTS; j++) {
        temp += (uint64_t)(uint32_t)ring[j];
    }

    temp = (uint64_t)sqrt((double)(temp * 65536u / MSR_HALFWAVE_SLOTS)) * 256u / kf;
    dead = (uint64_t)deadzone_ix * ct / MULTIPLE_I;
    temp = temp * ct / MULTIPLE_I / krms;
    if (temp < dead) {
        temp = 0;
    }

    return (uint32_t)temp;
}

/**
 * @brief   半波单相电流：写入环槽后换算
 *
 * @param[in]     emu_i  本拍芯片半波寄存器
 * @param[in,out] ring   两槽环缓冲
 * @param[in]     slot   本拍写入槽索引
 * @param[in]     kf     频率修正系数
 * @param[in]     rated_current / reg_ix / factor_current / deadzone_ix / ct  校表与变比
 *
 * @return 一次侧半波电流有效值
 */
static uint32_t curr_halfwave_phase_single(int32_t emu_i,
                                           int32_t *ring,
                                           uint32_t slot,
                                           float kf,
                                           uint32_t reg_ix,
                                           uint32_t rated_current,
                                           uint32_t factor_current,
                                           uint32_t deadzone_ix,
                                           uint32_t ct)
{
    ring[slot] = emu_i;
    return curr_halfwave_rms_of_ring(ring, kf, reg_ix, rated_current,
                                     factor_current, deadzone_ix, ct);
}

/**
 * @brief   半波相电流（按接线模式）
 *
 * P1W2 只出 Ia；P3W3/P3W4 出三相；半波不算 In。写完递增 st->wr。
 *
 * @param[out]    out    Ia/Ib/Ic/Iavg
 * @param[in]     cfg    接线、CT、校表、死区
 * @param[in]     in     半波 IA/IB/IC
 * @param[in,out] st     环缓冲与 wr
 * @param[in]     hz_hp  频率×10000
 *
 * @return 0 成功；接线非法返回 -1
 */
static int32_t current_halfwave_phase(struct msr_data_realtime *out,
                                      const struct msr_config *cfg,
                                      const struct msr_input_fullfund *in,
                                      struct msr_halfwave_curr_state *st,
                                      uint32_t hz_hp)
{
    const struct cfg_adjust   *pAdjust = &cfg->sys.adjust;
    const struct cfg_deadzone *pDead   = &cfg->sys.deadzone;
    uint32_t slot = st->wr % MSR_HALFWAVE_SLOTS;
    float kf = sqrtf((float)(4000 * 10000) / (float)hz_hp);
    uint32_t ia, ib, ic;

    ia = curr_halfwave_phase_single(in->IA, st->ia, slot, kf,
        pAdjust->reg_ia, pAdjust->rated_current, pAdjust->factor_current,
        pDead->ia, cfg->user.ct);
    ib = curr_halfwave_phase_single(in->IB, st->ib, slot, kf,
        pAdjust->reg_ib, pAdjust->rated_current, pAdjust->factor_current,
        pDead->ib, cfg->user.ct);
    ic = curr_halfwave_phase_single(in->IC, st->ic, slot, kf,
        pAdjust->reg_ic, pAdjust->rated_current, pAdjust->factor_current,
        pDead->ic, cfg->user.ct);

    switch (cfg->user.wiremode) {
    case WIREMODE_P1W2:
        out->Ia = ia;
        out->Ib = 0;
        out->Ic = 0;
        out->Iavg = (out->Ia + out->Ib + out->Ic) / 3;
        break;
    case WIREMODE_P3W3:
        out->Ia = 0;
        out->Ib = 0;
        out->Ic = 0;
        out->Iavg = 0;
        break;
    case WIREMODE_P3W4:
        out->Ia = ia;
        out->Ib = ib;
        out->Ic = ic;
        out->Iavg = (uint32_t)(((uint64_t)out->Ia + out->Ib + out->Ic) / 3);
        break;
    default:
        return -1;
    }

    st->wr++;
    return 0;
}

/**
 * @brief   半波电流单入口（按 cfg->user.wiremode 分支）
 *
 * 环缓冲与写槽由 st 持有；hz_hp 为频率×10000。
 * - P1W2：只出 Ia
 * - P3W3 / P3W4：Ia/Ib/Ic
 * 半波不算 In（与 Globird 一致）。
 *
 * @param[out]    out    Ia/Ib/Ic/Iavg
 * @param[in]     cfg    接线、CT、校表、死区
 * @param[in]     in     半波电参快照
 * @param[in,out] st     跨拍原始值环与 wr
 * @param[in]     hz_hp  频率（×10000）
 *
 * @return 0 成功；hz_hp==0、参数为空或接线非法返回 -1
 */
int32_t algorithm_halfwave_current(struct msr_data_realtime *out,
                             const struct msr_config *cfg,
                             const struct msr_input_fullfund *in,
                             struct msr_halfwave_curr_state *st,
                             uint32_t hz_hp)
{
    if (out == NULL || cfg == NULL || in == NULL || st == NULL || hz_hp == 0) {
        return -1;
    }

    return current_halfwave_phase(out, cfg, in, st, hz_hp);
}
