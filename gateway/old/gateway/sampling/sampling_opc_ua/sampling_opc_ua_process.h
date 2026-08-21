#ifndef _SAMPLING_OPC_UA_PROCESS_H_
#define _SAMPLING_OPC_UA_PROCESS_H_

#include <open62541/client.h>
#include "sampling_cfg.h"
#include "db_struct.h"

struct opc_ua_mgr_t
{
    struct config_channel_attr          ChannelAttr;
    UA_Client                          *client;
    int32_t                             ChannelNo;
    struct db_spl_realtime_data_tbl_t  *pVmem;
    struct sampling_optarg_t           *poptarg;
    bool                                connected;
};

int opc_ua_channel_process(struct opc_ua_mgr_t *pMgr);
void opc_ua_channel_start(struct db_spl_realtime_data_tbl_t *pRTDb, struct opc_ua_mgr_t *pMgr);

#endif
