/*
 * File      : config_devices.c
 * This file is part of ech3200
 * COPYRIGHT (C) 2006 - 2018, 19, Communication team, chenzy.
 *
 * 
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-09     chenzy      
 */
#include <string.h>
#include <stdlib.h>

#include "pkt.h"






/*********************************************************************************************************
 ** Function name:		 
** Descriptions:		 把pbuffer的请求帧内容挂载到本设备下的 帧发送链表后面节点 
						 问答式请求帧，在主sampling_channelpoll 里轮询 dev 中的 config_request_list
						 本函数用于挂载发送帧到  帧队列链表， sampling_channelpoll 即自动轮询

						 pkt_request_list_static_append 是静态帧轮训， 即直接把生成的（或者用户自定义配置的）固定报文
						 装载到队列中。 主程序即轮询队列，发送静态帧。

						 如果需要对发送帧在发送时 生成，或者需要修改，需要使用 pkt_request_list_dynamic_append
** input parameters:	 config_devices_info_attr 	该设备属性结构
						 pbuffer					发送帧地址
						 len						发送帧长度
						 pfnReceiveCallback			接收处理函数。 对应本请求帧 得到的回应帧 回调处理函数。
						 parameter					参数传递。    请求帧 传递到 消息处理帧时，需要传递参数时，采用本参数。 不同规约 可自行实现不同的 参数结构体    		  
** output parameters:	 NONE
** Returned value: 	 
*********************************************************************************************************/
int pkt_request_list_static_append(struct config_devices_info_attr *pDevInfoAttr, uint8_t *pbuffer, uint16_t len, pfnPktCallback_t pfnReceiveCallback, pfnPktCallback_t pfnBeforeSendCallback, void *parameter)
{
	struct config_request_list *pkt_request_list;

	/*
		申请帧链表节点空间
	*/
	pkt_request_list = calloc(1, sizeof(struct config_request_list));
	if(!pkt_request_list)
		return -1;

	/*
		申请发送帧空间
	*/
	pkt_request_list->pxmit_buff = malloc(len);
	if(!pkt_request_list->pxmit_buff)
		goto __error;
	
	/* 拷贝 pbuffer中的数据到发送帧 pxmit_buff */
	memcpy(pkt_request_list->pxmit_buff, pbuffer, len);

	/* 指示发送帧长度 */
	pkt_request_list->xmit_len = len;

	/* 本帧ID */
	pkt_request_list->phy_id = pDevInfoAttr->phy_id;
	pkt_request_list->logic_id = pDevInfoAttr->logic_id;
	pkt_request_list->channel_id = pDevInfoAttr->channel_id;

	/* 装载该协议私有参数 */
	pkt_request_list->private = parameter;

	/* 静态 */
	pkt_request_list->is_dynamic = false;

	/* 
		装载帧的处理回调函数 
	*/
	pkt_request_list->pfnReceiveCallback = pfnReceiveCallback;
	pkt_request_list->pfnBeforeSendCallback = pfnBeforeSendCallback;

	/* 把当前帧链表节点挂载到 当前设备的发送帧链表后面 */
	slist_append(&pDevInfoAttr->pkt_request_list_head.slist, &pkt_request_list->slist);

	return 0;

__error:
	free(pkt_request_list);
	return -1;
}



/*********************************************************************************************************
 ** Function name:		 
** Descriptions:		 把pbuffer的请求帧内容挂载到本设备下的 帧发送链表后面节点 
						 问答式请求帧，在主sampling_channelpoll 里轮询 dev 中的 config_request_list
						 本函数用于挂载发送帧到  帧队列链表， sampling_channelpoll 即自动轮询

						 pkt_request_list_dynamic_append 是动态帧轮训， 本函数不装载固定报文，因此 is_dynamic = true 
						 只申请发送报文空间，不生成报文。在回调中生成后再						 
						

						 如果需要对发送帧在发送时 生成，或者需要修改，需要使用 pkt_request_list_dynamic_append
** input parameters:			  
** output parameters:	 NONE
** Returned value: 	 
*********************************************************************************************************/
int pkt_request_list_dynamic_append(struct config_devices_info_attr *pDevInfoAttr, uint16_t len, pfnPktCallback_t pfnReceiveCallback, void *parameter)
{
	struct config_request_list *pkt_request_list;

	/*
		申请帧链表节点空间
	*/
	pkt_request_list = calloc(1, sizeof(struct config_request_list));
	if(!pkt_request_list)
		return -1;

	/*
		申请发送帧空间
	*/
	pkt_request_list->pxmit_buff = malloc(len);
	if(!pkt_request_list->pxmit_buff)
		goto __error;

	/* 指示发送帧长度 */
	pkt_request_list->xmit_len = len;

	/* 本帧ID */
	pkt_request_list->phy_id = pDevInfoAttr->phy_id;
	pkt_request_list->logic_id = pDevInfoAttr->logic_id;
	pkt_request_list->channel_id = pDevInfoAttr->channel_id;

	/* 装载该协议私有参数 */
	pkt_request_list->private = parameter;

	/* 动态 */
	pkt_request_list->is_dynamic = true;

	/* 
		装载帧的处理回调函数 
	*/
	pkt_request_list->pfnReceiveCallback = pfnReceiveCallback;

	/* 把当前帧链表节点挂载到 当前设备的发送帧链表后面 */
	slist_append(&pDevInfoAttr->pkt_request_list_head.slist, &pkt_request_list->slist);

	return 0;

__error:
	free(pkt_request_list);
	return -1;
}



/*********************************************************************************************************
 ** Function name:		 
** Descriptions:		 用于一些如IEC103之类，具有复杂的多态报文请求流程，框架中一问一答的报文收发无法满足，需要使用 本函数装载过程回调函数
						 数据请求和回应的流程由协议驱动内部的 pfnFlowCallback 指向的函数进行全权处理，直接跳过channelpoll的一问一答流程

** input parameters:	 config_devices_info_attr 	该设备属性结构
						 pfnFlowCallback			数据流程处理函数。 
						 parameter					参数传递。    请求帧 传递到 消息处理帧时，需要传递参数时，采用本参数。 不同规约 可自行实现不同的 参数结构体    		  
** output parameters:	 NONE
** Returned value: 	 
*********************************************************************************************************/
int pkt_flow_callback_append(struct config_devices_info_attr *pDevInfoAttr, pfnFlowCallback_t pfnFlowCallback, void *parameter)
{
	pDevInfoAttr->pfnFlowCallback = pfnFlowCallback;

	return 0;
}

/*********************************************************************************************************
 ** Function name:		 
** Descriptions:		  未完成，请勿调用。或完善后再调用
** input parameters:			  
** output parameters:	 NONE
** Returned value: 	 
*********************************************************************************************************/
void pkt_request_list_all_destory(struct config_devices_info_attr *pDevInfoAttr)
{
	struct config_request_list *pCurRequestlist, tmpCurRequestlist;

	slist_for_each_entry(pCurRequestlist, &pDevInfoAttr->pkt_request_list_head.slist, slist)
	{
		/* 备份节点，接下来会删掉该节点内存。 */
		tmpCurRequestlist = *pCurRequestlist;

		/* 释放发送buffer 缓存 */
		if(pCurRequestlist->pxmit_buff)
			free(pCurRequestlist->pxmit_buff);

		/* 释放config_request_list缓存 */
		free(pCurRequestlist);

		/* 指向临时备份节点，用于循环找下一个节点 */
		pCurRequestlist = &tmpCurRequestlist;

		/**/
	}
}
