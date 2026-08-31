/* DNS 管理 - 保底 DNS 配置 */
#ifndef NETMGR_V4_DNS_H
#define NETMGR_V4_DNS_H

#include <stddef.h>
#include <stdbool.h>

#include "netmgr_v4_policy.h"

#define NM4_DNS_FALLBACK_COUNT 1
#define NM4_DNS_GLOBAL_FALLBACK "8.8.8.8"
#define NM4_RESOLV_CONF "/etc/resolv.conf"
#define NM4_RESOLV_CONF_TMP "/etc/resolv.conf.tmp"

/*********************************************************************************************************
** Function name:       nm4_dns_ensure_fallback
** Descriptions:        Ensure fallback DNS (8.8.8.8) is present after WAN DNS in resolv.conf.
**                      Reads current resolv.conf, removes duplicates of fallback servers,
**                      prepends them, and writes back atomically.
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_dns_ensure_fallback(void);

/*********************************************************************************************************
** Function name:       nm4_dns_set_fallback_servers
** Descriptions:        Set custom fallback DNS servers (default: 8.8.8.8).
** input parameters:    servers - array of DNS server IP strings.
**                      count   - number of servers (max NM4_DNS_FALLBACK_COUNT).
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_dns_set_fallback_servers(const char *servers[], size_t count);

/*********************************************************************************************************
** Function name:       nm4_dns_get_fallback_servers
** Descriptions:        Get current fallback DNS server list.
** input parameters:    NONE
** output parameters:   servers - array to receive server strings (must have room for NM4_DNS_FALLBACK_COUNT).
**                      count   - number of servers returned.
** Returned value:      0 on success.
*********************************************************************************************************/
int nm4_dns_get_fallback_servers(const char *servers[], size_t *count);

/*********************************************************************************************************
** Function name:       nm4_dns_cache_write
** Descriptions:        Write DNS server list to /var/run/netmgr/dns.<ifname> (one IP per line).
** input parameters:    ifname  - interface name.
**                      servers - array of IP strings (may be NULL if count is 0).
**                      count   - number of servers.
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_dns_cache_write(const char *ifname, const char *servers[], size_t count);

/*********************************************************************************************************
** Function name:       nm4_dns_cache_clear
** Descriptions:        Remove DNS cache file for interface.
** input parameters:    ifname  - interface name.
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_dns_cache_clear(const char *ifname);

/*********************************************************************************************************
** Function name:       nm4_dns_apply
** Descriptions:        Rebuild resolv.conf from active WAN DNS cache plus fallback servers.
** input parameters:    want    - current default-route want (selects dns.<if> source).
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_dns_apply(nm4_want_t want);

#endif
