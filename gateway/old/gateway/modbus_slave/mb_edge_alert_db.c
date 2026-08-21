/*
 * mb_edge_alert_db.c
 * edge_alert 告警库只读访问
 */

#include "mb_edge_alert_db.h"

#include "sqlite3.h"
#include "cJSON.h"

#include <stdio.h>
#include <string.h>

#define MB_ALARM_PAGE_SELECT \
    "SELECT id, alarm_key, chnl_id, dev_id, rule_id, rule_name, level, " \
    "trigger_ms, recover_ms, trigger_value, threshold, " \
    "recover_value, max_value, max_value_ms, min_value, min_value_ms, " \
    "snapshot_json " \
    "FROM alarm_records WHERE 1=1"

/* Modbus 应答 JSON 缓冲有限，截断 snapshot 以便单页返回更多记录 */
#define MB_ALARM_SNAPSHOT_JSON_MAX 128

static int mb_edge_alert_db_open(sqlite3 **db_out)
{
    int rc;

    if (!db_out)
        return -1;

    *db_out = NULL;
    rc = sqlite3_open_v2(MB_EDGE_ALERT_DB_PATH, db_out,
                         SQLITE_OPEN_READONLY, NULL);
    if (rc != SQLITE_OK)
    {
        if (*db_out)
        {
            sqlite3_close(*db_out);
            *db_out = NULL;
        }
        return -1;
    }

    return 0;
}

static const char *mb_edge_alert_count_sql(const mb_alarm_time_range_t *range)
{
    if (range->start_ms > 0 && range->end_ms > 0)
        return "SELECT COUNT(*) FROM alarm_records WHERE trigger_ms >= ? AND trigger_ms <= ?;";
    if (range->start_ms > 0)
        return "SELECT COUNT(*) FROM alarm_records WHERE trigger_ms >= ?;";
    if (range->end_ms > 0)
        return "SELECT COUNT(*) FROM alarm_records WHERE trigger_ms <= ?;";
    return "SELECT COUNT(*) FROM alarm_records;";
}

static const char *mb_edge_alert_page_sql(const mb_alarm_time_range_t *range)
{
    if (range->start_ms > 0 && range->end_ms > 0)
        return MB_ALARM_PAGE_SELECT " AND trigger_ms >= ? AND trigger_ms <= ? "
               "ORDER BY trigger_ms DESC, id DESC LIMIT ? OFFSET ?;";
    if (range->start_ms > 0)
        return MB_ALARM_PAGE_SELECT " AND trigger_ms >= ? "
               "ORDER BY trigger_ms DESC, id DESC LIMIT ? OFFSET ?;";
    if (range->end_ms > 0)
        return MB_ALARM_PAGE_SELECT " AND trigger_ms <= ? "
               "ORDER BY trigger_ms DESC, id DESC LIMIT ? OFFSET ?;";
    return MB_ALARM_PAGE_SELECT " ORDER BY trigger_ms DESC, id DESC LIMIT ? OFFSET ?;";
}

static void mb_edge_alert_bind_time_range(sqlite3_stmt *stmt,
                                          const mb_alarm_time_range_t *range,
                                          int *bind_idx)
{
    if (range->start_ms > 0)
        sqlite3_bind_int64(stmt, (*bind_idx)++, (sqlite3_int64)range->start_ms);
    if (range->end_ms > 0)
        sqlite3_bind_int64(stmt, (*bind_idx)++, (sqlite3_int64)range->end_ms);
}

static cJSON *mb_edge_alert_row_to_json(sqlite3_stmt *stmt)
{
    cJSON *obj;
    const unsigned char *text;

    obj = cJSON_CreateObject();
    if (!obj)
        return NULL;

    cJSON_AddNumberToObject(obj, "id", sqlite3_column_int(stmt, 0));

    text = sqlite3_column_text(stmt, 1);
    cJSON_AddStringToObject(obj, "alarm_key", text ? (const char *)text : "");

    cJSON_AddNumberToObject(obj, "chnl_id", sqlite3_column_int(stmt, 2));
    cJSON_AddNumberToObject(obj, "dev_id", sqlite3_column_int(stmt, 3));
    cJSON_AddNumberToObject(obj, "rule_id", sqlite3_column_int(stmt, 4));

    text = sqlite3_column_text(stmt, 5);
    cJSON_AddStringToObject(obj, "rule_name", text ? (const char *)text : "");

    cJSON_AddNumberToObject(obj, "level", sqlite3_column_int(stmt, 6));
    cJSON_AddNumberToObject(obj, "trigger_ms", (double)sqlite3_column_int64(stmt, 7));
    cJSON_AddNumberToObject(obj, "recover_ms", (double)sqlite3_column_int64(stmt, 8));
    cJSON_AddNumberToObject(obj, "trigger_value", sqlite3_column_double(stmt, 9));
    cJSON_AddNumberToObject(obj, "threshold", sqlite3_column_double(stmt, 10));
    cJSON_AddNumberToObject(obj, "recover_value", sqlite3_column_double(stmt, 11));
    cJSON_AddNumberToObject(obj, "max_value", sqlite3_column_double(stmt, 12));
    cJSON_AddNumberToObject(obj, "max_value_ms", (double)sqlite3_column_int64(stmt, 13));
    cJSON_AddNumberToObject(obj, "min_value", sqlite3_column_double(stmt, 14));
    cJSON_AddNumberToObject(obj, "min_value_ms", (double)sqlite3_column_int64(stmt, 15));

    text = sqlite3_column_text(stmt, 16);
    if (text)
    {
        size_t snap_len = strlen((const char *)text);

        if (snap_len <= MB_ALARM_SNAPSHOT_JSON_MAX)
        {
            cJSON_AddStringToObject(obj, "snapshot_json", (const char *)text);
        }
        else
        {
            char snap_buf[MB_ALARM_SNAPSHOT_JSON_MAX + 4];

            snprintf(snap_buf, sizeof(snap_buf), "%.*s...",
                     MB_ALARM_SNAPSHOT_JSON_MAX - 3, (const char *)text);
            cJSON_AddStringToObject(obj, "snapshot_json", snap_buf);
        }
    }
    else
    {
        cJSON_AddStringToObject(obj, "snapshot_json", "");
    }

    return obj;
}

int mb_edge_alert_db_count(const mb_alarm_time_range_t *range, uint32_t *total)
{
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    int rc;
    int bind_idx = 1;

    if (!range || !total)
        return -2;

    *total = 0;

    if (mb_edge_alert_db_open(&db) != 0)
        return -1;

    rc = sqlite3_prepare_v2(db, mb_edge_alert_count_sql(range), -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        sqlite3_close(db);
        return -2;
    }

    mb_edge_alert_bind_time_range(stmt, range, &bind_idx);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
        *total = (uint32_t)sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if (rc != SQLITE_ROW && rc != SQLITE_DONE)
        return -2;

    return 0;
}

int mb_edge_alert_db_query_page(const mb_alarm_time_range_t *range,
                                uint16_t offset,
                                uint16_t limit,
                                char *json_buf,
                                int json_buf_size,
                                uint16_t *count_out,
                                uint32_t *total_out)
{
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    cJSON *root = NULL;
    cJSON *records = NULL;
    char *json_text = NULL;
    int rc;
    int bind_idx = 1;
    int db_ret = 0;

    if (!range || !json_buf || json_buf_size <= 0 || !count_out || !total_out)
        return -2;

    json_buf[0] = '\0';
    *count_out = 0;
    *total_out = 0;

    db_ret = mb_edge_alert_db_count(range, total_out);
    if (db_ret != 0)
        return db_ret;

    if (mb_edge_alert_db_open(&db) != 0)
        return -1;

    rc = sqlite3_prepare_v2(db, mb_edge_alert_page_sql(range), -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        sqlite3_close(db);
        return -2;
    }

    mb_edge_alert_bind_time_range(stmt, range, &bind_idx);
    sqlite3_bind_int(stmt, bind_idx++, (int)limit);
    sqlite3_bind_int(stmt, bind_idx++, (int)offset);

    root = cJSON_CreateObject();
    records = cJSON_CreateArray();
    if (!root || !records)
    {
        db_ret = -2;
        goto cleanup;
    }
    cJSON_AddItemToObject(root, "records", records);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        cJSON *row = mb_edge_alert_row_to_json(stmt);

        if (!row)
        {
            db_ret = -2;
            goto cleanup;
        }

        cJSON_AddItemToArray(records, row);
        (*count_out)++;

        json_text = cJSON_PrintUnformatted(root);
        if (!json_text)
        {
            db_ret = -2;
            goto cleanup;
        }

        if ((int)strlen(json_text) >= json_buf_size)
        {
            cJSON_DeleteItemFromArray(records, cJSON_GetArraySize(records) - 1);
            (*count_out)--;
            cJSON_free(json_text);
            json_text = NULL;
            break;
        }

        cJSON_free(json_text);
        json_text = NULL;
    }

    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
    {
        db_ret = -2;
        goto cleanup;
    }

    json_text = cJSON_PrintUnformatted(root);
    if (!json_text)
    {
        db_ret = -2;
        goto cleanup;
    }

    if ((int)strlen(json_text) >= json_buf_size)
    {
        db_ret = -2;
        goto cleanup;
    }

    snprintf(json_buf, (size_t)json_buf_size, "%s", json_text);
    db_ret = 0;

cleanup:
    if (json_text)
        cJSON_free(json_text);
    if (root)
        cJSON_Delete(root);
    if (stmt)
        sqlite3_finalize(stmt);
    if (db)
        sqlite3_close(db);

    if (db_ret != 0)
    {
        json_buf[0] = '\0';
        *count_out = 0;
    }

    return db_ret;
}
