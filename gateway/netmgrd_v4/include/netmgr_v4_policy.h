#ifndef NETMGR_V4_POLICY_H
#define NETMGR_V4_POLICY_H

#include <stdint.h>

#include "netmgr_v4_probe.h"

typedef enum {
    NM4_WANT_NONE = 0,
    NM4_WANT_ETH0,
    NM4_WANT_WLAN0,
    NM4_WANT_PPP0
} nm4_want_t;

typedef struct {
    int eth_up;
    int wlan_up;
    int ppp_up;
} nm4_bits_t;

/*********************************************************************************************************
** Function name:       nm4_policy_to_r
** Descriptions:        Pack availability bits into 3-bit R; see src/policy.c.
** input parameters:    b       - bits struct.
** output parameters:   NONE
** Returned value:      R value 0..7.
*********************************************************************************************************/
uint8_t nm4_policy_to_r(const nm4_bits_t *b);

/*********************************************************************************************************
** Function name:       nm4_policy_pick_want
** Descriptions:        Pick WAN by eth>wlan>ppp; eth LAN-only when wlan+ppp down; see policy.c.
** input parameters:    b           - debounced bits.
**                      eth_probe   - latest eth0 probe for LAN fallback (NULL to skip).
** output parameters:   NONE
** Returned value:      NM4_WANT_* enum.
*********************************************************************************************************/
nm4_want_t nm4_policy_pick_want(const nm4_bits_t *b, const nm4_probe_result_t *eth_probe);

/*********************************************************************************************************
** Function name:       nm4_policy_want_name
** Descriptions:        String name for want enum; see src/policy.c.
** input parameters:    w       - NM4_WANT_* value.
** output parameters:   NONE
** Returned value:      Pointer to static string.
*********************************************************************************************************/
const char *nm4_policy_want_name(nm4_want_t w);
void nm4_policy_load_prior(const char *path);

#endif
