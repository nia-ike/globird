/*************************************************************************/
/** adapter_ppp.c — pppd + chat peer，幂等启动                                   **/
/*************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "netmgr_v4_adapter.h"
#include "adapter_internal.h"

/*********************************************************************************************************
** Function name:       nm4_adapter_ppp_start
** Descriptions:        Idempotent PPP start: if pppd already running for ifname, return success;
**                      otherwise generate chat/peer files and launch pppd in background.
** input parameters:    p       - PPP parameters (ifname, device, apn, user, password, dial).
** output parameters:   NONE
** Returned value:      0 on success or already running, -1 on invalid args or spawn failure.
*********************************************************************************************************/
int nm4_adapter_ppp_start(const nm4_ppp_params_t *p)
{
    char peer_path[160];
    char chat_path[160];
    char chat_cmd[4096];
    char peer_cmd[4096];
    char run[512];
    const char *dial;
    const char *apn;

    if (!p || p->ifname[0] == '\0' || p->device[0] == '\0') {
        fprintf(stderr, "adapter_ppp: need -i ppp0 -D /dev/ttyUSB0\n");
        return -1;
    }

    if (nm4_pppd_running(p->ifname))
        return 0;

    apn = (p->apn[0] ? p->apn : "internet");
    dial = (p->dial[0] ? p->dial : "*99#");

    snprintf(peer_path, sizeof(peer_path), "/var/run/ppp/peer/%s", p->ifname);
    snprintf(chat_path, sizeof(chat_path), "/var/run/ppp/peer/%s-chat", p->ifname);

    (void)nm4_run_cmd("mkdir -p /var/run/ppp/peer");

    snprintf(chat_cmd, sizeof(chat_cmd),
             "echo \"ABORT    'DELAYED'\" > %s && "
             "echo \"ABORT    'BUSY'\" >> %s && "
             "echo \"ABORT    'ERROR'\" >> %s && "
             "echo \"ABORT    'NO DIALTONE'\" >> %s && "
             "echo \"ABORT    'NO CARRIER'\" >> %s && "
             "echo \"''       \\\\d+++\\\\d\" >> %s && "
             "echo \"''       AT\" >> %s && "
             "echo \"OK       ATE0V1\" >> %s && "
             "echo \"OK       ATH\" >> %s && "
             "echo \"TIMEOUT  10\" >> %s && "
             "echo \"OK       AT+CGDCONT=1,\\\"IP\\\",\\\"%s\\\"\" >> %s && "
             "echo \"TIMEOUT  10\" >> %s && "
             "echo \"OK       ATDT%s\" >> %s && "
             "echo \"CONNECT  \\\"\\\"\" >> %s",
             chat_path, chat_path, chat_path, chat_path, chat_path,
             chat_path, chat_path, chat_path, chat_path, chat_path,
             apn, chat_path, chat_path, dial, chat_path, chat_path);
    if (nm4_run_cmd(chat_cmd) != 0) {
        fprintf(stderr, "adapter_ppp: generate chat script failed\n");
        return -1;
    }

    snprintf(peer_cmd, sizeof(peer_cmd),
             "echo '%s' > %s && "
             "echo '115200' >> %s && "
             "echo 'nocrtscts' >> %s && "
             "echo 'nodetach' >> %s && "
             "echo 'usepeerdns' >> %s && "
             "echo 'noipdefault' >> %s && "
             "echo 'nodefaultroute' >> %s && "
             "echo 'user \"%s\"' >> %s && "
             "echo 'password \"%s\"' >> %s && "
             "echo '0.0.0.0:0.0.0.0' >> %s && "
             "echo 'lcp-echo-failure 3' >> %s && "
             "echo 'lcp-echo-interval 120' >> %s && "
             "echo 'connect \"/usr/sbin/chat -s -v -f %s\"' >> %s",
             p->device,
             peer_path, peer_path, peer_path, peer_path, peer_path, peer_path, peer_path,
             p->user, peer_path, p->password, peer_path,
             peer_path, peer_path, peer_path,
             chat_path, peer_path);
    if (nm4_run_cmd(peer_cmd) != 0) {
        fprintf(stderr, "adapter_ppp: generate peer failed\n");
        return -1;
    }

    snprintf(run, sizeof(run),
             "pppd file %s linkname %s >/dev/null 2>&1 &",
             peer_path, p->ifname);
    if (nm4_run_cmd(run) != 0) {
        fprintf(stderr, "adapter_ppp: pppd start failed\n");
        return -1;
    }

    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_adapter_ppp_stop
** Descriptions:        Stop pppd processes matching the given PPP interface linkname.
** input parameters:    ifname  - PPP interface name (e.g. ppp0).
** output parameters:   NONE
** Returned value:      Always 0 after attempting kill script.
*********************************************************************************************************/
int nm4_adapter_ppp_stop(const char *ifname)
{
    char cmd[384];

    if (!ifname || !ifname[0]) {
        fprintf(stderr, "adapter_ppp: missing ifname\n");
        return -1;
    }

    snprintf(cmd, sizeof(cmd),
             "kill $(ps 2>/dev/null | grep '[p]ppd' | grep -F '%s' | grep -v grep | awk '{print $1}') 2>/dev/null",
             ifname);
    (void)nm4_run_cmd(cmd);
    return 0;
}
