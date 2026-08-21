
/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_reactivepower.c
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
#include "Rn7302.h"
#include "Rn7302_register.h"
#include <stdint.h>
#include <math.h>
#include "msr_process.h"
#include "algorithm_reactivepower.h"

#define PI 3.1415926
#ifdef HIGH_PRECISION
#define MULTIPLE_RATE  10 //10000  
#else
#define MULTIPLE_RATE  1
#endif 


/*********************************************************************************************************
** Function name:       rms_algorithm_ActiverPowerJudge
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t rms_algorithm_Current_ReactivePowerJudge(struct db_database_measure_base_type *pMeasureBase, struct db_basic_realtime_type *out)
{
    struct db_cfg_user_para     *pCfgUser = &(pMeasureBase->db_cfg.user);
    uint16_t                     wireMode = (pCfgUser->base.WiringMode.value & 0x03);
    
    if((out->Ia == 0) || (out->Va == 0))
    {
        out->Qa = 0;
    }
    if((out->Ib == 0) || (out->Vb == 0))
    {
        out->Qb = 0;
    }
    if((out->Ic == 0) || (out->Vc == 0))
    {
        out->Qc = 0;
    }

    if(wireMode == P3W3)
    {
        out->Qab = out->Qa;
        out->Qca = out->Qc;
    }
    else 
    {
        out->Qab = 0;
        out->Qca = 0;
    }
    return 0;
}



/*********************************************************************************************************
** Function name:       rms_algorithm_Reactivepower
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_reactivepower(struct db_database_measure_base_type *pMeasureBase, struct db_basic_realtime_type *out,  void *input, uint32_t pt, uint32_t ct)
{
    int64_t                                  temp = 0;
    uint32_t                         RN7302_Krmsp = 1;      
    struct db_cfg_sys_para               *pCfgSys = &(pMeasureBase->db_cfg.sys);
    struct rn7302_emu                       *pEmu = (struct rn7302_emu*)input;
     
    RN7302_Krmsp = pCfgSys->adjustAdvalue.Adjust_Pa.value / pCfgSys->adjustAdvalue.Adjust_P.value / pCfgSys->adjustAd.Krms_P.value;
    temp = (int64_t)pEmu->QA * MULTIPLE_RATE * pt  * ct / 10000 / (int32_t)RN7302_Krmsp;
    out->Qa = temp;

    RN7302_Krmsp = pCfgSys->adjustAdvalue.Adjust_Pb.value / pCfgSys->adjustAdvalue.Adjust_P.value / pCfgSys->adjustAd.Krms_P.value;
    temp = (int64_t)pEmu->QB * MULTIPLE_RATE * pt  * ct / 10000 / (int32_t)RN7302_Krmsp;
    out->Qb = temp;

    RN7302_Krmsp = pCfgSys->adjustAdvalue.Adjust_Pc.value / pCfgSys->adjustAdvalue.Adjust_P.value / pCfgSys->adjustAd.Krms_P.value;
    temp = (int64_t)pEmu->QC * MULTIPLE_RATE * pt  * ct / 10000 / (int32_t)RN7302_Krmsp;
    out->Qc = temp;
    
    RN7302_Krmsp = pCfgSys->adjustAdvalue.Adjust_Pa.value / pCfgSys->adjustAdvalue.Adjust_P.value / pCfgSys->adjustAd.Krms_P.value;
    temp = (int64_t)pEmu->QT * MULTIPLE_RATE * pt  * ct / 10000 / (int32_t)RN7302_Krmsp;
    //out->Qtot = temp;

    rms_algorithm_Current_ReactivePowerJudge(pMeasureBase, out);

    /* 星型接线时，为保证总功率数据的一致性，采用三相功率代数和的计算方式 */
    out->Qtot = out->Qa + out->Qb + out->Qc;
/* 
*/
    return 0;
}

