/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_energy_runing.c
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "db_struct.h"
#include "Rn7302.h"
#include "algorithm_energy_remain.h"
#include "algorithm_energy_runing.h"

/*********************************************************************************************************
** Function name:       AccumulateRuningEnergy
** Descriptions:        convert sample value data to actual data
** input parameters:    u64remain   - 电度剩余值
                        real        - 
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
static int32_t AccumulateRuningEnergy(uint64_t *u64remain, uint64_t *real)
{
    uint64_t real_tmp;

    real_tmp = *real;
    if(*u64remain > 0)
    {
        real_tmp += *u64remain;
        real_tmp %= (ENERGY_MAXNUM + 1);
        *real = real_tmp;
        return 0;
    }
    else
    {
        return -1;
    }
}


/*********************************************************************************************************
** Function name:       rms_algorithm_RuningEnergy
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_RuningEnergy_hp(struct db_database_measure_base_type *pMeasureBase, struct db_runing_energy_type *out,  void *pNet, uint32_t pt, uint32_t ct)
{
    int i;
    struct db_basic_realtime_type       *pRealtime = &(pMeasureBase->db_measure.numeric.fullwave.primary);
    struct db_cfg_user_runing_para        *pSetpoint = &(pMeasureBase->db_cfg.user.runing);
    struct db_realtime_energy_type           *pNetEnergy = (struct db_realtime_energy_type*)pNet;

    uint64_t kWh_imp_temp   = 0;
    uint64_t kWh_exp_temp   = 0;
    uint64_t kvarh_imp_temp = 0;
    uint64_t kvarh_exp_temp = 0;
    uint64_t Kvah_temp      = 0;

    if(llabs(pRealtime->Ptot) > pSetpoint->setPtot.value)
    {
        if((pMeasureBase->db_cfg.user.base.WiringMode.value & 0x03) == P3W4)
        {
            for(i = 0; i < PHASE_TOT; i++)
            {
                AccumulateRuningEnergy(&pNetEnergy->kWhImp[i]  , &out->kWhImp[i]   );
                AccumulateRuningEnergy(&pNetEnergy->kWhExp[i]  , &out->kWhExp[i]   );
                AccumulateRuningEnergy(&pNetEnergy->kvarhImp[i], &out->kvarhImp[i] );
                AccumulateRuningEnergy(&pNetEnergy->kvarhExp[i], &out->kvarhExp[i] );
                AccumulateRuningEnergy(&pNetEnergy->kVAh[i]    , &out->kVAh[i]     );

                kWh_imp_temp   += pNetEnergy->kWhImp[i];
                kWh_exp_temp   += pNetEnergy->kWhExp[i];
                kvarh_imp_temp += pNetEnergy->kvarhImp[i];
                kvarh_exp_temp += pNetEnergy->kvarhExp[i];
                Kvah_temp      += pNetEnergy->kVAh[i];
            }

            out->kWhImp[3]   = (out->kWhImp[3]   + kWh_imp_temp   ) % (ENERGY_MAXNUM + 1);
            out->kWhExp[3]   = (out->kWhExp[3]   + kWh_exp_temp   ) % (ENERGY_MAXNUM + 1);
            out->kvarhImp[3] = (out->kvarhImp[3] + kvarh_imp_temp ) % (ENERGY_MAXNUM + 1);
            out->kvarhExp[3] = (out->kvarhExp[3] + kvarh_exp_temp ) % (ENERGY_MAXNUM + 1);
            out->kVAh[3]     = (out->kVAh[3]     + Kvah_temp      ) % (ENERGY_MAXNUM + 1);
        }
        else
        {
            AccumulateRuningEnergy(&pNetEnergy->kWhImp[3]  , &out->kWhImp[3]   );
            AccumulateRuningEnergy(&pNetEnergy->kWhExp[3]  , &out->kWhExp[3]   );
            AccumulateRuningEnergy(&pNetEnergy->kvarhImp[3], &out->kvarhImp[3] );
            AccumulateRuningEnergy(&pNetEnergy->kvarhExp[3], &out->kvarhExp[3] );
            AccumulateRuningEnergy(&pNetEnergy->kVAh[3]     , &out->kVAh[3]     );

        }

        out->kWhTot[3]   = (out->kWhTot[0]   + out->kWhTot[1]   + out->kWhTot[2]   ) % (ENERGY_MAXNUM + 1);
        out->kvarhTot[3] = (out->kvarhTot[0] + out->kvarhTot[1] + out->kvarhTot[2] ) % (ENERGY_MAXNUM + 1);
    }

    return 0;
}
