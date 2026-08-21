#ifndef __IEC101_RECV_H__
#define __IEC101_RECV_H__


#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "sampling_cfg.h"
#include "iec101-cfg.h"
#include "iec101-data.h"

// 接收并解析数据，返回接收报文是否正确
bool RecvAndExplainFrame(struct config_devices_attr *pDevAttr, struct S_CTRL *pCtrl);

#endif
