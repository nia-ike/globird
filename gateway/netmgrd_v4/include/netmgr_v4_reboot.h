#ifndef NETMGR_V4_REBOOT_H
#define NETMGR_V4_REBOOT_H

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
int nm4_request_system_reboot(const char *reason);

#endif
