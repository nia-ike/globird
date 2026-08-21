/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_frequency.c
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

#include "algorithm_frequency.h"

#define MULTIPLE_UFreq  10000  
#define FOSC        8192000
#define UFREQ       50
#define UFreq_MAX   (UFREQ * 1.3)
#define UFreq_MIN   (UFREQ * 0.7)
#define REGF        (FOSC * 8 / UFREQ)
#define REGF_MAX    (FOSC * 8 / UFreq_MIN)
#define REGF_MIN    (FOSC * 8 / UFreq_MAX)
/*********************************************************************************************************
** Function name:       rms_algorithm_FrequencyJudge
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t rms_algorithm_FrequencyJudge(struct db_database_measure_base_type *pstVmem, struct db_basic_realtime_type *out)
{
    uint16_t                        mode = 0;
    struct db_cfg_user_para        *pCfgUser = &(pstVmem->db_cfg.user);

    mode = (pCfgUser->base.WiringMode.value & 0x03);

    if((out->Hz_hp > ((uint32_t)MULTIPLE_UFreq * UFreq_MAX)) || (out->Hz_hp < ((uint32_t)MULTIPLE_UFreq * UFreq_MIN)))
    {
        out->Hz_hp = UFREQ * MULTIPLE_UFreq;
    }

    if(P3W4 == mode)
    {
        if((out->Va == 0) && (out->Vb == 0) && (out->Vc == 0))
        {
            out->Hz_hp = 0;
        }
    }
    else
    {
        if((out->Vab == 0) && (out->Vbc == 0) && (out->Vca == 0))
        {
            out->Hz_hp = 0;
        }
    }

    return 0;
}



/*********************************************************************************************************
** Function name:       rms_algorithm_Frequency
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_frequency(struct db_database_measure_base_type *pstVmem, struct db_basic_realtime_type *out,  void *input, uint32_t pt, uint32_t ct)
{
    struct rn7302_emu                          *pEmu = (struct rn7302_emu*)input;

    if(pEmu->UFreq == 0)
    {
        out->Hz_hp = 0;
        return -1;
    }
    out->Hz_hp = (uint32_t)((uint64_t)FOSC * 8 * MULTIPLE_UFreq / pEmu->UFreq + 1);
    rms_algorithm_FrequencyJudge(pstVmem, out);

    out->Hz = out->Hz_hp / 100;
    return 0;
}

/*********************************************************************************************************
** Function name:       rms_algorithm_Frequency
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
uint32_t rms_algorithm_frequency_hw(struct db_database_measure_base_type *pstVmem, void *input)
{
    uint32_t Hz_hp;
    struct rn7302_emu  *pEmu = (struct rn7302_emu*)input;

    if(pEmu->UFreq == 0)
    {
        Hz_hp = 0;
        return Hz_hp;
    }
    Hz_hp = (uint32_t)((uint64_t)FOSC * 8 * MULTIPLE_UFreq / pEmu->UFreq + 1);
    
    return Hz_hp;
}
