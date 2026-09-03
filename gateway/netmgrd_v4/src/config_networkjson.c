#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include "netmgr_v4_config.h"
#include "netmgr_v4_probe.h"
#include "netmgr_v4_gateway.h"
#include "netmgr_v4_dns.h"
#include "cJSON.h"

#define NM4_FORWARD_DIR       "/userdata/config/forward"
#define NM4_FORWARD_APP_DIR   "/userdata/config/forward/app"
#define NM4_DEFAULT_PPP_DEV   "/dev/ttyUSB4"
#define NM4_DEFAULT_PPP_DIAL  "*99#"
#define NM4_PROBE_TIMEOUT_MS  2000

static char *nm4_read_file(const char *path, size_t *out_sz)
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
    fseek(fp, 0, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (sz <= 0) {
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

static int nm4_json_checked(cJSON *obj)
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

static void nm4_json_str(cJSON *obj, const char *k, char *out, size_t n)
{
    cJSON *it = cJSON_GetObjectItem(obj, k);
    if (it && cJSON_IsString(it) && it->valuestring)
        snprintf(out, n, "%s", it->valuestring);
    else
        out[0] = '\0';
}

static int nm4_host_valid(const char *host)
{
    if (!host || !host[0])
        return 0;
    if (strcmp(host, "127.0.0.1") == 0 || strcmp(host, "0.0.0.0") == 0)
        return 0;
    return 1;
}

static void nm4_extract_host(const char *dev, char *out, size_t n)
{
    const char *colon;
    size_t len;

    if (!out || n == 0)
        return;
    out[0] = '\0';
    if (!dev || !dev[0])
        return;
    colon = strchr(dev, ':');
    len = colon ? (size_t)(colon - dev) : strlen(dev);
    if (len >= n)
        len = n - 1;
    memcpy(out, dev, len);
    out[len] = '\0';
}

static int nm4_probe_target_exists(const nm4_probe_params_t *p, const char *host)
{
    int i;

    for (i = 0; i < p->target_count; i++) {
        if (strcmp(p->targets[i], host) == 0)
            return 1;
    }
    return 0;
}

static int nm4_probe_add_target(nm4_probe_params_t *p, const char *host)
{
    if (!p || !nm4_host_valid(host))
        return 0;
    if (nm4_probe_target_exists(p, host))
        return 0;
    if (p->target_count >= NM4_PROBE_MAX_TARGETS)
        return 0;
    (void)snprintf(p->targets[p->target_count], sizeof(p->targets[p->target_count]), "%s", host);
    p->target_count++;
    return 1;
}

static void nm4_scan_forward_hosts(nm4_probe_params_t *p, const char *dir, int app_style)
{
    DIR *d;
    struct dirent *ent;
    char path[512];
    char *buf;
    size_t bz;
    cJSON *root, *checked, *dev;

    d = opendir(dir);
    if (!d)
        return;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.')
            continue;
        (void)snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
        if (access(path, R_OK) != 0)
            continue;
        buf = nm4_read_file(path, &bz);
        if (!buf)
            continue;
        root = cJSON_Parse(buf);
        free(buf);
        if (!root)
            continue;
        if (app_style) {
            checked = cJSON_GetObjectItem(root, "Checked");
            if (!checked || !cJSON_IsNumber(checked) || checked->valueint == 0) {
                cJSON_Delete(root);
                continue;
            }
            dev = cJSON_GetObjectItem(root, "ip");
        } else {
            cJSON *attr = cJSON_GetObjectItem(root, "attr");
            if (!attr || !cJSON_IsObject(attr)) {
                cJSON_Delete(root);
                continue;
            }
            checked = cJSON_GetObjectItem(attr, "checked");
            if (!checked || !cJSON_IsNumber(checked) || checked->valueint == 0) {
                cJSON_Delete(root);
                continue;
            }
            cJSON *common = cJSON_GetObjectItem(attr, "common");
            dev = common ? cJSON_GetObjectItem(common, "dev") : NULL;
        }
        if (dev && cJSON_IsString(dev) && dev->valuestring) {
            char host[64];
            nm4_extract_host(dev->valuestring, host, sizeof(host));
            nm4_probe_add_target(p, host);
        }
        cJSON_Delete(root);
    }
    closedir(d);
}

int nm4_config_load_wifi_from_network_json(const char *path, nm4_wpa_params_t *out, char *err, size_t errlen)
{
    cJSON *root, *wifi;
    char *buf;
    size_t bz;

    (void)snprintf(err, errlen, "config error");
    if (!out || !path) {
        (void)snprintf(err, errlen, "invalid args");
        return -1;
    }
    memset(out, 0, sizeof(*out));

    if (access(path, R_OK) != 0) {
        (void)snprintf(err, errlen, "cannot read %s", path);
        return -1;
    }
    buf = nm4_read_file(path, &bz);
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
    if (!wifi || !cJSON_IsObject(wifi) || !nm4_json_checked(wifi)) {
        cJSON_Delete(root);
        (void)snprintf(err, errlen, "wifiSta not enabled in %s", path);
        return -1;
    }
    nm4_json_str(wifi, "ssid", out->ssid, sizeof(out->ssid));
    nm4_json_str(wifi, "psk", out->psk, sizeof(out->psk));
    if (out->ssid[0] == '\0') {
        cJSON_Delete(root);
        (void)snprintf(err, errlen, "wifiSta ssid empty in %s", path);
        return -1;
    }
    (void)snprintf(out->ifname, sizeof(out->ifname), "wlan0");
    out->wpa_conf[0] = '\0';
    cJSON_Delete(root);
    err[0] = '\0';
    return 0;
}

int nm4_config_load_ppp_from_network_json(const char *path, nm4_ppp_params_t *out, char *err, size_t errlen)
{
    cJSON *root, *ppp;
    char *buf;
    size_t bz;

    (void)snprintf(err, errlen, "config error");
    if (!out || !path) {
        (void)snprintf(err, errlen, "invalid args");
        return -1;
    }
    memset(out, 0, sizeof(*out));
    (void)snprintf(out->dial, sizeof(out->dial), "%s", NM4_DEFAULT_PPP_DIAL);

    if (access(path, R_OK) != 0) {
        (void)snprintf(err, errlen, "cannot read %s", path);
        return -1;
    }
    buf = nm4_read_file(path, &bz);
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
    ppp = cJSON_GetObjectItem(root, "ppp");
    if (!ppp || !cJSON_IsObject(ppp) || !nm4_json_checked(ppp)) {
        cJSON_Delete(root);
        (void)snprintf(err, errlen, "ppp not enabled in %s", path);
        return -1;
    }
    nm4_json_str(ppp, "apn", out->apn, sizeof(out->apn));
    nm4_json_str(ppp, "user", out->user, sizeof(out->user));
    nm4_json_str(ppp, "passwd", out->password, sizeof(out->password));
    (void)snprintf(out->device, sizeof(out->device), "%s", NM4_DEFAULT_PPP_DEV);
    (void)snprintf(out->ifname, sizeof(out->ifname), "ppp0");
    cJSON_Delete(root);
    err[0] = '\0';
    return 0;
}

int nm4_config_list_enabled_ifaces_network(
    const char *path, char ifnames[NM4_NETLIST_MAX][NM4_IFNAME_MAX], int *nout, char *err, size_t errlen)
{
    cJSON *root, *eth, *wifiSta, *ppp;
    char *buf;
    size_t bz;
    int c = 0;

    (void)snprintf(err, errlen, "config error");
    if (!ifnames || !nout || !path) {
        (void)snprintf(err, errlen, "invalid args");
        return -1;
    }
    *nout = 0;
    if (access(path, R_OK) != 0) {
        (void)snprintf(err, errlen, "cannot read %s", path);
        return -1;
    }
    buf = nm4_read_file(path, &bz);
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
    eth = cJSON_GetObjectItem(root, "eth0");
    if (eth && cJSON_IsObject(eth) && nm4_json_checked(eth)) {
        (void)snprintf(ifnames[c], NM4_IFNAME_MAX, "eth0");
        c++;
    }
    wifiSta = cJSON_GetObjectItem(root, "wifiSta");
    if (wifiSta && cJSON_IsObject(wifiSta) && nm4_json_checked(wifiSta) && c < NM4_NETLIST_MAX) {
        (void)snprintf(ifnames[c], NM4_IFNAME_MAX, "wlan0");
        c++;
    }
    ppp = cJSON_GetObjectItem(root, "ppp");
    if (ppp && cJSON_IsObject(ppp) && nm4_json_checked(ppp) && c < NM4_NETLIST_MAX) {
        (void)snprintf(ifnames[c], NM4_IFNAME_MAX, "ppp0");
        c++;
    }
    cJSON_Delete(root);
    *nout = c;
    if (c == 0) {
        (void)snprintf(err, errlen, "no enabled interface in %s", path);
        return -1;
    }
    err[0] = '\0';
    return 0;
}

void nm4_config_fill_probe_params(nm4_probe_params_t *out)
{
    if (!out)
        return;

    nm4_probe_params_default(out);
    out->auto_gateway = 0;
    out->timeout_ms = NM4_PROBE_TIMEOUT_MS;
    out->target_count = 0;

    /* 业务地址优先：转发/云端配置（等同原 remote_host 路径 0） */
    nm4_scan_forward_hosts(out, NM4_FORWARD_DIR, 0);
    nm4_scan_forward_hosts(out, NM4_FORWARD_APP_DIR, 1);

    /* 公网兜底（写死） */
    nm4_probe_add_target(out, "8.8.8.8");
    nm4_probe_add_target(out, "223.5.5.5");
}

/*********************************************************************************************************
** Function name:       nm4_config_write_eth_runtime_cache
** Descriptions:        Write eth0 gateway/dns from network.json into /var/run/netmgr cache files.
** input parameters:    path    - network.json path.
** output parameters:   NONE
** Returned value:      0 on success, -1 on missing file or parse error.
*********************************************************************************************************/
int nm4_config_write_eth_runtime_cache(const char *path)
{
    cJSON *root, *eth;
    char *buf;
    size_t bz;
    const char *gw, *dns1, *dns2;
    const char *dns_list[2];
    size_t dns_count = 0;

    if (!path || path[0] == '\0')
        return -1;
    if (access(path, R_OK) != 0)
        return -1;

    buf = nm4_read_file(path, &bz);
    if (!buf)
        return -1;
    root = cJSON_Parse(buf);
    free(buf);
    if (!root)
        return -1;

    eth = cJSON_GetObjectItemCaseSensitive(root, "eth0");
    if (!eth || !cJSON_IsObject(eth)) {
        cJSON_Delete(root);
        return -1;
    }

    if (nm4_run_dir_ensure() != 0) {
        cJSON_Delete(root);
        return -1;
    }

    gw = NULL;
    {
        cJSON *g = cJSON_GetObjectItemCaseSensitive(eth, "gateway");
        if (cJSON_IsString(g) && g->valuestring && g->valuestring[0])
            gw = g->valuestring;
    }
    (void)nm4_gw_cache_write("eth0", gw);

    dns1 = NULL;
    dns2 = NULL;
    {
        cJSON *d1 = cJSON_GetObjectItemCaseSensitive(eth, "dns1");
        cJSON *d2 = cJSON_GetObjectItemCaseSensitive(eth, "dns2");
        if (cJSON_IsString(d1) && d1->valuestring && d1->valuestring[0])
            dns1 = d1->valuestring;
        if (cJSON_IsString(d2) && d2->valuestring && d2->valuestring[0])
            dns2 = d2->valuestring;
    }
    if (dns1)
        dns_list[dns_count++] = dns1;
    if (dns2)
        dns_list[dns_count++] = dns2;
    (void)nm4_dns_cache_write("eth0", dns_list, dns_count);

    cJSON_Delete(root);
    return 0;
}

int nm4_config_load_rule_prior(const char *path, char *prior, size_t n)
{
    cJSON *root, *rule;
    char *buf;
    size_t bz;

    if (!prior || n == 0)
        return -1;
    prior[0] = '\0';
    if (access(path, R_OK) != 0)
        return -1;
    buf = nm4_read_file(path, &bz);
    if (!buf)
        return -1;
    root = cJSON_Parse(buf);
    free(buf);
    if (!root)
        return -1;
    rule = cJSON_GetObjectItem(root, "rule");
    if (rule && cJSON_IsObject(rule))
        nm4_json_str(rule, "prior", prior, n);
    cJSON_Delete(root);
    return prior[0] ? 0 : -1;
}
