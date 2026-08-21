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

#ifndef __DB_CFG_USER_EXPAND_H__
#define __DB_CFG_USER_EXPAND_H__

#include "db_cfg_info.h"
#include "db_cfg_type.h"


/* 
    USE for map table define
 */
#define DB_toString(__CLASS__)                          #__CLASS__


/************************************************************************************/
#define RELAY_BASE_NUM  2


struct db_realtime_expand_type
{    
    uint16_t    ErrorState;    
    uint32_t    AlarmState;    
    uint16_t    DiState;   
    uint16_t    D0State;    
    uint16_t    DI1;
    uint16_t    DI2;
    uint16_t    DO1;
    uint16_t    DO2;
    uint16_t    I0;    
    uint32_t    W_Flag;   //写标志
};

// 继电器参数
struct db_cfg_user_relay_basic_type
{
    struct db_cfg_CellProperties_u16        mode;             //继电器工作模式 远程模式、本地模式
    struct db_cfg_CellProperties_u16        reset_time;       //继电器复归时间  仅远程模式下有效
};


struct db_cfg_user_relay_para
{
    struct db_cfg_user_relay_basic_type     relay[RELAY_BASE_NUM];
};



struct db_database_expand_type
{
    struct db_realtime_expand_type      db_realtime_expand;           //其他测量值

    struct db_cfg_user_relay_para      db_relay;                    //继电器
    /* 预留。产品迭代会越来越多参数，这里需要预留空间，在新添加参数时使用预留空间进行分配，以避免多次迭代后各种参数的分区
    存在较多的交替 */
    uint8_t revered[2048-36-48-192];//Mac:36 Di:48 Runing_energy:192

};





#endif
