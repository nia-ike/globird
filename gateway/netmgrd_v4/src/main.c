#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <inttypes.h>
#include <getopt.h>
#include "netmgr_v4.h"
#include "netmgr_v4_adapter.h"
#include "netmgr_v4_config.h"
#include "netmgr_v4_gateway.h"
#include "netmgr_v4_linkkeeper.h"
#include "netmgr_v4_manager.h"
#include "netmgr_v4_apply.h"
#include "netmgr_v4_policy.h"
#include "netmgr_v4_probe.h"
#include "netmgr_v4_route.h"
#include "adapter_internal.h"

/*********************************************************************************************************
** Function name:       netmgr_v4_run
** Descriptions:        Placeholder library entry when no CLI action is selected (reserved for future use).
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      Always 0.
*********************************************************************************************************/
int netmgr_v4_run(void)
{
    return 0;
}

static volatile sig_atomic_t g_linkkeeper_stop;

/*********************************************************************************************************
** Function name:       now_ms_main
** Descriptions:        Current wall time in milliseconds for linkkeeper-run loop timing.
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      Milliseconds since epoch.
*********************************************************************************************************/
static uint64_t now_ms_main(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
}

/*********************************************************************************************************
** Function name:       on_linkkeeper_stop
** Descriptions:        Signal handler to stop --linkkeeper-run loop on SIGINT/SIGTERM.
** input parameters:    signo   - signal number (ignored).
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void on_linkkeeper_stop(int signo)
{
    (void)signo;
    g_linkkeeper_stop = 1;
}

/*********************************************************************************************************
** Function name:       nm4_collect_bits
** Descriptions:        Probe eth0/wlan0/ppp0 with default params and fill aggregate up bits.
** input parameters:    bits    - output aggregate bits struct.
** output parameters:    bits    - eth_up/wlan_up/ppp_up set from probe results.
** Returned value:      NONE
*********************************************************************************************************/
static void nm4_collect_bits(nm4_bits_t *bits)
{
    nm4_probe_params_t p;
    nm4_probe_result_t r;
    memset(bits, 0, sizeof(*bits));
    nm4_probe_params_default(&p);
    if (nm4_probe_one("eth0", &p, &r) == 0)
        bits->eth_up = r.if_exists && r.link_up && r.internet_ok;
    if (nm4_probe_one("wlan0", &p, &r) == 0)
        bits->wlan_up = r.if_exists && r.link_up && r.internet_ok;
    if (nm4_probe_one("ppp0", &p, &r) == 0)
        bits->ppp_up = r.if_exists && r.link_up && r.internet_ok;
}

/*********************************************************************************************************
** Function name:       nm4_render_linkkeeper_table
** Descriptions:        Redraw linkkeeper TTY table with time, policy bits, want/have, LK stats.
** input parameters:    (multiple) now, tick, interval, cfgpath, lk, bits, want, have.
** output parameters:   NONE (prints to stdout).
** Returned value:      NONE
*********************************************************************************************************/
static void nm4_render_linkkeeper_table(
    uint64_t now,
    uint64_t tick,
    uint64_t interval_ms,
    const char *cfgpath,
    const nm4_linkkeeper_t *lk,
    const nm4_bits_t *bits,
    nm4_want_t want,
    nm4_want_t have)
{
    time_t t = (time_t)(now / 1000UL);
    struct tm tmv;
    char tbuf[32];
    char wifi_due_buf[16] = "--:--:--";
    char ppp_due_buf[16] = "--:--:--";
    uint64_t wifi_next_s = 0;
    uint64_t ppp_next_s = 0;

    localtime_r(&t, &tmv);
    (void)strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", &tmv);
    if (lk->last_wifi_keep_ms > 0) {
        time_t wt = (time_t)(lk->last_wifi_keep_ms / 1000UL);
        struct tm wtm;
        localtime_r(&wt, &wtm);
        (void)strftime(wifi_due_buf, sizeof(wifi_due_buf), "%H:%M:%S", &wtm);
    }
    if (lk->last_ppp_keep_ms > 0) {
        time_t pt = (time_t)(lk->last_ppp_keep_ms / 1000UL);
        struct tm ptm;
        localtime_r(&pt, &ptm);
        (void)strftime(ppp_due_buf, sizeof(ppp_due_buf), "%H:%M:%S", &ptm);
    }
    wifi_next_s = (lk->wifi_next_in_ms + 999UL) / 1000UL;
    ppp_next_s = (lk->ppp_next_in_ms + 999UL) / 1000UL;

    printf("\033[H");
    printf("netmgrd_v4 linkkeeper monitor  mode=table  interval=%" PRIu64 "ms  cfg=%s\n",
           interval_ms, cfgpath);
    printf("time=%s  tick=%" PRIu64 "  R=%u(bits=%d%d%d)  want=%s  have=%s  pending=n/a\n\n",
           tbuf, tick, (unsigned)nm4_policy_to_r(bits),
           bits->eth_up, bits->wlan_up, bits->ppp_up,
           nm4_policy_want_name(want), nm4_policy_want_name(have));
    printf("+-------+---------+----------+-----------+---------+--------+----------+----------------------+\n");
    printf("| iface | enabled | last_due | next_in_s | last_rc | ok_cnt | fail_cnt | last_msg             |\n");
    printf("+-------+---------+----------+-----------+---------+--------+----------+----------------------+\n");
    printf("| eth0  | yes     | n/a      | n/a       | n/a     | n/a    | n/a      | not managed by LK    |\n");
    printf("| wlan0 | %-7s | %-8s | %-9" PRIu64 " | %-7s | %-6" PRIu64 " | %-8" PRIu64 " | %-20.20s |\n",
           lk->has_wifi ? "yes" : "no",
           wifi_due_buf,
           wifi_next_s,
           lk->wifi_last_rc == 0 ? "OK" : "FAIL",
           lk->wifi_ok_cnt,
           lk->wifi_fail_cnt,
           lk->wifi_last_msg);
    printf("| ppp0  | %-7s | %-8s | %-9" PRIu64 " | %-7s | %-6" PRIu64 " | %-8" PRIu64 " | %-20.20s |\n",
           lk->has_ppp ? "yes" : "no",
           ppp_due_buf,
           ppp_next_s,
           lk->ppp_last_rc == 0 ? "OK" : "FAIL",
           lk->ppp_ok_cnt,
           lk->ppp_fail_cnt,
           lk->ppp_last_msg);
    printf("+-------+---------+----------+-----------+---------+--------+----------+----------------------+\n");
    printf("hint: Ctrl+C to stop\n");
    (void)fflush(stdout);
}

/*********************************************************************************************************
** Function name:       usage
** Descriptions:        Print program help text to stdout.
** input parameters:    p       - argv[0] program name for Usage line.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void usage(const char *p)
{
    printf(
        "Usage: %s [options]\n"
        "\n"
        "General:\n"
        "  -C, --config FILE      配置文件（默认: %s）\n"
        "  -d, --debug            调试日志\n"
        "  -V, --version          打印版本\n"
        "  -h, --help             显示帮助\n"
        "\n"
        "Adapter actions (single-shot, one at a time):\n"
        "  -w, --wifi-up          启动 WiFi（默认从配置读；可用 -i/-f/-s/-k 覆盖）\n"
        "  -W, --wifi-down        关闭 WiFi（无 -i 时从配置取 ifname）\n"
        "  -p, --ppp-up           启动 PPP（默认从配置读；可用 -i/-D/-a/-U/-K/-T 覆盖）\n"
        "  -P, --ppp-down         关闭 PPP（无 -i 时从配置取 ifname）\n"
        "\n"
        "Adapter override params:\n"
        "  -i, --ifname IF        接口名（wlan0/ppp0）\n"
        "  -f, --wpa-conf FILE    WiFi: 直接指定 wpa 配置\n"
        "  -s, --ssid SSID        WiFi: SSID（需配合 -k）\n"
        "  -k, --psk PSK          WiFi: PSK（需配合 -s）\n"
        "  -D, --device DEV       PPP: 串口设备（如 /dev/ttyUSB0）\n"
        "  -a, --apn APN          PPP: APN\n"
        "  -U, --user USER        PPP: 用户名\n"
        "  -K, --password PASS    PPP: 密码\n"
        "  -T, --dial DIAL        PPP: 拨号号（默认 *99#）\n"
        "\n"
        "Module tests:\n"
        "  -N, --net-probe        探测模块：存在/链路/IPv4/ICMP（可配 -i）\n"
        "  -M, --managed          方案B守护：LinkKeeper + R策略 + RouteApply\n"
        "      --apply            通知已运行的 -M 守护：touch /var/run/netmgr/apply 原地重读 network.json（不换 PID）\n"
        "      --managed-view MODE  managed 显示模式(log|table)\n"
        "      --policy-bits XYZ  policy 测试：X/Y/Z=eth/wlan/ppp(0/1)\n"
        "      --route-get        route 测试：读取当前默认出口(have)\n"
        "      --route-apply IF   route 测试：应用默认出口(IF=none|eth0|wlan0|ppp0)\n"
        "      --linkkeeper-once  linkkeeper 测试：按配置执行一次保活 tick\n"
        "      --linkkeeper-run   linkkeeper 持续运行（Ctrl+C 退出）\n"
        "      --linkkeeper-interval MS  linkkeeper-run 循环间隔（默认1000ms）\n"
        "      --linkkeeper-view MODE  linkkeeper-run 显示模式(log|table)\n"
        "      --lk-wifi-ms MS   linkkeeper: wlan0 保活周期（默认10000ms）\n"
        "      --lk-ppp-ms MS    linkkeeper: ppp0 保活周期（默认15000ms）\n"
        "      --gateway IF       gateway 测试：解析 IF 网关\n",
        p, NM4_CONFIG_DEFAULT_PATH);
}

/*********************************************************************************************************
** Function name:       main
** Descriptions:        CLI entry: parse options, run one-shot actions or managed daemon, or idle stub.
** input parameters:    argc    - argument count.
**                      argv    - argument vector.
** output parameters:   NONE
** Returned value:      Process exit code 0/1 per action outcome.
*********************************************************************************************************/
int main(int argc, char *argv[])
{
    static const struct option longopts[] = {
        { "debug",     no_argument,       NULL, 'd' },
        { "version",   no_argument,       NULL, 'V' },
        { "help",      no_argument,       NULL, 'h' },
        { "wifi-up",   no_argument,       NULL, 'w' },
        { "wifi-down", no_argument,       NULL, 'W' },
        { "ppp-up",    no_argument,       NULL, 'p' },
        { "ppp-down",  no_argument,       NULL, 'P' },
        { "ifname",    required_argument, NULL, 'i' },
        { "config",    required_argument, NULL, 'C' },
        { "wpa-conf",  required_argument, NULL, 'f' },
        { "ssid",      required_argument, NULL, 's' },
        { "psk",       required_argument, NULL, 'k' },
        { "device",    required_argument, NULL, 'D' },
        { "apn",       required_argument, NULL, 'a' },
        { "user",      required_argument, NULL, 'U' },
        { "password",  required_argument, NULL, 'K' },
        { "dial",      required_argument, NULL, 'T' },
        { "net-probe", no_argument,       NULL, 'N' },
        { "managed",   no_argument,       NULL, 'M' },
        { "apply",     no_argument,       NULL, 1012 },
        { "managed-view", required_argument, NULL, 1009 },
        { "policy-bits", required_argument, NULL, 1001 },
        { "route-get",   no_argument,       NULL, 1002 },
        { "route-apply", required_argument, NULL, 1003 },
        { "linkkeeper-once", no_argument,   NULL, 1004 },
        { "gateway", required_argument,     NULL, 1005 },
        { "linkkeeper-run", no_argument,    NULL, 1006 },
        { "linkkeeper-interval", required_argument, NULL, 1007 },
        { "linkkeeper-view", required_argument, NULL, 1008 },
        { "lk-wifi-ms", required_argument,  NULL, 1010 },
        { "lk-ppp-ms", required_argument,   NULL, 1011 },
        { NULL, 0, NULL, 0 }
    };

    int c;
    bool debug = false;
    int do_w = 0, do_W = 0, do_p = 0, do_P = 0, do_N = 0, do_M = 0, do_apply = 0;
    int do_policy_bits = 0, do_route_get = 0, do_route_apply = 0, do_linkkeeper_once = 0, do_gateway = 0;
    int do_linkkeeper_run = 0;
    int have_s = 0, have_k = 0, have_f = 0, have_i = 0;
    int have_D = 0, have_a = 0, have_U = 0, have_K = 0, have_T = 0;
    char ifbuf[NM4_IFNAME_MAX] = "";
    char bitsbuf[8] = "";
    char managed_view[16] = "log";
    char lk_view[16] = "log";
    char route_apply_if[16] = "";
    char gw_if[NM4_IFNAME_MAX] = "";
    uint64_t lk_interval_ms = 1000ULL;
    uint64_t lk_wifi_ms = 10000ULL;
    uint64_t lk_ppp_ms = 15000ULL;
    char cfgpath[NM4_PATH_MAX];
    char err[256];
    char ov_ssid[64] = "", ov_psk[64] = "", ov_wconf[NM4_PATH_MAX] = "";
    char ov_D[NM4_PATH_MAX] = "", ov_apn[64] = "", ov_U[32] = "", ov_Pw[64] = "", ov_dial[32] = "";

    nm4_wpa_params_t wpa;
    nm4_ppp_params_t ppp;
    nm4_linkkeeper_t lk;
    memset(&wpa, 0, sizeof(wpa));
    memset(&ppp, 0, sizeof(ppp));
    snprintf(ppp.dial, sizeof(ppp.dial), "*99#");
    snprintf(cfgpath, sizeof(cfgpath), "%s", NM4_CONFIG_DEFAULT_PATH);

    while ((c = getopt_long(argc, argv, "dVhwpWPi:C:f:s:k:D:a:U:K:T:NM", longopts, NULL)) != -1) {
        switch (c) {
        case 'd': debug = true; break;
        case 'V': printf("%s\n", NETMGR_V4_VERSION); return 0;
        case 'h': usage(argv[0]); return 0;
        case 'w': do_w = 1; break;
        case 'W': do_W = 1; break;
        case 'p': do_p = 1; break;
        case 'P': do_P = 1; break;
        case 'i':
            have_i = 1;
            snprintf(ifbuf, sizeof(ifbuf), "%s", optarg);
            break;
        case 'C':
            snprintf(cfgpath, sizeof(cfgpath), "%s", optarg);
            break;
        case 'f':
            have_f = 1;
            snprintf(ov_wconf, sizeof(ov_wconf), "%s", optarg);
            break;
        case 's':
            have_s = 1;
            snprintf(ov_ssid, sizeof(ov_ssid), "%s", optarg);
            break;
        case 'k':
            have_k = 1;
            snprintf(ov_psk, sizeof(ov_psk), "%s", optarg);
            break;
        case 'D':
            have_D = 1;
            snprintf(ov_D, sizeof(ov_D), "%s", optarg);
            break;
        case 'a':
            have_a = 1;
            snprintf(ov_apn, sizeof(ov_apn), "%s", optarg);
            break;
        case 'U':
            have_U = 1;
            snprintf(ov_U, sizeof(ov_U), "%s", optarg);
            break;
        case 'K':
            have_K = 1;
            snprintf(ov_Pw, sizeof(ov_Pw), "%s", optarg);
            break;
        case 'T':
            have_T = 1;
            snprintf(ov_dial, sizeof(ov_dial), "%s", optarg);
            break;
        case 'N': do_N = 1; break;
        case 'M': do_M = 1; break;
        case 1012:
            do_apply = 1;
            break;
        case 1001:
            do_policy_bits = 1;
            snprintf(bitsbuf, sizeof(bitsbuf), "%s", optarg);
            break;
        case 1002:
            do_route_get = 1;
            break;
        case 1003:
            do_route_apply = 1;
            snprintf(route_apply_if, sizeof(route_apply_if), "%s", optarg);
            break;
        case 1004:
            do_linkkeeper_once = 1;
            break;
        case 1005:
            do_gateway = 1;
            snprintf(gw_if, sizeof(gw_if), "%s", optarg);
            break;
        case 1006:
            do_linkkeeper_run = 1;
            break;
        case 1007:
            lk_interval_ms = (uint64_t)strtoull(optarg, NULL, 10);
            if (lk_interval_ms == 0) lk_interval_ms = 1000ULL;
            break;
        case 1008:
            snprintf(lk_view, sizeof(lk_view), "%s", optarg);
            break;
        case 1009:
            snprintf(managed_view, sizeof(managed_view), "%s", optarg);
            break;
        case 1010:
            lk_wifi_ms = (uint64_t)strtoull(optarg, NULL, 10);
            if (lk_wifi_ms == 0) lk_wifi_ms = 10000ULL;
            break;
        case 1011:
            lk_ppp_ms = (uint64_t)strtoull(optarg, NULL, 10);
            if (lk_ppp_ms == 0) lk_ppp_ms = 15000ULL;
            break;
        default: usage(argv[0]); return 1;
        }
    }

    {
        int nact = (do_w ? 1 : 0) + (do_W ? 1 : 0) + (do_p ? 1 : 0) +
                   (do_P ? 1 : 0) + (do_N ? 1 : 0) + (do_M ? 1 : 0) +
                   (do_policy_bits ? 1 : 0) + (do_route_get ? 1 : 0) +
                   (do_route_apply ? 1 : 0) + (do_linkkeeper_once ? 1 : 0) +
                   (do_gateway ? 1 : 0) + (do_linkkeeper_run ? 1 : 0) +
                   (do_apply ? 1 : 0);
        if (nact > 1) {
            fprintf(stderr, "only one action at a time (-w/-W/-p/-P/-N/-M/--apply/--policy-bits/--route-get/--route-apply/--linkkeeper-once/--linkkeeper-run/--gateway)\n");
            return 1;
        }

        if (nact) {
            int rc;
            netmgr_v4_log_init(debug);
            nm4_linkkeeper_set_intervals(lk_wifi_ms, lk_ppp_ms);

            if (do_apply) {
                printf("[netmgrd_v4] action: apply (touch %s)\n", NM4_APPLY_FLAG);
                (void)fflush(stdout);
                return nm4_apply_client(0);
            }

            if (do_w) {
                int manual = (have_f || (have_s && have_k)) ? 1 : 0;

                if (manual) {
                    if (have_f) {
                        snprintf(wpa.wpa_conf, sizeof(wpa.wpa_conf), "%s", ov_wconf);
                    } else {
                        snprintf(wpa.ssid, sizeof(wpa.ssid), "%s", ov_ssid);
                        snprintf(wpa.psk, sizeof(wpa.psk), "%s", ov_psk);
                    }
                } else {
                    if (nm4_config_load_wifi_from_v3_json(cfgpath, &wpa, err, sizeof(err)) != 0) {
                        fprintf(stderr, "[netmgrd_v4] config: %s\n", err);
                        return 1;
                    }
                    if (have_f)
                        snprintf(wpa.wpa_conf, sizeof(wpa.wpa_conf), "%s", ov_wconf);
                    if (have_s)
                        snprintf(wpa.ssid, sizeof(wpa.ssid), "%s", ov_ssid);
                    if (have_k)
                        snprintf(wpa.psk, sizeof(wpa.psk), "%s", ov_psk);
                }
                if (have_i)
                    snprintf(wpa.ifname, sizeof(wpa.ifname), "%s", ifbuf);
                else if (wpa.ifname[0] == '\0')
                    snprintf(wpa.ifname, sizeof(wpa.ifname), "wlan0");

                printf("[netmgrd_v4] config: %s\n", cfgpath);
                printf("[netmgrd_v4] action: WiFi up\n");
                if (wpa.wpa_conf[0])
                    printf("[netmgrd_v4]   ifname=%s wpa_conf=%s\n", wpa.ifname, wpa.wpa_conf);
                else
                    printf("[netmgrd_v4]   ifname=%s ssid=%s psk=%s\n",
                           wpa.ifname, wpa.ssid, wpa.psk);
                (void)fflush(stdout);

                rc = nm4_adapter_wpa_start(&wpa);
                printf("[netmgrd_v4] result: %s (exit %d)\n", (rc == 0 ? "OK" : "FAIL"), rc);
                return rc != 0 ? 1 : 0;
            }
            if (do_W) {
                nm4_wpa_params_t tmp;
                if (!have_i) {
                    if (nm4_config_load_wifi_from_v3_json(cfgpath, &tmp, err, sizeof(err)) == 0 &&
                        tmp.ifname[0])
                        snprintf(ifbuf, sizeof(ifbuf), "%s", tmp.ifname);
                    else
                        snprintf(ifbuf, sizeof(ifbuf), "wlan0");
                } else {
                    /* ifbuf 已有 */
                }
                printf("[netmgrd_v4] config: %s (for ifname)\n", cfgpath);
                printf("[netmgrd_v4] action: WiFi down ifname=%s\n", ifbuf);
                (void)fflush(stdout);
                rc = nm4_adapter_wpa_stop(ifbuf);
                printf("[netmgrd_v4] result: %s (exit %d)\n", (rc == 0 ? "OK" : "FAIL"), rc);
                return rc != 0 ? 1 : 0;
            }
            if (do_p) {
                int manual = (have_D && have_a) ? 1 : 0;

                if (manual) {
                    snprintf(ppp.device, sizeof(ppp.device), "%s", ov_D);
                    snprintf(ppp.apn, sizeof(ppp.apn), "%s", ov_apn);
                    if (have_U) snprintf(ppp.user, sizeof(ppp.user), "%s", ov_U);
                    if (have_K) snprintf(ppp.password, sizeof(ppp.password), "%s", ov_Pw);
                    if (have_T) snprintf(ppp.dial, sizeof(ppp.dial), "%s", ov_dial);
                    if (!have_i) snprintf(ppp.ifname, sizeof(ppp.ifname), "ppp0");
                    else snprintf(ppp.ifname, sizeof(ppp.ifname), "%s", ifbuf);
                } else {
                    if (nm4_config_load_ppp_from_v3_json(cfgpath, &ppp, err, sizeof(err)) != 0) {
                        fprintf(stderr, "[netmgrd_v4] config: %s\n", err);
                        return 1;
                    }
                    if (have_D) snprintf(ppp.device, sizeof(ppp.device), "%s", ov_D);
                    if (have_a) snprintf(ppp.apn, sizeof(ppp.apn), "%s", ov_apn);
                    if (have_U) snprintf(ppp.user, sizeof(ppp.user), "%s", ov_U);
                    if (have_K) snprintf(ppp.password, sizeof(ppp.password), "%s", ov_Pw);
                    if (have_T) snprintf(ppp.dial, sizeof(ppp.dial), "%s", ov_dial);
                    if (have_i) snprintf(ppp.ifname, sizeof(ppp.ifname), "%s", ifbuf);
                }
                if (ppp.ifname[0] == '\0') snprintf(ppp.ifname, sizeof(ppp.ifname), "ppp0");
                if (ppp.dial[0] == '\0') snprintf(ppp.dial, sizeof(ppp.dial), "*99#");

                printf("[netmgrd_v4] config: %s\n", cfgpath);
                printf("[netmgrd_v4] action: PPP up\n");
                printf("[netmgrd_v4]   ifname=%s device=%s apn=%s user=%s password=%s dial=%s\n",
                       ppp.ifname, ppp.device, ppp.apn, ppp.user,
                       (ppp.password[0] ? "(set)" : ""), ppp.dial);
                (void)fflush(stdout);

                rc = nm4_adapter_ppp_start(&ppp);
                printf("[netmgrd_v4] result: %s (exit %d)\n", (rc == 0 ? "OK" : "FAIL"), rc);
                return rc != 0 ? 1 : 0;
            }
            if (do_P) {
                nm4_ppp_params_t tmp;
                if (!have_i) {
                    if (nm4_config_load_ppp_from_v3_json(cfgpath, &tmp, err, sizeof(err)) == 0 &&
                        tmp.ifname[0])
                        snprintf(ifbuf, sizeof(ifbuf), "%s", tmp.ifname);
                    else
                        snprintf(ifbuf, sizeof(ifbuf), "ppp0");
                }
                printf("[netmgrd_v4] config: %s (for ifname)\n", cfgpath);
                printf("[netmgrd_v4] action: PPP down ifname=%s\n", ifbuf);
                (void)fflush(stdout);
                rc = nm4_adapter_ppp_stop(ifbuf);
                printf("[netmgrd_v4] result: %s (exit %d)\n", (rc == 0 ? "OK" : "FAIL"), rc);
                return rc != 0 ? 1 : 0;
            }
            if (do_N) {
                int i, n;
                char list[NM4_NETLIST_MAX][NM4_IFNAME_MAX];
                nm4_probe_params_t p;
                nm4_probe_result_t r;

                netmgr_v4_log_init(debug);
                nm4_probe_init();
                nm4_probe_params_default(&p);
                if (have_i) {
                    n = 1;
                    (void)snprintf(list[0], sizeof(list[0]), "%s", ifbuf);
                } else {
                    if (nm4_config_list_enabled_ifaces(cfgpath, list, &n, err, sizeof(err)) != 0) {
                        fprintf(stderr, "[netmgrd_v4] %s\n", err);
                        return 1;
                    }
                }
                printf("[netmgrd_v4] net probe (config: %s)\n", cfgpath);
                printf("%-10s  %-7s  %-4s  %-4s  %-4s\n",
                       "ifname", "exists", "link", "ipv4", "inet");
                for (i = 0; i < n; i++) {
                    if (nm4_probe_one(list[i], &p, &r) != 0) {
                        fprintf(stderr, "[netmgrd_v4] probe %s: internal error\n", list[i]);
                        return 1;
                    }
                    printf("%-10s  %-7s  %-4s  %-4s  %-4s\n",
                           list[i],
                           r.if_exists ? "yes" : "no",
                           r.link_up ? "up" : "down",
                           r.ipv4_ok ? "yes" : "no",
                           r.internet_ok ? "ok" : "no");
                }
                return 0;
            }
            if (do_M) {
                nm4_manager_view_t view = NM4_MANAGER_VIEW_LOG;
                if (strcmp(managed_view, "table") == 0) view = NM4_MANAGER_VIEW_TABLE;
                else if (strcmp(managed_view, "log") == 0) view = NM4_MANAGER_VIEW_LOG;
                else {
                    fprintf(stderr, "--managed-view MODE must be log|table\n");
                    return 1;
                }
                return nm4_manager_run(cfgpath, debug, view) == 0 ? 0 : 1;
            }
            if (do_policy_bits) {
                nm4_bits_t b;
                uint8_t r;
                nm4_want_t w;
                if (strlen(bitsbuf) != 3 ||
                    (bitsbuf[0] != '0' && bitsbuf[0] != '1') ||
                    (bitsbuf[1] != '0' && bitsbuf[1] != '1') ||
                    (bitsbuf[2] != '0' && bitsbuf[2] != '1')) {
                    fprintf(stderr, "--policy-bits requires 3 chars 0/1, e.g. 101\n");
                    return 1;
                }
                b.eth_up = (bitsbuf[0] == '1');
                b.wlan_up = (bitsbuf[1] == '1');
                b.ppp_up = (bitsbuf[2] == '1');
                r = nm4_policy_to_r(&b);
                w = nm4_policy_pick_want(&b, NULL);
                printf("[netmgrd_v4] policy bits=%s -> R=%u want=%s\n",
                       bitsbuf, (unsigned)r, nm4_policy_want_name(w));
                return 0;
            }
            if (do_route_get) {
                nm4_want_t have = nm4_route_get_have();
                printf("[netmgrd_v4] route have=%s\n", nm4_policy_want_name(have));
                return 0;
            }
            if (do_route_apply) {
                nm4_want_t w = NM4_WANT_NONE;
                int rc;
                if (strcmp(route_apply_if, "none") == 0) w = NM4_WANT_NONE;
                else if (strcmp(route_apply_if, "eth0") == 0) w = NM4_WANT_ETH0;
                else if (strcmp(route_apply_if, "wlan0") == 0) w = NM4_WANT_WLAN0;
                else if (strcmp(route_apply_if, "ppp0") == 0) w = NM4_WANT_PPP0;
                else {
                    fprintf(stderr, "--route-apply IF must be one of: none|eth0|wlan0|ppp0\n");
                    return 1;
                }
                rc = nm4_route_apply(w);
                printf("[netmgrd_v4] route apply want=%s rc=%d\n", nm4_policy_want_name(w), rc);
                return rc == 0 ? 0 : 1;
            }
            if (do_linkkeeper_once) {
                memset(&lk, 0, sizeof(lk));
                if (nm4_linkkeeper_init(&lk, cfgpath, err, sizeof(err)) != 0) {
                    fprintf(stderr, "[netmgrd_v4] linkkeeper init: %s\n", err);
                    return 1;
                }
                netmgr_v4_log_init(debug);
                nm4_linkkeeper_tick(&lk, 0);
                printf("[netmgrd_v4] linkkeeper once done (wifi=%d ppp=%d)\n",
                       lk.has_wifi, lk.has_ppp);
                return 0;
            }
            if (do_linkkeeper_run) {
                int table_mode;
                uint64_t tick = 0;
                nm4_bits_t bits;
                nm4_want_t want;
                nm4_want_t have;

                memset(&lk, 0, sizeof(lk));
                if (nm4_linkkeeper_init(&lk, cfgpath, err, sizeof(err)) != 0) {
                    fprintf(stderr, "[netmgrd_v4] linkkeeper init: %s\n", err);
                    return 1;
                }
                if (strcmp(lk_view, "log") != 0 && strcmp(lk_view, "table") != 0) {
                    fprintf(stderr, "--linkkeeper-view MODE must be log|table\n");
                    return 1;
                }
                table_mode = (strcmp(lk_view, "table") == 0) && isatty(STDOUT_FILENO);
                netmgr_v4_log_init(table_mode ? false : debug);
                nm4_linkkeeper_set_log_enabled(table_mode ? 0 : 1);
                nm4_adapter_set_quiet(table_mode ? 1 : 0);
                nm4_probe_init();
                signal(SIGINT, on_linkkeeper_stop);
                signal(SIGTERM, on_linkkeeper_stop);
                printf("[netmgrd_v4] linkkeeper run start (interval=%lums, Ctrl+C to stop)\n",
                       (unsigned long)lk_interval_ms);
                printf("[netmgrd_v4] linkkeeper keepalive: wlan0=%" PRIu64 "ms ppp0=%" PRIu64 "ms\n",
                       lk_wifi_ms, lk_ppp_ms);
                (void)fflush(stdout);
                if (table_mode) {
                    printf("\033[2J\033[H");
                    (void)fflush(stdout);
                }
                while (!g_linkkeeper_stop) {
                    uint64_t now = now_ms_main();
                    nm4_linkkeeper_tick(&lk, now);
                    if (table_mode) {
                        nm4_collect_bits(&bits);
                        want = nm4_policy_pick_want(&bits, NULL);
                        have = nm4_route_get_have();
                        nm4_render_linkkeeper_table(now, tick, lk_interval_ms, cfgpath, &lk, &bits, want, have);
                    }
                    tick++;
                    usleep((useconds_t)(lk_interval_ms * 1000ULL));
                }
                nm4_adapter_set_quiet(0);
                printf("[netmgrd_v4] linkkeeper run stop\n");
                return 0;
            }
            if (do_gateway) {
                char gw[64];
                int rc = nm4_gw_resolve(gw_if, gw, sizeof(gw));
                if (rc != 0) {
                    printf("[netmgrd_v4] gateway if=%s not found\n", gw_if);
                    return 1;
                }
                printf("[netmgrd_v4] gateway if=%s -> %s\n", gw_if, gw);
                return 0;
            }
        }
    }

    netmgr_v4_log_init(debug);
    if (debug)
        netmgr_v4_log("netmgrd_v4 " NETMGR_V4_VERSION);
    (void)netmgr_v4_run();
    return 0;
}
