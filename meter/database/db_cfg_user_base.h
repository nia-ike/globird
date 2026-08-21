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

#ifndef __DB_CFG_USER_BASE_H__
#define __DB_CFG_USER_BASE_H__

#include "db_cfg_info.h"
#include "db_cfg_type.h"

#define PULSE_NUM       2

/* 
    USE for map table define
 */
#define DB_toString(__CLASS__)                          #__CLASS__


/************************************************************************************/

//电气测量点基本参数
struct db_cfg_user_base_para
{
    struct db_cfg_CellProperties_u16 PassWord;     //密码
    struct db_cfg_CellProperties_u16 Addr;         //地址
    struct db_cfg_CellProperties_u32 BaudRate;     //波特率 真实数字表示1200,2400,4800,9600,19200,38400,57600
    struct db_cfg_CellProperties_u16 CharSize;     //位数
    struct db_cfg_CellProperties_u16 ParityBit;    //奇偶检验位
    struct db_cfg_CellProperties_u16 StopBit;      //停止位数
    struct db_cfg_CellProperties_u16 WiringMode;   //接线方式
    struct db_cfg_CellProperties_u32 PT;       //电压互感器变比
    struct db_cfg_CellProperties_u32 CT;       //电流互感器变比
    struct db_cfg_CellProperties_u16 RelayMode;    //继电器工作模式
    struct db_cfg_CellProperties_u16 PhaseAdjust1; //第一路电流通道
    struct db_cfg_CellProperties_u16 PhaseAdjust2; //第二路电流通道
    struct db_cfg_CellProperties_u16 PhaseAdjust3; //第三路电流通道
    struct db_cfg_CellProperties_u16 Language;     //语言
    struct db_cfg_CellProperties_u16 NoKeyTime;         //按键超时返回时间     //0表示操作不返回，单位S
    struct db_cfg_CellProperties_u16 BackLightTime;      //背光超时灭背光时间  //0表示常亮背光，单位S
    struct db_cfg_CellProperties_u16 Protocols;          //485规约： 0:标准MODBUS  1:donghong  2:chengtai
};


/*
    需量参数
*/
struct db_cfg_user_demand_para
{
    struct db_cfg_CellProperties_u16 mode;    //需量模式
    struct db_cfg_CellProperties_u16 window;  //需量周期
    struct db_cfg_CellProperties_u16 slip_window; //滑差周期
};




// 负荷运行参数
struct db_cfg_user_loadrun_para
{  
    struct db_cfg_CellProperties_u16     obj;       //对象：0、禁止计时功能、1、测量电压、2、测量电流  3、总有功功率
    struct db_cfg_CellProperties_u32 over_I;    //电流上限   
    struct db_cfg_CellProperties_u32 over_V;    //电压上限  
    struct db_cfg_CellProperties_u32 under_V;   //电压下限  
    struct db_cfg_CellProperties_u32 over_Ptot;      //总有功功率  
};

struct db_cfg_user_network_para
{  
    struct db_cfg_CellProperties_u32 ip_addr;        //IP   
    struct db_cfg_CellProperties_u32 subnet;        // netmask  
    struct db_cfg_CellProperties_u32 gateway;      // gateway  
    struct db_cfg_CellProperties_u32 dns;           //dns  
    struct db_cfg_CellProperties_u32 dhcp;          //dhcp  
    char                             mac_addr[18];
    char                             revered[18];
};



struct db_cfg_user_runing_para
{  
    struct db_cfg_CellProperties_u32 setPtot; 
    struct db_cfg_CellProperties_u32 setPa;
    struct db_cfg_CellProperties_u32 setPb;
    struct db_cfg_CellProperties_u32 setPc;     
    struct db_cfg_CellProperties_u32 setQtot;
    struct db_cfg_CellProperties_u32 setQa;
    struct db_cfg_CellProperties_u32 setQb;
    struct db_cfg_CellProperties_u32 setQc; 
    struct db_cfg_CellProperties_u32 setStot; 
    struct db_cfg_CellProperties_u32 setSa;
    struct db_cfg_CellProperties_u32 setSb;
    struct db_cfg_CellProperties_u32 setSc;      
};


struct db_cfg_user_refer_para
{  
    struct db_cfg_CellProperties_u32 refer_voltage;    //参考电压   
    struct db_cfg_CellProperties_u32 refer_current;    //参考电流  
    struct db_cfg_CellProperties_u32 refer_frequency;    //参考电流  
    struct db_cfg_CellProperties_u32 refer_Temperature;    //参考电流  
};

// 脉冲参数
struct db_cfg_user_pulse_base_para
{
    struct db_cfg_CellProperties_u16    mode;       //脉冲输出方式
    struct db_cfg_CellProperties_u16    constant;   //脉冲常数
    struct db_cfg_CellProperties_u16    wide;       //脉冲宽度
};

struct db_cfg_user_pulseout_para
{
    struct db_cfg_CellProperties_u16    checked;    //使能脉冲式输出
    struct db_cfg_user_pulse_base_para  pulse[PULSE_NUM];
};

struct db_cfg_user_vendor_para
{
    struct db_cfg_CellProperties_u16    vendor;    //厂商
};

#endif
