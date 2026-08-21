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
int32_t rms_algorithm_RealtimeEnergy_trans(struct db_database_measure_base_type    *pMeasureBase, struct db_energy_realtime_translate_type *out, struct db_realtime_energy_type *phpEnergy, uint32_t pt, uint32_t ct)
{
    struct db_realtime_energy_type *pEngRtHP = &(pMeasureBase->db_measure.energy.realtime_hp);

    out->EPT     = pEngRtHP->kWhTot[3]   % (ENERGY_MAXNUM_6D3 + 1);
    out->EQT     = pEngRtHP->kvarhTot[3] % (ENERGY_MAXNUM_6D3 + 1);
    out->EPAImp  = pEngRtHP->kWhImp[0]   % (ENERGY_MAXNUM_6D3 + 1);
    out->EPBImp  = pEngRtHP->kWhImp[1]   % (ENERGY_MAXNUM_6D3 + 1);
    out->EPCImp  = pEngRtHP->kWhImp[2]   % (ENERGY_MAXNUM_6D3 + 1);
    out->EPTImp  = pEngRtHP->kWhImp[3]   % (ENERGY_MAXNUM_6D3 + 1);
    out->EPAExp  = pEngRtHP->kWhExp[0]   % (ENERGY_MAXNUM_6D3 + 1);
    out->EPBExp  = pEngRtHP->kWhExp[1]   % (ENERGY_MAXNUM_6D3 + 1);
    out->EPCExp  = pEngRtHP->kWhExp[2]   % (ENERGY_MAXNUM_6D3 + 1);
    out->EPTExp  = pEngRtHP->kWhExp[3]   % (ENERGY_MAXNUM_6D3 + 1);
    out->EQAImp  = pEngRtHP->kvarhImp[0] % (ENERGY_MAXNUM_6D3 + 1);
    out->EQBImp  = pEngRtHP->kvarhImp[1] % (ENERGY_MAXNUM_6D3 + 1);
    out->EQCImp  = pEngRtHP->kvarhImp[2] % (ENERGY_MAXNUM_6D3 + 1);
    out->EQTImp  = pEngRtHP->kvarhImp[3] % (ENERGY_MAXNUM_6D3 + 1);
    out->EQAExp  = pEngRtHP->kvarhExp[0] % (ENERGY_MAXNUM_6D3 + 1);
    out->EQBExp  = pEngRtHP->kvarhExp[1] % (ENERGY_MAXNUM_6D3 + 1);
    out->EQCExp  = pEngRtHP->kvarhExp[2] % (ENERGY_MAXNUM_6D3 + 1);
    out->EQTExp  = pEngRtHP->kvarhExp[3] % (ENERGY_MAXNUM_6D3 + 1);
    out->ESA     = pEngRtHP->kVAh[0]   % (ENERGY_MAXNUM_6D3 + 1);
    out->ESB     = pEngRtHP->kVAh[1]   % (ENERGY_MAXNUM_6D3 + 1);
    out->ESC     = pEngRtHP->kVAh[2]   % (ENERGY_MAXNUM_6D3 + 1);
    out->EST     = pEngRtHP->kVAh[3]   % (ENERGY_MAXNUM_6D3 + 1);
}
