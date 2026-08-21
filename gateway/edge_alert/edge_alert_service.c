/*
* Copyright (c) 2024
* All rights reserved.
*
* 文件名称：edge_alert_service.c
* 文件标识：
* 描    述：edge_alert 服务主模块实现
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：创建
* 完成日期：20240702
*
*/

#include "edge_alert_service.h"
#include "edge_alert_engine.h"
#include "edge_alert_cfg_load.h"
#include "edge_alert_log.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

/* 内部常量 */
#define EDGE_ALERT_BATCH_FLUSH_INTERVAL   5   /* 批量写入间隔周期数 */
#define EDGE_ALERT_EXTREME_FLUSH_INTERVAL 10  /* 极值批量更新间隔周期数 */
#define EDGE_ALERT_ARCHIVE_INTERVAL_SEC   3600 /* 归档检查间隔（秒） */

/*************************************************************************/
/** Function Name:       edge_alert_service_handle_db_corrupted              **/
/** Purpose:             处理数据库损坏错误 - 清理并自杀重启               **/
/** Notice:              检测到数据库损坏时，关闭连接、删除文件、exit(1)  **/
/**                    让 monit 自动重启，从干净状态开始                   **/
/*************************************************************************/
static void edge_alert_service_handle_db_corrupted(struct EdgeAlertServiceContext *ctx)
{
    edge_log(LOG_ERROR, "Database corrupted detected! Cleaning up and aborting for restart...");

    /* 关闭数据库连接 */
    if (ctx && ctx->db)
    {
        edge_alert_storage_close(ctx->db);
        ctx->db = NULL;
    }

    /* 删除损坏的数据库文件 */
    edge_alert_storage_delete_db_file(NULL);

    /* 给日志系统一点时间写入 */
    usleep(100000); /* 100ms */

    /* 使用 exit(1) 异常退出，让 monit/systemd 识别并自动重启 */
    exit(1);
}

/*************************************************************************/
/** Function Name:       edge_alert_service_get_time_ms                      **/
/** Purpose:             获取当前时间戳（毫秒）                             **/
/*************************************************************************/
static uint64_t edge_alert_service_get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/*************************************************************************/
/** Function Name:       edge_alert_service_flush_write_batch               **/
/** Purpose:             刷新批量写入缓冲区                                **/
/** Notice:              检测到数据库损坏(-2)时调用handle_db_corrupted自杀  **/
/*************************************************************************/
static int edge_alert_service_flush_write_batch(struct EdgeAlertServiceContext *ctx)
{
    int i;
    struct AlarmRecord record;
    int64_t record_id;
    int ret;

    if (!ctx || !ctx->db)
        return -1;

    /* 写入新触发的记录 */
    for (i = 0; i < ctx->write_batch.pending_count; i++)
    {
        record = ctx->write_batch.pending_records[i];
        record_id = edge_alert_storage_insert_record(ctx->db, &record, ctx->cfg.max_records);

        if (record_id > 0)
        {
            /* 找到对应的活跃实例并更新db_record_id */
            struct ActiveAlarmInstance *inst = alarm_engine_find_instance(
                &ctx->active_alarms, record.rule_id, record.chnl_id, record.dev_id);
            if (inst)
            {
                inst->db_record_id = record_id;
            }
        }
        else if (record_id == EDGE_ALERT_STORAGE_CORRUPTED)
        {
            /* 数据库损坏，自杀重启 */
            edge_alert_service_handle_db_corrupted(ctx);
            /* 不会执行到这里 */
        }
        else
        {
            /* 写入失败（非损坏错误，如约束冲突等），记录日志继续 */
            edge_log(LOG_ERROR, "Failed to insert alarm record: key=%s", record.alarm_key);
        }
    }

    /* 更新已恢复的记录 */
    for (i = 0; i < ctx->write_batch.update_count; i++)
    {
        ret = edge_alert_storage_update_recover(ctx->db,
                                                ctx->write_batch.update_ids[i],
                                                ctx->write_batch.update_recover_ms[i],
                                                ctx->write_batch.update_recover_val[i]);
        if (ret == EDGE_ALERT_STORAGE_CORRUPTED)
        {
            edge_alert_service_handle_db_corrupted(ctx);
        }
    }

    /* 清空缓冲区 */
    ctx->write_batch.pending_count = 0;
    ctx->write_batch.update_count = 0;

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_service_flush_extreme_batch             **/
/** Purpose:             刷新极值批量更新缓冲区                              **/
/** Notice:              检测到数据库损坏(-2)时调用handle_db_corrupted自杀  **/
/*************************************************************************/
static int edge_alert_service_flush_extreme_batch(struct EdgeAlertServiceContext *ctx)
{
    int ret;

    if (!ctx || !ctx->db)
        return -1;

    if (ctx->extreme_batch.count == 0)
        return 0;

    ret = edge_alert_storage_batch_update_extreme(ctx->db, &ctx->extreme_batch);

    /* 清空批次 */
    ctx->extreme_batch.count = 0;

    /* 检查数据库损坏 */
    if (ret == EDGE_ALERT_STORAGE_CORRUPTED)
    {
        edge_alert_service_handle_db_corrupted(ctx);
    }

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_service_add_to_batch                   **/
/** Purpose:             将新告警添加到写入批次                            **/
/*************************************************************************/
static int edge_alert_service_add_to_batch(struct EdgeAlertServiceContext *ctx,
                                           struct AlarmRecord *record)
{
    if (!ctx || !record)
        return -1;

    if (ctx->write_batch.pending_count >= 16)
    {
        /* 缓冲区满，立即刷新 */
        edge_alert_service_flush_write_batch(ctx);
    }

    ctx->write_batch.pending_records[ctx->write_batch.pending_count] = *record;
    ctx->write_batch.pending_count++;

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_service_add_recover_to_batch           **/
/** Purpose:             将恢复记录添加到更新批次                          **/
/*************************************************************************/
static int edge_alert_service_add_recover_to_batch(struct EdgeAlertServiceContext *ctx,
                                                   int64_t record_id,
                                                   uint64_t recover_ms,
                                                   double recover_val)
{
    if (!ctx)
        return -1;

    if (ctx->write_batch.update_count >= 16)
    {
        /* 缓冲区满，立即刷新 */
        edge_alert_service_flush_write_batch(ctx);
    }

    ctx->write_batch.update_ids[ctx->write_batch.update_count] = record_id;
    ctx->write_batch.update_recover_ms[ctx->write_batch.update_count] = recover_ms;
    ctx->write_batch.update_recover_val[ctx->write_batch.update_count] = recover_val;
    ctx->write_batch.update_count++;

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_service_add_extreme_to_batch           **/
/** Purpose:             将极值更新添加到批次                              **/
/*************************************************************************/
static int edge_alert_service_add_extreme_to_batch(struct EdgeAlertServiceContext *ctx,
                                                   struct ActiveAlarmInstance *instance)
{
    if (!ctx || !instance)
        return -1;

    if (instance->db_record_id <= 0)
        return -1; /* 尚未写入数据库 */

    if (ctx->extreme_batch.count >= 32)
    {
        /* 缓冲区满，立即刷新 */
        edge_alert_service_flush_extreme_batch(ctx);
    }

    ctx->extreme_batch.record_ids[ctx->extreme_batch.count] = instance->db_record_id;
    ctx->extreme_batch.max_values[ctx->extreme_batch.count] = instance->max_value;
    ctx->extreme_batch.min_values[ctx->extreme_batch.count] = instance->min_value;
    ctx->extreme_batch.max_value_ms[ctx->extreme_batch.count] = instance->max_value_ms;
    ctx->extreme_batch.min_value_ms[ctx->extreme_batch.count] = instance->min_value_ms;
    ctx->extreme_batch.count++;

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_service_eval_single_rule               **/
/** Purpose:             评估单条规则                                      **/
/*************************************************************************/
static int edge_alert_service_eval_single_rule(struct EdgeAlertServiceContext *ctx,
                                               struct AlarmRule *rule)
{
    struct ActiveAlarmInstance *instance;
    int should_fire = 0;
    int should_recover = 0;
    int should_pending = 0;
    int should_cancel_pending = 0;
    uint64_t now_ms;
    struct AlarmRecord record;
    double current_val;
    int extreme_status;

    if (!ctx || !rule)
        return -1;

    if (!rule->enabled)
        return 0;

    now_ms = edge_alert_service_get_time_ms();

    /* 查找现有活跃实例 */
    instance = alarm_engine_find_instance(&ctx->active_alarms,
                                          rule->rule_id, rule->chnl_id, rule->dev_id);

    /* 评估规则 */
    if (alarm_engine_eval_rule(rule, instance, now_ms, &should_fire, &should_recover,
                               &should_pending, &should_cancel_pending) != 0)
    {
        /* 评估出错（如点位读取失败），跳过此规则 */
        return 0;
    }

    /* 首次越限：创建延时确认实例（尚未写入数据库） */
    if (should_pending)
    {
        if (alarm_engine_read_point_value(rule->point_id, &current_val) != 0)
            current_val = 0.0;

        instance = alarm_engine_create_instance(&ctx->active_alarms, rule, current_val, now_ms);
    }

    /* 延时确认/待触发期间条件消失：移除未入库实例 */
    if (should_cancel_pending && instance)
    {
        alarm_engine_remove_instance(&ctx->active_alarms, instance);
        instance = NULL;
    }

    /* 延时确认完成：正式触发告警 */
    if (should_fire && instance)
    {
        /* 读取当前值作为触发值 */
        if (alarm_engine_read_point_value(rule->point_id, &current_val) != 0)
        {
            current_val = instance->trigger_value;
        }
        instance->trigger_value = current_val;

        /* 构建告警记录 */
        edge_alert_storage_build_record_from_instance(instance, rule, &record);

        /* 添加快照 */
        alarm_engine_build_snapshot(rule, record.snapshot_json, sizeof(record.snapshot_json));

        /* 添加到写入批次并立即入库，避免 db_record_id 仍为 0 时重复触发 */
        edge_alert_service_add_to_batch(ctx, &record);
        edge_alert_service_flush_write_batch(ctx);

        if (instance->db_record_id > 0)
            EDGE_ALERT_LOG_TRIGGER(instance->alarm_key, rule->name, current_val, rule->threshold);
    }

    /* 处理恢复 */
    if (should_recover && instance)
    {
        char alarm_key[EDGE_ALERT_CFG_ALARM_KEY_LEN];

        /* 读取当前值作为恢复值 */
        if (alarm_engine_read_point_value(rule->point_id, &current_val) != 0)
        {
            current_val = 0.0;
        }

        strncpy(alarm_key, instance->alarm_key, sizeof(alarm_key));
        alarm_key[sizeof(alarm_key) - 1] = '\0';

        /* 添加到恢复更新批次并立即入库，再移除内存实例 */
        edge_alert_service_add_recover_to_batch(ctx, instance->db_record_id, now_ms, current_val);
        edge_alert_service_flush_write_batch(ctx);

        EDGE_ALERT_LOG_RECOVER(alarm_key, rule->name, current_val);

        /* 从活跃列表移除 */
        alarm_engine_remove_instance(&ctx->active_alarms, instance);
    }

    /* 更新极值（对于持续中的告警） */
    if (instance && instance->db_record_id > 0)
    {
        /* 读取当前值更新极值 */
        if (alarm_engine_read_point_value(rule->point_id, &current_val) == 0)
        {
            extreme_status = alarm_engine_update_extreme(instance, current_val, now_ms);
            if (extreme_status > 0)
            {
                /* 极值有变化，添加到更新批次 */
                edge_alert_service_add_extreme_to_batch(ctx, instance);
                EDGE_ALERT_LOG_EXTREME(instance->alarm_key, instance->min_value, instance->max_value);
            }
        }
    }

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_service_archive_check                    **/
/** Purpose:             定期归档检查（ID循环方式下无需操作）              **/
/** Notice:              当前使用0-8191 ID REPLACE循环方式，                 **/
/**                    无需归档/删除操作，保留函数供未来扩展              **/
/*************************************************************************/
static void edge_alert_service_archive_check(struct EdgeAlertServiceContext *ctx)
{
    (void)ctx;
    /* ID循环方式下，记录数自动限制在max_records以内，无需归档 */
}

/*************************************************************************/
/** Function Name:       edge_alert_service_create                           **/
/** Purpose:             创建并初始化告警服务                                **/
/*************************************************************************/
struct EdgeAlertServiceContext *edge_alert_service_create(const char *config_base)
{
    struct EdgeAlertServiceContext *ctx;
    int ret;

    if (!config_base)
        return NULL;

    /* 分配上下文 */
    ctx = (struct EdgeAlertServiceContext *)malloc(sizeof(struct EdgeAlertServiceContext));
    if (!ctx)
    {
        edge_log(LOG_ERROR, "Failed to allocate service context");
        return NULL;
    }

    memset(ctx, 0, sizeof(struct EdgeAlertServiceContext));

    /* 加载配置 */
    if (edge_alert_cfg_load(&ctx->cfg, config_base) != 0)
    {
        edge_log(LOG_ERROR, "Failed to load configuration");
        free(ctx);
        return NULL;
    }

    edge_log(LOG_INFO, "Configuration loaded: %d rules", ctx->cfg.rule_count);

    /* 初始化数据库（启动时损坏会自动重建，返回-2表示重建也失败） */
    ret = edge_alert_storage_init(EDGE_ALERT_DB_PATH, &ctx->db);
    if (ret != EDGE_ALERT_STORAGE_OK)
    {
        edge_log(LOG_ERROR, "Failed to initialize database (ret=%d)", ret);
        /* 启动时数据库损坏且重建失败，不需要自杀，让main正常退出 */
        free(ctx);
        return NULL;
    }

    /* 尝试恢复活跃告警（从数据库加载未恢复的记录） */
    /* TODO: 可选实现 - 从数据库恢复活跃告警状态 */

    ctx->running = 0;
    ctx->poll_cycle = 0;

    edge_log(LOG_INFO, "Edge alert service created successfully");

    return ctx;
}

/*************************************************************************/
/** Function Name:       edge_alert_service_destroy                          **/
/** Purpose:             销毁告警服务，释放资源                            **/
/*************************************************************************/
void edge_alert_service_destroy(struct EdgeAlertServiceContext *ctx)
{
    if (!ctx)
        return;

    /* 停止运行 */
    ctx->running = 0;

    /* 刷新所有缓冲区 */
    edge_alert_service_flush_extreme_batch(ctx);
    edge_alert_service_flush_write_batch(ctx);

    /* 关闭数据库 */
    edge_alert_storage_close(ctx->db);
    ctx->db = NULL;

    free(ctx);

    edge_log(LOG_INFO, "Edge alert service destroyed");
}

/*************************************************************************/
/** Function Name:       edge_alert_service_poll                             **/
/** Purpose:             执行一次告警评估周期                             **/
/*************************************************************************/
int edge_alert_service_poll(struct EdgeAlertServiceContext *ctx)
{
    int i;

    if (!ctx)
        return -1;

    /* 遍历所有规则评估 */
    for (i = 0; i < ctx->cfg.rule_count; i++)
    {
        edge_alert_service_eval_single_rule(ctx, &ctx->cfg.rules[i]);
    }

    /* 定期刷新写入批次 */
    if (ctx->poll_cycle % EDGE_ALERT_BATCH_FLUSH_INTERVAL == 0)
    {
        edge_alert_service_flush_write_batch(ctx);
    }

    /* 定期刷新极值更新批次 */
    if (ctx->poll_cycle % EDGE_ALERT_EXTREME_FLUSH_INTERVAL == 0)
    {
        edge_alert_service_flush_extreme_batch(ctx);
    }

    /* 归档检查 */
    edge_alert_service_archive_check(ctx);

    ctx->poll_cycle++;

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_service_wait_next_poll                  **/
/** Purpose:             按配置的 poll_interval_ms 休眠，等待下一评估周期   **/
/*************************************************************************/
void edge_alert_service_wait_next_poll(struct EdgeAlertServiceContext *ctx)
{
    uint32_t interval_ms;

    if (!ctx)
        return;

    interval_ms = ctx->cfg.poll_interval_ms;
    if (interval_ms == 0)
        interval_ms = 1000;

    usleep(interval_ms * 1000U);
}

/*************************************************************************/
/** Function Name:       edge_alert_service_start                           **/
/** Purpose:             启动告警服务主循环                              **/
/*************************************************************************/
int edge_alert_service_start(struct EdgeAlertServiceContext *ctx)
{
    if (!ctx)
        return -1;

    if (ctx->running)
        return -1; /* 已在运行 */

    ctx->running = 1;

    edge_log(LOG_INFO, "Edge alert service started");

    while (ctx->running)
    {
        /* 执行一次评估周期 */
        if (edge_alert_service_poll(ctx) != 0)
        {
            edge_log(LOG_ERROR, "Poll failed, attempting to continue");
        }

        /* 根据配置间隔休眠 */
        edge_alert_service_wait_next_poll(ctx);
    }

    edge_log(LOG_INFO, "Edge alert service stopped");

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_service_stop                           **/
/** Purpose:             停止告警服务主循环                              **/
/*************************************************************************/
void edge_alert_service_stop(struct EdgeAlertServiceContext *ctx)
{
    if (!ctx)
        return;

    ctx->running = 0;
    edge_log(LOG_INFO, "Stop signal sent to service");
}

/*************************************************************************/
/** Function Name:       edge_alert_service_reload_config                  **/
/** Purpose:             重新加载配置（热重载）                            **/
/*************************************************************************/
int edge_alert_service_reload_config(struct EdgeAlertServiceContext *ctx, const char *config_base)
{
    struct EdgeAlertConfig new_cfg;

    if (!ctx || !config_base)
        return -1;

    /* 加载新配置 */
    memset(&new_cfg, 0, sizeof(new_cfg));
    if (edge_alert_cfg_load(&new_cfg, config_base) != 0)
    {
        edge_log(LOG_ERROR, "Failed to reload configuration");
        return -1;
    }

    /* 替换配置（保持活跃告警） */
    ctx->cfg = new_cfg;

    edge_log(LOG_INFO, "Configuration reloaded: %d rules", ctx->cfg.rule_count);

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_service_get_stats                        **/
/** Purpose:             获取服务运行统计信息                             **/
/*************************************************************************/
int edge_alert_service_get_stats(struct EdgeAlertServiceContext *ctx,
                                  uint16_t *active_count, uint16_t *total_rules)
{
    if (!ctx)
        return -1;

    if (active_count)
        *active_count = ctx->active_alarms.count;

    if (total_rules)
        *total_rules = ctx->cfg.rule_count;

    return 0;
}
