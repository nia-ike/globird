#include <stdint.h>
#include <pthread.h>
#include "sampling_cfg.h"
#include "db_node_fops.h"
#include "dlt698_45_cfg.h"
#include "dlt698_45.h"

int32_t fnReceiveCallback(struct config_devices_info_attr *pDevInfoAttr, struct config_request_list *prequest_list)
{
    int i, nodeSize;
    uint32_t recv_oad = 0;
    double fVal = 0;
    struct config_devices_attr *pDevAttr;
    struct config_node_attr *pNodeAttr;
    struct db_spl_remap_info_t *pIndexInfo;

    pDevAttr = (struct config_devices_attr *)pDevInfoAttr->private;
    if (!pDevAttr) return -1;

    if (dlt698_45_codec.UnPacket91H(prequest_list->precv_buff, prequest_list->recv_len, &recv_oad, &fVal) < 0) {
        return -1;
    }

    pIndexInfo = DataMgmtFops.getIndexByPortAndDev(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id);
    if (!pIndexInfo) return -1;

    nodeSize = dlt698_45_cfg_node_attr_size(pDevAttr);
    for (i = 0; i < nodeSize; i++) {
        pNodeAttr = dlt698_45_cfg_node_attr(pDevAttr, i);
        if (!pNodeAttr) continue;
        if (pNodeAttr->reg_no == recv_oad) {
            fVal = fVal * (pNodeAttr->kfactor * pNodeAttr->pt_flag1 * pNodeAttr->ct_flag2);
            DataMgmtFops.writepoint_in1dev(pIndexInfo, i, &fVal);
            return 0;
        }
    }
    return -1;
}
