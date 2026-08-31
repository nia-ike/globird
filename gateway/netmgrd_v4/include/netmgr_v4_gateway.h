/* 网关解析：run 文件 → 主表 netlink → network.json，与探测目标一致 */
#ifndef NETMGR_V4_GATEWAY_H
#define NETMGR_V4_GATEWAY_H

#include <stddef.h>

#define NM4_NETWORK_JSON "/userdata/config/project/network.json"
#define NM4_NETMGR_RUN_DIR "/var/run/netmgr"
#define NM4_ARBITRATE_FLAG NM4_NETMGR_RUN_DIR "/arbitrate"

/*********************************************************************************************************
** Function name:       nm4_run_dir_ensure
** Descriptions:        Create /var/run/netmgr if missing.
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_run_dir_ensure(void);

/*********************************************************************************************************
** Function name:       nm4_gw_cache_write
** Descriptions:        Write gateway IPv4 to /var/run/netmgr/gateway.<ifname>.
** input parameters:    ifname  - interface name.
**                      gw      - dotted-quad gateway (NULL clears cache).
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_gw_cache_write(const char *ifname, const char *gw);

/*********************************************************************************************************
** Function name:       nm4_gw_cache_clear
** Descriptions:        Remove gateway cache file for interface.
** input parameters:    ifname  - interface name.
** output parameters:   NONE
** Returned value:      0 on success, -1 if missing or error.
*********************************************************************************************************/
int nm4_gw_cache_clear(const char *ifname);

/*********************************************************************************************************
** Function name:       nm4_gw_resolve
** Descriptions:        Resolve IPv4 default gateway for interface; see src/gateway.c.
** input parameters:    ifname  - interface name.
**                      gwlen   - size of gw buffer.
** output parameters:    gw      - dotted-quad string on success.
** Returned value:      0 on success, -1 on failure.
*********************************************************************************************************/
int nm4_gw_resolve(const char *ifname, char *gw, size_t gwlen);

#endif
