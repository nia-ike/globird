/*
 * File      : custom_process.h
 * Descriptions: customProto 收包处理接口
 */
#ifndef __CUSTOM_PROCESS_H__
#define __CUSTOM_PROCESS_H__

#include "sampling_cfg.h"

/*********************************************************************************************************
 ** Function name:		 fnReceiveCallback
** Descriptions:		 解包回调：可选帧匹配 + 按帧绝对偏移解析测点写库
** input parameters:	 pDevInfoAttr, prequest_list
** Returned value: 	 0 正常；-1 失败
*********************************************************************************************************/
int32_t fnReceiveCallback(struct config_devices_info_attr *pDevInfoAttr,
			  struct config_request_list *prequest_list);

#endif
