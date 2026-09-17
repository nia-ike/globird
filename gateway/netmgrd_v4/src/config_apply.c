/*************************************************************************/
/** config_apply.c — touch /var/run/netmgr/apply 触发，原地重读 network.json **/
/*************************************************************************/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "netmgr_v4_apply.h"
#include "netmgr_v4_config.h"
#include "netmgr_v4_adapter.h"
#include "netmgr_v4_manager.h"
#include "adapter_internal.h"
#include "netmgr_v4.h"
#include "cJSON.h"

typedef struct {
    int wifi_enabled;
    nm4_wpa_params_t wpa;
    int ppp_enabled;
    nm4_ppp_params_t ppp;
} nm4_apply_snap_t;

static int json_checked(cJSON *obj)
{
    cJSON *c;

    if (!obj)
        return 0;
    c = cJSON_GetObjectItem(obj, "checked");
    if (!c)
        return 0;
    if (cJSON_IsBool(c))
        return cJSON_IsTrue(c) ? 1 : 0;
    if (cJSON_IsNumber(c))
        return c->valueint != 0;
    return 0;
}

static void json_str(cJSON *obj, const char *k, char *out, size_t n)
{
    cJSON *it = cJSON_GetObjectItem(obj, k);

    if (it && cJSON_IsString(it) && it->valuestring)
        (void)snprintf(out, n, "%s", it->valuestring);
    else if (out && n)
        out[0] = '\0';
}

static char *read_file(const char *path, size_t *out_sz)
{
    FILE *fp;
    long sz;
    char *buf;

    if (!path || !out_sz)
        return NULL;
    *out_sz = 0;
    fp = fopen(path, "rb");
    if (!fp)
        return NULL;
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    sz = ftell(fp);
    if (sz <= 0) {
        fclose(fp);
        return NULL;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }
    buf = malloc((size_t)sz + 1u);
    if (!buf) {
        fclose(fp);
        return NULL;
    }
    if (fread(buf, 1, (size_t)sz, fp) != (size_t)sz) {
        free(buf);
        fclose(fp);
        return NULL;
    }
    buf[sz] = '\0';
    fclose(fp);
    *out_sz = (size_t)sz;
    return buf;
}

static int load_snap(const char *path, nm4_apply_snap_t *out, char *err, size_t errlen)
{
    cJSON *root, *wifi, *ppp;
    char *buf;
    size_t bz;

    if (!path || !out) {
        (void)snprintf(err, errlen, "invalid args");
        return -1;
    }
    memset(out, 0, sizeof(*out));
    (void)snprintf(out->wpa.ifname, sizeof(out->wpa.ifname), "wlan0");
    (void)snprintf(out->ppp.ifname, sizeof(out->ppp.ifname), "ppp0");
    (void)snprintf(out->ppp.device, sizeof(out->ppp.device), "/dev/ttyUSB4");
    (void)snprintf(out->ppp.dial, sizeof(out->ppp.dial), "*99#");

    if (access(path, R_OK) != 0) {
        (void)snprintf(err, errlen, "cannot read %s", path);
        return -1;
    }
    buf = read_file(path, &bz);
    if (!buf) {
        (void)snprintf(err, errlen, "empty or unreadable %s", path);
        return -1;
    }
    root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        (void)snprintf(err, errlen, "JSON parse error in %s", path);
        return -1;
    }

    wifi = cJSON_GetObjectItem(root, "wifiSta");
    if (wifi && cJSON_IsObject(wifi)) {
        out->wifi_enabled = json_checked(wifi);
        json_str(wifi, "ssid", out->wpa.ssid, sizeof(out->wpa.ssid));
        json_str(wifi, "psk", out->wpa.psk, sizeof(out->wpa.psk));
        out->wpa.wpa_conf[0] = '\0';
    }

    ppp = cJSON_GetObjectItem(root, "ppp");
    if (ppp && cJSON_IsObject(ppp)) {
        out->ppp_enabled = json_checked(ppp);
        json_str(ppp, "apn", out->ppp.apn, sizeof(out->ppp.apn));
        json_str(ppp, "user", out->ppp.user, sizeof(out->ppp.user));
        json_str(ppp, "passwd", out->ppp.password, sizeof(out->ppp.password));
        {
            char dial[32];
            json_str(ppp, "phonenumber", dial, sizeof(dial));
            if (dial[0])
                (void)snprintf(out->ppp.dial, sizeof(out->ppp.dial), "%s", dial);
        }
    }

    cJSON_Delete(root);
    err[0] = '\0';
    return 0;
}

static int validate_snap(const nm4_apply_snap_t *s, char *err, size_t errlen)
{
    if (!s) {
        (void)snprintf(err, errlen, "null snap");
        return -1;
    }
    if (s->wifi_enabled) {
        if (!s->wpa.ssid[0]) {
            (void)snprintf(err, errlen, "wifiSta enabled but ssid empty");
            return -1;
        }
        if (!s->wpa.psk[0]) {
            (void)snprintf(err, errlen, "wifiSta enabled but psk empty");
            return -1;
        }
    }
    if (s->ppp_enabled) {
        if (!s->ppp.apn[0]) {
            (void)snprintf(err, errlen, "ppp enabled but apn empty");
            return -1;
        }
    }
    err[0] = '\0';
    return 0;
}

static int wpa_params_equal(const nm4_wpa_params_t *a, const nm4_wpa_params_t *b)
{
    if (!a || !b)
        return 0;
    if (strcmp(a->ifname, b->ifname) != 0)
        return 0;
    if (strcmp(a->wpa_conf, b->wpa_conf) != 0)
        return 0;
    if (strcmp(a->ssid, b->ssid) != 0)
        return 0;
    if (strcmp(a->psk, b->psk) != 0)
        return 0;
    return 1;
}

static int ppp_params_equal(const nm4_ppp_params_t *a, const nm4_ppp_params_t *b)
{
    if (!a || !b)
        return 0;
    if (strcmp(a->ifname, b->ifname) != 0)
        return 0;
    if (strcmp(a->device, b->device) != 0)
        return 0;
    if (strcmp(a->apn, b->apn) != 0)
        return 0;
    if (strcmp(a->user, b->user) != 0)
        return 0;
    if (strcmp(a->password, b->password) != 0)
        return 0;
    if (strcmp(a->dial, b->dial) != 0)
        return 0;
    return 1;
}

static void write_result(int ok, const char *wifi_act, const char *ppp_act,
                         const char *eth_act, const char *msg)
{
    char tmp[160];
    FILE *fp;
    time_t now = time(NULL);

    (void)nm4_run_dir_ensure();
    (void)snprintf(tmp, sizeof(tmp), "%s.tmp", NM4_APPLY_RESULT);
    fp = fopen(tmp, "w");
    if (!fp)
        return;
    (void)fprintf(fp,
                  "{\"ok\":%s,\"ts\":%ld,\"wifi\":\"%s\",\"ppp\":\"%s\",\"eth\":\"%s\",\"msg\":\"%s\"}\n",
                  ok ? "true" : "false",
                  (long)now,
                  wifi_act ? wifi_act : "noop",
                  ppp_act ? ppp_act : "noop",
                  eth_act ? eth_act : "noop",
                  msg ? msg : "");
    (void)fclose(fp);
    (void)rename(tmp, NM4_APPLY_RESULT);
}

static const char *apply_wifi(nm4_linkkeeper_t *lk, const nm4_apply_snap_t *cand, int *changed)
{
    int need;

    *changed = 0;
    need = (lk->has_wifi != cand->wifi_enabled) ||
           (cand->wifi_enabled && !wpa_params_equal(&lk->wpa, &cand->wpa));

    if (!need)
        return "noop";

    *changed = 1;
    if (lk->has_wifi) {
        const char *ifn = lk->wpa.ifname[0] ? lk->wpa.ifname : "wlan0";
        (void)nm4_adapter_wpa_stop(ifn);
        lk->has_wifi = 0;
    }

    if (cand->wifi_enabled) {
        lk->wpa = cand->wpa;
        lk->has_wifi = 1;
        lk->wifi_no_ip_since_ms = 0;
        if (nm4_adapter_wpa_start(&lk->wpa) != 0)
            return "restart_fail";
        return "restart";
    }
    return "stop";
}

static const char *apply_ppp(nm4_linkkeeper_t *lk, const nm4_apply_snap_t *cand, int *changed)
{
    int need;

    *changed = 0;
    need = (lk->has_ppp != cand->ppp_enabled) ||
           (cand->ppp_enabled && !ppp_params_equal(&lk->ppp, &cand->ppp));

    if (!need)
        return "noop";

    *changed = 1;
    if (lk->has_ppp) {
        const char *ifn = lk->ppp.ifname[0] ? lk->ppp.ifname : "ppp0";
        (void)nm4_adapter_ppp_stop(ifn);
        lk->has_ppp = 0;
    }

    if (cand->ppp_enabled) {
        lk->ppp = cand->ppp;
        lk->has_ppp = 1;
        if (nm4_adapter_ppp_start(&lk->ppp) != 0)
            return "restart_fail";
        return "restart";
    }
    return "stop";
}

int nm4_config_apply_runtime(const char *cfgpath, nm4_linkkeeper_t *lk)
{
    nm4_apply_snap_t cand;
    char err[256];
    const char *wifi_act = "noop";
    const char *ppp_act = "noop";
    const char *eth_act = "cache";
    int wchg = 0, pchg = 0;
    int ok = 1;

    if (!cfgpath || !cfgpath[0] || !lk) {
        write_result(0, "noop", "noop", "noop", "invalid args");
        return -1;
    }

    if (load_snap(cfgpath, &cand, err, sizeof(err)) != 0) {
        write_result(0, "noop", "noop", "noop", err);
        netmgr_v4_log("apply: load fail: %s", err);
        return -1;
    }
    if (validate_snap(&cand, err, sizeof(err)) != 0) {
        write_result(0, "noop", "noop", "noop", err);
        netmgr_v4_log("apply: validate fail: %s", err);
        return -1;
    }

    wifi_act = apply_wifi(lk, &cand, &wchg);
    ppp_act = apply_ppp(lk, &cand, &pchg);
    (void)nm4_config_write_eth_runtime_cache(cfgpath);

    if (strcmp(wifi_act, "restart_fail") == 0 || strcmp(ppp_act, "restart_fail") == 0) {
        ok = 0;
        (void)snprintf(err, sizeof(err), "iface apply failed wifi=%s ppp=%s", wifi_act, ppp_act);
    } else {
        err[0] = '\0';
    }

    write_result(ok, wifi_act, ppp_act, eth_act, err);
    netmgr_v4_log("apply: ok=%d wifi=%s ppp=%s eth=%s msg=%s",
                  ok, wifi_act, ppp_act, eth_act, err[0] ? err : "-");

    if (wchg || pchg)
        nm4_manager_request_arbitrate();

    return ok ? 0 : -1;
}

static int read_daemon_pid(pid_t *out)
{
    FILE *fp;
    int pid;

    if (!out)
        return -1;
    fp = fopen(NM4_PIDFILE, "r");
    if (!fp)
        return -1;
    if (fscanf(fp, "%d", &pid) != 1 || pid <= 1) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    if (kill((pid_t)pid, 0) != 0)
        return -1;
    *out = (pid_t)pid;
    return 0;
}

static int result_ok(void)
{
    char *buf;
    size_t bz;
    int ok = 0;

    buf = read_file(NM4_APPLY_RESULT, &bz);
    if (!buf)
        return -1;
    if (strstr(buf, "\"ok\":true"))
        ok = 1;
    else if (strstr(buf, "\"ok\":false"))
        ok = 0;
    else
        ok = -1;
    free(buf);
    return ok;
}

int nm4_apply_client(unsigned timeout_ms)
{
    pid_t pid;
    unsigned waited = 0;
    unsigned step = 100;
    int fd;

    if (timeout_ms == 0)
        timeout_ms = NM4_APPLY_WAIT_MS;

    if (read_daemon_pid(&pid) != 0) {
        fprintf(stderr, "[netmgrd] --apply: no running daemon (%s)\n", NM4_PIDFILE);
        return 1;
    }
    (void)pid;

    (void)nm4_run_dir_ensure();
    (void)unlink(NM4_APPLY_RESULT);

    fd = open(NM4_APPLY_FLAG, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        fprintf(stderr, "[netmgrd] --apply: cannot create %s: %s\n",
                NM4_APPLY_FLAG, strerror(errno));
        return 1;
    }
    (void)close(fd);

    while (waited < timeout_ms) {
        if (access(NM4_APPLY_RESULT, R_OK) == 0) {
            int r = result_ok();
            if (r == 1) {
                printf("[netmgrd] --apply: OK\n");
                return 0;
            }
            if (r == 0) {
                fprintf(stderr, "[netmgrd] --apply: daemon reported fail (see %s)\n",
                        NM4_APPLY_RESULT);
                return 1;
            }
        }
        usleep(step * 1000U);
        waited += step;
    }

    fprintf(stderr, "[netmgrd] --apply: timeout waiting %s\n", NM4_APPLY_RESULT);
    return 1;
}
