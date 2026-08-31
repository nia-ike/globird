#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include "netmgr_v4.h"
#include "netmgr_v4_config.h"
#include "netmgr_v4_linkkeeper.h"
#include "netmgr_v4_manager.h"
#include "netmgr_v4_adapter.h"
#include "netmgr_v4_hw_sim.h"
#include "adapter_internal.h"

#define NM4_KEEPALIVE_WIFI_MS_DEFAULT 10000ULL
#define NM4_KEEPALIVE_PPP_MS_DEFAULT  60000ULL
#define NM4_MODEM_RECOVER_COOLDOWN_MS 300000ULL
#define NM4_MODEM_RECOVER_WAIT_TTY_MS 45000ULL
#define NM4_MODEM_STOP_SETTLE_MS 5000ULL
#define NM4_MODEM_START_WAIT_TTY_MS 30000ULL
#define NM4_LTE_CTRL_PATH "/ech/etc/init.d/lte_ctrl"

/* WiFi deadlink detection thresholds */
#define WIFI_DEADLINK_NOIP_TIMEOUT_MS 60000   /* 60 seconds without IP = deadlink */
#define WIFI_MAC_CHANGE_COOLDOWN_MS   300000  /* 5 minutes between MAC changes */
#define WIFI_MAC_SUFFIX_MIN           0x01    /* MAC suffix range: 01-0F */
#define WIFI_MAC_SUFFIX_MAX           0x0F

static int g_lk_log_enabled = 1;
static uint64_t g_keepalive_wifi_ms = NM4_KEEPALIVE_WIFI_MS_DEFAULT;
static uint64_t g_keepalive_ppp_ms = NM4_KEEPALIVE_PPP_MS_DEFAULT;

/*********************************************************************************************************
** Function name:       path_exists
** Descriptions:        Return whether path is non-empty and accessible via F_OK.
** input parameters:    path    - filesystem path.
** output parameters:   NONE
** Returned value:      1 if exists, 0 otherwise.
*********************************************************************************************************/
static int path_exists(const char *path)
{
    return path && path[0] && access(path, F_OK) == 0;
}

static int s_lk_had_wifi_ip;
static int s_lk_had_ppp_ip;

/*********************************************************************************************************
** Function name:       tmp_log_ppp_recover
** Descriptions:        Append one diagnostic line to /tmp/netmgrd_v4_ppp_recover.log.
** input parameters:    msg        - stage description.
**                      dev        - modem device path or tag.
**                      rc_stop    - lte_ctrl stop return code placeholder.
**                      rc_start   - lte_ctrl start return code placeholder.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void tmp_log_ppp_recover(const char *msg, const char *dev, int rc_stop, int rc_start)
{
    FILE *fp = fopen("/tmp/netmgrd_v4_ppp_recover.log", "a");
    if (!fp) return;
    (void)fprintf(fp, "[netmgrd_v4][ppp_recover] %s dev=%s rc_stop=%d rc_start=%d\n",
                  msg ? msg : "-", dev ? dev : "-", rc_stop, rc_start);
    (void)fclose(fp);
}

/*********************************************************************************************************
** Function name:       run_lte_ctrl
** Descriptions:        Run lte_ctrl script with given action (e.g. stop/start), output discarded.
** input parameters:    action  - argument passed to lte_ctrl.
** output parameters:   NONE
** Returned value:      nm4_run_cmd return status.
*********************************************************************************************************/
static int run_lte_ctrl(const char *action)
{
    char cmd[256];
    if (!action || !action[0]) return -1;
    (void)snprintf(cmd, sizeof(cmd), NM4_LTE_CTRL_PATH " %s >/dev/null 2>&1", action);
    return nm4_run_cmd(cmd);
}

/*********************************************************************************************************
** Function name:       wait_path_state
** Descriptions:        Poll until path existence matches expect_exists or timeout_ms elapses.
** input parameters:    path           - device node path.
**                      expect_exists  - 1 wait until present, 0 wait until absent.
**                      timeout_ms     - maximum wait in milliseconds.
** output parameters:   NONE
** Returned value:      0 on match, -1 on timeout.
*********************************************************************************************************/
static int wait_path_state(const char *path, int expect_exists, uint64_t timeout_ms)
{
    uint64_t waited = 0;
    while (waited < timeout_ms) {
        int exists = path_exists(path);
        if ((expect_exists && exists) || (!expect_exists && !exists))
            return 0;
        usleep(100000);
        waited += 100;
    }
    return -1;
}

/*********************************************************************************************************
** Function name:       ppp_try_modem_recover
** Descriptions:        On missing PPP serial device: stop pppd, lte_ctrl stop/start, wait for tty.
** input parameters:    lk       - linkkeeper state (device path, cooldown timestamps).
**                      now_ms   - monotonic wall ms for cooldown.
** output parameters:    lk       - may update modem recover counters and messages.
** Returned value:       0 if lte_ctrl start succeeded, -1 otherwise.
*********************************************************************************************************/
static int ppp_try_modem_recover(nm4_linkkeeper_t *lk, uint64_t now_ms)
{
    int rc_stop;
    int rc_start;

    tmp_log_ppp_recover("enter", lk ? lk->ppp.device : "-", -999, -999);

    if (!lk->ppp.device[0]) {
        tmp_log_ppp_recover("ppp device empty", "-", -1, -1);
        (void)snprintf(lk->ppp_last_msg, sizeof(lk->ppp_last_msg), "ppp device empty");
        return -1;
    }
    if (now_ms - lk->ppp_last_modem_recover_ms < NM4_MODEM_RECOVER_COOLDOWN_MS) {
        tmp_log_ppp_recover("cooldown", lk->ppp.device, -2, -2);
        (void)snprintf(lk->ppp_last_msg, sizeof(lk->ppp_last_msg), "tty missing,cooldown");
        return -1;
    }
    if (nm4_sim_present() != 1) {
        tmp_log_ppp_recover("no sim", lk->ppp.device, -3, -3);
        (void)snprintf(lk->ppp_last_msg, sizeof(lk->ppp_last_msg), "no sim,skip recover");
        return -1;
    }

    (void)nm4_adapter_ppp_stop(lk->ppp.ifname[0] ? lk->ppp.ifname : "ppp0");
    rc_stop = run_lte_ctrl("stop");
    (void)wait_path_state(lk->ppp.device, 0, NM4_MODEM_STOP_SETTLE_MS);
    rc_start = run_lte_ctrl("start");
    if (rc_start == 0)
        (void)wait_path_state(lk->ppp.device, 1, NM4_MODEM_START_WAIT_TTY_MS);
    tmp_log_ppp_recover("after stop/start", lk->ppp.device, rc_stop, rc_start);
    lk->ppp_last_modem_recover_ms = now_ms;

    if (rc_start == 0) {
        tmp_log_ppp_recover("recover success", lk->ppp.device, rc_stop, rc_start);
        lk->ppp_modem_recover_cnt++;
        lk->ppp_wait_tty_after_recover = 1;
        (void)snprintf(lk->ppp_last_msg, sizeof(lk->ppp_last_msg), "tty missing,restart modem");
        if (g_lk_log_enabled) {
            netmgr_v4_log("linkkeeper: ppp tty %s missing, lte_ctrl stop/start done (rc_stop=%d rc_start=%d)",
                          lk->ppp.device, rc_stop, rc_start);
        }
        return 0;
    }

    (void)snprintf(lk->ppp_last_msg, sizeof(lk->ppp_last_msg), "lte_ctrl start failed");
    tmp_log_ppp_recover("recover failed", lk->ppp.device, rc_stop, rc_start);
    if (g_lk_log_enabled) {
        netmgr_v4_log("linkkeeper: lte_ctrl stop/start failed rc_stop=%d rc_start=%d",
                      rc_stop, rc_start);
    }
    return -1;
}

/*********************************************************************************************************
** Function name:       nm4_linkkeeper_set_log_enabled
** Descriptions:        Enable or disable linkkeeper verbose netmgr_v4_log lines.
** input parameters:    enabled - non-zero to log.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_linkkeeper_set_log_enabled(int enabled)
{
    g_lk_log_enabled = enabled ? 1 : 0;
}

/*********************************************************************************************************
** Function name:       nm4_linkkeeper_set_intervals
** Descriptions:        Override global WiFi and PPP keepalive periods (ms).
** input parameters:    wifi_ms - WLAN keepalive period; 0 leaves previous.
**                      ppp_ms  - PPP keepalive period; 0 leaves previous.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_linkkeeper_set_intervals(uint64_t wifi_ms, uint64_t ppp_ms)
{
    if (wifi_ms > 0) g_keepalive_wifi_ms = wifi_ms;
    if (ppp_ms > 0) g_keepalive_ppp_ms = ppp_ms;
}

/*********************************************************************************************************
** Function name:       nm4_linkkeeper_get_intervals
** Descriptions:        Read current global WiFi and PPP keepalive periods into caller pointers.
** input parameters:    wifi_ms - optional pointer (may be NULL).
**                      ppp_ms  - optional pointer (may be NULL).
** output parameters:    *wifi_ms / *ppp_ms - current values when pointer non-NULL.
** Returned value:      NONE
*********************************************************************************************************/
void nm4_linkkeeper_get_intervals(uint64_t *wifi_ms, uint64_t *ppp_ms)
{
    if (wifi_ms) *wifi_ms = g_keepalive_wifi_ms;
    if (ppp_ms) *ppp_ms = g_keepalive_ppp_ms;
}

/*********************************************************************************************************
** Function name:       nm4_linkkeeper_init
** Descriptions:        Load WiFi and PPP adapter params from JSON into lk; set has_wifi/has_ppp flags.
** input parameters:    cfgpath - path to v3-style netmgr JSON.
**                      errlen  - size of err buffer.
** output parameters:    lk      - initialized structure.
**                      err     - error message on failure.
** Returned value:      0 if at least one of wifi/ppp configured, -1 if none.
*********************************************************************************************************/
int nm4_linkkeeper_init(nm4_linkkeeper_t *lk, const char *cfgpath, char *err, unsigned errlen)
{
    int rc;
    memset(lk, 0, sizeof(*lk));
    lk->wifi_last_rc = 0;
    lk->ppp_last_rc = 0;
    (void)snprintf(lk->wifi_last_msg, sizeof(lk->wifi_last_msg), "n/a");
    (void)snprintf(lk->ppp_last_msg, sizeof(lk->ppp_last_msg), "n/a");

    /* WiFi deadlink detection init */
    lk->wifi_no_ip_since_ms = 0;
    lk->wifi_mac_suffix = 0;
    lk->wifi_mac_change_cnt = 0;
    lk->wifi_last_mac_change_ms = 0;

    rc = nm4_config_load_wifi_from_v3_json(cfgpath, &lk->wpa, err, errlen);
    if (rc == 0) {
        lk->has_wifi = 1;
        if (lk->wpa.ifname[0] == '\0')
            (void)snprintf(lk->wpa.ifname, sizeof(lk->wpa.ifname), "wlan0");
        /* Read original MAC suffix at init */
        lk->wifi_mac_suffix = nm4_adapter_wpa_read_original_mac(lk->wpa.ifname);
        if (lk->wifi_mac_suffix == 0)
            lk->wifi_mac_suffix = 0x01;  /* Default if cannot read */
    }

    rc = nm4_config_load_ppp_from_v3_json(cfgpath, &lk->ppp, err, errlen);
    if (rc == 0) {
        lk->has_ppp = 1;
        if (lk->ppp.ifname[0] == '\0')
            (void)snprintf(lk->ppp.ifname, sizeof(lk->ppp.ifname), "ppp0");
    }

    if (!lk->has_wifi && !lk->has_ppp) {
        (void)snprintf(err, errlen, "no wifi/ppp config for linkkeeper");
        return -1;
    }
    if (errlen > 0) err[0] = '\0';
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_linkkeeper_shutdown
** Descriptions:        Stop WiFi/PPP adapters on manager exit so restart reapplies config.
** input parameters:    lk      - linkkeeper state with ifnames.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_linkkeeper_shutdown(const nm4_linkkeeper_t *lk)
{
    if (!lk)
        return;

    if (lk->has_wifi && lk->wpa.ifname[0])
        (void)nm4_adapter_wpa_stop(lk->wpa.ifname);
    if (lk->has_ppp && lk->ppp.ifname[0])
        (void)nm4_adapter_ppp_stop(lk->ppp.ifname);
}

/*********************************************************************************************************
** Function name:       nm4_linkkeeper_tick
** Descriptions:        One keepalive tick: optional WiFi/PPP adapter start, modem recover if tty missing.
** input parameters:    lk      - linkkeeper state.
**                      now_ms  - current time in milliseconds.
** output parameters:    lk      - last_keep timestamps, due flags, counters, last_msg fields updated.
** Returned value:      NONE
*********************************************************************************************************/
void nm4_linkkeeper_tick(nm4_linkkeeper_t *lk, uint64_t now_ms)
{
    int wifi_due = 0;
    int ppp_due = 0;
    int rc;
    uint64_t wifi_next_in = 0;
    uint64_t ppp_next_in = 0;

    if (lk->has_wifi && (lk->last_wifi_keep_ms == 0 ||
        now_ms - lk->last_wifi_keep_ms >= g_keepalive_wifi_ms))
        wifi_due = 1;
    if (lk->has_ppp && (lk->last_ppp_keep_ms == 0 ||
        now_ms - lk->last_ppp_keep_ms >= g_keepalive_ppp_ms))
        ppp_due = 1;
    if (lk->has_ppp && !nm4_sim_ppp_effective(lk))
        ppp_due = 0;

    if (lk->has_wifi) {
        if (lk->last_wifi_keep_ms == 0 || wifi_due) wifi_next_in = 0;
        else wifi_next_in = g_keepalive_wifi_ms - (now_ms - lk->last_wifi_keep_ms);
    }
    if (lk->has_ppp) {
        if (lk->last_ppp_keep_ms == 0 || ppp_due) ppp_next_in = 0;
        else ppp_next_in = g_keepalive_ppp_ms - (now_ms - lk->last_ppp_keep_ms);
    }

    lk->wifi_due = wifi_due;
    lk->ppp_due = ppp_due;
    lk->wifi_next_in_ms = wifi_next_in;
    lk->ppp_next_in_ms = ppp_next_in;

    if (g_lk_log_enabled) {
        netmgr_v4_log(
            "linkkeeper: tick now=%" PRIu64 " wifi(due=%d,next_in=%" PRIu64 "ms) ppp(due=%d,next_in=%" PRIu64 "ms)",
            now_ms, wifi_due, wifi_next_in, ppp_due, ppp_next_in);
    }

    if (lk->has_ppp && !nm4_sim_ppp_effective(lk)) {
        lk->ppp_due = 0;
        lk->ppp_next_in_ms = 0;
        (void)snprintf(lk->ppp_last_msg, sizeof(lk->ppp_last_msg), "no sim,ppp idle");
    }

    if (wifi_due) {
        int has_ip;
        uint64_t no_ip_duration;

        lk->last_wifi_keep_ms = now_ms;
        if (g_lk_log_enabled)
            netmgr_v4_log("linkkeeper: wifi keepalive interval reached, running start/check");
        rc = nm4_adapter_wpa_start(&lk->wpa);
        lk->wifi_last_rc = rc;
        if (rc != 0) {
            lk->wifi_fail_cnt++;
            (void)snprintf(lk->wifi_last_msg, sizeof(lk->wifi_last_msg), "keepalive start failed");
            if (g_lk_log_enabled)
                netmgr_v4_log("linkkeeper: wifi keepalive start failed");
        } else {
            lk->wifi_ok_cnt++;
            (void)snprintf(lk->wifi_last_msg, sizeof(lk->wifi_last_msg), "keepalive ok");
            if (g_lk_log_enabled)
                netmgr_v4_log("linkkeeper: wifi keepalive ok");
        }

        /* WiFi deadlink detection: wpa_supplicant running but no IP for too long */
        has_ip = nm4_if_has_ipv4(lk->wpa.ifname);
        if (has_ip && !s_lk_had_wifi_ip)
            nm4_manager_request_arbitrate();
        s_lk_had_wifi_ip = has_ip;
        if (has_ip) {
            /* Got IP, reset timer */
            if (lk->wifi_no_ip_since_ms != 0) {
                lk->wifi_no_ip_since_ms = 0;
                if (g_lk_log_enabled)
                    netmgr_v4_log("linkkeeper: wifi got IP, reset deadlink timer");
            }
        } else {
            /* No IP, start/continue timer */
            if (lk->wifi_no_ip_since_ms == 0) {
                lk->wifi_no_ip_since_ms = now_ms;
                if (g_lk_log_enabled)
                    netmgr_v4_log("linkkeeper: wifi no IP, start deadlink timer");
            } else {
                no_ip_duration = now_ms - lk->wifi_no_ip_since_ms;
                if (g_lk_log_enabled)
                    netmgr_v4_log("linkkeeper: wifi no IP for %" PRIu64 " ms", no_ip_duration);

                /* Check if deadlink timeout reached */
                if (no_ip_duration > WIFI_DEADLINK_NOIP_TIMEOUT_MS) {
                    int can_change_mac = 1;

                    /* Check cooldown */
                    if (lk->wifi_last_mac_change_ms != 0 &&
                        (now_ms - lk->wifi_last_mac_change_ms) < WIFI_MAC_CHANGE_COOLDOWN_MS) {
                        can_change_mac = 0;
                        if (g_lk_log_enabled)
                            netmgr_v4_log("linkkeeper: wifi deadlink detected but MAC change in cooldown");
                    }

                    if (can_change_mac) {
                        /* Increment MAC suffix (01->02->...->0F->01) */
                        lk->wifi_mac_suffix++;
                        if (lk->wifi_mac_suffix > WIFI_MAC_SUFFIX_MAX)
                            lk->wifi_mac_suffix = WIFI_MAC_SUFFIX_MIN;

                        if (g_lk_log_enabled)
                            netmgr_v4_log("linkkeeper: wifi deadlink detected, changing MAC (suffix=0x%02X)",
                                          lk->wifi_mac_suffix);

                        /* Stop WiFi and change MAC */
                        rc = nm4_adapter_wpa_change_mac(lk->wpa.ifname, lk->wifi_mac_suffix);
                        if (rc == 0) {
                            lk->wifi_mac_change_cnt++;
                            lk->wifi_last_mac_change_ms = now_ms;
                            lk->wifi_no_ip_since_ms = 0;  /* Reset timer after MAC change */
                            (void)snprintf(lk->wifi_last_msg, sizeof(lk->wifi_last_msg),
                                           "deadlink,MAC changed");
                            if (g_lk_log_enabled)
                                netmgr_v4_log("linkkeeper: wifi MAC changed, count=%u",
                                              lk->wifi_mac_change_cnt);
                        } else {
                            (void)snprintf(lk->wifi_last_msg, sizeof(lk->wifi_last_msg),
                                           "deadlink,MAC change failed");
                            if (g_lk_log_enabled)
                                netmgr_v4_log("linkkeeper: wifi MAC change failed");
                        }
                    }
                }
            }
        }
    }

    if (lk->has_ppp && !nm4_sim_ppp_effective(lk)) {
        return;
    }

    if (lk->has_ppp && !path_exists(lk->ppp.device)) {
        tmp_log_ppp_recover("tty missing branch", lk->ppp.device[0] ? lk->ppp.device : "(empty)", -100, -100);
        lk->ppp_last_rc = -1;
        if (lk->ppp_wait_tty_after_recover &&
            now_ms - lk->ppp_last_modem_recover_ms < NM4_MODEM_RECOVER_WAIT_TTY_MS) {
            tmp_log_ppp_recover("waiting tty window", lk->ppp.device[0] ? lk->ppp.device : "(empty)", -101, -101);
            (void)snprintf(lk->ppp_last_msg, sizeof(lk->ppp_last_msg), "waiting tty after restart");
        } else {
            lk->ppp_wait_tty_after_recover = 0;
            (void)ppp_try_modem_recover(lk, now_ms);
            lk->ppp_fail_cnt++;
        }
        if (g_lk_log_enabled) {
            netmgr_v4_log("linkkeeper: ppp device missing (%s), skip pppd start",
                          lk->ppp.device[0] ? lk->ppp.device : "(empty)");
        }
        return;
    }

    if (!lk->has_ppp) {
        tmp_log_ppp_recover("skip: has_ppp=0", "(none)", -200, -200);
    }

    if (ppp_due) {
        lk->last_ppp_keep_ms = now_ms;
        if (g_lk_log_enabled)
            netmgr_v4_log("linkkeeper: ppp keepalive interval reached, running start/check");
        lk->ppp_wait_tty_after_recover = 0;
        rc = nm4_adapter_ppp_start(&lk->ppp);
        lk->ppp_last_rc = rc;
        if (rc != 0) {
            lk->ppp_fail_cnt++;
            (void)snprintf(lk->ppp_last_msg, sizeof(lk->ppp_last_msg), "keepalive start failed");
            if (g_lk_log_enabled)
                netmgr_v4_log("linkkeeper: ppp keepalive start failed");
        } else {
            lk->ppp_ok_cnt++;
            (void)snprintf(lk->ppp_last_msg, sizeof(lk->ppp_last_msg), "keepalive ok");
            if (g_lk_log_enabled)
                netmgr_v4_log("linkkeeper: ppp keepalive ok");
        }
        {
            int ppp_ip = nm4_if_has_ipv4(lk->ppp.ifname[0] ? lk->ppp.ifname : "ppp0");
            if (ppp_ip && !s_lk_had_ppp_ip)
                nm4_manager_request_arbitrate();
            s_lk_had_ppp_ip = ppp_ip;
        }
    }
}
