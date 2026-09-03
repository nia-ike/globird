/*************************************************************************/
/** hw_sim.c — SIM 卡槽检测 + 合并 sim_detect.sh（边沿触发 lte_ctrl restart）      **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#include "netmgr_v4.h"
#include "netmgr_v4_hw_sim.h"
#include "netmgr_v4_gateway.h"
#include "netmgr_v4_dns.h"
#include "netmgr_v4_manager.h"
#include "netmgr_v4_adapter.h"

#define NM4_SIM_DEV_PATH            "/dev/input/event1"
#define NM4_SIM_KEY_CODE            12
#define NM4_LTE_CTRL_PATH           "/ech/etc/init.d/lte_ctrl"
/* 快拔插：首次触发后窗口内再次 event/电平变化合并为一次处理 */
#define NM4_SIM_HYSTERESIS_MS         5000ULL
#define NM4_SIM_PRESENT_WAIT_MS       5000ULL
#define NM4_MODEM_START_WAIT_TTY_MS   30000ULL

static int s_sim_fd = -1;
static int s_last_present = -1;
static uint64_t s_hysteresis_until_ms = 0;

static int sim_read_present_ioctl(int *out)
{
    int fd;
    unsigned char key_map[(KEY_MAX + 7) / 8];

    if (!out)
        return -1;
    *out = -1;

    fd = open(NM4_SIM_DEV_PATH, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
        return -1;

    memset(key_map, 0, sizeof(key_map));
    if (ioctl(fd, EVIOCGKEY(sizeof(key_map)), key_map) >= 0)
        *out = !(key_map[NM4_SIM_KEY_CODE / 8] & (1 << (NM4_SIM_KEY_CODE % 8)));
    close(fd);
    return (*out >= 0) ? 0 : -1;
}

static void sim_write_cache(int present)
{
    char path[128];
    FILE *fp;

    if (nm4_run_dir_ensure() != 0)
        return;
    (void)snprintf(path, sizeof(path), "%s", NM4_SIM_PRESENT_PATH);
    fp = fopen(path, "w");
    if (!fp)
        return;
    if (present < 0)
        (void)fprintf(fp, "-1\n");
    else
        (void)fprintf(fp, "%d\n", present);
    (void)fclose(fp);
}

static int wait_path_exists(const char *path, int want_exists, unsigned wait_ms)
{
    unsigned waited = 0;

    if (!path || !path[0])
        return -1;
    while (waited < wait_ms) {
        if (want_exists && access(path, F_OK) == 0)
            return 0;
        if (!want_exists && access(path, F_OK) != 0)
            return 0;
        usleep(100000);
        waited += 100;
    }
    return -1;
}

static int run_lte_ctrl(const char *action)
{
    char cmd[128];

    if (!action || !action[0])
        return -1;
    (void)snprintf(cmd, sizeof(cmd), "%s %s >/dev/null 2>&1", NM4_LTE_CTRL_PATH, action);
    return system(cmd);
}

static int modem_lte_restart(void)
{
    return run_lte_ctrl("restart") == 0 ? 0 : -1;
}

static void sim_wait_present_stable(int want_present)
{
    unsigned waited = 0;

    while (waited < NM4_SIM_PRESENT_WAIT_MS) {
        int p = nm4_sim_present();
        if (want_present == 1 && p == 1) {
            s_last_present = 1;
            sim_write_cache(1);
            return;
        }
        if (want_present == 0 && p == 0) {
            s_last_present = 0;
            sim_write_cache(0);
            return;
        }
        usleep(100000);
        waited += 100;
    }
}

int nm4_sim_init(void)
{
    int present;

    s_sim_fd = open(NM4_SIM_DEV_PATH, O_RDONLY | O_NONBLOCK);
    s_hysteresis_until_ms = 0;
    present = nm4_sim_present();
    s_last_present = present;
    sim_write_cache(present);

    if (present != 1 && s_sim_fd >= 0) {
        netmgr_v4_log("hw_sim: init sim=%d (no card or detect err)", present);
    } else if (present == 1) {
        netmgr_v4_log("hw_sim: init sim=1 (card present)");
    }

    return (s_sim_fd >= 0 || present >= 0) ? 0 : -1;
}

void nm4_sim_shutdown(void)
{
    if (s_sim_fd >= 0) {
        close(s_sim_fd);
        s_sim_fd = -1;
    }
}

int nm4_sim_present(void)
{
    int state = -1;

    if (sim_read_present_ioctl(&state) != 0)
        return -1;
    return state;
}

int nm4_sim_last_present(void)
{
    return s_last_present;
}

int nm4_sim_debounce_active(uint64_t now_ms)
{
    return (s_hysteresis_until_ms > 0 && now_ms < s_hysteresis_until_ms) ? 1 : 0;
}

nm4_sim_edge_t nm4_sim_poll(uint64_t now_ms)
{
    int present;
    int activity = 0;

    if (s_sim_fd >= 0) {
        struct input_event ev;
        ssize_t n;

        for (;;) {
            n = read(s_sim_fd, &ev, sizeof(ev));
            if (n == (ssize_t)sizeof(ev)) {
                activity = 1;
                continue;
            }
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                break;
            break;
        }
    }

    present = nm4_sim_present();
    if (present >= 0)
        sim_write_cache(present);

    if (!activity && present >= 0 && s_last_present >= 0 && present != s_last_present)
        activity = 1;

    if (present >= 0)
        s_last_present = present;

    if (!activity)
        return NM4_SIM_EDGE_NONE;

    if (s_hysteresis_until_ms > 0 && now_ms < s_hysteresis_until_ms) {
        netmgr_v4_log("hw_sim: activity suppressed (hysteresis %" PRIu64 "ms left)",
                      s_hysteresis_until_ms - now_ms);
        return NM4_SIM_EDGE_NONE;
    }

    s_hysteresis_until_ms = now_ms + NM4_SIM_HYSTERESIS_MS;
    netmgr_v4_log("hw_sim: activity -> edge (hysteresis %" PRIu64 "ms)", NM4_SIM_HYSTERESIS_MS);
    return NM4_SIM_EDGE_CHANGE;
}

int nm4_sim_ppp_effective(const nm4_linkkeeper_t *lk)
{
    if (!lk || !lk->has_ppp)
        return 0;
    return nm4_sim_present() == 1 ? 1 : 0;
}

int nm4_sim_on_edge(nm4_linkkeeper_t *lk, uint64_t now_ms)
{
    int present_before;
    int present;
    int rc;
    const char *ifn;

    present_before = nm4_sim_present();

    netmgr_v4_log("hw_sim: handle edge (present=%d), lte_ctrl restart", present_before);

    if (lk && lk->has_ppp) {
        ifn = lk->ppp.ifname[0] ? lk->ppp.ifname : "ppp0";
        (void)nm4_adapter_ppp_stop(ifn);
        (void)nm4_dns_cache_clear(ifn);
        lk->ppp_wait_tty_after_recover = 0;
    }

    rc = modem_lte_restart();

    /*
     * restart 后等待稳态：快拔插时窗口内可能最终是插卡或空槽，
     * 优先等插卡(1)，超时则以当前读数为准。
     */
    if (present_before != 1)
        sim_wait_present_stable(1);
    else
        sim_wait_present_stable(0);

    present = nm4_sim_present();
    if (present >= 0) {
        s_last_present = present;
        sim_write_cache(present);
    }

    if (rc == 0 && lk && lk->has_ppp && lk->ppp.device[0]) {
        lk->ppp_last_modem_recover_ms = now_ms;
        lk->ppp_wait_tty_after_recover = 1;
        lk->ppp_modem_recover_cnt++;
        (void)wait_path_exists(lk->ppp.device, 1, NM4_MODEM_START_WAIT_TTY_MS);
        (void)snprintf(lk->ppp_last_msg, sizeof(lk->ppp_last_msg), "sim edge,modem restart");
    }

    nm4_manager_request_arbitrate();
    return rc;
}
