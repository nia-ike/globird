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

#ifndef __DB_MEASURE_ENERGY_H__
#define __DB_MEASURE_ENERGY_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/* 
    能量数据库文件路径常量定义
*/
#define DB_ENERGY_SQL_PATH          "/userdata/energy"
#define DB_ENERGY_SQL_DIR           "/userdata/energy/"

/* 
    能量数据库文件类型枚举
*/
typedef enum {
    DB_ENERGY_DB_TYPE_MAIN = 0,     // 主数据库
    DB_ENERGY_DB_TYPE_BACKUP = 1,   // 备份数据库
} db_energy_db_type_t;

/* 
    动态生成能量数据库文件路径
    规则：
    - 通道 0: rt_energy_main.db / rt_energy_bakup.db
    - 通道 1: rt_energy_main_2ed.db / rt_energy_bakup_2ed.db
    - 通道 n: rt_energy_main_{n+1}ed.db / rt_energy_bakup_{n+1}ed.db
*/
static inline const char* db_energy_build_path(const char *base_name, 
                                                uint16_t channel, 
                                                db_energy_db_type_t type)
{
    static char path_buffer[256];
    const char *suffix = (type == DB_ENERGY_DB_TYPE_BACKUP) ? "bakup" : "main";
    
    if(channel == 0)
    {
        // channel 0: rt_energy_main.db / rt_energy_bakup.db
        snprintf(path_buffer, sizeof(path_buffer), "%s%s_%s.db", 
                 DB_ENERGY_SQL_DIR, base_name, suffix);
    }
    else
    {
        // channel 1+: rt_energy_main_2ed.db, rt_energy_main_3ed.db, ...
        snprintf(path_buffer, sizeof(path_buffer), "%s%s_%s_%ded.db", 
                 DB_ENERGY_SQL_DIR, base_name, suffix, channel + 1);
    }
    
    return path_buffer;
}

// 实时能量数据库路径生成
static inline const char* db_energy_get_realtime_main(uint16_t channel)
{
    return db_energy_build_path("rt_energy", channel, DB_ENERGY_DB_TYPE_MAIN);
}

static inline const char* db_energy_get_realtime_backup(uint16_t channel)
{
    return db_energy_build_path("rt_energy", channel, DB_ENERGY_DB_TYPE_BACKUP);
}

// 多费率能量数据库路径生成
static inline const char* db_energy_get_multirate_main(uint16_t channel)
{
    return db_energy_build_path("multirate_energy", channel, DB_ENERGY_DB_TYPE_MAIN);
}

static inline const char* db_energy_get_multirate_backup(uint16_t channel)
{
    return db_energy_build_path("multirate_energy", channel, DB_ENERGY_DB_TYPE_BACKUP);
}

// 历史能量数据库路径生成（只有主库，无备份）
static inline const char* db_energy_get_history_main(uint16_t channel)
{
    static char path[256];
    
    if(channel == 0)
    {
        // channel 0: rt_energy_history.db
        snprintf(path, sizeof(path), "%srt_energy_history.db", DB_ENERGY_SQL_DIR);
    }
    else
    {
        // channel 1+: rt_energy_history_2ed.db, rt_energy_history_3ed.db, ...
        snprintf(path, sizeof(path), "%srt_energy_history_%ded.db", 
                 DB_ENERGY_SQL_DIR, channel + 1);
    }
    
    return path;
}

// 运行能量数据库路径生成（只有主库，无备份）
static inline const char* db_energy_get_runing_main(uint16_t channel)
{
    static char path[256];
    
    if(channel == 0)
    {
        // channel 0: runing_energy.db
        snprintf(path, sizeof(path), "%sruning_energy.db", DB_ENERGY_SQL_DIR);
    }
    else
    {
        // channel 1+: runing_energy_2ed.db, runing_energy_3ed.db, ...
        snprintf(path, sizeof(path), "%sruning_energy_%ded.db", 
                 DB_ENERGY_SQL_DIR, channel + 1);
    }
    
    return path;
}

// ========== 需量最值数据库路径 ==========
#define DB_MAXDMD_SQL_DIR  "/userdata/maxdmd/"

static inline const char* db_maxdmd_get_main(uint16_t channel)
{
    static char path[256];
    
    if(channel == 0)
    {
        snprintf(path, sizeof(path), "%sdemand_realtime_max.db", DB_MAXDMD_SQL_DIR);
    }
    else
    {
        snprintf(path, sizeof(path), "%sdemand_realtime_max_%ded.db", 
                 DB_MAXDMD_SQL_DIR, channel + 1);
    }
    
    return path;
}

static inline const char* db_maxdmd_get_backup(uint16_t channel)
{
    static char path[256];
    
    if(channel == 0)
    {
        snprintf(path, sizeof(path), "%sdemand_realtime_max_bakup.db", DB_MAXDMD_SQL_DIR);
    }
    else
    {
        snprintf(path, sizeof(path), "%sdemand_realtime_max_bakup_%ded.db", 
                 DB_MAXDMD_SQL_DIR, channel + 1);
    }
    
    return path;
}

// ========== 负载运行数据库路径 ==========
#define DB_LOADRUN_SQL_DIR  "/userdata/loadrun/"

static inline const char* db_loadrun_get_main(uint16_t channel)
{
    static char path[256];
    
    if(channel == 0)
    {
        snprintf(path, sizeof(path), "%sloadrun.db", DB_LOADRUN_SQL_DIR);
    }
    else
    {
        snprintf(path, sizeof(path), "%sloadrun_%ded.db", 
                 DB_LOADRUN_SQL_DIR, channel + 1);
    }
    
    return path;
}


/*
    =======================================================
    =====   以下为参照旧代码保留的宏定义，实际已不再使用 =====
    =======================================================
#define DB_ENERGY_SQL_MAIN          "/userdata/energy/rt_energy_main.db"
#define DB_ENERGY_SQL_BAKUP         "/userdata/energy/rt_energy_bakup.db"
#define DB_ENERGY_SQL_MAIN_2ED      "/userdata/energy/rt_energy_main_2ed.db"
#define DB_ENERGY_SQL_BAKUP_2ED     "/userdata/energy/rt_energy_bakup_2ed.db"

#define DB_MULTIRATE_ENERGY_SQL_PATH         "/userdata/energy/"
#define DB_MULTIRATE_ENERGY_SQL_MAIN         "/userdata/energy/multirate_energy_main.db"
#define DB_MULTIRATE_ENERGY_SQL_BAKUP        "/userdata/energy/multirate_energy_bakup.db"
#define DB_MULTIRATE_ENERGY_SQL_MAIN_2ED     "/userdata/energy/multirate_energy_main_2ed.db"
#define DB_MULTIRATE_ENERGY_SQL_BAKUP_2ED    "/userdata/energy/multirate_energy_bakup_2ed.db"

#define DB_HISTORY_SQL_PATH         "/userdata/energy/"
#define DB_HISTORY_SQL_MAIN         "/userdata/energy/rt_energy_history.db"
#define DB_HISTORY_SQL_MAIN_2ED     "/userdata/energy/rt_energy_history_2ed.db"
*/

enum 
{
    PHASE_A,
    PHASE_B,
    PHASE_C,
    PHASE_TOT,

    PHASE_NUM,                      /* A/B/C/总相总个数 */
};

enum 
{
    AREA_1,
    AREA_2,
    AREA_3,
    AREA_4,

    AREA_NUM,
};



//实时电度
struct db_realtime_energy_type
{
    uint64_t kWhImp[PHASE_NUM];       //ABC总正向有功    
    uint64_t kWhExp[PHASE_NUM];       //ABC总反向有功    
    uint64_t kvarhImp[PHASE_NUM];     //ABC总正向无功    
    uint64_t kvarhExp[PHASE_NUM];     //ABC总反向无功    
    uint64_t kVAh[PHASE_NUM];         //ABC合相视在电度   
    uint64_t kWhTot[PHASE_NUM];       //总有功电度
    uint64_t kvarhTot[PHASE_NUM];     //总无功电度
};

//实时电度扩展，用于转换成32位的6+3或8+1电能，供modbus读取
struct db_energy_realtime_translate_type
{
    uint32_t EPT;
    uint32_t EQT;
    uint32_t EPAImp;
    uint32_t EPBImp;
    uint32_t EPCImp;
    uint32_t EPTImp;
    uint32_t EPAExp;
    uint32_t EPBExp;
    uint32_t EPCExp;
    uint32_t EPTExp;
    uint32_t EQAImp;
    uint32_t EQBImp;
    uint32_t EQCImp;
    uint32_t EQTImp;
    uint32_t EQAExp;
    uint32_t EQBExp;
    uint32_t EQCExp;
    uint32_t EQTExp;
    uint32_t ESA;
    uint32_t ESB;
    uint32_t ESC;
    uint32_t EST;
};



/*
    四象限电度
*/
struct db_energy_fourquadrant_type
{
    uint32_t  kWh[PHASE_NUM][AREA_NUM];
    uint32_t  kvarh[PHASE_NUM][AREA_NUM];
};



/****************复费率参数定义******************/
//复费率参数结构
#define RATES_NUM           8   //费率数

/* 复费率电度 */
struct db_energy_multirate_uint
{
    uint64_t kWhImp;          // 复费率有功电度
    uint64_t kWhExp;          // 复费率有功电度
    uint64_t kvarhImp;        // 复费率无功电度
    uint64_t kvarhExp;        // 复费率无功电度
};

struct db_energy_multirate_type
{
    struct db_energy_multirate_uint rate[RATES_NUM];

    uint16_t crc16;
};



struct db_energy_multirate_running_status_t
{
    uint16_t rate;
    uint16_t time_index;
    uint16_t day_index;
    uint16_t week_index;
    uint16_t season_index;
    uint16_t holiday_index;
    uint16_t model_index;
};

/*************************************************************************************************************/

/* 历史电度 */
#define DB_HISTORY_ENERGY_DAY      31
#define DB_HISTORY_ENERGY_MONTH    12
#define DB_HISTORY_ENERGY_YEAR     10


/* 
    历史数据存储结构
    日  31
    月  12
    年  10
*/
struct db_history_base_t
{
    uint32_t time;
    uint32_t kWh;
    uint32_t kvarh;
    uint32_t kWhImp;
    uint32_t kWhExp;
    uint32_t kvarhImp;
    uint32_t kvarhExp;
};

struct db_history_energy_day_t
{
    uint16_t                    nItem;
    struct db_history_base_t    record[DB_HISTORY_ENERGY_DAY];
};

struct db_history_energy_month_t
{
    uint16_t                    nItem;
    struct db_history_base_t    record[DB_HISTORY_ENERGY_MONTH];
};

struct db_history_energy_year_t
{
    uint16_t                    nItem;
    struct db_history_base_t    record[DB_HISTORY_ENERGY_YEAR];
};

struct db_history_energy_commond_t
{
    uint16_t                    nItem;
    struct db_history_base_t    record[DB_HISTORY_ENERGY_DAY];
};


struct db_energy_history_type
{
    struct db_history_energy_day_t day;
    struct db_history_energy_month_t month;
    struct db_history_energy_year_t year;
};

/*************************************************************************************************************/

/*
    碳排放电度各相电度的集合
*/
struct db_energy_co2_type
{
    uint32_t kWhImp[PHASE_NUM];        //输入有功电度
    uint32_t kWhExp[PHASE_NUM];        //输出有功电度
    uint32_t kWhTot[PHASE_NUM];        //总有功电度
};


/* 负载运行电度 */
struct db_runing_energy_type
{
    uint64_t kWhImp[PHASE_NUM];       //ABC总正向有功    
    uint64_t kWhExp[PHASE_NUM];       //ABC总反向有功    
    uint64_t kvarhImp[PHASE_NUM];     //ABC总正向无功    
    uint64_t kvarhExp[PHASE_NUM];     //ABC总反向无功    
    uint64_t kVAh[PHASE_NUM];         //ABC合相视在电度   
    uint64_t kWhTot[PHASE_NUM];       //总有功电度
    uint64_t kvarhTot[PHASE_NUM];     //总无功电度
};


/****************************************** 计量类电能型数据结构体集合 *****************************************/
struct db_measure_energy_type
{
    struct db_realtime_energy_type              realtime_hp;        // 实时电能：8+3
    struct db_realtime_energy_type              remain_hp;          // 剩余电能
    struct db_realtime_energy_type              realtime;           // 实时电能：8+1
    struct db_realtime_energy_type              fundwave;           // 基波电能：8+1
    struct db_realtime_energy_type              fundwave_remain;    // 基波剩余电能：8+1
    struct db_energy_fourquadrant_type          fourquadrant;       // 四象限电能：8+1
    struct db_energy_multirate_type             multirate;          // 多费率电能：8+1
    struct db_energy_multirate_type             multirate_hp;       // 多费率电能：8+3
    struct db_energy_multirate_running_status_t multirate_status;   // 多费率运行状态
    struct db_energy_history_type               history;            // 历史电能：8+1
    struct db_energy_co2_type                   co2;                // 碳排放电能：8+1
    struct db_runing_energy_type                runing_hp;          // 运行电能：8+3
    struct db_energy_realtime_translate_type    realtime_translate; // 实时电能：6+3，用于modbus读取
};
/*************************************************************************************************************/



#endif
