/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_fundwave.c
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
#include "Rn7302.h"
#include "Rn7302File.h"
#include "algorithm_fundwave.h"
#include "msr_process.h"
#include "algorithm_linevoltage.h"
#include "algorithm_phasevoltage.h"
#include "algorithm_current.h"
#include "algorithm_activepower.h"
#include "algorithm_reactivepower.h"
#include "algorithm_frequency.h"
#include "algorithm_apparentpower.h"
#include "algorithm_powerfactor.h"
#include "algorithm_voltageangle.h"
#include "algorithm_currentangle.h"
#include "algorithm_energy_remain.h"
#include "algorithm_energy_realtime.h"
#include "algorithm_energy_realtime_hp.h"
#include "algorithm_energy_runing.h"
#include "algorithm_energy_multirate.h"
#include "algorithm_energy_multirate_hp.h"

#include "Rn7302_read.h"

/*********************************************************************************************************
** Function name:       rms_algorithm_fundwave
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_fundwave(struct db_database_measure_base_type *pMeasure, struct db_basic_realtime_type *out, uint32_t pt, uint32_t ct)
{
    struct db_realtime_energy_type           *pRemainEnergy = &(pMeasure->db_measure.energy.fundwave_remain);
    struct db_realtime_energy_type         *pRealtimeEnergy = &(pMeasure->db_measure.energy.fundwave);
    struct rn7302_emu                         *pEmuFundWave = get_EmuFund();
    struct rn7302_emu_energy                *pEmuFundEnergy = get_EmuFundEnergy();
    struct db_realtime_energy_type              *pNetEnergy = get_FundNetEnergy();

    struct db_cfg_user_para     *pCfgUser = &(pMeasure->db_cfg.user);
    /* 1. calculate the angle first */
    rms_algorithm_voltageangle(   pMeasure,   out,     pEmuFundWave,  1,  1);
    rms_algorithm_currentangle(   pMeasure,   out,     pEmuFundWave,  1,  1);

    /* 2. calculate the line volt */
    rms_algorithm_linevoltage(    pMeasure,   out,     pEmuFundWave, pt,  1);
    if((pCfgUser->base.WiringMode.value & 0x03) == P3W3)
    {/* 三相三线 寄存器读出来是线电压 */
        rms_algorithm_phasevoltage_3p3w(   pMeasure,   out,     pEmuFundWave, pt,  1);
    }
    else 
    {
        rms_algorithm_phasevoltage_3p4w(   pMeasure,   out,     pEmuFundWave, pt,  1);
    }

    rms_algorithm_current(          pMeasure,  out,   pEmuFundWave,  1, ct);
    rms_algorithm_activepower(      pMeasure,  out,   pEmuFundWave, pt, ct);
    rms_algorithm_reactivepower(    pMeasure,  out,   pEmuFundWave, pt, ct);
    rms_algorithm_frequency(        pMeasure,  out,   pEmuFundWave,  1,  1);
    rms_algorithm_powerfactor(      pMeasure,  out,           NULL,  1,  1);
    rms_algorithm_apparentpower(    pMeasure,  out,   pEmuFundWave, pt, ct);
    rms_algorithm_RemainEnergy(     pMeasure,  pRemainEnergy,   pEmuFundEnergy, pNetEnergy, pt, ct);
    rms_algorithm_RealtimeEnergy_hp(pMeasure,  pRealtimeEnergy,  pNetEnergy, pt, ct);
    return 0;
}
