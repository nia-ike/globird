#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include "netmgr_v4_policy.h"
#include "netmgr_v4_status.h"
#include "adapter_internal.h"

/*********************************************************************************************************
** Function name:       yn
** Descriptions:        Map integer boolean to JSON keyword "true" or "false".
** input parameters:    v       - zero for false, non-zero for true.
** output parameters:   NONE
** Returned value:      Pointer to static string "true" or "false".
*********************************************************************************************************/
static const char *yn(int v) { return v ? "true" : "false"; }

/*********************************************************************************************************
** Function name:       nm4_status_write
** Descriptions:        Atomically write manager snapshot to NM4_STATUS_JSON (tmp + rename).
** input parameters:    s       - snapshot with bits, probes, want/have, linkkeeper stats.
** output parameters:   NONE
** Returned value:      0 on success, -1 on invalid argument or I/O error.
*********************************************************************************************************/
int nm4_status_write(const nm4_status_snapshot_t *s)
{
    char tmp[128];
    char cmd[256];
    FILE *fp;
    time_t sec;
    struct tm tmv;
    char ts[32];

    if (!s || !s->bits || !s->eth || !s->wlan || !s->ppp || !s->lk)
        return -1;

    (void)snprintf(cmd, sizeof(cmd), "mkdir -p %s", NM4_STATUS_DIR);
    (void)nm4_run_cmd(cmd);

    (void)snprintf(tmp, sizeof(tmp), "%s.tmp", NM4_STATUS_JSON);
    fp = fopen(tmp, "w");
    if (!fp) return -1;

    sec = (time_t)(s->now_ms / 1000ULL);
    localtime_r(&sec, &tmv);
    (void)strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tmv);

    (void)fprintf(fp, "{\n");
    (void)fprintf(fp, "  \"time\": \"%s\",\n", ts);
    (void)fprintf(fp, "  \"tick\": %" PRIu64 ",\n", s->tick);
    (void)fprintf(fp, "  \"r\": %u,\n", (unsigned)s->r);
    (void)fprintf(fp, "  \"bits\": {\"eth\": %d, \"wlan\": %d, \"ppp\": %d},\n",
                  s->bits->eth_up, s->bits->wlan_up, s->bits->ppp_up);
    (void)fprintf(fp, "  \"want\": \"%s\",\n", nm4_policy_want_name(s->want));
    (void)fprintf(fp, "  \"have\": \"%s\",\n", nm4_policy_want_name(s->have));
    (void)fprintf(fp, "  \"pending\": %s,\n", yn(s->pending));
    (void)fprintf(fp, "  \"interfaces\": {\n");
    (void)fprintf(fp, "    \"eth0\": {\"exists\": %s, \"link\": %s, \"ipv4\": %s, \"inet\": %s},\n",
                  yn(s->eth->if_exists), yn(s->eth->link_up), yn(s->eth->ipv4_ok), yn(s->eth->internet_ok));
    (void)fprintf(fp, "    \"wlan0\": {\"exists\": %s, \"link\": %s, \"ipv4\": %s, \"inet\": %s, \"lk_ok\": %" PRIu64 ", \"lk_fail\": %" PRIu64 "},\n",
                  yn(s->wlan->if_exists), yn(s->wlan->link_up), yn(s->wlan->ipv4_ok), yn(s->wlan->internet_ok),
                  s->lk->wifi_ok_cnt, s->lk->wifi_fail_cnt);
    (void)fprintf(fp, "    \"ppp0\": {\"exists\": %s, \"link\": %s, \"ipv4\": %s, \"inet\": %s, \"lk_ok\": %" PRIu64 ", \"lk_fail\": %" PRIu64 "}\n",
                  yn(s->ppp->if_exists), yn(s->ppp->link_up), yn(s->ppp->ipv4_ok), yn(s->ppp->internet_ok),
                  s->lk->ppp_ok_cnt, s->lk->ppp_fail_cnt);
    (void)fprintf(fp, "  }\n");
    (void)fprintf(fp, "}\n");

    if (fclose(fp) != 0) return -1;
    if (rename(tmp, NM4_STATUS_JSON) != 0) return -1;
    return 0;
}
