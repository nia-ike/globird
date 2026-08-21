/*
* Copyright (c) 2024
* All rights reserved.
*
* 文件名称：edge_alert_storage.h
* 文件标识：
* 描    述：edge_alert 存储层头文件（SQLite操作接口）
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：创建
* 完成日期：20240702
*
*/

#ifndef EDGE_ALERT_STORAGE_H
#define EDGE_ALERT_STORAGE_H

#include "edge_alert_types.h"
#include <sqlite3.h>

/* 存储层错误码 */
#define EDGE_ALERT_STORAGE_OK           0
#define EDGE_ALERT_STORAGE_ERROR        -1
#define EDGE_ALERT_STORAGE_CORRUPTED    -2  /* 数据库损坏 */

/*************************************************************************/
/** Function Name:       edge_alert_storage_init                           **/
/** Purpose:             初始化SQLite数据库                                  **/
/** Params:            db_path - 数据库文件路径（NULL使用默认路径）          **/
/**                    db - 输出数据库连接句柄                               **/
/** Return:            0-成功； -1-失败； -2-数据库损坏（需重建）             **/
/** Notice:            启动时若数据库损坏或不存在，自动删除并重建           **/
/*************************************************************************/
int edge_alert_storage_init(const char *db_path, sqlite3 **db);

/*************************************************************************/
/** Function Name:       edge_alert_storage_close                          **/
/** Purpose:             关闭数据库连接                                    **/
/** Params:            db - 数据库连接句柄                               **/
/*************************************************************************/
void edge_alert_storage_close(sqlite3 *db);

/*************************************************************************/
/** Function Name:       edge_alert_storage_insert_record                  **/
/** Purpose:             插入新的告警记录（0-8191 ID循环）                    **/
/** Params:            db - 数据库连接句柄                               **/
/**                    record - 告警记录数据                             **/
/**                    max_records - 最大记录数（默认8192）                **/
/** Return:            >0 - 插入的记录ID； 0 - 失败（可重试错误）；          **/
/**                    -2 - 数据库损坏（需重建，调用者处理）                **/
/** Notice:            ID达到上限后使用REPLACE方式循环复用                  **/
/*************************************************************************/
int64_t edge_alert_storage_insert_record(sqlite3 *db, struct AlarmRecord *record, uint16_t max_records);

/*************************************************************************/
/** Function Name:       edge_alert_storage_update_recover                 **/
/** Purpose:             更新告警记录的恢复信息                            **/
/** Return:            0-成功； -1-失败（可重试）； -2-数据库损坏           **/
/*************************************************************************/
int edge_alert_storage_update_recover(sqlite3 *db, int64_t record_id,
                                      uint64_t recover_ms, double recover_value);

/*************************************************************************/
/** Function Name:       edge_alert_storage_update_extreme                 **/
/** Purpose:             更新单条告警记录的极值统计                         **/
/** Return:            0-成功； -1-失败（可重试）； -2-数据库损坏          **/
/*************************************************************************/
int edge_alert_storage_update_extreme(sqlite3 *db, int64_t record_id,
                                      double max_value, double min_value,
                                      uint64_t max_ms, uint64_t min_ms);

/*************************************************************************/
/** Function Name:       edge_alert_storage_batch_update_extreme           **/
/** Purpose:             批量更新多条记录的极值                            **/
/** Return:            0-成功； -1-失败（可重试）； -2-数据库损坏          **/
/*************************************************************************/
int edge_alert_storage_batch_update_extreme(sqlite3 *db, struct ExtremeUpdateBatch *batch);

/*************************************************************************/
/** Function Name:       edge_alert_storage_query_active_records           **/
/** Purpose:             查询所有活跃告警记录                                **/
/** Return:            >=0 - 记录数； -1-失败； -2-数据库损坏              **/
/*************************************************************************/
int edge_alert_storage_query_active_records(sqlite3 *db, struct AlarmRecord *records, int max_count);

/*************************************************************************/
/** Function Name:       edge_alert_storage_build_record_from_instance     **/
/** Purpose:             从活跃告警实例构建告警记录                          **/
/*************************************************************************/
int edge_alert_storage_build_record_from_instance(struct ActiveAlarmInstance *instance,
                                                  struct AlarmRule *rule,
                                                  struct AlarmRecord *record);

/*************************************************************************/
/** Function Name:       edge_alert_storage_delete_db_file                 **/
/** Purpose:             删除数据库文件（用于损坏后重建）                  **/
/*************************************************************************/
void edge_alert_storage_delete_db_file(const char *db_path);

#endif /* EDGE_ALERT_STORAGE_H */
