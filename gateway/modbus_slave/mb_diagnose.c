/*
 * mb_diagnose.c
 * Modbus 0x50 现场诊断
 */

#define _GNU_SOURCE
#include "mb_diagnose.h"
#include "mb_protocol.h"
#include "mb_com_baseclass.h"
#include "cJSON.h"
#include "db_struct.h"
#include "db_node_fops.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MB_DIAG_JSON_BUF_MAX    3200
#define MB_DIAG_SUMMARY_MAX     1024
#define MB_DIAG_RAW_MAX         8192

enum {
    MB_DIAG_ST_QUEUED     = 1,
    MB_DIAG_ST_RUNNING    = 2,
    MB_DIAG_ST_SUCCEEDED  = 3,
    MB_DIAG_ST_FAILED     = 4,
    MB_DIAG_ST_CANCELLED  = 5
};

typedef struct mb_diag_job {
    int         used;
    uint32_t    id;
    char        type[16];
    int         state;
    int         progress;
    int         cancel;
    char        host[MB_DIAG_HOST_LEN_MAX + 1];
    int         ping_count;
    int         ping_timeout_ms;
    char        summary[MB_DIAG_SUMMARY_MAX];
    char        raw[MB_DIAG_RAW_MAX];
    size_t      raw_len;
    time_t      created;
    time_t      updated;
    time_t      expire_at;
    pthread_t   tid;
    int         thread_started;
} mb_diag_job_t;

static mb_diag_job_t g_jobs[MB_DIAG_JOBS_MAX];
static uint32_t g_job_seq = 1;
static pthread_mutex_t g_jobs_mu = PTHREAD_MUTEX_INITIALIZER;

/* ---------------- endian helpers ---------------- */

static uint16_t mb_diag_read_u16_be(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t mb_diag_read_u32_be(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void mb_diag_write_u16_be(uint8_t *p, uint16_t v)
{
    MB_ALIGN_PT_16BIT_WRITE(p, v);
}

static void mb_diag_write_u32_be(uint8_t *p, uint32_t v)
{
    MB_ALIGN_PT_32BIT_WRITE(p, v);
}

static long long mb_diag_now_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}

static int mb_diag_host_ok(const char *s)
{
    size_t n;
    size_t i;

    if (!s)
        return 0;
    n = strlen(s);
    if (n == 0 || n > MB_DIAG_HOST_LEN_MAX)
        return 0;
    for (i = 0; i < n; i++) {
        char c = s[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '.' || c == '-'))
            return 0;
    }
    return 1;
}

static void mb_diag_write_rsp_head(uint8_t *resp, uint8_t dev_addr, uint8_t op,
                                   uint8_t result, uint16_t payload_len)
{
    resp[0] = dev_addr;
    resp[1] = _FC_DIAGNOSE;
    resp[2] = op;
    resp[3] = result;
    resp[4] = 0;
    resp[5] = 0;
    mb_diag_write_u16_be(resp + 6, payload_len);
}

static int mb_diag_emit_json(uint8_t *resp_pdu, uint16_t resp_buf_size,
                             uint16_t *resp_pdu_len, uint8_t dev_addr,
                             uint8_t op, uint8_t result, const char *json)
{
    uint16_t jlen;
    uint16_t need;

    if (!json)
        json = "";
    jlen = (uint16_t)strlen(json);
    need = (uint16_t)(MB_DIAG_RSP_HEAD_LEN + jlen);
    if (need > resp_buf_size)
        return -1;

    mb_diag_write_rsp_head(resp_pdu, dev_addr, op, result, jlen);
    if (jlen > 0)
        memcpy(resp_pdu + MB_DIAG_RSP_HEAD_LEN, json, jlen);
    *resp_pdu_len = need;
    return 0;
}

static int mb_diag_emit_empty(uint8_t *resp_pdu, uint16_t resp_buf_size,
                              uint16_t *resp_pdu_len, uint8_t dev_addr,
                              uint8_t op, uint8_t result)
{
    if (resp_buf_size < MB_DIAG_RSP_HEAD_LEN)
        return -1;
    mb_diag_write_rsp_head(resp_pdu, dev_addr, op, result, 0);
    *resp_pdu_len = MB_DIAG_RSP_HEAD_LEN;
    return 0;
}

/* ---------------- CAPS / NETIF / ROUTE ---------------- */

static int mb_diag_handle_caps(uint8_t dev_addr, uint8_t *resp_pdu,
                               uint16_t resp_buf_size, uint16_t *resp_pdu_len)
{
    char json[MB_DIAG_JSON_BUF_MAX];
    int n;

    n = snprintf(json, sizeof(json),
                 "{\"proto\":1,"
                 "\"ops\":[1,16,17,18,19,32,33,48,49,50,51,64,65,66],"
                 "\"job_types\":[\"ping\"],"
                 "\"limits\":{"
                 "\"ping_count_max\":%d,"
                 "\"ping_timeout_ms_max\":%d,"
                 "\"tcp_timeout_ms_max\":%d,"
                 "\"dns_timeout_ms_max\":%d,"
                 "\"result_chunk_max\":%d,"
                 "\"jobs_max\":%d,"
                 "\"job_ttl_sec\":%d,"
                 "\"bypass_chnl_min\":%d,"
                 "\"bypass_chnl_max\":%d,"
                 "\"bypass_lease_sec_max\":%d"
                 "}}",
                 MB_DIAG_PING_COUNT_MAX,
                 MB_DIAG_PING_TIMEOUT_MS_MAX,
                 MB_DIAG_TCP_TIMEOUT_MS_MAX,
                 MB_DIAG_DNS_TIMEOUT_MS_MAX,
                 MB_DIAG_RESULT_CHUNK_MAX,
                 MB_DIAG_JOBS_MAX,
                 MB_DIAG_JOB_TTL_SEC,
                 DB_BYPASS_SERIAL_CHNL_MIN,
                 DB_BYPASS_SERIAL_CHNL_MAX,
                 DB_BYPASS_LEASE_SEC_MAX);
    if (n <= 0 || n >= (int)sizeof(json))
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_CAPS, MB_DIAG_RESULT_INTERNAL);

    return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
                             dev_addr, MB_DIAG_OP_CAPS, MB_DIAG_RESULT_OK, json);
}

static void mb_diag_json_escape_append(char *dst, size_t dst_sz, size_t *off,
                                       const char *src)
{
    size_t i;

    if (!src)
        src = "";
    for (i = 0; src[i] && *off + 2 < dst_sz; i++) {
        char c = src[i];
        if (c == '"' || c == '\\') {
            dst[(*off)++] = '\\';
            if (*off >= dst_sz)
                break;
        }
        if ((unsigned char)c < 0x20)
            continue;
        dst[(*off)++] = c;
    }
}

static int mb_diag_handle_netif(uint8_t dev_addr, uint8_t *resp_pdu,
                                uint16_t resp_buf_size, uint16_t *resp_pdu_len)
{
    struct ifaddrs *ifa_list = NULL;
    struct ifaddrs *ifa;
    char json[MB_DIAG_JSON_BUF_MAX];
    size_t off = 0;
    int first_if = 1;

    if (getifaddrs(&ifa_list) != 0) {
        return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
                                 dev_addr, MB_DIAG_OP_NETIF_LIST,
                                 MB_DIAG_RESULT_EXEC_FAIL,
                                 "{\"error\":\"getifaddrs\"}");
    }

    off += (size_t)snprintf(json + off, sizeof(json) - off, "{\"ifaces\":[");

    for (ifa = ifa_list; ifa && off + 80 < sizeof(json); ifa = ifa->ifa_next) {
        char ipv4[64];
        int up;
        int carrier;
        unsigned int flags;
        struct ifaddrs *ifa2;
        struct ifaddrs *chk;
        int first_ip;
        int seen = 0;

        if (!ifa->ifa_name)
            continue;
        for (chk = ifa_list; chk != ifa; chk = chk->ifa_next) {
            if (chk->ifa_name && strcmp(chk->ifa_name, ifa->ifa_name) == 0) {
                seen = 1;
                break;
            }
        }
        if (seen)
            continue;

        flags = ifa->ifa_flags;
        up = (flags & IFF_UP) ? 1 : 0;
        carrier = (flags & IFF_RUNNING) ? 1 : 0;

        if (!first_if)
            off += (size_t)snprintf(json + off, sizeof(json) - off, ",");
        first_if = 0;

        off += (size_t)snprintf(json + off, sizeof(json) - off, "{\"name\":\"");
        mb_diag_json_escape_append(json, sizeof(json), &off, ifa->ifa_name);
        off += (size_t)snprintf(json + off, sizeof(json) - off,
                                "\",\"up\":%s,\"carrier\":%s,\"mtu\":0,\"mac\":\"\",\"ipv4\":[",
                                up ? "true" : "false",
                                carrier ? "true" : "false");

        first_ip = 1;
        for (ifa2 = ifa_list; ifa2 && off + 48 < sizeof(json); ifa2 = ifa2->ifa_next) {
            if (!ifa2->ifa_name || strcmp(ifa2->ifa_name, ifa->ifa_name) != 0)
                continue;
            if (!ifa2->ifa_addr || ifa2->ifa_addr->sa_family != AF_INET)
                continue;
            if (!inet_ntop(AF_INET,
                           &((struct sockaddr_in *)ifa2->ifa_addr)->sin_addr,
                           ipv4, sizeof(ipv4)))
                continue;
            if (!first_ip)
                off += (size_t)snprintf(json + off, sizeof(json) - off, ",");
            first_ip = 0;
            off += (size_t)snprintf(json + off, sizeof(json) - off, "\"%s\"", ipv4);
        }
        off += (size_t)snprintf(json + off, sizeof(json) - off, "],\"ipv6\":[]}");
    }

    freeifaddrs(ifa_list);

    if (off + 3 >= sizeof(json)) {
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_NETIF_LIST,
                                  MB_DIAG_RESULT_INTERNAL);
    }
    off += (size_t)snprintf(json + off, sizeof(json) - off, "]}");
    return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
                             dev_addr, MB_DIAG_OP_NETIF_LIST,
                             MB_DIAG_RESULT_OK, json);
}

static void mb_diag_route_fmt_addr(uint32_t addr_le, char *out, size_t out_sz)
{
    struct in_addr a;
    a.s_addr = addr_le; /* /proc/net/route 已是网络字节序数值（小端主机上为 LE 读入的 hex） */
    if (addr_le == 0) {
        snprintf(out, out_sz, "default");
        return;
    }
    inet_ntop(AF_INET, &a, out, (socklen_t)out_sz);
}

static int mb_diag_handle_route(uint8_t dev_addr, uint8_t *resp_pdu,
                                uint16_t resp_buf_size, uint16_t *resp_pdu_len)
{
    FILE *fp;
    char line[256];
    char json[MB_DIAG_JSON_BUF_MAX];
    size_t off = 0;
    int first = 1;
    int skip_hdr = 1;

    fp = fopen("/proc/net/route", "r");
    if (!fp) {
        return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
                                 dev_addr, MB_DIAG_OP_ROUTE_LIST,
                                 MB_DIAG_RESULT_EXEC_FAIL,
                                 "{\"error\":\"open_route\"}");
    }

    off += (size_t)snprintf(json + off, sizeof(json) - off, "{\"routes\":[");

    while (fgets(line, sizeof(line), fp) && off + 120 < sizeof(json)) {
        char iface[64];
        unsigned int dest, gw, flags, refcnt, use, metric, mask;
        char dst_s[64], via_s[64];

        if (skip_hdr) {
            skip_hdr = 0;
            continue;
        }
        if (sscanf(line, "%63s %X %X %X %u %u %u %X",
                   iface, &dest, &gw, &flags, &refcnt, &use, &metric, &mask) < 8)
            continue;

        mb_diag_route_fmt_addr((uint32_t)dest, dst_s, sizeof(dst_s));
        if (gw == 0)
            snprintf(via_s, sizeof(via_s), "");
        else
            mb_diag_route_fmt_addr((uint32_t)gw, via_s, sizeof(via_s));

        if (!first)
            off += (size_t)snprintf(json + off, sizeof(json) - off, ",");
        first = 0;
        off += (size_t)snprintf(json + off, sizeof(json) - off,
                                "{\"dst\":\"%s\",\"via\":\"%s\",\"dev\":\"",
                                dst_s, via_s);
        mb_diag_json_escape_append(json, sizeof(json), &off, iface);
        off += (size_t)snprintf(json + off, sizeof(json) - off,
                                "\",\"metric\":%u}", metric);
    }
    fclose(fp);

    if (off + 3 >= sizeof(json)) {
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_ROUTE_LIST,
                                  MB_DIAG_RESULT_INTERNAL);
    }
    off += (size_t)snprintf(json + off, sizeof(json) - off, "]}");
    return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
                             dev_addr, MB_DIAG_OP_ROUTE_LIST,
                             MB_DIAG_RESULT_OK, json);
}

/* ---------------- CONN_LIST ---------------- */

#define MB_DIAG_CONN_MAX        256
#define MB_DIAG_CONN_LIMIT_MAX  80

typedef struct {
    char proto[8];
    char state[16];
    char local[72];
    char peer[72];
} mb_diag_conn_t;

static void mb_diag_fmt_ipv4_port(unsigned int addr_le, unsigned int port,
                                  char *out, size_t out_sz)
{
    snprintf(out, out_sz, "%u.%u.%u.%u:%u",
             addr_le & 0xffu,
             (addr_le >> 8) & 0xffu,
             (addr_le >> 16) & 0xffu,
             (addr_le >> 24) & 0xffu,
             port & 0xffffu);
}

static const char *mb_diag_tcp_state_name(unsigned int st)
{
    switch (st) {
    case 0x01: return "ESTABLISHED";
    case 0x02: return "SYN_SENT";
    case 0x03: return "SYN_RECV";
    case 0x04: return "FIN_WAIT1";
    case 0x05: return "FIN_WAIT2";
    case 0x06: return "TIME_WAIT";
    case 0x07: return "CLOSE";
    case 0x08: return "CLOSE_WAIT";
    case 0x09: return "LAST_ACK";
    case 0x0A: return "LISTEN";
    case 0x0B: return "CLOSING";
    default:   return "UNKNOWN";
    }
}

static int mb_diag_conn_want(uint8_t filter, int is_udp, unsigned int tcp_st)
{
    if (is_udp)
        return filter == 0; /* 仅「全部」包含 UDP */
    if (filter == 0)
        return 1;
    if (filter == 1)
        return tcp_st == 0x0A;
    if (filter == 2)
        return tcp_st == 0x01;
    if (filter == 3)
        return (tcp_st == 0x0A || tcp_st == 0x01);
    return 1;
}

static int mb_diag_load_proc_net(const char *path, int is_udp, uint8_t filter,
                                 mb_diag_conn_t *arr, int *count, int maxn)
{
    FILE *fp;
    char line[256];
    int skip = 1;

    fp = fopen(path, "r");
    if (!fp)
        return -1;

    while (fgets(line, sizeof(line), fp) && *count < maxn) {
        unsigned int lip = 0, lport = 0, rip = 0, rport = 0, st = 0;
        mb_diag_conn_t *c;

        if (skip) {
            skip = 0;
            continue;
        }
        if (sscanf(line, "%*d: %X:%X %X:%X %X",
                   &lip, &lport, &rip, &rport, &st) < 5)
            continue;
        if (!mb_diag_conn_want(filter, is_udp, st))
            continue;

        c = &arr[*count];
        memset(c, 0, sizeof(*c));
        strncpy(c->proto, is_udp ? "udp" : "tcp", sizeof(c->proto) - 1);
        if (is_udp)
            strncpy(c->state, "UDP", sizeof(c->state) - 1);
        else
            strncpy(c->state, mb_diag_tcp_state_name(st), sizeof(c->state) - 1);
        mb_diag_fmt_ipv4_port(lip, lport, c->local, sizeof(c->local));
        if (!is_udp && st == 0x0A)
            snprintf(c->peer, sizeof(c->peer), "-");
        else if (rip == 0 && rport == 0)
            snprintf(c->peer, sizeof(c->peer), "-");
        else
            mb_diag_fmt_ipv4_port(rip, rport, c->peer, sizeof(c->peer));
        (*count)++;
    }
    fclose(fp);
    return 0;
}

static int mb_diag_handle_conn(uint8_t dev_addr, const uint8_t *pdu, uint16_t pdu_len,
                               uint8_t *resp_pdu, uint16_t resp_buf_size,
                               uint16_t *resp_pdu_len)
{
    uint8_t filter;
    uint8_t proto;
    uint16_t offset;
    uint16_t limit;
    mb_diag_conn_t *arr;
    int total = 0;
    int i;
    int count;
    char json[MB_DIAG_JSON_BUF_MAX];
    size_t off = 0;
    int first = 1;

    if (pdu_len < MB_DIAG_REQ_HEAD_LEN + 6)
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_CONN_LIST,
                                  MB_DIAG_RESULT_BAD_PARAM);

    filter = pdu[4];
    proto = pdu[5];
    offset = mb_diag_read_u16_be(pdu + 6);
    limit = mb_diag_read_u16_be(pdu + 8);

    if (filter > 3 || proto > 2 || limit == 0 || limit > MB_DIAG_CONN_LIMIT_MAX)
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_CONN_LIST,
                                  MB_DIAG_RESULT_BAD_PARAM);

    arr = (mb_diag_conn_t *)calloc(MB_DIAG_CONN_MAX, sizeof(mb_diag_conn_t));
    if (!arr)
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_CONN_LIST,
                                  MB_DIAG_RESULT_INTERNAL);

    {
        int read_ok = 0;
        if (proto == 0 || proto == 1) {
            if (mb_diag_load_proc_net("/proc/net/tcp", 0, filter, arr, &total,
                                      MB_DIAG_CONN_MAX) == 0)
                read_ok = 1;
        }
        if ((proto == 0 || proto == 2) && total < MB_DIAG_CONN_MAX) {
            if (mb_diag_load_proc_net("/proc/net/udp", 1, filter, arr, &total,
                                      MB_DIAG_CONN_MAX) == 0)
                read_ok = 1;
        }
        if (!read_ok) {
            free(arr);
            return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
                                     dev_addr, MB_DIAG_OP_CONN_LIST,
                                     MB_DIAG_RESULT_EXEC_FAIL,
                                     "{\"error\":\"open_proc_net\"}");
        }
    }

    if (offset > (uint16_t)total)
        offset = (uint16_t)total;
    count = (int)total - (int)offset;
    if (count > (int)limit)
        count = (int)limit;
    if (count < 0)
        count = 0;

    off += (size_t)snprintf(json + off, sizeof(json) - off,
                            "{\"total\":%d,\"offset\":%u,\"count\":%d,\"items\":[",
                            total, (unsigned)offset, count);

    for (i = 0; i < count && off + 160 < sizeof(json); i++) {
        mb_diag_conn_t *c = &arr[(int)offset + i];
        if (!first)
            off += (size_t)snprintf(json + off, sizeof(json) - off, ",");
        first = 0;
        off += (size_t)snprintf(json + off, sizeof(json) - off,
                                "{\"proto\":\"%s\",\"state\":\"%s\",\"local\":\"%s\",\"peer\":\"%s\"}",
                                c->proto, c->state, c->local, c->peer);
    }
    free(arr);

    if (off + 3 >= sizeof(json))
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_CONN_LIST,
                                  MB_DIAG_RESULT_INTERNAL);

    off += (size_t)snprintf(json + off, sizeof(json) - off, "]}");
    return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
                             dev_addr, MB_DIAG_OP_CONN_LIST,
                             MB_DIAG_RESULT_OK, json);
}

/* ---------------- DNS / TCP ---------------- */

static int mb_diag_handle_dns(uint8_t dev_addr, const uint8_t *pdu, uint16_t pdu_len,
                              uint8_t *resp_pdu, uint16_t resp_buf_size,
                              uint16_t *resp_pdu_len)
{
    uint16_t timeout_ms;
    uint8_t name_len;
    char name[MB_DIAG_HOST_LEN_MAX + 1];
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct addrinfo *rp;
    char json[MB_DIAG_JSON_BUF_MAX];
    size_t off = 0;
    int first = 1;
    long long t0, elapsed;
    int gai;

    if (pdu_len < MB_DIAG_REQ_HEAD_LEN + 3)
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_DNS_RESOLVE,
                                  MB_DIAG_RESULT_BAD_PARAM);

    timeout_ms = mb_diag_read_u16_be(pdu + 4);
    name_len = pdu[6];
    if (name_len == 0 || name_len > MB_DIAG_HOST_LEN_MAX ||
        pdu_len < (uint16_t)(MB_DIAG_REQ_HEAD_LEN + 3 + name_len))
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_DNS_RESOLVE,
                                  MB_DIAG_RESULT_BAD_PARAM);

    memcpy(name, pdu + 7, name_len);
    name[name_len] = '\0';
    if (!mb_diag_host_ok(name))
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_DNS_RESOLVE,
                                  MB_DIAG_RESULT_BAD_PARAM);

    if (timeout_ms == 0)
        timeout_ms = 2000;
    if (timeout_ms > MB_DIAG_DNS_TIMEOUT_MS_MAX)
        timeout_ms = MB_DIAG_DNS_TIMEOUT_MS_MAX;
    (void)timeout_ms; /* getaddrinfo 无超时接口；由上层总超时兜底 */

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    t0 = mb_diag_now_ms();
    gai = getaddrinfo(name, NULL, &hints, &res);
    elapsed = mb_diag_now_ms() - t0;

    if (gai != 0) {
        snprintf(json, sizeof(json),
                 "{\"name\":\"%s\",\"addrs\":[],\"elapsed_ms\":%lld,\"error\":\"%s\"}",
                 name, elapsed, gai_strerror(gai));
        return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
                                 dev_addr, MB_DIAG_OP_DNS_RESOLVE,
                                 MB_DIAG_RESULT_OK, json);
    }

    off += (size_t)snprintf(json + off, sizeof(json) - off,
                            "{\"name\":\"%s\",\"addrs\":[", name);
    for (rp = res; rp && off + 48 < sizeof(json); rp = rp->ai_next) {
        char ip[INET6_ADDRSTRLEN];
        void *addr;

        if (rp->ai_family == AF_INET)
            addr = &((struct sockaddr_in *)rp->ai_addr)->sin_addr;
        else if (rp->ai_family == AF_INET6)
            addr = &((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr;
        else
            continue;
        if (!inet_ntop(rp->ai_family, addr, ip, sizeof(ip)))
            continue;
        if (!first)
            off += (size_t)snprintf(json + off, sizeof(json) - off, ",");
        first = 0;
        off += (size_t)snprintf(json + off, sizeof(json) - off, "\"%s\"", ip);
    }
    freeaddrinfo(res);
    off += (size_t)snprintf(json + off, sizeof(json) - off,
                            "],\"elapsed_ms\":%lld,\"error\":\"\"}", elapsed);
    return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
                             dev_addr, MB_DIAG_OP_DNS_RESOLVE,
                             MB_DIAG_RESULT_OK, json);
}

static int mb_diag_tcp_connect_once(const struct addrinfo *rp, int timeout_ms,
                                    int *elapsed_ms, char *err, size_t err_sz)
{
    int fd;
    int flags;
    int ret;
    struct pollfd pfd;
    long long t0;

    err[0] = '\0';
    t0 = mb_diag_now_ms();
    fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (fd < 0) {
        snprintf(err, err_sz, "socket:%d", errno);
        return -1;
    }
    flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0)
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    ret = connect(fd, rp->ai_addr, rp->ai_addrlen);
    if (ret == 0) {
        *elapsed_ms = (int)(mb_diag_now_ms() - t0);
        close(fd);
        return 0;
    }
    if (errno != EINPROGRESS) {
        snprintf(err, err_sz, "connect:%d", errno);
        close(fd);
        *elapsed_ms = (int)(mb_diag_now_ms() - t0);
        return -1;
    }

    pfd.fd = fd;
    pfd.events = POLLOUT;
    ret = poll(&pfd, 1, timeout_ms);
    if (ret == 0) {
        snprintf(err, err_sz, "timeout");
        close(fd);
        *elapsed_ms = timeout_ms;
        return -1;
    }
    if (ret < 0) {
        snprintf(err, err_sz, "poll:%d", errno);
        close(fd);
        *elapsed_ms = (int)(mb_diag_now_ms() - t0);
        return -1;
    }
    {
        int soerr = 0;
        socklen_t sl = sizeof(soerr);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &sl) < 0 || soerr != 0) {
            snprintf(err, err_sz, "refused:%d", soerr ? soerr : errno);
            close(fd);
            *elapsed_ms = (int)(mb_diag_now_ms() - t0);
            return -1;
        }
    }
    *elapsed_ms = (int)(mb_diag_now_ms() - t0);
    close(fd);
    return 0;
}

static int mb_diag_handle_tcp(uint8_t dev_addr, const uint8_t *pdu, uint16_t pdu_len,
                              uint8_t *resp_pdu, uint16_t resp_buf_size,
                              uint16_t *resp_pdu_len)
{
    uint16_t timeout_ms;
    uint16_t port;
    uint8_t host_len;
    char host[MB_DIAG_HOST_LEN_MAX + 1];
    char port_s[16];
    char json[MB_DIAG_JSON_BUF_MAX];
    char err[64];
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct addrinfo *rp;
    int elapsed = 0;
    int ok = 0;
    int gai;

    if (pdu_len < MB_DIAG_REQ_HEAD_LEN + 5)
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_TCP_PROBE,
                                  MB_DIAG_RESULT_BAD_PARAM);

    timeout_ms = mb_diag_read_u16_be(pdu + 4);
    port = mb_diag_read_u16_be(pdu + 6);
    host_len = pdu[8];
    if (port == 0 || host_len == 0 || host_len > MB_DIAG_HOST_LEN_MAX ||
        pdu_len < (uint16_t)(MB_DIAG_REQ_HEAD_LEN + 5 + host_len))
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_TCP_PROBE,
                                  MB_DIAG_RESULT_BAD_PARAM);

    memcpy(host, pdu + 9, host_len);
    host[host_len] = '\0';
    if (!mb_diag_host_ok(host))
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_TCP_PROBE,
                                  MB_DIAG_RESULT_BAD_PARAM);

    if (timeout_ms == 0)
        timeout_ms = 3000;
    if (timeout_ms > MB_DIAG_TCP_TIMEOUT_MS_MAX)
        timeout_ms = MB_DIAG_TCP_TIMEOUT_MS_MAX;

    snprintf(port_s, sizeof(port_s), "%u", (unsigned)port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    gai = getaddrinfo(host, port_s, &hints, &res);
    if (gai != 0) {
        snprintf(json, sizeof(json),
                 "{\"host\":\"%s\",\"port\":%u,\"ok\":false,\"elapsed_ms\":0,\"error\":\"dns:%s\"}",
                 host, (unsigned)port, gai_strerror(gai));
        return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
                                 dev_addr, MB_DIAG_OP_TCP_PROBE,
                                 MB_DIAG_RESULT_OK, json);
    }

    err[0] = '\0';
    for (rp = res; rp; rp = rp->ai_next) {
        if (mb_diag_tcp_connect_once(rp, (int)timeout_ms, &elapsed, err, sizeof(err)) == 0) {
            ok = 1;
            break;
        }
    }
    freeaddrinfo(res);

    snprintf(json, sizeof(json),
             "{\"host\":\"%s\",\"port\":%u,\"ok\":%s,\"elapsed_ms\":%d,\"error\":\"%s\"}",
             host, (unsigned)port, ok ? "true" : "false", elapsed, err);
    return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
                             dev_addr, MB_DIAG_OP_TCP_PROBE,
                             MB_DIAG_RESULT_OK, json);
}

/* ---------------- jobs / ping ---------------- */

static void mb_diag_jobs_gc_locked(void)
{
    int i;
    time_t now = time(NULL);

    for (i = 0; i < MB_DIAG_JOBS_MAX; i++) {
        mb_diag_job_t *j = &g_jobs[i];
        if (!j->used)
            continue;
        if (j->state == MB_DIAG_ST_SUCCEEDED ||
            j->state == MB_DIAG_ST_FAILED ||
            j->state == MB_DIAG_ST_CANCELLED) {
            if (j->expire_at > 0 && now >= j->expire_at) {
                if (j->thread_started) {
                    pthread_join(j->tid, NULL);
                    j->thread_started = 0;
                }
                memset(j, 0, sizeof(*j));
            }
        }
    }
}

static mb_diag_job_t *mb_diag_job_find_locked(uint32_t id)
{
    int i;
    for (i = 0; i < MB_DIAG_JOBS_MAX; i++) {
        if (g_jobs[i].used && g_jobs[i].id == id)
            return &g_jobs[i];
    }
    return NULL;
}

static const char *mb_diag_state_str(int st)
{
    switch (st) {
    case MB_DIAG_ST_QUEUED: return "queued";
    case MB_DIAG_ST_RUNNING: return "running";
    case MB_DIAG_ST_SUCCEEDED: return "succeeded";
    case MB_DIAG_ST_FAILED: return "failed";
    case MB_DIAG_ST_CANCELLED: return "cancelled";
    default: return "unknown";
    }
}

static void *mb_diag_ping_thread(void *arg)
{
    mb_diag_job_t *job = (mb_diag_job_t *)arg;
    char cmd[320];
    char line[256];
    FILE *fp;
    int wait_s;
    int sent = 0, recv = 0;
    float rtt_min = 0, rtt_avg = 0, rtt_max = 0;
    int have_rtt = 0;

    pthread_mutex_lock(&g_jobs_mu);
    job->state = MB_DIAG_ST_RUNNING;
    job->progress = 0;
    job->updated = time(NULL);
    job->raw_len = 0;
    job->raw[0] = '\0';
    job->summary[0] = '\0';
    wait_s = (job->ping_timeout_ms + 999) / 1000;
    if (wait_s < 1)
        wait_s = 1;
    /* host 已白名单校验 */
    snprintf(cmd, sizeof(cmd), "ping -c %d -W %d %s 2>&1",
             job->ping_count, wait_s, job->host);
    pthread_mutex_unlock(&g_jobs_mu);

    fp = popen(cmd, "r");
    if (!fp) {
        pthread_mutex_lock(&g_jobs_mu);
        snprintf(job->summary, sizeof(job->summary),
                 "{\"host\":\"%s\",\"ok\":false,\"error\":\"popen\"}", job->host);
        job->state = MB_DIAG_ST_FAILED;
        job->progress = 100;
        job->updated = time(NULL);
        job->expire_at = time(NULL) + MB_DIAG_JOB_TTL_SEC;
        pthread_mutex_unlock(&g_jobs_mu);
        return NULL;
    }

    while (fgets(line, sizeof(line), fp)) {
        size_t n;
        pthread_mutex_lock(&g_jobs_mu);
        if (job->cancel) {
            pthread_mutex_unlock(&g_jobs_mu);
            break;
        }
        n = strlen(line);
        if (job->raw_len + n < MB_DIAG_RAW_MAX - 1) {
            memcpy(job->raw + job->raw_len, line, n);
            job->raw_len += n;
            job->raw[job->raw_len] = '\0';
        }
        /* 简单解析 */
        if (strstr(line, "bytes from")) {
            float t = 0;
            char *p = strstr(line, "time=");
            if (p && sscanf(p, "time=%f", &t) == 1) {
                if (!have_rtt) {
                    rtt_min = rtt_avg = rtt_max = t;
                    have_rtt = 1;
                } else {
                    if (t < rtt_min) rtt_min = t;
                    if (t > rtt_max) rtt_max = t;
                    rtt_avg = (rtt_avg * (float)recv + t) / (float)(recv + 1);
                }
                recv++;
            } else {
                recv++;
            }
        }
        if (sscanf(line, "%d packets transmitted, %d received", &sent, &recv) == 2) {
            /* busybox / iputils 统计行 */
        }
        {
            int pct = 0;
            if (job->ping_count > 0)
                pct = (recv * 100) / job->ping_count;
            if (pct > 99)
                pct = 99;
            job->progress = pct;
            job->updated = time(NULL);
        }
        pthread_mutex_unlock(&g_jobs_mu);
    }
    pclose(fp);

    pthread_mutex_lock(&g_jobs_mu);
    if (job->cancel) {
        job->state = MB_DIAG_ST_CANCELLED;
        snprintf(job->summary, sizeof(job->summary),
                 "{\"host\":\"%s\",\"ok\":false,\"error\":\"cancelled\"}", job->host);
    } else {
        int loss = 0;
        int ok;
        if (sent <= 0)
            sent = job->ping_count;
        if (sent > 0)
            loss = ((sent - recv) * 100) / sent;
        if (loss < 0)
            loss = 0;
        ok = (recv > 0);
        snprintf(job->summary, sizeof(job->summary),
                 "{\"host\":\"%s\",\"transmitted\":%d,\"received\":%d,\"loss_pct\":%d,"
                 "\"rtt_min_ms\":%.1f,\"rtt_avg_ms\":%.1f,\"rtt_max_ms\":%.1f,\"ok\":%s}",
                 job->host, sent, recv, loss,
                 have_rtt ? rtt_min : 0.0, have_rtt ? rtt_avg : 0.0, have_rtt ? rtt_max : 0.0,
                 ok ? "true" : "false");
        job->state = MB_DIAG_ST_SUCCEEDED;
    }
    job->progress = 100;
    job->updated = time(NULL);
    job->expire_at = time(NULL) + MB_DIAG_JOB_TTL_SEC;
    pthread_mutex_unlock(&g_jobs_mu);
    return NULL;
}

static int mb_diag_handle_job_start(uint8_t dev_addr, const uint8_t *pdu,
                                    uint16_t pdu_len, uint8_t *resp_pdu,
                                    uint16_t resp_buf_size, uint16_t *resp_pdu_len)
{
    uint8_t type_len;
    uint16_t param_len;
    char type[16];
    const uint8_t *param;
    cJSON *root = NULL;
    cJSON *jhost;
    cJSON *jcount;
    cJSON *jtimeout;
    const char *host;
    int count = 4;
    int timeout_ms = 2000;
    mb_diag_job_t *slot = NULL;
    int i;
    char json[256];
    long long now_ms;

    if (pdu_len < MB_DIAG_REQ_HEAD_LEN + 1)
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_START,
                                  MB_DIAG_RESULT_BAD_PARAM);

    type_len = pdu[4];
    if (type_len == 0 || type_len >= sizeof(type) ||
        pdu_len < (uint16_t)(MB_DIAG_REQ_HEAD_LEN + 1 + type_len + 2))
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_START,
                                  MB_DIAG_RESULT_BAD_PARAM);

    memcpy(type, pdu + 5, type_len);
    type[type_len] = '\0';
    param_len = mb_diag_read_u16_be(pdu + 5 + type_len);
    if (pdu_len < (uint16_t)(MB_DIAG_REQ_HEAD_LEN + 1 + type_len + 2 + param_len))
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_START,
                                  MB_DIAG_RESULT_BAD_PARAM);
    param = pdu + 5 + type_len + 2;

    if (strcmp(type, "ping") != 0)
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_START,
                                  MB_DIAG_RESULT_UNSUPPORTED);

    {
        char *param_str = (char *)malloc(param_len + 1);
        if (!param_str)
            return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                      dev_addr, MB_DIAG_OP_JOB_START,
                                      MB_DIAG_RESULT_INTERNAL);
        memcpy(param_str, param, param_len);
        param_str[param_len] = '\0';
        root = cJSON_Parse(param_str);
        free(param_str);
    }
    if (!root)
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_START,
                                  MB_DIAG_RESULT_BAD_PARAM);

    jhost = cJSON_GetObjectItem(root, "host");
    if (!cJSON_IsString(jhost) || !jhost->valuestring || !mb_diag_host_ok(jhost->valuestring)) {
        cJSON_Delete(root);
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_START,
                                  MB_DIAG_RESULT_BAD_PARAM);
    }
    host = jhost->valuestring;
    jcount = cJSON_GetObjectItem(root, "count");
    if (cJSON_IsNumber(jcount))
        count = jcount->valueint;
    jtimeout = cJSON_GetObjectItem(root, "timeout_ms");
    if (cJSON_IsNumber(jtimeout))
        timeout_ms = jtimeout->valueint;

    if (count < 1 || count > MB_DIAG_PING_COUNT_MAX ||
        timeout_ms < 200 || timeout_ms > MB_DIAG_PING_TIMEOUT_MS_MAX) {
        cJSON_Delete(root);
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_START,
                                  MB_DIAG_RESULT_BAD_PARAM);
    }

    pthread_mutex_lock(&g_jobs_mu);
    mb_diag_jobs_gc_locked();
    for (i = 0; i < MB_DIAG_JOBS_MAX; i++) {
        if (!g_jobs[i].used) {
            slot = &g_jobs[i];
            break;
        }
    }
    if (!slot) {
        pthread_mutex_unlock(&g_jobs_mu);
        cJSON_Delete(root);
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_START,
                                  MB_DIAG_RESULT_BUSY);
    }

    memset(slot, 0, sizeof(*slot));
    slot->used = 1;
    slot->id = g_job_seq++;
    if (g_job_seq == 0)
        g_job_seq = 1;
    strncpy(slot->type, "ping", sizeof(slot->type) - 1);
    strncpy(slot->host, host, sizeof(slot->host) - 1);
    slot->ping_count = count;
    slot->ping_timeout_ms = timeout_ms;
    slot->state = MB_DIAG_ST_QUEUED;
    slot->created = slot->updated = time(NULL);
    cJSON_Delete(root);

    if (pthread_create(&slot->tid, NULL, mb_diag_ping_thread, slot) != 0) {
        memset(slot, 0, sizeof(*slot));
        pthread_mutex_unlock(&g_jobs_mu);
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_START,
                                  MB_DIAG_RESULT_INTERNAL);
    }
    slot->thread_started = 1;
    now_ms = mb_diag_now_ms();
    snprintf(json, sizeof(json),
             "{\"job_id\":%u,\"type\":\"ping\",\"state\":\"queued\",\"accepted_ms\":%lld}",
             (unsigned)slot->id, now_ms);
    pthread_mutex_unlock(&g_jobs_mu);

    return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
                             dev_addr, MB_DIAG_OP_JOB_START,
                             MB_DIAG_RESULT_OK, json);
}

static int mb_diag_handle_job_status(uint8_t dev_addr, const uint8_t *pdu,
                                     uint16_t pdu_len, uint8_t *resp_pdu,
                                     uint16_t resp_buf_size, uint16_t *resp_pdu_len)
{
    uint32_t job_id;
    mb_diag_job_t *job;
    char json[320];

    if (pdu_len < MB_DIAG_REQ_HEAD_LEN + 4)
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_STATUS,
                                  MB_DIAG_RESULT_BAD_PARAM);

    job_id = mb_diag_read_u32_be(pdu + 4);
    pthread_mutex_lock(&g_jobs_mu);
    mb_diag_jobs_gc_locked();
    job = mb_diag_job_find_locked(job_id);
    if (!job) {
        pthread_mutex_unlock(&g_jobs_mu);
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_STATUS,
                                  MB_DIAG_RESULT_NO_JOB);
    }
    snprintf(json, sizeof(json),
             "{\"job_id\":%u,\"type\":\"%s\",\"state\":\"%s\",\"progress\":%d,"
             "\"created_ms\":%lld,\"updated_ms\":%lld,\"error\":\"\"}",
             (unsigned)job->id, job->type, mb_diag_state_str(job->state), job->progress,
             (long long)job->created * 1000LL, (long long)job->updated * 1000LL);
    pthread_mutex_unlock(&g_jobs_mu);
    return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
                             dev_addr, MB_DIAG_OP_JOB_STATUS,
                             MB_DIAG_RESULT_OK, json);
}

static int mb_diag_handle_job_cancel(uint8_t dev_addr, const uint8_t *pdu,
                                     uint16_t pdu_len, uint8_t *resp_pdu,
                                     uint16_t resp_buf_size, uint16_t *resp_pdu_len)
{
    uint32_t job_id;
    mb_diag_job_t *job;
    char json[160];
    int st;

    if (pdu_len < MB_DIAG_REQ_HEAD_LEN + 4)
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_CANCEL,
                                  MB_DIAG_RESULT_BAD_PARAM);

    job_id = mb_diag_read_u32_be(pdu + 4);
    pthread_mutex_lock(&g_jobs_mu);
    mb_diag_jobs_gc_locked();
    job = mb_diag_job_find_locked(job_id);
    if (!job) {
        pthread_mutex_unlock(&g_jobs_mu);
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_CANCEL,
                                  MB_DIAG_RESULT_NO_JOB);
    }
    if (job->state == MB_DIAG_ST_QUEUED || job->state == MB_DIAG_ST_RUNNING)
        job->cancel = 1;
    st = job->state;
    if (st == MB_DIAG_ST_QUEUED) {
        job->state = MB_DIAG_ST_CANCELLED;
        job->expire_at = time(NULL) + MB_DIAG_JOB_TTL_SEC;
        st = MB_DIAG_ST_CANCELLED;
    }
    snprintf(json, sizeof(json), "{\"job_id\":%u,\"state\":\"%s\"}",
             (unsigned)job->id, mb_diag_state_str(st));
    pthread_mutex_unlock(&g_jobs_mu);
    return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
                             dev_addr, MB_DIAG_OP_JOB_CANCEL,
                             MB_DIAG_RESULT_OK, json);
}

static int mb_diag_handle_job_result(uint8_t dev_addr, const uint8_t *pdu,
                                     uint16_t pdu_len, uint8_t *resp_pdu,
                                     uint16_t resp_buf_size, uint16_t *resp_pdu_len)
{
    uint32_t job_id;
    uint8_t part;
    uint16_t offset;
    uint16_t limit;
    mb_diag_job_t *job;
    const char *src;
    uint32_t total;
    uint16_t chunk;
    uint16_t need;
    uint8_t state_code;

    if (pdu_len < MB_DIAG_REQ_HEAD_LEN + 10)
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_RESULT,
                                  MB_DIAG_RESULT_BAD_PARAM);

    job_id = mb_diag_read_u32_be(pdu + 4);
    part = pdu[8];
    offset = mb_diag_read_u16_be(pdu + 10);
    limit = mb_diag_read_u16_be(pdu + 12);
    if (limit == 0 || limit > MB_DIAG_RESULT_CHUNK_MAX)
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_RESULT,
                                  MB_DIAG_RESULT_BAD_PARAM);
    if (part > 1)
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_RESULT,
                                  MB_DIAG_RESULT_BAD_PARAM);

    pthread_mutex_lock(&g_jobs_mu);
    mb_diag_jobs_gc_locked();
    job = mb_diag_job_find_locked(job_id);
    if (!job) {
        pthread_mutex_unlock(&g_jobs_mu);
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_RESULT,
                                  MB_DIAG_RESULT_NO_JOB);
    }
    if (job->state == MB_DIAG_ST_QUEUED || job->state == MB_DIAG_ST_RUNNING) {
        pthread_mutex_unlock(&g_jobs_mu);
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_JOB_RESULT,
                                  MB_DIAG_RESULT_NOT_READY);
    }

    if (part == 0) {
        src = job->summary;
        total = (uint32_t)strlen(src);
    } else {
        src = job->raw;
        total = (uint32_t)job->raw_len;
    }
    state_code = (uint8_t)job->state;

    if ((uint32_t)offset > total)
        offset = (uint16_t)total;

    {
        uint32_t remain = total - (uint32_t)offset;
        chunk = (uint16_t)(remain > limit ? limit : remain);
    }

    need = (uint16_t)(MB_DIAG_RSP_HEAD_LEN + 14 + chunk);
    if (need > resp_buf_size) {
        pthread_mutex_unlock(&g_jobs_mu);
        return -1;
    }

    mb_diag_write_rsp_head(resp_pdu, dev_addr, MB_DIAG_OP_JOB_RESULT,
                           MB_DIAG_RESULT_OK, (uint16_t)(14 + chunk));
    mb_diag_write_u32_be(resp_pdu + MB_DIAG_RSP_HEAD_LEN, job_id);
    resp_pdu[MB_DIAG_RSP_HEAD_LEN + 4] = state_code;
    resp_pdu[MB_DIAG_RSP_HEAD_LEN + 5] = part;
    mb_diag_write_u32_be(resp_pdu + MB_DIAG_RSP_HEAD_LEN + 6, total);
    mb_diag_write_u16_be(resp_pdu + MB_DIAG_RSP_HEAD_LEN + 10, offset);
    mb_diag_write_u16_be(resp_pdu + MB_DIAG_RSP_HEAD_LEN + 12, chunk);
    if (chunk > 0)
        memcpy(resp_pdu + MB_DIAG_RSP_HEAD_LEN + 14, src + offset, chunk);
    *resp_pdu_len = need;
    pthread_mutex_unlock(&g_jobs_mu);
    return 0;
}

/* ---------------- BYPASS (临时串口透传) ---------------- */

/*
 * START req payload (after 4B head):
 *   u8  chnl
 *   u8  reserved
 *   u16 listen_port BE  (0 => 7000+chnl)
 *   u16 lease_sec   BE  (0 => 600)
 */
static int mb_diag_handle_bypass_start(uint8_t dev_addr, const uint8_t *pdu, uint16_t pdu_len,
                                       uint8_t *resp_pdu, uint16_t resp_buf_size,
                                       uint16_t *resp_pdu_len)
{
	struct db_spl_realtime_data_tbl_t *pDB;
	struct db_spl_bypass_ctl_t *pb;
	uint8_t chnl;
	uint16_t listen_port;
	uint16_t lease_sec;
	char json[256];
	time_t now;

	if (pdu_len < MB_DIAG_REQ_HEAD_LEN + 6)
		return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
					  dev_addr, MB_DIAG_OP_BYPASS_START, MB_DIAG_RESULT_BAD_PARAM);

	chnl = pdu[4];
	listen_port = mb_diag_read_u16_be(pdu + 6);
	lease_sec = mb_diag_read_u16_be(pdu + 8);

	if (chnl < DB_BYPASS_SERIAL_CHNL_MIN || chnl > DB_BYPASS_SERIAL_CHNL_MAX)
		return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
					  dev_addr, MB_DIAG_OP_BYPASS_START, MB_DIAG_RESULT_BAD_PARAM);

	if (listen_port == 0)
		listen_port = (uint16_t)(DB_BYPASS_PORT_BASE + chnl);
	if (lease_sec == 0)
		lease_sec = DB_BYPASS_LEASE_SEC_DEFAULT;
	if (lease_sec > DB_BYPASS_LEASE_SEC_MAX)
		lease_sec = DB_BYPASS_LEASE_SEC_MAX;

	pDB = db_DataTblObj(NULL);
	pb = db_bypass_ctl(pDB);
	if (pb == NULL)
		return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
					  dev_addr, MB_DIAG_OP_BYPASS_START, MB_DIAG_RESULT_INTERNAL);

	now = time(NULL);
	/* 已有其它通道旁路且未到期 */
	if (pb->chnl != 0 && pb->chnl != (int32_t)chnl &&
	    (pb->deadline == 0 || (uint32_t)now < pb->deadline))
		return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
					  dev_addr, MB_DIAG_OP_BYPASS_START, MB_DIAG_RESULT_BUSY);

	/* 先写参数，最后写 chnl（读者以 chnl 为生效条件） */
	pb->listen_port = listen_port;
	pb->deadline = (uint32_t)now + lease_sec;
	pb->status = DB_BYPASS_STATUS_IDLE;
	pb->seq++;
	pb->chnl = (int32_t)chnl;

	snprintf(json, sizeof(json),
		 "{\"chnl\":%u,\"listen_port\":%u,\"lease_sec\":%u,\"deadline\":%u,\"seq\":%u}",
		 (unsigned)chnl, (unsigned)listen_port, (unsigned)lease_sec,
		 pb->deadline, pb->seq);
	return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
				 dev_addr, MB_DIAG_OP_BYPASS_START, MB_DIAG_RESULT_OK, json);
}

static int mb_diag_handle_bypass_stop(uint8_t dev_addr, const uint8_t *pdu, uint16_t pdu_len,
				      uint8_t *resp_pdu, uint16_t resp_buf_size,
				      uint16_t *resp_pdu_len)
{
	struct db_spl_bypass_ctl_t *pb;
	char json[128];
	uint8_t chnl = 0;

	(void)pdu_len;
	if (pdu_len >= MB_DIAG_REQ_HEAD_LEN + 1)
		chnl = pdu[4];

	pb = db_bypass_ctl(db_DataTblObj(NULL));
	if (pb == NULL)
		return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
					  dev_addr, MB_DIAG_OP_BYPASS_STOP, MB_DIAG_RESULT_INTERNAL);

	if (chnl != 0 && pb->chnl != 0 && pb->chnl != (int32_t)chnl)
		return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
					  dev_addr, MB_DIAG_OP_BYPASS_STOP, MB_DIAG_RESULT_DENIED);

	pb->chnl = 0;
	pb->deadline = 0;
	pb->status = DB_BYPASS_STATUS_IDLE;
	pb->seq++;

	snprintf(json, sizeof(json), "{\"chnl\":0,\"seq\":%u}", pb->seq);
	return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
				 dev_addr, MB_DIAG_OP_BYPASS_STOP, MB_DIAG_RESULT_OK, json);
}

static int mb_diag_handle_bypass_query(uint8_t dev_addr, uint8_t *resp_pdu,
				       uint16_t resp_buf_size, uint16_t *resp_pdu_len)
{
	struct db_spl_bypass_ctl_t *pb;
	char json[256];
	time_t now = time(NULL);

	pb = db_bypass_ctl(db_DataTblObj(NULL));
	if (pb == NULL)
		return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
					  dev_addr, MB_DIAG_OP_BYPASS_QUERY, MB_DIAG_RESULT_INTERNAL);

	/* 过期自闭合（modbus 侧也可协助） */
	if (pb->chnl != 0 && pb->deadline != 0 && (uint32_t)now >= pb->deadline) {
		pb->chnl = 0;
		pb->status = DB_BYPASS_STATUS_IDLE;
		pb->seq++;
	}

	snprintf(json, sizeof(json),
		 "{\"chnl\":%d,\"listen_port\":%u,\"deadline\":%u,\"status\":%u,\"seq\":%u,\"now\":%u}",
		 (int)pb->chnl, (unsigned)pb->listen_port, pb->deadline,
		 pb->status, pb->seq, (unsigned)now);
	return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
				 dev_addr, MB_DIAG_OP_BYPASS_QUERY, MB_DIAG_RESULT_OK, json);
}

/* ---------------- 工况摘要 0x13：通道 193 + UART TX/RX ---------------- */

#define MB_DIAG_WC_CHNL     193
#define MB_DIAG_WC_DEV      1
#define MB_DIAG_WC_IMEI     5
#define MB_DIAG_WC_ICCID    7
#define MB_DIAG_WC_CPU      10
#define MB_DIAG_WC_MEM      11
#define MB_DIAG_WC_UPTIME   12
#define MB_DIAG_WC_CSQ      13
#define MB_DIAG_WC_RSRP     14
#define MB_DIAG_WC_SIM      15
#define MB_DIAG_WC_CREG     16
#define MB_DIAG_WC_DISK     17

static void mb_diag_read_file_line(const char *path, char *out, size_t out_sz)
{
    FILE *fp;
    size_t n;

    if (!out || out_sz == 0)
        return;
    out[0] = '\0';
    if (!path)
        return;
    fp = fopen(path, "r");
    if (!fp)
        return;
    if (fgets(out, (int)out_sz, fp)) {
        n = strlen(out);
        while (n > 0 && (out[n - 1] == '\n' || out[n - 1] == '\r' ||
                         out[n - 1] == ' ' || out[n - 1] == '\t'))
            out[--n] = '\0';
    }
    fclose(fp);
}

static int mb_diag_read_wc_int(uint16_t idx, int *out)
{
    double lf = 0;

    *out = 0;
    if (DataMgmtFops.readpoint(MB_DIAG_WC_CHNL, MB_DIAG_WC_DEV, idx, &lf) < 0)
        return -1;
    *out = (int)lf;
    return 0;
}

static void mb_diag_append_uart(char *json, size_t json_sz, size_t *off)
{
    char buf[1536];
    FILE *fp;
    int n;
    int first = 1;
    char *line;
    char *nl;

    *off += (size_t)snprintf(json + *off, json_sz - *off, "\"uart\":[");
    fp = fopen("/proc/tty/driver/uart", "r");
    if (!fp)
        goto done;
    n = (int)fread(buf, 1, sizeof(buf) - 1, fp);
    fclose(fp);
    if (n <= 0)
        goto done;
    buf[n] = '\0';
    line = buf;
    while (line && *off + 64 < json_sz) {
        unsigned tx;
        unsigned rx;
        int id;
        char *ptx;
        char *prx;

        nl = strchr(line, '\n');
        if (nl)
            *nl = '\0';
        if (sscanf(line, "%d:", &id) == 1) {
            ptx = strstr(line, "tx:");
            prx = strstr(line, "rx:");
            if (ptx && prx) {
                tx = (unsigned)strtoul(ptx + 3, NULL, 10);
                rx = (unsigned)strtoul(prx + 3, NULL, 10);
                if (!first)
                    *off += (size_t)snprintf(json + *off, json_sz - *off, ",");
                first = 0;
                *off += (size_t)snprintf(json + *off, json_sz - *off,
                                         "{\"id\":%d,\"tx\":%u,\"rx\":%u}",
                                         id, tx, rx);
            }
        }
        line = nl ? (nl + 1) : NULL;
    }
done:
    *off += (size_t)snprintf(json + *off, json_sz - *off, "]");
}

static int mb_diag_handle_res_summary(uint8_t dev_addr, uint8_t *resp_pdu,
                                      uint16_t resp_buf_size, uint16_t *resp_pdu_len)
{
    char json[MB_DIAG_JSON_BUF_MAX];
    size_t off = 0;
    char sn[40];
    char model[40];
    char imei[16];
    char iccid[24];
    int sim = 0, creg = 0, csq = -1, rsrp = -1;
    int cpu = 0, mem = 0, disk = 0, uptime = 0;

    memset(imei, 0, sizeof(imei));
    memset(iccid, 0, sizeof(iccid));
    mb_diag_read_file_line("/userdata/system/sn", sn, sizeof(sn));
    mb_diag_read_file_line("/userdata/system/model", model, sizeof(model));
    if (DataMgmtFops.readpoint_string(MB_DIAG_WC_CHNL, MB_DIAG_WC_DEV,
                                      MB_DIAG_WC_IMEI, imei, sizeof(imei)) < 0)
        imei[0] = '\0';
    imei[sizeof(imei) - 1] = '\0';
    if (DataMgmtFops.readpoint_string(MB_DIAG_WC_CHNL, MB_DIAG_WC_DEV,
                                      MB_DIAG_WC_ICCID, iccid, sizeof(iccid)) < 0)
        iccid[0] = '\0';
    iccid[sizeof(iccid) - 1] = '\0';

    mb_diag_read_wc_int(MB_DIAG_WC_SIM, &sim);
    mb_diag_read_wc_int(MB_DIAG_WC_CREG, &creg);
    if (mb_diag_read_wc_int(MB_DIAG_WC_CSQ, &csq) < 0)
        csq = -1;
    if (mb_diag_read_wc_int(MB_DIAG_WC_RSRP, &rsrp) < 0)
        rsrp = -1;
    mb_diag_read_wc_int(MB_DIAG_WC_CPU, &cpu);
    mb_diag_read_wc_int(MB_DIAG_WC_MEM, &mem);
    mb_diag_read_wc_int(MB_DIAG_WC_DISK, &disk);
    mb_diag_read_wc_int(MB_DIAG_WC_UPTIME, &uptime);

    off += (size_t)snprintf(json + off, sizeof(json) - off, "{\"sn\":\"");
    mb_diag_json_escape_append(json, sizeof(json), &off, sn);
    off += (size_t)snprintf(json + off, sizeof(json) - off, "\",\"model\":\"");
    mb_diag_json_escape_append(json, sizeof(json), &off, model);
    off += (size_t)snprintf(json + off, sizeof(json) - off, "\",\"imei\":\"");
    mb_diag_json_escape_append(json, sizeof(json), &off, imei);
    off += (size_t)snprintf(json + off, sizeof(json) - off, "\",\"iccid\":\"");
    mb_diag_json_escape_append(json, sizeof(json), &off, iccid);
    off += (size_t)snprintf(json + off, sizeof(json) - off,
                            "\",\"sim_ready\":%s,\"creg\":%d,\"csq\":%d,\"rsrp\":%d,"
                            "\"cpu\":%d,\"memory\":%d,\"disk\":%d,\"uptime\":%d,",
                            sim ? "true" : "false", creg, csq, rsrp,
                            cpu, mem, disk, uptime);
    mb_diag_append_uart(json, sizeof(json), &off);
    if (off + 2 >= sizeof(json))
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, MB_DIAG_OP_RES_SUMMARY,
                                  MB_DIAG_RESULT_INTERNAL);
    off += (size_t)snprintf(json + off, sizeof(json) - off, "}");
    return mb_diag_emit_json(resp_pdu, resp_buf_size, resp_pdu_len,
                             dev_addr, MB_DIAG_OP_RES_SUMMARY,
                             MB_DIAG_RESULT_OK, json);
}

/* ---------------- entry ---------------- */

int mb_diagnose_handle(uint8_t dev_addr,
                       const uint8_t *pdu,
                       uint16_t pdu_len,
                       uint8_t *resp_pdu,
                       uint16_t resp_buf_size,
                       uint16_t *resp_pdu_len)
{
    uint8_t op;

    if (!pdu || !resp_pdu || !resp_pdu_len || pdu_len < MB_DIAG_REQ_HEAD_LEN)
        return -1;
    if (pdu[1] != _FC_DIAGNOSE)
        return -1;

    op = pdu[2];

    switch (op) {
    case MB_DIAG_OP_CAPS:
        return mb_diag_handle_caps(dev_addr, resp_pdu, resp_buf_size, resp_pdu_len);
    case MB_DIAG_OP_NETIF_LIST:
        return mb_diag_handle_netif(dev_addr, resp_pdu, resp_buf_size, resp_pdu_len);
    case MB_DIAG_OP_ROUTE_LIST:
        return mb_diag_handle_route(dev_addr, resp_pdu, resp_buf_size, resp_pdu_len);
    case MB_DIAG_OP_CONN_LIST:
        return mb_diag_handle_conn(dev_addr, pdu, pdu_len, resp_pdu, resp_buf_size,
                                   resp_pdu_len);
    case MB_DIAG_OP_DNS_RESOLVE:
        return mb_diag_handle_dns(dev_addr, pdu, pdu_len, resp_pdu, resp_buf_size,
                                  resp_pdu_len);
    case MB_DIAG_OP_TCP_PROBE:
        return mb_diag_handle_tcp(dev_addr, pdu, pdu_len, resp_pdu, resp_buf_size,
                                  resp_pdu_len);
    case MB_DIAG_OP_JOB_START:
        return mb_diag_handle_job_start(dev_addr, pdu, pdu_len, resp_pdu,
                                        resp_buf_size, resp_pdu_len);
    case MB_DIAG_OP_JOB_STATUS:
        return mb_diag_handle_job_status(dev_addr, pdu, pdu_len, resp_pdu,
                                         resp_buf_size, resp_pdu_len);
    case MB_DIAG_OP_JOB_RESULT:
        return mb_diag_handle_job_result(dev_addr, pdu, pdu_len, resp_pdu,
                                         resp_buf_size, resp_pdu_len);
    case MB_DIAG_OP_JOB_CANCEL:
        return mb_diag_handle_job_cancel(dev_addr, pdu, pdu_len, resp_pdu,
                                         resp_buf_size, resp_pdu_len);
    case MB_DIAG_OP_BYPASS_START:
        return mb_diag_handle_bypass_start(dev_addr, pdu, pdu_len, resp_pdu,
                                           resp_buf_size, resp_pdu_len);
    case MB_DIAG_OP_BYPASS_STOP:
        return mb_diag_handle_bypass_stop(dev_addr, pdu, pdu_len, resp_pdu,
                                          resp_buf_size, resp_pdu_len);
    case MB_DIAG_OP_BYPASS_QUERY:
        return mb_diag_handle_bypass_query(dev_addr, resp_pdu, resp_buf_size,
                                           resp_pdu_len);
    case MB_DIAG_OP_RES_SUMMARY:
        return mb_diag_handle_res_summary(dev_addr, resp_pdu, resp_buf_size,
                                          resp_pdu_len);
    case MB_DIAG_OP_JOB_LIST:
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, op, MB_DIAG_RESULT_UNSUPPORTED);
    default:
        return mb_diag_emit_empty(resp_pdu, resp_buf_size, resp_pdu_len,
                                  dev_addr, op, MB_DIAG_RESULT_UNSUPPORTED);
    }
}
