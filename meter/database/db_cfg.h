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

#ifndef __DB_CFG_H__
#define __DB_CFG_H__

#include "db_cfg_sys.h"
#include "db_cfg_info.h"
#include "db_cfg_type.h"
#include "db_cfg_user_base.h"
#include "db_cfg_user_multirate.h"
#include "db_cfg_user_expand.h"

/* 
    USE for map table define
 */
#define DB_toString(__CLASS__)                          #__CLASS__


/* 
    用户参数集合
 */
struct db_cfg_user_para
{
    struct db_cfg_user_base_para            base;
    struct db_cfg_user_demand_para          demand;
    
    struct db_cfg_user_pulseout_para        pulseout;
    struct db_cfg_user_loadrun_para         loadrun;
    struct db_cfg_user_refer_para           refer;
    struct db_cfg_user_network_para         network;
    struct db_cfg_user_runing_para          runing;

    /* 预留。产品迭代会越来越多参数，这里需要预留空间，在新添加参数时使用预留空间进行分配，以避免多次迭代后各种参数的分区
    存在较多的交替 */
    uint8_t revered[2048];
};


struct db_database_cfg_para
{
    struct db_cfg_user_para         user;
    struct db_cfg_sys_para          sys;
}; 






#endif
