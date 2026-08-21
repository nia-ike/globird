#ifndef NETMGR_V4_ROUTE_H
#define NETMGR_V4_ROUTE_H

#include "netmgr_v4_policy.h"

/*********************************************************************************************************
** Function name:       nm4_route_get_have
** Descriptions:        Read current default route device as NM4_WANT_*; see src/route.c.
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NM4_WANT_* for eth0/wlan0/ppp0 or NONE.
*********************************************************************************************************/
nm4_want_t nm4_route_get_have(void);

/*********************************************************************************************************
** Function name:       nm4_route_apply
** Descriptions:        Replace or delete IPv4 default route for want; see src/route.c.
** input parameters:    want    - desired default WAN.
** output parameters:   NONE
** Returned value:      0 on success, -1 on failure.
*********************************************************************************************************/
int nm4_route_apply(nm4_want_t want);

/*********************************************************************************************************
** Function name:       nm4_route_repair_for_probe
** Descriptions:        Set main default route for probe repair path; see src/route.c.
** input parameters:    ifname  - eth0, wlan0, or ppp0.
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_route_repair_for_probe(const char *ifname);

/*********************************************************************************************************
** Function name:       nm4_route_probe_prepare
** Descriptions:        Add /32 host route for probe target via interface; see src/route.c.
** input parameters:    ifname  - egress interface.
**                      target  - IPv4 host string.
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_route_probe_prepare(const char *ifname, const char *target);

/*********************************************************************************************************
** Function name:       nm4_route_probe_cleanup
** Descriptions:        Remove /32 probe route for target; see src/route.c.
** input parameters:    ifname  - interface used in prepare.
**                      target  - IPv4 host string.
** output parameters:   NONE
** Returned value:      Always 0.
*********************************************************************************************************/
int nm4_route_probe_cleanup(const char *ifname, const char *target);

/*********************************************************************************************************
** Function name:       nm4_route_probe_ensure_policy
** Descriptions:        Ensure fwmark rule and policy table default for marked probes; see src/route.c.
** input parameters:    ifname  - eth0, wlan0, or ppp0.
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_route_probe_ensure_policy(const char *ifname);

#endif
