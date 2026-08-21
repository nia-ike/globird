#include "edge_log.h"

#include "dev_log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static int s_debug;
static int s_log_ready;

/*************************************************************************/
/** Function Name:       edge_log_v                                        **/
/** Purpose:             写入 dev_log（log_write_string）并可选 stderr       **/
/** Params:            script_id - 脚本 id，NULL 表示 [service]              **/
/**                      level - INFO/WARN/ERROR                           **/
/**                      fmt - 内容格式（用户文本须用 %s 传入）            **/
/** Return:            None.                                               **/
/** Notice:            每条记录末尾追加 \\r\\n，便于 49H 从环形缓冲按行切分          **/
/*************************************************************************/
static void edge_log_v(const char *script_id, const char *level,
                       const char *fmt, va_list args)
{
    char body[512];
    char line[640];
    int n;

    n = vsnprintf(body, sizeof(body), fmt, args);
    if (n < 0)
        return;
    if (n >= (int)sizeof(body))
        body[sizeof(body) - 1] = '\0';

    if (script_id && script_id[0])
        snprintf(line, sizeof(line), "[%s] %s\r\n", script_id, body);
    else
        snprintf(line, sizeof(line), "[service] %s\r\n", body);

    if (s_log_ready)
        log_write_string(EDGE_LOG_CHANNEL, EDGE_LOG_DEV, (char *)level, "%s", line);

    if (s_debug)
        fprintf(stderr, "[edge_script] %s\n", line);
}

/*************************************************************************/
/** Function Name:       edge_log_init                                     **/
/** Purpose:             打开 dev_log 并记录 debug 标志                    **/
/** Params:            debug - 是否同时 printf 到 stderr                   **/
/** Return:            0-成功；-1-log_open 失败                            **/
/** Notice:            进程入口调用一次                                    **/
/*************************************************************************/
int edge_log_init(int debug)
{
    s_debug = debug ? 1 : 0;
    s_log_ready = 0;

    if (log_open(NULL) == NULL)
        return -1;

    s_log_ready = 1;
    return 0;
}

/*************************************************************************/
/** Function Name:       edge_log_shutdown                                 **/
/** Purpose:             关闭 dev_log                                      **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
void edge_log_shutdown(void)
{
    if (!s_log_ready)
        return;

    log_close();
    s_log_ready = 0;
}

static void edge_log_service_v(const char *level, const char *fmt, va_list args)
{
    edge_log_v(NULL, level, fmt, args);
}

static void edge_log_script_v(const char *script_id, const char *level,
                              const char *fmt, va_list args)
{
    edge_log_v(script_id, level, fmt, args);
}

void edge_log_service_info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    edge_log_service_v(LOG_LEVEL_INFO, fmt, args);
    va_end(args);
}

void edge_log_service_warn(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    edge_log_service_v(LOG_LEVEL_WARN, fmt, args);
    va_end(args);
}

void edge_log_service_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    edge_log_service_v(LOG_LEVEL_ERROR, fmt, args);
    va_end(args);
}

void edge_log_script_info(const char *script_id, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    edge_log_script_v(script_id, LOG_LEVEL_INFO, fmt, args);
    va_end(args);
}

void edge_log_script_warn(const char *script_id, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    edge_log_script_v(script_id, LOG_LEVEL_WARN, fmt, args);
    va_end(args);
}

void edge_log_script_error(const char *script_id, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    edge_log_script_v(script_id, LOG_LEVEL_ERROR, fmt, args);
    va_end(args);
}
