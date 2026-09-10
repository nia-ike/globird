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


/**
 * @brief : 全波算法补全半波算法里的功率因数和视在功率
 * @note  : 快速刷新寄存器里，没有视在功率和功率因数，因此我们在get_EmuHalf 去掉这部分，这里也不算。把功因和视在的数据从慢速拷贝过来即可
 * @param : pFullRealtime: 全波算法实时数据
 * @param : out: 半波算法实时数据
 * @return: 0: 成功, -1: 失败
 */
static int32_t rms_algorithm_fullwave_complement(struct db_basic_realtime_type *pFullRealtime, struct db_basic_realtime_type *out)
{
    out->Vab    = pFullRealtime->Vab;
    out->Vbc    = pFullRealtime->Vbc;
    out->Vca    = pFullRealtime->Vca;
    out->VllAvg = pFullRealtime->VllAvg;
    out->J_Ia   = pFullRealtime->J_Ia;
    out->J_Ib   = pFullRealtime->J_Ib;
    out->J_Ic   = pFullRealtime->J_Ic;
    out->J_Va   = pFullRealtime->J_Va;
    out->J_Vb   = pFullRealtime->J_Vb;
    out->J_Vc   = pFullRealtime->J_Vc;
    out->PFa    = pFullRealtime->PFa;
    out->PFb    = pFullRealtime->PFb;
    out->PFc    = pFullRealtime->PFc;
    out->PFtot  = pFullRealtime->PFtot;
    out->Sa     = pFullRealtime->Sa;
    out->Sb     = pFullRealtime->Sb;
    out->Sc     = pFullRealtime->Sc;
    out->Stot   = pFullRealtime->Stot;
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
    rms_algorithm_activepower(        pMeasure,  out,   pEmuHalfWave,  pt, ct);
    rms_algorithm_reactivepower(      pMeasure,  out,   pEmuHalfWave,  pt, ct);
    rms_algorithm_frequency(          pMeasure,  out,   pEmuHalfWave,  1,  1);

    // 快速刷新寄存器里，没有视在功率和功率因数，因此我们在get_EmuHalf 去掉这部分，这里也不算。把功因和视在的数据从慢速拷贝过来即可
    rms_algorithm_fullwave_complement(pFullRealtime, out);

    rms_algorithm_halfwave_Primary_to_secondary(pMeasure, pRealtimeSecondary,    pt,   ct);

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
    out->Sa   = sqrt(out->Pa * out->Pa + out->Qa * out->Qa);
    out->Sb   = sqrt(out->Pb * out->Pb + out->Qb * out->Qb);
    out->Sc   = sqrt(out->Pc * out->Pc + out->Qc * out->Qc);
    out->Stot = sqrt(out->Ptot * out->Ptot + out->Qtot * out->Qtot);
   
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
 * @brief : 全波算法补全半波算法里的功率因数和视在功率
 * @note  : 快速刷新寄存器里，没有视在功率和功率因数，因此我们在get_EmuHalf 去掉这部分，这里也不算。把功因和视在的数据从慢速拷贝过来即可
 * @param : pFullRealtime: 全波算法实时数据
 * @param : out: 半波算法实时数据
 * @return: 0: 成功, -1: 失败
 */
static int32_t rms_algorithm_phasecheck_on_fullwave_complement(struct db_basic_realtime_type *pFullRealtime, struct db_basic_realtime_type *out)
{
    out->Vab    = pFullRealtime->Vab;
    out->Vbc    = pFullRealtime->Vbc;
    out->Vca    = pFullRealtime->Vca;
    out->VllAvg = pFullRealtime->VllAvg;
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

    uint32_t                    ct;
    uint32_t                    pt;
    uint32_t                    hz;
    
    pt = pCfgUser->base.PT.value;
    ct = pCfgUser->base.CT.value;

    hz = rms_algorithm_frequency_hw(  pMeasure,         pEmuHalfWave);
    rms_algorithm_halfwave_voltage(   pMeasure,  out,   pEmuHalfWave,  pt,  1, time, hz);

    /* 开启相序调整后，半波有效值未调整，手动重映射电流通道 */
    int32_t emui_val[3] = {
        pEmuHalfWave->IA,
        pEmuHalfWave->IB,
        pEmuHalfWave->IC,
    };
    pEmuHalfWave->IA = emui_val[Rn7302_phase_adj_to_ch_idx(pCfgUser->base.PhaseAdjust1.value)];
    pEmuHalfWave->IB = emui_val[Rn7302_phase_adj_to_ch_idx(pCfgUser->base.PhaseAdjust2.value)];
    pEmuHalfWave->IC = emui_val[Rn7302_phase_adj_to_ch_idx(pCfgUser->base.PhaseAdjust3.value)];
    rms_algorithm_halfwave_current(   pMeasure,  out,   pEmuHalfWave,  1, ct, time, hz);
    rms_algorithm_frequency(          pMeasure,  out,   pEmuHalfWave,  1,  1);

    /* 
        1. √ 开启电流相序调整后，半波电流是否跟随调整？
            - 半波电流未调整，需手动调整，已实现
        2. √ 开启相序调整后，谐波测量是否调整过来了？
            - 谐波有效值调整过来了，但谐波含有率未调整，还未实现

        3. √ 相间角度校准系数是否保存？
            - 已保存
        4. × 只加载电压或电流谐波时，快速刷新功率是否与全波、基波功率一致？
        
        5. √ 电流角度需手动调整
        6. × 需整周波计算快速刷新功率
     */
    
    rms_algorithm_voltageangle          (   pMeasure,   out,    pEmuHalfWave,   1,  1   );
    rms_algorithm_currentangle          (   pMeasure,   out,    pEmuHalfWave,   1,  1   );
    rms_algorithm_halfwave_powerfactor  (   pMeasure,   out,    pEmuHalfWave,   pt, ct  );
    rms_algorithm_halfwave_activepower  (   pMeasure,   out,    pEmuHalfWave,   pt, ct  );
    rms_algorithm_halfwave_reactivepower(   pMeasure,   out,    pEmuHalfWave,   pt, ct  );
    rms_algorithm_halfwave_apparentpower(   pMeasure,   out,    pEmuHalfWave,   pt, ct  );

    rms_algorithm_phasecheck_on_fullwave_complement(pFullRealtime, out);

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
