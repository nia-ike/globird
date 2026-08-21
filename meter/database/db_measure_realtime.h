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

#ifndef __DB_MEASURE_REALTIME_H__
#define __DB_MEASURE_REALTIME_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>


//基本测量实时参数
struct db_basic_realtime_type
{    
    uint32_t    Va;           // A相相电压          
    uint32_t    Vb;           // B相相电压    
    uint32_t    Vc;           // C相相电压    
    uint32_t    VlnAvg;       // 平均相电压    
    uint32_t    Vab;          // AB线电压    
    uint32_t    Vbc;          // BC线电压    
    uint32_t    Vca;          // CA线电压    
    uint32_t    VllAvg;       // 平均线电压    
    uint32_t    Ia;           // A相相电流    
    uint32_t    Ib;           // B相相电流    
    uint32_t    Ic;           // C相相电流    
    uint32_t    IAvg;         // 平均电流   
    int32_t     Pa;           // A相有功功率   
    int32_t     Pb;           // B相有功功率    
    int32_t     Pc;           // C相有功功率    
    int32_t     Ptot;         // 总有功功率    
    int32_t     Qa;           // A相无功功率    
    int32_t     Qb;           // B相无功功率    
    int32_t     Qc;           // C相无功功率    
    int32_t     Qtot;         // 总无功功率    
    uint32_t    Sa;           // A相视在功率    
    uint32_t    Sb;           // B相视在功率    
    uint32_t    Sc;           // C相视在功率    
    uint32_t    Stot;         // 总视在功率 
    int16_t     PFa;          // A相功率因数    
    int16_t     PFb;          // B相功率因数    
    int16_t     PFc;          // C相功率因数    
    int16_t     PFtot;        // 总功率因数  
    uint32_t    Hz_hp;        // 频率高精度   
    uint16_t    Hz;           // 频率     
    int16_t     J_Va;         // A相电压相位角    
    int16_t     J_Vb;         // B相电压相位角    
    int16_t     J_Vc;         // C相电压相位角    
    int16_t     J_Ia;         // A相电流相位角    
    int16_t     J_Ib;         // B相电流相位角    
    int16_t     J_Ic;         // C相电流相位角        
    int32_t     Pab;           // AB线有功功率   
    int32_t     Pca;           // CA线有功功率    
    int32_t     Qab;           // AB线无功功率   
    int32_t     Qca;           // CA线无功功率    
    uint32_t    Sab;           // AB线视在功率   
    uint32_t    Sca;           // CA线视在功率  
    uint32_t    Vn;            // 零地电压    
    uint32_t    In;            // 中性线电流
    int16_t     P_IndCap;     //0-3位分别代表A,B,C,合相容感性,值0:感性1:容性    
    int16_t     J_PF;               // 功率因数角度  三相三相模式下
    uint16_t    DemoFlag;        

};

//二次侧实时值
struct db_realtime_secondary_type
{    
    uint16_t    Va;       // A相相电压    
    uint16_t    Vb;       // B相相电压    
    uint16_t    Vc;       // C相相电压    
    uint16_t    VlnAvg;   // 平均相电压    
    uint16_t    Vab;      // AB线电压    
    uint16_t    Vbc;      // BC线电压    
    uint16_t    Vca;      // CA线电压    
    uint16_t    VllAvg;   // 平均线电压    
    uint16_t    Ia;       // A相相电流    
    uint16_t    Ib;       // B相相电流    
    uint16_t    Ic;       // C相相电流    
    uint16_t    IAvg;     // 平均电流   
    int32_t     Pa;       // A相有功功率   
    int32_t     Pb;       // B相有功功率    
    int32_t     Pc;       // C相有功功率    
    int32_t     Ptot;     // 总有功功率    
    int32_t     Qa;       // A相无功功率    
    int32_t     Qb;       // B相无功功率    
    int32_t     Qc;       // C相无功功率    
    int32_t     Qtot;     // 总无功功率    
    uint32_t    Sa;       // A相视在功率    
    uint32_t    Sb;       // B相视在功率    
    uint32_t    Sc;       // C相视在功率    
    uint32_t    Stot;     // 总视在功率 
    int32_t     Pab;       // AB线有功功率   
    int32_t     Pca;       // CA线有功功率    
    int32_t     Qab;       // AB线无功功率   
    int32_t     Qca;       // CA线无功功率    
    uint32_t    Sab;       // AB线视在功率   
    uint32_t    Sca;       // CA线视在功率    
    uint32_t    Vn;        // 零地电压    
    uint32_t    In;        // 中性线电流
    int16_t     loadrate;       // 总有功功率负荷率
 
    uint32_t    W_Flag;   //写标志
};




/*
    计量类数值型基本测量数据结构体
*/
struct db_numeric_fullwave_type
{
    struct db_basic_realtime_type           primary;
    struct db_realtime_secondary_type       secondary;
};

/*
    计量类数值型基波测量数据结构体
*/
struct db_numeric_fundwave_type
{
    struct db_basic_realtime_type        primary;
    struct db_realtime_secondary_type    secondary;
};

/*
    计量类数值型基波测量数据结构体
*/
struct db_numeric_halfwave_type
{
    struct db_basic_realtime_type        primary;
    struct db_realtime_secondary_type    secondary;
};

/*
    计量类数值型数据结构体
*/
struct db_measure_numeric_type
{
    struct db_numeric_fullwave_type     fullwave;
    struct db_numeric_fundwave_type     fundwave;
    struct db_numeric_halfwave_type     halfwave;

};





#endif
