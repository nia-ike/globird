#ifndef NETMGR_V4_MANAGER_H
#define NETMGR_V4_MANAGER_H

#include <stdbool.h>

typedef enum {
    NM4_MANAGER_VIEW_LOG = 0,
    NM4_MANAGER_VIEW_TABLE
} nm4_manager_view_t;

/*********************************************************************************************************
** Function name:       nm4_manager_run
** Descriptions:        Managed-mode main loop (-M); see src/manager.c.
** input parameters:    cfgpath - netmgr JSON path.
**                      debug   - enable logging when not in table mode.
**                      view    - log or TTY table mode.
** output parameters:   NONE
** Returned value:      0 when loop exits normally.
*********************************************************************************************************/
int nm4_manager_run(const char *cfgpath, bool debug, nm4_manager_view_t view);

void nm4_manager_request_arbitrate(void);

/*********************************************************************************************************
** Function name:       nm4_manager_request_apply
** Descriptions:        Request in-process network.json apply (same as touch apply flag).
*********************************************************************************************************/
void nm4_manager_request_apply(void);

/*********************************************************************************************************
** Function name:       nm4_manager_apply_pending
** Descriptions:        Return non-zero if apply request is pending.
*********************************************************************************************************/
int nm4_manager_apply_pending(void);

#endif
