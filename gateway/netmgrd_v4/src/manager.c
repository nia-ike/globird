#include <signal.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include "netmgr_v4.h"
#include "netmgr_v4_linkkeeper.h"
#include "netmgr_v4_manager.h"
#include "netmgr_v4_policy.h"
#include "netmgr_v4_probe.h"
#include "netmgr_v4_route.h"
#include "netmgr_v4_status.h"
#include "netmgr_v4_dns.h"
#include "adapter_internal.h"

#define NM4_PENDING_WINDOW_MS 30000ULL
#define NM4_TICK_MS 30000
/* ppp0：ICMP 每 N 次 bits_from_probe 执行一次，其余拍沿用缓存（L0–L2 仍每拍更新） */
#define NM4_PPP_ICMP_STRIDE 4

#define NM4_EVENT_RING 5
#define NM4_UP_TH 2
#define NM4_DOWN_TH 2
#define NM4_LED_BRIGHT_PATH "/sys/class/leds/link/brightness"
#define NM4_LED_TRIGGER_PATH "/sys/class/leds/link/trigger"
#define NM4_LED_DELAY_ON_PATH "/sys/class/leds/link/delay_on"
#define NM4_LED_DELAY_OFF_PATH "/sys/class/leds/link/delay_off"

static volatile sig_atomic_t g_stop;
static int g_led_mode = -1;
static int g_led_bright = -1;

enum {
    NM4_LED_OFF = 0,
    NM4_LED_SOLID = 1,
    NM4_LED_BLINK_500 = 2,
    NM4_LED_BLINK_2000 = 3
};

/*********************************************************************************************************
** Function name:       led_write_str
** Descriptions:        Write a line of text to sysfs LED control file (e.g. trigger).
** input parameters:    path    - sysfs path.
**                      value   - string to write (newline appended).
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void led_write_str(const char *path, const char *value)
{
    FILE *fp = fopen(path, "w");
    if (!fp) return;
    (void)fprintf(fp, "%s\n", value);
    (void)fclose(fp);
}

/*********************************************************************************************************
** Function name:       led_write_int
** Descriptions:        Write decimal integer and newline to sysfs LED file (e.g. brightness).
** input parameters:    path    - sysfs path.
**                      value   - integer to write.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void led_write_int(const char *path, int value)
{
    FILE *fp = fopen(path, "w");
    if (!fp) return;
    (void)fprintf(fp, "%d\n", value);
    (void)fclose(fp);
}

/*********************************************************************************************************
** Function name:       nm4_led_set_off
** Descriptions:        Turn link LED off (trigger none, brightness 0).
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void nm4_led_set_off(void)
{
    led_write_str(NM4_LED_TRIGGER_PATH, "none");
    led_write_int(NM4_LED_BRIGHT_PATH, 0);
    g_led_bright = 0;
}

/*********************************************************************************************************
** Function name:       nm4_led_set_solid
** Descriptions:        Set link LED solid on (trigger none, brightness 1).
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void nm4_led_set_solid(void)
{
    led_write_str(NM4_LED_TRIGGER_PATH, "none");
    led_write_int(NM4_LED_BRIGHT_PATH, 1);
    g_led_bright = 1;
}

/*********************************************************************************************************
** Function name:       nm4_led_set_timer_ms
** Descriptions:        Configure link LED kernel timer trigger on/off milliseconds.
** input parameters:    on_ms   - LED on duration.
**                      off_ms  - LED off duration.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void nm4_led_set_timer_ms(int on_ms, int off_ms)
{
    led_write_str(NM4_LED_TRIGGER_PATH, "timer");
    led_write_int(NM4_LED_DELAY_ON_PATH, on_ms);
    led_write_int(NM4_LED_DELAY_OFF_PATH, off_ms);
    g_led_bright = -1;
}

/*********************************************************************************************************
** Function name:       nm4_led_update
** Descriptions:        Map active WAN to link LED pattern (blink rates / solid) with hysteresis by mode.
** input parameters:    active  - current default route want/have enum.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
/*********************************************************************************************************
** Function name:       nm4_led_update_from_bits
** Descriptions:        Map availability bits to link LED: solid if any WAN up, off if none.
**                      Uses priority order: eth (blink 500) > wlan (solid) > ppp (blink 2000).
** input parameters:    bits    - pointer to debounced availability bits.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void nm4_led_update_from_bits(const nm4_bits_t *bits)
{
    int mode = NM4_LED_OFF;

    /* Priority: eth0 > wlan0 > ppp0, show first available */
    if (bits->eth_up) mode = NM4_LED_BLINK_500;
    else if (bits->wlan_up) mode = NM4_LED_SOLID;
    else if (bits->ppp_up) mode = NM4_LED_BLINK_2000;

    if (mode == g_led_mode) return;
    g_led_mode = mode;

    if (mode == NM4_LED_OFF) {
        nm4_led_set_off();
        return;
    }
    if (mode == NM4_LED_SOLID) {
        nm4_led_set_solid();
        return;
    }
    if (mode == NM4_LED_BLINK_500) {
        nm4_led_set_timer_ms(500, 500);
        return;
    }
    nm4_led_set_timer_ms(2000, 2000);
}

typedef struct {
    nm4_probe_result_t eth;
    nm4_probe_result_t wlan;
    nm4_probe_result_t ppp;
} nm4_probe_snapshot_t;

typedef struct {
    uint64_t bits_change_cnt;
    uint64_t mismatch_repair_cnt;
    uint64_t route_apply_ok_cnt;
    uint64_t route_apply_fail_cnt;
    uint64_t pending_timeout_cnt;
} nm4_manager_stats_t;

typedef struct {
    char lines[NM4_EVENT_RING][96];
    int next;
    int count;
} nm4_event_ring_t;

/*********************************************************************************************************
** Function name:       now_ms
** Descriptions:        Current wall time in milliseconds for manager loop timestamps.
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
** Function name:       on_sigint
** Descriptions:        Signal handler to request graceful manager loop exit.
** input parameters:    signo   - signal number (ignored).
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void on_sigint(int signo)
{
    (void)signo;
    g_stop = 1;
}

/*********************************************************************************************************
** Function name:       bits_from_probe
** Descriptions:        Probe eth0/wlan0/ppp0 with policy routing; fill raw_bits and snap. ppp0 分步探测且
**                      ICMP 每 NM4_PPP_ICMP_STRIDE 次才执行一次。
** input parameters:    snap    - snapshot struct for three interfaces.
** output parameters:    raw_bits - eth_up/wlan_up/ppp_up from probe (before debounce).
**                      snap     - full probe results per interface.
** Returned value:      NONE
*********************************************************************************************************/
static void bits_from_probe(nm4_bits_t *raw_bits, nm4_probe_snapshot_t *snap)
{
    static uint32_t s_ppp_icmp_phase;
    static int s_ppp_inet_cached;
    nm4_probe_params_t pp;

    nm4_probe_params_default(&pp);
    /*
     * manager 的“可用性”需要代表公网可达，不能把“仅网关可达”当作通过。
     * 因此禁用 auto_gateway，并显式只测公网目标。
     */
    pp.auto_gateway = 0;
    pp.target_count = 2;
    (void)snprintf(pp.targets[0], sizeof(pp.targets[0]), "8.8.8.8");
    (void)snprintf(pp.targets[1], sizeof(pp.targets[1]), "1.1.1.1");
    memset(raw_bits, 0, sizeof(*raw_bits));

    /*
     * 非侵入式探测：使用 fwmark + 独立 table 的策略路由，仅让探测包走指定网卡。
     * 业务默认路由和主路由表不应被探测逻辑改动。
     */
    (void)nm4_route_probe_ensure_policy("eth0");
    (void)nm4_probe_one("eth0", &pp, &snap->eth);
    raw_bits->eth_up = snap->eth.if_exists && snap->eth.link_up && snap->eth.internet_ok;

    (void)nm4_route_probe_ensure_policy("wlan0");
    (void)nm4_probe_one("wlan0", &pp, &snap->wlan);
    raw_bits->wlan_up = snap->wlan.if_exists && snap->wlan.link_up && snap->wlan.internet_ok;

    (void)nm4_route_probe_ensure_policy("ppp0");
    nm4_probe_result_clear(&snap->ppp);
    (void)nm4_probe_if_exists("ppp0", &snap->ppp);
    if (!snap->ppp.if_exists) {
        s_ppp_inet_cached = 0;
    } else {
        (void)nm4_probe_link("ppp0", &snap->ppp);
        (void)nm4_probe_ipv4("ppp0", &snap->ppp);
        if (!snap->ppp.link_up || !snap->ppp.ipv4_ok) {
            s_ppp_inet_cached = 0;
            snap->ppp.internet_ok = 0;
        } else {
            uint32_t q = s_ppp_icmp_phase++;
            if (q % (uint32_t)NM4_PPP_ICMP_STRIDE == 0U) {
                (void)nm4_probe_icmp("ppp0", &pp, &snap->ppp);
                s_ppp_inet_cached = snap->ppp.internet_ok ? 1 : 0;
            } else {
                snap->ppp.internet_ok = s_ppp_inet_cached ? 1 : 0;
            }
        }
    }

    raw_bits->ppp_up = snap->ppp.if_exists && snap->ppp.link_up && snap->ppp.internet_ok;
}

/*********************************************************************************************************
** Function name:       event_push
** Descriptions:        Push one timestamped message into fixed-size ring buffer for table UI.
** input parameters:    r       - ring buffer.
**                      now_ms  - time for log prefix.
**                      msg     - message text.
** output parameters:    r       - updated next/count.
** Returned value:      NONE
*********************************************************************************************************/
static void event_push(nm4_event_ring_t *r, uint64_t now_ms, const char *msg)
{
    time_t t = (time_t)(now_ms / 1000ULL);
    struct tm tmv;
    char tb[16];
    localtime_r(&t, &tmv);
    (void)strftime(tb, sizeof(tb), "%H:%M:%S", &tmv);
    (void)snprintf(r->lines[r->next], sizeof(r->lines[r->next]), "%s  %s", tb, msg);
    r->next = (r->next + 1) % NM4_EVENT_RING;
    if (r->count < NM4_EVENT_RING) r->count++;
}

/*********************************************************************************************************
** Function name:       render_iface_row
** Descriptions:        Print one ASCII table row for manager TTY table view.
** input parameters:    (multiple) labels and probe/linkkeeper stats for one interface.
** output parameters:   NONE (prints to stdout).
** Returned value:      NONE
*********************************************************************************************************/
static void render_iface_row(
    const char *name,
    const char *role,
    const nm4_probe_result_t *p,
    int deb_up,
    int is_have,
    uint64_t lk_ok,
    uint64_t lk_fail)
{
    const char *exists = p->if_exists ? "yes" : "no";
    const char *link = p->link_up ? "up" : "down";
    const char *ip = p->ipv4_ok ? "yes" : "no";
    const char *inet = p->internet_ok ? "ok" : "no";
    const char *note = is_have ? "current-have" : "-";
    printf("| %-5s | %-9s | %-5d | %-3s | %-4s | %-4s | %-4s | %-6" PRIu64 " | %-8" PRIu64 " | %-12s |\n",
           name, role, deb_up, exists, link, ip, inet, lk_ok, lk_fail, note);
}

/*********************************************************************************************************
** Function name:       render_table
** Descriptions:        Redraw full manager status table on TTY (ANSI home cursor).
** input parameters:    (many) cfgpath, time, bits, want/have, stats, events, etc.
** output parameters:   NONE (prints to stdout).
** Returned value:      NONE
*********************************************************************************************************/
static void render_table(
    const char *cfgpath,
    uint64_t now_ms,
    uint64_t tick,
    const nm4_linkkeeper_t *lk,
    const nm4_bits_t *bits,
    const nm4_bits_t *prev,
    nm4_want_t want,
    nm4_want_t have,
    int pending,
    uint64_t pending_deadline,
    int changed,
    int mismatch,
    int need_apply,
    const nm4_probe_snapshot_t *snap,
    const nm4_manager_stats_t *st,
    const nm4_event_ring_t *ev)
{
    time_t t = (time_t)(now_ms / 1000ULL);
    struct tm tmv;
    char tb[32];
    int i;
    uint64_t left = 0;
    char ppp_recov_tm[16] = "--:--:--";
    localtime_r(&t, &tmv);
    (void)strftime(tb, sizeof(tb), "%Y-%m-%d %H:%M:%S", &tmv);
    if (pending && pending_deadline > now_ms) left = (pending_deadline - now_ms + 999ULL) / 1000ULL;
    if (lk->ppp_last_modem_recover_ms > 0) {
        time_t rt = (time_t)(lk->ppp_last_modem_recover_ms / 1000ULL);
        struct tm rtm;
        localtime_r(&rt, &rtm);
        (void)strftime(ppp_recov_tm, sizeof(ppp_recov_tm), "%H:%M:%S", &rtm);
    }

    printf("\033[H");
    printf("netmgrd_v4 manager monitor  mode=table  tick=%dms  cfg=%s\n", NM4_TICK_MS, cfgpath);
    printf("time=%s  tick=%" PRIu64 "  R=%u(bits=%d%d%d)  want=%s  have=%s  pending=%d(%" PRIu64 "s)\n",
        tb, tick, (unsigned)nm4_policy_to_r(bits),
        bits->eth_up, bits->wlan_up, bits->ppp_up,
        nm4_policy_want_name(want), nm4_policy_want_name(have), pending, left);
    printf("decision: ACTION=%s  reason:A(bits_changed=%d) B(want!=have=%d) apply_needed=%d\n\n",
        need_apply ? "APPLY_ROUTE" : "KEEP", changed, mismatch, need_apply);

    printf("+-------+-----------+-------+-----+------+-----+------+---------+----------+--------------+\n");
    printf("| iface | role      | ready | exi | link | ip  | inet | lk_ok   | lk_fail  | note         |\n");
    printf("+-------+-----------+-------+-----+------+-----+------+---------+----------+--------------+\n");
    render_iface_row("eth0", "primary", &snap->eth, bits->eth_up, have == NM4_WANT_ETH0, 0, 0);
    render_iface_row("wlan0", "secondary", &snap->wlan, bits->wlan_up, have == NM4_WANT_WLAN0,
                     lk->wifi_ok_cnt, lk->wifi_fail_cnt);
    render_iface_row("ppp0", "tertiary", &snap->ppp, bits->ppp_up, have == NM4_WANT_PPP0,
                     lk->ppp_ok_cnt, lk->ppp_fail_cnt);
    printf("+-------+-----------+-------+-----+------+-----+------+---------+----------+--------------+\n");

    printf("fsm: prev_R=%u(bits=%d%d%d) -> curr_R=%u(bits=%d%d%d)\n",
        (unsigned)nm4_policy_to_r(prev), prev->eth_up, prev->wlan_up, prev->ppp_up,
        (unsigned)nm4_policy_to_r(bits), bits->eth_up, bits->wlan_up, bits->ppp_up);
    printf("counters: apply_ok=%" PRIu64 " apply_fail=%" PRIu64 " bits_change=%" PRIu64
           " mismatch_repair=%" PRIu64 " pending_timeout=%" PRIu64 "\n",
        st->route_apply_ok_cnt, st->route_apply_fail_cnt, st->bits_change_cnt,
        st->mismatch_repair_cnt, st->pending_timeout_cnt);
    printf("ppp_recover: cnt=%" PRIu64 " waiting_tty=%d last=%s msg=%s\n",
        lk->ppp_modem_recover_cnt, lk->ppp_wait_tty_after_recover, ppp_recov_tm, lk->ppp_last_msg);
    printf("last_events:\n");
    for (i = 0; i < ev->count; i++) {
        int idx = (ev->next - ev->count + i + NM4_EVENT_RING) % NM4_EVENT_RING;
        printf("  %s\n", ev->lines[idx]);
    }
    printf("hint: Ctrl+C to stop\n");
    (void)fflush(stdout);
}

/*********************************************************************************************************
** Function name:       nm4_manager_run
** Descriptions:        Managed-mode main loop: linkkeeper, probe, debounce, route apply, status JSON, LED.
** input parameters:    cfgpath - netmgr JSON path.
**                      debug   - enable logging when not in table mode.
**                      view    - log or TTY table refresh mode.
** output parameters:   NONE
** Returned value:      0 when loop exits normally (SIGINT/SIGTERM or g_stop).
*********************************************************************************************************/
int nm4_manager_run(const char *cfgpath, bool debug, nm4_manager_view_t view)
{
    nm4_bits_t bits = {0}, prev = {0};
    nm4_bits_t raw_bits = {0};
    nm4_probe_snapshot_t snap;
    nm4_want_t want, have;
    nm4_linkkeeper_t lk;
    nm4_manager_stats_t stats;
    nm4_event_ring_t events;
    char err[256];
    int pending = 0;
    uint64_t pending_deadline = 0;
    uint64_t t;
    uint64_t tick = 0;
    int table_mode = (view == NM4_MANAGER_VIEW_TABLE) && isatty(STDOUT_FILENO);
    int up_streak[3] = {0, 0, 0};
    int down_streak[3] = {0, 0, 0};
    int deb_ready[3] = {0, 0, 0};

    memset(&snap, 0, sizeof(snap));
    memset(&stats, 0, sizeof(stats));
    memset(&events, 0, sizeof(events));
    g_led_mode = -1;
    g_led_bright = -1;
    netmgr_v4_log_init(table_mode ? false : debug);
    nm4_linkkeeper_set_log_enabled(table_mode ? 0 : 1);
    nm4_adapter_set_quiet(table_mode ? 1 : 0);
    nm4_probe_init();
    signal(SIGINT, on_sigint);
    signal(SIGTERM, on_sigint);

    if (nm4_linkkeeper_init(&lk, cfgpath, err, sizeof(err)) != 0) {
        netmgr_v4_log("manager: linkkeeper init warning: %s", err);
        memset(&lk, 0, sizeof(lk));
        event_push(&events, now_ms(), "linkkeeper init warning");
    }

    if (table_mode) {
        printf("\033[2J\033[H");
        (void)fflush(stdout);
    }
    netmgr_v4_log("manager: start cfg=%s", cfgpath);
    while (!g_stop) {
        int changed, mismatch, need_apply, rc;
        int i;
        int raw_arr[3];
        t = now_ms();

        nm4_linkkeeper_tick(&lk, t);
        bits_from_probe(&raw_bits, &snap);
        raw_arr[0] = raw_bits.eth_up;
        raw_arr[1] = raw_bits.wlan_up;
        raw_arr[2] = raw_bits.ppp_up;
        for (i = 0; i < 3; i++) {
            if (raw_arr[i]) {
                up_streak[i]++;
                down_streak[i] = 0;
                if (up_streak[i] >= NM4_UP_TH) deb_ready[i] = 1;
            } else {
                down_streak[i]++;
                up_streak[i] = 0;
                if (down_streak[i] >= NM4_DOWN_TH) deb_ready[i] = 0;
            }
        }
        bits.eth_up = deb_ready[0];
        bits.wlan_up = deb_ready[1];
        bits.ppp_up = deb_ready[2];
        want = nm4_policy_pick_want(&bits);
        have = nm4_route_get_have();
        changed = (memcmp(&bits, &prev, sizeof(bits)) != 0);
        mismatch = (want != have);
        if (changed) stats.bits_change_cnt++;

        if (pending && t >= pending_deadline) {
            pending = 0;
            netmgr_v4_log("manager: pending timeout reached");
            stats.pending_timeout_cnt++;
            event_push(&events, t, "pending timeout");
        }

        need_apply = changed || (!pending && mismatch);
        if (!changed && !pending && mismatch) stats.mismatch_repair_cnt++;
        netmgr_v4_log("manager: R=%u bits(%d%d%d) want=%s have=%s pending=%d trig=%d",
            (unsigned)nm4_policy_to_r(&bits),
            bits.eth_up, bits.wlan_up, bits.ppp_up,
            nm4_policy_want_name(want), nm4_policy_want_name(have),
            pending, need_apply);

        if (need_apply) {
            rc = nm4_route_apply(want);
            pending = 1;
            pending_deadline = t + NM4_PENDING_WINDOW_MS;
            netmgr_v4_log("manager: route apply %s -> rc=%d",
                          nm4_policy_want_name(want), rc);
            if (rc == 0) {
                stats.route_apply_ok_cnt++;
                event_push(&events, t, "route apply ok");
            } else {
                stats.route_apply_fail_cnt++;
                event_push(&events, t, "route apply fail");
            }
            if (rc == 0) {
                have = want;
                pending = 0;
            }
        }

        nm4_led_update_from_bits(&bits);

        /* Periodic DNS fallback ensure: prevents DHCP/PPP from leaving stale DNS.
         * This is best-effort; failure is not fatal to network management.
         */
        (void)nm4_dns_ensure_fallback();

        if (table_mode) {
            render_table(cfgpath, t, tick, &lk, &bits, &prev, want, have,
                         pending, pending_deadline, changed, mismatch, need_apply,
                         &snap, &stats, &events);
        }
        {
            nm4_status_snapshot_t ss;
            ss.now_ms = t;
            ss.tick = tick;
            ss.r = nm4_policy_to_r(&bits);
            ss.want = want;
            ss.have = have;
            ss.pending = pending;
            ss.bits = &bits;
            ss.eth = &snap.eth;
            ss.wlan = &snap.wlan;
            ss.ppp = &snap.ppp;
            ss.lk = &lk;
            (void)nm4_status_write(&ss);
        }
        prev = bits;
        tick++;
        usleep((useconds_t)NM4_TICK_MS * 1000U);
    }

    if (table_mode) printf("\n");
    nm4_led_set_off();
    nm4_adapter_set_quiet(0);
    netmgr_v4_log("manager: stop");
    return 0;
}
