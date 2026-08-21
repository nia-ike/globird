/* 读取与 netmgrd_v3 同结构的 JSON（默认 /ech/bin/netmgr_v3.json） */
#ifndef NETMGR_V4_CONFIG_H
#define NETMGR_V4_CONFIG_H

#include <stddef.h>
#include "netmgr_v4_adapter.h"

#define NM4_CONFIG_DEFAULT_PATH "/ech/bin/netmgr_v3.json"
#define NM4_GW_CONF_PATH "/ems/config/gw_conf.json"

#define NM4_NETLIST_MAX 16

/*********************************************************************************************************
** Function name:       nm4_config_list_enabled_ifaces
** Descriptions:        Collect enabled interface names from v3 JSON; see src/config_v3json.c.
** input parameters:    path    - JSON file path.
**                      errlen  - error buffer size.
** output parameters:    ifnames - array of ifnames written.
**                      nout    - count written.
**                      err     - error message on failure.
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_config_list_enabled_ifaces(
    const char *path, char ifnames[NM4_NETLIST_MAX][NM4_IFNAME_MAX], int *nout, char *err, size_t errlen);

/*********************************************************************************************************
** Function name:       nm4_config_load_wifi_from_v3_json
** Descriptions:        Load first enabled wifi_sta (or gw_conf override); see src/config_v3json.c.
** input parameters:    path    - v3 JSON path.
**                      errlen  - error buffer size.
** output parameters:    out     - WPA params filled.
**                      err     - error message on failure.
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_config_load_wifi_from_v3_json(const char *path, nm4_wpa_params_t *out, char *err, size_t errlen);

/*********************************************************************************************************
** Function name:       nm4_config_load_ppp_from_v3_json
** Descriptions:        Load first enabled ppp interface from v3 JSON; see src/config_v3json.c.
** input parameters:    path    - v3 JSON path.
**                      errlen  - error buffer size.
** output parameters:    out     - PPP params filled.
**                      err     - error message on failure.
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_config_load_ppp_from_v3_json(const char *path, nm4_ppp_params_t *out, char *err, size_t errlen);

#endif
