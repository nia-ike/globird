/**
 * @file        algorithm_demand.c
 * @brief       需量计算（实时 + 日/月最值）
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-28  yansh    1.0.0   需量计算
 */
#include <stddef.h>
#include <string.h>
#include "algorithm_demand.h"

#define sum(x, y)      ((x) + (y))
#define sum_pos(x, y)  (((x) > 0) ? ((x) + (y)) : (y))
#define sum_neg(x, y)  (((x) < 0) ? ((x) + (y)) : (y))

/**
 * @brief   清零周期累加缓冲
 *
 * @param[out] sum  累加缓冲
 */
static void demand_sum_flush(struct msr_demand_sum *sum)
{
    memset(sum, 0, sizeof(*sum));
}

/**
 * @brief   初始化需量状态与配置（见 algorithm_demand.h）
 */
int32_t algorithm_demand_init(struct msr_demand_state *st,
                        uint16_t window,
                        uint16_t mode,
                        uint16_t slip)
{
    if (st == NULL) {
        return -1;
    }

    memset(st, 0, sizeof(*st));
    st->cfg.window = window;
    st->cfg.mode = mode;
    st->cfg.slip_window = slip;
    if (st->cfg.window == 0U || st->cfg.window > MSR_DMD_WINDOW_MAX) {
        st->cfg.window = MSR_DMD_WINDOW_MAX;
    }
    st->last_min = -1;
    st->last_time = 0;
    return 0;
}

/**
 * @brief   清零实时需量结果（见 algorithm_demand.h）
 */
int32_t algorithm_demand_clear(struct msr_demand *out)
{
    if (out == NULL) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    return 0;
}

/**
 * @brief   周期到：累加缓冲 → 实时需量平均
 */
static void demand_flush_to_out(struct msr_demand *out, struct msr_demand_sum *sum)
{
    uint16_t n = sum->DmdIncBufCnt;

    if (n == 0U) {
        return;
    }

    out->Ia   = (uint32_t)(sum->IncIa   / n);
    out->Ib   = (uint32_t)(sum->IncIb   / n);
    out->Ic   = (uint32_t)(sum->IncIc   / n);
    out->IAvg = (uint32_t)(sum->IncIAvg / n);
    out->In   = (uint32_t)(sum->IncIn   / n);

    out->Pa   = (int32_t)(sum->IncPa   / n);
    out->Pb   = (int32_t)(sum->IncPb   / n);
    out->Pc   = (int32_t)(sum->IncPc   / n);
    out->Ptot = (int32_t)(sum->IncPtot / n);
    out->Qa   = (int32_t)(sum->IncQa   / n);
    out->Qb   = (int32_t)(sum->IncQb   / n);
    out->Qc   = (int32_t)(sum->IncQc   / n);
    out->Qtot = (int32_t)(sum->IncQtot / n);
    out->Sa   = (uint32_t)(sum->IncSa   / n);
    out->Sb   = (uint32_t)(sum->IncSb   / n);
    out->Sc   = (uint32_t)(sum->IncSc   / n);
    out->Stot = (uint32_t)(sum->IncStot / n);

    out->Pa_neg   = (int32_t)(sum->IncPa_neg   / n);
    out->Pb_neg   = (int32_t)(sum->IncPb_neg   / n);
    out->Pc_neg   = (int32_t)(sum->IncPc_neg   / n);
    out->Ptot_neg = (int32_t)(sum->IncPtot_neg / n);
    out->Qa_neg   = (int32_t)(sum->IncQa_neg   / n);
    out->Qb_neg   = (int32_t)(sum->IncQb_neg   / n);
    out->Qc_neg   = (int32_t)(sum->IncQc_neg   / n);
    out->Qtot_neg = (int32_t)(sum->IncQtot_neg / n);

    demand_sum_flush(sum);
}

/**
 * @brief   本秒累加全波电参
 */
static void demand_accum_sec(struct msr_demand_sum *sum,
                             const struct msr_data_realtime *rt)
{
    sum->IncIa   = sum(rt->Ia,   sum->IncIa);
    sum->IncIb   = sum(rt->Ib,   sum->IncIb);
    sum->IncIc   = sum(rt->Ic,   sum->IncIc);
    sum->IncIAvg = sum(rt->Iavg, sum->IncIAvg);
    sum->IncIn   = sum(rt->In,   sum->IncIn);

    sum->IncPa   = sum_pos(rt->Pa,   sum->IncPa);
    sum->IncPb   = sum_pos(rt->Pb,   sum->IncPb);
    sum->IncPc   = sum_pos(rt->Pc,   sum->IncPc);
    sum->IncPtot = sum_pos(rt->Ptot, sum->IncPtot);
    sum->IncQa   = sum_pos(rt->Qa,   sum->IncQa);
    sum->IncQb   = sum_pos(rt->Qb,   sum->IncQb);
    sum->IncQc   = sum_pos(rt->Qc,   sum->IncQc);
    sum->IncQtot = sum_pos(rt->Qtot, sum->IncQtot);
    sum->IncSa   = sum_pos((int32_t)rt->Sa,   sum->IncSa);
    sum->IncSb   = sum_pos((int32_t)rt->Sb,   sum->IncSb);
    sum->IncSc   = sum_pos((int32_t)rt->Sc,   sum->IncSc);
    sum->IncStot = sum_pos((int32_t)rt->Stot, sum->IncStot);

    sum->IncPa_neg   = sum_neg(rt->Pa,   sum->IncPa_neg);
    sum->IncPb_neg   = sum_neg(rt->Pb,   sum->IncPb_neg);
    sum->IncPc_neg   = sum_neg(rt->Pc,   sum->IncPc_neg);
    sum->IncPtot_neg = sum_neg(rt->Ptot, sum->IncPtot_neg);
    sum->IncQa_neg   = sum_neg(rt->Qa,   sum->IncQa_neg);
    sum->IncQb_neg   = sum_neg(rt->Qb,   sum->IncQb_neg);
    sum->IncQc_neg   = sum_neg(rt->Qc,   sum->IncQc_neg);
    sum->IncQtot_neg = sum_neg(rt->Qtot, sum->IncQtot_neg);

    sum->DmdIncBufCnt++;
}

/**
 * @brief   需量每秒处理（见 algorithm_demand.h）
 */
int32_t algorithm_demand_process(struct msr_demand *out,
                           struct msr_demand_state *st,
                           const struct msr_data_realtime *rt,
                           time_t now)
{
    struct tm tm_now;
    uint16_t window;

    if (out == NULL || st == NULL || rt == NULL) {
        return -1;
    }
    if (localtime_r(&now, &tm_now) == NULL) {
        return -1;
    }

    window = st->cfg.window;
    if (window == 0U) {
        window = 1U;
    }

    /* 周期起点：分钟整除 window 且秒为 0 */
    if (((tm_now.tm_min % (int)window) == 0)
        && (tm_now.tm_sec == 0)
        && (st->last_min != (int8_t)tm_now.tm_min)) {
        demand_flush_to_out(out, &st->sum);
    }

    if (st->last_time != now) {
        demand_accum_sec(&st->sum, rt);
    }

    st->last_time = now;
    st->last_min = (int8_t)tm_now.tm_min;
    return 0;
}

/**
 * @brief   更新 uint32 最值（更大则替换）
 */
static void dmd_max_u32(struct msr_demand_max_u32 *m, uint32_t v, uint32_t t)
{
    if (v > m->value) {
        m->value = v;
        m->time = t;
    }
}

/**
 * @brief   更新 int32 最值（按绝对值更大）
 */
static void dmd_max_s32_abs(struct msr_demand_max_s32 *m, int32_t v, uint32_t t)
{
    int32_t a = (v < 0) ? -v : v;
    int32_t b = (m->value < 0) ? -m->value : m->value;

    if (a > b) {
        m->value = v;
        m->time = t;
    }
}

/**
 * @brief   用一份需量刷新一组最值基座
 */
static void dmd_max_base_update(struct msr_demand_max_base *base,
                                const struct msr_demand *dmd,
                                uint32_t now)
{
    dmd_max_u32(&base->current.Ia,   dmd->Ia,   now);
    dmd_max_u32(&base->current.Ib,   dmd->Ib,   now);
    dmd_max_u32(&base->current.Ic,   dmd->Ic,   now);
    dmd_max_u32(&base->current.IAvg, dmd->IAvg, now);
    dmd_max_u32(&base->current.In,   dmd->In,   now);

    dmd_max_s32_abs(&base->power.Pa,   dmd->Pa,   now);
    dmd_max_s32_abs(&base->power.Pb,   dmd->Pb,   now);
    dmd_max_s32_abs(&base->power.Pc,   dmd->Pc,   now);
    dmd_max_s32_abs(&base->power.Ptot, dmd->Ptot, now);
    dmd_max_s32_abs(&base->power.Qa,   dmd->Qa,   now);
    dmd_max_s32_abs(&base->power.Qb,   dmd->Qb,   now);
    dmd_max_s32_abs(&base->power.Qc,   dmd->Qc,   now);
    dmd_max_s32_abs(&base->power.Qtot, dmd->Qtot, now);

    dmd_max_u32(&base->power.Sa,   dmd->Sa,   now);
    dmd_max_u32(&base->power.Sb,   dmd->Sb,   now);
    dmd_max_u32(&base->power.Sc,   dmd->Sc,   now);
    dmd_max_u32(&base->power.Stot, dmd->Stot, now);

    dmd_max_s32_abs(&base->power.Pa_neg,   dmd->Pa_neg,   now);
    dmd_max_s32_abs(&base->power.Pb_neg,   dmd->Pb_neg,   now);
    dmd_max_s32_abs(&base->power.Pc_neg,   dmd->Pc_neg,   now);
    dmd_max_s32_abs(&base->power.Ptot_neg, dmd->Ptot_neg, now);
    dmd_max_s32_abs(&base->power.Qa_neg,   dmd->Qa_neg,   now);
    dmd_max_s32_abs(&base->power.Qb_neg,   dmd->Qb_neg,   now);
    dmd_max_s32_abs(&base->power.Qc_neg,   dmd->Qc_neg,   now);
    dmd_max_s32_abs(&base->power.Qtot_neg, dmd->Qtot_neg, now);
}

/**
 * @brief   用实时需量刷新日/月最大需量（见 algorithm_demand.h）
 */
int32_t algorithm_demand_max_update(struct msr_demand_max *max,
                              const struct msr_demand *dmd,
                              uint32_t now)
{
    if (max == NULL || dmd == NULL) {
        return -1;
    }
    dmd_max_base_update(&max->day, dmd, now);
    dmd_max_base_update(&max->month, dmd, now);
    return 0;
}

/**
 * @brief   清零最值基座并把时间戳置为 now
 */
static void dmd_max_base_clear(struct msr_demand_max_base *base, uint32_t now)
{
    memset(base, 0, sizeof(*base));
    base->current.Ia.time = now;
    base->current.Ib.time = now;
    base->current.Ic.time = now;
    base->current.IAvg.time = now;
    base->current.In.time = now;
    base->power.Pa.time = now;
    base->power.Pb.time = now;
    base->power.Pc.time = now;
    base->power.Ptot.time = now;
    base->power.Qa.time = now;
    base->power.Qb.time = now;
    base->power.Qc.time = now;
    base->power.Qtot.time = now;
    base->power.Sa.time = now;
    base->power.Sb.time = now;
    base->power.Sc.time = now;
    base->power.Stot.time = now;
    base->power.Pa_neg.time = now;
    base->power.Pb_neg.time = now;
    base->power.Pc_neg.time = now;
    base->power.Ptot_neg.time = now;
    base->power.Qa_neg.time = now;
    base->power.Qb_neg.time = now;
    base->power.Qc_neg.time = now;
    base->power.Qtot_neg.time = now;
}

/**
 * @brief   清零日最大需量（见 algorithm_demand.h）
 */
int32_t algorithm_demand_max_day_clear(struct msr_demand_max *max, uint32_t now)
{
    if (max == NULL) {
        return -1;
    }
    dmd_max_base_clear(&max->day, now);
    return 0;
}

/**
 * @brief   清零月最大需量（见 algorithm_demand.h）
 */
int32_t algorithm_demand_max_month_clear(struct msr_demand_max *max, uint32_t now)
{
    if (max == NULL) {
        return -1;
    }
    dmd_max_base_clear(&max->month, now);
    return 0;
}
