/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_halfwave_current.c
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
#include <sys/time.h>

#include <time.h>

#include "Rn7302.h"
#include "msr_process.h"
#include "algorithm_halfwave_current.h"
#include "Rn7302_register.h"
#include "algorithm_halfwave.h"

#include "Rn7302_read.h"

#define MULTIPLE_I  100
#define MULTIPLE_HZ  10000

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
** Function name:       rms_algorithm_Current
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_halfwave_current(struct db_database_measure_base_type *pMeasureBase, struct db_basic_realtime_type *out, void *input, uint32_t pt, uint32_t ct, uint32_t time, uint32_t Hz_hp)
{
    uint64_t                    temp = 0;
    uint64_t                    dead = 0;
    struct db_cfg_sys_para       *pCfgSys = &(pMeasureBase->db_cfg.sys);
    struct rn7302_emu            *pEmu = (struct rn7302_emu*)input;
    uint32_t                     RN7302_Krmsi = 1; 
    float                       kf;     
    static struct db_basic_realtime_type hw[HALFWAVE_NUM];
    int i=0,j=0;
    i = time % HALFWAVE_NUM;
    //FILE_NAME_IA
    RN7302_Krmsi = pCfgSys->adjustAdvalue.Adjust_Ia.value / pCfgSys->adjustAdvalue.Adjust_I.value / pCfgSys->adjustAd.Krms_I.value;
    hw[i].Ia = pEmu->IA;
    kf = sqrt((float)(4000 * 10000) / Hz_hp);
    temp = 0;
    for(j=0;j<HALFWAVE_NUM; j++)
    {
        temp += hw[j].Ia;
    }
    temp = (uint64_t)sqrt(temp * 65536 / HALFWAVE_NUM) * 256 / kf;
    dead = (uint64_t)pCfgSys->deadzone.Ia.value * ct / MULTIPLE_I;
    temp = (uint64_t)temp * ct / MULTIPLE_I  / RN7302_Krmsi;
    if (0 != rms_algorithm_RangeConstaraint(temp, dead))
    {
        temp = 0;
    }
    out->Ia = temp;

    //FILE_NAME_IB
    RN7302_Krmsi = pCfgSys->adjustAdvalue.Adjust_Ib.value / pCfgSys->adjustAdvalue.Adjust_I.value / pCfgSys->adjustAd.Krms_I.value;
    hw[i].Ib = pEmu->IB;
    temp = 0;
    for(j=0;j<HALFWAVE_NUM; j++)
    {
        temp += hw[j].Ib;
    }
    temp = (uint64_t)sqrt(temp * 65536 / HALFWAVE_NUM) * 256 / kf;
    dead = (uint64_t)pCfgSys->deadzone.Ib.value * ct / MULTIPLE_I;
    temp = (uint64_t)temp * ct / MULTIPLE_I  / RN7302_Krmsi;
    if (0 != rms_algorithm_RangeConstaraint(temp, dead))
    {
        temp = 0;
    }
    out->Ib = temp;
    
    //FILE_NAME_IC
    RN7302_Krmsi = pCfgSys->adjustAdvalue.Adjust_Ic.value / pCfgSys->adjustAdvalue.Adjust_I.value / pCfgSys->adjustAd.Krms_I.value;
    hw[i].Ic = pEmu->IC;
    temp = 0;
    for(j=0;j<HALFWAVE_NUM; j++)
    {
        temp += hw[j].Ic;
    }
    temp = (uint64_t)sqrt(temp * 65536 / HALFWAVE_NUM) * 256 / kf;
    dead = (uint64_t)pCfgSys->deadzone.Ic.value * ct / MULTIPLE_I;
    temp = (uint64_t)temp * ct / MULTIPLE_I  / RN7302_Krmsi;
    if (0 != rms_algorithm_RangeConstaraint(temp, dead))
    {
        temp = 0;
    }
    out->Ic = temp;

    out->IAvg = (out->Ia + out->Ib + out->Ic) / 3;
      /*
      */ 
    return 0;
}

