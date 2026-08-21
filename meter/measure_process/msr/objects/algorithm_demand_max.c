/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_demand_max.c
*  文件标识：
*  描    述：需量最值
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
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include "algorithm_demand_max.h"
#include "algorithm_demand.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <unistd.h>


/*********************************************************************************************************
** Function name:       DemandMaxDayClear
** Descriptions:        清除需量日最值
                        
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t DemandMaxDayClear(struct db_demand_max_type *out,uint32_t time)
{
    memset(&out->day,0,sizeof(struct db_demand_max_base_type));
    out->day.current.Ia.time = time;
    out->day.current.Ib.time = time;
    out->day.current.Ic.time = time;
    out->day.current.IAvg.time = time;
    out->day.current.In.time = time;
    out->day.power.Pa.time = time;
    out->day.power.Pb.time = time;
    out->day.power.Pc.time = time;
    out->day.power.Ptot.time = time;
    out->day.power.Qa.time = time;
    out->day.power.Qb.time = time;
    out->day.power.Qc.time = time;
    out->day.power.Qtot.time = time;
    out->day.power.Sa.time = time;
    out->day.power.Sb.time = time;
    out->day.power.Sc.time = time;
    out->day.power.Stot.time = time;
    return 0;
}

/*********************************************************************************************************
** Function name:       DemandMaxDayClear
** Descriptions:        清除需量月最值
                        
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t DemandMaxMonthClear(struct db_demand_max_type *out,uint32_t time)
{
    memset(&out->month,0,sizeof(struct db_demand_max_base_type));
    out->month.current.Ia.time = time;
    out->month.current.Ib.time = time;
    out->month.current.Ic.time = time;
    out->month.current.IAvg.time = time;
    out->month.current.In.time = time;
    out->month.power.Pa.time = time;
    out->month.power.Pb.time = time;
    out->month.power.Pc.time = time;
    out->month.power.Ptot.time = time;
    out->month.power.Qa.time = time;
    out->month.power.Qb.time = time;
    out->month.power.Qc.time = time;
    out->month.power.Qtot.time = time;
    out->month.power.Sa.time = time;
    out->month.power.Sb.time = time;
    out->month.power.Sc.time = time;
    out->month.power.Stot.time = time;

    return 0;
}




/*********************************************************************************************************
** Function name:       ExtremumDayClear
** Descriptions:        清除日极值
                        
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t DmdDayUpdate_adjust(struct db_database_measure_base_type *pMeasureBase, uint32_t DmdTimeTmp)
{
    struct db_demand_max_type *pDemandMax = &(pMeasureBase->db_measure.statistical.demand.max);
    struct db_demand_base_type *StrDemandVal = &(pMeasureBase->db_measure.statistical.demand.realtime);

    pDemandMax->day.current.Ia.value = StrDemandVal->Ia;
    pDemandMax->day.current.Ib.value = StrDemandVal->Ib;
    pDemandMax->day.current.Ic.value = StrDemandVal->Ic;
    pDemandMax->day.current.IAvg.value = StrDemandVal->IAvg;
    pDemandMax->day.current.In.value = StrDemandVal->In;
    pDemandMax->day.power.Pa.value = StrDemandVal->Pa;
    pDemandMax->day.power.Pb.value = StrDemandVal->Pb;
    pDemandMax->day.power.Pc.value = StrDemandVal->Pc;
    pDemandMax->day.power.Ptot.value = StrDemandVal->Ptot;
    pDemandMax->day.power.Qa.value = StrDemandVal->Qa;
    pDemandMax->day.power.Qb.value = StrDemandVal->Qb;
    pDemandMax->day.power.Qc.value = StrDemandVal->Qc;
    pDemandMax->day.power.Qtot.value = StrDemandVal->Qtot;
    pDemandMax->day.power.Sa.value = StrDemandVal->Sa;
    pDemandMax->day.power.Sb.value = StrDemandVal->Sb;
    pDemandMax->day.power.Sc.value = StrDemandVal->Sc;
    pDemandMax->day.power.Stot.value = StrDemandVal->Stot;
    pDemandMax->day.current.Ia.time = DmdTimeTmp;
    pDemandMax->day.current.Ib.time = DmdTimeTmp;
    pDemandMax->day.current.Ic.time = DmdTimeTmp;
    pDemandMax->day.current.IAvg.time = DmdTimeTmp;
    pDemandMax->day.current.In.time = DmdTimeTmp;
    pDemandMax->day.power.Pa.time = DmdTimeTmp;
    pDemandMax->day.power.Pb.time = DmdTimeTmp;
    pDemandMax->day.power.Pc.time = DmdTimeTmp;
    pDemandMax->day.power.Ptot.time = DmdTimeTmp;
    pDemandMax->day.power.Qa.time = DmdTimeTmp;
    pDemandMax->day.power.Qb.time = DmdTimeTmp;
    pDemandMax->day.power.Qc.time = DmdTimeTmp;
    pDemandMax->day.power.Qtot.time = DmdTimeTmp;
    pDemandMax->day.power.Sa.time = DmdTimeTmp;
    pDemandMax->day.power.Sb.time = DmdTimeTmp;
    pDemandMax->day.power.Sc.time = DmdTimeTmp;
    pDemandMax->day.power.Stot.time = DmdTimeTmp;
    return 0;
}

/*********************************************************************************************************
** Function name:       ExtremumDayClear
** Descriptions:        清除日极值
                        
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t DmdMonthUpdate_adjust(struct db_database_measure_base_type *pMeasureBase, uint32_t DmdTimeTmp)
{
    struct db_demand_max_type *pDemandMax = &(pMeasureBase->db_measure.statistical.demand.max);
    struct db_demand_base_type *StrDemandVal = &(pMeasureBase->db_measure.statistical.demand.realtime);

    pDemandMax->month.current.Ia.value = StrDemandVal->Ia;
    pDemandMax->month.current.Ib.value = StrDemandVal->Ib;
    pDemandMax->month.current.Ic.value = StrDemandVal->Ic;
    pDemandMax->month.current.IAvg.value = StrDemandVal->IAvg;
    pDemandMax->month.current.In.value = StrDemandVal->In;
    pDemandMax->month.power.Pa.value = StrDemandVal->Pa;
    pDemandMax->month.power.Pb.value = StrDemandVal->Pb;
    pDemandMax->month.power.Pc.value = StrDemandVal->Pc;
    pDemandMax->month.power.Ptot.value = StrDemandVal->Ptot;
    pDemandMax->month.power.Qa.value = StrDemandVal->Qa;
    pDemandMax->month.power.Qb.value = StrDemandVal->Qb;
    pDemandMax->month.power.Qc.value = StrDemandVal->Qc;
    pDemandMax->month.power.Qtot.value = StrDemandVal->Qtot;
    pDemandMax->month.power.Sa.value = StrDemandVal->Sa;
    pDemandMax->month.power.Sb.value = StrDemandVal->Sb;
    pDemandMax->month.power.Sc.value = StrDemandVal->Sc;
    pDemandMax->month.power.Stot.value = StrDemandVal->Stot;
    pDemandMax->month.current.Ia.time = DmdTimeTmp;
    pDemandMax->month.current.Ib.time = DmdTimeTmp;
    pDemandMax->month.current.Ic.time = DmdTimeTmp;
    pDemandMax->month.current.IAvg.time = DmdTimeTmp;
    pDemandMax->month.current.In.time = DmdTimeTmp;
    pDemandMax->month.power.Pa.time = DmdTimeTmp;
    pDemandMax->month.power.Pb.time = DmdTimeTmp;
    pDemandMax->month.power.Pc.time = DmdTimeTmp;
    pDemandMax->month.power.Ptot.time = DmdTimeTmp;
    pDemandMax->month.power.Qa.time = DmdTimeTmp;
    pDemandMax->month.power.Qb.time = DmdTimeTmp;
    pDemandMax->month.power.Qc.time = DmdTimeTmp;
    pDemandMax->month.power.Qtot.time = DmdTimeTmp;
    pDemandMax->month.power.Sa.time = DmdTimeTmp;
    pDemandMax->month.power.Sb.time = DmdTimeTmp;
    pDemandMax->month.power.Sc.time = DmdTimeTmp;
    pDemandMax->month.power.Stot.time = DmdTimeTmp;
    return 0;
}

/*********************************************************************************************************
** Function name:       DmdRecordUpdate_adjust
** Descriptions:        零点判断
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t DmdRecordUpdate_adjust(struct db_database_measure_base_type *pMeasureBase)
{
    static time_t last;
    struct tm tm_now, tm_last;
    time_t now; 
    
    now = time(NULL); 
    localtime_r((time_t *)&now, &tm_now); 
    /* 0 点 0分, 延迟3秒为了跟DB记录的0点时间错开，延时5秒清数据 整1分钟都会进入 */
    if((tm_now.tm_hour == 0) && (tm_now.tm_min == 0))
    {
        if(tm_now.tm_sec > 3)
        {
            localtime_r((time_t *)&last, &tm_last);
            /* last 与 now的 小时或者分钟不同，即代表这分钟内第一次跨零点才触发，  */
            if(((tm_last.tm_hour != 0) || (tm_last.tm_min != 0)))
            {
                DmdDayUpdate_adjust(pMeasureBase, now);
                if(tm_now.tm_mday == 1)
                {
                    DmdMonthUpdate_adjust(pMeasureBase, now); 
                }   
            }
            
            last = now;
            return 0;

        }
        
        /* 过零，但未达到延时3秒，不更新last， 用于满足延迟3秒后，last仍保持23:59:59，执行 */
        return -1;
    }
    /* 更新last */
    last = now;
    return 0;
}


/*********************************************************************************************************
** Function name:       DmdRecordDayUpdate
** Descriptions:        更新日需量最大值
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t  demand_max_adjust_U32(uint64_t value, struct db_demand_max_u32_type *max , uint32_t time)
{
    if ((value) > (max->value))
    {            
        max->value = value;
        max->time = time;
        return 1;
    }
    if(((value) == 0) && ((max->value) == 0) && (max->time == 0))
    {
        max->time = time;
    }
    return 0;
}


/*********************************************************************************************************
** Function name:       DmdRecordDayUpdate
** Descriptions:        更新日需量最大值
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t  demand_max_adjust_S32_pos(int64_t value, struct db_demand_max_s32_type *max , uint32_t time)
{
    if ((value) > (max->value))
    {            
        max->value = value;
        max->time = time;
        return 1;
    }
    if(((value) == 0) && ((max->value) == 0) && (max->time == 0))
    {
        max->time = time;
    }
    return 0;
}

/*********************************************************************************************************
** Function name:       DmdRecordDayUpdate
** Descriptions:        更新日需量最大值
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t  demand_max_adjust_S32_neg(int64_t value, struct db_demand_max_s32_type *max , uint32_t time)
{
    if(value > 0)
        return 0;

    if ((value) < (max->value))
    {            
        max->value = value;
        max->time = time;
        return 1;
    }
    if(((value) == 0) && ((max->value) == 0) && (max->time == 0))
    {
        max->time = time;
    }
    return 0;
}


/*********************************************************************************************************
** Function name:       DmdRecordDayUpdate
** Descriptions:        更新日需量最大值
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t DmdRecordDayUpdate(struct db_database_measure_base_type *pMeasureBase, struct db_demand_max_type *pDemandMax)
{
    static uint32_t DmdTimeTmp;
    struct db_demand_base_type *StrDemandVal = &(pMeasureBase->db_measure.statistical.demand.realtime);
    DmdTimeTmp = time(NULL);
   /* 判断是否需更新最大需量记录 */
   /*日最大需量*/
    demand_max_adjust_U32(StrDemandVal->Ia,   &pDemandMax->day.current.Ia,   DmdTimeTmp);
    demand_max_adjust_U32(StrDemandVal->Ib,   &pDemandMax->day.current.Ib,   DmdTimeTmp);
    demand_max_adjust_U32(StrDemandVal->Ic,   &pDemandMax->day.current.Ic,   DmdTimeTmp);
    demand_max_adjust_U32(StrDemandVal->IAvg, &pDemandMax->day.current.IAvg, DmdTimeTmp);
    demand_max_adjust_U32(StrDemandVal->In,   &pDemandMax->day.current.In,   DmdTimeTmp);
    demand_max_adjust_S32_pos(StrDemandVal->Pa,   &pDemandMax->day.power.Pa,     DmdTimeTmp);
    demand_max_adjust_S32_pos(StrDemandVal->Pb,   &pDemandMax->day.power.Pb,     DmdTimeTmp);
    demand_max_adjust_S32_pos(StrDemandVal->Pc,   &pDemandMax->day.power.Pc,     DmdTimeTmp);
    demand_max_adjust_S32_pos(StrDemandVal->Ptot, &pDemandMax->day.power.Ptot,   DmdTimeTmp);
    demand_max_adjust_S32_pos(StrDemandVal->Qa,   &pDemandMax->day.power.Qa,     DmdTimeTmp);
    demand_max_adjust_S32_pos(StrDemandVal->Qb,   &pDemandMax->day.power.Qb,     DmdTimeTmp);
    demand_max_adjust_S32_pos(StrDemandVal->Qc,   &pDemandMax->day.power.Qc,     DmdTimeTmp);
    demand_max_adjust_S32_pos(StrDemandVal->Qtot, &pDemandMax->day.power.Qtot,   DmdTimeTmp);
    demand_max_adjust_U32(StrDemandVal->Sa,   &pDemandMax->day.power.Sa,     DmdTimeTmp);
    demand_max_adjust_U32(StrDemandVal->Sb,   &pDemandMax->day.power.Sb,     DmdTimeTmp);
    demand_max_adjust_U32(StrDemandVal->Sc,   &pDemandMax->day.power.Sc,     DmdTimeTmp);
    demand_max_adjust_U32(StrDemandVal->Stot, &pDemandMax->day.power.Stot,   DmdTimeTmp);
    
    // 反向有功需量最值（2025-10-29 添加）
    demand_max_adjust_S32_neg(StrDemandVal->Pa_neg,   &pDemandMax->day.power.Pa_neg,     DmdTimeTmp);
    demand_max_adjust_S32_neg(StrDemandVal->Pb_neg,   &pDemandMax->day.power.Pb_neg,     DmdTimeTmp);
    demand_max_adjust_S32_neg(StrDemandVal->Pc_neg,   &pDemandMax->day.power.Pc_neg,     DmdTimeTmp);
    demand_max_adjust_S32_neg(StrDemandVal->Ptot_neg, &pDemandMax->day.power.Ptot_neg,   DmdTimeTmp);
    
    // 反向无功需量最值（2025-10-29 添加）
    demand_max_adjust_S32_neg(StrDemandVal->Qa_neg,   &pDemandMax->day.power.Qa_neg,     DmdTimeTmp);
    demand_max_adjust_S32_neg(StrDemandVal->Qb_neg,   &pDemandMax->day.power.Qb_neg,     DmdTimeTmp);
    demand_max_adjust_S32_neg(StrDemandVal->Qc_neg,   &pDemandMax->day.power.Qc_neg,     DmdTimeTmp);
    demand_max_adjust_S32_neg(StrDemandVal->Qtot_neg, &pDemandMax->day.power.Qtot_neg,   DmdTimeTmp);
    
    return 0;
}


/*********************************************************************************************************
** Function name:       DmdRecordMonthUpdate
** Descriptions:        更新月需量最大值
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int32_t DmdRecordMonthUpdate(struct db_database_measure_base_type *pMeasureBase, struct db_demand_max_type *pDemandMax)
{
    static uint32_t DmdTimeTmp;
    struct db_demand_base_type *StrDemandVal = &(pMeasureBase->db_measure.statistical.demand.realtime);
    DmdTimeTmp = time(NULL);

    /* 判断是否需更新最大需量记录 */
    /*月最大需量*/
    demand_max_adjust_U32(StrDemandVal->Ia,   &pDemandMax->month.current.Ia,   DmdTimeTmp);
    demand_max_adjust_U32(StrDemandVal->Ib,   &pDemandMax->month.current.Ib,   DmdTimeTmp);
    demand_max_adjust_U32(StrDemandVal->Ic,   &pDemandMax->month.current.Ic,   DmdTimeTmp);
    demand_max_adjust_U32(StrDemandVal->IAvg, &pDemandMax->month.current.IAvg, DmdTimeTmp);
    demand_max_adjust_U32(StrDemandVal->In,   &pDemandMax->month.current.In,   DmdTimeTmp);
    demand_max_adjust_S32_pos(StrDemandVal->Pa,   &pDemandMax->month.power.Pa,     DmdTimeTmp);
    demand_max_adjust_S32_pos(StrDemandVal->Pb,   &pDemandMax->month.power.Pb,     DmdTimeTmp);
    demand_max_adjust_S32_pos(StrDemandVal->Pc,   &pDemandMax->month.power.Pc,     DmdTimeTmp);
    demand_max_adjust_S32_pos(StrDemandVal->Ptot, &pDemandMax->month.power.Ptot,   DmdTimeTmp);
    demand_max_adjust_S32_pos(StrDemandVal->Qa,   &pDemandMax->month.power.Qa,     DmdTimeTmp);
    demand_max_adjust_S32_pos(StrDemandVal->Qb,   &pDemandMax->month.power.Qb,     DmdTimeTmp);
    demand_max_adjust_S32_pos(StrDemandVal->Qc,   &pDemandMax->month.power.Qc,     DmdTimeTmp);
    demand_max_adjust_S32_pos(StrDemandVal->Qtot, &pDemandMax->month.power.Qtot,   DmdTimeTmp);
    demand_max_adjust_U32(StrDemandVal->Sa,   &pDemandMax->month.power.Sa,     DmdTimeTmp);
    demand_max_adjust_U32(StrDemandVal->Sb,   &pDemandMax->month.power.Sb,     DmdTimeTmp);
    demand_max_adjust_U32(StrDemandVal->Sc,   &pDemandMax->month.power.Sc,     DmdTimeTmp);
    demand_max_adjust_U32(StrDemandVal->Stot, &pDemandMax->month.power.Stot,   DmdTimeTmp);
    
    // 反向有功需量最值（2025-10-29 添加）
    demand_max_adjust_S32_neg(StrDemandVal->Pa_neg,   &pDemandMax->month.power.Pa_neg,     DmdTimeTmp);
    demand_max_adjust_S32_neg(StrDemandVal->Pb_neg,   &pDemandMax->month.power.Pb_neg,     DmdTimeTmp);
    demand_max_adjust_S32_neg(StrDemandVal->Pc_neg,   &pDemandMax->month.power.Pc_neg,     DmdTimeTmp);
    demand_max_adjust_S32_neg(StrDemandVal->Ptot_neg, &pDemandMax->month.power.Ptot_neg,   DmdTimeTmp);
    
    // 反向无功需量最值（2025-10-29 添加）
    demand_max_adjust_S32_neg(StrDemandVal->Qa_neg,   &pDemandMax->month.power.Qa_neg,     DmdTimeTmp);
    demand_max_adjust_S32_neg(StrDemandVal->Qb_neg,   &pDemandMax->month.power.Qb_neg,     DmdTimeTmp);
    demand_max_adjust_S32_neg(StrDemandVal->Qc_neg,   &pDemandMax->month.power.Qc_neg,     DmdTimeTmp);
    demand_max_adjust_S32_neg(StrDemandVal->Qtot_neg, &pDemandMax->month.power.Qtot_neg,   DmdTimeTmp);

    return 0;
}



/*********************************************************************************************************
** Function name:       DemandMaxClear
** Descriptions:        清除需量
                        
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void DemandMaxClear(struct db_database_measure_base_type *pMeasureBase)
{
    uint32_t unixtime;
    struct db_demand_max_type *pDemandMax = &(pMeasureBase->db_measure.statistical.demand.max);

    unixtime = time(NULL);
    DemandMaxDayClear(pDemandMax,unixtime);   
    DemandMaxMonthClear(pDemandMax,unixtime);   
}


/*********************************************************************************************************
** Function name:       DmdRecordUpdate
** Descriptions:        更新需量最大值
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int32_t DmdRecordUpdate(struct db_database_measure_base_type *pMeasureBase, struct db_demand_max_type *pDemandMax)
{
    /* 判断是否需更新最大需量记录 */
    DmdRecordDayUpdate(pMeasureBase, pDemandMax);
    DmdRecordMonthUpdate(pMeasureBase, pDemandMax);
    return 0;
}

/*********************************************************************************************************
** Function name:       DemandCal
** Descriptions:        
                
** input parameters:    
                        in  -  some meter's data come from ADC, so we need input the adc data with parameter "in"
                        object - 
                        out -  realtime data struct
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void rms_algorithm_demand_max(struct db_database_measure_base_type *pMeasureBase, struct db_demand_max_type *out, uint32_t pt, uint32_t ct)
{ 
    DmdRecordUpdate_adjust(pMeasureBase);
    DmdRecordUpdate(pMeasureBase,  out);
    return;
}


