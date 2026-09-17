/*************************************************************************/
/** adapter_wpa.c — wpa_supplicant + udhcpc，幂等启动                            **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include "netmgr_v4_adapter.h"
#include "adapter_internal.h"

#define NM4_UDHCPC_SCRIPT "/ech/script/udhcpc.netmgrd"
#define NM4_WPA_CTRL_DIR  "/var/run/wpa_supplicant"
#define NM4_WPA_STOP_WAIT_MS     1500
#define NM4_WPA_STOP_SLICE_MS    50
#define NM4_UDHCPC_RELEASE_MS    300

static char s_applied_ssid[64];
static char s_applied_psk[64];
static char s_applied_conf[NM4_PATH_MAX];

/*********************************************************************************************************
** Function name:       wpa_clear_applied
** Descriptions:        Clear cached WiFi credentials used for idempotent start matching.
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void wpa_clear_applied(void)
{
    s_applied_ssid[0] = '\0';
    s_applied_psk[0] = '\0';
    s_applied_conf[0] = '\0';
}

/*********************************************************************************************************
** Function name:       wpa_mark_applied
** Descriptions:        Remember WiFi params after a successful wpa_supplicant start.
** input parameters:    p       - WPA parameters that were applied.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void wpa_mark_applied(const nm4_wpa_params_t *p)
{
    if (!p)
        return;
    if (p->wpa_conf[0]) {
        (void)snprintf(s_applied_conf, sizeof(s_applied_conf), "%s", p->wpa_conf);
        s_applied_ssid[0] = '\0';
        s_applied_psk[0] = '\0';
    } else {
        (void)snprintf(s_applied_ssid, sizeof(s_applied_ssid), "%s", p->ssid);
        (void)snprintf(s_applied_psk, sizeof(s_applied_psk), "%s", p->psk);
        s_applied_conf[0] = '\0';
    }
}

/*********************************************************************************************************
** Function name:       wpa_params_match_applied
** Descriptions:        Return whether running wpa_supplicant was started with same ssid/psk/conf.
** input parameters:    p       - desired WPA parameters.
** output parameters:   NONE
** Returned value:      1 if matches cached applied config, else 0.
*********************************************************************************************************/
static int wpa_params_match_applied(const nm4_wpa_params_t *p)
{
    if (!p)
        return 0;
    if (p->wpa_conf[0]) {
        if (!s_applied_conf[0])
            return 0;
        return strcmp(p->wpa_conf, s_applied_conf) == 0;
    }
    if (!p->ssid[0] || !p->psk[0])
        return 0;
    if (!s_applied_ssid[0])
        return 0;
    return strcmp(p->ssid, s_applied_ssid) == 0 &&
           strcmp(p->psk, s_applied_psk) == 0;
}

/*********************************************************************************************************
** Function name:       nm4_fputc_escaped
** Descriptions:        Write string to FILE escaping backslash and double-quote for wpa_supplicant conf.
** input parameters:    fp      - output stream.
**                      s       - null-terminated string (may be NULL).
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void nm4_fputc_escaped(FILE *fp, const char *s)
{
    for (; s && *s; s++) {
        if (*s == '\\' || *s == '"')
            (void)fputc('\\', fp);
        (void)fputc((unsigned char)*s, fp);
    }
}

/*********************************************************************************************************
** Function name:       nm4_write_wpa_psk_conf
** Descriptions:        Write a minimal wpa_supplicant conf (ctrl_interface + network/PSK) to path.
** input parameters:    path    - output file path.
**                      ssid    - WiFi SSID.
**                      psk     - WiFi pre-shared key (ASCII).
** output parameters:   NONE
** Returned value:      0 on success, -1 on I/O error.
*********************************************************************************************************/
static int nm4_write_wpa_psk_conf(const char *path, const char *ssid, const char *psk)
{
    FILE *fp = fopen(path, "w");
    if (!fp) {
        perror("adapter_wpa: fopen conf");
        return -1;
    }
    /* ctrl_interface 必须写上，否则 wpa_cli / 外部工具连不上（ctrl_ifname=nil） */
    (void)fprintf(fp, "ctrl_interface=%s\nupdate_config=1\n\n", NM4_WPA_CTRL_DIR);
    (void)fprintf(fp, "network={\n\tssid=\"");
    nm4_fputc_escaped(fp, ssid);
    (void)fprintf(fp, "\"\n\tpsk=\"");
    nm4_fputc_escaped(fp, psk);
    (void)fprintf(fp, "\"\n}\n");
    if (fclose(fp) != 0) {
        perror("adapter_wpa: fclose conf");
        return -1;
    }
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_if_is_up
** Descriptions:        Query whether the named interface has IFF_UP set via SIOCGIFFLAGS.
** input parameters:    ifname  - interface name.
** output parameters:   NONE
** Returned value:      1 if UP, 0 if down or on error.
*********************************************************************************************************/
static int nm4_if_is_up(const char *ifname)
{
    int fd;
    struct ifreq ifr;
    int is_up = 0;

    if (!ifname || !ifname[0])
        return 0;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return 0;

    memset(&ifr, 0, sizeof(ifr));
    (void)snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0)
        is_up = ((ifr.ifr_flags & IFF_UP) != 0);
    (void)close(fd);
    return is_up;
}

/*********************************************************************************************************
** Function name:       wpa_wait_pidfile_gone
** Descriptions:        Poll until pidfile process is dead or timeout.
** input parameters:    pidf       - pidfile path.
**                      timeout_ms - maximum wait.
** output parameters:   NONE
** Returned value:      1 if gone, 0 if still alive after timeout.
*********************************************************************************************************/
static int wpa_wait_pidfile_gone(const char *pidf, unsigned timeout_ms)
{
    unsigned waited = 0;

    if (!pidf || !pidf[0])
        return 1;
    while (waited < timeout_ms) {
        if (!nm4_pidfile_alive(pidf))
            return 1;
        usleep(NM4_WPA_STOP_SLICE_MS * 1000U);
        waited += NM4_WPA_STOP_SLICE_MS;
    }
    return nm4_pidfile_alive(pidf) ? 0 : 1;
}

/*********************************************************************************************************
** Function name:       wpa_kill_pidfile
** Descriptions:        SIGTERM pidfile process, wait, then SIGKILL if still alive.
** input parameters:    pidf - pidfile path.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void wpa_kill_pidfile(const char *pidf)
{
    char cmd[256];

    if (!pidf || !pidf[0] || !nm4_pidfile_alive(pidf))
        return;

    (void)snprintf(cmd, sizeof(cmd), "kill $(cat %s 2>/dev/null) 2>/dev/null", pidf);
    (void)nm4_run_cmd(cmd);
    if (wpa_wait_pidfile_gone(pidf, NM4_WPA_STOP_WAIT_MS))
        return;
    (void)snprintf(cmd, sizeof(cmd), "kill -9 $(cat %s 2>/dev/null) 2>/dev/null", pidf);
    (void)nm4_run_cmd(cmd);
    (void)wpa_wait_pidfile_gone(pidf, NM4_WPA_STOP_WAIT_MS);
}

/*********************************************************************************************************
** Function name:       wpa_flush_ipv4
** Descriptions:        Drop leftover IPv4 and netmgr gw/dns cache for ifname.
** input parameters:    ifname - WiFi interface name.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void wpa_flush_ipv4(const char *ifname)
{
    char cmd[256];

    if (!ifname || !ifname[0])
        return;
    (void)snprintf(cmd, sizeof(cmd), "ip addr flush dev %s 2>/dev/null", ifname);
    (void)nm4_run_cmd(cmd);
    (void)snprintf(cmd, sizeof(cmd),
                   "rm -f /var/run/netmgr/gateway.%s /var/run/netmgr/dns.%s 2>/dev/null",
                   ifname, ifname);
    (void)nm4_run_cmd(cmd);
}

/*********************************************************************************************************
** Function name:       nm4_adapter_wpa_start
** Descriptions:        Idempotent WiFi STA start: bring iface up, start wpa_supplicant and udhcpc if needed.
** input parameters:    p       - WPA params (ifname, wpa_conf or ssid/psk).
** output parameters:   NONE
** Returned value:      0 on success, -1 on missing args or command failure.
*********************************************************************************************************/
int nm4_adapter_wpa_start(const nm4_wpa_params_t *p)
{
    char wpa_pidf[160];
    char dhc_pidf[160];
    char cmd[640];
    int wpa_ok, u_ok;
    int did = 0;

    if (!p || p->ifname[0] == '\0') {
        fprintf(stderr, "adapter_wpa: missing ifname\n");
        return -1;
    }

    /* 恢复场景：若外部把网卡 down（如 ifconfig wlan0 down），先拉起接口。 */
    snprintf(cmd, sizeof(cmd), "ifconfig %s up 2>/dev/null", p->ifname);
    (void)nm4_run_cmd(cmd);

    snprintf(wpa_pidf, sizeof(wpa_pidf), "/var/run/wpa_%s.pid", p->ifname);
    snprintf(dhc_pidf, sizeof(dhc_pidf), "/var/run/udhcpc.%s.pid", p->ifname);

    wpa_ok = nm4_pidfile_alive(wpa_pidf);
    u_ok = nm4_pidfile_alive(dhc_pidf);

    if (wpa_ok && u_ok && nm4_if_is_up(p->ifname) && wpa_params_match_applied(p))
        return 0;

    if (wpa_ok && !wpa_params_match_applied(p)) {
        nm4_adapter_wpa_stop(p->ifname);
        wpa_clear_applied();
        wpa_ok = 0;
        u_ok = 0;
        /* stop 会 ifconfig down；拉起后再启 wpa/udhcpc */
        snprintf(cmd, sizeof(cmd), "ifconfig %s up 2>/dev/null", p->ifname);
        (void)nm4_run_cmd(cmd);
    }

    if (!wpa_ok) {
        (void)wpa_wait_pidfile_gone(wpa_pidf, NM4_WPA_STOP_WAIT_MS);
        (void)unlink(wpa_pidf);
    }
    if (!u_ok) {
        (void)wpa_wait_pidfile_gone(dhc_pidf, NM4_WPA_STOP_WAIT_MS);
        (void)unlink(dhc_pidf);
        /* 旧地址残留会被其它软件当成已联网，也会让下次看起来像“没 DHCP” */
        if (nm4_if_has_ipv4(p->ifname))
            wpa_flush_ipv4(p->ifname);
    }

    if (!wpa_ok) {
        /* 确保 ctrl socket 目录存在；-C 覆盖外部 conf 里缺失的 ctrl_interface */
        snprintf(cmd, sizeof(cmd), "mkdir -p %s", NM4_WPA_CTRL_DIR);
        (void)nm4_run_cmd(cmd);

        if (p->wpa_conf[0]) {
            snprintf(cmd, sizeof(cmd),
                     "wpa_supplicant -i %s -c %s -C %s -B -P %s",
                     p->ifname, p->wpa_conf, NM4_WPA_CTRL_DIR, wpa_pidf);
        } else if (p->ssid[0] && p->psk[0]) {
            char confpath[NM4_PATH_MAX];
            snprintf(confpath, sizeof(confpath), "/tmp/wpa_%s.conf", p->ifname);
            if (nm4_write_wpa_psk_conf(confpath, p->ssid, p->psk) != 0)
                return -1;
            snprintf(cmd, sizeof(cmd),
                     "wpa_supplicant -i %s -c %s -C %s -B -P %s",
                     p->ifname, confpath, NM4_WPA_CTRL_DIR, wpa_pidf);
        } else {
            fprintf(stderr,
                    "adapter_wpa: need -f wpa.conf or -s/-k ssid/psk\n");
            return -1;
        }

        if (nm4_run_cmd(cmd) != 0) {
            fprintf(stderr, "adapter_wpa: wpa_supplicant failed\n");
            return -1;
        }
        did = 1;
    }

    if (!u_ok) {
        snprintf(cmd, sizeof(cmd),
                 "udhcpc -i %s -b -p %s -s %s >/dev/null 2>&1 &",
                 p->ifname, dhc_pidf, NM4_UDHCPC_SCRIPT);
        (void)nm4_run_cmd(cmd);
        did = 1;
    }

    if (did || wpa_ok)
        wpa_mark_applied(p);

    (void)did;
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_adapter_wpa_stop
** Descriptions:        Release DHCP, stop udhcpc/wpa, wait until gone, flush IPv4, then ifconfig down.
** input parameters:    ifname  - WiFi interface name.
** output parameters:   NONE
** Returned value:      Always 0 after best-effort kill commands.
*********************************************************************************************************/
int nm4_adapter_wpa_stop(const char *ifname)
{
    char cmd[256];
    char wpa_pidf[160];
    char dhc_pidf[160];

    if (!ifname || !ifname[0]) {
        fprintf(stderr, "adapter_wpa: missing ifname\n");
        return -1;
    }

    snprintf(wpa_pidf, sizeof(wpa_pidf), "/var/run/wpa_%s.pid", ifname);
    snprintf(dhc_pidf, sizeof(dhc_pidf), "/var/run/udhcpc.%s.pid", ifname);

    /* SIGUSR2：busybox udhcpc 释放租约并走 deconfig，避免只杀进程留下地址 */
    if (nm4_pidfile_alive(dhc_pidf)) {
        snprintf(cmd, sizeof(cmd),
                 "kill -USR2 $(cat %s 2>/dev/null) 2>/dev/null", dhc_pidf);
        (void)nm4_run_cmd(cmd);
        usleep(NM4_UDHCPC_RELEASE_MS * 1000U);
    }

    wpa_kill_pidfile(dhc_pidf);
    wpa_kill_pidfile(wpa_pidf);
    (void)unlink(dhc_pidf);
    (void)unlink(wpa_pidf);

    wpa_flush_ipv4(ifname);

    snprintf(cmd, sizeof(cmd), "ifconfig %s down 2>/dev/null", ifname);
    (void)nm4_run_cmd(cmd);

    wpa_clear_applied();
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_adapter_wpa_read_original_mac
** Descriptions:        Read original MAC address of interface and return last byte suffix.
** input parameters:    ifname - WiFi interface name.
** output parameters:   NONE
** Returned value:      Last byte value (0x01-0xFF) on success, 0 on failure.
*********************************************************************************************************/
uint8_t nm4_adapter_wpa_read_original_mac(const char *ifname)
{
    char path[128];
    char mac[32];
    FILE *fp;
    uint8_t suffix = 0;

    if (!ifname || !ifname[0])
        return 0;

    snprintf(path, sizeof(path), "/sys/class/net/%s/address", ifname);
    fp = fopen(path, "r");
    if (!fp)
        return 0;

    if (fscanf(fp, "%17s", mac) == 1) {
        /* Parse last byte from format "AA:BB:CC:DD:EE:01" */
        if (strlen(mac) >= 17) {
            suffix = (uint8_t)strtol(mac + 15, NULL, 16);
        }
    }
    fclose(fp);
    return suffix;
}

/*********************************************************************************************************
** Function name:       nm4_adapter_wpa_change_mac
** Descriptions:        Change WiFi MAC address to bypass router MAC table stuck issue.
**                      Stops WiFi, changes MAC, brings interface back up.
** input parameters:    ifname     - WiFi interface name (e.g., "wlan0").
**                      mac_suffix - MAC suffix byte (0x01-0x0F) to use for last byte.
** output parameters:   NONE
** Returned value:      0 on success, -1 on failure.
*********************************************************************************************************/
int nm4_adapter_wpa_change_mac(const char *ifname, uint8_t mac_suffix)
{
    char cmd[256];
    char new_mac[18];

    if (!ifname || !ifname[0])
        return -1;

    /* Validate suffix range */
    if (mac_suffix == 0 || mac_suffix > 0x0F)
        mac_suffix = 1;

    /* Read current MAC prefix (first 5 bytes) */
    uint8_t orig_suffix = nm4_adapter_wpa_read_original_mac(ifname);
    if (orig_suffix == 0)
        orig_suffix = 0x01;  /* Default if cannot read */

    /* Use fixed prefix AA:BB:CC:DD:EE, only change last byte */
    (void)snprintf(new_mac, sizeof(new_mac), "AA:BB:CC:DD:EE:%02X", mac_suffix);

    /* 1. Stop WiFi services */
    nm4_adapter_wpa_stop(ifname);
    usleep(500000);  /* 500ms settle time */

    /* 2. Bring interface down */
    snprintf(cmd, sizeof(cmd), "ifconfig %s down 2>/dev/null", ifname);
    (void)nm4_run_cmd(cmd);
    usleep(200000);  /* 200ms */

    /* 3. Change MAC address */
    snprintf(cmd, sizeof(cmd), "ip link set %s address %s", ifname, new_mac);
    if (nm4_run_cmd(cmd) != 0) {
        fprintf(stderr, "adapter_wpa: failed to set MAC %s\n", new_mac);
        /* Try to bring interface back up even on failure */
        snprintf(cmd, sizeof(cmd), "ifconfig %s up 2>/dev/null", ifname);
        (void)nm4_run_cmd(cmd);
        return -1;
    }

    /* 4. Bring interface up */
    snprintf(cmd, sizeof(cmd), "ifconfig %s up 2>/dev/null", ifname);
    (void)nm4_run_cmd(cmd);
    usleep(200000);  /* 200ms */

    fprintf(stderr, "adapter_wpa: MAC changed to %s\n", new_mac);
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_adapter_wpa_renew
** Descriptions:        Signal running udhcpc (SIGUSR1) to renew DHCP lease.
** input parameters:    ifname  - WiFi interface name.
** output parameters:   NONE
** Returned value:      0 on success, -1 if udhcpc not running or signal failed.
*********************************************************************************************************/
int nm4_adapter_wpa_renew(const char *ifname)
{
    char pidf[160];
    char cmd[256];

    if (!ifname || !ifname[0])
        return -1;

    snprintf(pidf, sizeof(pidf), "/var/run/udhcpc.%s.pid", ifname);
    if (!nm4_pidfile_alive(pidf))
        return -1;

    snprintf(cmd, sizeof(cmd),
             "kill -USR1 $(cat %s) 2>/dev/null",
             pidf);
    return nm4_run_cmd(cmd) == 0 ? 0 : -1;
}
