#ifndef NETMGR_V4_WIFI_USB_WATCH_H
#define NETMGR_V4_WIFI_USB_WATCH_H

#include <stdint.h>

/*********************************************************************************************************
** Function name:       nm4_wifi_usb_watch_tick
** Descriptions:        Periodic AIC8800 USB presence check. If WiFi USB is missing while Quectel
**                      LTE USB is still present (RF board attached), accumulate failures and request
**                      whole-system reboot after N misses. If both WiFi and LTE USB are absent,
**                      treat as RF board removed for debug and skip fast reboot (night reboot remains).
** input parameters:    now_ms  - monotonic/wall ms used for interval gating (same clock as manager).
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_wifi_usb_watch_tick(uint64_t now_ms);

#endif
