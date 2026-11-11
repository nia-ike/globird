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
#include "algorithm_fullwave.h"
#include "msr_process.h"
#include "algorithm_linevoltage.h"
#include "algorithm_phasevoltage.h"
#include "algorithm_current.h"
#include "algorithm_voltageangle.h"
#include "algorithm_currentangle.h"
#include "algorithm_activepower.h"
#include "algorithm_reactivepower.h"
#include "algorithm_frequency.h"
#include "algorithm_apparentpower.h"
#include "algorithm_powerfactor.h"
#include "algorithm_energy_remain.h"
#include "algorithm_energy_realtime.h"
#include "algorithm_energy_realtime_hp.h"
#include "algorithm_energy_runing.h"
#include "algorithm_energy_multirate.h"
#include "algorithm_energy_multirate_hp.h"
#include "algorithm_energy_realtime_translate.h"
#include "Rn7302_read.h"
#include "Rn7302_phasecheck.h"


/*********************************************************************************************************
** Function name:       rms_algorithm_fullwave
** Descriptions:        全波算法；电参读失败则本拍不计算且不读电能（读后清零保护）
** Returned value:      0 成功；-1 驱动读失败
*********************************************************************************************************/
int32_t rms_algorithm_fullwave(struct db_database_measure_base_type *pMeasure, struct db_basic_realtime_type *out, uint32_t pt, uint32_t ct)
{
    struct db_realtime_energy_type           *pEngRemain     = &(pMeasure->db_measure.energy.remain_hp);
    struct db_realtime_energy_type           *pEngRt         = &(pMeasure->db_measure.energy.realtime);
	struct db_realtime_energy_type           *pEngRtHP       = &(pMeasure->db_measure.energy.realtime_hp);
    struct db_runing_energy_type             *pEngRun        = &(pMeasure->db_measure.energy.runing_hp);
    struct db_energy_multirate_type          *pEngMulti      = &(pMeasure->db_measure.energy.multirate);
    struct db_energy_multirate_type          *pEngMultiHP    = &(pMeasure->db_measure.energy.multirate_hp);
    struct rn7302_emu                        *pEmuFullWave   = NULL;
    struct rn7302_emu_energy                 *pEmuFullEnergy = NULL;
    struct db_realtime_energy_type           *pNetEnergy     = NULL;
    struct db_cfg_user_para                  *pCfgUser       = &(pMeasure->db_cfg.user);
    struct db_energy_realtime_translate_type *pEngRtTrans    = &(pMeasure->db_measure.energy.realtime_translate);

    /* 先读电参；失败则不读电能，避免读后清零芯片丢增量 */
    if (ICHSEL_ON == get_ICHSEL_flag())
    {
        if (get_EmuFund(&pEmuFullWave) != 0)
            return -1;
        pEmuFullEnergy = get_EmuFundEnergy();
        pNetEnergy     = get_FundNetEnergy();
    }
    else
    {
        if (get_EmuFull(&pEmuFullWave) != 0)
            return -1;
        pEmuFullEnergy = get_EmuFullEnergy();
        pNetEnergy     = get_FullNetEnergy();
    }

    /* 1. calculate the angle first */
    rms_algorithm_voltageangle(   pMeasure,   out,     pEmuFullWave,  1,  1);
    rms_algorithm_currentangle(   pMeasure,   out,     pEmuFullWave,  1,  1);

    /* 2. calculate the line volt */
    rms_algorithm_linevoltage(    pMeasure,   out,     pEmuFullWave, pt,  1);
    if((pCfgUser->base.WiringMode.value & 0x03) == P3W3)
    {/* 三相三线 寄存器读出来是线电压 */
        rms_algorithm_phasevoltage_3p3w(   pMeasure,   out,     pEmuFullWave, pt,  1);
    }
    else 
    {
        rms_algorithm_phasevoltage_3p4w(   pMeasure,   out,     pEmuFullWave, pt,  1);
    }

    rms_algorithm_current              (  pMeasure,   out,         pEmuFullWave,   1,             ct);
    rms_algorithm_activepower          (  pMeasure,   out,         pEmuFullWave,   pt,            ct);
    rms_algorithm_reactivepower        (  pMeasure,   out,         pEmuFullWave,   pt,            ct);
    rms_algorithm_frequency            (  pMeasure,   out,         pEmuFullWave,   1,             1);
    rms_algorithm_powerfactor          (  pMeasure,   out,         NULL,           1,             1);
    rms_algorithm_apparentpower        (  pMeasure,   out,         pEmuFullWave,   pt,            ct);

    rms_algorithm_RemainEnergy         (  pMeasure,   pEngRemain,  pEmuFullEnergy, pNetEnergy,    pt, ct);

    rms_algorithm_RealtimeEnergy_hp    (  pMeasure,   pEngRtHP,    pNetEnergy,     pt,            ct);
    rms_algorithm_RealtimeEnergy       (  pMeasure,   pEngRt,      pEngRtHP,       pt,            ct);
    rms_algorithm_MultirateEnergy_hp   (  pMeasure,   pEngMultiHP, pNetEnergy,     pt,            ct);
    rms_algorithm_MultirateEnergy      (  pMeasure,   pEngMulti,   pEngMultiHP,    pt,            ct);
    rms_algorithm_RuningEnergy_hp      (  pMeasure,   pEngRun,     pNetEnergy,     pt,            ct);
    rms_algorithm_RealtimeEnergy_trans (  pMeasure,   pEngRtTrans, pEngRtHP,       pt,            ct);
    
    return 0;
}
