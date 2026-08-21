#ifndef NETMGR_V4_LINKKEEPER_H
#define NETMGR_V4_LINKKEEPER_H

#include <stdint.h>
#include "netmgr_v4_adapter.h"

typedef struct {
    int has_wifi;
    int has_ppp;
    nm4_wpa_params_t wpa;
    nm4_ppp_params_t ppp;
    uint64_t last_wifi_keep_ms;
    uint64_t last_ppp_keep_ms;
    int wifi_due;
    int ppp_due;
    uint64_t wifi_next_in_ms;
    uint64_t ppp_next_in_ms;
    int wifi_last_rc;
    int ppp_last_rc;
    uint64_t wifi_ok_cnt;
    uint64_t wifi_fail_cnt;
    uint64_t ppp_ok_cnt;
    uint64_t ppp_fail_cnt;
    uint64_t ppp_modem_recover_cnt;
    uint64_t ppp_last_modem_recover_ms;
    int ppp_wait_tty_after_recover;
    char wifi_last_msg[64];
    char ppp_last_msg[64];
    /* WiFi deadlink detection and MAC change */
    uint64_t wifi_no_ip_since_ms;       /* Timestamp when IP was first lost */
    uint8_t  wifi_mac_suffix;           /* Current MAC suffix (0x01-0x0F) */
    uint32_t wifi_mac_change_cnt;       /* Number of MAC changes performed */
    uint64_t wifi_last_mac_change_ms;   /* Timestamp of last MAC change */
} nm4_linkkeeper_t;

/*********************************************************************************************************
** Function name:       nm4_linkkeeper_init
** Descriptions:        Load WiFi/PPP config into linkkeeper state; see src/linkkeeper.c.
** input parameters:    cfgpath - v3 JSON path.
**                      errlen  - error buffer size.
** output parameters:    lk      - initialized state.
**                      err     - error message on failure.
** Returned value:      0 if at least one of wifi/ppp present, -1 otherwise.
*********************************************************************************************************/
int nm4_linkkeeper_init(nm4_linkkeeper_t *lk, const char *cfgpath, char *err, unsigned errlen);

/*********************************************************************************************************
** Function name:       nm4_linkkeeper_tick
** Descriptions:        One keepalive tick for WiFi/PPP adapters; see src/linkkeeper.c.
** input parameters:    lk      - linkkeeper state.
**                      now_ms  - current time in ms.
** output parameters:    lk      - counters and timestamps updated.
** Returned value:      NONE
*********************************************************************************************************/
void nm4_linkkeeper_tick(nm4_linkkeeper_t *lk, uint64_t now_ms);

/*********************************************************************************************************
** Function name:       nm4_linkkeeper_set_log_enabled
** Descriptions:        Enable or disable verbose linkkeeper logs; see src/linkkeeper.c.
** input parameters:    enabled - non-zero to log.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_linkkeeper_set_log_enabled(int enabled);

/*********************************************************************************************************
** Function name:       nm4_linkkeeper_set_intervals
** Descriptions:        Set global WiFi and PPP keepalive intervals (ms); see src/linkkeeper.c.
** input parameters:    wifi_ms - WiFi period (0 to keep previous).
**                      ppp_ms  - PPP period (0 to keep previous).
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_linkkeeper_set_intervals(uint64_t wifi_ms, uint64_t ppp_ms);

/*********************************************************************************************************
** Function name:       nm4_linkkeeper_get_intervals
** Descriptions:        Read global WiFi and PPP keepalive intervals; see src/linkkeeper.c.
** input parameters:    wifi_ms - optional out pointer.
**                      ppp_ms  - optional out pointer.
** output parameters:    *wifi_ms / *ppp_ms - current values when non-NULL.
** Returned value:      NONE
*********************************************************************************************************/
void nm4_linkkeeper_get_intervals(uint64_t *wifi_ms, uint64_t *ppp_ms);

#endif
