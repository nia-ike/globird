
/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_primary_to_secondary.c
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
#include "algorithm_primary_to_secondary.h"



/*********************************************************************************************************
** Function name:       rms_algorithm_PrimaryJudge
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t rms_algorithm_PrimaryJudge(struct db_database_measure_base_type *pMeasureBase, struct db_realtime_secondary_type *out)
{
    uint16_t                    mode = 0;
    struct db_cfg_user_para             *pCfgUser = &(pMeasureBase->db_cfg.user);
    struct db_cfg_sys_para               *pCfgSys = &(pMeasureBase->db_cfg.sys);
    struct db_basic_realtime_type *pRealtimePri = &(pMeasureBase->db_measure.numeric.fullwave.primary);
    mode = (pCfgUser->base.WiringMode.value & 0x03);

    /*由于二次值电流是u16类型，在选项为100A时，电流计算过程中除了10倍，故在转换成一次侧时需要*10*/
    if(pCfgSys->interface.RatedCurrent.value == CURRENT_RATED_K100)
    {
        out->Ia = out->Ia * 10;
        out->Ib = out->Ib * 10;
        out->Ic = out->Ic * 10;
        out->IAvg = out->IAvg * 10;
    }

    if(P3W4 == mode)
    {
        if((pRealtimePri->Va == 0) && (pRealtimePri->Vb == 0) && (pRealtimePri->Vc == 0))
        {
            out->Vab = 0;
            out->Vbc = 0;
            out->Vca = 0;
            out->VllAvg = 0;
            pRealtimePri->J_Va = 0;
            pRealtimePri->J_Vb = 0;
            pRealtimePri->J_Vc = 0;
            if((out->Ia == 0) && (out->Ib == 0) && (out->Ic == 0))
            {
                pRealtimePri->J_Ia = 0;
                pRealtimePri->J_Ib = 0;
                pRealtimePri->J_Ic = 0;                
            }
        }
    }
    return 0;
}

/*********************************************************************************************************
** Function name:       rms_algorithm_Primary
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_Primary_to_secondary(struct db_database_measure_base_type *pMeasureBase, struct db_realtime_secondary_type *out, uint32_t pt, uint32_t ct)
{
    struct db_basic_realtime_type    *pRealtimePrimary = &(pMeasureBase->db_measure.numeric.fullwave.primary);

    out->Va = (uint64_t)pRealtimePrimary->Va * 100 / pt;
    out->Vb = (uint64_t)pRealtimePrimary->Vb * 100 / pt;
    out->Vc = (uint64_t)pRealtimePrimary->Vc * 100 / pt;
    out->VlnAvg = (uint64_t)pRealtimePrimary->VlnAvg * 100 / pt;
    out->Vab = (uint64_t)pRealtimePrimary->Vab * 100 / pt;
    out->Vbc = (uint64_t)pRealtimePrimary->Vbc * 100 / pt;
    out->Vca = (uint64_t)pRealtimePrimary->Vca * 100 / pt;
    out->VllAvg = (uint64_t)pRealtimePrimary->VllAvg * 100 / pt;
    out->Ia = (uint64_t)pRealtimePrimary->Ia * 1000 / ct;
    out->Ib = (uint64_t)pRealtimePrimary->Ib * 1000 / ct;
    out->Ic = (uint64_t)pRealtimePrimary->Ic * 1000 / ct;
    out->In = (uint32_t)pRealtimePrimary->In;        //漏电电流固定变比，与CT变比无关
    out->IAvg = (uint64_t)pRealtimePrimary->IAvg * 1000 / ct;
    out->Pa = (int64_t)pRealtimePrimary->Pa * 100 / pt * 100 / ct;
    out->Pb = (int64_t)pRealtimePrimary->Pb * 100 / pt * 100 / ct;
    out->Pc = (int64_t)pRealtimePrimary->Pc * 100 / pt * 100 / ct;
    out->Ptot = (int64_t)pRealtimePrimary->Ptot * 100 / pt * 100 / ct;
    out->Qa = (int64_t)pRealtimePrimary->Qa * 10000 / pt / ct;
    out->Qb = (int64_t)pRealtimePrimary->Qb * 10000 / pt / ct;
    out->Qc = (int64_t)pRealtimePrimary->Qc * 10000 / pt / ct;
    out->Qtot = (int64_t)pRealtimePrimary->Qtot * 10000 / pt / ct;
    out->Sa = (uint64_t)pRealtimePrimary->Sa * 10000 / pt / ct;
    out->Sb = (uint64_t)pRealtimePrimary->Sb * 10000 / pt / ct;
    out->Sc = (uint64_t)pRealtimePrimary->Sc * 10000 / pt / ct;
    out->Stot = (uint64_t)pRealtimePrimary->Stot * 10000 / pt / ct;


    out->Pab = (int64_t)pRealtimePrimary->Pab * 10000 / pt / ct;
    out->Pca = (int64_t)pRealtimePrimary->Pca * 10000 / pt / ct;
    out->Qab = (int64_t)pRealtimePrimary->Qab * 10000 / pt / ct;
    out->Qca = (int64_t)pRealtimePrimary->Qca * 10000 / pt / ct;
    out->Sab = (uint64_t)pRealtimePrimary->Sab * 10000 / pt / ct;
    out->Sca = (uint64_t)pRealtimePrimary->Sca * 10000 / pt / ct;


    rms_algorithm_PrimaryJudge(pMeasureBase, out);

    return 0;
}


/*********************************************************************************************************
** Function name:       rms_algorithm_fundwave_Judge
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t rms_algorithm_fundwave_Judge(struct db_database_measure_base_type *pMeasureBase)
{
    uint16_t                    mode = 0;
    struct db_cfg_user_para *pCfgUser = &(pMeasureBase->db_cfg.user);
    struct db_realtime_secondary_type *pRealtimeSecondary = &(pMeasureBase->db_measure.numeric.fundwave.secondary);

    mode = (pCfgUser->base.WiringMode.value & 0x03);
    if(P3W4 == mode)
    {
        if((pRealtimeSecondary->Va == 0) && (pRealtimeSecondary->Vb == 0) && (pRealtimeSecondary->Vc == 0))
        {
            pRealtimeSecondary->Vab = 0;
            pRealtimeSecondary->Vbc = 0;
            pRealtimeSecondary->Vca = 0;
            pRealtimeSecondary->VllAvg = 0;
           
        }
    }
    return 0;
}


/*********************************************************************************************************
** Function name:       rms_algorithm_Primary
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_fundwave_secondary(struct db_database_measure_base_type *pMeasureBase, struct db_realtime_secondary_type *out, uint32_t pt, uint32_t ct)
{
    struct db_cfg_user_para *pCfgUser = &(pMeasureBase->db_cfg.user);
    struct db_basic_realtime_type  *pRealtimePrimary   = &(pMeasureBase->db_measure.numeric.fundwave.primary);
    
    out->Va = (uint64_t)pRealtimePrimary->Va * 100 / pt;
    out->Vb = (uint64_t)pRealtimePrimary->Vb * 100 / pt;
    out->Vc = (uint64_t)pRealtimePrimary->Vc * 100 / pt;
    out->VlnAvg = (uint64_t)pRealtimePrimary->VlnAvg * 100 / pt;
    out->Vab = (uint64_t)pRealtimePrimary->Vab * 100 / pt;
    out->Vbc = (uint64_t)pRealtimePrimary->Vbc * 100 / pt;
    out->Vca = (uint64_t)pRealtimePrimary->Vca * 100 / pt;
    out->VllAvg = (uint64_t)pRealtimePrimary->VllAvg * 100 / pt;
    out->Vn = (uint64_t)pRealtimePrimary->Vn;        //漏电电流固定变比，与CT变比无关
    out->Ia = (uint64_t)pRealtimePrimary->Ia * 100 / ct;
    out->Ib = (uint64_t)pRealtimePrimary->Ib * 100 / ct;
    out->Ic = (uint64_t)pRealtimePrimary->Ic * 100 / ct;
    out->In = (uint64_t)pRealtimePrimary->In;        //漏电电流固定变比，与CT变比无关
    out->IAvg = (uint64_t)pRealtimePrimary->IAvg * 100 / ct;
    out->Pa = (int64_t)pRealtimePrimary->Pa * 100 / pt * 100 / ct;
    out->Pb = (int64_t)pRealtimePrimary->Pb * 100 / pt * 100 / ct;
    out->Pc = (int64_t)pRealtimePrimary->Pc * 100 / pt * 100 / ct;
    out->Ptot = (int64_t)pRealtimePrimary->Ptot * 100 / pt * 100 / ct;
    out->Qa = (int64_t)pRealtimePrimary->Qa * 100 / pt * 100 / ct;
    out->Qb = (int64_t)pRealtimePrimary->Qb * 100 / pt * 100 / ct;
    out->Qc = (int64_t)pRealtimePrimary->Qc * 100 / pt * 100 / ct;
    out->Qtot = (int64_t)pRealtimePrimary->Qtot * 100 / pt * 100 / ct;
    out->Sa = (uint64_t)pRealtimePrimary->Sa * 100 / pt * 100 / ct;
    out->Sb = (uint64_t)pRealtimePrimary->Sb * 100 / pt * 100 / ct;
    out->Sc = (uint64_t)pRealtimePrimary->Sc * 100 / pt * 100 / ct;
    out->Stot = (uint64_t)pRealtimePrimary->Stot * 100 / pt * 100 / ct;



    rms_algorithm_fundwave_Judge(pMeasureBase);

    return 0;
}



/*********************************************************************************************************
** Function name:       rms_algorithm_PrimaryJudge
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t rms_algorithm_halfwave_PrimaryJudge(void *in, struct db_realtime_secondary_type *out)
{
    uint16_t                    mode = 0;
    struct db_database_measure_base_type          *pstVmem = (struct db_database_measure_base_type *)in;
    struct db_cfg_user_para             *pCfgUser = &(pstVmem->db_cfg.user);
    struct db_cfg_sys_para               *pCfgSys = &(pstVmem->db_cfg.sys);
    struct db_basic_realtime_type *pRealtimePri = &(pstVmem->db_measure.numeric.halfwave.primary);
    mode = (pCfgUser->base.WiringMode.value & 0x03);

    if(pCfgSys->interface.RatedCurrent.value == CURRENT_RATED_K100)
    {
        out->Ia = out->Ia * 10;
        out->Ib = out->Ib * 10;
        out->Ic = out->Ic * 10;
        out->IAvg = out->IAvg * 10;
    }

    if(P3W4 == mode)
    {
        if((pRealtimePri->Va == 0) && (pRealtimePri->Vb == 0) && (pRealtimePri->Vc == 0))
        {
            out->Vab = 0;
            out->Vbc = 0;
            out->Vca = 0;
            out->VllAvg = 0;
            pRealtimePri->J_Va = 0;
            pRealtimePri->J_Vb = 0;
            pRealtimePri->J_Vc = 0;
            if((out->Ia == 0) && (out->Ib == 0) && (out->Ic == 0))
            {
                pRealtimePri->J_Ia = 0;
                pRealtimePri->J_Ib = 0;
                pRealtimePri->J_Ic = 0;                
            }
        }
    }
    return 0;
}

/*********************************************************************************************************
** Function name:       rms_algorithm_Primary
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t rms_algorithm_halfwave_Primary_to_secondary(struct db_database_measure_base_type *pstVmem, struct db_realtime_secondary_type *out, uint32_t pt, uint32_t ct)
{
    struct db_basic_realtime_type    *pRealtimePrimary = &(pstVmem->db_measure.numeric.halfwave.primary);

    out->Va = (uint64_t)pRealtimePrimary->Va * 100 / pt;
    out->Vb = (uint64_t)pRealtimePrimary->Vb * 100 / pt;
    out->Vc = (uint64_t)pRealtimePrimary->Vc * 100 / pt;
    out->VlnAvg = (uint64_t)pRealtimePrimary->VlnAvg * 100 / pt;
    out->Vab = (uint64_t)pRealtimePrimary->Vab * 100 / pt;
    out->Vbc = (uint64_t)pRealtimePrimary->Vbc * 100 / pt;
    out->Vca = (uint64_t)pRealtimePrimary->Vca * 100 / pt;
    out->VllAvg = (uint64_t)pRealtimePrimary->VllAvg * 100 / pt;
    out->Ia = (uint64_t)pRealtimePrimary->Ia * 1000 / ct;
    out->Ib = (uint64_t)pRealtimePrimary->Ib * 1000 / ct;
    out->Ic = (uint64_t)pRealtimePrimary->Ic * 1000 / ct;
    out->In = (uint32_t)pRealtimePrimary->In;        //漏电电流固定变比，与CT变比无关
    out->IAvg = (uint64_t)pRealtimePrimary->IAvg * 1000 / ct;
    out->Pa = (int64_t)pRealtimePrimary->Pa * 100 / pt * 100 / ct;
    out->Pb = (int64_t)pRealtimePrimary->Pb * 100 / pt * 100 / ct;
    out->Pc = (int64_t)pRealtimePrimary->Pc * 100 / pt * 100 / ct;
    out->Ptot = (int64_t)pRealtimePrimary->Ptot * 100 / pt * 100 / ct;
    out->Qa = (int64_t)pRealtimePrimary->Qa * 10000 / pt / ct;
    out->Qb = (int64_t)pRealtimePrimary->Qb * 10000 / pt / ct;
    out->Qc = (int64_t)pRealtimePrimary->Qc * 10000 / pt / ct;
    out->Qtot = (int64_t)pRealtimePrimary->Qtot * 10000 / pt / ct;
    out->Sa = (uint64_t)pRealtimePrimary->Sa * 10000 / pt / ct;
    out->Sb = (uint64_t)pRealtimePrimary->Sb * 10000 / pt / ct;
    out->Sc = (uint64_t)pRealtimePrimary->Sc * 10000 / pt / ct;
    out->Stot = (uint64_t)pRealtimePrimary->Stot * 10000 / pt / ct;


    out->Pab = (int64_t)pRealtimePrimary->Pab * 10000 / pt / ct;
    out->Pca = (int64_t)pRealtimePrimary->Pca * 10000 / pt / ct;
    out->Qab = (int64_t)pRealtimePrimary->Qab * 10000 / pt / ct;
    out->Qca = (int64_t)pRealtimePrimary->Qca * 10000 / pt / ct;
    out->Sab = (uint64_t)pRealtimePrimary->Sab * 10000 / pt / ct;
    out->Sca = (uint64_t)pRealtimePrimary->Sca * 10000 / pt / ct;


    rms_algorithm_halfwave_PrimaryJudge(pstVmem, out);
/* 

*/
    return 0;
}