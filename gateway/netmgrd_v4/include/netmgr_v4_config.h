/* network.json 配置（/userdata/config/project/network.json） */
#ifndef NETMGR_V4_CONFIG_H
#define NETMGR_V4_CONFIG_H

#include <stddef.h>
#include "netmgr_v4_adapter.h"
#include "netmgr_v4_probe.h"

#define NM4_NETWORK_JSON        "/userdata/config/project/network.json"
#define NM4_CONFIG_DEFAULT_PATH NM4_NETWORK_JSON
#define NM4_NETMGR_RUN_DIR      "/var/run/netmgr"

#define NM4_NETLIST_MAX 16

int nm4_config_list_enabled_ifaces(
    const char *path, char ifnames[NM4_NETLIST_MAX][NM4_IFNAME_MAX], int *nout, char *err, size_t errlen);

int nm4_config_load_wifi_from_v3_json(const char *path, nm4_wpa_params_t *out, char *err, size_t errlen);
int nm4_config_load_ppp_from_v3_json(const char *path, nm4_ppp_params_t *out, char *err, size_t errlen);

int nm4_config_load_wifi_from_network_json(const char *path, nm4_wpa_params_t *out, char *err, size_t errlen);
int nm4_config_load_ppp_from_network_json(const char *path, nm4_ppp_params_t *out, char *err, size_t errlen);
int nm4_config_list_enabled_ifaces_network(
    const char *path, char ifnames[NM4_NETLIST_MAX][NM4_IFNAME_MAX], int *nout, char *err, size_t errlen);

/* 探测目标：转发云端地址 + 固定公网兜底（不读 network.json probe 段） */
void nm4_config_fill_probe_params(nm4_probe_params_t *out);

/* rule.prior，缺省等同 ppp */
int nm4_config_load_rule_prior(const char *path, char *prior, size_t n);

/* 将 eth0 静态 gateway/dns 写入 /var/run/netmgr 缓存供 route/dns 模块使用 */
int nm4_config_write_eth_runtime_cache(const char *path);

#endif
