/**
 * @file        msr_data.h
 * @brief       测量数据结构（电参 / 电能 / 需量）
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   测量数据结构
 *  2026-10-28  yansh    1.0.1   合并 energy / demand / multirate
 *  2026-10-28  yansh    1.0.2   移除复费率数据结构
 */
#ifndef MSR_DATA_H
#define MSR_DATA_H

#include <stdint.h>
#include <time.h>
#include "msr_config.h"

/** 半波环缓冲槽数（与 Globird HALFWAVE_NUM 一致） */
#define MSR_HALFWAVE_SLOTS  2

/**
 * 半波电压跨拍工作区：存芯片原始有效值，由算子维护 wr。
 * 产品 abort 丢半波对时应清零本结构。
 */
struct msr_halfwave_volt_state {
    int32_t  ua[MSR_HALFWAVE_SLOTS];
    int32_t  ub[MSR_HALFWAVE_SLOTS];
    int32_t  uc[MSR_HALFWAVE_SLOTS];
    uint32_t wr;   /* 下一写入槽 */
};

/**
 * 半波电流跨拍工作区：存芯片原始有效值，由算子维护 wr。
 * 产品 abort 丢半波对时应清零本结构。
 */
struct msr_halfwave_curr_state {
    int32_t  ia[MSR_HALFWAVE_SLOTS];
    int32_t  ib[MSR_HALFWAVE_SLOTS];
    int32_t  ic[MSR_HALFWAVE_SLOTS];
    uint32_t wr;
};

/**
 * @brief   一拍电参实时结果（全波 / 基波 / 半波各一份）
 *
 * 相角单位 0.01°；频率 Freq_hp×10000、Freq×100。
 */
struct msr_data_realtime {
    uint32_t Va,  Vb,  Vc,  VlnAvg, Vn;      /* 相电压、零地、平均相电压 0.01V */
    uint32_t Vab, Vbc, Vca, VllAvg;          /* 线电压、平均线电压 0.01V */
    uint32_t Ia,  Ib,  Ic,  Iavg,   In;      /* 相电流、平均电流、中性线 0.001A */
    int32_t  Pa,  Pb,  Pc,  Ptot;            /* 有功 0.1W */
    int32_t  Qa,  Qb,  Qc,  Qtot;            /* 无功 0.1var */
    uint32_t Sa,  Sb,  Sc,  Stot;            /* 视在 0.1VA */
    int16_t  PFa, PFb, PFc, PFtot;           /* 功因 0.001 */
    uint32_t Freq;                           /* 频率 0.0001Hz */
    int16_t  AngleUa, AngleUb, AngleUc;      /* 电压相角 0.01° */
    int16_t  AngleIa, AngleIb, AngleIc;      /* 电流相角 0.01° */

    int32_t  Pab, Pca;                       /* 线有功 0.1W */
    int32_t  Qab, Qca;                       /* 线无功 0.1var */
    uint32_t Sab, Sca;                       /* 线视在 0.1VA */
};

/** 谐波占位（后续扩展） */
struct msr_data_harmonic {
    uint16_t res;
};

/* -------------------------------------------------------------------------- */
/* 电能                                                                       */
/* -------------------------------------------------------------------------- */

/**
 * 电能通道组：下标 0/1/2/3 = A/B/C/合相
 */
struct msr_energy_channels {
    uint64_t kWhImp[MSR_PHASE_NUM];
    uint64_t kWhExp[MSR_PHASE_NUM];
    uint64_t kvarhImp[MSR_PHASE_NUM];
    uint64_t kvarhExp[MSR_PHASE_NUM];
    uint64_t kVAh[MSR_PHASE_NUM];
    uint64_t kWhTot[MSR_PHASE_NUM];
    uint64_t kvarhTot[MSR_PHASE_NUM];
};

/**
 * 一份波形（全波或基波）的电能跨拍状态。
 * 产品为全波、基波各持有一份；半波不算电能。
 */
struct msr_energy_realtime {
    struct msr_energy_channels remain;      /* 余数 */
    struct msr_energy_channels net;         /* 本拍净增量（原 NetEnergy） */
    struct msr_energy_channels realtime_hp; /* 高精度实时 8+3 */
    struct msr_energy_channels realtime;    /* 通讯实时 8+1 */
};

/**
 * @brief   四象限电能（8+1 刻度，与 Globird fourquadrant 一致）
 *
 * kWh/kvarh[相][象限]；相 = A/B/C/合相，象限 = MSR_AREA_Q1..Q4。
 */
struct msr_energy_fourquadrant {
    uint32_t kWh[MSR_PHASE_NUM][MSR_AREA_NUM];
    uint32_t kvarh[MSR_PHASE_NUM][MSR_AREA_NUM];
};

struct msr_energy_multirate {
    struct msr_energy_multirate_rates {
        uint32_t EPimp;
        uint32_t EPexp;
        uint32_t EQimp;
        uint32_t EQexp;
    }rates[MSR_MULTIRATE_NUM];
};

struct msr_data_energy {
    struct msr_energy_realtime     realtime;
    struct msr_energy_fourquadrant fourquadrant;
    struct msr_energy_multirate    multirate;
};

struct msr_data_multirate {
    uint16_t current_rate;
};

/** 测量核心对外数据块：三波形 realtime + 谐波 */
struct msr_data {
    struct msr_data_realtime  realtime_full, realtime_fund, realtime_half;
    struct msr_data_harmonic  harmonic;
    struct msr_data_energy    energy_full, energy_fund;
    struct msr_data_multirate multirate;
};

/* -------------------------------------------------------------------------- */
/* 需量                                                                       */
/* -------------------------------------------------------------------------- */

/**
 * @brief   需量周期内累加缓冲（每秒累加，周期末取平均）
 */
struct msr_demand_sum {
    uint64_t IncIa, IncIb, IncIc, IncIAvg, IncIn;
    int64_t  IncPa, IncPb, IncPc, IncPtot;
    int64_t  IncPa_neg, IncPb_neg, IncPc_neg, IncPtot_neg;
    int64_t  IncQa, IncQb, IncQc, IncQtot;
    int64_t  IncQa_neg, IncQb_neg, IncQc_neg, IncQtot_neg;
    uint64_t IncSa, IncSb, IncSc, IncStot;
    uint16_t DmdIncBufCnt;
};

/** 需量配置 */
struct msr_demand_cfg {
    uint16_t window;        /* 周期（分钟）：1/5/10/15/30 */
    uint16_t mode;          /* 需量方式（产品保留） */
    uint16_t slip_window;   /* 滑差（产品保留） */
};

/** 实时需量结果 */
struct msr_demand {
    uint32_t Ia, Ib, Ic, IAvg, In;
    int32_t  Pa, Pb, Pc, Ptot;
    int32_t  Qa, Qb, Qc, Qtot;
    uint32_t Sa, Sb, Sc, Stot;
    int32_t  Pa_neg, Pb_neg, Pc_neg, Ptot_neg;
    int32_t  Qa_neg, Qb_neg, Qc_neg, Qtot_neg;
};

/** 带时间戳的最值单元 */
struct msr_demand_max_u32 {
    uint32_t value;
    uint32_t time;
};

struct msr_demand_max_s32 {
    int32_t  value;
    uint32_t time;
};

struct msr_demand_max_current {
    struct msr_demand_max_u32 Ia, Ib, Ic, IAvg, In;
};

struct msr_demand_max_power {
    struct msr_demand_max_s32 Pa, Pb, Pc, Ptot;
    struct msr_demand_max_s32 Qa, Qb, Qc, Qtot;
    struct msr_demand_max_u32 Sa, Sb, Sc, Stot;
    struct msr_demand_max_s32 Pa_neg, Pb_neg, Pc_neg, Ptot_neg;
    struct msr_demand_max_s32 Qa_neg, Qb_neg, Qc_neg, Qtot_neg;
};

struct msr_demand_max_base {
    struct msr_demand_max_current current;
    struct msr_demand_max_power  power;
};

/** 日/月最大需量 */
struct msr_demand_max {
    struct msr_demand_max_base day;
    struct msr_demand_max_base month;
};

/**
 * @brief   需量跨拍状态（每表计一份，避免 static）
 */
struct msr_demand_state {
    struct msr_demand_sum sum;
    struct msr_demand_cfg cfg;
    int8_t  last_min;
    time_t  last_time;
};

#endif
