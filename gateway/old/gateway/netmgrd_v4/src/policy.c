#include "netmgr_v4_policy.h"

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
** Function name:       nm4_policy_pick_want
** Descriptions:        Choose preferred default-route interface: eth, then wlan, then ppp.
** input parameters:    b       - pointer to debounced availability bits.
** output parameters:   NONE
** Returned value:      NM4_WANT_* enum value for the chosen interface or NONE.
*********************************************************************************************************/
nm4_want_t nm4_policy_pick_want(const nm4_bits_t *b)
{
    if (b->eth_up) return NM4_WANT_ETH0;
    if (b->wlan_up) return NM4_WANT_WLAN0;
    if (b->ppp_up) return NM4_WANT_PPP0;
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
