/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_linevoltage.c
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
#include "Rn7302_register.h"
#include "algorithm_linevoltage.h"

#include "msr_process.h"
#include "debug.h"

#define MULTIPLE_U  100
#define  PI 3.1415926


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
** Function name:       rms_algorithm_LineVoltage
** Descriptions:        
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_linevoltage(struct db_database_measure_base_type *pMeasure, struct db_basic_realtime_type *out,  void *input, uint32_t pt, uint32_t ct)
{

    uint32_t                    u32Tmp = 0;
    uint64_t                    dead = 0;
    uint16_t                    mode = 0;
    uint32_t                    RN7302_Krmsu = 1000;
    uint64_t                    u64tmp[3]={0,0,0};
    double                      TmpF32,pha[3] = {0,0,0};
    struct db_cfg_user_para     *pCfgUser = &(pMeasure->db_cfg.user);
    struct db_cfg_sys_para      *pCfgSys = &(pMeasure->db_cfg.sys);
    struct rn7302_emu           *pEmu = (struct rn7302_emu*)input;


    mode = (pCfgUser->base.WiringMode.value & 0x03);
    RN7302_Krmsu = pCfgSys->adjustAdvalue.Adjust_Va.value / pCfgSys->adjustAdvalue.Adjust_V.value / pCfgSys->adjustAd.Krms_V.value;

    u64tmp[0] = (uint64_t)pEmu->UA * pt / RN7302_Krmsu;
    dead = (uint64_t)pCfgSys->deadzone.Ua.value * pt ;
    if (0 != rms_algorithm_RangeConstaraint(u64tmp[0], dead))
    {
        u64tmp[0] = 0;
    }
    u64tmp[0] = u64tmp[0] / MULTIPLE_U;

    u64tmp[1] = (uint64_t)pEmu->UB * pt / RN7302_Krmsu;
    dead = (uint64_t)pCfgSys->deadzone.Ub.value * pt ;
    if (0 != rms_algorithm_RangeConstaraint(u64tmp[1], dead))
    {
        u64tmp[1] = 0;
    }
    u64tmp[1] = u64tmp[1] / MULTIPLE_U;

    u64tmp[2] = (uint64_t)pEmu->UC * pt / RN7302_Krmsu;
    dead = (uint64_t)pCfgSys->deadzone.Uc.value * pt ;
    if (0 != rms_algorithm_RangeConstaraint(u64tmp[2], dead))
    {
        u64tmp[2] = 0;
    }
    u64tmp[2] = u64tmp[2] / MULTIPLE_U;


    pha[0] = (out->J_Va - out->J_Vb) * PI / 18000;
    pha[1] = (out->J_Vb - out->J_Vc) * PI / 18000;
    if(P3W3 == mode)
    {
        pha[2] = 5.23578;//1.04719;
    }
    else
    {
        pha[2] = (out->J_Vc - out->J_Va) * PI / 18000;
    }

    TmpF32 =  ((double)u64tmp[0] * u64tmp[0] + (double)u64tmp[1] * u64tmp[1] - ((double)2 * u64tmp[0] * u64tmp[1]) * cos(pha[0]));
    if(TmpF32 < 0)
    {
        TmpF32 = -TmpF32;
    }     
    u32Tmp = (uint32_t)sqrt((double)TmpF32);
    dead = (uint64_t)pCfgSys->deadzone.Uab.value * pt / MULTIPLE_U;
    if (0 != rms_algorithm_RangeConstaraint(u32Tmp, dead))
    {
        u32Tmp = 0;
    }
    out->Vab = u32Tmp;
    
    TmpF32 =  ((double)u64tmp[1] * u64tmp[1]  + (double)u64tmp[2] * u64tmp[2] - ((double)2 * u64tmp[1] * u64tmp[2]) * cos(pha[1]));
    if(TmpF32 < 0)
    {
        TmpF32 = -TmpF32;
    }     
    u32Tmp = (uint32_t)sqrt((double)TmpF32);
    dead = (uint64_t)pCfgSys->deadzone.Ubc.value * pt / MULTIPLE_U;
    if (0 != rms_algorithm_RangeConstaraint(u32Tmp, dead))
    {
        u32Tmp = 0;
    }
    out->Vbc = u32Tmp;

    TmpF32 =  ((double)u64tmp[2] * u64tmp[2] + (double)u64tmp[0] * u64tmp[0] - ((double)2 * u64tmp[2] * u64tmp[0]) * cos(pha[2]));   
    if(TmpF32 < 0)
    {
        TmpF32 = -TmpF32;
    }     
    u32Tmp = (uint32_t)sqrt((double)TmpF32);
    dead = (uint64_t)pCfgSys->deadzone.Uca.value * pt / MULTIPLE_U;
    if (0 != rms_algorithm_RangeConstaraint(u32Tmp, dead))
    {
        u32Tmp = 0;
    }
    out->Vca = u32Tmp;
    

    out->VllAvg = (out->Vab + out->Vbc + out->Vca) / 3;
/*
*/    
    return 0;
}
