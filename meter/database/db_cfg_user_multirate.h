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

#ifndef __DB_CFG_USER_MULTIRATE_H__
#define __DB_CFG_USER_MULTIRATE_H__


#include <stdint.h>
#include "db_cfg_type.h"


#define DB_MULTIRATE_CFG_USER_FILE                                "/userdata/config/meter/meter_multirate_usercfg.json"
#define DB_MULTIRATE_CFG_USER_FILE_BAKUP                          "/userdata/config/meter/bakup/meter_multirate_usercfg.json"
#define DB_MULTIRATE_CFG_USER_FILE_DEFAULT                        "/userdata/config/meter/default/meter_multirate_usercfg.json"

/* 
    USE for map table define
 */
#define DB_toString(__CLASS__)                                      #__CLASS__


/*
    支持2种费率。 最大90个特殊日 和 常规时段费率设置。
    其中特殊日为高优先级，即如果常规日与特殊日重叠，就以特殊日计量 

    1.[基本单元]为 时段。 即 小时分钟为分隔， 还有费率字段即 json配置的 {"hm":-1 , "rate":0}
    2.12个基本单元组成 [日单元] 即12个 {"hm":-1 , "rate":0}
    3.每周1-7 可最大切分4份
    4.1年最大可切分12个季节段 （当前中国只需要分2份：夏季和非夏季， 这里设计为国外有订单需求做的预留）

    JSON格式：
    "table_day":[
        {"id":0 ,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":1 ,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":2 ,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":3 ,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":4 ,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":5 ,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":6 ,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":7 ,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":8 ,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":9 ,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        
        {"id":10,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":11,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":12,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":13,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":14,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":15,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":16,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":17,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":18,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]},
        {"id":19,"day":[{"hm":0, "rate":0}, {"hm":-1 , "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}, {"hm":-1, "rate":0}]}
    ],

    "table_week":[
        {"s_week":1},
        {"s_week":-1},
        {"s_week":-1},
        {"s_week":-1}
    ],

    "normal":{
    "season":[
            {
                "md": 101,
                "week":[
                    {
                        "day":0
                    },
                    {"day":-1},
                    {"day":-1},
                    {"day":-1}

                ]
            },
            ......
        ]
    }
    -------------------------------------------------------------------------------------------
    12个季节段           4个日单元               12 个时段
    -------------------------------------------------------------------------------------------
    |  月日  |   --->   | 周 |  ------------>   | 时分 | 
    |  月日  |          | 周 |                  | 时分 |
    |  月日  |          | 周 |                  | 时分 |
    |  月日  |          | 周 |                  | 时分 |
    |  月日  |                                  | 时分 |
    |  月日  |                                  | 时分 |
    |  月日  |                                  | 时分 |
    |  月日  |                                  | 时分 |
    |  月日  |                                  | 时分 |
    |  月日  |                                  | 时分 |
    |  月日  |                                  | 时分 |
    |  月日  |                                  | 时分 |

    ================================================================================================================
    特殊日:
    "special":[
        {
            {"md":-1,"day":-1}   // day 关联 table_day的id。
        },
    ]
    -------------------------------------------------------------------------------------------
    90个日单元                      12 个时段
    -------------------------------------------------------------------------------------------
    | 特殊日期 |  ------------>      | 时分 | 
    | 特殊日期 |                     | 时分 |
    | 特殊日期 |                     | 时分 |
    | 特殊日期 |                     | 时分 |
      ......                        | 时分 |
    | 特殊日期 |                     | 时分 |
                                    | 时分 |
                                    | 时分 |
                                    | 时分 |
                                    | 时分 |
                                    | 时分 |
                                    | 时分 |


    配置中时间有3个： md 月日，  周 w，  时分  hm，满足：
    1. 时间为BCD码：  即 12月3日，表示为 1203，  周一表示为 0001，  14:24 表示为 1424
    2. 所有季节段、周段、时段的有效性依据为  各自的起始时间不为-1
    3. 多段的所有时间配置都必须从小到大顺序排列， 从开始检索到-1即代表有效时段结束。即如果配了2个季节段， 第3个季节段的时间要为-1. 周段和时段同理

    支持切换费率：
    用途： 用于自动切换多费率。 一般供电局发布新费率，不会马上执行，会在某天执行。 此时旧费率要继续运行，该功能可自动在时间达到后启用新费率


*/



#define MULTRATE_NUM_OLDNEW             2  // 新旧
#define MULTRATE_NUM_SEASON             12 // 12个季节段。 即1年可根据月日划分 12段
#define MULTRATE_NUM_WEEK               4  // 1周可划分4段 
#define MULTRATE_NUM_DAY                16 // 1天可划分16段
#define MULTRATE_NUM_DAY_SPEC           90 // 90个特殊日 
#define MULTRATE_NUM_RATES              4  // 4种费率: 尖峰平谷
#define MULTRATE_NUM_DAY_TABLE          20 // 20个时段表。可映射到特殊日和常规日


/* 
    基本单元: 时分-费率 
      20 日时段表                               每个日时段表有12个时段                      每个时段包括 时分， 费率
    db_cfg_multirate_day_table_type     ->   db_cfg_multirate_unit_group_type      ->  db_cfg_multirate_unit_type
    db_cfg_multirate_day_table_type          db_cfg_multirate_unit_group_type
    db_cfg_multirate_day_table_type          db_cfg_multirate_unit_group_type
    db_cfg_multirate_day_table_type             ...
    ......                                   db_cfg_multirate_unit_group_type
    db_cfg_multirate_day_table_type
*/
struct db_cfg_multirate_unit_type
{
    struct db_cfg_CellProperties_noscope_u16 	s_hm;                     /* 时段起始时间 */
    struct db_cfg_CellProperties_noscope_u16 	rate;
};

struct db_cfg_multirate_unit_group_type
{
    struct db_cfg_multirate_unit_type           day[MULTRATE_NUM_DAY];
};

struct db_cfg_multirate_day_table_type
{
    struct db_cfg_multirate_unit_group_type     table_day[MULTRATE_NUM_DAY_TABLE];
};


/*
    周段（工作日/节假日）列表
    最大可把1周划分成4分。  比如可划分2份： 周1-周5是工作日；  周6-周7是 节假日
    本划分对整个复费率有效。（即所有计费季都有相同的周段划分）
*/
struct db_cfg_multirate_week_unit_type
{
    struct db_cfg_CellProperties_noscope_u16 	s_week;                     /* week起始时间 */
};

struct db_cfg_multirate_week_table_type
{
    struct db_cfg_multirate_week_unit_type      week[MULTRATE_NUM_WEEK];
};





/* 
    时段表索引, 这里只填充序号，即db_cfg_multirate_day_table_type 的序号 

    day ->  db_cfg_multirate_day_table_type.table_day[0]
            db_cfg_multirate_day_table_type.table_day[1]
            db_cfg_multirate_day_table_type.table_day[2]
            db_cfg_multirate_day_table_type.table_day[3]
            db_cfg_multirate_day_table_type.table_day[4]
            db_cfg_multirate_day_table_type.table_day[5]
            ....
            db_cfg_multirate_day_table_type.table_day[19]


*/
struct db_cfg_multirate_day_type
{
    struct db_cfg_CellProperties_noscope_u16    day;
};

/* 周段，包含4个日单元 */
struct db_cfg_multirate_week_type
{
    struct db_cfg_CellProperties_noscope_u16    s_md;
    struct db_cfg_multirate_day_type            week[MULTRATE_NUM_WEEK];
};

/* 常规日，包含12个季节段 */
struct db_cfg_multirate_general_type
{
    struct db_cfg_multirate_week_type           season[MULTRATE_NUM_SEASON];
};


//------------------------------
/* 特殊日 */
/* day 时段表索引,这里只填充序号，即db_cfg_multirate_day_table_type 的序号 
     day -> db_cfg_multirate_day_table_type.table_day[0]
            db_cfg_multirate_day_table_type.table_day[1]
            db_cfg_multirate_day_table_type.table_day[2]
            db_cfg_multirate_day_table_type.table_day[3]
            db_cfg_multirate_day_table_type.table_day[4]
            db_cfg_multirate_day_table_type.table_day[5]
            ....
            db_cfg_multirate_day_table_type.table_day[19]

 */
struct db_cfg_multirate_specialday_type
{
    struct db_cfg_CellProperties_noscope_u16    s_md;
    struct db_cfg_CellProperties_noscope_u16    day;
};

struct db_cfg_multirate_special_type
{
    struct db_cfg_multirate_specialday_type cfg[MULTRATE_NUM_DAY_SPEC];
};

//------------------------------
struct db_cfg_multirate_schema_type
{
    struct db_cfg_multirate_day_table_type  table_day;
    struct db_cfg_multirate_week_table_type table_week;
    struct db_cfg_multirate_general_type    general;
    struct db_cfg_multirate_special_type    special;
};



struct db_cfg_user_multirate_para
{
    struct db_cfg_CellProperties_noscope_u32 	unix_switch;            /* 费率切换时间 ： 年月日时分秒。 */
     
    struct db_cfg_multirate_schema_type     schema;
    struct db_cfg_multirate_schema_type     schema_new;
};



#endif 
