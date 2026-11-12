/**
 * @file        algorithm_power.c
 * @brief       功率计算（有功 / 无功 / 视在）
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   功率计算
 */
#include <stdlib.h>
#include "algorithm_power.h"


/**
 * @brief   芯片功率寄存器 → 一次侧单相功率
 *
 * temp = emu × pt × ct / 10000 / Krms；Krms 非法时用 RN7302_Krmsp。
 *
 * @param[in] emu_p         芯片功率寄存器
 * @param[in] rated_power   额定功率（校表）
 * @param[in] reg_px        校表寄存器目标
 * @param[in] factor_power  精度系数
 * @param[in] pt, ct        互感器变比
 *
 * @return 一次侧功率（有符号）
 */
static int32_t power_fullfund_phase_single(int32_t emu_p,
                                           uint32_t reg_px,
                                           uint32_t rated_power,
                                           uint32_t factor_power,
                                           uint32_t pt,
                                           uint32_t ct)
{
    uint32_t krms = reg_px / rated_power / factor_power;

    if (krms == 0) {
        krms = RN7302_Krmsp;
    }

    return (int32_t)((int64_t)emu_p * (int64_t)pt * (int64_t)ct
                     / MULTIPLE_PTCT / (int32_t)krms);
}

/**
 * @brief   V 或 I 为 0 时清对应相有功/无功（不清视在）
 *
 * @param[in,out] out  读 Va/Ia…，清零对应 Pa/Qa…
 */
static void power_fullfund_zero_pq_by_vi(struct msr_data_realtime *out)
{
    if (out->Ia == 0 || out->Va == 0) {
        out->Pa = 0;
        out->Qa = 0;
    }
    if (out->Ib == 0 || out->Vb == 0) {
        out->Pb = 0;
        out->Qb = 0;
    }
    if (out->Ic == 0 || out->Vc == 0) {
        out->Pc = 0;
        out->Qc = 0;
    }
}

/**
 * @brief   V 或 I 为 0 时清对应相视在
 *
 * @param[in,out] out  读 Va/Ia…，清零对应 Sa/Sb/Sc
 */
static void power_fullfund_zero_s_by_vi(struct msr_data_realtime *out)
{
    if (out->Ia == 0 || out->Va == 0) {
        out->Sa = 0;
    }
    if (out->Ib == 0 || out->Vb == 0) {
        out->Sb = 0;
    }
    if (out->Ic == 0 || out->Vc == 0) {
        out->Sc = 0;
    }
}

/**
 * @brief   视在与有功一致性修正（Globird PowerJudge）
 *
 * S < P（P>0）或 P=Q=0 时，将该相/合相 S 置为 P。
 *
 * @param[in,out] out  读写 Sa…/Stot 与 Pa…/Ptot、Qa…
 */
static void power_fullfund_apparent_fix(struct msr_data_realtime *out)
{
    if (((out->Sa < (uint32_t)out->Pa) && (out->Pa > 0))
        || ((out->Pa == 0) && (out->Qa == 0))) {
        out->Sa = (uint32_t)out->Pa;
    }
    if (((out->Sb < (uint32_t)out->Pb) && (out->Pb > 0))
        || ((out->Pb == 0) && (out->Qb == 0))) {
        out->Sb = (uint32_t)out->Pb;
    }
    if (((out->Sc < (uint32_t)out->Pc) && (out->Pc > 0))
        || ((out->Pc == 0) && (out->Qc == 0))) {
        out->Sc = (uint32_t)out->Pc;
    }
    if (((out->Stot < (uint32_t)out->Ptot) && (out->Ptot > 0))
        || ((out->Ptot == 0) && (out->Qtot == 0))) {
        out->Stot = (uint32_t)out->Ptot;
    }
}

/**
 * @brief   全波/基波有功：读 PA/PB/PC，按接线写 Pa/Pb/Pc
 *
 * @param[out] out  Pa/Pb/Pc
 * @param[in]  cfg  接线、PT、CT、校表
 * @param[in]  in   含 PA/PB/PC
 *
 * @return 0 成功；接线非法返回 -1
 */
static int32_t power_fullfund_active(struct msr_data_realtime *out,
                                     const struct msr_config *cfg,
                                     const struct msr_input_fullfund *in)
{
    const struct cfg_adjust *pAdjust = &cfg->sys.adjust;
    uint32_t pt = cfg->user.pt;
    uint32_t ct = cfg->user.ct;

    switch (cfg->user.wiremode) {
    case WIREMODE_P1W2:
        out->Pa = power_fullfund_phase_single(in->PA, pAdjust->reg_pa, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Pb = 0;
        out->Pc = 0;
        out->Ptot = out->Pa;
        break;
    case WIREMODE_P3W3:
        out->Pa = 0;
        out->Pb = 0;
        out->Pc = 0;
        out->Pab = power_fullfund_phase_single(in->PA, pAdjust->reg_pa, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Pca = power_fullfund_phase_single(in->PC, pAdjust->reg_pc, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Ptot = out->Pab + out->Pca;
        break;
    case WIREMODE_P3W4:
        out->Pa = power_fullfund_phase_single(in->PA, pAdjust->reg_pa, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Pb = power_fullfund_phase_single(in->PB, pAdjust->reg_pb, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Pc = power_fullfund_phase_single(in->PC, pAdjust->reg_pc, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Ptot = out->Pa + out->Pb + out->Pc;
        break;
    default:
        return -1;
    }

    return 0;
}

/**
 * @brief   全波/基波无功：读 QA/QB/QC，按接线写 Qa/Qb/Qc
 *
 * @param[out] out  Qa/Qb/Qc
 * @param[in]  cfg  接线、PT、CT、校表
 * @param[in]  in   含 QA/QB/QC
 *
 * @return 0 成功；接线非法返回 -1
 */
static int32_t power_fullfund_reactive(struct msr_data_realtime *out,
                                       const struct msr_config *cfg,
                                       const struct msr_input_fullfund *in)
{
    const struct cfg_adjust *pAdjust = &cfg->sys.adjust;
    uint32_t pt = cfg->user.pt;
    uint32_t ct = cfg->user.ct;

    switch (cfg->user.wiremode) {
    case WIREMODE_P1W2:
        out->Qa = power_fullfund_phase_single(in->QA, pAdjust->reg_pa, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Qb = 0;
        out->Qc = 0;
        out->Qtot = out->Qa;
        break;
    case WIREMODE_P3W3:
        out->Qa = 0;
        out->Qb = 0;
        out->Qc = 0;
        out->Qab = power_fullfund_phase_single(in->QA, pAdjust->reg_pa, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Qca = power_fullfund_phase_single(in->QC, pAdjust->reg_pc, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Qtot = out->Qab + out->Qca;
        break;
    case WIREMODE_P3W4:
        out->Qa = power_fullfund_phase_single(in->QA, pAdjust->reg_pa, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Qb = power_fullfund_phase_single(in->QB, pAdjust->reg_pb, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Qc = power_fullfund_phase_single(in->QC, pAdjust->reg_pc, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Qtot = out->Qa + out->Qb + out->Qc;
        break;
    default:
        return -1;
    }

    return 0;
}

/**
 * @brief   全波/基波视在：读 SA/SB/SC，按接线写 Sa/Sb/Sc
 *
 * @param[out] out  Sa/Sb/Sc
 * @param[in]  cfg  接线、PT、CT、校表
 * @param[in]  in   含 SA/SB/SC
 *
 * @return 0 成功；接线非法返回 -1
 */
static int32_t power_fullfund_apparent(struct msr_data_realtime *out,
                                       const struct msr_config *cfg,
                                       const struct msr_input_fullfund *in)
{
    const struct cfg_adjust *pAdjust = &cfg->sys.adjust;
    uint32_t pt = cfg->user.pt;
    uint32_t ct = cfg->user.ct;

    switch (cfg->user.wiremode)
    {
    case WIREMODE_P1W2:
        out->Sa = (uint32_t)power_fullfund_phase_single(in->SA, pAdjust->reg_pa, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Sb = 0;
        out->Sc = 0;
        out->Stot = out->Sa;
        break;
    case WIREMODE_P3W3:
        out->Sa = 0;
        out->Sb = 0;
        out->Sc = 0;
        out->Sab = (uint32_t)power_fullfund_phase_single(in->SA, pAdjust->reg_pa, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Sca = (uint32_t)power_fullfund_phase_single(in->SC, pAdjust->reg_pc, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Stot = out->Sab + out->Sca;
        break;
    case WIREMODE_P3W4:
        out->Sa = (uint32_t)power_fullfund_phase_single(in->SA, pAdjust->reg_pa, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Sb = (uint32_t)power_fullfund_phase_single(in->SB, pAdjust->reg_pb, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Sc = (uint32_t)power_fullfund_phase_single(in->SC, pAdjust->reg_pc, 
            pAdjust->rated_power, pAdjust->factor_power, pt, ct);
        out->Stot = out->Sa + out->Sb + out->Sc;
        break;
    default:
        return -1;
    }

    return 0;
}

/**
 * @brief   全波/基波有功 + 无功（读芯片 PA/QA…）
 *
 * 不写视在。依赖 out 中已有 Va/Ia 等：V 或 I 为 0 时清对应相 P/Q。
 *
 * @param[out] out  Pa/Pb/Pc/Ptot、Qa/Qb/Qc/Qtot
 * @param[in]  cfg  接线、PT、CT、校表
 * @param[in]  in   电参快照
 *
 * @return 0 成功；参数非法或接线非法返回 -1
 */
int32_t algorithm_power_fullfund(struct msr_data_realtime *out,
                           const struct msr_config *cfg,
                           const struct msr_input_fullfund *in)
{
    int32_t ret;

    if (out == NULL || cfg == NULL || in == NULL) {
        return -1;
    }

    ret = power_fullfund_active(out, cfg, in);
    if (ret != 0) {
        return ret;
    }
    ret = power_fullfund_reactive(out, cfg, in);
    if (ret != 0) {
        return ret;
    }

    // power_fullfund_zero_pq_by_vi(out);

    return 0;
}

/**
 * @brief   全波/基波视在（读芯片 SA…）
 *
 * V/I 死区清零后写 Stot，并做 PowerJudge（S 与 P 一致性）。
 *
 * @param[out] out  Sa/Sb/Sc/Stot
 * @param[in]  cfg  接线、PT、CT、校表
 * @param[in]  in   电参快照
 *
 * @return 0 成功；参数非法或接线非法返回 -1
 */
int32_t algorithm_apparent_fullfund(struct msr_data_realtime *out,
                              const struct msr_config *cfg,
                              const struct msr_input_fullfund *in)
{
    int32_t ret;

    if (out == NULL || cfg == NULL || in == NULL) {
        return -1;
    }

    ret = power_fullfund_apparent(out, cfg, in);
    if (ret != 0) {
        return ret;
    }

    power_fullfund_zero_s_by_vi(out);
    out->Stot = out->Sa + out->Sb + out->Sc;
    power_fullfund_apparent_fix(out);
    return 0;
}

/**
 * @brief   由已有 P/Q 合成视在：S = sqrt(P²+Q²)，含合相
 *
 * @param[in,out] out  读 Pa/Qa…，写 Sa/Sb/Sc/Stot
 */
static void power_halfwave_apparent_calc(struct msr_data_realtime *out)
{
    out->Sa = (uint32_t)sqrt((double)out->Pa * (double)out->Pa
                             + (double)out->Qa * (double)out->Qa);
    out->Sb = (uint32_t)sqrt((double)out->Pb * (double)out->Pb
                             + (double)out->Qb * (double)out->Qb);
    out->Sc = (uint32_t)sqrt((double)out->Pc * (double)out->Pc
                             + (double)out->Qc * (double)out->Qc);
    out->Stot = (uint32_t)sqrt((double)out->Ptot * (double)out->Ptot
                               + (double)out->Qtot * (double)out->Qtot);
}

/**
 * @brief   半波有功/无功合成：P=V·I·cos(Δθ)/10000，Q=V·I·sin(Δθ)/10000
 *
 * 依赖 out 中已有相电压、相电流、相角；写出 Pa…/Qa…/Ptot/Qtot。
 *
 * @param[in,out] out  读写 realtime
 * @param[in]     cfg  接线模式
 *
 * @return 0 成功；接线非法返回 -1
 */
static int32_t power_halfwave_synth(struct msr_data_realtime *out,
                                    const struct msr_config *cfg)
{
    double pa = 0, pb = 0, pc = 0;
    double qa = 0, qb = 0, qc = 0;

    switch (cfg->user.wiremode) {
    case WIREMODE_P1W2:
        pa = (double)out->Va * (double)out->Ia
             * cos((out->AngleUa - out->AngleIa) * PI / 18000.0) / 10000.0;
        qa = (double)out->Va * (double)out->Ia
             * sin((out->AngleUa - out->AngleIa) * PI / 18000.0) / 10000.0;
        break;
    case WIREMODE_P3W3:
    case WIREMODE_P3W4:
        pa = (double)out->Va * (double)out->Ia
             * cos((out->AngleUa - out->AngleIa) * PI / 18000.0) / 10000.0;
        pb = (double)out->Vb * (double)out->Ib
             * cos((out->AngleUb - out->AngleIb) * PI / 18000.0) / 10000.0;
        pc = (double)out->Vc * (double)out->Ic
             * cos((out->AngleUc - out->AngleIc) * PI / 18000.0) / 10000.0;
        qa = (double)out->Va * (double)out->Ia
             * sin((out->AngleUa - out->AngleIa) * PI / 18000.0) / 10000.0;
        qb = (double)out->Vb * (double)out->Ib
             * sin((out->AngleUb - out->AngleIb) * PI / 18000.0) / 10000.0;
        qc = (double)out->Vc * (double)out->Ic
             * sin((out->AngleUc - out->AngleIc) * PI / 18000.0) / 10000.0;
        break;
    default:
        return -1;
    }

    out->Pa = (int32_t)pa;
    out->Pb = (int32_t)pb;
    out->Pc = (int32_t)pc;
    out->Qa = (int32_t)qa;
    out->Qb = (int32_t)qb;
    out->Qc = (int32_t)qc;
    out->Ptot = out->Pa + out->Pb + out->Pc;
    out->Qtot = out->Qa + out->Qb + out->Qc;
    return 0;
}

/**
 * @brief   半波功率合成（相序开启：V·I·角 → P/Q/S）
 *
 * P = V×I×cos(Δθ)/10000；Q = V×I×sin(Δθ)/10000；S = sqrt(P²+Q²)。
 * 依赖 out 中已有相电压、相电流、相角。
 *
 * @param[in,out] out  读写 realtime
 * @param[in]     cfg  接线模式
 *
 * @return 0 成功；参数非法或接线非法返回 -1
 */
int32_t algorithm_halfwave_power(struct msr_data_realtime *out,
                           const struct msr_config *cfg)
{
    int32_t ret;

    if (out == NULL || cfg == NULL) {
        return -1;
    }

    ret = power_halfwave_synth(out, cfg);
    if (ret != 0) {
        return ret;
    }

    power_halfwave_apparent_calc(out);
    return 0;
}

/**
 * @brief   半波视在：由已有 P/Q 算 S = sqrt(P²+Q²)
 *
 * P/Q 须已由芯片路径或其它算子写出。
 *
 * @param[in,out] out  读写 Sa/Sb/Sc/Stot
 *
 * @return 0 成功；out==NULL 返回 -1
 */
int32_t algorithm_halfwave_apparent(struct msr_data_realtime *out)
{
    if (out == NULL) {
        return -1;
    }

    power_halfwave_apparent_calc(out);
    return 0;
}
