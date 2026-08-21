/*
 * File      : config_devices.c
 * This file is part of ech3200
 * COPYRIGHT (C) 2006 - 2018, 19, Communication team, chenzy.
 *
 * 
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-09     chenzy       channel-device-node setting.
 */


#ifndef __PKT_H__
#define __PKT_H__

#include <stdint.h>

#include "sampling_cfg.h"



typedef int32_t (*pfnPktCallback_t)(struct config_devices_info_attr *, struct config_request_list *);




int pkt_request_list_static_append(struct config_devices_info_attr *pDevInfoAttr, uint8_t *pbuffer, uint16_t len, pfnPktCallback_t pfnReceiveCallback, pfnPktCallback_t fnBeforeSendCallback, void *parameter);
int pkt_request_list_dynamic_append(struct config_devices_info_attr *pDevInfoAttr, uint16_t len, pfnPktCallback_t pfnReceiveCallback, void *parameter);
int pkt_flow_callback_append(struct config_devices_info_attr *pDevInfoAttr, pfnFlowCallback_t pfnFlowCallback, void *parameter);

void pkt_request_list_all_destory(struct config_devices_info_attr *pDevInfoAttr);







#endif
