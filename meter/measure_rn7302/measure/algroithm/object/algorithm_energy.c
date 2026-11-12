/**
 * @file        algorithm_energy.c
 * @brief       电能计算（余数 / 实时 / 四象限）
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   电能计算
 *  2026-10-28  yansh    1.0.1   合并四象限
 */
#include <stddef.h>
#include "algorithm_energy.h"


/**
 * @brief   单通道余数进位（与 Globird energy_remain_handle 一致）
 *
 * @param[in,out] remain          跨拍余数
 * @param[out]    net             本拍净增量（格数）
 * @param[in]     value           芯片脉冲增量
 * @param[in]     pt, ct          互感器变比
 * @param[in]     pulse_constant  脉冲常数 imp/kWh
 * @param[in]     start_pqs       门控功率；0 则本通道不计度
 *
 * @return 1 有净增量；0 无增量或丢弃
 */
static int32_t energy_remain_handle(uint64_t *remain,
                                    uint64_t *net,
                                    uint32_t value,
                                    uint32_t pt,
                                    uint32_t ct,
                                    uint32_t pulse_constant,
                                    int32_t start_pqs)
{
    uint64_t value_tmp;
    uint64_t remain_tmp;
    uint64_t denom;

    if (start_pqs == 0) {
        *net = 0;
        return 0;
    }
    if (pulse_constant == 0) {
        *net = 0;
        return 0;
    }
    if (value > MSR_ENERGY_PULSE_MAX) {
        *net = 0;
        return 0;
    }

    value_tmp = (uint64_t)value * ct * pt;
    remain_tmp = value_tmp + (*remain);
    denom = (uint64_t)MSR_ENERGY_PRECISION * pulse_constant;
    *remain = remain_tmp % denom;
    *net = remain_tmp / denom;
    return (*net > 0) ? 1 : 0;
}

/**
 * @brief   电能余数处理，写出本拍净增量 eng->net
 *
 * 对各通道：脉冲×PT×CT 累加到 remain，满一格进位写入 net。
 * 对应相功率为 0、脉冲常数为 0 或脉冲异常偏大时本通道 net=0。
 * P3W4 下合相净增量由三相之和重算。
 *
 * @param[in,out] eng  全波或基波各持有一份电能状态
 * @param[in]     cfg  pt/ct/wiremode/pulse_constant
 * @param[in]     in   芯片电能增量快照（PosEP、NegEP、ES 等字段）
 * @param[in]     pwr  本拍功率，作计度门控（P/Q/S==0 不计）
 *
 * @return 0 成功；参数为空返回 -1
 */
int32_t algorithm_energy_remain(struct msr_energy_realtime *eng,
                          const struct msr_config *cfg,
                          const struct msr_input_energy *in,
                          const struct msr_data_realtime *pwr)
{
    struct msr_energy_channels *rm;
    struct msr_energy_channels *net;
    uint32_t pt, ct, pulse;

    if (eng == NULL || cfg == NULL || in == NULL || pwr == NULL) {
        return -1;
    }

    rm = &eng->remain;
    net = &eng->net;
    pt = cfg->user.pt;
    ct = cfg->user.ct;
    pulse = cfg->user.pulse_constant;

    energy_remain_handle(&rm->kWhImp[0], &net->kWhImp[0],
        (uint32_t)in->PosEPa, pt, ct, pulse, pwr->Pa);
    energy_remain_handle(&rm->kWhImp[1], &net->kWhImp[1],
        (uint32_t)in->PosEPb, pt, ct, pulse, pwr->Pb);
    energy_remain_handle(&rm->kWhImp[2], &net->kWhImp[2],
        (uint32_t)in->PosEPc, pt, ct, pulse, pwr->Pc);
    energy_remain_handle(&rm->kWhImp[3], &net->kWhImp[3],
        (uint32_t)in->PosEPt, pt, ct, pulse, pwr->Ptot);

    energy_remain_handle(&rm->kWhExp[0], &net->kWhExp[0],
        (uint32_t)in->NegEPa, pt, ct, pulse, pwr->Pa);
    energy_remain_handle(&rm->kWhExp[1], &net->kWhExp[1],
        (uint32_t)in->NegEPb, pt, ct, pulse, pwr->Pb);
    energy_remain_handle(&rm->kWhExp[2], &net->kWhExp[2],
        (uint32_t)in->NegEPc, pt, ct, pulse, pwr->Pc);
    energy_remain_handle(&rm->kWhExp[3], &net->kWhExp[3],
        (uint32_t)in->NegEPt, pt, ct, pulse, pwr->Ptot);

    energy_remain_handle(&rm->kvarhImp[0], &net->kvarhImp[0],
        (uint32_t)in->PosEQa, pt, ct, pulse, pwr->Qa);
    energy_remain_handle(&rm->kvarhImp[1], &net->kvarhImp[1],
        (uint32_t)in->PosEQb, pt, ct, pulse, pwr->Qb);
    energy_remain_handle(&rm->kvarhImp[2], &net->kvarhImp[2],
        (uint32_t)in->PosEQc, pt, ct, pulse, pwr->Qc);
    energy_remain_handle(&rm->kvarhImp[3], &net->kvarhImp[3],
        (uint32_t)in->PosEQt, pt, ct, pulse, pwr->Qtot);

    energy_remain_handle(&rm->kvarhExp[0], &net->kvarhExp[0],
        (uint32_t)in->NegEQa, pt, ct, pulse, pwr->Qa);
    energy_remain_handle(&rm->kvarhExp[1], &net->kvarhExp[1],
        (uint32_t)in->NegEQb, pt, ct, pulse, pwr->Qb);
    energy_remain_handle(&rm->kvarhExp[2], &net->kvarhExp[2],
        (uint32_t)in->NegEQc, pt, ct, pulse, pwr->Qc);
    energy_remain_handle(&rm->kvarhExp[3], &net->kvarhExp[3],
        (uint32_t)in->NegEQt, pt, ct, pulse, pwr->Qtot);

    energy_remain_handle(&rm->kVAh[0], &net->kVAh[0],
        (uint32_t)in->ESa, pt, ct, pulse, (int32_t)pwr->Sa);
    energy_remain_handle(&rm->kVAh[1], &net->kVAh[1],
        (uint32_t)in->ESb, pt, ct, pulse, (int32_t)pwr->Sb);
    energy_remain_handle(&rm->kVAh[2], &net->kVAh[2],
        (uint32_t)in->ESc, pt, ct, pulse, (int32_t)pwr->Sc);
    energy_remain_handle(&rm->kVAh[3], &net->kVAh[3],
        (uint32_t)in->Estv, pt, ct, pulse, (int32_t)pwr->Stot);

    net->kWhTot[0] = net->kWhImp[0] + net->kWhExp[0];
    net->kWhTot[1] = net->kWhImp[1] + net->kWhExp[1];
    net->kWhTot[2] = net->kWhImp[2] + net->kWhExp[2];
    net->kWhTot[3] = net->kWhImp[3] + net->kWhExp[3];
    net->kvarhTot[0] = net->kvarhImp[0] + net->kvarhExp[0];
    net->kvarhTot[1] = net->kvarhImp[1] + net->kvarhExp[1];
    net->kvarhTot[2] = net->kvarhImp[2] + net->kvarhExp[2];
    net->kvarhTot[3] = net->kvarhImp[3] + net->kvarhExp[3];

    if (cfg->user.wiremode == WIREMODE_P3W4) {
        net->kWhImp[3] = net->kWhImp[0] + net->kWhImp[1] + net->kWhImp[2];
        net->kWhExp[3] = net->kWhExp[0] + net->kWhExp[1] + net->kWhExp[2];
        net->kvarhImp[3] = net->kvarhImp[0] + net->kvarhImp[1] + net->kvarhImp[2];
        net->kvarhExp[3] = net->kvarhExp[0] + net->kvarhExp[1] + net->kvarhExp[2];
        net->kVAh[3] = net->kVAh[0] + net->kVAh[1] + net->kVAh[2];
    }

    return 0;
}

/**
 * @brief   单通道：把 net 累加到高精度实时值并回绕
 *
 * @param[in]     net   本拍净增量；为 0 则不累加
 * @param[in,out] real  高精度实时累计量（模 MSR_ENERGY_MAX_HP+1）
 */
static void energy_accum_hp(uint64_t *net, uint64_t *real)
{
    if (*net > 0) {
        *real = (*real + *net) % (MSR_ENERGY_MAX_HP + 1);
    }
}

/**
 * @brief   高精度实时电能累加（8+3 位）
 *
 * 将 eng->net 累加到 eng->realtime_hp，按 MSR_ENERGY_MAX_HP 回绕；
 * P3W4 合相由三相之和重算。
 *
 * @param[in,out] eng  读写 net / realtime_hp
 * @param[in]     cfg  接线模式
 *
 * @return 0 成功；参数为空返回 -1
 */
int32_t algorithm_energy_realtime_hp(struct msr_energy_realtime *eng,
                               const struct msr_config *cfg)
{
    struct msr_energy_channels *net;
    struct msr_energy_channels *hp;
    int i;

    if (eng == NULL || cfg == NULL) {
        return -1;
    }

    net = &eng->net;
    hp = &eng->realtime_hp;

    for (i = 0; i < MSR_PHASE_NUM; i++) {
        energy_accum_hp(&net->kWhImp[i], &hp->kWhImp[i]);
        energy_accum_hp(&net->kWhExp[i], &hp->kWhExp[i]);
        energy_accum_hp(&net->kvarhImp[i], &hp->kvarhImp[i]);
        energy_accum_hp(&net->kvarhExp[i], &hp->kvarhExp[i]);
        energy_accum_hp(&net->kVAh[i], &hp->kVAh[i]);
        energy_accum_hp(&net->kWhTot[i], &hp->kWhTot[i]);
        energy_accum_hp(&net->kvarhTot[i], &hp->kvarhTot[i]);

        hp->kWhTot[i] = (hp->kWhImp[i] + hp->kWhExp[i]) % (MSR_ENERGY_MAX_HP + 1);
        hp->kvarhTot[i] = (hp->kvarhImp[i] + hp->kvarhExp[i]) % (MSR_ENERGY_MAX_HP + 1);
    }

    if (cfg->user.wiremode == WIREMODE_P3W4) {
        hp->kWhImp[3] = (hp->kWhImp[0] + hp->kWhImp[1] + hp->kWhImp[2]) % (MSR_ENERGY_MAX_HP + 1);
        hp->kWhExp[3] = (hp->kWhExp[0] + hp->kWhExp[1] + hp->kWhExp[2]) % (MSR_ENERGY_MAX_HP + 1);
        hp->kvarhImp[3] = (hp->kvarhImp[0] + hp->kvarhImp[1] + hp->kvarhImp[2]) % (MSR_ENERGY_MAX_HP + 1);
        hp->kvarhExp[3] = (hp->kvarhExp[0] + hp->kvarhExp[1] + hp->kvarhExp[2]) % (MSR_ENERGY_MAX_HP + 1);
        hp->kVAh[3] = (hp->kVAh[0] + hp->kVAh[1] + hp->kVAh[2]) % (MSR_ENERGY_MAX_HP + 1);
        hp->kWhTot[3] = (hp->kWhTot[0] + hp->kWhTot[1] + hp->kWhTot[2]) % (MSR_ENERGY_MAX_HP + 1);
        hp->kvarhTot[3] = (hp->kvarhTot[0] + hp->kvarhTot[1] + hp->kvarhTot[2]) % (MSR_ENERGY_MAX_HP + 1);
    }

    return 0;
}

/**
 * @brief   单通道：高精度 8+3 → 通讯 8+1（÷100）
 *
 * @param[in]  hp    高精度实时值
 * @param[out] real  通讯实时值（模 MSR_ENERGY_MAX_8D1+1）
 */
static void energy_from_hp(uint64_t *hp, uint64_t *real)
{
    *real = (*hp / 100) % (MSR_ENERGY_MAX_8D1 + 1);
}

/**
 * @brief   通讯实时电能（8+1 位）
 *
 * eng->realtime = eng->realtime_hp / 100，按 MSR_ENERGY_MAX_8D1 回绕；
 * P3W4 合相由三相之和重算。
 *
 * @param[in,out] eng  读 realtime_hp，写 realtime
 * @param[in]     cfg  接线模式
 *
 * @return 0 成功；参数为空返回 -1
 */
int32_t algorithm_energy_realtime(struct msr_energy_realtime *eng,
                            const struct msr_config *cfg)
{
    struct msr_energy_channels *hp;
    struct msr_energy_channels *rt;
    int i;

    if (eng == NULL || cfg == NULL) {
        return -1;
    }

    hp = &eng->realtime_hp;
    rt = &eng->realtime;

    for (i = 0; i < MSR_PHASE_NUM; i++) {
        energy_from_hp(&hp->kWhImp[i], &rt->kWhImp[i]);
        energy_from_hp(&hp->kWhExp[i], &rt->kWhExp[i]);
        energy_from_hp(&hp->kvarhImp[i], &rt->kvarhImp[i]);
        energy_from_hp(&hp->kvarhExp[i], &rt->kvarhExp[i]);
        energy_from_hp(&hp->kVAh[i], &rt->kVAh[i]);
        energy_from_hp(&hp->kWhTot[i], &rt->kWhTot[i]);
        energy_from_hp(&hp->kvarhTot[i], &rt->kvarhTot[i]);
    }

    if (cfg->user.wiremode == WIREMODE_P3W4) {
        rt->kWhImp[3] = (rt->kWhImp[0] + rt->kWhImp[1] + rt->kWhImp[2]) % (MSR_ENERGY_MAX_8D1 + 1);
        rt->kWhExp[3] = (rt->kWhExp[0] + rt->kWhExp[1] + rt->kWhExp[2]) % (MSR_ENERGY_MAX_8D1 + 1);
        rt->kvarhImp[3] = (rt->kvarhImp[0] + rt->kvarhImp[1] + rt->kvarhImp[2]) % (MSR_ENERGY_MAX_8D1 + 1);
        rt->kvarhExp[3] = (rt->kvarhExp[0] + rt->kvarhExp[1] + rt->kvarhExp[2]) % (MSR_ENERGY_MAX_8D1 + 1);
        rt->kVAh[3] = (rt->kVAh[0] + rt->kVAh[1] + rt->kVAh[2]) % (MSR_ENERGY_MAX_8D1 + 1);
        rt->kWhTot[3] = (rt->kWhTot[0] + rt->kWhTot[1] + rt->kWhTot[2]) % (MSR_ENERGY_MAX_8D1 + 1);
        rt->kvarhTot[3] = (rt->kvarhTot[0] + rt->kvarhTot[1] + rt->kvarhTot[2]) % (MSR_ENERGY_MAX_8D1 + 1);
    }

    return 0;
}

/**
 * @brief   由有功/无功符号得到象限索引
 *
 * @param[in] p  有功（>=0 为正向）
 * @param[in] q  无功（>=0 为正向）
 *
 * @return MSR_AREA_Q1..Q4
 */
static int fq_area_of(int32_t p, int32_t q)
{
    if (p >= 0) {
        return (q >= 0) ? MSR_AREA_Q1 : MSR_AREA_Q4;
    }
    return (q >= 0) ? MSR_AREA_Q2 : MSR_AREA_Q3;
}

/**
 * @brief   8+1 累加：delta 为 realtime_hp 精度增量 ÷100
 */
static void fq_add_u32(uint32_t *dst, uint64_t net_hp)
{
    uint64_t add = net_hp / 100U;

    if (add > 0U) {
        *dst = (uint32_t)(((uint64_t)(*dst) + add) % (MSR_ENERGY_MAX_8D1 + 1U));
    }
}

/**
 * @brief   单相累加到对应象限
 *
 * @param[in,out] out   四象限结果
 * @param[in]     ph    相索引 0..3
 * @param[in]     p, q  该相有功/无功
 * @param[in]     net   净增量
 */
static void fq_accum_phase(struct msr_energy_fourquadrant *out,
                           int ph,
                           int32_t p,
                           int32_t q,
                           const struct msr_energy_channels *net)
{
    int area = fq_area_of(p, q);

    if (p >= 0) {
        fq_add_u32(&out->kWh[ph][area], net->kWhImp[ph]);
    } else {
        fq_add_u32(&out->kWh[ph][area], net->kWhExp[ph]);
    }

    if (q >= 0) {
        fq_add_u32(&out->kvarh[ph][area], net->kvarhImp[ph]);
    } else {
        fq_add_u32(&out->kvarh[ph][area], net->kvarhExp[ph]);
    }
}

/**
 * @brief   四象限电能累加（见 algorithm_energy.h）
 */
int32_t algorithm_energy_fourquadrant(struct msr_energy_fourquadrant *out,
                                const struct msr_energy_channels *net,
                                const struct msr_data_realtime *pwr)
{
    if (out == NULL || net == NULL || pwr == NULL) {
        return -1;
    }

    fq_accum_phase(out, 0, pwr->Pa, pwr->Qa, net);
    fq_accum_phase(out, 1, pwr->Pb, pwr->Qb, net);
    fq_accum_phase(out, 2, pwr->Pc, pwr->Qc, net);
    fq_accum_phase(out, 3, pwr->Ptot, pwr->Qtot, net);
    return 0;
}

/**
 * @brief   多费率电能累加
 *
 * @param[in,out] out  多费率电能
 * @param[in]     net  本拍净增量
 * @param[in]     pwr  本拍功率
 * @param[in]     data  数据
 */
int32_t algorithm_energy_multirate(struct msr_energy_multirate *out,
                                const struct msr_energy_channels *net,
                                const struct msr_data *data)
{
    if (out == NULL || net == NULL || data == NULL) {
        return -1;
    }

    uint16_t rate = data->multirate.current_rate;

    if (rate >= MSR_MULTIRATE_NUM) {
        rate = 0;
    }

    out->rates[rate].EPimp = net->kWhImp[3];
    out->rates[rate].EPexp = net->kWhExp[3];
    out->rates[rate].EQimp = net->kvarhImp[3];
    out->rates[rate].EQexp = net->kvarhExp[3];

    return 0;
}
