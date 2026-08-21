
/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_powerfactor.c
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
#include <stdint.h>
#include <math.h>

#include "algorithm_powerfactor.h"

#define PI 3.1415926
#define POWERFACTOR_MULTIPLE        1000

/*********************************************************************************************************
** Function name:       rms_algorithm_PowerFactorAngle
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_PowerFactorAngle(struct db_database_measure_base_type *pMeasureBase)
{
    struct db_basic_realtime_type    *pRealtimePrimary = &(pMeasureBase->db_measure.numeric.fullwave.primary);
    if((pRealtimePrimary->P_IndCap >> 3) && 0x1 == 0x1)
    {
        if(pRealtimePrimary->PFtot < 0)
        {
            pRealtimePrimary->J_PF =  acosf((float)(pRealtimePrimary->PFtot-1) / 1000) * 180.0 / PI * 100;
        }
        else
        {
            pRealtimePrimary->J_PF = 36000 - acosf((float)(pRealtimePrimary->PFtot+1) / 1000) * 180.0 / PI * 100;
        }
    }
    else
    {
        if(pRealtimePrimary->PFtot < 0)
        {
            pRealtimePrimary->J_PF =  36000 - acosf((float)(pRealtimePrimary->PFtot-1) / 1000) * 180.0 / PI * 100;
        }
        else
        {
            pRealtimePrimary->J_PF = acosf((float)(pRealtimePrimary->PFtot+1) / 1000) * 180.0 / PI * 100;
        }
    }
    return 0;
}


/*********************************************************************************************************
** Function name:       rms_algorithm_PFJudge
** Descriptions:        
** input parameters:    int64_t p : Active Power
                        int64_t s : Apparent Power
** Returned value:      int16_t pf : Power Factor
*********************************************************************************************************/
static int16_t rms_algorithm_PFJudge(int64_t p, int64_t s)
{
    int16_t pf = 0;

    if (s > 0)
    {        
        if (p > s)
        {
            pf = (1) * POWERFACTOR_MULTIPLE;
        }
        else if (-p > s)
        {
            pf = (-1) * POWERFACTOR_MULTIPLE;
        }
        else
        {
            pf = p * POWERFACTOR_MULTIPLE / s;
        }
    }
    return pf;
}



/*********************************************************************************************************
** Function name:       rms_algorithm_powerfactor
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_powerfactor(struct db_database_measure_base_type *pMeasureBase, struct db_basic_realtime_type *out, void *in, uint32_t pt, uint32_t ct)
{
    if(out->Sa != 0)
    {
        // out->PFa = (int16_t)(out->Pa * POWERFACTOR_MULTIPLE / (int64_t)out->Sa);
        out->PFa = rms_algorithm_PFJudge(out->Pa, out->Sa);
    }
    else
    {
        out->PFa = 0;
    }

    if(out->Sb != 0)
    {
        // out->PFb = (int16_t)(out->Pb * POWERFACTOR_MULTIPLE / (int64_t)out->Sb);
        out->PFb = rms_algorithm_PFJudge(out->Pb, out->Sb);
    }
    else
    {
        out->PFb = 0;
    }

    if(out->Sc != 0)
    {
        // out->PFc = (int16_t)(out->Pc * POWERFACTOR_MULTIPLE / (int64_t)out->Sc);
        out->PFc = rms_algorithm_PFJudge(out->Pc, out->Sc);
    }
    else
    {
        out->PFc = 0;
    }

    if(out->Stot != 0)
    {
        // out->PFtot = (int16_t)(out->Ptot * POWERFACTOR_MULTIPLE / (int64_t)out->Stot);
        out->PFtot = rms_algorithm_PFJudge(out->Ptot, out->Stot);
    }
    else
    {
        out->PFtot = 0;
    }
    
    return 0;
}
