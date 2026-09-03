/*************************************************************************/
/** led.c — wifi / lte 指示灯：闪烁=有 IP，常亮=防抖后 internet_ok 可用               **/
/*************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "netmgr_v4_led.h"
#include "netmgr_v4_probe.h"
#include "adapter_internal.h"

#define NM4_LED_WIFI_PATH   "/sys/class/leds/wifi"
#define NM4_LED_LTE_PATH    "/sys/class/leds/lte"

#define NM4_LED_BLINK_MS    500

typedef enum {
    NM4_LED_MODE_OFF = 0,
    NM4_LED_MODE_BLINK,
    NM4_LED_MODE_SOLID
} nm4_led_mode_t;

typedef struct {
    const char *base;
    int last_mode;
} nm4_led_channel_t;

static nm4_led_channel_t s_ch_wifi = { NM4_LED_WIFI_PATH, -1 };
static nm4_led_channel_t s_ch_lte  = { NM4_LED_LTE_PATH, -1 };

/*********************************************************************************************************
** Function name:       led_path_writable
** Descriptions:        Check whether LED sysfs brightness node exists and is writable.
** input parameters:    base    - LED class path without /brightness suffix.
** output parameters:   NONE
** Returned value:      1 if writable, else 0.
*********************************************************************************************************/
static int led_path_writable(const char *base)
{
    char path[128];

    if (!base || !base[0])
        return 0;
    (void)snprintf(path, sizeof(path), "%s/brightness", base);
    return access(path, W_OK) == 0;
}

/*********************************************************************************************************
** Function name:       led_write_str
** Descriptions:        Write string to sysfs LED attribute file.
** input parameters:    path    - full sysfs path.
**                      value   - string to write.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void led_write_str(const char *path, const char *value)
{
    FILE *fp;

    if (!path || !value)
        return;
    fp = fopen(path, "w");
    if (!fp)
        return;
    (void)fprintf(fp, "%s\n", value);
    (void)fclose(fp);
}

/*********************************************************************************************************
** Function name:       led_write_int
** Descriptions:        Write integer to sysfs LED attribute file.
** input parameters:    path    - full sysfs path.
**                      value   - integer to write.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void led_write_int(const char *path, int value)
{
    FILE *fp;

    if (!path)
        return;
    fp = fopen(path, "w");
    if (!fp)
        return;
    (void)fprintf(fp, "%d\n", value);
    (void)fclose(fp);
}

/*********************************************************************************************************
** Function name:       led_apply_mode
** Descriptions:        Apply off / blink / solid to one LED channel (skip if path missing).
** input parameters:    ch      - channel with base path and last_mode cache.
**                      mode    - NM4_LED_MODE_*.
** output parameters:    ch      - last_mode updated on change.
** Returned value:      NONE
*********************************************************************************************************/
static void led_apply_mode(nm4_led_channel_t *ch, nm4_led_mode_t mode)
{
    char trigger[160];
    char bright[160];
    char delay_on[160];
    char delay_off[160];

    if (!ch || !ch->base || !led_path_writable(ch->base))
        return;
    if (ch->last_mode == (int)mode)
        return;
    ch->last_mode = (int)mode;

    (void)snprintf(trigger, sizeof(trigger), "%s/trigger", ch->base);
    (void)snprintf(bright, sizeof(bright), "%s/brightness", ch->base);
    (void)snprintf(delay_on, sizeof(delay_on), "%s/delay_on", ch->base);
    (void)snprintf(delay_off, sizeof(delay_off), "%s/delay_off", ch->base);

    if (mode == NM4_LED_MODE_OFF) {
        led_write_str(trigger, "none");
        led_write_int(bright, 0);
        return;
    }
    if (mode == NM4_LED_MODE_SOLID) {
        led_write_str(trigger, "none");
        led_write_int(bright, 1);
        return;
    }
    led_write_str(trigger, "timer");
    led_write_int(delay_on, NM4_LED_BLINK_MS);
    led_write_int(delay_off, NM4_LED_BLINK_MS);
}

/*********************************************************************************************************
** Function name:       iface_led_mode
** Descriptions:        Map live L1/L2 + debounced availability to off/blink/solid.
** input parameters:    ifname  - wlan0 or ppp0.
**                      p       - probe result (if_exists).
**                      avail   - debounced available bit (includes internet_ok).
** output parameters:   NONE
** Returned value:      NM4_LED_MODE_OFF / BLINK / SOLID.
*********************************************************************************************************/
static nm4_led_mode_t iface_led_mode(const char *ifname, const nm4_probe_result_t *p, int avail)
{
    int link_up;
    int ipv4_ok;

    if (!ifname || !ifname[0] || !p || !p->if_exists)
        return NM4_LED_MODE_OFF;

    link_up = nm4_if_link_up(ifname);
    ipv4_ok = link_up && nm4_if_has_ipv4(ifname);
    if (!link_up || !ipv4_ok)
        return NM4_LED_MODE_OFF;
    if (avail)
        return NM4_LED_MODE_SOLID;
    return NM4_LED_MODE_BLINK;
}

/*********************************************************************************************************
** Function name:       nm4_led_update
** Descriptions:        Update wifi/lte LEDs: blink = link+IP, solid = debounced internet_ok.
** input parameters:    in      - wlan/ppp probe snap + availability bits.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_led_update(const nm4_led_input_t *in)
{
    nm4_led_mode_t wifi_m;
    nm4_led_mode_t lte_m;

    if (!in)
        return;

    wifi_m = iface_led_mode("wlan0", in->wlan, in->bits ? in->bits->wlan_up : 0);
    lte_m = iface_led_mode("ppp0", in->ppp, in->bits ? in->bits->ppp_up : 0);

    led_apply_mode(&s_ch_wifi, wifi_m);
    led_apply_mode(&s_ch_lte, lte_m);
}

/*********************************************************************************************************
** Function name:       nm4_led_all_off
** Descriptions:        Turn off wifi and lte LEDs.
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_led_all_off(void)
{
    led_apply_mode(&s_ch_wifi, NM4_LED_MODE_OFF);
    led_apply_mode(&s_ch_lte, NM4_LED_MODE_OFF);
    s_ch_wifi.last_mode = NM4_LED_MODE_OFF;
    s_ch_lte.last_mode = NM4_LED_MODE_OFF;
}
