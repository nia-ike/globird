/*************************************************************************/
/** dns.c — DNS：/var/run/netmgr 缓存 + fallback，由 netmgrd 统一写 resolv.conf      **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include "netmgr_v4_dns.h"
#include "netmgr_v4_gateway.h"

#define NM4_DNS_CACHE_MAX 8

static const char *g_fallback_dns[NM4_DNS_FALLBACK_COUNT] = {
    NM4_DNS_GLOBAL_FALLBACK
};

static bool server_in_list(const char *ip, char servers[][64], int n)
{
    int i;

    if (!ip || !ip[0])
        return false;
    for (i = 0; i < n; i++) {
        if (strcmp(ip, servers[i]) == 0)
            return true;
    }
    return false;
}

/*********************************************************************************************************
** Function name:       strip_newline
** Descriptions:        Remove trailing newline from string (if any).
** input parameters:    s       - null-terminated string (modified in place).
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void strip_newline(char *s)
{
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n')
        s[len - 1] = '\0';
}

/*********************************************************************************************************
** Function name:       dns_cache_path
** Descriptions:        Build path for per-interface DNS cache file.
** input parameters:    ifname  - interface name.
**                      path    - output buffer.
**                      pathlen - buffer size.
** output parameters:    path    - filled path string.
** Returned value:      0 on success, -1 on bad args.
*********************************************************************************************************/
static int dns_cache_path(const char *ifname, char *path, size_t pathlen)
{
    if (!ifname || !ifname[0] || !path || pathlen < 32)
        return -1;
    (void)snprintf(path, pathlen, "%s/dns.%s", NM4_NETMGR_RUN_DIR, ifname);
    return 0;
}

/*********************************************************************************************************
** Function name:       want_ifname
** Descriptions:        Map want enum to interface name for DNS cache lookup.
** input parameters:    want    - NM4_WANT_* value.
** output parameters:   NONE
** Returned value:      Interface name or NULL.
*********************************************************************************************************/
static const char *want_ifname(nm4_want_t want)
{
    switch (want) {
    case NM4_WANT_ETH0:
        return "eth0";
    case NM4_WANT_WLAN0:
        return "wlan0";
    case NM4_WANT_PPP0:
        return "ppp0";
    case NM4_WANT_NONE:
    default:
        return NULL;
    }
}

/*********************************************************************************************************
** Function name:       read_dns_cache
** Descriptions:        Read DNS IPs from /var/run/netmgr/dns.<ifname>.
** input parameters:    ifname  - interface name.
**                      servers - output array of C strings.
**                      max     - max entries.
** output parameters:    servers - filled IPs.
** Returned value:      Number of servers read (0 if file missing).
*********************************************************************************************************/
static int read_dns_cache(const char *ifname, char servers[][64], int max)
{
    char path[256];
    FILE *fp;
    char line[128];
    int n = 0;
    struct in_addr a;

    if (!ifname || !ifname[0] || !servers || max <= 0)
        return 0;
    if (dns_cache_path(ifname, path, sizeof(path)) != 0)
        return 0;

    fp = fopen(path, "r");
    if (!fp)
        return 0;

    while (n < max && fgets(line, sizeof(line), fp)) {
        strip_newline(line);
        if (!line[0])
            continue;
        if (inet_pton(AF_INET, line, &a) != 1)
            continue;
        (void)snprintf(servers[n], 64, "%s", line);
        n++;
    }
    fclose(fp);
    return n;
}

/*********************************************************************************************************
** Function name:       nm4_dns_cache_write
** Descriptions:        Write DNS server list to /var/run/netmgr/dns.<ifname> (one IP per line).
** input parameters:    ifname  - interface name.
**                      servers - array of IP strings (may be NULL if count is 0).
**                      count   - number of servers.
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_dns_cache_write(const char *ifname, const char *servers[], size_t count)
{
    char path[256];
    FILE *fp;
    size_t i;
    struct in_addr a;

    if (!ifname || !ifname[0])
        return -1;
    if (nm4_run_dir_ensure() != 0)
        return -1;
    if (dns_cache_path(ifname, path, sizeof(path)) != 0)
        return -1;

    if (count == 0 || !servers) {
        (void)unlink(path);
        return 0;
    }

    fp = fopen(path, "w");
    if (!fp)
        return -1;

    int wrote_fb = 0;

    for (i = 0; i < count; i++) {
        if (!servers[i] || !servers[i][0])
            continue;
        if (inet_pton(AF_INET, servers[i], &a) != 1)
            continue;
        if (strcmp(servers[i], NM4_DNS_GLOBAL_FALLBACK) == 0)
            wrote_fb = 1;
        (void)fprintf(fp, "%s\n", servers[i]);
    }
    if (!wrote_fb && inet_pton(AF_INET, NM4_DNS_GLOBAL_FALLBACK, &a) == 1)
        (void)fprintf(fp, "%s\n", NM4_DNS_GLOBAL_FALLBACK);
    if (fclose(fp) != 0)
        return -1;
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_dns_cache_clear
** Descriptions:        Remove DNS cache file for interface.
** input parameters:    ifname  - interface name.
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_dns_cache_clear(const char *ifname)
{
    char path[256];

    if (!ifname || !ifname[0])
        return -1;
    if (dns_cache_path(ifname, path, sizeof(path)) != 0)
        return -1;
    if (unlink(path) != 0 && access(path, F_OK) == 0)
        return -1;
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_dns_ensure_fallback
** Descriptions:        Legacy: prepend fallback DNS to existing resolv.conf (prefer nm4_dns_apply).
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_dns_ensure_fallback(void)
{
    return nm4_dns_apply(NM4_WANT_NONE);
}

/*********************************************************************************************************
** Function name:       nm4_dns_apply
** Descriptions:        Rebuild resolv.conf from active WAN DNS cache plus fallback servers.
** input parameters:    want    - current default-route want (selects dns.<if> source).
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_dns_apply(nm4_want_t want)
{
    FILE *fp_out = NULL;
    char wan_servers[NM4_DNS_CACHE_MAX][64];
    char written[NM4_DNS_CACHE_MAX + NM4_DNS_FALLBACK_COUNT][64];
    int wan_count = 0;
    int written_count = 0;
    const char *ifn;
    size_t i;
    int ret = -1;
    int has_fb = 0;

    ifn = want_ifname(want);
    if (ifn)
        wan_count = read_dns_cache(ifn, wan_servers, NM4_DNS_CACHE_MAX);

    fp_out = fopen(NM4_RESOLV_CONF_TMP, "w");
    if (!fp_out)
        goto cleanup;

    for (i = 0; i < (size_t)wan_count; i++) {
        if (!wan_servers[i][0])
            continue;
        if (server_in_list(wan_servers[i], written, written_count))
            continue;
        if (strcmp(wan_servers[i], NM4_DNS_GLOBAL_FALLBACK) == 0)
            has_fb = 1;
        (void)fprintf(fp_out, "nameserver %s\n", wan_servers[i]);
        (void)snprintf(written[written_count], sizeof(written[written_count]), "%s", wan_servers[i]);
        written_count++;
    }

    for (i = 0; i < NM4_DNS_FALLBACK_COUNT; i++) {
        if (!g_fallback_dns[i] || !g_fallback_dns[i][0])
            continue;
        if (server_in_list(g_fallback_dns[i], written, written_count))
            continue;
        (void)fprintf(fp_out, "nameserver %s\n", g_fallback_dns[i]);
        (void)snprintf(written[written_count], sizeof(written[written_count]), "%s", g_fallback_dns[i]);
        written_count++;
        if (strcmp(g_fallback_dns[i], NM4_DNS_GLOBAL_FALLBACK) == 0)
            has_fb = 1;
    }

    if (!has_fb && written_count < NM4_DNS_CACHE_MAX + NM4_DNS_FALLBACK_COUNT) {
        (void)fprintf(fp_out, "nameserver %s\n", NM4_DNS_GLOBAL_FALLBACK);
    }

    if (written_count == 0 && !has_fb)
        (void)fprintf(fp_out, "nameserver %s\n", NM4_DNS_GLOBAL_FALLBACK);

    if (fclose(fp_out) != 0) {
        fp_out = NULL;
        goto cleanup;
    }
    fp_out = NULL;

    if (rename(NM4_RESOLV_CONF_TMP, NM4_RESOLV_CONF) != 0)
        goto cleanup;

    ret = 0;

cleanup:
    if (fp_out) {
        fclose(fp_out);
        unlink(NM4_RESOLV_CONF_TMP);
    }
    return ret;
}

/*********************************************************************************************************
** Function name:       nm4_dns_set_fallback_servers
** Descriptions:        Set custom fallback DNS servers (runtime configuration).
** input parameters:    servers - array of DNS server IP strings.
**                      count   - number of servers (max NM4_DNS_FALLBACK_COUNT).
** output parameters:   NONE
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_dns_set_fallback_servers(const char *servers[], size_t count)
{
    if (!servers || count == 0 || count > NM4_DNS_FALLBACK_COUNT)
        return -1;

    for (size_t i = 0; i < count; i++) {
        if (!servers[i] || !servers[i][0])
            return -1;
        g_fallback_dns[i] = servers[i];
    }

    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_dns_get_fallback_servers
** Descriptions:        Get current fallback DNS server list.
** input parameters:    NONE
** output parameters:   servers - array to receive server strings.
**                      count   - number of servers returned.
** Returned value:      0 on success.
*********************************************************************************************************/
int nm4_dns_get_fallback_servers(const char *servers[], size_t *count)
{
    if (!servers || !count)
        return -1;

    *count = NM4_DNS_FALLBACK_COUNT;
    for (size_t i = 0; i < NM4_DNS_FALLBACK_COUNT; i++)
        servers[i] = g_fallback_dns[i];

    return 0;
}
