#ifndef _DLT698_45_PROCESS_H_
#define _DLT698_45_PROCESS_H_

#include <pthread.h>
#include "sampling_cfg.h"

int32_t fnReceiveCallback(struct config_devices_info_attr *pDevInfoAttr, struct config_request_list *prequest_list);

#endif
