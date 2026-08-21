/* netmgrd_v4 — 适配器：WiFi / PPP，可单独测与将来守护集成 */
#ifndef NETMGR_V4_ADAPTER_H
#define NETMGR_V4_ADAPTER_H

#include <stdbool.h>
#include <stdint.h>

#define NM4_IFNAME_MAX 32
#define NM4_PATH_MAX   256

typedef struct {
    char ifname[NM4_IFNAME_MAX];
    /** 非空则直接用该文件，忽略 ssid/psk */
    char wpa_conf[NM4_PATH_MAX];
    /** 与 psk 同时非空时写 /tmp/wpa_<if>.conf，不调用 wpa_passphrase；psk 为 8–63 字节 WPA2 口令（ASCII） */
    char ssid[64];
    char psk[64];
} nm4_wpa_params_t;

typedef struct {
    char ifname[NM4_IFNAME_MAX];  /* 逻辑名 = linkname，如 ppp0 */
    char device[NM4_PATH_MAX];    /* 串口，如 /dev/ttyUSB0 */
    char apn[64];
    char user[32];
    char password[32];
    char dial[32];                /* 默认 *99# */
} nm4_ppp_params_t;

/*********************************************************************************************************
** Function name:       nm4_adapter_wpa_start
** Descriptions:        Idempotent WiFi STA start (wpa_supplicant + udhcpc); see src/adapter_wpa.c.
** input parameters:    p       - WPA parameters.
** output parameters:   NONE
** Returned value:      0 on success, -1 on failure.
*********************************************************************************************************/
int nm4_adapter_wpa_start(const nm4_wpa_params_t *p);

/*********************************************************************************************************
** Function name:       nm4_adapter_wpa_stop
** Descriptions:        Stop udhcpc, wpa_supplicant, and ifconfig down; see src/adapter_wpa.c.
** input parameters:    ifname  - WiFi interface name.
** output parameters:   NONE
** Returned value:      0 after best-effort stop.
*********************************************************************************************************/
int nm4_adapter_wpa_stop(const char *ifname);

/*********************************************************************************************************
** Function name:       nm4_adapter_ppp_start
** Descriptions:        Idempotent PPP start (peer/chat + pppd); see src/adapter_ppp.c.
** input parameters:    p       - PPP parameters.
** output parameters:   NONE
** Returned value:      0 on success or already running, -1 on failure.
*********************************************************************************************************/
int nm4_adapter_ppp_start(const nm4_ppp_params_t *p);

/*********************************************************************************************************
** Function name:       nm4_adapter_ppp_stop
** Descriptions:        Kill pppd for linkname; see src/adapter_ppp.c.
** input parameters:    ifname  - PPP interface name.
** output parameters:   NONE
** Returned value:      Always 0.
*********************************************************************************************************/
int nm4_adapter_ppp_stop(const char *ifname);

/*********************************************************************************************************
** Function name:       nm4_adapter_wpa_change_mac
** Descriptions:        Change WiFi MAC address to bypass router MAC table stuck issue.
**                      Stops WiFi, changes MAC, brings interface back up.
** input parameters:    ifname     - WiFi interface name (e.g., "wlan0").
**                      mac_suffix - MAC suffix byte (0x01-0x0F) to use for last byte.
** output parameters:   NONE
** Returned value:      0 on success, -1 on failure.
*********************************************************************************************************/
int nm4_adapter_wpa_change_mac(const char *ifname, uint8_t mac_suffix);

/*********************************************************************************************************
** Function name:       nm4_adapter_wpa_read_original_mac
** Descriptions:        Read original MAC address of interface and return last byte suffix.
** input parameters:    ifname - WiFi interface name.
** output parameters:   NONE
** Returned value:      Last byte value (0x01-0xFF) on success, 0 on failure.
*********************************************************************************************************/
uint8_t nm4_adapter_wpa_read_original_mac(const char *ifname);

#endif
