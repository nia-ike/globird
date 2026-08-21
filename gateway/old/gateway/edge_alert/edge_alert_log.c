/*
* Copyright (c) 2024
* All rights reserved.
*
* 文件名称：edge_alert_log.c
* 文件标识：
* 描    述：edge_alert 日志模块实现（参照 edge_script 风格）
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：创建
* 完成日期：20240702
*
*/

#include "edge_alert_log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static int s_debug = 0;
static int s_log_ready = 0;

/*************************************************************************/
/** Function Name:       edge_alert_log_v                                  **/
/** Purpose:             写入 dev_log（log_write_string）并可选 stderr     **/
/** Params:            level - INFO/WARN/ERROR/DEBUG                         **/
/**                      fmt - 内容格式                                    **/
/** Return:            None.                                               **/
/** Notice:            每条记录末尾追加 \r\n，便于 49H 从环形缓冲按行切分  **/
/*************************************************************************/
static void edge_alert_log_v(const char *level, const char *fmt, va_list args, int to_stderr)
{
    char body[512];
    char line[640];
    int n;

    n = vsnprintf(body, sizeof(body), fmt, args);
    if (n < 0)
        return;
    if (n >= (int)sizeof(body))
        body[sizeof(body) - 1] = '\0';

    snprintf(line, sizeof(line), "[%s] %s\r\n", EDGE_ALERT_LOG_APP_NAME, body);

    if (s_log_ready)
        log_write_string(EDGE_ALERT_LOG_CHANNEL, EDGE_ALERT_LOG_DEV, (char *)level, "%s", line);

    if (s_debug && to_stderr)
        fprintf(stderr, "[edge_alert] %s\n", body);
}

/*************************************************************************/
/** Function Name:       edge_alert_log_init                              **/
/** Purpose:             打开 dev_log 并记录 debug 标志                   **/
/** Params:            debug - 是否同时 printf 到 stderr                  **/
/** Return:            0-成功；-1-log_open 失败                           **/
/** Notice:            进程入口调用一次                                    **/
/*************************************************************************/
int edge_alert_log_init(int debug)
{
    s_debug = debug ? 1 : 0;
    s_log_ready = 0;

    if (log_open(NULL) == NULL)
        return -1;

    s_log_ready = 1;
    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_log_shutdown                          **/
/** Purpose:             关闭 dev_log                                      **/
/*************************************************************************/
void edge_alert_log_shutdown(void)
{
    if (!s_log_ready)
        return;

    log_close();
    s_log_ready = 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_log_info                             **/
/** Purpose:             记录 INFO 级别日志                              **/
/*************************************************************************/
void edge_alert_log_info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    edge_alert_log_v(LOG_LEVEL_INFO, fmt, args, 0);
    va_end(args);
}

/*************************************************************************/
/** Function Name:       edge_alert_log_warn                             **/
/** Purpose:             记录 WARN 级别日志                              **/
/*************************************************************************/
void edge_alert_log_warn(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    edge_alert_log_v(LOG_LEVEL_WARN, fmt, args, 0);
    va_end(args);
}

/*************************************************************************/
/** Function Name:       edge_alert_log_error                            **/
/** Purpose:             记录 ERROR 级别日志                             **/
/*************************************************************************/
void edge_alert_log_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    edge_alert_log_v(LOG_LEVEL_ERROR, fmt, args, 1);
    va_end(args);
}

/*************************************************************************/
/** Function Name:       edge_alert_log_debug                            **/
/** Purpose:             记录 DEBUG 级别日志（仅 debug 模式）              **/
/*************************************************************************/
void edge_alert_log_debug(const char *fmt, ...)
{
    va_list args;

    if (!s_debug)
        return;

    va_start(args, fmt);
    edge_alert_log_v(LOG_LEVEL_DEBUG, fmt, args, 0);
    va_end(args);
}

/*************************************************************************/
/** Function Name:       edge_alert_log_action                             **/
/** Purpose:             记录告警动作日志，写入 dev_log；-d 1 时同时输出 stderr **/
/*************************************************************************/
void edge_alert_log_action(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    edge_alert_log_v(LOG_LEVEL_INFO, fmt, args, 1);
    va_end(args);
}
