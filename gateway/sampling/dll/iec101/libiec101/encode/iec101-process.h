#ifndef __IEC101_PROCESS_H__
#define __IEC101_PROCESS_H__

#include <stdint.h>
#include <stdio.h>
#include "sampling_cfg.h"
#include "iec101-cfg.h"

void ProcessRtData(struct config_devices_attr *pDevAttr);// 处理实时数据
void ProcessCmd(struct config_devices_attr *pDevAttr);// 处理命令
bool ResetLinkState(struct config_devices_attr *pDevAttr);// 复位链路
void Query_AllStart(struct config_devices_attr *pDevAttr);// 总召
void Query_AllAccStart(struct config_devices_attr *pDevAttr);// 电度总召
void AdjustTime(struct config_devices_attr *pDevAttr);// 校时

#endif
