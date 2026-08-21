#ifndef _IEC101_SEND_H_
#define _IEC101_SEND_H_

#include <stdint.h>
#include <stdbool.h>
#include "iec101-cfg.h"
#include "iec101-data.h"

void SendFrame(struct config_devices_attr *pDevAttr, uint8_t fun, uint8_t *pASDU, uint8_t len);
void SendFrameFix(struct config_devices_attr *pDevAttr, uint8_t fun);
void SendFrameCtrl(struct config_devices_attr *pDevAttr, struct M_CTRL *pCtrl);

#endif
