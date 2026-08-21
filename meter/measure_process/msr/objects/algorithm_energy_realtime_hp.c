/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_energy.c
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
#include "Rn7302File.h"
#include "db_cfg.h"
#include <stdint.h>
#include <math.h>
#include "algorithm_energy_realtime.h"


/* ���������ֵ */
#define ENERGY_MAXNUM           999999999   //      keep     .001 kwh


/*********************************************************************************************************
** Function name:       Rn7302ReadEnergy
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t AccumulateEnergy(uint32_t *net, uint32_t* real)
{
    uint64_t eng = 0;
    eng = (uint64_t)(*real);
   
    if(*net > 0)
    {
        eng += (uint64_t)(*net);
        eng %= (ENERGY_MAXNUM + 1); 
        *real = (uint32_t)eng;
        return 0;
    }
    else
    {
        return -1;
    }   
}



/*********************************************************************************************************
** Function name:       rms_algorithm_RealtimeEnergy_hp
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_RealtimeEnergy_hp(struct db_database_measure_base_type    *pMeasureBase, struct db_realtime_energy_type *out, void *pNet, uint32_t pt, uint32_t ct)
{
    int i;
    struct db_cfg_user_para                     *pCfgUser = &(pMeasureBase->db_cfg.user);
    struct db_realtime_energy_type            *pNetEnergy = (struct db_realtime_energy_type*)pNet;

    for(i = 0; i < PHASE_NUM; i++)
    {
        AccumulateEnergy(&pNetEnergy->kWhImp[i]  , &out->kWhImp[i]   );
        AccumulateEnergy(&pNetEnergy->kWhExp[i]  , &out->kWhExp[i]   );
        AccumulateEnergy(&pNetEnergy->kvarhImp[i], &out->kvarhImp[i] );
        AccumulateEnergy(&pNetEnergy->kvarhExp[i], &out->kvarhExp[i] );
        AccumulateEnergy(&pNetEnergy->kVAh[i]    , &out->kVAh[i]     );

        AccumulateEnergy(&pNetEnergy->kWhTot[i]    , &out->kWhTot[i]   );
        AccumulateEnergy(&pNetEnergy->kvarhTot[i]  , &out->kvarhTot[i] );

        out->kWhTot[i] = (out->kWhImp[i] + out->kWhExp[i]) % (ENERGY_MAXNUM + 1);
        out->kvarhTot[i] = (out->kvarhImp[i] + out->kvarhExp[i]) % (ENERGY_MAXNUM + 1);

    }

    if((pCfgUser->base.WiringMode.value & 0x03) == P3W4)
    {
        out->kWhImp[3]   = (out->kWhImp[0]   + out->kWhImp[1]   + out->kWhImp[2]   ) % (ENERGY_MAXNUM + 1);
        out->kWhExp[3]   = (out->kWhExp[0]   + out->kWhExp[1]   + out->kWhExp[2]   ) % (ENERGY_MAXNUM + 1);
        out->kvarhImp[3] = (out->kvarhImp[0] + out->kvarhImp[1] + out->kvarhImp[2] ) % (ENERGY_MAXNUM + 1);
        out->kvarhExp[3] = (out->kvarhExp[0] + out->kvarhExp[1] + out->kvarhExp[2] ) % (ENERGY_MAXNUM + 1);
        out->kVAh[3]     = (out->kVAh[0]     + out->kVAh[1]     + out->kVAh[2]     ) % (ENERGY_MAXNUM + 1);
        out->kWhTot[3]   = (out->kWhTot[0]   + out->kWhTot[1]   + out->kWhTot[2]   ) % (ENERGY_MAXNUM + 1);
        out->kvarhTot[3] = (out->kvarhTot[0] + out->kvarhTot[1] + out->kvarhTot[2] ) % (ENERGY_MAXNUM + 1);
    }


    return 0;
}
