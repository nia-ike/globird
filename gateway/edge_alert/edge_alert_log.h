/*
* Copyright (c) 2024
* All rights reserved.
*
* 文件名称：edge_alert_log.h
* 文件标识：
* 描    述：edge_alert 日志模块接口（参照 edge_script 风格）
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：创建
* 完成日期：20240702
*
*/

#ifndef EDGE_ALERT_LOG_H
#define EDGE_ALERT_LOG_H

#include "dev_log.h"

/* edge_alert 日志应用名称 */
#define EDGE_ALERT_LOG_APP_NAME     "edge_alert"
#define EDGE_ALERT_LOG_CHANNEL      0
#define EDGE_ALERT_LOG_DEV          0

/* 日志级别（与 dev_log.h 一致） */
#define EDGE_ALERT_LOG_NONE    0
#define EDGE_ALERT_LOG_ERROR   1
#define EDGE_ALERT_LOG_WARN    2
#define EDGE_ALERT_LOG_INFO    3
#define EDGE_ALERT_LOG_DEBUG   4

/*************************************************************************/
/** Function Name:       edge_alert_log_init                               **/
/** Purpose:             初始化日志模块                                    **/
/** Params:            debug - 是否同时输出到 stderr                       **/
/** Return:            0-成功；-1-失败                                     **/
/*************************************************************************/
int edge_alert_log_init(int debug);

/*************************************************************************/
/** Function Name:       edge_alert_log_shutdown                           **/
/** Purpose:             关闭日志模块                                    **/
/*************************************************************************/
void edge_alert_log_shutdown(void);

/*************************************************************************/
/** Function Name:       edge_alert_log_info                              **/
/** Purpose:             记录 INFO 级别日志                              **/
/*************************************************************************/
void edge_alert_log_info(const char *fmt, ...);

/*************************************************************************/
/** Function Name:       edge_alert_log_warn                             **/
/** Purpose:             记录 WARN 级别日志                              **/
/*************************************************************************/
void edge_alert_log_warn(const char *fmt, ...);

/*************************************************************************/
/** Function Name:       edge_alert_log_error                            **/
/** Purpose:             记录 ERROR 级别日志                             **/
/*************************************************************************/
void edge_alert_log_error(const char *fmt, ...);

/*************************************************************************/
/** Function Name:       edge_alert_log_debug                            **/
/** Purpose:             记录 DEBUG 级别日志（仅 debug 模式）              **/
/*************************************************************************/
void edge_alert_log_debug(const char *fmt, ...);

/*************************************************************************/
/** Function Name:       edge_alert_log_action                             **/
/** Purpose:             记录告警动作日志（触发/恢复/更新），-d 1 时输出 stderr **/
/*************************************************************************/
void edge_alert_log_action(const char *fmt, ...);

/* 便捷的宏定义（使用整数级别） */
#define LOG_INFO    3
#define LOG_WARN    2
#define LOG_ERROR   1
#define LOG_DEBUG   4

#define edge_log(lvl, ...) do { \
    if ((lvl) == LOG_ERROR) edge_alert_log_error(__VA_ARGS__); \
    else if ((lvl) == LOG_WARN) edge_alert_log_warn(__VA_ARGS__); \
    else if ((lvl) == LOG_INFO) edge_alert_log_info(__VA_ARGS__); \
    else if ((lvl) == LOG_DEBUG) edge_alert_log_debug(__VA_ARGS__); \
} while(0)

/* 关键日志点宏 - 带 [ALM] 前缀供上位机识别，仅在有越限动作时输出 */
#define EDGE_ALERT_LOG_TRIGGER(key, name, val, th) \
    edge_alert_log_action("[ALM]TRIGGER: key=%s name=%s val=%.3f th=%.3f", key, name, val, th)

#define EDGE_ALERT_LOG_RECOVER(key, name, val) \
    edge_alert_log_action("[ALM]RECOVER: key=%s name=%s val=%.3f", key, name, val)

#define EDGE_ALERT_LOG_EXTREME(key, min_val, max_val) \
    edge_alert_log_action("[ALM]UPDATE: key=%s min=%.3f max=%.3f", key, min_val, max_val)

#define EDGE_ALERT_LOG_DB_ERROR(op, err) \
    edge_alert_log_error("[ALM]DB-%s: %s", op, err)

#endif /* EDGE_ALERT_LOG_H */
