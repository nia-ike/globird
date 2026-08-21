/*
* Copyright (c) 2015,珠海科技股份有限公司
* All rights reserved.
* 
* 文件名称：db_realtime.h
* 文件标识：
* 描    述：
* 
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：
* 完成日期：20190909
* 运行环境：
操作系统: rt-thread
硬件平台：STM32F030
固件库  ：V3.5
* 修改记录 
*  作者       时间         版本      修改描述
*  chenzy   2014.11.26       V1.00      创建
*/

#ifndef __DB_MEASURE_STATISTICAL_H__
#define __DB_MEASURE_STATISTICAL_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>



/****************************需量数据结构*********************************************/

/* 实时需量数据结构 */
struct db_demand_base_type
{
    uint32_t    Ia;
    uint32_t    Ib;
    uint32_t    Ic;
    uint32_t    IAvg;                  // 平均电流
    uint32_t    In;                    // 中性线电流

    int32_t     Pa;
    int32_t     Pb;
    int32_t     Pc;
    int32_t     Ptot;
       
    int32_t     Qa;
    int32_t     Qb;
    int32_t     Qc;
    int32_t     Qtot;

    uint32_t    Sa;
    uint32_t    Sb;
    uint32_t    Sc;
    uint32_t    Stot;

// 20240413 add. modify by chenzy.  neg P Q demand
    int32_t     Pa_neg;
    int32_t     Pb_neg;
    int32_t     Pc_neg;
    int32_t     Ptot_neg;

    int32_t     Qa_neg;
    int32_t     Qb_neg;
    int32_t     Qc_neg;
    int32_t     Qtot_neg;
};

/* 实时视在需量数据结构 */
struct db_realtime_apparent_demand_type
{
    uint64_t    Sa;
    uint64_t    Sb;
    uint64_t    Sc;
    uint64_t    Stot;
};

struct db_demand_max_u32_type{
    uint32_t value;    // 数值
    uint32_t time;     // Unix时间
};

struct db_demand_max_s32_type{
    int32_t value;    // 数值
    uint32_t time;     // Unix时间
};

struct db_demand_max_s64_type{
    int64_t value;    // 数值
    uint32_t time;     // Unix时间
};

struct db_demand_max_u64_type{
    uint64_t value;    // 数值
    uint32_t time;     // Unix时间
};

/* 需量最值数据结构 */
struct db_demand_current_type
{
    struct db_demand_max_u32_type   Ia;
    struct db_demand_max_u32_type   Ib;
    struct db_demand_max_u32_type   Ic;
    struct db_demand_max_u32_type   IAvg;
    struct db_demand_max_u32_type   In;
};

struct db_demand_power_type{
    struct db_demand_max_s32_type   Pa;
    struct db_demand_max_s32_type   Pb;
    struct db_demand_max_s32_type   Pc;
    struct db_demand_max_s32_type   Ptot;

    struct db_demand_max_s32_type   Qa;
    struct db_demand_max_s32_type   Qb;
    struct db_demand_max_s32_type   Qc;
    struct db_demand_max_s32_type   Qtot;

    struct db_demand_max_u32_type   Sa;
    struct db_demand_max_u32_type   Sb;
    struct db_demand_max_u32_type   Sc;
    struct db_demand_max_u32_type   Stot;

    // 反向有功需量最值（2025-10-29 添加）
    struct db_demand_max_s32_type   Pa_neg;
    struct db_demand_max_s32_type   Pb_neg;
    struct db_demand_max_s32_type   Pc_neg;
    struct db_demand_max_s32_type   Ptot_neg;

    // 反向无功需量最值（2025-10-29 添加）
    struct db_demand_max_s32_type   Qa_neg;
    struct db_demand_max_s32_type   Qb_neg;
    struct db_demand_max_s32_type   Qc_neg;
    struct db_demand_max_s32_type   Qtot_neg;
};

/* 需量最值数据结构 */
struct db_demand_max_base_type
{
    uint32_t   time;
    struct db_demand_current_type  current;
    struct db_demand_power_type    power;
};
struct db_demand_max_type
{
    struct db_demand_max_base_type day;//日最大需量
    struct db_demand_max_base_type month;//月最大需量
    uint32_t W_Flag;//写标志
};


/*需量数据结构*/
struct db_statistical_demand_type
{
    struct db_demand_base_type      realtime;   //实时需量
    struct db_demand_max_type       max;        //最值需量
};

/**************************************************************************************/


/****************************极值数据结构********************************************/
struct db_group_s32_type
{
    int32_t     value;
    uint32_t    time;
};

struct db_group_u32_type
{
    uint32_t    value;
    uint32_t    time;
};

struct db_group_s64_type
{
    int64_t     value;
    uint32_t    time;
};

struct db_group_u64_type
{
    uint64_t    value;
    uint32_t    time;
};

struct db_extremum_group_s32_type
{
    struct db_group_s32_type       max;
    struct db_group_s32_type       min;
};
struct db_extremum_group_u32_type
{
    struct db_group_u32_type       max;
    struct db_group_u32_type       min;
};

struct db_extremum_group_s64_type
{
    struct db_group_s64_type       max;
    struct db_group_s64_type       min;
};
struct db_extremum_group_u64_type
{
    struct db_group_u64_type       max;
    struct db_group_u64_type       min;
};

struct db_extremum_basic_type
{
    uint32_t  time;
    struct db_extremum_group_u32_type   Va;                 // A相电压最值
    struct db_extremum_group_u32_type   Vb;
    struct db_extremum_group_u32_type   Vc;
   
    struct db_extremum_group_u32_type   Vab;                // AB线电压最值
    struct db_extremum_group_u32_type   Vbc;
    struct db_extremum_group_u32_type   Vca;

    struct db_extremum_group_u32_type   Vn;                 // 零地电压最值

    struct db_extremum_group_u32_type   Ia;                 // A相电流最值
    struct db_extremum_group_u32_type   Ib;
    struct db_extremum_group_u32_type   Ic;

    struct db_extremum_group_u32_type   In;                 // 中性线电流最值

    struct db_extremum_group_s32_type   Pa;                 // A相有功功率最值
    struct db_extremum_group_s32_type   Pb;
    struct db_extremum_group_s32_type   Pc;
    struct db_extremum_group_s32_type   Ptot;

    struct db_extremum_group_s32_type   Qa;                 // A相无功功率最值
    struct db_extremum_group_s32_type   Qb;
    struct db_extremum_group_s32_type   Qc;
    struct db_extremum_group_s32_type   Qtot;

    struct db_extremum_group_u32_type   Sa;                 // A相视在功率最值
    struct db_extremum_group_u32_type   Sb;
    struct db_extremum_group_u32_type   Sc;
    struct db_extremum_group_u32_type   Stot;

    struct db_extremum_group_s32_type   PFa;                // A相功率因数最值
    struct db_extremum_group_s32_type   PFb;
    struct db_extremum_group_s32_type   PFc;
    struct db_extremum_group_s32_type   PFtot;
    
    struct db_extremum_group_u32_type   Hz;                 // 频率最值
};

/*
    计量类统计型极值数据结构
*/
struct db_statistical_extremum_type
{
    struct db_extremum_basic_type    day;
    struct db_extremum_basic_type  month;
    struct db_extremum_basic_type   year;
};
/**************************************************************************************/


/******************************负载运行参数数据结构*******************************************/

/*
    计量类统计型负载运行参数数据结构
*/
struct db_statistical_loadrun_type
{
    uint32_t    lrh;        //负载运行时间load run hour;
    uint32_t    loh;        //负载上电时间load on hour; 
    uint32_t    lrc;        //负载运行次数load run accumulation;           
    uint32_t    doh;        //仪表运行时间，出厂后          
    uint32_t    doh_uptime;   //仪表运行时间，最后一次上电          
};
/**************************************************************************************/


/*
    计量类统计型数据结构
*/
struct db_measure_statistical_type
{
    struct db_statistical_demand_type       demand;
    struct db_statistical_extremum_type     extremum;
    struct db_statistical_loadrun_type      loadrun;
};


#endif
