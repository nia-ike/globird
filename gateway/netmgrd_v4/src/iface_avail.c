#include "netmgr_v4_iface_avail.h"

#include <string.h>

/*********************************************************************************************************
** Function name:       usable_eth
** Descriptions:        eth0 WAN candidate: link + IPv4 + internet_ok (full upstream).
** input parameters:    p       - probe result for eth0.
** output parameters:   NONE
** Returned value:      1 if usable this tick, else 0.
*********************************************************************************************************/
static int usable_eth(const nm4_probe_result_t *p)
{
    if (!p)
        return 0;
    return p->if_exists && p->link_up && p->ipv4_ok && p->internet_ok;
}

/*********************************************************************************************************
** Function name:       usable_wlan
** Descriptions:        wlan0 candidate: link + IPv4 + internet_ok (reject WiFi fake-connect).
** input parameters:    p       - probe result for wlan0.
** output parameters:   NONE
** Returned value:      1 if usable this tick, else 0.
*********************************************************************************************************/
static int usable_wlan(const nm4_probe_result_t *p)
{
    if (!p)
        return 0;
    return p->if_exists && p->link_up && p->ipv4_ok && p->internet_ok;
}

/*********************************************************************************************************
** Function name:       usable_ppp
** Descriptions:        ppp0 candidate: full L3 reachability (metered fallback).
** input parameters:    p       - probe result for ppp0.
** output parameters:   NONE
** Returned value:      1 if usable this tick, else 0.
*********************************************************************************************************/
static int usable_ppp(const nm4_probe_result_t *p)
{
    if (!p)
        return 0;
    return p->if_exists && p->link_up && p->ipv4_ok && p->internet_ok;
}

/*********************************************************************************************************
** Function name:       raw_usable
** Descriptions:        Dispatch usable rule by interface id.
** input parameters:    id      - interface id.
**                      probe   - probe result.
** output parameters:   NONE
** Returned value:      1 if raw usable, else 0.
*********************************************************************************************************/
static int raw_usable(nm4_iface_id_t id, const nm4_probe_result_t *probe)
{
    switch (id) {
    case NM4_IFACE_ETH0:
        return usable_eth(probe);
    case NM4_IFACE_WLAN0:
        return usable_wlan(probe);
    case NM4_IFACE_PPP0:
        return usable_ppp(probe);
    default:
        return 0;
    }
}

/*********************************************************************************************************
** Function name:       nm4_iface_avail_init
** Descriptions:        Init per-interface asymmetric debounce cfg (eth/wlan fast-up, ppp slow-up).
** input parameters:    s       - state to initialize.
**                      id      - NM4_IFACE_ETH0 / WLAN0 / PPP0.
** output parameters:    s       - zeroed with cfg filled.
** Returned value:      NONE
*********************************************************************************************************/
void nm4_iface_avail_init(nm4_iface_avail_t *s, nm4_iface_id_t id)
{
    if (!s)
        return;

    memset(s, 0, sizeof(*s));
    s->id = id;

    switch (id) {
    case NM4_IFACE_ETH0:
        s->cfg.up_th = 1;
        s->cfg.down_th = 3;
        break;
    case NM4_IFACE_WLAN0:
        s->cfg.up_th = 1;
        s->cfg.down_th = 3;
        break;
    case NM4_IFACE_PPP0:
        s->cfg.up_th = 2;
        s->cfg.down_th = 1;
        break;
    default:
        s->cfg.up_th = 1;
        s->cfg.down_th = 1;
        break;
    }
}

/*********************************************************************************************************
** Function name:       nm4_iface_avail_update
** Descriptions:        Evaluate probe snapshot and apply asymmetric up/down streak debounce.
** input parameters:    s       - per-interface availability state.
**                      probe   - latest probe result for this interface.
** output parameters:    s       - raw, streaks, available updated.
** Returned value:      NONE
*********************************************************************************************************/
void nm4_iface_avail_update(nm4_iface_avail_t *s, const nm4_probe_result_t *probe)
{
    if (!s)
        return;

    /* L1 快降：carrier 丢失立即不可用，不等 down_th 防抖 */
    if (!probe || !probe->if_exists || !probe->link_up) {
        s->raw = 0;
        s->available = 0;
        s->down_streak = s->cfg.down_th;
        s->up_streak = 0;
        return;
    }

    s->raw = raw_usable(s->id, probe);
    if (s->raw) {
        s->up_streak++;
        s->down_streak = 0;
        if (s->up_streak >= s->cfg.up_th)
            s->available = 1;
    } else {
        s->down_streak++;
        s->up_streak = 0;
        if (s->down_streak >= s->cfg.down_th)
            s->available = 0;
    }
}

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
    nm4_bits_t *out)
{
    if (!out)
        return;

    out->eth_up = (eth && eth->available) ? 1 : 0;
    out->wlan_up = (wlan && wlan->available) ? 1 : 0;
    out->ppp_up = (ppp && ppp->available) ? 1 : 0;
}
