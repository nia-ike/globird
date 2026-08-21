/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_phasevoltage.c
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
#include "algorithm_phasevoltage.h"

#include "msr_process.h"

#define MULTIPLE_PT  100  

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
** Function name:       rms_algorithm_PhaseVoltageJudge
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t rms_algorithm_PhaseVoltageJudge(struct db_database_measure_base_type  *pMeasureBase,  struct db_basic_realtime_type *out)
{
    uint16_t                    mode = 0;

    return 0;
}


/*********************************************************************************************************
** Function name:       rms_algorithm_phasevoltage
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_phasevoltage_3p4w(struct db_database_measure_base_type *pMeasure, struct db_basic_realtime_type *out,  void *input, uint32_t pt, uint32_t ct)
{
    uint32_t                    temp = 0;
    uint64_t                    dead = 0;
    uint32_t                    RN7302_Krmsu = 1000;
    struct db_cfg_sys_para               *pCfgSys = &(pMeasure->db_cfg.sys);
    struct rn7302_emu                       *pEmu = (struct rn7302_emu*)input;
    /* get the system parameter obj */

    RN7302_Krmsu = pCfgSys->adjustAdvalue.Adjust_Va.value / pCfgSys->adjustAdvalue.Adjust_V.value / pCfgSys->adjustAd.Krms_V.value;

    temp = (uint64_t)pEmu->UA * pt / RN7302_Krmsu;
    dead = (uint64_t)pCfgSys->deadzone.Ua.value * pt ;
    if (0 != rms_algorithm_RangeConstaraint(temp, dead))
    {
        temp = 0;
    }
    out->Va = temp / MULTIPLE_PT;

    temp = (uint64_t)pEmu->UB * pt / RN7302_Krmsu;
    dead = (uint64_t)pCfgSys->deadzone.Ub.value * pt;

    if (0 != rms_algorithm_RangeConstaraint(temp, dead))
    {
        temp = 0;
    }
    out->Vb = temp / MULTIPLE_PT;

    temp = (uint64_t)pEmu->UC * pt / RN7302_Krmsu;
    dead = (uint64_t)pCfgSys->deadzone.Uc.value * pt;
    if (0 != rms_algorithm_RangeConstaraint(temp, dead))
    {
        temp = 0;
    }
    out->Vc = temp / MULTIPLE_PT;

    out->VlnAvg = ((uint64_t)out->Va + out->Vb + out->Vc) / 3;
 
    rms_algorithm_PhaseVoltageJudge(pMeasure, out); 
    return 0;
}



/*********************************************************************************************************
** Function name:       rms_algorithm_phasevoltage
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_phasevoltage_3p3w(struct db_database_measure_base_type *pMeasure, struct db_basic_realtime_type *out,  void *input, uint32_t pt, uint32_t ct)
{
    out->Va = out->Vab / sqrt(3);
    out->Vb = out->Vbc / sqrt(3);
    out->Vc = out->Vca / sqrt(3);
    out->VlnAvg = ((uint64_t)out->Va + out->Vb + out->Vc) / 3;
    return 0;
}

