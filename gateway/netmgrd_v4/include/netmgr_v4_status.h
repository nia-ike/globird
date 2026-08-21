#ifndef NETMGR_V4_STATUS_H
#define NETMGR_V4_STATUS_H

#include <stdint.h>
#include "netmgr_v4_linkkeeper.h"
#include "netmgr_v4_policy.h"
#include "netmgr_v4_probe.h"

#define NM4_STATUS_DIR "/var/run/netmgr"
#define NM4_STATUS_JSON "/var/run/netmgr/status.json"

typedef struct {
    uint64_t now_ms;
    uint64_t tick;
    uint8_t r;
    nm4_want_t want;
    nm4_want_t have;
    int pending;
    const nm4_bits_t *bits;
    const nm4_probe_result_t *eth;
    const nm4_probe_result_t *wlan;
    const nm4_probe_result_t *ppp;
    const nm4_linkkeeper_t *lk;
} nm4_status_snapshot_t;

/*********************************************************************************************************
** Function name:       nm4_status_write
** Descriptions:        Atomically write status JSON snapshot; see src/status.c.
** input parameters:    s       - snapshot pointers (must all be non-NULL).
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_status_write(const nm4_status_snapshot_t *s);

#endif
