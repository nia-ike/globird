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

/*********************************************************************************************************
** Function name:       rms_algorithm_halfwave
** Descriptions:

** input parameters:
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object -
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_fullwave_complement(struct db_basic_realtime_type *pFullRealtime, struct db_basic_realtime_type *out)
{
    out->Vab = pFullRealtime->Vab;
    out->Vbc = pFullRealtime->Vbc;
    out->Vca = pFullRealtime->Vca;
    out->VllAvg = pFullRealtime->VllAvg;
    out->J_Ia = pFullRealtime->J_Ia;
    out->J_Ib = pFullRealtime->J_Ib;
    out->J_Ic = pFullRealtime->J_Ic;
    out->J_Va = pFullRealtime->J_Va;    
    out->J_Vb = pFullRealtime->J_Vb;
    out->J_Vc = pFullRealtime->J_Vc;
    out->PFa = pFullRealtime->PFa;
    out->PFb = pFullRealtime->PFb;
    out->PFc = pFullRealtime->PFc;
    out->PFtot = pFullRealtime->PFtot;
    out->Sa = pFullRealtime->Sa;
    out->Sb = pFullRealtime->Sb;
    out->Sc = pFullRealtime->Sc;
    out->Stot = pFullRealtime->Stot;
    return 0;
}

/*********************************************************************************************************
** Function name:       rms_algorithm_halfwave
** Descriptions:

** input parameters:
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object -
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_halfwave(struct db_database_measure_base_type *pMeasure, struct db_basic_realtime_type *out, uint32_t time)
{     
    struct rn7302_emu    *pEmuHalfWave = get_EmuHalf();
    struct db_cfg_user_para *pCfgUser = &(pMeasure->db_cfg.user);
    struct db_realtime_secondary_type   *pRealtimeSecondary = &(pMeasure->db_measure.numeric.halfwave.secondary);
    struct db_basic_realtime_type *pFullRealtime = &(pMeasure->db_measure.numeric.fullwave.primary);

    uint32_t                    ct;
    uint32_t                    pt;
    uint32_t                    hz;
    if((pCfgUser->base.PhaseAdjust1.value != 1) || (pCfgUser->base.PhaseAdjust2.value != 2) || (pCfgUser->base.PhaseAdjust3.value != 3))
    {
        out = &(pMeasure->db_measure.numeric.fullwave.primary);
    }
    else
    {
        pt = pCfgUser->base.PT.value;
        ct = pCfgUser->base.CT.value;
        hz = rms_algorithm_frequency_hw(  pMeasure,         pEmuHalfWave);
        rms_algorithm_halfwave_voltage(   pMeasure,  out,   pEmuHalfWave,  pt,  1, time, hz);
        rms_algorithm_halfwave_current(   pMeasure,  out,   pEmuHalfWave,  1, ct, time, hz);
        rms_algorithm_activepower(        pMeasure,  out,   pEmuHalfWave,  pt, ct);
        rms_algorithm_reactivepower(      pMeasure,  out,   pEmuHalfWave,  pt, ct);
        rms_algorithm_frequency(          pMeasure,  out,   pEmuHalfWave,  1,  1);

        // 快速刷新寄存器里，没有实在功率和功率因数，因此我们在get_EmuHalf 去掉这部分，这里也不算。把功因和视在的数据从慢速拷贝过来即可
        // 即 rms_algorithm_fullwave_complement 里把功率因数和视在功率的数据拷贝过来
        rms_algorithm_fullwave_complement(pFullRealtime, out);

        rms_algorithm_halfwave_Primary_to_secondary(pMeasure, pRealtimeSecondary,    pt,   ct);


    }

    return 0;
}
