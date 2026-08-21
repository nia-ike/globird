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

#ifndef __DB_MEASURE_POWERQUALITY_H__
#define __DB_MEASURE_POWERQUALITY_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>


#define DB_MSR_PHASE_CHNL           (3 + 1)         /* 通道：  “A B C 合” = 3 + 1 个 */
#define DB_MSR_HARMONIC_0_63         (64)            /* 直流分量，基波，2 - 63次谐波 */
#define DB_MSR_THD_2_63             (62)            /* 2 - 63次谐波 */
#define DB_MSR_PHASE                 (3)            /* 2 - 63次谐波 */



struct UnbalanceValueType
{
    uint32_t  S0;       //零序
    uint32_t  S1;       // 负序
    uint32_t  S2;       // 正序
};

struct UnbalanceFactorType
{
    uint16_t  I0;       // 零序不平衡度
    uint16_t  I1;       //负序不平衡度
};

struct unbalance_type{
    struct UnbalanceValueType UnbalanceValue;//零序负序正序
    struct UnbalanceFactorType UnbalanceFactor;   // 不平衡率
};

struct db_powerquality_unbalance_type{
    struct unbalance_type Unbalance_V; // 相电压不平衡
    struct unbalance_type Unbalance_I;  // 电流不平衡
};


/* 总谐波畸变率数据结构 */
struct db_powerquality_harmonic_THD_type{
    uint16_t THDVa;   // A相电压THD
    uint16_t THDVb;   // B相电压THD
    uint16_t THDVc;   // C相电压THD
    uint16_t THDIa;   // A相电流THD
    uint16_t THDIb;   // B相电流THD
    uint16_t THDIc;   // C相电流THD

    uint16_t TEHDVa;  // A相电压TEHD
    uint16_t TEHDVb;  // B相电压TEHD
    uint16_t TEHDVc;  // C相电压TEHD
    uint16_t TEHDIa;  // A相电流TEHD
    uint16_t TEHDIb;  // B相电流TEHD
    uint16_t TEHDIc;  // C相电流TEHD

    uint16_t TOHDVa;  // A相电压TOHD
    uint16_t TOHDVb;  // B相电压TOHD
    uint16_t TOHDVc;  // C相电压TOHD
    uint16_t TOHDIa;  // A相电流TOHD
    uint16_t TOHDIb;  // B相电流TOHD
    uint16_t TOHDIc;  // C相电流TOHD
};
/* 谐波含有率数据结构（2~63次） */
struct db_powerquality_harmonic_HR_type
{
    uint16_t HRVa[DB_MSR_HARMONIC_0_63];   // A相谐波电压含有率
    uint16_t HRVb[DB_MSR_HARMONIC_0_63];   // C相谐波电压含有率
    uint16_t HRVc[DB_MSR_HARMONIC_0_63];   // B相谐波电流含有率
    uint16_t HRIa[DB_MSR_HARMONIC_0_63];   // A相谐波电压含有率
    uint16_t HRIb[DB_MSR_HARMONIC_0_63];   // B相谐波电流含有率
    uint16_t HRIc[DB_MSR_HARMONIC_0_63];   // C相谐波电流含有率
};
/* 谐波有效值数据结构（2~63次） */
struct db_powerquality_harmonic_RMS_type
{
    uint32_t VaValue[DB_MSR_HARMONIC_0_63];   // A相电压谐波有效值
    uint32_t VbValue[DB_MSR_HARMONIC_0_63];   // B相电压谐波有效值
    uint32_t VcValue[DB_MSR_HARMONIC_0_63];   // C相电压谐波有效值
    uint32_t IaValue[DB_MSR_HARMONIC_0_63];   // A相电流谐波有效值
    uint32_t IbValue[DB_MSR_HARMONIC_0_63];   // B相电流谐波有效值
    uint32_t IcValue[DB_MSR_HARMONIC_0_63];   // C相电流谐波有效值
};
/*谐波角度*/
struct db_powerquality_harmonic_angle_type
{
    int16_t VaAngle;
    int16_t IaAngle;
    int16_t VbAngle;
    int16_t IbAngle;
    int16_t VcAngle;
    int16_t IcAngle;
};
/* 谐波类数据结构 */
struct db_powerquality_harmonic_type
{
    struct db_powerquality_harmonic_THD_type      THD;       // 总谐波畸变率
    struct db_powerquality_harmonic_HR_type       HR;        // 谐波含有率
    struct db_powerquality_harmonic_RMS_type      RMS;       // 谐波有效值
    struct db_powerquality_harmonic_angle_type    Angle;          //谐波角度
    uint32_t W_Flag;//写标志
};


/*************************************************************************************************************/


/*
    计量类电能质量基本数据结构
*/
struct db_powerquality_basic_type
{
    uint32_t PosVol[DB_MSR_PHASE];           // 正序电压
    uint32_t NegVol[DB_MSR_PHASE];           // 负序电压
    uint32_t ZeroVol[DB_MSR_PHASE];          // 零序电压 
    uint32_t PosCur[DB_MSR_PHASE];           // 正序电流
    uint32_t NegCur[DB_MSR_PHASE];           // 负序电流
    uint32_t ZeroCur[DB_MSR_PHASE];          // 零序电流
    uint16_t NegUnbVol[DB_MSR_PHASE];        // 负序电压不平衡率
    uint16_t ZeroUnbVol[DB_MSR_PHASE];       // 零序电压不平衡率
    uint16_t NegUnbCur[DB_MSR_PHASE];        // 负序电流不平衡率
    uint16_t ZeroUnbCur[DB_MSR_PHASE];       // 零序电流不平衡率

    struct db_powerquality_unbalance_type    unbalance;
    int16_t  loadrate;       // 总有功功率负荷率

    uint32_t UpOffsetVa;            // A相电压上偏差
    uint32_t UpOffsetVb;
    uint32_t UpOffsetVc;
    uint32_t DownOffsetVa;          // A相电压下偏差
    uint32_t DownOffsetVb;
    uint32_t DownOffsetVc;
    
    uint32_t UpOffsetVab;           // A相电压上偏差
    uint32_t UpOffsetVbc;
    uint32_t UpOffsetVca;
    uint32_t DownOffsetVab;         // A相电压下偏差
    uint32_t DownOffsetVbc;
    uint32_t DownOffsetVca;

    int16_t OffsetVa;             // A相电压偏差率
    int16_t OffsetVb;
    int16_t OffsetVc;
    int16_t OffsetVab;            // 线电压偏差率
    int16_t OffsetVbc;
    int16_t OffsetVca;

    int16_t  OffsetHz;              // 频率偏差率

    uint16_t WaveMaxFactorUa;       // A相电压波峰因子
    uint16_t WaveMaxFactorUb;
    uint16_t WaveMaxFactorUc;
    
    uint16_t WaveMaxFactorIa;       // A相电流波峰因子
    uint16_t WaveMaxFactorIb;
    uint16_t WaveMaxFactorIc;

    uint16_t TDDIa;                 // A相电流TDD
    uint16_t TDDIb;
    uint16_t TDDIc;
    
    int32_t  KFactorIa;              // A相电流K因子
    int32_t  KFactorIb;
    int32_t  KFactorIc;

    uint32_t ShortTermFlickerUa;    // A相电压短时闪变
    uint32_t ShortTermFlickerUb;
    uint32_t ShortTermFlickerUc;
    uint32_t LongTermFlickerUa;     // A相电压长时闪变
    uint32_t LongTermFlickerUb;
    uint32_t LongTermFlickerUc;

    uint32_t THFFUa;                // A相电压谐波波形因数
    uint32_t THFFUb;
    uint32_t THFFUc;
    uint32_t THFFIa;                // A相电流谐波波形因数
    uint32_t THFFIb;
    uint32_t THFFIc;
};
/*************************************************************************************************************/



//    计量类电能质量数据结构
struct db_measure_powerquality_type
{
    struct db_powerquality_basic_type           basic;
    struct db_powerquality_harmonic_type        harmonic;
};


#endif
