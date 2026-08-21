/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：
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
*/


#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "Rn7302.h"
#include "algorithm_energy_multirate.h"

#define FACTOR                  100

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
static int32_t AccumulateEnergy(uint64_t *hp, uint64_t* real)
{
   uint64_t eng;

   eng = *hp / FACTOR;
   eng %= (ENERGY_MAXNUM_8D1 + 1); 
   *real = eng;

   return 0;
}



/*********************************************************************************************************
** Function name:       rms_algorithm_RealtimeEnergy
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_MultirateEnergy(struct db_database_measure_base_type *pMeasureBase, struct db_energy_multirate_type *out, struct db_energy_multirate_type *phpEnergy, uint32_t pt, uint32_t ct)
{
    int i;

    for(i = 0; i < RATES_NUM; i++)
    {
        AccumulateEnergy(&phpEnergy->rate[i].kWhImp  , &out->rate[i].kWhImp   );
        AccumulateEnergy(&phpEnergy->rate[i].kWhExp  , &out->rate[i].kWhExp   );
        AccumulateEnergy(&phpEnergy->rate[i].kvarhImp, &out->rate[i].kvarhImp );
        AccumulateEnergy(&phpEnergy->rate[i].kvarhExp, &out->rate[i].kvarhExp );
    }

    return 0;
}
