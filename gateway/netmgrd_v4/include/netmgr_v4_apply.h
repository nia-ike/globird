/* netmgrd 原地 apply：touch /var/run/netmgr/apply 触发（与 arbitrate 同款），结果写 apply.result */
#ifndef NETMGR_V4_APPLY_H
#define NETMGR_V4_APPLY_H

#include "netmgr_v4_linkkeeper.h"
#include "netmgr_v4_gateway.h"

#define NM4_PIDFILE          "/var/run/netmgrd.pid"
#define NM4_APPLY_FLAG       NM4_NETMGR_RUN_DIR "/apply"
#define NM4_APPLY_RESULT     NM4_NETMGR_RUN_DIR "/apply.result"
#define NM4_APPLY_WAIT_MS    30000

/*********************************************************************************************************
** Function name:       nm4_config_apply_runtime
** Descriptions:        Re-read network.json, validate, diff-apply wifi/ppp into lk, refresh eth cache.
**                      Writes NM4_APPLY_RESULT. Does not change process PID.
** input parameters:    cfgpath - JSON path (same as -M -C).
**                      lk      - live linkkeeper state to update.
** output parameters:   lk      - has_wifi/has_ppp and params updated on success path.
** Returned value:      0 if apply committed (even with per-iface best-effort fails logged in result),
**                      -1 if validation failed (runtime unchanged except result file).
*********************************************************************************************************/
int nm4_config_apply_runtime(const char *cfgpath, nm4_linkkeeper_t *lk);

/*********************************************************************************************************
** Function name:       nm4_apply_client
** Descriptions:        CLI: touch NM4_APPLY_FLAG and wait for apply.result (daemon -M must be running).
** input parameters:    timeout_ms - max wait (0 = NM4_APPLY_WAIT_MS).
** output parameters:   NONE
** Returned value:      0 if result ok=true, 1 on failure/timeout/no daemon.
*********************************************************************************************************/
int nm4_apply_client(unsigned timeout_ms);

#endif
