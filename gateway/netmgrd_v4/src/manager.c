#include <signal.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include "netmgr_v4.h"
#include "netmgr_v4_linkkeeper.h"
#include "netmgr_v4_manager.h"
#include "netmgr_v4_policy.h"
#include "netmgr_v4_probe.h"
#include "netmgr_v4_route.h"
#include "netmgr_v4_status.h"
#include "netmgr_v4_dns.h"
#include "netmgr_v4_config.h"
#include "netmgr_v4_iface_avail.h"
#include "netmgr_v4_gateway.h"
#include "netmgr_v4_adapter.h"
#include "netmgr_v4_led.h"
#include "netmgr_v4_hw_sim.h"
#include "adapter_internal.h"
#include "db_msgevent_api.h"

#define NM4_PENDING_WINDOW_MS 30000ULL
#define NM4_TICK_MS 30000
#define NM4_WAIT_SLICE_MS 1000
/* ppp0 ICMP：主出口为 eth/wlan 时降频；仅 eth/wlan 均不可用或主出口为 ppp 时按主循环探测 */
#define NM4_PROBE_PPP_ICMP_SLOW_MS 120000ULL
/* 凌晨 03:xx 且 want=none：日常恢复已跑完仍无可用出口则整机重启（每日一次，防重启环） */
#define NM4_NIGHT_REBOOT_HOUR        3
#define NM4_NIGHT_REBOOT_MIN_UPTIME  1800U

#define NM4_EVENT_RING 5

static volatile sig_atomic_t g_stop;
static volatile sig_atomic_t g_arbitrate_now;

static int s_watch_had_eth_ip;
static int s_watch_had_wifi_ip;
static int s_watch_had_ppp_ip;
static int s_watch_had_eth_link;
static int s_watch_had_wifi_link;
static int s_watch_had_ppp_link;

static uint64_t now_ms(void);

static unsigned boot_uptime_sec(void)
{
    struct timespec ts;

#if defined(CLOCK_BOOTTIME)
    if (clock_gettime(CLOCK_BOOTTIME, &ts) == 0)
        return (unsigned)ts.tv_sec;
#endif
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        return (unsigned)ts.tv_sec;
    return 0;
}

/*********************************************************************************************************
** Function name:       night_reboot_maybe
** Descriptions:        Once per day around 03:xx, if policy has no WAN, ask db_process save+reboot.
** input parameters:    want    - current policy pick.
**                      now_ms  - wall clock ms.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void night_reboot_maybe(nm4_want_t want, uint64_t now_ms)
{
    time_t sec;
    struct tm tm;
    static int done_ymd = -1;
    int ymd;

    if (want != NM4_WANT_NONE)
        return;
    if (boot_uptime_sec() < NM4_NIGHT_REBOOT_MIN_UPTIME)
        return;

    sec = (time_t)(now_ms / 1000ULL);
    if (localtime_r(&sec, &tm) == NULL)
        return;
    if (tm.tm_hour != NM4_NIGHT_REBOOT_HOUR)
        return;

    ymd = (tm.tm_year + 1900) * 10000 + (tm.tm_mon + 1) * 100 + tm.tm_mday;
    if (done_ymd == ymd)
        return;

    if (db_msg_event_api_init() != 0) {
        netmgr_v4_log("manager: night reboot deferred (db msg queue unavailable)");
        return;
    }
    if (db_msg_event_cmd(DB_EVENT_TYPE_REBOOT) != 0) {
        netmgr_v4_log("manager: night reboot deferred (db reboot msg send failed)");
        return;
    }

    done_ymd = ymd;
    netmgr_v4_log("manager: 03:xx all WAN down, db_process save+reboot posted");
}

void nm4_manager_request_arbitrate(void)
{
    g_arbitrate_now = 1;
}

/*********************************************************************************************************
** Function name:       nm4_manager_poll_arbitrate_flag
** Descriptions:        Check script touch file /var/run/netmgr/arbitrate and request fast loop.
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void nm4_manager_poll_arbitrate_flag(void)
{
    if (access(NM4_ARBITRATE_FLAG, F_OK) == 0) {
        (void)unlink(NM4_ARBITRATE_FLAG);
        g_arbitrate_now = 1;
    }
}

static int nm4_watch_l3_reconnect(void)
{
    int hit = 0;
    int e = nm4_if_has_ipv4("eth0");
    int w = nm4_if_has_ipv4("wlan0");
    int p = nm4_if_has_ipv4("ppp0");

    if ((!s_watch_had_eth_ip && e) || (!s_watch_had_wifi_ip && w) || (!s_watch_had_ppp_ip && p))
        hit = 1;
    s_watch_had_eth_ip = e;
    s_watch_had_wifi_ip = w;
    s_watch_had_ppp_ip = p;
    return hit;
}

/*********************************************************************************************************
** Function name:       nm4_watch_link_change
** Descriptions:        Poll carrier/RUNNING each wait slice; trigger fast arbitrate on L1 edge.
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      1 if any interface link state changed since last poll.
*********************************************************************************************************/
static int nm4_watch_link_change(void)
{
    int hit = 0;
    int e = nm4_if_link_up("eth0");
    int w = nm4_if_link_up("wlan0");
    int p = nm4_if_link_up("ppp0");

    if (e != s_watch_had_eth_link)
        hit = 1;
    if (w != s_watch_had_wifi_link) {
        hit = 1;
        if (!s_watch_had_wifi_link && w)
            (void)nm4_adapter_wpa_renew("wlan0");
    }
    if (p != s_watch_had_ppp_link)
        hit = 1;

    s_watch_had_eth_link = e;
    s_watch_had_wifi_link = w;
    s_watch_had_ppp_link = p;
    return hit;
}

static int nm4_watch_fast_events(bool debug, bool table_mode, nm4_linkkeeper_t *lk)
{
    if (nm4_sim_poll(now_ms()) == NM4_SIM_EDGE_CHANGE) {
        (void)nm4_sim_on_edge(lk, now_ms());
        if (debug && !table_mode)
            netmgr_v4_log("manager: SIM edge, fast arbitrate");
        return 1;
    }
    if (nm4_watch_l3_reconnect()) {
        if (debug && !table_mode)
            netmgr_v4_log("manager: L3 IP gained, fast arbitrate");
        return 1;
    }
    if (nm4_watch_link_change()) {
        if (debug && !table_mode)
            netmgr_v4_log("manager: L1 link change, fast arbitrate");
        return 1;
    }
    return 0;
}

static uint64_t next_probe_in_ms(uint64_t now_ms, uint64_t last_probe_ms)
{
    if (last_probe_ms == 0 || now_ms <= last_probe_ms)
        return (uint64_t)NM4_TICK_MS;
    {
        uint64_t elapsed = now_ms - last_probe_ms;
        if (elapsed >= (uint64_t)NM4_TICK_MS)
            return 0;
        return (uint64_t)NM4_TICK_MS - elapsed;
    }
}

static uint64_t lk_next_keep_ms(const nm4_linkkeeper_t *lk, int is_wifi, uint64_t now_ms)
{
    uint64_t wifi_ms = 0;
    uint64_t ppp_ms = 0;
    uint64_t last;
    uint64_t interval;

    if (!lk)
        return 0;
    nm4_linkkeeper_get_intervals(&wifi_ms, &ppp_ms);
    if (is_wifi) {
        if (!lk->has_wifi)
            return 0;
        last = lk->last_wifi_keep_ms;
        interval = wifi_ms;
    } else {
        if (!lk->has_ppp)
            return 0;
        last = lk->last_ppp_keep_ms;
        interval = ppp_ms;
    }
    if (last == 0)
        return 0;
    if (now_ms <= last || now_ms - last >= interval)
        return 0;
    return interval - (now_ms - last);
}

static uint64_t debounce_flip_eta_ms(
    const nm4_iface_avail_t *a,
    uint64_t now_ms,
    uint64_t last_probe_ms)
{
    int ticks_needed;
    uint64_t wait_next;

    if (!a)
        return 0;
    if (a->available && a->raw)
        return 0;
    if (!a->available && !a->raw)
        return 0;

    if (a->available && !a->raw)
        ticks_needed = a->cfg.down_th - a->down_streak;
    else
        ticks_needed = a->cfg.up_th - a->up_streak;

    if (ticks_needed <= 0)
        return 0;

    wait_next = next_probe_in_ms(now_ms, last_probe_ms);
    return wait_next + (uint64_t)(ticks_needed - 1) * (uint64_t)NM4_TICK_MS;
}

static void fmt_debounce(char *buf, size_t len, const nm4_iface_avail_t *a)
{
    if (!buf || len == 0)
        return;
    if (!a) {
        (void)snprintf(buf, len, "-");
        return;
    }
    if (a->available && a->raw)
        (void)snprintf(buf, len, "hold");
    else if (a->available && !a->raw)
        (void)snprintf(buf, len, "dn %d/%d", a->down_streak, a->cfg.down_th);
    else if (!a->available && a->raw)
        (void)snprintf(buf, len, "up %d/%d", a->up_streak, a->cfg.up_th);
    else
        (void)snprintf(buf, len, "idle");
}

static void fmt_eta(char *buf, size_t len, uint64_t eta_ms)
{
    if (!buf || len == 0)
        return;
    if (eta_ms == 0)
        (void)snprintf(buf, len, "-");
    else
        (void)snprintf(buf, len, "~%lus", (unsigned long)((eta_ms + 999ULL) / 1000ULL));
}

static void fmt_switch_eta(
    char *buf,
    size_t len,
    nm4_want_t have,
    nm4_want_t want,
    const nm4_bits_t *bits,
    const nm4_iface_avail_t *eth,
    const nm4_iface_avail_t *wlan,
    const nm4_iface_avail_t *ppp,
    uint64_t now_ms,
    uint64_t last_probe_ms)
{
    nm4_want_t next;
    uint64_t eta;

    if (!buf || len == 0)
        return;
    if (want != have) {
        (void)snprintf(buf, len, "now -> %s", nm4_policy_want_name(want));
        return;
    }
    if (have == NM4_WANT_WLAN0 && wlan && wlan->available && !wlan->raw && bits && bits->ppp_up) {
        eta = debounce_flip_eta_ms(wlan, now_ms, last_probe_ms);
        if (eta > 0) {
            (void)snprintf(buf, len, "~%lus -> ppp0", (unsigned long)((eta + 999ULL) / 1000ULL));
            return;
        }
    }
    if (have == NM4_WANT_ETH0 && eth && eth->available && !eth->raw) {
        next = bits && bits->wlan_up ? NM4_WANT_WLAN0 :
               (bits && bits->ppp_up ? NM4_WANT_PPP0 : NM4_WANT_NONE);
        if (next != NM4_WANT_NONE) {
            eta = debounce_flip_eta_ms(eth, now_ms, last_probe_ms);
            if (eta > 0) {
                (void)snprintf(buf, len, "~%lus -> %s",
                               (unsigned long)((eta + 999ULL) / 1000ULL),
                               nm4_policy_want_name(next));
                return;
            }
        }
    }
    if (have == NM4_WANT_PPP0 && ppp && ppp->available && !ppp->raw) {
        next = bits && bits->eth_up ? NM4_WANT_ETH0 :
               (bits && bits->wlan_up ? NM4_WANT_WLAN0 : NM4_WANT_NONE);
        if (next != NM4_WANT_NONE) {
            eta = debounce_flip_eta_ms(ppp, now_ms, last_probe_ms);
            if (eta > 0) {
                (void)snprintf(buf, len, "~%lus -> %s",
                               (unsigned long)((eta + 999ULL) / 1000ULL),
                               nm4_policy_want_name(next));
                return;
            }
        }
    }
    (void)snprintf(buf, len, "-");
}

/*********************************************************************************************************
** Function name:       nm4_probe_snapshot_t
** Descriptions:        Per-tick probe results for eth0/wlan0/ppp0 (typedef below).
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
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

typedef struct {
    int active;
    const char *cfgpath;
    uint64_t tick;
    nm4_linkkeeper_t *lk;
    nm4_bits_t bits;
    nm4_bits_t prev;
    nm4_want_t want;
    nm4_want_t have;
    int pending;
    uint64_t pending_deadline;
    int changed;
    int mismatch;
    int need_apply;
    nm4_probe_snapshot_t snap;
    nm4_manager_stats_t stats;
    nm4_event_ring_t events;
    nm4_iface_avail_t eth_a;
    nm4_iface_avail_t wlan_a;
    nm4_iface_avail_t ppp_a;
    uint64_t last_probe_ms;
} nm4_table_live_t;

static nm4_table_live_t g_tbl;

typedef struct {
    int active;
    const char *cfgpath;
    nm4_probe_snapshot_t *snap;
    nm4_iface_avail_t *eth_a;
    nm4_iface_avail_t *wlan_a;
    nm4_iface_avail_t *ppp_a;
    nm4_bits_t *bits;
    nm4_bits_t *prev;
    nm4_want_t *want;
    nm4_want_t *have;
    int *pending;
    uint64_t *pending_deadline;
    nm4_manager_stats_t *stats;
    nm4_event_ring_t *events;
    int *changed;
    int *mismatch;
    int *need_apply;
    nm4_linkkeeper_t *lk;
} nm4_manager_live_t;

static nm4_manager_live_t g_live;

static void event_push(nm4_event_ring_t *r, uint64_t now_ms, const char *msg);
static void render_table(const nm4_table_live_t *ctx, uint64_t now_ms);
static void table_sync_from_live(void);

static void refresh_probe_l12(const char *ifname, nm4_probe_result_t *out)
{
    int inet_keep;

    if (!ifname || !ifname[0] || !out)
        return;
    inet_keep = out->internet_ok;
    (void)nm4_probe_link(ifname, out);
    (void)nm4_probe_ipv4(ifname, out);
    if (!out->link_up || !out->ipv4_ok)
        out->internet_ok = 0;
    else
        out->internet_ok = inet_keep;
}

static void manager_led_refresh(void)
{
    nm4_led_input_t li;

    if (!g_live.active || !g_live.snap || !g_live.bits)
        return;
    li.wlan = &g_live.snap->wlan;
    li.ppp = &g_live.snap->ppp;
    li.bits = g_live.bits;
    nm4_led_update(&li);
}

static void manager_fast_link_tick(uint64_t t, bool debug, bool table_mode)
{
    nm4_bits_t old_bits;
    int rc;

    if (!g_live.active || !g_live.snap || !g_live.bits || !g_live.prev ||
        !g_live.want || !g_live.have || !g_live.eth_a || !g_live.wlan_a ||
        !g_live.ppp_a)
        return;

    refresh_probe_l12("eth0", &g_live.snap->eth);
    refresh_probe_l12("wlan0", &g_live.snap->wlan);
    if (g_live.lk && nm4_sim_ppp_effective(g_live.lk))
        refresh_probe_l12("ppp0", &g_live.snap->ppp);
    else
        nm4_probe_result_clear(&g_live.snap->ppp);

    nm4_iface_avail_update(g_live.eth_a, &g_live.snap->eth);
    nm4_iface_avail_update(g_live.wlan_a, &g_live.snap->wlan);
    nm4_iface_avail_update(g_live.ppp_a, &g_live.snap->ppp);
    if (!g_live.lk || !nm4_sim_ppp_effective(g_live.lk)) {
        g_live.ppp_a->available = 0;
        g_live.ppp_a->raw = 0;
        g_live.ppp_a->down_streak = g_live.ppp_a->cfg.down_th;
        g_live.ppp_a->up_streak = 0;
    }
    old_bits = *g_live.bits;
    nm4_iface_avail_to_bits(g_live.eth_a, g_live.wlan_a, g_live.ppp_a, g_live.bits);
    *g_live.want = nm4_policy_pick_want(g_live.bits, &g_live.snap->eth);

    if (g_live.changed)
        *g_live.changed = (memcmp(g_live.bits, &old_bits, sizeof(*g_live.bits)) != 0) ? 1 : 0;
    if (g_live.mismatch)
        *g_live.mismatch = (*g_live.want != *g_live.have) ? 1 : 0;
    if (g_live.need_apply)
        *g_live.need_apply = (*g_live.changed ||
                              (!*g_live.pending && *g_live.mismatch)) ? 1 : 0;

    if (g_live.stats && *g_live.changed)
        g_live.stats->bits_change_cnt++;

    if (g_live.need_apply && *g_live.need_apply) {
        if (!*g_live.pending && g_live.mismatch && *g_live.mismatch && g_live.stats)
            g_live.stats->mismatch_repair_cnt++;
        rc = nm4_route_apply(*g_live.want);
        if (g_live.pending)
            *g_live.pending = 1;
        if (g_live.pending_deadline)
            *g_live.pending_deadline = t + NM4_PENDING_WINDOW_MS;
        if (debug && !table_mode)
            netmgr_v4_log("manager: fast L1 tick route apply %s -> rc=%d",
                          nm4_policy_want_name(*g_live.want), rc);
        if (g_live.stats) {
            if (rc == 0)
                g_live.stats->route_apply_ok_cnt++;
            else
                g_live.stats->route_apply_fail_cnt++;
        }
        if (g_live.events) {
            if (rc == 0)
                event_push(g_live.events, t, "fast L1 route ok");
            else
                event_push(g_live.events, t, "fast L1 route fail");
        }
        if (rc == 0) {
            *g_live.have = *g_live.want;
            if (g_live.pending)
                *g_live.pending = 0;
        }
    }

    manager_led_refresh();
    table_sync_from_live();
    if (g_tbl.active)
        render_table(&g_tbl, t);
}

static void table_sync_from_live(void)
{
    if (!g_tbl.active || !g_live.active)
        return;
    if (g_live.bits)
        g_tbl.bits = *g_live.bits;
    if (g_live.want)
        g_tbl.want = *g_live.want;
    if (g_live.have)
        g_tbl.have = *g_live.have;
    if (g_live.pending)
        g_tbl.pending = *g_live.pending;
    if (g_live.pending_deadline)
        g_tbl.pending_deadline = *g_live.pending_deadline;
    if (g_live.changed)
        g_tbl.changed = *g_live.changed;
    if (g_live.mismatch)
        g_tbl.mismatch = *g_live.mismatch;
    if (g_live.need_apply)
        g_tbl.need_apply = *g_live.need_apply;
    if (g_live.snap)
        g_tbl.snap = *g_live.snap;
}

static void table_live_refresh(uint64_t now_ms)
{
    if (!g_tbl.active)
        return;
    table_sync_from_live();
    render_table(&g_tbl, now_ms);
}

static void nm4_manager_wait_steady(bool debug, int table_refresh, nm4_linkkeeper_t *lk)
{
    int elapsed = 0;

    while (elapsed < NM4_TICK_MS) {
        if (g_arbitrate_now)
            break;
        if (nm4_watch_fast_events(debug, table_refresh, lk)) {
            manager_fast_link_tick(now_ms(), debug, table_refresh);
            g_arbitrate_now = 1;
            break;
        }
        manager_led_refresh();
        if (table_refresh)
            table_live_refresh(now_ms());
        usleep((useconds_t)NM4_WAIT_SLICE_MS * 1000U);
        nm4_manager_poll_arbitrate_flag();
        elapsed += NM4_WAIT_SLICE_MS;
    }
    g_arbitrate_now = 0;
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
** Function name:       ppp_icmp_interval_ms
** Descriptions:        Choose ppp0 ICMP interval: fast when ppp is WAN or no free path, else slow.
** input parameters:    have        - current default route interface.
**                      prev_bits   - debounced availability from prior tick.
** output parameters:   NONE
** Returned value:      Interval in milliseconds before next ppp ICMP probe.
*********************************************************************************************************/
static uint64_t ppp_icmp_interval_ms(nm4_want_t have, const nm4_bits_t *prev_bits)
{
    if (have == NM4_WANT_PPP0)
        return (uint64_t)NM4_TICK_MS;
    if (prev_bits && (prev_bits->eth_up || prev_bits->wlan_up))
        return NM4_PROBE_PPP_ICMP_SLOW_MS;
    return (uint64_t)NM4_TICK_MS;
}

/*********************************************************************************************************
** Function name:       probe_ppp_iface
** Descriptions:        Probe ppp0 L0–L2 every tick; ICMP throttled / skipped when eth/wlan carry WAN.
** input parameters:    pp          - probe parameters.
**                      now_ms      - wall time for ICMP throttle.
**                      have        - current default route (from route table).
**                      prev_bits   - prior debounced bits.
** output parameters:    out         - probe result; internet_ok may be cached when ICMP skipped.
** Returned value:      NONE
*********************************************************************************************************/
static void probe_ppp_iface(
    const nm4_probe_params_t *pp,
    nm4_probe_result_t *out,
    uint64_t now_ms,
    nm4_want_t have,
    const nm4_bits_t *prev_bits)
{
    static int s_inet_cached;
    static uint64_t s_last_icmp_ms;
    uint64_t interval;
    int run_icmp;

    if (!pp || !out)
        return;

    nm4_probe_result_clear(out);
    (void)nm4_route_probe_ensure_policy("ppp0");
    if (nm4_probe_if_exists("ppp0", out) != 0)
        return;
    if (!out->if_exists) {
        s_inet_cached = 0;
        return;
    }
    (void)nm4_probe_link("ppp0", out);
    (void)nm4_probe_ipv4("ppp0", out);
    if (!out->link_up || !out->ipv4_ok) {
        s_inet_cached = 0;
        out->internet_ok = 0;
        return;
    }

    interval = ppp_icmp_interval_ms(have, prev_bits);
    run_icmp = (s_last_icmp_ms == 0) || (now_ms - s_last_icmp_ms >= interval);
    if (!run_icmp) {
        out->internet_ok = s_inet_cached ? 1 : 0;
        return;
    }

    (void)nm4_probe_icmp("ppp0", pp, out);
    s_inet_cached = out->internet_ok ? 1 : 0;
    s_last_icmp_ms = now_ms;
}

/*********************************************************************************************************
** Function name:       probe_all_ifaces
** Descriptions:        Probe eth0/wlan0 full each tick; ppp0 L3 ICMP throttled per WAN state.
** input parameters:    snap        - snapshot struct for three interfaces.
**                      now_ms      - current time in milliseconds.
**                      have        - current default route interface.
**                      prev_bits   - debounced bits from previous manager tick.
** output parameters:    snap        - full probe results per interface.
** Returned value:      NONE
*********************************************************************************************************/
static void probe_all_ifaces(
    nm4_probe_snapshot_t *snap,
    uint64_t now_ms,
    nm4_want_t have,
    const nm4_bits_t *prev_bits,
    int ppp_effective)
{
    nm4_probe_params_t pp;

    nm4_probe_params_default(&pp);
    nm4_config_fill_probe_params(&pp);
    pp.auto_gateway = 0;

    (void)nm4_route_probe_ensure_policy("eth0");
    (void)nm4_probe_one("eth0", &pp, &snap->eth);

    (void)nm4_route_probe_ensure_policy("wlan0");
    (void)nm4_probe_one("wlan0", &pp, &snap->wlan);

    if (ppp_effective)
        probe_ppp_iface(&pp, &snap->ppp, now_ms, have, prev_bits);
    else
        nm4_probe_result_clear(&snap->ppp);
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

static void table_strip_newline(char *s)
{
    size_t len;

    if (!s)
        return;
    len = strlen(s);
    if (len > 0 && s[len - 1] == '\n')
        s[len - 1] = '\0';
}

static int table_if_ipv4(const char *ifname, char *out, size_t outlen)
{
    struct ifaddrs *ifa0, *ifa;

    if (!ifname || !ifname[0] || !out || outlen < 8)
        return 0;
    out[0] = '\0';
    if (getifaddrs(&ifa0) != 0)
        return 0;
    for (ifa = ifa0; ifa; ifa = ifa->ifa_next) {
        struct sockaddr_in *sin;
        if (!ifa->ifa_name || !ifa->ifa_addr)
            continue;
        if (strcmp(ifa->ifa_name, ifname) != 0)
            continue;
        if (ifa->ifa_addr->sa_family != AF_INET)
            continue;
        sin = (struct sockaddr_in *)ifa->ifa_addr;
        if (!inet_ntop(AF_INET, &sin->sin_addr, out, (socklen_t)outlen))
            continue;
        freeifaddrs(ifa0);
        return 1;
    }
    freeifaddrs(ifa0);
    return 0;
}

static int table_read_cached_gw(const char *ifname, char *out, size_t outlen)
{
    char path[256];
    FILE *fp;

    if (!ifname || !ifname[0] || !out || outlen < 2)
        return 0;
    out[0] = '\0';
    (void)snprintf(path, sizeof(path), "%s/gateway.%s", NM4_NETMGR_RUN_DIR, ifname);
    fp = fopen(path, "r");
    if (!fp)
        return 0;
    if (!fgets(out, (int)outlen, fp)) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    table_strip_newline(out);
    return out[0] ? 1 : 0;
}

static int table_read_dns_cache_fmt(const char *ifname, char *out, size_t outlen)
{
    char path[256];
    FILE *fp;
    char line[64];
    size_t pos = 0;
    int n = 0;

    if (!ifname || !ifname[0] || !out || outlen < 4)
        return 0;
    out[0] = '\0';
    (void)snprintf(path, sizeof(path), "%s/dns.%s", NM4_NETMGR_RUN_DIR, ifname);
    fp = fopen(path, "r");
    if (!fp)
        return 0;
    while (fgets(line, sizeof(line), fp)) {
        table_strip_newline(line);
        if (!line[0])
            continue;
        if (n > 0) {
            if (pos + 1 >= outlen)
                break;
            out[pos++] = ',';
            out[pos] = '\0';
        }
        if (pos + strlen(line) + 1 >= outlen)
            break;
        (void)snprintf(out + pos, outlen - pos, "%s", line);
        pos = strlen(out);
        n++;
    }
    fclose(fp);
    return n > 0 ? 1 : 0;
}

static void table_print_cmd_lines(const char *cmd, int max_lines, const char *prefix)
{
    FILE *fp;
    char line[160];
    int n = 0;

    if (!cmd || max_lines <= 0)
        return;
    fp = popen(cmd, "r");
    if (!fp)
        return;
    while (n < max_lines && fgets(line, sizeof(line), fp)) {
        table_strip_newline(line);
        if (!line[0])
            continue;
        printf("%s%s\n", prefix ? prefix : "", line);
        n++;
    }
    (void)pclose(fp);
    if (n == 0)
        printf("%s(none)\n", prefix ? prefix : "");
}

static void render_route_dns_debug(nm4_want_t want, nm4_want_t have)
{
    static const char *ifnames[] = { "eth0", "wlan0", "ppp0" };
    char ip[20];
    char gw_cache[20];
    char gw_res[20];
    char dns[88];
    int i;

    printf("\n--- default route (kernel, all) ---\n");
    table_print_cmd_lines("ip route show default 2>/dev/null", 5, "  ");

    printf("--- netmgr managed default (metric 10) ---\n");
    table_print_cmd_lines("ip route show default metric 10 2>/dev/null", 3, "  ");
    printf("  policy: want=%s have=%s (metric 10 owned by netmgrd)\n",
           nm4_policy_want_name(want), nm4_policy_want_name(have));

    printf("--- egress cache (%s) ---\n", NM4_NETMGR_RUN_DIR);
    printf("| iface | ipv4          | gw_cache      | gw_resolve    | dns_cache                      |\n");
    printf("|-------|---------------|---------------|---------------|--------------------------------|\n");
    for (i = 0; i < 3; i++) {
        const char *ifn = ifnames[i];

        if (!table_if_ipv4(ifn, ip, sizeof(ip)))
            (void)snprintf(ip, sizeof(ip), "-");
        if (!table_read_cached_gw(ifn, gw_cache, sizeof(gw_cache)))
            (void)snprintf(gw_cache, sizeof(gw_cache), "-");
        if (nm4_gw_resolve(ifn, gw_res, sizeof(gw_res)) != 0)
            (void)snprintf(gw_res, sizeof(gw_res), "-");
        if (!table_read_dns_cache_fmt(ifn, dns, sizeof(dns)))
            (void)snprintf(dns, sizeof(dns), "-");
        printf("| %-5s | %-13s | %-13s | %-13s | %-30s |\n",
               ifn, ip, gw_cache, gw_res, dns);
    }

    printf("--- resolv.conf (active) ---\n");
    table_print_cmd_lines("grep -E '^nameserver' /etc/resolv.conf 2>/dev/null | head -8", 8, "  ");
    printf("  active_wan_dns: dns.%s + fallback %s\n",
           have == NM4_WANT_ETH0 ? "eth0" :
           (have == NM4_WANT_WLAN0 ? "wlan0" :
            (have == NM4_WANT_PPP0 ? "ppp0" : "(none)")),
           NM4_DNS_GLOBAL_FALLBACK);
    printf("\n");
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
    const nm4_iface_avail_t *avail,
    int deb_up,
    int is_have,
    uint64_t now_ms,
    uint64_t last_probe_ms,
    int live_l12)
{
    int link_up = live_l12 ? nm4_if_link_up(name) : p->link_up;
    int ipv4_ok = live_l12 ? (link_up && nm4_if_has_ipv4(name)) : p->ipv4_ok;
    const char *link = link_up ? "up" : "down";
    const char *ip = ipv4_ok ? "yes" : "no";
    const char *inet = p->internet_ok ? "ok" : "no";
    const char *note = is_have ? "current-have" : "-";
    char deb[12];
    char eta[8];

    fmt_debounce(deb, sizeof(deb), avail);
    fmt_eta(eta, sizeof(eta), debounce_flip_eta_ms(avail, now_ms, last_probe_ms));
    printf("| %-5s | %-9s | %-5d | %-4s | %-4s | %-4s | %-7s | %-6s | %-12s |\n",
           name, role, deb_up, link, ip, inet, deb, eta, note);
}

/*********************************************************************************************************
** Function name:       render_table
** Descriptions:        Redraw full manager status table on TTY (ANSI home cursor).
** input parameters:    (many) cfgpath, time, bits, want/have, stats, events, etc.
** output parameters:   NONE (prints to stdout).
** Returned value:      NONE
*********************************************************************************************************/
static void render_table(const nm4_table_live_t *ctx, uint64_t now_ms)
{
    const char *cfgpath = ctx->cfgpath;
    uint64_t tick = ctx->tick;
    const nm4_linkkeeper_t *lk = ctx->lk;
    const nm4_bits_t *bits = &ctx->bits;
    const nm4_bits_t *prev = &ctx->prev;
    nm4_want_t want = ctx->want;
    nm4_want_t have = ctx->have;
    int pending = ctx->pending;
    uint64_t pending_deadline = ctx->pending_deadline;
    int changed = ctx->changed;
    int mismatch = ctx->mismatch;
    int need_apply = ctx->need_apply;
    const nm4_probe_snapshot_t *snap = &ctx->snap;
    const nm4_manager_stats_t *st = &ctx->stats;
    const nm4_event_ring_t *ev = &ctx->events;
    uint64_t last_probe_ms = ctx->last_probe_ms;
    time_t t = (time_t)(now_ms / 1000ULL);
    struct tm tmv;
    char tb[32];
    int i;
    uint64_t left = 0;
    char ppp_recov_tm[16] = "--:--:--";
    char switch_eta[32];
    char probe_eta[16];
    char wifi_k[12];
    char ppp_k[12];
    uint64_t probe_in = next_probe_in_ms(now_ms, last_probe_ms);
    uint64_t wifi_k_ms = lk_next_keep_ms(lk, 1, now_ms);
    uint64_t ppp_k_ms = lk_next_keep_ms(lk, 0, now_ms);

    localtime_r(&t, &tmv);
    (void)strftime(tb, sizeof(tb), "%Y-%m-%d %H:%M:%S", &tmv);
    if (pending && pending_deadline > now_ms)
        left = (pending_deadline - now_ms + 999ULL) / 1000ULL;
    if (lk->ppp_last_modem_recover_ms > 0) {
        time_t rt = (time_t)(lk->ppp_last_modem_recover_ms / 1000ULL);
        struct tm rtm;
        localtime_r(&rt, &rtm);
        (void)strftime(ppp_recov_tm, sizeof(ppp_recov_tm), "%H:%M:%S", &rtm);
    }
    fmt_eta(probe_eta, sizeof(probe_eta), probe_in);
    fmt_switch_eta(switch_eta, sizeof(switch_eta), have, want, bits,
                   &ctx->eth_a, &ctx->wlan_a, &ctx->ppp_a, now_ms, last_probe_ms);
    if (wifi_k_ms > 0)
        (void)snprintf(wifi_k, sizeof(wifi_k), "%lus", (unsigned long)((wifi_k_ms + 999ULL) / 1000ULL));
    else
        (void)snprintf(wifi_k, sizeof(wifi_k), "due");
    if (ppp_k_ms > 0)
        (void)snprintf(ppp_k, sizeof(ppp_k), "%lus", (unsigned long)((ppp_k_ms + 999ULL) / 1000ULL));
    else if (!nm4_sim_ppp_effective(lk))
        (void)snprintf(ppp_k, sizeof(ppp_k), "skip");
    else
        (void)snprintf(ppp_k, sizeof(ppp_k), "due");

    printf("\033[H");
    printf("netmgrd_v4 manager monitor  mode=table  tick=%dms  cfg=%s\n", NM4_TICK_MS, cfgpath);
    printf("time=%s  tick=%" PRIu64 "  R=%u(bits=%d%d%d)  want=%s  have=%s  pending=%d(%" PRIu64 "s)\n",
        tb, tick, (unsigned)nm4_policy_to_r(bits),
        bits->eth_up, bits->wlan_up, bits->ppp_up,
        nm4_policy_want_name(want), nm4_policy_want_name(have), pending, left);
    printf("sim: present=%d  config_ppp=%d  ppp_effective=%d\n",
           nm4_sim_last_present(), lk->has_ppp, nm4_sim_ppp_effective(lk));
    printf("timing: next_probe=%s  route_switch=%s\n", probe_eta, switch_eta);
    printf("decision: ACTION=%s  reason:A(bits_changed=%d) B(want!=have=%d) apply_needed=%d\n\n",
        need_apply ? "APPLY_ROUTE" : "KEEP", changed, mismatch, need_apply);

    printf("+-------+-----------+-------+------+-----+------+---------+--------+--------------+\n");
    printf("| iface | role      | ready | link | ip  | inet | deb     | eta    | note         |\n");
    printf("+-------+-----------+-------+------+-----+------+---------+--------+--------------+\n");
    render_iface_row("eth0", "primary", &snap->eth, &ctx->eth_a, bits->eth_up, have == NM4_WANT_ETH0,
                     now_ms, last_probe_ms, 1);
    render_iface_row("wlan0", "secondary", &snap->wlan, &ctx->wlan_a, bits->wlan_up,
                     have == NM4_WANT_WLAN0, now_ms, last_probe_ms, 1);
    render_iface_row("ppp0", "tertiary", &snap->ppp, &ctx->ppp_a, bits->ppp_up, have == NM4_WANT_PPP0,
                     now_ms, last_probe_ms, 1);
    printf("+-------+-----------+-------+------+-----+------+---------+--------+--------------+\n");

    render_route_dns_debug(want, have);

    printf("linkkeeper: wlan_k=%s (ok=%" PRIu64 " fail=%" PRIu64 ")  ppp_k=%s (ok=%" PRIu64 " fail=%" PRIu64 ")\n",
           wifi_k, lk->wifi_ok_cnt, lk->wifi_fail_cnt, ppp_k, lk->ppp_ok_cnt, lk->ppp_fail_cnt);

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
** Descriptions:        Managed-mode main loop: linkkeeper → probe → iface_avail → policy → route.
** input parameters:    cfgpath - netmgr JSON path.
**                      debug   - enable logging when not in table mode.
**                      view    - log or TTY table refresh mode.
** output parameters:   NONE
** Returned value:      0 when loop exits normally (SIGINT/SIGTERM or g_stop).
*********************************************************************************************************/
int nm4_manager_run(const char *cfgpath, bool debug, nm4_manager_view_t view)
{
    nm4_bits_t bits = {0}, prev = {0};
    nm4_probe_snapshot_t snap;
    nm4_iface_avail_t eth_a, wlan_a, ppp_a;
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

    memset(&snap, 0, sizeof(snap));
    memset(&stats, 0, sizeof(stats));
    memset(&events, 0, sizeof(events));
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
    nm4_policy_load_prior(cfgpath);
    (void)nm4_run_dir_ensure();
    (void)nm4_config_write_eth_runtime_cache(cfgpath);
    nm4_iface_avail_init(&eth_a, NM4_IFACE_ETH0);
    nm4_iface_avail_init(&wlan_a, NM4_IFACE_WLAN0);
    nm4_iface_avail_init(&ppp_a, NM4_IFACE_PPP0);

    (void)nm4_sim_init();
    if (lk.has_ppp && nm4_sim_present() != 1) {
        const char *pif = lk.ppp.ifname[0] ? lk.ppp.ifname : "ppp0";
        (void)nm4_adapter_ppp_stop(pif);
        (void)nm4_dns_cache_clear(pif);
        netmgr_v4_log("manager: no SIM, PPP idle (config_ppp=%d)", lk.has_ppp);
    }

    s_watch_had_eth_ip = nm4_if_has_ipv4("eth0");
    s_watch_had_wifi_ip = nm4_if_has_ipv4("wlan0");
    s_watch_had_ppp_ip = nm4_if_has_ipv4("ppp0");
    s_watch_had_eth_link = nm4_if_link_up("eth0");
    s_watch_had_wifi_link = nm4_if_link_up("wlan0");
    s_watch_had_ppp_link = nm4_if_link_up("ppp0");

    if (table_mode) {
        printf("\033[2J\033[H");
        (void)fflush(stdout);
    }
    netmgr_v4_log("manager: start cfg=%s prior=eth0>wlan0>ppp0", cfgpath);
    while (!g_stop) {
        int fast = g_arbitrate_now ? 1 : 0;
        int changed, mismatch, need_apply, rc;
        int ppp_eff;
        t = now_ms();

        nm4_manager_poll_arbitrate_flag();
        (void)nm4_config_write_eth_runtime_cache(cfgpath);

        if (nm4_sim_poll(now_ms()) == NM4_SIM_EDGE_CHANGE) {
            (void)nm4_sim_on_edge(&lk, t);
            fast = 1;
        }

        ppp_eff = nm4_sim_ppp_effective(&lk);

        if (fast && debug && !table_mode)
            netmgr_v4_log("manager: fast arbitrate (link/L3/SIM event)");

        nm4_linkkeeper_tick(&lk, t);
        have = nm4_route_get_have();
        probe_all_ifaces(&snap, t, have, &prev, ppp_eff);
        nm4_iface_avail_update(&eth_a, &snap.eth);
        nm4_iface_avail_update(&wlan_a, &snap.wlan);
        nm4_iface_avail_update(&ppp_a, &snap.ppp);
        if (!ppp_eff) {
            ppp_a.available = 0;
            ppp_a.raw = 0;
            ppp_a.down_streak = ppp_a.cfg.down_th;
            ppp_a.up_streak = 0;
        }
        nm4_iface_avail_to_bits(&eth_a, &wlan_a, &ppp_a, &bits);
        want = nm4_policy_pick_want(&bits, &snap.eth);
        night_reboot_maybe(want, t);
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
                if (want == NM4_WANT_WLAN0)
                    (void)nm4_adapter_wpa_renew("wlan0");
            }
            if (rc == 0) {
                have = want;
                pending = 0;
            }
        }

        {
            nm4_led_input_t li;
            li.wlan = &snap.wlan;
            li.ppp = &snap.ppp;
            li.bits = &bits;
            nm4_led_update(&li);
        }

        if (table_mode) {
            g_tbl.active = 1;
            g_tbl.cfgpath = cfgpath;
            g_tbl.tick = tick;
            g_tbl.lk = &lk;
            g_tbl.bits = bits;
            g_tbl.prev = prev;
            g_tbl.want = want;
            g_tbl.have = have;
            g_tbl.pending = pending;
            g_tbl.pending_deadline = pending_deadline;
            g_tbl.changed = changed;
            g_tbl.mismatch = mismatch;
            g_tbl.need_apply = need_apply;
            g_tbl.snap = snap;
            g_tbl.stats = stats;
            g_tbl.events = events;
            g_tbl.eth_a = eth_a;
            g_tbl.wlan_a = wlan_a;
            g_tbl.ppp_a = ppp_a;
            g_tbl.last_probe_ms = t;
            render_table(&g_tbl, t);
        } else {
            g_tbl.active = 0;
        }

        g_live.active = 1;
        g_live.cfgpath = cfgpath;
        g_live.snap = &snap;
        g_live.eth_a = &eth_a;
        g_live.wlan_a = &wlan_a;
        g_live.ppp_a = &ppp_a;
        g_live.bits = &bits;
        g_live.prev = &prev;
        g_live.want = &want;
        g_live.have = &have;
        g_live.pending = &pending;
        g_live.pending_deadline = &pending_deadline;
        g_live.stats = &stats;
        g_live.events = &events;
        g_live.changed = &changed;
        g_live.mismatch = &mismatch;
        g_live.need_apply = &need_apply;
        g_live.lk = &lk;
        nm4_manager_wait_steady(table_mode ? false : debug, table_mode, &lk);
        g_live.active = 0;
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
    }

    if (table_mode) printf("\n");
    nm4_sim_shutdown();
    nm4_linkkeeper_shutdown(&lk);
    nm4_led_all_off();
    nm4_adapter_set_quiet(0);
    netmgr_v4_log("manager: stop");
    return 0;
}
