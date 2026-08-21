/*************************************************************************/
/** probe.c — 每网卡：存在 / L1 链路 / L2 IPv4 / L3 ICMP + SO_BINDTODEVICE    **/
/*************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include "netmgr_v4_probe.h"
#include "netmgr_v4_gateway.h"

static volatile int g_probe_stop;

/*********************************************************************************************************
** Function name:       mark_for_ifname
** Descriptions:        Map fixed interface names to SO_MARK values for policy routing.
** input parameters:    ifn     - interface name (eth0/wlan0/ppp0).
** output parameters:   NONE
** Returned value:      Mark value or 0 if unknown.
*********************************************************************************************************/
static uint32_t mark_for_ifname(const char *ifn)
{
    if (!ifn) return 0;
    if (strcmp(ifn, "eth0") == 0) return 0x64;
    if (strcmp(ifn, "wlan0") == 0) return 0x65;
    if (strcmp(ifn, "ppp0") == 0) return 0x66;
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_if_exists
** Descriptions:        Check whether /sys/class/net/<ifname> exists.
** input parameters:    ifname  - interface name.
** output parameters:   NONE
** Returned value:      1 if exists, 0 otherwise.
*********************************************************************************************************/
int nm4_if_exists(const char *ifname)
{
    char path[128];
    if (!ifname || !ifname[0]) return 0;
    snprintf(path, sizeof(path), "/sys/class/net/%s", ifname);
    return access(path, F_OK) == 0 ? 1 : 0;
}

/*********************************************************************************************************
** Function name:       check_link
** Descriptions:        Determine L1 link state from carrier file or IFF_UP/RUNNING/POINTOPOINT.
** input parameters:    ifname  - interface name.
** output parameters:   NONE
** Returned value:      true if link appears up.
*********************************************************************************************************/
static bool check_link(const char *ifname)
{
    char path[128];
    FILE *fp;

    snprintf(path, sizeof(path), "/sys/class/net/%s/carrier", ifname);
    fp = fopen(path, "r");
    if (!fp) {
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        struct ifreq ifr;
        bool up = false;
        if (fd < 0) return false;
        memset(&ifr, 0, sizeof(ifr));
        snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ifname);
        if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0)
            up = (ifr.ifr_flags & IFF_UP) &&
                 ((ifr.ifr_flags & IFF_RUNNING) || (ifr.ifr_flags & IFF_POINTOPOINT));
        close(fd);
        return up;
    } else {
        char b[4];
        bool u = false;
        if (fgets(b, sizeof(b), fp)) u = (b[0] == '1');
        fclose(fp);
        return u;
    }
}

/*********************************************************************************************************
** Function name:       check_ip
** Descriptions:        Return true if interface has at least one AF_INET address.
** input parameters:    ifname  - interface name.
** output parameters:   NONE
** Returned value:      true if IPv4 present.
*********************************************************************************************************/
static bool check_ip(const char *ifname)
{
    struct ifaddrs *ifa0, *ifa;
    bool ok = false;
    if (getifaddrs(&ifa0) != 0) return false;
    for (ifa = ifa0; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_name || !ifa->ifa_addr) continue;
        if (strcmp(ifa->ifa_name, ifname) != 0) continue;
        if (ifa->ifa_addr->sa_family == AF_INET) {
            ok = true;
            break;
        }
    }
    freeifaddrs(ifa0);
    return ok;
}

/*********************************************************************************************************
** Function name:       icmp_sum
** Descriptions:        Compute Internet checksum for ICMP header and payload.
** input parameters:    buf     - data buffer.
**                      len     - length in bytes.
** output parameters:   NONE
** Returned value:      16-bit one's complement checksum.
*********************************************************************************************************/
static uint16_t icmp_sum(const void *buf, size_t len)
{
    const uint16_t *p = (const uint16_t *)buf;
    uint32_t s = 0;
    while (len > 1) { s += *p++; len -= 2; }
    if (len) s += *(const uint8_t *)p;
    while (s >> 16) s = (s & 0xFFFFu) + (s >> 16);
    return (uint16_t)(~s);
}

/*********************************************************************************************************
** Function name:       now_ms
** Descriptions:        Current wall time in milliseconds (gettimeofday).
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      Milliseconds since epoch.
*********************************************************************************************************/
static uint64_t now_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
}

/*********************************************************************************************************
** Function name:       ping_once
** Descriptions:        Send one ICMP echo via RAW socket bound to device and marked for policy route.
** input parameters:    ifname      - bind device name (may be empty for any).
**                      target      - destination host string.
**                      timeout_ms  - wait for reply timeout.
** output parameters:   NONE
** Returned value:      0 on echo reply, -1 on error or timeout.
*********************************************************************************************************/
static int ping_once(const char *ifname, const char *target, int timeout_ms)
{
    struct addrinfo hints, *res = NULL;
    struct sockaddr_in dst;
    int sock = -1;
    int ret = -1;
    uint16_t ident = (uint16_t)(getpid() & 0xFFFF);
    uint16_t seq = (uint16_t)(rand() & 0xFFFF);
    uint64_t deadline;
    struct {
        struct icmphdr h;
        uint8_t pl[4];
    } pkt;

    if (g_probe_stop) return -1;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP;
    if (getaddrinfo(target, NULL, &hints, &res) != 0 || !res) return -1;
    memcpy(&dst, res->ai_addr, sizeof(dst));
    freeaddrinfo(res);

    sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) return -1;
    if (ifname && ifname[0] &&
        setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen(ifname) + 1) != 0) {
        close(sock);
        return -1;
    }
    {
        uint32_t m = mark_for_ifname(ifname);
        if (m) (void)setsockopt(sock, SOL_SOCKET, SO_MARK, &m, sizeof(m));
    }
    memset(&pkt, 0, sizeof(pkt));
    pkt.h.type = ICMP_ECHO;
    pkt.h.un.echo.id = htons(ident);
    pkt.h.un.echo.sequence = htons(seq);
    pkt.pl[0] = 'N';
    pkt.pl[1] = 'M';
    pkt.h.checksum = icmp_sum(&pkt, sizeof(pkt));
    if (sendto(sock, &pkt, sizeof(pkt), 0, (struct sockaddr *)&dst, sizeof(dst)) < 0) {
        close(sock);
        return -1;
    }
    deadline = now_ms() + (uint64_t)timeout_ms;
    while (!g_probe_stop) {
        uint64_t t = now_ms();
        struct timeval tv;
        fd_set r;
        uint64_t rem;
        if (t >= deadline) break;
        rem = deadline - t;
        tv.tv_sec = (time_t)(rem / 1000u);
        tv.tv_usec = (suseconds_t)((rem % 1000u) * 1000u);
        FD_ZERO(&r);
        FD_SET(sock, &r);
        if (select(sock + 1, &r, NULL, NULL, &tv) <= 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (FD_ISSET(sock, &r)) {
            uint8_t b[512];
            ssize_t n = recvfrom(sock, b, sizeof(b), 0, NULL, NULL);
            struct iphdr *iph;
            size_t hlen;
            struct icmphdr *ic;
            if (n <= 0) continue;
            if ((size_t)n < sizeof(struct iphdr) + sizeof(struct icmphdr)) continue;
            iph = (struct iphdr *)b;
            hlen = (size_t)iph->ihl * 4u;
            if ((size_t)n < hlen + sizeof(struct icmphdr)) continue;
            ic = (struct icmphdr *)(b + hlen);
            if (ic->type == ICMP_ECHOREPLY && ic->un.echo.id == htons(ident) &&
                ic->un.echo.sequence == htons(seq)) {
                ret = 0;
                break;
            }
        }
    }
    close(sock);
    return ret;
}

/*********************************************************************************************************
** Function name:       nm4_probe_init
** Descriptions:        Seed PRNG and clear probe stop flag for new probe session.
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_probe_init(void)
{
    srand((unsigned)time(NULL));
    g_probe_stop = 0;
}

/*********************************************************************************************************
** Function name:       nm4_probe_request_stop
** Descriptions:        Request in-flight probe loops to exit (cooperative stop).
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_probe_request_stop(void)
{
    g_probe_stop = 1;
}

/*********************************************************************************************************
** Function name:       nm4_probe_params_default
** Descriptions:        Fill probe params with defaults: 2s timeout, auto_gateway, empty targets.
** input parameters:    p       - pointer to struct to initialize.
** output parameters:    p       - defaults written.
** Returned value:      NONE
*********************************************************************************************************/
void nm4_probe_params_default(nm4_probe_params_t *p)
{
    if (!p) return;
    memset(p, 0, sizeof(*p));
    p->timeout_ms = 2000;
    p->auto_gateway = 1;
    p->target_count = 0;
}

/*********************************************************************************************************
** Function name:       build_targets
** Descriptions:        Build list of probe target IP strings from cfg and optional gateway expansion.
** input parameters:    cfg     - probe parameters.
**                      ifname  - interface for $GATEWAY and gw resolve.
**                      maxn    - max targets (array t row count).
** output parameters:    t       - array of target strings filled.
** Returned value:      Number of targets n (>=1).
*********************************************************************************************************/
static int build_targets(const nm4_probe_params_t *cfg, const char *ifname,
    char t[][64], int maxn)
{
    int n = 0, i;
    if (cfg->target_count > 0) {
        for (i = 0; i < cfg->target_count && n < maxn; i++) {
            if (strcmp(cfg->targets[i], "$GATEWAY") == 0) {
                char gw[64];
                if (nm4_gw_resolve(ifname, gw, sizeof(gw)) == 0) {
                    snprintf(t[n], 64, "%s", gw);
                    n++;
                }
            } else {
                snprintf(t[n], 64, "%s", cfg->targets[i]);
                n++;
            }
        }
        if (n == 0 && cfg->auto_gateway) {
            char gw[64];
            if (nm4_gw_resolve(ifname, gw, sizeof(gw)) == 0) {
                snprintf(t[n], 64, "%s", gw);
                n++;
            }
        }
    } else {
        snprintf(t[n], 64, "8.8.8.8");
        n++;
        if (n < maxn) {
            snprintf(t[n], 64, "1.1.1.1");
            n++;
        }
        if (cfg->auto_gateway && n < maxn) {
            char gw[64];
            if (nm4_gw_resolve(ifname, gw, sizeof(gw)) == 0) {
                snprintf(t[n], 64, "%s", gw);
                n++;
            }
        }
    }
    if (n == 0) {
        snprintf(t[0], 64, "8.8.8.8");
        n = 1;
    }
    return n;
}

/*********************************************************************************************************
** Function name:       nm4_probe_result_clear
** Descriptions:        Zero nm4_probe_result_t so callers can fill fields step by step.
** input parameters:    out     - result struct (NULL no-op).
** output parameters:    out     - all fields 0.
** Returned value:      NONE
*********************************************************************************************************/
void nm4_probe_result_clear(nm4_probe_result_t *out)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));
}

/*********************************************************************************************************
** Function name:       nm4_probe_if_exists
** Descriptions:        L0: set out->if_exists from /sys/class/net presence only.
** input parameters:    ifname  - interface name.
** output parameters:    out     - if_exists updated.
** Returned value:      0 on success, -1 on bad args.
*********************************************************************************************************/
int nm4_probe_if_exists(const char *ifname, nm4_probe_result_t *out)
{
    if (!ifname || !ifname[0] || !out) return -1;
    out->if_exists = nm4_if_exists(ifname) ? 1 : 0;
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_probe_link
** Descriptions:        L1: set out->link_up (carrier or SIOCGIFFLAGS); does not read other out fields.
** input parameters:    ifname  - interface name.
** output parameters:    out     - link_up updated.
** Returned value:      0 on success, -1 on bad args.
*********************************************************************************************************/
int nm4_probe_link(const char *ifname, nm4_probe_result_t *out)
{
    if (!ifname || !ifname[0] || !out) return -1;
    out->link_up = check_link(ifname) ? 1 : 0;
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_probe_ipv4
** Descriptions:        L2: set out->ipv4_ok = (out->link_up && has IPv4 on ifname).
** input parameters:    ifname  - interface name.
** output parameters:    out     - ipv4_ok updated (uses current out->link_up).
** Returned value:      0 on success, -1 on bad args.
*********************************************************************************************************/
int nm4_probe_ipv4(const char *ifname, nm4_probe_result_t *out)
{
    if (!ifname || !ifname[0] || !out) return -1;
    out->ipv4_ok = (out->link_up && check_ip(ifname)) ? 1 : 0;
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_probe_icmp
** Descriptions:        L3: ICMP echo to built target list; only updates out->internet_ok.
** input parameters:    ifname  - interface name.
**                      p       - probe parameters (must be non-NULL).
** output parameters:    out     - internet_ok set; if L0/L1/L2 not ready, sets internet_ok=0.
** Returned value:      0 on normal completion, -1 on bad args.
*********************************************************************************************************/
int nm4_probe_icmp(const char *ifname, const nm4_probe_params_t *p, nm4_probe_result_t *out)
{
    char tgs[NM4_PROBE_MAX_TARGETS][64];
    int nt, j;

    if (!ifname || !ifname[0] || !out || !p) return -1;
    if (!out->if_exists || !out->link_up || !out->ipv4_ok) {
        out->internet_ok = 0;
        return 0;
    }
    nt = build_targets(p, ifname, tgs, NM4_PROBE_MAX_TARGETS);
    for (j = 0; j < nt; j++) {
        if (g_probe_stop) {
            out->internet_ok = 0;
            return 0;
        }
        if (ping_once(ifname, tgs[j], p->timeout_ms) == 0) {
            out->internet_ok = 1;
            return 0;
        }
    }
    out->internet_ok = 0;
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_probe_one
** Descriptions:        Full probe: clear → if_exists → link → ipv4 → icmp (same semantics as before).
** input parameters:    ifname  - interface name.
**                      p       - probe parameters (NULL uses defaults).
** output parameters:    out     - filled nm4_probe_result_t.
** Returned value:      0 on normal completion (including iface missing), -1 on bad args.
*********************************************************************************************************/
int nm4_probe_one(const char *ifname, const nm4_probe_params_t *p, nm4_probe_result_t *out)
{
    nm4_probe_params_t def;
    const nm4_probe_params_t *pp = p;

    if (!ifname || !ifname[0] || !out) return -1;
    if (!p) {
        nm4_probe_params_default(&def);
        pp = &def;
    }
    nm4_probe_result_clear(out);
    if (nm4_probe_if_exists(ifname, out) != 0) return -1;
    if (!out->if_exists)
        return 0;
    if (nm4_probe_link(ifname, out) != 0) return -1;
    if (nm4_probe_ipv4(ifname, out) != 0) return -1;
    return nm4_probe_icmp(ifname, pp, out);
}
