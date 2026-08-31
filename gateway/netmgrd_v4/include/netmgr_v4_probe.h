/* 按网卡探测：存在性 / 链路 / IPv4 / ICMP 公网 */
#ifndef NETMGR_V4_PROBE_H
#define NETMGR_V4_PROBE_H

#include <stddef.h>

#define NM4_PROBE_MAX_TARGETS 8

typedef struct {
    int  timeout_ms;
    int  target_count; /* 0 表示用默认 8.8.8.8 与可选 $GATEWAY */
    int  auto_gateway; /* target_count==0 时：解析不到网关是否追加探测 */
    char targets[NM4_PROBE_MAX_TARGETS][64];
} nm4_probe_params_t;

typedef struct {
    int if_exists; /* /sys/class/net/<if> 存在 */
    int link_up;   /* carrier 或 UP+RUNNING/POINTOPOINT */
    int ipv4_ok;   /* 有 IPv4 地址 */
    int internet_ok; /* ICMP 命中任一目标 */
} nm4_probe_result_t;

/*********************************************************************************************************
** Function name:       nm4_probe_init
** Descriptions:        Initialize probe subsystem (PRNG, stop flag); see src/probe.c.
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_probe_init(void);

/*********************************************************************************************************
** Function name:       nm4_probe_request_stop
** Descriptions:        Request cooperative stop of in-flight probes; see src/probe.c.
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_probe_request_stop(void);

/*********************************************************************************************************
** Function name:       nm4_probe_params_default
** Descriptions:        Fill default probe parameters; see src/probe.c.
** input parameters:    p       - struct to initialize.
** output parameters:    p       - defaults written.
** Returned value:      NONE
*********************************************************************************************************/
void nm4_probe_params_default(nm4_probe_params_t *p);

/*********************************************************************************************************
** Function name:       nm4_if_exists
** Descriptions:        Return 1 if /sys/class/net/<ifname> exists; see src/probe.c.
** input parameters:    ifname  - interface name.
** output parameters:   NONE
** Returned value:      1 or 0.
*********************************************************************************************************/
int  nm4_if_exists(const char *ifname);

/*********************************************************************************************************
** Function name:       nm4_if_link_up
** Descriptions:        Cheap L1 check: carrier or IFF_UP+RUNNING; see nm4_probe_link in src/probe.c.
** input parameters:    ifname  - interface name.
** output parameters:   NONE
** Returned value:      1 if link appears up, else 0.
*********************************************************************************************************/
int  nm4_if_link_up(const char *ifname);

/*********************************************************************************************************
** Function name:       nm4_probe_result_clear
** Descriptions:        Zero all fields of out before composing stepwise probes; see src/probe.c.
** input parameters:    out     - result struct (may be NULL, no-op).
** output parameters:    out     - all fields set to 0.
** Returned value:      NONE
*********************************************************************************************************/
void nm4_probe_result_clear(nm4_probe_result_t *out);

/*********************************************************************************************************
** Function name:       nm4_probe_if_exists
** Descriptions:        Set out->if_exists only (/sys/class/net); see src/probe.c.
** input parameters:    ifname  - interface name.
** output parameters:    out     - if_exists updated.
** Returned value:      0 on success, -1 on bad args.
*********************************************************************************************************/
int  nm4_probe_if_exists(const char *ifname, nm4_probe_result_t *out);

/*********************************************************************************************************
** Function name:       nm4_probe_link
** Descriptions:        Set out->link_up only (carrier or IFF flags); see src/probe.c.
** input parameters:    ifname  - interface name.
** output parameters:    out     - link_up updated.
** Returned value:      0 on success, -1 on bad args.
*********************************************************************************************************/
int  nm4_probe_link(const char *ifname, nm4_probe_result_t *out);

/*********************************************************************************************************
** Function name:       nm4_probe_ipv4
** Descriptions:        Set out->ipv4_ok from out->link_up && has IPv4; see src/probe.c.
** input parameters:    ifname  - interface name.
** output parameters:    out     - ipv4_ok updated (reads out->link_up).
** Returned value:      0 on success, -1 on bad args.
*********************************************************************************************************/
int  nm4_probe_ipv4(const char *ifname, nm4_probe_result_t *out);

/*********************************************************************************************************
** Function name:       nm4_probe_icmp
** Descriptions:        Set out->internet_ok via ICMP to built targets; see src/probe.c.
** input parameters:    ifname  - interface name.
**                      p       - probe params (must be non-NULL).
** output parameters:    out     - internet_ok updated; L1/L2 fields unchanged.
** Returned value:      0 on success, -1 on bad args.
*********************************************************************************************************/
int  nm4_probe_icmp(const char *ifname, const nm4_probe_params_t *p, nm4_probe_result_t *out);

/*********************************************************************************************************
** Function name:       nm4_probe_one
** Descriptions:        Full chain: clear, if_exists, link, ipv4, icmp; see src/probe.c.
** input parameters:    ifname  - interface name.
**                      p       - probe params (NULL for defaults).
** output parameters:    out     - probe results.
** Returned value:      0 on normal completion, -1 on bad args.
*********************************************************************************************************/
int  nm4_probe_one(const char *ifname, const nm4_probe_params_t *p, nm4_probe_result_t *out);

#endif
