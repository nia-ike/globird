
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

#ifndef __DB_CFG_TYPE_H__
#define __DB_CFG_TYPE_H__


#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* 
    配置文件路径常量定义
    目录结构: /userdata/config/meter/meter{N}/[bakup/]xxx.json
    默认配置: /userdata/config.default/meter/meter{N}/xxx.json
*/
#define DB_CFG_METER_ROOT                   "/userdata/config/meter/"
#define DB_CFG_METER_DEFAULT_ROOT           "/userdata/config.default/meter/"

/* 配置文件类型枚举 */
typedef enum {
    DB_CFG_TYPE_NORMAL = 0,     // 正常配置文件
    DB_CFG_TYPE_BACKUP = 1,     // 备份文件
    DB_CFG_TYPE_DEFAULT = 2,    // 默认文件
} db_cfg_file_type_t;

static inline const char* db_cfg_get_meter_subdir(uint16_t channel, db_cfg_file_type_t type)
{
    static char subdir[64];
    const char *root = (type == DB_CFG_TYPE_DEFAULT) ? DB_CFG_METER_DEFAULT_ROOT : DB_CFG_METER_ROOT;

    snprintf(subdir, sizeof(subdir), "%smeter%d/", root, channel + 1);
    return subdir;
}

/* meter1 下全局共享的配置文件（多费率、扩展参数等） */
static inline const char* db_cfg_build_meter1_shared_path(const char *filename, db_cfg_file_type_t type)
{
    static char path_buffer[256];
    const char *root = (type == DB_CFG_TYPE_DEFAULT) ? DB_CFG_METER_DEFAULT_ROOT : DB_CFG_METER_ROOT;

    if(type == DB_CFG_TYPE_BACKUP)
        snprintf(path_buffer, sizeof(path_buffer), "%smeter1/bakup/%s", root, filename);
    else
        snprintf(path_buffer, sizeof(path_buffer), "%smeter1/%s", root, filename);

    return path_buffer;
}

/* 
    动态生成各通道配置文件路径
    channel 0 -> meter1/meter_cfg_sys.json
    channel 1 -> meter2/meter_cfg_sys2.json
*/
static inline const char* db_cfg_build_path(const char *base_name,
                                              uint16_t channel,
                                              db_cfg_file_type_t type)
{
    static char path_buffer[256];
    const char *subdir = db_cfg_get_meter_subdir(channel, type);

    if(channel == 0)
    {
        if(type == DB_CFG_TYPE_BACKUP)
            snprintf(path_buffer, sizeof(path_buffer), "%sbakup/%s.json", subdir, base_name);
        else
            snprintf(path_buffer, sizeof(path_buffer), "%s%s.json", subdir, base_name);
    }
    else
    {
        if(type == DB_CFG_TYPE_BACKUP)
            snprintf(path_buffer, sizeof(path_buffer), "%sbakup/%s%d.json", subdir, base_name, channel + 1);
        else
            snprintf(path_buffer, sizeof(path_buffer), "%s%s%d.json", subdir, base_name, channel + 1);
    }

    return path_buffer;
}



/*
    =======================================================
    =====   以下为参照旧代码保留的宏定义，实际已不再使用 =====
    =======================================================
#define DB_CFG_SYS_FILE                                 "/userdata/config/meter/meter_cfg_sys.json"
#define DB_CFG_SYS2_FILE                                "/userdata/config/meter/meter_cfg_sys2.json"
#define DB_CFG_SYS3_FILE                                "/userdata/config/meter/meter_cfg_sys3.json"
#define DB_CFG_USER_BASE_FILE                           "/userdata/config/meter/meter_cfg_user_base.json"
#define DB_CFG_USER_BASE2_FILE                           "/userdata/config/meter/meter_cfg_user_base2.json"
#define DB_CFG_USER_BASE3_FILE                           "/userdata/config/meter/meter_cfg_user_base3.json"
#define DB_CFG_USER_MULTIRATE_CFG                       "/userdata/config/meter/meter_cfg_user_multirate.json"
#define DB_CFG_USER_MULTIRATE_CFG_SCHEMA_OLD            "/userdata/config/meter/meter_cfg_user_multirate_scheme1.json"
#define DB_CFG_USER_MULTIRATE_CFG_SCHEMA_NEW            "/userdata/config/meter/meter_cfg_user_multirate_scheme2.json"
#define DB_CFG_USER_POWERQUALITY_FILE                   "/userdata/config/meter/meter_cfg_user_powerquality.json"
#define DB_CFG_USER_EXPAND_FILE                         "/userdata/config/meter/meter_cfg_user_expand.json"

#define DB_CFG_SYS_FILE_BACKUP                          "/userdata/config/meter/bakup/meter_cfg_sys.json"
#define DB_CFG_SYS2_FILE_BACKUP                          "/userdata/config/meter/bakup/meter_cfg_sys2.json"
#define DB_CFG_SYS3_FILE_BACKUP                          "/userdata/config/meter/bakup/meter_cfg_sys3.json"
#define DB_CFG_USER_BASE_FILE_BACKUP                    "/userdata/config/meter/bakup/meter_cfg_user_base.json"
#define DB_CFG_USER_BASE2_FILE_BACKUP                    "/userdata/config/meter/bakup/meter_cfg_user_base2.json"
#define DB_CFG_USER_BASE3_FILE_BACKUP                    "/userdata/config/meter/bakup/meter_cfg_user_base3.json"
#define DB_CFG_USER_MULTIRATE_CFG_BAK                   "/userdata/config/meter/bakup/meter_cfg_user_multirate.json"
#define DB_CFG_USER_MULTIRATE_CFG_BAK_SCHEMA_OLD        "/userdata/config/meter/bakup/meter_cfg_user_multirate_scheme1.json"
#define DB_CFG_USER_MULTIRATE_CFG_BAK_SCHEMA_NEW        "/userdata/config/meter/bakup/meter_cfg_user_multirate_scheme2.json"
#define DB_CFG_USER_POWERQUALITY_FILE_BACKUP            "/userdata/config/meter/bakup/meter_cfg_user_powerquality.json"
#define DB_CFG_USER_EXPAND_FILE_BACKUP                  "/userdata/config/meter/bakup/meter_cfg_user_expand.json"

#define DB_CFG_SYS_FILE_DEFAULT                         "/userdata/config/meter/default/meter_cfg_sys.json"
#define DB_CFG_SYS2_FILE_DEFAULT                         "/userdata/config/meter/default/meter_cfg_sys2.json"
#define DB_CFG_SYS3_FILE_DEFAULT                         "/userdata/config/meter/default/meter_cfg_sys3.json"
#define DB_CFG_USER_BASE_FILE_DEFAULT                   "/userdata/config/meter/default/meter_cfg_user_base.json"
#define DB_CFG_USER_BASE2_FILE_DEFAULT                   "/userdata/config/meter/default/meter_cfg_user_base2.json"
#define DB_CFG_USER_BASE3_FILE_DEFAULT                   "/userdata/config/meter/default/meter_cfg_user_base3.json"
#define DB_CFG_USER_MULTIRATE_CFG_DEFAULT               "/userdata/config/meter/default/meter_cfg_user_multirate.json"
#define DB_CFG_USER_MULTIRATE_CFG_DEFAULT_SCHEMA_OLD    "/userdata/config/meter/default/meter_cfg_user_multirate_scheme1.json"
#define DB_CFG_USER_MULTIRATE_CFG_DEFAULT_SCHEMA_NEW    "/userdata/config/meter/default/meter_cfg_user_multirate_scheme2.json"
#define DB_CFG_USER_POWERQUALITY_FILE_DEFAULT           "/userdata/config/meter/default/meter_cfg_user_powerquality.json"
#define DB_CFG_USER_EXPAND_FILE_DEFAULT                 "/userdata/config/meter/default/meter_cfg_user_expand.json"


#define DB_CFG_USER_DIGI_FILE                                "/userdata/config/meter/meter_usercfg_digi.json"
#define DB_CFG_USER_DIGI_FILE_BACKUP                          "/userdata/config/meter/bakup/meter_usercfg_digi.json"
#define DB_CFG_USER_DIGI_FILE_DEFAULT                       "/userdata/config/meter/default/meter_usercfg_digi.json"
*/

/* 
    动态配置文件路径辅助函数
    基于规则动态生成配置文件路径
*/

// 系统配置文件路径生成
static inline const char* db_cfg_get_sys_file(uint16_t channel)
{
    return db_cfg_build_path("meter_cfg_sys", channel, DB_CFG_TYPE_NORMAL);
}

static inline const char* db_cfg_get_sys_file_backup(uint16_t channel)
{
    return db_cfg_build_path("meter_cfg_sys", channel, DB_CFG_TYPE_BACKUP);
}

static inline const char* db_cfg_get_sys_file_default(uint16_t channel)
{
    return db_cfg_build_path("meter_cfg_sys", channel, DB_CFG_TYPE_DEFAULT);
}

// 用户基础配置文件路径生成
static inline const char* db_cfg_get_user_base_file(uint16_t channel)
{
    return db_cfg_build_path("meter_cfg_user_base", channel, DB_CFG_TYPE_NORMAL);
}

static inline const char* db_cfg_get_user_base_file_backup(uint16_t channel)
{
    return db_cfg_build_path("meter_cfg_user_base", channel, DB_CFG_TYPE_BACKUP);
}

static inline const char* db_cfg_get_user_base_file_default(uint16_t channel)
{
    return db_cfg_build_path("meter_cfg_user_base", channel, DB_CFG_TYPE_DEFAULT);
}

// 用户多费率配置文件路径生成（全局共享，位于 meter1）
static inline const char* db_cfg_get_multirate_file(db_cfg_file_type_t type)
{
    return db_cfg_build_meter1_shared_path("meter_cfg_user_multirate.json", type);
}

// 用户多费率方案1配置文件路径生成
static inline const char* db_cfg_get_multirate_schema1_file(db_cfg_file_type_t type)
{
    return db_cfg_build_meter1_shared_path("meter_cfg_user_multirate_scheme1.json", type);
}

// 用户多费率方案2配置文件路径生成
static inline const char* db_cfg_get_multirate_schema2_file(db_cfg_file_type_t type)
{
    return db_cfg_build_meter1_shared_path("meter_cfg_user_multirate_scheme2.json", type);
}

// 用户电能质量配置文件路径生成
static inline const char* db_cfg_get_powerquality_file(db_cfg_file_type_t type)
{
    return db_cfg_build_meter1_shared_path("meter_cfg_user_powerquality.json", type);
}

// 用户扩展配置文件路径生成
static inline const char* db_cfg_get_expand_file(db_cfg_file_type_t type)
{
    return db_cfg_build_meter1_shared_path("meter_cfg_user_expand.json", type);
}

// 数字输入配置文件路径生成
static inline const char* db_cfg_get_digi_file(db_cfg_file_type_t type)
{
    return db_cfg_build_meter1_shared_path("meter_usercfg_digi.json", type);
}


/* 
    USE for map table define
 */
#define DB_toString(__CLASS__)                          #__CLASS__


/************************************************************************************/
/* 
    参数约束条件
 */
struct db_cfg_ScopeCellDef_u16
{
    uint16_t ValDef; // default
    uint16_t max;    // max
    uint16_t min;    // min
};
struct db_cfg_ScopeCellDef_u32
{
    uint32_t ValDef; // default
    uint32_t max;    // max
    uint32_t min;    // min
};

struct db_cfg_ScopeCellDef_u64
{
    uint64_t ValDef; // default
    uint64_t max;   // max
    uint64_t min;   // min
};


/* 
    通用参数属性
 */
struct db_cfg_CellProperties_u16
{
    uint16_t                    value;
    struct db_cfg_ScopeCellDef_u16  scope;
};

struct db_cfg_CellProperties_u32
{
    uint32_t                    value;
    struct db_cfg_ScopeCellDef_u32  scope;
};

struct db_cfg_CellProperties_u64
{
    uint64_t                        value;
    struct db_cfg_ScopeCellDef_u64  scope;
};


struct db_base_mapdef
{
    char                         *pName;
    uint32_t                     offset;
    uint16_t                       size;
};

//-------------------
struct db_cfg_CellProperties_noscope_u16
{
    uint16_t                        value;
};

struct db_cfg_CellProperties_noscope_u32
{
    uint32_t                        value;
};

struct db_null_para
{
    struct db_cfg_ScopeCellDef_u16    dummy;
};
 

#endif
