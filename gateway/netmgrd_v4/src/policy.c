#include "netmgr_v4_policy.h"
#include "netmgr_v4_probe.h"

/* 固定优先级：eth0 > wlan0 > ppp0（暂不从 network.json 读取 rule.prior） */
static const nm4_want_t g_prior_order[3] = {
    NM4_WANT_ETH0, NM4_WANT_WLAN0, NM4_WANT_PPP0
};

/*********************************************************************************************************
** Function name:       nm4_policy_load_prior
** Descriptions:        Set fixed pick order eth0 > wlan0 > ppp0 (config ignored for now).
** input parameters:    path    - network.json path (unused).
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_policy_load_prior(const char *path)
{
    (void)path;
}

/*********************************************************************************************************
** Function name:       nm4_bit_for_want
** Descriptions:        Map want enum to corresponding bit in availability struct.
** input parameters:    w       - NM4_WANT_* value.
**                      b       - debounced availability bits.
** output parameters:   NONE
** Returned value:      0 or 1 for the selected interface bit.
*********************************************************************************************************/
static int nm4_bit_for_want(nm4_want_t w, const nm4_bits_t *b)
{
    switch (w) {
    case NM4_WANT_ETH0:  return b->eth_up;
    case NM4_WANT_WLAN0: return b->wlan_up;
    case NM4_WANT_PPP0:  return b->ppp_up;
    default: return 0;
    }
}

/*********************************************************************************************************
** Function name:       nm4_policy_to_r
** Descriptions:        Pack eth/wlan/ppp availability bits into one 3-bit value R.
** input parameters:    b       - pointer to bits (eth_up, wlan_up, ppp_up).
** output parameters:   NONE
** Returned value:      R in range 0..7.
*********************************************************************************************************/
uint8_t nm4_policy_to_r(const nm4_bits_t *b)
{
    return (uint8_t)(((b->eth_up ? 1 : 0) << 2) |
                     ((b->wlan_up ? 1 : 0) << 1) |
                     (b->ppp_up ? 1 : 0));
}

/*********************************************************************************************************
** Function name:       eth_lan_fallback_ok
** Descriptions:        eth0 LAN-only fallback: link + IPv4 when wlan/ppp WAN are down.
** input parameters:    p       - latest eth0 probe snapshot.
** output parameters:   NONE
** Returned value:      1 if eth can carry default route as last resort.
*********************************************************************************************************/
static int eth_lan_fallback_ok(const nm4_probe_result_t *p)
{
    if (!p)
        return 0;
    return p->if_exists && p->link_up && p->ipv4_ok;
}

/*********************************************************************************************************
** Function name:       nm4_policy_pick_want
** Descriptions:        Pick WAN: eth>wlan>ppp when debounced up; eth LAN-only if wlan+ppp down.
** input parameters:    b           - debounced availability bits.
**                      eth_probe   - latest eth0 probe (for LAN fallback); NULL skips fallback.
** output parameters:   NONE
** Returned value:      NM4_WANT_* enum value for the chosen interface or NONE.
*********************************************************************************************************/
nm4_want_t nm4_policy_pick_want(const nm4_bits_t *b, const nm4_probe_result_t *eth_probe)
{
    int i;

    if (!b)
        return NM4_WANT_NONE;

    for (i = 0; i < 3; i++) {
        nm4_want_t w = g_prior_order[i];

        if (w == NM4_WANT_ETH0) {
            /* eth WAN: debounced up + live internet_ok; do not block wlan/ppp on stale eth_up */
            if (b->eth_up && eth_probe && eth_probe->internet_ok)
                return NM4_WANT_ETH0;
            continue;
        }
        if (nm4_bit_for_want(w, b))
            return w;
    }
    if (!b->wlan_up && !b->ppp_up && eth_lan_fallback_ok(eth_probe))
        return NM4_WANT_ETH0;
    return NM4_WANT_NONE;
}

/*********************************************************************************************************
** Function name:       nm4_policy_want_name
** Descriptions:        Return a stable string name for a want enum (for logs and JSON).
** input parameters:    w       - NM4_WANT_* value.
** output parameters:   NONE
** Returned value:      Pointer to static string "eth0", "wlan0", "ppp0", or "none".
*********************************************************************************************************/
const char *nm4_policy_want_name(nm4_want_t w)
{
    switch (w) {
    case NM4_WANT_ETH0: return "eth0";
    case NM4_WANT_WLAN0: return "wlan0";
    case NM4_WANT_PPP0: return "ppp0";
    case NM4_WANT_NONE:
    default: return "none";
    }
}
