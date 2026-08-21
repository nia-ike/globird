
/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_powercaporind.c
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
#include <stdint.h>
#include <math.h>

#include "algorithm_powercaporind.h"


/*********************************************************************************************************
** Function name:       rms_algorithm_PowerCapOrInd
** Descriptions:        A,B,C,�����ݸ����ж�
                        0-3λ�ֱ����A,B,C,�����ݸ���,ֵ0:���� 1:����
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_PowerCapOrInd(struct db_database_measure_base_type  *pMeasureBase, struct db_basic_realtime_type *out, uint32_t pt, uint32_t ct)
{
    uint8_t   cap_tmp[4] ={0};

    if(((out->Pa >= 0) && (out->Qa >= 0)) || ((out->Pa < 0) && (out->Qa < 0)))
    {
        cap_tmp[0] = 0;
    }
    else
    {
        cap_tmp[0] = 1;
    }
    
    if(((out->Pb >= 0) && (out->Qb >= 0)) || ((out->Pb < 0) && (out->Qb < 0)))
    {
        cap_tmp[1] = 0;
    }
    else
    {
        cap_tmp[1] = 1;
    }
    
    if(((out->Pc >= 0) && (out->Qc >= 0)) || ((out->Pc < 0) && (out->Qc < 0)))
    {
        cap_tmp[2] = 0;
    }
    else
    {
        cap_tmp[2] = 1;
    }
    
    if(((out->Ptot >= 0) && (out->Qtot >= 0)) || ((out->Ptot < 0) && (out->Qtot < 0)))
    {
        cap_tmp[3] = 0;
    }
    else
    {
        cap_tmp[3] = 1;
    }
    
    out->P_IndCap = (cap_tmp[0] << 0) | (cap_tmp[1] << 1) | (cap_tmp[2] << 2) | (cap_tmp[3] << 3);
 
//   printf("P_IndCap: %d \n",out->P_IndCap);
    
    return 0;
}

