/* 网关解析：run 文件 → 主表 netlink → network.json，与探测目标一致 */
#ifndef NETMGR_V4_GATEWAY_H
#define NETMGR_V4_GATEWAY_H

#include <stddef.h>

#define NM4_NETWORK_JSON "/userdata/config/project/network.json"

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
