/*************************************************************************/
/** gateway.c — 网关：run 文件 → 主表 netlink → network.json            **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <net/if.h>
#include "netmgr_v4_gateway.h"
#include "cJSON.h"

/*********************************************************************************************************
** Function name:       trim_in_place
** Descriptions:        Trim ASCII whitespace from both ends of string in place.
** input parameters:    s       - mutable C string.
** output parameters:   s       - trimmed content moved to start if leading spaces removed.
** Returned value:      0 if non-empty after trim, -1 if empty or NULL.
*********************************************************************************************************/
static int trim_in_place(char *s)
{
    char *a = s, *b;
    if (!s) return -1;
    while (*a == ' ' || *a == '\t' || *a == '\n' || *a == '\r') a++;
    b = a + strlen(a);
    while (b > a && (b[-1] == ' ' || b[-1] == '\t' || b[-1] == '\n' || b[-1] == '\r'))
        b--;
    *b = '\0';
    if (a != s) memmove(s, a, (size_t)(b - a + 1));
    return s[0] ? 0 : -1;
}

/*********************************************************************************************************
** Function name:       read_cached_gw
** Descriptions:        Read last written gateway IPv4 for interface from /var/run/netmgr/gateway.<if>.
** input parameters:    ifname  - interface name.
**                      gwlen   - size of gw buffer.
** output parameters:   gw      - filled with dotted-quad on success.
** Returned value:      0 on success, -1 on missing file or invalid address.
*********************************************************************************************************/
static int read_cached_gw(const char *ifname, char *gw, size_t gwlen)
{
    char path[256];
    FILE *fp;
    struct in_addr a;

    snprintf(path, sizeof(path), "/var/run/netmgr/gateway.%s", ifname);
    fp = fopen(path, "r");
    if (!fp) return -1;
    if (!fgets(gw, (int)gwlen, fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    if (trim_in_place(gw) != 0) return -1;
    if (inet_pton(AF_INET, gw, &a) != 1) return -1;
    return 0;
}

/*********************************************************************************************************
** Function name:       open_netlink_rt
** Descriptions:        Create and bind AF_NETLINK socket for RTNETLINK route queries.
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      socket fd on success, -1 on error.
*********************************************************************************************************/
static int open_netlink_rt(void)
{
    int s = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    struct sockaddr_nl sa;
    if (s < 0) return -1;
    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
        close(s);
        return -1;
    }
    return s;
}

/*********************************************************************************************************
** Function name:       gw_from_netlink_main
** Descriptions:        Dump IPv4 main table routes via netlink and find default gateway for given OIF.
** input parameters:    ifname  - interface name.
**                      gwlen   - size of gw buffer.
** output parameters:   gw      - dotted-quad default gateway if found.
** Returned value:      0 on success, -1 if not found or error.
*********************************************************************************************************/
static int gw_from_netlink_main(const char *ifname, char *gw, size_t gwlen)
{
    int sock;
    struct {
        struct nlmsghdr n;
        struct rtmsg r;
    } req;
    int ifidx;
    char out[INET_ADDRSTRLEN] = "";

    ifidx = (int)if_nametoindex(ifname);
    if (ifidx == 0) return -1;

    sock = open_netlink_rt();
    if (sock < 0) return -1;

    memset(&req, 0, sizeof(req));
    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req.n.nlmsg_type = RTM_GETROUTE;
    req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    req.r.rtm_family = AF_INET;
    req.r.rtm_table = RT_TABLE_MAIN;

    if (send(sock, &req, req.n.nlmsg_len, 0) < 0) {
        close(sock);
        return -1;
    }

    for (;;) {
        char buf[8192];
        ssize_t rd = recv(sock, buf, sizeof(buf), 0);
        struct nlmsghdr *h;
        unsigned rem;

        if (rd <= 0) break;
        h = (struct nlmsghdr *)buf;
        for (rem = (unsigned)rd; NLMSG_OK(h, rem); h = NLMSG_NEXT(h, rem)) {
            struct rtattr *tb[RTA_MAX + 1];
            int attrlen;
            struct rtmsg *rm;
            if (h->nlmsg_type == NLMSG_DONE) {
                close(sock);
                return -1;
            }
            if (h->nlmsg_type != RTM_NEWROUTE) continue;
            rm = (struct rtmsg *)NLMSG_DATA(h);
            if (rm->rtm_family != AF_INET) continue;
            if (rm->rtm_dst_len != 0) continue;

            attrlen = RTM_PAYLOAD(h);
            memset(tb, 0, sizeof(tb));
            {
                struct rtattr *ra = RTM_RTA(rm);
                while (RTA_OK(ra, attrlen)) {
                    if (ra->rta_type <= RTA_MAX) tb[ra->rta_type] = ra;
                    ra = RTA_NEXT(ra, attrlen);
                }
            }
            if (!tb[RTA_OIF] || !tb[RTA_GATEWAY]) continue;
            if (*(int *)RTA_DATA(tb[RTA_OIF]) != ifidx) continue;

            (void)inet_ntop(AF_INET, RTA_DATA(tb[RTA_GATEWAY]), out, sizeof(out));
            if (out[0]) {
                close(sock);
                snprintf(gw, gwlen, "%s", out);
                return 0;
            }
        }
    }
    close(sock);
    return -1;
}

/*********************************************************************************************************
** Function name:       gw_from_network_json
** Descriptions:        Parse NM4_NETWORK_JSON for per-interface gateway string.
** input parameters:    ifname   - key under root or under interfaces object.
**                      jsonpath - path to JSON file.
**                      gwlen    - size of gw buffer.
** output parameters:    gw       - dotted-quad on success.
** Returned value:       0 on success, -1 on parse error or missing field.
*********************************************************************************************************/
static int gw_from_network_json(const char *ifname, const char *jsonpath, char *gw, size_t gwlen)
{
    char *buf;
    FILE *fp;
    long sz;
    cJSON *root, *jif, *g;
    int r = -1;

    if (!ifname || !jsonpath) return -1;
    fp = fopen(jsonpath, "rb");
    if (!fp) return -1;
    fseek(fp, 0, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (sz <= 0) {
        fclose(fp);
        return -1;
    }
    buf = malloc((size_t)sz + 1u);
    if (!buf) {
        fclose(fp);
        return -1;
    }
    if (fread(buf, 1, (size_t)sz, fp) != (size_t)sz) {
        free(buf);
        fclose(fp);
        return -1;
    }
    buf[sz] = '\0';
    fclose(fp);
    root = cJSON_Parse(buf);
    free(buf);
    if (!root) return -1;
    jif = cJSON_GetObjectItemCaseSensitive(root, ifname);
    if (!jif)
        jif = cJSON_GetObjectItemCaseSensitive(
            cJSON_GetObjectItemCaseSensitive(root, "interfaces"), ifname);
    if (jif && cJSON_IsObject(jif)) {
        g = cJSON_GetObjectItemCaseSensitive(jif, "gateway");
        if (cJSON_IsString(g) && g->valuestring) {
            struct in_addr a;
            if (inet_pton(AF_INET, g->valuestring, &a) == 1) {
                snprintf(gw, gwlen, "%s", g->valuestring);
                r = 0;
            }
        }
    }
    cJSON_Delete(root);
    return r;
}

/*********************************************************************************************************
** Function name:       nm4_gw_resolve
** Descriptions:        Resolve IPv4 default gateway for interface: cache file, then netlink, then JSON.
** input parameters:    ifname  - interface name (e.g. eth0).
**                      gwlen   - size of gw buffer (must fit INET_ADDRSTRLEN minimum).
** output parameters:   gw      - dotted-quad string on success.
** Returned value:      0 on success, -1 on failure.
*********************************************************************************************************/
int nm4_gw_resolve(const char *ifname, char *gw, size_t gwlen)
{
    if (!ifname || ifname[0] == '\0' || !gw || gwlen < (size_t)INET_ADDRSTRLEN) return -1;
    if (read_cached_gw(ifname, gw, gwlen) == 0) return 0;
    if (gw_from_netlink_main(ifname, gw, gwlen) == 0) return 0;
    if (gw_from_network_json(ifname, NM4_NETWORK_JSON, gw, gwlen) == 0) return 0;
    return -1;
}
