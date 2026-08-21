/*
* Copyright (c) 2024
* All rights reserved.
*
* 文件名称：edge_alert_service.h
* 文件标识：
* 描    述：edge_alert 服务主模块头文件
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：创建
* 完成日期：20240702
*
*/

#ifndef EDGE_ALERT_SERVICE_H
#define EDGE_ALERT_SERVICE_H

#include "edge_alert_types.h"
#include "edge_alert_storage.h"

/*************************************************************************/
/** 服务上下文结构体                                                        **/
/*************************************************************************/
struct EdgeAlertServiceContext
{
    /* 配置 */
    struct EdgeAlertConfig cfg;

    /* 运行时状态 */
    struct ActiveAlarmRegistry active_alarms;

    /* 批量写入缓冲区 */
    struct AlarmWriteBatch write_batch;

    /* 极值更新批次 */
    struct ExtremeUpdateBatch extreme_batch;

    /* 数据库连接 */
    sqlite3 *db;

    /* 运行标志 */
    int running;

    /* 周期计数 */
    uint32_t poll_cycle;
};

/*************************************************************************/
/** Function Name:       edge_alert_service_create                           **/
/** Purpose:             创建并初始化告警服务                                **/
/** Params:            config_base - 配置根路径                            **/
/** Return:            服务上下文指针； NULL-失败                            **/
/** Notice:            加载配置、初始化数据库、恢复活跃告警状态              **/
/*************************************************************************/
struct EdgeAlertServiceContext *edge_alert_service_create(const char *config_base);

/*************************************************************************/
/** Function Name:       edge_alert_service_destroy                          **/
/** Purpose:             销毁告警服务，释放资源                            **/
/** Params:            ctx - 服务上下文                                   **/
/** Notice:            刷新批量写入缓冲区，关闭数据库连接                    **/
/*************************************************************************/
void edge_alert_service_destroy(struct EdgeAlertServiceContext *ctx);

/*************************************************************************/
/** Function Name:       edge_alert_service_poll                             **/
/** Purpose:             执行一次告警评估周期                             **/
/** Params:            ctx - 服务上下文                                   **/
/** Return:            0-正常； -1-严重错误需重启服务                       **/
/** Notice:            遍历所有规则评估、更新极值、批量写入数据库          **/
/*************************************************************************/
int edge_alert_service_poll(struct EdgeAlertServiceContext *ctx);

/*************************************************************************/
/** Function Name:       edge_alert_service_wait_next_poll                  **/
/** Purpose:             按配置的 poll_interval_ms 休眠，等待下一评估周期   **/
/** Params:            ctx - 服务上下文                                   **/
/*************************************************************************/
void edge_alert_service_wait_next_poll(struct EdgeAlertServiceContext *ctx);

/*************************************************************************/
/** Function Name:       edge_alert_service_start                           **/
/** Purpose:             启动告警服务主循环                              **/
/** Params:            ctx - 服务上下文                                   **/
/** Return:            退出码（0-正常退出）                                 **/
/** Notice:            阻塞直到service_stop被调用或发生致命错误             **/
/*************************************************************************/
int edge_alert_service_start(struct EdgeAlertServiceContext *ctx);

/*************************************************************************/
/** Function Name:       edge_alert_service_stop                           **/
/** Purpose:             停止告警服务主循环                              **/
/** Params:            ctx - 服务上下文                                   **/
/*************************************************************************/
void edge_alert_service_stop(struct EdgeAlertServiceContext *ctx);

/*************************************************************************/
/** Function Name:       edge_alert_service_reload_config                  **/
/** Purpose:             重新加载配置（热重载）                            **/
/** Params:            ctx - 服务上下文                                   **/
/** Return:            0-成功； -1-失败                                    **/
/** Notice:            保持活跃告警状态，重新加载配置规则                  **/
/*************************************************************************/
int edge_alert_service_reload_config(struct EdgeAlertServiceContext *ctx, const char *config_base);

/*************************************************************************/
/** Function Name:       edge_alert_service_get_stats                        **/
/** Purpose:             获取服务运行统计信息                             **/
/** Params:            ctx - 服务上下文                                   **/
/**                    active_count - 输出活跃告警数                       **/
/**                    total_rules - 输出总规则数                          **/
/** Return:            0-成功； -1-失败                                    **/
/*************************************************************************/
int edge_alert_service_get_stats(struct EdgeAlertServiceContext *ctx,
                                  uint16_t *active_count, uint16_t *total_rules);

#endif /* EDGE_ALERT_SERVICE_H */
