/*
 * File      : config_devices.h
 * This file is part of xgate5-fire
 * COPYRIGHT (C) 2006 - 2018, 19 development, Communication team, chenzy.
 *
 * 
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-04-19     chenzy       .
 */


#ifndef __CONFIG_DEVICES_H__
#define __CONFIG_DEVICES_H__

#include <stdint.h>

#include "sampling_cfg.h"
#include "sampling_cs104_process.h"


int config_channel_init(struct cs104_mgr_t *pcs104Mgr, char *file);
int mb_get_nodeattr_by_infaddr(struct config_node_attr index[], uint16_t inf_addr, int low, int high);


#endif


