/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_current_reverse.c
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
#include "algorithm_current_reverse.h"




/*********************************************************************************************************
** Function name:       rms_algorithm_Primary
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_CurrentReverse(struct db_database_measure_base_type *pMeasureBase, struct db_basic_realtime_type *out, uint32_t pt, uint32_t ct)
{
    int  phase_A = 1,phase_B = 1,phase_C = 1;
    struct db_cfg_user_para     *pCfgUser = &(pMeasureBase->db_cfg.user);
    uint16_t mode = 0;
    mode = (pCfgUser->base.WiringMode.value & 0x03);
   
    if(P3W4 == mode)
    {   
        if((pCfgUser->base.PhaseAdjust1.value & 0x8000) == 0x8000)
        {
            phase_A = -1;
        }
        if((pCfgUser->base.PhaseAdjust2.value & 0x8000) == 0x8000)
        {
            phase_B = -1;
        }
        if((pCfgUser->base.PhaseAdjust3.value & 0x8000) == 0x8000)
        {
            phase_C = -1;
        }       

        out->Pa   = out->Pa * phase_A;
        out->Pb   = out->Pb * phase_B;
        out->Pc   = out->Pc * phase_C;
        out->Ptot = out->Pa + out->Pb + out->Pc;
        out->Qa   = out->Qa * phase_A;
        out->Qb   = out->Qb * phase_B;
        out->Qc   = out->Qc * phase_C;
        out->Qtot = out->Qa + out->Qb + out->Qc;
        out->PFa  = out->PFa * phase_A;
        out->PFb  = out->PFb * phase_B;
        out->PFc  = out->PFc * phase_C;
        if(((phase_A + phase_B + phase_C) != 3) && (out->Stot != 0))
        {
            out->PFtot = (float)out->Ptot / out->Stot * 1000.0;
        }
    }
/*
    printf("pRealtimeSecondary->Ia:%d ,Ib:%d ，Ic:%d \n",pRealtimeSecondary->Ia,pRealtimeSecondary->Ib,pRealtimeSecondary->Ic);
    printf("pRealtimeSecondary->Pa:%d ,Pb:%d ，Pc:%d \n",pRealtimeSecondary->Pa,pRealtimeSecondary->Pb,pRealtimeSecondary->Pc);
    printf("pRealtimeSecondary->Sa:%d ,Sb:%d ，Sc:%d \n",pRealtimeSecondary->Sa,pRealtimeSecondary->Sb,pRealtimeSecondary->Sc);
*/
    return 0;
}

