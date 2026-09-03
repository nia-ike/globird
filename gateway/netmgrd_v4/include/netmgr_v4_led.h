#ifndef NETMGR_V4_LED_H
#define NETMGR_V4_LED_H

#include "netmgr_v4_policy.h"
#include "netmgr_v4_probe.h"

typedef struct {
    const nm4_probe_result_t *wlan;
    const nm4_probe_result_t *ppp;
    const nm4_bits_t *bits;
} nm4_led_input_t;

/*********************************************************************************************************
** Function name:       nm4_led_update
** Descriptions:        Update wifi/lte LEDs: blink = link+IP, solid = debounced internet_ok.
** input parameters:    in      - wlan/ppp probe snap + availability bits.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_led_update(const nm4_led_input_t *in);

/*********************************************************************************************************
** Function name:       nm4_led_all_off
** Descriptions:        Turn off wifi and lte LEDs.
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_led_all_off(void);

#endif
