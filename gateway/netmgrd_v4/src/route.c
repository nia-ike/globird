#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "netmgr_v4_route.h"
#include "netmgr_v4_gateway.h"
#include "netmgr_v4_dns.h"

/*********************************************************************************************************
** Function name:       run_cmd
** Descriptions:        Execute shell command and return whether exit status is zero.
** input parameters:    cmd     - shell command line.
** output parameters:   NONE
** Returned value:      Return value of system(cmd).
*********************************************************************************************************/
static int run_cmd(const char *cmd)
{
    return system(cmd);
}

/*********************************************************************************************************
** Function name:       nm4_route_get_have
** Descriptions:        Parse main table default route from "ip route show default" for eth0/wlan0/ppp0.
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NM4_WANT_* matching current default dev, or NONE if not found.
*********************************************************************************************************/
nm4_want_t nm4_route_get_have(void)
{
    FILE *fp = popen("ip route show default 2>/dev/null", "r");
    char line[256];
    if (!fp) return NM4_WANT_NONE;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, " dev eth0")) {
            (void)pclose(fp);
            return NM4_WANT_ETH0;
        }
        if (strstr(line, " dev wlan0")) {
            (void)pclose(fp);
            return NM4_WANT_WLAN0;
        }
        if (strstr(line, " dev ppp0")) {
            (void)pclose(fp);
            return NM4_WANT_PPP0;
        }
    }
    (void)pclose(fp);
    return NM4_WANT_NONE;
}

/*********************************************************************************************************
** Function name:       nm4_route_apply
** Descriptions:        Set or clear IPv4 default route to match desired WAN (metric 10).
** input parameters:    want    - desired default interface enum.
** output parameters:   NONE
** Returned value:      0 on success for known want; -1 on failure or unknown want.
*********************************************************************************************************/
int nm4_route_apply(nm4_want_t want)
{
    char cmd[256];
    char gw[64];
    int ret = -1;

    if (want == NM4_WANT_NONE) {
        int i;
        for (i = 0; i < 8; i++) {
            if (run_cmd("ip route del default 2>/dev/null") != 0)
                break;
        }
        ret = 0;
    } else if (want == NM4_WANT_PPP0) {
        snprintf(cmd, sizeof(cmd), "ip route replace default dev ppp0 metric 10");
        ret = run_cmd(cmd) == 0 ? 0 : -1;
    } else if (want == NM4_WANT_ETH0) {
        if (nm4_gw_resolve("eth0", gw, sizeof(gw)) == 0) {
            snprintf(cmd, sizeof(cmd),
                     "ip route replace default via %s dev eth0 metric 10", gw);
        } else {
            snprintf(cmd, sizeof(cmd),
                     "ip route replace default dev eth0 metric 10");
        }
        ret = run_cmd(cmd) == 0 ? 0 : -1;
    } else if (want == NM4_WANT_WLAN0) {
        if (nm4_gw_resolve("wlan0", gw, sizeof(gw)) == 0) {
            snprintf(cmd, sizeof(cmd),
                     "ip route replace default via %s dev wlan0 metric 10", gw);
        } else {
            snprintf(cmd, sizeof(cmd),
                     "ip route replace default dev wlan0 metric 10");
        }
        ret = run_cmd(cmd) == 0 ? 0 : -1;
    }

    /* Ensure fallback DNS servers are always at the top of resolv.conf
     * This prevents DNS issues when switching between different network interfaces
     * with different DNS servers provided by DHCP/PPP.
     */
    if (ret == 0) {
        (void)nm4_dns_ensure_fallback();
    }

    return ret;
}

/*********************************************************************************************************
** Function name:       nm4_route_repair_for_probe
** Descriptions:        Replace main default route for probe repair (legacy helper; use with care).
** input parameters:    ifname  - eth0, wlan0, or ppp0.
** output parameters:   NONE
** Returned value:      0 on success, -1 on unsupported ifname or command failure.
*********************************************************************************************************/
int nm4_route_repair_for_probe(const char *ifname)
{
    char cmd[256];
    char gw[64];

    if (!ifname || !ifname[0]) return -1;

    if (strcmp(ifname, "ppp0") == 0) {
        snprintf(cmd, sizeof(cmd), "ip route replace default dev ppp0 metric 10");
        return run_cmd(cmd) == 0 ? 0 : -1;
    }

    if (strcmp(ifname, "eth0") == 0 || strcmp(ifname, "wlan0") == 0) {
        if (nm4_gw_resolve(ifname, gw, sizeof(gw)) == 0) {
            snprintf(cmd, sizeof(cmd),
                     "ip route replace default via %s dev %s metric 10", gw, ifname);
        } else {
            snprintf(cmd, sizeof(cmd),
                     "ip route replace default dev %s metric 10", ifname);
        }
        return run_cmd(cmd) == 0 ? 0 : -1;
    }

    return -1;
}

/*********************************************************************************************************
** Function name:       nm4_route_probe_prepare
** Descriptions:        Install host /32 route toward target via ifname for isolated probing.
** input parameters:    ifname  - egress interface.
**                      target  - IPv4 address string for /32 route.
** output parameters:   NONE
** Returned value:      0 on success, -1 on bad args or ip failure.
*********************************************************************************************************/
int nm4_route_probe_prepare(const char *ifname, const char *target)
{
    char cmd[256];
    char gw[64];

    if (!ifname || !ifname[0] || !target || !target[0]) return -1;

    if (strcmp(ifname, "ppp0") == 0) {
        snprintf(cmd, sizeof(cmd), "ip route replace %s/32 dev %s metric 5", target, ifname);
        return run_cmd(cmd) == 0 ? 0 : -1;
    }

    if (strcmp(ifname, "eth0") == 0 || strcmp(ifname, "wlan0") == 0) {
        if (nm4_gw_resolve(ifname, gw, sizeof(gw)) == 0) {
            snprintf(cmd, sizeof(cmd),
                     "ip route replace %s/32 via %s dev %s metric 5", target, gw, ifname);
        } else {
            snprintf(cmd, sizeof(cmd),
                     "ip route replace %s/32 dev %s metric 5", target, ifname);
        }
        return run_cmd(cmd) == 0 ? 0 : -1;
    }

    return -1;
}

/*********************************************************************************************************
** Function name:       nm4_route_probe_cleanup
** Descriptions:        Remove host /32 route for target (best-effort, dev-specific then generic).
** input parameters:    ifname  - interface used in original replace.
**                      target  - IPv4 address string.
** output parameters:   NONE
** Returned value:      Always 0.
*********************************************************************************************************/
int nm4_route_probe_cleanup(const char *ifname, const char *target)
{
    char cmd[256];
    if (!ifname || !ifname[0] || !target || !target[0]) return -1;
    snprintf(cmd, sizeof(cmd), "ip route del %s/32 dev %s 2>/dev/null", target, ifname);
    (void)run_cmd(cmd);
    snprintf(cmd, sizeof(cmd), "ip route del %s/32 2>/dev/null", target);
    (void)run_cmd(cmd);
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_route_probe_ensure_policy
** Descriptions:        Ensure ip rule fwmark->table and per-table default for marked probe traffic.
** input parameters:    ifname  - eth0, wlan0, or ppp0 (selects mark and table id).
** output parameters:   NONE
** Returned value:      0 on success, -1 on unknown ifname or ip failure.
*********************************************************************************************************/
int nm4_route_probe_ensure_policy(const char *ifname)
{
    char cmd[256];
    char gw[64];
    int table = 0;
    unsigned mark = 0;
    int prio = 0;

    if (!ifname || !ifname[0]) return -1;

    if (strcmp(ifname, "eth0") == 0) {
        table = 100; mark = 0x64u; prio = 10064;
    } else if (strcmp(ifname, "wlan0") == 0) {
        table = 101; mark = 0x65u; prio = 10065;
    } else if (strcmp(ifname, "ppp0") == 0) {
        table = 102; mark = 0x66u; prio = 10066;
    } else {
        return -1;
    }

    snprintf(cmd, sizeof(cmd), "ip rule add fwmark 0x%x lookup %d priority %d 2>/dev/null || true",
             mark, table, prio);
    (void)run_cmd(cmd);

    if (strcmp(ifname, "ppp0") == 0) {
        snprintf(cmd, sizeof(cmd), "ip route replace table %d default dev %s metric 10", table, ifname);
        return run_cmd(cmd) == 0 ? 0 : -1;
    }

    if (nm4_gw_resolve(ifname, gw, sizeof(gw)) == 0) {
        snprintf(cmd, sizeof(cmd),
                 "ip route replace table %d default via %s dev %s metric 10",
                 table, gw, ifname);
    } else {
        snprintf(cmd, sizeof(cmd),
                 "ip route replace table %d default dev %s metric 10",
                 table, ifname);
    }
    return run_cmd(cmd) == 0 ? 0 : -1;
}
