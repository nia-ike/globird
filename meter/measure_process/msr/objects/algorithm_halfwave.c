/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_halfwave.c
*  文件标识：
*  描    述：
*
*  当前版本：V1.0
*  作    者：raoy
*  运行环境：
*  操作系统: linux
*  硬件平台：f1c100s
*  固件库  ：
*  修改记录
*  作者       时间         版本      修改描述
*  raoy      2021       V1.00
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <time.h>

#include "Rn7302.h"
#include "Rn7302_read.h"
#include "algorithm_powerfactor.h"
#include "msr_process.h"
#include "algorithm_halfwave.h"
#include "algorithm_halfwave_voltage.h"
#include "algorithm_halfwave_current.h"
#include "algorithm_frequency.h"
#include "algorithm_activepower.h"
#include "algorithm_reactivepower.h"
#include "algorithm_apparentpower.h"
#include "algorithm_primary_to_secondary.h"
#include "Rn7302_phasecheck.h"
#include "algorithm_voltageangle.h"
#include "algorithm_currentangle.h"
#include "algorithm_linevoltage.h"


#define MULTIPLE_PT  100 


/**
 * @brief : 相序调整后的半波算法，根据电压、电流有效值及功因计算有功功率
 * @param : pMeasureBase: 测量上下文
 * @param : out: 半波算法实时数据
 * @param : input: 物理通道电流值
 * @param : pt: 电压通道倍率
 * @param : ct: 电流通道倍率
 * @return: 0: 成功, -1: 失败
 */
static int32_t rms_algorithm_halfwave_activepower(struct db_database_measure_base_type *pMeasureBase, struct db_basic_realtime_type *out, void *input, uint32_t pt, uint32_t ct)
{
    struct rn7302_emu *pEmu = (struct rn7302_emu*)input;

    out->Pa   = (int32_t)((double)out->Va * (double)out->Ia * cos((out->J_Va - out->J_Ia) * PI / 18000.0f) / 10000.0f);
    out->Pb   = (int32_t)((double)out->Vb * (double)out->Ib * cos((out->J_Vb - out->J_Ib) * PI / 18000.0f) / 10000.0f);
    out->Pc   = (int32_t)((double)out->Vc * (double)out->Ic * cos((out->J_Vc - out->J_Ic) * PI / 18000.0f) / 10000.0f);
    out->Ptot = out->Pa + out->Pb + out->Pc;
   
    return 0;
}

/**
 * @brief : 相序调整后的半波算法，根据电压、电流有效值及功因计算无功功率
 * @param : pMeasureBase: 测量上下文
 * @param : out: 半波算法实时数据
 * @param : input: 物理通道电流值
 * @param : pt: 电压通道倍率
 * @param : ct: 电流通道倍率
 * @return: 0: 成功, -1: 失败
 */
static int32_t rms_algorithm_halfwave_reactivepower(struct db_database_measure_base_type *pMeasureBase, struct db_basic_realtime_type *out, void *input, uint32_t pt, uint32_t ct)
{
    struct rn7302_emu *pEmu = (struct rn7302_emu*)input;

    out->Qa   = (int32_t)((double)out->Va * (double)out->Ia * sin((out->J_Va - out->J_Ia) * PI / 18000.0f) / 10000.0f);
    out->Qb   = (int32_t)((double)out->Vb * (double)out->Ib * sin((out->J_Vb - out->J_Ib) * PI / 18000.0f) / 10000.0f);
    out->Qc   = (int32_t)((double)out->Vc * (double)out->Ic * sin((out->J_Vc - out->J_Ic) * PI / 18000.0f) / 10000.0f);
    out->Qtot = out->Qa + out->Qb + out->Qc;
   
    return 0;
}

/**
 * @brief : 相序调整后的半波算法，根据电压、电流有效值及功因计算视在功率
 * @param : pMeasureBase: 测量上下文
 * @param : out: 半波算法实时数据
 * @param : input: 物理通道电流值
 * @param : pt: 电压通道倍率
 * @param : ct: 电流通道倍率
 * @return: 0: 成功, -1: 失败
 */
static int32_t rms_algorithm_halfwave_apparentpower(struct db_database_measure_base_type *pMeasureBase, struct db_basic_realtime_type *out, void *input, uint32_t pt, uint32_t ct)
{
    (void)pMeasureBase;
    (void)input;
    (void)pt;
    (void)ct;

    out->Sa   = sqrt((double)out->Pa * (double)out->Pa + (double)out->Qa * (double)out->Qa);
    out->Sb   = sqrt((double)out->Pb * (double)out->Pb + (double)out->Qb * (double)out->Qb);
    out->Sc   = sqrt((double)out->Pc * (double)out->Pc + (double)out->Qc * (double)out->Qc);
    out->Stot = sqrt((double)out->Ptot * (double)out->Ptot + (double)out->Qtot * (double)out->Qtot);

    return 0;
}

/**
 * @brief : 相序调整后的半波算法，根据电压、电流有效值及功因计算有功功率
 * @param : pMeasureBase: 测量上下文
 * @param : out: 半波算法实时数据
 * @param : input: 物理通道电流值
 * @param : pt: 电压通道倍率
 * @param : ct: 电流通道倍率
 * @return: 0: 成功, -1: 失败
 */
static int32_t rms_algorithm_halfwave_powerfactor(struct db_database_measure_base_type *pMeasureBase, struct db_basic_realtime_type *out, void *input, uint32_t pt, uint32_t ct)
{
    out->PFa   = (out->Sa   != 0) ? (int16_t)((double)out->Pa   * 1000.0f / (double)out->Sa)   : 0;
    out->PFb   = (out->Sb   != 0) ? (int16_t)((double)out->Pb   * 1000.0f / (double)out->Sb)   : 0;
    out->PFc   = (out->Sc   != 0) ? (int16_t)((double)out->Pc   * 1000.0f / (double)out->Sc)   : 0;
    out->PFtot = (out->Stot != 0) ? (int16_t)((double)out->Ptot * 1000.0f / (double)out->Stot) : 0;
   
    return 0;
}

/**
 * @brief : 范围约束
 * @param : value: 值
 * @param : deadzone: 死区
 * @return: 0: 成功, -1: 失败
 */
static int32_t rms_algorithm_range_constraint(uint64_t value, uint64_t deadzone)
{
    if (value < deadzone)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief : 计算线电压有效值
 * @param : va: 电压A相有效值
 * @param : vb: 电压B相有效值
 * @param : pha_rad: 电压夹角
 * @param : deadzone: 死区
 * @return: 线电压有效值
 */
static uint32_t linevoltage_calc_rms(double va, double vb, double pha_rad, uint64_t deadzone)
{
    double s = va * va + vb * vb - 2.0 * va * vb * cos(pha_rad);

    if (s < 0.0)
    {
        s = -s;
    }
    uint32_t rms = (uint32_t)sqrt(s);
    if (rms_algorithm_range_constraint(rms, deadzone) != 0)
    {
        rms = 0;
    }
    return rms;
}

/**
 * @brief : 计算线电压有效值
 * @param : pMeasure: 测量上下文
 * @param : out: 半波算法实时数据
 * @param : input: 物理通道电流值
 * @param : pt: 电压通道倍率
 * @param : ct: 电流通道倍率
 * @return: 0: 成功, -1: 失败
 */
static int32_t rms_algorithm_halfwave_linevoltage(struct db_database_measure_base_type *pMeasure, struct db_basic_realtime_type *out,  void *input, uint32_t pt, uint32_t ct)
{
    // uint32_t                    u32Tmp = 0;
    // uint64_t                    dead = 0;
    // uint16_t                    mode = 0;
    // uint32_t                    RN7302_Krmsu = 1000;
    // uint64_t                    u64tmp[3]={0,0,0};
    // double                      TmpF32,pha[3] = {0,0,0};
    // struct db_cfg_user_para     *pCfgUser = &(pMeasure->db_cfg.user);
    struct db_cfg_sys_para      *pCfgSys = &(pMeasure->db_cfg.sys);
    // struct rn7302_emu           *pEmu = (struct rn7302_emu*)input;
    uint32_t               voltage[3] = {0, 0, 0};
    double                 pha[3]     = {0, 0, 0};
    
    pha[0] = (out->J_Va - out->J_Vb) * PI / 18000;
    pha[1] = (out->J_Vb - out->J_Vc) * PI / 18000;
    pha[2] = (out->J_Vc - out->J_Va) * PI / 18000;

    voltage[0] = out->Va;
    voltage[1] = out->Vb;
    voltage[2] = out->Vc;

    out->Vab    = linevoltage_calc_rms((double)voltage[0], (double)voltage[1], pha[0], 
        (uint64_t)pCfgSys->deadzone.Ua.value * pt / MULTIPLE_PT);
    out->Vbc    = linevoltage_calc_rms((double)voltage[1], (double)voltage[2], pha[1], 
        (uint64_t)pCfgSys->deadzone.Ub.value * pt / MULTIPLE_PT);
    out->Vca    = linevoltage_calc_rms((double)voltage[2], (double)voltage[0], pha[2],
        (uint64_t)pCfgSys->deadzone.Uc.value * pt / MULTIPLE_PT);
    out->VllAvg = ((uint64_t)out->Vab + out->Vbc + out->Vca) / 3;
    
    return 0;
}

/**
 * @brief : 半波算法，未使能相序调整时，使用计量芯片半波数据计算功率
 * @param : pMeasure: 测量上下文
 * @param : out: 半波算法实时数据
 * @param : time: 时间
 * @return: 0: 成功, -1: 失败
 */
static int32_t rms_algorithm_halfwave_phasecheck_off(struct db_database_measure_base_type *pMeasure, struct db_basic_realtime_type *out, uint32_t time)
{
    struct rn7302_emu                 *pEmuHalfWave       = get_EmuHalf();
    struct db_cfg_user_para           *pCfgUser           = &(pMeasure->db_cfg.user);
    struct db_realtime_secondary_type *pRealtimeSecondary = &(pMeasure->db_measure.numeric.halfwave.secondary);
    struct db_basic_realtime_type     *pFullRealtime      = &(pMeasure->db_measure.numeric.fullwave.primary);

    uint32_t                    ct;
    uint32_t                    pt;
    uint32_t                    hz;

    pt = pCfgUser->base.PT.value;
    ct = pCfgUser->base.CT.value;
    hz = rms_algorithm_frequency_hw(  pMeasure,         pEmuHalfWave);
    rms_algorithm_halfwave_voltage(   pMeasure,  out,   pEmuHalfWave,  pt,  1, time, hz);
    rms_algorithm_halfwave_current(   pMeasure,  out,   pEmuHalfWave,  1, ct, time, hz);
    
    rms_algorithm_voltageangle          (   pMeasure,   out,    pEmuHalfWave,   1,  1   );
    rms_algorithm_currentangle          (   pMeasure,   out,    pEmuHalfWave,   1,  1   );

    rms_algorithm_halfwave_linevoltage(        pMeasure,  out,   pEmuHalfWave,  pt, ct);

    rms_algorithm_activepower(        pMeasure,  out,   pEmuHalfWave,  pt, ct);
    rms_algorithm_reactivepower(      pMeasure,  out,   pEmuHalfWave,  pt, ct);
    rms_algorithm_frequency(          pMeasure,  out,   pEmuHalfWave,  1,  1);
    
    rms_algorithm_halfwave_apparentpower(   pMeasure,   out,    pEmuHalfWave,   pt, ct  );
    rms_algorithm_halfwave_powerfactor  (   pMeasure,   out,    pEmuHalfWave,   pt, ct  );

    rms_algorithm_halfwave_Primary_to_secondary(pMeasure, pRealtimeSecondary,    pt,   ct);

    return 0;
}

/**
 * @brief : 半波算法，使能相序调整时，使用半波电压、电流有效值及快速刷新角度来计算功率
 * @param : pMeasure: 测量上下文
 * @param : out: 半波算法实时数据
 * @param : time: 时间
 * @return: 0: 成功, -1: 失败
 */
static int32_t rms_algorithm_halfwave_phasecheck_on(struct db_database_measure_base_type *pMeasure, struct db_basic_realtime_type *out, uint32_t time)
{
    struct rn7302_emu                 *pEmuHalfWave       = get_EmuHalf();
    struct db_cfg_user_para           *pCfgUser           = &(pMeasure->db_cfg.user);
    struct db_realtime_secondary_type *pRealtimeSecondary = &(pMeasure->db_measure.numeric.halfwave.secondary);
    struct db_basic_realtime_type     *pFullRealtime      = &(pMeasure->db_measure.numeric.fullwave.primary);
    uint32_t                           i_phys[3];

    uint32_t                    ct;
    uint32_t                    pt;
    uint32_t                    hz;

    pt = pCfgUser->base.PT.value;
    ct = pCfgUser->base.CT.value;

    hz = rms_algorithm_frequency_hw(  pMeasure,         pEmuHalfWave);
    rms_algorithm_halfwave_voltage(   pMeasure,  out,   pEmuHalfWave,  pt,  1, time, hz);

    /*
     * 半波有效值寄存器不跟随 ICHSEL 换相：
     * 先按物理通道做校准/滤波，再按 PhaseAdjust 映射到逻辑相，
     * 避免套错 Adjust_Ix，也不改写全局 EmuHalf。
     */
    rms_algorithm_halfwave_current(   pMeasure,  out,   pEmuHalfWave,  1, ct, time, hz);
    i_phys[0] = out->Ia;
    i_phys[1] = out->Ib;
    i_phys[2] = out->Ic;
    out->Ia = i_phys[Rn7302_phase_adj_to_ch_idx(pCfgUser->base.PhaseAdjust1.value)];
    out->Ib = i_phys[Rn7302_phase_adj_to_ch_idx(pCfgUser->base.PhaseAdjust2.value)];
    out->Ic = i_phys[Rn7302_phase_adj_to_ch_idx(pCfgUser->base.PhaseAdjust3.value)];
    out->IAvg = (out->Ia + out->Ib + out->Ic) / 3;

    rms_algorithm_frequency(          pMeasure,  out,   pEmuHalfWave,  1,  1);

    rms_algorithm_voltageangle          (   pMeasure,   out,    pEmuHalfWave,   1,  1   );
    rms_algorithm_currentangle          (   pMeasure,   out,    pEmuHalfWave,   1,  1   );

    rms_algorithm_halfwave_linevoltage(        pMeasure,  out,   pEmuHalfWave,  pt, ct);

    /* 先 P/Q/S，再功因，避免使用未更新的 Pa/Sa */
    rms_algorithm_halfwave_activepower  (   pMeasure,   out,    pEmuHalfWave,   pt, ct  );
    rms_algorithm_halfwave_reactivepower(   pMeasure,   out,    pEmuHalfWave,   pt, ct  );
    rms_algorithm_halfwave_apparentpower(   pMeasure,   out,    pEmuHalfWave,   pt, ct  );
    rms_algorithm_halfwave_powerfactor  (   pMeasure,   out,    pEmuHalfWave,   pt, ct  );

    rms_algorithm_halfwave_Primary_to_secondary(pMeasure, pRealtimeSecondary, pt, ct);

    return 0;
}
/**
 * @brief : 半波算法
 * @note  : 1. 未使能相序调整时，使用计量芯片半波数据计算功率；
 *          2. 使能相序调整时，使用半波电压、电流有效值及快速刷新角度来计算功率
 * @param : pMeasure: 测量上下文
 * @param : out: 输出数据
 * @param : time: 时间
 * @return: 0: 成功, -1: 失败
 */
int32_t rms_algorithm_halfwave(struct db_database_measure_base_type *pMeasure, struct db_basic_realtime_type *out, uint32_t time)
{
    if (ICHSEL_ON == get_ICHSEL_flag())
    {
        return rms_algorithm_halfwave_phasecheck_on(pMeasure, out, time);
    }
    else
    {
        return rms_algorithm_halfwave_phasecheck_off(pMeasure, out, time);
    }
}
