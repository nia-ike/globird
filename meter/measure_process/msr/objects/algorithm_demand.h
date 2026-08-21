/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_demand.h
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




#ifndef _ALGORITHM_DEMAND_H_
#define _ALGORITHM_DEMAND_H_

#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include "db_struct.h"

#define  ALGORITHM_DMD_60SEC          60 // 最小需量周期, 60秒累加平均
#define  ALGORITHM_DMD_WINDOW_MAX     30 // 最大需量周期, 目前建议最大只支持30分钟


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
 */

struct dmd_item_sum_t
{
    uint64_t   IncIa;    
    uint64_t   IncIb;    
    uint64_t   IncIc;    
    uint64_t   IncIAvg;    
    uint64_t   IncIn;    
    int64_t    IncPa;       // 正向有功需量
    int64_t    IncPb;    
    int64_t    IncPc;    
    int64_t    IncPtot;  
    int64_t    IncPa_neg;   // 反向有功需量 
    int64_t    IncPb_neg;    
    int64_t    IncPc_neg;    
    int64_t    IncPtot_neg;  
    int64_t    IncQa;       // 正向无功需量
    int64_t    IncQb;    
    int64_t    IncQc;    
    int64_t    IncQtot;  
    int64_t    IncQa_neg;   // 反向无功需量 
    int64_t    IncQb_neg;    
    int64_t    IncQc_neg;    
    int64_t    IncQtot_neg; 
    uint64_t   IncSa;    
    uint64_t   IncSb;    
    uint64_t   IncSc;    
    uint64_t   IncStot;  
    uint16_t   DmdIncBufCnt;     
};


struct dmd_cfg_t
{
    uint16_t u16gDmdWindow;          // 需量方式
    uint16_t u16gDmdMode;            // 需量周期
    uint16_t u16gDmdSlipWindow;      // 滑差时间
};



int rms_algorithm_dmd_process(struct db_database_measure_base_type  *pMeasureBase);
int rms_algorithm_dmd_init(struct db_database_measure_base_type  *pMeasureBase);
int rms_algoithm_dmd_clear(struct db_database_measure_base_type  *pMeasureBase);


#endif
