/**
 * @file        msr_config.h
 * @brief       配置文件
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   配置文件
 */
#ifndef MSR_CONFIG_H
#define MSR_CONFIG_H

#include <stdint.h>


enum {
    WIREMODE_P3W4 = 0,
    WIREMODE_P3W3,
    WIREMODE_P1W2,
};

enum {
    PHASEADJMODE_CURRENT = 0,   /* 电流相序调整模式 */
    PHASEAJDMODE_VOLTAGE,       /* 电压相序调整模式 */
};

#define VOL_VAL             22000
#define ADJ_VOL_VAL         22000000        //  U标准 RN7302_ADJ_U_RMS
#define RN7302_Krmsu        (ADJ_VOL_VAL / VOL_VAL)//1000
#define MULTIPLE_PT         100
#define MULTIPLE_I          100
#define MULTIPLE_PTCT       10000           /* pt*ct 缩放，与 Globird 功率一致 */
#define RN7302_Krmsi        1000            /* krms_i 兜底，与 Globird 一致 */
#define RN7302_Krmsp        1000            /* krms_p 兜底 */
#define PI                  3.14159265358979323846
#define ANGLE_MULTIPLE      100
#define ANGLE_J360_MAX      (360 * ANGLE_MULTIPLE)
#define ANGLE_CONVERT(x)    (((double)(x) * 360 * ANGLE_MULTIPLE / (1 << 24)) )

#define POWERFACTOR_MULTIPLE  1000
#define MULTIPLE_UFREQ        10000
#define MSR_FOSC              8192000
#define MSR_UFREQ_NOM         50
#define MSR_UFREQ_MAX         (MSR_UFREQ_NOM * 13 / 10)
#define MSR_UFREQ_MIN         (MSR_UFREQ_NOM * 7 / 10)

#define MSR_PHASE_NUM         4             /* A/B/C/合相 */
#define MSR_ENERGY_PRECISION  10
#define MSR_ENERGY_MAX_HP     99999999999ull  /* 8+3 */
#define MSR_ENERGY_MAX_8D1    999999999ull    /* 8+1 */
#define MSR_ENERGY_PULSE_MAX  825             /* 单拍脉冲上限（异常丢弃） */

#define MSR_AREA_NUM          4             /* 四象限：Q1..Q4 */

#define MSR_DMD_WINDOW_MAX        30        /* 需量周期上限（分钟） */

/** type_i0：0 漏电电流；1 第四路电流（IN 乘 3/2） */
#define MSR_TYPE_I0_LEAK    0
#define MSR_TYPE_I0_CH4     1

#define MSR_MULTIRATE_NUM     8             /* 多费率 */

/** 四象限索引（IEC：以有功/无功符号划分） */
enum {
    MSR_AREA_Q1 = 0,    /* +P +Q */
    MSR_AREA_Q2,        /* -P +Q */
    MSR_AREA_Q3,        /* -P -Q */
    MSR_AREA_Q4,        /* +P -Q */
};


struct cfg_user
{
    uint32_t pt, ct;
    uint32_t wiremode;
    uint16_t type_i0;           /* MSR_TYPE_I0_*，仅全波/基波中性线电流 */
    uint16_t pulse_mode;        /* 脉冲模式：0:有功脉冲;1:无功脉冲 */
    uint32_t pulse_constant;    /* 脉冲常数 imp/kWh，电能余数用 */
    uint16_t phase_adjust_mode; /* 0:电流相序调整;1:电压相序调整 */
    uint16_t phase_adjust[3];   /* PhaseAdjust1/2/3：半波相序重映射 / 电流反向极性 */
};

struct cfg_adjust
{
    /* 校正目标（额定值） */
    uint32_t rated_voltage, rated_current, rated_power, rated_angle;
    /* 校正目标（寄存器值） */
    uint32_t reg_va, reg_vb, reg_vc, reg_vn;
    uint32_t reg_ia, reg_ib, reg_ic, reg_in;
    uint32_t reg_pa, reg_pb, reg_pc;
    /* 精度系数 */
    uint16_t factor_voltage, factor_current, factor_power;
    uint16_t factor_angle, factor_freq, factor_energy, factor_powerfactor;
    /* 偏移校正 */
    uint16_t offset_ia, offset_ib, offset_ic;
};

struct cfg_deadzone
{
    uint16_t va, vb, vc, vn;
    uint16_t ia, ib, ic, in;
    uint16_t pa, pb, pc;
    uint16_t qa, qb, qc;
    uint16_t sa, sb, sc;
};

struct cfg_chip
{
    uint16_t HFConst1;
    uint16_t HFConst2;
    uint16_t IStart_PS;
    uint16_t IStart_Q;
    uint16_t GSUA;
    uint16_t GSUB;
    uint16_t GSUC;
    uint16_t GSIA;
    uint16_t GSIB;
    uint16_t GSIC;
    uint16_t GSIN;
    uint32_t PHSUA;
    uint32_t PHSUB;
    uint32_t PHSUC;
    uint32_t PHSIA;
    uint32_t PHSIB;
    uint32_t PHSIC;
    uint32_t PHSIN;
    uint16_t PA_PHSL;
    uint16_t PB_PHSL;
    uint16_t PC_PHSL;
    uint16_t QA_PHSL;
    uint16_t QB_PHSL;
    uint16_t QC_PHSL;

    uint16_t PRTH1L;
    uint16_t PRTH1H;
    uint16_t PRTH2L;
    uint16_t PRTH2H;
    uint16_t PHsxR1;
    uint16_t PHsxR2;
    uint16_t PHsxR3;
};

struct cfg_sys
{
    struct cfg_chip     chip;
    struct cfg_adjust   adjust;
    struct cfg_deadzone deadzone;
};

struct msr_config
{
    struct cfg_user     user;
    struct cfg_sys      sys;
};

#endif
