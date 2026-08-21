/*
* Copyright (c) 2024
* All rights reserved.
*
* 文件名称：edge_alert_types.h
* 文件标识：
* 描    述：edge_alert 告警服务核心数据类型定义
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：创建
* 完成日期：20240702
*
*/

#ifndef EDGE_ALERT_TYPES_H
#define EDGE_ALERT_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* 常量定义 */
#define EDGE_ALERT_CFG_BASE_PATH_LEN         256
#define EDGE_ALERT_CFG_MAX_RULES             64
#define EDGE_ALERT_CFG_MAX_RECORDS           8192
#define EDGE_ALERT_CFG_MAX_ACTIVE_INSTANCES  64
#define EDGE_ALERT_CFG_RULE_NAME_LEN         64
#define EDGE_ALERT_CFG_ALARM_KEY_LEN         16
#define EDGE_ALERT_CFG_SNAPSHOT_JSON_LEN     128

/* 默认配置路径 */
#define EDGE_ALERT_CFG_PATH     "/userdata/config/edge_alert/edge_alert.json"
#define EDGE_ALERT_DB_PATH        "/var/run/record/edge_alert.db"

/* 告警比较方法 */
enum AlarmCompareMethod
{
    ALARM_CMP_GT = 1,      /* >  高于阈值 */
    ALARM_CMP_LT,          /* <  低于阈值 */
    ALARM_CMP_EQ,          /* == 等于 */
    ALARM_CMP_NEQ,         /* != 不等于 */
    ALARM_CMP_BIT_ON,      /* 位为1 */
    ALARM_CMP_BIT_OFF      /* 位为0 */
};

/* 复合逻辑 */
enum AlarmLogic
{
    ALARM_LOGIC_OR = 0,
    ALARM_LOGIC_AND
};

/* 告警状态 */
enum AlarmState
{
    ALARM_STATE_NORMAL = 0,     /* 正常 */
    ALARM_STATE_PENDING,        /* 待确认 */
    ALARM_STATE_FIRED,          /* 已触发 */
    ALARM_STATE_RECOVERING,     /* 恢复中 */
    ALARM_STATE_RECOVERED       /* 已恢复 */
};

/* 单个条件 */
struct AlarmCondition
{
    uint32_t            point_id;       /* 点位ID (dblink格式) */
    uint16_t            datattr;        /* 数据属性/类型 (0=AI, 1=DI, 2=ACC) */
    char                alias[16];      /* 别名，用于日志 */
    enum AlarmCompareMethod method;     /* 比较方法 */
    double              threshold;      /* 阈值 */
    uint8_t             hysteresis_pct; /* 滞后百分比 0-100 */
    uint8_t             level;          /* 条件级别 */
};

/* 触发器配置 */
struct AlarmTrigger
{
    enum AlarmLogic     logic;          /* OR / AND */
    uint8_t             condition_count;
    struct AlarmCondition conditions[8];  /* 最多8个条件 */
};

/* 上下文点位（告警时记录） */
struct AlarmContextPoint
{
    uint32_t            point_id;
    uint16_t            datattr;        /* 数据属性/类型 (0=AI, 1=DI, 2=ACC) */
    char                alias[32];
};

/* 告警规则（运行时配置） */
struct AlarmRule
{
    uint16_t            rule_id;            /* 规则ID */
    uint8_t             enabled;            /* 使能 */
    char                name[EDGE_ALERT_CFG_RULE_NAME_LEN]; /* 规则名称 */
    char                type[32];           /* 类型标识：leak/overload等 */

    /* 主监控点配置 */
    uint32_t            point_id;           /* 主点位ID */
    uint16_t            datattr;            /* 数据属性/类型 (0=AI, 1=DI, 2=ACC) */
    enum AlarmCompareMethod method;         /* 比较方法 */
    double              threshold;          /* 阈值 */
    uint8_t             level;              /* 告警级别 1-4 */
    uint16_t            delay_sec;          /* 延时确认秒数 */
    uint8_t             hysteresis_pct;     /* 滞后百分比 */

    struct AlarmTrigger trigger;            /* 复合触发条件 */

    /* 上下文数据配置 */
    uint8_t             context_count;
    struct AlarmContextPoint context_points[16]; /* 最多16个上下文点 */

    /* 归属信息 */
    uint8_t             chnl_id;            /* 通道ID */
    uint8_t             dev_id;             /* 设备ID */
};

/* 告警服务全局配置 */
struct EdgeAlertConfig
{
    char                version[16];
    uint16_t            max_records;
    uint16_t            poll_interval_ms;
    char                ramfs_path[EDGE_ALERT_CFG_BASE_PATH_LEN];
    uint16_t            rule_count;
    struct AlarmRule    rules[EDGE_ALERT_CFG_MAX_RULES];
};

/* 告警快照点位（运行时暂存） */
struct AlarmSnapshotPoint
{
    uint32_t            point_id;      /* 点位ID (dblink格式) */
    double              value;         /* 触发时刻的值 */
};

/* 告警记录（单条描述完整事件） */
struct AlarmRecord
{
    /* 主键 */
    int64_t             id;                     /* SQLite自增 */

    /* 定位信息 */
    char                alarm_key[EDGE_ALERT_CFG_ALARM_KEY_LEN];
    uint8_t             chnl_id;                /* 通道ID */
    uint8_t             dev_id;                 /* 设备ID */
    uint16_t            rule_id;                /* 规则ID */

    /* 规则快照 */
    char                rule_name[EDGE_ALERT_CFG_RULE_NAME_LEN];
    uint8_t             level;                    /* 1-4 */

    /* 核心时间 */
    uint64_t            trigger_ms;             /* 触发时间 */
    uint64_t            recover_ms;             /* 恢复时间（0=未恢复） */

    /* 触发数据 */
    double              trigger_value;          /* 触发值 */
    double              threshold;              /* 阈值 */
    uint32_t            trigger_point_id;       /* 触发点位ID */

    /* 恢复数据 */
    double              recover_value;          /* 恢复时的值（0=未记录） */

    /* === 极值统计 === */
    double              max_value;              /* 持续期间最大值 */
    uint64_t            max_value_ms;           /* 最大值发生时间 */
    double              min_value;              /* 持续期间最小值 */
    uint64_t            min_value_ms;           /* 最小值发生时间 */

    /* 快照数据 */
    char                snapshot_json[EDGE_ALERT_CFG_SNAPSHOT_JSON_LEN];

    /* 归档标记 */
    uint8_t             archived;               /* 0/1 */
};

/* 活跃告警实例（内存中维护极值） */
struct ActiveAlarmInstance
{
    uint16_t            rule_id;
    uint8_t             chnl_id;
    uint8_t             dev_id;
    char                alarm_key[EDGE_ALERT_CFG_ALARM_KEY_LEN];
    uint32_t            point_id;           /* 监控点位ID */

    int64_t             db_record_id;       /* 数据库记录ID */

    /* 触发信息 */
    uint64_t            trigger_ms;
    double              trigger_value;

    /* === 极值统计 === */
    double              max_value;
    double              min_value;
    uint64_t            max_value_ms;
    uint64_t            min_value_ms;

    /* 标记：本周期极值是否变化 */
    uint8_t             extreme_changed;    /* 0=未变, 1=max变, 2=min变, 3=都变 */
};

/* 活跃告警管理器 */
struct ActiveAlarmRegistry
{
    struct ActiveAlarmInstance instances[EDGE_ALERT_CFG_MAX_ACTIVE_INSTANCES];
    uint16_t            count;
};

/* 批量UPDATE批次 */
struct ExtremeUpdateBatch
{
    int64_t             record_ids[32];         /* 需要UPDATE的记录ID */
    double              max_values[32];
    double              min_values[32];
    uint64_t            max_value_ms[32];       /* 最大值发生时间 */
    uint64_t            min_value_ms[32];       /* 最小值发生时间 */
    uint16_t            count;
};

/* 批量写入批次 */
struct AlarmWriteBatch
{
    struct AlarmRecord  pending_records[16];    /* 待写入的新触发记录 */
    uint16_t            pending_count;

    int64_t             update_ids[16];         /* 待更新的记录ID */
    uint64_t            update_recover_ms[16];  /* 恢复时间戳 */
    double              update_recover_val[16]; /* 恢复时的值 */
    uint16_t            update_count;
};

#endif /* EDGE_ALERT_TYPES_H */
