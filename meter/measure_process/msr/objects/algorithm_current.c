/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_current.c
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
#include "Rn7302_register.h"
#include "algorithm_current.h"
#include "msr_process.h"

#define MULTIPLE_I  100

/*********************************************************************************************************
** Function name:       rms_algorithm_RangeConstaraint
** Descriptions:        constrain the phase active power
** input parameters:    deadzone-  active power dead zone
                        v       -  the phase voltage
                        i       -  the phase current
** output parameters:   NONE
** Returned value:      return 0 on success, -1 otherwise
*********************************************************************************************************/
static int32_t rms_algorithm_RangeConstaraint(uint64_t value, uint64_t deadzone)
{
    if (value < deadzone)
    {
        return -1;
    }

    return 0;
}


/*********************************************************************************************************
** Function name:       rms_algorithm_Current
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_current(struct db_database_measure_base_type *pMeasureBase, struct db_basic_realtime_type *out,  void *in, uint32_t pt, uint32_t ct)
{
    uint32_t                            temp ;
    uint64_t                            dead ;
    struct db_cfg_sys_para               *pCfgSys = &(pMeasureBase->db_cfg.sys);
    uint32_t                            RN7302_Krmsi;      
    struct rn7302_emu                       *pEmu = (struct rn7302_emu*)in;


    RN7302_Krmsi = pCfgSys->adjustAdvalue.Adjust_Ia.value / pCfgSys->adjustAdvalue.Adjust_I.value / pCfgSys->adjustAd.Krms_I.value;
    temp = (uint64_t)pEmu->IA * ct / MULTIPLE_I  / RN7302_Krmsi;
    dead = (uint64_t)pCfgSys->deadzone.Ia.value * ct / MULTIPLE_I;
    if (0 != rms_algorithm_RangeConstaraint(temp, dead))
    {
        temp = 0;
    }
    out->Ia = temp;

    RN7302_Krmsi = pCfgSys->adjustAdvalue.Adjust_Ib.value / pCfgSys->adjustAdvalue.Adjust_I.value / pCfgSys->adjustAd.Krms_I.value;
    temp = (uint64_t)pEmu->IB * ct / MULTIPLE_I  / RN7302_Krmsi;
    dead = (uint64_t)pCfgSys->deadzone.Ib.value * ct / MULTIPLE_I;
    if (0 != rms_algorithm_RangeConstaraint(temp, dead))
    {
        temp = 0;
    }
    out->Ib = temp;

    RN7302_Krmsi = pCfgSys->adjustAdvalue.Adjust_Ic.value / pCfgSys->adjustAdvalue.Adjust_I.value / pCfgSys->adjustAd.Krms_I.value;
    temp = (uint64_t)pEmu->IC * ct / MULTIPLE_I  / RN7302_Krmsi;
    dead = (uint64_t)pCfgSys->deadzone.Ic.value * ct / MULTIPLE_I;
    if (0 != rms_algorithm_RangeConstaraint(temp, dead))
    {
        temp = 0;
    }
    out->Ic = temp;
    
    out->IAvg = (out->Ia + out->Ib + out->Ic) / 3;

    // In 采用 5A  2000:1 增益x1的系数校表   漏电电流2000：1，第4路电流3000：1，故第4路电流= 1.5 * 漏电电流
    RN7302_Krmsi = pCfgSys->adjustI0value.Adjust_I0_value.value / pCfgSys->adjustI0value.Adjust_I0.value / pCfgSys->adjustAd.Krms_I.value;
    if(pCfgSys->adjustI0value.TypeI0.value == 1)
    {
        temp = (uint64_t)pEmu->IN * 3 / 2 / RN7302_Krmsi;
    }
    else
    {
        temp = (uint64_t)pEmu->IN / RN7302_Krmsi;
    }
    dead = (uint64_t)pCfgSys->deadzone.In.value;
    if (0 != rms_algorithm_RangeConstaraint(temp, dead))
    {
        temp = 0;
    }
    out->In = temp;

    return 0;
}