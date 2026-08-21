/*
* Copyright (c) 2024
* All rights reserved.
*
* 文件名称：edge_alert_storage.c
* 文件标识：
* 描    述：edge_alert 存储层实现（SQLite操作）
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：错误码返回设计，自杀逻辑交由service层处理
* 完成日期：20240702
*
*/

#include "edge_alert_storage.h"
#include "edge_alert_log.h"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

/* SQL语句定义 */
#define EDGE_ALERT_SQL_CREATE_TABLE \
    "CREATE TABLE IF NOT EXISTS alarm_records (" \
    "    id INTEGER PRIMARY KEY UNIQUE ON CONFLICT REPLACE," \
    "    alarm_key TEXT NOT NULL," \
    "    chnl_id INTEGER NOT NULL," \
    "    dev_id INTEGER NOT NULL," \
    "    rule_id INTEGER NOT NULL," \
    "    rule_name TEXT," \
    "    level INTEGER NOT NULL," \
    "    trigger_ms INTEGER NOT NULL," \
    "    recover_ms INTEGER DEFAULT 0," \
    "    trigger_value REAL NOT NULL," \
    "    threshold REAL NOT NULL," \
    "    trigger_point_id INTEGER NOT NULL," \
    "    recover_value REAL DEFAULT 0," \
    "    max_value REAL DEFAULT 0," \
    "    max_value_ms INTEGER DEFAULT 0," \
    "    min_value REAL DEFAULT 0," \
    "    min_value_ms INTEGER DEFAULT 0," \
    "    snapshot_json TEXT" \
    ");" \
    "CREATE INDEX IF NOT EXISTS idx_alarm_key ON alarm_records(alarm_key);" \
    "CREATE INDEX IF NOT EXISTS idx_trigger_ms ON alarm_records(trigger_ms);"

#define EDGE_ALERT_SQL_INSERT \
    "INSERT OR REPLACE INTO alarm_records " \
    "(id, alarm_key, chnl_id, dev_id, rule_id, rule_name, level, " \
    "trigger_ms, trigger_value, threshold, trigger_point_id, snapshot_json) " \
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"

#define EDGE_ALERT_SQL_UPDATE_RECOVER \
    "UPDATE alarm_records SET recover_ms = ?, recover_value = ? WHERE id = ?;"

#define EDGE_ALERT_SQL_UPDATE_EXTREME \
    "UPDATE alarm_records SET max_value = ?, min_value = ?, " \
    "max_value_ms = ?, min_value_ms = ? WHERE id = ?;"

#define EDGE_ALERT_SQL_QUERY_ACTIVE \
    "SELECT id, alarm_key, chnl_id, dev_id, rule_id, rule_name, level, " \
    "trigger_ms, recover_ms, trigger_value, threshold, trigger_point_id, " \
    "recover_value, max_value, max_value_ms, min_value, min_value_ms, " \
    "snapshot_json FROM alarm_records ORDER BY trigger_ms DESC;"

#define EDGE_ALERT_SQL_GET_NEXT_ID \
    "SELECT COALESCE(MAX(id), 0) + 1 FROM alarm_records;"

#define EDGE_ALERT_SQL_GET_MIN_ID \
    "SELECT COALESCE(MIN(id), 0) FROM alarm_records;"

#define EDGE_ALERT_SQL_COUNT \
    "SELECT COUNT(*) FROM alarm_records;"

/* 内部函数声明 */
static unsigned long edge_alert_db_file_size(const char *path);
static int edge_alert_db_is_corrupted(int rc);
static int edge_alert_db_create_tables(sqlite3 *db);

/*************************************************************************/
/** Function Name:       edge_alert_db_file_size                           **/
/** Purpose:             获取数据库文件大小                                **/
/*************************************************************************/
static unsigned long edge_alert_db_file_size(const char *path)
{
    struct stat statbuff;

    if (!path)
        return 0;

    if (stat(path, &statbuff) != 0)
        return 0;

    return (unsigned long)statbuff.st_size;
}

/*************************************************************************/
/** Function Name:       edge_alert_db_is_corrupted                        **/
/** Purpose:             检查错误码是否表示数据库损坏                      **/
/*************************************************************************/
static int edge_alert_db_is_corrupted(int rc)
{
    if (rc == SQLITE_CORRUPT ||
        rc == SQLITE_NOTADB ||
        rc == SQLITE_IOERR ||
        rc == SQLITE_CANTOPEN)
    {
        return 1;
    }

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_storage_delete_db_file                 **/
/** Purpose:             删除数据库文件（供service层调用）                 **/
/*************************************************************************/
void edge_alert_storage_delete_db_file(const char *db_path)
{
    const char *path = db_path ? db_path : EDGE_ALERT_DB_PATH;

    if (unlink(path) == 0)
    {
        edge_log(LOG_WARN, "Deleted database file: %s", path);
    }
    else if (errno != ENOENT)
    {
        edge_log(LOG_ERROR, "Failed to delete database: %s (errno=%d)", path, errno);
    }
}

/*************************************************************************/
/** Function Name:       edge_alert_db_create_tables                       **/
/** Purpose:             在已打开的数据库中创建表结构                        **/
/** Return:            0-成功； -1-一般错误； -2-数据库损坏                  **/
/*************************************************************************/
static int edge_alert_db_create_tables(sqlite3 *db)
{
    int rc;
    char *err_msg = NULL;

    if (!db)
        return EDGE_ALERT_STORAGE_ERROR;

    rc = sqlite3_exec(db, EDGE_ALERT_SQL_CREATE_TABLE, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK)
    {
        edge_log(LOG_ERROR, "Failed to create tables: %s (rc=%d)", err_msg, rc);
        sqlite3_free(err_msg);

        if (edge_alert_db_is_corrupted(rc))
        {
            return EDGE_ALERT_STORAGE_CORRUPTED;
        }

        return EDGE_ALERT_STORAGE_ERROR;
    }

    return EDGE_ALERT_STORAGE_OK;
}

/*************************************************************************/
/** Function Name:       edge_alert_storage_init                           **/
/** Purpose:             初始化SQLite数据库                                  **/
/** Return:            0-成功； -1-失败； -2-数据库损坏（需重建）             **/
/** Notice:            启动时若数据库损坏或不存在，自动删除并重建           **/
/*************************************************************************/
int edge_alert_storage_init(const char *db_path, sqlite3 **db)
{
    const char *path = db_path ? db_path : EDGE_ALERT_DB_PATH;
    int rc;
    int need_recreate = 0;

    if (!db)
        return EDGE_ALERT_STORAGE_ERROR;

    *db = NULL;

    /* 快速检查1：文件大小为0表示损坏 */
    if (edge_alert_db_file_size(path) == 0)
    {
        edge_log(LOG_WARN, "Database file size is 0 or not exist, will recreate");
        need_recreate = 1;
    }

    /* 快速检查2：能否打开 */
    if (!need_recreate)
    {
        rc = sqlite3_open(path, db);
        if (rc != SQLITE_OK)
        {
            edge_log(LOG_WARN, "Cannot open database (rc=%d): %s, need recreate", rc, sqlite3_errmsg(*db));
            need_recreate = 1;
            if (*db)
            {
                sqlite3_close(*db);
                *db = NULL;
            }
        }
        else
        {
            /* 能打开，简单测试执行一个语句 */
            rc = sqlite3_exec(*db, "SELECT 1;", NULL, NULL, NULL);
            if (rc != SQLITE_OK)
            {
                edge_log(LOG_WARN, "Database test query failed (rc=%d): %s, need recreate", rc, sqlite3_errmsg(*db));
                sqlite3_close(*db);
                *db = NULL;
                need_recreate = 1;
            }
        }
    }

    /* 启动时重建（此时状态机还未建立，可以安全重建） */
    if (need_recreate)
    {
        edge_log(LOG_WARN, "Recreating database at startup...");
        edge_alert_storage_delete_db_file(path);

        /* 确保目录存在 */
        char dir_path[256];
        strncpy(dir_path, path, sizeof(dir_path) - 1);
        dir_path[sizeof(dir_path) - 1] = '\0';

        char *last_slash = strrchr(dir_path, '/');
        if (last_slash)
        {
            *last_slash = '\0';
            char cmd[512];
            snprintf(cmd, sizeof(cmd), "mkdir -p %s 2>/dev/null", dir_path);
            system(cmd);
        }

        /* 重新打开 */
        rc = sqlite3_open(path, db);
        if (rc != SQLITE_OK)
        {
            edge_log(LOG_ERROR, "Failed to open database after recreate: %s", sqlite3_errmsg(*db));
            if (*db)
                sqlite3_close(*db);
            *db = NULL;
            return EDGE_ALERT_STORAGE_ERROR;
        }
    }

    /* 确保表结构存在 */
    rc = edge_alert_db_create_tables(*db);
    if (rc != EDGE_ALERT_STORAGE_OK)
    {
        sqlite3_close(*db);
        *db = NULL;
        return rc;
    }

    edge_log(LOG_INFO, "Database initialized: %s", path);
    return EDGE_ALERT_STORAGE_OK;
}

/*************************************************************************/
/** Function Name:       edge_alert_storage_close                          **/
/** Purpose:             关闭数据库连接                                    **/
/*************************************************************************/
void edge_alert_storage_close(sqlite3 *db)
{
    if (db)
    {
        sqlite3_close(db);
        edge_log(LOG_INFO, "Database closed");
    }
}

/*************************************************************************/
/** Function Name:       edge_alert_storage_get_next_id                     **/
/** Purpose:             获取下一个可用的记录ID（0-8191循环）              **/
/** Return:            >0 - 可用ID； 0 - 失败； -2 - 数据库损坏            **/
/*************************************************************************/
static int edge_alert_storage_get_next_id(sqlite3 *db, uint16_t max_records)
{
    sqlite3_stmt *stmt = NULL;
    int rc;
    int count = 0;
    int min_id = 0;
    int next_id = 1;

    if (!db || max_records == 0)
        return 0;

    /* 获取当前记录数 */
    rc = sqlite3_prepare_v2(db, EDGE_ALERT_SQL_COUNT, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        edge_log(LOG_ERROR, "Failed to prepare count query (rc=%d): %s", rc, sqlite3_errmsg(db));
        if (edge_alert_db_is_corrupted(rc))
            return -2;
        return 0;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    /* 如果未达到上限，获取当前最大ID+1 */
    if (count < max_records)
    {
        rc = sqlite3_prepare_v2(db, EDGE_ALERT_SQL_GET_NEXT_ID, -1, &stmt, NULL);
        if (rc != SQLITE_OK)
        {
            edge_log(LOG_ERROR, "Failed to prepare next_id query (rc=%d): %s", rc, sqlite3_errmsg(db));
            if (edge_alert_db_is_corrupted(rc))
                return -2;
            return 0;
        }

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            next_id = sqlite3_column_int(stmt, 0);
            if (next_id > max_records)
            {
                next_id = 1; /* 从1重新开始 */
            }
        }
        sqlite3_finalize(stmt);
    }
    else
    {
        /* 达到上限，获取最小ID作为下一个（REPLACE方式） */
        rc = sqlite3_prepare_v2(db, EDGE_ALERT_SQL_GET_MIN_ID, -1, &stmt, NULL);
        if (rc != SQLITE_OK)
        {
            edge_log(LOG_ERROR, "Failed to prepare min_id query (rc=%d): %s", rc, sqlite3_errmsg(db));
            if (edge_alert_db_is_corrupted(rc))
                return -2;
            return 0;
        }

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            min_id = sqlite3_column_int(stmt, 0);
            next_id = min_id;
            edge_log(LOG_INFO, "Record limit reached (%d), recycling ID %d", max_records, next_id);
        }
        sqlite3_finalize(stmt);
    }

    return next_id;
}

/*************************************************************************/
/** Function Name:       edge_alert_storage_insert_record                  **/
/** Purpose:             插入新的告警记录（0-8191 ID循环）                    **/
/** Return:            >0 - 插入的记录ID； 0 - 失败（可重试）； -2 - 损坏   **/
/*************************************************************************/
int64_t edge_alert_storage_insert_record(sqlite3 *db, struct AlarmRecord *record, uint16_t max_records)
{
    sqlite3_stmt *stmt = NULL;
    int rc;
    int64_t record_id = 0;
    int next_id;

    if (!db || !record)
        return 0;

    /* 获取下一个可用ID */
    next_id = edge_alert_storage_get_next_id(db, max_records);
    if (next_id == 0)
    {
        edge_log(LOG_ERROR, "Failed to get next record ID");
        return 0;
    }
    if (next_id < 0) /* -2 数据库损坏 */
    {
        return -2;
    }

    /* 准备语句 - 使用 REPLACE 方式 */
    rc = sqlite3_prepare_v2(db, EDGE_ALERT_SQL_INSERT, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        edge_log(LOG_ERROR, "Failed to prepare insert statement (rc=%d): %s", rc, sqlite3_errmsg(db));
        if (edge_alert_db_is_corrupted(rc))
            return -2;
        return 0;
    }

    /* 绑定参数 - id 放在第一个位置 */
    sqlite3_bind_int(stmt, 1, next_id);
    sqlite3_bind_text(stmt, 2, record->alarm_key, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, record->chnl_id);
    sqlite3_bind_int(stmt, 4, record->dev_id);
    sqlite3_bind_int(stmt, 5, record->rule_id);
    sqlite3_bind_text(stmt, 6, record->rule_name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, record->level);
    sqlite3_bind_int64(stmt, 8, (sqlite3_int64)record->trigger_ms);
    sqlite3_bind_double(stmt, 9, record->trigger_value);
    sqlite3_bind_double(stmt, 10, record->threshold);
    sqlite3_bind_int(stmt, 11, (int)record->trigger_point_id);
    sqlite3_bind_text(stmt, 12, record->snapshot_json, -1, SQLITE_STATIC);

    /* 执行 */
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        edge_log(LOG_ERROR, "Failed to insert record (rc=%d): %s", rc, sqlite3_errmsg(db));
        if (edge_alert_db_is_corrupted(rc))
            return -2;
        return 0;
    }

    record_id = next_id;
    record->id = record_id;

    return record_id;
}

/*************************************************************************/
/** Function Name:       edge_alert_storage_update_recover                 **/
/** Purpose:             更新告警记录的恢复信息                              **/
/** Return:            0-成功； -1-失败（可重试）； -2-数据库损坏           **/
/*************************************************************************/
int edge_alert_storage_update_recover(sqlite3 *db, int64_t record_id,
                                      uint64_t recover_ms, double recover_value)
{
    sqlite3_stmt *stmt = NULL;
    int rc;

    if (!db || record_id <= 0)
        return EDGE_ALERT_STORAGE_ERROR;

    rc = sqlite3_prepare_v2(db, EDGE_ALERT_SQL_UPDATE_RECOVER, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        edge_log(LOG_ERROR, "Failed to prepare update statement (rc=%d): %s", rc, sqlite3_errmsg(db));
        if (edge_alert_db_is_corrupted(rc))
            return EDGE_ALERT_STORAGE_CORRUPTED;
        return EDGE_ALERT_STORAGE_ERROR;
    }

    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)recover_ms);
    sqlite3_bind_double(stmt, 2, recover_value);
    sqlite3_bind_int64(stmt, 3, record_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        edge_log(LOG_ERROR, "Failed to update recover info (rc=%d): %s", rc, sqlite3_errmsg(db));
        if (edge_alert_db_is_corrupted(rc))
            return EDGE_ALERT_STORAGE_CORRUPTED;
        return EDGE_ALERT_STORAGE_ERROR;
    }

    return EDGE_ALERT_STORAGE_OK;
}

/*************************************************************************/
/** Function Name:       edge_alert_storage_update_extreme                 **/
/** Purpose:             更新单条告警记录的极值统计                          **/
/** Return:            0-成功； -1-失败（可重试）； -2-数据库损坏          **/
/*************************************************************************/
int edge_alert_storage_update_extreme(sqlite3 *db, int64_t record_id,
                                      double max_value, double min_value,
                                      uint64_t max_ms, uint64_t min_ms)
{
    sqlite3_stmt *stmt = NULL;
    int rc;

    if (!db || record_id <= 0)
        return EDGE_ALERT_STORAGE_ERROR;

    rc = sqlite3_prepare_v2(db, EDGE_ALERT_SQL_UPDATE_EXTREME, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        edge_log(LOG_ERROR, "Failed to prepare extreme update statement (rc=%d): %s", rc, sqlite3_errmsg(db));
        if (edge_alert_db_is_corrupted(rc))
            return EDGE_ALERT_STORAGE_CORRUPTED;
        return EDGE_ALERT_STORAGE_ERROR;
    }

    sqlite3_bind_double(stmt, 1, max_value);
    sqlite3_bind_double(stmt, 2, min_value);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)max_ms);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)min_ms);
    sqlite3_bind_int64(stmt, 5, record_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        edge_log(LOG_ERROR, "Failed to update extreme values (rc=%d): %s", rc, sqlite3_errmsg(db));
        if (edge_alert_db_is_corrupted(rc))
            return EDGE_ALERT_STORAGE_CORRUPTED;
        return EDGE_ALERT_STORAGE_ERROR;
    }

    return EDGE_ALERT_STORAGE_OK;
}

/*************************************************************************/
/** Function Name:       edge_alert_storage_batch_update_extreme           **/
/** Purpose:             批量更新多条记录的极值                              **/
/** Return:            0-成功； -1-失败（可重试）； -2-数据库损坏          **/
/*************************************************************************/
int edge_alert_storage_batch_update_extreme(sqlite3 *db, struct ExtremeUpdateBatch *batch)
{
    int i;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int success_count = 0;

    if (!db || !batch || batch->count == 0)
        return EDGE_ALERT_STORAGE_OK;

    /* 开始事务 */
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        edge_log(LOG_ERROR, "Failed to begin transaction (rc=%d): %s", rc, sqlite3_errmsg(db));
        if (edge_alert_db_is_corrupted(rc))
            return EDGE_ALERT_STORAGE_CORRUPTED;
        return EDGE_ALERT_STORAGE_ERROR;
    }

    /* 准备语句 */
    rc = sqlite3_prepare_v2(db, EDGE_ALERT_SQL_UPDATE_EXTREME, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        edge_log(LOG_ERROR, "Failed to prepare batch update statement (rc=%d): %s", rc, sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        if (edge_alert_db_is_corrupted(rc))
            return EDGE_ALERT_STORAGE_CORRUPTED;
        return EDGE_ALERT_STORAGE_ERROR;
    }

    /* 批量绑定并执行 */
    for (i = 0; i < batch->count; i++)
    {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);

        sqlite3_bind_double(stmt, 1, batch->max_values[i]);
        sqlite3_bind_double(stmt, 2, batch->min_values[i]);
        sqlite3_bind_int64(stmt, 3, (sqlite3_int64)batch->max_value_ms[i]);
        sqlite3_bind_int64(stmt, 4, (sqlite3_int64)batch->min_value_ms[i]);
        sqlite3_bind_int64(stmt, 5, batch->record_ids[i]);

        rc = sqlite3_step(stmt);
        if (rc == SQLITE_DONE)
        {
            success_count++;
        }
        else
        {
            edge_log(LOG_WARN, "Failed to update record %lld (rc=%d): %s",
                     (long long)batch->record_ids[i], rc, sqlite3_errmsg(db));
            if (edge_alert_db_is_corrupted(rc))
            {
                sqlite3_finalize(stmt);
                sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
                return EDGE_ALERT_STORAGE_CORRUPTED;
            }
        }
    }

    sqlite3_finalize(stmt);

    /* 提交事务 */
    rc = sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        edge_log(LOG_ERROR, "Failed to commit transaction (rc=%d): %s", rc, sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        if (edge_alert_db_is_corrupted(rc))
            return EDGE_ALERT_STORAGE_CORRUPTED;
        return EDGE_ALERT_STORAGE_ERROR;
    }

    return EDGE_ALERT_STORAGE_OK;
}

/*************************************************************************/
/** Function Name:       edge_alert_storage_query_active_records           **/
/** Purpose:             查询所有活跃告警记录                                **/
/** Return:            >=0 - 记录数； -1-失败； -2-数据库损坏              **/
/*************************************************************************/
int edge_alert_storage_query_active_records(sqlite3 *db, struct AlarmRecord *records, int max_count)
{
    sqlite3_stmt *stmt = NULL;
    int rc;
    int count = 0;

    if (!db || !records || max_count <= 0)
        return 0;

    rc = sqlite3_prepare_v2(db, EDGE_ALERT_SQL_QUERY_ACTIVE, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        edge_log(LOG_ERROR, "Failed to prepare query statement (rc=%d): %s", rc, sqlite3_errmsg(db));
        if (edge_alert_db_is_corrupted(rc))
            return EDGE_ALERT_STORAGE_CORRUPTED;
        return EDGE_ALERT_STORAGE_ERROR;
    }

    while (count < max_count && sqlite3_step(stmt) == SQLITE_ROW)
    {
        struct AlarmRecord *r = &records[count];

        r->id = sqlite3_column_int64(stmt, 0);
        strncpy(r->alarm_key, (const char *)sqlite3_column_text(stmt, 1), sizeof(r->alarm_key) - 1);
        r->alarm_key[sizeof(r->alarm_key) - 1] = '\0';
        r->chnl_id = (uint8_t)sqlite3_column_int(stmt, 2);
        r->dev_id = (uint8_t)sqlite3_column_int(stmt, 3);
        r->rule_id = (uint16_t)sqlite3_column_int(stmt, 4);
        strncpy(r->rule_name, (const char *)sqlite3_column_text(stmt, 5), sizeof(r->rule_name) - 1);
        r->rule_name[sizeof(r->rule_name) - 1] = '\0';
        r->level = (uint8_t)sqlite3_column_int(stmt, 6);
        r->trigger_ms = (uint64_t)sqlite3_column_int64(stmt, 7);
        r->recover_ms = (uint64_t)sqlite3_column_int64(stmt, 8);
        r->trigger_value = sqlite3_column_double(stmt, 9);
        r->threshold = sqlite3_column_double(stmt, 10);
        r->trigger_point_id = (uint32_t)sqlite3_column_int(stmt, 11);
        r->recover_value = sqlite3_column_double(stmt, 12);
        r->max_value = sqlite3_column_double(stmt, 13);
        r->max_value_ms = (uint64_t)sqlite3_column_int64(stmt, 14);
        r->min_value = sqlite3_column_double(stmt, 15);
        r->min_value_ms = (uint64_t)sqlite3_column_int64(stmt, 16);

        const char *snapshot = (const char *)sqlite3_column_text(stmt, 17);
        if (snapshot)
        {
            strncpy(r->snapshot_json, snapshot, sizeof(r->snapshot_json) - 1);
            r->snapshot_json[sizeof(r->snapshot_json) - 1] = '\0';
        }
        else
        {
            r->snapshot_json[0] = '\0';
        }

        r->archived = 0;

        count++;
    }

    sqlite3_finalize(stmt);

    edge_log(LOG_INFO, "Queried %d active records", count);

    return count;
}

/*************************************************************************/
/** Function Name:       edge_alert_storage_build_record_from_instance       **/
/** Purpose:             从活跃告警实例构建告警记录                          **/
/*************************************************************************/
int edge_alert_storage_build_record_from_instance(struct ActiveAlarmInstance *instance,
                                                  struct AlarmRule *rule,
                                                  struct AlarmRecord *record)
{
    if (!instance || !rule || !record)
        return EDGE_ALERT_STORAGE_ERROR;

    memset(record, 0, sizeof(struct AlarmRecord));

    /* 定位信息 */
    strncpy(record->alarm_key, instance->alarm_key, sizeof(record->alarm_key) - 1);
    record->alarm_key[sizeof(record->alarm_key) - 1] = '\0';
    record->chnl_id = instance->chnl_id;
    record->dev_id = instance->dev_id;
    record->rule_id = instance->rule_id;

    /* 规则快照 */
    strncpy(record->rule_name, rule->name, sizeof(record->rule_name) - 1);
    record->rule_name[sizeof(record->rule_name) - 1] = '\0';
    record->level = rule->level;

    /* 核心时间 */
    record->trigger_ms = instance->trigger_ms;
    record->recover_ms = 0; /* 新记录未恢复 */

    /* 触发数据 */
    record->trigger_value = instance->trigger_value;
    record->threshold = rule->threshold;
    record->trigger_point_id = instance->point_id;

    /* 初始极值 */
    record->max_value = instance->max_value;
    record->max_value_ms = instance->max_value_ms;
    record->min_value = instance->min_value;
    record->min_value_ms = instance->min_value_ms;

    /* 归档标记 */
    record->archived = 0;

    return EDGE_ALERT_STORAGE_OK;
}
