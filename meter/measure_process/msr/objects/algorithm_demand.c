/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_demand.c
*  文件标识：
*  描    述：不平衡度
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
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include "algorithm_demand.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>

/**
 * 需量计算机制说明：
 * 
 * 数据流向图：
 * ┌─────────────┐    ┌─────────────┐ 
 * │   1秒数据   │───▶│ 900秒累加平均 
 * │ (原始测量值) │    │ (可配     )  
 * └─────────────┘    └─────────────┘ 
 *                           │         
 * 

 2025 版 JJF 1245
需量周期为 1,5,10,15,30,分钟
 */

 #define sum(x, y)           ((x) + (y))
 #define sum_pos(x, y)       ((x) > 0 ? (x) + (y) : (x + 0))
 #define sum_neg(x, y)       ((x) < 0 ? (x) + (y) : (x + 0))
 
 #define average(x, y)       ((x) / (y))
 
 /* 变量定义 */
 
 
 /* 60秒累加数据 */
 static struct dmd_item_sum_t dmd_item_sum;
 /* 配置 */
 static struct dmd_cfg_t DmdCfg;
 
 
 
 /*********************************************************************************************************
 ** Function name:       rms_algorithm_dmd_flush
 ** Descriptions:        清0
 ** input parameters:    NONE
 ** output parameters:   NONE
 ** Returned value:      NONE
 *********************************************************************************************************/
 static __inline__ void rms_algorithm_dmd_buffer_flush(struct dmd_item_sum_t *pdmd_item_sum)
 {
     memset(pdmd_item_sum, 0, sizeof(struct dmd_item_sum_t));
 }
 
 
 /*********************************************************************************************************
 ** Function name:       rms_algorithm_dmd_clear
 ** Descriptions:        清0
 ** input parameters:    NONE
 ** output parameters:   NONE
 ** Returned value:      NONE
 *********************************************************************************************************/
 int rms_algoithm_dmd_clear(struct db_database_measure_base_type  *pMeasureBase)
 {
     struct db_demand_base_type *pDemand = &(pMeasureBase->db_measure.statistical.demand.realtime);
 
     memset(pDemand, 0, sizeof(struct db_demand_base_type));
     
     return 0;
 }
 
 
 /*********************************************************************************************************
 ** Function name:       rms_algorithm_dmd_1sec
 ** Descriptions:        
                         每秒累加数据
 ** input parameters:    
                         pMeasureBase - 测量基础数据
                         pdmd_item_sum - 60秒累加数据
                         out - 1分钟累加数据
 ** output parameters:   NONE
 ** Returned value:      NONE
 *********************************************************************************************************/
 int rms_algorithm_dmd_process(struct db_database_measure_base_type  *pMeasureBase)
 {
     
     /* 
        从周期起始时刻00秒开始累加  （周期为 1,5,10,15,30,分钟）
        每秒统计1次
     
     */
     struct tm tm_now;
     static int8_t last_min = -1;
     static time_t last_time = 0;
 
     time_t now = time(NULL);
     localtime_r(&now, &tm_now);
 
     // 计算周期到
     if((tm_now.tm_min % DmdCfg.u16gDmdWindow == 0) && (tm_now.tm_sec == 0) && (last_min != tm_now.tm_min))
     {
         if(dmd_item_sum.DmdIncBufCnt > 0)
         {
             pMeasureBase->db_measure.statistical.demand.realtime.Ia = dmd_item_sum.IncIa / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Ib = dmd_item_sum.IncIb / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Ic = dmd_item_sum.IncIc / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.IAvg = dmd_item_sum.IncIAvg / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.In = dmd_item_sum.IncIn / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Pa = dmd_item_sum.IncPa / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Pb = dmd_item_sum.IncPb / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Pc = dmd_item_sum.IncPc / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Ptot = dmd_item_sum.IncPtot / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Qa = dmd_item_sum.IncQa / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Qb = dmd_item_sum.IncQb / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Qc = dmd_item_sum.IncQc / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Qtot = dmd_item_sum.IncQtot / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Sa = dmd_item_sum.IncSa / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Sb = dmd_item_sum.IncSb / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Sc = dmd_item_sum.IncSc / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Stot = dmd_item_sum.IncStot / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Pa_neg = dmd_item_sum.IncPa_neg / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Pb_neg = dmd_item_sum.IncPb_neg / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Pc_neg = dmd_item_sum.IncPc_neg / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Ptot_neg = dmd_item_sum.IncPtot_neg / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Qa_neg = dmd_item_sum.IncQa_neg / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Qb_neg = dmd_item_sum.IncQb_neg / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Qc_neg = dmd_item_sum.IncQc_neg / dmd_item_sum.DmdIncBufCnt;
             pMeasureBase->db_measure.statistical.demand.realtime.Qtot_neg = dmd_item_sum.IncQtot_neg / dmd_item_sum.DmdIncBufCnt;
 
             /* 清0 */
             rms_algorithm_dmd_buffer_flush(&dmd_item_sum);
         }
         
     }
 
     //每秒累加数据
     if(last_time != now)
     {
         dmd_item_sum.IncIa   = sum(pMeasureBase->db_measure.numeric.fullwave.primary.Ia, dmd_item_sum.IncIa);
         dmd_item_sum.IncIb   = sum(pMeasureBase->db_measure.numeric.fullwave.primary.Ib, dmd_item_sum.IncIb);
         dmd_item_sum.IncIc   = sum(pMeasureBase->db_measure.numeric.fullwave.primary.Ic, dmd_item_sum.IncIc);
         dmd_item_sum.IncIAvg = sum(pMeasureBase->db_measure.numeric.fullwave.primary.IAvg, dmd_item_sum.IncIAvg);
         dmd_item_sum.IncIn   = sum(pMeasureBase->db_measure.numeric.fullwave.primary.In, dmd_item_sum.IncIn);
         dmd_item_sum.IncPa   = sum_pos(pMeasureBase->db_measure.numeric.fullwave.primary.Pa, dmd_item_sum.IncPa);
         dmd_item_sum.IncPb   = sum_pos(pMeasureBase->db_measure.numeric.fullwave.primary.Pb, dmd_item_sum.IncPb);
         dmd_item_sum.IncPc   = sum_pos(pMeasureBase->db_measure.numeric.fullwave.primary.Pc, dmd_item_sum.IncPc);
         dmd_item_sum.IncPtot = sum_pos(pMeasureBase->db_measure.numeric.fullwave.primary.Ptot, dmd_item_sum.IncPtot);
         dmd_item_sum.IncQa   = sum_pos(pMeasureBase->db_measure.numeric.fullwave.primary.Qa, dmd_item_sum.IncQa);
         dmd_item_sum.IncQb   = sum_pos(pMeasureBase->db_measure.numeric.fullwave.primary.Qb, dmd_item_sum.IncQb);
         dmd_item_sum.IncQc   = sum_pos(pMeasureBase->db_measure.numeric.fullwave.primary.Qc, dmd_item_sum.IncQc);
         dmd_item_sum.IncQtot = sum_pos(pMeasureBase->db_measure.numeric.fullwave.primary.Qtot, dmd_item_sum.IncQtot);
         dmd_item_sum.IncSa   = sum_pos(pMeasureBase->db_measure.numeric.fullwave.primary.Sa, dmd_item_sum.IncSa);
         dmd_item_sum.IncSb   = sum_pos(pMeasureBase->db_measure.numeric.fullwave.primary.Sb, dmd_item_sum.IncSb);
         dmd_item_sum.IncSc   = sum_pos(pMeasureBase->db_measure.numeric.fullwave.primary.Sc, dmd_item_sum.IncSc);
         dmd_item_sum.IncStot = sum_pos(pMeasureBase->db_measure.numeric.fullwave.primary.Stot, dmd_item_sum.IncStot);
 
         dmd_item_sum.IncPa_neg = sum_neg(pMeasureBase->db_measure.numeric.fullwave.primary.Pa, dmd_item_sum.IncPa_neg);
         dmd_item_sum.IncPb_neg = sum_neg(pMeasureBase->db_measure.numeric.fullwave.primary.Pb, dmd_item_sum.IncPb_neg);
         dmd_item_sum.IncPc_neg = sum_neg(pMeasureBase->db_measure.numeric.fullwave.primary.Pc, dmd_item_sum.IncPc_neg);
         dmd_item_sum.IncPtot_neg = sum_neg(pMeasureBase->db_measure.numeric.fullwave.primary.Ptot, dmd_item_sum.IncPtot_neg);
         dmd_item_sum.IncQa_neg = sum_neg(pMeasureBase->db_measure.numeric.fullwave.primary.Qa, dmd_item_sum.IncQa_neg);
         dmd_item_sum.IncQb_neg = sum_neg(pMeasureBase->db_measure.numeric.fullwave.primary.Qb, dmd_item_sum.IncQb_neg);
         dmd_item_sum.IncQc_neg = sum_neg(pMeasureBase->db_measure.numeric.fullwave.primary.Qc, dmd_item_sum.IncQc_neg);
         dmd_item_sum.IncQtot_neg = sum_neg(pMeasureBase->db_measure.numeric.fullwave.primary.Qtot, dmd_item_sum.IncQtot_neg);
 
         dmd_item_sum.DmdIncBufCnt++;
     }
 
     last_time = now;
     last_min = tm_now.tm_min;
 
     return 0;
 }
 
 
 /*********************************************************************************************************
 ** Function name:       rms_algorithm_dmd_init
 ** Descriptions:        初始化
 ** input parameters:    pMeasureBase - 测量基础数据
 ** output parameters:   
 ** Returned value:      NONE
 *********************************************************************************************************/
 int rms_algorithm_dmd_init(struct db_database_measure_base_type  *pMeasureBase)
 {
     
     /* 初始化数据 */
     rms_algorithm_dmd_buffer_flush(&dmd_item_sum); 
 
     /* 初始化配置 */
     DmdCfg.u16gDmdWindow     = pMeasureBase->db_cfg.user.demand.window.value;
     DmdCfg.u16gDmdMode       = pMeasureBase->db_cfg.user.demand.mode.value;
     DmdCfg.u16gDmdSlipWindow = pMeasureBase->db_cfg.user.demand.slip_window.value;
 
     if(DmdCfg.u16gDmdWindow > ALGORITHM_DMD_WINDOW_MAX)
     {
         DmdCfg.u16gDmdWindow = ALGORITHM_DMD_WINDOW_MAX;
     }
 
     return 0;
 }
 