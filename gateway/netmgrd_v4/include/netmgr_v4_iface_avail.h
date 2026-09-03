#ifndef NETMGR_V4_IFACE_AVAIL_H
#define NETMGR_V4_IFACE_AVAIL_H

#include "netmgr_v4_policy.h"
#include "netmgr_v4_probe.h"

typedef enum {
    NM4_IFACE_ETH0 = 0,
    NM4_IFACE_WLAN0,
    NM4_IFACE_PPP0
} nm4_iface_id_t;

typedef struct {
    int up_th;
    int down_th;
} nm4_iface_avail_cfg;

typedef struct {
    nm4_iface_id_t id;
    nm4_iface_avail_cfg cfg;
    int raw;
    int up_streak;
    int down_streak;
    int available;
} nm4_iface_avail_t;

/*********************************************************************************************************
** Function name:       nm4_iface_avail_init
** Descriptions:        Init per-interface asymmetric debounce cfg (eth/wlan fast-up, ppp slow-up).
** input parameters:    s       - state to initialize.
**                      id      - NM4_IFACE_ETH0 / WLAN0 / PPP0.
** output parameters:    s       - zeroed with cfg filled.
** Returned value:      NONE
*********************************************************************************************************/
void nm4_iface_avail_init(nm4_iface_avail_t *s, nm4_iface_id_t id);

/*********************************************************************************************************
** Function name:       nm4_iface_avail_update
** Descriptions:        Evaluate probe snapshot and apply asymmetric up/down streak debounce.
** input parameters:    s       - per-interface availability state.
**                      probe   - latest probe result for this interface.
** output parameters:    s       - raw, streaks, available updated.
** Returned value:      NONE
*********************************************************************************************************/
void nm4_iface_avail_update(nm4_iface_avail_t *s, const nm4_probe_result_t *probe);

/*********************************************************************************************************
** Function name:       nm4_iface_avail_to_bits
** Descriptions:        Pack debounced available flags into nm4_bits_t for policy arbitration.
** input parameters:    eth/wlan/ppp - three interface availability states.
** output parameters:    out     - eth_up, wlan_up, ppp_up set from available fields.
** Returned value:      NONE
*********************************************************************************************************/
void nm4_iface_avail_to_bits(
    const nm4_iface_avail_t *eth,
    const nm4_iface_avail_t *wlan,
    const nm4_iface_avail_t *ppp,
    nm4_bits_t *out);

#endif
