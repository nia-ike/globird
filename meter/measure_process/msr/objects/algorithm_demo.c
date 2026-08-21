
/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_demo.c
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
#include "Rn7302File.h"
#include "algorithm_demo.h"

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
int32_t rms_algorithm_Demo(struct db_database_measure_base_type  *pMeasureBase)
{
    struct db_basic_realtime_type    *pRealtimePrimary = &(pMeasureBase->db_measure.numeric.fullwave.primary);

    pRealtimePrimary->Va = 22001;
    pRealtimePrimary->Vb = 22002;
    pRealtimePrimary->Vc = 22003;
    pRealtimePrimary->VlnAvg = 22002;
    pRealtimePrimary->Vab = 38101;
    pRealtimePrimary->Vbc = 38102;
    pRealtimePrimary->Vca = 38103;
    pRealtimePrimary->VllAvg = 38102;
    pRealtimePrimary->Ia = 5001;
    pRealtimePrimary->Ib = 5002;
    pRealtimePrimary->Ic = 5003;
    pRealtimePrimary->IAvg = 5002;
    pRealtimePrimary->In = 0;
    pRealtimePrimary->Pa = 5501;
    pRealtimePrimary->Pb = 5502;
    pRealtimePrimary->Pc = 5503;
    pRealtimePrimary->Ptot = 16501;
    pRealtimePrimary->Qa = 9521;
    pRealtimePrimary->Qb = 9522;
    pRealtimePrimary->Qc = 9523;
    pRealtimePrimary->Qtot = 28564;
    pRealtimePrimary->Sa = 11001;
    pRealtimePrimary->Sb = 11002;
    pRealtimePrimary->Sc = 11003;
    pRealtimePrimary->Stot = 33004;
    pRealtimePrimary->PFa = 501;
    pRealtimePrimary->PFb = 502;
    pRealtimePrimary->PFc = 503;
    pRealtimePrimary->PFtot = 502;
    pRealtimePrimary->Hz = 5001;

    pRealtimePrimary->J_Va = 0;
    pRealtimePrimary->J_Vb = 12001;
    pRealtimePrimary->J_Vc = 24002;
    pRealtimePrimary->J_Ia = 6001;
    pRealtimePrimary->J_Ib = 18002;
    pRealtimePrimary->J_Ic = 30003;

    return 0;
}
