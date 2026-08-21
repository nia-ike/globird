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

#include "cjt188_cfg.h"
#include "cjt188.h"
#include "cjt188_process.h"
#include "pkt.h"
#include "db_node_fops.h"


/*
    通讯协议驱动使用.so 动态库形式动态加载。 
    每个通讯协议驱动按理只需要port.c即可。只要对外 实现 _create  _destroy  2个函数

    对内实现  modbus_pkt_request_list_create，用于生成和装载发送帧

    如果 modbus_pkt_request_list_create 发送帧有多种实现方式，比如动态和静态：

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
static void cjt188_pkt_request_list_create(struct config_devices_info_attr *pDevInfoAttr, struct config_devices_attr *pDevAttr)
{
	uint8_t	npkt;
	int i;
	uint8_t function;
	uint8_t buff[256];
	int nsend;

	/* do not larger than CONFIG_MAX_REQUEST_PKT_LEN */
	npkt = (pDevAttr->node_num > CFG_MAX_POINT) ? CFG_MAX_POINT : pDevAttr->node_num;
	for(i=0; i<npkt; i++)
	{
		switch (pDevAttr->node_attr[i].code)
		{
			case _FC_CJT188_READ_DATA:
				function = atoi(pDevAttr->function);
				nsend = mb_encode_decode.Packet01H(buff, pDevAttr->node_attr[i].leadnum, pDevInfoAttr->phy_id, pDevAttr->node_attr[i].reg_no, function);
			if(nsend > 0)
				pkt_request_list_static_append(pDevInfoAttr, buff, nsend, fnReceiveCallback, NULL, &pDevAttr->node_attr[i]);
			break;		
		default:
			break;
		}
	}
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
	cjt188_cfg_load(pDevAttr, pDevInfoAttr->cfg_file);



	/*
		4. 根据配置生成发送帧，并压入本设备发送帧队列
           由于每个协议可能存在多个设备，这里通过动态申请内存，实现多个设备公用一个库
	*/
	cjt188_pkt_request_list_create(pDevInfoAttr, pDevAttr);

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
