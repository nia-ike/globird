/*
 * File      : config_devices.c
 * This file is part of xgate5-fire
 * COPYRIGHT (C) 2006 - 2018, 19 development, Communication team, chenzy.
 *
 * 
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-09     chenzy       channel-device-node setting.
 */
#ifndef __CJT188_PROCESS_H__
#define __CJT188_PROCESS_H__


#include <stdint.h>
#include <stdio.h>
#include "sampling_cfg.h"





int32_t fnReceiveCallback(struct config_devices_info_attr *pDevInfoAttr, struct config_request_list *prequest_list);


#endif



