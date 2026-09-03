/*************************************************************************/
/** reboot.c — 整机重启：优先 db_process 落盘，消息队列失败则直接 reboot          **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "netmgr_v4.h"
#include "netmgr_v4_reboot.h"
#include "db_msgevent_api.h"

#define NM4_REBOOT_MSG_FALLBACK_DELAY_SEC  5

/*********************************************************************************************************
** Function name:       nm4_reboot_fallback_direct
** Descriptions:        Sleep then sync and call system reboot when db message path failed.
** input parameters:    reason  - short tag for logs (may be NULL).
** output parameters:   NONE
** Returned value:      -1 (caller should treat reboot as attempted).
*********************************************************************************************************/
static int nm4_reboot_fallback_direct(const char *reason)
{
    netmgr_v4_log("reboot: %s, fallback in %d s (sync + system reboot)",
                  reason ? reason : "requested", NM4_REBOOT_MSG_FALLBACK_DELAY_SEC);
    sleep(NM4_REBOOT_MSG_FALLBACK_DELAY_SEC);
    sync();
    (void)system("reboot -d 2");
    return -1;
}

/*********************************************************************************************************
** Function name:       nm4_request_system_reboot
** Descriptions:        Request whole-system reboot via db_process (save then reboot). If the message
**                      queue is unavailable or DB_EVENT_TYPE_REBOOT post fails, sleep 5 seconds,
**                      sync, and invoke system reboot as fallback.
** input parameters:    reason  - short tag for logs (may be NULL).
** output parameters:   NONE
** Returned value:      0 if reboot message posted to db_process; -1 if fallback reboot was attempted
**                      (normally does not return when reboot succeeds).
*********************************************************************************************************/
int nm4_request_system_reboot(const char *reason)
{
    if (db_msg_event_api_init() == 0) {
        if (db_msg_event_cmd(DB_EVENT_TYPE_REBOOT) == 0) {
            netmgr_v4_log("reboot: %s, db_process save+reboot posted",
                          reason ? reason : "requested");
            return 0;
        }
        netmgr_v4_log("reboot: %s, db reboot msg send failed", reason ? reason : "requested");
    } else {
        netmgr_v4_log("reboot: %s, db msg queue unavailable", reason ? reason : "requested");
    }

    return nm4_reboot_fallback_direct(reason);
}
