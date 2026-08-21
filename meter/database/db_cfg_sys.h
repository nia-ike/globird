/*
* Copyright (c) 2013,珠海派诺电子有限公司
* All rights reserved.
* 
* 文件名称：db_cfg.h
* 文件标识：
* 描    述：
* 
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：
* 完成日期：20131108
*
* 修改记录 
*  作者       时间         版本      修改描述
*  chenzy   2013.11.08   V1.00      创建
*/

#ifndef __DB_CFG_SYS_H__
#define __DB_CFG_SYS_H__

#include "db_cfg_info.h"
#include "db_cfg_type.h"

/* 
    USE for map table define
 */
#define DB_toString(__CLASS__)                          #__CLASS__


/************************************************************************************/
/*
    校表参数
*/

struct db_cfg_sys_adjust_ad_para
{    
    struct db_cfg_CellProperties_u16 Adjust_Gv;        //电压ADC通道增益（芯片设置）
    struct db_cfg_CellProperties_u16 Adjust_Gi;        //电流ADC通道增益（芯片设置）
    struct db_cfg_CellProperties_u16 Krms_V;           //电压精度系数
    struct db_cfg_CellProperties_u16 Krms_I;           //电流精度系数
    struct db_cfg_CellProperties_u16 Krms_P;           //功率精度系数
    struct db_cfg_CellProperties_u16 Krms_Pf;          //功因精度系数
    struct db_cfg_CellProperties_u16 Krms_Hz;          //频率精度系数
    struct db_cfg_CellProperties_u16 Krms_E;           //电能精度系数
};

struct db_cfg_sys_adjust_ad_value_para
{    
    struct db_cfg_CellProperties_u16 Adjust_V;         //校正电压值
    struct db_cfg_CellProperties_u16 Adjust_I;         //校正电流值
    struct db_cfg_CellProperties_u16 Adjust_P;         //校正有功功率值
    struct db_cfg_CellProperties_u16 Adjust_A;         //校正角度，预留
    struct db_cfg_CellProperties_u32 Adjust_Va;
    struct db_cfg_CellProperties_u32 Adjust_Vb;
    struct db_cfg_CellProperties_u32 Adjust_Vc;
    struct db_cfg_CellProperties_u32 Adjust_Ia;
    struct db_cfg_CellProperties_u32 Adjust_Ib;
    struct db_cfg_CellProperties_u32 Adjust_Ic;
    struct db_cfg_CellProperties_u32 Adjust_Pa;
    struct db_cfg_CellProperties_u32 Adjust_Pb;
    struct db_cfg_CellProperties_u32 Adjust_Pc;

};

struct db_cfg_sys_adjust_prth_para
{
    struct db_cfg_CellProperties_u16 Prth1L;         //相位补偿区域1L
    struct db_cfg_CellProperties_u16 Prth1H;         //相位补偿区域1H
    struct db_cfg_CellProperties_u16 Prth2L;         //相位补偿区域2L
    struct db_cfg_CellProperties_u16 Prth2H;         //相位补偿区域2H
    struct db_cfg_CellProperties_u16 PhsxR1;         //相位补偿区域1
    struct db_cfg_CellProperties_u16 PhsxR2;         //相位补偿区域2
    struct db_cfg_CellProperties_u16 PhsxR3;         //相位补偿区域3
};

struct db_cfg_sys_adjust_I0_para
{    
    struct db_cfg_CellProperties_u16 RatedI0;   //额定电流
    struct db_cfg_CellProperties_u16 Adjust_Gi0;        //电流ADC通道增益（芯片设置）
    struct db_cfg_CellProperties_u16 TypeI0;   //漏电电流工作类型  默认是0为漏电电流，1为第四路电流测量
    struct db_cfg_CellProperties_u16 Adjust_I0;         //校正电流值
    struct db_cfg_CellProperties_u32 Adjust_I0_value;         //校正电流值
};

/*
    校表参数
*/
struct db_cfg_sys_adjust_value_type
{    
    struct db_cfg_CellProperties_u16 Adjust_Va;
    struct db_cfg_CellProperties_u16 Adjust_Vb;
    struct db_cfg_CellProperties_u16 Adjust_Vc;
    struct db_cfg_CellProperties_u16 Adjust_Ia;
    struct db_cfg_CellProperties_u16 Adjust_Ib;
    struct db_cfg_CellProperties_u16 Adjust_Ic;
    struct db_cfg_CellProperties_u16 Adjust_Pfa;
    struct db_cfg_CellProperties_u16 Adjust_Pfb;
    struct db_cfg_CellProperties_u16 Adjust_Pfc;
};

/*
    校表参数属性表
*/
struct db_cfg_sys_adjust_para
{
    struct db_cfg_CellProperties_u16 HfConst1; //高频脉冲
    struct db_cfg_CellProperties_u16 HfConst2;
    struct db_cfg_CellProperties_u16 IStart_PS; //有功视在启动电流阈值
    struct db_cfg_CellProperties_u16 IStart_Q;  //无功启动电流阈值

    struct db_cfg_CellProperties_u16 GsUa; //电压通道增益
    struct db_cfg_CellProperties_u16 GsUb;
    struct db_cfg_CellProperties_u16 GsUc;
    struct db_cfg_CellProperties_u16 GsIa; //电流通道增益
    struct db_cfg_CellProperties_u16 GsIb;
    struct db_cfg_CellProperties_u16 GsIc;
    struct db_cfg_CellProperties_u16 GsIn;

    struct db_cfg_CellProperties_u32 Phs_Ua; //电压通道相位
    struct db_cfg_CellProperties_u32 Phs_Ub;
    struct db_cfg_CellProperties_u32 Phs_Uc;
    struct db_cfg_CellProperties_u32 Phs_Ia; //电流通道相位
    struct db_cfg_CellProperties_u32 Phs_Ib;
    struct db_cfg_CellProperties_u32 Phs_Ic;
    struct db_cfg_CellProperties_u32 Phs_In;

    struct db_cfg_CellProperties_u16 Pa_PhsL; //有功分段相位校正寄存器-低段
    struct db_cfg_CellProperties_u16 Pb_PhsL;
    struct db_cfg_CellProperties_u16 Pc_PhsL;
    struct db_cfg_CellProperties_u16 Qa_PhsL; //无功分段相位校正寄存器-低段
    struct db_cfg_CellProperties_u16 Qb_PhsL;
    struct db_cfg_CellProperties_u16 Qc_PhsL;

    struct db_cfg_CellProperties_u32 ChkSum; //EMM校表和配置寄存器校验和
    struct db_cfg_CellProperties_u16 I0Adj;  //漏电电流校准值
    struct db_cfg_CellProperties_u16 T0Adj;  //温度校准值
    struct db_cfg_CellProperties_u16 T1Adj;
    struct db_cfg_CellProperties_u16 T2Adj;
    struct db_cfg_CellProperties_u16 T3Adj;

    struct db_cfg_CellProperties_u16 Offset_Ia;
    struct db_cfg_CellProperties_u16 Offset_Ib;
    struct db_cfg_CellProperties_u16 Offset_Ic;

};

/*
    死区参数属性表
*/
struct db_cfg_sys_deadzone_para
{
    struct db_cfg_CellProperties_u16 Ua;
    struct db_cfg_CellProperties_u16 Ub;
    struct db_cfg_CellProperties_u16 Uc;
    struct db_cfg_CellProperties_u16 Ia;
    struct db_cfg_CellProperties_u16 Ib;
    struct db_cfg_CellProperties_u16 Ic;
    struct db_cfg_CellProperties_u16 In;
    struct db_cfg_CellProperties_u16 Pa;
    struct db_cfg_CellProperties_u16 Pb;
    struct db_cfg_CellProperties_u16 Pc;
    struct db_cfg_CellProperties_u16 Qa;
    struct db_cfg_CellProperties_u16 Qb;
    struct db_cfg_CellProperties_u16 Qc;
    struct db_cfg_CellProperties_u16 Sa;
    struct db_cfg_CellProperties_u16 Sb;
    struct db_cfg_CellProperties_u16 Sc;
    struct db_cfg_CellProperties_u16 Uab;
    struct db_cfg_CellProperties_u16 Ubc;
    struct db_cfg_CellProperties_u16 Uca;
};


/*
    接口参数属性表
*/
struct db_cfg_sys_interface_para
{
    struct db_cfg_CellProperties_u16 RatedVoltage;   //额定电压
    struct db_cfg_CellProperties_u16 RatedCurrent;   //额定电流
    struct db_cfg_CellProperties_u16 RatedFrequency; //频率
    struct db_cfg_CellProperties_u64 EnergyMax;      //翻转电度
};

/*
    系统参数集合
*/
struct db_cfg_sys_para
{
    struct db_cfg_sys_adjust_para       adjust;
    struct db_cfg_sys_adjust_value_type adjustvalue;
    struct db_cfg_sys_deadzone_para     deadzone;
    struct db_cfg_sys_interface_para    interface;

    struct db_cfg_sys_adjust_ad_para    adjustAd;
    struct db_cfg_sys_adjust_ad_value_para adjustAdvalue;
    struct db_cfg_sys_adjust_prth_para  prthvalue;
    struct db_cfg_sys_adjust_I0_para    adjustI0value;
    
    char                                sn[32];
    char                                model[32];
};

#endif
