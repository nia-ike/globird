/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_halfwave_voltage.c
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
#include "algorithm_halfwave_voltage.h"
#include "Rn7302_register.h"
#include "algorithm_halfwave.h"

#define MULTIPLE_PT  100 

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
** Descriptions:        如果半波有效值寄存器 HW（选择为开根号）等于 32’h11E090=十进制 1171600，此时频率为 50Hz，
                            那么有效值寄存器 RMS 为：RMS=1171600*256/sqrt(80)= 299929600/8.94427191=33533148.7，
                            转换为十六进制为：28’ h1FFACDC
                        如果选择了不做开根号的结果，那么半波有效值寄存器 HW（自乘平方并累加）与有效值寄存器 RMS（0x31C~0x338）的关系是：
                            RMS= sqrt(HW*2^16)*256/ sqrt(4000/Ufreq)，其中 Ufreq 为基波频率。
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_halfwave_voltage(struct db_database_measure_base_type *pMeasureBase, struct db_basic_realtime_type *out, void *input, uint32_t pt, uint32_t ct, uint32_t time, uint32_t Hz_hp)
{
    uint64_t                    temp;
    uint64_t                    dead;
    struct db_cfg_sys_para               *pCfgSys = &(pMeasureBase->db_cfg.sys);
    struct rn7302_emu                       *pEmu = (struct rn7302_emu*)input;
    static struct db_basic_realtime_type hw[HALFWAVE_NUM];
    int i=0,j=0;
    float kf;
    i = time % HALFWAVE_NUM;
    uint32_t Krms;
    //FILE_NAME_UA
    Krms = pCfgSys->adjustAdvalue.Adjust_Va.value / pCfgSys->adjustAdvalue.Adjust_V.value / pCfgSys->adjustAd.Krms_V.value;
    hw[i].Va = pEmu->UA;
    kf = sqrt((float)(4000 * 10000) / Hz_hp);
    temp = 0;
    for(j=0;j<HALFWAVE_NUM; j++)
    {
        temp += hw[j].Va;
    }
    temp = (uint64_t)sqrt(temp * 65536 / HALFWAVE_NUM) * 256 / kf; // * 10000 . 因为Hz_hp放大了10000倍
    temp = (uint64_t) temp * pt / MULTIPLE_PT / Krms;
    dead = (uint64_t)pCfgSys->deadzone.Ua.value * pt / MULTIPLE_PT;
    if (0 != rms_algorithm_RangeConstaraint(temp, dead))
    {
        temp = 0;
    }    
    out->Va = temp;

    //FILE_NAME_UB
    Krms = pCfgSys->adjustAdvalue.Adjust_Vb.value / pCfgSys->adjustAdvalue.Adjust_V.value / pCfgSys->adjustAd.Krms_V.value;
    hw[i].Vb = pEmu->UB;
    temp = 0;
    for(j=0;j<HALFWAVE_NUM; j++)
    {
        temp += hw[j].Vb;
    }
    temp = (uint64_t)sqrt(temp * 65536 / HALFWAVE_NUM) * 256 / kf;
    temp = (uint64_t)temp  * pt / MULTIPLE_PT / Krms;
    dead = (uint64_t)pCfgSys->deadzone.Ub.value * pt / MULTIPLE_PT;
    if (0 != rms_algorithm_RangeConstaraint(temp, dead))
    {
        temp = 0;
    }    
    out->Vb = temp;
    //FILE_NAME_UC
    Krms = pCfgSys->adjustAdvalue.Adjust_Vc.value / pCfgSys->adjustAdvalue.Adjust_V.value / pCfgSys->adjustAd.Krms_V.value;
    hw[i].Vc = pEmu->UC;
    temp = 0;
    for(j=0;j<HALFWAVE_NUM; j++)
    {
        temp += hw[j].Vc;
    }
    temp = (uint64_t)sqrt(temp * 65536 / HALFWAVE_NUM) * 256 / kf;
    temp = (uint64_t)temp  * pt / MULTIPLE_PT / Krms;
    dead = (uint64_t)pCfgSys->deadzone.Uc.value * pt / MULTIPLE_PT;
    if (0 != rms_algorithm_RangeConstaraint(temp, dead))
    {
        temp = 0;
    }    
    out->Vc = temp;
    out->VlnAvg = ((uint64_t)out->Va + out->Vb + out->Vc) / 3;
/*  
*/
    return 0;
}

