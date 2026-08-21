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


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "iec101-cfg.h"
#include "iec101-send.h"
#include "iec101-recv.h"
#include "iec101-process.h"
#include "pkt.h"
#include "db_node_fops.h"


/*
    通讯协议驱动使用.so 动态库形式动态加载。 
    每个通讯协议驱动按理只需要port.c即可。只要对外 实现 _create  _destroy  2个函数

    对内实现  iec101_pkt_request_list_create，用于生成和装载发送帧

    如果 iec101_pkt_request_list_create 发送帧有多种实现方式，比如动态和静态：

    1. 如本示例，通过寄存器号和长度生成发送帧
    2. 静态报文。这时候只需要硬编码生成多个数组，调用pkt_request_list_static_append即可，如
       pkt_group [][] = {
           {01, 03, 00 ,00 ,00 ,02 , CRCL, CRCH},
           {01, 03, 00 ,02 ,00 ,02 , CRCL, CRCH},
           {01, 03, 00 ,04 ,00 ,02 , CRCL, CRCH},
           ...
       }
       静态通常用于  只有单台节点，无地址，内容固定的设备。如某ups设备，只发固定字符串
       静态代码实现非常简单


	   注意： 本驱动框架不能有 全局变量 和静态变量。  所有变量需要动态申请，比如 malloc， 请参考lib_create
*/



/*********************************************************************************************************
 ** Function name:		 
** Descriptions:		 本设备请求帧装载
						 所有问答式的协议驱动，均需要调用pkt.c 中的接口，把协议生成的发送帧，append到发送链表队列中pkt_request_list_static_append
						 每调用一次pkt_request_list_static_append，即在本设备中增加发送帧链表节点
** input parameters:			  
** output parameters:	 NONE
** Returned value: 	 
*********************************************************************************************************/
static int32_t iec101_pfnFlowCallback(struct config_devices_info_attr *pDevInfoAttr, struct sampling_com_baseclass_ops_t *pBaseComm, struct sampling_comm_ctx_t  *pcom_ctx)
{
	struct config_devices_attr *pDevCfg = (struct config_devices_attr *)pDevInfoAttr->private;
    struct config_context_attr *pContextAttr = &(pDevCfg->context_attr);
	pContextAttr->pBaseComm = pBaseComm;
	pContextAttr->pcom_ctx = pcom_ctx;

	pcom_ctx->channel_no = pContextAttr->channel_id;
	pcom_ctx->dev_no = pContextAttr->logic_id;

	if (pContextAttr->need_reset)
	{// 复位链路
		if (!ResetLinkState(pDevCfg)) return 1;// 复位远方链路失败
		
		// 设备初始化成功
		pContextAttr->need_reset = false;

		// 总召唤
		pContextAttr->tm_last_call_all = time(NULL);
		Query_AllStart(pDevCfg);
		if (pContextAttr->has_acc) Query_AllAccStart(pDevCfg);
	}
	
	if ((uint32_t)(time(NULL) - pContextAttr->tm_last_call_all) >= 30)
	{// 周期总召唤
		pContextAttr->tm_last_call_all = time(NULL);
		Query_AllStart(pDevCfg);
		if (pContextAttr->has_acc) Query_AllAccStart(pDevCfg);
	}

	// 处理命令
	ProcessCmd(pDevCfg);

	// 读实时数据
	ProcessRtData(pDevCfg);
	printf("flow:%d\n", pDevInfoAttr->logic_id);

	if (pDevCfg->timing_inv > 0 &&
		(uint32_t)(time(NULL) - pContextAttr->tm_last_adjust_time) > pDevCfg->timing_inv * 60)
	{// 校时
		pContextAttr->tm_last_adjust_time = time(NULL);
		AdjustTime(pDevCfg);
	}
	// ret = pBaseComm->write(pcom_ctx, "123", 3);

	return 0;
}



/*********************************************************************************************************
 ** Function name:		 
** Descriptions:		 本驱动初始化
** input parameters:			  
** output parameters:	 NONE
** Returned value: 	 
*********************************************************************************************************/
int32_t lib_create(struct config_devices_info_attr *pDevInfoAttr)
{
	struct config_devices_attr *pDevAttr;

	/*  0. 实时数据库绑定 */
	DataMgmtFops.init(pDevInfoAttr->pVmem);

	/*
		1. 申请当前帧点表属性管理空间
	*/
	pDevAttr = calloc(1, sizeof(struct config_devices_attr));
	if(!pDevAttr)
		return -1;

	/*
		2. 保存当前点表到设备信息管理中
	*/
	pDevInfoAttr->private = pDevAttr;

	pDevInfoAttr->pfnFlowCallback = NULL;

	/*
		3. 从json加载当前设备的配置
	*/
	iec101_cfg_load(pDevAttr, pDevInfoAttr->cfg_file);
    printf("addr_size=%d, cause_size=%d\n", pDevAttr->addr_size, pDevAttr->cause_size);
	pDevAttr->context_attr.phy_id = pDevInfoAttr->phy_id;
	pDevAttr->context_attr.channel_id = pDevInfoAttr->channel_id;
	pDevAttr->context_attr.logic_id = pDevInfoAttr->logic_id;
	pDevAttr->context_attr.timeout_read = 1000;

    pDevAttr->context_attr.tm_last_call_all = 0;
    pDevAttr->context_attr.tm_last_adjust_time = 0;
    pDevAttr->context_attr.failTimes = 0;
    pDevAttr->context_attr.need_reset = true;
    pDevAttr->context_attr.fcb_flag = false;
    pDevAttr->context_attr.commStatus = eCommStatus_Unknown;

	pDevAttr->context_attr.has_acc = false;
	for (int i = 0; i < pDevAttr->node_num; i++)
	{
		if (pDevAttr->node_attr[i].datattr != 2) continue;
    	pDevAttr->context_attr.has_acc = true;
		break;
	}

	/*
		4. 根据配置生成发送帧，并压入本设备发送帧队列
           由于每个协议可能存在多个设备，这里通过动态申请内存，实现多个设备公用一个库
	*/
	// iec101_pkt_request_list_create(pDevInfoAttr, pDevAttr);
	pkt_flow_callback_append(pDevInfoAttr, iec101_pfnFlowCallback, NULL);

	return 0;
}



/*********************************************************************************************************
 ** Function name:		 
** Descriptions:		 本驱动初始化
** input parameters:			  
** output parameters:	 NONE
** Returned value: 	 
*********************************************************************************************************/
void lib_destroy(struct config_devices_info_attr *pDevInfoAttr)
{
	/* 释放请求报文申请的内存 */
	pkt_request_list_all_destory(pDevInfoAttr);

	/* 释放点表申请的内存 */
	if(pDevInfoAttr->private)
		free(pDevInfoAttr->private);
}
