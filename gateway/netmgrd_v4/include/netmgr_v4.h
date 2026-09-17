/* netmgrd_v4 — 公共头；实现随迭代填充 */
#ifndef NETMGR_V4_H
#define NETMGR_V4_H

#include <stdbool.h>
#include <stdint.h>

#define NETMGR_V4_VERSION "0.5.2-apply-touch"

/*********************************************************************************************************
** Function name:       netmgr_v4_log_init
** Descriptions:        Enable or disable debug logging for netmgrd_v4 (see src/log.c).
** input parameters:    debug   - true to print logs, false to silence them.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void netmgr_v4_log_init(bool debug);

/*********************************************************************************************************
** Function name:       netmgr_v4_log
** Descriptions:        Print a timestamped debug line when debug mode is on (see src/log.c).
** input parameters:    fmt     - printf-style format string.
**                      ...     - format arguments.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void netmgr_v4_log(const char *fmt, ...);

/*********************************************************************************************************
** Function name:       netmgr_v4_run
** Descriptions:        Placeholder entry when no CLI action is selected (see src/main.c).
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      Always 0.
*********************************************************************************************************/
int netmgr_v4_run(void);

#endif
