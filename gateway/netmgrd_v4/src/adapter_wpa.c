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
** Descriptions:        Write a minimal wpa_supplicant network block with plaintext PSK to path.
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

    if (wpa_ok && u_ok && nm4_if_is_up(p->ifname))
        return 0;

    if (!wpa_ok)
        (void)unlink(wpa_pidf);
    if (!u_ok)
        (void)unlink(dhc_pidf);

    if (!wpa_ok) {
        if (p->wpa_conf[0]) {
            snprintf(cmd, sizeof(cmd),
                     "wpa_supplicant -i %s -c %s -B -P %s",
                     p->ifname, p->wpa_conf, wpa_pidf);
        } else if (p->ssid[0] && p->psk[0]) {
            char confpath[NM4_PATH_MAX];
            snprintf(confpath, sizeof(confpath), "/tmp/wpa_%s.conf", p->ifname);
            if (nm4_write_wpa_psk_conf(confpath, p->ssid, p->psk) != 0)
                return -1;
            snprintf(cmd, sizeof(cmd),
                     "wpa_supplicant -i %s -c %s -B -P %s",
                     p->ifname, confpath, wpa_pidf);
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
                 "udhcpc -i %s -b -p %s >/dev/null 2>&1 &",
                 p->ifname, dhc_pidf);
        (void)nm4_run_cmd(cmd);
        did = 1;
    }

    (void)did;
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_adapter_wpa_stop
** Descriptions:        Stop udhcpc and wpa_supplicant for the interface and bring interface down.
** input parameters:    ifname  - WiFi interface name.
** output parameters:   NONE
** Returned value:      Always 0 after best-effort kill commands.
*********************************************************************************************************/
int nm4_adapter_wpa_stop(const char *ifname)
{
    char cmd[256];

    if (!ifname || !ifname[0]) {
        fprintf(stderr, "adapter_wpa: missing ifname\n");
        return -1;
    }

    snprintf(cmd, sizeof(cmd),
             "kill $(cat /var/run/udhcpc.%s.pid 2>/dev/null) 2>/dev/null",
             ifname);
    (void)nm4_run_cmd(cmd);

    snprintf(cmd, sizeof(cmd),
             "kill $(cat /var/run/wpa_%s.pid 2>/dev/null) 2>/dev/null",
             ifname);
    (void)nm4_run_cmd(cmd);

    snprintf(cmd, sizeof(cmd), "ifconfig %s down 2>/dev/null", ifname);
    (void)nm4_run_cmd(cmd);

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
