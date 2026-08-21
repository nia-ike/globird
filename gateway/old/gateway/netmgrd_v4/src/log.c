#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "netmgr_v4.h"

static bool s_debug;

/*********************************************************************************************************
** Function name:       netmgr_v4_log_init
** Descriptions:        Enable or disable debug logging for netmgrd_v4.
** input parameters:    debug   - true to print logs, false to silence them.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void netmgr_v4_log_init(bool debug) { s_debug = debug; }

/*********************************************************************************************************
** Function name:       netmgr_v4_log
** Descriptions:        Print a timestamped debug line to stderr when debug mode is on.
** input parameters:    fmt     - printf-style format string.
**                      ...     - format arguments.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void netmgr_v4_log(const char *fmt, ...)
{
    if (!s_debug) return;
    va_list ap;
    time_t t = time(NULL);
    struct tm tmv;
    char buf[32];
    localtime_r(&t, &tmv);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
    fprintf(stderr, "[%s] ", buf);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}
