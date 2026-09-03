/*************************************************************************/
/** wifi_usb_watch.c — AIC8800 USB 在位看门狗：缺 WiFi 且 4G 仍在则整机重启   **/
/*************************************************************************/
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "netmgr_v4.h"
#include "netmgr_v4_wifi_usb_watch.h"
#include "netmgr_v4_reboot.h"

/* AICSemi AIC 8800D80 */
#define NM4_WIFI_USB_VID        "a69c"
#define NM4_WIFI_USB_PID        "8d81"
/* Quectel EG800AK-EU */
#define NM4_LTE_USB_VID         "2c7c"
#define NM4_LTE_USB_PID         "6002"

#define NM4_WIFI_USB_CHECK_MS           60000ULL
#define NM4_WIFI_USB_MISS_THRESHOLD     3
#define NM4_WIFI_USB_MIN_UPTIME_SEC     300U   /* 开机保护，避开枚举窗口 */

/*********************************************************************************************************
** Function name:       wifi_usb_boot_uptime_sec
** Descriptions:        Return seconds since boot (BOOTTIME preferred, else MONOTONIC).
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      Uptime in seconds, or 0 on failure.
*********************************************************************************************************/
static unsigned wifi_usb_boot_uptime_sec(void)
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
** Function name:       wifi_usb_read_id_file
** Descriptions:        Read a short sysfs id file (idVendor/idProduct) into buf (trimmed).
** input parameters:    path    - full file path.
**                      buf     - output buffer.
**                      buflen  - size of buf.
** output parameters:   buf     - null-terminated id string when successful.
** Returned value:      0 on success, -1 on failure.
*********************************************************************************************************/
static int wifi_usb_read_id_file(const char *path, char *buf, size_t buflen)
{
    FILE *fp;
    size_t n;

    if (!path || !buf || buflen < 2)
        return -1;
    fp = fopen(path, "r");
    if (!fp)
        return -1;
    if (!fgets(buf, (int)buflen, fp)) {
        (void)fclose(fp);
        return -1;
    }
    (void)fclose(fp);
    n = strlen(buf);
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == ' ')) {
        buf[--n] = '\0';
    }
    return (n > 0) ? 0 : -1;
}

/*********************************************************************************************************
** Function name:       wifi_usb_sysfs_has_vid_pid
** Descriptions:        Scan /sys/bus/usb/devices for a device matching vendor/product hex ids.
** input parameters:    vid     - USB idVendor string (lowercase hex, e.g. "a69c").
**                      pid     - USB idProduct string (lowercase hex, e.g. "8d81").
** output parameters:   NONE
** Returned value:      1 if found, 0 if not found.
*********************************************************************************************************/
static int wifi_usb_sysfs_has_vid_pid(const char *vid, const char *pid)
{
    DIR *dir;
    struct dirent *ent;
    char path[512];
    char vbuf[16];
    char pbuf[16];
    int found = 0;

    if (!vid || !pid)
        return 0;

    dir = opendir("/sys/bus/usb/devices");
    if (!dir)
        return 0;

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.')
            continue;
        if (strlen(ent->d_name) > 64)
            continue;
        (void)snprintf(path, sizeof(path), "/sys/bus/usb/devices/%s/idVendor", ent->d_name);
        if (wifi_usb_read_id_file(path, vbuf, sizeof(vbuf)) != 0)
            continue;
        (void)snprintf(path, sizeof(path), "/sys/bus/usb/devices/%s/idProduct", ent->d_name);
        if (wifi_usb_read_id_file(path, pbuf, sizeof(pbuf)) != 0)
            continue;
        if (strcasecmp(vbuf, vid) == 0 && strcasecmp(pbuf, pid) == 0) {
            found = 1;
            break;
        }
    }

    (void)closedir(dir);
    return found;
}

/*********************************************************************************************************
** Function name:       nm4_wifi_usb_watch_tick
** Descriptions:        Periodic AIC8800 USB presence check. If WiFi USB is missing while Quectel
**                      LTE USB is still present (RF board attached), accumulate failures and request
**                      whole-system reboot after N misses. If both WiFi and LTE USB are absent,
**                      treat as RF board removed for debug and skip fast reboot (night reboot remains).
** input parameters:    now_ms  - clock ms used for interval gating (same as manager loop).
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_wifi_usb_watch_tick(uint64_t now_ms)
{
    static uint64_t last_check_ms;
    static int miss_streak;
    static int reboot_posted;
    int wifi_ok;
    int lte_ok;

    if (reboot_posted)
        return;

    if (last_check_ms != 0 && (now_ms - last_check_ms) < NM4_WIFI_USB_CHECK_MS)
        return;
    last_check_ms = now_ms;

    if (wifi_usb_boot_uptime_sec() < NM4_WIFI_USB_MIN_UPTIME_SEC) {
        miss_streak = 0;
        return;
    }

    wifi_ok = wifi_usb_sysfs_has_vid_pid(NM4_WIFI_USB_VID, NM4_WIFI_USB_PID);
    lte_ok = wifi_usb_sysfs_has_vid_pid(NM4_LTE_USB_VID, NM4_LTE_USB_PID);

    if (wifi_ok) {
        if (miss_streak != 0)
            netmgr_v4_log("wifi_usb_watch: AIC8800 USB present again, clear streak");
        miss_streak = 0;
        return;
    }

    /* WiFi gone + LTE gone => RF board likely removed; do not fast-reboot */
    if (!lte_ok) {
        if (miss_streak != 0)
            netmgr_v4_log("wifi_usb_watch: WiFi+LTE USB absent, skip fast reboot (rf board?)");
        miss_streak = 0;
        return;
    }

    /* WiFi gone but LTE present => RF board attached, WiFi/USB host abnormal */
    miss_streak++;
    netmgr_v4_log("wifi_usb_watch: AIC8800 USB missing (lte present), streak=%d/%d",
                  miss_streak, NM4_WIFI_USB_MISS_THRESHOLD);

    if (miss_streak < NM4_WIFI_USB_MISS_THRESHOLD)
        return;

    if (nm4_request_system_reboot("wifi usb missing with lte present") == 0) {
        reboot_posted = 1;
        netmgr_v4_log("wifi_usb_watch: AIC8800 USB missing x%d with LTE present, db_process save+reboot posted",
                      NM4_WIFI_USB_MISS_THRESHOLD);
    } else {
        /* Fallback reboot invoked; if it failed, retry on next interval */
        miss_streak = NM4_WIFI_USB_MISS_THRESHOLD;
    }
}
