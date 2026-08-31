#include "netmgr_v4_config.h"

int nm4_config_load_wifi_from_v3_json(const char *path, nm4_wpa_params_t *out, char *err, size_t errlen)
{
    return nm4_config_load_wifi_from_network_json(path, out, err, errlen);
}

int nm4_config_list_enabled_ifaces(
    const char *path, char ifnames[NM4_NETLIST_MAX][NM4_IFNAME_MAX], int *nout, char *err, size_t errlen)
{
    return nm4_config_list_enabled_ifaces_network(path, ifnames, nout, err, errlen);
}

int nm4_config_load_ppp_from_v3_json(const char *path, nm4_ppp_params_t *out, char *err, size_t errlen)
{
    return nm4_config_load_ppp_from_network_json(path, out, err, errlen);
}
