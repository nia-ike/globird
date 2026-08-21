
/*
* Copyright (c) 2019,�麣�Ƽ��ɷ����޹�˾
* All rights reserved.
* 
* �ļ����ƣ�algorithm_current.c
* �ļ���ʶ��
* ��    ����
* 
* ��ǰ�汾��V1.0
* ��    �ߣ�raoy
* ����˵����
* ������ڣ�20190425
* ���л�����
����ϵͳ: RT-Thread
Ӳ��ƽ̨��STM32F030
�̼���  ��V3.1.2
* �޸ļ�¼ 
*  ����       ʱ��         �汾      �޸�����
*  raoy   2019.04.25       V1.00      ����
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <time.h>

#include "Rn7302.h"
#include "algorithm_halfwave_reactivepower.h"

#include "Rn7302_read.h"

#define NUM  2  
#define MULTIPLE_PTCT  10000  


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

static int32_t rms_algorithm_current_reactivePowerJudge(struct db_database_measure_base_type *pMeasureBase, int64_t *out1, int64_t *out2, int64_t *out3, int64_t *out4)
{
    struct db_basic_realtime_type *realtime = &(pMeasureBase->db_measure.numeric.halfwave.primary);
    struct db_cfg_user_para *pCfgUser = &(pMeasureBase->db_cfg.user);
    uint16_t                        wireMode               = 0;
    wireMode = (pCfgUser->base.WiringMode.value & 0x03);
    
    if((realtime->Ia == 0) || (realtime->Va == 0))
    {
        *out1 = 0;
    }
    if((realtime->Ib == 0) || (realtime->Vb == 0))
    {
        *out2 = 0;
    }
    if((realtime->Ic == 0) || (realtime->Vc == 0))
    {
        *out3 = 0;
    }

    if(wireMode == P3W3)
    {
        realtime->Qab = *out1;
        realtime->Qca = *out3;
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
int32_t rms_algorithm_halfwave_reactivepower(struct db_database_measure_base_type *pMeasureBase, int64_t *out1, int64_t *out2, int64_t *out3, int64_t *out4, int64_t input1, int32_t input2, int64_t input3, int64_t input4, uint32_t pt, uint32_t ct, uint32_t time)
{
    int64_t                    temp;
    struct db_cfg_sys_para      *pCfgSys = &(pMeasureBase->db_cfg.sys);
    uint32_t                     RN7302_Krmsp = 1;    
    
      
    RN7302_Krmsp = pCfgSys->adjustAdvalue.Adjust_Pa.value / pCfgSys->adjustAdvalue.Adjust_P.value / pCfgSys->adjustAd.Krms_P.value;
    temp = (int64_t)input1 *pt /100 * ct / 100 / (int32_t)RN7302_Krmsp;
    *out1= temp;
    
    RN7302_Krmsp = pCfgSys->adjustAdvalue.Adjust_Pb.value / pCfgSys->adjustAdvalue.Adjust_P.value / pCfgSys->adjustAd.Krms_P.value;
    temp = (int64_t)input2 *pt /100 * ct / 100 / (int32_t)RN7302_Krmsp;
    *out2 = temp;

    RN7302_Krmsp = pCfgSys->adjustAdvalue.Adjust_Pc.value / pCfgSys->adjustAdvalue.Adjust_P.value / pCfgSys->adjustAd.Krms_P.value;
    temp = (int64_t)input3 *pt /100 * ct / 100 / (int32_t)RN7302_Krmsp;
    *out3 = temp;
    

    rms_algorithm_current_reactivePowerJudge(pMeasureBase, out1, out2, out3, out4);

    /* 星型接线时，为保证总功率数据的一致性，采用三相功率代数和的计算方式 */
    *out4 = *out1 + *out2 + *out3;
/*  
  */ 

    return 0;
}


