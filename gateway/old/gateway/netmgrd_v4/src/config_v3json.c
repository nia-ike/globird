#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include "netmgr_v4_config.h"
#include "cJSON.h"

/*********************************************************************************************************
** Function name:       read_file
** Descriptions:        Read entire file into a malloc'd null-terminated buffer.
** input parameters:    path    - file path.
** output parameters:   out_sz  - set to file size in bytes on success.
** Returned value:      Pointer to buffer or NULL on error; caller must free.
*********************************************************************************************************/
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

/*********************************************************************************************************
** Function name:       cj_enabled
** Descriptions:        Read boolean "enabled" from JSON object; default true if absent.
** input parameters:    obj     - cJSON object.
** output parameters:   NONE
** Returned value:      1 if enabled or missing, 0 if explicitly false.
*********************************************************************************************************/
static int cj_enabled(cJSON *obj)
{
    cJSON *e = cJSON_GetObjectItem(obj, "enabled");
    if (e && cJSON_IsBool(e))
        return cJSON_IsTrue(e) ? 1 : 0;
    return 1;
}

/*********************************************************************************************************
** Function name:       cj_str
** Descriptions:        Copy string field k from JSON object into out buffer, or clear out.
** input parameters:    obj     - cJSON object.
**                      k       - key name.
**                      n       - size of out buffer.
** output parameters:    out     - written string or empty.
** Returned value:      NONE
*********************************************************************************************************/
static void cj_str(cJSON *obj, const char *k, char *out, size_t n)
{
    cJSON *it = cJSON_GetObjectItem(obj, k);
    if (it && cJSON_IsString(it) && it->valuestring)
        snprintf(out, n, "%s", it->valuestring);
    else
        out[0] = '\0';
}

/*********************************************************************************************************
** Function name:       nm4_config_load_wifi_from_gw_conf
** Descriptions:        Load STA WiFi ssid/psk from NM4_GW_CONF_PATH (product override).
** input parameters:    errlen  - size of err buffer.
** output parameters:    out     - filled nm4_wpa_params_t (ifname wlan0, wpa_conf cleared).
**                      err     - error message on failure.
** Returned value:      0 on success, -1 on missing/invalid config.
*********************************************************************************************************/
static int nm4_config_load_wifi_from_gw_conf(nm4_wpa_params_t *out, char *err, size_t errlen)
{
    cJSON *root, *network, *wlan0;
    char *buf;
    size_t bz;
    cJSON *mode;
    cJSON *ssid;
    cJSON *psk;

    if (!out) return -1;
    if (access(NM4_GW_CONF_PATH, R_OK) != 0) {
        (void)snprintf(err, errlen, "cannot read %s", NM4_GW_CONF_PATH);
        return -1;
    }
    buf = read_file(NM4_GW_CONF_PATH, &bz);
    if (!buf) {
        (void)snprintf(err, errlen, "empty or unreadable %s", NM4_GW_CONF_PATH);
        return -1;
    }

    root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        (void)snprintf(err, errlen, "JSON parse error in %s", NM4_GW_CONF_PATH);
        return -1;
    }
    network = cJSON_GetObjectItem(root, "network");
    wlan0 = network ? cJSON_GetObjectItem(network, "wlan0") : NULL;
    if (!wlan0 || !cJSON_IsObject(wlan0)) {
        cJSON_Delete(root);
        (void)snprintf(err, errlen, "missing network.wlan0 in %s", NM4_GW_CONF_PATH);
        return -1;
    }

    mode = cJSON_GetObjectItem(wlan0, "mode");
    if (mode && cJSON_IsString(mode) && mode->valuestring &&
        strcmp(mode->valuestring, "sta") != 0) {
        cJSON_Delete(root);
        (void)snprintf(err, errlen, "network.wlan0 mode is not sta in %s", NM4_GW_CONF_PATH);
        return -1;
    }

    ssid = cJSON_GetObjectItem(wlan0, "ap_ssid");
    psk = cJSON_GetObjectItem(wlan0, "ap_psk");
    if (!ssid || !cJSON_IsString(ssid) || !ssid->valuestring ||
        !psk || !cJSON_IsString(psk) || !psk->valuestring) {
        cJSON_Delete(root);
        (void)snprintf(err, errlen, "missing ap_ssid/ap_psk in %s", NM4_GW_CONF_PATH);
        return -1;
    }

    (void)snprintf(out->ifname, sizeof(out->ifname), "wlan0");
    (void)snprintf(out->ssid, sizeof(out->ssid), "%s", ssid->valuestring);
    (void)snprintf(out->psk, sizeof(out->psk), "%s", psk->valuestring);
    out->wpa_conf[0] = '\0';
    cJSON_Delete(root);
    err[0] = '\0';
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_config_load_wifi_from_v3_json
** Descriptions:        Load first enabled wifi_sta from v3-style JSON, or from gw_conf if present.
** input parameters:    path    - netmgr v3 JSON path.
**                      errlen  - size of err buffer.
** output parameters:    out     - filled WPA params.
**                      err     - error message on failure.
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_config_load_wifi_from_v3_json(const char *path, nm4_wpa_params_t *out, char *err, size_t errlen)
{
    cJSON *root, *arr, *jif, *conn, *t;
    char *buf;
    size_t bz;
    int i, n, found = 0;

    (void)snprintf(err, errlen, "config error");
    if (!out || !path) {
        (void)snprintf(err, errlen, "invalid args");
        return -1;
    }
    memset(out, 0, sizeof(*out));

    /*
     * 产品定制：wlan0 的 ssid/psk 优先从 gw_conf.json 读取；
     * 仅当该配置不可用时再回退到 netmgr_v3.json。
     */
    if (nm4_config_load_wifi_from_gw_conf(out, err, errlen) == 0)
        return 0;

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
    arr = cJSON_GetObjectItem(root, "interfaces");
    if (!arr || !cJSON_IsArray(arr)) {
        cJSON_Delete(root);
        (void)snprintf(err, errlen, "missing interfaces[] in %s", path);
        return -1;
    }
    n = cJSON_GetArraySize(arr);
    for (i = 0; i < n; i++) {
        jif = cJSON_GetArrayItem(arr, i);
        if (!jif) continue;
        t = cJSON_GetObjectItem(jif, "type");
        if (!t || !cJSON_IsString(t) || !t->valuestring) continue;
        if (strcmp(t->valuestring, "wifi_sta") != 0) continue;
        if (!cj_enabled(jif)) continue;
        cj_str(jif, "ifname", out->ifname, sizeof(out->ifname));
        conn = cJSON_GetObjectItem(jif, "conn");
        if (conn) {
            cj_str(conn, "ssid", out->ssid, sizeof(out->ssid));
            cj_str(conn, "psk", out->psk, sizeof(out->psk));
            cj_str(conn, "wpa_conf", out->wpa_conf, sizeof(out->wpa_conf));
        }
        if (out->ifname[0] == '\0')
            (void)snprintf(out->ifname, sizeof(out->ifname), "wlan0");
        found = 1;
        break;
    }
    cJSON_Delete(root);
    if (!found) {
        (void)snprintf(err, errlen, "no enabled wifi_sta in %s", path);
        return -1;
    }
    err[0] = '\0';
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_config_list_enabled_ifaces
** Descriptions:        Collect ifname of all enabled interfaces from v3 JSON (with type defaults).
** input parameters:    path    - JSON path.
**                      errlen  - err buffer size.
** output parameters:    ifnames - array of interface names written.
**                      nout    - count of entries written.
**                      err     - error message on failure.
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_config_list_enabled_ifaces(
    const char *path, char ifnames[NM4_NETLIST_MAX][NM4_IFNAME_MAX], int *nout, char *err, size_t errlen)
{
    cJSON *root, *arr, *jif;
    char *buf;
    size_t bz;
    int i, n, c;

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
    arr = cJSON_GetObjectItem(root, "interfaces");
    if (!arr || !cJSON_IsArray(arr)) {
        cJSON_Delete(root);
        (void)snprintf(err, errlen, "missing interfaces[] in %s", path);
        return -1;
    }
    n = cJSON_GetArraySize(arr);
    c = 0;
    for (i = 0; i < n; i++) {
        jif = cJSON_GetArrayItem(arr, i);
        if (!jif) continue;
        if (!cj_enabled(jif)) continue;
        cj_str(jif, "ifname", ifnames[c], (size_t)NM4_IFNAME_MAX);
        if (ifnames[c][0] == '\0') {
            cJSON *t = cJSON_GetObjectItem(jif, "type");
            if (t && cJSON_IsString(t) && t->valuestring) {
                if (strcmp(t->valuestring, "ethernet") == 0)
                    (void)snprintf(ifnames[c], NM4_IFNAME_MAX, "eth0");
                else if (strcmp(t->valuestring, "wifi_sta") == 0)
                    (void)snprintf(ifnames[c], NM4_IFNAME_MAX, "wlan0");
                else if (strcmp(t->valuestring, "ppp") == 0)
                    (void)snprintf(ifnames[c], NM4_IFNAME_MAX, "ppp0");
            }
        }
        if (ifnames[c][0] == '\0') continue;
        c++;
        if (c >= NM4_NETLIST_MAX) break;
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

/*********************************************************************************************************
** Function name:       nm4_config_load_ppp_from_v3_json
** Descriptions:        Load first enabled ppp interface from v3-style JSON into nm4_ppp_params_t.
** input parameters:    path    - JSON path.
**                      errlen  - err buffer size.
** output parameters:    out     - filled PPP params (default dial *99#).
**                      err     - error message on failure.
** Returned value:      0 on success, -1 on error.
*********************************************************************************************************/
int nm4_config_load_ppp_from_v3_json(const char *path, nm4_ppp_params_t *out, char *err, size_t errlen)
{
    cJSON *root, *arr, *jif, *conn, *t;
    char *buf;
    size_t bz;
    int i, n, found = 0;

    (void)snprintf(err, errlen, "config error");
    if (!out || !path) {
        (void)snprintf(err, errlen, "invalid args");
        return -1;
    }
    memset(out, 0, sizeof(*out));
    (void)snprintf(out->dial, sizeof(out->dial), "*99#");

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
    arr = cJSON_GetObjectItem(root, "interfaces");
    if (!arr || !cJSON_IsArray(arr)) {
        cJSON_Delete(root);
        (void)snprintf(err, errlen, "missing interfaces[] in %s", path);
        return -1;
    }
    n = cJSON_GetArraySize(arr);
    for (i = 0; i < n; i++) {
        jif = cJSON_GetArrayItem(arr, i);
        if (!jif) continue;
        t = cJSON_GetObjectItem(jif, "type");
        if (!t || !cJSON_IsString(t) || !t->valuestring) continue;
        if (strcmp(t->valuestring, "ppp") != 0) continue;
        if (!cj_enabled(jif)) continue;
        cj_str(jif, "ifname", out->ifname, sizeof(out->ifname));
        conn = cJSON_GetObjectItem(jif, "conn");
        if (conn) {
            cj_str(conn, "device", out->device, sizeof(out->device));
            cj_str(conn, "apn", out->apn, sizeof(out->apn));
            cj_str(conn, "user", out->user, sizeof(out->user));
            cj_str(conn, "password", out->password, sizeof(out->password));
            cj_str(conn, "dial_number", out->dial, sizeof(out->dial));
        }
        if (out->ifname[0] == '\0')
            (void)snprintf(out->ifname, sizeof(out->ifname), "ppp0");
        if (out->dial[0] == '\0')
            (void)snprintf(out->dial, sizeof(out->dial), "*99#");
        found = 1;
        break;
    }
    cJSON_Delete(root);
    if (!found) {
        (void)snprintf(err, errlen, "no enabled ppp in %s", path);
        return -1;
    }
    err[0] = '\0';
    return 0;
}
