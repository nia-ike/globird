/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_apparentpower.c
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
#include "db_cfg.h"
#include <stdint.h>
#include <math.h>

#include "algorithm_apparentpower.h"
#include "Rn7302_read.h"


#ifdef HIGH_PRECISION
#define MULTIPLE_S  10 //10000  
#else
#define MULTIPLE_S  1
#endif

#define  PI 3.1415926

/*********************************************************************************************************
** Function name:       rms_algorithm_ApparentPowerJudge
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t rms_algorithm_PowerJudge(struct db_database_measure_base_type  *pMeasureBase, struct db_basic_realtime_type *out)
{

    if(((out->Sa < out->Pa) && (out->Pa > 0)) || ((out->Pa == 0) && ((out->Qa == 0))))
    {
        out->Sa = out->Pa;
    }
    if(((out->Sb < out->Pb) && (out->Pb > 0)) || ((out->Pb == 0) && ((out->Qb == 0)))) 
    {
        out->Sb = out->Pb;
    }
    if(((out->Sc < out->Pc) && (out->Pc > 0)) || ((out->Pc == 0) && ((out->Qc == 0)))) 
    {
        out->Sc = out->Pc;
    }
    if(((out->Stot < out->Ptot) && (out->Ptot > 0)) || ((out->Ptot == 0) && ((out->Qtot == 0)))) 
    {
        out->Stot = out->Ptot;
    }
    return 0;
}


/*********************************************************************************************************
** Function name:       rms_algorithm_ApparentPowerJudge
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t rms_algorithm_ApparentPowerJudge(struct db_database_measure_base_type  *pMeasureBase, struct db_basic_realtime_type *out)
{
    uint16_t                    mode = 0;
    struct db_cfg_user_para *pCfgUser = &(pMeasureBase->db_cfg.user);
    mode = (pCfgUser->base.WiringMode.value & 0x03);
   
    if(P3W3 == mode)
    {
        out->Sab = out->Sa;
        out->Sca = out->Sc;
    }
    else
    {
        out->Sab = 0;
        out->Sca = 0;
    }

    rms_algorithm_PowerJudge(pMeasureBase, out);
    return 0;
}


/*********************************************************************************************************
** Function name:       rms_algorithm_Apparentpower
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_apparentpower(struct db_database_measure_base_type  *pMeasureBase, struct db_basic_realtime_type *out, void *in, uint32_t pt, uint32_t ct)
{
    int64_t                                 temp;
    struct db_cfg_sys_para                  *pCfgSys = &(pMeasureBase->db_cfg.sys);
    uint32_t                            RN7302_Krmsp = 1;       
    struct rn7302_emu                          *pEmu = (struct rn7302_emu*)in;

    RN7302_Krmsp = pCfgSys->adjustAdvalue.Adjust_Pa.value / pCfgSys->adjustAdvalue.Adjust_P.value / pCfgSys->adjustAd.Krms_P.value;
    temp = (int64_t)pEmu->SA * MULTIPLE_S * pt  * ct / 10000 / (int32_t)RN7302_Krmsp;
    out->Sa = temp;            
                               
    RN7302_Krmsp = pCfgSys->adjustAdvalue.Adjust_Pb.value / pCfgSys->adjustAdvalue.Adjust_P.value / pCfgSys->adjustAd.Krms_P.value;
    temp = (int64_t)pEmu->SB * MULTIPLE_S * pt  * ct / 10000 / (int32_t)RN7302_Krmsp;
    out->Sb = temp;            
                               
    RN7302_Krmsp = pCfgSys->adjustAdvalue.Adjust_Pc.value / pCfgSys->adjustAdvalue.Adjust_P.value / pCfgSys->adjustAd.Krms_P.value;
    temp = (int64_t)pEmu->SC * MULTIPLE_S * pt  * ct / 10000 / (int32_t)RN7302_Krmsp;
    out->Sc = temp;


    rms_algorithm_ApparentPowerJudge(pMeasureBase,out);

    /* 星型接线时，为保证总功率数据的一致性，采用三相功率代数和的计算方式 */
    out->Stot = out->Sa + out->Sb + out->Sc;
    return 0;
}
