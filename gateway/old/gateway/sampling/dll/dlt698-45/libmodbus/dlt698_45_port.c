#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "dlt698_45_cfg.h"
#include "dlt698_45.h"
#include "dlt698_45_data.h"
#include "dlt698_45_process.h"
#include "pkt.h"
#include "db_node_fops.h"

static void dlt698_45_pkt_request_list_create(struct config_devices_info_attr *pDevInfoAttr, struct config_devices_attr *pDevAttr)
{
    int i, nsend;
    uint8_t npkt;
    uint8_t buff[256];
    uint64_t address;

    npkt = (pDevAttr->node_num > CFG_MAX_POINT) ? CFG_MAX_POINT : pDevAttr->node_num;
    address = pDevInfoAttr->phy_id ? pDevInfoAttr->phy_id : DLT698_45_BROADCAST_PHYADDR;

    for (i = 0; i < npkt; i++) {
        if (pDevAttr->node_attr[i].reg_no == 0) {
            continue;
        }
        nsend = dlt698_45_codec.Packet11H(buff, pDevAttr->node_attr[i].leadnum, address, pDevAttr->node_attr[i].reg_no);
        if (nsend > 0) {
            pkt_request_list_static_append(pDevInfoAttr, buff, nsend, fnReceiveCallback, NULL, &pDevAttr->node_attr[i]);
        }
    }
}

int32_t lib_create(struct config_devices_info_attr *pDevInfoAttr)
{
    struct config_devices_attr *pDevAttr;

    DataMgmtFops.init(pDevInfoAttr->pVmem);
    pDevAttr = calloc(1, sizeof(struct config_devices_attr));
    if (!pDevAttr) return -1;

    pDevInfoAttr->private = pDevAttr;
    pDevInfoAttr->pfnFlowCallback = NULL;

    if (dlt698_45_cfg_load(pDevAttr, pDevInfoAttr->cfg_file) < 0) {
        free(pDevAttr);
        pDevInfoAttr->private = NULL;
        return -1;
    }

    dlt698_45_pkt_request_list_create(pDevInfoAttr, pDevAttr);
    return 0;
}

void lib_destroy(struct config_devices_info_attr *pDevInfoAttr)
{
    pkt_request_list_all_destory(pDevInfoAttr);
    if (pDevInfoAttr->private) {
        free(pDevInfoAttr->private);
        pDevInfoAttr->private = NULL;
    }
}

int32_t lib_control(struct config_devices_info_attr *pDevInfoAttr, void *pargs, void *pbuffer, uint32_t len)
{
    (void)pDevInfoAttr;
    (void)pargs;
    (void)pbuffer;
    (void)len;
    return -1;
}
